#pragma once

#include <cstdint>
#include <filesystem>
#include <stdexcept>
#include <string>

#include "thirdparty/json.hpp"

namespace k6wp {

// MED-18: upper bound on config/settings/library file reads. Every ReadFile
// checks std::filesystem::file_size first and throws ConfigError when the
// file is larger, so a hostile multi-gigabyte file can never be slurped into
// memory. 1 MiB is generous: real files are a few KiB.
inline constexpr std::uintmax_t kMaxConfigBytes = 1024u * 1024u;

// Version of the on-disk config schema. v0 = legacy (missing fields are
// defaulted on load); v1 = monitor_id without render effect; v2 = monitor
// targeting (-1 = all screens); v3 = legacy "fill" normalized to "cover";
// v4 = battery_mode ("cap24" | "static", default "cap24");
// v5 = current (cpu_affinity "auto" | "all" + gpu_adapter "auto" |
// "integrated" | "discrete", both default "auto"; Phase 3-lite, shared-decode
// descoped so no render_mode key).
inline constexpr int kConfigSchemaVersion = 5;

// Structured error thrown on corrupt/invalid config. Carries a human-readable
// message; consumers (Engine config_watch, Studio settings) catch this and
// fall back to the last-valid config / safe defaults.
class ConfigError : public std::runtime_error {
 public:
  explicit ConfigError(const std::string& message) : std::runtime_error(message) {}
};

// On-disk wallpaper configuration. All fields have sensible defaults so a
// v0/legacy config (or a config missing fields) migrates without crashing.
struct WallpaperConfig {
  int version = kConfigSchemaVersion;
  std::wstring video_path;  // UTF-8 on disk, wide in memory (Win32-friendly)
  std::string fit_mode = "cover";  // cover | fill (legacy alias) | fit | stretch | center
  double speed = 1.0;             // 0.5 - 2.0 (RESERVED: not applied by the renderer)
  int monitor_id = -1;            // -1 = all screens; >=0 = that monitor only
  int crf = 22;                   // 16 - 28
  int resolution_w = 0;           // 0 = monitor native
  int resolution_h = 0;           // 0 = monitor native
  int fps_cap = 24;               // 1 - 30
  bool battery_saver = false;
  std::string battery_mode = "cap24";  // cap24 | static (DC policy, master-gated by battery_saver)
  std::string cpu_affinity = "auto";   // auto | all (P3L: E-core pin on hybrid when auto)
  std::string gpu_adapter = "auto";  // auto | integrated | discrete (P3L: iGPU pin on hybrid when auto)
};

// Loads, migrates and validates the config at `path`.
// Throws ConfigError on: unreadable file, corrupt JSON, unknown schema
// version, or values rejected by ValidateConfig.
// Migration (Todo 37): an old file (v0, or v1 missing newer fields) is
// backed up byte-for-byte to "<config>.bak" and rewritten at the current
// schema with defaults filled, so migration runs exactly once. Corrupt or
// unmigratable input is likewise backed up to "<config>.bak" before the
// throw, letting the caller fall back to last-valid/safe defaults without
// losing data. Backup I/O is best-effort and never throws.
WallpaperConfig LoadConfig(const std::filesystem::path& path);

// Struct -> on-disk JSON mapping: the single definition site for the config
// field set, shared by SaveConfig and PersistConfigField so both write
// identical bytes. Exposed so tests can pin the field set against the
// config_schema.json doc.
nlohmann::json ConfigToJson(const WallpaperConfig& cfg);

// Serializes `cfg` to `path` (creates parent directories). Throws ConfigError
// on write failure.
// HIGH-4 atomicity: the payload is written to "<config>.tmp", flushed and
// closed, then published with MoveFileExW(MOVEFILE_REPLACE_EXISTING |
// MOVEFILE_WRITE_THROUGH). A crash/force-kill before the rename leaves the
// old file intact; a leftover "<config>.tmp" is ignored by LoadConfig and
// overwritten on the next save — it never shadows the main file.
void SaveConfig(const std::filesystem::path& path, const WallpaperConfig& cfg);

// HIGH-4 crash-safe publish, shared by config.json and studio_settings.json
// (and any future settings file): the payload goes to "<path>.tmp" (flushed
// + closed, stream state checked), then MoveFileExW(MOVEFILE_REPLACE_EXISTING
// | MOVEFILE_WRITE_THROUGH) atomically replaces the main file. A crash or
// force-kill before the rename leaves the old file intact; a leftover .tmp
// is never shadowed and is truncated on the next save. Throws ConfigError on
// write/rename failure.
void AtomicWriteJson(const std::filesystem::path& path, const nlohmann::json& j);

// Atomically persists ONE top-level field of the config file (HIGH-4,
// Engine persist path). Unlike SaveConfig (full struct rewrite), the update
// is a JSON-level merge: the current file is read as raw JSON, only `field`
// is replaced with `value`, and the result is published through the same
// atomic .tmp + MoveFileExW path. Unknown/extra keys (e.g. a unique dummy
// field, or keys from a newer schema) survive the round-trip — a struct
// round-trip would silently drop them.
//
// Load problems (missing / corrupt / invalid file): the existing bytes are
// backed up to "<config>.bak" when present (same forensic contract as
// LoadConfig), then the write starts from `fallback` (default: schema
// defaults) with `field` applied. The merged document is validated
// (MigrateConfig + ValidateConfig) before anything is written, so a bad
// mutation throws ConfigError and leaves the file untouched.
// Returns true when a write happened, false when `field` already equaled
// `value` (no mtime bump, so the config watcher stays quiet).
bool PersistConfigField(const std::filesystem::path& path,
                        const std::string& field, const nlohmann::json& value,
                        const WallpaperConfig& fallback = WallpaperConfig{});

// Rejects (throws ConfigError) values outside the accepted ranges:
//   - crf not in [16, 28]
//   - speed not in [0.5, 2.0]
//   - fps_cap > 30 or < 1
//   - negative resolution
// Takes a non-const ref so future normalizing rules can rewrite in place.
void ValidateConfig(WallpaperConfig& cfg);

// Converts a raw JSON object into a WallpaperConfig. Missing fields are filled
// with defaults (v0/v1 -> v2 migration forces monitor_id = -1: pre-upgrade
// the id had no render effect, so preserving it would change real behavior;
// v<3 with fit_mode "fill" normalizes to "cover").
// Throws ConfigError on unknown schema version or type-corrupt fields.
WallpaperConfig MigrateConfig(const nlohmann::json& raw);

// %LOCALAPPDATA%/K6WP/config.json (falls back to %USERPROFILE%/AppData/Local).
std::filesystem::path DefaultConfigPath();

}  // namespace k6wp