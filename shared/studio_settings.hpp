#pragma once

#include <filesystem>
#include <string>

#include "thirdparty/json.hpp"

namespace k6wp {

// Version of the on-disk studio-settings schema. v0 = legacy/absent
// (missing fields are defaulted on load); v1 = lockscreen sync fields;
// v2 = compressor advanced-box visibility; v3 = current (dead controls
// keep_original/start_minimized removed; old files carrying them migrate
// by ignoring those keys).
inline constexpr int kStudioSettingsSchemaVersion = 3;

// Studio-side preferences, stored at %LOCALAPPDATA%/K6WP/studio_settings.json.
// This is deliberately SEPARATE from WallpaperConfig (config.json), which is
// pure Engine playback state (video_path/fit/monitor/speed/fps/battery).
// Engine never reads this file; Studio never writes playback state here.
struct StudioSettings {
  int version = kStudioSettingsSchemaVersion;
  bool auto_compress_on_import = true;
  std::wstring compress_output_dir;  // default %LOCALAPPDATA%/K6WP/wallpapers
  int default_crf = 22;              // 16 - 28 (agrees with T7 CRF 22 default)
  int default_fps = 30;              // 1 - 30
  std::string default_resolution_mode =
      "match_monitor";  // match_monitor | source | 720p | 1080p | 2160p
  bool start_with_windows = false;
  std::wstring cache_dir;  // default %LOCALAPPDATA%/K6WP/cache
  // Bagian B — lock screen sync (static frame only, B5).
  // When true, engine/studio fire `compressor --lockframe` on every video
  // change (debounced, below-normal, fire-and-forget); the launcher applies
  // the resulting %PROGRAMDATA%\K6WP\lockscreen.jpg via
  // K6WP.exe --elevate-lockscreen on|off. Engine reads this flag read-only
  // (its only exception to the "Engine never reads this file" rule).
  bool lockscreen_sync = false;
  double lockscreen_offset_sec = 1.0;  // frame seek offset in seconds (>= 0)
  // Compressor tab: advanced-box expanded state (session UI only, never
  // affects jobs). v1 files migrate it to false.
  bool compress_advanced_visible = false;
  // Optional "new version available" check (Studio-only: never downloads,
  // never installs). When true, Studio GETs the releases API once per start
  // (plus manual Help -> "Check for updates"); network failures are silent.
  // Files missing this key (written before it existed) migrate it to true.
  bool check_updates = true;
};

// Defaults with environment-resolved directories (LOCALAPPDATA, USERPROFILE
// fallback). Used by MigrateStudioSettings for missing fields and by first
// run when no studio_settings.json exists yet.
StudioSettings DefaultStudioSettings();

// Loads, migrates and validates the studio settings at `path`.
// Throws ConfigError (shared/config_schema.hpp) on: unreadable file, corrupt
// JSON, unknown schema version, or values rejected by
// ValidateStudioSettings. Same contract as LoadConfig: an old file (v0, or v1
// missing newer fields) is backed up byte-for-byte to "<file>.bak" and
// rewritten at the current schema with defaults filled, so migration runs
// exactly once. Corrupt or unmigratable input is likewise backed up before
// the throw, letting the caller fall back to last-valid/safe defaults
// without losing data. Backup I/O is best-effort and never throws.
StudioSettings LoadStudioSettings(const std::filesystem::path& path);

// Serializes `settings` to `path` (creates parent directories). Throws
// ConfigError on write failure.
void SaveStudioSettings(const std::filesystem::path& path,
                        const StudioSettings& settings);

// Rejects (throws ConfigError) values outside the accepted ranges:
//   - default_crf not in [16, 28]
//   - default_fps not in [1, 30]
//   - empty compress_output_dir / cache_dir
//   - unknown default_resolution_mode
// Takes a non-const ref so future normalizing rules can rewrite in place.
void ValidateStudioSettings(StudioSettings& settings);

// Converts a raw JSON object into StudioSettings. Missing fields are filled
// with defaults (v0/v1/v2 -> v3 migration). Keys keep_original and
// start_minimized were removed in v3 and are ignored when present in old
// files. Throws ConfigError on unknown schema version or type-corrupt fields.
StudioSettings MigrateStudioSettings(const nlohmann::json& raw);

// %LOCALAPPDATA%/K6WP/studio_settings.json (falls back to
// %USERPROFILE%/AppData/Local).
std::filesystem::path DefaultStudioSettingsPath();

// %LOCALAPPDATA%/K6WP/wallpapers (USERPROFILE fallback, then temp dir).
// Mirrors compressor/src/main.cpp DefaultWallpapersDir.
std::filesystem::path DefaultCompressOutputDir();

// %LOCALAPPDATA%/K6WP/cache (USERPROFILE fallback, then temp dir).
// Mirrors compressor/src/cache_manager.cpp CacheRoot (without K6WP_*
// overrides: explicit studio settings always win over env here).
std::filesystem::path DefaultStudioCacheDir();

}  // namespace k6wp
