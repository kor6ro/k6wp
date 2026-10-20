#include "legacy_lockscreen_policy.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <string>
#include <vector>

#include "lockscreen_exit.hpp"

namespace k6wp {

namespace {

constexpr wchar_t kPolicySubkey[] =
    L"SOFTWARE\\Policies\\Microsoft\\Windows\\Personalization";
constexpr wchar_t kPolicyValueName[] = L"LockScreenImage";

// The test seam (nullptr = the real spawn).
LegacyCleanupFn g_cleanup_fn = nullptr;

std::filesystem::path ProgramDataBase() {
  wchar_t buf[MAX_PATH] = {};
  const DWORD len = GetEnvironmentVariableW(L"PROGRAMDATA", buf, MAX_PATH);
  if (len > 0 && len < MAX_PATH) return std::filesystem::path(buf);
  return std::filesystem::path(L"C:\\ProgramData");
}

std::wstring ReadPolicyValue() {
  HKEY key = nullptr;
  if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, kPolicySubkey, 0, KEY_QUERY_VALUE,
                    &key) != ERROR_SUCCESS) {
    return {};
  }
  DWORD type = 0;
  DWORD size = 0;
  std::wstring out;
  if (RegQueryValueExW(key, kPolicyValueName, nullptr, &type, nullptr, &size) ==
          ERROR_SUCCESS &&
      type == REG_SZ && size > 0) {
    out.resize(size / sizeof(wchar_t));
    if (RegQueryValueExW(key, kPolicyValueName, nullptr, &type,
                         reinterpret_cast<LPBYTE>(out.data()),
                         &size) != ERROR_SUCCESS) {
      out.clear();
    }
    while (!out.empty() && out.back() == L'\0') out.pop_back();
  }
  RegCloseKey(key);
  return out;
}

std::filesystem::path SiblingLauncher() {
  wchar_t exe_path[MAX_PATH] = {};
  const DWORD len = GetModuleFileNameW(nullptr, exe_path, MAX_PATH);
  if (len == 0 || len >= MAX_PATH) return {};
  std::error_code ec;
  const std::filesystem::path cand =
      std::filesystem::path(exe_path).parent_path() / L"K6WP.exe";
  if (std::filesystem::exists(cand, ec)) return cand;
  return {};
}

}  // namespace

std::filesystem::path LegacyLockscreenDir() {
  try {
    return ProgramDataBase() / L"K6WP";
  } catch (...) {
    return {};
  }
}

std::filesystem::path LegacyLockscreenImagePath() {
  return LegacyLockscreenDir() / L"lockscreen.jpg";
}

LegacyLockscreenState ReadLegacyLockscreenState() noexcept {
  LegacyLockscreenState state;
  try {
    const std::wstring policy = ReadPolicyValue();
    if (!policy.empty()) {
      state.policy_present = true;
      const std::wstring ours = LegacyLockscreenImagePath().wstring();
      if (!ours.empty() && _wcsicmp(policy.c_str(), ours.c_str()) == 0) {
        state.policy_ours = true;
      } else {
        state.policy_foreign = true;
      }
    }
    std::error_code ec;
    state.legacy_dir_exists = std::filesystem::exists(LegacyLockscreenDir(), ec);
    state.legacy_image_exists =
        std::filesystem::exists(LegacyLockscreenImagePath(), ec);
  } catch (...) {
    // A broken read is reported as "nothing to clean": never offer a button
    // whose outcome we cannot verify.
    return LegacyLockscreenState{};
  }
  return state;
}

LegacyCleanupOutcome ClassifyLegacyCleanup(bool ran, int exit_code) noexcept {
  if (!ran) return LegacyCleanupOutcome::kSpawnFailed;
  switch (exit_code) {
    case kExitOk:
      return LegacyCleanupOutcome::kCleaned;
    case kExitElevateDeclined:
      return LegacyCleanupOutcome::kDeclined;
    case kExitElevateTimeout:
      return LegacyCleanupOutcome::kTimedOut;
    case kExitBadFlag:
    case kExitSpawnFailure:
      return LegacyCleanupOutcome::kSpawnFailed;
    default:
      return LegacyCleanupOutcome::kWorkerFailed;
  }
}

LegacyCleanupOutcome RunLegacyCleanup(int timeout_ms) noexcept {
  if (g_cleanup_fn != nullptr) return g_cleanup_fn(timeout_ms);
  try {
    const std::filesystem::path launcher = SiblingLauncher();
    if (launcher.empty()) return LegacyCleanupOutcome::kSpawnFailed;

    std::wstring args = L"--elevate-lockscreen off";
    std::vector<wchar_t> cmd(args.begin(), args.end());
    cmd.push_back(L'\0');
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    if (!CreateProcessW(launcher.c_str(), cmd.data(), nullptr, nullptr, FALSE,
                        CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
      return LegacyCleanupOutcome::kSpawnFailed;
    }
    CloseHandle(pi.hThread);
    const DWORD wait_ms = timeout_ms > 0 ? static_cast<DWORD>(timeout_ms) : 0;
    const DWORD wait = WaitForSingleObject(pi.hProcess, wait_ms);
    if (wait != WAIT_OBJECT_0) {
      // The launcher is waiting for the human at the UAC prompt. It performs a
      // single (idempotent) cleanup, so leaving it running is safe.
      CloseHandle(pi.hProcess);
      return LegacyCleanupOutcome::kTimedOut;
    }
    DWORD exit_code = kExitSpawnFailure;
    const bool have_exit = GetExitCodeProcess(pi.hProcess, &exit_code) != 0;
    CloseHandle(pi.hProcess);
    return ClassifyLegacyCleanup(have_exit, static_cast<int>(exit_code));
  } catch (...) {
    return LegacyCleanupOutcome::kSpawnFailed;
  }
}

void SetLegacyCleanupFn(LegacyCleanupFn fn) noexcept { g_cleanup_fn = fn; }

}  // namespace k6wp
