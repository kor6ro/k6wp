#pragma once
// Compressor cache manager — LRU file cache (max 5 GiB) under
// %LOCALAPPDATA%/K6WP/cache. Key = SHA1(input identity + job params).
// This header is windows.h-free; only std types + filesystem.

#include <cstdint>
#include <filesystem>
#include <string>

namespace k6wp::compressor {

// Default budget: 5 GiB.
inline constexpr std::uint64_t kCacheMaxBytes =
    5ULL * 1024ULL * 1024ULL * 1024ULL;

/// Cache root: %LOCALAPPDATA%/K6WP/cache (USERPROFILE fallback).
/// Test override: K6WP_CACHE_DIR env var wins when non-empty.
std::filesystem::path CacheRoot();

/// Effective budget in bytes. Test override: K6WP_CACHE_MAX_BYTES env var
/// (decimal) wins when it parses to > 0; otherwise kCacheMaxBytes.
std::uint64_t CacheMaxBytes();

/// Build the cache key for a job. Input identity = absolute path + file
/// size + last-write time, mixed with res/fps/crf/encoder. Returns "" when
/// the input file cannot be stated (caller should skip the cache).
std::string CacheKeyHex(const std::filesystem::path& in, int res_w,
                        int res_h, int fps, int crf,
                        const std::string& encoder);

/// Full path of the cached artifact for a key: <root>/<hex>.mp4
std::filesystem::path CachePathForKey(const std::string& key_hex);

/// True when a usable cached artifact exists for `key_hex`. Touches the
/// cached file's mtime (LRU recency) on hit. Never throws.
bool CacheHas(const std::string& key_hex);

/// Copy a cached artifact to `out` (overwrite). "" error on success.
/// Disk-full / write errors come back as a human-readable `error` + nonzero.
int CacheFetchToOut(const std::string& key_hex,
                    const std::filesystem::path& out, std::string& error);

/// Copy a freshly-encoded `out` file into the cache slot for `key_hex`,
/// then prune to budget. "" error on success.
int CacheStoreFromOut(const std::string& key_hex,
                      const std::filesystem::path& out, std::string& error);

/// Delete oldest-mtime *.mp4 files until total <= CacheMaxBytes().
/// Best-effort: returns 0 even when individual deletes fail. Never throws.
int CachePrune(std::string& error);

}  // namespace k6wp::compressor
