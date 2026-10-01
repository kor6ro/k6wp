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

 private:
  LogFn log_;
  std::wstring saved_;
  bool valid_ = false;
};

}  // namespace k6wp
