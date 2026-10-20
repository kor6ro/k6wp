#include "lockscreen_edition.hpp"

#include "lockscreen.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <string>

namespace k6wp {

namespace {

constexpr wchar_t kCurrentVersionKey[] =
    L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion";
constexpr wchar_t kPolicySubkey[] =
    L"SOFTWARE\\Policies\\Microsoft\\Windows\\Personalization";
constexpr wchar_t kPolicyValueName[] = L"LockScreenImage";

std::string ReadStringValue(HKEY root, const wchar_t* subkey,
                             const wchar_t* value_name) {
  HKEY key = nullptr;
  if (RegOpenKeyExW(root, subkey, 0, KEY_QUERY_VALUE, &key) != ERROR_SUCCESS) {
    return {};
  }
  DWORD type = 0;
  DWORD size = 0;
  std::string out;
  if (RegQueryValueExW(key, value_name, nullptr, &type, nullptr, &size) ==
          ERROR_SUCCESS &&
      type == REG_SZ && size > 0) {
    std::wstring buf(size / sizeof(wchar_t) + 1, L'\0');
    if (RegQueryValueExW(key, value_name, nullptr, &type,
                         reinterpret_cast<LPBYTE>(buf.data()),
                         &size) == ERROR_SUCCESS) {
      while (!buf.empty() && buf.back() == L'\0') buf.pop_back();
      const int n = WideCharToMultiByte(CP_UTF8, 0, buf.c_str(),
                                        static_cast<int>(buf.size()),
                                        nullptr, 0, nullptr, nullptr);
      if (n > 0) {
        out.resize(static_cast<std::size_t>(n));
        WideCharToMultiByte(CP_UTF8, 0, buf.c_str(),
                            static_cast<int>(buf.size()), out.data(), n,
                            nullptr, nullptr);
      }
    }
  }
  RegCloseKey(key);
  return out;
}

std::wstring ReadWideValue(HKEY root, const wchar_t* subkey,
                            const wchar_t* value_name) {
  HKEY key = nullptr;
  if (RegOpenKeyExW(root, subkey, 0, KEY_QUERY_VALUE, &key) != ERROR_SUCCESS) {
    return {};
  }
  DWORD type = 0;
  DWORD size = 0;
  std::wstring out;
  if (RegQueryValueExW(key, value_name, nullptr, &type, nullptr, &size) ==
          ERROR_SUCCESS &&
      type == REG_SZ && size > 0) {
    out.resize(size / sizeof(wchar_t));
    RegQueryValueExW(key, value_name, nullptr, &type,
                     reinterpret_cast<LPBYTE>(out.data()), &size);
    while (!out.empty() && out.back() == L'\0') out.pop_back();
  }
  RegCloseKey(key);
  return out;
}

}  // namespace

LockscreenEditionSupport ClassifyLockscreenEdition(
    const std::string& edition_id) {
  if (edition_id.empty()) return LockscreenEditionSupport::kUnknown;
  // Enterprise / Education: the Group Policy is officially supported.
  if (edition_id.rfind("Enterprise", 0) == 0) {
    return LockscreenEditionSupport::kSupported;
  }
  if (edition_id.rfind("Education", 0) == 0) {
    return LockscreenEditionSupport::kSupported;
  }
  if (edition_id == "IoTEnterprise" || edition_id == "Server") {
    return LockscreenEditionSupport::kSupported;
  }
  // Pro: not officially supported (Intune/CSP is), but it may work.
  if (edition_id.rfind("Professional", 0) == 0) {
    return LockscreenEditionSupport::kUncertain;
  }
  // Home / Core: not supported.
  if (edition_id.rfind("Home", 0) == 0 || edition_id.rfind("Core", 0) == 0) {
    return LockscreenEditionSupport::kUnsupported;
  }
  return LockscreenEditionSupport::kUnknown;
}

LockscreenPolicyStatus ReadLockscreenPolicyStatus() {
  LockscreenPolicyStatus status;
  status.edition_id =
      ReadStringValue(HKEY_LOCAL_MACHINE, kCurrentVersionKey, L"EditionID");
  status.product_name = ReadStringValue(HKEY_LOCAL_MACHINE,
                                         kCurrentVersionKey, L"ProductName");
  status.support = ClassifyLockscreenEdition(status.edition_id);

  const std::wstring policy = ReadWideValue(HKEY_LOCAL_MACHINE, kPolicySubkey,
                                             kPolicyValueName);
  if (!policy.empty()) {
    const std::wstring ours = LockscreenJpgPath().wstring();
    if (_wcsicmp(policy.c_str(), ours.c_str()) == 0) {
      status.policy_registered = true;
    } else {
      status.policy_foreign = true;
    }
  }
  return status;
}

}  // namespace k6wp
