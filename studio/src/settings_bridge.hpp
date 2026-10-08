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

#include <functional>

#include <QtQml/qqmlregistration.h>

#include "config_schema.hpp"
#include "studio_settings.hpp"
#include "ui_language.hpp"

namespace k6wp {

// Plan todo 17 / brief B10: process-global mirror of the closeToTray
// preference. Lives here (not in qml_shell.cpp) because studio_logic_test
// links settings_bridge.cpp but NOT qml_shell.cpp, and SettingsBridge is
// the owner of the setting: ctor/reload/setter write it, QmlShell::closeEvent
// reads it. Default false = pre-todo-17 behavior (close = exit).
void SetCloseToTrayEnabled(bool on);
bool CloseToTrayEnabled();
// Listener invoked on every closeToTray change (reload + setter). QmlShell
// registers it to show/hide the tray icon live; nullptr clears. Invoked on
// the GUI thread; a null listener is a no-op.
void SetCloseToTrayListener(std::function<void(bool)> listener);

// Plan todo 14 (brief B8 + D1): the value set a performance preset maps to.
// Fields mirror the WallpaperConfig playback keys the preset rewrites in
// config.json; the preset NAME itself lives in studio_settings.json
// performancePreset (todo-8 key). Pure C++ — no Qt — so studio_logic_test
// drives the mapping without constructing the QML singleton.
struct PerformancePresetValues {
  std::string cpuAffinity;  // "auto" | "all"
  std::string gpuAdapter;   // "auto" | "integrated" | "discrete"
  int fpsCap = 30;          // 1 - 30
  int crf = 22;             // 16 - 28
  double speed = 1.0;       // 0.5 - 2.0
  int resolutionW = 0;      // 0 = monitor native
  int resolutionH = 0;      // 0 = monitor native
};

// D1 final numbers (decided at S7 / todo 14; brief §B-LOCKED D1):
//   Seimbang = "nilai kini" (current values): auto/auto, fps 30, CRF 22,
//              speed 1.0, native 0x0 — the product defaults per D1 + README
//              ("Battery saver: FPS cap (default 30…)"; note the
//              config-schema struct default of 24 predates that doc — D1
//              governs the preset mapping, config_schema.* stays untouched).
//   Hemat    = reduced-res + cap-fps: auto/integrated, fps 24 (the existing
//              DC battery-saver cap in engine power.cpp), CRF 26 (lighter
//              encode), speed 1.0, 1280x720 (the existing 720p mode).
//   Maksimal = full: all/discrete, fps 30 (schema max), CRF 16 (min = best
//              quality), speed 1.0, native 0x0.
// Unknown/empty preset string -> Seimbang values (default; never crashes).
PerformancePresetValues PresetToValues(const std::string& preset);

// Nearest preset for a value set. Round-trips PresetToValues for all three
// presets. Categorical mismatches (cpuAffinity/gpuAdapter) weigh 100 each so
// a mode change always outranks numeric drift; ties resolve to "Seimbang"
// (the D1 default) because the table is scanned in that order with strict
// less-than. Always returns one of the three names — never throws.
std::string ValuesToPreset(const PerformancePresetValues& values);

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
  // Plan todo 9 (consumes the todo-8 GAP-7 key): rotation source,
  // "all" | "custom". studio_settings.json playlistSource; persisted on
  // apply() like the other studio preferences.
  Q_PROPERTY(QString playlistSource READ playlistSource WRITE setPlaylistSource NOTIFY changed)
  // Plan todo 14 (consumes the todo-8 GAP-7 key; brief B8): performance
  // preset name, "Hemat" | "Seimbang" | "Maksimal". Setting it rewrites the
  // mapped config.json playback fields via PresetToValues; presets touching
  // cpuAffinity/gpuAdapter flip engineRestartNeeded and emit
  // performancePresetApplied(true) so QML can trigger the automatic restart
  // with a friendly notice (never the word "engine").
  Q_PROPERTY(QString performancePreset READ performancePreset WRITE setPerformancePreset NOTIFY changed)
  // Plan todo 17 / brief B10 + C-18: "Tutup ke tray (Studio tetap jalan di
  // latar)". Consumes the todo-8 key studio_settings.json "closeToTray"
  // (default false = old behavior byte-for-byte). Written through on toggle
  // like startWithWindows: the closeEvent decision needs the live value, not
  // an unapplied edit. Persists via PersistStudioSettingsNow() (DEF-2).
  Q_PROPERTY(bool closeToTray READ closeToTray WRITE setCloseToTray NOTIFY changed)

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
  QString playlistSource() const { return QString::fromStdString(studio_.playlist_source); }
  QString performancePreset() const { return QString::fromStdString(studio_.performance_preset); }
  bool closeToTray() const { return studio_.close_to_tray; }

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
  Q_INVOKABLE void setPlaylistSource(const QString& source);
  // Plan todo 14: applies PresetToValues to the config.json playback fields
  // and stores the preset name. Unknown names are refused without moving the
  // property (same contract as setPlaylistSource).
  Q_INVOKABLE void setPerformancePreset(const QString& preset);
  // Plan todo 17: persists studio_.close_to_tray, mirrors it into the
  // process-global CloseToTrayEnabled() (what QmlShell::closeEvent reads)
  // and nudges the active shell so the tray icon appears/disappears live.
  Q_INVOKABLE void setCloseToTray(bool on);

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
  // Plan todo 14: fired after a successful setPerformancePreset. True when
  // the preset touched cpu_affinity/gpu_adapter (engineRestartNeeded flipped
  // on) — QML connects this to Studio.startEngine() plus the friendly
  // "Menerapkan performa…" notice (glossary §5: never "engine").
  void performancePresetApplied(bool needsRestart);

 private:
  void AppendLog(const QString& line);
  void SetLastError(const QString& error);
  // Persists studio_settings.json immediately (write-through). Used by
  // behavioral setters (closeToTray, checkUpdates, lockscreenSync, …) whose
  // values must survive a crash before the next clean quit — the
  // aboutToQuit flush is a safety net, not the contract. SaveStudioSettings
  // directly (not apply()) because these setters touch only studio settings,
  // not config.json; apply() would rewrite the engine config as a side effect.
  void PersistStudioSettingsNow();

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
