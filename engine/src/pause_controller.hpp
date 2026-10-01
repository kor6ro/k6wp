#pragma once

#include <atomic>

namespace k6wp {

// Pause ownership bitmask (Step 3.1). State only: the fan-out to the renderer /
// live slots / tray stays in EngineApp::ApplyPauseState, which the owner calls
// after SetBit reports a change. Thread-safe (atomic mask), so the IPC worker
// and the UI thread can both set/clear owner bits.
class PauseController {
 public:
  enum : int {
    kUser = 1,
    kFullscreen = 2,
    kSuspend = 4,
    kPower = 8,
    kSessionLock = 16,
    kScreenOff = 32,
  };

  // UI-visible pause: user, fullscreen, suspend, session-lock, or screen-off.
  bool UiPaused() const {
    return (mask_.load(std::memory_order_acquire) &
            (kUser | kFullscreen | kSuspend | kSessionLock | kScreenOff)) != 0;
  }

  // Slot pause: ANY owner (incl. the DC power cap and screen-off).
  bool SlotsPaused() const {
    return mask_.load(std::memory_order_acquire) != 0;
  }

  int mask() const { return mask_.load(std::memory_order_acquire); }

  // Sets/clears one owner bit; returns true when the merged mask changed.
  bool SetBit(int bit, bool on) {
    const int old = on ? mask_.fetch_or(bit, std::memory_order_acq_rel)
                       : mask_.fetch_and(~bit, std::memory_order_acq_rel);
    const int updated = on ? (old | bit) : (old & ~bit);
    return updated != old;
  }

 private:
  std::atomic<int> mask_{0};
};

// Human-readable name for a kUser..kScreenOff bit (logs). "unknown" otherwise.
inline const char* PauseOwnerName(int bit) {
  switch (bit) {
    case PauseController::kUser:
      return "user";
    case PauseController::kFullscreen:
      return "fullscreen";
    case PauseController::kSuspend:
      return "suspend";
    case PauseController::kPower:
      return "power";
    case PauseController::kSessionLock:
      return "session-lock";
    case PauseController::kScreenOff:
      return "screen-off";
    default:
      return "unknown";
  }
}

}  // namespace k6wp
