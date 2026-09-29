#pragma once

// Studio IPC client (Todo 29, Wave 4). Connects to the engine's per-session
// named pipe (k6wp::CurrentSessionPipeName(), see shared/ipc_protocol.hpp)
// and sends NDJSON commands.
//
// Engine-dead handling: Connect() returns false when the engine is not
// running (pipe absent after 3x retry). Send() on a dead engine returns
// IpcResult{status=kNotRunning} — DISTINCT from kError — so the UI can
// offer Start via ApplyManager::RestartEngine (Todo 25).
//
// Header stays windows.h-free (same pattern as engine/src/desktop_inject.hpp
// and shared/ipc_protocol.hpp): the pipe HANDLE lives in the .cpp Impl.

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "ipc_protocol.hpp"

namespace k6wp {

// Distinct outcome of an IPC operation. kNotRunning means "engine is not
// listening" (offer Start); kError means a real failure (log the message).
enum class IpcStatus {
  kOk,
  kNotRunning,
  kError,
};

// Result of Send() / convenience commands.
struct IpcResult {
  IpcStatus status = IpcStatus::kError;
  // Decoded ack when the engine replied with an IpcMessage (NDJSON line).
  IpcMessage reply;
  bool has_reply = false;
  // Raw ack JSON (Todo 28 acks are {"ok":true} / {"error":"..."} lines,
  // which are NOT IpcMessage-shaped; kept so the UI can inspect state).
  nlohmann::json raw = nlohmann::json::object();
  // Human-readable message for kError (also appended to log()).
  std::string error;
};

// Extracts the engine PID from a get_state ack (raw = {"ok":true,
// "state":{"pid":N,...}}). Returns 0 when missing or invalid. Single source
// shared by ApplyManager::RestartEngine and the Settings engine stop worker
// (was duplicated in both TUs). uint32_t == Win32 DWORD (header stays
// windows.h-free).
std::uint32_t EnginePidFromState(const IpcResult& res);

// T15: ONE shared IpcClient per Studio process (owned by MainWindow::ipc_,
// borrowed by ApplyManager via SetIpcClient). Concurrent set_video calls are
// serialized by mutex_: Send() holds it across connect + write + ack-read as
// one atomic transaction, so rapid sequential set_video calls never
// interleave bytes on the pipe. LOW-6: the pipe handle is KEPT across Sends
// and reaped only after 5 s idle (kIdleDisconnectMs in the .cpp) — no
// Disconnect per poll — with broken-pipe paths still dropping it at once so
// the next Send reconnects.
class IpcClient {
 public:
  IpcClient();
  ~IpcClient();

  IpcClient(const IpcClient&) = delete;
  IpcClient& operator=(const IpcClient&) = delete;
  IpcClient(IpcClient&& other) noexcept;
  IpcClient& operator=(IpcClient&& other) noexcept;

  // Opens the pipe: WaitNamedPipeW(pipe, 500) then
  // CreateFileW(pipe, GENERIC_READ|GENERIC_WRITE, 0, nullptr,
  //             OPEN_EXISTING, 0, nullptr). Retries 3x, 500ms apart;
  // ERROR_PIPE_BUSY retries immediately-ish (WaitNamedPipe already waited).
  // Returns true when connected. Returns false (logs the reason) when the
  // engine is not running — the caller should map that to kNotRunning /
  // offer Start via ApplyManager::RestartEngine. Idempotent: true at once
  // when already connected.
  bool Connect(std::string* error_out = nullptr);

  // Closes the pipe handle. Idempotent.
  void Disconnect();

  bool IsConnected() const;

  // Encodes {cmd, payload} via k6wp::Encode, writes the line, reads one ack
  // line (2s deadline), and decodes it. Auto-connects when disconnected.
  // Engine-dead → {kNotRunning}; ack {"error":"..."} or transport failure
  // → {kError, error}; otherwise {kOk} with reply/raw filled.
  IpcResult Send(Cmd cmd,
                 const nlohmann::json& payload = nlohmann::json::object());

  // Convenience commands (payload shapes match the engine dispatcher).
  IpcResult SetVideo(const std::string& utf8_path);
  IpcResult SetVideo(const std::filesystem::path& path);
  IpcResult SetMonitor(int monitor_id);
  IpcResult Pause();
  IpcResult Resume();
  IpcResult GetState();
  IpcResult Quit();

  const std::vector<std::string>& log() const { return log_; }
  void ClearLog() { log_.clear(); }

  // HIGH-3 slice B: cooperative cancellation for worker-thread Sends. The
  // blocking paths (Connect retries, 2s ack poll) sleep in ~10ms slices and
  // check this flag, so RequestCancel() aborts an in-flight Send within
  // milliseconds — never TerminateThread. Thread-safe; may be called from
  // the GUI thread while a worker runs Send (the shared client stays one
  // object, serialized by mutex_).
  void RequestCancel() { cancel_.store(true, std::memory_order_relaxed); }
  void ClearCancel() { cancel_.store(false, std::memory_order_relaxed); }
  bool IsCancelRequested() const {
    return cancel_.load(std::memory_order_relaxed);
  }

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
  std::vector<std::string> log_;
  // Serializes whole transactions (see class doc). Mutable so const
  // observers stay const; moves get a fresh mutex (never transfers a lock).
  mutable std::mutex mutex_;
  // Cancellation flag (see above). Atomic so the GUI thread can set it
  // while a worker thread polls it; never moved (like mutex_).
  std::atomic<bool> cancel_{false};

  void AppendLog(const std::string& line);
  // Ensures a live connection (connects when needed). False → not running.
  bool EnsureConnected(std::string* error_out);
  // LOW-6: stamps the last-use clock after a Send (called with mutex_
  // held). Implemented in the .cpp against Impl.
  void NoteUse();
  // LOW-6: drops the held pipe when it has been idle longer than the
  // idle budget (called with mutex_ held at the top of Send).
  void ReapIdleConnection();

  // LOW-6: scope guard that stamps last-use on every Send return path
  // (replaces the old per-transaction release guard).
  struct UseGuard {
    explicit UseGuard(IpcClient* c) : client(c) {}
    UseGuard(const UseGuard&) = delete;
    UseGuard& operator=(const UseGuard&) = delete;
    ~UseGuard();
    IpcClient* client;
  };
};

}  // namespace k6wp
