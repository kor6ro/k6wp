#pragma once

#include <string>

namespace k6wp {

// Saves the OS desktop wallpaper at boot and restores it on graceful shutdown.
// Best-effort: a failed save leaves Restore() a no-op.
class OsWallpaperGuard {
 public:
  // Matches EngineApp::Log (void(const char* fmt, ...)); may be null.
  using LogFn = void (*)(const char* fmt, ...);

  explicit OsWallpaperGuard(LogFn log = nullptr) : log_(log) {}

  void Save();
  void Restore();

  // E-01: the snapshot the guard will restore on shutdown. EngineApp compares
  // it against the live OS wallpaper on WM_SETTINGCHANGE to detect an external
  // change (Windows Settings, slideshow) and re-Save() so Restore() never
  // clobbers the user's new wallpaper with a stale boot value.
  const std::wstring& saved() const { return saved_; }
  bool valid() const { return valid_; }

 private:
  LogFn log_;
  std::wstring saved_;
  bool valid_ = false;
};

}  // namespace k6wp
