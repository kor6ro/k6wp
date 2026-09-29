// Unit tests for engine/src/occlusion_watch pure helpers (P2.5, Todo 12).
// No external test framework: plain asserts with a pass/fail counter,
// same style as tests/config_test.cpp. Exit code 0 = all pass.
//
// Synthetic rects only — no live windows (no EnumWindows, no Check()).
// Covers: overlapping-window union (naive-sum regression), multi-monitor
// clipping, 95/90 hysteresis boundaries, and the self-suspend armed flag.
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "occlusion_watch.hpp"

namespace k6wp {
// Link-only stubs, NEVER executed: OcclusionWatch::Check (the live-windows
// path) is not called by any test below, but MSVC resolves externals for
// the whole occlusion_watch.obj (/Gy is defeated by the default /ZI debug
// format), so the five MultiMonitor symbols Check references must exist at
// link time. multi_monitor.cpp is deliberately NOT linked (it drags in
// DesktopInjector + MpvRenderer + libmpv). Any accidental call aborts
// loudly instead of silently passing.
void MultiMonitor::PauseSlot(size_t, bool) { std::abort(); }
bool MultiMonitor::IsSlotPaused(size_t) const { std::abort(); }
std::vector<int> MultiMonitor::monitor_ids() const { std::abort(); }
MultiMonitorMode MultiMonitor::mode() const { std::abort(); }
SpanGeometry MultiMonitor::GetSpanGeometry() noexcept { std::abort(); }
}  // namespace k6wp

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

bool Near(double a, double b, double eps = 1e-9) {
  return std::fabs(a - b) <= eps;
}

k6wp::CoverRect R(int l, int t, int r, int b) {
  k6wp::CoverRect rc;
  rc.left = l;
  rc.top = t;
  rc.right = r;
  rc.bottom = b;
  return rc;
}

// 1920x1080 monitor at origin: area 2073600 (exact integer percentages below).
const k6wp::CoverRect kMon = R(0, 0, 1920, 1080);

void TestClipRect() {
  const k6wp::CoverRect cut =
      k6wp::ClipRect(R(1720, 880, 2120, 1280), kMon);
  Check(cut.left == 1720 && cut.top == 880 && cut.right == 1920 &&
            cut.bottom == 1080,
        "clip: spanning rect clipped to monitor");
  const k6wp::CoverRect dis = k6wp::ClipRect(R(2000, 0, 2100, 100), kMon);
  Check(dis.right <= dis.left || dis.bottom <= dis.top,
        "clip: disjoint rect is empty");
}

void TestOverlapUnion() {
  // Two 1000x1000 windows overlapping over 500x1000. Union = 1500000;
  // a naive sum-area implementation returns 2000000 (double-counts).
  const k6wp::CoverRect rects[] = {R(0, 0, 1000, 1000),
                                   R(500, 0, 1500, 1000)};
  const long long area =
      k6wp::UnionAreaClipped(rects, 2, R(0, 0, 2000, 2000));
  Check(area == 1500000LL, "union: overlap counted once (1500000)");
  Check(area != 2000000LL, "union: not the naive sum (2000000)");
  // Three-way overlap: third window inside the union adds nothing.
  const k6wp::CoverRect rects3[] = {R(0, 0, 1000, 1000),
                                    R(500, 0, 1500, 1000),
                                    R(200, 200, 600, 600)};
  Check(k6wp::UnionAreaClipped(rects3, 3, R(0, 0, 2000, 2000)) == 1500000LL,
        "union: fully-contained third window adds zero");
  // Empty / null input covers nothing.
  Check(k6wp::UnionAreaClipped(nullptr, 0, R(0, 0, 2000, 2000)) == 0LL,
        "union: empty input is zero");
}

void TestMultiMonitorClipping() {
  // Window spanning from monitor 0 into monitor 1 (x >= 1920): only the
  // 200x200 overlap counts toward monitor 0 (40000 / 2073600).
  const k6wp::CoverRect span[] = {R(1720, 880, 2120, 1280)};
  const double cov = k6wp::ComputeCoverage(kMon, span, 1);
  Check(Near(cov, 40000.0 / 2073600.0),
        "coverage: cross-monitor window clipped to 40000/2073600");
  // Window entirely on the second monitor: zero coverage here.
  const k6wp::CoverRect other[] = {R(1920, 0, 3840, 1080)};
  Check(k6wp::ComputeCoverage(kMon, other, 1) == 0.0,
        "coverage: other-monitor window is zero");
  // Degenerate monitor: zero, never NaN.
  const double deg =
      k6wp::ComputeCoverage(R(0, 0, 0, 0), span, 1);
  Check(deg == 0.0, "coverage: degenerate monitor is zero");
}

void TestHysteresisDirect() {
  using k6wp::HysteresisDecide;
  Check(!HysteresisDecide(0.949, false), "hysteresis: 94.9% unpaused stays");
  Check(HysteresisDecide(0.95, false), "hysteresis: 95.0% unpaused pauses");
  Check(HysteresisDecide(0.90, true), "hysteresis: 90.0% paused stays");
  Check(!HysteresisDecide(0.899, true), "hysteresis: 89.9% paused resumes");
  Check(!HysteresisDecide(0.92, false),
        "hysteresis: 92% unpaused holds (no flap up)");
  Check(HysteresisDecide(0.92, true),
        "hysteresis: 92% paused holds (no flap down)");
}

void TestHysteresisViaCoverage() {
  using k6wp::ComputeCoverage;
  using k6wp::HysteresisDecide;
  // Exact-integer areas: 1920*1026 = 1969920 = 95.0% of 2073600;
  // 1920*1025 = 1968000 -> 94.907% (no pause); 1920*972 = 1866240 =
  // 90.0% (stays paused); 1920*971 = 1864320 -> 89.913% (resume).
  const k6wp::CoverRect at95[] = {R(0, 0, 1920, 1026)};
  const k6wp::CoverRect below95[] = {R(0, 0, 1920, 1025)};
  const k6wp::CoverRect at90[] = {R(0, 0, 1920, 972)};
  const k6wp::CoverRect below90[] = {R(0, 0, 1920, 971)};
  const double c95 = ComputeCoverage(kMon, at95, 1);
  const double cBelow95 = ComputeCoverage(kMon, below95, 1);
  const double c90 = ComputeCoverage(kMon, at90, 1);
  const double cBelow90 = ComputeCoverage(kMon, below90, 1);
  Check(c95 >= 0.95, "coverage: 1920x1026 reaches 95.0%");
  Check(cBelow95 < 0.95, "coverage: 1920x1025 stays below 95%");
  Check(HysteresisDecide(c95, false), "e2e: 95.0% coverage pauses slot");
  Check(!HysteresisDecide(cBelow95, false),
        "e2e: 94.9% coverage does not pause slot");
  Check(HysteresisDecide(c90, true), "e2e: 90.0% coverage stays paused");
  Check(!HysteresisDecide(cBelow90, true),
        "e2e: 89.9% coverage resumes slot");
}

void TestSelfSuspend() {
  // Armed-flag logic only — no live windows, no timers, no Check().
  // Any pause owner active -> disarmed (tick self-suspends with the
  // INFINITE loop wait); mask back to zero -> re-armed. Null notify
  // window: flag still flips, no PostMessage crash.
  k6wp::OcclusionWatch watch;
  Check(watch.armed(), "self-suspend: armed by default");
  watch.OnPauseMaskChanged(true);
  Check(!watch.armed(), "self-suspend: pause owner disarms the tick");
  watch.OnPauseMaskChanged(false);
  Check(watch.armed(), "self-suspend: mask-zero re-arms the tick");
  watch.OnRearmMessage();  // loop-thread consume path: harmless no-op
  Check(watch.armed(), "self-suspend: rearm message keeps armed");
}

// --qa mode: print the union area for fixed fixtures as `qa <name>
// area=<value>` lines. Deterministic, byte-comparable output used to prove
// two sort implementations yield identical union results on the same input.
void QaPrintUnion(const char* name, const std::vector<k6wp::CoverRect>& rects,
                  k6wp::CoverRect clip) {
  std::cout << "qa " << name << " area="
            << k6wp::UnionAreaClipped(rects.data(), rects.size(), clip)
            << "\n";
}

void RunQa() {
  // Existing test fixtures (same rects as TestOverlapUnion /
  // TestMultiMonitorClipping).
  QaPrintUnion("overlap", {R(0, 0, 1000, 1000), R(500, 0, 1500, 1000)},
               R(0, 0, 2000, 2000));
  QaPrintUnion("three-way",
               {R(0, 0, 1000, 1000), R(500, 0, 1500, 1000),
                R(200, 200, 600, 600)},
               R(0, 0, 2000, 2000));
  QaPrintUnion("cross-monitor", {R(1720, 880, 2120, 1280)}, kMon);
  // Stress: 40 overlapping 300x300 rects on a 100px stride grid — exercises
  // the x-edge and per-slab interval sorts with many elements.
  std::vector<k6wp::CoverRect> grid;
  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 10; ++j) {
      grid.push_back(R(i * 100, j * 100, i * 100 + 300, j * 100 + 300));
    }
  }
  QaPrintUnion("grid-40", grid, R(0, 0, 2000, 2000));
  // Duplicate x-edges: rects sharing left/right boundaries.
  QaPrintUnion("shared-x",
               {R(0, 0, 500, 100), R(0, 100, 500, 200), R(0, 50, 500, 150)},
               R(0, 0, 1000, 1000));
  // Negative coordinates (packed int64 keys sort by signed top first).
  QaPrintUnion("negative", {R(-500, -500, 0, 0), R(-250, -250, 250, 250)},
               R(-1000, -1000, 1000, 1000));
  // Fully-contained rects add nothing to the union.
  QaPrintUnion("contained",
               {R(0, 0, 1000, 1000), R(100, 100, 200, 200),
                R(300, 300, 400, 400)},
               R(0, 0, 2000, 2000));
}

}  // namespace

int main(int argc, char** argv) {
  if (argc > 1 && std::string(argv[1]) == "--qa") {
    RunQa();
    return 0;
  }
  TestClipRect();
  TestOverlapUnion();
  TestMultiMonitorClipping();
  TestHysteresisDirect();
  TestHysteresisViaCoverage();
  TestSelfSuspend();
  std::cout << "occlusion_test: " << g_checks << " checks, " << g_failures
            << " failures\n";
  return g_failures == 0 ? 0 : 1;
}
