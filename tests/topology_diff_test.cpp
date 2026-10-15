// Unit tests for engine/src/topology_diff pure core (display resilience P1,
// prompt v5.1 §5.5). No external test framework: plain asserts with a
// pass/fail counter, same style as tests/desktop_placement_test.cpp.
// Exit code 0 = all pass.
//
// windows.h-free, synthetic snapshots only. Covers the §R7.1 classification
// (added/removed/moved/resized/dpi/orientation/primary + hash_changed), the
// "one key can land in several categories" rule, deterministic sorted order
// and the identical-snapshot idempotency (no-op) contract.
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#include "topology_diff.hpp"

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

k6wp::TopologyMonitor Mon(const std::string& key, int x, int y, int w, int h,
                          int scale = 100, int orientation = 0,
                          bool primary = false) {
  k6wp::TopologyMonitor m;
  m.stable_key = key;
  m.gdi_name = "\\\\.\\DISPLAY_" + key;
  m.x = x;
  m.y = y;
  m.width = w;
  m.height = h;
  m.scale_pct = scale;
  m.orientation = orientation;
  m.is_primary = primary;
  m.degraded = false;
  return m;
}

k6wp::TopologySnapshot Snap(std::vector<k6wp::TopologyMonitor> monitors,
                            const std::string& hash = "h0") {
  k6wp::TopologySnapshot s;
  s.monitors = std::move(monitors);
  s.hash_snapshot = hash;
  return s;
}

bool Has(const std::vector<std::string>& v, const std::string& key) {
  return std::find(v.begin(), v.end(), key) != v.end();
}

bool Sorted(const std::vector<std::string>& v) {
  return std::is_sorted(v.begin(), v.end());
}

void TestIdenticalIsNoOp() {
  const k6wp::TopologySnapshot a = Snap(
      {Mon("ccd:A", 0, 0, 1920, 1080, 100, 0, true),
       Mon("ccd:B", 1920, 0, 1920, 1080)});
  const k6wp::TopologyDiff d = k6wp::diff_topology(a, a);
  Check(d.empty(), "identical: empty() is true");
  Check(d.added_keys.empty() && d.removed_keys.empty() && d.moved_keys.empty() &&
            d.resized_keys.empty() && d.dpi_keys.empty() &&
            d.orientation_keys.empty(),
        "identical: no key lands in any category");
  Check(!d.primary_changed, "identical: primary_changed is false");
  Check(!d.hash_changed, "identical: hash_changed is false");
}

void TestAddedRemoved() {
  const k6wp::TopologySnapshot one =
      Snap({Mon("ccd:A", 0, 0, 1920, 1080)});
  const k6wp::TopologySnapshot two =
      Snap({Mon("ccd:A", 0, 0, 1920, 1080), Mon("ccd:B", 1920, 0, 1920, 1080)});

  const k6wp::TopologyDiff add = k6wp::diff_topology(one, two);
  Check(add.added_keys == std::vector<std::string>{"ccd:B"},
        "added: new key shows up in added_keys");
  Check(add.removed_keys.empty(), "added: nothing removed");

  const k6wp::TopologyDiff rem = k6wp::diff_topology(two, one);
  Check(rem.removed_keys == std::vector<std::string>{"ccd:B"},
        "removed: dropped key shows up in removed_keys");
  Check(rem.added_keys.empty(), "removed: nothing added");
}

void TestMoved() {
  const k6wp::TopologySnapshot a =
      Snap({Mon("ccd:A", 0, 0, 1920, 1080)});
  const k6wp::TopologySnapshot b =
      Snap({Mon("ccd:A", 100, -50, 1920, 1080)});
  const k6wp::TopologyDiff d = k6wp::diff_topology(a, b);
  Check(Has(d.moved_keys, "ccd:A"), "moved: x/y change is classified moved");
  Check(!Has(d.resized_keys, "ccd:A"), "moved: size unchanged is NOT resized");
  Check(!d.empty(), "moved: diff is not empty");
}

void TestResized() {
  const k6wp::TopologySnapshot a =
      Snap({Mon("ccd:A", 0, 0, 1920, 1080)});
  const k6wp::TopologySnapshot b =
      Snap({Mon("ccd:A", 0, 0, 2560, 1440)});
  const k6wp::TopologyDiff d = k6wp::diff_topology(a, b);
  Check(Has(d.resized_keys, "ccd:A"),
        "resized: width/height change is classified resized");
  Check(!Has(d.moved_keys, "ccd:A"), "resized: origin unchanged is NOT moved");
}

void TestDpi() {
  const k6wp::TopologySnapshot a = Snap({Mon("ccd:A", 0, 0, 1920, 1080, 100)});
  const k6wp::TopologySnapshot b = Snap({Mon("ccd:A", 0, 0, 1920, 1080, 150)});
  const k6wp::TopologyDiff d = k6wp::diff_topology(a, b);
  Check(Has(d.dpi_keys, "ccd:A"), "dpi: scale_pct change is classified dpi");
  Check(!Has(d.moved_keys, "ccd:A") && !Has(d.resized_keys, "ccd:A"),
        "dpi: geometry unchanged stays out of moved/resized");
}

void TestOrientation() {
  const k6wp::TopologySnapshot a =
      Snap({Mon("ccd:A", 0, 0, 1080, 1920, 100, 0)});
  const k6wp::TopologySnapshot b =
      Snap({Mon("ccd:A", 0, 0, 1080, 1920, 100, 1)});
  const k6wp::TopologyDiff d = k6wp::diff_topology(a, b);
  Check(Has(d.orientation_keys, "ccd:A"),
        "orientation: rotation is classified orientation");
}

void TestPrimaryChanged() {
  const k6wp::TopologySnapshot a =
      Snap({Mon("ccd:A", 0, 0, 1920, 1080, 100, 0, true),
            Mon("ccd:B", 1920, 0, 1920, 1080, 100, 0, false)});
  const k6wp::TopologySnapshot b =
      Snap({Mon("ccd:A", 0, 0, 1920, 1080, 100, 0, false),
            Mon("ccd:B", 1920, 0, 1920, 1080, 100, 0, true)});
  const k6wp::TopologyDiff d = k6wp::diff_topology(a, b);
  Check(d.primary_changed, "primary: swapping the primary sets the flag");
  Check(!Has(d.moved_keys, "ccd:A") && !Has(d.moved_keys, "ccd:B"),
        "primary: primary swap is not a move");
}

void TestCombinedCategories() {
  const k6wp::TopologySnapshot a =
      Snap({Mon("ccd:A", 0, 0, 1920, 1080, 100, 0)});
  const k6wp::TopologySnapshot b =
      Snap({Mon("ccd:A", 10, 20, 2560, 1440, 150, 2)});
  const k6wp::TopologyDiff d = k6wp::diff_topology(a, b);
  Check(Has(d.moved_keys, "ccd:A") && Has(d.resized_keys, "ccd:A") &&
            Has(d.dpi_keys, "ccd:A") && Has(d.orientation_keys, "ccd:A"),
        "combined: one key can land in several categories at once");
}

void TestSortedDeterministicOrder() {
  // Keys intentionally built out of order; output must be sorted and stable.
  const k6wp::TopologySnapshot a =
      Snap({Mon("z", 0, 0, 100, 100), Mon("m", 0, 0, 100, 100)});
  const k6wp::TopologySnapshot b = Snap({Mon("z", 5, 5, 100, 100),
                                         Mon("m", 5, 5, 100, 100),
                                         Mon("a", 1, 1, 100, 100),
                                         Mon("b", 1, 1, 100, 100)});
  const k6wp::TopologyDiff d1 = k6wp::diff_topology(a, b);
  const k6wp::TopologyDiff d2 = k6wp::diff_topology(a, b);
  Check(Sorted(d1.added_keys) && Sorted(d1.moved_keys),
        "order: added/moved key vectors are sorted");
  Check(d1.added_keys == std::vector<std::string>({"a", "b"}),
        "order: added keys sorted deterministically");
  Check(d1.added_keys == d2.added_keys && d1.moved_keys == d2.moved_keys,
        "order: repeated diff is deterministic");
}

void TestHashChanged() {
  const k6wp::TopologySnapshot a =
      Snap({Mon("ccd:A", 0, 0, 1920, 1080)}, "aaaa");
  const k6wp::TopologySnapshot b =
      Snap({Mon("ccd:A", 0, 0, 1920, 1080)}, "bbbb");
  const k6wp::TopologyDiff d = k6wp::diff_topology(a, b);
  Check(d.hash_changed, "hash: differing hash_snapshot sets hash_changed");
  // A pure global change (same monitors) leaves the key vectors empty but is
  // still observable through hash_changed.
  Check(d.added_keys.empty() && d.removed_keys.empty() && d.moved_keys.empty(),
        "hash: no per-key change when only the hash differs");
}

}  // namespace

int main() {
  TestIdenticalIsNoOp();
  TestAddedRemoved();
  TestMoved();
  TestResized();
  TestDpi();
  TestOrientation();
  TestPrimaryChanged();
  TestCombinedCategories();
  TestSortedDeterministicOrder();
  TestHashChanged();
  std::cout << "topology_diff_test: " << g_checks << " checks, " << g_failures
            << " failures\n";
  return g_failures == 0 ? 0 : 1;
}
