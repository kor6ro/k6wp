#include "process_paths.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <filesystem>

namespace k6wp::launcher {

bool IsK6wpAppImageName(const std::wstring& base_name) {
  return _wcsicmp(base_name.c_str(), kEngineExe) == 0 ||
         _wcsicmp(base_name.c_str(), kStudioExe) == 0 ||
         _wcsicmp(base_name.c_str(), kLauncherExe) == 0;
}

std::wstring BaseNameText(const std::wstring& full_image) {
  const std::size_t slash = full_image.find_last_of(L"\\/");
  return (slash == std::wstring::npos) ? full_image
                                       : full_image.substr(slash + 1);
}

std::wstring ParentDirText(const std::wstring& full_image) {
  const std::size_t slash = full_image.find_last_of(L"\\/");
  if (slash == std::wstring::npos) return std::wstring();
  return full_image.substr(0, slash);
}

std::wstring TrimTrailingSeparators(std::wstring dir) {
  while (dir.size() > 1 && (dir.back() == L'\\' || dir.back() == L'/')) {
    if (dir[dir.size() - 2] == L':') break;  // keep a drive root as-is
    dir.pop_back();
  }
  return dir;
}

// The scope test that replaces the image-name kill: a K6WP image name is only
// OURS when it sits directly in own_dir. Compared component-wise (not by
// prefix), so "K6WP-old" never matches "K6WP", and case-insensitively, because
// Windows paths are.
bool IsK6wpOwnImage(const std::wstring& full_image,
                    const std::wstring& own_dir) {
  if (full_image.empty() || own_dir.empty()) return false;
  if (!IsK6wpAppImageName(BaseNameText(full_image))) return false;
  return _wcsicmp(TrimTrailingSeparators(ParentDirText(full_image)).c_str(),
                  TrimTrailingSeparators(own_dir).c_str()) == 0;
}

std::wstring OwnDir() {
  wchar_t exe_path[MAX_PATH] = {};
  const DWORD len = GetModuleFileNameW(nullptr, exe_path, MAX_PATH);
  if (len == 0 || len >= MAX_PATH) return std::wstring();
  return std::filesystem::path(exe_path).parent_path().wstring();
}

std::string NarrowAscii(const wchar_t* text) {
  std::string out;
  if (text == nullptr) return out;
  for (const wchar_t* p = text; *p != 0 && *p < 0x80; ++p) {
    out.push_back(static_cast<char>(*p));
  }
  return out;
}

}  // namespace k6wp::launcher
