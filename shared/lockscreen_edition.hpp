#pragma once

// Windows-edition support for the lock-screen image policy.
//
// WHY
// The lock-screen picture is applied through the HKLM policy
// (SOFTWARE\Policies\Microsoft\Windows\Personalization\LockScreenImage).
// Microsoft documents that policy as supported on Enterprise and Education;
// for Pro the Group Policy table marks it as NOT supported (the Intune /
// Configuration Service Provider path is), and Home does not support it at
// all. There are community reports that the policy sometimes works on Pro
// anyway, and that the PersonalizationCSP path
// (HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\PersonalizationCSP ->
// LockScreenImagePath / LockScreenImageUrl / LockScreenImageStatus) is
// honoured there - but NEITHER is verified in this repository.
//
// So the Studio must not promise more than it can deliver: it reports what
// is actually true - is the policy registered, and does this Windows
// edition officially support it.
//
// Header stays windows.h-free (the Win32/registry calls live in the .cpp).

#include <string>

namespace k6wp {

// How well the current Windows edition supports the lock-screen policy.
enum class LockscreenEditionSupport {
  // Enterprise / Education: the policy is officially supported.
  kSupported,
  // Pro: the policy is NOT officially supported (the Intune/CSP path is).
  // It may still work on some builds - the Studio says exactly that.
  kUncertain,
  // Home: not supported.
  kUnsupported,
  // The edition could not be read (no registry, an unexpected value).
  kUnknown,
};

// The live state of the lock-screen policy, as the Studio shows it.
struct LockscreenPolicyStatus {
  LockscreenEditionSupport support = LockscreenEditionSupport::kUnknown;
  // The edition id as Windows reports it ("Professional", "Enterprise", ...).
  std::string edition_id;
  // The human-readable product name ("Windows 10 Pro", ...). May be empty.
  std::string product_name;
  // True when the HKLM LockScreenImage value is present AND points at
  // %PROGRAMDATA%\K6WP\lockscreen.jpg - i.e. the policy really points at
  // our frame.
  bool policy_registered = false;
  // True when the policy value exists but points somewhere else (the user's
  // own wallpaper - the launcher refuses to overwrite it).
  bool policy_foreign = false;
};

// Pure classification of a Windows edition id. Empty / unknown ids map to
// kUnknown. Unit-tested.
LockscreenEditionSupport ClassifyLockscreenEdition(
    const std::string& edition_id);

// Reads the live status: the Windows edition (registry) and whether the
// lock-screen policy is registered. Never throws.
LockscreenPolicyStatus ReadLockscreenPolicyStatus();

}  // namespace k6wp
