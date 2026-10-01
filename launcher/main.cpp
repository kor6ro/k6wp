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
constexpr wchar_t kEngineExe[] = L"engine.exe";
constexpr wchar_t kStudioExe[] = L"studio.exe";
constexpr wchar_t kLauncherExe[] = L"K6WP.exe";

// Process exit codes (also in PrintUsage, and the contract the uninstallers
// read: packaging/installer.nsi + packaging/uninstall.bat). 4 and 5 exist so a
// declined UAC prompt is distinguishable from a broken policy write.
constexpr int kExitOk = 0;
constexpr int kExitBadFlag = 2;
constexpr int kExitSpawnFailure = 3;    // missing sibling / spawn failure
constexpr int kExitElevateTimeout = 4;  // elevated worker outlived its wait
constexpr int kExitElevateDeclined = 5;  // UAC prompt was dismissed

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

struct Options {
  bool want_studio = true;   // default + --studio
  bool show_help = false;
  bool minimized = false;       // passthrough marker for engine
  bool silent = false;          // ensure engine only, open nothing
  bool stop = false;            // --stop: uninstall helper (see RunStop)
  std::wstring engine_extra;  // forwarded engine flags (verbatim)
  bool elevate_lockscreen = false;  // --elevate-lockscreen on|off
  std::wstring elevate_action;      // L"on" or L"off"
  bool elevated_worker = false;     // hidden --elevated (already elevated)
};

void PrintUsage(FILE* out) {
  // fputws, not fwprintf (which the rest of this file uses): this text has no
  // substitutions, and its literal %PROGRAMDATA% would be read as a conversion
  // specifier, pulling a vararg that does not exist.
  std::fputws(
                L"K6WP launcher (Qt-free dispatcher)\n"
                L"Usage: K6WP.exe [options]\n"
                L"  (no args)            ensure engine, then launch studio\n"
                L"  --studio             same as default\n"
                L"  --engine             ensure engine resident, open nothing\n"
                L"  --engine --silent    autostart path (engine only)\n"
                L"  --silent             ensure engine only (autostart path)\n"
                L"  --minimized --silent alias of --engine --silent\n"
                L"  --minimized          passthrough marker forwarded to engine\n"
                L"  --stop               uninstall helper: stop the K6WP "
                L"processes that live in this K6WP.exe's own folder "
                L"(engine, Studio, launcher) - never by image name. Sends the "
                L"engine an IPC quit first so it can restore the OS "
                L"wallpaper, then escalates per PID.\n"
                L"  --config <path>      forwarded to engine.exe\n"
                L"  --video <path>       forwarded to engine.exe\n"
                L"  --wallpaper-mode <m> forwarded to engine.exe\n"
                L"  --elevate-lockscreen <on|off>\n"
                L"                       static lockscreen sync: ACLs "
                L"%PROGRAMDATA%\\K6WP, backs up and sets the HKLM "
                L"LockScreenImage policy (UAC prompt, static image only)\n"
                L"  --help, -h           show this help and exit\n"
                L"Exit codes: 0 ok, 2 bad flag, 3 missing sibling / spawn "
                L"failure (also: the elevated lockscreen helper failed), 4 the "
                L"elevated helper was still running when its wait budget "
                L"expired, 5 the UAC prompt was declined.\n",
                out);
}

// Parses launcher flags. Engine passthrough flags (--config/--video/
// --wallpaper-mode/--minimized) are forwarded verbatim, never reinterpreted.
// Returns 0 ok, 1 help requested, 2 bad flag.
int ParseArgs(int argc, wchar_t** argv, Options& out) {
  for (int i = 1; i < argc; ++i) {
    const std::wstring arg = argv[i];
    if (arg == L"--help" || arg == L"-h") {
      out.show_help = true;
      return 1;
    } else if (arg == L"--studio") {
      out.want_studio = true;
    } else if (arg == L"--engine") {
      out.want_studio = false;
    } else if (arg == L"--silent") {
      out.want_studio = false;
      out.silent = true;
    } else if (arg == L"--minimized") {
      out.minimized = true;
      out.engine_extra += L" --minimized";
    } else if (arg == L"--config" || arg == L"--video" ||
               arg == L"--wallpaper-mode") {
      if (i + 1 >= argc) {
        std::fwprintf(stderr, L"K6WP: error: %ls requires a value\n",
                      arg.c_str());
        return kExitBadFlag;
      }
      // Quote the value; engine ParseCli accepts "--flag value".
      out.engine_extra += L" " + arg + L" \"" + std::wstring(argv[++i]) + L"\"";
    } else if (arg == L"--elevate-lockscreen") {
      if (i + 1 >= argc) {
        std::fwprintf(stderr,
                      L"K6WP: error: --elevate-lockscreen requires on|off\n");
        return kExitBadFlag;
      }
      const std::wstring action = argv[++i];
      if (action != L"on" && action != L"off") {
        std::fwprintf(stderr,
                      L"K6WP: error: --elevate-lockscreen expects on|off, got "
                      L"'%ls'\n",
                      action.c_str());
        return kExitBadFlag;
      }
      out.elevate_lockscreen = true;
      out.elevate_action = action;
      out.want_studio = false;
    } else if (arg == L"--elevated") {
      out.elevated_worker = true;
    } else if (arg == L"--stop") {
      out.stop = true;
      out.want_studio = false;
      out.silent = true;
    } else if (arg.rfind(L"--config=", 0) == 0 ||
               arg.rfind(L"--video=", 0) == 0 ||
               arg.rfind(L"--wallpaper-mode=", 0) == 0) {
      out.engine_extra += L" " + arg;
    } else {
      std::fwprintf(stderr, L"K6WP: error: unknown flag '%ls'\n", arg.c_str());
      return kExitBadFlag;
    }
  }
  return 0;
}

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

bool IsK6wpAppImageName(const std::wstring& base_name) {
  return _wcsicmp(base_name.c_str(), kEngineExe) == 0 ||
         _wcsicmp(base_name.c_str(), kStudioExe) == 0 ||
         _wcsicmp(base_name.c_str(), kLauncherExe) == 0;
}

std::wstring BaseNameText(const std::wstring& full_image) {
  const std::size_t slash = full_image.find_last_of(L"\\/");
  return (slash == std::wstring::npos) ? full_image
                                       : full_image.substr(slash + 1);
}

std::wstring ParentDirText(const std::wstring& full_image) {
  const std::size_t slash = full_image.find_last_of(L"\\/");
  if (slash == std::wstring::npos) return std::wstring();
  return full_image.substr(0, slash);
}

std::wstring TrimTrailingSeparators(std::wstring dir) {
  while (dir.size() > 1 && (dir.back() == L'\\' || dir.back() == L'/')) {
    if (dir[dir.size() - 2] == L':') break;  // keep a drive root as-is
    dir.pop_back();
  }
  return dir;
}

// The scope test that replaces the image-name kill: a K6WP image name is only
// OURS when it sits directly in own_dir. Compared component-wise (not by
// prefix), so "K6WP-old" never matches "K6WP", and case-insensitively, because
// Windows paths are.
bool IsK6wpOwnImage(const std::wstring& full_image,
                    const std::wstring& own_dir) {
  if (full_image.empty() || own_dir.empty()) return false;
  if (!IsK6wpAppImageName(BaseNameText(full_image))) return false;
  return _wcsicmp(TrimTrailingSeparators(ParentDirText(full_image)).c_str(),
                  TrimTrailingSeparators(own_dir).c_str()) == 0;
}

std::wstring OwnDir() {
  wchar_t exe_path[MAX_PATH] = {};
  const DWORD len = GetModuleFileNameW(nullptr, exe_path, MAX_PATH);
  if (len == 0 || len >= MAX_PATH) return std::wstring();
  return std::filesystem::path(exe_path).parent_path().wstring();
}

std::string NarrowAscii(const wchar_t* text) {
  std::string out;
  if (text == nullptr) return out;
  for (const wchar_t* p = text; *p != 0 && *p < 0x80; ++p) {
    out.push_back(static_cast<char>(*p));
  }
  return out;
}

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

// ---- lockscreen sync elevation (Part B, static image only) ----------------
// One-time admin setup for the lockscreen frame sync:
//   %PROGRAMDATA%\K6WP\lockscreen.jpg  <- compressor --lockframe refreshes it
//   HKLM\...\Personalization LockScreenImage -> that jpg (static policy only;
//   nothing touches Winlogon, no live render, nothing outside the K6WP dir).
constexpr wchar_t kPolicySubkey[] =
    L"SOFTWARE\\Policies\\Microsoft\\Windows\\Personalization";
constexpr wchar_t kPolicyValueName[] = L"LockScreenImage";
constexpr wchar_t kLockscreenFileName[] = L"lockscreen.jpg";
constexpr wchar_t kLockscreenBackupName[] = L"lockscreen_policy_backup.json";

std::wstring FormatSysError(DWORD code) {
  wchar_t* msg_buf = nullptr;
  const DWORD len = FormatMessageW(
      FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
          FORMAT_MESSAGE_IGNORE_INSERTS,
      nullptr, code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      reinterpret_cast<LPWSTR>(&msg_buf), 0, nullptr);
  std::wstring out = L"error " + std::to_wstring(code);
  if (len > 0 && msg_buf != nullptr) {
    std::wstring msg(msg_buf, len);
    while (!msg.empty() &&
           (msg.back() == L'\r' || msg.back() == L'\n' || msg.back() == L' ')) {
      msg.pop_back();
    }
    out += L": " + msg;
  }
  if (msg_buf != nullptr) LocalFree(msg_buf);
  return out;
}

bool IsElevated() {
  HANDLE token = nullptr;
  if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
    return false;
  }
  HandleGuard guard(token);
  TOKEN_ELEVATION elevation{};
  DWORD ret = 0;
  if (!GetTokenInformation(guard.get(), TokenElevation, &elevation,
                           sizeof(elevation), &ret)) {
    return false;
  }
  return elevation.TokenIsElevated != 0;
}

std::filesystem::path ProgramDataK6wpDir() {
  wchar_t buf[MAX_PATH] = {};
  const DWORD n = GetEnvironmentVariableW(L"PROGRAMDATA", buf, MAX_PATH);
  if (n > 0 && n < MAX_PATH) {
    return std::filesystem::path(buf) / L"K6WP";
  }
  return std::filesystem::path();
}

bool ReadLockscreenPolicy(std::wstring& value_out, bool& present) {
  present = false;
  HKEY key = nullptr;
  const LSTATUS open_rc = RegOpenKeyExW(HKEY_LOCAL_MACHINE, kPolicySubkey, 0,
                                        KEY_QUERY_VALUE, &key);
  if (open_rc == ERROR_FILE_NOT_FOUND) return true;
  if (open_rc != ERROR_SUCCESS) return false;
  wchar_t buf[32768] = {};
  DWORD size = sizeof(buf);
  DWORD type = 0;
  const LSTATUS q =
      RegQueryValueExW(key, kPolicyValueName, nullptr, &type,
                       reinterpret_cast<LPBYTE>(buf), &size);
  RegCloseKey(key);
  if (q == ERROR_FILE_NOT_FOUND) return true;
  if (q != ERROR_SUCCESS || type != REG_SZ) return false;
  value_out.assign(buf, size / sizeof(wchar_t));
  while (!value_out.empty() && value_out.back() == L'\0') value_out.pop_back();
  present = true;
  return true;
}

bool WriteLockscreenPolicy(const std::wstring& image_path) {
  HKEY key = nullptr;
  const LSTATUS create_rc = RegCreateKeyExW(
      HKEY_LOCAL_MACHINE, kPolicySubkey, 0, nullptr, REG_OPTION_NON_VOLATILE,
      KEY_SET_VALUE, nullptr, &key, nullptr);
  if (create_rc != ERROR_SUCCESS) return false;
  const LSTATUS s = RegSetValueExW(key, kPolicyValueName, 0, REG_SZ,
                        reinterpret_cast<const BYTE*>(image_path.c_str()),
                        static_cast<DWORD>((image_path.size() + 1) *
                                           sizeof(wchar_t)));
  RegCloseKey(key);
  return s == ERROR_SUCCESS;
}

bool DeleteLockscreenPolicy() {
  HKEY key = nullptr;
  const LSTATUS open_rc = RegOpenKeyExW(HKEY_LOCAL_MACHINE, kPolicySubkey, 0,
                                        KEY_SET_VALUE, &key);
  if (open_rc == ERROR_FILE_NOT_FOUND) return true;
  if (open_rc != ERROR_SUCCESS) return false;
  const LSTATUS del = RegDeleteValueW(key, kPolicyValueName);
  RegCloseKey(key);
  return del == ERROR_SUCCESS || del == ERROR_FILE_NOT_FOUND;
}

std::string EscapeBackupJson(const std::wstring& wide) {
  // Todo 19 (HIGH-2): non-ASCII used to flatten to '?' (data loss). Convert
  // the whole string to UTF-8 first so surrogate pairs survive, then JSON-
  // escape only what JSON requires; UTF-8 bytes >= 0x80 pass through raw.
  std::string utf8;
  if (!wide.empty()) {
    const int utf8_len = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(),
                                             static_cast<int>(wide.size()),
                                             nullptr, 0, nullptr, nullptr);
    if (utf8_len > 0) {
      utf8.resize(static_cast<std::size_t>(utf8_len));
      WideCharToMultiByte(CP_UTF8, 0, wide.c_str(),
                          static_cast<int>(wide.size()), utf8.data(),
                          utf8_len, nullptr, nullptr);
    }
  }
  std::string out;
  for (const unsigned char c : utf8) {
    if (c == '\\') {
      out += "\\\\";
    } else if (c == '"') {
      out += "\\\"";
    } else if (c < 0x20) {
      char hex[8] = {};
      std::snprintf(hex, sizeof(hex), "\\u%04x", c);
      out += hex;
    } else {
      out += static_cast<char>(c);
    }
  }
  return out;
}

bool WriteLockscreenBackup(const std::filesystem::path& backup_path,
                           bool had_value, const std::wstring& value) {
  // Crash-safe publish (same contract as config.json): write a sibling .tmp,
  // then replace atomically. Losing this file loses the user's pre-K6WP policy
  // permanently, so a truncated in-place write is not acceptable here.
  const std::filesystem::path tmp =
      backup_path.parent_path() /
      (backup_path.filename().wstring() + L".tmp");
  {
    std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
    if (!out) return false;
    out << "{\"had_value\":" << (had_value ? "true" : "false") << ",\"value\":\""
        << EscapeBackupJson(value) << "\"}";
    out.flush();
    if (!out) {
      DeleteFileW(tmp.c_str());
      return false;
    }
  }
  if (!MoveFileExW(tmp.c_str(), backup_path.c_str(),
                   MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
    DeleteFileW(tmp.c_str());
    return false;
  }
  return true;
}

bool ReadLockscreenBackup(const std::filesystem::path& backup_path,
                          bool& had_value, std::wstring& value) {
  had_value = false;
  value.clear();
  std::ifstream in(backup_path, std::ios::binary);
  if (!in) return false;
  const std::string text((std::istreambuf_iterator<char>(in)),
                         std::istreambuf_iterator<char>());
  had_value = text.find("\"had_value\":true") != std::string::npos;
  const auto vpos = text.find("\"value\":\"");
  if (vpos == std::string::npos) return had_value;
  // Todo 19 (HIGH-2): decode JSON escapes properly (\\, \", \uXXXX with
  // surrogate pairs) into UTF-8, then convert to wide. The old byte-for-byte
  // copy mangled every escape and every multi-byte UTF-8 sequence.
  std::string utf8;
  bool esc = false;
  wchar_t pending_high = 0;
  for (std::size_t i = vpos + 9; i < text.size(); ++i) {
    const char c = text[i];
    if (esc) {
      esc = false;
      if (c == '\\') {
        utf8 += '\\';
      } else if (c == '"') {
        utf8 += '"';
      } else if (c == 'u') {
        if (i + 4 < text.size()) {
          unsigned int cp = 0;
          bool ok = true;
          for (std::size_t j = i + 1; j <= i + 4; ++j) {
            const char h = text[j];
            cp <<= 4;
            if (h >= '0' && h <= '9') {
              cp |= static_cast<unsigned int>(h - '0');
            } else if (h >= 'a' && h <= 'f') {
              cp |= static_cast<unsigned int>(h - 'a' + 10);
            } else if (h >= 'A' && h <= 'F') {
              cp |= static_cast<unsigned int>(h - 'A' + 10);
            } else {
              ok = false;
              break;
            }
          }
          i += 4;
          if (ok) {
            const wchar_t unit = static_cast<wchar_t>(cp);
            char buf[4] = {};
            int n = 0;
            if (pending_high != 0) {
              const wchar_t pair[2] = {pending_high, unit};
              pending_high = 0;
              n = WideCharToMultiByte(CP_UTF8, 0, pair, 2, buf,
                                      static_cast<int>(sizeof(buf)), nullptr,
                                      nullptr);
            } else if (unit >= 0xD800 && unit <= 0xDBFF) {
              pending_high = unit;
            } else {
              n = WideCharToMultiByte(CP_UTF8, 0, &unit, 1, buf,
                                      static_cast<int>(sizeof(buf)), nullptr,
                                      nullptr);
            }
            if (n > 0) {
              utf8.append(buf, static_cast<std::size_t>(n));
            }
          }
        }
      } else {
        utf8 += c;  // unknown escape: keep the literal char
      }
    } else if (c == '\\') {
      esc = true;
    } else if (c == '"') {
      break;
    } else {
      utf8 += c;
    }
  }
  if (!utf8.empty()) {
    const int wide_len = MultiByteToWideChar(CP_UTF8, 0, utf8.data(),
                                             static_cast<int>(utf8.size()),
                                             nullptr, 0);
    if (wide_len <= 0) return false;
    value.resize(static_cast<std::size_t>(wide_len));
    MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()),
                        value.data(), wide_len);
  }
  // Todo 19 (HIGH-2) backward compat: a pre-fix backup flattened every
  // non-ASCII char to '?'. Warn explicitly instead of silently restoring a
  // corrupted path.
  if (value.find(L'?') != std::wstring::npos) {
    std::fwprintf(stderr,
                  L"K6WP: warning: lockscreen backup looks corrupted (legacy "
                  L"'?' escaping) -- original path may be unrecoverable.\n");
  }
  return true;
}

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

// ---- elevated lockscreen helper: bounded wait + honest outcome -------------
// Defect B: RelaunchElevated used ShellExecuteW, which returns as soon as the
// runas verb is ACCEPTED, so the caller's ExecWait completed while the elevated
// worker was still writing HKLM - and the uninstaller then deleted this very
// binary out from under it. SEE_MASK_NOCLOSEPROCESS is the only documented way
// to get the elevated process handle, and therefore the only way to know the
// HKLM write has actually happened.
//
// Wait budget: the worker itself does a handful of registry + ACL calls
// (milliseconds), so the only unbounded input is the human reading the UAC
// consent dialog. 2 minutes is generous for that, and a hard bound matters
// because a user who walks away must not hang the uninstaller forever.
constexpr DWORD kElevatedWorkerWaitMs = 120000;

enum class ElevatedWaitOutcome {
  kRestored,      // the worker finished the policy restore
  kDeclined,      // ERROR_CANCELLED: the user dismissed the UAC prompt
  kTimedOut,      // the budget expired; the worker was terminated
  kLaunchFailed,  // ShellExecuteExW failed for a non-consent reason
  kWaitFailed,    // WaitForSingleObject itself failed
  kWorkerFailed,  // the worker ran and returned non-zero
};

ElevatedWaitOutcome ClassifyElevatedWait(bool launched, DWORD launch_error,
                                        DWORD wait_result, DWORD exit_code) {
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

// What is left behind when the restore did not happen, and the one command that
// fixes it. Empty on success. The uninstallers print the same wording in their
// own message boxes (they cannot read this process's stderr), so the manual
// command string here is the authoritative copy.
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
