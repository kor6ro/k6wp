// Phase 4 of the Widgets -> QML migration: the Pengaturan tab's backend.
// See settings_bridge.hpp for why autostart is written through immediately
// while everything else waits for apply().

#include "settings_bridge.hpp"

#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QUrl>

#include <filesystem>

#include "autostart.hpp"
#include "config_schema.hpp"

namespace k6wp {

SettingsBridge::SettingsBridge(QObject* parent) : QObject(parent) {
  settings_path_ = DefaultStudioSettingsPath();
  config_path_ = DefaultConfigPath();
  // Read before reload() so a QML binding on `language` sees the stored value
  // on its first evaluation instead of flashing the default.
  ui_language_ = LoadUiLanguage();
  reload();
}

SettingsBridge::~SettingsBridge() = default;

void SettingsBridge::reload() {
  try {
    studio_ = LoadStudioSettings(settings_path_);
  } catch (const ConfigError& e) {
    // LoadStudioSettings already preserved the bad bytes as
    // studio_settings.json.bak; fall back to defaults and say so.
    studio_ = DefaultStudioSettings();
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
  SetLastError(QString());
  emit changed();
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
    return;  // validation rejects an empty dir; ignore rather than save junk
  }
  studio_.compress_output_dir = dir.toStdWString();
  emit changed();
}

void SettingsBridge::setDefaultCrf(int value) {
  if (value < 16 || value > 28) {
    return;
  }
  studio_.default_crf = value;
  emit changed();
}

void SettingsBridge::setDefaultFps(int value) {
  if (value < 1 || value > 30) {
    return;
  }
  studio_.default_fps = value;
  emit changed();
}

void SettingsBridge::setDefaultResolutionMode(const QString& mode) {
  static const QStringList kModes = {"match_monitor", "source", "720p", "1080p",
                                     "2160p"};
  if (!kModes.contains(mode)) {
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
    return;
  }
  config_.fit_mode = mode.toStdString();
  emit changed();
}

void SettingsBridge::setMonitorId(int id) {
  if (id < -1) {
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
    return;
  }
  config_.battery_mode = mode.toStdString();
  emit changed();
}

void SettingsBridge::setCpuAffinity(const QString& mode) {
  if (mode != "auto" && mode != "all") {
    return;
  }
  config_.cpu_affinity = mode.toStdString();
  emit changed();
}

void SettingsBridge::setGpuAdapter(const QString& mode) {
  if (mode != "auto" && mode != "integrated" && mode != "discrete") {
    return;
  }
  config_.gpu_adapter = mode.toStdString();
  emit changed();
}

void SettingsBridge::setFpsCap(int value) {
  if (value < 1 || value > 30) {
    return;
  }
  config_.fps_cap = value;
  emit changed();
}

void SettingsBridge::setCrf(int value) {
  if (value < 16 || value > 28) {
    return;
  }
  config_.crf = value;
  emit changed();
}

void SettingsBridge::setSpeed(double value) {
  if (value < 0.5 || value > 2.0) {
    return;
  }
  config_.speed = value;
  emit changed();
}

void SettingsBridge::setResolutionW(int value) {
  if (value < 0) {
    return;
  }
  config_.resolution_w = value;
  emit changed();
}

void SettingsBridge::setResolutionH(int value) {
  if (value < 0) {
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
  if (!dir.isEmpty()) {
    setCacheDir(dir);
  }
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
  int removed = 0;
  int failed = 0;
  // Recursive, regular files only: never follows a directory symlink out of the
  // cache, and never deletes the cache root itself.
  for (std::filesystem::recursive_directory_iterator it(root, ec), end;
       it != end && !ec; it.increment(ec)) {
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
    AppendLog(QStringLiteral("Sebagian cache gagal dihapus: %1").arg(failed));
  }
  AppendLog(QStringLiteral("Cache dibersihkan (%1 entri dihapus).").arg(removed));
  return removed;
}

// --- diagnostics ------------------------------------------------------------

void SettingsBridge::AppendLog(const QString& line) {
  log_.append(line);
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
