#pragma once

// Legacy lock-screen policy: READ-ONLY detection (plus the one cleanup path).
//
// The previous mechanism made Windows show a lock-screen picture by writing the
// HKLM policy
//   SOFTWARE\Policies\Microsoft\Windows\Personalization\LockScreenImage
// pointing at %PROGRAMDATA%\K6WP\lockscreen.jpg. That needed administrator
// rights, only worked on some editions, and is GONE from the normal flow: the
// per-user still helper (shared/still.hpp) owns the lock screen now.
//
// What remains is cleanup. A machine that ran the old version may still carry
// the policy and the folder. Studio shows a "Bersihkan" button when this
// module detects them, and the button runs the launcher's one elevated reset
// (`K6WP.exe --elevate-lockscreen off`, one UAC prompt).
//
// Header stays windows.h-free (same pattern as still.hpp).

#include <filesystem>

namespace k6wp {

// %PROGRAMDATA%\K6WP: the legacy folder (empty path when PROGRAMDATA is unset).
std::filesystem::path LegacyLockscreenDir();

// %PROGRAMDATA%\K6WP\lockscreen.jpg: the image the old policy pointed at.
std::filesystem::path LegacyLockscreenImagePath();

// What is left of the old mechanism on this machine.
struct LegacyLockscreenState {
  // The HKLM value exists.
  bool policy_present = false;
  // ...and points at LegacyLockscreenImagePath(): OUR leftover.
  bool policy_ours = false;
  // ...and points somewhere else: the user's own policy. Never touched.
  bool policy_foreign = false;
  // The legacy folder still holds lockscreen.jpg.
  bool legacy_image_exists = false;
  // The legacy folder exists at all (it may hold only the old permissive ACL).
  bool legacy_dir_exists = false;

  // True when the Studio should offer the cleanup button. A foreign policy is
  // never offered: the folder may belong to that policy's owner.
  bool cleanup_needed() const {
    return policy_ours ||
           (!policy_present && (legacy_image_exists || legacy_dir_exists));
  }
};

// Reads the registry + the folder. Read-only, needs no administrator rights,
// never throws.
LegacyLockscreenState ReadLegacyLockscreenState() noexcept;

// The outcome of `K6WP.exe --elevate-lockscreen off`.
enum class LegacyCleanupOutcome {
  kCleaned,      // the policy/folder are gone (or were never ours)
  kDeclined,     // the user dismissed the UAC prompt
  kTimedOut,     // the elevated worker did not report back in time
  kSpawnFailed,  // K6WP.exe is missing / could not be started
  kWorkerFailed, // the worker ran and failed
};

// Classifies the launcher result. `ran=false` means no exit code exists
// (spawn failure or timeout). Pure, unit-tested.
LegacyCleanupOutcome ClassifyLegacyCleanup(bool ran, int exit_code) noexcept;

// Runs the cleanup through the launcher and waits (bounded by `timeout_ms`).
// The launcher self-elevates, so this covers the whole "one UAC prompt" flow.
// Never throws.
LegacyCleanupOutcome RunLegacyCleanup(int timeout_ms) noexcept;

// TEST SEAM: replaces the launcher spawn. nullptr restores the real one.
// Production never calls this; studio_logic_test drives both branches.
using LegacyCleanupFn = LegacyCleanupOutcome (*)(int timeout_ms);
void SetLegacyCleanupFn(LegacyCleanupFn fn) noexcept;

}  // namespace k6wp
