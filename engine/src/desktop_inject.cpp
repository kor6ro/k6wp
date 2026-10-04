// engine/src/desktop_inject.cpp
// Desktop injection: attach a child window behind desktop icons.
//
// Validated recipe (spike Todo 3, docs/spike-results.md, build 26200 / 24H2):
//   1. Create top-level WS_POPUP with WS_EX_LAYERED|WS_EX_NOACTIVATE
//      (cross-process CreateWindowEx with a foreign parent = error 5).
//   2. SetParent(wnd, progman).
//   3. Re-apply WS_EX_LAYERED after SetParent (SetParent strips it!) +
//      SetLayeredWindowAttributes(alpha=255).
//   4. SetWindowPos(wnd, defView) -> z-order [DefView, wnd, WorkerW-child].
//   5. ShowWindow(SW_SHOW).
//
// 24H2 detection: Progman has WS_EX_NOREDIRECTIONBITMAP -> layered-child path.
// Classic (pre-24H2): Strategy A (WorkerW hosting DefView), then Strategy B
// (0x052C-spawned empty WorkerW), then Progman fallback.

#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include "desktop_inject.hpp"
#include "desktop_placement.hpp"

#include <cstdarg>
#include <cstdio>
#include <string>
#include <utility>
#include <vector>

namespace k6wp {

// Row 10: defined after the anonymous namespace at k6wp linkage so the
// workerw_span_test can prove the null guard; the strategy-B sweep below
// calls through this declaration.
bool WorkerWSpansVirtualScreen(HWND hwnd);

namespace {

constexpr wchar_t kProgmanClass[] = L"Progman";
constexpr wchar_t kDefViewClass[] = L"SHELLDLL_DefView";
constexpr wchar_t kWorkerWClass[] = L"WorkerW";
constexpr wchar_t kInjectedClass[] = L"K6WP.DesktopInject.1";
constexpr UINT kSpawnWorkerW = 0x052C;
// Documented spawn parameters for 0x052C. The (0, 0) form previously used here
// is a no-op on 24H2, so Strategy B never found the WorkerW it asked for.
constexpr WPARAM kSpawnWorkerWWParam = 0xD;
constexpr LPARAM kSpawnWorkerWLParam = 0x1;

// Microsoft's suggested 24H2 signal: Progman is created with
// WS_EX_NOREDIRECTIONBITMAP (no GDI content at all).
constexpr LONG_PTR kNoRedirectionBitmap = 0x00200000L;

bool IsClass(HWND hwnd, const wchar_t* cls) {
  if (!hwnd) return false;
  wchar_t buf[256];
  if (GetClassNameW(hwnd, buf, 256) == 0) return false;
  return std::wstring(buf) == cls;
}

std::wstring ClassOf(HWND hwnd) {
  if (!hwnd) return L"(null)";
  wchar_t buf[256];
  if (GetClassNameW(hwnd, buf, 256) == 0) return L"(?)";
  return buf;
}

// --- desktop window discovery ----------------------------------------------

struct EnumCtx {
  HWND def_view = nullptr;
  HWND def_view_host = nullptr;  // top-level window whose direct child is DefView
  std::vector<HWND> worker_ws;
};

BOOL CALLBACK EnumTopLevelProc(HWND hwnd, LPARAM lParam) {
  auto* ctx = reinterpret_cast<EnumCtx*>(lParam);
  if (IsClass(hwnd, kWorkerWClass)) {
    ctx->worker_ws.push_back(hwnd);
  }
  HWND child = FindWindowExW(hwnd, nullptr, kDefViewClass, nullptr);
  if (child != nullptr) {
    ctx->def_view = child;
    ctx->def_view_host = hwnd;
  }
  return TRUE;
}

struct DesktopWindows {
  HWND progman = nullptr;
  HWND def_view = nullptr;
  HWND def_view_host = nullptr;  // top-level window whose direct child is DefView
  std::vector<HWND> worker_ws;          // top-level WorkerW (classic layout)
  std::vector<HWND> progman_worker_ws;  // WorkerW parented to Progman (24H2)
};

BOOL CALLBACK EnumProgmanChildProc(HWND hwnd, LPARAM lParam) {
  auto* out = reinterpret_cast<std::vector<HWND>*>(lParam);
  if (IsClass(hwnd, kWorkerWClass)) out->push_back(hwnd);
  return TRUE;
}

DesktopWindows FindDesktopWindows() {
  DesktopWindows d;
  d.progman = FindWindowW(kProgmanClass, nullptr);
  EnumCtx ctx;
  EnumWindows(EnumTopLevelProc, reinterpret_cast<LPARAM>(&ctx));
  d.def_view = ctx.def_view;
  d.def_view_host = ctx.def_view_host;
  d.worker_ws = std::move(ctx.worker_ws);
  // On 24H2 the shell parents its wallpaper WorkerW to Progman, so the
  // EnumWindows sweep above structurally cannot see it.
  if (d.progman) {
    EnumChildWindows(d.progman, EnumProgmanChildProc,
                     reinterpret_cast<LPARAM>(&d.progman_worker_ws));
  }
  return d;
}

// Strategy A: the top-level WorkerW whose direct child is SHELLDLL_DefView.
// Classic Win10-era layout; N/A on 24H2 (DefView is a direct child of Progman).
HWND FindWorkerWStrategyA(const DesktopWindows& d) {
  if (d.def_view_host && d.def_view_host != d.progman &&
      IsClass(d.def_view_host, kWorkerWClass)) {
    return d.def_view_host;
  }
  return nullptr;
}

// Row 10: LogShared is defined after the strategy helpers; the strategy-B
// sweep logs its `placement:` line through this declaration.
void LogShared(LogFn log, const char* fmt, ...);

// Strategy B: the empty WorkerW spawned by 0x052C. Prefer the WorkerW directly
// behind Progman in z-order (GW_HWNDNEXT); else any empty WorkerW.
// Row 10: the last-resort sweep over arbitrary top-level WorkerWs only
// accepts a candidate whose window rect spans the virtual-screen union
// (WorkerWSpansVirtualScreen) - a non-spanning WorkerW would clip the span
// child. Returns nullptr when nothing qualifies; callers degrade honestly.
HWND FindWorkerWStrategyB(const DesktopWindows& d, LogFn log = nullptr) {
  if (!d.progman) return nullptr;
  for (HWND w : d.progman_worker_ws) {
    if (GetWindow(w, GW_CHILD) == nullptr) return w;
  }
  HWND below = GetWindow(d.progman, GW_HWNDNEXT);
  if (below && IsClass(below, kWorkerWClass) && GetWindow(below, GW_CHILD) == nullptr) {
    return below;
  }
  HWND above = GetWindow(d.progman, GW_HWNDPREV);
  if (above && IsClass(above, kWorkerWClass) && GetWindow(above, GW_CHILD) == nullptr) {
    return above;
  }
  for (HWND w : d.worker_ws) {
    if (GetWindow(w, GW_CHILD) != nullptr) continue;
    if (!WorkerWSpansVirtualScreen(w)) continue;
    RECT rc{};
    if (GetWindowRect(w, &rc)) {
      LogShared(log,
                "placement: strategy-b workerw=(%ld,%ld,%ld,%ld) spans "
                "virtual screen",
                rc.left, rc.top, rc.right, rc.bottom);
    } else {
      LogShared(log,
                "placement: strategy-b workerw=0x%p spans virtual screen "
                "(rect unreadable)",
                w);
    }
    return w;
  }
  return nullptr;
}

// Sends the shell's 0x052C "spawn WorkerW" message to Progman and returns the
// resulting WorkerW (if any) plus the DefView found afterwards. Does not log:
// the caller distinguishes a SendMessageTimeoutW failure from "sent but no
// WorkerW appeared" (the call sites log differing fall-back text).
struct SpawnWorkerWResult {
  bool sent = false;
  HWND workerw = nullptr;
  HWND def_view = nullptr;
};
SpawnWorkerWResult SpawnWorkerWViaProgman(HWND progman, LogFn log = nullptr) {
  DWORD_PTR result = 0;
  if (!SendMessageTimeoutW(progman, kSpawnWorkerW, kSpawnWorkerWWParam,
                           kSpawnWorkerWLParam, SMTO_NORMAL, 1000, &result)) {
    return {false, nullptr, nullptr};
  }
  const DesktopWindows after = FindDesktopWindows();
  return {true, FindWorkerWStrategyB(after, log), after.def_view};
}

// 24H2 detection: Progman carries WS_EX_NOREDIRECTIONBITMAP.
bool IsWin11_24H2(HWND progman) {
  if (!progman) return false;
  const LONG_PTR ex = GetWindowLongPtrW(progman, GWL_EXSTYLE);
  return (ex & kNoRedirectionBitmap) != 0;
}

const char* InjectModeToString(InjectMode mode) {
  switch (mode) {
    case InjectMode::kWorkerW:
      return "workerw";
    case InjectMode::kProgman:
      return "progman";
    case InjectMode::kAuto:
    default:
      return "auto";
  }
}

// Frameless wallpaper surface enforcement. Forces WS_POPUP|WS_CLIPCHILDREN,
// strips every frame bit (OVERLAPPEDWINDOW/CAPTION/SYSMENU/THICKFRAME/
// MINIMIZEBOX/MAXIMIZEBOX), forces ex TOOLWINDOW|NOACTIVATE (+LAYERED when
// layered), strips APPWINDOW (SetParent can set it -> taskbar/Alt-Tab entry),
// then SWP_FRAMECHANGED so the non-client area is recalculated at once.
void EnforceFramelessStyle(HWND wnd, bool layered) {
  if (!wnd) return;
  LONG_PTR style = GetWindowLongPtrW(wnd, GWL_STYLE);
  style |= (WS_POPUP | WS_CLIPCHILDREN);
  style &= ~(WS_OVERLAPPEDWINDOW | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME |
             WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
  SetWindowLongPtrW(wnd, GWL_STYLE, style);
  LONG_PTR ex = GetWindowLongPtrW(wnd, GWL_EXSTYLE);
  ex |= (WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE);
  if (layered) ex |= WS_EX_LAYERED;
  ex &= ~WS_EX_APPWINDOW;
  SetWindowLongPtrW(wnd, GWL_EXSTYLE, ex);
  SetWindowPos(wnd, nullptr, 0, 0, 0, 0,
               SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE |
                   SWP_FRAMECHANGED);
}

// --- injected window -------------------------------------------------------

LRESULT CALLBACK InjectedWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  // The injected window is a plain layered child; DefWindowProc handles
  // WM_ERASEBKGND/WM_PRINTCLIENT with the class background brush. Rendering
  // is done by the renderer (mpv D3D11, Todo 10) via DX presents.
  return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// Null-tolerant variadic logging for the free ResolveSharedHost (row 7):
// same shape as Impl::Logf but usable outside the pimpl.
void LogShared(LogFn log, const char* fmt, ...) {
  if (!log) return;
  va_list args;
  va_start(args, fmt);
  char buf[1024];
  std::vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  log("%s", buf);
}

}  // namespace

// Row 10: Win32 half of the strategy-B span gate (k6wp linkage so the
// workerw_span_test can prove the null guard). Reads the candidate's
// window rect and the virtual-screen union via the same four
// GetSystemMetrics(SM_*VIRTUALSCREEN) calls GetSpanGeometry reads; the
// containment itself is the pure WorkerWSpansRect. Null or unreadable
// handles are not spanning (false, never a crash).
bool WorkerWSpansVirtualScreen(HWND hwnd) {
  if (!hwnd) return false;
  RECT rc{};
  if (!GetWindowRect(hwnd, &rc)) return false;
  const PlacementRect worker{rc.left, rc.top, rc.right, rc.bottom};
  const int vs_left = ::GetSystemMetrics(SM_XVIRTUALSCREEN);
  const int vs_top = ::GetSystemMetrics(SM_YVIRTUALSCREEN);
  PlacementRect vs;
  vs.left = vs_left;
  vs.top = vs_top;
  vs.right = vs_left + ::GetSystemMetrics(SM_CXVIRTUALSCREEN);
  vs.bottom = vs_top + ::GetSystemMetrics(SM_CYVIRTUALSCREEN);
  return WorkerWSpansRect(worker, vs);
}

struct DesktopInjector::Impl {
  LogFn log;
  HWND injected = nullptr;
  bool layered_path = false;
  bool class_registered = false;
  HINSTANCE hinstance = nullptr;
  InjectMode inject_mode = InjectMode::kAuto;
  // Row 7: the attach pass's resolved host (ResolveSharedHost). Install via
  // SetSharedHost before Attach; has_shared_host_ distinguishes "pass ran,
  // host unresolved (null Progman)" from "SetSharedHost never called".
  SharedHost shared_host_;
  bool has_shared_host_ = false;
  // Row 4: last CoverageReason token emitted by LogPlacement ("" before the
  // first attempt). Copied per slot by MultiMonitor::AttachSlot; rows 15/19
  // surface it as get_state display_coverage.
  std::string last_coverage_reason_;

  void Logf(const char* fmt, ...) {
    if (!log) return;
    va_list args;
    va_start(args, fmt);
    char buf[1024];
    std::vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    log("%s", buf);
  }

  bool RegisterClass() {
    if (class_registered) return true;
    WNDCLASSW wc{};
    wc.lpfnWndProc = &InjectedWndProc;
    wc.hInstance = hinstance;
    wc.lpszClassName = kInjectedClass;
    wc.hbrBackground = reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    if (!RegisterClassW(&wc)) {
      const DWORD err = GetLastError();
      if (err != ERROR_CLASS_ALREADY_EXISTS) {
        Logf("desktop-inject: RegisterClassW failed (error %lu)", err);
        return false;
      }
    }
    class_registered = true;
    return true;
  }

  // Create the child window as a top-level WS_POPUP first (cross-process
  // SetParent requires this; creating directly with a foreign parent fails
  // with error 5), then SetParent to the target.
  bool CreateAndAttach(HWND target, HWND insert_after, int x, int y, int width,
                       int height, bool layered) {
    // Row 7: screen -> host-client conversion via MapWindowPoints (MS: SetWindowPos
    // takes coordinates "in client coordinates"; a NULL from-window means screen
    // coordinates). Replaces the hand-rolled `x -= host.left; y -= host.top`
    // subtraction, which ignored the client-origin/DPI mapping the OS does for
    // us. Fallback keeps the old arithmetic if the mapping fails, so behaviour
    // degrades to pre-row-7 rather than to a wrong placement.
    POINT pt{x, y};
    if (MapWindowPoints(nullptr, target, &pt, 1) == 0) {
      RECT host{};
      if (GetWindowRect(target, &host)) {
        pt.x = x - host.left;
        pt.y = y - host.top;
      }
    }
    x = pt.x;
    y = pt.y;
    const DWORD ex_style = layered ? (WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE) : 0;
    HWND wnd = CreateWindowExW(ex_style, kInjectedClass, L"K6WP Wallpaper",
                               WS_POPUP | WS_CLIPCHILDREN, x, y, width, height,
                               nullptr, nullptr, hinstance, nullptr);
    if (!wnd) {
      Logf("desktop-inject: CreateWindowExW failed (error %lu)", GetLastError());
      return false;
    }
    EnforceFramelessStyle(wnd, layered);
    if (layered) {
      SetLayeredWindowAttributes(wnd, 0, 255, LWA_ALPHA);
    }
    if (SetParent(wnd, target) == nullptr) {
      const DWORD err = GetLastError();
      Logf("desktop-inject: SetParent failed (error %lu)", err);
      DestroyWindow(wnd);
      return false;
    }
    EnforceFramelessStyle(wnd, layered);
    if (layered) {
      // SetParent strips WS_EX_LAYERED and WS_EX_TOOLWINDOW (and can strip
      // WS_EX_NOACTIVATE on some builds); EnforceFramelessStyle above already
      // re-applied the full ex-style set and dropped WS_EX_APPWINDOW, so just
      // restore alpha here so DWM composites the child into the desktop.
      SetLayeredWindowAttributes(wnd, 0, 255, LWA_ALPHA);
    }
    // Z-order: directly below DefView (behind icons, above wallpaper layer).
    // HWND_BOTTOM fallback keeps the surface at the bottom with SWP_NOACTIVATE
    // so it never steals focus; size is the monitor/virtual-screen rect.
    Logf("desktop-inject: z-order insert_after=0x%p", insert_after);
    SetWindowPos(wnd, insert_after, x, y, width, height,
                 SWP_NOACTIVATE | SWP_SHOWWINDOW);
    ShowWindow(wnd, SW_SHOW);
    EnforceFramelessStyle(wnd, layered);
    Logf("desktop-inject: window pos (%d,%d) size %dx%d", x, y, width, height);
    // Log final styles for audit: style must be POPUP|CLIPCHILDREN with no
    // frame bits; ex must carry TOOLWINDOW|NOACTIVATE (+LAYERED) sans APPWINDOW.
    LONG_PTR final_style = GetWindowLongPtrW(wnd, GWL_STYLE);
    LONG_PTR final_ex = GetWindowLongPtrW(wnd, GWL_EXSTYLE);
    Logf("desktop-inject: final style 0x%08lx POPUP=%s CLIPCHILDREN=%s FRAME=%s",
         final_style, (final_style & WS_POPUP) ? "YES" : "NO",
         (final_style & WS_CLIPCHILDREN) ? "YES" : "NO",
         (final_style & (WS_CAPTION | WS_THICKFRAME | WS_SYSMENU |
                         WS_MINIMIZEBOX | WS_MAXIMIZEBOX))
             ? "YES-BAD"
             : "NO");
    Logf("desktop-inject: final exstyle 0x%08lx TOOLWINDOW=%s NOACTIVATE=%s APPWINDOW=%s",
         final_ex, (final_ex & WS_EX_TOOLWINDOW) ? "YES" : "NO",
         (final_ex & WS_EX_NOACTIVATE) ? "YES" : "NO",
         (final_ex & WS_EX_APPWINDOW) ? "YES-BAD" : "NO");
    injected = wnd;
    return true;
  }

  // Row 4: structured per-attempt placement facts. Logging only — no
  // control flow. Every line carries the stable `placement:` prefix plus the
  // CoverageReason token (row 3) so Select-String extracts exactly these.
  // Verdict mapping (row 3 issues.md consistent-mapping trap): the child is
  // placed from the host WINDOW origin (as CreateAndAttach does) while the
  // monitor is mapped from the host CLIENT origin; deriving both from the
  // same origin would yield kCovered by construction.
  void LogPlacement(const char* branch, HWND target, int req_x, int req_y,
                    int req_w, int req_h, bool ok) {
    RECT host_win{};
    GetWindowRect(target, &host_win);
    RECT host_cli{};
    GetClientRect(target, &host_cli);
    POINT cli_org{0, 0};
    ClientToScreen(target, &cli_org);
    const PlacementRect host_win_pr{host_win.left, host_win.top, host_win.right,
                                    host_win.bottom};
    const PlacementRect host_cli_pr{
        cli_org.x, cli_org.y,
        cli_org.x + (host_cli.right - host_cli.left),
        cli_org.y + (host_cli.bottom - host_cli.top)};
    const PlacementRect mon_pr{req_x, req_y, req_x + req_w, req_y + req_h};
    // Row 4 verdict split (kept verbatim): the child model is placed from the
    // host WINDOW origin while the monitor is mapped from the host CLIENT
    // origin - deriving both from the same origin would yield kCovered by
    // construction (row 3 issues.md).
    const ClientOffset placed = HostClientOffset(host_win_pr, mon_pr);
    const PlacementRect child_in_client =
        ChildRectInClient(placed, mon_pr, host_cli_pr);
    // Row 7: the logged delta is now the MapWindowPoints screen->client
    // conversion - the exact coordinates handed to SetWindowPos (row 4's
    // "MapWindowPoints delta once todo 7 lands"). On the borderless desktop
    // hosts (WorkerW/Progman) it equals `placed`; the fallback keeps the row-4
    // value if the mapping fails.
    POINT mp{req_x, req_y};
    const ClientOffset delta =
        MapWindowPoints(nullptr, target, &mp, 1) != 0
            ? ClientOffset{mp.x, mp.y}
            : placed;
    const PlacementRect mon_as_client{mon_pr.left - cli_org.x,
                                      mon_pr.top - cli_org.y,
                                      mon_pr.right - cli_org.x,
                                      mon_pr.bottom - cli_org.y};
    const CoverageVerdict verdict =
        CoversMonitor(child_in_client, mon_as_client);
    const char* reason = CoverageReason(verdict);
    last_coverage_reason_ = reason ? reason : "";
    RECT final_rc{};
    const bool have_final =
        (injected != nullptr) && (GetWindowRect(injected, &final_rc) != FALSE);
    char final_buf[64];
    if (have_final) {
      std::snprintf(final_buf, sizeof(final_buf), "(%ld,%ld,%ld,%ld)",
                    final_rc.left, final_rc.top, final_rc.right,
                    final_rc.bottom);
    } else {
      std::snprintf(final_buf, sizeof(final_buf), "(none)");
    }
    Logf("placement: branch=%s ok=%d reason=%s host=%ls(0x%p) "
         "host_win=(%ld,%ld,%ld,%ld) host_client=(%d,%d,%d,%d) "
         "monitor=(%d,%d,%d,%d) delta=(%d,%d) child_final=%s",
         branch ? branch : "?", ok ? 1 : 0, last_coverage_reason_.c_str(),
         ClassOf(target).c_str(), target, host_win.left, host_win.top,
         host_win.right, host_win.bottom, host_cli_pr.left, host_cli_pr.top,
         host_cli_pr.right, host_cli_pr.bottom, req_x, req_y, req_w, req_h,
         delta.dx, delta.dy, final_buf);
  }

  // The full attach sequence against the attach pass's resolved host (row 7).
  // FindDesktopWindows + strategy selection now live in ResolveSharedHost
  // (called once per pass by MultiMonitor, delivered via SetSharedHost);
  // this per-slot path only creates/positions the child.
  // Returns the target HWND used, or nullptr.
  HWND AttachToDesktop(int x, int y, int width, int height) {
    if (!has_shared_host_) {
      Logf("desktop-inject: no shared host set (row 7: ResolveSharedHost once per attach pass), skipping attach");
      return nullptr;
    }
    const SharedHost& sh = shared_host_;
    if (!sh.host) {
      Logf("desktop-inject: shared host unresolved, skipping attach (retry on re-anchor), engine keeps running");
      return nullptr;
    }
    HWND target = static_cast<HWND>(sh.host);
    HWND insert_after = static_cast<HWND>(sh.insert_after);
    bool layered = sh.layered;
    const char* branch =
        (sh.branch != nullptr && sh.branch[0] != '\0') ? sh.branch
                                                       : "shared host";

    // Row 4: structured placement facts per attempt (logging only).
    const bool first_ok =
        CreateAndAttach(target, insert_after, x, y, width, height, layered);
    LogPlacement(branch, target, x, y, width, height, first_ok);
    if (!first_ok) {
      // Attach to a WorkerW failed (SetParent/create): retry once against
      // Progman before giving up. CreateAndAttach already logged the error.
      // The 24H2/forced-progman paths target Progman already, so no retry
      // applies there; forced-workerw never falls back to Progman either.
      if (inject_mode == InjectMode::kWorkerW) {
        Logf("desktop-inject: strategy forced workerw: attach failed, no Progman fallback");
        return nullptr;
      }
      if (target != static_cast<HWND>(sh.progman) && sh.progman) {
        Logf("desktop-inject: attach to WorkerW failed, retrying Progman fallback");
        target = static_cast<HWND>(sh.progman);
        insert_after =
            sh.def_view ? static_cast<HWND>(sh.def_view) : HWND_BOTTOM;
        layered = true;
        // Row 4: log the retry attempt too (the false-success site).
        const bool retry_ok =
            CreateAndAttach(target, insert_after, x, y, width, height, layered);
        LogPlacement("Progman fallback retry", target, x, y, width, height,
                     retry_ok);
        if (!retry_ok) {
          return nullptr;
        }
      } else {
        return nullptr;
      }
    }
    // Row 9: honest post-attach verification. CreateAndAttach returning true
    // (first attempt OR Progman-fallback retry) is not proof the child
    // covers its monitor - a clipped/out-of-bounds child is a FALSE SUCCESS
    // that would suppress the existing headless indicator (GAP-8). Read the
    // FINAL child rect back with GetWindowRect (SCREEN coordinates) and
    // compare it DIRECTLY against the requested monitor rect (also SCREEN
    // coordinates: the x/y/width/height Attach was called with). Do NOT
    // convert through host-client space - that would flag every
    // negative-origin monitor as clipped (row 3 issues.md
    // consistent-mapping trap / oracle note 2). CoversMonitor's parameters
    // are named *_in_client but document "both rects in the same space";
    // two screen-space rects are the same space. On kOutOfBounds or any
    // kClipped*: log `placement: RETRY-FALSE-SUCCESS` + the honest reason,
    // destroy the non-covering child (so injected_hwnd()==nullptr drives
    // the headless census), and return nullptr -> Attach() false ->
    // AttachSlot headless path. On kCovered: proceed as today. An unreadable
    // child rect is also a false success - coverage that cannot be proven is
    // not success.
    {
      RECT final_rc{};
      if (!injected || !GetWindowRect(injected, &final_rc)) {
        last_coverage_reason_ = "placement: OUT-OF-BOUNDS";
        Logf("placement: RETRY-FALSE-SUCCESS reason=child-rect-unreadable "
             "monitor=(%d,%d,%d,%d)",
             x, y, x + width, y + height);
        // Same teardown as DesktopInjector::Detach (Impl cannot call the
        // outer method): destroy the non-covering child and clear the
        // handles so injected_hwnd()==nullptr drives the headless census.
        if (injected) {
          if (!DestroyWindow(injected)) {
            Logf("desktop-inject: DestroyWindow(0x%p) failed (GetLastError=%lu)",
                 injected, static_cast<unsigned long>(GetLastError()));
          }
          injected = nullptr;
          layered_path = false;
        }
        return nullptr;
      }
      const PlacementRect child_screen{final_rc.left, final_rc.top,
                                       final_rc.right, final_rc.bottom};
      const PlacementRect mon_screen{x, y, x + width, y + height};
      const CoverageVerdict v = CoversMonitor(child_screen, mon_screen);
      if (v != CoverageVerdict::kCovered) {
        const char* reason = CoverageReason(v);
        // Row 4 chain stays honest: last_coverage_reason_ must carry the
        // REAL final verdict (AttachSlot copies it into Slot::coverage_reason
        // after Attach returns) - not LogPlacement's modelled one.
        last_coverage_reason_ = reason ? reason : "";
        Logf("placement: RETRY-FALSE-SUCCESS reason=%s "
             "child=(%ld,%ld,%ld,%ld) monitor=(%d,%d,%d,%d)",
             last_coverage_reason_.c_str(), final_rc.left, final_rc.top,
             final_rc.right, final_rc.bottom, x, y, x + width, y + height);
        if (injected) {
          if (!DestroyWindow(injected)) {
            Logf("desktop-inject: DestroyWindow(0x%p) failed (GetLastError=%lu)",
                 injected, static_cast<unsigned long>(GetLastError()));
          }
          injected = nullptr;
          layered_path = false;
        }
        return nullptr;
      }
    }
    layered_path = layered;
    Logf("desktop-inject: attached 0x%p to 0x%p (%ls), layered=%s", injected,
         target, ClassOf(target).c_str(), layered ? "YES" : "NO");
    return target;
  }
};

// Row 7: ONE host resolution per attach pass. This is the strategy selection
// AttachToDesktop used to run on EVERY slot attach - including the 0x052C
// SpawnWorkerWViaProgman call, which spawned one fresh WorkerW per monitor.
// MultiMonitor calls this exactly once per pass and shares the result across
// all slots via DesktopInjector::SetSharedHost. Emits exactly ONE
// `placement: host-resolution` log line per call (the acceptance token).
SharedHost ResolveSharedHost(InjectMode mode, LogFn log) {
  SharedHost sh;
  const DesktopWindows d = FindDesktopWindows();
  if (!d.progman) {
    LogShared(log,
              "desktop-inject: Progman not found, host unresolved (retry on "
              "re-anchor), engine keeps running");
    LogShared(log, "placement: host-resolution branch=none host=(null)");
    return sh;
  }
  const bool is_24h2 = IsWin11_24H2(d.progman);
  LogShared(log,
            "desktop-inject: progman=0x%p defView=0x%p defViewHost=0x%p (%ls) 24H2=%s",
            d.progman, d.def_view, d.def_view_host,
            ClassOf(d.def_view_host).c_str(), is_24h2 ? "YES" : "NO");
  LogShared(log, "desktop-inject: strategy=%s", InjectModeToString(mode));

  HWND target = nullptr;
  HWND insert_after = HWND_BOTTOM;
  bool layered = false;
  // Row 4: which attach branch was taken (logged via LogPlacement).
  const char* branch = "unknown";

  if (mode == InjectMode::kProgman) {
    // Forced Progman: straight to the validated 24H2 layered recipe, no
    // WorkerW probing at all.
    branch = "forced progman";
    target = d.progman;
    insert_after = d.def_view ? d.def_view : HWND_BOTTOM;
    layered = true;
    LogShared(log, "desktop-inject: strategy forced progman -> layered child into Progman");
  } else if (mode == InjectMode::kWorkerW) {
    // Forced WorkerW: Strategy A then B. No usable WorkerW is an honest
    // headless slot — never a silent Progman fallback.
    branch = "forced workerw";
    HWND a = FindWorkerWStrategyA(d);
    if (a) {
      target = a;
      insert_after = d.def_view ? d.def_view : HWND_BOTTOM;
      layered = true;
      LogShared(log, "desktop-inject: strategy forced workerw -> Strategy A WorkerW 0x%p", a);
    } else {
      const SpawnWorkerWResult spawn = SpawnWorkerWViaProgman(d.progman, log);
      if (spawn.workerw) {
        target = spawn.workerw;
        insert_after = HWND_BOTTOM;
        layered = true;
        LogShared(log, "desktop-inject: strategy forced workerw -> Strategy B empty WorkerW 0x%p",
                  spawn.workerw);
      } else if (!spawn.sent) {
        LogShared(log, "desktop-inject: 0x052C SendMessageTimeoutW timeout/failed (error %lu)",
                  GetLastError());
      }
      if (!target) {
        LogShared(log, "desktop-inject: strategy forced workerw: no usable WorkerW");
        return sh;
      }
    }
  } else if (is_24h2) {
    // 24H2: the shell's wallpaper WorkerW is parented to Progman, so ask for
    // it and prefer it when it appears -- that is Microsoft's arrangement
    // (our surface above the wallpaper layer, below the icons). Without one,
    // fall back to a layered child of Progman directly below DefView.
    branch = "24H2 path";
    const SpawnWorkerWResult spawn = SpawnWorkerWViaProgman(d.progman, log);
    if (spawn.workerw) {
      target = spawn.workerw;
      insert_after = spawn.def_view ? spawn.def_view : HWND_BOTTOM;
      layered = true;
      LogShared(log, "desktop-inject: 24H2 path -> wallpaper WorkerW 0x%p (Progman child)",
                spawn.workerw);
    }
    if (!target) {
      target = d.progman;
      insert_after = d.def_view ? d.def_view : HWND_BOTTOM;
      layered = true;
      LogShared(log, "desktop-inject: 24H2 path -> layered child into Progman "
                     "(no wallpaper WorkerW)");
    }
  } else {
    // Classic path: Strategy A (WorkerW hosting DefView), then Strategy B
    // (0x052C-spawned empty WorkerW), then Progman fallback.
    branch = "classic path";
    HWND a = FindWorkerWStrategyA(d);
    if (a) {
      target = a;
      insert_after = d.def_view ? d.def_view : HWND_BOTTOM;
      layered = true;
      LogShared(log, "desktop-inject: Strategy A -> WorkerW 0x%p (hosts DefView)", a);
    } else {
      // Try to spawn a WorkerW via 0x052C (Strategy B).
      const SpawnWorkerWResult spawn = SpawnWorkerWViaProgman(d.progman, log);
      if (spawn.workerw) {
        target = spawn.workerw;
        insert_after = HWND_BOTTOM;
        layered = true;
        LogShared(log, "desktop-inject: Strategy B -> empty WorkerW 0x%p", spawn.workerw);
      } else if (!spawn.sent) {
        LogShared(log, "desktop-inject: 0x052C SendMessageTimeoutW timeout/failed (error %lu), falling back to Progman",
                  GetLastError());
      }
      if (!target) {
        target = d.progman;
        insert_after = d.def_view ? d.def_view : HWND_BOTTOM;
        layered = true;
        LogShared(log, "desktop-inject: fallback -> Progman");
      }
    }
  }

  sh.host = target;
  sh.progman = d.progman;
  sh.def_view = d.def_view;
  sh.insert_after = insert_after;
  sh.layered = layered;
  sh.branch = branch;
  // Measure the host's client rect in screen coordinates: the shared
  // "measured host" baseline (row 11 compares it across passes) and the
  // frame of reference MapWindowPoints maps child coordinates into.
  RECT cli{};
  POINT org{0, 0};
  if (GetClientRect(target, &cli) && ClientToScreen(target, &org)) {
    sh.client_rect.left = org.x;
    sh.client_rect.top = org.y;
    sh.client_rect.right = org.x + (cli.right - cli.left);
    sh.client_rect.bottom = org.y + (cli.bottom - cli.top);
  }
  LogShared(log,
            "placement: host-resolution branch=%s host=%ls(0x%p) "
            "insert_after=0x%p layered=%s client=(%d,%d,%d,%d)",
            branch, ClassOf(target).c_str(), target, insert_after,
            layered ? "YES" : "NO", sh.client_rect.left, sh.client_rect.top,
            sh.client_rect.right, sh.client_rect.bottom);
  return sh;
}

DesktopInjector::DesktopInjector(LogFn log) : impl_(std::make_unique<Impl>()) {
  impl_->log = log;
  impl_->hinstance = GetModuleHandleW(nullptr);
}

DesktopInjector::~DesktopInjector() {
  if (!impl_) return;
  Detach();
  if (impl_->class_registered) {
    UnregisterClassW(kInjectedClass, impl_->hinstance);
  }
}

DesktopInjector::DesktopInjector(DesktopInjector&& other) noexcept
    : impl_(std::move(other.impl_)) {}

DesktopInjector& DesktopInjector::operator=(DesktopInjector&& other) noexcept {
  if (this != &other) {
    Detach();
    impl_ = std::move(other.impl_);
  }
  return *this;
}

bool DesktopInjector::Attach(int x, int y, int width, int height) {
  if (!impl_) return false;
  if (impl_->injected) {
    impl_->Logf("desktop-inject: already attached (0x%p), detaching first",
                impl_->injected);
    Detach();
  }
  if (!impl_->RegisterClass()) return false;
  return impl_->AttachToDesktop(x, y, width, height) != nullptr;
}

void DesktopInjector::SetInjectMode(InjectMode mode) {
  if (!impl_) return;
  impl_->inject_mode = mode;
}

void DesktopInjector::SetSharedHost(const SharedHost& host) {
  if (!impl_) return;
  impl_->shared_host_ = host;
  impl_->has_shared_host_ = true;
}

std::string DesktopInjector::last_coverage_reason() const {
  if (!impl_) return {};
  return impl_->last_coverage_reason_;
}

void DesktopInjector::Detach() {
  if (!impl_ || !impl_->injected) return;
  if (!DestroyWindow(impl_->injected)) {
    impl_->Logf("desktop-inject: DestroyWindow(0x%p) failed (GetLastError=%lu)",
                impl_->injected, static_cast<unsigned long>(GetLastError()));
  }
  impl_->injected = nullptr;
  impl_->layered_path = false;
}

void DesktopInjector::OnDisplayChange(int x, int y, int width, int height) {
  if (!impl_) return;
  if (!impl_->injected) {
    impl_->Logf("desktop-inject: OnDisplayChange ignored (not attached)");
    return;
  }
  impl_->Logf("desktop-inject: display change -> re-attach (%d,%d %dx%d)", x, y,
              width, height);
  // Re-find the desktop target and re-attach. The injected HWND is destroyed
  // and recreated; the renderer must re-query injected_hwnd() after this.
  Detach();
  Attach(x, y, width, height);
}

void* DesktopInjector::injected_hwnd() const {
  return impl_ ? impl_->injected : nullptr;
}

void DesktopInjector::ReassertFrameless() {
  if (!impl_ || !impl_->injected) return;
  // Re-anchor enforcement: Explorer recreations must never leave a
  // framed/taskbar/Alt-Tab window. Never creates a window.
  EnforceFramelessStyle(impl_->injected, impl_->layered_path);
  if (impl_->layered_path) {
    SetLayeredWindowAttributes(impl_->injected, 0, 255, LWA_ALPHA);
  }
  impl_->Logf("desktop-inject: reassert frameless on 0x%p", impl_->injected);
}

}  // namespace k6wp
