#pragma once

#include <atomic>
#include <string>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include "pending_queue.hpp"

namespace k6wp {

// CRIT-2 worker -> main-loop marshal for set_video / set_monitor /
// set_display_video. These private UINTs are posted to the hidden window;
// the main loop dispatches on them.
inline constexpr UINT kSetMonitorMessage = WM_APP + 0x54u;
inline constexpr UINT kSetVideoMessage = WM_APP + 0x55u;
inline constexpr UINT kSetDisplayVideoMessage = WM_APP + 0x56u;

// Pairwise-distinct guarantee (timer_ids.hpp convention,
// docs/compliance-matrix.md:33): an accidental duplicate private message id
// fails the build instead of silently aliasing two marshal channels on the
// same hidden window.
static_assert(kSetDisplayVideoMessage != kSetMonitorMessage,
              "marshal message IDs must be pairwise distinct");
static_assert(kSetDisplayVideoMessage != kSetVideoMessage,
              "marshal message IDs must be pairwise distinct");
static_assert(kSetMonitorMessage != kSetVideoMessage,
              "marshal message IDs must be pairwise distinct");

// The IPC worker validates + stashes a payload and posts the matching message;
// the main loop pops and runs the executor. One slot each (last write wins).
class IpcCommandMarshal {
 public:
  using LogFn = void (*)(const char* fmt, ...);

  explicit IpcCommandMarshal(LogFn log = nullptr) : log_(log) {}

  // Set/clear the hidden window (atomic: read from the IPC worker thread).
  void SetWindow(void* hwnd) { hwnd_.store(hwnd, std::memory_order_release); }
  void ClearWindow() { hwnd_.store(nullptr, std::memory_order_release); }

  // Worker-side halves: validate + stash + PostMessage. False => {"error"} ack.
  bool QueueVideo(const std::string& payload_json);
  bool QueueMonitor(const std::string& payload_json);
  bool QueueDisplayVideo(const std::string& payload_json);

  // Main-loop halves: pop the pending payload (false when empty).
  bool TakeVideo(std::string* out) { return pending_.TakeVideo(out); }
  bool TakeMonitor(std::string* out) { return pending_.TakeMonitor(out); }
  bool TakeDisplayVideo(std::string* out) {
    return pending_.TakeDisplayVideo(out);
  }

 private:
  LogFn log_;
  std::atomic<void*> hwnd_{nullptr};
  PendingCommandQueue pending_;
};

}  // namespace k6wp
