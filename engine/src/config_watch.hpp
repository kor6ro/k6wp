#pragma once

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

#include "config_schema.hpp"
#include "timer_ids.hpp"

namespace k6wp {

// Polls a config JSON file every ~500 ms and reloads via LoadConfig when the
// file's mtime or size changes. On corrupt JSON, logs the error and keeps the
// last-valid config (never crashes). RAII: constructing Start()s, destructing
// Stop()s.
//
// Thread-safe snapshot (Step 3.2): the IPC worker thread reads GetConfig()
// while the main loop's Poll() may reload. GetConfig() returns a BY-VALUE
// copy under cfg_mutex_; TryReload() assigns under the lock, then invokes
// on_change WITHOUT the lock (the callback fans out to ApplyFitMode +
// PowerSaver::Update, which read GetConfig() back — holding the lock across
// the call would deadlock).
//
// Header stays windows.h-free (std types only); Win32 lives in the .cpp.
class ConfigWatcher {
 public:
  using ConfigCallback = std::function<void(const WallpaperConfig&)>;

  ConfigWatcher() = default;
  ~ConfigWatcher();

  ConfigWatcher(const ConfigWatcher&) = delete;
  ConfigWatcher& operator=(const ConfigWatcher&) = delete;

  // Begins polling `config_path`. Calls `on_change` each time the config is
  // (re)loaded. An empty path is a no-op (Poll() returns immediately).
  void Start(const std::filesystem::path& config_path,
             ConfigCallback on_change);

  // Polls for file changes (mtime + size). Call from the engine's idle loop.
  // Internally throttled: the file is stat'ed at most once per
  // kPollIntervalMs (500 ms), so reload latency after a change is <= ~1 s.
  //
  // P2.1 (Todo 2): Poll() is the FALLBACK path only. When the event-driven
  // watch is armed (EventHandle() != nullptr) the primary trigger is
  // OnDirectoryEvent(); Poll() remains as the safety net (setup failure,
  // missed notifications) and is still called every loop iteration.
  void Poll();

  // P2.1 (Todo 2) event path: returns the overlapped completion event for
  // the directory watch, or nullptr when event-driven watching is not
  // armed (setup failed → Poll() fallback). The engine Run loop joins this
  // handle into its MsgWaitForMultipleObjects wait array and calls
  // OnDirectoryEvent() when the wait reports it signaled. Win32-free
  // signature: the HANDLE is carried as void*.
  void* EventHandle() const;

  // P2.1 (Todo 2) event path: reaps one completed ReadDirectoryChangesW,
  // parses the FILE_NOTIFY_INFORMATION chain, and — for basenames equal to
  // the watched config filename with action MODIFIED / ADDED /
  // RENAMED_OLD_NAME / RENAMED_NEW_NAME — ARMS the 250 ms hidden-window
  // debounce (Todo 3: SetTimer/KillTimer re-arm; expiry runs the unchanged
  // conditional reload via OnDebounceExpired). Returns true when a matching
  // notification was consumed. Always re-issues the watch (unless the
  // directory died, in which case it drops back to the Poll() fallback with
  // one log line). On buffer overflow (ERROR_NOTIFY_ENUM_DIR) performs the
  // full-rescan fallback through the same debounce + one log line.
  //
  // Call only on the engine loop thread, and only when EventHandle() is
  // non-null (harmless no-op otherwise).
  bool OnDirectoryEvent();

  // P2.1 (Todo 3) debounce target: the engine hidden message window used for
  // SetTimer/KillTimer. Wired once from CreateMessageWindow (before Start);
  // cleared implicitly by Stop(). Win32-free signature: HWND as void*.
  // When unset (nullptr, e.g. unit probes without a window) OnDirectoryEvent
  // takes the direct conditional-reload path (Todo 2 behavior) so a change
  // is never lost for lack of a timer.
  void SetDebounceWindow(void* hwnd);

  // P2.1 (Todo 3) debounce expiry: runs the UNCHANGED old path (stat
  // mtime+size → LoadConfig → callback; corrupt → keep-last-valid + log).
  // Called from the engine hidden-window WM_TIMER handler on the loop
  // thread. Harmless no-op when no debounce is pending.
  void OnDebounceExpired();

  // Stops polling and clears state. Idempotent.
  //
  // P2.1 (Todo 3): Stop() kills the pending debounce timer (if the debounce
  // window is set), then CancelIoEx on the outstanding ReadDirectoryChangesW
  // + closes the dir/event handles (Todo 2 RAII cleanup). No watcher thread
  // exists — completions are reaped on the engine loop thread — so there is
  // no join: zero-join is BY DESIGN (not a missing join), and Stop logs its
  // teardown timing as evidence (<200 ms budget applies to the whole Stop).
  void Stop();

  // Returns a snapshot of the last-valid config (by value, under lock).
  // Returns a default config if no file was ever successfully loaded.
  WallpaperConfig GetConfig() const;

  // P2.1 (Todo 3): hidden-window timer id for the config debounce. Carried
  // as uintptr_t so this header stays windows.h-free (cast to UINT_PTR at
  // the SetTimer/KillTimer/WM_TIMER sites in the .cpp + engine_app.cpp).
  // Defined in timer_ids.hpp (central registry, pairwise-distinct asserts).
  static constexpr std::uintptr_t DebounceTimerId() { return kDebounceTimerId; }

 private:
  static constexpr int kPollIntervalMs = 500;
  static constexpr int kDebounceMs = 250;
  // P2.1: 64 KiB notify buffer (MSDN-recommended; 4 KiB would overflow and
  // silently stop notifications via ERROR_NOTIFY_ENUM_DIR). NEVER shrink.
  static constexpr std::size_t kNotifyBufferBytes = 64 * 1024;

  std::filesystem::path config_path_;
  ConfigCallback on_change_;
  mutable std::mutex cfg_mutex_;
  WallpaperConfig current_config_{};
  bool started_ = false;

  // File snapshot for change detection.
  std::filesystem::file_time_type last_mtime_{};
  std::uintmax_t last_size_ = 0;

  // Last time Poll() stat'ed the file (throttle). Default-constructed
  // (epoch) so the first Poll() after Start() proceeds immediately.
  std::chrono::steady_clock::time_point last_poll_{};

  // P2.1 event-driven state (all Win32 handles as void* so this header
  // stays windows.h-free; owned, closed in Stop()). All touched only on
  // the engine loop thread (Start/Poll/OnDirectoryEvent/Stop), except
  // EventHandle() which the same loop thread reads to build its wait array.
  void* dir_handle_ = nullptr;    // CreateFileW on the config directory
  void* event_handle_ = nullptr;  // manual-reset overlapped completion event
  void* overlapped_ = nullptr;    // heap OVERLAPPED* (defined in the .cpp)
  std::vector<std::uint8_t> notify_buf_;  // kNotifyBufferBytes receive buffer
  std::wstring watch_filename_;  // target basename, e.g. L"config.json"
  bool event_armed_ = false;     // a ReadDirectoryChangesW is outstanding
  bool event_driven_ = false;    // setup succeeded → wait-array path is live
  // P2.1 (Todo 3) debounce state (loop thread only): hidden window for
  // SetTimer/KillTimer, pending flag, and coalesced-event count for the
  // expiry evidence line. No thread, no mutex — same-thread use only.
  void* debounce_hwnd_ = nullptr;  // engine hidden message window (void* HWND)
  bool debounce_pending_ = false;  // a 250 ms quiet-period timer is armed
  int debounce_events_ = 0;        // FS events coalesced into the pending arm

  // Unthrottled stat mtime+size → conditional TryReload. Shared by Poll()
  // (which throttles before calling) and OnDebounceExpired().
  void CheckForChange();
  // P2.1 (Todo 3): (re-)arms the 250 ms quiet-period timer on the debounce
  // window (KillTimer + SetTimer). Returns true when deferred to the timer;
  // false when no debounce window is set — the caller then runs the direct
  // conditional reload (Todo 2 path) so no change is ever lost.
  bool ArmDebounce();
  // (Re)issues the outstanding ReadDirectoryChangesW. On failure drops to
  // the Poll() fallback (event_driven_ = false) with one reason log line.
  void IssueWatch();
  // Closes dir/event handles, frees OVERLAPPED + buffer, clears flags.
  void TeardownWatch();

  // Try to load the config; on failure log + keep last-valid.
  void TryReload();
};

}  // namespace k6wp
