#pragma once

// Engine tray icon (Todo 35, fase-5): Shell_NotifyIconW menu on the engine's
// hidden message-only window.
//
// Design notes:
// - Header stays windows.h-free (project convention, cf. Todos 7/11/28):
//   HWND/HICON travel as opaque void*, WM_APP is spelled as its value
//   (0x8000). windows.h + shellapi.h live in tray.cpp only.
// - App icon (Todo 9): engine/app.rc embeds packaging/k6wp-on.ico as
//   IDI_APPICON (101) and packaging/k6wp-off.ico as IDI_APPICON_PAUSED
//   (102). tray.cpp loads the resource via LoadImageW and falls back to the
//   stock shared icons (IDI_APPLICATION = running, IDI_EXCLAMATION = paused)
//   when the resource is missing — the fallback is load-bearing, not
//   optional. The pause state is ALSO reflected in the tooltip text
//   ("K6WP Engine - Paused"), so the state change is visible even where the
//   glyphs look similar.
// - EngineApp owns one TrayIcon by value and forwards three things from its
//   existing WndProc (no second message window):
//     1. msg == kTrayCallbackMessage  -> OnTrayNotify(hwnd, wParam, lParam)
//     2. msg == taskbar-created msg   -> OnTaskbarCreated() (Explorer restart)
//     3. WM_COMMAND                   -> OnMenuCommand(id)
// - Menu actions leave TrayIcon through TrayCallbacks so the class never
//   touches EngineApp/MPV directly (testable, no circular include).

#include <cstddef>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

namespace k6wp {

// WM_APP + 20. Must match the uCallbackMessage given to Shell_NotifyIconW.
// Spelled numerically so this header needs no windows.h.
inline constexpr unsigned kTrayCallbackMessage = 0x8000u + 20;

// App-icon resource ids: single definition lives in packaging/resource.h
// (101 = running, 102 = paused) — keep these two constants in sync with it.
// tray.cpp falls back to stock shared icons when the resource is missing
// (e.g. a build without the .rc) — see LoadAppIcon in tray.cpp.
// Error state keeps icon 101 with an error tooltip (amber is paused-only).
inline constexpr unsigned kTrayResourceId = 101;
inline constexpr unsigned kTrayResourceIdPaused = 102;

// Menu command ids (private range, must not clash with anything else: the
// engine message window owns no other menus).
inline constexpr unsigned kTrayCmdOpenStudio = 1002;
inline constexpr unsigned kTrayCmdExit = 1003;
inline constexpr unsigned kTrayCmdNextWallpaper = 1004;
// Task 21: separate Pause / Resume entries (gray-state driven by is_paused).
// Both ids funnel through the same on_toggle_pause callback, guarded by the
// live paused state, so no EngineApp change is needed (EngineApp only sees
// OnMenuCommand ids).
inline constexpr unsigned kTrayCmdPause = 1005;
inline constexpr unsigned kTrayCmdResume = 1006;
// Donation touchpoint (non-intrusive): "Support the developer" tray item
// placed before Exit. Fires on_support like every other menu action.
inline constexpr unsigned kTrayCmdSupport = 1007;
inline constexpr unsigned kTrayCmdQuickBase = 1100;
inline constexpr unsigned kTrayCmdQuickMax = 5;  // last 5 videos

struct TrayCallbacks {
  std::function<void()> on_toggle_pause;                  // Pause/Resume item
  std::function<void(std::size_t)> on_quick_switch;       // quick-switch index
  std::function<void()> on_open_studio;                   // Show Studio item
  std::function<void()> on_support;                       // Support item (donate URL)
  std::function<void()> on_exit;                          // Exit item
  std::function<bool()> is_paused;                        // label: Pause vs Resume
  std::function<std::string()> get_current;               // current video (UTF-8), for Next
  std::function<void(std::size_t)> on_next;               // Next Wallpaper: MRU index to switch to
};

// Wiring (landed in engine_app.cpp Init, Todo 35 section):
//   tray_cb.get_current = [this]() { lock; return current_video_utf8_; };
//   tray_cb.on_next = [this](std::size_t idx) { OnTrayQuickSwitch(idx); };
// Both slots are live: Next Wallpaper resolves the MRU index and hot-swaps
// via the validated set_video path; an empty MRU is a logged no-op.

class TrayIcon {
 public:
  TrayIcon() = default;
  ~TrayIcon();

  TrayIcon(const TrayIcon&) = delete;
  TrayIcon& operator=(const TrayIcon&) = delete;

  // Registers the TaskbarCreated window message, stores hwnd/msg/callbacks
  // and NIM_ADDs the icon. hwnd is the engine's message-only HWND (void* to
  // keep windows.h out of the header). Failure (e.g. no Explorer yet) is
  // non-fatal: returns false and the engine keeps running; OnTaskbarCreated()
  // re-adds the icon when the taskbar appears.
  bool Install(void* hwnd, TrayCallbacks callbacks);

  // NIM_DELETEs the icon. Idempotent; also called from the destructor.
  void Remove();

  // Switches running/paused icon + tooltip. Thread-safe (may be called from
  // the IPC worker thread via OnSuspend/OnResume); only touches the icon
  // when installed_.
  void SetPaused(bool paused);

  // Flips the tooltip to an error state ("K6WP Engine - Injection failed
  // (headless)") when the wallpaper surface could not be attached. Thread-
  // safe; only touches the icon when installed_.
  void SetError(bool error);

  // Records a successful set_video path at the front of the MRU list (max 5,
  // de-duplicated). Thread-safe; the menu is rebuilt from this list on every
  // popup, so no icon update is needed here.
  void PushRecent(const std::string& utf8_path);

  // Handles msg == kTrayCallbackMessage from EngineApp::HandleMessage.
  // wParam = icon id, lParam = mouse message (WM_RBUTTONUP -> popup menu,
  // WM_LBUTTONDBLCLK -> open Studio via the launcher singleton). Must run
  // on the UI thread (WndProc).
  void OnTrayNotify(void* hwnd, std::size_t w_param, long l_param);

  // Handles WM_COMMAND for the kTrayCmd* ids and the quick-switch range.
  // Unknown ids are ignored. Runs on the UI thread.
  void OnMenuCommand(unsigned id);

  // Re-NIM_ADDs the icon after Explorer restarts (TaskbarCreated). No-op
  // when never installed.
  void OnTaskbarCreated();

  // Returns the MRU entry at idx (UTF-8) or empty when out of range.
  // Used by the on_quick_switch callback to resolve the index to a path.
  std::string RecentAt(std::size_t idx) const;

  // The RegisterWindowMessageW(L"TaskbarCreated") value captured at Install().
  // EngineApp compares incoming unknown messages against this (0 = not
  // installed yet).
  unsigned taskbar_created_msg() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return taskbar_created_msg_;
  }

  // Centralized icon state update: computes the effective paused state
  // (paused_ only), loads the appropriate cached resource icon (101/102),
  // and calls ModifyIconLocked() if installed.
  // Thread-safe (locks mutex_); may be called from IPC worker thread.
  void UpdateIconForState();

 private:
  // (Re-)adds the icon with the current paused_ state. Caller holds mutex_.
  // hwnd_locked is the stored message HWND.
  bool AddIconLocked();
  void DeleteIconLocked();
  void ModifyIconLocked();
  // Fills cbSize/hWnd/uID/flags/icon/tip; extra_flags is OR'd into uFlags
  // (NIF_MESSAGE for the initial ADD). Replaces the previous owned icon.
  // Takes the NOTIFYICONDATAW by void* so the header stays windows.h-free.
  void PrepareIconLocked(void* nid, unsigned extra_flags);

  // Computes whether the tray should show the paused (off) icon.
  // Returns true when paused_ == true (error_ never borrows the amber icon;
  // error keeps icon 101 with the error tooltip). Caller must hold mutex_.
  bool ResolveShouldShowPausedLocked() const;

  mutable std::mutex mutex_;
  void* hwnd_ = nullptr;  // engine message window (HWND); process-lifetime
  TrayCallbacks callbacks_;
  std::vector<std::string> recent_utf8_;  // MRU, front = newest, max 5
  unsigned taskbar_created_msg_ = 0;
  bool installed_ = false;
  bool paused_ = false;
  bool error_ = false;
  // Currently loaded icon (HICON, opaque). Owned (DestroyIcon on replace /
  // remove) when icon_owned_ is true — that is, when it came from the .rc
  // resource via LoadImageW; stock shared icons are never destroyed.
  void* icon_ = nullptr;
  bool icon_owned_ = false;
};

}  // namespace k6wp
