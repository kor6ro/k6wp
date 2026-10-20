#pragma once

// Exit codes of `K6WP.exe --elevate-lockscreen on|off`.
//
// Single source of truth: the launcher (launcher/exit_codes.hpp) and the
// Studio (shared/lockscreen_bridge.hpp) must agree on the numbers, because
// the Studio maps the launcher's exit code onto the lock-screen toggle.
//
// 4 and 5 exist so a declined UAC prompt is distinguishable from a broken
// policy write: the Studio tells the user WHICH happened instead of
// showing a generic failure.

namespace k6wp {

inline constexpr int kExitOk = 0;
inline constexpr int kExitBadFlag = 2;
inline constexpr int kExitSpawnFailure = 3;   // missing sibling / spawn failure
inline constexpr int kExitElevateTimeout = 4;  // elevated worker outlived its wait
inline constexpr int kExitElevateDeclined = 5;  // UAC prompt was dismissed

}  // namespace k6wp
