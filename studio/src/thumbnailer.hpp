#pragma once

#include <filesystem>
#include <string>

namespace k6wp {

// Central mtime-keyed video thumbnail cache.
//
// Thumbnailer owns the shared cache at %LOCALAPPDATA%/K6WP/thumbs/:
//   * GetThumb() maps ANY video path to a cached JPEG whose filename is the
//     SHA1 of (abs-path | size | mtime), so a changed file automatically
//     misses and regenerates. It uses fast input seek (-ss 1 BEFORE -i)
//     plus fixed 320x180 output to stay inside the <500ms grid budget.
// Header stays Win32-free (same pattern as config_schema.hpp);
// windows.h lives only in thumbnailer.cpp.
class Thumbnailer {
 public:
  Thumbnailer();

  // Returns the thumbs cache root directory.
  // Honors the K6WP_THUMBS_DIR env override for tests (mirrors the
  // K6WP_CACHE_DIR pattern from compressor/src/cache_manager.cpp).
  [[nodiscard]] std::filesystem::path ThumbsRoot() const;

  // Ensures the thumbs root exists. Returns false (never throws) on failure.
  [[nodiscard]] bool EnsureRoot();

  // Cache key for a video: SHA1(abs-path | size | mtime) as 40 hex chars.
  // Returns "" when the file cannot be stated.
  [[nodiscard]] static std::string CacheKeyHex(
      const std::filesystem::path& video);

  // Returns the cached JPEG path for a video, generating it on a miss.
  // Returns an empty path on ANY failure (missing/corrupt video, ffmpeg
  // missing or failing, I/O error) and logs to stderr — the caller shows a
  // placeholder. NEVER throws.
  [[nodiscard]] std::filesystem::path GetThumb(
      const std::filesystem::path& video);

  // Non-blocking cache lookup: returns the cached JPEG path when a
  // non-empty thumbnail already exists for the video, otherwise an empty
  // path. NEVER generates (no ffmpeg spawn) and never logs, so the grid's
  // Refresh() fast path can call it per item on the UI thread. NEVER
  // throws. Thread-safe (read-only): the async worker calls GetThumb()
  // while the UI thread calls this.
  [[nodiscard]] std::filesystem::path CachedThumb(
      const std::filesystem::path& video) const;

  // Runs bundled ffmpeg with fast input seek to render one frame:
  //   ffmpeg -y -ss 1 -i <video> -frames:v 1 -s 320x180 -q:v 4 <thumb>
  // Returns true on success. Never throws. Shared by Studio import/library
  // flows so the CreateProcessW invocation exists in exactly one place.
  [[nodiscard]] static bool GenerateTo(const std::filesystem::path& video,
                                       const std::filesystem::path& thumb);

  // Resolves the bundled ffmpeg.exe via k6wp::FindFfmpeg (env override +
  // <exe_dir>\ffmpeg.exe flat + build + vendored fallback). Never throws.
  [[nodiscard]] static std::filesystem::path FfmpegPath();

 private:
  std::filesystem::path root_;
};

}  // namespace k6wp
