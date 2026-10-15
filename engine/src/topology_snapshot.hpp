#pragma once

// TopologySnapshot — pure data contract for the reconcile pipeline
// (design 02 §R5 / §R12.1; brief §4.4).
//
// windows.h-free: plain std types only, so this header (and the pure diff /
// retry modules built on it) compiles headless, exactly like
// desktop_placement.hpp. No Win32, no clock, no logging. The Win32 side
// (snapshot_topology()) lives in a separate TU that is allowed to include
// <windows.h>; it fills these fields and hands the struct to the pure core.

#include <string>
#include <vector>

namespace k6wp {

struct TopologyMonitor {
  std::string stable_key;   // §4.3 tiered identity (never GDI-only unless fallback)
  std::string gdi_name;     // MonitorInfo::device_name as UTF-8 (\.\DISPLAYn)
  int x = 0;                // virtual-screen coords
  int y = 0;
  int width = 0;
  int height = 0;
  int scale_pct = 100;      // re-read every pass (§R3 / B11)
  int orientation = 0;      // DEVMODEW.dmDisplayOrientation
  bool is_primary = false;
  bool degraded = false;    // identity ambiguous (§R4.3) -> Degraded
};

struct TopologySnapshot {
  std::vector<TopologyMonitor> monitors;  // deterministic order: primary first, then stable_key
  int virtual_x = 0;        // virtual_origin
  int virtual_y = 0;
  int virtual_width = 0;    // virtual_size
  int virtual_height = 0;
  std::string hash_snapshot;// stable hash over canonical serialization (all fields above)
};

}  // namespace k6wp
