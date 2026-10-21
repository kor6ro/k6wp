#pragma once

// Minimum-OS check (C6).
//
// The product decision is: Windows 10 22H2 (build 19041) or Windows 11, x64
// only. That decision was NEVER implemented - the contract inventory found no
// version check anywhere in the source, so an unsupported machine installed
// the app and then failed in obscure ways.
//
// The check lives in shared/ so the ENGINE, the LAUNCHER (K6WP.exe) and the
// STUDIO all apply the SAME rule and print the SAME message.
//
// The version is read with RtlGetVersion (ntdll), not GetVersionEx: on a
// manifest-less process GetVersionEx is subject to the "version lie", and
// RtlGetVersion always reports the real build. The manifest also declares
// supportedOS (see engine/app.manifest.in), but the manifest only influences
// the shim layer - it does NOT refuse to run.
//
// The header is windows.h-free (plain ints and a string), so the test can use
// it without pulling in windows.h.

#include <string>

namespace k6wp {

// The running OS version. build is the NT build number (19041 = Win10 22H2,
// 22000 = Win11 21H2, 26100 = Win11 24H2).
struct OsVersion {
  unsigned long major = 0;
  unsigned long minor = 0;
  unsigned long build = 0;
};

// The minimum supported build: Windows 10 22H2.
inline constexpr unsigned long kMinSupportedBuild = 19041;

// Reads the real OS version. Never throws; on an unexpected failure it
// returns {0,0,0}, which the support check treats as UNSUPPORTED (a build we
// cannot read is a build we must not run on).
OsVersion GetOsVersion();

// The support decision, as a PURE function so it can be unit-tested:
// a build >= kMinSupportedBuild is supported, anything older is not.
// A version that could not be read (all zeroes) is NOT supported.
bool IsSupportedOsVersion(const OsVersion& version);

// The human-readable requirement, e.g.
// "Windows 10 22H2 (build 19041) or Windows 11, 64-bit".
const char* MinOsRequirementText();

// The full error message shown to the user, including what was detected and
// what to do about it. Never null.
std::string MinOsErrorMessage(const OsVersion& version);

// The same message as UTF-16, for the Win32 MessageBox path. It is a
// conversion of the message above, so the two can never drift apart.
std::wstring MinOsErrorMessageWide(const OsVersion& version);

// The exit code used when the OS is too old. Distinct from the flag error
// (2) so a launcher/script can tell "bad arguments" from "unsupported OS".
inline constexpr int kExitUnsupportedOs = 6;

}  // namespace k6wp
