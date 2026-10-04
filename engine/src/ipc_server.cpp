#include "ipc_server.hpp"
#include "log_file.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "ipc_protocol.hpp"

namespace k6wp {
namespace {

// 128 KiB datagram buffer: comfortably holds the largest legal message
// (64 KiB payload + JSON envelope) in a single message-mode ReadFile.
constexpr DWORD kPipeBufferBytes = 128 * 1024;
// The server accepts one client at a time, so 4 instances is generous
// headroom while bounding handle use against a hostile connect/disconnect
// race. Diagnostics counter incremented per accepted client.
constexpr DWORD kMaxPipeInstances = 4;
std::atomic<unsigned long long> g_clients_served{0};

// File-local RAII guards (project rule: no raw new/delete; handles closed
// exactly once even on early return).
struct HandleGuard {
  HANDLE h = nullptr;
  explicit HandleGuard(HANDLE handle) : h(handle) {}
  ~HandleGuard() {
    if (h != nullptr && h != INVALID_HANDLE_VALUE) CloseHandle(h);
  }
  HandleGuard(const HandleGuard&) = delete;
  HandleGuard& operator=(const HandleGuard&) = delete;
  HANDLE release() {
    HANDLE out = h;
    h = nullptr;
    return out;
  }
};

struct SaGuard {
  PSECURITY_ATTRIBUTES sa = nullptr;
  explicit SaGuard(PSECURITY_ATTRIBUTES s) : sa(s) {}
  ~SaGuard() {
    if (sa != nullptr) {
      if (sa->lpSecurityDescriptor != nullptr) LocalFree(sa->lpSecurityDescriptor);
      LocalFree(sa);
    }
  }
  SaGuard(const SaGuard&) = delete;
  SaGuard& operator=(const SaGuard&) = delete;
};

void Log(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  EngineLogfV(fmt, args);
  va_end(args);
}

// Minimal JSON string escaper for {"error":"..."} payloads.
std::string JsonEscape(const std::string& s) {
  std::string out;
  out.reserve(s.size() + 2);
  for (const char c : s) {
    switch (c) {
      case '"': out += "\\\""; break;
      case '\\': out += "\\\\"; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default:
        if (c >= 0 && c < 0x20) {
          char hex[8] = {};
          std::snprintf(hex, sizeof(hex), "\\u%04x", c);
          out += hex;
        } else {
          out += c;
        }
    }
  }
  return out;
}

std::string ErrorAck(const std::string& msg) {
  return "{\"error\":\"" + JsonEscape(msg) + "\"}\n";
}

// Event-driven wait (MED-6): blocks INFINITE on {op event, stop event} —
// zero wakeups while idle, no 10 Hz slice churn. On stop, cancels the
// pending I/O, drains the op event, and returns false (caller must
// DisconnectNamedPipe + CloseHandle and exit).
bool WaitWithStop(HANDLE pipe, OVERLAPPED* ov, HANDLE event,
                  HANDLE stop_event) {
  HANDLE handles[2] = {event, stop_event};
  const DWORD w = WaitForMultipleObjects(2, handles, FALSE, INFINITE);
  if (w == WAIT_OBJECT_0) return true;
  if (w == WAIT_OBJECT_0 + 1) {
    CancelIoEx(pipe, ov);
    WaitForSingleObject(event, INFINITE);
    return false;
  }
  // WAIT_FAILED / abandoned → treat as a disconnect and let the caller
  // re-listen.
  return true;
}

}  // namespace

struct IpcServer::Impl {
  std::thread worker;
  std::atomic<bool> stop{false};
  std::atomic<bool> running{false};
  HANDLE stop_event = nullptr;
  IpcHandlers handlers;

  Impl() { stop_event = CreateEventW(nullptr, TRUE, FALSE, nullptr); }
  ~Impl() {
    if (stop_event != nullptr) CloseHandle(stop_event);
  }
  Impl(const Impl&) = delete;
  Impl& operator=(const Impl&) = delete;

  // Builds the ack for one raw datagram. Returns "" when no reply must be
  // sent (empty or frameless datagram — the client's read times out; this is
  // the QA-fail path and must never crash or reply).
  std::string HandleMessage(const std::string& raw) {
    if (raw.empty()) return "";
    // Framing policy: Encode() always emits a trailing '\n'. A message-mode
    // datagram without one is not a complete NDJSON line → no ack.
    if (raw.back() != '\n') {
      Log("ipc: frameless datagram (%llu bytes), no ack (client will time out)",
          static_cast<unsigned long long>(raw.size()));
      return "";
    }
    IpcMessage msg;
    if (!Decode(raw, msg)) {
      return ErrorAck(
          "malformed IPC message (bad JSON, version, cmd, size, or frame)");
    }
    try {
      switch (msg.cmd) {
        case Cmd::set_video: {
          const std::string payload = msg.payload.dump();
          if (!handlers.set_video) return ErrorAck("set_video handler not installed");
          if (!handlers.set_video(payload)) return ErrorAck("set_video handler rejected the command");
          return "{\"ok\":true}\n";
        }
        case Cmd::set_monitor: {
          const std::string payload = msg.payload.dump();
          if (!handlers.set_monitor) return ErrorAck("set_monitor handler not installed");
          if (!handlers.set_monitor(payload)) return ErrorAck("set_monitor handler rejected the command");
          return "{\"ok\":true}\n";
        }
        case Cmd::set_display_video: {
          const std::string payload = msg.payload.dump();
          if (!handlers.set_display_video) return ErrorAck("set_display_video handler not installed");
          if (!handlers.set_display_video(payload)) return ErrorAck("set_display_video handler rejected the command");
          return "{\"ok\":true}\n";
        }
        case Cmd::pause:
          if (!handlers.pause) return ErrorAck("pause handler not installed");
          handlers.pause();
          return "{\"ok\":true}\n";
        case Cmd::resume:
          if (!handlers.resume) return ErrorAck("resume handler not installed");
          handlers.resume();
          return "{\"ok\":true}\n";
        case Cmd::get_state: {
          if (!handlers.get_state) return ErrorAck("get_state handler not installed");
          std::string state = handlers.get_state();
          if (state.empty()) state = "{}";
          return "{\"ok\":true,\"state\":" + state + "}\n";
        }
        case Cmd::quit: {
          if (!handlers.quit) return ErrorAck("quit handler not installed");
          handlers.quit();
          return "{\"ok\":true}\n";
        }
      }
    } catch (const std::exception& e) {
      return ErrorAck(std::string("handler threw: ") + e.what());
    } catch (...) {
      return ErrorAck("handler threw an unknown exception");
    }
    return ErrorAck("unknown command");
  }

  // Accepts one client and serves it until disconnect or stop. Returns true
  // to re-listen (client went away), false when stop was requested.
  bool AcceptAndServe() {
    SaGuard sa(MakeCurrentUserOnlySA());
    if (sa.sa == nullptr) {
      Log("ipc: MakeCurrentUserOnlySA failed, retrying");
      WaitForSingleObject(stop_event, 250);
      return !stop.load(std::memory_order_relaxed);
    }
  // Per-session pipe (MED-12): serve \\.\pipe\k6wp-engine-<session_id> for
  // THIS process's session (ProcessIdToSessionId inside
  // CurrentSessionPipeName) so concurrent sessions stay isolated. Resolved
  // per accept so a session change across restarts can never serve stale.
  const std::wstring pipe_name = CurrentSessionPipeName();
  HandleGuard pipe(CreateNamedPipeW(
      pipe_name.c_str(), PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        kMaxPipeInstances, kPipeBufferBytes, kPipeBufferBytes,
        /*nDefaultTimeOut=*/0, sa.sa));
    if (pipe.h == INVALID_HANDLE_VALUE) {
      const DWORD create_err = GetLastError();
      if (create_err == ERROR_PIPE_BUSY) {
        Log("ipc: pipe instances exhausted (cap %lu), retrying",
            static_cast<unsigned long>(kMaxPipeInstances));
      } else {
        Log("ipc: CreateNamedPipeW failed (error %lu), retrying", create_err);
      }
      WaitForSingleObject(stop_event, 250);
      return !stop.load(std::memory_order_relaxed);
    }

    // Overlapped connect so Stop() wakes the accept immediately via stop_event.
    HandleGuard connect_event(CreateEventW(nullptr, TRUE, FALSE, nullptr));
    if (connect_event.h == nullptr) {
      Log("ipc: CreateEventW failed (error %lu), retrying", GetLastError());
      WaitForSingleObject(stop_event, 250);
      return !stop.load(std::memory_order_relaxed);
    }
    OVERLAPPED connect_ov{};
    connect_ov.hEvent = connect_event.h;
    const BOOL connected = ConnectNamedPipe(pipe.h, &connect_ov);
    if (connected) {
      // Synchronous success (rare) — client already waiting.
    } else {
      const DWORD e = GetLastError();
      if (e == ERROR_PIPE_CONNECTED) {
        // Client connected between Create and Connect — success.
      } else if (e == ERROR_IO_PENDING) {
        if (!WaitWithStop(pipe.h, &connect_ov, connect_event.h, stop_event)) {
          DisconnectNamedPipe(pipe.h);
          return false;
        }
        DWORD ignored = 0;
        if (!GetOverlappedResult(pipe.h, &connect_ov, &ignored, FALSE)) {
          const DWORD ge = GetLastError();
          if (ge == ERROR_PIPE_CONNECTED) {
            // Won the race — treat as connected.
          } else {
            Log("ipc: accept failed (error %lu), re-listening", ge);
            DisconnectNamedPipe(pipe.h);
            return !stop.load(std::memory_order_relaxed);
          }
        }
      } else {
        Log("ipc: ConnectNamedPipe failed (error %lu), re-listening", e);
        DisconnectNamedPipe(pipe.h);
        return !stop.load(std::memory_order_relaxed);
      }
    }
    const unsigned long long served = g_clients_served.fetch_add(1) + 1;
    Log("ipc: client connected (served=%llu)", served);

    std::vector<char> buf(static_cast<size_t>(kPipeBufferBytes));
    HandleGuard read_event(CreateEventW(nullptr, TRUE, FALSE, nullptr));
    if (read_event.h == nullptr) {
      Log("ipc: CreateEventW failed (error %lu), dropping client", GetLastError());
      DisconnectNamedPipe(pipe.h);
      return !stop.load(std::memory_order_relaxed);
    }

    for (;;) {
      if (stop.load(std::memory_order_relaxed)) {
        DisconnectNamedPipe(pipe.h);
        return false;
      }
      ResetEvent(read_event.h);
      OVERLAPPED read_ov{};
      read_ov.hEvent = read_event.h;
      DWORD n = 0;
      const BOOL rok = ReadFile(pipe.h, buf.data(), kPipeBufferBytes, &n, &read_ov);
      if (!rok) {
        const DWORD e = GetLastError();
        if (e == ERROR_IO_PENDING) {
          if (!WaitWithStop(pipe.h, &read_ov, read_event.h, stop_event)) {
            DisconnectNamedPipe(pipe.h);
            return false;
          }
          if (!GetOverlappedResult(pipe.h, &read_ov, &n, FALSE)) {
            const DWORD ge = GetLastError();
            if (ge == ERROR_BROKEN_PIPE || ge == ERROR_PIPE_NOT_CONNECTED) {
              Log("ipc: client disconnected, re-listening");
              DisconnectNamedPipe(pipe.h);
              return true;
            }
            if (ge == ERROR_MORE_DATA) {
              // Datagram larger than 128 KiB — drain the remainder with
              // blocking reads, then reject (Decode would refuse it anyway).
              DWORD drain = 0;
              while (!ReadFile(pipe.h, buf.data(), kPipeBufferBytes, &drain, nullptr) &&
                     GetLastError() == ERROR_MORE_DATA) {
              }
              const std::string ack = ErrorAck("message exceeds maximum size");
              DWORD written = 0;
              WriteFile(pipe.h, ack.data(), static_cast<DWORD>(ack.size()), &written,
                        nullptr);
              continue;
            }
            Log("ipc: read failed (error %lu), re-listening", ge);
            DisconnectNamedPipe(pipe.h);
            return true;
          }
        } else if (e == ERROR_MORE_DATA) {
          // Datagram larger than 128 KiB arrived before the overlapped
          // read parked: synchronous MORE_DATA (same drain+ack as the
          // async GetOverlappedResult path above — Todo 31 fuzz msg 0).
          DWORD drain = 0;
          while (!ReadFile(pipe.h, buf.data(), kPipeBufferBytes, &drain, nullptr) &&
                 GetLastError() == ERROR_MORE_DATA) {
          }
          const std::string ack = ErrorAck("message exceeds maximum size");
          DWORD written = 0;
          WriteFile(pipe.h, ack.data(), static_cast<DWORD>(ack.size()), &written,
                    nullptr);
          continue;
        } else if (e == ERROR_BROKEN_PIPE || e == ERROR_PIPE_NOT_CONNECTED) {
          // Brutal disconnect: client vanished. Re-listen, never crash.
          Log("ipc: client disconnected (brutal), re-listening");
          DisconnectNamedPipe(pipe.h);
          return true;
        } else {
          Log("ipc: ReadFile failed (error %lu), re-listening", e);
          DisconnectNamedPipe(pipe.h);
          return true;
        }
      }

      const std::string ack = HandleMessage(std::string(buf.data(), n));
      if (ack.empty()) continue;  // Frameless/empty: no reply by policy.
      DWORD written = 0;
      const BOOL wok = WriteFile(pipe.h, ack.data(), static_cast<DWORD>(ack.size()),
                                 &written, nullptr);
      if (!wok || written != ack.size()) {
        Log("ipc: ack write failed (error %lu), re-listening", GetLastError());
        DisconnectNamedPipe(pipe.h);
        return true;
      }
    }
  }

  void ServeLoop() {
    Log("ipc: server thread started");
    while (!stop.load(std::memory_order_relaxed)) {
      if (!AcceptAndServe()) break;
    }
    Log("ipc: server thread stopped");
  }
};

IpcServer::IpcServer() : impl_(std::make_unique<Impl>()) {}

IpcServer::~IpcServer() { Stop(); }

bool IpcServer::Start(const IpcHandlers& handlers) {
  if (impl_->running.load(std::memory_order_relaxed)) return true;
  if (impl_->stop_event == nullptr) {
    impl_->stop_event = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (impl_->stop_event == nullptr) {
      Log("ipc: CreateEventW (stop) failed (error %lu)", GetLastError());
      return false;
    }
  }
  ResetEvent(impl_->stop_event);
  impl_->stop.store(false, std::memory_order_relaxed);
  impl_->handlers = handlers;
  try {
    impl_->worker = std::thread(&Impl::ServeLoop, impl_.get());
  } catch (const std::exception& e) {
    Log("ipc: failed to start server thread: %s", e.what());
    return false;
  }
  impl_->running.store(true, std::memory_order_relaxed);
  return true;
}

void IpcServer::Stop() {
  if (!impl_->running.load(std::memory_order_relaxed)) return;
  impl_->running.store(false, std::memory_order_relaxed);
  impl_->stop.store(true, std::memory_order_relaxed);
  if (impl_->stop_event != nullptr) SetEvent(impl_->stop_event);
  if (impl_->worker.joinable()) impl_->worker.join();
}

bool IpcServer::IsRunning() const {
  return impl_->running.load(std::memory_order_relaxed);
}

}  // namespace k6wp
