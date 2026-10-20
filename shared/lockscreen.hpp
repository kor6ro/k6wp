#pragma once

// Bagian B — lock screen sync helpers (static frame only, B5).
//
// Contract:
//   - The lockscreen image is a STATIC JPEG at %PROGRAMDATA%\K6WP\lockscreen.jpg,
//     produced by `compressor.exe --lockframe` (single ffmpeg frame extract).
//     Nothing here renders live content to Winlogon, and nothing writes
//     outside the K6WP folder.
//   - The elevated registry work (HKLM Policies\...\Personalization
//     LockScreenImage + ACL + backup JSON) lives in the launcher
//     (K6WP.exe --elevate-lockscreen on|off). This header only resolves the
//     agreed paths and fires the fire-and-forget frame extract.
//   - Header stays windows.h-free (same pattern as autostart / config_schema):
//     Win32 lives only in lockscreen.cpp, so Qt Studio can include this
//     header with no windows.h pollution.

#include <filesystem>

namespace k6wp {

// %PROGRAMDATA%\K6WP (falls back to C:\ProgramData\K6WP when PROGRAMDATA is
// unset). Never throws (error_code overloads; temp-dir fallback).
std::filesystem::path LockscreenDir();

// %PROGRAMDATA%\K6WP\lockscreen.jpg — the single static lockscreen frame.
std::filesystem::path LockscreenJpgPath();

// True when the user opted in via studio_settings.json (lockscreen_sync).
// Best-effort read; false on missing/corrupt settings. Never throws.
//
// The two settings the Engine may read (LOW-16, dev-contracts.md §3) are
// cached together and re-read only when the file's mtime/size change, so a
// single event parses studio_settings.json at most ONCE.
bool IsLockscreenSyncEnabled() noexcept;

// Seek offset (seconds) for the frame extract, from studio_settings.json
// (lockscreen_offset_sec, default 1.0). Never throws. Cached together with
// IsLockscreenSyncEnabled() (one parse, not two).
double LockscreenOffsetSec() noexcept;

// Why a lock-screen refresh was requested. The Engine maps its events onto
// these; the shared helper decides what to do with them.
enum class LockscreenTrigger {
  kVideoChanged,  // the user picked a different wallpaper
  kRotation,      // the playlist rotated to another video
  kSessionLock,   // the session was locked (Win+L)
  kSuspend,       // the machine is going to sleep
  kShutdown,      // the session is ending (shutdown / logoff)
  kMonitorOn,     // the screen came back on
};

// Pure policy: does this trigger refresh the lock screen at all? (The
// helper is a no-op anyway when lockscreen_sync is OFF.) Never throws.
bool ShouldRefreshLockscreen(LockscreenTrigger trigger);

// Pure policy: must the refresh FINISH before the trigger's event is over?
//
// Only the shutdown path: Windows gives the app a short window between
// WM_QUERYENDSESSION and WM_ENDSESSION, and the frame has to land before
// the session ends - otherwise the lock screen shows the PREVIOUS picture.
// Every other trigger stays fire-and-forget so the render loop (and the
// suspend path) is never blocked.
bool LockscreenTriggerWaits(LockscreenTrigger trigger);

// Pure policy: does this trigger bypass the debounce?
//
// The debounce exists to collapse a burst of video switches into one frame
// extract. A session lock / suspend / shutdown must never be swallowed by
// the debounce of an earlier refresh: the picture has to be current.
bool LockscreenTriggerBypassesDebounce(LockscreenTrigger trigger);

// Fire-and-forget frame extract, called when the wallpaper video changes and
// IsLockscreenSyncEnabled() is true:
//
//   compressor.exe --lockframe --in <video> --out <lockscreen.jpg>
//                  --offset-s <N>
//
// Debounced: a call within 5s of the previous fire for the SAME file
// (same path AND same mtime/size) is skipped. A DIFFERENT file - or the
// same path with different content - always proceeds. The child runs
// DETACHED_PROCESS | BELOW_NORMAL_PRIORITY_CLASS and is never waited on,
// so the engine render loop and the Studio GUI thread are never blocked.
// Any failure (sync off, missing compressor, spawn error) only emits
// an OutputDebugStringW line — never throws, never crashes the caller.
void FireLockscreenSyncAsync(const std::filesystem::path& video_path) noexcept;

// Trigger-aware variant. Same contract as FireLockscreenSyncAsync, plus:
//
//   * a trigger that does not refresh (ShouldRefreshLockscreen == false)
//     does nothing at all;
//   * a trigger that bypasses the debounce (session lock / suspend /
//     shutdown) always extracts a fresh frame;
//   * a trigger that waits (shutdown) waits for the child to finish, with a
//     bounded budget, so the frame is on disk before the session ends.
void FireLockscreenSyncForTrigger(const std::filesystem::path& video_path,
                                  LockscreenTrigger trigger) noexcept;

// --- the live frame ----------------------------------------------------------
//
// The frame that is on screen RIGHT NOW is a better lock-screen picture than
// a frame extracted from the start of the video file, so the Engine
// registers a live-frame provider (the mpv capture) and this helper prefers
// it. Without a provider (Studio, tests) the file-based extract is used, so
// the feature works either way.
//
// The live capture is ASYNCHRONOUS: mpv's screenshot-to-file is handled by
// the VO thread and the file lands a moment later. Producing the frame
// therefore needs a bounded wait, which is why the Engine does it on a
// background thread (engine/src/lockscreen_refresher.hpp).

// The live-frame seam: writes a JPEG of the picture on screen to `path`
// (UTF-8) and returns true. `path` is a TEMP file - the helper moves it
// into place atomically once it lands. Returning false (or not registering
// a provider at all) falls back to the file-based extract.
using LockscreenLiveFrameFn = bool (*)(const std::string& path);

// Registers the live-frame provider. nullptr (the default) removes it.
// Thread-safe.
void SetLockscreenLiveFrameFn(LockscreenLiveFrameFn fn);

// Where the produced frame came from. The Engine logs it, so "the lock
// screen was refreshed" can be told apart from "the live capture failed and
// the file-based extract was used instead".
enum class LockscreenFrameSource {
  kNone,       // nothing was produced (sync off, no video, debounced)
  kLiveFrame,  // the picture that was on screen right now
  kFileExtract // a frame extracted from the video file
};

// Produces the lock-screen frame for `video_path`:
//
//   1. the LIVE frame, when a provider is registered AND the file lands
//      within `wait_ms`;
//   2. otherwise the file-based extract (`compressor --lockframe`).
//
// `trigger` decides the debounce behaviour (a session lock / suspend /
// shutdown bypasses it; a video change / rotation is throttled by it).
// `wait_ms` bounds the wait for the live frame; 0 means "do not wait", so
// the live frame is skipped and the file-based extract is used instead.
//
// Never throws. Returns where the frame came from.
LockscreenFrameSource ProduceLockscreenFrame(
    const std::filesystem::path& video_path, LockscreenTrigger trigger,
    int wait_ms) noexcept;

}  // namespace k6wp
