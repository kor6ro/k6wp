#pragma once

#include <filesystem>
#include <string>

#include "thirdparty/json.hpp"

namespace k6wp {

// Version of the on-disk studio-settings schema. v0 = legacy/absent
// (missing fields are defaulted on load); v1 = lockscreen sync fields;
// v2 = compressor advanced-box visibility; v3 = the plan-todo-8 UI
// preferences (closeToTray / playlistSource / performancePreset, ADDITIVE
// inside v3: files written before they existed migrate by defaulting them -
// no version bump, same pattern check_updates followed when it landed).
// v4 = the lock-screen onboarding offer (lockscreen_offer_shown).
//
// v5 = the PER-USER still mechanism (shared/still.hpp):
//   * new `wallpaper_sync` - apply the first frame as the desktop wallpaper;
//   * `lockscreen_sync` keeps its name and its stored value but now means
//     "apply the per-user lock-screen image" (no administrator policy);
//   * `lockscreen_offset_sec` default moves 1.0 -> 0.0 (the FIRST frame).
// A file written by v <= 4 keeps both stored flags exactly as the user left
// them; only a value of exactly 1.0 for the offset migrates to 0.0, because
// 1.0 was the old hard-coded default and 0.0 is the new one. A fresh install
// (no file) defaults BOTH sync flags to true.
inline constexpr int kStudioSettingsSchemaVersion = 5;

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
  bool start_with_windows = false;  // RESERVED: autostart is read from HKCU Run, not this field
  std::wstring cache_dir;  // default %LOCALAPPDATA%/K6WP/cache
  // Per-user still (shared/still.hpp): the FIRST frame of the video becomes
  // the real Windows desktop wallpaper (SPI_SETDESKWALLPAPER) and/or the
  // lock-screen image (WinRT LockScreen::SetImageFileAsync). No HKLM policy,
  // no administrator rights, no UAC. The Engine reads these fields read-only
  // (LOW-16: exactly three fields - wallpaper_sync, lockscreen_sync and
  // lockscreen_offset_sec).
  //
  // Defaults are TRUE for a fresh install; a file that already carries these
  // keys keeps the user's stored value.
  bool wallpaper_sync = true;
  bool lockscreen_sync = true;
  // Frame seek offset in seconds (>= 0). Default 0.0 = the FIRST frame. Files
  // written at schema <= 4 with the old default of exactly 1.0 migrate to
  // 0.0 (v5 migration).
  double lockscreen_offset_sec = 0.0;
  // v4 (Studio-private): the one-time onboarding offer for the lock screen
  // has been shown. The offer lives in the first-run wizard (both toggles,
  // default checked); this flag records that it was shown so it is never
  // shown again. The ENGINE never reads it (LOW-16).
  bool lockscreen_offer_shown = false;
  // Compressor tab: advanced-box expanded state (session UI only, never
  // affects jobs). v1 files migrate it to false.
  bool compress_advanced_visible = false;
  // Optional "new version available" check (Studio-only: never downloads,
  // never installs). When true, Studio GETs the releases API once per start
  // (plus manual Help -> "Check for updates"); network failures are silent.
  // Files missing this key (written before it existed) migrate it to true.
  bool check_updates = true;
  // Plan todo 8 (GAP-7) — Studio UI preferences, ADDITIVE inside schema v3
  // (same no-bump pattern as check_updates above). Old files without these
  // keys load to the defaults and the loader self-heals the file. JSON keys
  // are camelCase to match the QML bindings the follow-up todos expose; C++
  // fields stay snake_case like every other field in this struct.
  bool close_to_tray = false;  // JSON "closeToTray": exit to tray, not quit
  // JSON "playlistSource": rotation source, "all" | "custom".
  std::string playlist_source = "all";
  // JSON "performancePreset": "Hemat" | "Seimbang" | "Maksimal".
  std::string performance_preset = "Seimbang";
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
// Normalizes (rewrites in place, never throws) the plan-todo-8 enum fields
// to their safe defaults when the value is outside the accepted set:
//   - playlist_source not in {all, custom} -> "all"
//   - performance_preset not in {Hemat, Seimbang, Maksimal} -> "Seimbang"
// Takes a non-const ref so future normalizing rules can rewrite in place.
void ValidateStudioSettings(StudioSettings& settings);

// Converts a raw JSON object into StudioSettings. Missing fields are filled
// with defaults (v0..v4 -> v5 migration; the newer keys default when
// absent). The v5 offset rule: a pre-v5 value of exactly 1.0 migrates to 0.0
// (the old hard-coded default -> the first frame). Keys keep_original and
// start_minimized were removed in v3 and are ignored when present in old
// files. Out-of-set playlistSource / performancePreset values normalize to
// their defaults (old-file tolerance: a bad hand-edited enum must not fail
// the load). Throws ConfigError on unknown schema version or type-corrupt
// fields.
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
