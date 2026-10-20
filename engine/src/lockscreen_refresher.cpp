#include "lockscreen_refresher.hpp"

#include "lockscreen.hpp"

#include <chrono>

namespace k6wp {

namespace {

// How long the worker waits for the live frame's file to land. The file is
// written by mpv's VO thread right after the capture command, so this is
// generous; the fallback (the file-based extract) takes over when it
// expires.
constexpr int kLiveFrameWaitMs = 2500;

}  // namespace

LockscreenRefresher::~LockscreenRefresher() { Stop(); }

void LockscreenRefresher::Start(CaptureFn capture, LogFn log) {
  if (running_.load(std::memory_order_acquire)) return;
  capture_ = capture;
  log_ = log;
  stop_.store(false, std::memory_order_release);
  try {
    std::thread worker([this] { WorkerMain(); });
    thread_ = new std::thread(std::move(worker));
    running_.store(true, std::memory_order_release);
  } catch (...) {
    if (log_) {
      log_("lockscreen: the refresher thread could not be started - the "
           "file-based extract is used instead");
    }
  }
}

void LockscreenRefresher::Request(const std::string& video_path,
                                  LockscreenTrigger trigger) {
  if (video_path.empty()) return;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    pending_video_ = video_path;
    pending_trigger_ = trigger;
    pending_valid_ = true;
  }
  wake_.notify_one();
}

void LockscreenRefresher::RefreshBlocking(const std::string& video_path,
                                          int timeout_ms) {
  if (video_path.empty()) return;
  // The caller blocks: the system gives the app a short window before the
  // session ends, and the frame has to be on disk by then.
  const LockscreenFrameSource source = ProduceLockscreenFrame(
      std::filesystem::u8path(video_path), LockscreenTrigger::kShutdown,
      timeout_ms);
  if (log_) {
    log_("lockscreen: the frame was refreshed (source=%s)",
         source == LockscreenFrameSource::kLiveFrame
             ? "live-frame"
             : (source == LockscreenFrameSource::kFileExtract ? "file-extract"
                                                              : "none"));
  }
}

void LockscreenRefresher::WorkerMain() {
  for (;;) {
    std::string video;
    LockscreenTrigger trigger = LockscreenTrigger::kVideoChanged;
    {
      std::unique_lock<std::mutex> lock(mutex_);
      wake_.wait(lock, [this] {
        return stop_.load(std::memory_order_acquire) || pending_valid_;
      });
      if (stop_.load(std::memory_order_acquire)) return;
      video = pending_video_;
      trigger = pending_trigger_;
      pending_valid_ = false;
    }
    const LockscreenFrameSource source = ProduceLockscreenFrame(
        std::filesystem::u8path(video), trigger, kLiveFrameWaitMs);
    if (log_) {
      log_("lockscreen: the frame was refreshed (source=%s)",
           source == LockscreenFrameSource::kLiveFrame
               ? "live-frame"
               : (source == LockscreenFrameSource::kFileExtract
                      ? "file-extract"
                      : "none"));
    }
  }
}

void LockscreenRefresher::Stop() {
  if (!running_.load(std::memory_order_acquire)) return;
  stop_.store(true, std::memory_order_release);
  {
    std::lock_guard<std::mutex> lock(mutex_);
    wake_.notify_all();
  }
  auto* worker = static_cast<std::thread*>(thread_);
  thread_ = nullptr;
  running_.store(false, std::memory_order_release);
  if (worker == nullptr) return;
  worker->join();
  delete worker;
}

}  // namespace k6wp
