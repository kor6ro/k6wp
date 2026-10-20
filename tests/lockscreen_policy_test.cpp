// lockscreen_policy_test.cpp - unit tests for the Bug A lock-screen policy.
//
// Three pure decision layers, all headless:
//
//   1. the TRIGGER policy (shared/lockscreen.cpp): which events refresh the
//      lock screen, which of them bypass the debounce, and which one waits
//      for the frame to land;
//   2. the ELEVATION classification (shared/lockscreen_bridge.cpp): the
//      launcher's exit code -> the reason the Studio shows;
//   3. the EDITION classification (shared/lockscreen_edition.cpp): the
//      Windows edition -> how well the LockScreenImage policy is supported.
//
// No external test framework: plain Check() counter; exit 0 = pass.
#include <cstdio>
#include <string>

#include "lockscreen.hpp"
#include "lockscreen_bridge.hpp"
#include "lockscreen_edition.hpp"

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

void TestTriggerPolicy() {
  using k6wp::LockscreenTrigger;
  // Every trigger refreshes (the engine decides WHEN, the helper decides
  // WHETHER the feature is on at all).
  Check(k6wp::ShouldRefreshLockscreen(LockscreenTrigger::kVideoChanged),
        "trigger: a video change refreshes");
  Check(k6wp::ShouldRefreshLockscreen(LockscreenTrigger::kRotation),
        "trigger: a playlist rotation refreshes");
  Check(k6wp::ShouldRefreshLockscreen(LockscreenTrigger::kSessionLock),
        "trigger: a session lock refreshes");
  Check(k6wp::ShouldRefreshLockscreen(LockscreenTrigger::kSuspend),
        "trigger: a suspend refreshes");
  Check(k6wp::ShouldRefreshLockscreen(LockscreenTrigger::kShutdown),
        "trigger: a shutdown refreshes");
  Check(k6wp::ShouldRefreshLockscreen(LockscreenTrigger::kMonitorOn),
        "trigger: a monitor-on refreshes");

  // The debounce may swallow a video switch, but never the events where the
  // picture has to be current.
  Check(!k6wp::LockscreenTriggerBypassesDebounce(
            LockscreenTrigger::kVideoChanged),
        "debounce: a video change is debounced");
  Check(!k6wp::LockscreenTriggerBypassesDebounce(LockscreenTrigger::kRotation),
        "debounce: a rotation is debounced (throttled)");
  Check(k6wp::LockscreenTriggerBypassesDebounce(LockscreenTrigger::kSessionLock),
        "debounce: a session lock bypasses the debounce");
  Check(k6wp::LockscreenTriggerBypassesDebounce(LockscreenTrigger::kSuspend),
        "debounce: a suspend bypasses the debounce");
  Check(k6wp::LockscreenTriggerBypassesDebounce(LockscreenTrigger::kShutdown),
        "debounce: a shutdown bypasses the debounce");

  // Only the shutdown path waits: the frame must be on disk before the
  // session ends. Everything else stays fire-and-forget.
  Check(k6wp::LockscreenTriggerWaits(LockscreenTrigger::kShutdown),
        "wait: a shutdown waits for the frame");
  Check(!k6wp::LockscreenTriggerWaits(LockscreenTrigger::kSessionLock),
        "wait: a session lock does not block on the frame");
  Check(!k6wp::LockscreenTriggerWaits(LockscreenTrigger::kSuspend),
        "wait: a suspend does not block on the frame");
  Check(!k6wp::LockscreenTriggerWaits(LockscreenTrigger::kVideoChanged),
        "wait: a video change does not block on the frame");
}

void TestElevationClassification() {
  using k6wp::LockscreenElevationFailure;
  using k6wp::LockscreenElevationResult;

  LockscreenElevationResult ok;
  ok.ran = true;
  ok.exit_code = k6wp::kExitOk;
  Check(k6wp::ClassifyLockscreenElevation(ok) ==
            LockscreenElevationFailure::kNone,
        "elevation: a successful registration is no failure");

  LockscreenElevationResult declined;
  declined.ran = true;
  declined.exit_code = k6wp::kExitElevateDeclined;
  Check(k6wp::ClassifyLockscreenElevation(declined) ==
            LockscreenElevationFailure::kDeclined,
        "elevation: a declined UAC prompt is reported as declined");

  LockscreenElevationResult timed_out;
  timed_out.ran = true;
  timed_out.exit_code = k6wp::kExitElevateTimeout;
  Check(k6wp::ClassifyLockscreenElevation(timed_out) ==
            LockscreenElevationFailure::kTimedOut,
        "elevation: an expired helper wait is reported as a timeout");

  LockscreenElevationResult spawn_failed;
  spawn_failed.ran = false;
  spawn_failed.exit_code = k6wp::kExitSpawnFailure;
  Check(k6wp::ClassifyLockscreenElevation(spawn_failed) ==
            LockscreenElevationFailure::kSpawnFailed,
        "elevation: a missing launcher is reported as a spawn failure");

  LockscreenElevationResult unknown;
  unknown.ran = true;
  unknown.exit_code = 42;
  Check(k6wp::ClassifyLockscreenElevation(unknown) ==
            LockscreenElevationFailure::kUnknown,
        "elevation: an unknown exit code is reported as unknown");

  // The toggle only turns ON when the registration really succeeded.
  Check(ok.ok(), "elevation: the result reports success only for exit code 0");
  Check(!declined.ok(), "elevation: a declined prompt is not a success");
  Check(!spawn_failed.ok(), "elevation: a spawn failure is not a success");
  Check(declined.declined(),
        "elevation: a declined prompt is recognisable on its own");
}

void TestEditionClassification() {
  using k6wp::LockscreenEditionSupport;
  // Enterprise / Education: the policy is officially supported.
  Check(k6wp::ClassifyLockscreenEdition("Enterprise") ==
            LockscreenEditionSupport::kSupported,
        "edition: Enterprise supports the lock-screen policy");
  Check(k6wp::ClassifyLockscreenEdition("Education") ==
            LockscreenEditionSupport::kSupported,
        "edition: Education supports the lock-screen policy");
  Check(k6wp::ClassifyLockscreenEdition("EnterpriseS") ==
            LockscreenEditionSupport::kSupported,
        "edition: EnterpriseS supports the lock-screen policy");
  // Pro: NOT officially supported (Intune/CSP is), but it may work.
  Check(k6wp::ClassifyLockscreenEdition("Professional") ==
            LockscreenEditionSupport::kUncertain,
        "edition: Pro is uncertain (the policy is not officially supported)");
  Check(k6wp::ClassifyLockscreenEdition("ProfessionalN") ==
            LockscreenEditionSupport::kUncertain,
        "edition: Pro N is uncertain");
  // Home: not supported.
  Check(k6wp::ClassifyLockscreenEdition("Home") ==
            LockscreenEditionSupport::kUnsupported,
        "edition: Home does not support the lock-screen policy");
  Check(k6wp::ClassifyLockscreenEdition("Core") ==
            LockscreenEditionSupport::kUnsupported,
        "edition: Core does not support the lock-screen policy");
  // Unknown / empty.
  Check(k6wp::ClassifyLockscreenEdition("") ==
            LockscreenEditionSupport::kUnknown,
        "edition: an empty edition id is unknown");
  Check(k6wp::ClassifyLockscreenEdition("SomethingElse") ==
            LockscreenEditionSupport::kUnknown,
        "edition: an unexpected edition id is unknown");
}

}  // namespace

int main() {
  TestTriggerPolicy();
  TestElevationClassification();
  TestEditionClassification();
  std::printf("%d checks, %d failures\n", g_checks, g_failures);
  return g_failures == 0 ? 0 : 1;
}
