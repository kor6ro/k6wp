#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <atomic>
#include <chrono>
#include <cstdarg>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>

#include "cli_options.hpp"
#include "config_watch.hpp"
#include "fullscreen_watch.hpp"
#include "ipc_server.hpp"
#include "mpv_renderer.hpp"
#include "multi_monitor.hpp"
#include "cpu_affinity.hpp"
#include "lockscreen_glue.hpp"
#include "occlusion_poke_scheduler.hpp"
#include "occlusion_watch.hpp"
#include "os_wallpaper.hpp"
#include "pin_verify_schedule.hpp"
#include "working_set_trim.hpp"
#include "pause_controller.hpp"
#include "ipc_command_marshal.hpp"
#include "playlist.hpp"
#include "playlist_controller.hpp"
#include "test_simulator.hpp"
#include "tray_actions.hpp"
#include "power.hpp"
#include "timer_ids.hpp"
#include "tray.hpp"

namespace k6wp {

// Single-instance guard (Todo 14): named mutex held for the process
// lifetime; the destructor releases it automatically. Never touched by
// Todo 15's shutdown/persistence work — release happens in the dtor,
// not in Shutdown().
struct SingletonMutexGuard {
  HANDLE handle = nullptr;
  SingletonMutexGuard() = default;
  ~SingletonMutexGuard() {
    if (handle != nullptr) {
      CloseHandle(handle);
      handle = nullptr;
    }
  }
  SingletonMutexGuard(const SingletonMutexGuard&) = delete;
  SingletonMutexGuard& operator=(const SingletonMutexGuard&) = delete;
};

// Resident wallpaper engine application.
//
// Owns the Win32 message loop, a hidden WS_POPUP top-level window that receives
// the IPC/tray/power messages, power-broadcast handling (PBT_APMSUSPEND /
// PBT_APMRESUMEAUTOMATIC), the device-lost / recreate hook points, and the
// injected per-monitor wallpaper surface (MultiMonitor).
class EngineApp {
 public:
  EngineApp() = default;
  ~EngineApp();

  EngineApp(const EngineApp&) = delete;
  EngineApp& operator=(const EngineApp&) = delete;

  // Parses CLI, registers the window class, creates the hidden window and
  // installs the Ctrl+C handler. Returns false on failure; use
  // InitExitCode() for the process exit code (0 = --help, 2 = parse error).
  bool Init(int argc, char** argv);

  // Runs the message loop until shutdown is requested (WM_CLOSE, Ctrl+C,
  // --exit-after-ms). Calls Shutdown() before returning.
  int Run();

  // Tears down the window and console handler. Idempotent; also called from
  // the destructor.
  void Shutdown();

  // Exit code to use when Init() returned false.
  int InitExitCode() const { return init_exit_code_; }

  // --- Renderer hooks (Todo 10 overrides these) ----------------------------
  // PBT_APMSUSPEND. Sets the suspend owner bit (Step 3.1); the renderer and
  // tray follow the merged mask via ApplyPauseState.
  virtual void OnSuspend();
  // PBT_APMRESUMEAUTOMATIC. Clears the suspend owner bit ONLY (Step 3.1) —
  // a user/fullscreen pause survives a sleep cycle.
  virtual void OnResume();
  // Device lost. Base: log + set the device-lost flag; the Run loop then
  // calls RecreateDevice().
  virtual void OnDeviceLost();
  // Called from the Run loop when the device-lost flag is set. Base: log +
  // no-op (Todo 10 recreates the D3D11 device here).
  virtual void RecreateDevice();
  // Display change (Todo 37). Fired from HandleMessage on WM_DISPLAYCHANGE
  // with the new resolution (LOWORD/HIWORD of lParam). Re-enumeration point:
  // once EngineApp owns a MultiMonitor (Todo 33) this calls its
  // OnDisplayChange() (diff ListMonitors vs live slots); single-window mode
  // calls DesktopInjector::OnDisplayChange(width, height) (Todo 9) instead.
  virtual void OnDisplayChange(int width, int height);

 private:
  static constexpr wchar_t kWindowClassName[] = L"K6WP.Engine.MessageWindow.1";
  static constexpr wchar_t kSingletonMutexName[] =
      L"Local\\K6WP-Engine-Singleton";
  static constexpr UINT kShutdownMessage = WM_APP + 1;
  // P2.4 (Todo 7): headless-renderer PROPERTY_CHANGE notification, posted by
  // its event thread on hwdec-current / vo-configured change. Mirrors
  // MpvRenderer::kMsgHwdecChange (WM_APP + 0x50; private there, so the value
  // is repeated here with this comment as the link — keep in sync).
  static constexpr UINT kMpvHwdecChangeMessage = WM_APP + 0x50u;
  // WM_APP allocation map (all routed in HandleMessage; keep in sync with
  // the watch headers): +1 shutdown, +0x14 tray, +0x50 mpv hwdec,
  // +0x51 fullscreen hook (FullscreenWatch::HookMessageId),
  // +0x52 occlusion re-arm (OcclusionWatch::RearmMessageId),
  // +0x53 destroy poke (FullscreenWatch::PokeMessageId, HOTFIX),
  // +0x54 queued set_monitor / +0x55 queued set_video are
  // kSetMonitorMessage / kSetVideoMessage in ipc_command_marshal.hpp.
  // Timer IDs live in timer_ids.hpp (central registry with pairwise-distinct
  // static_asserts): kWorkingSetTrimTimerId ('K6WP'+1) and
  // kOcclusionPokeTimerId ('K6WP'+2) are used here; kDebounceTimerId
  // ('K6WP') belongs to ConfigWatcher.

  static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
  static BOOL WINAPI ConsoleCtrlHandler(DWORD ctrl_type);
  static void Log(const char* fmt, ...);
  // Same as Log but flushes the batched log sink at once: use for state
  // transitions (video load, pause/resume, device-lost) that post-mortem
  // diagnostics must never miss after a taskkill.
  static void LogImportant(const char* fmt, ...);

  void HandlePowerBroadcast(WPARAM wParam, LPARAM lParam);
  LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
  bool RegisterWindowClass();
  bool CreateMessageWindow();
  void RequestShutdown();
  // CRIT-2 ack contract ("diterima" vs "selesai", LOW-15): the IPC worker
  // thread NEVER executes these — it only validates the payload
  // (ipc_marshal.hpp) and queues it via IpcCommandMarshal, which
  // PostMessageW kSetVideoMessage/kSetMonitorMessage to the hidden window.
  // The {"ok":true} ack therefore means "diterima" (accepted + queued for
  // the main loop), NOT "selesai" (applied to the desktop). A rejected
  // payload acks {"error"} and queues nothing. Clients that need certainty
  // verify via get_state (video/monitor fields reflect the applied state).
  // Window creation (DesktopInjector::Attach inside SetActiveMonitor, renderer
  // Create/LoadLoop, tray MRU, config persist) runs on the main thread only.
  // Validates the set_video payload ({"path": ...}), then hot-swaps the live
  // renderer via MpvRenderer::LoadLoop — no process restart. Returns false
  // (→ {"error"} ack, old video keeps playing) when the path is missing, not
  // a file, or the renderer rejects it. MAIN THREAD ONLY (HandleMessage /
  // tray quick-switch / Init boot path); the IPC worker path goes through
  // ValidateSetVideoPayload + IpcCommandMarshal::QueueVideo instead.
  //
  // rotation=true marks an automatic playlist rotation: the tray MRU is NOT
  // fed (it stays reserved for genuine user picks) and the lockscreen sync is
  // NOT re-fired (a rotation is not a user action), and the final success line
  // logs at Log level instead of LogImportant so a short interval cannot flush
  // the log every cycle.
  bool HandleSetVideo(const std::string& payload_json, bool rotation = false);
  // Validates the set_monitor payload ({"monitor": N}, alias
  // {"monitor_id": N}), then retargets the live slots via
  // MultiMonitor::SetActiveMonitor: -1 = all screens, >=0 = that monitor
  // only (absent id = zero slots, engine keeps running). Returns false
  // (→ {"error"} ack) unless the payload is an integer >= -1. On success the
  // new target is preserve-merged into the config file (best-effort: persist
  // failure only warns, the live state already changed). MAIN THREAD ONLY
  // (HandleMessage); the IPC worker path goes through
  // ParseSetMonitorPayload + IpcCommandMarshal::QueueMonitor instead.
  bool HandleSetMonitor(const std::string& payload_json);
  // Row 15: per-monitor assignment executor for set_display_video. Payload
  // via ParseSetDisplayVideoPayload (row 13): {"device","path"} assigns,
  // {"device","clear":true} drops the override. clear -> erase the device's
  // displays.json assignment and reload current_video_utf8_ onto that slot;
  // assign -> MultiMonitor::LoadLoopSlot onto the slot whose GDI device name
  // matches, fit mode applied to that slot only. Persisted atomically via
  // SaveDisplays (displays.json; NOT PersistConfigField — that is
  // config.json-only). Unknown device key -> false +
  // "ipc: set_display_video rejected (unknown device)" BEFORE any mutation
  // (displays.json stays byte-identical). MAIN THREAD ONLY (HandleMessage
  // kSetDisplayVideoMessage consume site); the IPC worker path validates
  // with ParseSetDisplayVideoPayload + IpcCommandMarshal::QueueDisplayVideo.
  bool HandleSetDisplayVideo(const std::string& payload_json);
  std::filesystem::path ResolvedConfigPath() const;
  // CRIT-2 main-thread halves: pop the pending value (IpcCommandMarshal)
  // and run the matching Handle* executor above. Called from HandleMessage
  // on kSetVideoMessage/kSetMonitorMessage. A lost race (flag set but empty)
  // only logs — shutdown destroys the window, discarding queued posts, so
  // no executor ever runs after teardown (Shutdown stops the IPC server
  // first, joining the worker before any surface is torn down).
  void ApplyPendingSetVideo();
  void ApplyPendingSetMonitor();
  HINSTANCE hinstance_ = nullptr;
  HWND message_hwnd_ = nullptr;
  bool class_registered_ = false;
  // Todo 14: process-lifetime single-instance mutex (acquired in Init
  // before any window/IPC/tray exists; RAII release at teardown).
  SingletonMutexGuard singleton_mutex_;
  // CRIT-1: parsed CLI snapshot. Written once in Init() (pre-Run, single
  // thread), then read from the main thread (Handle* executors, tray,
  // BuildStateJson snapshot) AND the IPC worker thread (BuildStateJson via
  // get_state). Every post-Init access takes options_mutex_; the main thread
  // copies what it needs (snapshot) and never holds the lock across window
  // creation, renderer calls, or config I/O.
  mutable std::mutex options_mutex_;
  CliOptions options_;
  // Hidden QA test flags; Run() drives it each loop.
  TestSimulator sim_;
  // P2.3: GUID_MONITOR_POWER_ON registration handle (HPOWERNOTIFY per
  // WinUser.h; Register in Init, Unregister in Shutdown — balanced, no
  // threads involved).
  HPOWERNOTIFY monitor_power_notify_ = nullptr;
  // Battery saver (Todo 40): owns the AC/DC cap logic; refs renderer_ +
  // config_watcher_ (both outlive it). Null until Init() builds it.
  std::unique_ptr<PowerSaver> power_saver_;
  // CRIT-1: read by BuildStateJson on the IPC worker thread, written by the
  // main loop (Run/RequestShutdown). Plain bool was a data race.
  std::atomic<bool> running_{false};
  bool shutdown_done_ = false;
  int init_exit_code_ = 2;
  std::atomic<bool> device_lost_{false};
  // Config file watcher (Todo 11): started in Init() from CliOptions
  // (--config, or DefaultConfigPath() when empty), polled in Run(),
  // stopped in Shutdown(). Value member is safe: config_watch.hpp is
  // Win32-free, so no incomplete-type pimpl issues.
  ConfigWatcher config_watcher_;
  // Fullscreen auto-pause (Todo 34): started in Init(), polled in Run(),
  // stopped in Shutdown(). Win32-free header, value member is safe.
  FullscreenWatch fullscreen_watch_;
  // IPC pipe server (Todo 28): started in Init(), stopped in Shutdown().
  // IpcServer declares its dtor out-of-line (pimpl), so holding it by value
  // here is safe — destruction runs inside engine_app.cpp's TU.
  IpcServer ipc_server_;
  // Live renderer for IPC set_video (Todo 30): created headless in Init()
  // (no wallpaper HWND yet — Todo 9's injector is not wired into EngineApp;
  // Create(nullptr) leaves mpv idle but LoadLoop still swaps the stream).
  // CRIT-1/CRIT-2: IPC payloads are validated on the worker and marshaled
  // onto the MAIN thread — HandleSetVideo only ever runs there (see the
  // "diterima vs selesai" queue contract above); the renderer's own mutex
  // additionally serializes against the event thread.
  // P2.4 (Todo 7): held by unique_ptr so Shutdown() can stop + join the
  // headless event thread (via the dtor: quit flag → mpv_wakeup → blocking
  // join → mpv_terminate_destroy) BEFORE ShutdownWallpaperSurface() runs.
  // Null until Init() builds it and again after Shutdown() stops it; every
  // use site null-checks.
  std::shared_ptr<MpvRenderer> renderer_;
  // Guards renderer_'s own lifetime: the IPC worker (get_state -> BuildStateJson,
  // pause/resume -> ApplyPauseState) copies it through AcquireRenderer() while
  // the main loop may reset/recreate it (pin verify, shutdown).
  mutable std::mutex renderer_mutex_;
  std::shared_ptr<MpvRenderer> AcquireRenderer() const;
  // Live desktop surface (Todo 9): per-monitor DesktopInjector + MpvRenderer
  // pairs owned by MultiMonitor. Constructed in Init() after tray install
  // (needs the log + message loop up), torn down in Shutdown(). The
  // headless renderer_ above stays as the pre-attach fallback: set_video
  // and pause/resume fan out to BOTH so either path keeps working.
  MultiMonitor multi_monitor_;
  // CRIT-1: cross-thread liveness flag. Written on the main thread
  // (InitWallpaperSurface / HandleSetMonitor executor / Shutdown), read from
  // the main loop AND the IPC worker thread (ApplyPauseState fan-out,
  // get_state paths). Plain bool was a data race.
  std::atomic<bool> wallpaper_surface_live_{false};
  // Per-monitor occlusion pause (P2.5, Todo 9): 1500 ms self-suspending
  // tick (no dedicated timer — Check runs on the unpaused-loop wake),
  // per-slot PauseSlot only, atomic arm/disarm via ApplyPauseState.
  OcclusionWatch occlusion_watch_;
  // Todo 15: OS wallpaper captured at Init, restored at Shutdown.
  OsWallpaperGuard os_wallpaper_{&EngineApp::Log};
  // Build the live surface (MultiMonitor::Init PerMonitor) + feed it the
  // current video/fit when available. Never fatal: attach failure degrades
  // to headless renderers slot-by-slot inside MultiMonitor.
  void InitWallpaperSurface(const std::string& video_utf8);
  // Reflects the wallpaper-surface health in the tray tooltip: error state
  // when any live slot is headless (injection failed), cleared on recovery.
  void UpdateTrayErrorStatus();
  // Detach all injected windows (idempotent via MultiMonitor::Shutdown).
  void ShutdownWallpaperSurface();
  // P2.4 (Todo 7): stop + join the headless renderer_ event thread at the
  // START of Shutdown(), before ShutdownWallpaperSurface(). reset() runs
  // ~MpvRenderer (quit → wakeup → blocking join with timing log, >200 ms
  // logs critical and keeps blocking, never TerminateThread, never joined
  // from inside an mpv callback) and only then destroys the mpv instance.
  void StopHeadlessRenderer();
  // P2.4 (Todo 7): state-dependent loop wait (Oracle round-03 issue 1).
  // 50 ms while a test flag is armed, INFINITE while SlotsPaused(), else
  // 1500 ms (doubles as the Todo 9 occlusion cadence — no separate timer).
  DWORD ComputeWaitTimeoutMs() const;
  // Any --exit-after-ms / --simulate-*-after-ms still needing its periodic
  // check (each simulate disarms once fired; exit-after stays armed — the
  // loop ends on it anyway). True ⇒ test run ⇒ 50 ms timeout.
  bool AnySimulateArmed() const;
  // Fan a WallpaperConfig fit_mode out to renderer_ + all live slots.
  void ApplyFitMode(const std::string& fit_mode);
  // --- Wallpaper playlist + rotation -----------------------------------------
  // playlist.json is engine-read and Studio-written; the controller reloads it
  // when its mtime/size change and fires rotation when the interval elapses
  // while NOT paused.
  PlaylistController playlist_{&EngineApp::Log};
  // P3L.3 pin verify scheduling (PATCH A): armed after every (re)load,
  // fired once post-start in the Run loop. 6 s covers vo-configured +
  // first frame settle; the pass itself requires EverStarted so a
  // transient pre-start "no" can never trigger a revert.
  void ArmPinVerify();
  void RunPinVerifyPass();
  // Headless-renderer half of the pass (only when it owns decode).
  // Returns true when it reverted (recreated unpinned + reloaded).
  bool VerifyHeadlessPin();
  // P4.1: one-shot working-set trim (loop thread only).
  WorkingSetTrim working_set_trim_{&EngineApp::Log};
  // HOTFIX (occlusion resume): debounced poke scheduler (loop thread only).
  OcclusionPokeScheduler occlusion_poke_{&EngineApp::Log};
  // Current video path (UTF-8), guarded by video_mutex_. Post-CRIT-2 the
  // executor runs on the main thread (Init() and HandleSetVideo both write
  // there; BuildStateJson reads on the main loop) — the mutex remains as
  // belt-and-braces for any worker-side reader.
  mutable std::mutex video_mutex_;
  std::string current_video_utf8_;
  // P3L: applied-at-boot config values (watcher flips log restart-required).
  std::string applied_affinity_ = "auto";
  std::string applied_adapter_mode_ = "auto";
  // P3L.3: resolved `d3d11-adapter` substring (empty = unpinned).
  std::string adapter_pin_value_;
  // P3L.3 verify scheduling (PATCH A): armed with a steady-clock deadline
  // after every (re)load; fired once by the Run loop.
  PinVerifySchedule pin_verify_;
  // True while the headless renderer_ (not the slots) owns decode: the
  // headless half of the verify pass only runs then. CRIT-1: atomic —
  // written by the main-thread set_video/boot executors, read by the
  // verify pass (main) and get_state-adjacent paths.
  std::atomic<bool> headless_owns_decode_{false};
  // CRIT-2 worker->main handoff; one slot each (last write wins, idempotent).
  IpcCommandMarshal ipc_marshal_{&EngineApp::Log};
  // Serializes the get_state payload (raw JSON object text).
  std::string BuildStateJson() const;
  // --- Tray menu actions (Todo 35, all run on the UI thread via WndProc) ----
  void OnTrayTogglePause();                 // Pause/Resume item + dbl-click
  void OnTrayQuickSwitch(std::size_t idx);  // quick-switch submenu index
  // Tray icon (Todo 35): owned by value (RAII: NIM_DELETE in dtor/Shutdown),
  // installed on the existing hidden window in Init(). Non-fatal when
  // Explorer is absent — engine keeps running, TaskbarCreated re-adds it.
  TrayIcon tray_;
  // Pause ownership now lives in PauseController; the aliases keep call sites
  // readable and UiPaused()/SlotsPaused() stay as thin forwards. Writers still
  // go through SetPauseOwner + ApplyPauseState.
  static constexpr int kPauseUser = PauseController::kUser;
  static constexpr int kPauseFullscreen = PauseController::kFullscreen;
  static constexpr int kPauseSuspend = PauseController::kSuspend;
  static constexpr int kPausePower = PauseController::kPower;
  static constexpr int kPauseSessionLock = PauseController::kSessionLock;
  static constexpr int kPauseScreenOff = PauseController::kScreenOff;
  bool UiPaused() const { return pause_.UiPaused(); }
  bool SlotsPaused() const { return pause_.SlotsPaused(); }
  // Sets/clears one owner bit (no-op when unchanged) and fans the merged
  // state out to renderer_, the live slots, and the tray.
  void SetPauseOwner(int bit, bool on);
  void ApplyPauseState(const char* owner);
  PauseController pause_;
};

}  // namespace k6wp