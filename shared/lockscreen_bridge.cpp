#include "lockscreen_bridge.hpp"

#include "lockscreen_exit.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <filesystem>
#include <vector>

namespace k6wp {

namespace {

// The launcher lives next to the Studio in the flattened install (and in
// the build/msvc-dev layout).
std::filesystem::path SiblingLauncher() {
  wchar_t exe_path[MAX_PATH] = {};
  const DWORD len = GetModuleFileNameW(nullptr, exe_path, MAX_PATH);
  if (len == 0 || len >= MAX_PATH) return {};
  std::error_code ec;
  const std::filesystem::path cand =
      std::filesystem::path(exe_path).parent_path() / L"K6WP.exe";
  if (!std::filesystem::exists(cand, ec)) return {};
  return cand;
}

// The test seam (nullptr = the real spawn).
LockscreenElevationFn g_elevation_fn = nullptr;

}  // namespace

void SetLockscreenElevationFn(LockscreenElevationFn fn) {
  g_elevation_fn = fn;
}

std::wstring LockscreenLauncherPath() {
  const std::filesystem::path p = SiblingLauncher();
  return p.empty() ? std::wstring() : p.wstring();
}

LockscreenElevationResult RunLockscreenElevation(bool enable,
                                                  int timeout_ms) {
  if (g_elevation_fn != nullptr) {
    return g_elevation_fn(enable, timeout_ms);
  }
  LockscreenElevationResult result;
  const std::filesystem::path launcher = SiblingLauncher();
  if (launcher.empty()) {
    result.ran = false;
    result.exit_code = kExitSpawnFailure;
    return result;
  }

  const std::wstring action = enable ? L"on" : L"off";
  std::wstring args = L"--elevate-lockscreen " + action;
  std::vector<wchar_t> cmd(args.begin(), args.end());
  cmd.push_back(L'\0');

  STARTUPINFOW si{};
  si.cb = sizeof(si);
  PROCESS_INFORMATION pi{};
  // The launcher shows the UAC prompt itself (ShellExecuteEx "runas"), so
  // this process only has to start it and wait for its exit code.
  if (!CreateProcessW(launcher.c_str(), cmd.data(), nullptr, nullptr, FALSE,
                      CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
    result.ran = false;
    result.exit_code = kExitSpawnFailure;
    return result;
  }
  CloseHandle(pi.hThread);

  const DWORD wait_ms = timeout_ms > 0 ? static_cast<DWORD>(timeout_ms) : 0;
  const DWORD wait = WaitForSingleObject(pi.hProcess, wait_ms);
  if (wait != WAIT_OBJECT_0) {
    // The launcher is stuck (it waits for the human at the UAC prompt). It
    // performs a single registry write, so leaving it running is safe; the
    // Studio reports the failure and keeps the toggle where it was.
    CloseHandle(pi.hProcess);
    result.ran = false;
    result.exit_code = kExitElevateTimeout;
    return result;
  }
  DWORD exit_code = 0;
  if (!GetExitCodeProcess(pi.hProcess, &exit_code)) {
    exit_code = kExitSpawnFailure;
  }
  CloseHandle(pi.hProcess);
  result.ran = true;
  result.exit_code = static_cast<int>(exit_code);
  return result;
}

LockscreenElevationFailure ClassifyLockscreenElevation(
    const LockscreenElevationResult& result) {
  if (result.ok()) return LockscreenElevationFailure::kNone;
  switch (result.exit_code) {
    case kExitElevateDeclined:
      return LockscreenElevationFailure::kDeclined;
    case kExitElevateTimeout:
      return LockscreenElevationFailure::kTimedOut;
    case kExitSpawnFailure:
    case kExitBadFlag:
      return LockscreenElevationFailure::kSpawnFailed;
    default:
      return LockscreenElevationFailure::kUnknown;
  }
}

}  // namespace k6wp
