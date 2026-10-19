#pragma once

// Renderer graveyard - deferred teardown of mpv renderers and of the
// windows they were rendering into.
//
// WHY THIS EXISTS (Bug B: freeze when the monitor count drops)
// ~MpvRenderer calls mpv_terminate_destroy(), which waits for the mpv VO
// thread. When a monitor is unplugged that VO thread can be stuck inside
// Present() on a device that no longer exists, and the wait has no timeout.
// The engine used to run that destructor on its message-loop thread, so the
// whole engine (tray menu, IPC, timers, power broadcast) froze while the
// tray icon stayed on screen: the observed "still visible but not
// responding" freeze. crash_restart_guard only covers CRASHES (WER), so a
// hang was never recovered.
//
// WHAT THIS DOES
// The renderer OBJECT is handed to a background worker that owns the
// blocking destructor. The message-loop thread never waits for it.
//
// The window the VO was rendering into must OUTLIVE the renderer: Windows
// forbids destroying a window while a foreign thread still renders into it,
// and mpv's VO owns a child window inside the injected surface. So the
// window is parked here as well and destroyed by the MAIN thread in
// Drain() - once the worker reports the renderer really gone.
//
// CONTRACT DEVIATION (docs/dev-contracts.md, "structural slot changes")
// The DECISION to destroy a slot is still taken on the message-loop thread
// and the slot map is still only mutated there. What moved off that thread
// is the unbounded blocking wait (the renderer destructor). The alternative
// - waiting on the message-loop thread - is exactly the hang this fixes.
//
// windows.h-free (the HWND travels as void*, the same pattern as
// device_lost_watch.hpp), so the header can be included from windows.h-free
// translation units.

#include <memory>

#include "mpv_renderer.hpp"

namespace k6wp {

class RendererGraveyard {
 public:
  // Matches EngineApp::Log (void(const char* fmt, ...)); may be null.
  using LogFn = void (*)(const char* fmt, ...);

  // Hands the renderer (and the window it was rendering into) to the
  // background worker.
  //
  // `renderer` may be null (nothing to destroy) as long as a window is
  // given - the window is then destroyed on the next Drain().
  // `window_to_destroy_after` may be null.
  //
  // Ownership of `renderer` moves to the graveyard. Returns false when the
  // graveyard could not take the work (the worker thread could not be
  // created): the caller keeps ownership and must destroy the renderer
  // itself. Never blocks, never throws.
  static bool Submit(std::unique_ptr<MpvRenderer> renderer,
                     void* window_to_destroy_after, LogFn log = nullptr);

  // MAIN THREAD ONLY. Destroys the windows whose renderer is already gone.
  // Returns the number of windows destroyed. Never blocks on the worker
  // (it only takes the ready list) and never throws.
  static int Drain();

  // Waits until the worker has destroyed every submitted renderer and no
  // item is in flight (bounded by `timeout_ms`). Used by the tests and by
  // the engine's shutdown path; the message loop never needs it.
  static bool WaitIdle(int timeout_ms);

  // Stops the worker. BOUNDED: a worker stuck inside the renderer
  // destructor is left detached (leaked) rather than joined forever - a
  // stuck teardown must never block the engine's exit. Returns true when
  // the worker stopped cleanly within `timeout_ms`.
  static bool Shutdown(int timeout_ms, LogFn log = nullptr);

  // Diagnostics: how many renderers were handed over, how many were really
  // destroyed, and how many windows are still waiting for their turn.
  static unsigned long long submitted_count();
  static unsigned long long destroyed_count();
  static int pending_window_count();
};

}  // namespace k6wp
