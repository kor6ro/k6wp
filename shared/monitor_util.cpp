#include "monitor_util.hpp"

#include <windows.h>

#include <shellscalingapi.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <map>
#include <tuple>
#include <utility>

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

namespace {

// ASCII-only lowercase (leaves non-ASCII bytes untouched). The CCD Tier 1
// path is lowercased so the key is case-insensitive across driver casing.
std::string AsciiLower(const std::string& in) {
  std::string out = in;
  for (char& c : out) {
    if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
  }
  return out;
}

// Decoded EDID Tier 2 identity, already in its canonical string form.
struct EdidIdentity {
  bool usable = false;
  std::string mfg;         // 3 UPPERCASE PNP letters.
  std::string product;     // UPPERCASE 4 hex digits (zero-padded).
  std::string serial_hex;  // lowercase, minimal digits ("0" when zero).
};

std::string HexUpper4(unsigned int v) {
  static const char kHex[] = "0123456789ABCDEF";
  std::string s(4, '0');
  for (int i = 3; i >= 0; --i) {
    s[static_cast<std::size_t>(i)] = kHex[v & 0xF];
    v >>= 4;
  }
  return s;
}

std::string HexLower(unsigned int v) {
  static const char kHex[] = "0123456789abcdef";
  if (v == 0) return "0";
  std::string s;
  while (v != 0) {
    s.push_back(kHex[v & 0xF]);
    v >>= 4;
  }
  std::reverse(s.begin(), s.end());
  return s;
}

// Parses the first 128 bytes of an EDID block into the canonical identity.
// Requires the standard header; rejects anything shorter / malformed.
bool ParseEdid(const std::vector<unsigned char>& edid, EdidIdentity* out) {
  if (edid.size() < 128) return false;
  static const unsigned char kHeader[8] = {0x00, 0xFF, 0xFF, 0xFF,
                                           0xFF, 0xFF, 0xFF, 0x00};
  for (int i = 0; i < 8; ++i) {
    if (edid[static_cast<std::size_t>(i)] != kHeader[i]) return false;
  }
  const unsigned int mfg_v =
      (static_cast<unsigned int>(edid[8]) << 8) | edid[9];
  char letters[3];
  letters[0] = static_cast<char>('A' + ((mfg_v >> 10) & 0x1F) - 1);
  letters[1] = static_cast<char>('A' + ((mfg_v >> 5) & 0x1F) - 1);
  letters[2] = static_cast<char>('A' + (mfg_v & 0x1F) - 1);
  for (char c : letters) {
    if (c < 'A' || c > 'Z') return false;  // malformed PNP id
  }
  const unsigned int product = static_cast<unsigned int>(edid[10]) |
                               (static_cast<unsigned int>(edid[11]) << 8);
  const unsigned int serial = static_cast<unsigned int>(edid[12]) |
                              (static_cast<unsigned int>(edid[13]) << 8) |
                              (static_cast<unsigned int>(edid[14]) << 16) |
                              (static_cast<unsigned int>(edid[15]) << 24);
  out->usable = true;
  out->mfg = std::string(letters, 3);
  out->product = HexUpper4(product);
  out->serial_hex = HexLower(serial);
  return true;
}

// One active CCD path joined to its GDI source device name.
struct CcdEntry {
  std::wstring gdi_name;             // viewGdiDeviceName (e.g. \\.\DISPLAY1)
  std::wstring monitor_device_path;  // monitorDevicePath (Tier 1)
};

// Tier 1 gather: QDC_ONLY_ACTIVE_PATHS -> per-path target/source names.
// Best-effort; returns empty on any failure (never throws).
std::vector<CcdEntry> QueryCcdMonitors() {
  std::vector<CcdEntry> out;
  UINT32 path_count = 0;
  UINT32 mode_count = 0;
  if (::GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS, &path_count,
                                    &mode_count) != ERROR_SUCCESS) {
    return out;
  }
  std::vector<DISPLAYCONFIG_PATH_INFO> paths(path_count);
  std::vector<DISPLAYCONFIG_MODE_INFO> modes(mode_count);
  if (::QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS, &path_count, paths.data(),
                           &mode_count, modes.data(), nullptr) !=
      ERROR_SUCCESS) {
    return out;
  }
  paths.resize(path_count);
  for (const auto& p : paths) {
    DISPLAYCONFIG_TARGET_DEVICE_NAME tn{};
    tn.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME;
    tn.header.size = sizeof(tn);
    tn.header.adapterId = p.targetInfo.adapterId;
    tn.header.id = p.targetInfo.id;
    if (::DisplayConfigGetDeviceInfo(&tn.header) != ERROR_SUCCESS) continue;

    DISPLAYCONFIG_SOURCE_DEVICE_NAME sn{};
    sn.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
    sn.header.size = sizeof(sn);
    sn.header.adapterId = p.sourceInfo.adapterId;
    sn.header.id = p.sourceInfo.id;
    if (::DisplayConfigGetDeviceInfo(&sn.header) != ERROR_SUCCESS) continue;

    CcdEntry e;
    e.gdi_name = sn.viewGdiDeviceName;
    e.monitor_device_path = tn.monitorDevicePath;
    if (e.gdi_name.empty() || e.monitor_device_path.empty()) continue;
    out.push_back(std::move(e));
  }
  return out;
}

// Tier 2 gather: EDID from the Enum\DISPLAY registry. monitorDevicePath looks
// like \\?\DISPLAY#<PNPid>#<instance>#<GUID>, which maps to
// HKLM\SYSTEM\CurrentControlSet\Enum\DISPLAY\<PNPid>\<instance>\Device
// Parameters (value "EDID"). Best-effort; false on any failure.
bool ReadEdidForDevicePath(const std::wstring& monitor_device_path,
                           EdidIdentity* out) {
  std::vector<std::wstring> parts;
  std::size_t start = 0;
  while (start <= monitor_device_path.size()) {
    const std::size_t pos = monitor_device_path.find(L'#', start);
    if (pos == std::wstring::npos) {
      parts.push_back(monitor_device_path.substr(start));
      break;
    }
    parts.push_back(monitor_device_path.substr(start, pos - start));
    start = pos + 1;
  }
  if (parts.size() < 3 || parts[1].empty() || parts[2].empty()) return false;

  const std::wstring subkey =
      L"SYSTEM\\CurrentControlSet\\Enum\\DISPLAY\\" + parts[1] + L"\\" +
      parts[2] + L"\\Device Parameters";
  HKEY key = nullptr;
  if (::RegOpenKeyExW(HKEY_LOCAL_MACHINE, subkey.c_str(), 0, KEY_READ, &key) !=
      ERROR_SUCCESS) {
    return false;
  }
  DWORD type = 0;
  DWORD size = 0;
  LONG r = ::RegQueryValueExW(key, L"EDID", nullptr, &type, nullptr, &size);
  if (r != ERROR_SUCCESS || type != REG_BINARY || size < 128 || size > 4096) {
    ::RegCloseKey(key);
    return false;
  }
  std::vector<unsigned char> buf(size);
  r = ::RegQueryValueExW(key, L"EDID", nullptr, &type, buf.data(), &size);
  ::RegCloseKey(key);
  if (r != ERROR_SUCCESS) return false;
  buf.resize(size);
  return ParseEdid(buf, out);
}

}  // namespace

std::vector<AssignedIdentity> AssignStableKeys(
    const std::vector<MonitorIdentityInputs>& inputs) {
  std::vector<AssignedIdentity> out(inputs.size());

  // Uniqueness counts over the canonical candidate keys (§R4.2: a tier is
  // only used when it is unique across the whole snapshot).
  std::map<std::string, int> ccd_count;
  std::map<std::string, int> edid_count;
  for (const auto& in : inputs) {
    if (!in.tier1_path.empty()) {
      ++ccd_count["ccd:" + AsciiLower(in.tier1_path)];
    }
    if (in.tier2_usable && !in.edid_serial_hex.empty() &&
        in.edid_serial_hex != "0") {
      ++edid_count["edid:" + in.edid_mfg + ":" + in.edid_product + ":" +
                   in.edid_serial_hex];
    }
  }

  for (std::size_t i = 0; i < inputs.size(); ++i) {
    const MonitorIdentityInputs& in = inputs[i];
    const std::string ccd_key = in.tier1_path.empty()
                                    ? std::string()
                                    : "ccd:" + AsciiLower(in.tier1_path);
    const bool tier2_ok = in.tier2_usable && !in.edid_serial_hex.empty() &&
                          in.edid_serial_hex != "0";
    const std::string edid_key =
        tier2_ok ? "edid:" + in.edid_mfg + ":" + in.edid_product + ":" +
                       in.edid_serial_hex
                 : std::string();

    if (!ccd_key.empty() && ccd_count[ccd_key] == 1) {
      out[i].stable_key = ccd_key;
      out[i].degraded = false;
    } else if (!edid_key.empty() && edid_count[edid_key] == 1) {
      out[i].stable_key = edid_key;
      out[i].degraded = false;
    } else {
      out[i].stable_key = "gdi:" + DeviceNameToUtf8(in.device_name);
      out[i].degraded = true;
    }
  }
  return out;
}

void EnrichMonitorIdentities(std::vector<MonitorInfo>& monitors) noexcept {
  if (monitors.empty()) return;

  std::vector<CcdEntry> ccd;
  try {
    ccd = QueryCcdMonitors();
  } catch (...) {
    ccd.clear();
  }
  std::map<std::wstring, std::wstring> path_by_gdi;
  for (auto& e : ccd) {
    path_by_gdi[e.gdi_name] = e.monitor_device_path;
  }

  std::vector<MonitorIdentityInputs> inputs(monitors.size());
  for (std::size_t i = 0; i < monitors.size(); ++i) {
    MonitorIdentityInputs& in = inputs[i];
    in.device_name = monitors[i].device_name;
    const auto it = path_by_gdi.find(monitors[i].device_name);
    if (it == path_by_gdi.end()) continue;  // no CCD path -> GDI fallback
    in.tier1_path = DeviceNameToUtf8(it->second);
    EdidIdentity edid;
    try {
      if (ReadEdidForDevicePath(it->second, &edid) && edid.usable) {
        in.tier2_usable = true;
        in.edid_mfg = edid.mfg;
        in.edid_product = edid.product;
        in.edid_serial_hex = edid.serial_hex;
      }
    } catch (...) {
      in.tier2_usable = false;
    }
  }

  std::vector<AssignedIdentity> assigned;
  try {
    assigned = AssignStableKeys(inputs);
  } catch (...) {
    assigned.clear();
  }
  for (std::size_t i = 0; i < monitors.size(); ++i) {
    if (i < assigned.size()) {
      monitors[i].stable_key = assigned[i].stable_key;
      monitors[i].identity_degraded = assigned[i].degraded;
    } else {
      monitors[i].stable_key =
          "gdi:" + DeviceNameToUtf8(monitors[i].device_name);
      monitors[i].identity_degraded = true;
    }
  }
}

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
           ",\"scale_pct\":" + std::to_string(m.scale_pct) +
           ",\"stable_key\":\"" + EscapeJsonString(m.stable_key) + "\"}";
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

  // P2a §R4: fill stable_key / identity_degraded from CCD + EDID (best-effort;
  // falls back to the GDI key when unavailable). Idempotent and no-throw.
  EnrichMonitorIdentities(monitors);

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