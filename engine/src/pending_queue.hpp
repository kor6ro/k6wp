#pragma once

#include <mutex>
#include <string>
#include <utility>

namespace k6wp {

// CRIT-2 worker -> main handoff: the IPC worker stashes one validated payload
// and posts a private message; the main loop pops it. One slot each is enough
// (the pipe serves one client at a time; last write wins and execution is
// idempotent). One mutex guards both directions.
class PendingCommandQueue {
 public:
  void SetVideo(std::string payload) {
    std::lock_guard<std::mutex> lock(mutex_);
    video_ = std::move(payload);
    has_video_ = true;
  }
  void ClearVideo() {
    std::lock_guard<std::mutex> lock(mutex_);
    video_.clear();
    has_video_ = false;
  }
  bool TakeVideo(std::string* out) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!has_video_) return false;
    if (out != nullptr) *out = std::move(video_);
    video_.clear();
    has_video_ = false;
    return true;
  }

  void SetMonitor(std::string payload) {
    std::lock_guard<std::mutex> lock(mutex_);
    monitor_ = std::move(payload);
    has_monitor_ = true;
  }
  void ClearMonitor() {
    std::lock_guard<std::mutex> lock(mutex_);
    monitor_.clear();
    has_monitor_ = false;
  }
  bool TakeMonitor(std::string* out) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!has_monitor_) return false;
    if (out != nullptr) *out = std::move(monitor_);
    monitor_.clear();
    has_monitor_ = false;
    return true;
  }

 private:
  std::mutex mutex_;
  std::string video_;
  bool has_video_ = false;
  std::string monitor_;
  bool has_monitor_ = false;
};

}  // namespace k6wp
