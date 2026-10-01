// iGPU/dGPU adapter pick (P3L.3). See gpu_pin.hpp.

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <dxgi1_6.h>

#include <string>
#include <vector>

#include "gpu_pin.hpp"

namespace k6wp {
namespace {

std::string Narrow(const wchar_t* w) {
  if (w == nullptr) return {};
  // DXGI_ADAPTER_DESC1::Description is up to 128 wchar_t; its UTF-8 form can
  // exceed the old fixed 128-byte buffer, and the unchecked failure returned an
  // empty description (silently breaking adapter pinning). Size the buffer from
  // the actual conversion instead.
  const int needed =
      WideCharToMultiByte(CP_UTF8, 0, w, -1, nullptr, 0, nullptr, nullptr);
  if (needed <= 0) return {};
  std::string out(static_cast<std::size_t>(needed), '\0');
  const int written = WideCharToMultiByte(CP_UTF8, 0, w, -1, out.data(), needed,
                                          nullptr, nullptr);
  if (written <= 0) return {};
  out.resize(static_cast<std::size_t>(written - 1));
  return out;
}

bool IsIntegrated(unsigned vendor, const std::string& desc,
                  unsigned long long dedicated_bytes) {
  if (vendor == 0x10DE) return false;  // NVIDIA always discrete
  if (vendor == 0x8086) {              // Intel: integrated unless Arc
    return desc.find("Arc") == std::string::npos &&
           desc.find("ARC") == std::string::npos;
  }
  if (vendor == 0x1002) {  // AMD ambiguous: APU vs card via VRAM size
    return dedicated_bytes < (512ull * 1024 * 1024);
  }
  return false;  // unknown vendor: never claim integrated
}

std::vector<GpuAdapterInfo> Enumerate() {
  std::vector<GpuAdapterInfo> out;
  IDXGIFactory1* f1 = nullptr;
  if (FAILED(CreateDXGIFactory1(__uuidof(IDXGIFactory1),
                                reinterpret_cast<void**>(&f1))) ||
      !f1) {
    return out;
  }
  for (int i = 0;; ++i) {
    IDXGIAdapter1* ad = nullptr;
    const HRESULT hr = f1->EnumAdapters1(i, &ad);
    if (hr == DXGI_ERROR_NOT_FOUND) break;
    if (FAILED(hr)) break;  // any other failure: stop instead of spinning
    DXGI_ADAPTER_DESC1 d{};
    if (ad && SUCCEEDED(ad->GetDesc1(&d)) &&
        !(d.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)) {
      GpuAdapterInfo info;
      info.desc = Narrow(d.Description);
      info.vendor = d.VendorId;
      info.dedicated_mb = d.DedicatedVideoMemory / (1024 * 1024);
      info.shared_mb = d.SharedSystemMemory / (1024 * 1024);
      info.integrated =
          IsIntegrated(d.VendorId, info.desc, d.DedicatedVideoMemory);
      out.push_back(info);
    }
    if (ad) ad->Release();
  }
  f1->Release();
  // Primary path: MINIMUM_POWER[0] outranks the table when Factory6
  // exists. Mark it by moving it first (stable, auditable in logs).
  IDXGIFactory6* f6 = nullptr;
  if (SUCCEEDED(CreateDXGIFactory1(__uuidof(IDXGIFactory6),
                                   reinterpret_cast<void**>(&f6))) &&
      f6) {
    IDXGIAdapter1* low = nullptr;
    if (SUCCEEDED(f6->EnumAdapterByGpuPreference(
            0, DXGI_GPU_PREFERENCE_MINIMUM_POWER,
            __uuidof(IDXGIAdapter1),
            reinterpret_cast<void**>(&low))) &&
        low) {
      DXGI_ADAPTER_DESC1 d{};
      if (SUCCEEDED(low->GetDesc1(&d))) {
        const std::string desc = Narrow(d.Description);
        for (size_t i = 0; i < out.size(); ++i) {
          if (out[i].desc == desc) {
            GpuAdapterInfo pick = out[i];
            out.erase(out.begin() + i);
            out.insert(out.begin(), pick);
            break;
          }
        }
      }
      low->Release();
    }
    f6->Release();
  }
  return out;
}

}  // namespace

std::string DescribeAdapters() {
  try {
    std::string s;
    for (const auto& a : Enumerate()) {
      char buf[256];
      snprintf(buf, sizeof(buf), "%s [vend=0x%04x ded=%lluMB %s]; ",
               a.desc.c_str(), a.vendor, a.dedicated_mb,
               a.integrated ? "integrated" : "discrete");
      s += buf;
    }
    return s;
  } catch (...) {
    return std::string();
  }
}

std::string ResolveAdapterPin(const std::string& mode) {
  try {
    const std::vector<GpuAdapterInfo> ads = Enumerate();
    if (ads.size() < 2) return std::string();  // single-adapter: no-op
    if (mode == "integrated") {
      for (const auto& a : ads) {
        if (a.integrated) return a.desc;
      }
      return std::string();
    }
    if (mode == "discrete") {
      unsigned long long best = 0;
      std::string pick;
      for (const auto& a : ads) {
        if (!a.integrated && a.dedicated_mb >= best) {
          best = a.dedicated_mb;
          pick = a.desc;
        }
      }
      return pick;
    }
    // "auto" (+ anything unknown, validated upstream): MINIMUM_POWER
    // winner sits at index 0 (see Enumerate). On a hybrid rig that is
    // the iGPU; single-adapter already returned above.
    return ads.empty() ? std::string() : ads[0].desc;
  } catch (...) {
    return std::string();
  }
}

}  // namespace k6wp
