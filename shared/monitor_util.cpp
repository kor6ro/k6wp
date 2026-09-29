#include "monitor_util.hpp"

#include <windows.h>

#include <algorithm>
#include <cstddef>

namespace k6wp {

namespace {

// DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 is only defined in the SDK for
// Windows 10 1703+; define it ourselves so the code still compiles against
// older SDKs (the value is stable: -4).
#ifndef DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 \
  reinterpret_cast<DPI_AWARENESS_CONTEXT>(-4)
#endif

struct EnumMonitorsContext {
  std::vector<MonitorInfo>* out;
};

BOOL CALLBACK OnMonitorEnum(HMONITOR hmon, HDC /*hdc*/, LPRECT /*rect*/,
                            LPARAM lparam) {
  auto* ctx = reinterpret_cast<EnumMonitorsContext*>(lparam);

  MONITORINFOEXW mi{};
  mi.cbSize = sizeof(mi);
  if (!::GetMonitorInfoW(hmon, &mi)) {
    // Skip monitors we cannot query; keep enumerating the rest.
    return TRUE;
  }

  MonitorInfo info;
  info.is_primary = (mi.dwFlags & MONITORINFOF_PRIMARY) != 0;
  info.device_name = mi.szDevice;
  info.x = mi.rcMonitor.left;
  info.y = mi.rcMonitor.top;

  DEVMODEW dm{};
  dm.dmSize = sizeof(dm);
  if (::EnumDisplaySettingsExW(mi.szDevice, ENUM_CURRENT_SETTINGS, &dm, 0)) {
    info.width = static_cast<int>(dm.dmPelsWidth);
    info.height = static_cast<int>(dm.dmPelsHeight);
  }

  ctx->out->push_back(std::move(info));
  return TRUE;
}

}  // namespace

std::vector<MonitorInfo> ListMonitors() noexcept {
  std::vector<MonitorInfo> monitors;
  EnumMonitorsContext ctx{&monitors};

  ::EnumDisplayMonitors(nullptr, nullptr, OnMonitorEnum,
                        reinterpret_cast<LPARAM>(&ctx));

  // Deterministic ordering: primary first, then by device name. Ids are the
  // sorted index, so id 0 is always the primary monitor.
  std::stable_sort(monitors.begin(), monitors.end(),
                   [](const MonitorInfo& a, const MonitorInfo& b) {
                     if (a.is_primary != b.is_primary) return a.is_primary;
                     return a.device_name < b.device_name;
                   });
  for (std::size_t i = 0; i < monitors.size(); ++i) {
    monitors[i].id = static_cast<int>(i);
  }

  return monitors;
}

MonitorInfo GetPrimaryMonitor() noexcept {
  const auto monitors = ListMonitors();
  for (const auto& m : monitors) {
    if (m.is_primary) return m;
  }
  return MonitorInfo{};
}

std::pair<int, int> GetTargetResolution(int monitor_id) noexcept {
  const auto monitors = ListMonitors();
  if (monitors.empty()) return {0, 0};

  for (const auto& m : monitors) {
    if (m.id == monitor_id) return {m.width, m.height};
  }

  // Unknown id -> fall back to the primary monitor's resolution.
  for (const auto& m : monitors) {
    if (m.is_primary) return {m.width, m.height};
  }
  return {monitors.front().width, monitors.front().height};
}

bool SetProcessDpiAwarenessContextPMDA() noexcept {
  return ::SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2) !=
         FALSE;
}

}  // namespace k6wp