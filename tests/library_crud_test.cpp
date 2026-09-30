// LibraryManager coverage (MED-2 todo 21 + HIGH-3 A): metadata index over a
// tmp dir — add/upsert/list/sort/persist/remove/clear plus corrupt-input and
// broken-entry behavior — plus ReferenceInPlace (valid in-place reference:
// src == dst absolute, empty thumb, deferred dimensions; missing-file and
// directory rejection; non-blocking repeated calls) and the shared-manager
// mutation/read contract (same-instance Add/Remove/Clear visible without a
// manual Load).
//
// No external test framework: plain asserts with a pass/fail counter, same
// style as tests/config_test.cpp. Exit code 0 = all pass.
//
// This TU #includes the REAL studio/src/library_manager.cpp
// (thumbnailer_test-style single-TU pattern) so the JSON round-trip and the
// upsert/probe-skip logic under test are the production code. Entries carry
// caller-provided probe metadata (width/height > 0), so Add never spawns
// ffprobe and the suite is hermetic. All removes use metadata-only mode, so
// QFile::moveToTrash is never invoked (Qt link is still required).
// The suite never touches %LOCALAPPDATA%: the manager is constructed with an
// explicit json path under %TEMP%.
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

#include "../studio/src/library_manager.cpp"

namespace {

int g_checks = 0;
int g_failures = 0;

void Check(bool cond, const std::string& name) {
  ++g_checks;
  if (cond) {
    std::printf("[PASS] %s\n", name.c_str());
  } else {
    ++g_failures;
    std::printf("[FAIL] %s\n", name.c_str());
  }
}

void WriteBytes(const std::filesystem::path& p, const std::string& data) {
  std::ofstream out(p, std::ios::binary | std::ios::trunc);
  out << data;
}

k6wp::LibraryEntry MakeEntry(const std::filesystem::path& src,
                             const std::filesystem::path& dst, int crf) {
  k6wp::LibraryEntry e;
  e.src = src;
  e.dst = dst;
  e.res = "1920x1080";
  e.fps = 30;
  e.crf = crf;
  e.encoder = "libx264";
  e.duration = 10.5;
  e.codec = "h264";
  e.width = 1920;
  e.height = 1080;
  return e;
}

void TestLibraryCrud() {
  auto dir = std::filesystem::temp_directory_path() / "k6wp_libcrud_test";
  std::error_code ec;
  std::filesystem::remove_all(dir, ec);
  std::filesystem::create_directories(dir, ec);
  const auto json = dir / "library.json";

  k6wp::LibraryManager m(json);
  m.Load();  // missing file => empty library (first run)
  Check(m.Size() == 0, "library CRUD tmp dir starts empty");

  const auto dst_b = dir / "b.mp4";
  const auto dst_a = dir / "a.mp4";
  WriteBytes(dst_b, "fake video b");
  WriteBytes(dst_a, "fake video a, slightly longer");

  m.Add(MakeEntry(dir / "orig_b.mp4", dst_b, 23));
  Check(m.Size() == 1, "library CRUD add grows to 1");
  {
    const auto items = m.ListItems();
    Check(items.size() == 1 && items[0].broken == false,
          "library CRUD fresh entry not broken");
    Check(items[0].res == "1920x1080" && items[0].codec == "h264" &&
              items[0].width == 1920 && items[0].height == 1080 &&
              items[0].duration == 10.5,
          "library CRUD caller metadata round-trips in memory");
  }

  m.Add(MakeEntry(dir / "orig_a.mp4", dst_a, 23));
  Check(m.Size() == 2, "library CRUD second add grows to 2");
  {
    const auto items = m.ListItems();
    Check(items.size() == 2 &&
              items[0].dst.filename() == "a.mp4" &&
              items[1].dst.filename() == "b.mp4",
          "library CRUD list sorted by dst filename");
  }

  // Upsert: same dst replaces instead of duplicating.
  m.Add(MakeEntry(dir / "orig_b.mp4", dst_b, 20));
  Check(m.Size() == 2, "library CRUD upsert same dst keeps size");
  {
    const auto items = m.ListItems();
    bool found = false;
    for (const auto& it : items) {
      if (it.dst == dst_b && it.crf == 20) {
        found = true;
      }
    }
    Check(found, "library CRUD upsert replaces entry fields");
  }

  // Persist: a fresh manager over the same tmp json sees the same rows.
  {
    k6wp::LibraryManager m2(json);
    m2.Load();
    Check(m2.Size() == 2, "library CRUD save persists across Load");
    const auto items = m2.ListItems();
    bool meta_ok = false;
    for (const auto& it : items) {
      if (it.dst == dst_b && it.codec == "h264" && it.width == 1920) {
        meta_ok = true;
      }
    }
    Check(meta_ok, "library CRUD persisted metadata survives reload");
  }

  // Cache-key material link (studio <-> compressor LRU).
  {
    const k6wp::LibraryEntry e = MakeEntry(dir / "orig_b.mp4", dst_b, 20);
    const std::string expected =
        std::filesystem::path(e.src).u8string() + "|1920x1080|30|20|libx264";
    Check(k6wp::LibraryManager::CacheKeyMaterial(e) == expected,
          "library CRUD cache key material stable");
  }

  // Broken-entry path: deleting dst flags broken but KEEPS the entry.
  std::filesystem::remove(dst_b, ec);
  {
    const auto items = m.ListItems();
    bool kept_broken = false;
    for (const auto& it : items) {
      if (it.dst == dst_b && it.broken) {
        kept_broken = true;
      }
    }
    Check(kept_broken, "library CRUD missing dst flagged broken but kept");
    Check(m.Size() == 2, "library CRUD broken entry still counted");
  }

  // Metadata-only remove: entry gone, user file stays on disk.
  Check(m.Remove(dst_a, false), "library CRUD remove present returns true");
  Check(m.Size() == 1, "library CRUD remove shrinks to 1");
  Check(std::filesystem::is_regular_file(dst_a),
        "library CRUD metadata-only remove keeps user file");
  Check(m.Remove(dir / "k6wp_no_such_entry_xyz.mp4", false) == false,
        "library CRUD remove missing returns false");

    // Corrupt main JSON recovers from the .bak instead of losing the library.
    // The .bak holds the state as of the last successful Save, which is the
    // best available answer; throwing would leave the user with no index at
    // all and no way to recover it short of hand-editing JSON.
    {
      WriteBytes(json, "{ not valid json !!!");
      k6wp::LibraryManager bad(json);
      bool threw = false;
      try {
        bad.Load();
      } catch (const k6wp::LibraryError&) {
        threw = true;
      }
      Check(!threw, "library CRUD corrupt json recovers from .bak, not throw");
      Check(bad.RecoveredFromBackup(),
            "library CRUD recovery is reported to the caller");
      Check(bad.Size() > 0,
            "library CRUD recovered entries survive the corrupt main file");
    }

    // With no usable .bak either, the load must still fail loudly rather than
    // silently presenting an empty library as if the user had no videos.
    {
      std::filesystem::remove(json);
      std::filesystem::path bak = json;
      bak += L".bak";
      std::filesystem::remove(bak);
      WriteBytes(json, "{ still not json !!!");
      k6wp::LibraryManager bad(json);
      bool threw = false;
      try {
        bad.Load();
      } catch (const k6wp::LibraryError&) {
        threw = true;
      }
      Check(threw, "library CRUD corrupt json with no .bak throws LibraryError");
    }

  // MED-18: library.json larger than kMaxConfigBytes (1 MiB) is rejected
  // BEFORE reading — Load throws ConfigError whose message contains
  // "exceeds maximum size".
  {
    const auto big = dir / "big_library.json";
    WriteBytes(big, std::string(2 * 1024 * 1024, 'x'));
    k6wp::LibraryManager bigm(big);
    try {
      bigm.Load();
      Check(false, "library CRUD oversized json throws (got success)");
    } catch (const k6wp::ConfigError& e) {
      Check(std::string(e.what()).find("exceeds maximum size") !=
                std::string::npos,
            "library CRUD oversized json throws ConfigError (exceeds maximum "
            "size)");
    } catch (...) {
      Check(false, "library CRUD oversized json throws ConfigError (wrong type)");
    }
  }

  // Clear empties the index and persists the empty state. The live manager
  // still holds its in-memory rows (the corrupt bytes above only live on
  // disk), so re-adding then clearing exercises Save over the corrupt file.
  {
    WriteBytes(dst_b, "fake video b again");
    m.Add(MakeEntry(dir / "orig_b.mp4", dst_b, 23));
    m.Clear();
    Check(m.Size() == 0, "library CRUD clear empties index");
    k6wp::LibraryManager m4(json);
    m4.Load();
    Check(m4.Size() == 0, "library CRUD clear persists empty index");
  }

  std::filesystem::remove_all(dir, ec);
}

// ReferenceInPlace (HIGH-3 A): import-time validation that references a
// video in place — src == dst == absolute source path, empty thumb, probed
// metadata deferred to Add(). Rejects missing files and directories, and
// never blocks (no ffprobe/thumb work).
void TestReferenceInPlace() {
  auto dir = std::filesystem::temp_directory_path() / "k6wp_libref_test";
  std::error_code ec;
  std::filesystem::remove_all(dir, ec);
  std::filesystem::create_directories(dir, ec);

  const auto video = dir / "clip.mp4";
  WriteBytes(video, "fake video bytes for reference-in-place test");

  // Valid reference: src == dst, absolute, empty thumb, deferred dims.
  k6wp::LibraryEntry ref;
  bool ref_ok = false;
  try {
    ref = k6wp::LibraryManager::ReferenceInPlace(video);
    ref_ok = true;
  } catch (const k6wp::LibraryError&) {
    ref_ok = false;
  }
  Check(ref_ok, "reference accepts a valid file (no throw)");
  Check(ref_ok && ref.src == ref.dst,
        "reference src == dst (in-place identity)");
  Check(ref_ok && ref.src.is_absolute(), "reference src is absolute");
  Check(ref_ok && ref.src == video, "reference src equals the source path");
  Check(ref_ok && ref.thumb.empty(), "reference thumb stays empty");
  Check(ref_ok && ref.width == 0 && ref.height == 0,
        "reference defers dimensions to Add()");

  // Missing file and directory rejection.
  bool threw_missing = false;
  try {
    (void)k6wp::LibraryManager::ReferenceInPlace(dir / "no_such.mp4");
  } catch (const k6wp::LibraryError&) {
    threw_missing = true;
  }
  Check(threw_missing, "reference missing file throws LibraryError");

  bool threw_dir = false;
  try {
    (void)k6wp::LibraryManager::ReferenceInPlace(dir);
  } catch (const k6wp::LibraryError&) {
    threw_dir = true;
  }
  Check(threw_dir, "reference directory throws LibraryError");

  // Non-blocking: 5 back-to-back references each return in <100ms (the old
  // synchronous thumbnail regression — no ffprobe/thumb work happens here).
  for (int i = 0; i < 5; ++i) {
    const auto t0 = std::chrono::steady_clock::now();
    k6wp::LibraryEntry e;
    bool ok = false;
    try {
      e = k6wp::LibraryManager::ReferenceInPlace(video);
      ok = true;
    } catch (const k6wp::LibraryError&) {
      ok = false;
    }
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - t0)
                        .count();
    Check(ok && ms < 100 && e.src == e.dst && e.thumb.empty(),
          "reference call returns in <100ms without blocking");
  }

  std::filesystem::remove_all(dir, ec);
}

// Shared-manager contract (library_manager.hpp: ONE LibraryManager per
// Studio process, shared by reference): mutations made through the shared
// instance are immediately visible to Size()/ListItems() on that same
// instance — no manual Load() needed to paper over staleness.
void TestSharedManagerVisibility() {
  auto dir = std::filesystem::temp_directory_path() / "k6wp_libshared_test";
  std::error_code ec;
  std::filesystem::remove_all(dir, ec);
  std::filesystem::create_directories(dir, ec);
  const auto json = dir / "library.json";

  k6wp::LibraryManager m(json);
  m.Load();  // missing file => empty library (first run)

  const auto dst = dir / "shared.mp4";
  WriteBytes(dst, "fake shared video");

  // Add is visible without a manual Load.
  m.Add(MakeEntry(dir / "orig_shared.mp4", dst, 23));
  Check(m.Size() == 1, "shared add visible without manual Load (size)");
  {
    const auto items = m.ListItems();
    Check(items.size() == 1 && items[0].dst == dst && !items[0].broken,
          "shared add visible without manual Load (list)");
  }

  // Remove is visible without a manual Load.
  Check(m.Remove(dst, false), "shared remove returns true");
  Check(m.Size() == 0, "shared remove visible without manual Load (size)");
  Check(m.ListItems().empty(),
        "shared remove visible without manual Load (list)");

  // Clear is visible without a manual Load.
  m.Add(MakeEntry(dir / "orig_shared.mp4", dst, 23));
  Check(m.Size() == 1, "shared re-add before clear");
  m.Clear();
  Check(m.Size() == 0, "shared clear visible without manual Load (size)");
  Check(m.ListItems().empty(),
        "shared clear visible without manual Load (list)");

  std::filesystem::remove_all(dir, ec);
}

}  // namespace

int main() {
  TestLibraryCrud();
  TestReferenceInPlace();
  TestSharedManagerVisibility();

  std::printf("%d checks, %d failures\n", g_checks, g_failures);
  return g_failures == 0 ? 0 : 1;
}
