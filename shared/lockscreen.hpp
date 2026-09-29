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

// %PROGRAMDATA%\K6WP\lockscreen_policy_backup.json — the launcher's backup of
// the previous LockScreenImage policy value. Never throws.
std::filesystem::path LockscreenBackupPath();

// True when the user opted in via studio_settings.json (lockscreen_sync).
// Best-effort read; false on missing/corrupt settings. Never throws.
bool IsLockscreenSyncEnabled() noexcept;

// Seek offset (seconds) for the frame extract, from studio_settings.json
// (lockscreen_offset_sec, default 1.0). Never throws.
double LockscreenOffsetSec() noexcept;

// Fire-and-forget frame extract, called when the wallpaper video changes and
// IsLockscreenSyncEnabled() is true:
//
//   compressor.exe --lockframe --in <video> --out <lockscreen.jpg>
//                  --offset-s <N>
//
// Debounced: a call within 5s of the previous fire is skipped (shared atomic
// timestamp, so rapid Apply/video-switch bursts extract at most one frame).
// The child runs DETACHED_PROCESS | BELOW_NORMAL_PRIORITY_CLASS and is never
// waited on, so the engine render loop and the Studio GUI thread are never
// blocked. Any failure (sync off, missing compressor, spawn error) only emits
// an OutputDebugStringW line — never throws, never crashes the caller.
void FireLockscreenSyncAsync(const std::filesystem::path& video_path) noexcept;

// Compatibility aliases (same behavior, alternate names used across the
// codebase). Inline so they cost no extra translation unit.
inline std::filesystem::path LockscreenImagePath() {
  return LockscreenJpgPath();
}
inline void RequestLockscreenSync(const std::wstring& video_path) noexcept {
  FireLockscreenSyncAsync(std::filesystem::path(video_path));
}

}  // namespace k6wp
