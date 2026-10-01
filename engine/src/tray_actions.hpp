#pragma once

namespace k6wp {

// Matches EngineApp::Log (void(const char* fmt, ...)); may be null.
using TrayLogFn = void (*)(const char* fmt, ...);

// Launches or focuses K6WP Studio via the launcher singleton (K6WP.exe
// --studio); falls back to focusing an existing "K6WP Studio" window or
// spawning studio.exe directly.
void OpenStudio(TrayLogFn log);

// Opens the donation URL (shared/links.hpp K6WP_DONATE_URL) in the browser.
void OpenSupport(TrayLogFn log);

}  // namespace k6wp
