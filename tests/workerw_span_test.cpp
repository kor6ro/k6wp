// Unit tests for the WorkerW strategy-B span gate (plan row 10).
// No external test framework: plain asserts with a pass/fail counter,
// same style as tests/desktop_placement_test.cpp (row 3). Exit 0 = pass.
//
// The predicate is split (decisions.md Task 10): pure containment
// k6wp::WorkerWSpansRect over POD rects (synthetic cases, no Win32) plus a
// thin HWND wrapper k6wp::WorkerWSpansVirtualScreen in desktop_inject.cpp
// (GetWindowRect + the four SM_*VIRTUALSCREEN metrics GetSpanGeometry
// reads). The null-handle case targets the wrapper: it must return false
// without crashing (`if (!w) return false;`).
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <iostream>
#include <string>
#include <vector>

#include "desktop_placement.hpp"

// k6wp::WorkerWSpansVirtualScreen lives in desktop_inject.cpp at k6wp
// namespace scope (the header stays windows.h-free, so no decl there).
namespace k6wp {
bool WorkerWSpansVirtualScreen(HWND hwnd);
}

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

void TestSpansExact() {
  // QA happy case 1: WorkerW (0,0,3840,1080) vs virtual (0,0,3840,1080).
  Check(k6wp::WorkerWSpansRect(R(0, 0, 3840, 1080), R(0, 0, 3840, 1080)),
        "span: exact virtual-screen match -> true");
}

void TestSpansNarrow() {
  // QA happy case 2: WorkerW (0,0,1920,1080) vs a two-wide virtual screen
  // with the second monitor LEFT of primary: virtual (x=-1920,y=0,w=3840,
  // h=1080) i.e. rect (-1920,0,1920,1080). The half-wide WorkerW misses the
  // left half -> false, so strategy B must NOT pick it.
  Check(!k6wp::WorkerWSpansRect(R(0, 0, 1920, 1080), R(-1920, 0, 1920, 1080)),
        "span: half-wide WorkerW vs left-extended virtual -> false");
}

void TestNullHandle() {
  // QA failure case: a null handle must return false without crashing.
  Check(!k6wp::WorkerWSpansVirtualScreen(nullptr),
        "span: nullptr WorkerW -> false (no crash)");
}

// --- C7: the WorkerW host-resolution rule ----------------------------------
//
// Regression guard for the bug where a re-anchor pass resolved the host
// BEFORE the old surface was released: the wallpaper WorkerW still held our
// OWN stale injected window, the "is it empty" test failed, and the engine
// fell back to the layered-Progman-child branch - which sits below the
// shell's DefView and is therefore INVISIBLE. The wallpaper vanished a
// fraction of a second after every start.
void TestWorkerWHostResolution() {
  const std::string own = "K6WP.DesktopInject.1";
  const std::string foreign = "SHELLDLL_DefView";

  // The shell's wallpaper WorkerW: empty -> it IS the host we want.
  Check(k6wp::WorkerWAcceptsChildClasses({}, own),
        "workerw: an empty WorkerW hosts the surface");

  // The re-anchor case: our own stale surface is still parented in it.
  // This is the case the bug got wrong.
  Check(k6wp::WorkerWAcceptsChildClasses({own}, own),
        "workerw: a WorkerW holding ONLY our own stale surface still hosts it");

  // Two of our own surfaces (a double attach) - still ours.
  Check(k6wp::WorkerWAcceptsChildClasses({own, own}, own),
        "workerw: a WorkerW holding only our own surfaces still hosts it");

  // A foreign child means it is NOT the wallpaper WorkerW.
  Check(!k6wp::WorkerWAcceptsChildClasses({foreign}, own),
        "workerw: a WorkerW holding a foreign child is rejected");
  Check(!k6wp::WorkerWAcceptsChildClasses({own, foreign}, own),
        "workerw: our own surface plus a foreign child is rejected");
}

}  // namespace

int main() {
  TestSpansExact();
  TestSpansNarrow();
  TestNullHandle();
  TestWorkerWHostResolution();
  std::cout << "workerw_span_test: " << g_checks << " checks, " << g_failures
            << " failures\n";
  return g_failures == 0 ? 0 : 1;
}
