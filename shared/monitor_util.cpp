#include "monitor_util.hpp"

#include <windows.h>

#include <shellscalingapi.h>

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
    info.orientation = static_cast<int>(dm.dmDisplayOrientation);
    info.refresh_hz = static_cast<int>(dm.dmDisplayFrequency);
  } else {
    // Driver refused the current mode (mirrored/RDP edge): fall back to the
    // monitor rect so a real monitor never reports 0x0.
    info.width = static_cast<int>(mi.rcMonitor.right - mi.rcMonitor.left);
    info.height = static_cast<int>(mi.rcMonitor.bottom - mi.rcMonitor.top);
  }

  // DEVICE_SCALE_FACTOR is already a percent (100 = 100%, 125 = 125%),
  // so it is stored directly with no DPI arithmetic. GetDpiForMonitor stays
  // unused: it is documented "not DPI aware" under PerMonitorV2.
  DEVICE_SCALE_FACTOR dsf = SCALE_100_PERCENT;
  const HRESULT scale_hr = ::GetScaleFactorForMonitor(hmon, &dsf);
  info.scale_pct =
      ResolveScalePercent(static_cast<int>(dsf), scale_hr == S_OK);

  ctx->out->push_back(std::move(info));
  return TRUE;
}

}  // namespace

int ResolveScalePercent(int dsf_value, bool api_ok) noexcept {
  if (!api_ok || dsf_value == -1) return 100;
  return dsf_value;
}

namespace {

// Converts a UTF-16 device name (e.g. L"\\.\DISPLAY1") to UTF-8 for JSON
// output. Returns "" (never throws) when the conversion fails.
std::string DeviceNameToUtf8(const std::wstring& wide) noexcept {
  if (wide.empty()) return {};
  const int needed = ::WideCharToMultiByte(
      CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), nullptr, 0,
      nullptr, nullptr);
  if (needed <= 0) return {};
  std::string out(static_cast<std::size_t>(needed), '\0');
  const int written = ::WideCharToMultiByte(
      CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), out.data(),
      needed, nullptr, nullptr);
  if (written <= 0) return {};
  return out;
}

// JSON-escapes an already UTF-8 string: backslash, quote, and C0 controls.
std::string EscapeJsonString(const std::string& in) {
  static const char kHex[] = "0123456789abcdef";
  std::string out;
  out.reserve(in.size() + 2);
  for (const unsigned char c : in) {
    switch (c) {
      case '"':
        out += "\\\"";
        break;
      case '\\':
        out += "\\\\";
        break;
      case '\b':
        out += "\\b";
        break;
      case '\f':
        out += "\\f";
        break;
      case '\n':
        out += "\\n";
        break;
      case '\r':
        out += "\\r";
        break;
      case '\t':
        out += "\\t";
        break;
      default:
        if (c < 0x20) {
          out += "\\u00";
          out += kHex[(c >> 4) & 0xF];
          out += kHex[c & 0xF];
        } else {
          out += static_cast<char>(c);
        }
        break;
    }
  }
  return out;
}

}  // namespace

VirtualScreenGeometry GetVirtualScreenGeometry() noexcept {
  // Same four calls as MultiMonitor::GetSpanGeometry (copied, not linked:
  // multi_monitor.cpp drags in libmpv). SM_*VIRTUALSCREEN are compile-time
  // constants, so GetSystemMetrics cannot fail here; a non-positive size
  // still maps to zeros per the AttachSpanSlot fallback contract.
  VirtualScreenGeometry g;
  g.x = ::GetSystemMetrics(SM_XVIRTUALSCREEN);
  g.y = ::GetSystemMetrics(SM_YVIRTUALSCREEN);
  g.width = ::GetSystemMetrics(SM_CXVIRTUALSCREEN);
  g.height = ::GetSystemMetrics(SM_CYVIRTUALSCREEN);
  if (g.width <= 0 || g.height <= 0) return VirtualScreenGeometry{};
  return g;
}

std::string FormatMonitorsJson(const std::vector<MonitorInfo>& monitors,
                               const VirtualScreenGeometry& virtual_screen) {
  std::string out = "{\"monitors\":[";
  for (std::size_t i = 0; i < monitors.size(); ++i) {
    if (i > 0) out += ",";
    const auto& m = monitors[i];
    out += "{\"id\":" + std::to_string(m.id) +
           ",\"x\":" + std::to_string(m.x) +
           ",\"y\":" + std::to_string(m.y) +
           ",\"width\":" + std::to_string(m.width) +
           ",\"height\":" + std::to_string(m.height) +
           ",\"is_primary\":" + (m.is_primary ? "true" : "false") +
           ",\"device_name\":\"" +
           EscapeJsonString(DeviceNameToUtf8(m.device_name)) + "\"" +
           ",\"orientation\":" + std::to_string(m.orientation) +
           ",\"refresh_hz\":" + std::to_string(m.refresh_hz) +
           ",\"scale_pct\":" + std::to_string(m.scale_pct) + "}";
  }
  out += "],\"virtual_screen\":{\"x\":" + std::to_string(virtual_screen.x) +
         ",\"y\":" + std::to_string(virtual_screen.y) +
         ",\"width\":" + std::to_string(virtual_screen.width) +
         ",\"height\":" + std::to_string(virtual_screen.height) + "}}";
  return out;
}

std::string FormatMonitorsJson(const std::vector<MonitorInfo>& monitors) {
  return FormatMonitorsJson(monitors, GetVirtualScreenGeometry());
}

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

bool SetProcessDpiAwarenessContextPMDA() noexcept {
  return ::SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2) !=
         FALSE;
}

}  // namespace k6wp