// DXGI adapter-removed watch (E-04). See device_lost_watch.hpp.
//
// IDXGIFactory7::RegisterAdaptersChangedEvent signature verified against the
// installed Windows SDK (10.0.26100.0, shared/dxgi1_6.h):
//   HRESULT RegisterAdaptersChangedEvent(HANDLE hEvent, DWORD* pdwCookie);
//   HRESULT UnregisterAdaptersChangedEvent(DWORD dwCookie);
// Raw COM (QueryInterface/Release) keeps the dependency to dxgi only — the
// engine already links dxgi — no WRL header pulled into the build.

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <dxgi1_6.h>  // IDXGIFactory7 (+ IDXGIFactory1 / CreateDXGIFactory1 via dxgi.h)

#include "device_lost_watch.hpp"

namespace k6wp {

namespace {

// Creates a fresh IDXGIFactory7. Returns nullptr on failure.
IDXGIFactory7* CreateFactory7() {
  IDXGIFactory1* factory1 = nullptr;
  HRESULT hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1),
                                  reinterpret_cast<void**>(&factory1));
  if (FAILED(hr) || factory1 == nullptr) return nullptr;
  IDXGIFactory7* factory7 = nullptr;
  hr = factory1->QueryInterface(__uuidof(IDXGIFactory7),
                                reinterpret_cast<void**>(&factory7));
  factory1->Release();
  if (FAILED(hr) || factory7 == nullptr) return nullptr;
  return factory7;
}

}  // namespace

DeviceLostWatch::~DeviceLostWatch() { Stop(); }

bool DeviceLostWatch::Start() {
  if (active_) return true;

  IDXGIFactory1* factory1 = nullptr;
  // __uuidof (not the IID_* symbols): the GUIDs are compile-time constants on
  // the interface types, so no dxguid.lib link dependency — same pattern as
  // gpu_pin.cpp's CreateDXGIFactory1(__uuidof(IDXGIFactory1), ...).
  HRESULT hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1),
                                  reinterpret_cast<void**>(&factory1));
  if (FAILED(hr) || factory1 == nullptr) {
    if (log_) {
      log_("device-lost: CreateDXGIFactory1 failed (hr=0x%08lX), adapter watch unavailable",
           static_cast<unsigned long>(hr));
    }
    return false;
  }

  // IDXGIFactory7 is Windows 10 1803+; on an older OS the QueryInterface fails
  // and the watch stays inactive (logged). Release factory1 either way — the
  // QI'd factory7 holds its own reference.
  IDXGIFactory7* factory7 = nullptr;
  hr = factory1->QueryInterface(__uuidof(IDXGIFactory7),
                                reinterpret_cast<void**>(&factory7));
  factory1->Release();
  if (FAILED(hr) || factory7 == nullptr) {
    if (log_) {
      log_("device-lost: IDXGIFactory7 unavailable (hr=0x%08lX), adapter watch disabled",
           static_cast<unsigned long>(hr));
    }
    return false;
  }

  // Manual-reset event (the SDK/engine expectation): the loop observes the
  // signal, handles it, then resets it to re-arm. An auto-reset event would
  // also work, but manual-reset lets a burst of adapter changes coalesce into
  // one recovery instead of one wake per change.
  HANDLE event = CreateEventW(nullptr, TRUE, FALSE, nullptr);
  if (event == nullptr) {
    if (log_) {
      log_("device-lost: CreateEventW failed (error %lu), adapter watch disabled",
           GetLastError());
    }
    factory7->Release();
    return false;
  }

  DWORD cookie = 0;
  hr = factory7->RegisterAdaptersChangedEvent(event, &cookie);
  if (FAILED(hr)) {
    if (log_) {
      log_("device-lost: RegisterAdaptersChangedEvent failed (hr=0x%08lX), adapter watch disabled",
           static_cast<unsigned long>(hr));
    }
    CloseHandle(event);
    factory7->Release();
    return false;
  }

  factory_ = factory7;
  event_ = event;
  cookie_ = cookie;
  active_ = true;
  return true;
}

bool DeviceLostWatch::Rearm() {
  if (event_ == nullptr) return false;
  // Drop the old registration first: the factory that carried it is about to
  // be released, and an un-registered event is what the engine's wait array
  // expects.
  if (factory_ != nullptr && cookie_ != 0) {
    static_cast<IDXGIFactory7*>(factory_)->UnregisterAdaptersChangedEvent(cookie_);
    cookie_ = 0;
  }
  if (factory_ != nullptr) {
    static_cast<IDXGIFactory7*>(factory_)->Release();
    factory_ = nullptr;
  }
  // A FRESH factory: the old one is no longer current after the adapter it
  // watched disappeared, and a factory that is not current delivers no
  // further events.
  IDXGIFactory7* fresh = CreateFactory7();
  if (fresh == nullptr) {
    if (log_) {
      log_("device-lost: the adapter watch could not be re-armed (no fresh "
           "DXGI factory); adapter-removed detection is off until the next "
           "engine start");
    }
    return false;
  }
  DWORD cookie = 0;
  const HRESULT hr = fresh->RegisterAdaptersChangedEvent(
      static_cast<HANDLE>(event_), &cookie);
  if (FAILED(hr)) {
    fresh->Release();
    if (log_) {
      log_("device-lost: the adapter watch could not be re-armed (hr=0x%08lX); "
           "adapter-removed detection is off until the next engine start",
           static_cast<unsigned long>(hr));
    }
    return false;
  }
  factory_ = fresh;
  cookie_ = cookie;
  if (log_) {
    log_("device-lost: adapter-removed watch re-armed on a fresh DXGI factory");
  }
  return true;
}

void DeviceLostWatch::Stop() {
  if (!active_) return;
  auto* factory = static_cast<IDXGIFactory7*>(factory_);
  // Unregister before releasing the factory/event so the registration can
  // never outlive the handles it references.
  if (factory != nullptr && cookie_ != 0) {
    factory->UnregisterAdaptersChangedEvent(cookie_);
  }
  if (factory != nullptr) factory->Release();
  if (event_ != nullptr) CloseHandle(static_cast<HANDLE>(event_));
  factory_ = nullptr;
  event_ = nullptr;
  cookie_ = 0;
  active_ = false;
}

}  // namespace k6wp
