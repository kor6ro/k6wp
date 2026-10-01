#include "cache_manager.hpp"

#include "sha1.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace k6wp::compressor {

// MSVC-safe getenv wrapper (windows.h-free; _dupenv_s lives in <cstdlib>).
// Returns "" when the variable is unset or empty.
std::string GetEnvStr(const char* name) {
  char* buf = nullptr;
  std::size_t len = 0;
  if (_dupenv_s(&buf, &len, name) != 0 || buf == nullptr) {
    return "";
  }
  std::string value(buf);
  std::free(buf);
  return value;
}

std::filesystem::path CacheRoot() {
  const std::string override = GetEnvStr("K6WP_CACHE_DIR");
  if (!override.empty()) {
    return std::filesystem::path(override);
  }
  const std::string local = GetEnvStr("LOCALAPPDATA");
  if (!local.empty()) {
    return std::filesystem::path(local) / "K6WP" / "cache";
  }
  const std::string profile = GetEnvStr("USERPROFILE");
  if (!profile.empty()) {
    return std::filesystem::path(profile) / "AppData" / "Local" / "K6WP" /
           "cache";
  }
  std::error_code ec;
  return std::filesystem::temp_directory_path(ec) / "K6WP" / "cache";
}

std::uint64_t CacheMaxBytes() {
  const std::string override = GetEnvStr("K6WP_CACHE_MAX_BYTES");
  if (!override.empty()) {
    try {
      const unsigned long long v =
          std::stoull(override, nullptr, 10);  // NOLINT
      if (v > 0) {
        return static_cast<std::uint64_t>(v);
      }
    } catch (...) {
    }
  }
  return kCacheMaxBytes;
}

std::string CacheKeyHex(const std::filesystem::path& in, int res_w,
                        int res_h, int fps, int crf,
                        const std::string& encoder) {
  std::error_code ec;
  const std::filesystem::path abs = std::filesystem::absolute(in, ec);
  if (ec) {
    return "";
  }
  const std::uint64_t size = std::filesystem::file_size(abs, ec);
  if (ec) {
    return "";
  }
  const auto mtime = std::filesystem::last_write_time(abs, ec);
  if (ec) {
    return "";
  }
  const long long mtime_count =
      static_cast<long long>(mtime.time_since_epoch().count());
  std::string material;
  material.reserve(256);
  material += abs.u8string();
  material += '|';
  material += std::to_string(size);
  material += '|';
  material += std::to_string(mtime_count);
  material += '|';
  material += std::to_string(res_w);
  material += 'x';
  material += std::to_string(res_h);
  material += '|';
  material += std::to_string(fps);
  material += '|';
  material += std::to_string(crf);
  material += '|';
  material += encoder;
  // Cache generation: v3 = single shared SHA1 (MED-3) + DPB-pinned encodes
  // (refs=2/bf=1, 2026-09-20). Bumps the key so pre-v3 cached files miss
  // and re-encode instead of being served forever.
  material += "|v3";
  return k6wp::Sha1Hex(material);
}

std::filesystem::path CachePathForKey(const std::string& key_hex) {
  return CacheRoot() / (key_hex + ".mp4");
}

bool CacheHas(const std::string& key_hex) {
  if (key_hex.empty()) {
    return false;
  }
  std::error_code ec;
  const auto path = CachePathForKey(key_hex);
  if (!std::filesystem::is_regular_file(path, ec) || ec) {
    return false;
  }
  if (std::filesystem::file_size(path, ec) == 0 || ec) {
    return false;
  }
  // LRU touch: a hit counts as recent use.
  const auto now = std::filesystem::file_time_type::clock::now();
  std::filesystem::last_write_time(path, now, ec);
  return true;
}

int CacheFetchToOut(const std::string& key_hex,
                    const std::filesystem::path& out, std::string& error) {
  const auto cached = CachePathForKey(key_hex);
  std::error_code ec;
  if (out.has_parent_path()) {
    std::filesystem::create_directories(out.parent_path(), ec);
    if (ec) {
      error = "cache: cannot create output dir: " + ec.message();
      return 1;
    }
  }
  std::filesystem::copy_file(cached, out,
                             std::filesystem::copy_options::overwrite_existing,
                             ec);
  if (ec) {
    error = "cache: cannot copy cached file to out (disk full?): " +
            ec.message();
    return 1;
  }
  return 0;
}

int CacheStoreFromOut(const std::string& key_hex,
                      const std::filesystem::path& out, std::string& error) {
  if (key_hex.empty()) {
    error = "cache: empty key";
    return 1;
  }
  const auto cached = CachePathForKey(key_hex);
  std::error_code ec;
  std::filesystem::create_directories(cached.parent_path(), ec);
  if (ec) {
    error = "cache: cannot create cache dir: " + ec.message();
    return 1;
  }
  std::filesystem::path tmp = cached;
  tmp += L".tmp";
  std::filesystem::copy_file(out, tmp,
                             std::filesystem::copy_options::overwrite_existing,
                             ec);
  if (ec) {
    error = "cache: cannot store to cache (disk full?): " + ec.message();
    std::error_code cleanup_ec;
    std::filesystem::remove(tmp, cleanup_ec);
    return 1;
  }
  // Publish atomically so a concurrent reader never sees a half-written .mp4:
  // rename() replaces on the same volume (MSVC MoveFileExW). A .tmp orphan is
  // ignored by CachePrune's extension filter.
  std::filesystem::rename(tmp, cached, ec);
  if (ec) {
    error = "cache: cannot publish cache file: " + ec.message();
    std::error_code cleanup_ec;
    std::filesystem::remove(tmp, cleanup_ec);
    return 1;
  }
  std::string prune_error;
  CachePrune(prune_error);  // Best-effort; publish already succeeded.
  return 0;
}

int CachePrune(std::string& error) {
  const std::uint64_t budget = CacheMaxBytes();
  const auto root = CacheRoot();
  std::error_code ec;
  if (!std::filesystem::is_directory(root, ec) || ec) {
    return 0;  // Nothing to prune.
  }
  struct Entry {
    std::filesystem::file_time_type mtime;
    std::uint64_t size = 0;
    std::filesystem::path path;
  };
  std::vector<Entry> entries;
  std::uint64_t total = 0;
  std::filesystem::directory_iterator it(root, ec);
  if (ec) {
    error = "cache: cannot enumerate cache dir: " + ec.message();
    return 1;
  }
  const std::filesystem::directory_iterator end;
  for (; it != end; it.increment(ec)) {
    if (ec) {
      break;
    }
    const auto& p = it->path();
    if (p.extension() != ".mp4") {
      continue;
    }
    std::error_code lec;
    if (!it->is_regular_file(lec) || lec) {
      continue;
    }
    const auto size = it->file_size(lec);
    if (lec) {
      continue;
    }
    const auto mtime = it->last_write_time(lec);
    if (lec) {
      continue;
    }
    entries.push_back(Entry{mtime, static_cast<std::uint64_t>(size), p});
    total += static_cast<std::uint64_t>(size);
  }
  if (total <= budget) {
    return 0;
  }
  std::sort(entries.begin(), entries.end(),
            [](const Entry& a, const Entry& b) { return a.mtime < b.mtime; });
  for (const auto& e : entries) {
    if (total <= budget) {
      break;
    }
    std::error_code dec;
    std::filesystem::remove(e.path, dec);
    if (!dec) {
      total -= (std::min)(total, e.size);
    }
  }
  return 0;
}

}  // namespace k6wp::compressor
