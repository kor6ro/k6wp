#include "lockscreen_glue.hpp"

#include "lockscreen.hpp"

#include <filesystem>

namespace k6wp {

void MaybeTriggerLockscreenSync(const std::string& video_utf8,
                                LockscreenLogFn log) {
  MaybeTriggerLockscreenSync(video_utf8, LockscreenTrigger::kVideoChanged,
                              log);
}

void MaybeTriggerLockscreenSync(const std::string& video_utf8,
                                LockscreenTrigger trigger,
                                LockscreenLogFn log) {
  if (video_utf8.empty()) return;
  if (!ShouldRefreshLockscreen(trigger)) return;
  // Canonical helper (shared/lockscreen.cpp): no-op unless lockscreen_sync is
  // ON, debounced to one spawn per 5s (a session lock / suspend / shutdown
  // bypasses the debounce), child detached BELOW_NORMAL, never throws.
  // Log-only here so sync can never break the render path.
  FireLockscreenSyncForTrigger(std::filesystem::u8path(video_utf8), trigger);
  if (log) {
    log("lockscreen: refresh requested for %s (trigger, best-effort)",
        video_utf8.c_str());
  }
}

}  // namespace k6wp
