#include "os_version.hpp"

#include <windows.h>

namespace k6wp {
namespace {

// RtlGetVersion lives in ntdll. It is the only version API that is immune to
// the "version lie" the application-compatibility shims apply to
// GetVersionEx, so it is the one that answers "what is really running".
using RtlGetVersionFn = LONG(WINAPI*)(PRTL_OSVERSIONINFOW);

RtlGetVersionFn ResolveRtlGetVersion() {
  HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
  if (ntdll == nullptr) return nullptr;
  return reinterpret_cast<RtlGetVersionFn>(
      reinterpret_cast<void*>(GetProcAddress(ntdll, "RtlGetVersion")));
}

}  // namespace

OsVersion GetOsVersion() {
  OsVersion version;
  const RtlGetVersionFn rtl = ResolveRtlGetVersion();
  if (rtl == nullptr) return version;
  RTL_OSVERSIONINFOW info = {};
  info.dwOSVersionInfoSize = sizeof(info);
  if (rtl(&info) != 0) return version;
  version.major = info.dwMajorVersion;
  version.minor = info.dwMinorVersion;
  version.build = info.dwBuildNumber;
  return version;
}

bool IsSupportedOsVersion(const OsVersion& version) {
  // A version we could not read is a version we must not run on: an
  // unreadable build number is not evidence of a supported OS.
  if (version.major == 0 && version.minor == 0 && version.build == 0) {
    return false;
  }
  return version.build >= kMinSupportedBuild;
}

const char* MinOsRequirementText() {
  return "Windows 10 22H2 (build 19041) or Windows 11, 64-bit";
}

std::string MinOsErrorMessage(const OsVersion& version) {
  std::string message = "K6WP requires ";
  message += MinOsRequirementText();
  message += ".\n\n";
  if (version.major == 0 && version.minor == 0 && version.build == 0) {
    message +=
        "This system's Windows version could not be detected, so K6WP will "
        "not start.\n";
  } else {
    message += "Detected: Windows ";
    message += std::to_string(version.major);
    message += ".";
    message += std::to_string(version.minor);
    message += " (build ";
    message += std::to_string(version.build);
    message += ").\n";
  }
  message +=
      "\nUpdate Windows to a supported version, then run K6WP again. Your "
      "videos and settings are not modified.";
  return message;
}

std::wstring MinOsErrorMessageWide(const OsVersion& version) {
  const std::string narrow = MinOsErrorMessage(version);
  const int needed = MultiByteToWideChar(CP_UTF8, 0, narrow.c_str(),
                                          static_cast<int>(narrow.size()),
                                          nullptr, 0);
  if (needed <= 0) return std::wstring();
  std::wstring wide(static_cast<size_t>(needed), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, narrow.c_str(),
                      static_cast<int>(narrow.size()), wide.data(), needed);
  return wide;
}

}  // namespace k6wp
