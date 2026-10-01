#pragma once

#include <QString>

#include <filesystem>

namespace k6wp {

// The opt-in marker pickCacheDir() drops in a folder the user chose. Its
// presence IS the opt-in: clearCache() refuses any folder outside the K6WP
// data root that does not carry it.
inline constexpr wchar_t kCacheOptInMarker[] = L".k6wp-cache";

// Drops the opt-in marker in a folder the user just picked, which is what lets
// clearCache() sweep it. Idempotent; false means the folder is not opted in.
bool MarkCacheDirOptedIn(const std::filesystem::path& dir);

// Whether clearCache() may sweep `configured`: not a volume root, not a
// protected/user folder (checked on the configured path AND the resolved
// target), and either inside the K6WP data root (outside the non-cache data
// dirs) or opted in / proven to hold no regular file. `refusal` receives the
// user-facing reason when it returns false.
bool CacheDirIsClearable(const std::filesystem::path& configured,
                         const std::filesystem::path& data_root,
                         QString* refusal);

}  // namespace k6wp
