// K6WP launcher (Todo 7): Qt-free Win32 dispatcher producing K6WP.exe.
//
// Modes (autostart writes --engine --silent; --minimized --silent is an alias):
//   K6WP.exe               == --studio : ensure engine, then launch studio
//   K6WP.exe --studio                 : same as default
//   K6WP.exe --engine                 : ensure engine resident, open nothing
//   K6WP.exe --engine --silent        : autostart path (engine only)
//   K6WP.exe --minimized --silent     : alias of --engine --silent
//   K6WP.exe --stop                   : uninstall helper - stop THIS folder's
//                                       K6WP processes (see "uninstall support")
//   K6WP.exe --help                   : list modes
//
// Exit codes: 0 ok | 2 bad flag | 3 missing sibling / spawn failure (also: the
// elevated lockscreen worker itself failed) | 4 the elevated lockscreen worker
// outlived its bounded wait | 5 the UAC prompt was declined. 3/4/5 are what the
// uninstallers read to decide what to tell the user (Defect C).
//
// Design notes (owner decision F1): this dispatcher links NO Qt so the
// resident path stays free of the Qt import cost (RAM budget < 80MB).
// windows.h lives in this .cpp only; no launcher.hpp/.cpp split is
// justified — the whole dispatcher fits in one translation unit.
//
// Todo 11 (done): /SUBSYSTEM:WINDOWS + wWinMain entry point, no console.
// Run(argc, argv) is unchanged; wWinMain only adapts the command line via
// CommandLineToArgvW.

// Todo 11: entry point is wWinMain (/SUBSYSTEM:WINDOWS, no console).
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shellapi.h>
#include <aclapi.h>
#include <tlhelp32.h>

#include "win32_raii.hpp"
#include "lockscreen_backup.hpp"
#include "process_paths.hpp"
#include "cli.hpp"
#include "exit_codes.hpp"
#include "lockscreen_policy.hpp"
#include "lockscreen_sync.hpp"
#include "process_stop.hpp"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace k6wp {
namespace launcher {
namespace {

constexpr wchar_t kStudioMutexName[] = L"Local\\K6WP-Studio-Singleton";

// Task 19 pipe timing budget (exact values quoted in
// .omo/evidence/task-19-launcher-singleton.txt):
//   probe = WaitNamedPipeW(session pipe, 2000) x2 with a 500ms confirm gap
//           (~4.5s worst case) before any spawn decision.
//   ready = 10000ms poll in 250ms slices after spawn, before Studio launches.
// Mutex ganda: the STUDIO mutex (kStudioMutexName) is owned HERE (held until
// this process exits, so a second K6WP.exe observes ALREADY_EXISTS and only
// focuses). The ENGINE mutex (kEngineMutexName) is owned by the engine itself
// (engine/src/engine_app.cpp); the launcher only probes the pipe and never
// creates it — a raced duplicate spawn is rejected engine-side with exit 0
//   without binding a second pipe.
constexpr DWORD kPipeReadyWaitMs = 10000;
constexpr DWORD kPipeReadyPollMs = 250;
constexpr wchar_t kEngineMutexName[] = L"Local\\K6WP-Engine-Singleton";

// Locates a sibling exe next to K6WP.exe (mirrors
// engine/src/engine_app.cpp:496 OnTrayOpenStudio). Empty path on failure.
std::filesystem::path SiblingPath(const wchar_t* file_name) {
  wchar_t exe_path[MAX_PATH] = {};
  const DWORD len = GetModuleFileNameW(nullptr, exe_path, MAX_PATH);
  if (len == 0 || len >= MAX_PATH) {
    return {};
  }
  return std::filesystem::path(exe_path).parent_path() / file_name;
}

// Engine owns the pipe lifecycle (IpcServer::Start in Init); the launcher
// only probes. WaitNamedPipeW success => engine alive, spawn nothing.
bool EngineAlive() {
  return WaitNamedPipeW(SessionPipeName().c_str(), kPipeProbeTimeoutMs) != 0;
}

// Polls until an engine pipe instance is connectable or the budget expires.
// Used after spawn so Studio never launches against a half-born engine.
bool WaitForEnginePipe(DWORD timeout_ms) {
  const std::wstring pipe_name = SessionPipeName();
  const ULONGLONG start = GetTickCount64();
  for (;;) {
    if (WaitNamedPipeW(pipe_name.c_str(), kPipeReadyPollMs) != 0) {
      return true;
    }
    if (GetTickCount64() - start >= timeout_ms) {
      return false;
    }
    // WaitNamedPipeW returns immediately with ERROR_FILE_NOT_FOUND while the
    // pipe does not exist yet (its timeout only applies to a busy pipe), so
    // without this sleep the loop burns a full core during engine boot.
    Sleep(kPipeReadyPollMs);
  }
}

// CreateProcessW requires a mutable command-line buffer. Closes thread +
// process handles (cf. OnTrayOpenStudio). DETACHED_PROCESS (Todo 8
// tanpa-console): children never inherit the launcher console, so the
// launcher can exit (or be reaped by a QA pipe) without taking the
// resident engine down; engine/studio own their own stdio.
bool SpawnDetached(const std::filesystem::path& exe,
                   const std::wstring& extra_args) {
  std::wstring cmd = L"\"" + exe.wstring() + L"\"" + extra_args;
  std::vector<wchar_t> cmd_buf(cmd.begin(), cmd.end());
  cmd_buf.push_back(L'\0');
  STARTUPINFOW si{};
  si.cb = sizeof(si);
  PROCESS_INFORMATION pi{};
  if (!CreateProcessW(exe.c_str(), cmd_buf.data(), nullptr, nullptr, FALSE,
                      DETACHED_PROCESS, nullptr, nullptr, &si, &pi)) {
    return false;
  }
  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);
  return true;
}

// Ensures exactly one engine: probe first, spawn ONLY when the pipe is
// absent (no blind duplicates). The probe is confirmed with one retry so a
// transiently-busy single pipe instance can never cause a duplicate engine.
// A stale/busy pipe (both 2000ms probes timed out but the name exists) gets
// an explicit warning, then the same duplicate-safe start attempt: the engine
// owns Local\K6WP-Engine-Singleton and rejects a second instance with exit 0.
// After spawn, waits up to kPipeReadyWaitMs for the pipe before returning so
// Studio (or --silent callers) never race a half-born engine.
// Returns exit code (0 ok, 3 on error).
int EnsureEngine(const Options& opts) {
  if (EngineAlive()) {
    return 0;
  }
  Sleep(500);  // one confirmation probe against a transiently-busy pipe
  if (EngineAlive()) {
    return 0;
  }
  // Classify the double-timeout: absent pipe (normal first boot, silent path)
  // vs stale/busy pipe (dead listener or engine mid-start). Zero-timeout
  // probe returns immediately: ERROR_FILE_NOT_FOUND means absent, anything
  // else means the name exists but no instance is connectable.
  DWORD classify = ERROR_FILE_NOT_FOUND;
  if (WaitNamedPipeW(SessionPipeName().c_str(), 0) != 0) {
    return 0;  // became connectable between the two probes
  } else {
    classify = GetLastError();
  }
  if (classify != ERROR_FILE_NOT_FOUND) {
    std::fwprintf(stderr,
                  L"K6WP: warning: engine pipe busy/stale after %lums probes "
                  L"(error %lu); attempting to start engine (duplicate-safe: "
                  L"mutex %ls rejects a second instance).\n",
                  static_cast<unsigned long>(kPipeProbeTimeoutMs) * 2,
                  static_cast<unsigned long>(classify), kEngineMutexName);
  }
  std::error_code ec;
  const std::filesystem::path engine = SiblingPath(kEngineExe);
  if (engine.empty() || !std::filesystem::exists(engine, ec)) {
    std::fwprintf(stderr, L"K6WP: error: sibling engine.exe not found next "
                          L"to K6WP.exe (reinstall the app).\n");
    return 3;
  }
  // Autostart/silent launches keep the tray-only marker so the engine log
  // distinguishes them from interactive starts.
  std::wstring args = opts.engine_extra;
  if ((opts.silent || opts.minimized) &&
      args.find(L"--minimized") == std::wstring::npos) {
    args += L" --minimized";
  }
  if (!SpawnDetached(engine, args)) {
    std::fwprintf(stderr, L"K6WP: error: failed to start engine.exe (error "
                          L"%lu).\n",
                  GetLastError());
    return 3;
  }
  if (!WaitForEnginePipe(kPipeReadyWaitMs)) {
    std::fwprintf(stderr,
                  L"K6WP: warning: engine pipe not ready %lums after start; "
                  L"continuing (Studio retries the pipe on its own).\n",
                  static_cast<unsigned long>(kPipeReadyWaitMs));
    // Best-effort by design: the start WAS attempted (duplicate-safe via the
    // engine mutex); failing autostart --silent here on a slow disk would be
    // worse than letting Studio/tray report live engine state.
  }
  return 0;
}

struct FocusCtx {
  const wchar_t* file_name = L"studio.exe";
  bool found = false;
};

BOOL CALLBACK FocusStudioWindow(HWND hwnd, LPARAM lparam) {
  auto* ctx = reinterpret_cast<FocusCtx*>(lparam);
  if (!IsWindowVisible(hwnd) || GetWindow(hwnd, GW_OWNER) != nullptr) {
    return TRUE;  // skip invisible / owned windows
  }
  DWORD pid = 0;
  GetWindowThreadProcessId(hwnd, &pid);
  if (pid == 0) {
    return TRUE;
  }
  HandleGuard process(
      OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid));
  if (!process.valid()) {
    return TRUE;
  }
  wchar_t image[MAX_PATH] = {};
  DWORD image_len = MAX_PATH;
  if (!QueryFullProcessImageNameW(process.get(), 0, image, &image_len)) {
    return TRUE;
  }
  const std::wstring image_str = image;
  const std::size_t slash = image_str.find_last_of(L"\\/");
  const std::wstring base =
      (slash == std::wstring::npos) ? image_str : image_str.substr(slash + 1);
  if (_wcsicmp(base.c_str(), ctx->file_name) != 0) {
    return TRUE;
  }
  ShowWindow(hwnd, SW_RESTORE);
  SetForegroundWindow(hwnd);
  ctx->found = true;
  return FALSE;  // stop enumeration
}

void FocusExistingStudio() {
  FocusCtx ctx;
  EnumWindows(&FocusStudioWindow, reinterpret_cast<LPARAM>(&ctx));
}

// ---- elevated lockscreen helper (see lockscreen_sync.cpp) ------------------

int Run(int argc, wchar_t** argv) {
  Options opts;
  const int parse_rc = ParseArgs(argc, argv, opts);
  if (parse_rc == 1 || opts.show_help) {
    PrintUsage(stdout);
    return 0;
  }
  if (parse_rc != 0) {
    PrintUsage(stderr);
    return kExitBadFlag;
  }

  if (opts.elevate_lockscreen) {
    const int rc = RunElevateLockscreen(opts);
    const std::wstring guidance = DescribeLockscreenRestore(rc);
    if (!guidance.empty()) {
      std::fwprintf(stderr, L"%ls\n", guidance.c_str());
    }
    return rc;
  }

  if (opts.stop) {
    return RunStop();
  }

  // Per-user studio singleton. When a previous launcher already signalled a
  // studio launch, focus that window and exit 0 WITHOUT spawning an engine.
  // (The first launcher holds the mutex while it spawns; the pipe probe in
  // EnsureEngine is the duplicate-engine guard for the sequential case.)
  HandleGuard mutex_guard(nullptr);
  if (opts.want_studio) {
    const HANDLE mutex =
        CreateMutexW(nullptr, FALSE, kStudioMutexName);
    if (mutex == nullptr) {
      std::fwprintf(stderr, L"K6WP: warning: CreateMutexW failed (error %lu), "
                            L"continuing without singleton guard.\n",
                    GetLastError());
    } else {
      if (GetLastError() == ERROR_ALREADY_EXISTS) {
        // Step 2.4a: a live Studio does not mean a live engine (the user may
        // have killed engine.exe). Ensure the engine first; a failure only
        // warns — focusing the existing Studio must still happen.
        const int ensure_rc = EnsureEngine(opts);
        if (ensure_rc != 0) {
          std::fwprintf(stderr, L"K6WP: warning: EnsureEngine failed (%d); "
                                L"still focusing the existing Studio.\n",
                        ensure_rc);
        }
        FocusExistingStudio();
        CloseHandle(mutex);
        return 0;
      }
      // Hold the mutex until this process exits so concurrent second
      // launches observe ALREADY_EXISTS instead of racing the spawn.
      mutex_guard = HandleGuard(mutex);
    }
  }

  const int engine_rc = EnsureEngine(opts);
  if (engine_rc != 0) {
    return engine_rc;
  }
  if (!opts.want_studio) {
    return 0;  // --engine / --silent: engine ensured, open nothing
  }

  std::error_code ec;
  const std::filesystem::path studio = SiblingPath(kStudioExe);
  if (studio.empty() || !std::filesystem::exists(studio, ec)) {
    std::fwprintf(stderr, L"K6WP: error: sibling studio.exe not found next "
                          L"to K6WP.exe (reinstall the app).\n");
    return 3;
  }
  // Spawn studio and wait on it while holding the singleton mutex, so a
  // concurrent second K6WP.exe observes ALREADY_EXISTS (focus + exit 0)
  // instead of racing the spawn. The mutex releases when studio exits.
  std::wstring cmd = L"\"" + studio.wstring() + L"\"";
  std::vector<wchar_t> cmd_buf(cmd.begin(), cmd.end());
  cmd_buf.push_back(L'\0');
  STARTUPINFOW si{};
  si.cb = sizeof(si);
  PROCESS_INFORMATION pi{};
  if (!CreateProcessW(studio.c_str(), cmd_buf.data(), nullptr, nullptr, FALSE,
                      DETACHED_PROCESS, nullptr, nullptr, &si, &pi)) {
    std::fwprintf(stderr, L"K6WP: error: failed to start studio.exe (error "
                          L"%lu).\n",
                  GetLastError());
    return 3;
  }
  CloseHandle(pi.hThread);
  WaitForSingleObject(pi.hProcess, INFINITE);
  CloseHandle(pi.hProcess);
  return 0;
}

}  // namespace
}  // namespace launcher
}  // namespace k6wp

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    LPWSTR lpCmdLine, int nShowCmd) {
  // DLL-planting hardening: per-user install dir is user-writable; restrict
  // DLL search to the application directory and System32 only (engine/launcher
  // run at logon). Ignore return value explicitly.
  (void)SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_APPLICATION_DIR |
                                 LOAD_LIBRARY_SEARCH_SYSTEM32);
  (void)hInstance;
  (void)hPrevInstance;
  (void)lpCmdLine;
  (void)nShowCmd;
  int argc = 0;
  LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
  if (argv == nullptr) {
    return 3;
  }
  const int rc = k6wp::launcher::Run(argc, argv);
  LocalFree(argv);
  return rc;
}
