#pragma once

// RAII mpv renderer for the wallpaper engine.
// Renders video via libmpv into an HWND with hardware decode fallback chain
// (d3d11va → dxva2 → software), infinite loop, no audio.
//
// windows.h lives in .cpp only — header exposes std types + opaque void* for HWND.
//
// MUTEX DISCIPLINE (P2.4):
// - mutex_ guards COMMAND CALLS ONLY: Create, SetHWND, LoadLoop, SetFitMode,
//   Pause, Resume, SetFpsCap, and mpv_terminate_destroy in dtor.
// - The EVENT THREAD (EventThreadLoop) NEVER holds mutex_ while blocked in
//   mpv_wait_event. It polls events lock-free; hwdec_active_ is atomic.
// - PROPERTY_CHANGE for hwdec-current/vo-configured posts a message to the
//   engine's hidden window; the MAIN THREAD runs TryFallbackHwdec (which
//   takes mutex_) and updates hwdec_active_.
// - EOF watchdog runs in the event thread with dynamic timeout:
//   500 ms while unpaused (check each wake), -1 (infinite) while paused.
//   Atomic paused_ flag controls the mode; Pause()/Resume() flip it and call
//   mpv_wakeup() to break the -1 block immediately.
// - Shutdown: quit_ flag → mpv_wakeup() → join thread → ONLY THEN
//   mpv_terminate_destroy under mutex_. >200 ms join → log critical but
//   keep blocking (NEVER TerminateThread). No join from inside mpv callback.

#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <mutex>
#include <string>
#include <thread>

// Opaque forward declarations — avoids including mpv/client.h in consumers.
struct mpv_handle;
struct mpv_event;

namespace k6wp {

class MpvRenderer {
 public:
  MpvRenderer();
  ~MpvRenderer();

  MpvRenderer(const MpvRenderer&) = delete;
  MpvRenderer& operator=(const MpvRenderer&) = delete;

  // Creates the mpv instance, sets hwdec chain (d3d11va→dxva2→no),
  // vo=gpu, audio=no, loop-file=inf, and initializes.
  // hwnd is the target window for embedding (set via wid option).
  // Also starts the dedicated event thread.
  // Returns true on success.
  bool Create(void* hwnd);

  // Updates the embedding window.
  // If called before Create(), stores the HWND for later use.
  // If called after Create(), sets the wid property at runtime.
  void SetHWND(void* hwnd);

  // P3L.3: iGPU/dGPU pin. Call BEFORE Create(): stores the `d3d11-adapter`
  // substring, applied as an mpv option at init (VO level — a runtime swap
  // is not honored, PATCH A). Empty = no pin. pin_active() reports whether
  // this instance was created with a pin (for the verify/revert pass).
  void SetAdapterPin(const std::string& substr);
  bool pin_active() const;

  // Loads a video file with infinite loop. Path must be UTF-8.
  // Idempotent: same path + unchanged size/mtime is a no-op returning true
  // (decode keeps running, no flash); changed bytes reload normally.
  // force=true re-issues loadfile unconditionally (device-lost recovery,
  // where the decoder state is gone even though the file is unchanged).
  bool LoadLoop(const std::string& path, bool force = false);

  // Applies a WallpaperConfig fit_mode ("cover" | "fill" | "fit" |
  // "stretch" | "center", see shared/config_schema.hpp) to the live mpv
  // instance. "cover" and "fill" are equivalent (fullscreen fill, no black
  // bars); "fit" is contain (letterbox). window_aspect is the target surface
  // aspect (width/height, > 0) used ONLY by "stretch" (forced
  // aspect-override = distort-to-fill); other modes ignore it. Unknown
  // values degrade to "fit" (letterbox, safe). No-op before Create().
  // Thread-safe (mutex_ guards command calls).
  void SetFitMode(const std::string& fit_mode, double window_aspect = 0.0);

  // Pauses playback. Sets atomic paused_ flag and wakes event thread
  // so it switches to -1 timeout (zero wakeups while paused).
  void Pause();

  // Resumes playback. Clears atomic paused_ flag and wakes event thread
  // so it rearms 500 ms watchdog cadence immediately.
  void Resume();

  // Caps the output frame rate via the mpv "vf" fps filter ("fps=N").
  // fps <= 0 clears the filter chain (restores full rate). Runtime-settable,
  // no re-encode, no pause — the battery-saver path uses this.
  void SetFpsCap(int fps);

  // Wakes the event thread out of mpv_wait_event (e.g. for fast IPC drain
  // or to re-arm watchdog after resume). Thread-safe, no mutex.
  void Wakeup();

  // True once any file started (START_FILE/FILE_LOADED seen on the event
  // thread). Gates the pin verify pass: hwdec "no" before the first start
  // is a transient normal, never a revert trigger (PATCH A).
  bool EverStarted() const {
    return ever_started_.load(std::memory_order_relaxed);
  }

  // Is hardware decoding active (hwdec-current != "no")?
  bool IsHwdecActive() const {
    return hwdec_active_.load(std::memory_order_relaxed);
  }

  // Registers the engine hidden window for PROPERTY_CHANGE notifications.
  // Called once by EngineApp during Init (Todo 7); until set, the event
  // thread handles property changes inline (pre-P2.4 behavior preserved).
  void SetMessageWindow(void* hwnd);

  // Called from engine main thread when PROPERTY_CHANGE message is received.
  // Runs the hwdec fallback logic (takes mutex_).
  void OnHwdecPropertyChange();

 private:
  // Event thread entry point. Runs mpv_wait_event with dynamic timeout:
  // 500 ms while unpaused (EOF watchdog check each wake), -1 while paused.
  // Processes events lock-free; posts PROPERTY_CHANGE to message_hwnd_.
  void EventThreadLoop();

  // Stops the event thread: quit flag -> mpv_wakeup -> join. Join >200 ms
  // logs critical but keeps blocking (never TerminateThread). No mutex_
  // held during join (the thread briefly takes mutex_ for watchdog/fallback
  // commands). Safe to call when no thread runs.
  void StopEventThread();

  // Handles one mpv event. Takes event_mutex_ internally (brief, non-blocking).
  // PROPERTY_CHANGE posts to message_hwnd_ when set, else applies the hwdec
  // decision inline. Called from the event thread only.
  void HandleEvent(mpv_event* ev);

  // Applies an observed hwdec-current value: updates hwdec_active_ (atomic),
  // dedups the log line, and returns true when the next hwdec fallback link
  // must run (TryFallbackHwdec advances the d3d11va → dxva2 → software
  // chain). Call with event_mutex_ held; the caller runs
  // TryFallbackHwdec() AFTER releasing event_mutex_ (it takes mutex_ —
  // never nest the two).
  bool ApplyHwdecValue(const std::string& cur);

  // Fallback link for the hwdec chain (d3d11va → dxva2 → software). When
  // hwdec-current stays "no" the caller advances one link: stage 0 requests
  // dxva2, stage 1 requests software ("no"), stage 2 gives up (logged once).
  // Takes mutex_ only (never event_mutex_); callable from any thread.
  void TryFallbackHwdec();

  // EOF watchdog check (called from event thread, NO mutex).
  // Re-issues loadfile if EOF stuck for >= 2000 ms.
  void CheckEofWatchdog();

  mpv_handle* mpv_ = nullptr;
  // Serializes COMMAND CALLS ONLY (Create, SetHWND, LoadLoop, SetFitMode,
  // Pause, Resume, SetFpsCap, dtor destroy). The event thread
  // NEVER holds this while blocked in mpv_wait_event.
  mutable std::mutex mutex_;
  // Guards event-loop state shared between the event thread and
  // OnHwdecPropertyChange(). Held only
  // briefly — never across mpv_wait_event or mutex_ (the async PostMessageW
  // inside HandleEvent does not block, so posting under this lock is safe).
  // Lock order (only LoadLoop nests): mutex_ -> event_mutex_. No path takes
  // mutex_ while holding event_mutex_, so the order is deadlock-free.
  mutable std::mutex event_mutex_;
  std::atomic<bool> hwdec_active_{false};
  bool initialized_ = false;
  // M3 (1.3.0-beta.2): hwdec fallback chain position, guarded by mutex_:
  // 0 = d3d11va requested at Create, 1 = dxva2 requested, 2 = software
  // requested. A one-shot dxva2 attempt (the old exchange guard) left the
  // wallpaper black when dxva2 was also unavailable.
  int hwdec_stage_ = 0;
  std::string last_hwdec_logged_;
  // Last vo-configured flag logged, so a VO restart is logged once per real
  // change instead of per event. Guarded by event_mutex_ (touched only in
  // HandleEvent).
  int last_vo_configured_ = -1;
  void* pending_hwnd_ = nullptr;  // Stored before Create() if SetHWND called first.
  // P3L.3: `d3d11-adapter` substring stored before Create(), applied as an
  // mpv option at init. pin_applied_ records the value used (mutex_-guarded,
  // written once in Create before the event thread starts).
  std::string pending_adapter_;
  std::string pin_applied_;
  // EOF watchdog: last LoadLoop path (UTF-8) + END_FILE(EOF) state. Native
  // loop-file=inf normally restarts by itself (emitting START_FILE, which
  // disarms the watchdog); only when no restart arrives within the grace
  // period does the event thread re-issue loadfile so a video can never
  // silently end on the desktop. last_path_/size_/mtime_ guarded by mutex_;
  // eof_pending_/eof_at_ guarded by event_mutex_.
  std::string last_path_;
  std::uintmax_t last_size_ = 0;
  std::filesystem::file_time_type last_mtime_{};
  bool eof_pending_ = false;
  std::chrono::steady_clock::time_point eof_at_{};

  // Event thread infrastructure (P2.4)
  std::thread event_thread_;
  std::atomic<bool> quit_{false};
  std::atomic<bool> paused_{false};  // Set by Pause(), cleared by Resume()
  // P3L.3: set on START_FILE/FILE_LOADED in the event thread (atomic —
  // no mutex needed). Gates the adapter-pin verify pass (PATCH A).
  std::atomic<bool> ever_started_{false};
  // Engine hidden window for PostMessage. Written once via SetMessageWindow
  // (Todo 7 wires it); nullptr until then, in which case PROPERTY_CHANGE is
  // handled inline in the event thread (preserves pre-P2.4 behavior).
  std::atomic<void*> message_hwnd_{nullptr};
  // WM_APP (0x8000) + 0x50. Posted by the event thread on hwdec-current /
  // vo-configured change; the engine WndProc (Todo 7) routes it to
  // OnHwdecPropertyChange(). Value kept in header so engine_app can use it
  // without including mpv headers.
  static constexpr unsigned kMsgHwdecChange = 0x8000u + 0x0050u;
};

}  // namespace k6wp
