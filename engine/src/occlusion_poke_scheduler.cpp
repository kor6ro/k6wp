#include "occlusion_poke_scheduler.hpp"

#include "timer_ids.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <utility>

namespace k6wp {

namespace {
constexpr UINT kPokeDebounceMs = 150;
}

void OcclusionPokeScheduler::SetHooks(void* hwnd, BoolFn surface_live,
                                      BoolFn any_slot_paused, VoidFn check_now) {
  hwnd_ = hwnd;
  surface_live_ = std::move(surface_live);
  any_slot_paused_ = std::move(any_slot_paused);
  check_now_ = std::move(check_now);
}

void OcclusionPokeScheduler::Schedule(unsigned long win_event) {
  // Loop thread only (called from HandleMessage). Cheap guard first: no
  // occlusion-paused slot means no poke (foreground/minimize storms cost
  // one slot-count scan, never an EnumWindows).
  if (armed_ || !surface_live_ || !surface_live_()) return;
  if (!any_slot_paused_ || !any_slot_paused_()) return;
  if (hwnd_ == nullptr) return;
  if (SetTimer(static_cast<HWND>(hwnd_),
               static_cast<UINT_PTR>(kOcclusionPokeTimerId), kPokeDebounceMs,
               nullptr) == 0) {
    // Fail-safe (still loop thread): run the check inline instead of
    // silently dropping the poke.
    if (log_) {
      log_("warning: occlusion poke SetTimer failed (error %lu), checking now",
           GetLastError());
    }
    if (check_now_) check_now_();
    return;
  }
  armed_ = true;
  if (log_) log_("occlusion: poke scheduled (win-event=0x%lX)", win_event);
}

void OcclusionPokeScheduler::OnTimer() {
  armed_ = false;
  if (surface_live_ && !surface_live_()) return;
  if (log_) log_("occlusion: poke check");
  if (check_now_) check_now_();
}

}  // namespace k6wp
