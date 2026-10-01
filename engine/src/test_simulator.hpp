#pragma once

#include <cstdint>

namespace k6wp {

struct CliOptions;

// Hidden QA test-flag scheduler (--exit-after-ms, --simulate-*-after-ms). Owns
// the after-ms values and the fired flags; Run() calls Tick() each loop and
// performs the side effects (logging, PostMessage/SendMessage, pause owners,
// device-lost flag). Kept out of EngineApp so the loop only orchestrates.
class TestSimulator {
 public:
  struct Fired {
    bool device_lost = false;
    bool suspend = false;
    bool resume = false;
    bool dc_on = false;       // forced DC latched on
    bool dc_restore = false;  // forced DC cleared
    bool monitor_off = false;
    bool monitor_on = false;
  };

  void Configure(const CliOptions& opts);

  bool AnyArmed() const;

  bool ExitReached(std::int64_t elapsed_ms) const {
    return exit_after_ms_ > 0 && elapsed_ms >= exit_after_ms_;
  }

  int exit_after_ms() const { return exit_after_ms_; }
  int device_lost_after_ms() const { return device_lost_ms_; }
  int suspend_after_ms() const { return suspend_ms_; }
  int dc_after_ms() const { return dc_ms_; }
  int monitor_off_after_ms() const { return monitor_off_ms_; }
  bool dc_latched() const { return dc_latched_; }

  // Advances the schedule and reports which events just fired.
  Fired Tick(std::int64_t elapsed_ms);

 private:
  int exit_after_ms_ = 0;
  int device_lost_ms_ = 0;
  int suspend_ms_ = 0;
  int dc_ms_ = 0;
  int monitor_off_ms_ = 0;
  bool device_lost_fired_ = false;
  bool suspend_fired_ = false;
  bool resume_fired_ = false;
  bool dc_fired_ = false;
  bool dc_restore_fired_ = false;
  bool monitor_off_fired_ = false;
  bool monitor_on_fired_ = false;
  bool dc_latched_ = false;
};

}  // namespace k6wp
