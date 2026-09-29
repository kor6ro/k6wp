#pragma once

// iGPU/dGPU adapter pick for the mpv `d3d11-adapter` pin (P3L.3).
//
// Primary: IDXGIFactory6::EnumAdapterByGpuPreference(MINIMUM_POWER) —
// verified on RTX 2050 + Intel UHD to select the iGPU (spike A2).
// Fallback (no Factory6): VendorId table — 0x10DE always discrete,
// 0x8086 integrated unless "Arc" in Description, 0x1002 ambiguous ->
// dedicated-memory threshold, others via Shared>Dedicated comparison.
// The prompt's bare Shared>Dedicated rule is NOT used alone: it
// misclassifies the RTX 2050 on this rig (dedicated 3962MB < shared
// 8035MB). Software (Basic Render) adapters are always skipped.
//
// windows.h lives in the .cpp only — header exposes std types.

#include <string>

namespace k6wp {

// One enumerated adapter (software adapters excluded).
struct GpuAdapterInfo {
  std::string desc;  // DXGI_ADAPTER_DESC1.Description (UTF-8)
  unsigned vendor = 0;
  unsigned long long dedicated_mb = 0;
  unsigned long long shared_mb = 0;
  bool integrated = false;  // per the VendorId table
};

// Enumerate all hardware adapters (for logs/tests).
// Never throws; empty on total failure.
std::string DescribeAdapters();

// Resolve `mode` ("auto" | "integrated" | "discrete") to a
// `d3d11-adapter` value. Returns empty when no pin applies:
//   - mode "auto" on a single-adapter rig (nothing to choose),
//   - requested class absent (log warns, engine runs unpinned).
// The returned string is a Description substring for mpv to match
// (format verified against this build's logs — see P3L.3 notes).
// Never throws.
std::string ResolveAdapterPin(const std::string& mode);

}  // namespace k6wp
