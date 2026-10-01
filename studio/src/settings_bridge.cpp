// Phase 4 of the Widgets -> QML migration: the Pengaturan tab's backend.
// See settings_bridge.hpp for why autostart is written through immediately
// while everything else waits for apply().

#include "settings_bridge.hpp"
#include "bridge_diagnostics.hpp"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QUrl>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shlobj.h>

#include <algorithm>
#include <cwctype>
#include <filesystem>
#include <fstream>
#include <vector>

#include "autostart.hpp"
#include "cache_dir_policy.hpp"
#include "config_schema.hpp"

namespace k6wp {

SettingsBridge::SettingsBridge(QObject* parent) : QObject(parent) {
  settings_path_ = DefaultStudioSettingsPath();
  config_path_ = DefaultConfigPath();
  // Read before reload() so a QML binding on `language` sees the stored value
  // on its first evaluation instead of flashing the default.
  ui_language_ = LoadUiLanguage();
  connect(this, &SettingsBridge::changed, this, [this]() {
    if (!loading_) dirty_ = true;
  });
  if (QCoreApplication* app = QCoreApplication::instance()) {
    // Flush unsaved Pengaturan edits on quit; dirty_ gates it so a launch with
    // no edits never rewrites config.json over engine-side changes.
    connect(app, &QCoreApplication::aboutToQuit, this, [this]() {
      if (dirty_) apply();
    });
  }
  reload();
}

SettingsBridge::~SettingsBridge() = default;

void SettingsBridge::reload() {
  loading_ = true;
  bool loaded = true;
  try {
    studio_ = LoadStudioSettings(settings_path_);
  } catch (const ConfigError& e) {
    // LoadStudioSettings already preserved the bad bytes as
    // studio_settings.json.bak; fall back to defaults and say so.
    studio_ = DefaultStudioSettings();
    loaded = false;
    SetLastError(QStringLiteral("Pengaturan studio rusak, kembali ke bawaan: %1")
                     .arg(QString::fromUtf8(e.what())));
  }
  try {
    config_ = LoadConfig(config_path_);
  } catch (const ConfigError& e) {
    config_ = WallpaperConfig{};
    AppendLog(QStringLiteral("Config engine tidak bisa dibaca: %1")
                  .arg(QString::fromUtf8(e.what())));
  }
  autostart_on_ = IsAutostart();
  // Clearing unconditionally wiped the message set a few lines above, so a
  // corrupt studio_settings.json reset the user to defaults in silence - the
  // QML banner had nothing left to show. Same guard, and for the same reason,
  // as LibraryGridModel::reload.
  if (loaded) {
    SetLastError(QString());
  }
  emit changed();
  loading_ = false;
}

void SettingsBridge::apply() {
  QString error;
  // Independent writes: a config.json failure must not discard the preferences
  // that did save.
  try {
    ValidateStudioSettings(studio_);
    SaveStudioSettings(settings_path_, studio_);
    AppendLog(QStringLiteral("Pengaturan studio tersimpan"));
  } catch (const ConfigError& e) {
    error = QStringLiteral("Gagal menyimpan pengaturan studio: %1")
                .arg(QString::fromUtf8(e.what()));
  }
  try {
    SaveConfig(config_path_, config_);
    if (error.isEmpty()) {
      AppendLog(QStringLiteral("Pengaturan engine tersimpan"));
    }
  } catch (const ConfigError& e) {
    const QString msg = QStringLiteral("Gagal menyimpan konfigurasi:\n%1")
                            .arg(QString::fromUtf8(e.what()));
    error = error.isEmpty() ? msg : error + QLatin1String("\n") + msg;
  }
  if (error.isEmpty()) {
    SetLastError(QString());
    dirty_ = false;
  } else {
    SetLastError(error);
  }
}

bool SettingsBridge::engineRestartNeeded() const {
  return config_.cpu_affinity != "auto" || config_.gpu_adapter != "auto";
}

// --- studio settings setters ------------------------------------------------

void SettingsBridge::setAutoCompressOnImport(bool on) {
  studio_.auto_compress_on_import = on;
  emit changed();
}

void SettingsBridge::setCompressOutputDir(const QString& dir) {
  if (dir.isEmpty()) {
    SetLastError(tr("Folder output tidak boleh kosong."));
    return;
  }
  studio_.compress_output_dir = dir.toStdWString();
  emit changed();
}

void SettingsBridge::setDefaultCrf(int value) {
  if (value < 16 || value > 28) {
    SetLastError(tr("CRF bawaan harus 16-28."));
    return;
  }
  studio_.default_crf = value;
  emit changed();
}

void SettingsBridge::setDefaultFps(int value) {
  if (value < 1 || value > 30) {
    SetLastError(tr("FPS bawaan harus 1-30."));
    return;
  }
  studio_.default_fps = value;
  emit changed();
}

void SettingsBridge::setDefaultResolutionMode(const QString& mode) {
  static const QStringList kModes = {"match_monitor", "source", "720p", "1080p",
                                     "2160p"};
  if (!kModes.contains(mode)) {
    SetLastError(tr("Mode resolusi tidak dikenal."));
    return;
  }
  studio_.default_resolution_mode = mode.toStdString();
  emit changed();
}

void SettingsBridge::setStartWithWindows(bool on) {
  std::string err;
  if (!SetAutostart(on, &err)) {
    SetLastError(QStringLiteral("Gagal menyimpan mulai otomatis: %1")
                     .arg(QString::fromStdString(err)));
  } else {
    AppendLog(QStringLiteral("Mulai otomatis %1")
                  .arg(on ? QStringLiteral("ON") : QStringLiteral("OFF")));
  }
  // Re-read rather than trusting the argument: on failure the Run value is
  // unchanged and the checkbox must snap back to reality.
  autostart_on_ = IsAutostart();
  emit changed();
}

void SettingsBridge::setCacheDir(const QString& dir) {
  if (dir.isEmpty()) {
    return;
  }
  studio_.cache_dir = dir.toStdWString();
  emit changed();
}

void SettingsBridge::setLockscreenSync(bool on) {
  studio_.lockscreen_sync = on;
  emit changed();
}

void SettingsBridge::setLockscreenOffsetSec(double seconds) {
  if (seconds < 0.0) {
    return;
  }
  studio_.lockscreen_offset_sec = seconds;
  emit changed();
}

void SettingsBridge::setCompressAdvancedVisible(bool on) {
  studio_.compress_advanced_visible = on;
  emit changed();
}

void SettingsBridge::setCheckUpdates(bool on) {
  studio_.check_updates = on;
  emit changed();
}

// --- UI language --------------------------------------------------------------

void SettingsBridge::setLanguage(const QString& code) {
  // Write-through (like setStartWithWindows): the translator is installed at
  // startup, so waiting for apply() would drop the user's choice.
  if (!SaveUiLanguage(code)) {
    // Unsupported code or an unwritable store: leave both the file and the
    // property alone so the picker snaps back to what is actually persisted.
    return;
  }
  // Re-read rather than echoing the argument: SaveUiLanguage normalises
  // (trimmed, lower-cased) and the property must show what is on disk.
  ui_language_ = LoadUiLanguage();
  emit changed();
}

// --- engine config setters --------------------------------------------------

void SettingsBridge::setFitMode(const QString& mode) {
  static const QStringList kModes = {"fill", "cover", "fit", "stretch", "center"};
  if (!kModes.contains(mode)) {
    SetLastError(tr("Mode pengisian layar tidak dikenal."));
    return;
  }
  config_.fit_mode = mode.toStdString();
  emit changed();
}

void SettingsBridge::setMonitorId(int id) {
  if (id < -1) {
    SetLastError(tr("Monitor tidak valid."));
    return;
  }
  config_.monitor_id = id;
  emit changed();
}

void SettingsBridge::setBatterySaver(bool on) {
  config_.battery_saver = on;
  emit changed();
}

void SettingsBridge::setBatteryMode(const QString& mode) {
  if (mode != "cap24" && mode != "static") {
    SetLastError(tr("Mode baterai tidak dikenal."));
    return;
  }
  config_.battery_mode = mode.toStdString();
  emit changed();
}

void SettingsBridge::setCpuAffinity(const QString& mode) {
  if (mode != "auto" && mode != "all") {
    SetLastError(tr("Afinitas CPU tidak dikenal."));
    return;
  }
  config_.cpu_affinity = mode.toStdString();
  emit changed();
}

void SettingsBridge::setGpuAdapter(const QString& mode) {
  if (mode != "auto" && mode != "integrated" && mode != "discrete") {
    SetLastError(tr("Pilihan GPU tidak dikenal."));
    return;
  }
  config_.gpu_adapter = mode.toStdString();
  emit changed();
}

void SettingsBridge::setFpsCap(int value) {
  if (value < 1 || value > 30) {
    SetLastError(tr("Batas FPS harus 1-30."));
    return;
  }
  config_.fps_cap = value;
  emit changed();
}

void SettingsBridge::setCrf(int value) {
  if (value < 16 || value > 28) {
    SetLastError(tr("CRF harus 16-28."));
    return;
  }
  config_.crf = value;
  emit changed();
}

void SettingsBridge::setSpeed(double value) {
  if (value < 0.5 || value > 2.0) {
    SetLastError(tr("Kecepatan harus 0.5-2.0."));
    return;
  }
  config_.speed = value;
  emit changed();
}

void SettingsBridge::setResolutionW(int value) {
  if (value < 0) {
    SetLastError(tr("Lebar resolusi tidak boleh negatif."));
    return;
  }
  config_.resolution_w = value;
  emit changed();
}

void SettingsBridge::setResolutionH(int value) {
  if (value < 0) {
    SetLastError(tr("Tinggi resolusi tidak boleh negatif."));
    return;
  }
  config_.resolution_h = value;
  emit changed();
}

// --- directories ------------------------------------------------------------

void SettingsBridge::pickCompressOutputDir() {
  const QString dir = QFileDialog::getExistingDirectory(
      nullptr, QStringLiteral("Pilih Folder Output"),
      QString::fromStdWString(studio_.compress_output_dir));
  if (!dir.isEmpty()) {
    setCompressOutputDir(dir);
  }
}

void SettingsBridge::pickCacheDir() {
  const QString dir = QFileDialog::getExistingDirectory(
      nullptr, QStringLiteral("Pilih Folder Cache"),
      QString::fromStdWString(studio_.cache_dir));
  if (dir.isEmpty()) {
    return;
  }
  // Choosing a folder IS the opt-in for sweeping it: clearCache() refuses any
  // folder outside %LOCALAPPDATA%\K6WP that does not carry the marker this
  // writes. Without it, a mistyped path in the field is one click away from
  // deleting itself.
  if (!MarkCacheDirOptedIn(std::filesystem::path(dir.toStdWString()))) {
    SetLastError(QStringLiteral("Folder cache tidak bisa ditandai, jadi tidak "
                                "akan bisa dibersihkan: %1")
                     .arg(dir));
  }
  setCacheDir(dir);
}

void SettingsBridge::openCacheDir() {
  const QString dir = cacheDir();
  if (!dir.isEmpty()) {
    QDesktopServices::openUrl(
        QUrl::fromLocalFile(QDir::toNativeSeparators(dir)));
  }
}

int SettingsBridge::clearCache() {
  const QString dir = cacheDir();
  if (dir.isEmpty()) {
    return 0;
  }
  const std::filesystem::path root(dir.toStdWString());
  std::error_code ec;
  if (!std::filesystem::exists(root, ec) || ec) {
    AppendLog(QStringLiteral("Cache tidak ada: %1").arg(dir));
    return 0;
  }
  // cache_dir is free text, so this is the only thing standing between one
  // click and the contents of, say, C:\Users\<me>\Videos. Refuse instead of
  // sweeping, and name the folder in the refusal: refusing silently reads as
  // "the cache was already empty", which is how the user finds out.
  QString refusal;
  if (!CacheDirIsClearable(root, settings_path_.parent_path(), &refusal)) {
    SetLastError(refusal);
    return 0;
  }
  int removed = 0;
  int failed = 0;
  // Recursive, regular files only: never follows a directory symlink out of the
  // cache, and never deletes the cache root itself.
  for (std::filesystem::recursive_directory_iterator it(root, ec), end;
       it != end && !ec; it.increment(ec)) {
    // The opt-in has to outlive the sweep it authorised, or the second clear
    // of the same folder would be refused.
    if (it->path().filename() == std::filesystem::path(kCacheOptInMarker)) {
      continue;
    }
    if (!it->is_regular_file(ec) || ec) {
      continue;
    }
    if (std::filesystem::remove(it->path(), ec) && !ec) {
      ++removed;
    } else {
      ++failed;
    }
  }
  if (failed > 0) {
    // Both channels: lastError is what the QML banner shows, the log line is
    // what a QA screenshot has. Logging "cache dibersihkan" after a partial
    // sweep is how a user learns their cache is only half-cleared.
    SetLastError(QStringLiteral("Sebagian cache gagal dihapus: %1").arg(failed));
    return removed;
  }
  // A clean sweep supersedes the refusal an earlier attempt reported, the same
  // way a successful apply() does - otherwise a corrected path still shows the
  // old "ditolak" banner over a sweep that worked.
  SetLastError(QString());
  AppendLog(QStringLiteral("Cache dibersihkan (%1 entri dihapus).").arg(removed));
  return removed;
}

// --- diagnostics ------------------------------------------------------------

void SettingsBridge::AppendLog(const QString& line) {
  AppendCapped(log_, line, 500);
  emit logChanged();
}

void SettingsBridge::SetLastError(const QString& error) {
  if (last_error_ == error) {
    return;
  }
  last_error_ = error;
  emit lastErrorChanged();
  if (!error.isEmpty()) {
    AppendLog(error);
  }
}

}  // namespace k6wp
