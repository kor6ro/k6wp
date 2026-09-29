// Per-monitor occlusion watcher (P2.5, Todo 9). See occlusion_watch.hpp.
//
// windows.h lives here only. DWMWA_CLOAKED is resolved through a cached
// dwmapi.dll module (LOW-7: LoadLibrary once, FreeLibrary at shutdown via
// the RAII holder below — no per-tick load/unload, no link dependency, no
// CMake change): when unavailable every window is treated as uncloaked plus
// one reason log line (logged-fallback convention).

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <algorithm>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <vector>

#include "monitor_util.hpp"
#include "occlusion_watch.hpp"

namespace k6wp {

namespace {

// Fixed scratch caps (stack only — the union path never heap-allocates and
// never creates HRGNs). 768 top-level windows per tick is far above any
// real desktop; beyond it extras are ignored (coverage under-reports, so
// the failure direction is "no pause", never a phantom pause).
constexpr size_t kMaxWindows = 768;
// Live slots per engine (map iteration order = PauseSlot idx order).
constexpr size_t kMaxSlots = 32;

// DWMWA_CLOAKED value (dwmapi.h, kept local so no link dependency).
constexpr unsigned kDwmWaCloaked = 14;
using DwmGetWindowAttributeFn = HRESULT(WINAPI*)(HWND, DWORD, PVOID, DWORD);

// LOW-7: process-wide cached dwmapi module. LoadLibraryW runs exactly once
// (first tick), GetProcAddress resolves DwmGetWindowAttribute a single time,
// and the destructor FreeLibrary()es at shutdown. CheckNow runs on the loop
// thread only, and function-local statics init thread-safely, so no lock is
// needed for the read-mostly handle.
struct CachedDwmApi {
  CachedDwmApi() {
    handle = ::LoadLibraryW(L"dwmapi.dll");
    if (handle != nullptr) {
      get_attr = reinterpret_cast<DwmGetWindowAttributeFn>(
          ::GetProcAddress(handle, "DwmGetWindowAttribute"));
    }
  }
  ~CachedDwmApi() {
    if (handle != nullptr) {
      ::FreeLibrary(handle);
    }
  }
  CachedDwmApi(const CachedDwmApi&) = delete;
  CachedDwmApi& operator=(const CachedDwmApi&) = delete;

  HMODULE handle = nullptr;
  DwmGetWindowAttributeFn get_attr = nullptr;
};

// Shell classes skipped exactly per plan (Oracle O9): the shell window
// itself plus Progman / WorkerW / Shell_TrayWnd (multi-WorkerW variants
// across Windows 10/11). Wallpaper injector children live in the engine
// PID and are covered by the own-PID skip.
bool IsShellClass(const wchar_t* name) noexcept {
  if (name == nullptr || name[0] == L'\0') return false;
  auto Eq = [](const wchar_t* a, const wchar_t* b) {
    for (;; ++a, ++b) {
      if (*a != *b) return false;
      if (*a == L'\0') return true;
    }
  };
  return Eq(name, L"Progman") || Eq(name, L"WorkerW") ||
         Eq(name, L"Shell_TrayWnd");
}

struct CollectCtx {
  DWORD own_pid = 0;
  HWND shell_wnd = nullptr;
  DwmGetWindowAttributeFn dwm_get_attr = nullptr;
  CoverRect out[kMaxWindows];
  size_t count = 0;
  size_t overflow = 0;
};

BOOL CALLBACK CollectWindows(HWND hwnd, LPARAM lparam) {
  auto* ctx = reinterpret_cast<CollectCtx*>(lparam);
  if (ctx == nullptr) return FALSE;
  // Invisible or minimized: cannot occlude the wallpaper.
  if (!::IsWindowVisible(hwnd) || ::IsIconic(hwnd)) return TRUE;
  // Own PID (covers injected wallpaper windows, which live in the engine).
  DWORD pid = 0;
  ::GetWindowThreadProcessId(hwnd, &pid);
  if (pid == ctx->own_pid) return TRUE;
  // Tool windows (tooltips, palettes): never fullscreen occluders.
  const LONG_PTR exstyle = ::GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
  if ((exstyle & WS_EX_TOOLWINDOW) != 0) return TRUE;
  // Cloaked (UWP/Store apps, virtual-desktop-hidden): present in the
  // z-order but not composited on screen.
  if (ctx->dwm_get_attr != nullptr) {
    DWORD cloaked = 0;
    if (SUCCEEDED(ctx->dwm_get_attr(hwnd, kDwmWaCloaked, &cloaked,
                                    sizeof(cloaked))) &&
        cloaked != 0) {
      return TRUE;
    }
  }
  // Shell surfaces (taskbar, Progman, WorkerW): desktop furniture, not
  // occluders. Exact-handle match first, class match second.
  if (hwnd == ctx->shell_wnd) return TRUE;
  wchar_t cls[64] = {};
  if (::GetClassNameW(hwnd, cls, static_cast<int>(sizeof(cls) / sizeof(cls[0]))) > 0 &&
      IsShellClass(cls)) {
    return TRUE;
  }
  // Zero-area or failed rect: contributes nothing.
  RECT rc{};
  if (!::GetWindowRect(hwnd, &rc)) return TRUE;
  if (rc.right <= rc.left || rc.bottom <= rc.top) return TRUE;
  if (ctx->count < kMaxWindows) {
    CoverRect r;
    r.left = rc.left;
    r.top = rc.top;
    r.right = rc.right;
    r.bottom = rc.bottom;
    ctx->out[ctx->count++] = r;
  } else {
    ++ctx->overflow;
  }
  return TRUE;
}

}  // namespace

CoverRect ClipRect(CoverRect r, CoverRect clip) noexcept {
  CoverRect c;
  c.left = r.left < clip.left ? clip.left : r.left;
  c.top = r.top < clip.top ? clip.top : r.top;
  c.right = r.right > clip.right ? clip.right : r.right;
  c.bottom = r.bottom > clip.bottom ? clip.bottom : r.bottom;
  if (c.right <= c.left || c.bottom <= c.top) {
    return CoverRect{};
  }
  return c;
}

long long UnionAreaClipped(const CoverRect* rects, size_t count,
                           CoverRect clip) noexcept {
  if (rects == nullptr || count == 0) return 0;
  if (clip.right <= clip.left || clip.bottom <= clip.top) return 0;
  // Clip + drop empties into stack scratch.
  CoverRect kept[kMaxWindows];
  size_t m = 0;
  for (size_t i = 0; i < count && m < kMaxWindows; ++i) {
    const CoverRect c = ClipRect(rects[i], clip);
    if (c.right > c.left && c.bottom > c.top) kept[m++] = c;
  }
  if (m == 0) return 0;
  // Unique x-edges of the clipped set.
  int xs[kMaxWindows * 2];
  int nx = 0;
  for (size_t i = 0; i < m; ++i) {
    xs[nx++] = kept[i].left;
    xs[nx++] = kept[i].right;
  }
  std::sort(xs, xs + nx);
  int ux = 0;
  for (int i = 0; i < nx; ++i) {
    if (ux == 0 || xs[i] != xs[ux - 1]) xs[ux++] = xs[i];
  }
  // Sweep slabs: y-union of rects spanning each [x0,x1) strip. Interval
  // endpoints pack into int64 (top<<32|bottom) for a single-key sort.
  long long area = 0;
  for (int i = 0; i + 1 < ux; ++i) {
    const int x0 = xs[i];
    const int x1 = xs[i + 1];
    if (x1 <= x0) continue;
    long long keys[kMaxWindows];
    int nk = 0;
    for (size_t j = 0; j < m; ++j) {
      if (kept[j].left <= x0 && kept[j].right >= x1) {
        keys[nk++] = (static_cast<long long>(kept[j].top) << 32) |
                     (static_cast<unsigned>(kept[j].bottom) & 0xFFFFFFFFu);
      }
    }
    if (nk == 0) continue;
    std::sort(keys, keys + nk);
    long long ysum = 0;
    int cur_t = static_cast<int>(keys[0] >> 32);
    int cur_b = static_cast<int>(keys[0] & 0xFFFFFFFFu);
    for (int k = 1; k < nk; ++k) {
      const int t = static_cast<int>(keys[k] >> 32);
      const int b = static_cast<int>(keys[k] & 0xFFFFFFFFu);
      if (t > cur_b) {
        ysum += static_cast<long long>(cur_b - cur_t);
        cur_t = t;
        cur_b = b;
      } else if (b > cur_b) {
        cur_b = b;
      }
    }
    ysum += static_cast<long long>(cur_b - cur_t);
    area += static_cast<long long>(x1 - x0) * ysum;
  }
  return area;
}

double ComputeCoverage(CoverRect monitor, const CoverRect* rects,
                       size_t count) noexcept {
  const long long mw = static_cast<long long>(monitor.right) - monitor.left;
  const long long mh = static_cast<long long>(monitor.bottom) - monitor.top;
  if (mw <= 0 || mh <= 0 || rects == nullptr || count == 0) return 0.0;
  const double total = static_cast<double>(mw) * static_cast<double>(mh);
  double covered =
      static_cast<double>(UnionAreaClipped(rects, count, monitor));
  if (covered < 0.0) covered = 0.0;
  if (covered > total) covered = total;
  return covered / total;
}

void OcclusionWatch::Log(const char* fmt, ...) {
  if (log_ == nullptr || fmt == nullptr) return;
  char buf[256];
  va_list args;
  va_start(args, fmt);
  std::vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  buf[sizeof(buf) - 1] = '\0';
  log_("%s", buf);
}

void OcclusionWatch::OnPauseMaskChanged(bool slots_paused) {
  if (slots_paused) {
    // Any pause owner (incl. kPausePower) stops the tick: the loop wait
    // goes INFINITE, so Check self-suspends with it.
    occlusion_armed_.store(false, std::memory_order_release);
    return;
  }
  occlusion_armed_.store(true, std::memory_order_release);
  // Mask returned to zero: wake the loop thread out of its INFINITE wait
  // so the 1500 ms cadence restarts promptly. PostMessage only — timer
  // create/kill is affine to the loop thread and must never run here
  // (this also runs on the IPC worker thread).
  if (void* hwnd = notify_hwnd_) {
    ::PostMessageW(static_cast<HWND>(hwnd), RearmMessageId(), 0, 0);
  }
}

void OcclusionWatch::OnRearmMessage() noexcept {
  // The wake itself is the re-arm; the flag was already stored by
  // OnPauseMaskChanged. Kept as a named handler so WndProc routes the
  // message explicitly instead of dropping it to DefWindowProcW.
}

void OcclusionWatch::Check(MultiMonitor& slots) {
  // Disarmed while any pause owner holds the engine (self-suspended).
  if (!occlusion_armed_.load(std::memory_order_acquire)) return;
  // 1500 ms throttle: the unpaused loop wakes early on messages, but the
  // occlusion scan runs at most once per tick (no new loop timer).
  const auto now = std::chrono::steady_clock::now();
  if (last_check_ != std::chrono::steady_clock::time_point{} &&
      now - last_check_ < std::chrono::milliseconds(kTickMs)) {
    return;
  }
  last_check_ = now;
  CheckNow(slots);
}

void OcclusionWatch::CheckNow(MultiMonitor& slots) {

  // Slot rects idx-aligned by construction: monitor_ids() IS the slots_
  // iteration order PauseSlot/IsSlotPaused index into (span mode is the
  // single kSpanSlotId slot at index 0).
  CoverRect mons[kMaxSlots];
  size_t nslots = 0;
  if (slots.mode() == MultiMonitorMode::Span) {
    const SpanGeometry g = MultiMonitor::GetSpanGeometry();
    if (g.width <= 0 || g.height <= 0) return;
    mons[0].left = g.x;
    mons[0].top = g.y;
    mons[0].right = g.x + g.width;
    mons[0].bottom = g.y + g.height;
    nslots = 1;
  } else {
    const std::vector<int> ids = slots.monitor_ids();
    if (ids.empty()) return;
    const std::vector<MonitorInfo> all = ListMonitors();
    for (size_t i = 0; i < ids.size() && nslots < kMaxSlots; ++i) {
      for (const MonitorInfo& mi : all) {
        if (mi.id == ids[i] && mi.width > 0 && mi.height > 0) {
          mons[nslots++] = MonitorCover(mi);
          break;
        }
      }
      // Id vanished mid-tick (display change): skip the slot this round,
      // leaving its pause state untouched.
    }
    if (nslots == 0) return;
  }

  // One EnumWindows snapshot shared by all slots (windows pre-clipped
  // per monitor inside the union helper, so multi-monitor spans work).
  // LOW-7: DwmGetWindowAttribute comes from the cached module (no
  // per-tick LoadLibrary/FreeLibrary).
  CollectCtx ctx;
  ctx.own_pid = ::GetCurrentProcessId();
  ctx.shell_wnd = ::GetShellWindow();
  static CachedDwmApi dwm;
  ctx.dwm_get_attr = dwm.get_attr;
  if (dwm.handle == nullptr) {
    Log("occlusion: dwmapi unavailable, cloaked windows treated as visible");
  }
  ::EnumWindows(&CollectWindows, reinterpret_cast<LPARAM>(&ctx));
  if (ctx.overflow > 0) {
    Log("occlusion: window cap hit, %llu extras ignored (no pause from them)",
        static_cast<unsigned long long>(ctx.overflow));
  }

  for (size_t i = 0; i < nslots; ++i) {
    const double coverage =
        ComputeCoverage(mons[i], ctx.out, ctx.count);
    const bool paused = slots.IsSlotPaused(i);
    const bool want = HysteresisDecide(coverage, paused);
    if (want == paused) continue;
    // Per-slot ONLY: never touches global pause bits, never sets
    // kPauseFullscreen (global bits override regardless).
    slots.PauseSlot(i, want);
    Log("occlusion: slot %llu coverage=%.1f%% %s",  // NOLINT
        static_cast<unsigned long long>(i), coverage * 100.0,
        want ? "pausing" : "resuming");
  }
}

}  // namespace k6wp
