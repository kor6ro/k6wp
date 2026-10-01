#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <mutex>
#include <string>

#include "playlist.hpp"

namespace k6wp {

// Engine-side playlist.json reader + timed rotation. State mutation is
// main-loop thread only; the *_atomic fields are the get_state (IPC worker)
// snapshot, refreshed whenever the playlist changes.
class PlaylistController {
 public:
  using LogFn = void (*)(const char* fmt, ...);
  using PathFn = std::function<std::filesystem::path()>;
  using StringFn = std::function<std::string()>;
  using SetVideoFn = std::function<bool(const std::string& utf8_path)>;

  explicit PlaylistController(LogFn log = nullptr) : log_(log) {}

  void SetHooks(PathFn resolve_path, StringFn current_video,
                SetVideoFn set_video);

  // Reloads playlist.json when its mtime/size changed; disarms when it is gone.
  void Reload();

  // (Re)arms the rotation interval from the current playlist.
  void Arm();

  // Restarts the interval on a resume edge (paused -> unpaused).
  void OnSlotsPausedChange(bool slots_paused_now);

  // True when rotation is armed and the interval has elapsed.
  bool Due() const;

  // Fires one rotation via the set_video hook; returns true on success.
  bool Fire();

  bool enabled() const { return playlist_.enabled; }
  bool has_order() const { return !playlist_.order.empty(); }
  bool get_enabled() const {
    return enabled_atomic_.load(std::memory_order_acquire);
  }
  long long get_size() const {
    return size_atomic_.load(std::memory_order_acquire);
  }
  long long get_index() const {
    return index_atomic_.load(std::memory_order_acquire);
  }

 private:
  LogFn log_;
  PathFn resolve_path_;
  StringFn current_video_;
  SetVideoFn set_video_;
  PlaylistConfig playlist_;
  std::filesystem::file_time_type mtime_{};
  std::uintmax_t size_ = 0;
  bool mtime_valid_ = false;
  bool missing_logged_ = false;
  std::uint64_t rng_ = 0x9E3779B97F4A7C15ull;
  std::atomic<bool> rotate_armed_{false};
  mutable std::mutex mutex_;
  std::chrono::steady_clock::time_point next_at_{};
  bool was_slots_paused_ = false;
  std::atomic<bool> enabled_atomic_{false};
  std::atomic<long long> size_atomic_{0};
  std::atomic<long long> index_atomic_{-1};
};

}  // namespace k6wp
