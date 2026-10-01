#include "studio_settings.hpp"

#include "config_schema.hpp"
#include "settings_io.hpp"

#include <windows.h>

#include <fstream>
#include <system_error>

namespace k6wp {

namespace {

bool NeedsSettingsMigration(const nlohmann::json& raw) {
  if (!raw.is_object()) {
    return true;
  }
  if (raw.value("version", 0) != kStudioSettingsSchemaVersion) {
    return true;
  }
  static const char* const kFields[] = {
      "auto_compress_on_import", "compress_output_dir", "default_crf",
      "default_fps",             "default_resolution_mode",
      "start_with_windows",      "cache_dir",
      "lockscreen_sync",         "lockscreen_offset_sec",
      "compress_advanced_visible", "check_updates",
  };
  for (const char* field : kFields) {
    if (!raw.contains(field)) {
      return true;
    }
  }
  return false;
}

std::filesystem::path K6wpLocalDir(const wchar_t* leaf) {
  wchar_t buf[MAX_PATH];
  const DWORD local = GetEnvironmentVariableW(L"LOCALAPPDATA", buf, MAX_PATH);
  if (local > 0 && local < MAX_PATH) {
    return std::filesystem::path(buf) / L"K6WP" / leaf;
  }
  const DWORD profile = GetEnvironmentVariableW(L"USERPROFILE", buf, MAX_PATH);
  if (profile > 0 && profile < MAX_PATH) {
    return std::filesystem::path(buf) / L"AppData" / L"Local" / L"K6WP" / leaf;
  }
  std::error_code ec;
  return std::filesystem::temp_directory_path(ec) / L"K6WP" / leaf;
}

}  // namespace

std::filesystem::path DefaultCompressOutputDir() {
  return K6wpLocalDir(L"wallpapers");
}

std::filesystem::path DefaultStudioCacheDir() {
  return K6wpLocalDir(L"cache");
}

StudioSettings DefaultStudioSettings() {
  StudioSettings s;
  s.version = kStudioSettingsSchemaVersion;
  s.auto_compress_on_import = true;
  s.compress_output_dir = DefaultCompressOutputDir().wstring();
  s.default_crf = 22;
  s.default_fps = 30;
  s.default_resolution_mode = "match_monitor";
  s.start_with_windows = false;
  s.lockscreen_sync = false;
  s.lockscreen_offset_sec = 1.0;
  s.compress_advanced_visible = false;
  s.check_updates = true;
  s.cache_dir = DefaultStudioCacheDir().wstring();
  return s;
}

StudioSettings LoadStudioSettings(const std::filesystem::path& path) {
  const std::string text = detail::ReadFile(path, "studio settings file");
  nlohmann::json raw;
  try {
    raw = nlohmann::json::parse(text);
  } catch (const nlohmann::json::exception& e) {
    // QA-fail path: keep the corrupt bytes for forensics, then report a
    // structured error. The caller falls back to last-valid/safe defaults.
    detail::BackupFile(path);
    throw ConfigError(std::string("corrupt studio settings JSON: ") + e.what());
  }
  StudioSettings settings;
  try {
    settings = MigrateStudioSettings(raw);
  } catch (const ConfigError&) {
    // Unknown schema version or type-corrupt field: back up before throwing
    // so the caller can fall back without losing data.
    detail::BackupFile(path);
    throw;
  }
  const bool migrated = NeedsSettingsMigration(raw);
  if (migrated) {
    // Old settings (v0, or v1 missing newer fields): preserve the original
    // bytes before the self-healing rewrite below overwrites them.
    detail::BackupFile(path);
  }
  try {
    ValidateStudioSettings(settings);
  } catch (const ConfigError&) {
    // Invalid value (e.g. unknown resolution mode): preserve the original
    // bytes before the caller falls back to last-valid/safe defaults.
    detail::BackupFile(path);
    throw;
  }
  if (migrated) {
    // Self-healing persist: the next load sees a current-schema file, so
    // migration runs exactly once. Best-effort: a read-only disk must not
    // turn a clean load into an error.
    try {
      SaveStudioSettings(path, settings);
    } catch (const ConfigError&) {
      // Load succeeded; persistence is advisory. The .bak backup is already
      // on disk.
    }
  }
  return settings;
}

void SaveStudioSettings(const std::filesystem::path& path,
                        const StudioSettings& settings) {
  // Never persist invalid settings: validate a copy first.
  StudioSettings copy = settings;
  ValidateStudioSettings(copy);

  nlohmann::json j;
  j["version"] = copy.version;
  j["auto_compress_on_import"] = copy.auto_compress_on_import;
  j["compress_output_dir"] =
      std::filesystem::path(copy.compress_output_dir).u8string();
  j["default_crf"] = copy.default_crf;
  j["default_fps"] = copy.default_fps;
  j["default_resolution_mode"] = copy.default_resolution_mode;
  j["start_with_windows"] = copy.start_with_windows;
  j["lockscreen_sync"] = copy.lockscreen_sync;
  j["lockscreen_offset_sec"] = copy.lockscreen_offset_sec;
  j["compress_advanced_visible"] = copy.compress_advanced_visible;
  j["check_updates"] = copy.check_updates;
  j["cache_dir"] = std::filesystem::path(copy.cache_dir).u8string();

  // HIGH-4 crash-safety parity with config.json: publish through the shared
  // atomic .tmp + MoveFileExW path (SaveStudioSettings used to write directly
  // with ofstream(trunc) — a crash mid-save could truncate/half-write the
  // file the loader then has to salvage from .bak).
  AtomicWriteJson(path, j);
}

void ValidateStudioSettings(StudioSettings& settings) {
  if (settings.default_crf < 16 || settings.default_crf > 28) {
    throw ConfigError("default_crf out of range [16,28]: " +
                      std::to_string(settings.default_crf));
  }
  if (settings.default_fps < 1 || settings.default_fps > 30) {
    throw ConfigError("default_fps out of range [1,30]: " +
                      std::to_string(settings.default_fps));
  }
  if (settings.compress_output_dir.empty()) {
    throw ConfigError("compress_output_dir must not be empty");
  }
  if (settings.cache_dir.empty()) {
    throw ConfigError("cache_dir must not be empty");
  }
  if (settings.lockscreen_offset_sec < 0.0) {
    throw ConfigError("lockscreen_offset_sec must be >= 0");
  }
  const std::string& mode = settings.default_resolution_mode;
  if (mode != "match_monitor" && mode != "source" && mode != "720p" &&
      mode != "1080p" && mode != "2160p") {
    throw ConfigError("unknown default_resolution_mode: " + mode);
  }
}

StudioSettings MigrateStudioSettings(const nlohmann::json& raw) {
  if (!raw.is_object()) {
    throw ConfigError("studio settings root must be a JSON object");
  }
  const StudioSettings defaults = DefaultStudioSettings();
  StudioSettings s;
  try {
    const int version = raw.value("version", 0);
    if (version < 0 || version > kStudioSettingsSchemaVersion) {
      throw ConfigError("unsupported studio settings schema version: " +
                        std::to_string(version));
    }
    // v0/v1/v2 -> v3: missing fields are filled with defaults below.
    // Always persist as the current version.
    s.version = kStudioSettingsSchemaVersion;
    s.auto_compress_on_import =
        raw.value("auto_compress_on_import", defaults.auto_compress_on_import);
    if (raw.contains("compress_output_dir")) {
      s.compress_output_dir = std::filesystem::u8path(
                                  raw.value("compress_output_dir", ""))
                                  .wstring();
    } else {
      s.compress_output_dir = defaults.compress_output_dir;
    }
    s.default_crf = raw.value("default_crf", defaults.default_crf);
    s.default_fps = raw.value("default_fps", defaults.default_fps);
    s.default_resolution_mode = raw.value("default_resolution_mode",
                                          defaults.default_resolution_mode);
    // v3 removed keep_original/start_minimized: old files may still carry
    // them; they are deliberately not read (ignored on migration).
    s.start_with_windows =
        raw.value("start_with_windows", defaults.start_with_windows);
    s.lockscreen_sync = raw.value("lockscreen_sync", defaults.lockscreen_sync);
    s.lockscreen_offset_sec =
        raw.value("lockscreen_offset_sec", defaults.lockscreen_offset_sec);
    s.compress_advanced_visible = raw.value("compress_advanced_visible",
                                            defaults.compress_advanced_visible);
    s.check_updates = raw.value("check_updates", defaults.check_updates);
    if (raw.contains("cache_dir")) {
      s.cache_dir =
          std::filesystem::u8path(raw.value("cache_dir", "")).wstring();
    } else {
      s.cache_dir = defaults.cache_dir;
    }
    // Empty-string dirs mean "unset" (e.g. hand-edited file): resolve to the
    // environment default instead of persisting an unusable empty path.
    if (s.compress_output_dir.empty()) {
      s.compress_output_dir = defaults.compress_output_dir;
    }
    if (s.cache_dir.empty()) {
      s.cache_dir = defaults.cache_dir;
    }
  } catch (const ConfigError&) {
    throw;
  } catch (const nlohmann::json::exception& e) {
    throw ConfigError(std::string("studio settings field type error: ") +
                      e.what());
  }
  return s;
}

std::filesystem::path DefaultStudioSettingsPath() {
  wchar_t buf[MAX_PATH];
  const DWORD local = GetEnvironmentVariableW(L"LOCALAPPDATA", buf, MAX_PATH);
  if (local > 0 && local < MAX_PATH) {
    return std::filesystem::path(buf) / L"K6WP" / L"studio_settings.json";
  }
  const DWORD profile = GetEnvironmentVariableW(L"USERPROFILE", buf, MAX_PATH);
  if (profile > 0 && profile < MAX_PATH) {
    return std::filesystem::path(buf) / L"AppData" / L"Local" / L"K6WP" /
           L"studio_settings.json";
  }
  throw ConfigError("LOCALAPPDATA and USERPROFILE are both unset");
}

}  // namespace k6wp
