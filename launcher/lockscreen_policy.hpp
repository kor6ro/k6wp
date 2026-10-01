#pragma once

#include <filesystem>
#include <string>

namespace k6wp::launcher {

inline constexpr wchar_t kLockscreenFileName[] = L"lockscreen.jpg";
inline constexpr wchar_t kLockscreenBackupName[] = L"lockscreen_policy_backup.json";

// Formats a Win32 error code as "error <n>: <system text>".
std::wstring FormatSysError(unsigned long code);

bool IsElevated();

// %PROGRAMDATA%\K6WP (empty path when PROGRAMDATA is unset).
std::filesystem::path ProgramDataK6wpDir();

// HKLM LockScreenImage policy (Part B, static image only).
bool ReadLockscreenPolicy(std::wstring& value_out, bool& present);
bool WriteLockscreenPolicy(const std::wstring& image_path);
bool DeleteLockscreenPolicy();

}  // namespace k6wp::launcher
