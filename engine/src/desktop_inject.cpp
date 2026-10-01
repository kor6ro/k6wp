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

#include <cstdarg>
#include <cstdio>
#include <string>
#include <utility>
#include <vector>

namespace k6wp {
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

// Strategy B: the empty WorkerW spawned by 0x052C. Prefer the WorkerW directly
// behind Progman in z-order (GW_HWNDNEXT); else any empty WorkerW.
HWND FindWorkerWStrategyB(const DesktopWindows& d) {
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
    if (GetWindow(w, GW_CHILD) == nullptr) return w;
  }
  return nullptr;
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

}  // namespace

struct DesktopInjector::Impl {
  LogFn log;
  HWND injected = nullptr;
  bool layered_path = false;
  bool class_registered = false;
  HINSTANCE hinstance = nullptr;
  InjectMode inject_mode = InjectMode::kAuto;

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
    // Child coords are relative to the desktop host's client origin; the
    // caller passes virtual-screen coords (monitor rect). Convert by
    // subtracting the host's window origin so the child lands exactly on its
    // monitor's screen rect regardless of where the virtual origin sits.
    RECT host{};
    if (GetWindowRect(target, &host)) {
      x -= host.left;
      y -= host.top;
    }
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

  // The full attach sequence. Returns the target HWND used, or nullptr.
  HWND AttachToDesktop(int x, int y, int width, int height) {
    const DesktopWindows d = FindDesktopWindows();
    if (!d.progman) {
      Logf("desktop-inject: Progman not found, skipping attach (retry on re-anchor), engine keeps running");
      return nullptr;
    }
    const bool is_24h2 = IsWin11_24H2(d.progman);
    Logf("desktop-inject: progman=0x%p defView=0x%p defViewHost=0x%p (%ls) 24H2=%s",
         d.progman, d.def_view, d.def_view_host, ClassOf(d.def_view_host).c_str(),
         is_24h2 ? "YES" : "NO");
    Logf("desktop-inject: strategy=%s", InjectModeToString(inject_mode));

    HWND target = nullptr;
    HWND insert_after = HWND_BOTTOM;
    bool layered = false;

    if (inject_mode == InjectMode::kProgman) {
      // Forced Progman: straight to the validated 24H2 layered recipe, no
      // WorkerW probing at all.
      target = d.progman;
      insert_after = d.def_view ? d.def_view : HWND_BOTTOM;
      layered = true;
      Logf("desktop-inject: strategy forced progman -> layered child into Progman");
    } else if (inject_mode == InjectMode::kWorkerW) {
      // Forced WorkerW: Strategy A then B. No usable WorkerW is an honest
      // headless slot — never a silent Progman fallback.
      HWND a = FindWorkerWStrategyA(d);
      if (a) {
        target = a;
        insert_after = d.def_view ? d.def_view : HWND_BOTTOM;
        layered = true;
        Logf("desktop-inject: strategy forced workerw -> Strategy A WorkerW 0x%p", a);
      } else {
        DWORD_PTR result = 0;
        if (SendMessageTimeoutW(d.progman, kSpawnWorkerW, kSpawnWorkerWWParam,
                                kSpawnWorkerWLParam, SMTO_NORMAL, 1000,
                                &result)) {
          const DesktopWindows after = FindDesktopWindows();
          HWND b = FindWorkerWStrategyB(after);
          if (b) {
            target = b;
            insert_after = HWND_BOTTOM;
            layered = true;
            Logf("desktop-inject: strategy forced workerw -> Strategy B empty WorkerW 0x%p", b);
          }
        } else {
          Logf("desktop-inject: 0x052C SendMessageTimeoutW timeout/failed (error %lu)",
               GetLastError());
        }
        if (!target) {
          Logf("desktop-inject: strategy forced workerw: no usable WorkerW");
          return nullptr;
        }
      }
    } else if (is_24h2) {
      // 24H2: the shell's wallpaper WorkerW is parented to Progman, so ask for
      // it and prefer it when it appears -- that is Microsoft's arrangement
      // (our surface above the wallpaper layer, below the icons). Without one,
      // fall back to a layered child of Progman directly below DefView.
      DWORD_PTR spawned = 0;
      if (SendMessageTimeoutW(d.progman, kSpawnWorkerW, kSpawnWorkerWWParam,
                              kSpawnWorkerWLParam, SMTO_NORMAL, 1000, &spawned)) {
        const DesktopWindows after = FindDesktopWindows();
        const HWND wallpaper_layer = FindWorkerWStrategyB(after);
        if (wallpaper_layer) {
          target = wallpaper_layer;
          insert_after = after.def_view ? after.def_view : HWND_BOTTOM;
          layered = true;
          Logf("desktop-inject: 24H2 path -> wallpaper WorkerW 0x%p (Progman child)",
               wallpaper_layer);
        }
      }
      if (!target) {
        target = d.progman;
        insert_after = d.def_view ? d.def_view : HWND_BOTTOM;
        layered = true;
        Logf("desktop-inject: 24H2 path -> layered child into Progman "
             "(no wallpaper WorkerW)");
      }
    } else {
      // Classic path: Strategy A (WorkerW hosting DefView), then Strategy B
      // (0x052C-spawned empty WorkerW), then Progman fallback.
      HWND a = FindWorkerWStrategyA(d);
      if (a) {
        target = a;
        insert_after = d.def_view ? d.def_view : HWND_BOTTOM;
        layered = true;
        Logf("desktop-inject: Strategy A -> WorkerW 0x%p (hosts DefView)", a);
      } else {
        // Try to spawn a WorkerW via 0x052C (Strategy B).
        DWORD_PTR result = 0;
        if (SendMessageTimeoutW(d.progman, kSpawnWorkerW, kSpawnWorkerWWParam,
                                kSpawnWorkerWLParam, SMTO_NORMAL, 1000,
                                &result)) {
          const DesktopWindows after = FindDesktopWindows();
          HWND b = FindWorkerWStrategyB(after);
          if (b) {
            target = b;
            insert_after = HWND_BOTTOM;
            layered = true;
            Logf("desktop-inject: Strategy B -> empty WorkerW 0x%p", b);
          }
        } else {
          Logf("desktop-inject: 0x052C SendMessageTimeoutW timeout/failed (error %lu), falling back to Progman",
               GetLastError());
        }
        if (!target) {
          target = d.progman;
          insert_after = d.def_view ? d.def_view : HWND_BOTTOM;
          layered = true;
          Logf("desktop-inject: fallback -> Progman");
        }
      }
    }

    if (!CreateAndAttach(target, insert_after, x, y, width, height, layered)) {
      // Attach to a WorkerW failed (SetParent/create): retry once against
      // Progman before giving up. CreateAndAttach already logged the error.
      // The 24H2/forced-progman paths target Progman already, so no retry
      // applies there; forced-workerw never falls back to Progman either.
      if (inject_mode == InjectMode::kWorkerW) {
        Logf("desktop-inject: strategy forced workerw: attach failed, no Progman fallback");
        return nullptr;
      }
      if (target != d.progman && d.progman) {
        Logf("desktop-inject: attach to WorkerW failed, retrying Progman fallback");
        target = d.progman;
        insert_after = d.def_view ? d.def_view : HWND_BOTTOM;
        layered = true;
        if (!CreateAndAttach(target, insert_after, x, y, width, height, layered)) {
          return nullptr;
        }
      } else {
        return nullptr;
      }
    }
    layered_path = layered;
    Logf("desktop-inject: attached 0x%p to 0x%p (%ls), layered=%s", injected,
         target, ClassOf(target).c_str(), layered ? "YES" : "NO");
    return target;
  }
};

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
