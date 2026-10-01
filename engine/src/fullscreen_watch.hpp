#pragma once

#include <chrono>
#include <functional>

namespace k6wp {

// Watches the foreground window (~1 s poll) and reports when a true
// fullscreen app (DX game, fullscreen video, borderless window) covers a
// whole monitor, so the engine can pause decoding (CPU ~0%) and resume on
// minimize/exit (resume latency <= ~1 s + one poll).
//
// False-positive guard is the CORE rule: a maximized-but-captioned window
// must NOT trigger. Maximized windows carry WS_CAPTION/WS_THICKFRAME and
// their rect overshoots the monitor (typically (-8,-8)-(W+8,H+8) on Win10+
// invisible borders). Fullscreen requires BOTH:
//   1. window rect EXACTLY equals a monitor rect, AND
//   2. style has no WS_CAPTION / WS_THICKFRAME bits.
//
// Header stays windows.h-free (std types only); Win32 lives in the .cpp.
// RAII: Start() begins watching, Stop() ends it; dtor stops implicitly.
class FullscreenWatch {
 public:
  using FullscreenCallback = std::function<void(bool fullscreen)>;

  FullscreenWatch() = default;
  ~FullscreenWatch();

  FullscreenWatch(const FullscreenWatch&) = delete;
  FullscreenWatch& operator=(const FullscreenWatch&) = delete;

  // Begins watching; `on_change` fires on each fullscreen<->windowed
  // transition. Empty callback is allowed (Poll() still updates state).
  void Start(FullscreenCallback on_change);

  // Polls the foreground window. Call from the engine's idle loop.
  // Internally throttled: the OS is queried at most once per
  // kPollIntervalMs (1000 ms), so pause latency after entering fullscreen
  // is <= ~2 s and resume latency <= ~1-2 s.
  void Poll();

  // Stops watching and clears state. Idempotent.
  void Stop();

  // P2.2 (Todo 4) event path: hidden message window the WinEvent hook
  // callback PostMessage()s to (WM_APP + HookMessageId offset). Wired once
  // from CreateMessageWindow (before Start); cleared implicitly by Stop().
  // Win32-free signature: HWND as void*. When unset (nullptr, e.g. unit
  // probes without a window) Start() skips hook installation and keeps the
  // 1 s Poll() fallback with one reason log line.
  void SetNotifyWindow(void* hwnd);

  // P2.2 (Todo 4) event path: runs the byte-identical detection
  // (QueryFullscreen + IsFullscreenCandidate + kConfirmPolls debounce) on
  // the loop thread after a hook callback PostMessage. Bypasses the 1 s
  // Poll() throttle (that is the point: <500 ms event latency) but shares
  // the debounce state machine, so UAC/Alt-Tab blips are still rejected.
  // Harmless no-op when not started.
  void OnHookEvent(unsigned long win_event);

  // P2.2 (Todo 4) hook-callback message: WM_APP (0x8000) + 0x51. Posted by
  // the WinEventProc on foreground/minimize events; the engine WndProc
  // routes it to OnHookEvent(). Chosen to avoid WM_APP+1 (shutdown),
  // WM_APP+20 (tray) and WM_APP+0x50 (mpv hwdec change).
  static constexpr unsigned HookMessageId() { return 0x8000u + 0x0051u; }

  // HOTFIX (occlusion resume): destroy-poke message WM_APP + 0x53. Posted
  // by the WinEventProc on EVENT_OBJECT_DESTROY / EVENT_OBJECT_LOCATIONCHANGE
  // (OBJID_WINDOW + top-level only) with wParam = win-event id, lParam = HWND.
  // The engine WndProc routes it to FullscreenWatch::OnWindowDestroyed /
  // OnWindowMoved (tracked-fullscreen immediate clear) plus a debounced
  // occlusion poke. Chosen to avoid +0x52 (occlusion re-arm); full map lives
  // on EngineApp::kMpvHwdecChangeMessage.
  static constexpr unsigned PokeMessageId() { return 0x8000u + 0x0053u; }

  // HOTFIX (occlusion resume): destroy of a tracked fullscreen window.
  // Loop thread only (routed from the PokeMessageId handler). When the
  // destroyed HWND is the tracked fullscreen holder, the bit clears
  // IMMEDIATELY (a destroyed window cannot be a debounce blip): re-query
  // once — still covered (stacked borderless pair) retracks the new
  // holder, otherwise fullscreen_=false + on_change_(false) fire at once.
  // Any other HWND (or inactive watcher) is a silent no-op. Never throws.
  // Public because EngineApp routes the destroy message to it.
  void OnWindowDestroyed(void* hwnd);

  // HOTFIX (F11-out): geometry change of a top-level window.
  // Loop thread only (routed from the PokeMessageId handler with the
  // moved HWND). Tracked holder → decisive immediate re-query (same
  // blip-proof reasoning as destroy); anything else → normal event
  // evaluation (F11-entry cascades complete without poll luck).
  // Public because EngineApp routes the locationchange message to it.
  void OnWindowMoved(void* hwnd);

  // windows.h-free predicate for the core rule (also directly testable):
  // rect (l,t,r,b) covers monitor (ml,mt,mr,mb) exactly AND `style` has no
  // caption/thick-frame bits. `style` is the GetWindowLongPtrW(GWL_STYLE) value.
  static bool IsFullscreenCandidate(int l, int t, int r, int b, int ml,
                                    int mt, int mr, int mb,
                                    unsigned long style);

 private:
  static constexpr int kPollIntervalMs = 1000;

  // Style bits mirrored from WinUser.h so the header stays Win32-free.
  static constexpr unsigned long kStyleCaption = 0x00C00000UL;     // WS_CAPTION
  static constexpr unsigned long kStyleThickFrame = 0x00040000UL;  // WS_THICKFRAME

  FullscreenCallback on_change_;
  bool started_ = false;
  bool fullscreen_ = false;

  // Debounce: a transition fires only after kConfirmPolls consecutive
  // identical readings (1 s poll each). Single-poll blips (UAC dim,
  // Alt+Tab flash) no longer pause/resume decode in a 1 s flap.
  static constexpr int kConfirmPolls = 2;
  bool pending_value_ = false;
  int pending_count_ = 0;

  // Last time Poll() queried the OS (throttle). Default-constructed
  // (epoch) so the first Poll() after Start() proceeds immediately.
  std::chrono::steady_clock::time_point last_poll_{};

  // Queries the OS once (foreground window vs monitor rects + style).
  // Never throws; false on any API failure. When non-null, *hwnd_out
  // receives the matched fullscreen HWND (or null when no match) so the
  // destroy-poke can track exactly which window holds the bit.
  static bool QueryFullscreenTarget(void** hwnd_out) noexcept;

  // Shared debounce transition: folds one OS reading into pending_* and
  // fires on_change_ after kConfirmPolls consecutive identical readings.
  // from_event selects the event=hook log tag (hook path) vs the plain
  // poll tag (fallback path). win_event is the triggering WinEvent id
  // (0 for the poll path), logged for evidence. hwnd is the fullscreen
  // candidate window when now_fullscreen is true (else null); stored as
  // fullscreen_hwnd_ on the transition so a later destroy can clear the
  // bit without waiting for a second confirm.
  void Evaluate(bool now_fullscreen, bool from_event, unsigned long win_event,
                void* hwnd);

  private:
  void* notify_hwnd_ = nullptr;  // engine hidden window (void* HWND)
  void* hook_foreground_ = nullptr;  // H1 HWINEVENTHOOK (void*, .cpp owns)
  void* hook_minimize_ = nullptr;    // H2 HWINEVENTHOOK (void*, .cpp owns)
  void* hook_destroy_ = nullptr;     // H3 EVENT_OBJECT_DESTROY (void*, .cpp owns)
  void* hook_location_ = nullptr;    // H4 EVENT_OBJECT_LOCATIONCHANGE (void*, .cpp owns)
  bool hook_active_ = false;  // both hooks installed -> event path live
  // HOTFIX: HWND holding the fullscreen bit (void* HWND, set on the
  // transition to true, cleared on false). Loop thread only.
  void* fullscreen_hwnd_ = nullptr;
};

}  // namespace k6wp
