#pragma once

#include <QString>

#include <filesystem>
#include <string>

#include "library_manager.hpp"

namespace k6wp {

// Case-insensitive substring match over the entry name and label.
bool FilterMatches(const std::string& name, const std::string& label,
                   const std::string& query);

// Grid label (filename, resolution, duration/badges). Ported verbatim from the
// Widgets grid so the grid text is identical.
std::string EntryLabel(const LibraryEntry& entry);

// file:// URL for a local path (empty for an empty path).
QString FileUrl(const std::filesystem::path& p);

}  // namespace k6wp
