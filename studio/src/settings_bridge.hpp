#pragma once

// Phase 4 of the Widgets -> QML migration: the Pengaturan tab's backend.
//
// SettingsBridge owns the studio preferences (StudioSettings, a plain struct
// with no QObject and therefore no Q_PROPERTY of its own) and the engine
// playback state (WallpaperConfig), and persists both.
//
// The two files are deliberately kept apart, matching the on-disk contract:
//   * studio_settings.json - Studio-only preferences. The engine never reads it.
//   * config.json           - pure engine playback state. Studio only writes it
//                             as a side effect of applying playback settings.
// Settings that the engine can only pick up on a restart (CPU affinity, GPU
// adapter, paste/battery policy) are surfaced, and the QML side pairs them with
// Studio.startEngine() for the restart, exactly as the Widgets "Restart engine"
// button did.
//
// Autostart is the one field written through immediately rather than on apply:
// it is an HKCU Run value, and the Widgets checkbox fired SetAutostart on every
// toggle, so delaying it until "Terapkan" would change the behaviour.

#include <QObject>
#include <QString>
#include <QStringList>

#include <QtQml/qqmlregistration.h>

#include "config_schema.hpp"
#include "studio_settings.hpp"
#include "ui_language.hpp"

namespace k6wp {

// NOT `final`: QML registration instantiates a QQmlElement<T> subclass, so a
// final QML_ELEMENT class does not compile (C3246).
class SettingsBridge : public QObject {
  // Registered as `Settings`; QML_NAMED_ELEMENT is required because bare
  // QML_ELEMENT would register `SettingsBridge` and every `Settings.*` binding
  // would silently keep its default with no QML error.
  Q_OBJECT
  QML_NAMED_ELEMENT(Settings)
  QML_ELEMENT
  QML_SINGLETON

 public:
  explicit SettingsBridge(QObject* parent = nullptr);
  ~SettingsBridge() override;

  // --- studio_settings.json ----------------------------------------------
  Q_PROPERTY(bool autoCompressOnImport READ autoCompressOnImport WRITE setAutoCompressOnImport NOTIFY changed)
  Q_PROPERTY(QString compressOutputDir READ compressOutputDir WRITE setCompressOutputDir NOTIFY changed)
  Q_PROPERTY(int defaultCrf READ defaultCrf WRITE setDefaultCrf NOTIFY changed)
  Q_PROPERTY(int defaultFps READ defaultFps WRITE setDefaultFps NOTIFY changed)
  Q_PROPERTY(QString defaultResolutionMode READ defaultResolutionMode WRITE setDefaultResolutionMode NOTIFY changed)
  Q_PROPERTY(bool startWithWindows READ startWithWindows WRITE setStartWithWindows NOTIFY changed)
  Q_PROPERTY(QString cacheDir READ cacheDir WRITE setCacheDir NOTIFY changed)
  Q_PROPERTY(bool lockscreenSync READ lockscreenSync WRITE setLockscreenSync NOTIFY changed)
  Q_PROPERTY(double lockscreenOffsetSec READ lockscreenOffsetSec WRITE setLockscreenOffsetSec NOTIFY changed)
  Q_PROPERTY(bool compressAdvancedVisible READ compressAdvancedVisible WRITE setCompressAdvancedVisible NOTIFY changed)
  Q_PROPERTY(bool checkUpdates READ checkUpdates WRITE setCheckUpdates NOTIFY changed)

  bool autoCompressOnImport() const { return studio_.auto_compress_on_import; }
  QString compressOutputDir() const { return QString::fromStdWString(studio_.compress_output_dir); }
  int defaultCrf() const { return studio_.default_crf; }
  int defaultFps() const { return studio_.default_fps; }
  QString defaultResolutionMode() const { return QString::fromStdString(studio_.default_resolution_mode); }
  bool startWithWindows() const { return autostart_on_; }
  QString cacheDir() const { return QString::fromStdWString(studio_.cache_dir); }
  bool lockscreenSync() const { return studio_.lockscreen_sync; }
  double lockscreenOffsetSec() const { return studio_.lockscreen_offset_sec; }
  bool compressAdvancedVisible() const { return studio_.compress_advanced_visible; }
  bool checkUpdates() const { return studio_.check_updates; }

  // --- studio_ui.ini (UI language) -------------------------------------------
  // Its own store rather than a StudioSettings field (see ui_language.hpp).
  // Written through on change like startWithWindows, NOT on apply(): the
  // QTranslator is installed during startup, so an unsaved choice would never
  // reach the next launch. A new language takes effect on the next start -
  // the bindings QmlShell already instantiated cannot be re-translated from
  // here.
  Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY changed)

  QString language() const { return ui_language_; }

  // --- config.json (engine playback state) --------------------------------
  Q_PROPERTY(QString fitMode READ fitMode WRITE setFitMode NOTIFY changed)
  Q_PROPERTY(int monitorId READ monitorId WRITE setMonitorId NOTIFY changed)
  Q_PROPERTY(bool batterySaver READ batterySaver WRITE setBatterySaver NOTIFY changed)
  Q_PROPERTY(QString batteryMode READ batteryMode WRITE setBatteryMode NOTIFY changed)
  Q_PROPERTY(QString cpuAffinity READ cpuAffinity WRITE setCpuAffinity NOTIFY changed)
  Q_PROPERTY(QString gpuAdapter READ gpuAdapter WRITE setGpuAdapter NOTIFY changed)
  Q_PROPERTY(int fpsCap READ fpsCap WRITE setFpsCap NOTIFY changed)
  Q_PROPERTY(int crf READ crf WRITE setCrf NOTIFY changed)
  Q_PROPERTY(double speed READ speed WRITE setSpeed NOTIFY changed)
  Q_PROPERTY(int resolutionW READ resolutionW WRITE setResolutionW NOTIFY changed)
  Q_PROPERTY(int resolutionH READ resolutionH WRITE setResolutionH NOTIFY changed)
  // True when cpu_affinity / gpu_adapter differ from "auto": the engine can only
  // pick these up on a restart, so the QML side shows a restart hint.
  Q_PROPERTY(bool engineRestartNeeded READ engineRestartNeeded NOTIFY changed)

  QString fitMode() const { return QString::fromStdString(config_.fit_mode); }
  int monitorId() const { return config_.monitor_id; }
  bool batterySaver() const { return config_.battery_saver; }
  QString batteryMode() const { return QString::fromStdString(config_.battery_mode); }
  QString cpuAffinity() const { return QString::fromStdString(config_.cpu_affinity); }
  QString gpuAdapter() const { return QString::fromStdString(config_.gpu_adapter); }
  int fpsCap() const { return config_.fps_cap; }
  int crf() const { return config_.crf; }
  double speed() const { return config_.speed; }
  int resolutionW() const { return config_.resolution_w; }
  int resolutionH() const { return config_.resolution_h; }
  bool engineRestartNeeded() const;

  // --- shell --------------------------------------------------------------
  Q_PROPERTY(QString settingsPath READ settingsPath CONSTANT)
  Q_PROPERTY(QString configPath READ configPath CONSTANT)
  Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
  Q_PROPERTY(QStringList log READ log NOTIFY logChanged)

  QString settingsPath() const { return QString::fromStdWString(settings_path_.wstring()); }
  QString configPath() const { return QString::fromStdWString(config_path_.wstring()); }
  QString lastError() const { return last_error_; }
  QStringList log() const { return log_; }

  // Re-reads both files, discarding unsaved edits.
  Q_INVOKABLE void reload();

  // Writes studio_settings.json and config.json. Each write is independent so a
  // config.json failure does not lose the preferences that did save.
  Q_INVOKABLE void apply();

  Q_INVOKABLE void pickCompressOutputDir();
  Q_INVOKABLE void pickCacheDir();
  Q_INVOKABLE void openCacheDir();
  // Deletes files under cacheDir; returns how many were removed. Never throws.
  Q_INVOKABLE int clearCache();

  Q_INVOKABLE void setAutoCompressOnImport(bool on);
  Q_INVOKABLE void setCompressOutputDir(const QString& dir);
  Q_INVOKABLE void setDefaultCrf(int value);
  Q_INVOKABLE void setDefaultFps(int value);
  Q_INVOKABLE void setDefaultResolutionMode(const QString& mode);
  Q_INVOKABLE void setStartWithWindows(bool on);
  Q_INVOKABLE void setCacheDir(const QString& dir);
  Q_INVOKABLE void setLockscreenSync(bool on);
  Q_INVOKABLE void setLockscreenOffsetSec(double seconds);
  Q_INVOKABLE void setCompressAdvancedVisible(bool on);
  Q_INVOKABLE void setCheckUpdates(bool on);

  // Rejects an unsupported code (returns without writing) and emits changed()
  // only on success, so the QML picker never shows a value the store refused.
  Q_INVOKABLE void setLanguage(const QString& code);

  Q_INVOKABLE void setFitMode(const QString& mode);
  Q_INVOKABLE void setMonitorId(int id);
  Q_INVOKABLE void setBatterySaver(bool on);
  Q_INVOKABLE void setBatteryMode(const QString& mode);
  Q_INVOKABLE void setCpuAffinity(const QString& mode);
  Q_INVOKABLE void setGpuAdapter(const QString& mode);
  Q_INVOKABLE void setFpsCap(int value);
  Q_INVOKABLE void setCrf(int value);
  Q_INVOKABLE void setSpeed(double value);
  Q_INVOKABLE void setResolutionW(int value);
  Q_INVOKABLE void setResolutionH(int value);

 signals:
  void changed();
  void lastErrorChanged();
  void logChanged();

 private:
  void AppendLog(const QString& line);
  void SetLastError(const QString& error);

  StudioSettings studio_;
  WallpaperConfig config_;
  std::filesystem::path settings_path_;
  std::filesystem::path config_path_;
  // Mirrors the HKCU Run value, which is the real source of truth for autostart
  // (the studio_settings.json flag is not what the OS reads).
  bool autostart_on_ = false;
  // Mirrors studio_ui.ini. Seeded in the constructor so the QML binding reads
  // the stored language on its first evaluation instead of the default.
  QString ui_language_ = QString::fromLatin1(kUiLanguageSource);
  QString last_error_;
  QStringList log_;
  // True when the user edited a value this session. A clean quit persists it,
  // so closing Studio no longer discards Pengaturan changes the user did not
  // explicitly "Terapkan". loading_ suppresses marking dirty during reload().
  bool dirty_ = false;
  bool loading_ = false;
};

}  // namespace k6wp
