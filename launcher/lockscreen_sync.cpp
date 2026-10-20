#include "lockscreen_sync.hpp"

#include "lockscreen_backup.hpp"
#include "lockscreen_policy.hpp"
#include "win32_raii.hpp"

#include "legacy_lockscreen_policy.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shellapi.h>

#include <cstdio>
#include <filesystem>

namespace k6wp::launcher {

namespace {

// Runs `K6WP.exe --elevate-lockscreen off --elevated` through the UAC prompt
// and waits for it, bounded. This is the ONE elevation left in the product
// and it only removes what the old HKLM policy mechanism left behind.
int RelaunchElevated() {
  wchar_t exe_path[MAX_PATH] = {};
  const DWORD len = GetModuleFileNameW(nullptr, exe_path, MAX_PATH);
  if (len == 0 || len >= MAX_PATH) {
    std::fwprintf(stderr, L"K6WP: error: cannot resolve own exe path (%ls).\n",
                  FormatSysError(GetLastError()).c_str());
    return kExitSpawnFailure;
  }
  const std::wstring params = L"--elevate-lockscreen off --elevated";
  SHELLEXECUTEINFOW sei{};
  sei.cbSize = sizeof(sei);
  sei.fMask = SEE_MASK_NOCLOSEPROCESS;
  sei.lpVerb = L"runas";
  sei.lpFile = exe_path;
  sei.lpParameters = params.c_str();
  sei.nShow = SW_HIDE;
  const BOOL launched = ShellExecuteExW(&sei);
  const DWORD launch_error = launched ? ERROR_SUCCESS : GetLastError();
  HandleGuard worker(launched ? sei.hProcess : nullptr);
  if (!worker.valid()) {
    const int code = LockscreenExitCodeFor(
        ClassifyElevatedWait(launched != FALSE, launch_error, WAIT_FAILED, 0));
    std::fwprintf(stderr,
                  L"K6WP: error: elevation prompt failed or was declined "
                  L"(%ls).\n",
                  FormatSysError(launch_error).c_str());
    return code;
  }
  const DWORD wait_result = WaitForSingleObject(worker.get(),
                                                kElevatedWorkerWaitMs);
  DWORD exit_code = 0;
  const bool have_exit_code = wait_result == WAIT_OBJECT_0 &&
                              GetExitCodeProcess(worker.get(), &exit_code) != 0;
  const ElevatedWaitOutcome outcome = ClassifyElevatedWait(
      true, ERROR_SUCCESS, wait_result, have_exit_code ? exit_code : 1);
  if (outcome == ElevatedWaitOutcome::kTimedOut) {
    // The caller deletes this binary next; a worker still running is exactly the
    // race this wait exists to close, so it is terminated rather than left
    // behind. Its single HKLM value write is atomic per RegSetValueEx call.
    TerminateProcess(worker.get(), kExitElevateTimeout);
    std::fwprintf(stderr,
                  L"K6WP: error: the elevated helper was still running after "
                  L"%lums; terminated it.\n",
                  static_cast<unsigned long>(kElevatedWorkerWaitMs));
  } else if (outcome == ElevatedWaitOutcome::kRestored) {
    std::fwprintf(stderr, L"K6WP: legacy lockscreen cleanup finished.\n");
  } else {
    std::fwprintf(stderr,
                  L"K6WP: error: the elevated cleanup failed (exit code %lu).\n",
                  static_cast<unsigned long>(exit_code));
  }
  return LockscreenExitCodeFor(outcome);
}

// Removes the two files the old mechanism owned, then the folder - but only
// when it is empty afterwards. A user who put anything else in
// %PROGRAMDATA%\K6WP keeps that folder (remove() on a non-empty directory
// fails harmlessly).
void RemoveLegacyFolder(const std::filesystem::path& dir) {
  std::error_code ec;
  std::filesystem::remove(dir / kLockscreenFileName, ec);
  std::filesystem::remove(dir / kLockscreenBackupName, ec);
  std::filesystem::remove(dir, ec);
}

}  // namespace

ElevatedWaitOutcome ClassifyElevatedWait(bool launched, unsigned long launch_error,
                                        unsigned long wait_result,
                                        unsigned long exit_code) {
  if (!launched) {
    return (launch_error == ERROR_CANCELLED) ? ElevatedWaitOutcome::kDeclined
                                             : ElevatedWaitOutcome::kLaunchFailed;
  }
  switch (wait_result) {
    case WAIT_TIMEOUT:
      return ElevatedWaitOutcome::kTimedOut;
    case WAIT_OBJECT_0:
      return (exit_code == 0) ? ElevatedWaitOutcome::kRestored
                              : ElevatedWaitOutcome::kWorkerFailed;
    default:
      return ElevatedWaitOutcome::kWaitFailed;
  }
}

int LockscreenExitCodeFor(ElevatedWaitOutcome outcome) {
  switch (outcome) {
    case ElevatedWaitOutcome::kRestored:
      return kExitOk;
    case ElevatedWaitOutcome::kDeclined:
      return kExitElevateDeclined;
    case ElevatedWaitOutcome::kTimedOut:
      return kExitElevateTimeout;
    case ElevatedWaitOutcome::kWorkerFailed:
    case ElevatedWaitOutcome::kLaunchFailed:
    case ElevatedWaitOutcome::kWaitFailed:
    default:
      return kExitSpawnFailure;
  }
}

std::wstring DescribeLockscreenRestore(int exit_code) {
  if (exit_code == kExitOk) return std::wstring();
  std::wstring out;
  if (exit_code == kExitElevateDeclined) {
    out = L"The administrator prompt was declined, so the legacy lockscreen "
          L"policy was NOT removed. ";
  } else if (exit_code == kExitElevateTimeout) {
    out = L"The administrator helper was stopped before it could report "
          L"success, so the legacy lockscreen policy may be half-removed. ";
  } else {
    out = L"The legacy lockscreen policy could not be removed. ";
  }
  out += L"It may still point at %PROGRAMDATA%\\K6WP\\lockscreen.jpg "
         L"(HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Personalization\\"
         L"LockScreenImage), which stays on this machine after the uninstall. "
         L"To remove it manually, run: K6WP.exe --elevate-lockscreen off "
         L"(from an administrator command prompt).";
  return out;
}

int RunElevateLockscreen(const Options& opts) {
  if (!opts.elevated_worker && !IsElevated()) {
    // Nothing to clean -> no UAC prompt. The check is read-only and mirrors
    // the Studio's "Bersihkan" detection; a policy pointing elsewhere is left
    // alone (and never prompts either).
    const k6wp::LegacyLockscreenState legacy =
        k6wp::ReadLegacyLockscreenState();
    if (!legacy.cleanup_needed()) {
      std::fwprintf(stderr,
                    L"K6WP: no legacy lockscreen policy left; nothing to "
                    L"clean up.\n");
      return 0;
    }
    return RelaunchElevated();
  }
  if (!IsElevated()) {
    std::fwprintf(stderr, L"K6WP: error: --elevate-lockscreen requires "
                          L"administrator rights.\n");
    return 3;
  }

  const std::filesystem::path dir = ProgramDataK6wpDir();
  if (dir.empty()) {
    std::fwprintf(stderr,
                  L"K6WP: error: PROGRAMDATA is not set, cannot resolve the "
                  L"K6WP folder.\n");
    return 3;
  }
  const std::filesystem::path image = dir / kLockscreenFileName;
  const std::filesystem::path backup = dir / kLockscreenBackupName;

  std::error_code ec;
  const bool dir_exists = std::filesystem::exists(dir, ec);
  std::wstring current;
  bool present = false;
  if (!ReadLockscreenPolicy(current, present)) {
    std::fwprintf(stderr, L"K6WP: error: cannot read the LockScreenImage "
                          L"policy.\n");
    return 3;
  }

  if (present && _wcsicmp(current.c_str(), image.c_str()) != 0) {
    // Someone else's policy: never touch it, and leave the folder alone (it
    // may belong to that policy's owner).
    std::fwprintf(stderr, L"K6WP: LockScreenImage points elsewhere, leaving "
                          L"it untouched.\n");
    return 0;
  }

  const bool backup_exists = std::filesystem::exists(backup, ec);
  bool had_value = false;
  std::wstring old_value;
  const bool backup_ok = ReadLockscreenBackup(backup, had_value, old_value);
  if (backup_exists && !backup_ok) {
    // The backup exists but could not be read: removing the policy now would
    // destroy the only record of the user's pre-K6WP wallpaper with no way back.
    std::fwprintf(stderr, L"K6WP: error: lockscreen backup is unreadable; "
                          L"refusing to remove the policy.\n");
    return 3;
  }
  if (backup_ok && had_value && !old_value.empty()) {
    if (!WriteLockscreenPolicy(old_value)) {
      std::fwprintf(stderr, L"K6WP: error: cannot restore the LockScreenImage "
                            L"policy (%ls).\n",
                    FormatSysError(GetLastError()).c_str());
      return 3;
    }
  } else if (present && !DeleteLockscreenPolicy()) {
    std::fwprintf(stderr, L"K6WP: error: cannot remove the LockScreenImage "
                          L"policy (%ls).\n",
                  FormatSysError(GetLastError()).c_str());
    return 3;
  }

  if (dir_exists) {
    RemoveLegacyFolder(dir);
  }
  std::fwprintf(stderr, L"K6WP: legacy lockscreen policy cleaned up.\n");
  return 0;
}

}  // namespace k6wp::launcher
