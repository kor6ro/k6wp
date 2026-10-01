#include "pin_verify_schedule.hpp"

namespace k6wp {

void PinVerifySchedule::Arm(bool has_pin) {
  if (!has_pin) return;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    at_ = std::chrono::steady_clock::now() + std::chrono::seconds(6);
  }
  armed_.store(true, std::memory_order_release);
}

bool PinVerifySchedule::Due() const {
  if (!armed_.load(std::memory_order_acquire)) return false;
  std::lock_guard<std::mutex> lock(mutex_);
  return std::chrono::steady_clock::now() >= at_;
}

int PinVerifySchedule::AddReverts(int n) {
  std::lock_guard<std::mutex> lock(mutex_);
  total_ += n;
  return total_;
}

int PinVerifySchedule::total() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return total_;
}

}  // namespace k6wp
