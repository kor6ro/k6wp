#pragma once

// Per-monitor occlusion watcher (P2.5, Todo 9).
//
// Pauses individual slots whose monitor is >=95% covered by visible
// top-level windows, resumes below 90% (hysteresis). Pauses go through
// MultiMonitor::PauseSlot per-slot only — global pause bits are never
// touched, and the fullscreen guard is independent (global bits override).
//
// Cadence: NO dedicated timer. The engine loop already waits 1500 ms while
// unpaused (Todo 7); Check() runs on that wake with an internal 1500 ms
// throttle, and the loop wait is INFINITE while SlotsPaused(), so a paused
// engine has zero occlusion wakeups (self-suspending by construction).
//
// Synchronization (Oracle round-03 issue 2): ApplyPauseState also runs on
// the IPC worker thread, so the arm/disarm flag is the mandatory atomic
// occlusion_armed_. The IPC-thread path never touches loop-thread timers —
// OnPauseMaskChanged(false→unpaused) only stores the flag and PostMessage()s
// the re-arm note to the loop thread. The per-slot flags touched here are
// the Todo 8 atomics (loop-thread PauseSlot vs IPC-thread ResumeAll).
//
// Header stays windows.h-free (std types only); Win32 lives in the .cpp.
// RAII: no handles owned (dwmapi is loaded once and cached for the process
// lifetime, the notify HWND is borrowed). Namespace k6wp, Log() pattern.
//
// Polling policy: the occlusion tick is the engine's highest-cadence
// intentional poll (1500 ms). FullscreenWatch (1 s) and ConfigWatcher
// (500 ms) also carry throttled Poll() fallbacks beside their primary
// event hooks; the IPC server and power path are fully event-driven.

#include <atomic>
#include <chrono>
#include <cstddef>

#include "multi_monitor.hpp"

namespace k6wp {

// Axis-aligned rect in virtual-screen coordinates (same space as
// MonitorInfo x/y/width/height).
struct CoverRect {
  int left = 0;
  int top = 0;
  int right = 0;
  int bottom = 0;
};

inline CoverRect MonitorCover(const MonitorInfo& mi) noexcept {
  CoverRect r;
  r.left = mi.x;
  r.top = mi.y;
  r.right = mi.x + mi.width;
  r.bottom = mi.y + mi.height;
  return r;
}

// Pure helpers (unit-testable without live windows; Todo 12 covers them):

// Intersect r with clip (empty when disjoint or degenerate).
CoverRect ClipRect(CoverRect r, CoverRect clip) noexcept;

// Union area of rects after clipping each to clip. Allocation-free
// sweep-line over stack scratch (NO CombineRgn-per-window, NO naive
// sum-area — overlapping windows must not double-count). Never throws;
// windows beyond the fixed scratch cap are ignored (conservative: coverage
// can only under-report, never over-pause).
long long UnionAreaClipped(const CoverRect* rects, size_t count,
                           CoverRect clip) noexcept;

// Fraction of monitor covered by rects in [0,1] (0 when the monitor area
// is degenerate or rects is null/empty).
double ComputeCoverage(CoverRect monitor, const CoverRect* rects,
                       size_t count) noexcept;

// Hysteresis: pause at coverage >= 95%, resume at < 90%, hold between.
// Boundary-exact: 95.0% pauses, 90.0% stays paused, 89.9% resumes.
inline bool HysteresisDecide(double coverage, bool currently_paused) noexcept {
  if (currently_paused) {
    return coverage >= 0.90;
  }
  return coverage >= 0.95;
}

class OcclusionWatch {
 public:
  // Unpaused-loop cadence doubling as the occlusion tick (Todo 7).
  static constexpr int kTickMs = 1500;
  static constexpr double kPauseAt = 0.95;
  static constexpr double kResumeBelow = 0.90;

  // Loop-thread re-arm note: WM_APP (0x8000) + 0x52. Posted by
  // OnPauseMaskChanged when the mask returns to zero (any thread posts;
  // the loop thread consumes in HandleMessage). Chosen to avoid WM_APP+1
  // (shutdown), WM_APP+0x14 (tray), WM_APP+0x50 (mpv hwdec),
  // WM_APP+0x51 (fullscreen hook) and WM_APP+0x53 (destroy poke).
  static constexpr unsigned RearmMessageId() { return 0x8000u + 0x0052u; }

  OcclusionWatch() = default;
  ~OcclusionWatch() = default;

  OcclusionWatch(const OcclusionWatch&) = delete;
  OcclusionWatch& operator=(const OcclusionWatch&) = delete;

  void SetLog(LogFn log) { log_ = log; }

  // Borrowed loop-thread window for the re-arm PostMessage. nullptr =
  // no wake post (armed flag still flips; the loop picks it up).
  void SetNotifyWindow(void* hwnd) { notify_hwnd_ = hwnd; }

  // Called at the end of ApplyPauseState on ANY thread (loop or IPC
  // worker). slots_paused=false (mask zero) re-arms: sets the atomic and
  // wakes the loop thread via PostMessage (never KillTimer/SetTimer here —
  // those are affine to the loop thread). slots_paused=true disarms.
  void OnPauseMaskChanged(bool slots_paused);

  bool armed() const { return occlusion_armed_.load(std::memory_order_acquire); }

  // Consumes RearmMessageId on the loop thread (wake already happened;
  // logs the re-arm for evidence). Harmless no-op otherwise.
  void OnRearmMessage() noexcept;

  // One occlusion tick. Loop thread ONLY. Early-outs while disarmed
  // (any pause owner active) and throttles to kTickMs between live scans.
  // Per slot: EnumWindows snapshot → clipped union coverage → hysteresis
  // vs IsSlotPaused → PauseSlot on transitions (logged). Never touches
  // global pause bits. Never throws.
  void Check(MultiMonitor& slots);
  // HOTFIX (occlusion resume): one unthrottled scan, same body as Check.
  // Called from the debounced destroy/foreground poke on the loop thread
  // (bypasses BOTH the disarm gate and the kTickMs throttle — that is the
  // point: resume must not wait for the tick or the mask). Hysteresis via
  // the shared HysteresisDecide. Never touches global pause bits.
  void CheckNow(MultiMonitor& slots);

 private:
  void Log(const char* fmt, ...);

  LogFn log_ = nullptr;
  void* notify_hwnd_ = nullptr;  // borrowed engine hidden window
  // MANDATORY atomic (see file comment): flipped from ApplyPauseState on
  // either the loop or the IPC worker thread, read by Check on the loop.
  std::atomic<bool> occlusion_armed_{true};
  // Loop-thread only: last live scan (throttle source).
  std::chrono::steady_clock::time_point last_check_{};
};

}  // namespace k6wp
