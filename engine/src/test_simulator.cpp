// Hidden QA test-flag scheduler (see test_simulator.hpp).

#include "test_simulator.hpp"

#include "cli_options.hpp"

namespace k6wp {

void TestSimulator::Configure(const CliOptions& opts) {
  exit_after_ms_ = opts.exit_after_ms;
  device_lost_ms_ = opts.simulate_device_lost_after_ms;
  suspend_ms_ = opts.simulate_suspend_after_ms;
  dc_ms_ = opts.simulate_dc_after_ms;
  monitor_off_ms_ = opts.simulate_monitor_off_after_ms;
}

bool TestSimulator::AnyArmed() const {
  if (exit_after_ms_ > 0) return true;
  if (device_lost_ms_ > 0 && !device_lost_fired_) return true;
  if (suspend_ms_ > 0 && !resume_fired_) return true;
  if (dc_ms_ > 0 && !dc_restore_fired_) return true;
  if (monitor_off_ms_ > 0 && !monitor_on_fired_) return true;
  return false;
}

TestSimulator::Fired TestSimulator::Tick(std::int64_t elapsed_ms) {
  Fired f;
  if (device_lost_ms_ > 0 && !device_lost_fired_ &&
      elapsed_ms >= device_lost_ms_) {
    device_lost_fired_ = true;
    f.device_lost = true;
  }
  if (suspend_ms_ > 0 && !suspend_fired_ && elapsed_ms >= suspend_ms_) {
    suspend_fired_ = true;
    f.suspend = true;
  }
  if (suspend_ms_ > 0 && !resume_fired_ && elapsed_ms >= suspend_ms_ + 2000) {
    resume_fired_ = true;
    f.resume = true;
  }
  if (dc_ms_ > 0 && !dc_fired_ && elapsed_ms >= dc_ms_) {
    dc_fired_ = true;
    dc_latched_ = true;
    f.dc_on = true;
  }
  if (dc_ms_ > 0 && !dc_restore_fired_ && elapsed_ms >= dc_ms_ + 2000) {
    dc_restore_fired_ = true;
    dc_latched_ = false;
    f.dc_restore = true;
  }
  if (monitor_off_ms_ > 0 && !monitor_off_fired_ &&
      elapsed_ms >= monitor_off_ms_) {
    monitor_off_fired_ = true;
    f.monitor_off = true;
  }
  if (monitor_off_ms_ > 0 && !monitor_on_fired_ &&
      elapsed_ms >= monitor_off_ms_ + 2000) {
    monitor_on_fired_ = true;
    f.monitor_on = true;
  }
  return f;
}

}  // namespace k6wp
