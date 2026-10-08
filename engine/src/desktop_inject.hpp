#pragma once

// Desktop injection for K6WP engine.
// Attaches a child window behind desktop icons for wallpaper rendering.
//
// Strategy A (classic, pre-24H2): Find WorkerW whose child is SHELLDLL_DefView,
//   inject behind DefView. Works on Win10/Win11 where DefView is NOT inside
//   Progman directly.
// Strategy B (Win11 pre-24H2): Send 0x052C to Progman to spawn an empty WorkerW,
//   inject behind that WorkerW. Only works when "Animate controls..." is ON.
// Progman fallback (24H2 primary): SetParent directly to Progman behind DefView,
//   re-apply WS_EX_LAYERED (SetParent strips it). Primary path on 24H2 where
//   DefView is a direct child of Progman and 0x052C spawns nothing.
//
// Windows.h must only be included in the .cpp file; this header exposes only
// std types + an opaque handle (void*).

#include <memory>
#include <string>

#include "desktop_placement.hpp"

namespace k6wp {

// Logging callback. Plain function pointer (not std::function): variadic
// function types are illegal as std::function template arguments. Matches
// EngineApp::Log's static signature.
using LogFn = void (*)(const char* fmt, ...);

// Injection strategy selector (Step 4: --wallpaper-mode actually controls
// the injector; header stays std-only).
enum class InjectMode {
  kAuto,     // 24H2 layered Progman; else A -> B -> Progman (legacy logic).
  kWorkerW,  // Strategy A then B only; no usable WorkerW -> headless, never
             // silent Progman.
  kProgman,  // Layered child straight into Progman (validated 24H2 recipe).
};

// Row 7: ONE measured desktop host per attach pass, shared by every slot.
// Produced by ResolveSharedHost() - the strategy selection (Strategy A/B,
// 0x052C spawn, 24H2/classic/forced branches) runs exactly once per pass
// instead of once per slot - and installed on each slot injector via
// DesktopInjector::SetSharedHost() before Attach().
// `client_rect` is the host's measured client rect in SCREEN coordinates
// (left/top = client origin in screen space, the target space of the
// MapWindowPoints screen->client conversion); row 11 compares it across
// passes to detect a moved/recreated host.
// host == nullptr means "unresolved" (no Progman): the next Attach fails
// honestly and MultiMonitor degrades that slot to its existing headless
// fallback - never a crash, never a fresh per-slot spawn.
struct SharedHost {
  void* host = nullptr;          // chosen desktop host HWND
  void* progman = nullptr;       // Progman (Progman-fallback retry input)
  void* def_view = nullptr;      // SHELLDLL_DefView (z-order hint source)
  void* insert_after = nullptr;  // resolved SetWindowPos hWndInsertAfter hint
  bool layered = false;          // WS_EX_LAYERED child path (all branches)
  const char* branch = "";       // row 4 branch label consumed by LogPlacement
  PlacementRect client_rect{};   // measured client rect, screen coordinates
};

// Resolve the desktop host ONCE per attach pass (row 7). Performs the full
// strategy selection AttachToDesktop used to run per slot (FindDesktopWindows
// + FindWorkerWStrategyA/B + SpawnWorkerWViaProgman + the forced/24H2/classic
// branches), measures the chosen host's client rect, and logs ONE
// `placement: host-resolution` line (plus the moved strategy lines).
// `log` may be nullptr (logging disabled, matching LogFn semantics).
// Returns SharedHost{} when no Progman exists - callers treat that as
// "attach will fail", not as a crash.
SharedHost ResolveSharedHost(InjectMode mode, LogFn log);

// Find desktop windows and attach a child window behind desktop icons.
//
// Lifetime: construct once per session. Call Attach() to create and attach
// the child window. Call OnDisplayChange() when WM_DISPLAYCHANGE fires to
// re-attach (theme changes / wallpaper transitions can recreate windows).
// The injected HWND is exposed via injected_hwnd() for the renderer (mpv).
// Destroying this object removes the injected window (RAII).
class DesktopInjector {
 public:
  // Pass a logging callback (e.g. from EngineApp::Log). Pass nullptr to disable.
  explicit DesktopInjector(LogFn log);
  ~DesktopInjector();

  DesktopInjector(const DesktopInjector&) = delete;
  DesktopInjector& operator=(const DesktopInjector&) = delete;
  DesktopInjector(DesktopInjector&& other) noexcept;
  DesktopInjector& operator=(DesktopInjector&& other) noexcept;

  // Create the child window, find the desktop target, attach, and set z-order.
  // x/y: monitor rect in virtual-screen coords (rcMonitor left/top); the
  // injector converts to desktop-host-relative coords so the window lands
  // exactly on its monitor. width/height: initial size (renderer will resize
  // via SetWindowPos later). Returns true on success. On failure, logs the
  // error and returns false. Can be called again after Detach() to re-attach.
  // Honors the mode set via SetInjectMode (default kAuto).
  bool Attach(int x, int y, int width, int height);

  // Selects the injection strategy for subsequent Attach() calls (Step 4).
  // Sticky per injector: OnDisplayChange/Reanchor re-attaches keep it.
  void SetInjectMode(InjectMode mode);

  // Row 7: install the attach pass's resolved host (ResolveSharedHost).
  // Call before Attach(): Attach no longer discovers/spawns a host per call -
  // the per-slot spawn is the bug this replaces. host == nullptr (unresolved
  // pass) makes the next Attach fail honestly, which routes the slot to
  // MultiMonitor's headless fallback. Survives Detach(): Reanchor's
  // Detach/Attach pair reuses this pass's host.
  void SetSharedHost(const SharedHost& host);

  // Row 4: last CoverageReason token logged by the attach path
  // ("placement: covered", ...). Empty before the first attempt. Read by
  // MultiMonitor::SlotCoverageReason for get_state display_coverage.
  std::string last_coverage_reason() const;

  // Remove the injected window from the desktop and destroy it.
  // Idempotent. After this, injected_hwnd() returns nullptr.
  void Detach();

  // Handle WM_DISPLAYCHANGE: re-attach to maintain correct z-order and size.
  // x/y: monitor rect in virtual-screen coords; width/height: new resolution.
  // If not currently attached, this is a no-op.
  void OnDisplayChange(int x, int y, int width, int height);

  // The HWND of the attached child window, or nullptr if not attached.
  // The caller (renderer) should NOT destroy this handle — DesktopInjector
  // owns it and will destroy it in Detach()/destructor.
  void* injected_hwnd() const;

  // Re-assert the frameless wallpaper surface style on the live injected
  // window (WS_POPUP|WS_CLIPCHILDREN, no frame bits; ex TOOLWINDOW|NOACTIVATE
  // + LAYERED, no APPWINDOW). Called from Reanchor paths after Attach so
  // Explorer recreations can never leave a framed/taskbar/Alt-Tab window.
  // No-op when not attached. Never creates a window.
  void ReassertFrameless();

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace k6wp
