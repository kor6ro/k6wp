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

  std::cout << "\n" << g_checks << " checks, " << g_failures << " failures\n";
  return g_failures == 0 ? 0 : 1;
}
