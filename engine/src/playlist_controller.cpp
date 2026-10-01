#include "playlist_controller.hpp"

#include "config_schema.hpp"  // ConfigError

#include <utility>

namespace k6wp {

void PlaylistController::SetHooks(PathFn resolve_path, StringFn current_video,
                                  SetVideoFn set_video) {
  resolve_path_ = std::move(resolve_path);
  current_video_ = std::move(current_video);
  set_video_ = std::move(set_video);
}

void PlaylistController::Reload() {
  std::filesystem::path path;
  try {
    path = resolve_path_();
  } catch (const ConfigError& e) {
    if (!missing_logged_) {
      if (log_) {
        log_("playlist: cannot resolve path (%s), rotation disabled", e.what());
      }
      missing_logged_ = true;
    }
    return;
  }
  std::error_code ec;
  const auto mtime = std::filesystem::last_write_time(path, ec);
  if (ec) {
    // No file: treat as "no playlist" and disarm. A later create re-arms.
    if (mtime_valid_) {
      mtime_valid_ = false;
      playlist_ = PlaylistConfig{};
      enabled_atomic_.store(false, std::memory_order_release);
      size_atomic_.store(0, std::memory_order_release);
      index_atomic_.store(-1, std::memory_order_release);
      rotate_armed_.store(false, std::memory_order_release);
      if (log_) log_("playlist: file removed, rotation disabled");
    } else if (!missing_logged_) {
      if (log_) {
        log_("playlist: no file at %s, rotation disabled", path.string().c_str());
      }
      missing_logged_ = true;
    }
    return;
  }
  const auto size = std::filesystem::file_size(path, ec);
  if (ec) return;
  if (mtime_valid_ && mtime == mtime_ && size == size_) {
    return;
  }
  mtime_ = mtime;
  size_ = size;
  mtime_valid_ = true;
  missing_logged_ = false;
  try {
    playlist_ = LoadPlaylist(path);
  } catch (const ConfigError& e) {
    if (log_) log_("playlist: reload failed (keeping last-valid): %s", e.what());
    return;
  }
  const std::string current = current_video_();
  const int idx = PlaylistIndexForPath(
      playlist_.order, std::filesystem::u8path(current).wstring());
  enabled_atomic_.store(playlist_.enabled, std::memory_order_release);
  size_atomic_.store(static_cast<long long>(playlist_.order.size()),
                     std::memory_order_release);
  index_atomic_.store(idx, std::memory_order_release);
  if (log_) {
    log_("playlist: loaded %llu entries (enabled=%d interval_min=%d shuffle=%d)",
         static_cast<unsigned long long>(playlist_.order.size()),
         playlist_.enabled ? 1 : 0, playlist_.interval_min,
         playlist_.shuffle ? 1 : 0);
  }
  Arm();
}

void PlaylistController::Arm() {
  if (!playlist_.enabled || playlist_.order.size() < 2) {
    rotate_armed_.store(false, std::memory_order_release);
    return;
  }
  const auto deadline = std::chrono::steady_clock::now() +
                        std::chrono::minutes(playlist_.interval_min);
  {
    std::lock_guard<std::mutex> lock(mutex_);
    next_at_ = deadline;
  }
  rotate_armed_.store(true, std::memory_order_release);
}

void PlaylistController::OnSlotsPausedChange(bool slots_paused_now) {
  if (was_slots_paused_ && !slots_paused_now) {
    Arm();
  }
  was_slots_paused_ = slots_paused_now;
}

bool PlaylistController::Due() const {
  if (!rotate_armed_.load(std::memory_order_acquire)) return false;
  std::lock_guard<std::mutex> lock(mutex_);
  return std::chrono::steady_clock::now() >= next_at_;
}

bool PlaylistController::Fire() {
  const std::size_t n = playlist_.order.size();
  if (!playlist_.enabled || n < 2) {
    rotate_armed_.store(false, std::memory_order_release);
    return false;
  }
  const std::string current = current_video_();
  // Unknown current (e.g. a manual apply not in the playlist) starts the walk
  // from the end so the first SelectNextIndex lands on entry 0.
  int idx = PlaylistIndexForPath(playlist_.order,
                                 std::filesystem::u8path(current).wstring());
  if (idx < 0) idx = static_cast<int>(n) - 1;
  for (std::size_t attempt = 0; attempt < n; ++attempt) {
    const std::size_t next =
        SelectNextIndex(static_cast<std::size_t>(idx), n, playlist_.shuffle,
                        rng_);
    idx = static_cast<int>(next);
    const std::wstring& cand = playlist_.order[next];
    std::error_code ec;
    if (!std::filesystem::is_regular_file(cand, ec)) {
      if (log_) {
        log_("playlist: skipping missing entry [%llu] %s",
             static_cast<unsigned long long>(next),
             std::filesystem::path(cand).u8string().c_str());
      }
      continue;
    }
    const std::string utf8 = std::filesystem::path(cand).u8string();
    if (set_video_(utf8)) {
      index_atomic_.store(static_cast<long long>(next),
                          std::memory_order_release);
      if (log_) {
        log_("playlist: rotated to [%llu/%llu] %s",
             static_cast<unsigned long long>(next),
             static_cast<unsigned long long>(n), utf8.c_str());
      }
      Arm();
      return true;
    }
  }
  if (log_) log_("playlist: no playable entry found, keeping current video");
  Arm();  // back off to the next interval instead of spinning
  return false;
}

}  // namespace k6wp
