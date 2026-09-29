#include "config_schema.hpp"

#include <windows.h>

#include <fstream>
#include <system_error>

namespace k6wp {

namespace {

// Reads the whole file into a string. RAII via std::ifstream; throws
// ConfigError when the file cannot be opened. MED-18: rejects files larger
// than kMaxConfigBytes BEFORE reading (file_size with error_code, so a
// missing/unstatable file still falls through to the cannot-open error).
std::string ReadFile(const std::filesystem::path& path) {
  std::error_code ec;
  const std::uintmax_t size = std::filesystem::file_size(path, ec);
  if (!ec && size > kMaxConfigBytes) {
    throw ConfigError("config file exceeds maximum size (" +
                      std::to_string(kMaxConfigBytes) + " bytes): " +
                      path.string());
  }
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    throw ConfigError("cannot open config file: " + path.string());
  }
  return std::string(std::istreambuf_iterator<char>(in),
                     std::istreambuf_iterator<char>());
}

// Backup destination for `path`: the same filename with ".bak" appended
// (e.g. config.json -> config.json.bak). operator+= appends to the filename
// component, so wide/non-UTF8 paths survive (no narrow string() round-trip).
std::filesystem::path BackupPath(const std::filesystem::path& path) {
  std::filesystem::path bak = path;
  bak += L".bak";
  return bak;
}

// Copies the original config bytes to "<config>.bak". Best-effort: uses the
// error_code overloads and never throws, so a failed backup can never turn
// a clean load into an error.
void BackupConfigFile(const std::filesystem::path& path) noexcept {
  std::error_code ec;
  std::filesystem::copy_file(path, BackupPath(path),
                             std::filesystem::copy_options::overwrite_existing, ec);
}

// True when `raw` predates the current schema: explicit older version, or any
// known field absent (e.g. a v1 file written before battery_saver /
// monitor_id existed). Called after MigrateConfig succeeded, so `version` is
// already known-good and value() cannot throw here.
bool NeedsMigration(const nlohmann::json& raw) {
  if (!raw.is_object()) {
    return true;
  }
  if (raw.value("version", 0) != kConfigSchemaVersion) {
    return true;
  }
  static const char* const kFields[] = {
      "video_path", "fit_mode",   "speed",        "monitor_id", "crf",
      "resolution_w", "resolution_h", "fps_cap",  "battery_saver",
      "battery_mode", "cpu_affinity", "gpu_adapter",
  };
  for (const char* field : kFields) {
    if (!raw.contains(field)) {
      return true;
    }
  }
  return false;
}

}  // namespace

// HIGH-4: crash-safe publish. The payload goes to "<path>.tmp" (flushed
// + closed, stream state checked), then MoveFileExW atomically replaces the
// main file. A force-kill before the rename leaves the old file intact; a
// leftover .tmp is ignored by LoadConfig and truncated on the next save.
// Declared in the header (k6wp scope) so studio_settings.json saves share
// the same crash-safety contract.
void AtomicWriteJson(const std::filesystem::path& path,
                     const nlohmann::json& j) {
  std::error_code ec;
  if (!path.parent_path().empty()) {
    std::filesystem::create_directories(path.parent_path(), ec);
  }
  std::filesystem::path tmp = path;
  tmp += L".tmp";
  {
    std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
    if (!out) {
      throw ConfigError("cannot write json temp file: " + tmp.string());
    }
    out << j.dump(2);
    out.flush();
    if (!out) {
      throw ConfigError("failed writing json temp file: " + tmp.string());
    }
    out.close();
    if (!out) {
      throw ConfigError("failed flushing json temp file: " + tmp.string());
    }
  }
  if (MoveFileExW(tmp.wstring().c_str(), path.wstring().c_str(),
                  MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) == 0) {
    const DWORD err = GetLastError();
    std::error_code ec2;
    std::filesystem::remove(tmp, ec2);
    throw ConfigError("atomic json replace failed (err=" +
                      std::to_string(err) + "): " + path.string());
  }
}

// Struct -> on-disk JSON mapping. Single definition site shared by SaveConfig
// and the PersistConfigField fallback path so both write identical bytes.
// Declared in the header (k6wp scope) so tests can pin the field set against
// config_schema.json.
nlohmann::json ConfigToJson(const WallpaperConfig& cfg) {
  nlohmann::json j;
  j["version"] = cfg.version;
  j["video_path"] = std::filesystem::path(cfg.video_path).u8string();
  j["fit_mode"] = cfg.fit_mode;
  j["speed"] = cfg.speed;
  j["monitor_id"] = cfg.monitor_id;
  j["crf"] = cfg.crf;
  j["resolution_w"] = cfg.resolution_w;
  j["resolution_h"] = cfg.resolution_h;
  j["fps_cap"] = cfg.fps_cap;
  j["battery_saver"] = cfg.battery_saver;
  j["battery_mode"] = cfg.battery_mode;
  j["cpu_affinity"] = cfg.cpu_affinity;
  j["gpu_adapter"] = cfg.gpu_adapter;
  return j;
}

WallpaperConfig LoadConfig(const std::filesystem::path& path) {
  const std::string text = ReadFile(path);
  nlohmann::json raw;
  try {
    raw = nlohmann::json::parse(text);
  } catch (const nlohmann::json::exception& e) {
    // QA-fail path: keep the corrupt bytes for forensics, then report a
    // structured error. The caller falls back to last-valid/safe defaults.
    BackupConfigFile(path);
    throw ConfigError(std::string("corrupt config JSON: ") + e.what());
  }
  WallpaperConfig cfg;
  try {
    cfg = MigrateConfig(raw);
  } catch (const ConfigError&) {
    // Unknown schema version or type-corrupt field: back up before throwing
    // so the caller can fall back to a safe default without losing data.
    BackupConfigFile(path);
    throw;
  }
  const bool migrated = NeedsMigration(raw);
  if (migrated) {
    // Old config (v0, or v1 missing newer fields): preserve the original
    // bytes before the self-healing rewrite below overwrites them.
    BackupConfigFile(path);
  }
  try {
    ValidateConfig(cfg);
  } catch (const ConfigError&) {
    // Invalid field value (e.g. unknown fit_mode): preserve the original
    // bytes before the caller falls back to last-valid/safe defaults.
    BackupConfigFile(path);
    throw;
  }
  if (migrated) {
    // Self-healing persist: the next load (and the ConfigWatcher poll) sees
    // a current-schema file, so migration runs exactly once. Best-effort: a
    // read-only disk must not turn a clean load into an error.
    try {
      SaveConfig(path, cfg);
    } catch (const ConfigError&) {
      // Load succeeded; persistence is advisory. The caller still gets the
      // migrated config, and the .bak backup is already on disk.
    }
  }
  return cfg;
}

void SaveConfig(const std::filesystem::path& path, const WallpaperConfig& cfg) {
  // Never persist an invalid config: validate a copy first.
  WallpaperConfig copy = cfg;
  ValidateConfig(copy);
  AtomicWriteJson(path, ConfigToJson(copy));
}

bool PersistConfigField(const std::filesystem::path& path,
                        const std::string& field, const nlohmann::json& value,
                        const WallpaperConfig& fallback) {
  nlohmann::json raw;
  bool have_valid = false;
  try {
    raw = nlohmann::json::parse(ReadFile(path));
    if (!raw.is_object()) {
      throw ConfigError("config root must be a JSON object");
    }
    WallpaperConfig probe = MigrateConfig(raw);
    ValidateConfig(probe);
    have_valid = true;
  } catch (const ConfigError&) {
    // Missing / corrupt / invalid: preserve the existing bytes for forensics
    // (no-op when the file is simply absent), then rebuild from fallback.
    BackupConfigFile(path);
    raw = ConfigToJson(fallback);
  }
  if (have_valid && raw.contains(field) && raw.at(field) == value) {
    return false;
  }
  raw[field] = value;
  // Validate the merged document BEFORE touching disk: a bad mutation throws
  // and the on-disk file (or the .bak-backed fallback) stays untouched.
  WallpaperConfig merged = MigrateConfig(raw);
  ValidateConfig(merged);
  AtomicWriteJson(path, raw);
  return true;
}

void ValidateConfig(WallpaperConfig& cfg) {
  if (cfg.crf < 16 || cfg.crf > 28) {
    throw ConfigError("crf out of range [16,28]: " + std::to_string(cfg.crf));
  }
  if (cfg.speed < 0.5 || cfg.speed > 2.0) {
    throw ConfigError("speed out of range [0.5,2.0]: " + std::to_string(cfg.speed));
  }
  if (cfg.fps_cap < 1 || cfg.fps_cap > 30) {
    throw ConfigError("fps_cap out of range [1,30]: " + std::to_string(cfg.fps_cap));
  }
  if (cfg.resolution_w < 0 || cfg.resolution_h < 0) {
    throw ConfigError("resolution must be >= 0");
  }
  if (cfg.monitor_id < -1) {
    throw ConfigError("monitor_id must be >= -1 (-1 = all screens): " +
                      std::to_string(cfg.monitor_id));
  }
  if (cfg.fit_mode != "cover" && cfg.fit_mode != "fill" && cfg.fit_mode != "fit" &&
      cfg.fit_mode != "stretch" && cfg.fit_mode != "center") {
    throw ConfigError("unknown fit_mode: " + cfg.fit_mode);
  }
  if (cfg.battery_mode != "cap24" && cfg.battery_mode != "static") {
    throw ConfigError("unknown battery_mode: " + cfg.battery_mode);
  }
  if (cfg.cpu_affinity != "auto" && cfg.cpu_affinity != "all") {
    throw ConfigError("unknown cpu_affinity: " + cfg.cpu_affinity);
  }
  if (cfg.gpu_adapter != "auto" && cfg.gpu_adapter != "integrated" &&
      cfg.gpu_adapter != "discrete") {
    throw ConfigError("unknown gpu_adapter: " + cfg.gpu_adapter);
  }
}

WallpaperConfig MigrateConfig(const nlohmann::json& raw) {
  if (!raw.is_object()) {
    throw ConfigError("config root must be a JSON object");
  }
  WallpaperConfig cfg;
  try {
    const int version = raw.value("version", 0);
    if (version < 0 || version > kConfigSchemaVersion) {
      throw ConfigError("unsupported config schema version: " + std::to_string(version));
    }
    // Step 5: v0/v1 -> v2 always writes monitor_id = -1 (the old id had no
    // render effect, so keeping it would newly single-out one screen).
    // v2 files keep their stored value (default -1 when absent).
    // Always persist as the current version.
    cfg.version = kConfigSchemaVersion;
    cfg.video_path = std::filesystem::u8path(raw.value("video_path", "")).wstring();
    std::string fit_mode = raw.value("fit_mode", "cover");
    if (version < 3 && fit_mode == "fill") {
      fit_mode = "cover";
    }
    cfg.fit_mode = fit_mode;
    cfg.speed = raw.value("speed", 1.0);
    cfg.monitor_id = (version < 2) ? -1 : raw.value("monitor_id", -1);
    cfg.crf = raw.value("crf", 22);
    cfg.resolution_w = raw.value("resolution_w", 0);
    cfg.resolution_h = raw.value("resolution_h", 0);
    cfg.fps_cap = raw.value("fps_cap", 24);
    cfg.battery_saver = raw.value("battery_saver", false);
    // Step P3L.1: v4 and older predate cpu_affinity/gpu_adapter; default
    // to "auto" (E-core pin + iGPU pin on hybrid). Present values are
    // preserved; unknown strings pass through here and are rejected by
    // ValidateConfig (ConfigError + .bak + keep-last-valid via LoadConfig).
    cfg.battery_mode = raw.value("battery_mode", "cap24");
    cfg.cpu_affinity = raw.value("cpu_affinity", "auto");
    cfg.gpu_adapter = raw.value("gpu_adapter", "auto");
  } catch (const ConfigError&) {
    throw;
  } catch (const nlohmann::json::exception& e) {
    throw ConfigError(std::string("config field type error: ") + e.what());
  }
  return cfg;
}

std::filesystem::path DefaultConfigPath() {
  wchar_t buf[MAX_PATH];
  const DWORD local = GetEnvironmentVariableW(L"LOCALAPPDATA", buf, MAX_PATH);
  if (local > 0 && local < MAX_PATH) {
    return std::filesystem::path(buf) / L"K6WP" / L"config.json";
  }
  const DWORD profile = GetEnvironmentVariableW(L"USERPROFILE", buf, MAX_PATH);
  if (profile > 0 && profile < MAX_PATH) {
    return std::filesystem::path(buf) / L"AppData" / L"Local" / L"K6WP" / L"config.json";
  }
  throw ConfigError("LOCALAPPDATA and USERPROFILE are both unset");
}

}  // namespace k6wp