#include "config_watch.hpp"
#include "log_file.hpp"

#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cwchar>
#include <mutex>
#include <new>
#include <filesystem>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace k6wp {
namespace {

// Local log helper — mirrors EngineApp::Log pattern (local-time timestamp,
// flush). windows.h is fine here; the header stays Win32-free.
void Log(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  EngineLogfV(fmt, args);
  va_end(args);
}

}  // namespace

ConfigWatcher::~ConfigWatcher() { Stop(); }

void ConfigWatcher::Start(const std::filesystem::path& config_path,
                          ConfigCallback on_change) {
  Stop();  // idempotent

  config_path_ = config_path;
  on_change_ = std::move(on_change);
  started_ = true;

  // Record initial file state for change detection.
  std::error_code ec;
  if (std::filesystem::exists(config_path_, ec)) {
    last_mtime_ = std::filesystem::last_write_time(config_path_, ec);
    if (ec) {
      Log("config-watch: cannot stat '%ls' (%s), will retry on poll",
          config_path_.c_str(), ec.message().c_str());
      last_mtime_ = {};
      last_size_ = 0;
    } else {
      last_size_ = std::filesystem::file_size(config_path_, ec);
      if (ec) last_size_ = 0;
    }
  } else {
    last_mtime_ = {};
    last_size_ = 0;
  }

  // Initial load — fail is non-fatal (keeps default config).
  TryReload();

  // Missing last video at startup is a safe fallback (engine boots without
  // wallpaper, never crashes); log it here so engine.log names the cause.
  const WallpaperConfig boot_cfg = GetConfig();
  if (!boot_cfg.video_path.empty()) {
    std::error_code missing_ec;
    if (!std::filesystem::exists(boot_cfg.video_path, missing_ec)) {
      Log("config-watch: last video missing '%ls', starting without wallpaper "
          "(safe fallback)",
          boot_cfg.video_path.c_str());
    }
  }

  Log("config-watch: started, path='%ls' poll-interval=%dms",
      config_path_.c_str(), kPollIntervalMs);

  // P2.1 (Todo 2): arm the event-driven directory watch. Any setup failure
  // keeps Poll() as the fallback path + one reason log line (QA failure
  // path: point at a bad dir → 500 ms polling + reason). Matching FS events
  // arm the 250 ms hidden-window debounce (Todo 3, needs SetDebounceWindow);
  // without a debounce window they reload directly (Todo 2 path).
  std::filesystem::path watch_dir = config_path_.parent_path();
  if (watch_dir.empty()) watch_dir = std::filesystem::path(L".");
  watch_filename_ = config_path_.filename().wstring();
  HANDLE dir = CreateFileW(watch_dir.c_str(), FILE_LIST_DIRECTORY,
                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           nullptr, OPEN_EXISTING,
                           FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
                           nullptr);
  if (dir == INVALID_HANDLE_VALUE) {
    const DWORD err = GetLastError();
    watch_filename_.clear();
    Log("config-watch: event watch unavailable on '%ls' (%lu), falling back "
        "to %dms polling",
        watch_dir.c_str(), static_cast<unsigned long>(err), kPollIntervalMs);
  } else {
    HANDLE ev = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    OVERLAPPED* ov = new (std::nothrow) OVERLAPPED{};
    if (ev == nullptr || ov == nullptr) {
      const DWORD err = GetLastError();
      if (ov != nullptr) delete ov;
      if (ev != nullptr) CloseHandle(ev);
      CloseHandle(dir);
      watch_filename_.clear();
      Log("config-watch: event watch init failed (%lu), falling back to %dms "
          "polling",
          static_cast<unsigned long>(err), kPollIntervalMs);
    } else {
      ov->hEvent = ev;
      dir_handle_ = dir;
      event_handle_ = ev;
      overlapped_ = ov;
      notify_buf_.assign(kNotifyBufferBytes, 0);
      event_driven_ = true;
      IssueWatch();
      if (event_driven_) {
        Log("config-watch: event=rdevchange watch armed on '%ls' (64 KiB "
            "buffer)",
            watch_dir.c_str());
      }
    }
  }
}

void ConfigWatcher::WatchSecondFile(const std::filesystem::path& path,
                                    SecondCallback on_change) {
  second_path_ = path;
  on_second_change_ = std::move(on_change);
  watch_filename2_.clear();
  second_mtime_ = {};
  second_size_ = 0;
  second_snapshot_valid_ = false;
  if (second_path_.empty()) return;

  // The directory handle is per-directory, so a second basename can only be
  // observed when it lives next to the config file (same handle, no second
  // watch handle — row 16 hard constraint). Refuse loudly otherwise.
  if (started_ && !config_path_.empty() &&
      second_path_.parent_path() != config_path_.parent_path()) {
    Log("warning: config-watch: second file '%ls' not in config directory "
        "'%ls'; not watched (no separate handle by design)",
        second_path_.c_str(), config_path_.parent_path().c_str());
    return;
  }

  std::error_code ec;
  if (std::filesystem::exists(second_path_, ec)) {
    const auto mtime = std::filesystem::last_write_time(second_path_, ec);
    if (!ec) {
      second_mtime_ = mtime;
      second_size_ = std::filesystem::file_size(second_path_, ec);
      if (ec) second_size_ = 0;
      second_snapshot_valid_ = true;
    }
  }
  watch_filename2_ = second_path_.filename().wstring();
  if (!started_) return;  // unit probes register without a directory watch
  if (!event_driven_) {
    Log("warning: config-watch: second-file watch unavailable (event watch "
        "not armed); live reload of '%ls' disabled, no poll fallback",
        second_path_.c_str());
    return;
  }
  Log("config-watch: second-file watch armed '%ls' (same directory handle, "
      "one debounce timer, no poll)",
      second_path_.c_str());
}

void ConfigWatcher::Poll() {
  if (!started_ || config_path_.empty()) return;

  // P2.1 (Todo 3): while a debounce is pending the armed 250 ms timer owns
  // the reload — Poll() must not stat mid-burst or it would steal
  // intermediate reloads and defeat the exactly-1-reload coalescing. The
  // expiry runs the same CheckForChange, so nothing is lost; setup-failure
  // and missed-notification cases never arm (pending stays false) and Poll
  // remains the fallback there.
  if (debounce_pending_) return;

  // Throttle: stat the file at most once per kPollIntervalMs even when the
  // engine's idle loop calls Poll() every loop iteration. P2.1: this is the
  // FALLBACK path — when the event watch is armed the primary trigger is
  // OnDirectoryEvent(), and Poll() stays as the safety net.
  const auto now = std::chrono::steady_clock::now();
  if (now - last_poll_ < std::chrono::milliseconds(kPollIntervalMs)) return;
  last_poll_ = now;

  CheckForChange();
}

void* ConfigWatcher::EventHandle() const {
  return event_driven_ ? event_handle_ : nullptr;
}

void ConfigWatcher::SetDebounceWindow(void* hwnd) { debounce_hwnd_ = hwnd; }

bool ConfigWatcher::ArmDebounce(bool is_second) {
  if (is_second && second_path_.empty()) return false;
  if (debounce_hwnd_ == nullptr) return false;  // no window → direct path
  HWND hwnd = static_cast<HWND>(debounce_hwnd_);
  constexpr UINT_PTR kId = static_cast<UINT_PTR>(kDebounceTimerId);
  KillTimer(hwnd, kId);  // re-arm: each FS event restarts the quiet period
  if (!debounce_pending_) {
    Log("config-watch: debounce armed (250ms quiet)");
  }
  debounce_pending_ = true;
  if (is_second) {
    debounce_pending_second_ = true;
  } else {
    debounce_pending_config_ = true;
  }
  ++debounce_events_;
  if (SetTimer(hwnd, kId, static_cast<UINT>(kDebounceMs), nullptr) == 0) {
    const DWORD err = GetLastError();
    debounce_pending_ = false;
    debounce_pending_config_ = false;
    debounce_pending_second_ = false;
    debounce_events_ = 0;
    Log("config-watch: SetTimer failed (%lu), direct reload fallback",
        static_cast<unsigned long>(err));
    return false;
  }
  return true;
}

void ConfigWatcher::OnDebounceExpired() {
  if (!debounce_pending_) return;  // stray WM_TIMER; not ours
  if (debounce_hwnd_ != nullptr) {
    KillTimer(static_cast<HWND>(debounce_hwnd_),
              static_cast<UINT_PTR>(kDebounceTimerId));
  }
  debounce_pending_ = false;
  // Row 16: one shared timer, per-file pending flags — run each file's
  // change check exactly once. UNCHANGED config path: stat mtime+size →
  // LoadConfig → callback; corrupt → keep-last-valid + log (CheckForChange/
  // TryReload). Second file: same stat rule → owner callback.
  const bool check_config = debounce_pending_config_;
  const bool check_second = debounce_pending_second_;
  debounce_pending_config_ = false;
  debounce_pending_second_ = false;
  const int coalesced = debounce_events_;
  debounce_events_ = 0;
  Log("config-watch: debounce expired (250ms quiet, %d event%s coalesced)",
      coalesced, coalesced == 1 ? "" : "s");
  if (check_config) {
    last_poll_ = std::chrono::steady_clock::now();
    CheckForChange();
  }
  if (check_second) CheckSecondForChange();
}

bool ConfigWatcher::OnDirectoryEvent() {
  if (!event_driven_ || !event_armed_) return false;
  HANDLE dir = static_cast<HANDLE>(dir_handle_);
  OVERLAPPED* ov = static_cast<OVERLAPPED*>(overlapped_);
  if (dir == nullptr || ov == nullptr) return false;

  DWORD bytes = 0;
  if (!GetOverlappedResult(dir, ov, &bytes, FALSE)) {
    const DWORD err = GetLastError();
    if (err == ERROR_IO_INCOMPLETE) return false;  // spurious wake; still armed
    event_armed_ = false;
    if (err == ERROR_NOTIFY_ENUM_DIR) {
      // Buffer overflowed (should not happen with 64 KiB, but MSDN says it
      // can under burst load): notifications were lost, so fall back to the
      // full rescan — stat mtime+size → conditional LoadConfig, exactly the
      // old Poll() path — deferred through the same 250 ms debounce (burst
      // load coalescing) plus one log line, then re-arm.
      Log("config-watch: event=rdevchange overflow (buffer overrun), "
          "full-rescan fallback");
      if (event_driven_) IssueWatch();
      // Row 16: notifications were lost for the whole directory — rescan
      // BOTH watched files, coalesced through the one debounce (or directly
      // when no window is set).
      const bool armed_config = ArmDebounce(false);
      const bool armed_second = ArmDebounce(true);
      if (!armed_config) {
        last_poll_ = std::chrono::steady_clock::now();
        CheckForChange();
      }
      if (!armed_second) CheckSecondForChange();
      return true;
    }
    // The watch died (directory deleted, handle invalid, ...): drop back to
    // the Poll() fallback with one reason log line.
    Log("config-watch: event watch failed (%lu), falling back to %dms polling",
        static_cast<unsigned long>(err), kPollIntervalMs);
    TeardownWatch();
    return false;
  }
  event_armed_ = false;

  // Parse the FILE_NOTIFY_INFORMATION chain; basenames equal to the watched
  // config filename OR the row-16 second filename with MODIFIED / ADDED /
  // RENAMED_OLD_NAME / RENAMED_NEW_NAME (atomic temp+rename save) arm the
  // 250 ms debounce (KillTimer + SetTimer re-arm on the hidden window;
  // expiry dispatches each pending file through OnDebounceExpired, so 10
  // rapid writes coalesce into exactly 1 reload per file after 250 ms quiet).
  // Without a debounce window the direct conditional reload runs (Todo 2
  // path). Both names share this one walk/handle/timer.
  bool matched_config = false;
  bool matched_second = false;
  if (bytes > 0 && bytes <= notify_buf_.size()) {
    DWORD offset = 0;
    for (;;) {
      const FILE_NOTIFY_INFORMATION* info =
          reinterpret_cast<const FILE_NOTIFY_INFORMATION*>(notify_buf_.data() +
                                                           offset);
      const bool interesting =
          (info->Action == FILE_ACTION_ADDED ||
           info->Action == FILE_ACTION_MODIFIED ||
           info->Action == FILE_ACTION_RENAMED_OLD_NAME ||
           info->Action == FILE_ACTION_RENAMED_NEW_NAME);
      if (interesting && info->FileNameLength > 0) {
        std::wstring name(info->FileName,
                          info->FileNameLength / sizeof(wchar_t));
        const auto slash = name.find_last_of(L"\\/");
        const std::wstring base =
            (slash == std::wstring::npos) ? name : name.substr(slash + 1);
        if (!watch_filename_.empty() &&
            ::_wcsicmp(base.c_str(), watch_filename_.c_str()) == 0) {
          matched_config = true;
          Log("config-watch: event=rdevchange action=%lu file='%ls'",
              static_cast<unsigned long>(info->Action), base.c_str());
        } else if (!watch_filename2_.empty() &&
                   ::_wcsicmp(base.c_str(), watch_filename2_.c_str()) == 0) {
          matched_second = true;
          Log("config-watch: event=rdevchange action=%lu file='%ls'",
              static_cast<unsigned long>(info->Action), base.c_str());
        }
      }
      if (info->NextEntryOffset == 0) break;
      offset += info->NextEntryOffset;
      if (offset >= bytes) break;  // malformed-chain guard
    }
  }
  if (matched_config) {
    if (!ArmDebounce(false)) {
      last_poll_ = std::chrono::steady_clock::now();
      CheckForChange();
    }
  }
  if (matched_second) {
    if (!ArmDebounce(true)) CheckSecondForChange();
  }
  if (event_driven_) IssueWatch();  // re-arm (falls back on failure)
  return matched_config || matched_second;
}

void ConfigWatcher::CheckForChange() {
  if (!started_ || config_path_.empty()) return;

  std::error_code ec;
  if (!std::filesystem::exists(config_path_, ec)) return;

  const auto mtime = std::filesystem::last_write_time(config_path_, ec);
  if (ec) return;

  std::uintmax_t size = std::filesystem::file_size(config_path_, ec);
  if (ec) size = 0;

  // Detect change via mtime OR size.
  if (mtime != last_mtime_ || size != last_size_) {
    last_mtime_ = mtime;
    last_size_ = size;
    TryReload();
  }
}

bool ConfigWatcher::CheckSecondForChange() {
  if (second_path_.empty() || on_second_change_ == nullptr) return false;

  std::error_code ec;
  if (!std::filesystem::exists(second_path_, ec)) {
    // Deleted between events: consume the snapshot so a later re-create is
    // seen as a change again. No callback on deletion (watch+reload only).
    second_snapshot_valid_ = false;
    second_mtime_ = {};
    second_size_ = 0;
    return false;
  }

  const auto mtime = std::filesystem::last_write_time(second_path_, ec);
  if (ec) return false;
  std::uintmax_t size = std::filesystem::file_size(second_path_, ec);
  if (ec) size = 0;

  if (second_snapshot_valid_ && mtime == second_mtime_ && size == second_size_) {
    return false;
  }
  second_mtime_ = mtime;
  second_size_ = size;
  second_snapshot_valid_ = true;
  // The owner's callback re-reads the file and re-converges; its own catch
  // keeps a corrupt file from throwing. The backstop here guarantees a
  // callback bug can never unwind the engine's message loop.
  try {
    on_second_change_();
  } catch (const std::exception& e) {
    Log("config-watch: second-file callback failed (ignored): %s", e.what());
  } catch (...) {
    Log("config-watch: second-file callback failed (ignored: unknown error)");
  }
  return true;
}

void ConfigWatcher::IssueWatch() {
  if (dir_handle_ == nullptr || event_handle_ == nullptr ||
      overlapped_ == nullptr || notify_buf_.size() < kNotifyBufferBytes) {
    event_driven_ = false;
    event_armed_ = false;
    return;
  }
  OVERLAPPED* ov = static_cast<OVERLAPPED*>(overlapped_);
  HANDLE ev = static_cast<HANDLE>(event_handle_);
  ResetEvent(ev);
  // Preserve hEvent across reuse; clear the rest of the OVERLAPPED.
  ZeroMemory(ov, sizeof(*ov));
  ov->hEvent = ev;
  constexpr DWORD kFilter = FILE_NOTIFY_CHANGE_FILE_NAME |
                            FILE_NOTIFY_CHANGE_DIR_NAME |
                            FILE_NOTIFY_CHANGE_ATTRIBUTES |
                            FILE_NOTIFY_CHANGE_SIZE |
                            FILE_NOTIFY_CHANGE_LAST_WRITE |
                            FILE_NOTIFY_CHANGE_CREATION;
  event_armed_ = true;
  const BOOL ok = ReadDirectoryChangesW(
      static_cast<HANDLE>(dir_handle_), notify_buf_.data(),
      static_cast<DWORD>(notify_buf_.size()), FALSE, kFilter, nullptr, ov,
      nullptr);
  if (!ok) {
    const DWORD err = GetLastError();
    if (err != ERROR_IO_PENDING) {
      event_armed_ = false;
      event_driven_ = false;
      Log("config-watch: event=rdevchange watch re-issue failed (%lu), "
          "falling back to %dms polling",
          static_cast<unsigned long>(err), kPollIntervalMs);
      TeardownWatch();
    }
  }
}

void ConfigWatcher::TeardownWatch() {
  // Kill any pending debounce first: the timer posts WM_TIMER to the
  // debounce window, which may be destroyed right after Stop().
  if (debounce_hwnd_ != nullptr) {
    KillTimer(static_cast<HWND>(debounce_hwnd_),
              static_cast<UINT_PTR>(kDebounceTimerId));
  }
  debounce_pending_ = false;
  debounce_pending_config_ = false;
  debounce_pending_second_ = false;
  debounce_events_ = 0;
  if (dir_handle_ != nullptr && overlapped_ != nullptr) {
    // Mirror the ipc_server Stop discipline: cancel outstanding I/O before
    // closing. No watcher thread exists (completions are reaped on the
    // engine loop thread), so no join is needed — Todo 3 owns the <200 ms
    // join if it ever adds one.
    CancelIoEx(static_cast<HANDLE>(dir_handle_),
               static_cast<LPOVERLAPPED>(overlapped_));
  }
  delete static_cast<OVERLAPPED*>(overlapped_);
  overlapped_ = nullptr;
  if (event_handle_ != nullptr) {
    CloseHandle(static_cast<HANDLE>(event_handle_));
    event_handle_ = nullptr;
  }
  if (dir_handle_ != nullptr) {
    CloseHandle(static_cast<HANDLE>(dir_handle_));
    dir_handle_ = nullptr;
  }
  notify_buf_.clear();
  notify_buf_.shrink_to_fit();
  watch_filename_.clear();
  watch_filename2_.clear();
  event_armed_ = false;
  event_driven_ = false;
}

void ConfigWatcher::Stop() {
  // No watcher thread exists (completions are reaped on the engine loop
  // thread), so there is no join — zero-join BY DESIGN. The <200 ms budget
  // applies to the whole Stop below; teardown timing is logged as evidence.
  const auto t0 = std::chrono::steady_clock::now();
  // Row 16: the second-file registration dies with the watcher in both
  // branches (a later Start() must re-register it).
  second_path_.clear();
  on_second_change_ = nullptr;
  second_mtime_ = {};
  second_size_ = 0;
  second_snapshot_valid_ = false;
  if (!started_) {
    TeardownWatch();  // belt-and-braces: never leak dir/event handles
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - t0)
                        .count();
    Log("config-watch: stopped (teardown=%lldms, no watcher thread, "
        "zero-join by design)",
        static_cast<long long>(ms));
    return;
  }
  TeardownWatch();
  config_path_.clear();
  on_change_ = nullptr;
  last_mtime_ = {};
  last_size_ = 0;
  last_poll_ = {};
  started_ = false;
  const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::steady_clock::now() - t0)
                      .count();
  Log("config-watch: stopped (teardown=%lldms, no watcher thread, "
      "zero-join by design)",
      static_cast<long long>(ms));
}

WallpaperConfig ConfigWatcher::GetConfig() const {
  std::lock_guard<std::mutex> lock(cfg_mutex_);
  return current_config_;
}

void ConfigWatcher::TryReload() {
  WallpaperConfig fresh;
  try {
    fresh = LoadConfig(config_path_);
  } catch (const ConfigError& e) {
    Log("config-watch: reload failed (keeping last-valid): %s", e.what());
    return;
  } catch (const std::exception& e) {
    Log("config-watch: unexpected error (keeping last-valid): %s", e.what());
    return;
  }
  {
    std::lock_guard<std::mutex> lock(cfg_mutex_);
    current_config_ = fresh;
  }
  // Callback runs WITHOUT the lock: it fans out to ApplyFitMode +
  // PowerSaver::Update, both of which read GetConfig() back.
  if (on_change_) {
    on_change_(fresh);
  }
  // speed is validated + persisted but RESERVED: no renderer consumer yet
  // (MpvRenderer::SetSpeed was removed unused). Logged for visibility only.
  Log("config-watch: loaded '%ls' (video='%ls' fit=%s speed=%.1f [reserved])",
      config_path_.c_str(), fresh.video_path.c_str(), fresh.fit_mode.c_str(),
      fresh.speed);
}

}  // namespace k6wp
