#include "ipc_command_marshal.hpp"

#include "ipc_marshal.hpp"

namespace k6wp {

bool IpcCommandMarshal::QueueVideo(const std::string& payload_json) {
  // Worker-side half: validate only (pure, ipc_marshal.hpp). A reject here
  // acks {"error"} and queues nothing — the old video keeps playing.
  if (!ValidateSetVideoPayload(payload_json)) {
    if (log_) {
      log_("ipc: set_video rejected (missing/invalid path or not a file)");
    }
    return false;
  }
  const HWND hwnd = static_cast<HWND>(hwnd_.load(std::memory_order_acquire));
  pending_.SetVideo(payload_json);
  // Post AFTER storing: the main thread pops under the same mutex, so it can
  // never observe the flag without the payload. A null window means teardown
  // already started — drop loudly.
  if (hwnd == nullptr || !PostMessageW(hwnd, kSetVideoMessage, 0, 0)) {
    pending_.ClearVideo();
    if (log_) {
      log_("warning: set_video post failed (error %lu), command dropped",
           GetLastError());
    }
    return false;
  }
  if (log_) {
    log_("ipc: set_video diterima (queued for main loop, verify via get_state)");
  }
  return true;
}

bool IpcCommandMarshal::QueueMonitor(const std::string& payload_json) {
  if (!ParseSetMonitorPayload(payload_json)) {
    if (log_) {
      log_("ipc: set_monitor rejected (missing/invalid \"monitor\" field)");
    }
    return false;
  }
  const HWND hwnd = static_cast<HWND>(hwnd_.load(std::memory_order_acquire));
  pending_.SetMonitor(payload_json);
  if (hwnd == nullptr || !PostMessageW(hwnd, kSetMonitorMessage, 0, 0)) {
    pending_.ClearMonitor();
    if (log_) {
      log_("warning: set_monitor post failed (error %lu), command dropped",
           GetLastError());
    }
    return false;
  }
  if (log_) {
    log_("ipc: set_monitor diterima (queued for main loop, verify via get_state)");
  }
  return true;
}

}  // namespace k6wp
