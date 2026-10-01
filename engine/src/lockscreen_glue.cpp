#include "lockscreen_glue.hpp"

#include "lockscreen.hpp"

#include <filesystem>

namespace k6wp {

void MaybeTriggerLockscreenSync(const std::string& video_utf8,
                                LockscreenLogFn log) {
  if (video_utf8.empty()) return;
  // Canonical helper (shared/lockscreen.cpp): no-op unless lockscreen_sync is
  // ON, debounced to one spawn per 5s, child detached BELOW_NORMAL, never
  // throws. Log-only here so sync can never break the render path.
  if (!IsLockscreenSyncEnabled()) return;
  FireLockscreenSyncAsync(std::filesystem::u8path(video_utf8));
  if (log) {
    log("lockscreen: refresh requested for %s (best-effort, debounced)",
        video_utf8.c_str());
  }
}

}  // namespace k6wp
