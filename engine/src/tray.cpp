// Engine tray icon implementation (Todo 35). windows.h + shellapi.h live
// here only; tray.hpp stays Win32-free.
//
// MSDN refs: Shell_NotifyIconW (NIM_ADD/NIM_MODIFY/NIM_DELETE,
//   NOTIFYICONDATAW, NIF_MESSAGE|NIF_ICON|NIF_TIP), TrackPopupMenu +
//   SetForegroundWindow + PostMessage(WM_NULL) popup pattern,
//   RegisterWindowMessageW(L"TaskbarCreated") for Explorer-restart survival.

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shellapi.h>

#include "tray.hpp"

#include <cstdarg>
#include <cstdio>
#include <exception>
#include <filesystem>

#include "log_file.hpp"

namespace k6wp {
namespace {

// Engine message-window icon id (single icon, arbitrary).
constexpr UINT kIconId = 1;
constexpr size_t kMaxRecent = 5;
// Stock icon ids (same numbers as IDI_APPLICATION / IDI_EXCLAMATION, which
// are MAKEINTRESOURCEA pointers and unusable with the W API directly).
constexpr WORD kStockAppIcon = 32512;
constexpr WORD kStockPausedIcon = 32515;

// Filename shown in the quick-switch submenu. Falls back to the raw UTF-8 on
// conversion failure (never throws out of the WndProc path).
std::wstring RecentLabel(const std::string& utf8_path) {
  try {
    const std::filesystem::path p = std::filesystem::u8path(utf8_path);
    std::wstring name = p.filename().wstring();
    if (!name.empty()) return name;
  } catch (...) {
  }
  try {
    return std::filesystem::u8path(utf8_path).wstring();
  } catch (...) {
  }
  return L"(unknown)";
}

// Appends the MRU "Quick-switch" submenu to `menu`. True when the submenu was
// parented to `menu` (and is therefore freed by the caller's DestroyMenu).
//
// The submenu is created INSIDE the `recent.empty()` check on purpose: creating
// it unconditionally leaked one USER menu handle per tray right-click while the
// MRU was empty (created, never parented, so DestroyMenu could not reach it),
// and recent_utf8_ is only filled by PushRecent() - i.e. it fired for the whole
// first-run window. Do not hoist the CreatePopupMenu out of this guard.
bool AppendQuickSwitchSubmenu(HMENU menu, const std::vector<std::string>& recent) {
  if (menu == nullptr || recent.empty()) {
    return false;
  }
  HMENU quick = CreatePopupMenu();
  if (quick == nullptr) {
    return false;
  }
  for (std::size_t i = 0; i < recent.size() && i < kTrayCmdQuickMax; ++i) {
    AppendMenuW(quick, MF_STRING, kTrayCmdQuickBase + static_cast<UINT>(i),
                RecentLabel(recent[i]).c_str());
  }
  // Attach last, releasing `quick` if the attach fails, so the handle is owned
  // by `menu` or destroyed - never orphaned.
  if (AppendMenuW(menu, MF_STRING | MF_POPUP, reinterpret_cast<UINT_PTR>(quick),
                  L"Quick-switch") == FALSE) {
    DestroyMenu(quick);
    return false;
  }
  return true;
}

void Log(const char* fmt, ...);

#ifndef K6WP_VERBOSE
#define K6WP_VERBOSE 0
#endif

void Log(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  EngineLogfV(fmt, args);
  va_end(args);
}

// Loads the app icon for the given state (Todo 9). Prefers the embedded .rc
// resource (IDI_APPICON / IDI_APPICON_PAUSED, sized to the system small-icon
// metric: 16 px at 100% DPI, 32 px at 200% DPI); falls back to the stock
// shared icons when the resource is missing (build without app.rc). Sets
// *owned_out = true when the caller must DestroyIcon the result (resource
// icons are owned; stock icons are shared and must never be destroyed).
HICON LoadAppIcon(bool paused, bool* owned_out) {
  const HINSTANCE hinst = GetModuleHandleW(nullptr);
  const WORD resource_id = paused ? kTrayResourceIdPaused : kTrayResourceId;
  const int cx = GetSystemMetrics(SM_CXSMICON);
  const int cy = GetSystemMetrics(SM_CYSMICON);
  HICON icon = static_cast<HICON>(LoadImageW(
      hinst, MAKEINTRESOURCEW(resource_id), IMAGE_ICON, cx, cy, LR_DEFAULTCOLOR));
  if (icon != nullptr) {
    Log("tray: using embedded app icon (resource %u, paused=%d)", resource_id, paused ? 1 : 0);
    *owned_out = true;
    return icon;
  }
  icon = LoadIconW(nullptr, MAKEINTRESOURCEW(paused ? kStockPausedIcon : kStockAppIcon));
  Log("tray: using stock icon (resource %u missing, paused=%d)", resource_id, paused ? 1 : 0);
  *owned_out = false;
  return icon;
}

}  // namespace

TrayIcon::~TrayIcon() { Remove(); }

bool TrayIcon::Install(void* hwnd, TrayCallbacks callbacks) {
  std::lock_guard<std::mutex> lock(mutex_);
  hwnd_ = hwnd;
  callbacks_ = std::move(callbacks);
  taskbar_created_msg_ = RegisterWindowMessageW(L"TaskbarCreated");
  if (taskbar_created_msg_ == 0) {
    Log("tray: RegisterWindowMessageW(TaskbarCreated) failed (error %lu)", GetLastError());
  }
  if (hwnd_ == nullptr) {
    Log("tray: Install with null HWND, skipping NIM_ADD");
    return false;
  }
  if (!AddIconLocked()) {
    Log("tray: NIM_ADD failed (error %lu), engine keeps running without tray", GetLastError());
    return false;
  }
  installed_ = true;
  Log("tray: icon installed (callback=WM_APP+20)");
  return true;
}

void TrayIcon::Remove() {
  std::lock_guard<std::mutex> lock(mutex_);
  DeleteIconLocked();
  installed_ = false;
  hwnd_ = nullptr;
}

bool TrayIcon::ResolveShouldShowPausedLocked() const {
  // Step 6.2c: error no longer borrows the paused (amber) icon — the error
  // state keeps the running icon (resource 101) with the error tooltip, so
  // amber is exclusive to a real pause.
  return paused_;
}

void TrayIcon::UpdateIconForState() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!installed_) return;
  // The icon selection is driven by ResolveShouldShowPausedLocked (paused_
  // only -> off icon, resource 102; else on icon, resource 101) inside
  // ModifyIconLocked. The tooltip logic inside ModifyIconLocked prioritizes
  // error_ over paused_ for the text, so the error state stays visible.
  ModifyIconLocked();
}

void TrayIcon::SetPaused(bool paused) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    paused_ = paused;
  }
  UpdateIconForState();
}

void TrayIcon::SetError(bool error) {
  bool changed = false;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (error_ != error) {
      error_ = error;
      changed = true;
    }
  }
  if (changed) {
    Log("tray: error state %s", error ? "entered" : "cleared");
  }
  UpdateIconForState();
}

void TrayIcon::PushRecent(const std::string& utf8_path) {
  if (utf8_path.empty()) return;
  std::lock_guard<std::mutex> lock(mutex_);
  // De-duplicate: move existing entry to front.
  for (auto it = recent_utf8_.begin(); it != recent_utf8_.end(); ++it) {
    if (*it == utf8_path) {
      recent_utf8_.erase(it);
      break;
    }
  }
  recent_utf8_.insert(recent_utf8_.begin(), utf8_path);
  if (recent_utf8_.size() > kMaxRecent) recent_utf8_.resize(kMaxRecent);
}

void TrayIcon::OnTrayNotify(void* hwnd, std::size_t w_param, long l_param) {
  if (w_param != kIconId) return;
  const HWND h = static_cast<HWND>(hwnd);
  const UINT mouse_msg = static_cast<UINT>(l_param);
  if (mouse_msg == WM_RBUTTONUP || mouse_msg == WM_CONTEXTMENU) {
    // Snapshot state under lock, build + track the menu without holding it
    // (callbacks re-enter PushRecent/SetPaused on other threads).
    bool paused = false;
    std::vector<std::string> recent;
    TrayCallbacks cb;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (!installed_) return;
      paused = paused_;
      recent = recent_utf8_;
      cb = callbacks_;
    }
    bool cb_paused = paused;
    if (cb.is_paused) {
      try {
        cb_paused = cb.is_paused();
      } catch (...) {
      }
    }

    HMENU menu = CreatePopupMenu();
    if (menu == nullptr) {
      Log("tray: CreatePopupMenu failed (error %lu)", GetLastError());
      return;
    }
    AppendMenuW(menu, MF_STRING, kTrayCmdOpenStudio, L"Open &K6WP");
    // Task 21: separate Pause / Resume entries (spec lists both). The live
    // paused state drives the gray-state; each entry only fires when it
    // would change state (guarded in OnMenuCommand via is_paused).
    AppendMenuW(menu, MF_STRING | (cb_paused ? MF_GRAYED : 0), kTrayCmdPause,
                L"&Pause");
    AppendMenuW(menu, MF_STRING | (cb_paused ? 0 : MF_GRAYED), kTrayCmdResume,
                L"&Resume");
    // Next Wallpaper cycles the MRU list; grayed when there is nothing to
    // cycle (no recent videos yet).
    AppendMenuW(menu, MF_STRING | (recent.empty() ? MF_GRAYED : 0),
                kTrayCmdNextWallpaper, L"Next &Wallpaper");

    AppendQuickSwitchSubmenu(menu, recent);
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, kTrayCmdSupport, L"Support the &developer");
    AppendMenuW(menu, MF_STRING, kTrayCmdExit, L"E&xit");

    POINT pt{};
    GetCursorPos(&pt);
    // Standard TrackPopupMenu pattern: foreground the message window so the
    // menu dismisses on click-away, then poke WM_NULL after tracking.
    SetForegroundWindow(h);
    TrackPopupMenu(menu, TPM_RIGHTBUTTON | TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, h,
                   nullptr);
    PostMessageW(h, WM_NULL, 0, 0);
    DestroyMenu(menu);  // destroys the quick-switch submenu too (child)
    Log("tray: popup menu shown (paused=%d recent=%llu)", cb_paused ? 1 : 0,
        static_cast<unsigned long long>(recent.size()));
  } else if (mouse_msg == WM_LBUTTONDBLCLK) {
    TrayCallbacks cb;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (!installed_) return;
      cb = callbacks_;
    }
    if (cb.on_open_studio) {
      try {
        cb.on_open_studio();
      } catch (...) {
      }
    }
  }
}

void TrayIcon::OnMenuCommand(unsigned id) {
  TrayCallbacks cb;
  std::vector<std::string> recent;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!installed_) return;
    cb = callbacks_;
    recent = recent_utf8_;
  }
  // Live paused state for the guarded Pause/Resume entries below.
  bool live_paused = false;
  if (cb.is_paused) {
    try {
      live_paused = cb.is_paused();
    } catch (...) {
    }
  }
  try {
    if (id == kTrayCmdPause) {
      Log("tray: menu Pause selected");
      if (live_paused) {
        Log("tray: Pause ignored (already paused)");
      } else if (cb.on_toggle_pause) {
        cb.on_toggle_pause();
      }
    } else if (id == kTrayCmdResume) {
      Log("tray: menu Resume selected");
      if (!live_paused) {
        Log("tray: Resume ignored (already running)");
      } else if (cb.on_toggle_pause) {
        cb.on_toggle_pause();
      }
    } else if (id == kTrayCmdOpenStudio) {
      Log("tray: menu Open K6WP selected");
      if (cb.on_open_studio) cb.on_open_studio();
    } else if (id == kTrayCmdSupport) {
      Log("tray: menu Support the developer selected");
      if (cb.on_support) cb.on_support();
    } else if (id == kTrayCmdNextWallpaper) {
      Log("tray: menu Next Wallpaper selected");
      if (recent.empty()) {
        Log("tray: Next Wallpaper ignored (MRU empty)");
      } else if (!cb.on_next) {
        Log("tray: Next Wallpaper ignored (on_next callback not set)");
      } else {
        // index = (current MRU position + 1) % size; unknown current wraps
        // to the front (index 0 = most recent).
        std::string current;
        if (cb.get_current) {
          try {
            current = cb.get_current();
          } catch (...) {
          }
        }
        std::size_t cur = recent.size();
        for (std::size_t i = 0; i < recent.size(); ++i) {
          if (recent[i] == current) {
            cur = i;
            break;
          }
        }
        const std::size_t next_idx = (cur + 1) % recent.size();
        Log("tray: Next Wallpaper -> MRU[%llu] (current='%s')",
            static_cast<unsigned long long>(next_idx), current.c_str());
        cb.on_next(next_idx);
      }
    } else if (id == kTrayCmdExit) {
      Log("tray: menu Exit selected");
      if (cb.on_exit) cb.on_exit();
    } else if (id >= kTrayCmdQuickBase && id < kTrayCmdQuickBase + kTrayCmdQuickMax) {
      const std::size_t idx = id - kTrayCmdQuickBase;
      Log("tray: menu Quick-switch[%llu] selected", static_cast<unsigned long long>(idx));
      if (cb.on_quick_switch) cb.on_quick_switch(idx);
    }
  } catch (const std::exception& e) {
    Log("tray: menu handler threw: %s", e.what());
  } catch (...) {
    Log("tray: menu handler threw an unknown exception");
  }
}

void TrayIcon::OnTaskbarCreated() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (hwnd_ == nullptr) return;  // never installed: nothing to re-add
  DeleteIconLocked();  // Explorer is fresh; ensure no stale state, then add
  if (AddIconLocked()) {
    installed_ = true;
    Log("tray: icon re-added after TaskbarCreated (Explorer restart)");
  } else {
    installed_ = false;
    Log("tray: re-add after TaskbarCreated failed (error %lu)", GetLastError());
  }
}

std::string TrayIcon::RecentAt(std::size_t idx) const {
  std::lock_guard<std::mutex> lock(mutex_);
  return idx < recent_utf8_.size() ? recent_utf8_[idx] : std::string();
}

bool TrayIcon::AddIconLocked() {
  NOTIFYICONDATAW nid{};
  nid.cbSize = sizeof(nid);
  nid.hWnd = static_cast<HWND>(hwnd_);
  nid.uID = kIconId;
  nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP | NIF_SHOWTIP;
  nid.uCallbackMessage = static_cast<UINT>(kTrayCallbackMessage);
  bool owned = false;
  nid.hIcon = LoadAppIcon(ResolveShouldShowPausedLocked(), &owned);
  if (icon_owned_) DestroyIcon(static_cast<HICON>(icon_));  // replace old owned
  icon_ = nid.hIcon;
  icon_owned_ = owned;
  const wchar_t* tip = error_ ? L"K6WP Engine - Injection failed (headless)"
                     : paused_ ? L"K6WP Engine - Paused" : L"K6WP Engine";
  wcsncpy_s(nid.szTip, tip, _TRUNCATE);
  return Shell_NotifyIconW(NIM_ADD, &nid) != FALSE;
}

void TrayIcon::DeleteIconLocked() {
  if (hwnd_ == nullptr) return;
  NOTIFYICONDATAW nid{};
  nid.cbSize = sizeof(nid);
  nid.hWnd = static_cast<HWND>(hwnd_);
  nid.uID = kIconId;
  Shell_NotifyIconW(NIM_DELETE, &nid);
  if (icon_owned_) {
    DestroyIcon(static_cast<HICON>(icon_));
    icon_ = nullptr;
    icon_owned_ = false;
  }
}

void TrayIcon::ModifyIconLocked() {
  NOTIFYICONDATAW nid{};
  nid.cbSize = sizeof(nid);
  nid.hWnd = static_cast<HWND>(hwnd_);
  nid.uID = kIconId;
  nid.uFlags = NIF_ICON | NIF_TIP | NIF_SHOWTIP;
  bool owned = false;
  nid.hIcon = LoadAppIcon(ResolveShouldShowPausedLocked(), &owned);
  if (icon_owned_) DestroyIcon(static_cast<HICON>(icon_));  // replace old owned
  icon_ = nid.hIcon;
  icon_owned_ = owned;
  const wchar_t* tip = error_ ? L"K6WP Engine - Injection failed (headless)"
                     : paused_ ? L"K6WP Engine - Paused" : L"K6WP Engine";
  wcsncpy_s(nid.szTip, tip, _TRUNCATE);
  if (!Shell_NotifyIconW(NIM_MODIFY, &nid)) {
    // Explorer may have restarted without us seeing TaskbarCreated yet;
    // try a fresh ADD so the icon comes back.
    AddIconLocked();
  }
}

}  // namespace k6wp
