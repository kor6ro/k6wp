#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include "autostart.hpp"

#include <filesystem>
#include <vector>

namespace k6wp {
namespace {

constexpr wchar_t kRunSubkey[] =
    L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";

// RAII guard for an open registry key (no raw CloseHandle management).
class RegKeyGuard {
 public:
  explicit RegKeyGuard(HKEY key) : key_(key) {}
  RegKeyGuard(const RegKeyGuard&) = delete;
  RegKeyGuard& operator=(const RegKeyGuard&) = delete;
  ~RegKeyGuard() {
    if (key_ != nullptr) RegCloseKey(key_);
  }
  HKEY get() const { return key_; }

 private:
  HKEY key_;
};

// UTF-16 -> UTF-8 for error strings (registry messages from FormatMessageW).
std::string WideToUtf8(const std::wstring& wide) {
  if (wide.empty()) return {};
  const int needed =
      WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, nullptr, 0, nullptr, nullptr);
  if (needed <= 0) return {};
  std::string out(static_cast<std::size_t>(needed - 1), '\0');
  WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, out.data(), needed, nullptr, nullptr);
  return out;
}

// "op failed (error N): <system message>" — clear enough to show in a dialog.
std::string FormatRegError(const char* op, DWORD code) {
  wchar_t* msg_buf = nullptr;
  const DWORD msg_len = FormatMessageW(
      FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
          FORMAT_MESSAGE_IGNORE_INSERTS,
      nullptr, code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      reinterpret_cast<LPWSTR>(&msg_buf), 0, nullptr);
  std::string out = std::string(op) + " failed (error " + std::to_string(code) + ")";
  if (msg_len > 0 && msg_buf != nullptr) {
    // Trim trailing CR/LF that FormatMessage appends.
    std::wstring msg(msg_buf, msg_len);
    while (!msg.empty() && (msg.back() == L'\r' || msg.back() == L'\n' || msg.back() == L' ')) {
      msg.pop_back();
    }
    out += ": " + WideToUtf8(msg);
  }
  if (msg_buf != nullptr) LocalFree(msg_buf);
  return out;
}

void SetError(std::string* error_out, const std::string& msg) {
  if (error_out != nullptr) *error_out = msg;
}

}  // namespace

std::wstring AutostartCommand() {
  wchar_t exe_path[MAX_PATH] = {};
  const DWORD len = GetModuleFileNameW(nullptr, exe_path, MAX_PATH);
  std::filesystem::path exe =
      (len == 0 || len >= MAX_PATH)
          ? std::filesystem::path()
          : std::filesystem::path(exe_path);
  // Determine the target exe for the Run value.
  // Priority 1: if the caller itself is K6WP.exe, use it.
  // Priority 2: otherwise, prefer <exe_dir>/K6WP.exe when it exists.
  // Priority 3: legacy fall-back to sibling engine.exe + --minimized.
  // Never throws (error_code overloads on filesystem ops).
  bool legacy_minimized = false;
  if (!exe.empty()) {
    std::error_code ec;
    std::wstring file = exe.filename().wstring();
    // Normalize to lower for comparison.
    for (auto& ch : file) ch = static_cast<wchar_t>(towlower(ch));

    if (file == L"k6wp.exe") {
      // Caller is K6WP.exe itself — use its own path, silent + engine flag.
      // exe is already the caller's path; keep it.
    } else {
      // Caller is something else (e.g. studio.exe). Prefer K6WP.exe sibling.
      const std::filesystem::path k6wp_sibling = exe.parent_path() / L"K6WP.exe";
      if (std::filesystem::exists(k6wp_sibling, ec)) {
        exe = k6wp_sibling;
      } else {
        // Legacy fall-back: sibling engine.exe + --minimized.
        const std::filesystem::path engine_sibling = exe.parent_path() / L"engine.exe";
        if (std::filesystem::exists(engine_sibling, ec)) {
          exe = engine_sibling;
          legacy_minimized = true;
        }
      }
    }
  }

  // Build command line based on chosen target.
  // K6WP.exe  → "--engine --silent" (ensure engine only, open nothing; the
  //             launcher maps --silent to skip Studio and forwards --minimized
  //             to the engine tray-only marker)
  // engine.exe → "--minimized" (legacy; engine.exe ParseCli does not know --engine/--silent)
  if (legacy_minimized) {
    return L"\"" + exe.wstring() + L"\" --minimized";
  }
  return L"\"" + exe.wstring() + L"\" --engine --silent";
}

bool IsAutostart() noexcept {
  HKEY key = nullptr;
  if (RegOpenKeyExW(HKEY_CURRENT_USER, kRunSubkey, 0, KEY_QUERY_VALUE, &key) !=
      ERROR_SUCCESS) {
    return false;
  }
  RegKeyGuard guard(key);
  return RegQueryValueExW(key, kAutostartValueName, nullptr, nullptr, nullptr,
                          nullptr) == ERROR_SUCCESS;
}

bool SetAutostart(bool enable, std::string* error_out) {
  if (enable) {
    HKEY key = nullptr;
    DWORD disposition = 0;
    const LSTATUS create_rc = RegCreateKeyExW(
        HKEY_CURRENT_USER, kRunSubkey, 0, nullptr, REG_OPTION_NON_VOLATILE,
        KEY_SET_VALUE, nullptr, &key, &disposition);
    if (create_rc != ERROR_SUCCESS) {
      SetError(error_out, FormatRegError("Cannot enable autostart: RegCreateKeyExW "
                                         "on HKCU\\...\\Run",
                                         static_cast<DWORD>(create_rc)));
      return false;
    }
    RegKeyGuard guard(key);
    const std::wstring command = AutostartCommand();
    const LSTATUS set_rc = RegSetValueExW(
        key, kAutostartValueName, 0, REG_SZ,
        reinterpret_cast<const BYTE*>(command.c_str()),
        static_cast<DWORD>((command.size() + 1) * sizeof(wchar_t)));
    if (set_rc != ERROR_SUCCESS) {
      SetError(error_out,
               FormatRegError("Cannot enable autostart: RegSetValueExW K6WP",
                              static_cast<DWORD>(set_rc)));
      return false;
    }
    (void)disposition;
    return true;
  }
  HKEY key = nullptr;
  const LSTATUS open_rc = RegOpenKeyExW(HKEY_CURRENT_USER, kRunSubkey, 0,
                                        KEY_SET_VALUE, &key);
  if (open_rc == ERROR_FILE_NOT_FOUND) return true;  // already OFF
  if (open_rc != ERROR_SUCCESS) {
    SetError(error_out, FormatRegError("Cannot disable autostart: RegOpenKeyExW "
                                       "on HKCU\\...\\Run",
                                       static_cast<DWORD>(open_rc)));
    return false;
  }
  RegKeyGuard guard(key);
  const LSTATUS del_rc = RegDeleteValueW(key, kAutostartValueName);
  if (del_rc != ERROR_SUCCESS && del_rc != ERROR_FILE_NOT_FOUND) {
    SetError(error_out,
             FormatRegError("Cannot disable autostart: RegDeleteValueW K6WP",
                            static_cast<DWORD>(del_rc)));
    return false;
  }
  return true;
}

}  // namespace k6wp
