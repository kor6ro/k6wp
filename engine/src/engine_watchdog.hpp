#pragma once

// Hang watchdog (C1).
//
// WHY
// The engine had NO recovery from a HANG. crash_restart_guard only covers
// CRASHES (Windows Error Reporting relaunches the process); a hang - the
// message loop stuck, the tray icon still on screen but nothing responding
// - stayed broken until the user killed the process by hand.
//
// HOW
// The message loop bumps a heartbeat counter on every iteration and marks
// itself "parked" while it waits INFINITE (the paused engine waits on
// purpose - that must NOT look like a hang). A separate thread watches the
// counter: when it stops advancing while the loop is NOT parked, the engine
// is hung.
//
// ON A HANG
//   1. a minidump is written (so the hang can be analysed like a crash);
//   2. the launcher is spawned DETACHED with `--engine --silent --restarted
//      --delay-ms N` - the delay makes the replacement engine start after
//      the hung one released the singleton mutex, and `--restarted` makes
//      the relaunch count against the engine's crash-restart backoff (no
//      restart loop);
//   3. the hung process is terminated.
//
// COST
// The watcher is a separate thread with a 5 s sleep between checks: it adds
// NO wakeups to the message loop and no periodic work to the paused engine
// (the loop stays parked, the watcher just reads two atomics).
//
// windows.h-free (the header exposes std types only).

#include <atomic>
#include <condition_variable>
#include <mutex>

namespace k6wp {

// The pure decision (header-only, so the test suite can drive it without
// linking the watcher - and therefore without dbghelp).
//
// `last_seen` is the heartbeat value the watcher saw last time, `current`
// the value now. The loop is hung when the heartbeat did not advance, the
// loop is NOT parked on purpose, and the stall lasted at least
// `hang_timeout_ms`.
inline bool IsEngineHung(unsigned long long last_seen,
                          unsigned long long current, bool parked,
                          bool enabled, long long now_ms,
                          long long last_change_ms, int hang_timeout_ms) {
  if (!enabled) return false;
  // The loop waits INFINITE on purpose while the engine is paused: that is
  // the zero-wakeup design, not a hang.
  if (parked) return false;
  if (current != last_seen) return false;
  if (hang_timeout_ms <= 0) return false;
  return (now_ms - last_change_ms) >= hang_timeout_ms;
}

class EngineWatchdog {
 public:
  // Matches EngineApp::Log (void(const char* fmt, ...)); may be null.
  using LogFn = void (*)(const char* fmt, ...);

  explicit EngineWatchdog(LogFn log = nullptr) : log_(log) {}
  ~EngineWatchdog();

  EngineWatchdog(const EngineWatchdog&) = delete;
  EngineWatchdog& operator=(const EngineWatchdog&) = delete;

  // Starts the watcher thread.
  //
  // `heartbeat` is bumped by the message loop on every iteration;
  // `parked` is true while the loop waits INFINITE on purpose (the paused
  // engine). Both must outlive the watchdog (the engine owns them).
  //
  // `hang_timeout_ms` is how long the heartbeat may stall before the engine
  // is declared hung. `check_interval_ms` is how often the watcher looks.
  // Never throws; a thread that cannot be started leaves the watchdog
  // inactive (the engine then runs exactly as before).
  void Start(std::atomic<unsigned long long>* heartbeat,
             std::atomic<bool>* parked, int hang_timeout_ms = 15000,
             int check_interval_ms = 5000);

  // Stops the watcher thread. Idempotent; joins with a bounded wait (a
  // watcher stuck in the minidump write is detached rather than waited on
  // forever - the engine's exit must not hang).
  void Stop();

  // The pure decision: given the last observed heartbeat value, the last
  // time it changed and the current time, is the loop hung? `parked` (the
  // loop waits on purpose) and `enabled` (the watchdog runs) short-circuit
  // to "not hung". Unit-tested (see IsEngineHung).
  static bool IsHung(unsigned long long last_seen,
                     unsigned long long current, bool parked, bool enabled,
                     long long now_ms, long long last_change_ms,
                     int hang_timeout_ms) {
    return IsEngineHung(last_seen, current, parked, enabled, now_ms,
                        last_change_ms, hang_timeout_ms);
  }

 private:
  LogFn log_ = nullptr;
  std::atomic<bool> stop_{false};
  std::atomic<bool> running_{false};
  // Lets Stop() wake the watcher immediately instead of waiting out the
  // check interval (the engine's exit must not linger).
  std::mutex wake_mutex_;
  std::condition_variable wake_cv_;
  // The thread handle is kept opaque (void*) so the header stays
  // windows.h-free.
  void* thread_ = nullptr;
};

}  // namespace k6wp
