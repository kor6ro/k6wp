#pragma once

// Background refresher for the lock-screen frame (Bug A).
//
// WHY A THREAD
// The live frame comes from mpv's `screenshot-to-file`, and the VO thread
// writes that file ASYNCHRONOUSLY. The frame has to be moved into place
// once it lands - waiting for it must not block the message loop (the
// render loop must never stall because of a lock-screen picture), so the
// request is handed to this worker.
//
// WHAT IT DOES
// The worker prefers the LIVE frame (the picture on screen right now) and
// falls back to the file-based extract (`compressor --lockframe`) when the
// live capture is not available - the feature works either way.
//
// The newest request wins: a burst of events (rotation + lock + suspend)
// collapses into one refresh.
//
// windows.h-free (the header exposes std types only).

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>

#include "lockscreen.hpp"

namespace k6wp {

class LockscreenRefresher {
 public:
  // Matches EngineApp::Log (void(const char* fmt, ...)); may be null.
  using LogFn = void (*)(const char* fmt, ...);
  // The live-frame seam: writes a JPEG of the picture on screen to `path`
  // (UTF-8) and returns true. nullptr = no live frame available (the
  // file-based extract is used instead).
  using CaptureFn = bool (*)(const std::string& path);

  explicit LockscreenRefresher(LogFn log = nullptr) : log_(log) {}
  ~LockscreenRefresher();

  LockscreenRefresher(const LockscreenRefresher&) = delete;
  LockscreenRefresher& operator=(const LockscreenRefresher&) = delete;

  // Starts the worker. `capture` is the live-frame provider (may be null).
  // Never throws; a thread that cannot be started leaves the refresher
  // inactive (the caller then falls back to the direct fire-and-forget
  // path).
  void Start(CaptureFn capture, LogFn log);

  // Requests a refresh of `video_path` (UTF-8). Returns immediately; the
  // worker does the work. The newest request wins. The trigger decides the
  // debounce behaviour (a session lock / suspend / shutdown bypasses it).
  void Request(const std::string& video_path, LockscreenTrigger trigger);

  // Refreshes NOW, on the calling thread, and waits for the result
  // (bounded by `timeout_ms`). Used by the shutdown path: Windows gives the
  // app a short window before the session ends, and the frame has to be on
  // disk by then.
  void RefreshBlocking(const std::string& video_path, int timeout_ms);

  // Stops the worker. Idempotent; the join returns immediately (the worker
  // waits on a condition variable, not a sleep).
  void Stop();

 private:
  void WorkerMain();

  LogFn log_ = nullptr;
  CaptureFn capture_ = nullptr;
  std::mutex mutex_;
  std::condition_variable wake_;
  std::string pending_video_;
  LockscreenTrigger pending_trigger_ = LockscreenTrigger::kVideoChanged;
  bool pending_valid_ = false;
  std::atomic<bool> stop_{false};
  std::atomic<bool> running_{false};
  // The thread handle is kept opaque (void*) so the header stays
  // windows.h-free.
  void* thread_ = nullptr;
};

}  // namespace k6wp
