#include "displays_schema.hpp"

#include "config_schema.hpp"  // ConfigError, AtomicWriteJson, kMaxConfigBytes
#include "settings_io.hpp"

#include <algorithm>
#include <fstream>
#include <set>
#include <system_error>
#include <tuple>

namespace k6wp {
namespace {

bool NeedsMigration(const nlohmann::json& raw) {
  if (!raw.is_object()) {
    return true;
  }
  if (raw.value("version", 0) != kDisplaysSchemaVersion) {
    return true;
  }
  if (!raw.contains("assignments") || !raw.contains("displays")) {
    return true;
  }
  return false;
}

// True when any path component of `p` is exactly "..". ".." inside a
// filename component ("foo..bar") is fine; only a traversal component is
// rejected, so a hostile assignment cannot escape the wallpapers root.
bool HasDotDotComponent(const std::wstring& p) {
  const std::filesystem::path fp(p);
  for (const auto& part : fp) {
    if (part == L"..") {
      return true;
    }
  }
  return false;
}

}  // namespace

nlohmann::json DisplaysToJson(const DisplaysConfig& cfg) {
  nlohmann::json assignments = nlohmann::json::object();
  for (const auto& [key, a] : cfg.assignments) {
    nlohmann::json value;
    value["path"] = std::filesystem::path(a.path).u8string();
    value["exists"] = a.exists;
    assignments[std::filesystem::path(key).u8string()] = std::move(value);
  }
  nlohmann::json j;
  j["version"] = cfg.version;
  j["assignments"] = std::move(assignments);
  j["displays"] = cfg.displays;
  return j;
}

DisplaysConfig LoadDisplays(const std::filesystem::path& path) {
  const std::string text = detail::ReadFile(path, "displays file");
  nlohmann::json raw;
  try {
    raw = nlohmann::json::parse(text);
  } catch (const nlohmann::json::exception& e) {
    // QA-fail path: keep the corrupt bytes for forensics, then report a
    // structured error. The caller falls back to last-valid/defaults.
    detail::BackupFile(path);
    throw ConfigError(std::string("corrupt displays JSON: ") + e.what());
  }
  DisplaysConfig cfg;
  try {
    cfg = MigrateDisplays(raw);
  } catch (const ConfigError&) {
    detail::BackupFile(path);
    throw;
  }
  const bool migrated = NeedsMigration(raw);
  if (migrated) {
    detail::BackupFile(path);
  }
  try {
    ValidateDisplays(cfg);
  } catch (const ConfigError&) {
    detail::BackupFile(path);
    throw;
  }
  if (migrated) {
    // Self-healing persist so migration runs exactly once. Best-effort: a
    // read-only disk must not turn a clean load into an error.
    try {
      SaveDisplays(path, cfg);
    } catch (const ConfigError&) {
    }
  }
  return cfg;
}

void SaveDisplays(const std::filesystem::path& path,
                  const DisplaysConfig& cfg) {
  DisplaysConfig copy = cfg;
  copy.version = kDisplaysSchemaVersion;
  ValidateDisplays(copy);
  AtomicWriteJson(path, DisplaysToJson(copy));
}

void ValidateDisplays(DisplaysConfig& cfg) {
  if (cfg.version < 0 || cfg.version > kDisplaysSchemaVersion) {
    throw ConfigError("displays schema version out of range [0," +
                      std::to_string(kDisplaysSchemaVersion) + "]: " +
                      std::to_string(cfg.version));
  }
  if (cfg.assignments.size() > kDisplaysMaxEntries) {
    throw ConfigError("displays exceeds maximum entries (" +
                      std::to_string(kDisplaysMaxEntries) + ")");
  }
  for (const auto& [key, a] : cfg.assignments) {
    if (a.path.empty()) {
      throw ConfigError("displays assignment path must be non-empty (key " +
                        std::filesystem::path(key).u8string() + ")");
    }
    if (HasDotDotComponent(a.path)) {
      throw ConfigError("displays assignment path must not contain '..': " +
                        std::filesystem::path(a.path).u8string());
    }
  }
}

DisplaysConfig MigrateDisplays(const nlohmann::json& raw) {
  if (!raw.is_object()) {
    throw ConfigError("displays root must be a JSON object");
  }
  DisplaysConfig cfg;
  try {
    const int version = raw.value("version", 0);
    if (version < 0 || version > kDisplaysSchemaVersion) {
      throw ConfigError("unsupported displays schema version: " +
                        std::to_string(version));
    }
    cfg.version = kDisplaysSchemaVersion;
    if (raw.contains("assignments")) {
      const auto& assigns = raw.at("assignments");
      if (!assigns.is_object()) {
        throw ConfigError("displays assignments must be an object");
      }
      for (auto it = assigns.begin(); it != assigns.end(); ++it) {
        if (!it.value().is_object()) {
          throw ConfigError("displays assignment value must be an object");
        }
        if (!it.value().contains("path") ||
            !it.value().at("path").is_string()) {
          throw ConfigError("displays assignment path must be a string");
        }
        MonitorAssignment a;
        a.path = std::filesystem::u8path(it.value().at("path").get<std::string>())
                     .wstring();
        a.exists = it.value().value("exists", false);
        cfg.assignments[std::filesystem::u8path(it.key()).wstring()] =
            std::move(a);
      }
    }
    if (raw.contains("displays")) {
      if (!raw.at("displays").is_array()) {
        throw ConfigError("displays must be an array");
      }
      cfg.displays = raw.at("displays");
    }
  } catch (const ConfigError&) {
    throw;
  } catch (const nlohmann::json::exception& e) {
    throw ConfigError(std::string("displays field type error: ") + e.what());
  }
  return cfg;
}

std::filesystem::path DefaultDisplaysPath() {
  return DefaultConfigPath().parent_path() / L"displays.json";
}

std::vector<std::wstring> DetectKeyCollision(
    const DisplaysConfig& cfg, const std::vector<MonitorInfo>& monitors) {
  std::vector<std::wstring> out;
  if (cfg.assignments.empty() || monitors.empty()) {
    return out;
  }

  // Rect (x, y, width, height) -> assignment keys resolving there.
  std::map<std::tuple<int, int, int, int>, std::set<std::wstring>> by_rect;
  // Device names that appear more than once in the live monitor list
  // (clone/duplicate mode: duplicated szDevice).
  std::map<std::wstring, int> device_hits;

  for (const auto& m : monitors) {
    ++device_hits[m.device_name];
  }

  for (const auto& [key, assign] : cfg.assignments) {
    (void)assign;
    for (const auto& m : monitors) {
      if (m.device_name == key) {
        by_rect[std::make_tuple(m.x, m.y, m.width, m.height)].insert(key);
        break;  // first matching monitor resolves the key
      }
    }
    if (device_hits[key] > 1) {
      out.push_back(key);  // duplicated szDevice: one key, two HMONITORs
    }
  }

  for (const auto& [rect, keys] : by_rect) {
    (void)rect;
    if (keys.size() > 1) {
      for (const auto& k : keys) {
        out.push_back(k);  // two keys share one physical rect
      }
    }
  }

  std::sort(out.begin(), out.end());
  out.erase(std::unique(out.begin(), out.end()), out.end());
  return out;
}

}  // namespace k6wp
