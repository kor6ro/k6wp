#include "tray_actions.hpp"

#include "links.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shellapi.h>

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace k6wp {

void OpenStudio(TrayLogFn log) {
  wchar_t exe_path[MAX_PATH] = {};
  const DWORD len = GetModuleFileNameW(nullptr, exe_path, MAX_PATH);
  if (len == 0 || len >= MAX_PATH) {
    if (log) {
      log("tray: Open Studio failed (GetModuleFileNameW error %lu)",
          GetLastError());
    }
    return;
  }
  const std::filesystem::path exe_dir =
      std::filesystem::path(exe_path).parent_path();
  std::error_code ec;
  // Prefer the launcher singleton: K6WP.exe --studio owns the
  // Local\K6WP-Studio-Singleton mutex and focuses an existing studio window
  // (launcher/main.cpp FocusExistingStudio, matched by exe image name)
  // instead of spawning a duplicate. Never bypass the launcher when present.
  const std::filesystem::path launcher = exe_dir / L"K6WP.exe";
  if (std::filesystem::exists(launcher, ec)) {
    std::wstring cmd = L"\"" + launcher.wstring() + L"\" --studio";
    std::vector<wchar_t> cmd_buf(cmd.begin(), cmd.end());
    cmd_buf.push_back(L'\0');
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    if (!CreateProcessW(launcher.c_str(), cmd_buf.data(), nullptr, nullptr,
                        FALSE, DETACHED_PROCESS, nullptr, nullptr, &si, &pi)) {
      if (log) {
        log("tray: Open Studio via launcher failed (CreateProcess error %lu)",
            GetLastError());
      }
      return;
    }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    if (log) log("tray: Open Studio via launcher (%ls --studio)", launcher.c_str());
    return;
  }
  // Fallback (launcher binary absent only): focus the real studio window by
  // its measured title, else spawn studio.exe directly. Measured 2026-09-17:
  // title "K6WP Studio", Qt-owned class (e.g. Qt683dQWindowIcon) — never
  // "Studio"/"StudioWindow" (Todo 12 duplicate root cause).
  HWND hwnd = FindWindowW(nullptr, L"K6WP Studio");
  if (hwnd != nullptr) {
    ShowWindow(hwnd, SW_RESTORE);
    SetForegroundWindow(hwnd);
    if (log) log("tray: Studio window focused (existing instance, launcher missing)");
    return;
  }
  const std::filesystem::path studio = exe_dir / L"studio.exe";
  if (!std::filesystem::exists(studio, ec)) {
    if (log) log("tray: Open Studio failed (not found: %ls)", studio.c_str());
    return;
  }
  STARTUPINFOW si{};
  si.cb = sizeof(si);
  PROCESS_INFORMATION pi{};
  // CreateProcessW requires a mutable command-line buffer.
  std::wstring cmd = L"\"" + studio.wstring() + L"\"";
  std::vector<wchar_t> cmd_buf(cmd.begin(), cmd.end());
  cmd_buf.push_back(L'\0');
  if (!CreateProcessW(studio.c_str(), cmd_buf.data(), nullptr, nullptr, FALSE, 0,
                      nullptr, nullptr, &si, &pi)) {
    if (log) log("tray: Open Studio failed (CreateProcess error %lu)", GetLastError());
    return;
  }
  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);
  if (log) log("tray: Open Studio fallback direct (%ls) (launcher missing)", studio.c_str());
}

void OpenSupport(TrayLogFn log) {
  const int wlen = MultiByteToWideChar(CP_UTF8, 0, K6WP_DONATE_URL, -1, nullptr, 0);
  if (wlen <= 0) {
    if (log) log("tray: Support open failed (URL conversion error %lu)", GetLastError());
    return;
  }
  std::vector<wchar_t> wurl(static_cast<std::size_t>(wlen));
  if (MultiByteToWideChar(CP_UTF8, 0, K6WP_DONATE_URL, -1, wurl.data(), wlen) <= 0) {
    if (log) log("tray: Support open failed (URL conversion error %lu)", GetLastError());
    return;
  }
  const HINSTANCE rc =
      ShellExecuteW(nullptr, L"open", wurl.data(), nullptr, nullptr, SW_SHOWNORMAL);
  if (reinterpret_cast<INT_PTR>(rc) <= 32) {
    if (log) {
      log("tray: Support open failed (ShellExecute error %lld)",
          static_cast<long long>(reinterpret_cast<INT_PTR>(rc)));
    }
  } else {
    if (log) log("tray: Support the developer opened in browser");
  }
}

}  // namespace k6wp
