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
};

// Enumerates all monitors via EnumDisplayMonitors/GetMonitorInfoW plus
// EnumDisplaySettingsExW for the current resolution. Sorted primary-first,
// then by device name, so ids are deterministic within a session. Returns an
// empty vector when no monitors are present (never throws).
std::vector<MonitorInfo> ListMonitors() noexcept;

// Returns the primary monitor, or a default-constructed MonitorInfo{} when no
// monitor exists.
MonitorInfo GetPrimaryMonitor() noexcept;

// Sets per-monitor DPI awareness (DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2,
// user32, Windows 10 1703+). Returns true on success. Call once at startup,
// before any window is created.
bool SetProcessDpiAwarenessContextPMDA() noexcept;

}  // namespace k6wp