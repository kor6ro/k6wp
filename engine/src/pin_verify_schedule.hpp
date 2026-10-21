#pragma once

#include <atomic>
#include <chrono>
#include <mutex>

namespace k6wp {

// P3L.3 verify scheduling (PATCH A): a one-shot steady-clock deadline armed
// after every (re)load, plus the lifetime revert counter. fired once by the
// Run loop. Thread-safe: the queued set_video executor arms it on the main
// thread while the loop polls it, and get_state reads the counter on the IPC
// worker thread.
// The pure core of the verify schedule (task G5): the delay after every
// (re)load and the two decisions, with the clock as a PARAMETER.
inline constexpr long long kPinVerifyDelayMs = 6000;

// True when the verify pass is due: it is armed AND the deadline elapsed.
// (An unarmed schedule is never due, whatever the clock says.)
inline bool PinVerifyDueAt(bool armed, long long deadline_ms, long long now_ms) {
  return armed && now_ms >= deadline_ms;
}

// The lifetime revert counter: the new total after n reverts.
inline int PinVerifyAddReverts(int total, int n) { return total + n; }

class PinVerifySchedule {
 public:
  // Arms the +6 s deadline when a pin is configured. No-op without a pin.
  void Arm(bool has_pin);

  // True when armed and the deadline has elapsed.
  bool Due() const;

  void Disarm() { armed_.store(false, std::memory_order_release); }

  // Adds to the lifetime revert counter; returns the new total.
  int AddReverts(int n);

  int total() const;

 private:
  std::atomic<bool> armed_{false};
  mutable std::mutex mutex_;
  std::chrono::steady_clock::time_point at_{};
  int total_ = 0;
};

}  // namespace k6wp
