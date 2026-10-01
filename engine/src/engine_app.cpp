#include "engine_app.hpp"
#include "bench_hook.hpp"
#include "gpu_pin.hpp"
#include "ipc_marshal.hpp"
#include "links.hpp"
#include "lockscreen.hpp"
#include "log_file.hpp"
#include "thirdparty/json.hpp"

#include <shellapi.h>
#include <tlhelp32.h>
#include <wtsapi32.h>
#include <psapi.h>

#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace k6wp {
namespace {

std::atomic<HWND> g_message_hwnd{nullptr};

unsigned long long CurrentThreadCount() {
  const DWORD pid = GetCurrentProcessId();
  HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
  if (snap == INVALID_HANDLE_VALUE) return 0;
  unsigned long long n = 0;
  THREADENTRY32 te{};
  te.dwSize = sizeof(te);
  if (Thread32First(snap, &te)) {
    do {
      if (te.th32OwnerProcessID == pid) ++n;
    } while (Thread32Next(snap, &te));
  }
  CloseHandle(snap);
  return n;
}

unsigned long long CurrentHandleCount() {
  DWORD n = 0;
  if (!GetProcessHandleCount(GetCurrentProcess(), &n)) return 0;
  return static_cast<unsigned long long>(n);
}

// P2.3: --simulate-monitor-off-after-ms payload. Built by value at the call
// site: SendMessageW is synchronous, so a stack buffer stays valid for the
// whole dispatch and no static state (or its lazy init) is needed.
struct MonitorPowerSetting {
  POWERBROADCAST_SETTING base;
  BYTE pad[3];  // base.Data is UCHAR[1]; room for the DWORD payload
};

void InitMonitorPowerSetting(MonitorPowerSetting& setting, DWORD data) {
  setting = {};
  setting.base.PowerSetting = GUID_MONITOR_POWER_ON;
  setting.base.DataLength = sizeof(DWORD);
  std::memcpy(setting.base.Data, &data, sizeof(data));
}

}  // namespace

EngineApp::~EngineApp() { Shutdown(); }

bool EngineApp::Init(int argc, char** argv) {
  const int parse_rc = ParseCli(argc, argv, options_);
  if (parse_rc != 0) {
    init_exit_code_ = (parse_rc == 1) ? 0 : 2;
    return false;
  }
  sim_.Configure(options_);

  // Todo 14: single-instance guard, acquired BEFORE any window, IPC pipe,
  // or tray icon exists and held for the process lifetime via the RAII
  // member (released automatically at teardown, never in Shutdown()). A
  // duplicate logs + exits 0 without binding a second pipe.
  singleton_mutex_.handle =
      CreateMutexW(nullptr, FALSE, kSingletonMutexName);
  if (singleton_mutex_.handle == nullptr) {
    Log("warning: CreateMutexW(%ls) failed (error %lu), continuing without singleton guard",
        kSingletonMutexName, GetLastError());
  } else if (GetLastError() == ERROR_ALREADY_EXISTS) {
    Log("engine: another instance is already running (mutex %ls held), exiting",
        kSingletonMutexName);
    CloseHandle(singleton_mutex_.handle);
    singleton_mutex_.handle = nullptr;
    init_exit_code_ = 0;
    return false;
  }

  // Todo 15: snapshot the active OS wallpaper path BEFORE the engine touches
  // the desktop (adjacent to the Todo 14 mutex block; that block is untouched).
  // Never fatal: an unreadable path logs + Init proceeds WITHOUT restore.
  os_wallpaper_.Save();

  hinstance_ = GetModuleHandleW(nullptr);
  if (!RegisterWindowClass()) return false;
  if (!CreateMessageWindow()) return false;

  // WER auto-restart: relaunch with "--restarted" after an unhandled crash.
  // ParseCli ignores that flag, so the restarted instance boots normally.
  // Never fatal: a failed registration only logs.
  const HRESULT restart_hr =
      RegisterApplicationRestart(L"--restarted", 0);
  if (FAILED(restart_hr)) {
    Log("warning: RegisterApplicationRestart failed (hr=0x%08lX)",
        static_cast<unsigned long>(restart_hr));
  }

  // Session-lock pause: WM_WTSSESSION_CHANGE needs a top-level window.
  // CreateMessageWindow builds a hidden WS_POPUP top-level window (never
  // shown, so no taskbar/Alt+Tab entry), which receives WTS notifications —
  // a message-only HWND_MESSAGE window would not.
  if (!WTSRegisterSessionNotification(message_hwnd_, NOTIFY_FOR_THIS_SESSION)) {
    Log("warning: WTSRegisterSessionNotification failed (error %lu), session-lock pause disabled",
        GetLastError());
  }

  // P2.3: monitor power-off notifications (GUID_MONITOR_POWER_ON). Never
  // fatal: a failed registration only logs; suspend/resume + battery paths
  // stay live and the PBT_POWERSETTINGCHANGE handler ignores unknown GUIDs.
  monitor_power_notify_ =
      RegisterPowerSettingNotification(message_hwnd_, &GUID_MONITOR_POWER_ON, 0);
  if (monitor_power_notify_ == nullptr) {
    Log("warning: RegisterPowerSettingNotification(GUID_MONITOR_POWER_ON) failed (error %lu), monitor-off pause disabled",
        GetLastError());
  } else {
    Log("engine init: monitor power notifications registered (GUID_MONITOR_POWER_ON)");
  }

  if (!SetConsoleCtrlHandler(&ConsoleCtrlHandler, TRUE)) {
    Log("warning: SetConsoleCtrlHandler failed (error %lu)", GetLastError());
  }
  g_message_hwnd.store(message_hwnd_);

  Log("engine init: video='%ls' config='%ls' wallpaper-mode=%s exit-after-ms=%d",
      options_.video_path.c_str(), options_.config_path.c_str(),
      WallpaperModeToString(options_.wallpaper_mode), sim_.exit_after_ms());
  if (options_.minimized) {
    // The engine never shows a window (message-only HWND + tray icon only),
    // so --minimized is a no-op marker from the HKCU Run entry — logged so an
    // autostart launch is distinguishable from an interactive one.
    Log("engine init: --minimized flag set, tray-only start (no window shown)");
  }

  // Todo 11: start the config watcher from CliOptions. Empty --config falls
  // back to %LOCALAPPDATA%/K6WP/config.json. Corrupt JSON keeps the
  // last-valid config (logged, never fatal).
  const std::filesystem::path config_path = ResolvedConfigPath();
  config_watcher_.Start(config_path, [this](const WallpaperConfig& cfg) {
    Log("config: active (video='%ls' fit=%s speed=%.1f monitor=%d)",
        cfg.video_path.c_str(), cfg.fit_mode.c_str(), cfg.speed,
        cfg.monitor_id);
    // Todo 40: a battery_saver/fps_cap flip applies on the next Update.
    if (power_saver_) power_saver_->Update();
    ApplyFitMode(cfg.fit_mode);
    // P3L: affinity/adapter apply at boot only (process mask / VO init).
    // A live flip logs restart-required instead of half-applying.
    if (cfg.cpu_affinity != applied_affinity_) {
      Log("config: cpu_affinity changed '%s' -> '%s', restart required",
          applied_affinity_.c_str(), cfg.cpu_affinity.c_str());
    }
    if (cfg.gpu_adapter != applied_adapter_mode_) {
      Log("config: gpu_adapter changed '%s' -> '%s', restart required",
          applied_adapter_mode_.c_str(), cfg.gpu_adapter.c_str());
    }
  });

  // P3L.2: E-core affinity from the live config (process-level, once at
  // boot; a later config flip only logs restart-required in the watcher).
  applied_affinity_ = config_watcher_.GetConfig().cpu_affinity;
  ApplyCpuAffinity(applied_affinity_, &EngineApp::Log);

  // P3L.3: adapter pin decision (once at boot; flips log restart-required).
  // Empty = unpinned (single-adapter rig or explicit no-match). The value
  // is a DXGI Description substring for mpv `d3d11-adapter` (format
  // verified against this build's logs, never assumed).
  applied_adapter_mode_ = config_watcher_.GetConfig().gpu_adapter;
  adapter_pin_value_ = ResolveAdapterPin(applied_adapter_mode_);
  Log("gpu-pin: adapters: %s", DescribeAdapters().c_str());
  Log("gpu-pin: mode=%s pin='%s'", applied_adapter_mode_.c_str(),
      adapter_pin_value_.empty() ? "(none)" : adapter_pin_value_.c_str());

  // Todo 30: create the live renderer headless. Hand it the hidden message
  // HWND as mpv's `wid` host: Create(nullptr) makes mpv spawn its OWN
  // top-level WS_OVERLAPPEDWINDOW as soon as LoadLoop runs (a taskbar/Alt+Tab
  // entry, plan Must-NOT-Have), whereas embedding into the never-shown
  // message window keeps this renderer truly windowless (Todo 2). A failed
  // Create() is non-fatal: set_video then acks {"error"} while the engine
  // keeps running (restart path stays as the fallback).
  // P2.4 (Todo 7): wire the hidden window for PROPERTY_CHANGE routing — the
  // headless event thread PostMessages hwdec-current / vo-configured changes
  // here and the loop thread runs OnHwdecPropertyChange (moved fallback
  // logic). Slot renderers keep the inline path (no MultiMonitor API change
  // in this todo). SetMessageWindow also wakes the thread so a queued change
  // is picked up even from its -1 paused block.
  renderer_ = std::make_shared<MpvRenderer>();
  if (!adapter_pin_value_.empty()) renderer_->SetAdapterPin(adapter_pin_value_);
  if (!renderer_->Create(message_hwnd_)) {
    Log("warning: MpvRenderer::Create failed, set_video will reject until restart");
  } else {
    renderer_->SetMessageWindow(message_hwnd_);
  }
  {
    std::lock_guard<std::mutex> lock(video_mutex_);
    current_video_utf8_ = std::filesystem::path(options_.video_path).u8string();
  }

  // Todo 34: fullscreen auto-pause. Poll() fires the callback only on
  // transitions; Pause()/Resume() are mutex-serialized in MpvRenderer.
  // P2.2 (Todo 4): the primary trigger is the SetWinEventHook event path
  // (foreground + minimize hooks -> PostMessage -> OnHookEvent, same
  // detection + 2-confirm debounce); Poll() below stays as the 1 s fallback.
  // Todo 6: fan out to the live wallpaper surface too — the visible decode
  // lives in the MultiMonitor slots, not the headless renderer_ — and mirror
  // the pause in the mask/tray so get_state reports the truth (Step 3.1:
  // the fullscreen owner bit, merged with user/suspend/power).
  fullscreen_watch_.Start([this](bool fullscreen) {
    if (fullscreen) {
      LogImportant("fullscreen: pausing decode");
      SetPauseOwner(kPauseFullscreen, true);
    } else {
      LogImportant("fullscreen: resuming decode");
      SetPauseOwner(kPauseFullscreen, false);
    }
  });

  // Todo 40: battery saver. The reader serves the forced-DC override while
  // the hidden --simulate-dc-after-ms test flag is latched, else the live
  // GetSystemPowerStatus query merged with the current config. The initial
  // Update() logs the AC/inert path at startup on every run.
  // Todo 6: the cap applier fans out to the headless renderer_ AND the live
  // MultiMonitor slots (the visible decode). The slots have no fps-cap API
  // (MultiMonitor is T3-owned), so the DC throttle mirrors as the power
  // owner bit (Step 3.1: the mask pauses/resumes the slots, never a direct
  // PauseAll/ResumeAll call from here).
  power_saver_ = std::make_unique<PowerSaver>(
      [this](PowerSaverState state, int fps) {
        // P2.7 static: battery_mode is consulted only on the DC-capped path
        // (PowerSaver emits kDcCapped solely when the saver is enabled and
        // the box is on DC). Slot decode stops via the reused kPausePower
        // bit (last frame shown); the fps-cap application is skipped.
        if (state == PowerSaverState::kDcCapped &&
            config_watcher_.GetConfig().battery_mode == "static") {
          Log("power-saver: DC power (battery), static pause (slots held)");
          SetPauseOwner(kPausePower, true);
        } else if (state == PowerSaverState::kDcCapped) {
          if (renderer_) renderer_->SetFpsCap(24);
          SetPauseOwner(kPausePower, true);
        } else {
          if (renderer_) renderer_->SetFpsCap(fps);
          SetPauseOwner(kPausePower, false);
        }
      },
      [this]() -> PowerReading {
        const WallpaperConfig cfg = config_watcher_.GetConfig();
        if (sim_.dc_latched()) {
          PowerReading forced;
          forced.ac_online = false;
          forced.ac_unknown = false;
          forced.has_battery = true;
          forced.battery_saver_on = cfg.battery_saver;
          forced.fps_restore = cfg.fps_cap;
          return forced;
        }
        return ReadSystemPower(cfg);
      },
      [](const std::string& message) { Log("power-saver: %s", message.c_str()); });
  power_saver_->Update();

  // Todo 28: start the overlapped IPC pipe server. pause / resume own the
  // user bit (Step 3.1), set_video / set_monitor VALIDATE on the worker and
  // queue for the main loop (CRIT-2: Queue* posts a private UINT to the
  // hidden window; the {"ok":true} ack means "diterima", verified later via
  // get_state), get_state reports the live engine state.
  IpcHandlers handlers;
  handlers.set_video = [this](const std::string& payload) {
    return QueueSetVideo(payload);
  };
  handlers.set_monitor = [this](const std::string& payload) {
    return QueueSetMonitor(payload);
  };
  handlers.pause = [this]() { SetPauseOwner(kPauseUser, true); };
  handlers.resume = [this]() { SetPauseOwner(kPauseUser, false); };
  handlers.get_state = [this]() { return BuildStateJson(); };
  handlers.quit = []() {
    FlushEngineLog();  // drain the batch before teardown starts
    if (HWND hwnd = g_message_hwnd.load()) {
      PostMessageW(hwnd, kShutdownMessage, 0, 0);
    }
  };
  if (!ipc_server_.Start(handlers)) {
    Log("warning: IPC server failed to start, continuing without IPC");
  }

  // Todo 35: tray icon on the existing message-only window (no second
  // window). Install is non-fatal: without Explorer NIM_ADD fails but the
  // engine keeps running, and TaskbarCreated re-adds the icon later.
  TrayCallbacks tray_cb;
  tray_cb.on_toggle_pause = [this]() { OnTrayTogglePause(); };
  tray_cb.on_quick_switch = [this](std::size_t idx) { OnTrayQuickSwitch(idx); };
  tray_cb.get_current = [this]() {
    std::lock_guard<std::mutex> lock(video_mutex_);
    return current_video_utf8_;
  };
  tray_cb.on_next = [this](std::size_t idx) {
    // Playlist active: Next advances within the playlist (restarting the
    // interval); otherwise the legacy MRU rotation applies.
    if (playlist_.enabled && !playlist_.order.empty()) {
      FireRotation();
    } else {
      OnTrayQuickSwitch(idx);
    }
  };
  tray_cb.on_open_studio = [this]() { OnTrayOpenStudio(); };
  tray_cb.on_support = [this]() { OnTraySupport(); };
  tray_cb.on_exit = [this]() { RequestShutdown(); };
  tray_cb.is_paused = [this]() { return pause_.UiPaused(); };
  tray_.Install(message_hwnd_, std::move(tray_cb));
  std::string boot_video;
  {
    std::lock_guard<std::mutex> lock(video_mutex_);
    // Seed video path from config if --video was not provided (boot-seed fix).
    // config_watcher_.Start() loads synchronously, so GetConfig() is valid here.
    // CRIT-1: the IPC worker is already up (get_state snapshots options_),
    // so this post-Start access takes options_mutex_ like every other.
    std::lock_guard<std::mutex> opts_lock(options_mutex_);
    if (options_.video_path.empty()) {
      options_.video_path = config_watcher_.GetConfig().video_path;
    }
    // --video was provided: use that path regardless of config.
    current_video_utf8_ = std::filesystem::path(options_.video_path).u8string();
    boot_video = current_video_utf8_;
    if (!boot_video.empty()) tray_.PushRecent(boot_video);
  }
  const bool boot_autoplay =
      !boot_video.empty() &&
      std::filesystem::exists(std::filesystem::u8path(boot_video));
  // Adopt the live desktop surface BEFORE the headless fallback decision:
  // InitWallpaperSurface loads the boot video into the visible slots itself,
  // so a headless LoadLoop here would decode the same file twice (~500MB on
  // 4K). Still AFTER tray/IPC are up (Todo 9: surface logging needs them).
  InitWallpaperSurface(boot_video);
  if (boot_autoplay) {
    headless_owns_decode_.store(
        !wallpaper_surface_live_.load(std::memory_order_acquire),
        std::memory_order_release);
    ArmPinVerify();  // P3L.3: verify pin post-start (PATCH A)
    if (!wallpaper_surface_live_.load(std::memory_order_acquire)) {
      Log("engine: boot autoplay '%s' — no live surface, starting headless renderer",
          boot_video.c_str());
      if (!renderer_ || !renderer_->LoadLoop(boot_video)) {
        Log("warning: renderer could not load '%s', keeping current video",
            boot_video.c_str());
      } else {
        MaybeTriggerLockscreenSync(boot_video);
      }
    } else {
      Log("engine: boot autoplay '%s' — slots own decode, headless stays idle",
          boot_video.c_str());
      MaybeTriggerLockscreenSync(boot_video);
    }
  } else {
    // No valid video at boot: seed paused state so tray shows off icon
    // (Step 3.1: the user owner bit — a later set_video does not clear it,
    // matching the old tray-paused boot behavior).
    SetPauseOwner(kPauseUser, true);
  }
  // Load playlist.json (missing = rotation disabled) and seed the resume-edge
  // detector with the boot pause state.
  MaybeReloadPlaylist();
  was_slots_paused_ = SlotsPaused();
  return true;
}

void EngineApp::InitWallpaperSurface(const std::string& video_utf8) {
  multi_monitor_ = MultiMonitor(&EngineApp::Log);
  // Todo 2: slots whose DesktopInjector::Attach fails embed mpv into this
  // never-shown window instead of spawning mpv's own top-level framed window.
  multi_monitor_.SetHeadlessHost(message_hwnd_);
  // P3L.3: slot adapter pin (VO-init level per slot, PATCH A).
  multi_monitor_.SetAdapterPin(adapter_pin_value_);
  // Step 5: honor the configured monitor target (-1 = all screens).
  multi_monitor_.SetActiveMonitor(config_watcher_.GetConfig().monitor_id);
  // Step 4: the --wallpaper-mode flag actually drives the injector now.
  // CRIT-1: snapshot under options_mutex_ (post-Start: the worker is up).
  WallpaperMode wallpaper_mode_snapshot = WallpaperMode::kAuto;
  {
    std::lock_guard<std::mutex> lock(options_mutex_);
    wallpaper_mode_snapshot = options_.wallpaper_mode;
  }
  InjectMode inject_mode = InjectMode::kAuto;  switch (wallpaper_mode_snapshot) {
    case WallpaperMode::kWorkerW:
      inject_mode = InjectMode::kWorkerW;
      break;
    case WallpaperMode::kProgman:
      inject_mode = InjectMode::kProgman;
      break;
    case WallpaperMode::kAuto:
    default:
      break;
  }
  multi_monitor_.SetInjectMode(inject_mode);
  wallpaper_surface_live_.store(
      multi_monitor_.Init(MultiMonitorMode::PerMonitor),
      std::memory_order_release);
  Log("engine: wallpaper surface init (slots=%llu live=%s)",
      static_cast<unsigned long long>(multi_monitor_.slot_count()),
      wallpaper_surface_live_.load(std::memory_order_acquire) ? "yes" : "no");
  ApplyFitMode(config_watcher_.GetConfig().fit_mode);
  if (!video_utf8.empty()) {
    if (!multi_monitor_.LoadLoopAll(video_utf8)) {
      Log("warning: wallpaper surface could not load '%s'", video_utf8.c_str());
    }
  }
  UpdateTrayErrorStatus();
}

void EngineApp::UpdateTrayErrorStatus() {
  tray_.SetError(multi_monitor_.has_headless_slots() ||
                 !wallpaper_surface_live_.load(std::memory_order_acquire));
}

void EngineApp::ShutdownWallpaperSurface() {
  if (!wallpaper_surface_live_.load(std::memory_order_acquire)) return;
  wallpaper_surface_live_.store(false, std::memory_order_release);
  // Slot event threads join inside this call (see Shutdown() order note):
  // each renderer's dtor stops its own thread before destroying mpv_.
  multi_monitor_.Shutdown();
  Log("engine: wallpaper surface detached");
}

std::shared_ptr<MpvRenderer> EngineApp::AcquireRenderer() const {
  std::lock_guard<std::mutex> lock(renderer_mutex_);
  return renderer_;
}

void EngineApp::StopHeadlessRenderer() {
  std::shared_ptr<MpvRenderer> doomed;
  {
    std::lock_guard<std::mutex> lock(renderer_mutex_);
    doomed = std::move(renderer_);
    renderer_.reset();
  }
  if (!doomed) return;
  // Signal first so a thread parked in its -1 paused block observes quit
  // promptly (StopEventThread re-wakes internally; this explicit Wakeup is
  // the documented signal step). Releasing the last reference then joins with
  // timing logs, outside renderer_mutex_ so a worker copy cannot deadlock.
  doomed->Wakeup();
  doomed.reset();
  LogImportant("engine shutdown: headless renderer event thread stopped (join timing above)");
}

bool EngineApp::AnySimulateArmed() const { return sim_.AnyArmed(); }

DWORD EngineApp::ComputeWaitTimeoutMs() const {
  // 50 ms ONLY while a test flag is armed (never a permanent short sleep);
  // INFINITE while SlotsPaused() (zero periodic wakeups when paused);
  // 1500 ms while unpaused (Todo 9 occlusion cadence hook).
  if (AnySimulateArmed()) return 50;
  if (SlotsPaused()) return INFINITE;
  return 1500;
}

void EngineApp::ApplyFitMode(const std::string& fit_mode) {
  if (renderer_) renderer_->SetFitMode(fit_mode);
  if (wallpaper_surface_live_.load(std::memory_order_acquire))
    multi_monitor_.ApplyFitModeAll(fit_mode);
  LogImportant("video: fit mode applied '%s'", fit_mode.c_str());
}

std::filesystem::path EngineApp::ResolvePlaylistPath() const {
  std::lock_guard<std::mutex> lock(options_mutex_);
  if (!options_.config_path.empty()) {
    return PlaylistPathForConfig(std::filesystem::path(options_.config_path));
  }
  return DefaultPlaylistPath();
}

void EngineApp::MaybeReloadPlaylist() {
  std::filesystem::path path;
  try {
    path = ResolvePlaylistPath();
  } catch (const ConfigError& e) {
    if (!playlist_missing_logged_) {
      Log("playlist: cannot resolve path (%s), rotation disabled", e.what());
      playlist_missing_logged_ = true;
    }
    return;
  }
  std::error_code ec;
  const auto mtime = std::filesystem::last_write_time(path, ec);
  if (ec) {
    // No file: treat as "no playlist" and disarm. A later create re-arms.
    if (playlist_mtime_valid_) {
      playlist_mtime_valid_ = false;
      playlist_ = PlaylistConfig{};
      playlist_enabled_atomic_.store(false, std::memory_order_release);
      playlist_size_atomic_.store(0, std::memory_order_release);
      playlist_index_atomic_.store(-1, std::memory_order_release);
      rotate_armed_.store(false, std::memory_order_release);
      Log("playlist: file removed, rotation disabled");
    } else if (!playlist_missing_logged_) {
      Log("playlist: no file at %s, rotation disabled", path.string().c_str());
      playlist_missing_logged_ = true;
    }
    return;
  }
  const auto size = std::filesystem::file_size(path, ec);
  if (ec) return;
  if (playlist_mtime_valid_ && mtime == playlist_mtime_ &&
      size == playlist_size_) {
    return;
  }
  playlist_mtime_ = mtime;
  playlist_size_ = size;
  playlist_mtime_valid_ = true;
  playlist_missing_logged_ = false;
  try {
    playlist_ = LoadPlaylist(path);
  } catch (const ConfigError& e) {
    Log("playlist: reload failed (keeping last-valid): %s", e.what());
    return;
  }
  std::string current;
  {
    std::lock_guard<std::mutex> lock(video_mutex_);
    current = current_video_utf8_;
  }
  const int idx = PlaylistIndexForPath(
      playlist_.order, std::filesystem::u8path(current).wstring());
  playlist_enabled_atomic_.store(playlist_.enabled, std::memory_order_release);
  playlist_size_atomic_.store(static_cast<long long>(playlist_.order.size()),
                              std::memory_order_release);
  playlist_index_atomic_.store(idx, std::memory_order_release);
  Log("playlist: loaded %llu entries (enabled=%d interval_min=%d shuffle=%d)",
      static_cast<unsigned long long>(playlist_.order.size()),
      playlist_.enabled ? 1 : 0, playlist_.interval_min,
      playlist_.shuffle ? 1 : 0);
  ArmRotation();
}

void EngineApp::ArmRotation() {
  if (!playlist_.enabled || playlist_.order.size() < 2) {
    rotate_armed_.store(false, std::memory_order_release);
    return;
  }
  const auto deadline = std::chrono::steady_clock::now() +
                        std::chrono::minutes(playlist_.interval_min);
  {
    std::lock_guard<std::mutex> lock(state_mutex_);
    next_rotate_at_ = deadline;
  }
  rotate_armed_.store(true, std::memory_order_release);
}

bool EngineApp::FireRotation() {
  const std::size_t n = playlist_.order.size();
  if (!playlist_.enabled || n < 2) {
    rotate_armed_.store(false, std::memory_order_release);
    return false;
  }
  std::string current;
  {
    std::lock_guard<std::mutex> lock(video_mutex_);
    current = current_video_utf8_;
  }
  // Unknown current (e.g. a manual apply not in the playlist) starts the walk
  // from the end so the first SelectNextIndex lands on entry 0.
  int idx = PlaylistIndexForPath(playlist_.order,
                                 std::filesystem::u8path(current).wstring());
  if (idx < 0) idx = static_cast<int>(n) - 1;
  for (std::size_t attempt = 0; attempt < n; ++attempt) {
    const std::size_t next =
        SelectNextIndex(static_cast<std::size_t>(idx), n, playlist_.shuffle,
                        rotate_rng_);
    idx = static_cast<int>(next);
    const std::wstring& cand = playlist_.order[next];
    std::error_code ec;
    if (!std::filesystem::is_regular_file(cand, ec)) {
      Log("playlist: skipping missing entry [%llu] %s",
          static_cast<unsigned long long>(next),
          std::filesystem::path(cand).u8string().c_str());
      continue;
    }
    const std::string utf8 = std::filesystem::path(cand).u8string();
    if (HandleSetVideo(nlohmann::json{{"path", utf8}}.dump(), /*rotation=*/true)) {
      playlist_index_atomic_.store(static_cast<long long>(next),
                                   std::memory_order_release);
      Log("playlist: rotated to [%llu/%llu] %s",
          static_cast<unsigned long long>(next),
          static_cast<unsigned long long>(n), utf8.c_str());
      ArmRotation();
      return true;
    }
  }
  Log("playlist: no playable entry found, keeping current video");
  ArmRotation();  // back off to the next interval instead of spinning
  return false;
}

void EngineApp::ArmPinVerify() {
  // No pin anywhere (or nothing loaded yet) = nothing to verify.
  if (adapter_pin_value_.empty()) return;
  {
    std::lock_guard<std::mutex> lock(state_mutex_);
    pin_verify_at_ = std::chrono::steady_clock::now() + std::chrono::seconds(6);
  }
  pin_verify_armed_.store(true, std::memory_order_release);
}
void EngineApp::RunPinVerifyPass() {
  pin_verify_armed_.store(false, std::memory_order_release);
  if (adapter_pin_value_.empty()) return;
  int reverted = 0;
  if (wallpaper_surface_live_.load(std::memory_order_acquire)) {
    reverted += multi_monitor_.VerifyPinAndRevert();
    if (reverted > 0) {
      multi_monitor_.ApplyFitModeAll(config_watcher_.GetConfig().fit_mode);
      ArmPinVerify();  // re-verify the fresh unpinned renderers next pass
    }
  }
  if (VerifyHeadlessPin()) ++reverted;
  int reverted_total = 0;
  {
    std::lock_guard<std::mutex> lock(state_mutex_);
    pin_reverted_total_ += reverted;
    reverted_total = pin_reverted_total_;
  }
  Log("gpu-pin: verify pass done reverted=%d total=%d", reverted,
      reverted_total);
}

bool EngineApp::VerifyHeadlessPin() {
  // Headless half: only when it owns decode, was created WITH pin, has
  // started, and still reports hwdec inactive. Recreate = teardown →
  // Create without pin → LoadLoop → re-verify next pass (PATCH A).
  std::string video;
  {
    std::lock_guard<std::mutex> lock(video_mutex_);
    video = current_video_utf8_;
  }
  // Hold renderer_mutex_ across the checks + recreate so a concurrent worker
  // copy (get_state / pause) can never observe a half-torn renderer.
  std::lock_guard<std::mutex> renderer_lock(renderer_mutex_);
  if (!renderer_ ||
      !headless_owns_decode_.load(std::memory_order_acquire))
    return false;
  if (!renderer_->pin_active()) return false;
  if (!renderer_->EverStarted()) return false;
  if (renderer_->IsHwdecActive()) return false;  // d3d11va OR dxva2: fine
  LogImportant("gpu-pin: headless hwdec inactive post-start with pin, "
               "reverting to unpinned");
  const bool was_paused = UiPaused();
  renderer_.reset();
  renderer_ = std::make_shared<MpvRenderer>();
  if (!renderer_->Create(message_hwnd_)) {
    Log("warning: gpu-pin: headless unpinned recreate failed");
    return false;
  }
  renderer_->SetMessageWindow(message_hwnd_);
  if (!video.empty()) renderer_->LoadLoop(video);
  if (was_paused) renderer_->Pause();
  ArmPinVerify();
  return true;
}

int EngineApp::Run() {
  running_.store(true, std::memory_order_release);
  const auto start = std::chrono::steady_clock::now();
  Log("engine run: message loop started (pid=%lu)", GetCurrentProcessId());

  MSG msg{};
  bool first_iteration = true;
  while (running_.load(std::memory_order_acquire)) {
    // Bench placeholder (Todo 12): emit Engine:FirstFrame on the first
    // message-loop iteration. Todo 10 moves this to the first real present.
    if (first_iteration) {
      first_iteration = false;
      MarkFirstFrame();
      working_set_trim_.Arm(message_hwnd_);  // P4.1: one-shot trim ~2 s after first frame
    }
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
      if (msg.message == WM_QUIT) {
        running_.store(false, std::memory_order_release);
        break;
      }
      TranslateMessage(&msg);
      DispatchMessageW(&msg);
    }
    if (!running_.load(std::memory_order_acquire)) break;

    const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                std::chrono::steady_clock::now() - start)
                                .count();

    const TestSimulator::Fired fired = sim_.Tick(elapsed_ms);
    if (fired.device_lost) {
      Log("test: --simulate-device-lost-after-ms=%d reached, firing OnDeviceLost()",
          sim_.device_lost_after_ms());
      OnDeviceLost();
    }
    if (fired.suspend) {
      Log("test: --simulate-suspend-after-ms=%d reached, posting PBT_APMSUSPEND",
          sim_.suspend_after_ms());
      PostMessageW(message_hwnd_, WM_POWERBROADCAST, PBT_APMSUSPEND, 0);
    }
    if (fired.resume) {
      Log("test: posting PBT_APMRESUMEAUTOMATIC");
      PostMessageW(message_hwnd_, WM_POWERBROADCAST, PBT_APMRESUMEAUTOMATIC, 0);
    }
    if (fired.dc_on) {
      Log("test: --simulate-dc-after-ms=%d reached, latching forced DC + posting PBT_APMPOWERSTATUSCHANGE",
          sim_.dc_after_ms());
      PostMessageW(message_hwnd_, WM_POWERBROADCAST, PBT_APMPOWERSTATUSCHANGE, 0);
    }
    if (fired.dc_restore) {
      Log("test: clearing DC override, posting PBT_APMPOWERSTATUSCHANGE (AC restore)");
      PostMessageW(message_hwnd_, WM_POWERBROADCAST, PBT_APMPOWERSTATUSCHANGE, 0);
    }
    // P2.3: monitor-off QA sequence — delivers the SAME PBT_POWERSETTINGCHANGE
    // off/on pair the OS would send, so the real parse path is exercised
    // headless (off at N via Data=0, on at N+2s via Data=1).
    if (fired.monitor_off) {
      Log("test: --simulate-monitor-off-after-ms=%d reached, sending PBT_POWERSETTINGCHANGE Data=0 (monitor off)",
          sim_.monitor_off_after_ms());
      // WM_POWERBROADCAST is sync-only: PostMessageW fails with
      // ERROR_MESSAGE_SYNC_ONLY (1159). SendMessageW to our own loop thread
      // dispatches synchronously, exactly like the OS broadcast.
      MonitorPowerSetting off_setting{};
      InitMonitorPowerSetting(off_setting, 0);
      SendMessageW(message_hwnd_, WM_POWERBROADCAST, PBT_POWERSETTINGCHANGE,
                   reinterpret_cast<LPARAM>(&off_setting.base));
    }
    if (fired.monitor_on) {
      Log("test: sending PBT_POWERSETTINGCHANGE Data=1 (monitor on)");
      MonitorPowerSetting on_setting{};
      InitMonitorPowerSetting(on_setting, 1);
      SendMessageW(message_hwnd_, WM_POWERBROADCAST, PBT_POWERSETTINGCHANGE,
                   reinterpret_cast<LPARAM>(&on_setting.base));
    }
    if (device_lost_.exchange(false)) {
      Log("device-lost: flag set, calling RecreateDevice()");
      RecreateDevice();
    }
    // P3L.3 pin verify pass (PATCH A): fires once ~6 s after every
    // (re)load, gated on EverStarted inside the pass itself.
    bool fire_pin_verify = false;
    if (pin_verify_armed_.load(std::memory_order_acquire)) {
      std::lock_guard<std::mutex> lock(state_mutex_);
      fire_pin_verify =
          std::chrono::steady_clock::now() >= pin_verify_at_;
    }
    if (fire_pin_verify) {
      RunPinVerifyPass();
    }
    if (sim_.ExitReached(elapsed_ms)) {
      Log("test: --exit-after-ms=%d reached, requesting shutdown", sim_.exit_after_ms());
      RequestShutdown();
      break;
    }
    // Todo 11: poll the config file (internally throttled to 500 ms).
    // P2.1 (Todo 2): Poll() is the FALLBACK path only — the primary trigger
    // is the config dir-watch event joined into the wait array below.
    config_watcher_.Poll();
    // Wallpaper playlist: reload on file change; rotate when due. While paused
    // the loop wait is INFINITE, so this block does not run (zero-wakeup budget
    // preserved) and the resume edge below restarts the full interval.
    MaybeReloadPlaylist();
    const bool slots_paused_now = SlotsPaused();
    if (was_slots_paused_ && !slots_paused_now) {
      ArmRotation();
    }
    was_slots_paused_ = slots_paused_now;
    if (!slots_paused_now && rotate_armed_.load(std::memory_order_acquire)) {
      bool rotate_due = false;
      {
        std::lock_guard<std::mutex> lock(state_mutex_);
        rotate_due = std::chrono::steady_clock::now() >= next_rotate_at_;
      }
      if (rotate_due) FireRotation();
    }
    // Todo 34: foreground-window check (internally throttled to 1 s).
    // P2.2 (Todo 4): Poll() is the FALLBACK path only — the primary trigger
    // is the WinEvent hook event (foreground/minimize -> OnHookEvent).
    fullscreen_watch_.Poll();
    // P2.4 (Todo 7): NO Tick()/TickAll() — the EOF watchdog + hwdec events
    // live in each renderer's event thread (Todo 6). The Poll() fallbacks
    // above stay (Todos 2-4 own them).
    // P2.5 (Todo 9): per-monitor occlusion pause. Runs on the unpaused
    // 1500 ms cadence only — Check() throttles internally and early-outs
    // while disarmed, and the loop wait below is INFINITE while
    // SlotsPaused(), so a paused engine spends zero wakeups here. No new
    // loop timer; per-slot PauseSlot only (never global bits).
    if (wallpaper_surface_live_.load(std::memory_order_acquire))
      occlusion_watch_.Check(multi_monitor_);
    // Idle-efficient wait with a STATE-DEPENDENT timeout (Oracle round-03
    // issue 1): 50 ms while a test flag is armed, INFINITE while
    // SlotsPaused() (wake only on posted messages / overlapped-FS event /
    // hook callbacks — zero periodic wakeups when paused), 1500 ms while
    // unpaused (doubles as the Todo 9 occlusion cadence; no separate
    // occlusion timer in this loop).
    // P2.1 (Todo 2): the config dir-watch completion event JOINS the wait
    // array (extended, not polled) so a config.json write wakes the loop via
    // the EVENT path (event=rdevchange, ≈ debounce, well under 500 ms).
    HANDLE wait_handles[1] = {};
    DWORD wait_count = 0;
    if (void* cfg_event = config_watcher_.EventHandle()) {
      wait_handles[0] = static_cast<HANDLE>(cfg_event);
      wait_count = 1;
    }
    const DWORD wait_result = MsgWaitForMultipleObjects(
        wait_count, wait_count > 0 ? wait_handles : nullptr, FALSE,
        ComputeWaitTimeoutMs(), QS_ALLINPUT);
    if (wait_count > 0 && wait_result == WAIT_OBJECT_0) {
      config_watcher_.OnDirectoryEvent();
    }
  }

  Shutdown();
  return 0;
}

void EngineApp::Shutdown() {
  if (shutdown_done_) return;
  shutdown_done_ = true;
  // P4.1: drop the one-shot trim timer when a shutdown lands inside the
  // 2 s window (KillTimer on a dead/unknown id is a harmless no-op).
  if (message_hwnd_ != nullptr) {
    KillTimer(message_hwnd_, kWorkingSetTrimTimerId);
    KillTimer(message_hwnd_, kOcclusionPokeTimerId);
  }
  working_set_trim_.Cancel();
  occlusion_poke_.Disarm();
  // HIGH-1 (audit-remediation): stop the IPC server FIRST, before any
  // renderer/surface teardown. The IPC worker thread runs handlers that
  // dereference renderer_ and multi_monitor_ (set_video -> LoadLoopAll /
  // Wakeup, pause/resume -> Pause/Resume, set_monitor -> SetActiveMonitor,
  // get_state -> BuildStateJson) and tray_ (PushRecent). IpcServer::Stop()
  // joins that worker, so once it returns no handler can still be running —
  // only then is it safe to reset renderer_ (StopHeadlessRenderer) and
  // destroy the slots (ShutdownWallpaperSurface). Previously Stop() ran
  // AFTER those resets, leaving a window where an in-flight handler could
  // touch a destroyed renderer_/multi_monitor_ -> use-after-free.
  ipc_server_.Stop();
  // Config watcher callback touches renderer_ too (ApplyFitMode); Stop()
  // nulls the callback so no stray directory event can fire it mid-teardown.
  // Cheap (no watcher thread, zero-join by design); GetConfig() stays valid
  // for any IPC handler still draining.
  config_watcher_.Stop();
  // P2.4 (Todo 7) shutdown order (Oracle round-02 issue 1, BLOCKING): the
  // headless event thread is signaled + joined HERE, before
  // ShutdownWallpaperSurface() destroys the slot renderers. Slot threads are
  // joined inside the teardown itself — MultiMonitor::Shutdown() →
  // ClearSlots() → ~MpvRenderer runs quit → wakeup → blocking join BEFORE
  // mpv_terminate_destroy per renderer, so no thread is ever joined after
  // its renderer is destroyed. >200 ms joins log critical and keep blocking
  // (never TerminateThread); no join runs from inside an mpv callback.
  StopHeadlessRenderer();
  ShutdownWallpaperSurface();
  os_wallpaper_.Restore();
  tray_.Remove();  // NIM_DELETE while message_hwnd_ is still valid
  fullscreen_watch_.Stop();
  occlusion_watch_.SetNotifyWindow(nullptr);
  if (monitor_power_notify_ != nullptr) {
    UnregisterPowerSettingNotification(monitor_power_notify_);
    monitor_power_notify_ = nullptr;
  }
  if (message_hwnd_ != nullptr) {
    WTSUnRegisterSessionNotification(message_hwnd_);
    DestroyWindow(message_hwnd_);
    message_hwnd_ = nullptr;
    g_message_hwnd.store(nullptr);
  }
  if (class_registered_) {
    UnregisterClassW(kWindowClassName, hinstance_);
    class_registered_ = false;
  }
  Log("engine shutdown complete");
  FlushEngineLog();  // last drain: the line above must reach disk
}

void EngineApp::OnSuspend() { SetPauseOwner(kPauseSuspend, true); }

void EngineApp::OnResume() { SetPauseOwner(kPauseSuspend, false); }

const char* EngineApp::PauseOwnerName(int bit) {
  switch (bit) {
    case EngineApp::kPauseUser:
      return "user";
    case EngineApp::kPauseFullscreen:
      return "fullscreen";
    case EngineApp::kPauseSuspend:
      return "suspend";
    case EngineApp::kPausePower:
      return "power";
    case EngineApp::kPauseSessionLock:
      return "session-lock";
    case EngineApp::kPauseScreenOff:
      return "screen-off";
    default:
      return "unknown";
  }
}

void EngineApp::SetPauseOwner(int bit, bool on) {
  if (!pause_.SetBit(bit, on)) {
    return;
  }
  ApplyPauseState(PauseOwnerName(bit));
}

void EngineApp::ApplyPauseState(const char* owner) {
  const int mask = pause_.mask();
  const bool ui = pause_.UiPaused();
  const bool slots = pause_.SlotsPaused();
  // Pause()/Resume() flip the renderer atomic + wake the event thread out of
  // its -1 block (500 ms watchdog rearms immediately), so the IPC
  // pause/resume worker paths and the tray path fast-drain via this fan-out.
  if (std::shared_ptr<MpvRenderer> renderer = AcquireRenderer()) {
    if (ui) {
      renderer->Pause();
    } else {
      renderer->Resume();
    }
  }
  if (wallpaper_surface_live_.load(std::memory_order_acquire)) {
    if (slots) {
      multi_monitor_.PauseAll();
    } else {
      multi_monitor_.ResumeAll();
    }
  }
  tray_.SetPaused(ui);
  // P2.5 (Todo 9): occlusion arm/disarm follows the merged mask. Mask
  // zero re-arms the 1500 ms tick (posts a wake to the loop thread when
  // called from the IPC worker — never touches loop timers directly);
  // any owner disarms it (the loop wait goes INFINITE, zero wakeups).
  occlusion_watch_.OnPauseMaskChanged(slots);
  LogImportant("pause: owner=%s mask=%d ui=%d slots=%d", owner, mask, ui ? 1 : 0,
               slots ? 1 : 0);
}

void EngineApp::OnDeviceLost() {
  LogImportant("device-lost: OnDeviceLost() hook - setting flag");
  device_lost_.store(true);
}

void EngineApp::OnDisplayChange(int width, int height) {
  Log("display: OnDisplayChange(%dx%d) - re-enumerating live slots",
      width, height);
  if (wallpaper_surface_live_.load(std::memory_order_acquire))
    multi_monitor_.OnDisplayChange();
  UpdateTrayErrorStatus();
}

bool EngineApp::QueueSetVideo(const std::string& payload_json) {
  // Worker-side half: validate only (pure, ipc_marshal.hpp). A reject here
  // acks {"error"} and queues nothing — the old video keeps playing.
  if (!ValidateSetVideoPayload(payload_json)) {
    Log("ipc: set_video rejected (missing/invalid path or not a file)");
    return false;
  }
  const HWND hwnd = g_message_hwnd.load(std::memory_order_acquire);
  pending_.SetVideo(payload_json);
  // Post AFTER storing: the main thread pops under the same mutex, so it
  // can never observe the flag without the payload. g_message_hwnd is
  // atomic and Shutdown nulls it only after ipc_server_.Stop() joined this
  // worker, so null here means teardown already started — drop loudly.
  if (hwnd == nullptr || !PostMessageW(hwnd, kSetVideoMessage, 0, 0)) {
    pending_.ClearVideo();
    Log("warning: set_video post failed (error %lu), command dropped",
        GetLastError());
    return false;
  }
  Log("ipc: set_video diterima (queued for main loop, verify via get_state)");
  return true;
}

bool EngineApp::QueueSetMonitor(const std::string& payload_json) {
  if (!ParseSetMonitorPayload(payload_json)) {
    Log("ipc: set_monitor rejected (missing/invalid \"monitor\" field)");
    return false;
  }
  const HWND hwnd = g_message_hwnd.load(std::memory_order_acquire);
  pending_.SetMonitor(payload_json);
  if (hwnd == nullptr || !PostMessageW(hwnd, kSetMonitorMessage, 0, 0)) {
    pending_.ClearMonitor();
    Log("warning: set_monitor post failed (error %lu), command dropped",
        GetLastError());
    return false;
  }
  Log("ipc: set_monitor diterima (queued for main loop, verify via get_state)");
  return true;
}

void EngineApp::ApplyPendingSetVideo() {
  std::string payload;
  if (!pending_.TakeVideo(&payload)) {
    Log("ipc: set_video main-loop wake with empty queue (ignoring)");
    return;
  }
  // The executor re-validates (the file may have vanished between queue and
  // execution); a late reject only logs — the {"ok":true} ack already meant
  // "diterima", and the client verifies via get_state.
  HandleSetVideo(payload);
}

void EngineApp::ApplyPendingSetMonitor() {
  std::string payload;
  if (!pending_.TakeMonitor(&payload)) {
    Log("ipc: set_monitor main-loop wake with empty queue (ignoring)");
    return;
  }
  HandleSetMonitor(payload);
}

std::filesystem::path EngineApp::ResolvedConfigPath() const {
  std::lock_guard<std::mutex> lock(options_mutex_);
  if (!options_.config_path.empty()) {
    return std::filesystem::path(options_.config_path);
  }
  return DefaultConfigPath();
}

bool EngineApp::HandleSetVideo(const std::string& payload_json, bool rotation) {
  // Payload shape matches IpcClient::SetVideo: {"path": "<utf8>"}. Re-validate
  // through the shared helper (the file may have vanished since queueing); a
  // bad path keeps the old wallpaper rendering.
  const std::optional<std::string> validated =
      ValidateSetVideoPayload(payload_json);
  if (!validated) {
    Log("ipc: set_video rejected (missing/invalid path or not a file)");
    return false;
  }
  const std::string& utf8_path = *validated;
  const std::filesystem::path fs_path = std::filesystem::u8path(utf8_path);
  // The visible slots own the decode: loading the headless renderer too
  // would decode the same file twice (~500MB wasted on 4K). Headless is
  // fallback only when no live surface exists. Thread-safe: MpvRenderer
  // serializes all mpv calls under one mutex.
  if (wallpaper_surface_live_.load(std::memory_order_acquire)) {
    if (!multi_monitor_.LoadLoopAll(utf8_path)) {
      Log("ipc: set_video rejected (decode failed for '%s', keeping old video)",
          utf8_path.c_str());
      return false;
    }
    multi_monitor_.ApplyFitModeAll(config_watcher_.GetConfig().fit_mode);
    // Slot threads self-wake: LoadLoop issues mpv commands that generate
    // events. The headless thread is woken explicitly below (fast drain).
  } else if (!renderer_ || !renderer_->LoadLoop(utf8_path)) {
    Log("ipc: set_video rejected (renderer refused %s)", utf8_path.c_str());
    return false;
  }
  // P2.4 (Todo 7): IPC worker fast drain — wake the headless event thread so
  // the load's events are picked up without waiting for the 500 ms watchdog
  // wake. Pause/resume IPC paths drain via ApplyPauseState's Pause/Resume.
  if (renderer_) renderer_->Wakeup();
  {
    std::lock_guard<std::mutex> lock(video_mutex_);
    current_video_utf8_ = utf8_path;
  }
  {
    // CRIT-1: options_ lives under options_mutex_ (get_state reads it from
    // the IPC worker thread). Snapshot discipline: copy under the lock,
    // use outside — never hold it across renderer calls or config I/O.
    std::lock_guard<std::mutex> lock(options_mutex_);
    options_.video_path = fs_path.wstring();
  }

  // Persist the new video_path to config.json so the engine restarts with
  // the correct wallpaper.
  const std::filesystem::path config_path = ResolvedConfigPath();

  // HIGH-4: single-field atomic persist — JSON-level merge preserves
  // unknown/future keys (no struct round-trip), publishes via .tmp +
  // MoveFileExW, and skips the write when unchanged (no watcher mtime bump).
  WallpaperConfig persist_fallback = config_watcher_.GetConfig();
  try {
    if (k6wp::PersistConfigField(config_path, "video_path",
                                 nlohmann::json(utf8_path),
                                 persist_fallback)) {
      Log("ipc: set_video config persisted to %s", config_path.string().c_str());
    }
  } catch (const k6wp::ConfigError& e) {
    // Non-fatal: the wallpaper already changed; just warn.
    Log("warning: set_video config persist failed: %s", e.what());
  }

  if (!rotation) {
    tray_.PushRecent(utf8_path);  // Todo 35: MRU is for manual picks only
    LogImportant("ipc: set_video live-switched to %s", utf8_path.c_str());
    MaybeTriggerLockscreenSync(utf8_path);
  } else {
    // Rotation is not a user action: no MRU feed, no lock-screen extract, and
    // Log (not LogImportant) so a short interval cannot flush the log per cycle.
    Log("playlist: rotation live-switched to %s", utf8_path.c_str());
  }
  headless_owns_decode_.store(
      !wallpaper_surface_live_.load(std::memory_order_acquire),
      std::memory_order_release);
  ArmPinVerify();  // P3L.3: verify pin post-start (PATCH A)
  return true;
}

bool EngineApp::HandleSetMonitor(const std::string& payload_json) {
  // Payload shape matches IpcClient::SetMonitor: {"monitor": N}, with
  // {"monitor_id": N} accepted as an alias. Shared validator: integer >= -1.
  const std::optional<int> parsed = ParseSetMonitorPayload(payload_json);
  if (!parsed) {
    Log("ipc: set_monitor rejected (missing/invalid \"monitor\" field)");
    return false;
  }
  const int id = *parsed;
  multi_monitor_.SetActiveMonitor(id);
  // Re-sync the live flag with the slot census: filtering down to an absent
  // monitor leaves zero slots (headless fallback owns decode), and
  // recovering back must hand decode to the slots again. Without this the
  // tray icon, pause fan-out, and set_video routing would stick to the old
  // topology.
  wallpaper_surface_live_.store(multi_monitor_.slot_count() > 0,
                                std::memory_order_release);

  // SetActiveMonitor/ApplyActiveFilter only CREATE new slot renderers - they
  // never load a file. Seed any freshly attached slot with the current video
  // and fit mode, or a monitor switch / hotplug shows a blank surface until
  // the next set_video.
  if (wallpaper_surface_live_.load(std::memory_order_acquire)) {
    std::string video;
    {
      std::lock_guard<std::mutex> lock(video_mutex_);
      video = current_video_utf8_;
    }
    if (!video.empty()) {
      if (!multi_monitor_.LoadLoopAll(video)) {
        Log("warning: set_monitor could not load video into new slot(s)");
      }
      multi_monitor_.ApplyFitModeAll(config_watcher_.GetConfig().fit_mode);
    }
  }

  // Persist the new target to config.json (preserve-merge, same path rule
  // as set_video). Best-effort: the live state already changed, so a
  // persist failure only warns while the ack stays ok.
  const std::filesystem::path config_path = ResolvedConfigPath();
  // HIGH-4: single-field atomic persist (same .tmp + MoveFileExW path as
  // set_video). Best-effort: the live state already changed, so a persist
  // failure only warns while the ack stays ok.
  try {
    if (k6wp::PersistConfigField(config_path, "monitor_id",
                                 nlohmann::json(id),
                                 config_watcher_.GetConfig())) {
      Log("ipc: set_monitor config persisted to %s", config_path.string().c_str());
    }
  } catch (const k6wp::ConfigError& e) {
    Log("warning: set_monitor config persist failed: %s", e.what());
  }

  UpdateTrayErrorStatus();
  Log("ipc: set_monitor active=%d", id);
  return true;
}

void EngineApp::MaybeTriggerLockscreenSync(const std::string& video_utf8) {
  if (video_utf8.empty()) return;
  // Canonical helper (shared/lockscreen.cpp): no-op unless lockscreen_sync is
  // ON, debounced to one spawn per 5s, child detached BELOW_NORMAL, never
  // throws. Log-only here so sync can never break the render path.
  if (!IsLockscreenSyncEnabled()) return;
  FireLockscreenSyncAsync(std::filesystem::u8path(video_utf8));
  Log("lockscreen: refresh requested for %s (best-effort, debounced)",
      video_utf8.c_str());
}
void EngineApp::RecreateDevice() {
  std::string video;
  {
    std::lock_guard<std::mutex> lock(video_mutex_);
    video = current_video_utf8_;
  }
  if (video.empty()) {
    LogImportant("device-lost: RecreateDevice() begin (no video loaded)");
    LogImportant("device-lost: RecreateDevice() end (nothing to reload)");
    return;
  }
  LogImportant("device-lost: RecreateDevice() begin reload '%s'", video.c_str());
  bool ok = false;
  if (wallpaper_surface_live_.load(std::memory_order_acquire)) {
    ok = multi_monitor_.LoadLoopAll(video, true);
  } else if (renderer_) {
    ok = renderer_->LoadLoop(video, true);
  }
  LogImportant("device-lost: RecreateDevice() end reload ok=%d", ok ? 1 : 0);
  if (ok) {
    ArmRotation();  // restart the playlist interval after the forced reload
    ArmPinVerify();  // P3L.3: re-verify pin after the forced reload
  }
}

void EngineApp::OnTrayTogglePause() {
  // Step 3.1: the tray item owns the user bit only — toggling it never
  // clears a fullscreen/suspend/power hold.
  if ((pause_.mask() & kPauseUser) != 0) {
    SetPauseOwner(kPauseUser, false);
  } else {
    SetPauseOwner(kPauseUser, true);
  }
}

void EngineApp::OnTrayQuickSwitch(std::size_t idx) {
  const std::string utf8_path = tray_.RecentAt(idx);
  if (utf8_path.empty()) {
    Log("tray: quick-switch [%llu] has no entry, ignoring",
        static_cast<unsigned long long>(idx));
    return;
  }
  // Reuse the validated IPC path: a stale entry acks the same {"error"} the
  // pipe client would get, and the current video keeps playing. Persist
  // ownership stays with HandleSetVideo (HIGH-4 PersistConfigField single-
  // field merge) — this site performs no direct config write.
  const nlohmann::json payload = {{"path", utf8_path}};
  if (!HandleSetVideo(payload.dump())) {
    Log("tray: quick-switch to %s rejected, keeping current video", utf8_path.c_str());
  }
}

void EngineApp::OnTrayOpenStudio() {
  wchar_t exe_path[MAX_PATH] = {};
  const DWORD len = GetModuleFileNameW(nullptr, exe_path, MAX_PATH);
  if (len == 0 || len >= MAX_PATH) {
    Log("tray: Open Studio failed (GetModuleFileNameW error %lu)", GetLastError());
    return;
  }
  const std::filesystem::path exe_dir =
      std::filesystem::path(exe_path).parent_path();
  std::error_code ec;
  // Prefer the launcher singleton: K6WP.exe --studio owns the
  // Local\K6WP-Studio-Singleton mutex and focuses an existing studio window
  // (launcher/main.cpp FocusExistingStudio, matched by exe image name)
  // instead of spawning a duplicate. Never bypass the launcher when present.
  const std::filesystem::path launcher = exe_dir / L"K6WP.exe";
  if (std::filesystem::exists(launcher, ec)) {
    std::wstring cmd = L"\"" + launcher.wstring() + L"\" --studio";
    std::vector<wchar_t> cmd_buf(cmd.begin(), cmd.end());
    cmd_buf.push_back(L'\0');
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    if (!CreateProcessW(launcher.c_str(), cmd_buf.data(), nullptr, nullptr,
                        FALSE, DETACHED_PROCESS, nullptr, nullptr, &si, &pi)) {
      Log("tray: Open Studio via launcher failed (CreateProcess error %lu)",
          GetLastError());
      return;
    }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    Log("tray: Open Studio via launcher (%ls --studio)", launcher.c_str());
    return;
  }
  // Fallback (launcher binary absent only): focus the real studio window by
  // its measured title, else spawn studio.exe directly. Measured 2026-09-17:
  // title "K6WP Studio", Qt-owned class (e.g. Qt683dQWindowIcon) — never
  // "Studio"/"StudioWindow" (Todo 12 duplicate root cause).
  HWND hwnd = FindWindowW(nullptr, L"K6WP Studio");
  if (hwnd != nullptr) {
    ShowWindow(hwnd, SW_RESTORE);
    SetForegroundWindow(hwnd);
    Log("tray: Studio window focused (existing instance, launcher missing)");
    return;
  }
  const std::filesystem::path studio = exe_dir / L"studio.exe";
  if (!std::filesystem::exists(studio, ec)) {
    Log("tray: Open Studio failed (not found: %ls)", studio.c_str());
    return;
  }
  STARTUPINFOW si{};
  si.cb = sizeof(si);
  PROCESS_INFORMATION pi{};
  // CreateProcessW requires a mutable command-line buffer.
  std::wstring cmd = L"\"" + studio.wstring() + L"\"";
  std::vector<wchar_t> cmd_buf(cmd.begin(), cmd.end());
  cmd_buf.push_back(L'\0');
  if (!CreateProcessW(studio.c_str(), cmd_buf.data(), nullptr, nullptr, FALSE, 0,
                      nullptr, nullptr, &si, &pi)) {
    Log("tray: Open Studio failed (CreateProcess error %lu)", GetLastError());
    return;
  }
  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);
  Log("tray: Open Studio fallback direct (%ls) (launcher missing)", studio.c_str());
}

void EngineApp::OnTraySupport() {
  const int wlen =
      MultiByteToWideChar(CP_UTF8, 0, K6WP_DONATE_URL, -1, nullptr, 0);
  if (wlen <= 0) {
    Log("tray: Support open failed (URL conversion error %lu)", GetLastError());
    return;
  }
  std::vector<wchar_t> wurl(static_cast<size_t>(wlen));
  if (MultiByteToWideChar(CP_UTF8, 0, K6WP_DONATE_URL, -1, wurl.data(), wlen) <= 0) {
    Log("tray: Support open failed (URL conversion error %lu)", GetLastError());
    return;
  }
  const HINSTANCE rc =
      ShellExecuteW(nullptr, L"open", wurl.data(), nullptr, nullptr, SW_SHOWNORMAL);
  if (reinterpret_cast<INT_PTR>(rc) <= 32) {
    Log("tray: Support open failed (ShellExecute error %lld)",
        static_cast<long long>(reinterpret_cast<INT_PTR>(rc)));
  } else {
    Log("tray: Support the developer opened in browser");
  }
}

std::string EngineApp::BuildStateJson() const {
  std::string video_utf8;
  {
    std::lock_guard<std::mutex> lock(video_mutex_);
    video_utf8 = current_video_utf8_;
  }
  // Step 3.3: report the resolved config path (empty --config means the
  // engine watches DefaultConfigPath()). Never throws out of get_state.
  // CRIT-1: options_ + pin_reverted_total_ are cross-thread state — snapshot
  // under their mutexes (this runs on the IPC worker thread). The slot
  // census below (active_monitor atomic; slot counts) is a best-effort
  // diagnostic read, same class as thread_count/handle_count.
  std::string config_utf8;
  WallpaperMode wallpaper_mode_copy = WallpaperMode::kAuto;
  int reverts_copy = 0;
  try {
    std::lock_guard<std::mutex> lock(options_mutex_);
    config_utf8 = options_.config_path.empty()
                      ? DefaultConfigPath().u8string()
                      : std::filesystem::path(options_.config_path).u8string();
    wallpaper_mode_copy = options_.wallpaper_mode;
  } catch (...) {
    config_utf8.clear();
  }
  {
    std::lock_guard<std::mutex> lock(state_mutex_);
    reverts_copy = pin_reverted_total_;
  }
  // WallpaperModeToString takes the enum by value — resolve outside the
  // lock from the snapshot.
  const char* wallpaper_mode_str = WallpaperModeToString(wallpaper_mode_copy);
  const std::shared_ptr<MpvRenderer> renderer_snapshot = AcquireRenderer();
  const nlohmann::json state = {
      {"running", running_.load(std::memory_order_acquire)},
      {"paused", UiPaused()},
      // P2.3, additive only: raw pause-owner mask (bit 32 = screen-off) so
      // QA can prove the screen-off bit sets and clears via get_state.
      {"pause_mask", pause_.mask()},
      {"pid", static_cast<unsigned long long>(GetCurrentProcessId())},
      {"wallpaper_mode", wallpaper_mode_str},
      {"video", video_utf8},
      {"config", config_utf8},
      // Step 4, additive only: old clients ignore unknown fields.
      {"headless_slots", multi_monitor_.headless_slot_count()},
      {"live", multi_monitor_.slot_count() > 0},
      // Step 5, additive only: live monitor target (-1 = all screens).
      {"monitor", multi_monitor_.active_monitor()},
      {"thread_count", CurrentThreadCount()},
      {"handle_count", CurrentHandleCount()},
      {"hwdec_active", renderer_snapshot ? renderer_snapshot->IsHwdecActive() : false},
      // P3L.3, additive only: resolved d3d11-adapter pin ("" = unpinned)
      // + lifetime revert count from the verify pass (PATCH A).
      {"gpu_pin", adapter_pin_value_},
      {"gpu_pin_reverts", reverts_copy},
      // Wallpaper playlist snapshot (additive; old clients ignore it).
      {"playlist_enabled", playlist_enabled_atomic_.load(std::memory_order_acquire)},
      {"playlist_size", playlist_size_atomic_.load(std::memory_order_acquire)},
      {"playlist_index", playlist_index_atomic_.load(std::memory_order_acquire)},
  };
  return state.dump();
}

void EngineApp::HandlePowerBroadcast(WPARAM wParam, LPARAM lParam) {
  switch (wParam) {
    case PBT_APMSUSPEND:
      Log("power: PBT_APMSUSPEND received");
      OnSuspend();
      return;
    case PBT_APMRESUMEAUTOMATIC:
      Log("power: PBT_APMRESUMEAUTOMATIC received");
      OnResume();
      // Sleep may have recreated the desktop windows: re-anchor the
      // live surface onto the current targets.
      if (wallpaper_surface_live_.load(std::memory_order_acquire)) multi_monitor_.Reanchor();
      UpdateTrayErrorStatus();
      return;
    case PBT_APMPOWERSTATUSCHANGE:
      Log("power: PBT_APMPOWERSTATUSCHANGE received");
      if (power_saver_) power_saver_->Update();
      return;
    case PBT_POWERSETTINGCHANGE: {
      // P2.3: monitor power (GUID_MONITOR_POWER_ON). Data: 0 = off →
      // screen-off pause, 1 = on → clear, 2 = dim → ignore. Any other
      // GUID (or a null/short payload) is ignored: no pause change,
      // no crash. Suspend/resume still route via OnSuspend/OnResume
      // (resume constant is PBT_APMRESUMEAUTOMATIC — no PBT_APMRESUME).
      const auto* setting = reinterpret_cast<POWERBROADCAST_SETTING*>(lParam);
      if (setting == nullptr ||
          !IsEqualGUID(setting->PowerSetting, GUID_MONITOR_POWER_ON)) {
        Log("power: PBT_POWERSETTINGCHANGE unknown GUID (ignored, no pause change)");
        return;
      }
      if (setting->DataLength < sizeof(DWORD)) {
        Log("power: PBT_POWERSETTINGCHANGE monitor payload too short (%lu, ignored)",
            static_cast<unsigned long>(setting->DataLength));
        return;
      }
      DWORD data = 0;
      std::memcpy(&data, setting->Data, sizeof(data));
      switch (data) {
        case 0:
          LogImportant("power: monitor off (Data=0), pausing decode");
          SetPauseOwner(kPauseScreenOff, true);
          break;
        case 1:
          LogImportant("power: monitor on (Data=1), resuming decode");
          SetPauseOwner(kPauseScreenOff, false);
          break;
        case 2:
          Log("power: monitor dimmed (Data=2, ignored)");
          break;
        default:
          Log("power: monitor unknown Data=%lu (ignored)",
              static_cast<unsigned long>(data));
          break;
      }
      return;
    }
    default:
      Log("power: WM_POWERBROADCAST wParam=0x%llX (ignored)",
          static_cast<unsigned long long>(wParam));
      return;
  }
}

LRESULT EngineApp::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  // Todo 35: Explorer-restart survival (QA-fail path). The value is captured
  // at tray Install(); 0 means "not installed yet" — never matches a real msg.
  const unsigned taskbar_created = tray_.taskbar_created_msg();
  if (taskbar_created != 0 && msg == taskbar_created) {
    Log("window: TaskbarCreated received, re-adding tray icon");
    tray_.OnTaskbarCreated();
    // Explorer restart also recreates Progman/WorkerW: re-anchor the live
    // injected windows onto the fresh desktop targets.
    if (wallpaper_surface_live_.load(std::memory_order_acquire)) multi_monitor_.Reanchor();
    UpdateTrayErrorStatus();
    return 0;
  }
  switch (msg) {
    case WM_POWERBROADCAST:
      HandlePowerBroadcast(wParam, lParam);
      return TRUE;
    case WM_CLOSE:
      Log("window: WM_CLOSE received");
      RequestShutdown();
      return 0;
    case WM_SETTINGCHANGE:
      // Slideshow wallpaper swap (SPI_SETDESKWALLPAPER): Explorer can tear
      // down/repaint the desktop windows out from under the injected
      // surface, leaving mpv decoding into a dead HWND — engine reports
      // active while the OS wallpaper shows instead. Re-anchor the live
      // slots like TaskbarCreated/display-change do. All other setting
      // changes are ignored so unrelated tweaks never flicker the surface.
      if (wParam == static_cast<WPARAM>(SPI_SETDESKWALLPAPER)) {
        Log("window: WM_SETTINGCHANGE (wallpaper changed), re-anchoring live surface");
        if (wallpaper_surface_live_.load(std::memory_order_acquire)) multi_monitor_.Reanchor();
        UpdateTrayErrorStatus();
      }
      return 0;
    case WM_DISPLAYCHANGE: {
      const int width = static_cast<int>(LOWORD(lParam));
      const int height = static_cast<int>(HIWORD(lParam));
      Log("display: WM_DISPLAYCHANGE bpp=%llu size=%dx%d, re-enumerating",
          static_cast<unsigned long long>(wParam), width, height);
      OnDisplayChange(width, height);
      return 0;
    }
    case WM_WTSSESSION_CHANGE:
      switch (wParam) {
        case WTS_SESSION_LOCK:
          LogImportant("session: WTS_SESSION_LOCK received, pausing decode");
          SetPauseOwner(kPauseSessionLock, true);
          return 0;
        case WTS_SESSION_UNLOCK:
          LogImportant("session: WTS_SESSION_UNLOCK received, resuming decode");
          SetPauseOwner(kPauseSessionLock, false);
          return 0;
        default:
          return 0;
      }
    case kSetMonitorMessage:
      // CRIT-2: queued set_monitor from the IPC worker (validated there).
      // Executes here on the main loop thread: SetActiveMonitor creates and
      // destroys desktop windows (DesktopInjector::Attach/Detach), which is
      // only safe on the thread that owns them.
      ApplyPendingSetMonitor();
      return 0;
    case kSetVideoMessage:
      // CRIT-2: queued set_video from the IPC worker (validated there).
      // Executes here on the main loop thread: LoadLoopAll drives every
      // live renderer + persists config + feeds the tray MRU, serialized
      // with RecreateDevice/OnDisplayChange/Shutdown on this same thread.
      // (LoadLoop itself creates no window — mpv loadfile only — but it
      // mutates the same slot set, so it rides the same queue.)
      ApplyPendingSetVideo();
      return 0;
    case kShutdownMessage:
      Log("window: shutdown message received");
      RequestShutdown();
      return 0;
    case kTrayCallbackMessage:
      tray_.OnTrayNotify(hwnd, static_cast<std::size_t>(wParam),
                         static_cast<long>(lParam));
      return 0;
    case WM_TIMER:
      // P2.1 (Todo 3): config debounce expiry — 250 ms quiet elapsed, run
      // the unchanged stat mtime+size → LoadConfig → callback path
      // (corrupt → keep-last-valid + log, inside the watcher). Other timer
      // ids fall through to DefWindowProcW.
      if (wParam == static_cast<WPARAM>(ConfigWatcher::DebounceTimerId())) {
        config_watcher_.OnDebounceExpired();
        return 0;
      }
      // P4.1: one-shot working-set trim — kill immediately (never periodic),
      // then trim once. Stray firings after done are ignored inside.
      if (wParam == static_cast<WPARAM>(kWorkingSetTrimTimerId)) {
        KillTimer(hwnd, kWorkingSetTrimTimerId);
        working_set_trim_.RunOnce();
        return 0;
      }
      // HOTFIX: debounced occlusion poke — kill immediately (transient,
      // never periodic), then run one direct coverage check.
      if (wParam == static_cast<WPARAM>(kOcclusionPokeTimerId)) {
        KillTimer(hwnd, kOcclusionPokeTimerId);
        occlusion_poke_.OnTimer();
        return 0;
      }
      return DefWindowProcW(hwnd, msg, wParam, lParam);
    case FullscreenWatch::HookMessageId():
      // P2.2 (Todo 4): WinEvent hook callback PostMessage — run the
      // byte-identical detection + 2-confirm debounce on this loop thread.
      fullscreen_watch_.OnHookEvent(static_cast<unsigned long>(wParam));
      // HOTFIX: the same event may have un-covered a slot (focus moved
      // back, window minimized) — schedule a debounced direct check.
      // Cheap no-op unless a slot is occlusion-paused.
      occlusion_poke_.Schedule(static_cast<unsigned long>(wParam));
      return 0;
    case FullscreenWatch::PokeMessageId():
      // HOTFIX: window geometry notification (wParam = win-event id,
      // lParam = HWND). Destroy of the tracked holder un-strands the
      // global bit at once; geometry change of the tracked holder
      // re-evaluates it at once; then the debounced occlusion poke
      // covers the slot state.
      if (wParam == static_cast<WPARAM>(EVENT_OBJECT_DESTROY)) {
        fullscreen_watch_.OnWindowDestroyed(
            reinterpret_cast<void*>(lParam));
      } else if (wParam == static_cast<WPARAM>(EVENT_OBJECT_LOCATIONCHANGE)) {
        fullscreen_watch_.OnWindowMoved(
            reinterpret_cast<void*>(lParam));
      }
      occlusion_poke_.Schedule(static_cast<unsigned long>(wParam));
      return 0;
    case kMpvHwdecChangeMessage:
      // P2.4 (Todo 7): headless event thread observed hwdec-current /
      // vo-configured — run the moved fallback logic on this loop thread.
      if (renderer_) renderer_->OnHwdecPropertyChange();
      return 0;
    case OcclusionWatch::RearmMessageId():
      // P2.5 (Todo 9): mask-zero re-arm posted by OnPauseMaskChanged
      // (possibly from the IPC worker thread). The wake itself restarts
      // the 1500 ms cadence; the handler is a named no-op.
      occlusion_watch_.OnRearmMessage();
      return 0;
    case WM_COMMAND:
      tray_.OnMenuCommand(static_cast<unsigned>(LOWORD(wParam)));
      return 0;
    case WM_DESTROY:
      PostQuitMessage(0);
      return 0;
    default:
      return DefWindowProcW(hwnd, msg, wParam, lParam);
  }
}

void EngineApp::RequestShutdown() {
  if (!running_.load(std::memory_order_acquire)) return;
  running_.store(false, std::memory_order_release);
  Log("shutdown: requested, posting WM_QUIT");
  PostQuitMessage(0);
}

bool EngineApp::RegisterWindowClass() {
  WNDCLASSEXW wc{};
  wc.cbSize = sizeof(wc);
  wc.lpfnWndProc = &EngineApp::WndProc;
  wc.hInstance = hinstance_;
  wc.lpszClassName = kWindowClassName;
  if (!RegisterClassExW(&wc)) {
    const DWORD err = GetLastError();
    if (err != ERROR_CLASS_ALREADY_EXISTS) {
      Log("error: RegisterClassExW failed (error %lu)", err);
      return false;
    }
  } else {
    class_registered_ = true;
  }
  return true;
}

bool EngineApp::CreateMessageWindow() {
  // Hidden top-level window (never shown): TaskbarCreated, WM_DISPLAYCHANGE,
  // and WM_POWERBROADCAST arrive via HWND_BROADCAST/top-level delivery, which
  // a message-only (HWND_MESSAGE) window never receives. WS_POPUP without
  // ShowWindow means no taskbar button and no Alt-Tab entry.
  message_hwnd_ = CreateWindowExW(
      /*dwExStyle=*/0, kWindowClassName, L"K6WP Engine", /*dwStyle=*/WS_POPUP,
      /*x=*/0, /*y=*/0, /*nWidth=*/0, /*nHeight=*/0,
      /*hWndParent=*/nullptr, /*hMenu=*/nullptr, hinstance_, /*lpParam=*/this);
  if (message_hwnd_ == nullptr) {
    Log("error: CreateWindowExW failed (error %lu)", GetLastError());
    return false;
  }
  // P2.1 (Todo 3): the config FS-event debounce (250 ms SetTimer/KillTimer
  // re-arm) targets this hidden window; WM_TIMER expiry runs the unchanged
  // stat → LoadConfig → callback path. Wired here so it exists before
  // ConfigWatcher::Start() arms the directory watch in Init().
  config_watcher_.SetDebounceWindow(message_hwnd_);
  // P2.2 (Todo 4): the fullscreen WinEvent-hook callback (PostMessage
  // only) targets this hidden window; OnHookEvent runs the unchanged
  // detection + debounce. Wired here so it exists before
  // FullscreenWatch::Start() installs the hooks in Init().
  fullscreen_watch_.SetNotifyWindow(message_hwnd_);
  // P2.5 (Todo 9): occlusion re-arm posts (mask-zero wake) target this
  // hidden window too; Check runs on the unpaused-loop wake (no new
  // timer). Wired here so it exists before InitWallpaperSurface runs.
  occlusion_watch_.SetLog(&EngineApp::Log);
  occlusion_watch_.SetNotifyWindow(message_hwnd_);
  occlusion_poke_.SetHooks(
      message_hwnd_,
      [this]() {
        return wallpaper_surface_live_.load(std::memory_order_acquire);
      },
      [this]() {
        for (std::size_t i = 0; i < multi_monitor_.slot_count(); ++i) {
          if (multi_monitor_.IsSlotPaused(i)) return true;
        }
        return false;
      },
      [this]() { occlusion_watch_.CheckNow(multi_monitor_); });
  return true;
}

LRESULT CALLBACK EngineApp::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  EngineApp* self = nullptr;
  if (msg == WM_NCCREATE) {
    const auto* cs = reinterpret_cast<const CREATESTRUCTW*>(lParam);
    self = static_cast<EngineApp*>(cs->lpCreateParams);
    SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
  } else {
    self = reinterpret_cast<EngineApp*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
  }
  if (self != nullptr) {
    return self->HandleMessage(hwnd, msg, wParam, lParam);
  }
  return DefWindowProcW(hwnd, msg, wParam, lParam);
}

BOOL WINAPI EngineApp::ConsoleCtrlHandler(DWORD ctrl_type) {
  switch (ctrl_type) {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_CLOSE_EVENT:
      Log("console: Ctrl+C (ctrl_type=%lu), requesting shutdown", ctrl_type);
      if (HWND hwnd = g_message_hwnd.load()) {
        PostMessageW(hwnd, kShutdownMessage, 0, 0);
      }
      return TRUE;
    default:
      return FALSE;
  }
}

#ifndef K6WP_VERBOSE
#define K6WP_VERBOSE 0
#endif

void EngineApp::Log(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  EngineLogfV(fmt, args, false);
  va_end(args);
}

void EngineApp::LogImportant(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  EngineLogfV(fmt, args, true);
  va_end(args);
}

}  // namespace k6wp