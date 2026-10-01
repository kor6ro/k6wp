#pragma once

#include <string>

namespace k6wp {

// Matches EngineApp::Log (void(const char* fmt, ...)); may be null.
using LockscreenLogFn = void (*)(const char* fmt, ...);

// Fires the lockscreen frame extract for a video change when sync is enabled
// (shared/lockscreen.cpp): no-op when disabled or the path is empty; the shared
// helper debounces to one spawn per 5 s and detaches the child, never throws.
void MaybeTriggerLockscreenSync(const std::string& video_utf8,
                                LockscreenLogFn log);

}  // namespace k6wp
