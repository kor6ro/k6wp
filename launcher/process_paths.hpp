#pragma once

#include <string>

namespace k6wp::launcher {

inline constexpr wchar_t kEngineExe[] = L"engine.exe";
inline constexpr wchar_t kStudioExe[] = L"studio.exe";
inline constexpr wchar_t kLauncherExe[] = L"K6WP.exe";

// True when a bare image name is one of ours (engine.exe/studio.exe/K6WP.exe),
// case-insensitively.
bool IsK6wpAppImageName(const std::wstring& base_name);

std::wstring BaseNameText(const std::wstring& full_image);
std::wstring ParentDirText(const std::wstring& full_image);
std::wstring TrimTrailingSeparators(std::wstring dir);

// Scope test that replaces the image-name kill: a K6WP image is ours only when
// it sits directly in own_dir (component-wise, case-insensitive).
bool IsK6wpOwnImage(const std::wstring& full_image,
                    const std::wstring& own_dir);

// Directory of the running K6WP.exe (empty when it cannot be resolved).
std::wstring OwnDir();

// Truncates at the first non-ASCII char (NDJSON command names are ASCII).
std::string NarrowAscii(const wchar_t* text);

}  // namespace k6wp::launcher
