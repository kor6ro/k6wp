// still_test.cpp - unit tests for the per-user still contract
// (shared/still.cpp) and the legacy lock-screen cleanup policy
// (shared/legacy_lockscreen_policy.cpp).
//
// All headless and side-effect free:
//   * paths: StillDir / status / original stores under LOCALAPPDATA, and the
//     J1 predicate IsStillPath (case-insensitive, separator-tolerant);
//   * the J8 refresh policy (manual always, rotation throttled + changed);
//   * the "WxH" parser used by --res and the helper;
//   * the original wallpaper / original lock-screen / status JSON stores;
//   * the helper spawn contract through the shared test seam (no process is
//     ever started here);
//   * the legacy HKLM policy cleanup classification + its seam.
//
// No external test framework: plain Check() counter; exit 0 = pass.
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include "legacy_lockscreen_policy.hpp"
#include "still.hpp"
#include "studio_settings.hpp"

namespace {

int g_checks = 0;
int g_failures = 0;

void Check(bool cond, const char* name) {
  ++g_checks;
  if (cond) {
    std::printf("[PASS] %s\n", name);
  } else {
    ++g_failures;
    std::printf("[FAIL] %s\n", name);
  }
}

std::filesystem::path MakeScratch(const wchar_t* name, int n) {
  std::error_code ec;
  const std::filesystem::path base =
      std::filesystem::temp_directory_path(ec) / "k6wp-still-test";
  const std::filesystem::path dir = base / (std::wstring(name) +
                                            std::to_wstring(n));
  std::filesystem::remove_all(dir, ec);
  std::filesystem::create_directories(dir, ec);
  return dir;
}

void PointLocalAppData(const std::filesystem::path& dir) {
  (void)SetEnvironmentVariableW(L"LOCALAPPDATA", dir.wstring().c_str());
}

std::wstring Upper(std::wstring text) {
  if (!text.empty()) _wcsupr_s(text.data(), text.size() + 1);
  return text;
}

// --- paths -------------------------------------------------------------------

void TestPaths() {
  const std::filesystem::path scratch = MakeScratch(L"paths", 0);
  PointLocalAppData(scratch);

  const std::filesystem::path dir = k6wp::StillDir();
  Check(dir == scratch / L"K6WP" / L"stills", "paths: StillDir under LOCALAPPDATA");
  Check(k6wp::StillStatusPath() == scratch / L"K6WP" / L"still_status.json",
        "paths: status file under LOCALAPPDATA\\K6WP");
  Check(k6wp::OriginalWallpaperStorePath() ==
            scratch / L"K6WP" / L"original_wallpaper.json",
        "paths: original wallpaper store");
  Check(k6wp::OriginalLockscreenStorePath() ==
            scratch / L"K6WP" / L"original_lockscreen.json",
        "paths: original lock-screen store");
  Check(k6wp::OriginalLockscreenImagePath() ==
            scratch / L"K6WP" / L"original_lockscreen.jpg",
        "paths: original lock-screen image");

  // J1 predicate: our own stills are recognised, everything else is not.
  Check(k6wp::IsStillPath(dir / L"still_1.jpg"),
        "IsStillPath: direct child true");
  Check(k6wp::IsStillPath(dir / L"nested" / L"still_2.jpg"),
        "IsStillPath: nested child true");
  Check(k6wp::IsStillPath(std::filesystem::path(dir.wstring() + L"\\x.jpg")),
        "IsStillPath: trailing-separator dir true");
  Check(k6wp::IsStillPath(std::filesystem::path(Upper(dir.wstring()) +
                                               L"\\STILL_3.JPG")),
        "IsStillPath: case-insensitive true");
  Check(!k6wp::IsStillPath(dir), "IsStillPath: the dir itself is not a still");
  Check(!k6wp::IsStillPath(std::filesystem::path()),
        "IsStillPath: empty path false");
  Check(!k6wp::IsStillPath(dir.parent_path() / L"still_1.jpg"),
        "IsStillPath: K6WP root is not the stills dir");
  Check(!k6wp::IsStillPath(std::filesystem::path(dir.wstring() + L"2\\x.jpg")),
        "IsStillPath: sibling prefix without separator false");
  Check(!k6wp::IsStillPath(L"C:\\Windows\\Web\\Wallpaper\\x.jpg"),
        "IsStillPath: an unrelated wallpaper false");
}

// --- pure helpers -------------------------------------------------------------

void TestParseRes() {
  int w = 0;
  int h = 0;
  Check(k6wp::ParseStillRes(L"1920x1080", w, h) && w == 1920 && h == 1080,
        "ParseStillRes: 1920x1080");
  Check(k6wp::ParseStillRes(L"3840X2160", w, h) && w == 3840 && h == 2160,
        "ParseStillRes: uppercase X accepted");
  Check(k6wp::ParseStillRes(L"1280x720", w, h) && w == 1280 && h == 720,
        "ParseStillRes: 1280x720");
  Check(!k6wp::ParseStillRes(L"1920", w, h), "ParseStillRes: missing x false");
  Check(!k6wp::ParseStillRes(L"x1080", w, h), "ParseStillRes: missing width false");
  Check(!k6wp::ParseStillRes(L"1920x", w, h), "ParseStillRes: missing height false");
  Check(!k6wp::ParseStillRes(L"0x1080", w, h), "ParseStillRes: zero width false");
  Check(!k6wp::ParseStillRes(L"1920x0", w, h), "ParseStillRes: zero height false");
  Check(!k6wp::ParseStillRes(L"-1920x1080", w, h),
        "ParseStillRes: negative width false");
  Check(!k6wp::ParseStillRes(L"abc", w, h), "ParseStillRes: garbage false");
  Check(!k6wp::ParseStillRes(L"", w, h), "ParseStillRes: empty false");
  Check(w == 0 && h == 0, "ParseStillRes: failure reports 0x0");
}

void TestRefreshPolicy() {
  using k6wp::StillRefreshAllowed;
  // Manual always refreshes, even for the same file and with no time passed.
  Check(StillRefreshAllowed(true, false, 0, 5000),
        "refresh: manual with a new file");
  Check(StillRefreshAllowed(true, true, 0, 5000),
        "refresh: manual with the same file");
  Check(StillRefreshAllowed(true, true, 0, 0),
        "refresh: manual ignores the throttle");
  // Rotation: same file is never re-rendered.
  Check(!StillRefreshAllowed(false, true, 999999, 5000),
        "refresh: rotation with the same file skipped");
  // Rotation: throttled by the minimum interval.
  Check(!StillRefreshAllowed(false, false, 0, 5000),
        "refresh: rotation at t=0 throttled");
  Check(!StillRefreshAllowed(false, false, 4999, 5000),
        "refresh: rotation just below the interval throttled");
  Check(StillRefreshAllowed(false, false, 5000, 5000),
        "refresh: rotation at the interval allowed");
  Check(StillRefreshAllowed(false, false, 6000, 5000),
        "refresh: rotation past the interval allowed");
  Check(StillRefreshAllowed(false, false, 0, 0),
        "refresh: zero interval allows a changed rotation");
}

// --- stores -------------------------------------------------------------------

void TestOriginalWallpaperStore() {
  const std::filesystem::path scratch = MakeScratch(L"origwp", 0);
  PointLocalAppData(scratch);
  std::error_code ec;
  std::filesystem::remove(k6wp::OriginalWallpaperStorePath(), ec);

  Check(k6wp::ReadOriginalWallpaper().empty(),
        "original wallpaper: missing store reads empty");
  const std::wstring path = L"C:\\Users\\B\u00fcro\\wall paper.jpg";
  Check(k6wp::WriteOriginalWallpaper(path), "original wallpaper: write");
  Check(k6wp::ReadOriginalWallpaper() == path,
        "original wallpaper: unicode round-trip");
  Check(!k6wp::WriteOriginalWallpaper(L""),
        "original wallpaper: empty write refused");
  Check(k6wp::ReadOriginalWallpaper() == path,
        "original wallpaper: refused write keeps the store");
}

void TestStillStatusStore() {
  const std::filesystem::path scratch = MakeScratch(L"status", 0);
  PointLocalAppData(scratch);
  std::error_code ec;
  std::filesystem::remove(k6wp::StillStatusPath(), ec);

  k6wp::StillStatus missing;
  Check(!k6wp::ReadStillStatus(missing),
        "status: missing file reads false");
  Check(!k6wp::StillStatusFromJson("not json", missing),
        "status: corrupt JSON rejected");
  Check(!k6wp::StillStatusFromJson("[]", missing),
        "status: non-object rejected");
  k6wp::StillStatus partial;
  Check(k6wp::StillStatusFromJson("{\"version\": 1}", partial) &&
            partial.ts == 0 && !partial.ok && partial.error.empty(),
        "status: object without fields yields safe defaults");

  k6wp::StillStatus status;
  status.ts = 1700000000;
  status.ok = true;
  status.wallpaper_ok = true;
  status.lockscreen_ok = true;
  status.lockscreen_mode_hint = "picture";
  Check(k6wp::WriteStillStatus(status), "status: write");
  k6wp::StillStatus back;
  Check(k6wp::ReadStillStatus(back), "status: read back");
  Check(back.ts == status.ts && back.ok && back.wallpaper_ok &&
            back.lockscreen_ok && back.error.empty() &&
            back.lockscreen_mode_hint == "picture",
        "status: round-trip keeps every field");

  k6wp::StillStatus failed;
  failed.ts = 1700000001;
  failed.error = "frame extract failed (compressor exit 1)";
  failed.lockscreen_mode_hint = "spotlight";
  const std::string text = k6wp::StillStatusToJson(failed);
  Check(text.find("\"version\":1") != std::string::npos,
        "status: JSON carries the schema version");
  k6wp::StillStatus failed_back;
  Check(k6wp::StillStatusFromJson(text, failed_back) &&
            failed_back.error == failed.error &&
            failed_back.lockscreen_mode_hint == "spotlight",
        "status: error + mode hint round-trip");
}

void TestOriginalLockscreenStore() {
  const std::filesystem::path scratch = MakeScratch(L"origls", 0);
  PointLocalAppData(scratch);
  std::error_code ec;
  std::filesystem::remove(k6wp::OriginalLockscreenStorePath(), ec);
  std::filesystem::remove(k6wp::OriginalLockscreenImagePath(), ec);

  k6wp::OriginalLockscreen missing;
  Check(!k6wp::ReadOriginalLockscreen(missing),
        "original lockscreen: missing store reads false");

  k6wp::OriginalLockscreen state;
  state.have_image = true;
  state.image_path = k6wp::OriginalLockscreenImagePath().wstring();
  state.spotlight_present = true;
  state.spotlight_enabled = 1;
  state.slideshow_present = false;
  state.slideshow_enabled = 0;
  const std::string text = k6wp::OriginalLockscreenToJson(state);
  k6wp::OriginalLockscreen parsed;
  Check(k6wp::OriginalLockscreenFromJson(text, parsed), "original lockscreen: parse");
  Check(parsed.have_image && parsed.spotlight_present &&
            parsed.spotlight_enabled == 1 && !parsed.slideshow_present &&
            parsed.image_path == state.image_path,
        "original lockscreen: round-trip keeps every field");

  k6wp::OriginalLockscreen wrong_version;
  Check(!k6wp::OriginalLockscreenFromJson("{\"version\": 2}", wrong_version),
        "original lockscreen: unknown version rejected");
  Check(!k6wp::OriginalLockscreenFromJson("[]", wrong_version),
        "original lockscreen: non-object rejected");
  Check(!k6wp::OriginalLockscreenFromJson("{oops", wrong_version),
        "original lockscreen: corrupt JSON rejected");

  Check(k6wp::WriteOriginalLockscreen(state), "original lockscreen: write");
  k6wp::OriginalLockscreen read;
  Check(k6wp::ReadOriginalLockscreen(read) && read.have_image &&
            read.spotlight_present,
        "original lockscreen: store round-trip");

  {
    std::ofstream image(k6wp::OriginalLockscreenImagePath(), std::ios::binary);
    image << "jpeg";
  }
  k6wp::ClearOriginalLockscreen();
  Check(!std::filesystem::exists(k6wp::OriginalLockscreenStorePath(), ec) &&
            !std::filesystem::exists(k6wp::OriginalLockscreenImagePath(), ec),
        "original lockscreen: clear removes store + saved image");
}

// --- helper spawn contract ----------------------------------------------------

bool g_spawn_result = true;
std::wstring g_spawn_args;
int g_spawn_calls = 0;

bool StubSpawn(const std::wstring& args) {
  ++g_spawn_calls;
  g_spawn_args = args;
  return g_spawn_result;
}

void WriteSettings(bool wallpaper, bool lockscreen, double offset) {
  k6wp::StudioSettings s = k6wp::DefaultStudioSettings();
  s.wallpaper_sync = wallpaper;
  s.lockscreen_sync = lockscreen;
  s.lockscreen_offset_sec = offset;
  k6wp::SaveStudioSettings(k6wp::DefaultStudioSettingsPath(), s);
}

void TestSpawnContract() {
  const std::filesystem::path scratch = MakeScratch(L"spawn", 0);
  PointLocalAppData(scratch);
  k6wp::SetStillSpawnFn(&StubSpawn);
  g_spawn_calls = 0;
  g_spawn_args.clear();

  // Both toggles off: the Engine may call unconditionally, nothing spawns.
  WriteSettings(false, false, 0.0);
  Check(!k6wp::RequestStillApply(std::filesystem::path(L"C:\\v\\movie.mp4")),
        "spawn: apply refused when both sides are off");
  Check(g_spawn_calls == 0, "spawn: both sides off spawns nothing");

  // Wallpaper only.
  WriteSettings(true, false, 0.0);
  g_spawn_args.clear();
  Check(k6wp::RequestStillApply(std::filesystem::path(L"C:\\v\\movie.mp4")),
        "spawn: apply accepted with wallpaper on");
  Check(g_spawn_args ==
            L" --apply-still --video \"C:\\v\\movie.mp4\"",
        "spawn: apply argument tail matches the helper contract");

  // Lock screen only (preserved legacy opt-in keeps applying).
  WriteSettings(false, true, 0.0);
  g_spawn_args.clear();
  Check(k6wp::RequestStillApply(std::filesystem::path(L"C:\\v\\movie.mp4")),
        "spawn: apply accepted with the lock screen on");
  Check(g_spawn_args.find(L"--apply-still") != std::wstring::npos,
        "spawn: lock-screen-only apply still names --apply-still");

  // A failed spawn is reported (callers treat it as "not started").
  WriteSettings(true, true, 0.0);
  g_spawn_result = false;
  Check(!k6wp::RequestStillApply(std::filesystem::path(L"C:\\v\\movie.mp4")),
        "spawn: a failed CreateProcess reports false");
  g_spawn_result = true;

  // Empty video: no helper call.
  g_spawn_calls = 0;
  Check(!k6wp::RequestStillApply(std::filesystem::path()),
        "spawn: empty video refused");
  Check(g_spawn_calls == 0, "spawn: empty video spawns nothing");

  // Restore selectors.
  g_spawn_args.clear();
  Check(k6wp::RequestStillRestore(true, true), "spawn: restore both");
  Check(g_spawn_args == L" --apply-still --restore",
        "spawn: restore both omits the side selectors");

  g_spawn_args.clear();
  Check(k6wp::RequestStillRestore(true, false), "spawn: restore wallpaper");
  Check(g_spawn_args.find(L"--wallpaper") != std::wstring::npos &&
            g_spawn_args.find(L"--lockscreen") == std::wstring::npos,
        "spawn: wallpaper-only restore names --wallpaper only");

  g_spawn_args.clear();
  Check(k6wp::RequestStillRestore(false, true),
        "spawn: restore lock screen");
  Check(g_spawn_args.find(L"--lockscreen") != std::wstring::npos &&
            g_spawn_args.find(L"--wallpaper") == std::wstring::npos,
        "spawn: lock-screen-only restore names --lockscreen only");

  g_spawn_calls = 0;
  Check(!k6wp::RequestStillRestore(false, false),
        "spawn: restore with no side refused");
  Check(g_spawn_calls == 0, "spawn: empty restore spawns nothing");

  k6wp::SetStillSpawnFn(nullptr);
}

// --- settings read (Engine side, LOW-16) --------------------------------------

void TestSettingsRead() {
  const std::filesystem::path scratch = MakeScratch(L"readsettings", 0);
  PointLocalAppData(scratch);

  k6wp::StillSettings missing = k6wp::ReadStillSettingsCached();
  Check(!missing.readable, "settings: missing file is not readable");
  Check(!missing.wallpaper_sync && !missing.lockscreen_sync,
        "settings: missing file disables both sides");

  WriteSettings(true, false, 2.5);
  k6wp::StillSettings read = k6wp::ReadStillSettingsCached();
  Check(read.readable, "settings: file readable");
  Check(read.wallpaper_sync && !read.lockscreen_sync,
        "settings: the three Engine-visible fields are read");
  Check(read.lockscreen_offset_sec == 2.5, "settings: offset read");

  // Cache key is mtime+size: a rewritten file (different size) is re-read.
  WriteSettings(false, true, 0.0);
  read = k6wp::ReadStillSettingsCached();
  Check(!read.wallpaper_sync && read.lockscreen_sync &&
            read.lockscreen_offset_sec == 0.0,
        "settings: rewrite is re-read (no stale cache)");

  // Corrupt file: both off, not readable, never throws.
  {
    std::ofstream bad(k6wp::DefaultStudioSettingsPath(),
                      std::ios::binary | std::ios::trunc);
    bad << "{not json";
  }
  read = k6wp::ReadStillSettingsCached();
  Check(!read.readable && !read.wallpaper_sync && !read.lockscreen_sync,
        "settings: corrupt file disables both sides");
}

// --- legacy lock-screen policy ------------------------------------------------

int g_cleanup_timeout = 0;
int g_cleanup_calls = 0;
k6wp::LegacyCleanupOutcome g_cleanup_result =
    k6wp::LegacyCleanupOutcome::kCleaned;

k6wp::LegacyCleanupOutcome StubCleanup(int timeout_ms) {
  ++g_cleanup_calls;
  g_cleanup_timeout = timeout_ms;
  return g_cleanup_result;
}

void TestLegacyPolicy() {
  using k6wp::LegacyCleanupOutcome;
  Check(k6wp::ClassifyLegacyCleanup(true, 0) == LegacyCleanupOutcome::kCleaned,
        "legacy: exit 0 -> cleaned");
  Check(k6wp::ClassifyLegacyCleanup(true, 5) == LegacyCleanupOutcome::kDeclined,
        "legacy: exit 5 (UAC declined) -> declined");
  Check(k6wp::ClassifyLegacyCleanup(true, 4) == LegacyCleanupOutcome::kTimedOut,
        "legacy: exit 4 -> timed out");
  Check(k6wp::ClassifyLegacyCleanup(true, 2) ==
            LegacyCleanupOutcome::kSpawnFailed,
        "legacy: exit 2 (bad flag) -> spawn failed");
  Check(k6wp::ClassifyLegacyCleanup(true, 3) ==
            LegacyCleanupOutcome::kSpawnFailed,
        "legacy: exit 3 -> spawn failed");
  Check(k6wp::ClassifyLegacyCleanup(true, 42) ==
            LegacyCleanupOutcome::kWorkerFailed,
        "legacy: unknown exit -> worker failed");
  Check(k6wp::ClassifyLegacyCleanup(false, 0) ==
            LegacyCleanupOutcome::kSpawnFailed,
        "legacy: no exit code -> spawn failed");

  // cleanup_needed: offer the button only for OUR leftovers or a bare image.
  k6wp::LegacyLockscreenState ours;
  ours.policy_present = true;
  ours.policy_ours = true;
  ours.legacy_image_exists = true;
  Check(ours.cleanup_needed(), "legacy: our policy needs cleanup");

  k6wp::LegacyLockscreenState foreign;
  foreign.policy_present = true;
  foreign.policy_foreign = true;
  Check(!foreign.cleanup_needed(),
        "legacy: someone else's policy is never offered");

  k6wp::LegacyLockscreenState image_only;
  image_only.legacy_image_exists = true;
  Check(image_only.cleanup_needed(),
        "legacy: leftover image without a policy needs cleanup");

  k6wp::LegacyLockscreenState dir_only;
  dir_only.legacy_dir_exists = true;
  Check(dir_only.cleanup_needed(),
        "legacy: an empty legacy folder (ACL residue) needs cleanup");

  k6wp::LegacyLockscreenState clean;
  Check(!clean.cleanup_needed(), "legacy: nothing left -> no cleanup");

  // Paths follow PROGRAMDATA.
  const std::filesystem::path scratch = MakeScratch(L"programdata", 0);
  (void)SetEnvironmentVariableW(L"PROGRAMDATA", scratch.wstring().c_str());
  Check(k6wp::LegacyLockscreenDir() == scratch / L"K6WP",
        "legacy: dir is %PROGRAMDATA%\\K6WP");
  Check(k6wp::LegacyLockscreenImagePath() ==
            scratch / L"K6WP" / L"lockscreen.jpg",
        "legacy: image path is %PROGRAMDATA%\\K6WP\\lockscreen.jpg");

  // Seam: the Studio's button waits with its own budget and maps the result.
  k6wp::SetLegacyCleanupFn(&StubCleanup);
  g_cleanup_calls = 0;
  g_cleanup_result = LegacyCleanupOutcome::kDeclined;
  Check(k6wp::RunLegacyCleanup(150000) == LegacyCleanupOutcome::kDeclined,
        "legacy: the seam result reaches the caller");
  Check(g_cleanup_calls == 1 && g_cleanup_timeout == 150000,
        "legacy: the seam receives the Studio's timeout");
  g_cleanup_result = LegacyCleanupOutcome::kCleaned;
  Check(k6wp::RunLegacyCleanup(10) == LegacyCleanupOutcome::kCleaned,
        "legacy: a cleaned result reaches the caller");
  k6wp::SetLegacyCleanupFn(nullptr);
}

}  // namespace

int main() {
  const std::wstring old_local = [&]() {
    wchar_t buf[MAX_PATH] = {};
    const DWORD n = GetEnvironmentVariableW(L"LOCALAPPDATA", buf, MAX_PATH);
    return (n > 0 && n < MAX_PATH) ? std::wstring(buf) : std::wstring();
  }();
  const std::wstring old_programdata = [&]() {
    wchar_t buf[MAX_PATH] = {};
    const DWORD n = GetEnvironmentVariableW(L"PROGRAMDATA", buf, MAX_PATH);
    return (n > 0 && n < MAX_PATH) ? std::wstring(buf) : std::wstring();
  }();

  TestPaths();
  TestParseRes();
  TestRefreshPolicy();
  TestOriginalWallpaperStore();
  TestStillStatusStore();
  TestOriginalLockscreenStore();
  TestSpawnContract();
  TestSettingsRead();
  TestLegacyPolicy();

  // Restore the machine's environment so a later suite is unaffected.
  (void)SetEnvironmentVariableW(L"LOCALAPPDATA",
                                old_local.empty() ? nullptr : old_local.c_str());
  (void)SetEnvironmentVariableW(L"PROGRAMDATA", old_programdata.empty()
                                                    ? nullptr
                                                    : old_programdata.c_str());
  std::error_code ec;
  std::filesystem::remove_all(
      std::filesystem::temp_directory_path(ec) / "k6wp-still-test", ec);

  std::printf("%d checks, %d failures\n", g_checks, g_failures);
  return g_failures == 0 ? 0 : 1;
}
