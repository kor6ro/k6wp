// Unit tests for the engine tray popup-menu construction (engine/src/tray.cpp).
// No external test framework: plain asserts with a pass/fail counter, same
// style as tests/occlusion_test.cpp / tests/gpu_pin_test.cpp. Exit code 0 =
// all pass.
//
// AppendQuickSwitchSubmenu lives in an anonymous namespace inside tray.cpp, so
// this TU #includes the REAL .cpp (gpu_pin_test-style single-TU pattern, cf.
// gpu_pin_test.cpp:20): hoisting its CreatePopupMenu back out of the empty-MRU
// guard turns the handle-balance checks RED.
//
// TrackPopupMenu is never called (it would block on an unowned menu), so the
// test drives the construction half - the half that owned the HMENU leak - and
// asserts USER-object handle balance through GetGuiResources. log_file.cpp is
// linked for AppendEngineLogLine, which tray.cpp's Log() references.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include "../engine/src/tray.cpp"

#include <cstdio>
#include <string>
#include <vector>

namespace {

int g_checks = 0;
int g_failures = 0;

void Check(bool cond, const std::string& name) {
  ++g_checks;
  if (cond) {
    std::printf("[PASS] %s\n", name.c_str());
  } else {
    ++g_failures;
    std::printf("[FAIL] %s\n", name.c_str());
  }
}

// Min of 3 samples 20 ms apart. GetGuiResources(GR_USER_OBJECTS) counts every
// USER object in the process (windows, icons and cursors as well as menus), so
// an unrelated object appearing mid-loop must not read as a leak; taking the
// minimum absorbs that. A real leak in this test is +1 per iteration over 64
// iterations, far above the noise this removes.
unsigned UserObjectFloor() {
  unsigned best = MAXDWORD;
  for (int i = 0; i < 3; ++i) {
    const unsigned n = GetGuiResources(GetCurrentProcess(), GR_USEROBJECTS);
    if (n < best) {
      best = n;
    }
    Sleep(20);
  }
  return best;
}

// Popups are the full TrayIcon::OnTrayNotify build path minus the tracking
// call: create parent -> append quick-switch -> DestroyMenu(parent).
unsigned PopupHandleDelta(const std::vector<std::string>& recent, int rounds) {
  // Warm-up round: the first menu a process creates can initialise USER state
  // that is not a per-menu object, so it is spent before the baseline.
  {
    HMENU warm = CreatePopupMenu();
    k6wp::AppendQuickSwitchSubmenu(warm, recent);
    DestroyMenu(warm);
  }
  const unsigned before = UserObjectFloor();
  for (int i = 0; i < rounds; ++i) {
    HMENU menu = CreatePopupMenu();
    k6wp::AppendQuickSwitchSubmenu(menu, recent);
    DestroyMenu(menu);
  }
  const unsigned after = UserObjectFloor();
  return after > before ? after - before : 0;
}

std::string DeltaName(const char* what, unsigned delta, int rounds) {
  return std::string(what) + " (delta=" + std::to_string(delta) + " over " +
         std::to_string(rounds) + " popups)";
}

constexpr int kRounds = 64;

void TestEmptyMruAddsNothing() {
  HMENU menu = CreatePopupMenu();
  if (menu == nullptr) {
    Check(false, "tray: CreatePopupMenu unavailable in this session");
    return;
  }
  Check(!k6wp::AppendQuickSwitchSubmenu(menu, {}),
        "tray: empty MRU reports no submenu added");
  Check(GetMenuItemCount(menu) == 0,
        "tray: empty MRU leaves the popup menu unchanged");
  DestroyMenu(menu);
}

// The regression: quick-switch used to be created unconditionally and parented
// only when the MRU was non-empty, so every right-click before the first
// applied video leaked one USER menu handle. This is the first-run case, since
// recent_utf8_ is only filled by PushRecent().
void TestEmptyMruLeaksNoHandles() {
  const unsigned delta = PopupHandleDelta({}, kRounds);
  Check(delta == 0,
        DeltaName("tray: empty MRU leaks no USER menu handles", delta, kRounds));
}

void TestPopulatedMruIsParented() {
  HMENU menu = CreatePopupMenu();
  if (menu == nullptr) {
    Check(false, "tray: CreatePopupMenu unavailable in this session");
    return;
  }
  const std::vector<std::string> recent = {"C:/videos/one.mp4", "C:/videos/two.mp4"};
  Check(k6wp::AppendQuickSwitchSubmenu(menu, recent),
        "tray: populated MRU adds the Quick-switch submenu");
  Check(GetMenuItemCount(menu) == 1, "tray: exactly one Quick-switch entry");
  HMENU sub = GetSubMenu(menu, 0);
  Check(sub != nullptr, "tray: the entry is a real submenu");
  if (sub != nullptr) {
    Check(GetMenuItemCount(sub) == static_cast<int>(recent.size()),
          "tray: submenu lists every MRU entry");
    Check(GetMenuItemID(sub, 0) == static_cast<UINT>(k6wp::kTrayCmdQuickBase) &&
              GetMenuItemID(sub, 1) ==
                  static_cast<UINT>(k6wp::kTrayCmdQuickBase + 1),
          "tray: submenu ids are the kTrayCmdQuickBase range");
  }
  // DestroyMenu(parent) is the only release point, so the balance check below
  // is what proves the child was really parented.
  DestroyMenu(menu);
  const unsigned delta = PopupHandleDelta(recent, kRounds);
  Check(delta == 0,
        DeltaName("tray: populated MRU leaves no handles after DestroyMenu",
                  delta, kRounds));
}

void TestMruIsCappedToQuickMax() {
  HMENU menu = CreatePopupMenu();
  if (menu == nullptr) {
    Check(false, "tray: CreatePopupMenu unavailable in this session");
    return;
  }
  const std::vector<std::string> recent(6, "C:/videos/x.mp4");
  Check(k6wp::AppendQuickSwitchSubmenu(menu, recent),
        "tray: over-long MRU still adds the submenu");
  HMENU sub = GetSubMenu(menu, 0);
  Check(sub != nullptr &&
            GetMenuItemCount(sub) == static_cast<int>(k6wp::kTrayCmdQuickMax),
        "tray: submenu is capped at kTrayCmdQuickMax items");
  DestroyMenu(menu);
}

void TestNullParentIsRejected() {
  const std::vector<std::string> recent = {"C:/videos/one.mp4"};
  const unsigned before = UserObjectFloor();
  Check(!k6wp::AppendQuickSwitchSubmenu(nullptr, recent),
        "tray: null parent menu is rejected");
  Check(UserObjectFloor() == before,
        "tray: null parent menu creates no submenu to leak");
}

}  // namespace

int main() {
  TestEmptyMruAddsNothing();
  TestEmptyMruLeaksNoHandles();
  TestPopulatedMruIsParented();
  TestMruIsCappedToQuickMax();
  TestNullParentIsRejected();
  std::printf("%d checks, %d failures\n", g_checks, g_failures);
  return g_failures == 0 ? 0 : 1;
}
