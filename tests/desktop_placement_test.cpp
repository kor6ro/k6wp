// Unit tests for engine/src/desktop_placement pure geometry helpers.
// No external test framework: plain asserts with a pass/fail counter,
// same style as tests/config_test.cpp. Exit code 0 = all pass.
//
// Synthetic rects only — no live windows, no Win32 calls. Covers:
// HostClientOffset (the desktop_inject.cpp:261-265 inline math), the
// ChildRectInClient mapping, all six CoverageVerdicts, and the stable
// greppable CoverageReason tokens consumed by later rows (`placement:`).
#include <iostream>
#include <string>

#include "desktop_placement.hpp"

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

k6wp::PlacementRect R(int l, int t, int r, int b) {
  k6wp::PlacementRect rc;
  rc.left = l;
  rc.top = t;
  rc.right = r;
  rc.bottom = b;
  return rc;
}

bool SameRect(const k6wp::PlacementRect& a, const k6wp::PlacementRect& b) {
  return a.left == b.left && a.top == b.top && a.right == b.right &&
         a.bottom == b.bottom;
}

void TestHappyPath() {
  // QA happy: host (0,0,1920,1080) + monitor (1920,0,3840,1080).
  const k6wp::PlacementRect host = R(0, 0, 1920, 1080);
  const k6wp::PlacementRect mon = R(1920, 0, 3840, 1080);
  const k6wp::ClientOffset off = k6wp::HostClientOffset(host, mon);
  Check(off.dx == 1920 && off.dy == 0, "offset: secondary monitor -> (1920,0)");
  const k6wp::PlacementRect host_client = R(0, 0, 1920, 1080);
  const k6wp::PlacementRect child =
      k6wp::ChildRectInClient(off, mon, host_client);
  Check(SameRect(child, R(1920, 0, 3840, 1080)),
        "child: placed exactly on the monitor rect");
  const k6wp::PlacementRect mon_as_client =
      R(mon.left - host.left, mon.top - host.top, mon.right - host.left,
        mon.bottom - host.top);
  Check(k6wp::CoversMonitor(child, mon_as_client) ==
            k6wp::CoverageVerdict::kCovered,
        "cover: happy placement is kCovered");
  Check(std::string(k6wp::CoverageReason(k6wp::CoverageVerdict::kCovered)) ==
            "placement: covered",
        "reason: kCovered -> \"placement: covered\"");
}

void TestClippedLeft() {
  // QA failure: host covers only the primary, monitor at x=-1920. The child
  // is placed from the host WINDOW origin while the monitor is mapped from
  // the host CLIENT origin (frame inset dx=8): the 8 px mismatch uncovers the
  // monitor's left strip -> kClippedLeft.
  const k6wp::PlacementRect host_window = R(0, 0, 1920, 1080);
  const k6wp::PlacementRect mon = R(-1920, 0, 0, 1080);
  const k6wp::ClientOffset off = k6wp::HostClientOffset(host_window, mon);
  Check(off.dx == -1920 && off.dy == 0,
        "offset: left monitor -> (-1920,0)");
  const k6wp::PlacementRect host_client = R(0, 0, 1920, 1080);
  const k6wp::PlacementRect child =
      k6wp::ChildRectInClient(off, mon, host_client);
  Check(SameRect(child, R(-1920, 0, 0, 1080)),
        "child: left monitor maps to (-1920,0,0,1080)");
  // Monitor mapped from an 8 px-inset client origin (window frame): every
  // edge sits 8 px left of the window-origin mapping, so the child misses
  // the left strip.
  const k6wp::PlacementRect mon_as_client = R(-1928, 0, -8, 1080);
  Check(k6wp::CoversMonitor(child, mon_as_client) ==
            k6wp::CoverageVerdict::kClippedLeft,
        "cover: frame-inset mismatch is kClippedLeft");
  Check(std::string(k6wp::CoverageReason(
            k6wp::CoverageVerdict::kClippedLeft)) ==
            "placement: CLIPPED-left",
        "reason: kClippedLeft -> \"placement: CLIPPED-left\"");
}

void TestRemainingVerdicts() {
  const k6wp::PlacementRect mon = R(0, 0, 1920, 1080);
  Check(k6wp::CoversMonitor(R(0, 100, 1920, 1080), mon) ==
            k6wp::CoverageVerdict::kClippedTop,
        "cover: top strip missing is kClippedTop");
  Check(std::string(k6wp::CoverageReason(
            k6wp::CoverageVerdict::kClippedTop)) == "placement: CLIPPED-top",
        "reason: kClippedTop -> \"placement: CLIPPED-top\"");
  Check(k6wp::CoversMonitor(R(0, 0, 1800, 1080), mon) ==
            k6wp::CoverageVerdict::kClippedRight,
        "cover: right strip missing is kClippedRight");
  Check(std::string(k6wp::CoverageReason(
            k6wp::CoverageVerdict::kClippedRight)) ==
            "placement: CLIPPED-right",
        "reason: kClippedRight -> \"placement: CLIPPED-right\"");
  Check(k6wp::CoversMonitor(R(0, 0, 1920, 1000), mon) ==
            k6wp::CoverageVerdict::kClippedBottom,
        "cover: bottom strip missing is kClippedBottom");
  Check(std::string(k6wp::CoverageReason(
            k6wp::CoverageVerdict::kClippedBottom)) ==
            "placement: CLIPPED-bottom",
        "reason: kClippedBottom -> \"placement: CLIPPED-bottom\"");
  // Monitor fully outside the host: disjoint child -> kOutOfBounds.
  Check(k6wp::CoversMonitor(R(1920, 0, 3840, 1080), mon) ==
            k6wp::CoverageVerdict::kOutOfBounds,
        "cover: disjoint monitor is kOutOfBounds");
  Check(std::string(k6wp::CoverageReason(
            k6wp::CoverageVerdict::kOutOfBounds)) ==
            "placement: OUT-OF-BOUNDS",
        "reason: kOutOfBounds -> \"placement: OUT-OF-BOUNDS\"");
}

}  // namespace

int main() {
  TestHappyPath();
  TestClippedLeft();
  TestRemainingVerdicts();
  std::cout << "desktop_placement_test: " << g_checks << " checks, "
            << g_failures << " failures\n";
  return g_failures == 0 ? 0 : 1;
}
