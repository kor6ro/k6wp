#pragma once

#include <filesystem>
#include <string>

namespace k6wp::detail {

// Reads a whole settings file, rejecting oversize files before reading
// (kMaxConfigBytes) and throwing ConfigError when it cannot be opened. `what`
// names the file for the error text ("config file" / "studio settings file" /
// "playlist file") so each caller keeps its existing message.
std::string ReadFile(const std::filesystem::path& path, const char* what);

// Copies the file to "<file>.bak" (filename += ".bak", so wide/non-UTF8 paths
// survive). Best-effort: never throws, so a failed backup cannot turn a clean
// load into an error.
void BackupFile(const std::filesystem::path& path) noexcept;

}  // namespace k6wp::detail
