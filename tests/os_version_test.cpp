// tests/os_version_test.cpp - C6: the minimum-OS rule.
//
// The product decision is "Windows 10 22H2 (build 19041) or Windows 11,
// x64". The decision was NEVER implemented until now: the contract inventory
// found no version check anywhere in the source, so an unsupported machine
// installed the app and then failed in obscure ways.
//
// The decision function is PURE (a version in, a verdict out), so it can be
// tested for every boundary without a machine of that vintage.
//
// No external test framework: plain checks with a pass/fail counter, the
// same style as the other tests in this repo. Exit 0 = pass.
#include <cstdio>
#include <string>

#include "os_version.hpp"

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

k6wp::OsVersion V(unsigned long major, unsigned long minor,
                  unsigned long build) {
  k6wp::OsVersion v;
  v.major = major;
  v.minor = minor;
  v.build = build;
  return v;
}

}  // namespace

int main() {
  using k6wp::IsSupportedOsVersion;
  using k6wp::OsVersion;

  // --- the boundary ---------------------------------------------------------
  Check(!IsSupportedOsVersion(V(10, 0, 19040)),
        "os: build 19040 (Win10 21H2) is too old");
  Check(IsSupportedOsVersion(V(10, 0, 19041)),
        "os: build 19041 (Win10 22H2) is supported");
  Check(IsSupportedOsVersion(V(10, 0, 19045)),
        "os: build 19045 (Win10 22H2) is supported");
  Check(IsSupportedOsVersion(V(10, 0, 22000)),
        "os: build 22000 (Win11 21H2) is supported");
  Check(IsSupportedOsVersion(V(10, 0, 26100)),
        "os: build 26100 (Win11 24H2) is supported");
  Check(IsSupportedOsVersion(V(11, 0, 26100)),
        "os: a reported major 11 is supported");

  // --- older Windows -------------------------------------------------------
  Check(!IsSupportedOsVersion(V(6, 1, 7601)),
        "os: Windows 7 SP1 is too old");
  Check(!IsSupportedOsVersion(V(6, 2, 9200)),
        "os: Windows 8 is too old");
  Check(!IsSupportedOsVersion(V(6, 3, 9600)),
        "os: Windows 8.1 is too old");
  Check(!IsSupportedOsVersion(V(10, 0, 15063)),
        "os: Windows 10 1507 is too old");
  Check(!IsSupportedOsVersion(V(10, 0, 17763)),
        "os: Windows 10 1809 is too old");
  Check(!IsSupportedOsVersion(V(10, 0, 18362)),
        "os: Windows 10 1903 is too old");
  Check(!IsSupportedOsVersion(V(10, 0, 18999)),
        "os: Windows 10 1909 is too old");

  // --- an unreadable version is NOT supported ------------------------------
  // A version we could not read is not evidence of a supported OS.
  Check(!IsSupportedOsVersion(V(0, 0, 0)),
        "os: an undetectable version is not supported");

  // --- the message ---------------------------------------------------------
  const std::string message = k6wp::MinOsErrorMessage(V(10, 0, 19041));
  Check(message.find("19041") != std::string::npos,
        "os: the error message states the required build");
  Check(message.find("Windows 10 22H2") != std::string::npos,
        "os: the error message names the requirement");
  Check(message.find("10.0") != std::string::npos,
        "os: the error message reports the detected version");
  Check(k6wp::MinOsRequirementText() != nullptr &&
            std::string(k6wp::MinOsRequirementText()).find("19041") !=
                std::string::npos,
        "os: the requirement text states the minimum build");

  // The exit code is distinct from the flag error (2), so a script can tell
  // "bad arguments" from "unsupported OS".
  Check(k6wp::kExitUnsupportedOs != 2,
        "os: the unsupported-OS exit code differs from the flag-error code");

  // --- the live probe -----------------------------------------------------
  // The pure rule above is the decision; this is the READ that feeds it. On
  // a supported machine the detected version must satisfy the rule, and the
  // version must be readable at all (an unreadable version would silently
  // disable the product).
  const OsVersion live = k6wp::GetOsVersion();
  std::printf("os: detected %lu.%lu build %lu\n", live.major, live.minor,
              live.build);
  Check(live.major != 0 || live.minor != 0 || live.build != 0,
        "os: the live version probe returns a version");
  Check(IsSupportedOsVersion(live) == (live.build >= k6wp::kMinSupportedBuild),
        "os: the live version agrees with the pure rule");

  std::printf("os_version_test: %d checks, %d failures\n", g_checks,
              g_failures);
  return g_failures == 0 ? 0 : 1;
}
