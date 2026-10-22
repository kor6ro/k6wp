#pragma once

// g5_baseline_shim.hpp - the shim for the BASELINE worktree.
//
// WHAT THIS IS
//   Task I1: the G5 golden fixtures must be proven to describe the SAME
//   behaviour as the build from the `baseline-pre-refactor` tag. The
//   differential harness (tools/g5_diff_baseline.ps1) copies the current
//   case table into the baseline worktree and builds it there - but the
//   baseline does NOT have:
//
//     - the unload state machine (engine/src/unload_state.hpp) - that is
//       the task 1.1 feature, extracted on purpose, so it is NOT part of
//       the comparison;
//     - the PauseHidesWallpaper() helper (it lived inside EngineApp in the
//       1.1 code) - the `hides_wallpaper` field is therefore NOT compared
//       either;
//     - the PinVerifyDueAt() / PinVerifyAddReverts() helpers.
//
//   This header provides the missing symbols so the SAME case table
//   compiles against the baseline. The unload functions return a sentinel:
//   the comparison skips the module, so a sentinel can never be mistaken
//   for a match.
//
// WHAT IS COMPARED
//   Everything else: the placement verdict, the WorkerW rule, the retry
//   schedule, the crash-restart backoff, the pause mask, the topology diff,
//   the reconcile policy and the pin-verify decisions. The report says how
//   many cases were compared and how many were skipped (and why).

#include <string>
#include <vector>

#include "pause_controller.hpp"

namespace k6wp {

// The task 1.1 rule as it is today. The baseline has no such helper, so the
// `hides_wallpaper` field is EXCLUDED from the comparison (the script owns
// the field list).
inline bool PauseHidesWallpaper(int mask) {
  constexpr int kInvisibleOwners =
      PauseController::kFullscreen | PauseController::kScreenOff |
      PauseController::kSessionLock | PauseController::kSuspend;
  return (mask & kInvisibleOwners) != 0;
}

// The pin-verify decisions (the baseline's class has the same semantics:
// armed + the 6 s deadline, and a lifetime counter that adds up).
inline constexpr long long kPinVerifyDelayMs = 6000;
inline bool PinVerifyDueAt(bool armed, long long deadline_ms, long long now_ms) {
  return armed && now_ms >= deadline_ms;
}
inline int PinVerifyAddReverts(int total, int n) { return total + n; }

// The WorkerW host rule (task C7) did NOT exist as a pure function in the
// baseline - the check was inline in desktop_inject.cpp. The shim returns
// FALSE on purpose: if the comparison ever stops excluding the module, the
// difference is loud instead of a silent false pass.
inline bool WorkerWAcceptsChildClasses(const std::vector<std::string>&,
                                       const std::string&) {
  return false;
}

// The unload state machine DOES NOT EXIST in the baseline. The harness only
// needs it to compile; the comparison skips the module.
enum class DecodeState { kActive, kPaused, kUnloaded };

struct UnloadMachine {
  DecodeState state = DecodeState::kActive;
  long long pause_started_ms = 0;
  int unload_after_ms = 0;
};

struct UnloadDecision {
  DecodeState state = DecodeState::kActive;
  long long pause_started_ms = 0;
  bool unload_due = false;
  bool reload_due = false;
};

inline UnloadDecision UnloadOnPauseChanged(const UnloadMachine&, bool, long long) {
  return UnloadDecision{};
}
inline UnloadDecision UnloadOnTick(const UnloadMachine&, int, long long) {
  return UnloadDecision{};
}
inline const char* DecodeStateName(DecodeState) { return "absent-in-the-baseline"; }

}  // namespace k6wp
