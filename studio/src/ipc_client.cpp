#include "ipc_client.hpp"

// windows.h lives ONLY here (header stays Win32-free, desktop_inject pattern).
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <chrono>
#include <thread>

namespace k6wp {
namespace {

// Retry policy (Todo 29 accept): 3 attempts, 500ms apart; each
// WaitNamedPipeW waits up to 500ms for the pipe instance to appear.
constexpr int kMaxAttempts = 3;
constexpr DWORD kWaitPipeMs = 500;
constexpr DWORD kRetryGapMs = 500;
// Ack read deadline (Todo 28 QA: unterminated message must time out, 2s).
constexpr DWORD kReadDeadlineMs = 2000;
// LOW-6: a held pipe is reaped after this long without a Send, so polling
// loops (get_state watchers) reuse one connection instead of reconnecting
// per poll, while a stale handle is never reused across process restarts.
constexpr long long kIdleDisconnectMs = 5000;

std::string WinError(DWORD code) {
  char* buf = nullptr;
  const DWORD n = FormatMessageA(
      FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
          FORMAT_MESSAGE_IGNORE_INSERTS,
      nullptr, code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      reinterpret_cast<LPSTR>(&buf), 0, nullptr);
  std::string msg =
      (n != 0 && buf != nullptr) ? std::string(buf, n) : "unknown error";
  if (buf != nullptr) {
    LocalFree(buf);
  }
  while (!msg.empty() && (msg.back() == '\n' || msg.back() == '\r')) {
    msg.pop_back();
  }
  return msg;
}

}  // namespace

std::uint32_t EnginePidFromState(const IpcResult& res) {
  try {
    if (res.status != IpcStatus::kOk) return 0;
    if (!res.raw.is_object() || !res.raw.contains("state")) return 0;
    const auto& state = res.raw.at("state");
    if (!state.is_object() || !state.contains("pid")) return 0;
    const auto& pid = state.at("pid");
    unsigned long long v = 0;
    if (pid.is_number_unsigned()) {
      v = pid.get<unsigned long long>();
    } else if (pid.is_number_integer()) {
      const long long s = pid.get<long long>();
      if (s <= 0) return 0;
      v = static_cast<unsigned long long>(s);
    } else {
      return 0;
    }
    if (v == 0 || v > 0xFFFFFFFFull) return 0;
    return static_cast<std::uint32_t>(v);
  } catch (...) {
    return 0;
  }
}

// LOW-6: stamps last-use in its dtor so ALL Send() return paths are
// covered (replaces the old Step 2.2 release-every-transaction guard).
IpcClient::UseGuard::~UseGuard() {
  if (client != nullptr) {
    client->NoteUse();
  }
}

// RAII pipe handle: CloseHandle in dtor. No raw new/delete anywhere.
struct IpcClient::Impl {
  HANDLE pipe = INVALID_HANDLE_VALUE;
  // LOW-6: last Send() completion (steady clock). A held pipe older than
  // kIdleDisconnectMs is reaped at the next Send; touched only with
  // mutex_ held (or before publication in the ctor).
  std::chrono::steady_clock::time_point last_use{};

  Impl() = default;
  ~Impl() { Close(); }

  Impl(const Impl&) = delete;
  Impl& operator=(const Impl&) = delete;

  void Close() {
    if (pipe != INVALID_HANDLE_VALUE) {
      CloseHandle(pipe);
      pipe = INVALID_HANDLE_VALUE;
    }
  }

  bool connected() const { return pipe != INVALID_HANDLE_VALUE; }
};

IpcClient::IpcClient() : impl_(std::make_unique<Impl>()) {}

IpcClient::~IpcClient() = default;

IpcClient::IpcClient(IpcClient&& other) noexcept
    : impl_(std::move(other.impl_)), log_(std::move(other.log_)) {
  // mutex_ is per-instance and never moved (a moved-from client keeps a
  // valid unlocked mutex; both sides stay independently usable). cancel_
  // is atomic (also non-movable): carry the flag value across instead.
  cancel_.store(other.cancel_.load(std::memory_order_relaxed),
                std::memory_order_relaxed);
}
IpcClient& IpcClient::operator=(IpcClient&& other) noexcept {
  if (this != &other) {
    std::lock_guard<std::mutex> lock(mutex_);
    impl_ = std::move(other.impl_);
    log_ = std::move(other.log_);
    cancel_.store(other.cancel_.load(std::memory_order_relaxed),
                  std::memory_order_relaxed);
  }
  return *this;
}

void IpcClient::AppendLog(const std::string& line) {
  log_.push_back(line);
  constexpr std::size_t kMaxLogLines = 500;
  while (log_.size() > kMaxLogLines) {
    log_.erase(log_.begin());
  }
}

bool IpcClient::IsConnected() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return IsConnectedLocked();
}

bool IpcClient::IsConnectedLocked() const {
  return impl_ != nullptr && impl_->connected();
}

void IpcClient::Disconnect() {
  std::lock_guard<std::mutex> lock(mutex_);
  DisconnectLocked();
}

void IpcClient::DisconnectLocked() {
  if (impl_ != nullptr) {
    impl_->Close();
  }
}

void IpcClient::NoteUse() {
  if (impl_ != nullptr) {
    impl_->last_use = std::chrono::steady_clock::now();
  }
}

void IpcClient::ReapIdleConnection() {
  if (impl_ == nullptr || !impl_->connected()) {
    return;
  }
  if (impl_->last_use == std::chrono::steady_clock::time_point{}) {
    return;
  }
  const auto idle = std::chrono::steady_clock::now() - impl_->last_use;
  if (idle > std::chrono::milliseconds(kIdleDisconnectMs)) {
    AppendLog("Ipc: idle timeout, disconnecting");
    DisconnectLocked();
  }
}

bool IpcClient::Connect(std::string* error_out) {
  std::lock_guard<std::mutex> lock(mutex_);
  return ConnectLocked(error_out);
}

bool IpcClient::ConnectLocked(std::string* error_out) {
  if (IsConnectedLocked()) {
    return true;
  }
  // Per-session pipe (MED-12): the engine listens on
  // \\.\pipe\k6wp-engine-<session_id> for THIS process's session, so resolve
  // once per Connect and use it for the log line, WaitNamedPipeW, and
  // CreateFileW alike.
  const std::wstring pipe_name = CurrentSessionPipeName();
  AppendLog("Ipc: connecting to " +
            std::filesystem::path(pipe_name).string() + " ...");
  for (int attempt = 1; attempt <= kMaxAttempts; ++attempt) {
    // Wait up to 500ms for a pipe instance (covers "pipe busy": all
    // instances in use → wait for one to free up).
    const BOOL waited = WaitNamedPipeW(pipe_name.c_str(), kWaitPipeMs);
    if (!waited) {
      const DWORD code = GetLastError();
      AppendLog("Ipc: connect attempt " + std::to_string(attempt) + "/" +
                std::to_string(kMaxAttempts) +
                ": WaitNamedPipe failed: " + WinError(code) + " (" +
                std::to_string(code) + ")");
    } else {
      HANDLE h = CreateFileW(pipe_name.c_str(), GENERIC_READ | GENERIC_WRITE, 0,
                              nullptr, OPEN_EXISTING, 0, nullptr);
      if (h != INVALID_HANDLE_VALUE) {
        // Message-mode pipe (Todo 28 server uses PIPE_TYPE_MESSAGE):
        // verify the mode so ReadFile returns whole NDJSON ack lines.
        DWORD mode = PIPE_READMODE_MESSAGE;
        if (SetNamedPipeHandleState(h, &mode, nullptr, nullptr) == 0) {
          const DWORD code = GetLastError();
          AppendLog("Ipc: SetNamedPipeHandleState failed: " +
                    WinError(code));
          CloseHandle(h);
        } else {
          impl_->pipe = h;
          AppendLog("Ipc: connected (attempt " + std::to_string(attempt) +
                    ")");
          return true;
        }
      } else {
        const DWORD code = GetLastError();
        AppendLog("Ipc: connect attempt " + std::to_string(attempt) + "/" +
                  std::to_string(kMaxAttempts) + ": CreateFile failed: " +
                  WinError(code) + " (" + std::to_string(code) + ")");
      }
    }
    if (attempt < kMaxAttempts) {
      // HIGH-3 slice B: cancellable gap — 50x10ms slices instead of one
      // 500ms sleep so RequestCancel() aborts a worker Send promptly.
      // Never TerminateThread; the worker just returns kNotRunning.
      for (DWORD waited_ms = 0; waited_ms < kRetryGapMs; waited_ms += 10) {
        if (IsCancelRequested()) {
          const std::string msg = "Ipc: connect cancelled";
          AppendLog(msg);
          if (error_out != nullptr) {
            *error_out = msg;
          }
          return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
      }
    }
  }
  const std::string msg =
      "Ipc: engine not running (pipe unavailable after " +
      std::to_string(kMaxAttempts) +
      " attempts); offer Start via ApplyManager::RestartEngine";
  AppendLog(msg);
  if (error_out != nullptr) {
    *error_out = msg;
  }
  return false;
}

bool IpcClient::EnsureConnected(std::string* error_out) {
  if (IsConnectedLocked()) {
    return true;
  }
  return ConnectLocked(error_out);
}

IpcResult IpcClient::Send(Cmd cmd, const nlohmann::json& payload) {
  // T15: one atomic transaction per Send — connect + write + ack-read all
  // happen under mutex_, so concurrent set_video calls serialize instead of
  // interleaving on the shared pipe handle.
  std::lock_guard<std::mutex> lock(mutex_);
  // LOW-6: reap a pipe idle past the budget; otherwise KEEP the connection
  // across Sends (exactly one reconnect per idle window, not one per poll).
  ReapIdleConnection();
  const UseGuard use_guard(this);
  IpcResult out;
  const char* name = CmdToString(cmd);
  const std::string cmd_name = (name != nullptr) ? name : "?";

  std::string err;
  if (!EnsureConnected(&err)) {
    // Connect() reports two different failures as one bool: the pipe really is
    // absent, or the caller cancelled. A cancel must not be reported as
    // "engine not running" - Studio shows that as the engine being down and
    // offers to start it, which is wrong when the engine is up and the user
    // simply asked to stop waiting. It also mis-flakes the mid-flight cancel
    // test whenever the cancel lands during connect rather than after it.
    out.status = IsCancelRequested() ? IpcStatus::kError
                                     : IpcStatus::kNotRunning;
    out.error = err;
    return out;
  }

  // Encode (throws IpcError on payload > 64KiB — Todo 6).
  std::string line;
  try {
    IpcMessage msg;
    msg.version = kProtocolVersion;
    msg.cmd = cmd;
    msg.payload = payload;
    line = Encode(msg);
  } catch (const IpcError& e) {
    out.status = IpcStatus::kError;
    out.error = std::string("Ipc: encode ") + cmd_name + " failed: " + e.what();
    AppendLog(out.error);
    return out;
  }

  // Write the whole NDJSON line as one message.
  DWORD written = 0;
  const BOOL wok = WriteFile(impl_->pipe, line.data(),
                             static_cast<DWORD>(line.size()), &written,
                             nullptr);
  if (wok == 0 || written != line.size()) {
    const DWORD code = GetLastError();
    out.status = IpcStatus::kError;
    out.error = "Ipc: write " + cmd_name + " failed: " + WinError(code);
    AppendLog(out.error);
    // Broken pipe → drop the handle so the next Send reconnects; map a
    // broken pipe on write to not-running (engine died mid-session).
    if (code == ERROR_BROKEN_PIPE || code == ERROR_PIPE_NOT_CONNECTED) {
      DisconnectLocked();
      out.status = IpcStatus::kNotRunning;
      out.error += " (engine closed the pipe)";
      AppendLog("Ipc: pipe broken; offer Start via RestartEngine");
    }
    return out;
  }

  // Read one ack line with a 2s deadline (synchronous handle + PeekNamedPipe
  // poll; message-mode preserves boundaries so one ReadFile = one ack).
  // HIGH-3 slice B: this poll runs on a QtConcurrent worker, never the GUI
  // thread; the 10ms sleep slices also check the atomic cancel flag.
  const auto deadline =
      std::chrono::steady_clock::now() + std::chrono::milliseconds(kReadDeadlineMs);
  DWORD avail = 0;
  BOOL have_data = FALSE;
  for (;;) {
    if (IsCancelRequested()) {
      out.status = IpcStatus::kError;
      out.error = "Ipc: Send " + cmd_name + " cancelled";
      AppendLog(out.error);
      // Drop the connection: the engine may still answer later, and that late
      // ack would otherwise be read as the reply to the NEXT command.
      DisconnectLocked();
      return out;
    }
    DWORD bytes_left = 0;
    have_data = PeekNamedPipe(impl_->pipe, nullptr, 0, nullptr, &avail,
                              &bytes_left);
    if (have_data == 0) {
      const DWORD code = GetLastError();
      out.status = IpcStatus::kError;
      out.error =
          "Ipc: PeekNamedPipe failed: " + WinError(code);
      AppendLog(out.error);
      if (code == ERROR_BROKEN_PIPE || code == ERROR_PIPE_NOT_CONNECTED) {
        DisconnectLocked();
        out.status = IpcStatus::kNotRunning;
      }
      return out;
    }
    if (avail > 0) {
      break;
    }
    if (std::chrono::steady_clock::now() >= deadline) {
      out.status = IpcStatus::kError;
      out.error = "Ipc: ack timeout (2s) for " + cmd_name +
                  " (engine sent no complete line)";
      AppendLog(out.error);
      // A late ack stays queued in the message-mode pipe; without dropping the
      // handle it would be consumed as the reply to the next Send (desync).
      DisconnectLocked();
      return out;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  std::string ack(avail, '\0');
  DWORD got = 0;
  const BOOL rok =
      ReadFile(impl_->pipe, ack.data(), avail, &got, nullptr);
  if (rok == 0 || got == 0) {
    const DWORD code = GetLastError();
    out.status = IpcStatus::kError;
    out.error = "Ipc: read ack for " + cmd_name + " failed: " + WinError(code);
    AppendLog(out.error);
    if (code == ERROR_BROKEN_PIPE || code == ERROR_PIPE_NOT_CONNECTED ||
        code == ERROR_MORE_DATA) {
      DisconnectLocked();
      if (code != ERROR_MORE_DATA) {
        out.status = IpcStatus::kNotRunning;
      }
    }
    return out;
  }
  ack.resize(got);

  // Ack shapes: Todo 28 acks are {"ok":true,...} / {"error":"..."} JSON
  // lines; IpcMessage lines also decode via k6wp::Decode. Try Decode first
  // (per Todo 29 spec), then fall back to raw ok/error JSON.
  IpcMessage decoded;
  if (Decode(ack, decoded)) {
    out.status = IpcStatus::kOk;
    out.reply = decoded;
    out.has_reply = true;
    out.raw = decoded.payload;
    AppendLog("Ipc: " + cmd_name + " ack OK (IpcMessage)");
    return out;
  }
  nlohmann::json raw;
  try {
    raw = nlohmann::json::parse(ack);
  } catch (const std::exception& e) {
    out.status = IpcStatus::kError;
    out.error =
        std::string("Ipc: malformed ack for ") + cmd_name + ": " + e.what();
    AppendLog(out.error);
    return out;
  }
  out.raw = raw;
  if (raw.is_object() && raw.contains("error")) {
    out.status = IpcStatus::kError;
    out.error = "Ipc: engine rejected " + cmd_name + ": " +
                raw.value("error", std::string("unknown error"));
    AppendLog(out.error);
    return out;
  }
  out.status = IpcStatus::kOk;
  AppendLog("Ipc: " + cmd_name + " ack OK");
  return out;
}

IpcResult IpcClient::SetVideo(const std::string& utf8_path) {
  return Send(Cmd::set_video, {{"path", utf8_path}});
}

IpcResult IpcClient::SetVideo(const std::filesystem::path& path) {
  return SetVideo(path.u8string());
}

IpcResult IpcClient::SetMonitor(int monitor_id) {
  return Send(Cmd::set_monitor, {{"monitor", monitor_id}});
}

IpcResult IpcClient::Pause() { return Send(Cmd::pause); }

IpcResult IpcClient::Resume() { return Send(Cmd::resume); }

IpcResult IpcClient::GetState() { return Send(Cmd::get_state); }

IpcResult IpcClient::Quit() { return Send(Cmd::quit); }

}  // namespace k6wp
