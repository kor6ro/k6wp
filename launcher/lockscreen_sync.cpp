#include "lockscreen_sync.hpp"

#include "lockscreen_backup.hpp"
#include "lockscreen_policy.hpp"
#include "win32_raii.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <aclapi.h>
#include <shellapi.h>

#include <cstdio>
#include <filesystem>
#include <vector>

namespace k6wp::launcher {

namespace {

// Wait budget: the worker itself does a handful of registry + ACL calls
// (milliseconds), so the only unbounded input is the human reading the UAC
// consent dialog. 2 minutes is generous for that, and a hard bound matters
// because a user who walks away must not hang the uninstaller forever.
constexpr DWORD kElevatedWorkerWaitMs = 120000;

bool GrantK6wpFolderWriteAccess(const std::wstring& dir,
                               std::wstring& error_out) {
  PACL old_dacl = nullptr;
  PSECURITY_DESCRIPTOR sd = nullptr;
  DWORD rc = GetNamedSecurityInfoW(dir.c_str(), SE_FILE_OBJECT,
                                   DACL_SECURITY_INFORMATION, nullptr, nullptr,
                                   &old_dacl, nullptr, &sd);
  if (rc != ERROR_SUCCESS) {
    error_out = L"GetNamedSecurityInfoW: " + FormatSysError(rc);
    return false;
  }

  HANDLE token = nullptr;
  if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
    error_out =
        L"OpenProcessToken: " + FormatSysError(GetLastError());
    LocalFree(sd);
    return false;
  }
  HandleGuard token_guard(token);
  DWORD token_len = 0;
  GetTokenInformation(token_guard.get(), TokenUser, nullptr, 0, &token_len);
  if (token_len == 0) {
    error_out =
        L"GetTokenInformation: " + FormatSysError(GetLastError());
    LocalFree(sd);
    return false;
  }
  std::vector<BYTE> token_buf(token_len);
  if (!GetTokenInformation(token_guard.get(), TokenUser, token_buf.data(),
                           token_len, &token_len)) {
    error_out =
        L"GetTokenInformation: " + FormatSysError(GetLastError());
    LocalFree(sd);
    return false;
  }
  const TOKEN_USER* tu =
      reinterpret_cast<const TOKEN_USER*>(token_buf.data());

  BYTE users_sid[SECURITY_MAX_SID_SIZE] = {};
  DWORD users_len = sizeof(users_sid);
  if (!CreateWellKnownSid(WinBuiltinUsersSid, nullptr, users_sid,
                          &users_len)) {
    error_out =
        L"CreateWellKnownSid(Users): " + FormatSysError(GetLastError());
    LocalFree(sd);
    return false;
  }

  EXPLICIT_ACCESSW ea[2] = {};
  ea[0].grfAccessPermissions = GENERIC_READ | GENERIC_WRITE | DELETE;
  ea[0].grfAccessMode = GRANT_ACCESS;
  ea[0].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
  ea[0].Trustee.pMultipleTrustee = nullptr;
  ea[0].Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
  ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
  ea[0].Trustee.TrusteeType = TRUSTEE_IS_USER;
  ea[0].Trustee.ptstrName = reinterpret_cast<LPWSTR>(tu->User.Sid);
  ea[1].grfAccessPermissions = GENERIC_READ | GENERIC_WRITE;
  ea[1].grfAccessMode = GRANT_ACCESS;
  ea[1].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
  ea[1].Trustee.pMultipleTrustee = nullptr;
  ea[1].Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
  ea[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
  ea[1].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
  ea[1].Trustee.ptstrName = reinterpret_cast<LPWSTR>(users_sid);

  PACL new_dacl = nullptr;
  rc = SetEntriesInAclW(2, ea, old_dacl, &new_dacl);
  LocalFree(sd);
  if (rc != ERROR_SUCCESS) {
    error_out = L"SetEntriesInAclW: " + FormatSysError(rc);
    return false;
  }
  rc = SetNamedSecurityInfoW(const_cast<LPWSTR>(dir.c_str()), SE_FILE_OBJECT,
                             DACL_SECURITY_INFORMATION, nullptr, nullptr,
                             new_dacl, nullptr);
  LocalFree(new_dacl);
  if (rc != ERROR_SUCCESS) {
    error_out = L"SetNamedSecurityInfoW: " + FormatSysError(rc);
    return false;
  }
  return true;
}

int RelaunchElevated(const std::wstring& action) {
  wchar_t exe_path[MAX_PATH] = {};
  const DWORD len = GetModuleFileNameW(nullptr, exe_path, MAX_PATH);
  if (len == 0 || len >= MAX_PATH) {
    std::fwprintf(stderr, L"K6WP: error: cannot resolve own exe path (%ls).\n",
                  FormatSysError(GetLastError()).c_str());
    return kExitSpawnFailure;
  }
  const std::wstring params =
      L"--elevate-lockscreen " + action + L" --elevated";
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
    std::fwprintf(stderr, L"K6WP: elevated lockscreen helper finished.\n");
  } else {
    std::fwprintf(stderr,
                  L"K6WP: error: the elevated helper failed (exit code %lu).\n",
                  static_cast<unsigned long>(exit_code));
  }
  return LockscreenExitCodeFor(outcome);
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
    out = L"The administrator prompt was declined, so the lockscreen policy "
          L"was NOT restored. ";
  } else if (exit_code == kExitElevateTimeout) {
    out = L"The administrator helper was stopped before it could report "
          L"success, so the lockscreen policy may be half-restored. ";
  } else {
    out = L"The lockscreen policy could not be restored. ";
  }
  out += L"It may still point at %PROGRAMDATA%\\K6WP\\lockscreen.jpg "
        L"(HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Personalization\\"
        L"LockScreenImage), which stays on this machine after the uninstall. "
        L"To restore it manually, run: K6WP.exe --elevate-lockscreen off "
        L"(from an administrator command prompt).";
  return out;
}

int RunElevateLockscreen(const Options& opts) {
  const bool turn_on = opts.elevate_action == L"on";
  if (!opts.elevated_worker && !IsElevated()) {
    return RelaunchElevated(opts.elevate_action);
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
  std::error_code ec;
  std::filesystem::create_directories(dir, ec);
  if (ec) {
    std::fwprintf(stderr, L"K6WP: error: cannot create %ls.\n",
                  dir.c_str());
    return 3;
  }
  const std::filesystem::path image = dir / kLockscreenFileName;
  const std::filesystem::path backup = dir / kLockscreenBackupName;

  std::wstring current;
  bool present = false;
  if (!ReadLockscreenPolicy(current, present)) {
    std::fwprintf(stderr, L"K6WP: error: cannot read the LockScreenImage "
                          L"policy.\n");
    return 3;
  }

  if (turn_on) {
    if (!std::filesystem::exists(backup, ec)) {
      if (!WriteLockscreenBackup(backup, present, current)) {
        std::fwprintf(stderr, L"K6WP: error: cannot write %ls.\n",
                      backup.c_str());
        return 3;
      }
    }
    std::wstring acl_error;
    if (!GrantK6wpFolderWriteAccess(dir.wstring(), acl_error)) {
      std::fwprintf(stderr, L"K6WP: error: cannot ACL %ls (%ls).\n",
                    dir.c_str(), acl_error.c_str());
      return 3;
    }
    if (!WriteLockscreenPolicy(image.wstring())) {
      std::fwprintf(stderr, L"K6WP: error: cannot write the LockScreenImage "
                            L"policy (%ls).\n",
                    FormatSysError(GetLastError()).c_str());
      return 3;
    }
    std::fwprintf(stderr, L"K6WP: lockscreen sync ON (%ls).\n",
                  image.c_str());
    return 0;
  }

  if (present && _wcsicmp(current.c_str(), image.c_str()) != 0) {
    std::fwprintf(stderr, L"K6WP: LockScreenImage points elsewhere, leaving "
                          L"it untouched.\n");
    return 0;
  }
  const bool backup_exists = std::filesystem::exists(backup, ec);
  bool had_value = false;
  std::wstring old_value;
  const bool backup_ok = ReadLockscreenBackup(backup, had_value, old_value);
  if (backup_ok && had_value && !old_value.empty()) {
    if (!WriteLockscreenPolicy(old_value)) {
      std::fwprintf(stderr, L"K6WP: error: cannot restore the LockScreenImage "
                            L"policy (%ls).\n",
                    FormatSysError(GetLastError()).c_str());
      return 3;
    }
  } else if (backup_exists && !backup_ok) {
    // The backup exists but could not be read: removing the policy now would
    // destroy the only record of the user's pre-K6WP wallpaper with no way back.
    std::fwprintf(stderr, L"K6WP: error: lockscreen backup is unreadable; "
                          L"refusing to remove the policy.\n");
    return 3;
  } else if (!DeleteLockscreenPolicy()) {
    std::fwprintf(stderr, L"K6WP: error: cannot remove the LockScreenImage "
                          L"policy (%ls).\n",
                  FormatSysError(GetLastError()).c_str());
    return 3;
  }
  std::filesystem::remove(backup, ec);
  std::fwprintf(stderr, L"K6WP: lockscreen sync OFF.\n");
  return 0;
}

}  // namespace k6wp::launcher
