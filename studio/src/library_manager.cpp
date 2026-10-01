#include "library_manager.hpp"

#include <algorithm>
#include <cstdio>
#include <fstream>

#include <QFile>

#include "ffprobe_helper.hpp"

// windows.h must stay out of the header; only the .cpp needs it
// (GetEnvironmentVariableW for the %LOCALAPPDATA% pattern).
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

// nlohmann/json is vendored at shared/thirdparty/json.hpp; k6wp_shared
// exposes shared/ on the include path (config_schema pattern).
#include "thirdparty/json.hpp"

// kMaxConfigBytes + ConfigError for the MED-18 read-size guard below.
#include "config_schema.hpp"

// Task 23: production logging. Diagnostics mirror to stderr only in verbose
// builds (root CMake option K6WP_VERBOSE); default build stays console-clean.
#ifndef K6WP_VERBOSE
#define K6WP_VERBOSE 0
#endif

namespace k6wp {

namespace {

constexpr wchar_t kLibraryFileSubdir[] = L"K6WP";
constexpr wchar_t kLibraryFileName[] = L"library.json";
constexpr int kLibrarySchemaVersion = 1;

// Reads the whole file into a string. RAII via std::ifstream. MED-18:
// rejects files larger than kMaxConfigBytes BEFORE reading (same guard as
// shared/config_schema.cpp); the error is ConfigError per the audit contract
// (all library Load call-sites use catch(...) so no caller needs changing).
std::string ReadFile(const std::filesystem::path& path, bool& exists) {
  std::error_code ec;
  exists = std::filesystem::exists(path, ec);
  if (ec || !exists) {
    exists = false;
    return {};
  }
  const std::uintmax_t size = std::filesystem::file_size(path, ec);
  if (!ec && size > kMaxConfigBytes) {
    throw ConfigError("library file exceeds maximum size (" +
                      std::to_string(kMaxConfigBytes) + " bytes): " +
                      path.string());
  }
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    throw LibraryError("cannot open library file: " + path.string());
  }
  return std::string(std::istreambuf_iterator<char>(in),
                     std::istreambuf_iterator<char>());
}

// Refreshes informational fields from disk. Missing dst => zeros (the
// entry is still valid; ListItems() will flag it broken).
void RefreshStat(LibraryEntry& entry) {
  std::error_code ec;
  const auto size = std::filesystem::file_size(entry.dst, ec);
  if (!ec) {
    entry.size = static_cast<std::uint64_t>(size);
  } else {
    entry.size = 0;
  }
  const auto ft = std::filesystem::last_write_time(entry.dst, ec);
  if (!ec) {
    entry.mtime = static_cast<std::int64_t>(ft.time_since_epoch().count());
  } else {
    entry.mtime = 0;
  }
}

nlohmann::json EntryToJson(const LibraryEntry& e) {
  nlohmann::json j;
  j["src"] = std::filesystem::path(e.src).u8string();
  j["dst"] = std::filesystem::path(e.dst).u8string();
  j["res"] = e.res;
  j["fps"] = e.fps;
  j["crf"] = e.crf;
  j["encoder"] = e.encoder;
  j["mtime"] = e.mtime;
  j["size"] = e.size;
  j["duration"] = e.duration;
  j["codec"] = e.codec;
  j["width"] = e.width;
  j["height"] = e.height;
  j["thumb"] = std::filesystem::path(e.thumb).u8string();
  return j;
}

LibraryEntry EntryFromJson(const nlohmann::json& j) {
  LibraryEntry e;
  // value() on a wrong-typed field throws type_error (a json::exception);
  // the caller converts it to LibraryError.
  e.src = std::filesystem::u8path(j.value("src", ""));
  e.dst = std::filesystem::u8path(j.value("dst", ""));
  e.res = j.value("res", "0x0");
  e.fps = j.value("fps", 30);
  e.crf = j.value("crf", 23);
  e.encoder = j.value("encoder", "libx264");
  e.mtime = j.value("mtime", std::int64_t{0});
  e.size = j.value("size", std::uint64_t{0});
  e.duration = j.value("duration", 0.0);
  e.codec = j.value("codec", "");
  e.width = j.value("width", 0);
  e.height = j.value("height", 0);
  e.thumb = std::filesystem::u8path(j.value("thumb", ""));
  e.broken = false;  // computed at ListItems() time, never persisted
  return e;
}

}  // namespace

LibraryManager::LibraryManager(std::filesystem::path json_path) {
  if (!json_path.empty()) {
    json_path_ = std::move(json_path);
    return;
  }
  json_path_ = DefaultLibraryJsonPath();
}

std::filesystem::path LibraryManager::DefaultLibraryJsonPath() {
  // Test override wins when set (never pollute the real library.json).
  char override_buf[MAX_PATH];
  const DWORD override_len =
      GetEnvironmentVariableA("K6WP_LIBRARY_JSON", override_buf, MAX_PATH);
  if (override_len > 0 && override_len < MAX_PATH) {
    return std::filesystem::path(override_buf);
  }
  wchar_t buf[MAX_PATH];
  const DWORD local = GetEnvironmentVariableW(L"LOCALAPPDATA", buf, MAX_PATH);
  if (local > 0 && local < MAX_PATH) {
    return std::filesystem::path(buf) / kLibraryFileSubdir / kLibraryFileName;
  }
  const DWORD profile = GetEnvironmentVariableW(L"USERPROFILE", buf, MAX_PATH);
  if (profile > 0 && profile < MAX_PATH) {
    return std::filesystem::path(buf) / L"AppData" / L"Local" /
           kLibraryFileSubdir / kLibraryFileName;
  }
  throw LibraryError("LOCALAPPDATA and USERPROFILE are both unset");
}

void LibraryManager::Load() {
  bool exists = false;
  std::string text;
  recovered_from_backup_ = false;
  try {
    text = ReadFile(json_path_, exists);
  } catch (const LibraryError&) {
    throw;
  }
  if (!exists) {
    entries_.clear();  // first run: no file yet
    return;
  }
  nlohmann::json root;
  try {
    root = nlohmann::json::parse(text);
  } catch (const nlohmann::json::exception& e) {
    // Fall back to the pre-save copy rather than leaving the user with an
    // unrecoverable index. The .bak is the state as of the last successful
    // Save, so at worst one change is lost instead of the whole library.
    std::error_code bak_ec;
    std::filesystem::path bak = json_path_;
    bak += L".bak";
    if (std::filesystem::exists(bak, bak_ec) && !bak_ec) {
      std::string bak_text;
      bool bak_exists = false;
      try {
        bak_text = ReadFile(bak, bak_exists);
      } catch (const LibraryError&) {
        bak_exists = false;
      }
      if (bak_exists) {
        try {
          nlohmann::json recovered = nlohmann::json::parse(bak_text);
          if (recovered.is_object()) {
            recovered_from_backup_ = true;
            root = std::move(recovered);
          } else {
            throw LibraryError(std::string("corrupt library JSON: ") + e.what());
          }
        } catch (const nlohmann::json::exception&) {
          throw LibraryError(std::string("corrupt library JSON: ") + e.what());
        }
      } else {
        throw LibraryError(std::string("corrupt library JSON: ") + e.what());
      }
    } else {
      throw LibraryError(std::string("corrupt library JSON: ") + e.what());
    }
  }
  try {
    if (!root.is_object()) {
      throw LibraryError("library root must be a JSON object");
    }
    std::vector<LibraryEntry> loaded;
    const auto it = root.find("entries");
    if (it != root.end()) {
      if (!it->is_array()) {
        throw LibraryError("library \"entries\" must be an array");
      }
      for (const auto& je : *it) {
        if (!je.is_object()) {
          throw LibraryError("library entry must be an object");
        }
        loaded.push_back(EntryFromJson(je));
      }
    }
    entries_ = std::move(loaded);
  } catch (const LibraryError&) {
    throw;
  } catch (const nlohmann::json::exception& e) {
    throw LibraryError(std::string("library field type error: ") + e.what());
  }
}

void LibraryManager::Save() const {
  nlohmann::json root;
  root["version"] = kLibrarySchemaVersion;
  root["entries"] = nlohmann::json::array();
  for (const auto& e : entries_) {
    root["entries"].push_back(EntryToJson(e));
  }
  // Same crash-safe contract as config.json / studio_settings.json: write to
  // "<path>.tmp", then replace atomically, so a force-kill during Add / Remove /
  // Clear cannot leave a truncated index. The previous contents are copied to
  // "<path>.bak" BEFORE the write, because after it the old bytes are gone and
  // Load() would have nothing to fall back to.
  std::error_code ec;
  if (!json_path_.parent_path().empty()) {
    std::filesystem::create_directories(json_path_.parent_path(), ec);
  }
  if (std::filesystem::exists(json_path_, ec) && !ec) {
    std::filesystem::path bak = json_path_;
    bak += L".bak";
    std::error_code copy_ec;
    std::filesystem::copy_file(json_path_, bak,
                               std::filesystem::copy_options::overwrite_existing,
                               copy_ec);
  }
  try {
    k6wp::AtomicWriteJson(json_path_, root);
  } catch (const k6wp::ConfigError& e) {
    throw LibraryError(std::string("cannot write library file: ") + e.what());
  }
}

void LibraryManager::Add(const LibraryEntry& entry, MetadataProbe probe) {
  LibraryEntry fresh = entry;
  RefreshStat(fresh);
  const auto same = [&](const LibraryEntry& e) { return e.dst == fresh.dst; };
  const auto it = std::find_if(entries_.begin(), entries_.end(), same);
  const bool existed = (it != entries_.end());

  const bool caller_has_meta = (fresh.width > 0 && fresh.height > 0);
  bool need_probe = !caller_has_meta;
  if (existed && !caller_has_meta) {
    const bool same_mtime = (it->mtime != 0 && it->mtime == fresh.mtime);
    const bool cached_meta = (it->width > 0 && it->height > 0);
    if (same_mtime && cached_meta) {
      need_probe = false;
      fresh.width = it->width;
      fresh.height = it->height;
      fresh.codec = it->codec;
      fresh.duration = it->duration;
      fresh.fps = it->fps;
      fresh.res = it->res;
      fresh.thumb = it->thumb;
    }
  }

  if (need_probe && probe == kNever) {
    need_probe = false;
  }

  if (need_probe) {
    std::error_code ec;
    std::filesystem::path target;
    if (!fresh.dst.empty() && std::filesystem::is_regular_file(fresh.dst, ec) &&
        !ec) {
      target = fresh.dst;
    } else if (!fresh.src.empty() &&
               std::filesystem::is_regular_file(fresh.src, ec) && !ec) {
      target = fresh.src;
    }
    if (!target.empty()) {
      VideoMetadata meta;
      if (FfprobeHelper().Probe(target, meta)) {
        fresh.width = meta.width;
        fresh.height = meta.height;
        fresh.codec = meta.codec;
        fresh.duration = meta.duration;
        if (meta.fps > 0.0) {
          fresh.fps = static_cast<int>(meta.fps + 0.5);
        }
        fresh.res = std::to_string(meta.width) + "x" +
                    std::to_string(meta.height);
      } else {
#if K6WP_VERBOSE
        std::fputs(("[library] probe failed, placeholder kept for: " +
                    target.u8string() + "\n")
                       .c_str(),
                   stderr);
        std::fflush(stderr);
#endif
      }
    }
  }

  if (fresh.thumb.empty() && !fresh.dst.empty()) {
    auto sibling = fresh.dst;
    sibling.replace_extension(L".jpg");
    std::error_code ec;
    if (std::filesystem::is_regular_file(sibling, ec) && !ec) {
      fresh.thumb = sibling;
    }
  }

  if (existed) {
    const auto pos =
        std::find_if(entries_.begin(), entries_.end(), same);
    if (pos != entries_.end()) {
      *pos = fresh;  // upsert: same dst => replace
    } else {
      entries_.push_back(fresh);
    }
  } else {
    entries_.push_back(fresh);
  }
  Save();
}

std::vector<LibraryEntry> LibraryManager::ListItems() const {
  std::vector<LibraryEntry> items = entries_;
  // Broken-entry path: stat each dst with error_code; missing => broken.
  // Entry KEPT (user may re-link), never throws.
  for (auto& e : items) {
    std::error_code ec;
    e.broken = !std::filesystem::exists(e.dst, ec) || ec;
  }
  std::sort(items.begin(), items.end(), [](const LibraryEntry& a, const LibraryEntry& b) {
    return a.dst.filename().wstring() < b.dst.filename().wstring();
  });
  return items;
}

bool LibraryManager::Remove(const std::filesystem::path& dst,
                            bool move_file_to_trash,
                            std::string* error_out) {
  const auto it =
      std::find_if(entries_.begin(), entries_.end(),
                   [&](const LibraryEntry& e) { return e.dst == dst; });
  if (it == entries_.end()) {
    return false;
  }
  if (move_file_to_trash) {
    // Thumbnail first and best-effort: if the primary file is trashed but its
    // .jpg fails, the entry must still be removed, or it would be kept while
    // pointing at a file that no longer exists.
    const std::filesystem::path thumb =
        std::filesystem::path(it->dst).replace_extension(L".jpg");
    std::error_code thumb_ec;
    if (!thumb.empty() &&
        std::filesystem::is_regular_file(thumb, thumb_ec) && !thumb_ec) {
      QFile::moveToTrash(QString::fromStdWString(thumb.wstring()));
    }
    std::error_code ec;
    if (std::filesystem::is_regular_file(it->dst, ec) && !ec &&
        !QFile::moveToTrash(QString::fromStdWString(it->dst.wstring()))) {
      if (error_out != nullptr) {
        *error_out = "could not move to Recycle Bin (no trash on this "
                     "volume?): " +
                     it->dst.u8string();
      }
      return false;
    }
  }
  entries_.erase(it);
  Save();
  return true;
}

std::size_t LibraryManager::Size() const { return entries_.size(); }

void LibraryManager::Clear() {
  entries_.clear();
  Save();
}

std::string LibraryManager::CacheKeyMaterial(const LibraryEntry& entry) {
  return std::filesystem::path(entry.src).u8string() + "|" + entry.res + "|" +
         std::to_string(entry.fps) + "|" + std::to_string(entry.crf) + "|" +
         entry.encoder;
}

LibraryEntry LibraryManager::ReferenceInPlace(
    const std::filesystem::path& src_path) {
  std::error_code ec;
  if (!std::filesystem::is_regular_file(src_path, ec) || ec) {
    throw LibraryError("Source file not found: " + src_path.u8string());
  }
  std::error_code abs_ec;
  std::filesystem::path dest = std::filesystem::absolute(src_path, abs_ec);
  if (abs_ec || dest.empty()) {
    dest = src_path;
  }
  LibraryEntry entry;
  entry.src = dest;
  entry.dst = dest;  // A5: referenced in place — src == dst
  return entry;
}

}  // namespace k6wp
