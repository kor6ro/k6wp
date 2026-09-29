#pragma once

// Overlapped named-pipe IPC server for the wallpaper engine (Todo 28).
//
// Listens on k6wp::CurrentSessionPipeName() (per-session
// \\.\pipe\k6wp-engine-<session_id>, PIPE_ACCESS_DUPLEX + FILE_FLAG_OVERLAPPED,
// PIPE_TYPE_MESSAGE), accepts one client at a time on a worker thread,
// parses each message-mode datagram as NDJSON via k6wp::Decode(), dispatches
// to the renderer hooks (Todo 30 does the full wiring; until then the
// handlers installed by EngineApp are logging stubs), and replies with a
// single NDJSON ack line: {"ok":true} (or {"ok":true,"state":{...}} for
// get_state) or {"error":"..."}.
//
// windows.h lives in the .cpp only — this header exposes std types only.

#include <functional>
#include <memory>
#include <string>

namespace k6wp {

// Callbacks invoked on the server worker thread. All are optional; an unset
// callback falls back to a logging stub that reports success. Payloads are
// passed as raw JSON text (IpcMessage::payload dumped); get_state returns
// raw JSON object text embedded verbatim into the ack.
//
// Ack contract (CRIT-2, audit-remediation todo 11 — "diterima" vs "selesai"):
// set_video / set_monitor handlers VALIDATE on this worker thread and queue
// the command for the main message loop (private UINT + PostMessageW); their
// true return — the {"ok":true} ack — means "diterima" (accepted + queued),
// NOT "selesai" (applied to the desktop). Window creation and renderer
// teardown only ever run on the main loop thread. Clients that need
// certainty verify via get_state (video/monitor fields reflect applied
// state). A false return means {"error"} ("ditolak"): nothing was queued.
struct IpcHandlers {
  // Return true = {"ok":true}, false = {"error":"handler rejected ..."}.
  std::function<bool(const std::string& payload_json)> set_video;
  std::function<bool(const std::string& payload_json)> set_monitor;
  std::function<void()> pause;
  std::function<void()> resume;
  // Must return a JSON object (e.g. "{}" when nothing to report).
  std::function<std::string()> get_state;
  // Graceful shutdown request: the server acks {"ok":true} FIRST, then the
  // handler must trigger shutdown asynchronously (e.g. PostMessage to the
  // UI thread). Never destroys state inline on the IPC worker thread.
  std::function<void()> quit;
};

// RAII pipe server. Start() spawns the worker thread (idempotent — a second
// call is a no-op returning true); Stop() signals shutdown, unblocks any
// pending Connect/Read and joins the thread. The destructor calls Stop().
// A brutal client disconnect (ERROR_BROKEN_PIPE) never escapes: the worker
// disconnects the instance and re-listens (auto-relisten loop).
class IpcServer {
 public:
  IpcServer();
  ~IpcServer();

  IpcServer(const IpcServer&) = delete;
  IpcServer& operator=(const IpcServer&) = delete;
  IpcServer(IpcServer&&) = delete;
  IpcServer& operator=(IpcServer&&) = delete;

  // Starts listening with the given handlers. Returns false only if the
  // worker thread cannot be created. Safe to call twice.
  bool Start(const IpcHandlers& handlers);

  // Stops the server and joins the worker thread. Idempotent.
  void Stop();

  // True after a successful Start() and before Stop().
  bool IsRunning() const;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace k6wp
