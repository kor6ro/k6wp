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

// Forward declaration: FireLockscreenSyncAsync (the legacy entry point) is
// defined before the producer, which is the shared implementation.
bool ProduceLockscreenFrame(const std::filesystem::path& video_path,
                             LockscreenTrigger trigger, int wait_ms) noexcept;

namespace {

// Debounce window for repeated syncs of the SAME file. A DIFFERENT file is
// allowed through immediately: quick-switching wallpapers must not leave the
// lockscreen showing the previous frame with no later event to correct it.
//
// The debounce key is (path, mtime, size): the same path with NEW content
// (a re-compressed wallpaper) is a different file and must refresh, even
// inside the window. Keying on the path alone swallowed that refresh.
constexpr ULONGLONG kDebounceMs = 5000;
// Budget for the shutdown path: the frame extract must land before the
// session ends. Windows gives the app a few seconds between
// WM_QUERYENDSESSION and WM_ENDSESSION; 4 s stays inside that window.
constexpr DWORD kShutdownWaitMs = 4000;

std::mutex g_fire_mutex;
std::wstring g_last_fire_path;
ULONGLONG g_last_fire_ms = 0;  // both guarded by g_fire_mutex
std::uintmax_t g_last_fire_size = 0;
std::uintmax_t g_last_fire_mtime = 0;

// Cached view of the two settings the Engine may read (LOW-16). The file is
// re-parsed only when its mtime/size change, so one event costs at most ONE
// parse (it used to cost two: one per accessor).
struct SettingsCache {
  std::mutex mutex;
  bool valid = false;
  std::uintmax_t size = 0;
  std::uintmax_t mtime = 0;
  bool enabled = false;
  double offset = 1.0;
};

SettingsCache& Cache() {
  static SettingsCache cache;
  return cache;
}

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

// mtime + size of a file, used as the cache/debounce key. Both are 0 when
// the file cannot be stat'ed.
void FileStamp(const std::filesystem::path& path, std::uintmax_t& size,
               std::uintmax_t& mtime) {
  size = 0;
  mtime = 0;
  std::error_code ec;
  const auto st = std::filesystem::status(path, ec);
  if (ec || !std::filesystem::exists(st)) return;
  const auto stamp = std::filesystem::last_write_time(path, ec);
  if (ec) return;
  mtime = static_cast<std::uintmax_t>(
      stamp.time_since_epoch().count());
  size = std::filesystem::file_size(path, ec);
  if (ec) size = 0;
}

struct SettingsView {
  bool enabled = false;
  double offset = 1.0;
};

SettingsView ReadSettingsCached() {
  SettingsView view;
  const std::filesystem::path path = DefaultStudioSettingsPath();
  std::uintmax_t size = 0;
  std::uintmax_t mtime = 0;
  FileStamp(path, size, mtime);

  SettingsCache& cache = Cache();
  {
    std::lock_guard<std::mutex> lock(cache.mutex);
    if (cache.valid && cache.size == size && cache.mtime == mtime) {
      view.enabled = cache.enabled;
      view.offset = cache.offset;
      return view;
    }
  }

  bool enabled = false;
  double offset = 1.0;
  bool parsed = false;
  try {
    // LOW-16 ENGINE-READ CONTRACT (dev-contracts.md §3): the Engine may
    // observe exactly two studio_settings.json fields — `lockscreen_sync`
    // (here) and `lockscreen_offset_sec` (LockscreenOffsetSec below). Every
    // other field is Studio-private; adding a third Engine read requires a
    // §3 update plus a studio_logic_test §6 case.
    const StudioSettings s = LoadStudioSettings(path);
    enabled = s.lockscreen_sync;
    offset = s.lockscreen_offset_sec < 0.0 ? 0.0 : s.lockscreen_offset_sec;
    parsed = true;
  } catch (...) {
    // File missing/corrupt: cannot confirm the user's lockscreen_sync intent.
    // Do not overwrite the file — let Studio re-enable on next toggle so the
    // user's choice is never silently discarded. Best-effort debug output so
    // the cause is visible in debugger / DBGVIEW.
    OutputDebugStringW(L"lockscreen: could not read studio_settings.json; "
                       L"lockscreen sync disabled (re-enable via Studio to restore)\n");
  }

  {
    std::lock_guard<std::mutex> lock(cache.mutex);
    // A failed read is cached too: the "file missing" state is stable until
    // the file appears, and re-parsing it on every event is pointless.
    cache.valid = true;
    cache.size = size;
    cache.mtime = mtime;
    cache.enabled = enabled;
    cache.offset = offset;
  }
  (void)parsed;
  view.enabled = enabled;
  view.offset = offset;
  return view;
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
  return ReadSettingsCached().enabled;
}

double LockscreenOffsetSec() noexcept {
  return ReadSettingsCached().offset;
}

bool ShouldRefreshLockscreen(LockscreenTrigger trigger) {
  switch (trigger) {
    case LockscreenTrigger::kVideoChanged:
    case LockscreenTrigger::kRotation:
    case LockscreenTrigger::kSessionLock:
    case LockscreenTrigger::kSuspend:
    case LockscreenTrigger::kShutdown:
    case LockscreenTrigger::kMonitorOn:
      return true;
  }
  return false;
}

bool LockscreenTriggerWaits(LockscreenTrigger trigger) {
  // Only the shutdown path waits: the frame has to be on disk before the
  // session ends. Everything else stays fire-and-forget.
  return trigger == LockscreenTrigger::kShutdown;
}

bool LockscreenTriggerBypassesDebounce(LockscreenTrigger trigger) {
  // The picture must be current when the screen locks / the machine sleeps /
  // the session ends: a debounce from an earlier refresh must not swallow
  // these.
  return trigger == LockscreenTrigger::kSessionLock ||
         trigger == LockscreenTrigger::kSuspend ||
         trigger == LockscreenTrigger::kShutdown;
}

namespace {

// Runs the frame extract. Returns false when it could not be started.
bool SpawnFrameExtract(const std::filesystem::path& video_path, double offset,
                       bool wait_for_completion) {
  const std::filesystem::path compressor = SiblingCompressor();
  if (compressor.empty()) {
    DebugLog(L"K6WP lockscreen: compressor.exe not found, skipping (ec=%lu)",
             GetLastError());
    return false;
  }

  // Ensure the output dir exists (best-effort; compressor would fail alone
  // and that failure is contained in the child either way).
  std::error_code ec;
  std::filesystem::create_directories(LockscreenDir(), ec);

  const std::wstring args =
      L"\"" + compressor.wstring() + L"\" --lockframe --in \"" +
      video_path.wstring() + L"\" --out \"" + LockscreenJpgPath().wstring() +
      L"\" --offset-s " + FormatOffset(offset);
  std::vector<wchar_t> cmd(args.begin(), args.end());
  cmd.push_back(L'\0');

  STARTUPINFOW si{};
  si.cb = sizeof(si);
  PROCESS_INFORMATION pi{};
  DWORD flags = BELOW_NORMAL_PRIORITY_CLASS;
  if (!wait_for_completion) flags |= DETACHED_PROCESS;
  if (!CreateProcessW(compressor.c_str(), cmd.data(), nullptr, nullptr, FALSE,
                      flags, nullptr, nullptr, &si, &pi)) {
    DebugLog(L"K6WP lockscreen: CreateProcess failed (%lu), skipping",
             GetLastError());
    return false;
  }
  CloseHandle(pi.hThread);
  if (wait_for_completion) {
    // Bounded wait: the shutdown path needs the frame on disk, but a stuck
    // ffmpeg must never block the session end.
    const DWORD wait = WaitForSingleObject(pi.hProcess, kShutdownWaitMs);
    if (wait != WAIT_OBJECT_0) {
      DebugLog(L"K6WP lockscreen: shutdown frame extract did not finish in time "
               L"(%lu), continuing",
               static_cast<DWORD>(kShutdownWaitMs));
    }
    CloseHandle(pi.hProcess);
  } else {
    // Fire-and-forget: detached + below-normal so a frame extract never
    // janks the render loop or the GUI thread. No wait, no pipe.
    CloseHandle(pi.hProcess);
  }
  return true;
}

}  // namespace

void FireLockscreenSyncAsync(const std::filesystem::path& video_path) noexcept {
  // The plain fire-and-forget path (no live frame, no wait): the legacy
  // entry point kept for callers that just want a frame on disk.
  ProduceLockscreenFrame(video_path, LockscreenTrigger::kVideoChanged,
                          /*wait_ms=*/0);
}

namespace {

// The live-frame provider (see SetLockscreenLiveFrameFn). Guarded by its
// own mutex: the Engine registers it once at start-up, the helper reads it
// from the message-loop thread and from the refresher thread.
std::mutex g_live_frame_mutex;
LockscreenLiveFrameFn g_live_frame_fn = nullptr;

// The temp file the live frame is written to. It sits next to the real one
// so the final move is a same-volume rename (atomic).
std::filesystem::path LiveFrameTempPath() {
  return LockscreenDir() / L"lockscreen.live.tmp.jpg";
}

// Moves `from` onto `to` atomically (the same pattern the compressor uses
// for its own output: replace + write-through, so Winlogon never reads a
// half-written file).
bool AtomicReplaceFile(const std::filesystem::path& from,
                        const std::filesystem::path& to) {
  if (!MoveFileExW(from.c_str(), to.c_str(),
                   MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
    return false;
  }
  return true;
}

// Waits for `path` to exist and to be non-empty (the live capture lands
// asynchronously). Bounded.
bool WaitForFile(const std::filesystem::path& path, int wait_ms) {
  if (wait_ms <= 0) return false;
  const ULONGLONG deadline = GetTickCount64() + static_cast<ULONGLONG>(wait_ms);
  for (;;) {
    std::error_code ec;
    if (std::filesystem::exists(path, ec) && !ec) {
      const auto size = std::filesystem::file_size(path, ec);
      if (!ec && size > 0) return true;
    }
    if (GetTickCount64() >= deadline) return false;
    Sleep(25);
  }
}

}  // namespace

void SetLockscreenLiveFrameFn(LockscreenLiveFrameFn fn) {
  std::lock_guard<std::mutex> lock(g_live_frame_mutex);
  g_live_frame_fn = fn;
}

bool ProduceLockscreenFrame(const std::filesystem::path& video_path,
                             LockscreenTrigger trigger, int wait_ms) noexcept {
  try {
    if (!ShouldRefreshLockscreen(trigger)) return false;
    if (video_path.empty()) return false;
    if (!IsLockscreenSyncEnabled()) return false;

    LockscreenLiveFrameFn live = nullptr;
    {
      std::lock_guard<std::mutex> lock(g_live_frame_mutex);
      live = g_live_frame_fn;
    }

    // 1. The live frame: the picture on screen right now.
    if (live != nullptr && wait_ms > 0) {
      const std::filesystem::path tmp = LiveFrameTempPath();
      std::error_code ec;
      std::filesystem::remove(tmp, ec);
      if (live(tmp.u8string()) && WaitForFile(tmp, wait_ms)) {
        if (AtomicReplaceFile(tmp, LockscreenJpgPath())) {
          return true;
        }
      }
      std::filesystem::remove(tmp, ec);
    }

    // 2. The file-based extract: a frame from the start of the video.
    const bool bypass = LockscreenTriggerBypassesDebounce(trigger);
    const bool wait = LockscreenTriggerWaits(trigger);

    std::uintmax_t size = 0;
    std::uintmax_t mtime = 0;
    FileStamp(video_path, size, mtime);

    if (!bypass) {
      // Reserve the debounce slot for this file. The same file inside the
      // window is dropped; a different file always proceeds. A reserved slot
      // is released again on any failure below so a failed attempt cannot
      // mute the next 5s.
      std::lock_guard<std::mutex> lock(g_fire_mutex);
      const ULONGLONG now = GetTickCount64();
      const bool same_file = !g_last_fire_path.empty() &&
                             video_path.wstring() == g_last_fire_path &&
                             size == g_last_fire_size &&
                             mtime == g_last_fire_mtime;
      if (same_file && now - g_last_fire_ms < kDebounceMs) {
        return false;
      }
      g_last_fire_path = video_path.wstring();
      g_last_fire_ms = now;
      g_last_fire_size = size;
      g_last_fire_mtime = mtime;
    }

    std::error_code ec;
    if (!std::filesystem::exists(video_path, ec)) {
      if (!bypass) {
        std::lock_guard<std::mutex> lock(g_fire_mutex);
        g_last_fire_path.clear();
      }
      DebugLog(L"K6WP lockscreen: video missing, skipping sync (ec=%lu)",
               static_cast<DWORD>(ec.value()));
      return false;
    }

    if (!SpawnFrameExtract(video_path, LockscreenOffsetSec(), wait)) {
      if (!bypass) {
        std::lock_guard<std::mutex> lock(g_fire_mutex);
        g_last_fire_path.clear();
      }
      return false;
    }
    return true;
  } catch (...) {
    DebugLog(L"K6WP lockscreen: unexpected exception, skipping (ec=%lu)", 0);
    return false;
  }
}

void FireLockscreenSyncForTrigger(const std::filesystem::path& video_path,
                                  LockscreenTrigger trigger) noexcept {
  // The direct fire-and-forget path: no wait, so the live frame is skipped
  // and the file-based extract is used. The Engine's refresher thread calls
  // ProduceLockscreenFrame() with a wait to get the live frame.
  ProduceLockscreenFrame(video_path, trigger,
                          LockscreenTriggerWaits(trigger)
                              ? static_cast<int>(kShutdownWaitMs)
                              : 0);
}

}  // namespace k6wp
