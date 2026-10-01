#pragma once

#include <functional>

namespace k6wp {

// HOTFIX (occlusion resume): debounced poke. Loop thread only. Arms a
// one-shot timer when at least one slot is occlusion-paused; the timer handler
// runs one direct coverage check. Fail-safe: a failed SetTimer checks inline.
class OcclusionPokeScheduler {
 public:
  using LogFn = void (*)(const char* fmt, ...);
  using BoolFn = std::function<bool()>;
  using VoidFn = std::function<void()>;

  explicit OcclusionPokeScheduler(LogFn log = nullptr) : log_(log) {}

  // Set once after the hidden window exists; the callbacks run on the loop
  // thread only.
  void SetHooks(void* hwnd, BoolFn surface_live, BoolFn any_slot_paused,
                VoidFn check_now);

  void Schedule(unsigned long win_event);
  void OnTimer();
  void Disarm() { armed_ = false; }

 private:
  LogFn log_;
  void* hwnd_ = nullptr;
  BoolFn surface_live_;
  BoolFn any_slot_paused_;
  VoidFn check_now_;
  bool armed_ = false;
};

}  // namespace k6wp
