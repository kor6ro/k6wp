#include "playlist.hpp"

#include "config_schema.hpp"  // ConfigError, AtomicWriteJson, kMaxConfigBytes
#include "settings_io.hpp"

#include <fstream>
#include <system_error>
#include <unordered_set>

namespace k6wp {
namespace {

bool NeedsMigration(const nlohmann::json& raw) {
  if (!raw.is_object()) {
    return true;
  }
  if (raw.value("version", 0) != kPlaylistSchemaVersion) {
    return true;
  }
  static const char* const kFields[] = {"enabled", "interval_min", "shuffle",
                                        "order"};
  for (const char* field : kFields) {
    if (!raw.contains(field)) {
      return true;
    }
  }
  return false;
}

// Lowercases ASCII letters in a wide string for case-insensitive compare.
// Windows path comparisons are case-insensitive; CharLowerW-style folding for
// the ASCII range is sufficient for de-dup/index (full Unicode folding is
// out of scope and the engine only ever compares paths it wrote itself).
std::wstring FoldAscii(const std::wstring& in) {
  std::wstring out = in;
  for (wchar_t& c : out) {
    if (c >= L'A' && c <= L'Z') {
      c = static_cast<wchar_t>(c - L'A' + L'a');
    }
  }
  return out;
}

}  // namespace

std::size_t SelectNextIndex(std::size_t current, std::size_t count,
                            bool shuffle, std::uint64_t& rng_state) {
  if (count == 0) {
    return 0;
  }
  if (!shuffle) {
    const std::size_t base = current < count ? current : count - 1;
    return (base + 1) % count;
  }
  if (count == 1) {
    return 0;
  }
  // splitmix64 step: advance the caller's state so successive calls differ.
  rng_state += 0x9E3779B97F4A7C15ull;
  std::uint64_t z = rng_state;
  z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
  z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
  z = z ^ (z >> 31);
  // Pick uniformly among the count-1 entries that are NOT `current`.
  std::size_t pick = static_cast<std::size_t>(z % (count - 1));
  const std::size_t cur = current < count ? current : 0;
  if (pick >= cur) {
    ++pick;
  }
  return pick;
}

int PlaylistIndexForPath(const std::vector<std::wstring>& order,
                         const std::wstring& current) {
  const std::wstring needle = FoldAscii(current);
  for (std::size_t i = 0; i < order.size(); ++i) {
    if (FoldAscii(order[i]) == needle) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

std::vector<std::wstring> NormalizeOrder(std::vector<std::wstring> order) {
  std::vector<std::wstring> out;
  out.reserve(order.size());
  std::unordered_set<std::wstring> seen;
  for (std::wstring& entry : order) {
    if (entry.empty()) {
      continue;
    }
    if (seen.insert(FoldAscii(entry)).second) {
      out.push_back(std::move(entry));
    }
  }
  return out;
}

nlohmann::json PlaylistToJson(const PlaylistConfig& cfg) {
  nlohmann::json order = nlohmann::json::array();
  for (const std::wstring& p : cfg.order) {
    order.push_back(std::filesystem::path(p).u8string());
  }
  nlohmann::json j;
  j["version"] = cfg.version;
  j["enabled"] = cfg.enabled;
  j["interval_min"] = cfg.interval_min;
  j["shuffle"] = cfg.shuffle;
  j["order"] = std::move(order);
  return j;
}

PlaylistConfig LoadPlaylist(const std::filesystem::path& path) {
  const std::string text = detail::ReadFile(path, "playlist file");
  nlohmann::json raw;
  try {
    raw = nlohmann::json::parse(text);
  } catch (const nlohmann::json::exception& e) {
    // QA-fail path: keep the corrupt bytes for forensics, then report a
    // structured error. The caller falls back to last-valid/defaults.
    detail::BackupFile(path);
    throw ConfigError(std::string("corrupt playlist JSON: ") + e.what());
  }
  PlaylistConfig cfg;
  try {
    cfg = MigratePlaylist(raw);
  } catch (const ConfigError&) {
    detail::BackupFile(path);
    throw;
  }
  const bool migrated = NeedsMigration(raw);
  if (migrated) {
    detail::BackupFile(path);
  }
  try {
    ValidatePlaylist(cfg);
  } catch (const ConfigError&) {
    detail::BackupFile(path);
    throw;
  }
  if (migrated) {
    // Self-healing persist so migration runs exactly once. Best-effort: a
    // read-only disk must not turn a clean load into an error.
    try {
      SavePlaylist(path, cfg);
    } catch (const ConfigError&) {
    }
  }
  return cfg;
}

void SavePlaylist(const std::filesystem::path& path, const PlaylistConfig& cfg) {
  PlaylistConfig copy = cfg;
  copy.version = kPlaylistSchemaVersion;
  copy.order = NormalizeOrder(std::move(copy.order));
  ValidatePlaylist(copy);
  AtomicWriteJson(path, PlaylistToJson(copy));
}

void ValidatePlaylist(PlaylistConfig& cfg) {
  if (cfg.interval_min < kPlaylistMinIntervalMin ||
      cfg.interval_min > kPlaylistMaxIntervalMin) {
    throw ConfigError("interval_min out of range [1,1440]: " +
                      std::to_string(cfg.interval_min));
  }
  if (cfg.order.size() > kPlaylistMaxEntries) {
    throw ConfigError("playlist exceeds maximum entries (" +
                      std::to_string(kPlaylistMaxEntries) + ")");
  }
}

PlaylistConfig MigratePlaylist(const nlohmann::json& raw) {
  if (!raw.is_object()) {
    throw ConfigError("playlist root must be a JSON object");
  }
  PlaylistConfig cfg;
  try {
    const int version = raw.value("version", 0);
    if (version < 0 || version > kPlaylistSchemaVersion) {
      throw ConfigError("unsupported playlist schema version: " +
                        std::to_string(version));
    }
    cfg.version = kPlaylistSchemaVersion;
    cfg.enabled = raw.value("enabled", false);
    cfg.interval_min = raw.value("interval_min", 30);
    cfg.shuffle = raw.value("shuffle", false);
    if (raw.contains("order")) {
      if (!raw.at("order").is_array()) {
        throw ConfigError("playlist order must be an array");
      }
      std::vector<std::wstring> order;
      for (const auto& entry : raw.at("order")) {
        if (!entry.is_string()) {
          throw ConfigError("playlist order entries must be strings");
        }
        order.push_back(
            std::filesystem::u8path(entry.get<std::string>()).wstring());
      }
      cfg.order = NormalizeOrder(std::move(order));
    }
  } catch (const ConfigError&) {
    throw;
  } catch (const nlohmann::json::exception& e) {
    throw ConfigError(std::string("playlist field type error: ") + e.what());
  }
  return cfg;
}

std::filesystem::path DefaultPlaylistPath() {
  return DefaultConfigPath().parent_path() / L"playlist.json";
}

std::filesystem::path PlaylistPathForConfig(
    const std::filesystem::path& config_path) {
  return config_path.parent_path() / L"playlist.json";
}

}  // namespace k6wp
