// reconcile_policy_test.cpp - unit tests for the Bug B reconcile policy.
//
// The policy module (engine/src/reconcile_policy.hpp) is the pure decision
// core of the debounced topology door:
//
//   * DecideReconcileAction(trigger)         -> what the door does
//   * ReconcileActionNeedsStableTopology(a)  -> does it wait for the
//                                               enumeration to settle?
//   * SlotIdentityKey(mi)                     -> the stable identity a slot
//                                               is bound to
//   * TopologyStable(a, b) / TopologySignature(monitors)
//                                             -> has the topology settled?
//
// All of it is windows.h-free and free of engine state, so the whole file
// runs headless.
//
// No external test framework: plain Check() counter; exit 0 = pass.
#include <cstdio>
#include <string>
#include <vector>

#include "reconcile_policy.hpp"

namespace {

int g_checks = 0;
int g_failures = 0;

void Check(bool cond, const char* name) {
  ++g_checks;
  if (cond) {
    std::printf("[PASS] %s\n", name);
  } else {
    ++g_failures;
    std::printf("[FAIL] %s\n", name);
  }
}

k6wp::MonitorInfo MakeMon(int id, int x, int y, int w, int h, bool primary,
                           const wchar_t* device, const char* stable_key) {
  k6wp::MonitorInfo m;
  m.id = id;
  m.x = x;
  m.y = y;
  m.width = w;
  m.height = h;
  m.is_primary = primary;
  m.device_name = device;
  m.stable_key = stable_key;
  return m;
}

// Two side-by-side monitors with a TIER-1 (ccd) identity - the production
// case: the identity survives a port change.
std::vector<k6wp::MonitorInfo> TwoStableMonitors() {
  return {
      MakeMon(0, 0, 0, 1920, 1080, true, L"\\\\.\\DISPLAY1",
              "ccd:\\\\?\\DISPLAY#DEL40A2"),
      MakeMon(1, 1920, 0, 1920, 1080, false, L"\\\\.\\DISPLAY2",
              "ccd:\\\\?\\DISPLAY#DEL40A3"),
  };
}

void TestActionMapping() {
  using k6wp::ReconcileAction;
  Check(k6wp::DecideReconcileAction("display-change") ==
            ReconcileAction::kConvergeSlots,
        "policy: display-change converges the slot set");
  Check(k6wp::DecideReconcileAction("monitor-on") ==
            ReconcileAction::kConvergeSlots,
        "policy: monitor-on converges the slot set");
  Check(k6wp::DecideReconcileAction("device-lost") ==
            ReconcileAction::kConvergeSlots,
        "policy: device-lost converges the slot set");
  Check(k6wp::DecideReconcileAction("resume") == ReconcileAction::kReanchorSlots,
        "policy: resume re-anchors the live surfaces");
  Check(k6wp::DecideReconcileAction("session-unlock") ==
            ReconcileAction::kReanchorSlots,
        "policy: session-unlock re-anchors the live surfaces");
  Check(k6wp::DecideReconcileAction("taskbar-created") ==
            ReconcileAction::kReanchorSlots,
        "policy: taskbar-created re-anchors the live surfaces");
  Check(k6wp::DecideReconcileAction("setting-change") ==
            ReconcileAction::kReanchorSlots,
        "policy: setting-change re-anchors the live surfaces");
  Check(k6wp::DecideReconcileAction("nonsense") == ReconcileAction::kNone,
        "policy: an unknown trigger does nothing");
  Check(k6wp::DecideReconcileAction("") == ReconcileAction::kNone,
        "policy: an empty trigger does nothing");

  Check(std::string(k6wp::ReconcileActionName(ReconcileAction::kConvergeSlots)) ==
            "converge",
        "policy: the converge action has a stable name");
  Check(std::string(k6wp::ReconcileActionName(ReconcileAction::kReanchorSlots)) ==
            "reanchor",
        "policy: the reanchor action has a stable name");
  Check(std::string(k6wp::ReconcileActionName(ReconcileAction::kNone)) == "none",
        "policy: the none action has a stable name");
}

void TestStableTopologyWait() {
  using k6wp::ReconcileAction;
  Check(k6wp::ReconcileActionNeedsStableTopology(ReconcileAction::kReanchorSlots),
        "policy: the re-anchor actions wait for a settled enumeration");
  Check(!k6wp::ReconcileActionNeedsStableTopology(
            ReconcileAction::kConvergeSlots),
        "policy: the converge action does NOT wait (a display change IS the "
        "topology change - waiting would only delay the convergence)");
  Check(!k6wp::ReconcileActionNeedsStableTopology(ReconcileAction::kNone),
        "policy: the none action never waits");
}

void TestSlotIdentityKey() {
  const auto monitors = TwoStableMonitors();
  Check(k6wp::SlotIdentityKey(monitors[0]) == "ccd:\\\\?\\DISPLAY#DEL40A2",
        "identity: the tiered stable_key wins");
  // No stable_key -> the GDI device name is the fallback.
  const k6wp::MonitorInfo gdi =
      MakeMon(3, 0, 0, 1920, 1080, true, L"\\\\.\\DISPLAY7", "");
  Check(k6wp::SlotIdentityKey(gdi) == "gdi:\\\\.\\DISPLAY7",
        "identity: the GDI device name is the fallback");
  // Neither -> a synthetic positional key (never empty: an empty identity
  // would match EVERY monitor).
  const k6wp::MonitorInfo bare = MakeMon(4, 0, 0, 1920, 1080, true, L"", "");
  Check(!k6wp::SlotIdentityKey(bare).empty(),
        "identity: the positional fallback is never empty");
  Check(k6wp::SlotIdentityKey(bare) != k6wp::SlotIdentityKey(gdi),
        "identity: different monitors get different identities");
}

void TestTopologyStability() {
  const auto a = TwoStableMonitors();
  auto b = TwoStableMonitors();
  Check(k6wp::TopologyStable(a, b),
        "stability: two identical enumerations are stable");

  // The same monitors, but the SECOND one is unplugged.
  auto fewer = a;
  fewer.pop_back();
  Check(!k6wp::TopologyStable(a, fewer),
        "stability: a monitor that disappeared is NOT stable");
  Check(!k6wp::TopologyStable(fewer, a),
        "stability: the comparison is symmetric");

  // The same monitor set, but the geometry moved (a resolution change).
  auto moved = TwoStableMonitors();
  moved[1].x = 2000;
  Check(!k6wp::TopologyStable(a, moved),
        "stability: a geometry change is NOT stable");

  // The same monitor set, but the primary moved.
  auto swapped = TwoStableMonitors();
  swapped[0].is_primary = false;
  swapped[1].is_primary = true;
  Check(!k6wp::TopologyStable(a, swapped),
        "stability: a primary change is NOT stable");

  // A DIFFERENT device with the same rect is a different topology.
  auto replaced = TwoStableMonitors();
  replaced[1].stable_key = "ccd:\\\\?\\DISPLAY#OTHER";
  Check(!k6wp::TopologyStable(a, replaced),
        "stability: a replaced device is NOT stable");

  // Order must not matter: the comparison is by identity, not by position.
  auto reordered = TwoStableMonitors();
  std::swap(reordered[0], reordered[1]);
  Check(k6wp::TopologyStable(a, reordered),
        "stability: the order of the enumeration does not matter");
}

void TestTopologySignature() {
  const auto a = TwoStableMonitors();
  auto reordered = TwoStableMonitors();
  std::swap(reordered[0], reordered[1]);
  Check(k6wp::TopologySignature(a) == k6wp::TopologySignature(reordered),
        "signature: the same topology has the same signature (order free)");

  auto changed = TwoStableMonitors();
  changed[1].width = 2560;
  Check(k6wp::TopologySignature(a) != k6wp::TopologySignature(changed),
        "signature: a different topology has a different signature");
  Check(k6wp::TopologySignature({}).empty(),
        "signature: an empty topology has an empty signature");
}

}  // namespace

int main() {
  TestActionMapping();
  TestStableTopologyWait();
  TestSlotIdentityKey();
  TestTopologyStability();
  TestTopologySignature();
  std::printf("%d checks, %d failures\n", g_checks, g_failures);
  return g_failures == 0 ? 0 : 1;
}
