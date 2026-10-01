#include "fullscreen_watch.hpp"
#include "log_file.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <mutex>
#include <vector>

namespace k6wp {
namespace {

std::mutex g_fs_log_mutex;

#ifndef K6WP_VERBOSE
#define K6WP_VERBOSE 0
#endif

void Log(const char* fmt, ...) {
  std::lock_guard<std::mutex> lock(g_fs_log_mutex);
  SYSTEMTIME st{};
  GetLocalTime(&st);
  char ts[64] = {};
  std::snprintf(ts, sizeof(ts), "[%02u:%02u:%02u.%03u]",
                static_cast<unsigned>(st.wHour),
                static_cast<unsigned>(st.wMinute),
                static_cast<unsigned>(st.wSecond),
                static_cast<unsigned>(st.wMilliseconds));
  char msg[4096] = {};
  va_list args;
  va_start(args, fmt);
  std::vsnprintf(msg, sizeof(msg), fmt, args);
  va_end(args);
  char line[4160] = {};
  std::snprintf(line, sizeof(line), "%s %s", ts, msg);
#if K6WP_VERBOSE
  std::fprintf(stdout, "%s\n", line);
  std::fflush(stdout);
#endif
  // Todo 11: mirror every line to engine.log (append, flushed; ODS fallback inside).
  AppendEngineLogLine(line);
}

struct MonitorRects {
  std::vector<RECT> rects;
};

BOOL CALLBACK CollectMonitorRect(HMONITOR monitor, HDC /*dc*/, LPRECT /*clip*/,
                                 LPARAM data) {
  auto* out = reinterpret_cast<MonitorRects*>(data);
  MONITORINFO mi{};
  mi.cbSize = sizeof(mi);
  if (GetMonitorInfoW(monitor, &mi)) {
    out->rects.push_back(mi.rcMonitor);
  }
  return TRUE;
}

// P2.2 (Todo 4): PostMessage target for the WinEvent hooks. Written once
// via SetNotifyWindow on the loop thread before Start(); read from the hook
// callback (OUTOFCONTEXT delivery may arrive off the loop thread —
// PostMessageW is async-safe, and the callback does nothing else).
std::atomic<HWND> g_hook_notify_hwnd{nullptr};

// P2.2 hook callback: PostMessage ONLY. Detection (QueryFullscreen +
// debounce) runs on the loop thread in OnHookEvent, never here.
// HOTFIX: EVENT_OBJECT_DESTROY (OBJID_WINDOW only — control destroys are
// dropped here, top-level filtering happens in the loop-thread handler)
// posts PokeMessageId with the destroyed HWND in lParam; everything else
// keeps the HookMessageId path byte-identical.
void CALLBACK FullscreenWinEventProc(HWINEVENTHOOK /*hook*/, DWORD event,
                                     HWND hwnd, LONG id_object,
                                     LONG /*id_child*/, DWORD /*thread*/,
                                     DWORD /*time*/) {
  if (HWND notify = g_hook_notify_hwnd.load(std::memory_order_acquire)) {
    // HOTFIX H3/H4 only: top-level window destroy + geometry changes
    // (OBJID_WINDOW + GA_ROOT walk while the window still exists).
    // Menus/tooltips/child events are dropped here; a failed walk drops
    // too (conservative: the tick and the foreground/minimize pokes
    // still cover it). SHOW/HIDE deliberately unhooked (storm volume).
    if ((event == EVENT_OBJECT_DESTROY ||
         event == EVENT_OBJECT_LOCATIONCHANGE) &&
        id_object == OBJID_WINDOW && hwnd != nullptr &&
        GetAncestor(hwnd, GA_ROOT) == hwnd) {
      PostMessageW(notify,
                   static_cast<UINT>(FullscreenWatch::PokeMessageId()),
                   static_cast<WPARAM>(event),
                   reinterpret_cast<LPARAM>(hwnd));
      return;  // geometry events never take the 0x51 fullscreen path
    }
    if (event == EVENT_OBJECT_DESTROY ||
        event == EVENT_OBJECT_LOCATIONCHANGE) {
      return;  // non-top-level geometry noise: drop silently
    }
    PostMessageW(notify, static_cast<UINT>(FullscreenWatch::HookMessageId()),
                 static_cast<WPARAM>(event), 0);
  }
}

}  // namespace

bool FullscreenWatch::IsFullscreenCandidate(int l, int t, int r, int b,
                                            int ml, int mt, int mr, int mb,
                                            unsigned long style) {
  // Exact-equality: a maximized window overshoots the monitor rect
  // ((-8,-8)-(W+8,H+8) invisible borders), so it fails here even before
  // the style check. Both guards together reject maximized-not-fullscreen.
  const bool covers_exactly =
      (l == ml) && (t == mt) && (r == mr) && (b == mb);
  if (!covers_exactly) return false;
  const bool has_frame =
      (style & (kStyleCaption | kStyleThickFrame)) != 0;
  return !has_frame;
}

bool FullscreenWatch::QueryFullscreenTarget(void** hwnd_out) noexcept {
  if (hwnd_out != nullptr) *hwnd_out = nullptr;
  try {
    const HWND fg = GetForegroundWindow();
    if (fg == nullptr) return false;
    // The desktop itself (Progman shell window) is never "fullscreen".
    if (fg == GetShellWindow()) return false;

    RECT win{};
    if (!GetWindowRect(fg, &win)) return false;
    const LONG_PTR style = GetWindowLongPtrW(
        fg, GWL_STYLE);  // 0 + error ignored: 0 = no frame bits

    MonitorRects monitors;
    if (!EnumDisplayMonitors(nullptr, nullptr, &CollectMonitorRect,
                             reinterpret_cast<LPARAM>(&monitors))) {
      return false;
    }
    for (const RECT& mon : monitors.rects) {
      if (IsFullscreenCandidate(win.left, win.top, win.right, win.bottom,
                                mon.left, mon.top, mon.right, mon.bottom,
                                static_cast<unsigned long>(style))) {
        if (hwnd_out != nullptr) *hwnd_out = fg;
        return true;
      }
    }
    return false;
  } catch (...) {
    return false;
  }
}

FullscreenWatch::~FullscreenWatch() { Stop(); }

void FullscreenWatch::SetNotifyWindow(void* hwnd) {
  notify_hwnd_ = hwnd;
  g_hook_notify_hwnd.store(static_cast<HWND>(hwnd),
                           std::memory_order_release);
}

void FullscreenWatch::Start(FullscreenCallback on_change) {
  // Re-entrant Start: drop any previous hooks before reinstalling.
  if (hook_foreground_ != nullptr) {
    UnhookWinEvent(static_cast<HWINEVENTHOOK>(hook_foreground_));
    hook_foreground_ = nullptr;
  }
  if (hook_minimize_ != nullptr) {
    UnhookWinEvent(static_cast<HWINEVENTHOOK>(hook_minimize_));
    hook_minimize_ = nullptr;
  }
  if (hook_destroy_ != nullptr) {
    UnhookWinEvent(static_cast<HWINEVENTHOOK>(hook_destroy_));
    hook_destroy_ = nullptr;
  }
  if (hook_location_ != nullptr) {
    UnhookWinEvent(static_cast<HWINEVENTHOOK>(hook_location_));
    hook_location_ = nullptr;
  }
  hook_active_ = false;
  on_change_ = std::move(on_change);
  started_ = true;
  fullscreen_ = false;
  fullscreen_hwnd_ = nullptr;
  pending_value_ = false;
  pending_count_ = 0;
  last_poll_ = {};
  if (notify_hwnd_ == nullptr) {
    Log("fullscreen-watch: started, poll-interval=%dms (no notify window, "
        "event=hook unavailable, 1s poll fallback)",
        kPollIntervalMs);
    return;
  }
  // P2.2 (Todo 4): TWO separate hooks — one call cannot cover two disjoint
  // ranges. H1 = foreground changes, H2 = minimize start..end. Installed on
  // the calling (message-loop) thread; the callback only PostMessages.
  HWINEVENTHOOK h1 = SetWinEventHook(
      EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND, nullptr,
      &FullscreenWinEventProc, 0, 0, WINEVENT_OUTOFCONTEXT);
  if (h1 == nullptr) {
    Log("fullscreen-watch: hook H1 (foreground) failed (%lu), keeping 1s "
        "poll fallback",
        GetLastError());
  } else {
    hook_foreground_ = static_cast<void*>(h1);
  }
  // Minimize 0x0016..0x0017 range: one call covers START..END.
  HWINEVENTHOOK h2 = SetWinEventHook(
      EVENT_SYSTEM_MINIMIZESTART, EVENT_SYSTEM_MINIMIZEEND, nullptr,
      &FullscreenWinEventProc, 0, 0, WINEVENT_OUTOFCONTEXT);
  if (h2 == nullptr) {
    Log("fullscreen-watch: hook H2 (minimize) failed (%lu), keeping 1s "
        "poll fallback",
        GetLastError());
  } else {
    hook_minimize_ = static_cast<void*>(h2);
  }
  hook_active_ = (hook_foreground_ != nullptr && hook_minimize_ != nullptr);
  // HOTFIX H3: destroy notifications for the tracked-fullscreen
  // immediate clear + occlusion poke. Best-effort: failure only logs
  // (the 1 s poll fallback + foreground/minimize events still work).
  HWINEVENTHOOK h3 = SetWinEventHook(
      EVENT_OBJECT_DESTROY, EVENT_OBJECT_DESTROY, nullptr,
      &FullscreenWinEventProc, 0, 0, WINEVENT_OUTOFCONTEXT);
  if (h3 == nullptr) {
    Log("fullscreen-watch: hook H3 (destroy) failed (%lu), destroy-poke "
        "unavailable",
        GetLastError());
  } else {
    hook_destroy_ = static_cast<void*>(h3);
  }
  // HOTFIX H4: geometry changes (F11 out, un-maximize, snap) for the same
  // tracked-window immediate clear. Same best-effort discipline as H3.
  // SHOW/HIDE deliberately NOT hooked (tooltip/menu storm volume).
  HWINEVENTHOOK h4 = SetWinEventHook(
      EVENT_OBJECT_LOCATIONCHANGE, EVENT_OBJECT_LOCATIONCHANGE, nullptr,
      &FullscreenWinEventProc, 0, 0, WINEVENT_OUTOFCONTEXT);
  if (h4 == nullptr) {
    Log("fullscreen-watch: hook H4 (locationchange) failed (%lu), "
        "geometry-poke unavailable",
        GetLastError());
  } else {
    hook_location_ = static_cast<void*>(h4);
  }
  if (hook_active_) {
    Log("fullscreen-watch: started, poll-interval=%dms, event=hook watch armed "
        "(foreground+minimize%s%s, 1s poll fallback live)",
        kPollIntervalMs, hook_destroy_ ? "+destroy" : "",
        hook_location_ ? "+locationchange" : "");
  } else {
    Log("fullscreen-watch: started, poll-interval=%dms (event=hook partial, "
        "1s poll fallback)",
        kPollIntervalMs);
  }
}

void FullscreenWatch::Evaluate(bool now_fullscreen, bool from_event,
                               unsigned long win_event, void* hwnd) {
  if (now_fullscreen == fullscreen_) {
    pending_count_ = 0;
    return;
  }
  if (pending_count_ == 0 || pending_value_ != now_fullscreen) {
    pending_value_ = now_fullscreen;
    pending_count_ = 1;
  } else {
    ++pending_count_;
  }
  if (pending_count_ >= kConfirmPolls) {
    fullscreen_ = now_fullscreen;
    fullscreen_hwnd_ = now_fullscreen ? hwnd : nullptr;
    pending_count_ = 0;
    if (from_event) {
      Log("fullscreen-watch: event=hook win-event=0x%lX %s",
          win_event,
          fullscreen_ ? "fullscreen ENTERED, pausing decode (foreground-change)"
                      : "fullscreen EXITED, resuming decode (foreground-change)");
    } else {
      Log("fullscreen-watch: %s", fullscreen_ ? "fullscreen ENTERED, pausing decode"
                                              : "fullscreen EXITED, resuming decode");
    }
    if (on_change_) {
      on_change_(fullscreen_);
    }
  } else if (from_event) {
    Log("fullscreen-watch: event=hook win-event=0x%lX reading=%d "
        "(confirm %d/%d)",
        win_event, now_fullscreen ? 1 : 0, pending_count_, kConfirmPolls);
  }
}

void FullscreenWatch::OnHookEvent(unsigned long win_event) {
  if (!started_) return;
  // Bypass the 1 s Poll() throttle (event latency <500 ms) but keep the
  // 2-confirm debounce via the shared Evaluate. Stamp last_poll_ so the
  // Poll() fallback does not re-query the OS on the next loop turn.
  last_poll_ = std::chrono::steady_clock::now();
  void* hwnd = nullptr;
  const bool now_fullscreen = QueryFullscreenTarget(&hwnd);
  Evaluate(now_fullscreen, true, win_event, now_fullscreen ? hwnd : nullptr);
}

void FullscreenWatch::OnWindowDestroyed(void* hwnd) {
  if (!started_ || !fullscreen_ || hwnd == nullptr) return;
  if (hwnd != fullscreen_hwnd_) return;  // not the holder: poke side handles it
  // The tracked holder is gone: it cannot be a debounce blip, so the bit
  // clears without a second confirm. Re-query once in case a stacked
  // borderless sibling is still covering (retrack instead of clear).
  void* current = nullptr;
  if (QueryFullscreenTarget(&current)) {
    fullscreen_hwnd_ = current;
    pending_count_ = 0;
    Log("fullscreen-watch: tracked window destroyed, still fullscreen "
        "(holder retracked)");
    return;
  }
  fullscreen_ = false;
  fullscreen_hwnd_ = nullptr;
  pending_count_ = 0;
  Log("fullscreen-watch: tracked window destroyed, fullscreen CLEARED "
      "(no second confirm needed)");
  if (on_change_) {
    on_change_(false);
  }
}

void FullscreenWatch::OnWindowMoved(void* hwnd) {
  if (!started_) return;
  // Tracked holder changed geometry (F11 out, un-maximize, snap): its own
  // rect flip is decisive, not a poll blip — re-query and clear at once
  // when it no longer covers, retrack when a stacked sibling still does.
  if (fullscreen_ && hwnd != nullptr && hwnd == fullscreen_hwnd_) {
    void* current = nullptr;
    if (QueryFullscreenTarget(&current)) {
      fullscreen_hwnd_ = current;
      pending_count_ = 0;
      Log("fullscreen-watch: tracked window moved, still fullscreen "
          "(holder retracked)");
    } else {
      fullscreen_ = false;
      fullscreen_hwnd_ = nullptr;
      pending_count_ = 0;
      Log("fullscreen-watch: tracked window moved out, fullscreen CLEARED "
          "(no second confirm needed)");
      if (on_change_) {
        on_change_(false);
      }
    }
    return;
  }
  // Any other top-level move/resize: normal event evaluation (accelerates
  // F11-entry cascades, harmless otherwise) + the poke side runs next.
  OnHookEvent(EVENT_OBJECT_LOCATIONCHANGE);
}

void FullscreenWatch::Poll() {
  if (!started_) return;

  // Throttle: query the OS at most once per kPollIntervalMs even when the
  // engine's idle loop calls Poll() every ~10 ms (ConfigWatcher pattern).
  const auto now = std::chrono::steady_clock::now();
  if (now - last_poll_ < std::chrono::milliseconds(kPollIntervalMs)) return;
  last_poll_ = now;

  void* hwnd = nullptr;
  const bool now_fullscreen = QueryFullscreenTarget(&hwnd);
  Evaluate(now_fullscreen, false, 0, now_fullscreen ? hwnd : nullptr);
}

void FullscreenWatch::Stop() {
  if (hook_foreground_ != nullptr) {
    UnhookWinEvent(static_cast<HWINEVENTHOOK>(hook_foreground_));
    hook_foreground_ = nullptr;
  }
  if (hook_minimize_ != nullptr) {
    UnhookWinEvent(static_cast<HWINEVENTHOOK>(hook_minimize_));
    hook_minimize_ = nullptr;
  }
  if (hook_destroy_ != nullptr) {
    UnhookWinEvent(static_cast<HWINEVENTHOOK>(hook_destroy_));
    hook_destroy_ = nullptr;
  }
  if (hook_location_ != nullptr) {
    UnhookWinEvent(static_cast<HWINEVENTHOOK>(hook_location_));
    hook_location_ = nullptr;
  }
  hook_active_ = false;
  // Drop the delivery target: a late OUTOFCONTEXT callback must not post to a
  // window that is about to be destroyed (and the global outlives the instance).
  g_hook_notify_hwnd.store(nullptr);
  notify_hwnd_ = nullptr;
  if (!started_) return;
  on_change_ = nullptr;
  fullscreen_ = false;
  fullscreen_hwnd_ = nullptr;
  pending_value_ = false;
  pending_count_ = 0;
  last_poll_ = {};
  started_ = false;
}

}  // namespace k6wp
