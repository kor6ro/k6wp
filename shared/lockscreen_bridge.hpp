#pragma once

// Bridge to the launcher's lock-screen policy registration.
//
// WHY THIS EXISTS (Bug A)
// The lock-screen feature has two halves that used to live in different
// processes and were never connected:
//
//   * the ENGINE writes %PROGRAMDATA%\K6WP\lockscreen.jpg (a static frame);
//   * the LAUNCHER registers the HKLM policy
//     (SOFTWARE\Policies\Microsoft\Windows\Personalization\LockScreenImage)
//     that makes Windows actually SHOW that file on the lock screen.
//
// Only the launcher can write the policy (it needs administrator rights), and
// the only caller of `K6WP.exe --elevate-lockscreen on` in the whole tree was
// the UNINSTALLER (the "off" path). So the Studio toggle could turn the
// feature "on" in studio_settings.json while Windows was never told about
// the image: the engine happily refreshed the jpg and the lock screen kept
// showing the Windows default.
//
// This bridge is the missing half: it runs the launcher and reports what
// happened, so the Studio can only leave the toggle ON when the policy is
// really registered.
//
// Header stays windows.h-free (same pattern as the rest of shared/).

#include <string>

#include "lockscreen_exit.hpp"

namespace k6wp {

// The outcome of `K6WP.exe --elevate-lockscreen on|off`.
struct LockscreenElevationResult {
  // False when the launcher could not be found or started at all (no exit
  // code to report).
  bool ran = false;
  // The launcher's exit code (see lockscreen_exit.hpp). Meaningful only
  // when `ran` is true.
  int exit_code = 0;
  // True when the policy is (or is not) in the requested state.
  bool ok() const { return ran && exit_code == kExitOk; }
  // True when the user dismissed the UAC prompt.
  bool declined() const { return ran && exit_code == kExitElevateDeclined; }
};

// WHY a lock-screen elevation failed. The UI copy lives in the Studio (it
// is translated there); this is the pure classification, so the mapping
// "exit code -> reason" is unit-tested.
enum class LockscreenElevationFailure {
  kNone,        // no failure
  kDeclined,    // the user dismissed the UAC prompt
  kTimedOut,    // the elevated helper did not report back in time
  kSpawnFailed, // the launcher is missing or could not be started
  kUnknown,     // the helper ran and failed for an unknown reason
};

// Classifies an elevation outcome. `result.ran == false` with a timeout
// exit code is reported as kTimedOut, everything else follows the exit
// code.
LockscreenElevationFailure ClassifyLockscreenElevation(
    const LockscreenElevationResult& result);

// Path of the sibling K6WP.exe (the launcher). Empty when it cannot be
// resolved (a broken install). Never throws.
std::wstring LockscreenLauncherPath();

// Runs `K6WP.exe --elevate-lockscreen <on|off>` and WAITS for it (bounded
// by `timeout_ms`).
//
// The launcher self-elevates through UAC (ShellExecuteEx "runas") and maps
// its own outcome onto the exit code, so this single call covers the whole
// "one UAC prompt" flow. A declined prompt comes back as
// kExitElevateDeclined; the Studio must then leave the toggle OFF.
//
// Never throws. On a timeout the launcher is left running (it only performs
// one registry write) and the result reports the failure.
LockscreenElevationResult RunLockscreenElevation(bool enable, int timeout_ms);

// TEST SEAM: replaces the real launcher spawn. The stub receives the same
// arguments and returns the result the production path would produce.
// Passing nullptr restores the real spawn. Production never calls this;
// the Studio tests use it to drive the "registration succeeded / declined"
// branches without an UAC prompt.
using LockscreenElevationFn = LockscreenElevationResult (*)(bool enable,
                                                             int timeout_ms);
void SetLockscreenElevationFn(LockscreenElevationFn fn);

}  // namespace k6wp
