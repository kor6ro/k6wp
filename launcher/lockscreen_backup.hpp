#pragma once

#include <filesystem>
#include <string>

namespace k6wp::launcher {

// JSON escape for the backup value: whole-string UTF-8 first (so surrogate
// pairs survive), then JSON-escape only what JSON requires; bytes >= 0x80
// pass through raw.
std::string EscapeBackupJson(const std::wstring& wide);

// Crash-safe publish of the lockscreen-policy backup (sibling .tmp then
// atomic replace). Returns false on any failure.
bool WriteLockscreenBackup(const std::filesystem::path& backup_path,
                           bool had_value, const std::wstring& value);

// Decodes the backup (\\, \", \uXXXX with surrogate pairs) back to a wide
// string. Returns false only when the file cannot be read.
bool ReadLockscreenBackup(const std::filesystem::path& backup_path,
                          bool& had_value, std::wstring& value);

}  // namespace k6wp::launcher
