// Mini unit tests for shared/monitor_util (MonitorInfo orientation, refresh,
// per-monitor scale plus pure helpers). No external test framework: plain
// asserts with a pass/fail counter. Exit code 0 = all pass.
#include <iostream>
#include <string>
#include <vector>

#include "monitor_util.hpp"

namespace {

int g_checks = 0;
int g_failures = 0;

void Check(bool cond, const std::string& name) {
  ++g_checks;
  if (cond) {
    std::cout << "[PASS] " << name << "\n";
  } else {
    ++g_failures;
    std::cout << "[FAIL] " << name << "\n";
  }
}

k6wp::MonitorInfo MakeInfo(int width, int height, int scale_pct) {
  k6wp::MonitorInfo m;
  m.width = width;
  m.height = height;
  m.scale_pct = scale_pct;
  return m;
}

bool Contains(const std::string& haystack, const std::string& needle) {
  return haystack.find(needle) != std::string::npos;
}

// Two-monitor task-2 fixture: landscape primary at 0,0 plus a portrait
// secondary at 1920,0 (union width 3840, x 0).
std::vector<k6wp::MonitorInfo> MakeTwoMonitorFixture() {
  std::vector<k6wp::MonitorInfo> monitors;
  k6wp::MonitorInfo primary;
  primary.id = 0;
  primary.x = 0;
  primary.y = 0;
  primary.width = 1920;
  primary.height = 1080;
  primary.is_primary = true;
  primary.device_name = L"\\\\.\\DISPLAY1";
  primary.orientation = 0;
  primary.refresh_hz = 60;
  primary.scale_pct = 100;
  monitors.push_back(primary);
  k6wp::MonitorInfo portrait;
  portrait.id = 1;
  portrait.x = 1920;
  portrait.y = 0;
  portrait.width = 1080;
  portrait.height = 1920;
  portrait.is_primary = false;
  portrait.device_name = L"\\\\.\\DISPLAY2";
  portrait.orientation = 1;
  portrait.refresh_hz = 144;
  portrait.scale_pct = 125;
  monitors.push_back(portrait);
  return monitors;
}

}  // namespace

int main() {
  // 1. Synthetic portrait rect (1080x1920) reads as portrait.
  {
    const auto m = MakeInfo(1080, 1920, 100);
    Check(m.IsPortrait(), "1. 1080x1920 IsPortrait()==true");
  }

  // 2. Synthetic landscape rect (1920x1080) reads as landscape.
  {
    const auto m = MakeInfo(1920, 1080, 100);
    Check(!m.IsPortrait(), "2. 1920x1080 IsPortrait()==false");
  }

  // 3-4. ScalePercent lives in the PERCENT domain that
  // GetScaleFactorForMonitor returns (100 = 100%, 125 = 125%).
  {
    const auto m = MakeInfo(1920, 1080, 100);
    Check(m.ScalePercent() == 100, "3. scale_pct==100 ScalePercent()==100");
  }
  {
    const auto m = MakeInfo(1920, 1080, 125);
    Check(m.ScalePercent() == 125, "4. scale_pct==125 ScalePercent()==125");
  }

  // 5. Fresh MonitorInfo carries safe fallbacks (scale 100, refresh 0,
  // orientation DMDO_DEFAULT 0) before enumeration fills it in.
  {
    const k6wp::MonitorInfo m;
    Check(m.scale_pct == 100, "5. default scale_pct==100");
    Check(m.refresh_hz == 0, "5. default refresh_hz==0");
    Check(m.orientation == 0, "5. default orientation==0 (DMDO_DEFAULT)");
  }

  // 6. Scale fallback seam: any API failure maps to the 100 fallback, a
  // success passes the percent value straight through (no DPI arithmetic).
  {
    Check(k6wp::ResolveScalePercent(125, true) == 125,
          "6. seam ok(125)==125");
    Check(k6wp::ResolveScalePercent(100, true) == 100,
          "6. seam ok(100)==100");
    Check(k6wp::ResolveScalePercent(125, false) == 100,
          "6. seam failed hr falls back to 100");
    Check(k6wp::ResolveScalePercent(-1, true) == 100,
          "6. seam dsf==-1 falls back to 100");
    Check(k6wp::ResolveScalePercent(-1, false) == 100,
          "6. seam failed hr + dsf==-1 falls back to 100");
  }

  // 7. Live enumeration never throws and still yields a non-empty,
  // primary-first, id-assigned list with sane extended fields.
  {
    std::vector<k6wp::MonitorInfo> monitors;
    bool threw = false;
    try {
      monitors = k6wp::ListMonitors();
    } catch (...) {
      threw = true;
    }
    Check(!threw, "7. ListMonitors() does not throw");
    Check(!monitors.empty(), "7. ListMonitors() non-empty");
    if (!monitors.empty()) {
      Check(monitors[0].id == 0, "7. first monitor id==0");
      Check(monitors[0].is_primary, "7. first monitor is_primary");
      Check(monitors[0].scale_pct > 0, "7. live scale_pct>0");
      Check(monitors[0].refresh_hz >= 0, "7. live refresh_hz>=0");
      Check(monitors[0].width > 0 && monitors[0].height > 0,
            "7. live width/height>0");
    } else {
      Check(false, "7. (skipped: no monitors enumerated)");
    }
  }

  // 8. Formatter happy path: two-monitor fixture yields two entries and
  // every named key plus the top-level virtual_screen.
  {
    const auto monitors = MakeTwoMonitorFixture();
    const k6wp::VirtualScreenGeometry vs{0, 0, 3840, 1920};
    std::string out;
    bool threw = false;
    try {
      out = k6wp::FormatMonitorsJson(monitors, vs);
    } catch (...) {
      threw = true;
    }
    Check(!threw, "8. FormatMonitorsJson(monitors, vs) does not throw");
    Check(Contains(out, "\"id\":0"), "8. formatter emits first entry id");
    Check(Contains(out, "\"id\":1"), "8. formatter emits second entry id");
    Check(Contains(out, "\"x\":0"), "8. formatter emits \"x\"");
    Check(Contains(out, "\"x\":1920"), "8. formatter emits second x==1920");
    Check(Contains(out, "\"y\":0"), "8. formatter emits \"y\"");
    Check(Contains(out, "\"device_name\""), "8. formatter emits \"device_name\"");
    Check(Contains(out, "\"orientation\":1"),
          "8. formatter emits portrait \"orientation\":1");
    Check(Contains(out, "\"refresh_hz\":144"),
          "8. formatter emits \"refresh_hz\":144");
    Check(Contains(out, "\"scale_pct\":125"),
          "8. formatter emits \"scale_pct\":125");
    Check(Contains(out, "\"is_primary\":true"),
          "8. formatter keeps \"is_primary\"");
    Check(Contains(out, "\"virtual_screen\""),
          "8. formatter emits \"virtual_screen\"");
    Check(Contains(out, "\"width\":3840"),
          "8. formatter virtual_screen.width==3840");
    Check(Contains(out, "\"monitors\""), "8. formatter emits \"monitors\"");
  }

  // 9. device_name (\\.\DISPLAY1) is JSON-escaped: each backslash doubled.
  {
    const auto monitors = MakeTwoMonitorFixture();
    const k6wp::VirtualScreenGeometry vs{0, 0, 3840, 1920};
    const std::string out = k6wp::FormatMonitorsJson(monitors, vs);
    Check(Contains(out, "\"device_name\":\"\\\\\\\\.\\\\DISPLAY1\""),
          "9. device_name backslashes escaped");
    Check(!Contains(out, "\"device_name\":\"\\\\.\\"),
          "9. device_name has no raw backslash");
  }

  // 10. Failure path: empty vector -> empty monitors array and a
  // non-positive virtual_screen, without throwing (AttachSpanSlot fallback
  // contract: report, never die).
  {
    const std::vector<k6wp::MonitorInfo> empty;
    const k6wp::VirtualScreenGeometry vs{0, 0, 0, 0};
    std::string out;
    bool threw = false;
    try {
      out = k6wp::FormatMonitorsJson(empty, vs);
    } catch (...) {
      threw = true;
    }
    Check(!threw, "10. empty formatter does not throw");
    Check(Contains(out, "\"monitors\":[]"), "10. empty formatter returns []");
    Check(Contains(out, "\"width\":0"),
          "10. empty virtual_screen reports non-positive size");
  }

  // 11. Single-arg overload uses the live virtual screen and still carries
  // every key for the fixture monitors.
  {
    const auto monitors = MakeTwoMonitorFixture();
    std::string out;
    bool threw = false;
    try {
      out = k6wp::FormatMonitorsJson(monitors);
    } catch (...) {
      threw = true;
    }
    Check(!threw, "11. FormatMonitorsJson(monitors) does not throw");
    Check(Contains(out, "\"device_name\""),
          "11. live formatter emits \"device_name\"");
    Check(Contains(out, "\"virtual_screen\""),
          "11. live formatter emits \"virtual_screen\"");
  }

  std::cout << "\n" << g_checks << " checks, " << g_failures << " failures\n";
  return g_failures == 0 ? 0 : 1;
}
