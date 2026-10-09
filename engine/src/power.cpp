// engine/src/power.cpp
// Engine battery-saver (Todo 40): GetSystemPowerStatus → cap 24 fps on DC.

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include "power.hpp"

#include <cstdio>

namespace k6wp {
namespace {

// DC cap mandated by the task acceptance (≤2 s transition, 24 fps).
constexpr int kDcFpsCap = 24;

std::string FormatRestore(int fps) {
  char buf[96] = {};
  std::snprintf(buf, sizeof(buf),
                "AC line restored, back to full rate (%dfps)", fps);
  return std::string(buf);
}

}  // namespace

PowerSaver::PowerSaver(CapApplier cap, Reader reader, LogFn log)
    : cap_(std::move(cap)), reader_(std::move(reader)), log_(std::move(log)) {}

PowerCapAction DecidePowerCapAction(PowerSaverState state,
                                    const std::string& battery_mode) {
  if (state == PowerSaverState::kDcCapped) {
    return battery_mode == "static" ? PowerCapAction::kFreeze
                                    : PowerCapAction::kCapFps;
  }
  return PowerCapAction::kNone;
}

void PowerSaver::Update() {
  if (!reader_ || !cap_) return;
  const PowerReading reading = reader_();
  const int restore =
      (reading.fps_restore >= 1 && reading.fps_restore <= 30)
          ? reading.fps_restore
          : 30;

  // Config OFF → never cap; lift a live cap if the user just disabled it.
  if (!reading.battery_saver_on) {
    if (state_ == PowerSaverState::kDcCapped) {
      cap_(PowerSaverState::kDisabled, 0);
      Log("battery_saver off in config, restoring full rate");
    }
    state_ = PowerSaverState::kDisabled;
    inert_logged_ = false;
    return;
  }

  // No system battery (desktop) → inert. Log once; also defensively clear a
  // stale cap so a battery-less box can never stay capped.
  if (!reading.has_battery) {
    if (!inert_logged_) {
      Log("no battery, saver inert");
      inert_logged_ = true;
    }
    if (state_ == PowerSaverState::kDcCapped) cap_(PowerSaverState::kInertNoBattery, 0);
    state_ = PowerSaverState::kInertNoBattery;
    return;
  }
  inert_logged_ = false;

  // DC only when truly offline (ACLineStatus == 0). Unknown AC (with a
  // battery present) is treated as AC: never cap without certain DC.
  const bool on_dc = !reading.ac_unknown && !reading.ac_online;
  if (on_dc) {
    if (state_ != PowerSaverState::kDcCapped) {
      cap_(PowerSaverState::kDcCapped, kDcFpsCap);
      Log("DC power (battery), capping to 24fps");
      state_ = PowerSaverState::kDcCapped;
    }
    return;
  }

  if (state_ == PowerSaverState::kDcCapped) {
    cap_(PowerSaverState::kAcFullRate, restore);
    Log(FormatRestore(restore));
  } else if (state_ != PowerSaverState::kAcFullRate) {
    Log("AC line, full rate");
  }
  state_ = PowerSaverState::kAcFullRate;
}

PowerReading ReadSystemPower(const WallpaperConfig& config) {
  PowerReading reading;
  reading.battery_saver_on = config.battery_saver;
  reading.fps_restore = config.fps_cap;

  SYSTEM_POWER_STATUS status{};
  if (!GetSystemPowerStatus(&status)) {
    // Query failed → never cap: report unknown AC with a battery assumed.
    reading.ac_online = true;
    reading.ac_unknown = true;
    reading.has_battery = true;
    return reading;
  }
  // ACLineStatus: 0 = offline (DC), 1 = online (AC), 128 = unknown.
  reading.ac_unknown = (status.ACLineStatus == 128);
  reading.ac_online = (status.ACLineStatus == 1);
  // BatteryFlag bit 7 (128) = no system battery. 255 (unknown status) has
  // bit 7 set too → has_battery false → inert path on desktops.
  reading.has_battery = ((status.BatteryFlag & 128) == 0);
  return reading;
}

}  // namespace k6wp
