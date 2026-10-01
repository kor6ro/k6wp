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

std::mutex g_log_mutex;
std::atomic<HWND> g_message_hwnd{nullptr};

const char* WallpaperModeToString(WallpaperMode mode) {
  switch (mode) {
    case WallpaperMode::kWorkerW:
      return "workerw";
    case WallpaperMode::kProgman:
      return "progman";
    case WallpaperMode::kAuto:
      return "auto";
  }
  return "auto";
}

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

void PrintUsage(FILE* out) {
  std::fprintf(out,
      "K6WP engine\n"
      "Usage: engine.exe [options]\n"
      "  --video <path>                 video file to render\n"
      "  --config <path>                config JSON path\n"
      "  --wallpaper-mode <m>           auto | workerw | progman (injection strategy)\n"
      "  --minimized                    tray-only start (autostart marker, no window)\n"
      "  --engine, --silent             aliases of --minimized (compat, keep working)\n"
      "  --help                         show this help and exit\n");
}

}  // namespace

int ParseCli(int argc, char** argv, CliOptions& out) {
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--help" || arg == "-h") {
      PrintUsage(stdout);
      return 1;
    }
    if (arg.rfind("--", 0) != 0) {
      std::fprintf(stderr, "warning: ignoring unknown argument '%s'\n", arg.c_str());
      continue;
    }
    // Support both "--flag value" and "--flag=value".
    std::string flag = arg;
    std::optional<std::string> inline_value;
    const size_t eq = arg.find('=');
    if (eq != std::string::npos) {
      flag = arg.substr(0, eq);
      inline_value = arg.substr(eq + 1);
    }
    const auto take_value = [&](const char* name) -> std::optional<std::string> {
      if (inline_value) return *inline_value;
      if (i + 1 >= argc) {
        std::fprintf(stderr, "error: %s requires a value\n", name);
        return std::nullopt;
      }
      return std::string(argv[++i]);
    };

    if (flag == "--video") {
      const auto value = take_value("--video");
      if (!value) return 2;
      out.video_path = std::filesystem::u8path(*value).wstring();
    } else if (flag == "--config") {
      const auto value = take_value("--config");
      if (!value) return 2;
      out.config_path = std::filesystem::u8path(*value).wstring();
    } else if (flag == "--wallpaper-mode") {
      const auto value = take_value("--wallpaper-mode");
      if (!value) return 2;
      if (*value == "auto") {
        out.wallpaper_mode = WallpaperMode::kAuto;
      } else if (*value == "workerw") {
        out.wallpaper_mode = WallpaperMode::kWorkerW;
      } else if (*value == "progman") {
        out.wallpaper_mode = WallpaperMode::kProgman;
      } else {
        std::fprintf(stderr, "error: unknown --wallpaper-mode '%s' (expected auto|workerw|progman)\n",
                     value->c_str());
        return 2;
      }
    } else if (flag == "--minimized") {
      if (inline_value) {
        std::fprintf(stderr, "error: --minimized takes no value\n");
        return 2;
      }
      out.minimized = true;
    } else if (flag == "--engine") {
      // --engine is an alias for --minimized (tray-only start, no window)
      out.minimized = true;
    } else if (flag == "--silent") {
      // --silent is an alias for --minimized (tray-only start, no window)
      out.minimized = true;
    } else if (flag == "--restarted") {
      // WER restart marker from RegisterApplicationRestart(L"--restarted", 0).
      // Intentionally ignored: the engine boots normally after a crash.
      if (inline_value) {
        std::fprintf(stderr, "error: --restarted takes no value\n");
        return 2;
      }
    } else if (flag == "--exit-after-ms") {
      const auto value = take_value("--exit-after-ms");
      if (!value) return 2;
      try {
        out.exit_after_ms = std::stoi(*value);
      } catch (...) {
        std::fprintf(stderr, "error: --exit-after-ms expects an integer, got '%s'\n", value->c_str());
        return 2;
      }
      if (out.exit_after_ms < 0) {
        std::fprintf(stderr, "error: --exit-after-ms must be >= 0\n");
        return 2;
      }
    } else if (flag == "--simulate-device-lost-after-ms") {
      const auto value = take_value("--simulate-device-lost-after-ms");
      if (!value) return 2;
      try {
        out.simulate_device_lost_after_ms = std::stoi(*value);
      } catch (...) {
        std::fprintf(stderr, "error: --simulate-device-lost-after-ms expects an integer, got '%s'\n",
                     value->c_str());
        return 2;
      }
      if (out.simulate_device_lost_after_ms < 0) {
        std::fprintf(stderr, "error: --simulate-device-lost-after-ms must be >= 0\n");
        return 2;
      }
    } else if (flag == "--simulate-suspend-after-ms") {
      const auto value = take_value("--simulate-suspend-after-ms");
      if (!value) return 2;
      try {
        out.simulate_suspend_after_ms = std::stoi(*value);
      } catch (...) {
        std::fprintf(stderr, "error: --simulate-suspend-after-ms expects an integer, got '%s'\n",
                     value->c_str());
        return 2;
      }
      if (out.simulate_suspend_after_ms < 0) {
        std::fprintf(stderr, "error: --simulate-suspend-after-ms must be >= 0\n");
        return 2;
      }
    } else if (flag == "--simulate-dc-after-ms") {
      const auto value = take_value("--simulate-dc-after-ms");
      if (!value) return 2;
      try {
        out.simulate_dc_after_ms = std::stoi(*value);
      } catch (...) {
        std::fprintf(stderr, "error: --simulate-dc-after-ms expects an integer, got '%s'\n",
                     value->c_str());
        return 2;
      }
      if (out.simulate_dc_after_ms < 0) {
        std::fprintf(stderr, "error: --simulate-dc-after-ms must be >= 0\n");
        return 2;
      }
    } else if (flag == "--simulate-monitor-off-after-ms") {
      const auto value = take_value("--simulate-monitor-off-after-ms");
      if (!value) return 2;
      try {
        out.simulate_monitor_off_after_ms = std::stoi(*value);
      } catch (...) {
        std::fprintf(stderr, "error: --simulate-monitor-off-after-ms expects an integer, got '%s'\n",
                     value->c_str());
        return 2;
      }
      if (out.simulate_monitor_off_after_ms < 0) {
        std::fprintf(stderr, "error: --simulate-monitor-off-after-ms must be >= 0\n");
        return 2;
      }
    } else {
      // Unknown --flag: usage to stderr + exit 2 (never ignore silently).
      // Bare positional args (no -- prefix) stay a warning for compat.
      // Hidden developer test flags (--exit-after-ms, --simulate-*-after-ms)
      // keep parsing above; see docs/dev-test-flags.md, never in --help.
      std::fprintf(stderr, "error: unknown flag '%s'\n", arg.c_str());
      PrintUsage(stderr);
      return 2;
    }
  }
  return 0;
}

EngineApp::~EngineApp() { Shutdown(); }

bool EngineApp::Init(int argc, char** argv) {
  const int parse_rc = ParseCli(argc, argv, options_);
  if (parse_rc != 0) {
    init_exit_code_ = (parse_rc == 1) ? 0 : 2;
    return false;
  }
  exit_after_ms_ = options_.exit_after_ms;
  simulate_device_lost_after_ms_ = options_.simulate_device_lost_after_ms;
  simulate_suspend_after_ms_ = options_.simulate_suspend_after_ms;
  simulate_dc_after_ms_ = options_.simulate_dc_after_ms;
  simulate_monitor_off_after_ms_ = options_.simulate_monitor_off_after_ms;

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
  SaveOsWallpaper();

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
      WallpaperModeToString(options_.wallpaper_mode), exit_after_ms_);
  if (options_.minimized) {
    // The engine never shows a window (message-only HWND + tray icon only),
    // so --minimized is a no-op marker from the HKCU Run entry — logged so an
    // autostart launch is distinguishable from an interactive one.
    Log("engine init: --minimized flag set, tray-only start (no window shown)");
  }

  // Todo 11: start the config watcher from CliOptions. Empty --config falls
  // back to %LOCALAPPDATA%/K6WP/config.json. Corrupt JSON keeps the
  // last-valid config (logged, never fatal).
  const std::filesystem::path config_path =
      options_.config_path.empty()
          ? DefaultConfigPath()
          : std::filesystem::path(options_.config_path);
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
  ApplyCpuAffinity(applied_affinity_);

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
        if (simulate_dc_latched_) {
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
  tray_cb.on_next = [this](std::size_t idx) { OnTrayQuickSwitch(idx); };
  tray_cb.on_open_studio = [this]() { OnTrayOpenStudio(); };
  tray_cb.on_support = [this]() { OnTraySupport(); };
  tray_cb.on_exit = [this]() { RequestShutdown(); };
  tray_cb.is_paused = [this]() { return paused_.load(); };
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

bool EngineApp::AnySimulateArmed() const {
  if (exit_after_ms_ > 0) return true;
  if (simulate_device_lost_after_ms_ > 0 && !device_lost_simulated_) return true;
  if (simulate_suspend_after_ms_ > 0 && !resume_simulated_) return true;
  if (simulate_dc_after_ms_ > 0 && !dc_restored_) return true;
  if (simulate_monitor_off_after_ms_ > 0 && !monitor_on_simulated_) return true;
  return false;
}

DWORD EngineApp::ComputeWaitTimeoutMs() const {
  // 50 ms ONLY while a test flag is armed (never a permanent short sleep);
  // INFINITE while SlotsPaused() (zero periodic wakeups when paused);
  // 1500 ms while unpaused (Todo 9 occlusion cadence hook).
  if (AnySimulateArmed()) return 50;
  if (SlotsPaused()) return INFINITE;
  return 1500;
}

void EngineApp::SaveOsWallpaper() {
  wchar_t buf[MAX_PATH] = {};
  if (SystemParametersInfoW(SPI_GETDESKWALLPAPER, MAX_PATH, buf, 0) &&
      buf[0] != L'\0') {
    saved_wallpaper_.assign(buf);
    saved_wallpaper_valid_ = true;
    Log("engine: saved OS wallpaper '%ls'", saved_wallpaper_.c_str());
  } else {
    saved_wallpaper_.clear();
    saved_wallpaper_valid_ = false;
    Log("warning: could not read OS wallpaper (error %lu), continuing without restore",
        GetLastError());
  }
}

void EngineApp::RestoreOsWallpaper() {
  if (!saved_wallpaper_valid_ || saved_wallpaper_.empty()) {
    Log("engine: no saved OS wallpaper, skipping restore");
    return;
  }
  if (SystemParametersInfoW(SPI_SETDESKWALLPAPER, 0,
                            const_cast<LPWSTR>(saved_wallpaper_.c_str()),
                            SPIF_UPDATEINIFILE | SPIF_SENDCHANGE)) {
    Log("engine: restored OS wallpaper '%ls'", saved_wallpaper_.c_str());
  } else {
    Log("warning: could not restore OS wallpaper '%ls' (error %lu)",
        saved_wallpaper_.c_str(), GetLastError());
  }
}

void EngineApp::ApplyFitMode(const std::string& fit_mode) {
  if (renderer_) renderer_->SetFitMode(fit_mode);
  if (wallpaper_surface_live_.load(std::memory_order_acquire))
    multi_monitor_.ApplyFitModeAll(fit_mode);
  LogImportant("video: fit mode applied '%s'", fit_mode.c_str());
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

void EngineApp::ScheduleOcclusionPoke(unsigned long win_event) {
  // Loop thread only (called from HandleMessage). Cheap guard first: no
  // occlusion-paused slot means no poke (foreground/minimize storms cost
  // one slot-count scan, never an EnumWindows).
  if (occlusion_poke_armed_ ||
      !wallpaper_surface_live_.load(std::memory_order_acquire))
    return;
  bool any_paused = false;
  for (size_t i = 0; i < multi_monitor_.slot_count(); ++i) {
    if (multi_monitor_.IsSlotPaused(i)) {
      any_paused = true;
      break;
    }
  }
  if (!any_paused) return;
  if (message_hwnd_ == nullptr) return;
  if (SetTimer(message_hwnd_, kOcclusionPokeTimerId,
               kOcclusionPokeDebounceMs, nullptr) == 0) {
    // Fail-safe (still loop thread): run the check inline instead of
    // silently dropping the poke.
    Log("warning: occlusion poke SetTimer failed (error %lu), checking now",
        GetLastError());
    occlusion_watch_.CheckNow(multi_monitor_);
    return;
  }
  occlusion_poke_armed_ = true;
  Log("occlusion: poke scheduled (win-event=0x%lX)",
      win_event);
}

void EngineApp::OnOcclusionPokeTimer() {
  occlusion_poke_armed_ = false;
  if (!wallpaper_surface_live_.load(std::memory_order_acquire)) return;
  Log("occlusion: poke check");
  occlusion_watch_.CheckNow(multi_monitor_);
}

void EngineApp::ArmWorkingSetTrim() {
  // One-shot only: never re-arm after the trim ran or while pending.
  if (working_set_trim_done_ || working_set_trim_armed_) return;
  if (message_hwnd_ == nullptr) {
    Log("warning: working-set trim not armed (no message window)");
    return;
  }
  if (SetTimer(message_hwnd_, kWorkingSetTrimTimerId, 2000, nullptr) == 0) {
    Log("warning: working-set trim SetTimer failed (error %lu), skipping trim",
        GetLastError());
    return;
  }
  working_set_trim_armed_ = true;
}

void EngineApp::TrimWorkingSetOnce() {
  // Fires exactly once per process: mark done FIRST so every path below
  // (including failure) can never re-trim. Periodic trim would thrash
  // paged-in pages back out, so there is intentionally no re-arm here.
  if (working_set_trim_done_) return;
  working_set_trim_done_ = true;
  working_set_trim_armed_ = false;
  PROCESS_MEMORY_COUNTERS_EX before{};
  before.cb = sizeof(before);
  SIZE_T ws_before_kb = 0;
  if (GetProcessMemoryInfo(GetCurrentProcess(),
                           reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&before),
                           sizeof(before))) {
    ws_before_kb = static_cast<SIZE_T>(before.WorkingSetSize / 1024);
  }
  if (!SetProcessWorkingSetSize(GetCurrentProcess(), (SIZE_T)-1,
                                (SIZE_T)-1)) {
    Log("warning: working-set trim failed (error %lu), continuing",
        GetLastError());
    return;
  }
  PROCESS_MEMORY_COUNTERS_EX after{};
  after.cb = sizeof(after);
  if (GetProcessMemoryInfo(GetCurrentProcess(),
                           reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&after),
                           sizeof(after))) {
    const SIZE_T ws_after_kb =
        static_cast<SIZE_T>(after.WorkingSetSize / 1024);
    Log("engine: working-set trim ws=%llu KB -> %llu KB",
        static_cast<unsigned long long>(ws_before_kb),
        static_cast<unsigned long long>(ws_after_kb));
  } else {
    Log("engine: working-set trim done (ws before=%llu KB)",
        static_cast<unsigned long long>(ws_before_kb));
  }
}

void EngineApp::ApplyCpuAffinity(const std::string& mode) {
  // P3L.2: opted out or explicitly "all" — no-op with a log line so the
  // choice is auditable in engine.log.
  DWORD_PTR proc_mask = 0, sys_mask = 0;
  GetProcessAffinityMask(GetCurrentProcess(), &proc_mask, &sys_mask);
  if (mode != "auto") {
    Log("affinity: mode=%s, no change (mask=0x%llx)",
        mode.c_str(), (unsigned long long)proc_mask);
    return;
  }
  // Enumerate physical cores with efficiency classes. Hybrid = at least
  // two distinct EfficiencyClass values. E-cores = the MINIMUM class set:
  // verified empirically on i7-12650H (P=1 SMT, E=0 single-thread); every
  // (class, mask) pair is logged so the choice is auditable on any rig.
  DWORD len = 0;
  GetLogicalProcessorInformationEx(RelationProcessorCore, nullptr, &len);
  std::vector<char> buf(len > 0 ? len : 1);
  auto* info = reinterpret_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*>(
      buf.data());
  if (len == 0 ||
      !GetLogicalProcessorInformationEx(RelationProcessorCore, info, &len)) {
    Log("warning: affinity auto: core enumeration failed (error %lu), "
        "no change (mask=0x%llx)",
        GetLastError(), (unsigned long long)proc_mask);
    return;
  }
  BYTE min_class = 255, max_class = 0;
  bool any = false;
  for (char* p = buf.data(); p < buf.data() + len;) {
    const auto* e =
        reinterpret_cast<const SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*>(p);
    if (e->Relationship == RelationProcessorCore) {
      any = true;
      const BYTE c = e->Processor.EfficiencyClass;
      if (c < min_class) min_class = c;
      if (c > max_class) max_class = c;
    }
    if (e->Size == 0) break;  // corrupt list guard
    p += e->Size;
  }
  if (!any || min_class == max_class) {
    Log("affinity: auto, non-hybrid CPU (single efficiency class), "
        "no change (mask=0x%llx)",
        (unsigned long long)proc_mask);
    return;
  }
  DWORD_PTR e_mask = 0;
  for (char* p = buf.data(); p < buf.data() + len;) {
    const auto* e =
        reinterpret_cast<const SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*>(p);
    if (e->Relationship == RelationProcessorCore) {
      Log("affinity: core class=%u group=%u mask=0x%llx%s",
          (unsigned)e->Processor.EfficiencyClass,
          (unsigned)e->Processor.GroupMask[0].Group,
          (unsigned long long)e->Processor.GroupMask[0].Mask,
          e->Processor.EfficiencyClass == min_class ? " (E)" : "");
      if (e->Processor.EfficiencyClass == min_class) {
        if (e->Processor.GroupMask[0].Group != 0) {
          Log("warning: affinity auto: E-core outside group 0, "
              "no change (multi-group unsupported)");
          return;
        }
        e_mask |= e->Processor.GroupMask[0].Mask;
      }
    }
    if (e->Size == 0) break;
    p += e->Size;
  }
  if (e_mask == 0 || (e_mask & proc_mask) == 0 || e_mask == proc_mask) {
    Log("warning: affinity auto: degenerate E-mask 0x%llx, no change",
        (unsigned long long)e_mask);
    return;
  }
  if (!SetProcessAffinityMask(GetCurrentProcess(), e_mask & proc_mask)) {
    Log("warning: affinity auto: SetProcessAffinityMask(0x%llx) failed "
        "(error %lu)",
        (unsigned long long)(e_mask & proc_mask), GetLastError());
    return;
  }
  Log("affinity: auto, hybrid CPU, pinned to E-cores (mask=0x%llx)",
      (unsigned long long)(e_mask & proc_mask));
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
      ArmWorkingSetTrim();  // P4.1: one-shot trim ~2 s after first frame
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

    if (simulate_device_lost_after_ms_ > 0 && !device_lost_simulated_ &&
        elapsed_ms >= simulate_device_lost_after_ms_) {
      device_lost_simulated_ = true;
      Log("test: --simulate-device-lost-after-ms=%d reached, firing OnDeviceLost()",
          simulate_device_lost_after_ms_);
      OnDeviceLost();
    }
    if (simulate_suspend_after_ms_ > 0 && !suspend_simulated_ && elapsed_ms >= simulate_suspend_after_ms_) {
      suspend_simulated_ = true;
      Log("test: --simulate-suspend-after-ms=%d reached, posting PBT_APMSUSPEND",
          simulate_suspend_after_ms_);
      PostMessageW(message_hwnd_, WM_POWERBROADCAST, PBT_APMSUSPEND, 0);
    }
    if (simulate_suspend_after_ms_ > 0 && !resume_simulated_ &&
        elapsed_ms >= simulate_suspend_after_ms_ + 2000) {
      resume_simulated_ = true;
      Log("test: posting PBT_APMRESUMEAUTOMATIC");
      PostMessageW(message_hwnd_, WM_POWERBROADCAST, PBT_APMRESUMEAUTOMATIC, 0);
    }
    if (simulate_dc_after_ms_ > 0 && !dc_simulated_ &&
        elapsed_ms >= simulate_dc_after_ms_) {
      dc_simulated_ = true;
      simulate_dc_latched_ = true;
      Log("test: --simulate-dc-after-ms=%d reached, latching forced DC + posting PBT_APMPOWERSTATUSCHANGE",
          simulate_dc_after_ms_);
      PostMessageW(message_hwnd_, WM_POWERBROADCAST, PBT_APMPOWERSTATUSCHANGE, 0);
    }
    if (simulate_dc_after_ms_ > 0 && !dc_restored_ &&
        elapsed_ms >= simulate_dc_after_ms_ + 2000) {
      dc_restored_ = true;
      simulate_dc_latched_ = false;
      Log("test: clearing DC override, posting PBT_APMPOWERSTATUSCHANGE (AC restore)");
      PostMessageW(message_hwnd_, WM_POWERBROADCAST, PBT_APMPOWERSTATUSCHANGE, 0);
    }
    // P2.3: monitor-off QA sequence — delivers the SAME PBT_POWERSETTINGCHANGE
    // off/on pair the OS would send, so the real parse path is exercised
    // headless (off at N via Data=0, on at N+2s via Data=1).
    if (simulate_monitor_off_after_ms_ > 0 && !monitor_off_simulated_ &&
        elapsed_ms >= simulate_monitor_off_after_ms_) {
      monitor_off_simulated_ = true;
      Log("test: --simulate-monitor-off-after-ms=%d reached, sending PBT_POWERSETTINGCHANGE Data=0 (monitor off)",
          simulate_monitor_off_after_ms_);
      // WM_POWERBROADCAST is sync-only: PostMessageW fails with
      // ERROR_MESSAGE_SYNC_ONLY (1159). SendMessageW to our own loop thread
      // dispatches synchronously, exactly like the OS broadcast.
      MonitorPowerSetting off_setting{};
      InitMonitorPowerSetting(off_setting, 0);
      SendMessageW(message_hwnd_, WM_POWERBROADCAST, PBT_POWERSETTINGCHANGE,
                   reinterpret_cast<LPARAM>(&off_setting.base));
    }
    if (simulate_monitor_off_after_ms_ > 0 && !monitor_on_simulated_ &&
        elapsed_ms >= simulate_monitor_off_after_ms_ + 2000) {
      monitor_on_simulated_ = true;
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
    if (exit_after_ms_ > 0 && elapsed_ms >= exit_after_ms_) {
      Log("test: --exit-after-ms=%d reached, requesting shutdown", exit_after_ms_);
      RequestShutdown();
      break;
    }
    // Todo 11: poll the config file (internally throttled to 500 ms).
    // P2.1 (Todo 2): Poll() is the FALLBACK path only — the primary trigger
    // is the config dir-watch event joined into the wait array below.
    config_watcher_.Poll();
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
  working_set_trim_armed_ = false;
  occlusion_poke_armed_ = false;
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
  RestoreOsWallpaper();
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
  const int old = on ? pause_mask_.fetch_or(bit, std::memory_order_acq_rel)
                     : pause_mask_.fetch_and(~bit, std::memory_order_acq_rel);
  const int updated = on ? (old | bit) : (old & ~bit);
  if (updated == old) {
    return;
  }
  ApplyPauseState(PauseOwnerName(bit));
}

void EngineApp::ApplyPauseState(const char* owner) {
  const int mask = pause_mask_.load(std::memory_order_acquire);
  const bool ui = UiPaused();
  const bool slots = SlotsPaused();
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
  paused_.store(ui, std::memory_order_release);
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
  {
    std::lock_guard<std::mutex> lock(marshal_mutex_);
    pending_video_ = payload_json;
    has_pending_video_ = true;
  }
  // Post AFTER storing: the main thread pops under the same mutex, so it
  // can never observe the flag without the payload. g_message_hwnd is
  // atomic and Shutdown nulls it only after ipc_server_.Stop() joined this
  // worker, so null here means teardown already started — drop loudly.
  if (hwnd == nullptr || !PostMessageW(hwnd, kSetVideoMessage, 0, 0)) {
    std::lock_guard<std::mutex> lock(marshal_mutex_);
    has_pending_video_ = false;
    pending_video_.clear();
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
  {
    std::lock_guard<std::mutex> lock(marshal_mutex_);
    pending_monitor_ = payload_json;
    has_pending_monitor_ = true;
  }
  if (hwnd == nullptr || !PostMessageW(hwnd, kSetMonitorMessage, 0, 0)) {
    std::lock_guard<std::mutex> lock(marshal_mutex_);
    has_pending_monitor_ = false;
    pending_monitor_.clear();
    Log("warning: set_monitor post failed (error %lu), command dropped",
        GetLastError());
    return false;
  }
  Log("ipc: set_monitor diterima (queued for main loop, verify via get_state)");
  return true;
}

void EngineApp::ApplyPendingSetVideo() {
  std::string payload;
  {
    std::lock_guard<std::mutex> lock(marshal_mutex_);
    if (!has_pending_video_) {
      Log("ipc: set_video main-loop wake with empty queue (ignoring)");
      return;
    }
    payload = std::move(pending_video_);
    pending_video_.clear();
    has_pending_video_ = false;
  }
  // The executor re-validates (the file may have vanished between queue and
  // execution); a late reject only logs — the {"ok":true} ack already meant
  // "diterima", and the client verifies via get_state.
  HandleSetVideo(payload);
}

void EngineApp::ApplyPendingSetMonitor() {
  std::string payload;
  {
    std::lock_guard<std::mutex> lock(marshal_mutex_);
    if (!has_pending_monitor_) {
      Log("ipc: set_monitor main-loop wake with empty queue (ignoring)");
      return;
    }
    payload = std::move(pending_monitor_);
    pending_monitor_.clear();
    has_pending_monitor_ = false;
  }
  HandleSetMonitor(payload);
}

bool EngineApp::HandleSetVideo(const std::string& payload_json) {
  // Payload shape matches IpcClient::SetVideo: {"path": "<utf8>"}.
  std::string utf8_path;
  try {
    const nlohmann::json payload = nlohmann::json::parse(payload_json);
    if (!payload.is_object() || !payload.contains("path") ||
        !payload.at("path").is_string()) {
      Log("ipc: set_video rejected (missing/invalid \"path\" field)");
      return false;
    }
    utf8_path = payload.at("path").get<std::string>();
  } catch (const std::exception& e) {
    Log("ipc: set_video rejected (payload parse failed: %s)", e.what());
    return false;
  }
  if (utf8_path.empty()) {
    Log("ipc: set_video rejected (empty path)");
    return false;
  }
  // Validate BEFORE loadfile: a bad path must ack {"error"} while the old
  // wallpaper keeps rendering (QA-fail path).
  std::error_code ec;
  const std::filesystem::path fs_path = std::filesystem::u8path(utf8_path);
  if (!std::filesystem::exists(fs_path, ec) ||
      !std::filesystem::is_regular_file(fs_path, ec)) {
    Log("ipc: set_video rejected (not a file: %s)", utf8_path.c_str());
    return false;
  }
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
  // the correct wallpaper.  Config path: options_.config_path if set, else
  // DefaultConfigPath() — matching how ConfigWatcher is started in Init().
  std::filesystem::path config_path;
  {
    std::lock_guard<std::mutex> lock(options_mutex_);
    if (!options_.config_path.empty()) {
      config_path = std::filesystem::path(options_.config_path);
    } else {
      config_path = DefaultConfigPath();
    }
  }

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

  tray_.PushRecent(utf8_path);  // Todo 35: feed the quick-switch MRU list
  LogImportant("ipc: set_video live-switched to %s", utf8_path.c_str());
  MaybeTriggerLockscreenSync(utf8_path);
  headless_owns_decode_.store(
      !wallpaper_surface_live_.load(std::memory_order_acquire),
      std::memory_order_release);
  ArmPinVerify();  // P3L.3: verify pin post-start (PATCH A)
  return true;
}

bool EngineApp::HandleSetMonitor(const std::string& payload_json) {
  // Payload shape matches IpcClient::SetMonitor: {"monitor": N}, with
  // {"monitor_id": N} accepted as an alias.
  int id = -1;
  try {
    const nlohmann::json payload = nlohmann::json::parse(payload_json);
    if (!payload.is_object()) {
      Log("ipc: set_monitor rejected (payload is not an object)");
      return false;
    }
    const nlohmann::json* value = nullptr;
    if (payload.contains("monitor")) {
      value = &payload.at("monitor");
    } else if (payload.contains("monitor_id")) {
      value = &payload.at("monitor_id");
    }
    if (value == nullptr || !value->is_number_integer()) {
      Log("ipc: set_monitor rejected (missing/invalid \"monitor\" field)");
      return false;
    }
    id = value->get<int>();
  } catch (const std::exception& e) {
    Log("ipc: set_monitor rejected (payload parse failed: %s)", e.what());
    return false;
  }
  if (id < -1) {
    Log("ipc: set_monitor rejected (monitor %d < -1)", id);
    return false;
  }
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
  std::filesystem::path config_path;
  {
    std::lock_guard<std::mutex> lock(options_mutex_);
    if (!options_.config_path.empty()) {
      config_path = std::filesystem::path(options_.config_path);
    } else {
      config_path = DefaultConfigPath();
    }
  }
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
  if (ok) ArmPinVerify();  // P3L.3: re-verify pin after the forced reload
}

void EngineApp::OnTrayTogglePause() {
  // Step 3.1: the tray item owns the user bit only — toggling it never
  // clears a fullscreen/suspend/power hold.
  if ((pause_mask_.load(std::memory_order_acquire) & kPauseUser) != 0) {
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
      {"pause_mask", pause_mask_.load(std::memory_order_acquire)},
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
  };
  return state.dump();
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
      switch (wParam) {
        case PBT_APMSUSPEND:
          Log("power: PBT_APMSUSPEND received");
          OnSuspend();
          return TRUE;
        case PBT_APMRESUMEAUTOMATIC:
          Log("power: PBT_APMRESUMEAUTOMATIC received");
          OnResume();
          // Sleep may have recreated the desktop windows: re-anchor the
          // live surface onto the current targets.
          if (wallpaper_surface_live_.load(std::memory_order_acquire)) multi_monitor_.Reanchor();
          UpdateTrayErrorStatus();
          return TRUE;
        case PBT_APMPOWERSTATUSCHANGE:
          Log("power: PBT_APMPOWERSTATUSCHANGE received");
          if (power_saver_) power_saver_->Update();
          return TRUE;
        case PBT_POWERSETTINGCHANGE: {
          // P2.3: monitor power (GUID_MONITOR_POWER_ON). Data: 0 = off →
          // screen-off pause, 1 = on → clear, 2 = dim → ignore. Any other
          // GUID (or a null/short payload) is ignored: no pause change,
          // no crash. Suspend/resume still route via OnSuspend/OnResume
          // (resume constant is PBT_APMRESUMEAUTOMATIC — no PBT_APMRESUME).
          const auto* setting =
              reinterpret_cast<POWERBROADCAST_SETTING*>(lParam);
          if (setting == nullptr ||
              !IsEqualGUID(setting->PowerSetting, GUID_MONITOR_POWER_ON)) {
            Log("power: PBT_POWERSETTINGCHANGE unknown GUID (ignored, no pause change)");
            return TRUE;
          }
          if (setting->DataLength < sizeof(DWORD)) {
            Log("power: PBT_POWERSETTINGCHANGE monitor payload too short (%lu, ignored)",
                static_cast<unsigned long>(setting->DataLength));
            return TRUE;
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
          return TRUE;
        }
        default:
          Log("power: WM_POWERBROADCAST wParam=0x%llX (ignored)",
              static_cast<unsigned long long>(wParam));
          return TRUE;
      }
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
        TrimWorkingSetOnce();
        return 0;
      }
      // HOTFIX: debounced occlusion poke — kill immediately (transient,
      // never periodic), then run one direct coverage check.
      if (wParam == static_cast<WPARAM>(kOcclusionPokeTimerId)) {
        KillTimer(hwnd, kOcclusionPokeTimerId);
        OnOcclusionPokeTimer();
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
      ScheduleOcclusionPoke(static_cast<unsigned long>(wParam));
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
      ScheduleOcclusionPoke(static_cast<unsigned long>(wParam));
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
  LogLineV(fmt, args, false);
  va_end(args);
}

void EngineApp::LogImportant(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  LogLineV(fmt, args, true);
  va_end(args);
}

void EngineApp::LogLineV(const char* fmt, va_list args, bool important) {
  std::lock_guard<std::mutex> lock(g_log_mutex);
  SYSTEMTIME st{};
  GetLocalTime(&st);
  char ts[64] = {};
  std::snprintf(ts, sizeof(ts), "[%02u:%02u:%02u.%03u]", static_cast<unsigned>(st.wHour),
                static_cast<unsigned>(st.wMinute), static_cast<unsigned>(st.wSecond),
                static_cast<unsigned>(st.wMilliseconds));
  char msg[4096] = {};
  std::vsnprintf(msg, sizeof(msg), fmt, args);
  char line[4160] = {};
  std::snprintf(line, sizeof(line), "%s %s", ts, msg);
#if K6WP_VERBOSE
  std::fprintf(stdout, "%s\n", line);
  std::fflush(stdout);
#endif
  // Todo 11: GUI subsystem has no console — mirror every line to
  // %LOCALAPPDATA%/K6WP/engine.log (batched; important lines flush at once).
  AppendEngineLogLine(line, important);
}

}  // namespace k6wp