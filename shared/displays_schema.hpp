#pragma once

// Per-monitor assignment store (displays.json, engine-read, Studio-written).
//
// The store lives in its own file (%LOCALAPPDATA%/K6WP/displays.json by
// default, sibling of config.json) rather than in config.json. Same two
// reasons as the playlist (CHANGELOG.md, shared/playlist.hpp):
//   1. config.json stays "pure Engine playback state" (docs/dev-contracts.md
//      §2); per-monitor assignment is neither playback state nor a Studio
//      preference, so it gets its own file.
//   2. An older Studio build rewrites config.json from its own
//      WallpaperConfig struct (ApplyManager::WriteConfig -> SaveConfig). An
//      assignment map stored in config.json would be silently deleted by that
//      struct round-trip. A separate file is never touched by old builds.
//
// On-disk shape (see displays_schema.json for the doc companion):
//   {
//     "version": 1,
//     "assignments": {
//       "\\\\.\\DISPLAY1": { "path": "C:\\Videos\\a.mp4", "exists": true },
//       "\\\\.\\DISPLAY2": { "path": "C:\\Videos\\b.webm", "exists": false }
//     },
//     "displays": [ /* display-only metadata; the engine ignores it */ ]
//   }
// `assignments` is keyed by the GDI device name (MONITORINFOEXW.szDevice,
// e.g. \\.\DISPLAY1). `displays` is a top-level array Studio may write for
// display-only metadata; the engine never reads it, and it survives the
// struct round-trip byte-for-byte as a raw JSON array.
//
// The engine READS this file (and watches it for changes); Studio writes it.
// Published through the shared atomic .tmp + MoveFileExW path.
//
// windows.h-free: std types + nlohmann/json only (config_schema pattern).

#include <cstddef>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

#include "monitor_util.hpp"
#include "thirdparty/json.hpp"

namespace k6wp {

// Version of the on-disk displays schema. v0 = legacy/absent (missing fields
// are defaulted on load); v1 = current.
inline constexpr int kDisplaysSchemaVersion = 1;

// Upper bound on assignment entries. Paths are strings; 100 keeps the
// serialized file comfortably under kMaxConfigBytes (1 MiB) even with long
// paths (mirrors playlist.hpp kPlaylistMaxEntries rationale).
inline constexpr std::size_t kDisplaysMaxEntries = 100;

// One per-monitor assignment: which video file plays on the monitor whose
// GDI device name is the map key.
struct MonitorAssignment {
  std::wstring path;  // UTF-8 on disk, wide in memory (Win32-friendly)
  bool exists = false;  // Studio-side existence hint at write time; the
                        // engine re-checks the filesystem at apply time.
};

// On-disk per-monitor assignment store. All fields have sensible defaults so
// a missing or legacy file loads without crashing.
struct DisplaysConfig {
  int version = kDisplaysSchemaVersion;
  // Keyed by the GDI device name (e.g. L"\\\\.\\DISPLAY1"), UTF-16 native,
  // the same string MonitorInfo::device_name carries. std::map (not
  // unordered_map) keeps serialization order deterministic across runs.
  std::map<std::wstring, MonitorAssignment> assignments;
  // Top-level display-only metadata Studio may write (orientation notes,
  // friendly names, ...). The engine IGNORES this array; it is preserved as
  // raw JSON so a Studio reader/writer never loses it.
  nlohmann::json displays = nlohmann::json::array();
};

// --- I/O (ConfigError from config_schema.hpp) -----------------------------

// Loads, migrates and validates the displays store at `path`.
// Throws ConfigError on: unreadable file, corrupt JSON, unknown schema
// version, oversize file (kMaxConfigBytes), or values rejected by
// ValidateDisplays. Same .bak contract as LoadConfig: on migration or corrupt
// input the original bytes are copied to "<displays>.bak" before the throw /
// self-healing rewrite, so migration runs exactly once and no data is lost.
DisplaysConfig LoadDisplays(const std::filesystem::path& path);

// Serializes `cfg` to `path` (creates parent directories), validates a copy
// first, and publishes through the shared AtomicWriteJson (.tmp +
// MoveFileExW). Throws ConfigError on write failure.
void SaveDisplays(const std::filesystem::path& path, const DisplaysConfig& cfg);

// Struct -> on-disk JSON mapping (single definition site, shared by
// SaveDisplays and tests). `version` is always the current schema version.
// Top-level keys are exactly: version, assignments, displays.
nlohmann::json DisplaysToJson(const DisplaysConfig& cfg);

// Rejects (throws ConfigError) values outside the accepted ranges:
//   - version not in [0, kDisplaysSchemaVersion]
//   - assignments longer than kDisplaysMaxEntries
//   - any assignment path empty, or containing a ".." path component
// Takes a non-const ref so future normalizing rules can rewrite in place.
void ValidateDisplays(DisplaysConfig& cfg);

// Converts a raw JSON object into a DisplaysConfig. Missing fields are
// defaulted. Throws ConfigError on unknown schema version or type-corrupt
// fields (non-object assignments, non-object assignment values, missing or
// non-string path, non-array displays).
DisplaysConfig MigrateDisplays(const nlohmann::json& raw);

// %LOCALAPPDATA%/K6WP/displays.json (USERPROFILE fallback). Throws
// ConfigError when both env vars are unset (mirrors DefaultConfigPath).
std::filesystem::path DefaultDisplaysPath();

// IS-7 collision detection. Returns the assignment keys whose device names
// resolve to monitors sharing the same physical rect, plus any key whose
// device name is duplicated in the live monitor list (clone/duplicate mode:
// GetMonitorInfoW reports the same szDevice for two HMONITORs). Catching
// this at load stops two videos from silently stacking on one rect.
// Keys that match no monitor are ignored. Deterministic order (map order).
std::vector<std::wstring> DetectKeyCollision(
    const DisplaysConfig& cfg, const std::vector<MonitorInfo>& monitors);

}  // namespace k6wp
