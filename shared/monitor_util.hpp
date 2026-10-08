#pragma once

#include <string>
#include <utility>
#include <vector>

namespace k6wp {

// One physical/virtual monitor as seen by the desktop.
struct MonitorInfo {
  int id = 0;              // Stable within a session; 0 = primary (sorted first).
  int x = 0;               // Monitor rect left/top, virtual-screen coords
  int y = 0;               // (rcMonitor from GetMonitorInfoW).
  int width = 0;           // Current resolution, pixels.
  int height = 0;          // Current resolution, pixels.
  bool is_primary = false; // True for the primary monitor (MONITORINFOF_PRIMARY).
  std::wstring device_name;  // e.g. L"\\\\.\\DISPLAY1" (UTF-16 native).
  int orientation = 0;       // DEVMODEW.dmDisplayOrientation (DMDO_DEFAULT 0,
                             // DMDO_90 1, DMDO_180 2, DMDO_270 3).
  int refresh_hz = 0;        // DEVMODEW.dmDisplayFrequency; 0 when unknown
                             // (driver refused the current mode).
  int scale_pct = 100;       // GetScaleFactorForMonitor percent (100 = 100%,
                             // 125 = 125%); 100 on API failure.

  // Portrait check from the already-rotated aspect (width/height reflect the
  // current orientation, so height > width means portrait).
  bool IsPortrait() const { return height > width; }

  // Percent-domain readout of the per-monitor scale (no arithmetic).
  int ScalePercent() const { return scale_pct; }
};

// Enumerates all monitors via EnumDisplayMonitors/GetMonitorInfoW plus
// EnumDisplaySettingsExW for the current resolution. Sorted primary-first,
// then by device name, so ids are deterministic within a session. Returns an
// empty vector when no monitors are present (never throws).
std::vector<MonitorInfo> ListMonitors() noexcept;

// Returns the primary monitor, or a default-constructed MonitorInfo{} when no
// monitor exists.
MonitorInfo GetPrimaryMonitor() noexcept;

// Maps a GetScaleFactorForMonitor result to scale_pct: api_ok is
// (hr == S_OK) and dsf_value is the raw DEVICE_SCALE_FACTOR (already a
// percent: 100 = 100%, 125 = 125%). Any failure (or dsf_value == -1)
// yields the 100 fallback. Pure function; also the unit-test seam for the
// scale failure path, which cannot be forced through the real API.
int ResolveScalePercent(int dsf_value, bool api_ok) noexcept;

// Virtual-screen union in pixels, same contract as
// MultiMonitor::GetSpanGeometry (origin + extents, never throws).
struct VirtualScreenGeometry {
  int x = 0;
  int y = 0;
  int width = 0;
  int height = 0;
};

// Reads the live virtual screen via the four GetSystemMetrics
// (SM_X/YVIRTUALSCREEN, SM_CX/CYVIRTUALSCREEN) calls GetSpanGeometry uses.
// Returns zeros (never throws) when the query yields a non-positive size,
// mirroring AttachSpanSlot's fallback contract.
VirtualScreenGeometry GetVirtualScreenGeometry() noexcept;

// Pure JSON formatter for the monitor list (monitor_dump's output):
// {"monitors":[{id,x,y,width,height,is_primary,device_name (UTF-8,
// JSON-escaped),orientation,refresh_hz,scale_pct},...],
//  "virtual_screen":{x,y,width,height}}.
// The two-arg overload formats with an explicit virtual screen (the
// unit-test seam); the one-arg overload reads the live screen via
// GetVirtualScreenGeometry(). Empty input yields {"monitors":[],...}
// with a non-positive virtual_screen and never throws.
std::string FormatMonitorsJson(const std::vector<MonitorInfo>& monitors,
                               const VirtualScreenGeometry& virtual_screen);
std::string FormatMonitorsJson(const std::vector<MonitorInfo>& monitors);

// Sets per-monitor DPI awareness (DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2,
// user32, Windows 10 1703+). Returns true on success. Call once at startup,
// before any window is created.
bool SetProcessDpiAwarenessContextPMDA() noexcept;

}  // namespace k6wp