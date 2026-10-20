#pragma once

// Still (first frame) wallpaper + lock-screen sync - the shared contract.
//
// WHAT THE FEATURE IS
// The FIRST frame of the selected video is rendered by
// `compressor.exe --lockframe` into a unique still file under
// %LOCALAPPDATA%\K6WP\stills, and that file is then applied as
//   * the real Windows desktop wallpaper (SystemParametersInfoW
//     SPI_SETDESKWALLPAPER), and
//   * the Windows lock-screen image (WinRT LockScreen::SetImageFileAsync).
// Both are PER-USER operations: no HKLM policy, no ACL grant, no
// administrator rights, no UAC. The previous mechanism (an elevated
// LockScreenImage policy pointing at %PROGRAMDATA%\K6WP\lockscreen.jpg) is
// gone; `K6WP.exe --elevate-lockscreen off` is kept only to clean up what it
// left behind.
//
// WHO DOES WHAT
//   * the ENGINE decides WHEN a still is wanted (boot with a video, a manual
//     video pick, a throttled playlist rotation) and spawns exactly ONE
//     detached, below-normal child (K6WP.exe --apply-still ...). It never
//     waits and never touches the lock screen itself.
//   * the HELPER (launcher/still_apply.cpp) does the OS work and writes
//     %LOCALAPPDATA%\K6WP\still_status.json.
//   * STUDIO flips the two toggles (writing studio_settings.json and spawning
//     the same helper) and READS the status file. The Engine never reads it.
//
// Header stays windows.h-free (same pattern as config_schema): the Win32
// calls live in still.cpp, so Qt Studio can include this with no windows.h
// pollution.

#include <filesystem>
#include <string>

namespace k6wp {

// --- paths -------------------------------------------------------------------
// %LOCALAPPDATA%\K6WP\stills (USERPROFILE fallback, then temp dir). Never
// throws.
std::filesystem::path StillDir();

// %LOCALAPPDATA%\K6WP\still_status.json - written by the helper, read by
// Studio ONLY (the Engine never reads it).
std::filesystem::path StillStatusPath();

// %LOCALAPPDATA%\K6WP\original_wallpaper.json - the wallpaper that was on the
// desktop before the first still was applied. PERSISTENT by design: the
// restore can happen long after the process that captured it (tray quit,
// toggle off, uninstall), and a shutdown must not lose it.
std::filesystem::path OriginalWallpaperStorePath();

// %LOCALAPPDATA%\K6WP\original_lockscreen.json - the previous lock-screen
// mode (spotlight / slideshow registry values) plus whether a previous image
// was saved.
std::filesystem::path OriginalLockscreenStorePath();

// %LOCALAPPDATA%\K6WP\original_lockscreen.jpg - the previous lock-screen
// image, saved before the first SetImageFileAsync so it can be put back.
std::filesystem::path OriginalLockscreenImagePath();

// --- settings the Engine may read (LOW-16, docs/dev-contracts.md §3) ---------
// The Engine observes exactly THREE studio_settings.json fields:
// `lockscreen_sync`, `lockscreen_offset_sec` and `wallpaper_sync`. Every other
// field is Studio-private; adding a fourth Engine read requires a §3 update
// plus a studio_logic_test §6 case.
struct StillSettings {
  // Apply the still as the desktop wallpaper. Default TRUE only for a fresh
  // install (no studio_settings.json yet).
  bool wallpaper_sync = true;
  // Apply the still as the lock-screen image. Preserved from the previous
  // mechanism: a user who had it ON keeps it ON.
  bool lockscreen_sync = false;
  // Seek offset (seconds) for the frame extract. Default 0.0 = the FIRST
  // frame; files written by the old mechanism (schema <= 4) that still carry
  // the old 1.0 default migrate to 0.0 (v5 migration, only when the value is
  // exactly 1.0).
  double lockscreen_offset_sec = 0.0;
  // False when studio_settings.json could not be read at all (missing,
  // corrupt). Callers then treat BOTH sync flags as off.
  bool readable = false;
};

// Cached read: studio_settings.json is re-parsed only when its mtime/size
// change, so one event costs at most ONE parse. Never throws.
StillSettings ReadStillSettingsCached() noexcept;

// --- pure helpers (unit-tested) ---------------------------------------------
// True when `path` lives inside StillDir(). Case-insensitive, tolerant of
// '/' vs '\' and of a trailing separator. Used to keep our own stills out of
// the "original wallpaper" store (J1) and to recognise our own wallpaper.
bool IsStillPath(const std::filesystem::path& path) noexcept;

// Pure refresh policy (J8): a manual pick (boot with a video / user picked a
// video / Studio applied) always refreshes; a playlist rotation refreshes at
// most every `min_interval_ms` AND only when the video actually changed
// (nothing to re-render otherwise).
bool StillRefreshAllowed(bool manual, bool same_file,
                         unsigned long long elapsed_ms,
                         unsigned long long min_interval_ms) noexcept;

// Parses "1920x1080" / "1920X1080" into w/h. Accepts only positive values.
bool ParseStillRes(const std::wstring& text, int& width, int& height) noexcept;

// --- original wallpaper store ------------------------------------------------
// The stored path ("" when the store does not exist / cannot be read).
// Best-effort: never throws.
std::wstring ReadOriginalWallpaper() noexcept;

// Publishes the store atomically (config_schema's AtomicWriteJson).
// Best-effort: returns false on failure, never throws.
bool WriteOriginalWallpaper(const std::wstring& path) noexcept;

// Reads the LIVE desktop wallpaper and stores it - unless the live wallpaper
// is one of our own stills (J1: our own writes must never become the
// "original"), in which case the store is left untouched. Returns true when
// the store now holds a new value.
bool CaptureOriginalWallpaper() noexcept;

// Puts the stored wallpaper back and clears the store on success. Returns
// true when the desktop now shows the stored wallpaper. A missing store is a
// no-op returning false.
bool RestoreOriginalWallpaper() noexcept;

// --- original lock-screen store ----------------------------------------------
// The lock-screen state as it was BEFORE the first still was applied.
struct OriginalLockscreen {
  bool have_image = false;      // a previous image was saved to disk
  std::wstring image_path;      // OriginalLockscreenImagePath() when saved
  bool spotlight_present = false;  // HKCU RotatingLockScreenEnabled existed
  unsigned long spotlight_enabled = 0;
  bool slideshow_present = false;  // HKCU SlideshowEnabled existed
  unsigned long slideshow_enabled = 0;
};

// Pure JSON round-trip (unit-tested; the helper is the only writer/reader).
std::string OriginalLockscreenToJson(const OriginalLockscreen& state);
bool OriginalLockscreenFromJson(const std::string& text,
                                OriginalLockscreen& out) noexcept;

bool ReadOriginalLockscreen(OriginalLockscreen& out) noexcept;
bool WriteOriginalLockscreen(const OriginalLockscreen& state) noexcept;
// Removes the store + the saved image. Best-effort.
void ClearOriginalLockscreen() noexcept;

// --- still status (helper -> Studio) -----------------------------------------
struct StillStatus {
  long long ts = 0;              // Unix seconds of the last attempt
  bool ok = false;               // every REQUESTED side succeeded
  bool wallpaper_ok = false;
  bool lockscreen_ok = false;
  std::string error;             // "" when ok
  // What the lock screen is in AFTER the apply: "picture" (our image is the
  // active picture), "spotlight" / "slideshow" (a rotating mode may still
  // override it), "" (nothing was requested).
  std::string lockscreen_mode_hint;
};

std::string StillStatusToJson(const StillStatus& status);
bool StillStatusFromJson(const std::string& text, StillStatus& out) noexcept;
bool ReadStillStatus(StillStatus& out) noexcept;
bool WriteStillStatus(const StillStatus& status) noexcept;

// --- spawning the helper ------------------------------------------------------
// Spawns the sibling K6WP.exe as
//   K6WP.exe --apply-still --video <video>       (apply)
//   K6WP.exe --apply-still --restore [...]       (restore)
// with DETACHED_PROCESS | BELOW_NORMAL_PRIORITY_CLASS. The caller never
// waits; failures only emit an OutputDebugStringW line. Both helpers no-op
// (and return false) when the corresponding setting is off, so the Engine can
// call RequestStillApply() unconditionally.
bool RequestStillApply(const std::filesystem::path& video) noexcept;
bool RequestStillRestore(bool wallpaper, bool lockscreen) noexcept;

// TEST SEAM: replaces the helper spawn. The stub receives the argument tail
// (" --apply-still ...") and reports whether it "started". Production never
// calls this; the Studio tests use it to assert that a toggle really asks the
// helper to apply/restore without spawning a process.
using StillSpawnFn = bool (*)(const std::wstring& args);
void SetStillSpawnFn(StillSpawnFn fn) noexcept;

}  // namespace k6wp
