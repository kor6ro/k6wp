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
  std::string stable_key;    // P2a §R4.2 tiered identity (UTF-8): "ccd:<lower
                             // monitorDevicePath>", "edid:<MFG>:<PPPP>:<hex>",
                             // or "gdi:<device_name>" last resort. Empty until
                             // EnrichMonitorIdentities fills it.
  bool identity_degraded = false;  // P2a §R4.3: true when the identity fell
                             // back to Tier 3 (GDI) because Tier 1 & 2 could
                             // not establish a unique, cross-port-stable key
                             // (ambiguous / registry unavailable).

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

// --- P2a tiered monitor identity (§R4.1/§R4.2/§R4.3) ------------------------
//
// Per-monitor identity inputs gathered by the Win32 enrich pass: the CCD
// Tier 1 device path (monitorDevicePath), the registry EDID Tier 2 identity
// (already decoded to its canonical string form), and the GDI device name
// Tier 3 fallback. windows.h-free, so the tier precedence + ambiguity rules
// can be driven headlessly by tests with no hardware.
struct MonitorIdentityInputs {
  std::string tier1_path;       // CCD monitorDevicePath (Tier 1); "" = none.
  bool tier2_usable = false;    // EDID parsed from the registry (Tier 2).
  std::string edid_mfg;         // UPPERCASE 3-letter PNP id, e.g. "DEL".
  std::string edid_product;     // UPPERCASE 4-hex-digit product, e.g. "4098".
  std::string edid_serial_hex;  // lowercase hex, minimal digits ("1a2b");
                                // "0" when the EDID serial is zero.
  std::wstring device_name;     // GDI name (Tier 3), e.g. L"\\\\.\\DISPLAY1".
};

// One monitor's assigned identity: the §R4.2 stable_key plus the §R4.3
// degraded flag (true = ambiguous / Tier 3 fallback).
struct AssignedIdentity {
  std::string stable_key;
  bool degraded = false;
};

// Pure: assigns each monitor's stable_key from the FULL snapshot's inputs,
// applying the §R4.2 tier precedence (ccd -> edid -> gdi) plus the §R4.3
// uniqueness rule — a tier is only used when it is unique across the
// snapshot; otherwise the monitor falls to the next tier. A monitor that
// reaches Tier 3 is flagged degraded. The i-th output corresponds to the
// i-th input; deterministic and closed over its argument (never throws).
std::vector<AssignedIdentity> AssignStableKeys(
    const std::vector<MonitorIdentityInputs>& inputs);

// Win32 enrich entry point used by ListMonitors: queries CCD
// (QueryDisplayConfig) for Tier 1 and the EDID registry value (advapi32) for
// Tier 2, then fills MonitorInfo::stable_key and
// MonitorInfo::identity_degraded via AssignStableKeys. Best-effort and
// no-throw: on any failure each monitor falls back to its GDI Tier 3 key
// (degraded).
void EnrichMonitorIdentities(std::vector<MonitorInfo>& monitors) noexcept;

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
// JSON-escaped),orientation,refresh_hz,scale_pct,stable_key (UTF-8,
// JSON-escaped, additive after scale_pct; "" when unenriched)},...],
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