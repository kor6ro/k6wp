#include "process_stop.hpp"

#include "exit_codes.hpp"
#include "process_paths.hpp"
#include "win32_raii.hpp"

#include <windows.h>
#include <tlhelp32.h>

#include <cstdio>
#include <string>
#include <vector>

namespace k6wp::launcher {

namespace {

// Per-session pipe base (MED-12 follow-up): the engine serves ONLY
// \\.\pipe\k6wp-engine-<session_id>. This TU must NEVER link k6wp_shared
// (see CMakeLists.txt: zero-dep keeps the Qt-free proof trivial), so the
// suffix is computed locally with the same ProcessIdToSessionId logic as
// k6wp::CurrentSessionPipeName() - keep the two in sync.
constexpr wchar_t kPipeBaseName[] = L"\\\\.\\pipe\\k6wp-engine";

constexpr DWORD kEngineQuitAckTimeoutMs = 2000;  // dev-contracts §1 ack deadline
constexpr DWORD kEngineExitWaitMs = 8000;        // dev-contracts §1 PID wait
constexpr DWORD kForceTerminateWaitMs = 2000;    // after TerminateProcess
constexpr DWORD kWindowCloseWaitMs = 3000;       // == uninstall.bat ping -n 4
constexpr DWORD kWaitSliceMs = 100;              // cancellable wait slices
constexpr DWORD kPipeCancelDrainMs = 1000;       // OVERLAPPED cancel drain
constexpr int kIpcProtocolVersion = 1;            // shared/ipc_protocol.hpp

std::wstring SessionPipeNameImpl() {
  DWORD session_id = 0;
  if (!ProcessIdToSessionId(GetCurrentProcessId(), &session_id)) {
    session_id = 0;
  }
  return std::wstring(kPipeBaseName) + L"-" + std::to_wstring(session_id);
}

}  // namespace

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

namespace {

enum class PipeSendResult { kAckOk, kNoEngine, kNoAck, kError };

// Engine pipe request/response. Every step is bounded: a missing, busy, or hung
// engine returns a result instead of blocking the uninstaller forever.
PipeSendResult SendEngineCommand(const std::string& line, DWORD ack_timeout_ms,
                                 std::string* ack_out) {
  if (ack_out != nullptr) ack_out->clear();
  const std::wstring pipe_name = SessionPipeNameImpl();
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

}  // namespace

std::wstring SessionPipeName() { return SessionPipeNameImpl(); }

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

}  // namespace k6wp::launcher
