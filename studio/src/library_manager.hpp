#pragma once

// Studio library metadata layer (Todo 38, fase-5).
//
// ONE LibraryManager per Studio process: LibraryGridModel owns it and wraps
// its throwing API for QML. It persists
// one JSON object per imported video to %LOCALAPPDATA%/K6WP/library.json so
// Studio can list/persist/delete entries and show compressor-cache status.
//
// Each entry stores the compressor cache key material (res/fps/crf/encoder
// + src identity) — the LRU link to Todo 17's cache_manager.
//
// Header stays Win32-free (config_schema pattern); windows.h lives only in
// the .cpp.

#include <cstdint>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

namespace k6wp {

// Structured error thrown on corrupt/unwritable library.json.
class LibraryError : public std::runtime_error {
 public:
  explicit LibraryError(const std::string& message) : std::runtime_error(message) {}
};

// One indexed library entry. `broken` is COMPUTED by ListItems() (dst
// missing on disk) and is never persisted — the entry is KEPT so the user
// may re-link it.
struct LibraryEntry {
  std::filesystem::path src;      // original source path (identity for cache key)
  std::filesystem::path dst;      // library copy (unique key)
  std::string res = "0x0";        // e.g. "1920x1080" (cache key material)
  int fps = 30;                   // cache key material
  int crf = 23;                   // cache key material
  std::string encoder = "libx264";  // cache key material
  std::int64_t mtime = 0;         // dst last-write time (file-time ticks, informational)
  std::uint64_t size = 0;         // dst file size in bytes (informational)
  // Metadata probed ONCE at import (or on mtime change) and cached:
  double duration = 0.0;          // seconds, 0 = unknown
  std::string codec = "";         // e.g. "h264", "hevc", "vp9"
  int width = 0;                  // video width in pixels
  int height = 0;                 // video height in pixels
  std::filesystem::path thumb;    // cached thumbnail (.jpg sibling), may be empty
  bool broken = false;            // computed: dst missing on disk
};

// Whether Add() may block on a synchronous ffprobe. See Add()'s comment for
// why a caller would ever opt out.
enum MetadataProbe {
  kIfMissing,  // probe inline when the entry has no dimensions (default)
  kNever,      // never probe inline; the caller supplies metadata later
};

class LibraryManager {
 public:
  // Empty path => DefaultLibraryJsonPath(). Explicit path (or the
  // K6WP_LIBRARY_JSON env override) is for tests — never pollute the real
  // %LOCALAPPDATA%/K6WP/library.json in tests.
  explicit LibraryManager(std::filesystem::path json_path = {});

  // %LOCALAPPDATA%/K6WP/library.json (USERPROFILE fallback).
  // Test override: K6WP_LIBRARY_JSON env var wins when non-empty.
  static std::filesystem::path DefaultLibraryJsonPath();

  // Loads entries from disk. Missing file => empty library (first run).
  // Corrupt main file falls back to the .bak; throws only when neither parses.
  void Load();

  // True when the last Load() fell back to the .bak. Reset by Load().
  [[nodiscard]] bool RecoveredFromBackup() const { return recovered_from_backup_; }

  // Persists entries, copying the previous contents to "<file>.bak" first and
  // then replacing atomically so a crash mid-write cannot truncate the index.
  void Save() const;

  // Adds an entry, or replaces the existing entry with the same dst
  // (upsert). Refreshes mtime/size from disk when dst exists; leaves the
  // caller's values otherwise. Probes ffprobe metadata exactly once for a
  // new or changed (mtime) file and caches it in the entry; refresh paths
  // (ListItems) never probe. A probe failure keeps the entry with empty
  // metadata so the grid shows a placeholder. Persists immediately.
  //
  // kIfMissing is the default and keeps every existing caller (the
  // compress bridge's post-compress upsert) correct without knowing about
  // this switch. kNever is for callers that own the probe themselves and
  // would otherwise block: FfprobeHelper::Probe spawns ffprobe and waits up
  // to 10s, so a UI-thread import of N videos froze for N*10s. kNever still
  // carries over the cached metadata of an unchanged entry, so a kNever
  // upsert never downgrades known metadata to a placeholder.
  void Add(const LibraryEntry& entry, MetadataProbe probe = kIfMissing);

  // Returns all entries sorted by dst filename. Stats each dst with
  // error_code; a missing dst sets broken=true (entry KEPT, never throws).
  [[nodiscard]] std::vector<LibraryEntry> ListItems() const;

  // Removes the entry keyed by dst (if present) and persists. NEVER deletes
  // user files permanently. move_file_to_trash == true moves dst (+ its
  // .jpg thumbnail sibling when present) via QFile::moveToTrash; a trash
  // failure is NOT silently upgraded to a permanent delete — returns false
  // with *error_out set so the caller can show the reason. == false removes
  // METADATA only (file + thumb stay on disk). Returns true when an entry
  // was removed. Trash failures report via error_out; persist errors throw
  // LibraryError like the other mutators.
  bool Remove(const std::filesystem::path& dst, bool move_file_to_trash,
              std::string* error_out = nullptr);

  [[nodiscard]] std::size_t Size() const;
  void Clear();

  // LRU link: stable cache-key material string for an entry
  // (src|res|fps|crf|encoder) so Studio can show cache status.
  [[nodiscard]] static std::string CacheKeyMaterial(const LibraryEntry& entry);

  // ReferenceInPlace: creates a library entry that references a video
  // file in place (src == dst = absolute source path, empty thumb).
  // Metadata is deferred to Add(). Throws LibraryError when source
  // is missing or not a regular file.
  [[nodiscard]] static LibraryEntry ReferenceInPlace(
      const std::filesystem::path& src_path);

 private:
  std::filesystem::path json_path_;
  std::vector<LibraryEntry> entries_;
  bool recovered_from_backup_ = false;
};

}  // namespace k6wp
