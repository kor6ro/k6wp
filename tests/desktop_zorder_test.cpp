// Unit tests for the ONE z-order contract across every injection branch
// (plan row 8). No external test framework: plain Check() counter, same
// style as tests/workerw_span_test.cpp (row 10). Exit 0 = pass.
//
// The predicate k6wp::ResolveZOrderInsertAfter lives in desktop_inject.cpp
// at k6wp scope. It is NOT declared in desktop_inject.hpp (that header must
// stay windows.h-free), so this TU declares the prototype itself - the row
// 10 linkage pattern.
//
// Acceptance (plan row 8): the predicate returns the DefView handle ONLY
// when GetParent(DefView) == GetParent(host) - i.e. DefView is a sibling of
// the window being z-ordered - and HWND_BOTTOM otherwise, with the reason
// token "z-order: sibling-mismatch-fallback". The predicate is a thin guard
// over Win32 GetParent, so the fixture builds a REAL hidden parent/child
// hierarchy with CreateWindowExW (headless: no window is ever shown).
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <cstring>
#include <iostream>
#include <string>

namespace k6wp {
// Row 8: k6wp-scope declaration (desktop_inject.cpp defines it between
// `namespace k6wp {` and its anonymous namespace - row 10 C5046 lesson).
//
// Contract: `target` is the desktop HOST the window being z-ordered is
// parented to (runtime: CreateAndAttach's target, the SetParent argument).
// Returns `insert_after` iff GetParent(insert_after) == target - i.e. the
// requested hint is a child of the host, which is exactly "the window being
// placed and insert_after are SIBLINGS":
//     GetParent(insert_after) == GetParent(placed_window)
// because GetParent(placed_window) == target by construction (SetParent).
// GetParent must NOT be called on the placed window itself: it keeps
// WS_POPUP (EnforceFramelessStyle), and GetParent on a WS_POPUP returns its
// OWNER (null) - proven live on the dev box (target_parent=0x0).
// On mismatch: HWND_BOTTOM + *reason "z-order: sibling-mismatch-fallback".
HWND ResolveZOrderInsertAfter(HWND insert_after, HWND target,
                              const char** reason);
}  // namespace k6wp

namespace {

int g_checks = 0;
int g_failures = 0;

void Check(bool cond, const std::string& name) {
  ++g_checks;
  if (cond) {
    std::cout << "[PASS] " << name << "\n";
  } else {
    ++g_failures;
    std::cout << "[FAIL] " << name << "\n";
  }
}

constexpr wchar_t kTestClass[] = L"K6WP.DesktopZorderTest.1";

// Hidden test window (never shown). `parent` == nullptr -> top-level.
HWND MakeWindow(HWND parent) {
  const DWORD style = parent ? WS_CHILD : WS_OVERLAPPED;
  return CreateWindowExW(0, kTestClass, L"", style, 0, 0, 8, 8, parent,
                         nullptr, GetModuleHandleW(nullptr), nullptr);
}

}  // namespace

int main() {
  WNDCLASSW wc{};
  wc.lpfnWndProc = DefWindowProcW;
  wc.hInstance = GetModuleHandleW(nullptr);
  wc.lpszClassName = kTestClass;
  if (!RegisterClassW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
    std::cout << "[FAIL] RegisterClassW failed (error " << GetLastError()
              << ")\n";
    return 1;
  }

  // Fixture.
  //   host (top-level desktop host): [def_view, placed]  <- siblings: the
  //     contract HOLDS: GetParent(def_view)==GetParent(placed)==host
  //   other_host: [def_view_other]  <- DefView parented to a different
  //     window than the host: FALLBACK
  // `placed` stands in for the injected window at the moment of the
  // SetWindowPos call: it has just been SetParent'ed to `host`, so the
  // sibling predicate GetParent(def_view)==GetParent(placed) is exactly the
  // guard the predicate implements as GetParent(insert_after)==target.
  HWND host = MakeWindow(nullptr);
  HWND other_host = MakeWindow(nullptr);
  HWND def_view = MakeWindow(host);
  HWND placed = MakeWindow(host);
  HWND def_view_other = MakeWindow(other_host);
  if (!host || !other_host || !def_view || !placed || !def_view_other) {
    std::cout << "[FAIL] fixture window creation failed (error "
              << GetLastError() << ")\n";
    return 1;
  }

  // --- happy: DefView is a child of the host (siblings of `placed`) -------
  const char* reason = "untouched";
  HWND resolved = k6wp::ResolveZOrderInsertAfter(def_view, host, &reason);
  Check(GetParent(def_view) == GetParent(placed),
        "fixture: GetParent(DefView)==GetParent(host) holds");
  Check(GetParent(placed) == host,
        "fixture: placed window's parent is the host (SetParent invariant)");
  Check(resolved == def_view,
        "z-order: GetParent(DefView)==GetParent(host) -> returns DefView");
  Check(reason != nullptr && reason[0] == '\0',
        "z-order: satisfied contract sets no fallback reason");
  Check((GetParent(def_view) == GetParent(placed)) ==
            (resolved == def_view),
        "z-order: returns DefView ONLY when GetParent(DefView)==GetParent("
        "host)");

  // --- failure: DefView parented to a different window than the host ------
  const char* mismatch_reason = "untouched";
  HWND fallback = k6wp::ResolveZOrderInsertAfter(def_view_other, host,
                                                 &mismatch_reason);
  Check(GetParent(def_view_other) != GetParent(placed),
        "fixture: GetParent(DefView)!=GetParent(host) holds");
  Check(fallback == HWND_BOTTOM,
        "z-order: DefView parented elsewhere -> HWND_BOTTOM");
  Check(mismatch_reason != nullptr &&
            std::strcmp(mismatch_reason,
                        "z-order: sibling-mismatch-fallback") == 0,
        "z-order: reason == \"z-order: sibling-mismatch-fallback\"");
  Check((GetParent(def_view_other) == GetParent(placed)) ==
            (fallback == def_view_other),
        "z-order: HWND_BOTTOM ONLY when GetParent(DefView)!=GetParent(host)");

  // --- no DefView at all -> HWND_BOTTOM, never a crash --------------------
  const char* null_reason = "untouched";
  Check(k6wp::ResolveZOrderInsertAfter(nullptr, host, &null_reason) ==
            HWND_BOTTOM,
        "z-order: nullptr DefView -> HWND_BOTTOM (no crash)");
  Check(null_reason != nullptr && null_reason[0] != '\0' &&
            std::strcmp(null_reason, "z-order: sibling-mismatch-fallback") !=
                0,
        "z-order: absent hint logs a distinct reason (never silent, never "
        "the sibling token)");

  // --- already at the bottom -> passthrough, no false fallback ------------
  const char* bottom_reason = "untouched";
  Check(k6wp::ResolveZOrderInsertAfter(HWND_BOTTOM, host, &bottom_reason) ==
            HWND_BOTTOM,
        "z-order: HWND_BOTTOM request -> HWND_BOTTOM");
  Check(bottom_reason != nullptr && bottom_reason[0] == '\0',
        "z-order: HWND_BOTTOM request is not logged as a fallback");

  // --- no window being z-ordered -> HWND_BOTTOM with a logged reason ------
  const char* no_target_reason = "";
  Check(k6wp::ResolveZOrderInsertAfter(def_view, nullptr, &no_target_reason) ==
            HWND_BOTTOM,
        "z-order: nullptr target -> HWND_BOTTOM");
  Check(no_target_reason != nullptr && no_target_reason[0] != '\0',
        "z-order: nullptr target is never a silent ignore");

  // --- null out-param tolerated (engine may log-less) ---------------------
  Check(k6wp::ResolveZOrderInsertAfter(def_view, host, nullptr) == def_view,
        "z-order: nullptr reason out-param tolerated");

  DestroyWindow(def_view_other);
  DestroyWindow(placed);
  DestroyWindow(def_view);
  DestroyWindow(other_host);
  DestroyWindow(host);
  UnregisterClassW(kTestClass, GetModuleHandleW(nullptr));

  std::cout << "desktop_zorder_test: " << g_checks << " checks, "
            << g_failures << " failures\n";
  return g_failures == 0 ? 0 : 1;
}
