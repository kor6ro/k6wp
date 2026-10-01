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

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace k6wp {
namespace launcher {
namespace {

// Per-session pipe base (MED-12 follow-up): the engine serves ONLY
// \\.\pipe\k6wp-engine-<session_id>. This TU must NEVER link k6wp_shared
// (see CMakeLists.txt: zero-dep keeps the Qt-free proof trivial), so the
// suffix is computed locally with the same ProcessIdToSessionId logic as
// k6wp::CurrentSessionPipeName() — keep the two in sync.
constexpr wchar_t kPipeBaseName[] = L"\\\\.\\pipe\\k6wp-engine";

// Pipe name for THIS process's session. Falls back to session 0 when the
// lookup fails (never throws, never empty).
std::wstring SessionPipeName() {
  DWORD session_id = 0;
  if (!ProcessIdToSessionId(GetCurrentProcessId(), &session_id)) {
    session_id = 0;
  }
  return std::wstring(kPipeBaseName) + L"-" + std::to_wstring(session_id);
}
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
// without binding a second pipe.
constexpr DWORD kPipeProbeTimeoutMs = 2000;
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

// ---- uninstall support: stop THIS folder's K6WP processes (--stop) ---------
// Contract for the stop path:
//   1. engine: IPC `quit` over the session pipe (dev-contracts.md §1), which
//      runs the engine's graceful shutdown and therefore RestoreOsWallpaper();
//   2. our own images still alive: WM_CLOSE to their real top-level windows,
//      wait a grace period, then TerminateProcess on that PID;
//   3. eligibility is the full image PATH sitting directly in this K6WP.exe's
//      own folder, so another app's engine.exe is never even a candidate.
constexpr DWORD kEngineQuitAckTimeoutMs = 2000;  // dev-contracts §1 ack deadline
constexpr DWORD kEngineExitWaitMs = 8000;        // dev-contracts §1 PID wait
constexpr DWORD kForceTerminateWaitMs = 2000;    // after TerminateProcess
constexpr DWORD kWindowCloseWaitMs = 3000;       // == uninstall.bat ping -n 4
constexpr DWORD kWaitSliceMs = 100;              // cancellable wait slices
constexpr DWORD kPipeCancelDrainMs = 1000;       // OVERLAPPED cancel drain
constexpr int kIpcProtocolVersion = 1;            // shared/ipc_protocol.hpp

// One NDJSON v1 request frame, byte-identical to shared/ipc_protocol.hpp
// Encode(): {"version":1,"cmd":"<name>","payload":{}}\n. The launcher links no
// k6wp_shared (zero-dep Qt-free proof, see launcher/CMakeLists.txt), so the
// frame is built here; tests/ipc_test.cpp locks the decode side of the same
// format, and this must not drift from it.
std::string BuildEngineCommandLine(const wchar_t* cmd_name) {
  std::string out = "{\"version\":";
  out += std::to_string(kIpcProtocolVersion);
  out += ",\"cmd\":\"";
  out += NarrowAscii(cmd_name);
  out += "\",\"payload\":{}}\n";
  return out;
}

bool AckIsOk(const std::string& ack) {
  return !ack.empty() && ack.back() == '\n' &&
         ack.rfind("{\"ok\":true", 0) == 0;
}

// Extracts the engine PID from a get_state ack
// ({"ok":true,"state":{...,"pid":N,...}}). The launcher links no JSON parser,
// so this walks the frame once while tracking string literals and escapes: a
// video PATH containing the text "pid" can never be read as the key, and only
// a "pid" key at the state object's own depth counts. Returns 0 for an old
// engine, an error ack, or anything malformed.
DWORD PidFromStateJson(const std::string& ack) {
  int depth = 0;
  int state_depth = 0;
  bool in_string = false;
  bool escaped = false;
  bool pending_key = false;
  bool expect_state_object = false;
  std::string token;
  for (std::size_t i = 0; i < ack.size(); ++i) {
    const char c = ack[i];
    if (in_string) {
      if (escaped) {
        escaped = false;
        token.push_back(c);
      } else if (c == '\\') {
        escaped = true;
      } else if (c == '"') {
        in_string = false;
      } else {
        token.push_back(c);
      }
      continue;
    }
    switch (c) {
      case '"':
        in_string = true;
        token.clear();
        pending_key = true;
        break;
      case '{':
      case '[':
        ++depth;
        if (expect_state_object) {
          state_depth = depth;
          expect_state_object = false;
        }
        pending_key = false;
        break;
      case '}':
      case ']':
        --depth;
        pending_key = false;
        break;
      case ',':
        pending_key = false;
        break;
      case ':':
        if (pending_key) {
          if (token == "state") {
            expect_state_object = true;
          } else if (token == "pid" && state_depth != 0 &&
                     depth == state_depth) {
            std::size_t j = i + 1;
            while (j < ack.size() && (ack[j] == ' ' || ack[j] == '\t')) ++j;
            unsigned long long value = 0;
            std::size_t digits = 0;
            while (j < ack.size() && ack[j] >= '0' && ack[j] <= '9') {
              if (digits < 20) {
                value = value * 10 + static_cast<unsigned>(ack[j] - '0');
                ++digits;
              }
              ++j;
            }
            if (digits == 0 || digits > 10) return 0;
            if (value == 0 || value > 0xFFFFFFFFull) return 0;
            return static_cast<DWORD>(value);
          }
        }
        pending_key = false;
        break;
      default:
        break;
    }
  }
  return 0;
}

enum class PipeSendResult { kAckOk, kNoEngine, kNoAck, kError };

// Engine pipe request/response. Every step is bounded: a missing, busy, or hung
// engine returns a result instead of blocking the uninstaller forever.
PipeSendResult SendEngineCommand(const std::string& line, DWORD ack_timeout_ms,
                                 std::string* ack_out) {
  if (ack_out != nullptr) ack_out->clear();
  const std::wstring pipe_name = SessionPipeName();
  if (WaitNamedPipeW(pipe_name.c_str(), kPipeProbeTimeoutMs) == 0) {
    return PipeSendResult::kNoEngine;
  }
  HandleGuard pipe(CreateFileW(pipe_name.c_str(), GENERIC_READ | GENERIC_WRITE,
                               0, nullptr, OPEN_EXISTING,
                               FILE_FLAG_OVERLAPPED, nullptr));
  if (!pipe.valid()) {
    return PipeSendResult::kNoEngine;
  }
  DWORD mode = PIPE_READMODE_MESSAGE;  // engine serves message-mode NDJSON
  if (SetNamedPipeHandleState(pipe.get(), &mode, nullptr, nullptr) == 0) {
    return PipeSendResult::kError;
  }
  DWORD written = 0;
  if (!WriteFile(pipe.get(), line.data(), static_cast<DWORD>(line.size()),
                 &written, nullptr) ||
      written != line.size()) {
    return PipeSendResult::kError;
  }
  HandleGuard done(CreateEventW(nullptr, TRUE, FALSE, nullptr));
  if (!done.valid()) {
    return PipeSendResult::kError;
  }
  char buf[16 * 1024] = {};
  OVERLAPPED ov{};
  ov.hEvent = done.get();
  if (ReadFile(pipe.get(), buf, sizeof(buf) - 1, nullptr, &ov) == 0 &&
      GetLastError() != ERROR_IO_PENDING) {
    return PipeSendResult::kError;
  }
  const DWORD wait = WaitForSingleObject(done.get(), ack_timeout_ms);
  if (wait != WAIT_OBJECT_0) {
    // The OVERLAPPED and its buffer may only be released once the I/O is
    // finished, so drain the cancellation before returning.
    CancelIoEx(pipe.get(), &ov);
    WaitForSingleObject(done.get(), kPipeCancelDrainMs);
    return PipeSendResult::kNoAck;
  }
  if (ack_out != nullptr) {
    const ULONG_PTR transferred = ov.InternalHigh;
    const std::size_t size =
        (transferred > sizeof(buf) - 1) ? sizeof(buf) - 1
                                        : static_cast<std::size_t>(transferred);
    ack_out->assign(buf, size);
  }
  return PipeSendResult::kAckOk;
}

bool WaitForProcessExit(HANDLE process, DWORD timeout_ms) {
  const ULONGLONG start = GetTickCount64();
  for (;;) {
    const ULONGLONG elapsed = GetTickCount64() - start;
    if (elapsed >= timeout_ms) return false;
    const DWORD remaining = static_cast<DWORD>(timeout_ms - elapsed);
    const DWORD slice = (remaining < kWaitSliceMs) ? remaining : kWaitSliceMs;
    const DWORD wait = WaitForSingleObject(process, slice);
    if (wait == WAIT_OBJECT_0) return true;
    if (wait == WAIT_FAILED) return false;
  }
}

struct WmCloseCtx {
  DWORD pid = 0;
  int posted = 0;
};

// Invisible windows are included on purpose: the engine's message window is a
// hidden WS_POPUP (engine/src/engine_app.cpp CreateMessageWindow), and its
// WM_CLOSE handler is what calls RequestShutdown() -> RestoreOsWallpaper().
BOOL CALLBACK PostWmCloseToPid(HWND hwnd, LPARAM lparam) {
  auto* ctx = reinterpret_cast<WmCloseCtx*>(lparam);
  DWORD pid = 0;
  GetWindowThreadProcessId(hwnd, &pid);
  if (pid == 0 || pid != ctx->pid) return TRUE;
  if (PostMessageW(hwnd, WM_CLOSE, 0, 0) != 0) {
    ++ctx->posted;
  }
  return TRUE;
}

int StopOwnProcess(DWORD pid, DWORD grace_ms) {
  HandleGuard process(OpenProcess(SYNCHRONIZE | PROCESS_TERMINATE, FALSE, pid));
  if (!process.valid()) {
    return 0;  // already gone: nothing to stop
  }
  WmCloseCtx ctx;
  ctx.pid = pid;
  EnumWindows(&PostWmCloseToPid, reinterpret_cast<LPARAM>(&ctx));
  if (ctx.posted > 0 && WaitForProcessExit(process.get(), grace_ms)) {
    return 0;  // graceful path (the engine restores the OS wallpaper here)
  }
  if (TerminateProcess(process.get(), 1) == 0 &&
      !WaitForProcessExit(process.get(), 0)) {
    return 3;  // still running (e.g. ACCESS_DENIED): the caller must say so
  }
  return WaitForProcessExit(process.get(), kForceTerminateWaitMs) ? 0 : 3;
}

// PIDs whose image is one of the K6WP images AND lives directly in own_dir.
// The name is only a cheap pre-filter; IsK6wpOwnImage (the full path) decides.
std::vector<DWORD> OwnImagePids(const std::wstring& own_dir,
                                DWORD exclude_pid) {
  std::vector<DWORD> pids;
  HandleGuard snapshot(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));
  if (!snapshot.valid()) return pids;
  PROCESSENTRY32W entry{};
  entry.dwSize = sizeof(entry);
  if (!Process32FirstW(snapshot.get(), &entry)) return pids;
  do {
    if (entry.th32ProcessID == 0 || entry.th32ProcessID == exclude_pid) {
      continue;
    }
    if (!IsK6wpAppImageName(entry.szExeFile)) continue;
    HandleGuard process(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE,
                                    entry.th32ProcessID));
    if (!process.valid()) continue;
    wchar_t image[MAX_PATH] = {};
    DWORD image_len = MAX_PATH;
    if (!QueryFullProcessImageNameW(process.get(), 0, image, &image_len)) {
      continue;
    }
    if (IsK6wpOwnImage(std::wstring(image), own_dir)) {
      pids.push_back(entry.th32ProcessID);
    }
  } while (Process32NextW(snapshot.get(), &entry));
  return pids;
}

void StopEngineOverIpc() {
  std::string ack;
  if (SendEngineCommand(BuildEngineCommandLine(L"get_state"),
                        kEngineQuitAckTimeoutMs, &ack) != PipeSendResult::kAckOk) {
    std::fwprintf(stderr,
                  L"K6WP: --stop: engine not reachable over IPC; falling back "
                  L"to a per-PID stop of this folder's engine.exe.\n");
    return;
  }
  const DWORD engine_pid = PidFromStateJson(ack);
  if (engine_pid == 0) return;
  HandleGuard engine(OpenProcess(SYNCHRONIZE | PROCESS_TERMINATE, FALSE,
                                 engine_pid));
  if (!engine.valid()) return;  // already gone
  std::fwprintf(stderr, L"K6WP: --stop: asking engine PID %lu to quit...\n",
                static_cast<unsigned long>(engine_pid));
  // A successful WriteFile already delivered the request; a lost ack (the
  // engine tears the pipe down while shutting down) is not a failure.
  SendEngineCommand(BuildEngineCommandLine(L"quit"), kEngineQuitAckTimeoutMs,
                    &ack);
  if (WaitForProcessExit(engine.get(), kEngineExitWaitMs)) {
    std::fwprintf(stderr,
                  L"K6WP: --stop: engine PID %lu exited gracefully.\n",
                  static_cast<unsigned long>(engine_pid));
    return;
  }
  std::fwprintf(stderr,
                L"K6WP: --stop: engine PID %lu still alive after %lums; "
                L"terminating that PID.\n",
                static_cast<unsigned long>(engine_pid),
                static_cast<unsigned long>(kEngineExitWaitMs));
  StopOwnProcess(engine_pid, 0);
}

// Returns 0 when every K6WP process in this folder is gone (including the
// already-clean case), 3 when something is still running so the uninstaller
// can tell the user instead of deleting files out from under a live process.
int RunStop() {
  const std::wstring own_dir = OwnDir();
  if (own_dir.empty()) {
    std::fwprintf(stderr,
                  L"K6WP: error: cannot resolve own exe path (error %lu).\n",
                  static_cast<unsigned long>(GetLastError()));
    return kExitSpawnFailure;
  }
  std::fwprintf(stderr,
                L"K6WP: --stop: stopping K6WP processes in %ls (matched by "
                L"full path, never by image name).\n",
                own_dir.c_str());
  StopEngineOverIpc();
  int failures = 0;
  for (const DWORD pid : OwnImagePids(own_dir, GetCurrentProcessId())) {
    if (StopOwnProcess(pid, kWindowCloseWaitMs) != 0) {
      ++failures;
      std::fwprintf(stderr,
                    L"K6WP: error: PID %lu survived WM_CLOSE + "
                    L"TerminateProcess.\n",
                    static_cast<unsigned long>(pid));
    }
  }
  for (const DWORD pid : OwnImagePids(own_dir, GetCurrentProcessId())) {
    ++failures;
    std::fwprintf(stderr, L"K6WP: error: PID %lu is still running.\n",
                  static_cast<unsigned long>(pid));
  }
  if (failures != 0) return kExitSpawnFailure;
  std::fwprintf(stderr, L"K6WP: --stop: no K6WP process left in %ls.\n",
                own_dir.c_str());
  return kExitOk;
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
