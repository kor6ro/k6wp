#include "pin_verify_schedule.hpp"

namespace k6wp {
namespace {

// The steady clock in milliseconds (the epoch is arbitrary - only the
// differences matter).
long long SteadyMs() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::steady_clock::now().time_since_epoch())
      .count();
}

}  // namespace

void PinVerifySchedule::Arm(bool has_pin) {
  if (!has_pin) return;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    at_ = std::chrono::steady_clock::now() +
          std::chrono::milliseconds(kPinVerifyDelayMs);
  }
  armed_.store(true, std::memory_order_release);
}

bool PinVerifySchedule::Due() const {
  if (!armed_.load(std::memory_order_acquire)) return false;
  std::lock_guard<std::mutex> lock(mutex_);
  const long long deadline_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(at_.time_since_epoch())
          .count();
  return PinVerifyDueAt(true, deadline_ms, SteadyMs());
}

int PinVerifySchedule::AddReverts(int n) {
  std::lock_guard<std::mutex> lock(mutex_);
  total_ = PinVerifyAddReverts(total_, n);
  return total_;
}

int PinVerifySchedule::total() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return total_;
}

}  // namespace k6wp
