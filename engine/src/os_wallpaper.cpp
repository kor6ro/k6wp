#include "os_wallpaper.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace k6wp {

void OsWallpaperGuard::Save() {
  wchar_t buf[MAX_PATH] = {};
  if (SystemParametersInfoW(SPI_GETDESKWALLPAPER, MAX_PATH, buf, 0) &&
      buf[0] != L'\0') {
    saved_.assign(buf);
    valid_ = true;
    if (log_) log_("engine: saved OS wallpaper '%ls'", saved_.c_str());
  } else {
    saved_.clear();
    valid_ = false;
    if (log_) {
      log_("warning: could not read OS wallpaper (error %lu), continuing without restore",
           GetLastError());
    }
  }
}

void OsWallpaperGuard::Restore() {
  if (!valid_ || saved_.empty()) {
    if (log_) log_("engine: no saved OS wallpaper, skipping restore");
    return;
  }
  if (SystemParametersInfoW(SPI_SETDESKWALLPAPER, 0,
                            const_cast<LPWSTR>(saved_.c_str()),
                            SPIF_UPDATEINIFILE | SPIF_SENDCHANGE)) {
    if (log_) log_("engine: restored OS wallpaper '%ls'", saved_.c_str());
  } else {
    if (log_) {
      log_("warning: could not restore OS wallpaper '%ls' (error %lu)",
           saved_.c_str(), GetLastError());
    }
  }
}

}  // namespace k6wp
