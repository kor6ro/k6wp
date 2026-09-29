#include "ffmpeg_path.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <string>
#include <vector>

namespace k6wp {
namespace {

std::filesystem::path ExeDir() {
  try {
    std::vector<wchar_t> buf(32768);
    const DWORD n =
        GetModuleFileNameW(nullptr, buf.data(), static_cast<DWORD>(buf.size()));
    if (n == 0 || n >= buf.size()) return {};
    const std::wstring path(buf.data(), n);
    const auto pos = path.find_last_of(L"\\/");
    if (pos == std::wstring::npos) return {};
    return std::filesystem::path(path.substr(0, pos));
  } catch (...) {
    return {};
  }
}

bool ExistsNoThrow(const std::filesystem::path& p) {
  try {
    std::error_code ec;
    return std::filesystem::exists(p, ec) && !ec;
  } catch (...) {
    return false;
  }
}

std::filesystem::path FindTool(const wchar_t* env_name,
                               const wchar_t* file_name) {
  try {
    // (a) env override, only when it points at an existing file.
    wchar_t env[32768];
    const DWORD len = GetEnvironmentVariableW(env_name, env, 32768);
    if (len > 0 && len < 32768) {
      const std::filesystem::path p(std::wstring(env, len));
      if (ExistsNoThrow(p)) return p;
    }
    const std::filesystem::path exe_dir = ExeDir();
    if (exe_dir.empty()) return {};
    // (b) FLAT — release layout: ffmpeg/ffprobe sit next to the exe.
    const std::filesystem::path flat = exe_dir / file_name;
    if (ExistsNoThrow(flat)) return flat;
    // (c) build layout: exe in build/<preset>/ -> repo root two levels up.
    const std::filesystem::path build =
        exe_dir / L"..\\..\\vendor\\ffmpeg" / file_name;
    if (ExistsNoThrow(build)) return build;
    // (d) vendored subdir next to the exe.
    const std::filesystem::path vendored =
        exe_dir / L"vendor\\ffmpeg" / file_name;
    if (ExistsNoThrow(vendored)) return vendored;
    return {};
  } catch (...) {
    return {};
  }
}

}  // namespace

std::filesystem::path FindFfmpeg() {
  try {
    return FindTool(L"K6WP_FFMPEG", L"ffmpeg.exe");
  } catch (...) {
    return {};
  }
}

std::filesystem::path FindFfprobe() {
  try {
    return FindTool(L"K6WP_FFPROBE", L"ffprobe.exe");
  } catch (...) {
    return {};
  }
}

}  // namespace k6wp
