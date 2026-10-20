#pragma once

#include <string>

#include "lockscreen.hpp"

namespace k6wp {

// Matches EngineApp::Log (void(const char* fmt, ...)); may be null.
using LockscreenLogFn = void (*)(const char* fmt, ...);

// Fires the lockscreen frame extract for a video change when sync is enabled
// (shared/lockscreen.cpp): no-op when disabled or the path is empty; the shared
// helper debounces to one spawn per 5 s and detaches the child, never throws.
void MaybeTriggerLockscreenSync(const std::string& video_utf8,
                                LockscreenLogFn log);

// Trigger-aware variant. The trigger decides three things (all pure, all
// unit-tested in shared/lockscreen.cpp):
//   * whether a refresh happens at all (ShouldRefreshLockscreen);
//   * whether the debounce is bypassed - a session lock / suspend / shutdown
//     must never be swallowed by an earlier refresh
//     (LockscreenTriggerBypassesDebounce);
//   * whether the extract is WAITED for - only the shutdown path, where the
//     frame has to land before the session ends
//     (LockscreenTriggerWaits).
void MaybeTriggerLockscreenSync(const std::string& video_utf8,
                                LockscreenTrigger trigger,
                                LockscreenLogFn log);

}  // namespace k6wp
