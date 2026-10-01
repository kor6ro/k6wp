#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include "lockscreen.hpp"

#include <atomic>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>
#include <system_error>
#include <vector>

#include "studio_settings.hpp"

namespace k6wp {
namespace {

// Debounce window for repeated syncs of the SAME video. A DIFFERENT video is
// allowed through immediately: quick-switching wallpapers must not leave the
// lockscreen showing the previous frame with no later event to correct it.
constexpr ULONGLONG kDebounceMs = 5000;
std::mutex g_fire_mutex;
std::wstring g_last_fire_path;
ULONGLONG g_last_fire_ms = 0;  // both guarded by g_fire_mutex

void DebugLog(const wchar_t* fmt, DWORD code) {
  wchar_t line[512] = {};
  _snwprintf_s(line, sizeof(line) / sizeof(line[0]), _TRUNCATE, fmt, code);
  OutputDebugStringW(line);
}

std::filesystem::path ProgramDataBase() {
  wchar_t buf[MAX_PATH] = {};
  const DWORD n = GetEnvironmentVariableW(L"PROGRAMDATA", buf, MAX_PATH);
  if (n > 0 && n < MAX_PATH) return std::filesystem::path(buf);
  return std::filesystem::path(L"C:\\ProgramData");
}

// Sibling compressor.exe next to the current module (flattened install and
// build/msvc-dev layouts both keep all exes side by side). Empty on failure.
std::filesystem::path SiblingCompressor() {
  wchar_t exe_path[MAX_PATH] = {};
  const DWORD len = GetModuleFileNameW(nullptr, exe_path, MAX_PATH);
  if (len == 0 || len >= MAX_PATH) return {};
  std::error_code ec;
  const std::filesystem::path cand =
      std::filesystem::path(exe_path).parent_path() / L"compressor.exe";
  if (std::filesystem::exists(cand, ec)) return cand;
  return {};
}

std::wstring FormatOffset(double v) {
  // One decimal is plenty for a seek offset; avoids locale issues by
  // formatting manually (never uses swprintf %f locale paths).
  char buf[32] = {};
  _snprintf_s(buf, sizeof(buf), _TRUNCATE, "%.1f", v);
  return std::wstring(buf, buf + strlen(buf));
}

}  // namespace

std::filesystem::path LockscreenDir() {
  std::error_code ec;
  try {
    return ProgramDataBase() / L"K6WP";
  } catch (...) {
    return std::filesystem::temp_directory_path(ec) / L"K6WP";
  }
}

std::filesystem::path LockscreenJpgPath() { return LockscreenDir() / L"lockscreen.jpg"; }

bool IsLockscreenSyncEnabled() noexcept {
  // LOW-16 ENGINE-READ CONTRACT (dev-contracts.md §3): the Engine may
  // observe exactly two studio_settings.json fields — `lockscreen_sync`
  // (here) and `lockscreen_offset_sec` (LockscreenOffsetSec below). Every
  // other field is Studio-private; adding a third Engine read requires a
  // §3 update plus a studio_logic_test §6 case.
  try {
    const StudioSettings s = LoadStudioSettings(DefaultStudioSettingsPath());
    return s.lockscreen_sync;
  } catch (...) {
    return false;
  }
}

double LockscreenOffsetSec() noexcept {
  try {
    const StudioSettings s = LoadStudioSettings(DefaultStudioSettingsPath());
    return s.lockscreen_offset_sec < 0.0 ? 0.0 : s.lockscreen_offset_sec;
  } catch (...) {
    return 1.0;
  }
}

void FireLockscreenSyncAsync(const std::filesystem::path& video_path) noexcept {
  try {
    if (video_path.empty()) return;
    if (!IsLockscreenSyncEnabled()) return;

    // Reserve the debounce slot for this path. Same video inside the window is
    // dropped; a different video always proceeds. A reserved slot is released
    // again on any failure below so a failed attempt cannot mute the next 5s.
    {
      std::lock_guard<std::mutex> lock(g_fire_mutex);
      const ULONGLONG now = GetTickCount64();
      if (!g_last_fire_path.empty() && video_path.wstring() == g_last_fire_path &&
          now - g_last_fire_ms < kDebounceMs) {
        return;
      }
      g_last_fire_path = video_path.wstring();
      g_last_fire_ms = now;
    }
    const auto release_slot = [] {
      std::lock_guard<std::mutex> lock(g_fire_mutex);
      g_last_fire_path.clear();
    };

    std::error_code ec;
    if (!std::filesystem::exists(video_path, ec)) {
      release_slot();
      DebugLog(L"K6WP lockscreen: video missing, skipping sync (ec=%lu)",
               static_cast<DWORD>(ec.value()));
      return;
    }

    const std::filesystem::path compressor = SiblingCompressor();
    if (compressor.empty()) {
      release_slot();
      DebugLog(L"K6WP lockscreen: compressor.exe not found, skipping (ec=%lu)",
               GetLastError());
      return;
    }

    // Ensure the output dir exists (best-effort; compressor would fail alone
    // and that failure is contained in the child either way).
    std::filesystem::create_directories(LockscreenDir(), ec);

    const std::wstring args =
        L"\"" + compressor.wstring() + L"\" --lockframe --in \"" +
        video_path.wstring() + L"\" --out \"" + LockscreenJpgPath().wstring() +
        L"\" --offset-s " + FormatOffset(LockscreenOffsetSec());
    std::vector<wchar_t> cmd(args.begin(), args.end());
    cmd.push_back(L'\0');

    // Fire-and-forget: detached + below-normal so a frame extract never
    // janks the render loop or the GUI thread. No wait, no pipe.
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    if (!CreateProcessW(compressor.c_str(), cmd.data(), nullptr, nullptr, FALSE,
                        DETACHED_PROCESS | BELOW_NORMAL_PRIORITY_CLASS,
                        nullptr, nullptr, &si, &pi)) {
      release_slot();
      DebugLog(L"K6WP lockscreen: CreateProcess failed (%lu), skipping",
               GetLastError());
      return;
    }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
  } catch (...) {
    // Gagal hanya log, tidak crash (B-part contract).
    DebugLog(L"K6WP lockscreen: unexpected exception, skipping (ec=%lu)", 0);
  }
}

}  // namespace k6wp
