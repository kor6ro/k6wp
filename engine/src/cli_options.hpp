#pragma once

// Engine command-line surface, split out of engine_app.hpp so the resident
// app does not also own pure argv parsing. Contract unchanged: ParseCli keeps
// its (argc, char**) entry and return codes; see docs/dev-test-flags.md for
// the hidden test flags.

#include <string>

namespace k6wp {

// Desktop injection strategy. Parsed from --wallpaper-mode.
enum class WallpaperMode { kAuto, kWorkerW, kProgman };

// Parsed command-line options. Canonical silent spelling is --minimized;
// --engine/--silent are compat aliases (old autostart entries keep working).
// Test flags below are hidden from --help but fully parsed.
struct CliOptions {
  std::wstring video_path;   // --video <path> (mpv source)
  std::wstring config_path;  // --config <path> (config_watch)
  WallpaperMode wallpaper_mode = WallpaperMode::kAuto;  // --wallpaper-mode
  bool minimized = false;  // --minimized (tray-only autostart start)
  int exit_after_ms = 0;  // --exit-after-ms <N> (hidden test flag; 0 = forever)
  int simulate_device_lost_after_ms = 0;  // hidden test flag; 0 = never
  int simulate_suspend_after_ms = 0;  // hidden test flag; 0 = never (+2s resume)
  int simulate_dc_after_ms = 0;  // hidden test flag; 0 = never (+2s AC restore)
  int simulate_monitor_off_after_ms = 0;  // hidden; 0 = never (+2s monitor on)
};

const char* WallpaperModeToString(WallpaperMode mode);

// Parses argv into `out`. Returns 0 on success, 1 if --help was shown
// (caller exits 0), 2 on malformed arguments (caller exits 2).
int ParseCli(int argc, char** argv, CliOptions& out);

}  // namespace k6wp
