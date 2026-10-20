#include "still.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <mutex>
#include <system_error>
#include <vector>

#include "config_schema.hpp"   // ConfigError, AtomicWriteJson
#include "settings_io.hpp"     // detail::ReadFile
#include "studio_settings.hpp"

namespace k6wp {

namespace {

constexpr int kOriginalLockscreenSchema = 1;
constexpr int kStatusSchema = 1;

std::filesystem::path LocalAppDataBase() {
  wchar_t buf[MAX_PATH] = {};
  const DWORD local = GetEnvironmentVariableW(L"LOCALAPPDATA", buf, MAX_PATH);
  if (local > 0 && local < MAX_PATH) return std::filesystem::path(buf);
  const DWORD profile = GetEnvironmentVariableW(L"USERPROFILE", buf, MAX_PATH);
  if (profile > 0 && profile < MAX_PATH) {
    return std::filesystem::path(buf) / L"AppData" / L"Local";
  }
  std::error_code ec;
  return std::filesystem::temp_directory_path(ec);
}

void DebugLogW(const wchar_t* text) { OutputDebugStringW(text); }

// mtime + size of a file, used as the settings cache key. Both are 0 when the
// file cannot be stat'ed.
void FileStamp(const std::filesystem::path& path, std::uintmax_t& size,
               std::uintmax_t& mtime) {
  size = 0;
  mtime = 0;
  std::error_code ec;
  const auto st = std::filesystem::status(path, ec);
  if (ec || !std::filesystem::exists(st)) return;
  const auto stamp = std::filesystem::last_write_time(path, ec);
  if (ec) return;
  mtime = static_cast<std::uintmax_t>(stamp.time_since_epoch().count());
  size = std::filesystem::file_size(path, ec);
  if (ec) size = 0;
}

// Cached view of the three settings the Engine may read (LOW-16). The file is
// re-parsed only when its mtime/size change, so one event costs at most ONE
// parse.
struct SettingsCache {
  std::mutex mutex;
  bool valid = false;
  std::uintmax_t size = 0;
  std::uintmax_t mtime = 0;
  StillSettings value;
};

SettingsCache& Cache() {
  static SettingsCache cache;
  return cache;
}

std::filesystem::path SiblingLauncher() {
  wchar_t exe_path[MAX_PATH] = {};
  const DWORD len = GetModuleFileNameW(nullptr, exe_path, MAX_PATH);
  if (len == 0 || len >= MAX_PATH) return {};
  std::error_code ec;
  const std::filesystem::path cand =
      std::filesystem::path(exe_path).parent_path() / L"K6WP.exe";
  if (std::filesystem::exists(cand, ec)) return cand;
  return {};
}

// DETACHED_PROCESS: the helper outlives its parent (the Engine may exit right
// after asking for a refresh) and never inherits a console.
// BELOW_NORMAL_PRIORITY_CLASS: a frame extract must never jank the render
// loop or the Studio GUI.
StillSpawnFn g_spawn_fn = nullptr;

bool SpawnHelperDetached(const std::wstring& extra_args) {
  if (g_spawn_fn != nullptr) return g_spawn_fn(extra_args);
  const std::filesystem::path helper = SiblingLauncher();
  if (helper.empty()) {
    DebugLogW(L"K6WP still: K6WP.exe not found next to this module, skipping\n");
    return false;
  }
  std::wstring cmd = L"\"" + helper.wstring() + L"\"" + extra_args;
  std::vector<wchar_t> cmd_buf(cmd.begin(), cmd.end());
  cmd_buf.push_back(L'\0');
  STARTUPINFOW si{};
  si.cb = sizeof(si);
  PROCESS_INFORMATION pi{};
  if (!CreateProcessW(helper.c_str(), cmd_buf.data(), nullptr, nullptr, FALSE,
                      DETACHED_PROCESS | BELOW_NORMAL_PRIORITY_CLASS, nullptr,
                      nullptr, &si, &pi)) {
    DebugLogW(L"K6WP still: CreateProcess failed, skipping\n");
    return false;
  }
  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);
  return true;
}

std::wstring Utf8ToWide(const std::string& utf8) {
  if (utf8.empty()) return {};
  const int n = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(),
                                    static_cast<int>(utf8.size()), nullptr, 0);
  if (n <= 0) return {};
  std::wstring out(static_cast<std::size_t>(n), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), static_cast<int>(utf8.size()),
                      out.data(), n);
  return out;
}

std::string WideToUtf8(const std::wstring& wide) {
  if (wide.empty()) return {};
  const int n = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(),
                                    static_cast<int>(wide.size()), nullptr, 0,
                                    nullptr, nullptr);
  if (n <= 0) return {};
  std::string out(static_cast<std::size_t>(n), '\0');
  WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), static_cast<int>(wide.size()),
                      out.data(), n, nullptr, nullptr);
  return out;
}

}  // namespace

// --- paths -------------------------------------------------------------------

std::filesystem::path StillDir() {
  std::error_code ec;
  try {
    return LocalAppDataBase() / L"K6WP" / L"stills";
  } catch (...) {
    return std::filesystem::temp_directory_path(ec) / L"K6WP" / L"stills";
  }
}

std::filesystem::path StillStatusPath() {
  std::error_code ec;
  try {
    return LocalAppDataBase() / L"K6WP" / L"still_status.json";
  } catch (...) {
    return std::filesystem::temp_directory_path(ec) / L"K6WP" /
           L"still_status.json";
  }
}

std::filesystem::path OriginalWallpaperStorePath() {
  std::error_code ec;
  try {
    return LocalAppDataBase() / L"K6WP" / L"original_wallpaper.json";
  } catch (...) {
    return std::filesystem::temp_directory_path(ec) / L"K6WP" /
           L"original_wallpaper.json";
  }
}

std::filesystem::path OriginalLockscreenStorePath() {
  std::error_code ec;
  try {
    return LocalAppDataBase() / L"K6WP" / L"original_lockscreen.json";
  } catch (...) {
    return std::filesystem::temp_directory_path(ec) / L"K6WP" /
           L"original_lockscreen.json";
  }
}

std::filesystem::path OriginalLockscreenImagePath() {
  std::error_code ec;
  try {
    return LocalAppDataBase() / L"K6WP" / L"original_lockscreen.jpg";
  } catch (...) {
    return std::filesystem::temp_directory_path(ec) / L"K6WP" /
           L"original_lockscreen.jpg";
  }
}

// --- settings ----------------------------------------------------------------

StillSettings ReadStillSettingsCached() noexcept {
  StillSettings view;
  view.wallpaper_sync = false;
  view.lockscreen_sync = false;
  try {
    const std::filesystem::path path = DefaultStudioSettingsPath();
    std::uintmax_t size = 0;
    std::uintmax_t mtime = 0;
    FileStamp(path, size, mtime);

    {
      std::lock_guard<std::mutex> lock(Cache().mutex);
      if (Cache().valid && Cache().size == size && Cache().mtime == mtime) {
        return Cache().value;
      }
    }

    bool wallpaper = false;
    bool lockscreen = false;
    double offset = 0.0;
    try {
      const StudioSettings s = LoadStudioSettings(path);
      wallpaper = s.wallpaper_sync;
      lockscreen = s.lockscreen_sync;
      offset = s.lockscreen_offset_sec < 0.0 ? 0.0 : s.lockscreen_offset_sec;
      view.readable = true;
    } catch (...) {
      // Missing/corrupt settings: cannot confirm the user's intent, so BOTH
      // sync flags stay off. The file is never rewritten here - Studio owns
      // it and re-enables on the next toggle.
      DebugLogW(L"K6WP still: could not read studio_settings.json; still sync "
                L"disabled (re-enable via Studio to restore)\n");
    }

    view.wallpaper_sync = wallpaper;
    view.lockscreen_sync = lockscreen;
    view.lockscreen_offset_sec = offset;
    {
      std::lock_guard<std::mutex> lock(Cache().mutex);
      // A failed read is cached too: the "file missing" state is stable until
      // the file appears.
      Cache().valid = true;
      Cache().size = size;
      Cache().mtime = mtime;
      Cache().value = view;
    }
    return view;
  } catch (...) {
    return view;
  }
}

// --- pure helpers -------------------------------------------------------------

bool IsStillPath(const std::filesystem::path& path) noexcept {
  try {
    if (path.empty()) return false;
    const std::filesystem::path dir = StillDir();
    if (dir.empty()) return false;
    std::wstring ours = dir.lexically_normal().wstring();
    std::wstring probe = path.lexically_normal().wstring();
    while (!ours.empty() && (ours.back() == L'\\' || ours.back() == L'/')) {
      ours.pop_back();
    }
    while (!probe.empty() && (probe.back() == L'\\' || probe.back() == L'/')) {
      probe.pop_back();
    }
    if (ours.empty() || probe.size() <= ours.size()) return false;
    if (_wcsnicmp(probe.c_str(), ours.c_str(), ours.size()) != 0) return false;
    return probe[ours.size()] == L'\\' || probe[ours.size()] == L'/';
  } catch (...) {
    return false;
  }
}

bool StillRefreshAllowed(bool manual, bool same_file,
                         unsigned long long elapsed_ms,
                         unsigned long long min_interval_ms) noexcept {
  if (manual) return true;
  if (same_file) return false;  // nothing changed: no second render needed
  return elapsed_ms >= min_interval_ms;
}

bool ParseStillRes(const std::wstring& text, int& width, int& height) noexcept {
  width = 0;
  height = 0;
  const std::size_t x = text.find_first_of(L"xX");
  if (x == std::wstring::npos || x == 0 || x + 1 >= text.size()) return false;
  try {
    const long w = std::stol(text.substr(0, x));
    const long h = std::stol(text.substr(x + 1));
    if (w <= 0 || h <= 0) return false;
    width = static_cast<int>(w);
    height = static_cast<int>(h);
    return true;
  } catch (...) {
    return false;
  }
}

// --- original wallpaper store --------------------------------------------------

std::wstring ReadOriginalWallpaper() noexcept {
  try {
    const std::filesystem::path path = OriginalWallpaperStorePath();
    const std::string text = detail::ReadFile(path, "original wallpaper store");
    const nlohmann::json raw = nlohmann::json::parse(text);
    if (!raw.is_object()) return {};
    return Utf8ToWide(raw.value("wallpaper", std::string()));
  } catch (...) {
    return {};
  }
}

bool WriteOriginalWallpaper(const std::wstring& path) noexcept {
  try {
    if (path.empty()) return false;
    std::error_code ec;
    std::filesystem::create_directories(OriginalWallpaperStorePath().parent_path(),
                                        ec);
    nlohmann::json j;
    j["version"] = 1;
    j["wallpaper"] = WideToUtf8(path);
    AtomicWriteJson(OriginalWallpaperStorePath(), j);
    return true;
  } catch (...) {
    return false;
  }
}

bool CaptureOriginalWallpaper() noexcept {
  try {
    wchar_t current[MAX_PATH] = {};
    if (!SystemParametersInfoW(SPI_GETDESKWALLPAPER, MAX_PATH, current, 0) ||
        current[0] == L'\0') {
      return false;
    }
    // J1: our own still must never be recorded as the user's original. The
    // store keeps the value captured before the FIRST still was applied.
    if (IsStillPath(std::filesystem::path(current))) return false;
    return WriteOriginalWallpaper(current);
  } catch (...) {
    return false;
  }
}

bool RestoreOriginalWallpaper() noexcept {
  try {
    const std::wstring stored = ReadOriginalWallpaper();
    if (stored.empty()) return false;
    if (IsStillPath(std::filesystem::path(stored))) {
      // Defensive: a still path in the store would restore our own image.
      std::error_code ec;
      std::filesystem::remove(OriginalWallpaperStorePath(), ec);
      return false;
    }
    if (!SystemParametersInfoW(
            SPI_SETDESKWALLPAPER, 0,
            const_cast<LPWSTR>(stored.c_str()),
            SPIF_UPDATEINIFILE | SPIF_SENDCHANGE)) {
      DebugLogW(L"K6WP still: restoring the original wallpaper failed\n");
      return false;
    }
    std::error_code ec;
    std::filesystem::remove(OriginalWallpaperStorePath(), ec);
    return true;
  } catch (...) {
    return false;
  }
}

// --- original lock-screen store ------------------------------------------------

std::string OriginalLockscreenToJson(const OriginalLockscreen& state) {
  nlohmann::json j;
  j["version"] = kOriginalLockscreenSchema;
  j["have_image"] = state.have_image;
  j["image"] = WideToUtf8(state.image_path);
  j["spotlight_present"] = state.spotlight_present;
  j["spotlight_enabled"] = state.spotlight_enabled;
  j["slideshow_present"] = state.slideshow_present;
  j["slideshow_enabled"] = state.slideshow_enabled;
  return j.dump();
}

bool OriginalLockscreenFromJson(const std::string& text,
                                OriginalLockscreen& out) noexcept {
  try {
    const nlohmann::json raw = nlohmann::json::parse(text);
    if (!raw.is_object()) return false;
    if (raw.value("version", 0) != kOriginalLockscreenSchema) return false;
    OriginalLockscreen state;
    state.have_image = raw.value("have_image", false);
    state.image_path = Utf8ToWide(raw.value("image", std::string()));
    state.spotlight_present = raw.value("spotlight_present", false);
    state.spotlight_enabled = raw.value("spotlight_enabled", 0ul);
    state.slideshow_present = raw.value("slideshow_present", false);
    state.slideshow_enabled = raw.value("slideshow_enabled", 0ul);
    out = state;
    return true;
  } catch (...) {
    return false;
  }
}

bool ReadOriginalLockscreen(OriginalLockscreen& out) noexcept {
  try {
    const std::string text =
        detail::ReadFile(OriginalLockscreenStorePath(), "lock-screen store");
    return OriginalLockscreenFromJson(text, out);
  } catch (...) {
    return false;
  }
}

bool WriteOriginalLockscreen(const OriginalLockscreen& state) noexcept {
  try {
    std::error_code ec;
    std::filesystem::create_directories(
        OriginalLockscreenStorePath().parent_path(), ec);
    AtomicWriteJson(OriginalLockscreenStorePath(),
                    nlohmann::json::parse(OriginalLockscreenToJson(state)));
    return true;
  } catch (...) {
    return false;
  }
}

void ClearOriginalLockscreen() noexcept {
  std::error_code ec;
  std::filesystem::remove(OriginalLockscreenStorePath(), ec);
  std::filesystem::remove(OriginalLockscreenImagePath(), ec);
}

// --- still status ---------------------------------------------------------------

std::string StillStatusToJson(const StillStatus& status) {
  nlohmann::json j;
  j["version"] = kStatusSchema;
  j["ts"] = status.ts;
  j["ok"] = status.ok;
  j["wallpaper_ok"] = status.wallpaper_ok;
  j["lockscreen_ok"] = status.lockscreen_ok;
  j["error"] = status.error;
  j["lockscreen_mode_hint"] = status.lockscreen_mode_hint;
  return j.dump();
}

bool StillStatusFromJson(const std::string& text, StillStatus& out) noexcept {
  try {
    const nlohmann::json raw = nlohmann::json::parse(text);
    if (!raw.is_object()) return false;
    StillStatus status;
    status.ts = raw.value("ts", 0LL);
    status.ok = raw.value("ok", false);
    status.wallpaper_ok = raw.value("wallpaper_ok", false);
    status.lockscreen_ok = raw.value("lockscreen_ok", false);
    status.error = raw.value("error", std::string());
    status.lockscreen_mode_hint = raw.value("lockscreen_mode_hint", std::string());
    out = status;
    return true;
  } catch (...) {
    return false;
  }
}

bool ReadStillStatus(StillStatus& out) noexcept {
  try {
    const std::string text = detail::ReadFile(StillStatusPath(), "still status");
    return StillStatusFromJson(text, out);
  } catch (...) {
    return false;
  }
}

bool WriteStillStatus(const StillStatus& status) noexcept {
  try {
    std::error_code ec;
    std::filesystem::create_directories(StillStatusPath().parent_path(), ec);
    AtomicWriteJson(StillStatusPath(), nlohmann::json::parse(StillStatusToJson(status)));
    return true;
  } catch (...) {
    return false;
  }
}

// --- spawning the helper ---------------------------------------------------------

bool RequestStillApply(const std::filesystem::path& video) noexcept {
  try {
    if (video.empty()) return false;
    const StillSettings settings = ReadStillSettingsCached();
    if (!settings.wallpaper_sync && !settings.lockscreen_sync) return false;
    return SpawnHelperDetached(L" --apply-still --video \"" + video.wstring() +
                               L"\"");
  } catch (...) {
    return false;
  }
}

bool RequestStillRestore(bool wallpaper, bool lockscreen) noexcept {
  try {
    if (!wallpaper && !lockscreen) return false;
    std::wstring args = L" --apply-still --restore";
    if (wallpaper && !lockscreen) args += L" --wallpaper";
    if (lockscreen && !wallpaper) args += L" --lockscreen";
    return SpawnHelperDetached(args);
  } catch (...) {
    return false;
  }
}

void SetStillSpawnFn(StillSpawnFn fn) noexcept { g_spawn_fn = fn; }

}  // namespace k6wp
