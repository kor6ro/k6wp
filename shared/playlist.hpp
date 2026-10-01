#pragma once

// Wallpaper playlist contract (engine-owned, Studio-edited).
//
// The playlist lives in its own file (%LOCALAPPDATA%/K6WP/playlist.json by
// default, sibling of config.json) rather than in config.json. Two reasons:
//   1. config.json stays "pure Engine playback state" (docs/dev-contracts.md
//      §2); the rotation schedule is neither playback state nor a Studio
//      preference, so it gets its own file.
//   2. An older Studio build rewrites config.json from its own
//      WallpaperConfig struct (ApplyManager::WriteConfig -> SaveConfig). A
//      playlist stored in config.json would be silently deleted by that
//      struct round-trip. A separate file is never touched by old builds.
//
// The engine READS this file (and watches it for changes); Studio writes it.
// Exactly one writer, one reader, published through the shared atomic
// .tmp + MoveFileExW path.
//
// windows.h-free: std types + nlohmann/json only (config_schema pattern).

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "thirdparty/json.hpp"

namespace k6wp {

// Version of the on-disk playlist schema. v0 = legacy/absent (missing fields
// are defaulted on load); v1 = current.
inline constexpr int kPlaylistSchemaVersion = 1;

// Rotation interval bounds, in minutes. 1 minute is the floor: a shorter
// interval would let rotations stack with the 6 s GPU-pin verify pass.
inline constexpr int kPlaylistMinIntervalMin = 1;
inline constexpr int kPlaylistMaxIntervalMin = 1440;  // 24 h

// Upper bound on playlist length. Paths are strings; 500 keeps the serialized
// file comfortably under kMaxConfigBytes (1 MiB) even with long paths.
inline constexpr std::size_t kPlaylistMaxEntries = 500;

// On-disk wallpaper playlist. All fields have sensible defaults so a missing
// or legacy file loads without crashing.
struct PlaylistConfig {
  int version = kPlaylistSchemaVersion;
  bool enabled = false;
  int interval_min = 30;  // [kPlaylistMinIntervalMin, kPlaylistMaxIntervalMin]
  bool shuffle = false;
  // Absolute video paths in play order. UTF-8 on disk (u8path round-trip),
  // wide in memory (Win32-friendly) exactly like WallpaperConfig::video_path.
  std::vector<std::wstring> order;
};

// --- Pure helpers (unit-tested, no I/O) -----------------------------------

// Returns the next rotation index for a playlist of `count` entries whose
// current position is `current`. Non-shuffle: (current + 1) % count, wrapping.
// Shuffle: a uniform pick in [0, count) that never equals `current` (so a
// shuffle step always moves), driven by `rng_state` (splitmix64; mutated in
// place so successive calls differ and are reproducible from a fixed seed).
// count == 0 -> 0. Out-of-range `current` is treated as "start of list".
std::size_t SelectNextIndex(std::size_t current, std::size_t count,
                            bool shuffle, std::uint64_t& rng_state);

// Index of `current` in `order`, compared case-insensitively on the wide
// string (Windows paths are case-insensitive). Returns -1 when absent.
int PlaylistIndexForPath(const std::vector<std::wstring>& order,
                         const std::wstring& current);

// Normalizes an order list in place: drops empty entries, de-duplicates
// case-insensitively (first occurrence wins), preserves order. Returns the
// normalized vector. Does not resolve/normalize path spelling.
std::vector<std::wstring> NormalizeOrder(std::vector<std::wstring> order);

// --- I/O (ConfigError from config_schema.hpp) -----------------------------

// Loads, migrates and validates the playlist at `path`.
// Throws ConfigError on: unreadable file, corrupt JSON, unknown schema
// version, oversize file (kMaxConfigBytes), or values rejected by
// ValidatePlaylist. Same .bak contract as LoadConfig: on migration or corrupt
// input the original bytes are copied to "<playlist>.bak" before the throw /
// self-healing rewrite, so migration runs exactly once and no data is lost.
PlaylistConfig LoadPlaylist(const std::filesystem::path& path);

// Serializes `cfg` to `path` (creates parent directories), validates a copy
// first, normalizes order, and publishes through the shared AtomicWriteJson
// (.tmp + MoveFileExW). Throws ConfigError on write failure.
void SavePlaylist(const std::filesystem::path& path, const PlaylistConfig& cfg);

// Struct -> on-disk JSON mapping (single definition site, shared by
// SavePlaylist and tests). `version` is always the current schema version.
nlohmann::json PlaylistToJson(const PlaylistConfig& cfg);

// Rejects (throws ConfigError) values outside the accepted ranges:
//   - interval_min not in [1, 1440]
//   - order longer than kPlaylistMaxEntries
// Takes a non-const ref so future normalizing rules can rewrite in place.
void ValidatePlaylist(PlaylistConfig& cfg);

// Converts a raw JSON object into a PlaylistConfig. Missing fields are
// defaulted; the order array is normalized. Throws ConfigError on unknown
// schema version or type-corrupt fields.
PlaylistConfig MigratePlaylist(const nlohmann::json& raw);

// %LOCALAPPDATA%/K6WP/playlist.json (USERPROFILE fallback). Throws
// ConfigError when both env vars are unset (mirrors DefaultConfigPath).
std::filesystem::path DefaultPlaylistPath();

// playlist.json next to an explicit config path, so an engine started with
// `--config <dir>/config.json` reads `<dir>/playlist.json`. A bare filename
// yields just "playlist.json" (empty parent), matching LoadConfig semantics.
std::filesystem::path PlaylistPathForConfig(const std::filesystem::path& config_path);

}  // namespace k6wp
