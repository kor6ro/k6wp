#pragma once

// Engine battery-saver (Todo 40, fase-6).
//
// Reduces decode cost on DC (battery) power when the `battery_saver` config
// flag is ON: caps output to 24 fps, restores the configured `fps_cap`
// (default 30) on AC. A battery-less box (desktop: ACLineStatus == Unknown +
// BatteryFlag & NoSystemBattery) leaves the feature inert with a log line.
//
// Header stays windows.h-free (std types + config_schema only); Win32
// (GetSystemPowerStatus) lives in power.cpp. RAII only, no new/delete.
//
// Implementation choice (documented per task): the fps cap uses mpv's
// runtime-settable `vf` property with the `fps=N` filter
// (MpvRenderer::SetFpsCap) — no re-encode, no pause, no stream reload.
// The "pause-total" alternative was rejected: the config schema exposes only
// a single `battery_saver` bool (no pause-total flag), and pausing would
// fight the user-visible pause state owned by Todo 35 (tray/IPC/suspend).
// PowerSaver never touches paused_/tray — suspend/resume (Todo 8/35) coexist
// untouched.
//
// The cap is applied through a caller-supplied CapApplier instead of a
// renderer reference so the engine can fan the throttle out to EVERY
// decoder (headless renderer + live MultiMonitor slots); the applier also
// receives the new PowerSaverState so it can mirror the DC throttle on
// decoders that have no fps-cap API (the live slots pause on DC).

#include <functional>
#include <string>

#include "config_schema.hpp"

namespace k6wp {

// Battery-saver state machine (for logs / tests).
enum class PowerSaverState {
  kDisabled,        // battery_saver OFF in config — never caps.
  kInertNoBattery,  // desktop / no system battery — feature inert.
  kAcFullRate,      // AC line (or unknown AC with a battery) — full rate.
  kDcCapped,        // DC line + saver ON — capped to 24 fps.
};

// Windows.h-free snapshot of the power situation. Produced by
// ReadSystemPower() (live Win32 query + config merge) or injected by tests.
struct PowerReading {
  bool ac_online = true;       // ACLineStatus == 1.
  bool ac_unknown = false;     // ACLineStatus == 128 (or query failed).
  bool has_battery = true;     // false when BatteryFlag & 128 (NoSystemBattery).
  bool battery_saver_on = false;  // WallpaperConfig::battery_saver.
  int fps_restore = 30;        // WallpaperConfig::fps_cap (AC restore target).
};

class PowerSaver {
 public:
  // Applies the throttle: `fps` is the mpv vf fps value (<= 0 clears the
  // cap); `state` is the new state the machine is entering, so the applier
  // can mirror the throttle on decoders without an fps-cap API.
  using CapApplier = std::function<void(PowerSaverState state, int fps)>;
  using Reader = std::function<PowerReading()>;
  using LogFn = std::function<void(const std::string&)>;

  // cap: applies the fps cap to every decoder (headless renderer + live
  //   slots). reader: returns the current PowerReading (normally a lambda
  //   around ReadSystemPower(config_watcher.GetConfig()); tests inject
  //   DC/AC). log: single-string sink (std::function<void(const char*, ...)>
  //   is illegal C++ — variadic function types are rejected, cf. Todo 9).
  PowerSaver(CapApplier cap, Reader reader, LogFn log = LogFn());
  ~PowerSaver() = default;

  PowerSaver(const PowerSaver&) = delete;
  PowerSaver& operator=(const PowerSaver&) = delete;

  // Reads the current status and applies cap/restore. Idempotent: repeated
  // calls with the same reading only act on transitions. Safe to call from
  // the UI thread (WndProc PBT_APMPOWERSTATUSCHANGE), Init(), and the
  // config on_change callback.
  void Update();

  PowerSaverState state() const { return state_; }
  static const char* StateToString(PowerSaverState state);

 private:
  void Log(const std::string& message) const {
    if (log_) log_(message);
  }

  CapApplier cap_;
  Reader reader_;
  LogFn log_;
  // Starts kDisabled so the first Update() with the saver ON always emits
  // its path line (AC full-rate / DC capped / inert) — no silent startup.
  PowerSaverState state_ = PowerSaverState::kDisabled;
  bool inert_logged_ = false;
};

// Live Win32 query (GetSystemPowerStatus) merged with the current config.
// Edge cases: ACLineStatus 128 (Unknown) + BatteryFlag & 128
// (NoSystemBattery, incl. BatteryFlag 255 = unknown) → has_battery false →
// the inert path. DC is asserted ONLY on ACLineStatus == 0 (truly offline).
PowerReading ReadSystemPower(const WallpaperConfig& config);

}  // namespace k6wp
