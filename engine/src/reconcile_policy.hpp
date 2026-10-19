#pragma once

// Reconcile policy - the pure decision layer of the display-resilience
// pipeline (Bug B: freeze/crash when the monitor count drops).
//
// WHY THIS EXISTS
// The engine used to run the WHOLE slot reconciliation inside the
// WM_DISPLAYCHANGE handler:
//
//   HandleMessage(WM_DISPLAYCHANGE)
//     -> EngineApp::OnDisplayChange
//        -> MultiMonitor::OnDisplayChange
//           -> ApplyActiveFilter  (ListMonitors + ResolveHostForPass)
//           -> slots_.erase(...)  (-> ~MpvRenderer -> mpv_terminate_destroy)
//           -> DesktopInjector::Detach/Attach
//           -> MpvRenderer::SetHWND / LoadLoop
//
// WM_DISPLAYCHANGE is delivered with SendMessage by the system, so a slow
// or blocking handler stalls the whole display-change broadcast for the
// session. The observed symptom was "the tray icon is still there but the
// app does not respond" - a hang, not a crash.
//
// This module holds the DECISION part of the fix, windows.h-free and free
// of engine state, so it can be unit-tested headless:
//
//   * DecideReconcileAction(trigger) -> what the debounced door must do
//   * TopologyStable(a, b)           -> has the monitor enumeration settled?
//   * SlotIdentityKey(mi)            -> the stable identity of a monitor
//   * TopologySignature(monitors)    -> comparable snapshot of a topology
//
// The Windows side (the debounced door itself) lives in engine_app.cpp.

#include <string>
#include <vector>

#include "monitor_util.hpp"

namespace k6wp {

// What the debounced reconcile door does when it opens.
enum class ReconcileAction {
  // Nothing to do for this trigger.
  kNone,
  // Re-enumerate and CONVERGE the slot set: tear down slots whose monitor
  // vanished, attach newcomers, refresh the survivors' geometry. This is the
  // "the monitor set changed" action.
  kConvergeSlots,
  // RE-ANCHOR the live surfaces: keep the surfaces that are still alive
  // (and their streams), re-create the ones that died with the desktop.
  // This is the "the desktop/power state changed" action.
  kReanchorSlots,
};

// Pure mapping from a trigger token to the action the door runs. Unknown
// tokens map to kNone (never throws, never touches a slot).
ReconcileAction DecideReconcileAction(const std::string& trigger);

// Stable, log-friendly name of an action ("none" / "converge" / "reanchor").
const char* ReconcileActionName(ReconcileAction action);

// True when the action needs the monitor enumeration to have settled before
// any slot is touched (resume / unlock / Explorer restart: the topology can
// still be moving when the event arrives). A display change does NOT need
// it - the topology already changed and convergence is the point.
bool ReconcileActionNeedsStableTopology(ReconcileAction action);

// Stable identity of one monitor, used to match slots across a topology
// change instead of by position index:
//
//   1. the tiered stable_key ("ccd:..." / "edid:..." / "gdi:...") when the
//      enrichment pass filled it in;
//   2. the GDI device name ("\\.\DISPLAY2") as the fallback;
//   3. a synthetic positional key ("pos:<n>") when even the device name is
//      missing (never empty - an empty identity would match every monitor).
std::string SlotIdentityKey(const MonitorInfo& mi);

// True when two enumerations describe the SAME topology: the same monitor
// set (matched by identity), the same geometry and the same primary.
//
// Matching by identity is what keeps a survivor bound to its own monitor
// when the primary changes or a middle monitor is unplugged: the old
// position-index matching handed monitor A's renderer to monitor B.
//
// The comparison is order-insensitive (the enumerations are sorted, but a
// caller may pass them in any order) and never throws.
bool TopologyStable(const std::vector<MonitorInfo>& first,
                    const std::vector<MonitorInfo>& second);

// A comparable snapshot of a topology: identity + geometry + primary per
// monitor, joined in a stable order. Two enumerations with the same
// signature describe the same topology. Empty for an empty topology.
std::string TopologySignature(const std::vector<MonitorInfo>& monitors);

}  // namespace k6wp
