#include "still_apply.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

// C++/WinRT: header-only, Windows SDK only (no new third-party dependency).
// This translation unit is linked ONLY into the launcher/helper, never into
// engine.exe (tests/engine_no_winrt_test.cpp proves it).
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.System.UserProfile.h>
#include <winrt/base.h>

#include <algorithm>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <string>
#include <vector>

#include "exit_codes.hpp"
#include "still.hpp"
#include "win32_raii.hpp"

namespace k6wp::launcher {

namespace {

using winrt::Windows::Storage::StorageFile;
using winrt::Windows::Storage::Streams::DataReader;
using winrt::Windows::Storage::Streams::InputStreamOptions;
using winrt::Windows::Storage::Streams::IRandomAccessStream;
using winrt::Windows::System::UserProfile::LockScreen;

// A frame extract is sub-second on this hardware; 60 s covers a cold ffmpeg on
// a slow disk while still bounding a hung child. The helper owns the child, so
// a timeout terminates it (the engine never waits at all).
constexpr DWORD kFrameExtractTimeoutMs = 60000;
// One helper at a time: the engine (rotation) and Studio (toggle) may ask at
// the same moment. Serialising keeps the unique still filenames, the original
// stores and still_status.json consistent. Bounded so a stuck helper can never
// wedge the next request forever.
constexpr DWORD kHelperMutexWaitMs = 120000;
constexpr std::size_t kStillsToKeep = 2;

constexpr wchar_t kContentDeliveryKey[] =
    L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager";
constexpr wchar_t kRotatingValue[] = L"RotatingLockScreenEnabled";
constexpr wchar_t kLockScreenKey[] =
    L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Lock Screen";
constexpr wchar_t kSlideshowValue[] = L"SlideshowEnabled";

std::filesystem::path SiblingCompressor() {
  wchar_t exe_path[MAX_PATH] = {};
  const DWORD len = GetModuleFileNameW(nullptr, exe_path, MAX_PATH);
  if (len == 0 || len >= MAX_PATH) return {};
  std::error_code ec;
  const std::filesystem::path cand =
      std::filesystem::path(exe_path).parent_path() / L"compressor.exe";
  if (std::filesystem::exists(cand, ec)) return cand;
  return {};
}

// Locale-independent "seconds.millis" (the compressor parses it with strtod).
std::wstring FormatOffset(double seconds) {
  if (seconds < 0.0) seconds = 0.0;
  const long long whole = static_cast<long long>(seconds);
  long long millis = static_cast<long long>(
      (seconds - static_cast<double>(whole)) * 1000.0 + 0.5);
  if (millis > 999) millis = 999;
  wchar_t buf[64] = {};
  _snwprintf_s(buf, _TRUNCATE, L"%lld.%03lld", whole, millis);
  return buf;
}

// J7: every apply writes a NEW file. A unique name means the wallpaper never
// points at a file another apply is overwriting, and WinRT's StorageFile can
// never observe a half-written image (the compressor publishes with
// MoveFileEx over "<name>.jpg.tmp", but the name itself is unique anyway).
std::filesystem::path NewStillPath() {
  FILETIME ft{};
  GetSystemTimeAsFileTime(&ft);
  ULARGE_INTEGER value{};
  value.LowPart = ft.dwLowDateTime;
  value.HighPart = ft.dwHighDateTime;
  wchar_t name[64] = {};
  _snwprintf_s(name, _TRUNCATE, L"still_%020llu.jpg",
               static_cast<unsigned long long>(value.QuadPart));
  return k6wp::StillDir() / name;
}

bool StartsWith(const std::wstring& text, const wchar_t* prefix) {
  const std::size_t n = wcslen(prefix);
  return text.size() >= n && text.compare(0, n, prefix) == 0;
}

// J7: keep the two newest stills (the current wallpaper + the previous one)
// and delete the rest. Best-effort: a locked file is skipped, never fatal.
void PruneStills(std::size_t keep) {
  struct Entry {
    std::filesystem::file_time_type time;
    std::wstring name;
  };
  std::vector<Entry> entries;
  std::error_code ec;
  for (std::filesystem::directory_iterator it(k6wp::StillDir(), ec), end;
       !ec && it != end; it.increment(ec)) {
    std::error_code file_ec;
    if (!it->is_regular_file(file_ec) || file_ec) continue;
    const std::wstring name = it->path().filename().wstring();
    if (!StartsWith(name, L"still_") || it->path().extension() != L".jpg") {
      continue;
    }
    entries.push_back({it->last_write_time(file_ec), name});
  }
  std::sort(entries.begin(), entries.end(),
            [](const Entry& a, const Entry& b) {
              if (a.time != b.time) return a.time > b.time;
              return a.name > b.name;
            });
  for (std::size_t i = keep; i < entries.size(); ++i) {
    std::error_code remove_ec;
    std::filesystem::remove(k6wp::StillDir() / entries[i].name, remove_ec);
  }
}

bool ExtractFrame(const std::filesystem::path& video, double offset_s,
                  int res_w, int res_h, const std::filesystem::path& out,
                  DWORD& exit_code) {
  exit_code = 1;
  std::error_code ec;
  std::filesystem::create_directories(out.parent_path(), ec);

  const std::filesystem::path compressor = SiblingCompressor();
  if (compressor.empty()) return false;

  std::wstring args = L"\"" + compressor.wstring() + L"\" --lockframe --in \"" +
                      video.wstring() + L"\" --out \"" + out.wstring() +
                      L"\" --offset-s " + FormatOffset(offset_s);
  if (res_w > 0 && res_h > 0) {
    args += L" --res " + std::to_wstring(res_w) + L"x" +
            std::to_wstring(res_h);
  }
  std::vector<wchar_t> cmd(args.begin(), args.end());
  cmd.push_back(L'\0');

  STARTUPINFOW si{};
  si.cb = sizeof(si);
  PROCESS_INFORMATION pi{};
  if (!CreateProcessW(compressor.c_str(), cmd.data(), nullptr, nullptr, FALSE,
                      CREATE_NO_WINDOW | BELOW_NORMAL_PRIORITY_CLASS, nullptr,
                      nullptr, &si, &pi)) {
    return false;
  }
  CloseHandle(pi.hThread);
  if (WaitForSingleObject(pi.hProcess, kFrameExtractTimeoutMs) !=
      WAIT_OBJECT_0) {
    TerminateProcess(pi.hProcess, 1);
    WaitForSingleObject(pi.hProcess, 5000);
    CloseHandle(pi.hProcess);
    return false;
  }
  DWORD code = 1;
  GetExitCodeProcess(pi.hProcess, &code);
  CloseHandle(pi.hProcess);
  exit_code = code;
  if (code != 0) return false;
  const auto size = std::filesystem::file_size(out, ec);
  return !ec && size > 0;
}

// --- lock-screen mode (registry) --------------------------------------------

bool ReadDword(HKEY root, const wchar_t* subkey, const wchar_t* name,
               DWORD& out) {
  HKEY key = nullptr;
  if (RegOpenKeyExW(root, subkey, 0, KEY_QUERY_VALUE, &key) != ERROR_SUCCESS) {
    return false;
  }
  DWORD type = 0;
  DWORD size = sizeof(DWORD);
  DWORD value = 0;
  const bool ok = RegQueryValueExW(key, name, nullptr, &type,
                                   reinterpret_cast<LPBYTE>(&value),
                                   &size) == ERROR_SUCCESS &&
                  type == REG_DWORD && size == sizeof(DWORD);
  RegCloseKey(key);
  if (ok) out = value;
  return ok;
}

bool WriteDword(HKEY root, const wchar_t* subkey, const wchar_t* name,
                DWORD value) {
  HKEY key = nullptr;
  if (RegCreateKeyExW(root, subkey, 0, nullptr, 0, KEY_SET_VALUE, nullptr, &key,
                      nullptr) != ERROR_SUCCESS) {
    return false;
  }
  const LSTATUS status = RegSetValueExW(
      key, name, 0, REG_DWORD, reinterpret_cast<const BYTE*>(&value),
      sizeof(value));
  RegCloseKey(key);
  return status == ERROR_SUCCESS;
}

void DeleteValue(HKEY root, const wchar_t* subkey, const wchar_t* name) {
  HKEY key = nullptr;
  if (RegOpenKeyExW(root, subkey, 0, KEY_SET_VALUE, &key) != ERROR_SUCCESS) {
    return;
  }
  RegDeleteValueW(key, name);
  RegCloseKey(key);
}

// Copies the current lock-screen picture (Spotlight included - verified on
// Windows 11 Home) into `target`. Returns false when there is no picture or
// the copy failed; `have_image` reports which.
bool SaveCurrentLockscreenImage(const std::filesystem::path& target,
                                bool& have_image) {
  have_image = false;
  try {
    const IRandomAccessStream stream = LockScreen::GetImageStream();
    if (!stream || stream.Size() == 0) return true;  // nothing to save
    DataReader reader(stream.GetInputStreamAt(0));
    reader.InputStreamOptions(InputStreamOptions::ReadAhead);
    const auto size = static_cast<uint32_t>(stream.Size());
    reader.LoadAsync(size).get();
    const auto buffer = reader.ReadBuffer(size);
    HANDLE file = CreateFileW(target.c_str(), GENERIC_WRITE, 0, nullptr,
                              CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    DWORD written = 0;
    const BOOL ok = WriteFile(file, buffer.data(), buffer.Length(), &written,
                              nullptr);
    CloseHandle(file);
    if (!ok || written != buffer.Length()) return false;
    have_image = true;
    return true;
  } catch (...) {
    return false;
  }
}

bool SetLockscreenImage(const std::filesystem::path& image) {
  try {
    const StorageFile file =
        StorageFile::GetFileFromPathAsync(image.wstring()).get();
    LockScreen::SetImageFileAsync(file).get();
    return true;
  } catch (...) {
    return false;
  }
}

// What the lock screen is in AFTER the apply: our picture, or a rotating mode
// that may override it (J4: the Studio says which one).
std::string LockscreenModeHint() {
  DWORD slideshow = 0;
  DWORD spotlight = 0;
  const bool has_slideshow =
      ReadDword(HKEY_CURRENT_USER, kLockScreenKey, kSlideshowValue, slideshow);
  const bool has_spotlight = ReadDword(HKEY_CURRENT_USER, kContentDeliveryKey,
                                       kRotatingValue, spotlight);
  if (has_slideshow && slideshow != 0) return "slideshow";
  if (has_spotlight && spotlight != 0) return "spotlight";
  return "picture";
}

// The FIRST apply records the user's original mode. Later applies keep it:
// the registry values we write ourselves must never become the "original".
// Returns false only when the store itself cannot be written; a failed image
// copy is a warning (the registry restore still works).
bool CaptureLockscreenOriginal(k6wp::OriginalLockscreen& state,
                               std::string& warning) {
  if (k6wp::ReadOriginalLockscreen(state)) return true;
  k6wp::OriginalLockscreen fresh;
  if (!SaveCurrentLockscreenImage(k6wp::OriginalLockscreenImagePath(),
                                  fresh.have_image)) {
    warning = "previous lock-screen image could not be saved";
  }
  if (fresh.have_image) {
    fresh.image_path = k6wp::OriginalLockscreenImagePath().wstring();
  }
  fresh.spotlight_present = ReadDword(HKEY_CURRENT_USER, kContentDeliveryKey,
                                      kRotatingValue, fresh.spotlight_enabled);
  fresh.slideshow_present = ReadDword(HKEY_CURRENT_USER, kLockScreenKey,
                                      kSlideshowValue, fresh.slideshow_enabled);
  if (!k6wp::WriteOriginalLockscreen(fresh)) return false;
  state = fresh;
  return true;
}

void DisableRotatingLockscreenModes() {
  DWORD value = 0;
  // Only touch values that exist: writing 0 to an absent value would create a
  // key the user never had (and the restore would have to delete it again).
  if (ReadDword(HKEY_CURRENT_USER, kContentDeliveryKey, kRotatingValue, value) &&
      value != 0) {
    WriteDword(HKEY_CURRENT_USER, kContentDeliveryKey, kRotatingValue, 0);
  }
  if (ReadDword(HKEY_CURRENT_USER, kLockScreenKey, kSlideshowValue, value) &&
      value != 0) {
    WriteDword(HKEY_CURRENT_USER, kLockScreenKey, kSlideshowValue, 0);
  }
}

void RestoreLockscreenModes(const k6wp::OriginalLockscreen& state) {
  if (state.spotlight_present) {
    WriteDword(HKEY_CURRENT_USER, kContentDeliveryKey, kRotatingValue,
               state.spotlight_enabled);
  } else {
    DeleteValue(HKEY_CURRENT_USER, kContentDeliveryKey, kRotatingValue);
  }
  if (state.slideshow_present) {
    WriteDword(HKEY_CURRENT_USER, kLockScreenKey, kSlideshowValue,
               state.slideshow_enabled);
  } else {
    DeleteValue(HKEY_CURRENT_USER, kLockScreenKey, kSlideshowValue);
  }
}

bool ApplyLockscreen(const std::filesystem::path& still,
                     std::string& mode_hint, std::string& error) {
  k6wp::OriginalLockscreen original;
  std::string warning;
  if (!CaptureLockscreenOriginal(original, warning)) {
    error = "lock-screen store could not be written";
    return false;
  }
  DisableRotatingLockscreenModes();
  if (!SetLockscreenImage(still)) {
    error = "the lock-screen image could not be set (WinRT SetImageFileAsync)";
    return false;
  }
  mode_hint = LockscreenModeHint();
  if (mode_hint != "picture") {
    // Honest status: the image was set, but a rotating mode is (still) active
    // and may override it.
    error = "the lock screen is in " + mode_hint + " mode";
  } else if (!warning.empty()) {
    error = warning;
  }
  return true;
}

bool RestoreLockscreen(std::string& mode_hint, std::string& error) {
  k6wp::OriginalLockscreen original;
  if (!k6wp::ReadOriginalLockscreen(original)) {
    mode_hint = "nothing-saved";
    return true;
  }
  bool ok = true;
  if (original.have_image && !original.image_path.empty()) {
    std::error_code ec;
    if (std::filesystem::exists(original.image_path, ec)) {
      ok = SetLockscreenImage(original.image_path);
      if (!ok) error = "the previous lock-screen image could not be restored";
    }
  }
  RestoreLockscreenModes(original);
  mode_hint = ok ? "restored" : "restore-failed";
  if (ok) k6wp::ClearOriginalLockscreen();
  return ok;
}

bool SetWallpaper(const std::filesystem::path& still) {
  return SystemParametersInfoW(SPI_SETDESKWALLPAPER, 0,
                               const_cast<LPWSTR>(still.c_str()),
                               SPIF_UPDATEINIFILE | SPIF_SENDCHANGE) != FALSE;
}

// Physical pixels of the primary display. GetSystemMetrics would return the
// DPI-virtualized size because this helper has no DPI manifest, which would
// make the still smaller than the desktop on a scaled display; the current
// DEVMODE is physical regardless of awareness.
void PrimaryResolution(int& width, int& height) {
  DEVMODEW mode{};
  mode.dmSize = sizeof(mode);
  if (EnumDisplaySettingsW(nullptr, ENUM_CURRENT_SETTINGS, &mode) &&
      mode.dmPelsWidth > 0 && mode.dmPelsHeight > 0) {
    width = static_cast<int>(mode.dmPelsWidth);
    height = static_cast<int>(mode.dmPelsHeight);
    return;
  }
  width = GetSystemMetrics(SM_CXSCREEN);
  height = GetSystemMetrics(SM_CYSCREEN);
}

void PublishStatus(const k6wp::StillStatus& status) {
  k6wp::WriteStillStatus(status);
}

}  // namespace

int RunApplyStill(const Options& opts) {
  // One helper at a time (engine rotation + Studio toggle). A helper that
  // cannot get the slot within the budget reports "busy" instead of racing.
  HandleGuard helper_mutex(
      CreateMutexW(nullptr, FALSE, L"Local\\K6WP-Still-Helper"));
  if (helper_mutex.valid() &&
      WaitForSingleObject(helper_mutex.get(), kHelperMutexWaitMs) !=
          WAIT_OBJECT_0) {
    k6wp::StillStatus busy;
    busy.ts = static_cast<long long>(std::time(nullptr));
    busy.error = "another still helper is still running";
    PublishStatus(busy);
    return kExitSpawnFailure;
  }

  k6wp::StillStatus status;
  status.ts = static_cast<long long>(std::time(nullptr));

  const bool explicit_side = opts.still_wallpaper || opts.still_lockscreen;

  if (opts.still_restore) {
    const bool want_wallpaper = opts.still_wallpaper || !explicit_side;
    const bool want_lockscreen = opts.still_lockscreen || !explicit_side;

    if (want_wallpaper) {
      const bool had = !k6wp::ReadOriginalWallpaper().empty();
      status.wallpaper_ok = had ? k6wp::RestoreOriginalWallpaper() : true;
      if (had && !status.wallpaper_ok) {
        status.error = "the original wallpaper could not be restored";
      }
    }
    if (want_lockscreen) {
      std::string hint;
      std::string error;
      status.lockscreen_ok = RestoreLockscreen(hint, error);
      status.lockscreen_mode_hint = hint;
      if (!status.lockscreen_ok && status.error.empty()) status.error = error;
    }
    status.ok = (!want_wallpaper || status.wallpaper_ok) &&
                (!want_lockscreen || status.lockscreen_ok);
    PublishStatus(status);
    return status.ok ? kExitOk : kExitSpawnFailure;
  }

  // Apply. An explicit --wallpaper / --lockscreen wins; otherwise the two
  // studio settings decide (the Engine and Studio both spawn without a side).
  const k6wp::StillSettings settings = k6wp::ReadStillSettingsCached();
  const bool want_wallpaper =
      opts.still_wallpaper || (!explicit_side && settings.wallpaper_sync);
  const bool want_lockscreen =
      opts.still_lockscreen || (!explicit_side && settings.lockscreen_sync);
  if (!want_wallpaper && !want_lockscreen) {
    status.ok = true;  // nothing was asked for: not a failure
    PublishStatus(status);
    return kExitOk;
  }
  if (opts.still_video.empty()) {
    status.error = "no video path";
    PublishStatus(status);
    return kExitSpawnFailure;
  }

  // J8/J9: offset defaults to the setting (0.0 = the first frame); the target
  // resolution defaults to the PRIMARY monitor and the same image is used on
  // every monitor (phase 1).
  const double offset =
      opts.still_has_offset ? opts.still_offset_sec
                            : settings.lockscreen_offset_sec;
  int res_w = 0;
  int res_h = 0;
  if (!opts.still_res.empty()) {
    k6wp::ParseStillRes(opts.still_res, res_w, res_h);
  } else {
    PrimaryResolution(res_w, res_h);
  }

  const std::filesystem::path still = NewStillPath();
  DWORD frame_exit = 1;
  if (!ExtractFrame(opts.still_video, offset, res_w, res_h, still,
                    frame_exit)) {
    status.error = "frame extract failed (compressor exit " +
                   std::to_string(frame_exit) + ")";
    PublishStatus(status);
    return kExitSpawnFailure;
  }

  if (want_wallpaper) {
    // Capture the user's wallpaper BEFORE the first overwrite; J1 keeps our
    // own still out of the store on every later apply.
    k6wp::CaptureOriginalWallpaper();
    status.wallpaper_ok = SetWallpaper(still);
    if (!status.wallpaper_ok) {
      status.error = "the desktop wallpaper could not be set";
    }
  }

  if (want_lockscreen) {
    std::string hint;
    std::string error;
    status.lockscreen_ok = ApplyLockscreen(still, hint, error);
    status.lockscreen_mode_hint = hint;
    if (!status.lockscreen_ok) {
      if (status.error.empty()) status.error = error;
    } else if (!error.empty()) {
      // Applied, but the mode hint warns that a rotating mode may override.
      if (status.error.empty()) status.error = error;
    }
  }

  PruneStills(kStillsToKeep);
  status.ok = (!want_wallpaper || status.wallpaper_ok) &&
              (!want_lockscreen || status.lockscreen_ok);
  PublishStatus(status);
  return status.ok ? kExitOk : kExitSpawnFailure;
}

}  // namespace k6wp::launcher
