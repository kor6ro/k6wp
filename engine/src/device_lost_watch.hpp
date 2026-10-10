#pragma once

// DXGI adapter-removed watch (E-04, audit remediation).
//
// OnDeviceLost() was previously only ever fired by the QA simulator
// (--simulate-device-lost-after-ms); a real GPU TDR / adapter removal left the
// wallpaper black with no recovery. This small RAII unit arms
// IDXGIFactory7::RegisterAdaptersChangedEvent, whose event handle the engine
// joins into its message-loop wait array. When the event signals, the engine
// reuses the existing device-lost path (OnDeviceLost -> flag ->
// RecreateDevice). windows.h lives in the .cpp only (the os_wallpaper split),
// so this header exposes std types + opaque void* handles. Every COM failure
// path logs through the ctor LogFn and leaves the watch inactive — Start()
// never throws, and an unavailable watch just means the engine runs without
// adapter-removed detection.

namespace k6wp {

class DeviceLostWatch {
 public:
  // Matches EngineApp::Log (void(const char* fmt, ...)); may be null.
  using LogFn = void (*)(const char* fmt, ...);

  explicit DeviceLostWatch(LogFn log = nullptr) : log_(log) {}
  ~DeviceLostWatch();

  DeviceLostWatch(const DeviceLostWatch&) = delete;
  DeviceLostWatch& operator=(const DeviceLostWatch&) = delete;

  // Creates the DXGI factory, opens the (manual-reset) event and registers it.
  // Returns true only when the registration is live; false (with a logged
  // reason) on any failure, leaving the object inactive. Idempotent: a second
  // call while active is a no-op returning true.
  bool Start();

  // Unregisters, closes the event and releases the factory. Idempotent; safe
  // on a never-started object and from the destructor.
  void Stop();

  // The registered event handle (HANDLE) to add to the loop wait array, or
  // nullptr when not started. The engine resets it (manual-reset) after
  // handling.
  void* EventHandle() const { return event_; }

  bool active() const { return active_; }

 private:
  LogFn log_ = nullptr;
  void* factory_ = nullptr;  // IDXGIFactory7* (COM, one AddRef held)
  void* event_ = nullptr;    // HANDLE from CreateEventW
  unsigned long cookie_ = 0; // RegisterAdaptersChangedEvent cookie
  bool active_ = false;
};

}  // namespace k6wp
