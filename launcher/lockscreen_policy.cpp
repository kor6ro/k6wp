#include "lockscreen_policy.hpp"

#include "win32_raii.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace k6wp::launcher {

namespace {

constexpr wchar_t kPolicySubkey[] =
    L"SOFTWARE\\Policies\\Microsoft\\Windows\\Personalization";
constexpr wchar_t kPolicyValueName[] = L"LockScreenImage";

}  // namespace

std::wstring FormatSysError(unsigned long code) {
  wchar_t* msg_buf = nullptr;
  const DWORD len = FormatMessageW(
      FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
          FORMAT_MESSAGE_IGNORE_INSERTS,
      nullptr, code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      reinterpret_cast<LPWSTR>(&msg_buf), 0, nullptr);
  std::wstring out = L"error " + std::to_wstring(code);
  if (len > 0 && msg_buf != nullptr) {
    std::wstring msg(msg_buf, len);
    while (!msg.empty() &&
           (msg.back() == L'\r' || msg.back() == L'\n' || msg.back() == L' ')) {
      msg.pop_back();
    }
    out += L": " + msg;
  }
  if (msg_buf != nullptr) LocalFree(msg_buf);
  return out;
}

bool IsElevated() {
  HANDLE token = nullptr;
  if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
    return false;
  }
  HandleGuard guard(token);
  TOKEN_ELEVATION elevation{};
  DWORD ret = 0;
  if (!GetTokenInformation(guard.get(), TokenElevation, &elevation,
                           sizeof(elevation), &ret)) {
    return false;
  }
  return elevation.TokenIsElevated != 0;
}

std::filesystem::path ProgramDataK6wpDir() {
  wchar_t buf[MAX_PATH] = {};
  const DWORD n = GetEnvironmentVariableW(L"PROGRAMDATA", buf, MAX_PATH);
  if (n > 0 && n < MAX_PATH) {
    return std::filesystem::path(buf) / L"K6WP";
  }
  return std::filesystem::path();
}

bool ReadLockscreenPolicy(std::wstring& value_out, bool& present) {
  present = false;
  HKEY key = nullptr;
  const LSTATUS open_rc = RegOpenKeyExW(HKEY_LOCAL_MACHINE, kPolicySubkey, 0,
                                        KEY_QUERY_VALUE, &key);
  if (open_rc == ERROR_FILE_NOT_FOUND) return true;
  if (open_rc != ERROR_SUCCESS) return false;
  wchar_t buf[32768] = {};
  DWORD size = sizeof(buf);
  DWORD type = 0;
  const LSTATUS q =
      RegQueryValueExW(key, kPolicyValueName, nullptr, &type,
                       reinterpret_cast<LPBYTE>(buf), &size);
  RegCloseKey(key);
  if (q == ERROR_FILE_NOT_FOUND) return true;
  if (q != ERROR_SUCCESS || type != REG_SZ) return false;
  value_out.assign(buf, size / sizeof(wchar_t));
  while (!value_out.empty() && value_out.back() == L'\0') value_out.pop_back();
  present = true;
  return true;
}

bool WriteLockscreenPolicy(const std::wstring& image_path) {
  HKEY key = nullptr;
  const LSTATUS create_rc = RegCreateKeyExW(
      HKEY_LOCAL_MACHINE, kPolicySubkey, 0, nullptr, REG_OPTION_NON_VOLATILE,
      KEY_SET_VALUE, nullptr, &key, nullptr);
  if (create_rc != ERROR_SUCCESS) return false;
  const LSTATUS s = RegSetValueExW(key, kPolicyValueName, 0, REG_SZ,
                        reinterpret_cast<const BYTE*>(image_path.c_str()),
                        static_cast<DWORD>((image_path.size() + 1) *
                                           sizeof(wchar_t)));
  RegCloseKey(key);
  return s == ERROR_SUCCESS;
}

bool DeleteLockscreenPolicy() {
  HKEY key = nullptr;
  const LSTATUS open_rc = RegOpenKeyExW(HKEY_LOCAL_MACHINE, kPolicySubkey, 0,
                                        KEY_SET_VALUE, &key);
  if (open_rc == ERROR_FILE_NOT_FOUND) return true;
  if (open_rc != ERROR_SUCCESS) return false;
  const LSTATUS del = RegDeleteValueW(key, kPolicyValueName);
  RegCloseKey(key);
  return del == ERROR_SUCCESS || del == ERROR_FILE_NOT_FOUND;
}

}  // namespace k6wp::launcher
