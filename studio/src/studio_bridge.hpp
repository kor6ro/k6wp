#pragma once

// Phase 1 of the Widgets -> QML migration: the QML-facing backend.
//
// StudioBridge is the ONE object the QML tree talks to. It owns the same
// backend objects MainWindow owns (the single per-process IpcClient, the
// ApplyManager that borrows it, the Thumbnailer and the StudioSettings
// backing store) and re-exposes a small, read-only, QML-friendly surface on
// top: live engine status polled over the same IPC pipe, plus the preview
// invokables the shell needs.
//
// Registering: this class uses the QML macros (QML_ELEMENT + QML_SINGLETON)
// rather than qmlRegisterSingletonInstance. The macros let AUTOMOC generate
// the `K6WP` module's type registration as part of the target, so
// studio/CMakeLists.txt's qt_add_qml_module is the single place that knows
// about the module and there is no second registration call to keep in sync
// (the project also deliberately dropped i18n scaffolding -- see the MED-11
// note in studio/CMakeLists.txt -- so nothing else registers types).
//
// Because QML_SINGLETON types are instantiated by the QML engine itself, the
// bridge is ENGINE-owned, not owned by QmlShell, and is created lazily on the
// first property access. That is why the preview invokables do not touch a
// widget directly but forward through the process' active QmlShell (see
// qml_shell.hpp: SetActiveQmlShell / ActiveQmlShell).
//
// Threading (mirrors main_window.cpp verbatim, HIGH-3 slice B):
//   * every blocking IPC/process wait runs on a QtConcurrent worker and comes
//     back through a QFutureWatcher::finished slot on the GUI thread;
//   * a 1.5s QTimer drives one GetState per tick, and a tick that lands while
//     a poll is still in flight is SKIPPED (poll_busy_), never queued;
//   * DecideEngineStatus is the pure, unit-tested decision function and never
//     throws; it is reused, not reimplemented.

#include <QFutureWatcher>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>

#include <QtQml/qqmlregistration.h>

#include <atomic>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include "apply_manager.hpp"
#include "displays_schema.hpp"
#include "engine_status_controller.hpp"
#include "ipc_client.hpp"
#include "links.hpp"
#include "monitor_util.hpp"
#include "studio_settings.hpp"
#include "thumbnailer.hpp"

class QTimer;

namespace k6wp {

class UpdateChecker;

// Plan todo 17 / brief B10 + C-19: the Studio tray's engine-side actions.
// Defined in studio_bridge.cpp (NOT qml_shell.cpp) because studio_logic_test
// compiles studio_bridge.cpp but not qml_shell.cpp, and the IPC pause/resume
// + friendly status live on StudioBridge. QmlShell's QSystemTrayIcon
// registers here on construction and reads the copies at menu-popup time;
// StudioBridge::ApplyStatus pushes tooltip updates through the listener.
// Menu copy is C-19 verbatim: "Buka K6WP Studio / Jeda / Lanjut / Keluar" —
// deliberately NOT the engine tray's quick-switch MRU (engine_app.cpp).
struct StudioTrayHooks {
  std::function<void()> pause;          // "Jeda"
  std::function<void()> resume;         // "Lanjut"
  std::function<QString()> status_tip;  // friendly status, never "engine"
};
void SetStudioTrayHooks(StudioTrayHooks hooks);
// Clears the hooks; QmlShell's tray dtor and StudioBridge's dtor both call
// this so a dying side never leaves a dangling std::function.
void ClearStudioTrayHooks();
// Copies of the current hooks (empty std::function when unset). GUI-thread
// only; the tray reads them at menu-popup / action time.
StudioTrayHooks GetStudioTrayHooks();
// Called by StudioBridge::ApplyStatus after engineStatusChanged, so the tray
// tooltip tracks the live status without the shell polling IPC.
void NotifyStudioTrayStatusChanged();
// Tray installs this when it appears; cleared by ClearStudioTrayHooks.
void SetStudioTrayStatusListener(std::function<void()> listener);

// QML-facing mirror of EngineStatusView::Kind. The VALUES are the stable
// contract studio/qml/Main.qml switches on (it redeclares the same order as
// a QML enum); kBridgedStatusCount exists purely so the mapping can be
// asserted in studio_bridge.cpp instead of drifting silently.
enum class BridgeStatusKind {
  kConnected = 0,
  kPaused = 1,
  kDegraded = 2,
  kNotRunning = 3,
  kDisconnected = 4,
  kCount = 5,
};

// Worker payloads. Each is the structured return of one QtConcurrent::run
// body, so the GUI-thread slot can paint a result without touching the
// blocking client itself.

// startEngine(): graceful restart + the ready-wait, both blocking, both on
// one worker. Named EngineStartOutcome, not EngineStartResult, because
// main_window.hpp declares its own k6wp::EngineStartResult and both headers
// are compiled into the same target in this phase.
struct EngineStartOutcome {
  bool restarted = false;
  QString restart_error;
  bool ready = false;
};

// applyWallpaper(): the shared apply path. error is the engine's / manager's
// own message so the UI can show it verbatim instead of a generic failure.
struct ApplyOutcome {
  bool ok = false;
  bool restarted = false;
  QString error;
};

// --- Display model (row 19, read-only canvas data) ---------------------------
//
// Pure helpers the Display canvas is modelled on. They live beside the bridge
// (not inside it) so studio_logic_test can drive them with fixture monitor
// lists without constructing the full QML singleton.

// One QVariantMap per MonitorInfo, in list order, with exactly the keys the
// Display canvas consumes: {key, label, x, y, width, height, isPrimary,
// orientation ("portrait"|"landscape"), scalePercent, refreshHz,
// resolutionLabel, assignedPath, assignedExists, coverage}. `store` joins the
// per-monitor assignment map; `coverage` is the opaque get_state
// display_coverage token map (missing key -> empty string).
QVariantList BuildDisplayEntries(
    const std::vector<MonitorInfo>& monitors, const DisplaysConfig& store,
    const std::map<std::string, std::string>& coverage);

// IS-7 refusal sentence (Indonesian) naming the colliding keys; empty when
// `keys` is empty.
QString DuplicateModeNoticeText(const std::vector<std::wstring>& keys);

// True only when the parsed get_state carried display_capability == 1 (the
// feature-detect the engine emits; old engines leave the key absent).
bool DisplayCapabilityFromState(const EngineState& state);

// Plan todo 11 / brief C-14 + GATE 0 #3: the pure C-14 confirmation
// predicate. "Semua layar" is always safe to execute (monitor_id=-1), but
// the confirm dialog is required only when at least one per-key override
// would be replaced: assignment_count > 0. Beside the bridge so
// studio_logic_test can drive it without the QML singleton.
//   0 -> false (no overrides: apply immediately + toast)
//   2 -> true  (overrides exist: show C-14 "akan ikut diganti" dialog)
bool NeedsAllScreensConfirm(int assignment_count);

// Plan todo 9: playlist live snapshot decoded from the parsed get_state ack
// (playlist_enabled / playlist_size / playlist_index; engine_app already
// emits them). Absent or wrong-typed keys -> enabled false / size 0 /
// index -1. Beside the bridge so studio_logic_test can drive it with fixture
// ack payloads without a live engine.
struct PlaylistLiveState {
  bool enabled = false;
  int size = 0;
  int index = -1;
};
PlaylistLiveState PlaylistLiveFromState(const EngineState& state);

// --- friendly status surface (plan todo 4 / B2 / brief C-3 + C-20 + glossary §5) --
//
// Pure derivations the statusTitle / statusVideoName properties read. They
// live beside the bridge (not inside it) so studio_logic_test can drive
// them with fixture kind/detail pairs without constructing the QML
// singleton. `kind` is the BridgeStatusKind QML already switches on;
// `detail` is the live engineStatusDetail sentence (the video name is
// derived from its trailing segment); `video_active` mirrors the
// videoActive property.
//
// StatusTitleFor: the short state label, glossary §5 wording.
//   kConnected/kDegraded + video_active -> "Wallpaper aktif"
//   kConnected/kDegraded, no video      -> "Tidak aktif"
//   kPaused                              -> "Dijeda"
//   kNotRunning/kDisconnected            -> "Tidak aktif"
//   unknown kind + non-empty detail      -> `detail` (passthrough; never
//                                           invents a label)
//   unknown kind + empty detail          -> "Tidak aktif" (documented default)
//
// StatusVideoNameFor: the active video's display name, or empty when
// nothing is playing. Never invents a name: video_active false, the
// "(belum ada video aktif)" fallback, or a trailing separator with no
// segment all yield empty. `kind` is accepted for signature symmetry with
// StatusTitleFor and does not change the derivation.
QString StatusTitleFor(BridgeStatusKind kind, const QString& detail,
                       bool video_active);
QString StatusVideoNameFor(BridgeStatusKind kind, const QString& detail,
                           bool video_active);

// NOT `final`, unlike most classes in this project: Qt's QML type
// registration instantiates QQmlElement<T> which INHERITS from T, so a final
// QML_ELEMENT class does not compile (C3246). This is a hard Qt requirement,
// not a style regression.
class StudioBridge : public QObject {
  Q_OBJECT
  // Phase 1 module migration: reachable from QML as the `Studio` singleton
  // after `import K6WP` (QML_SINGLETON => one instance per QML engine).
  //
  // QML_NAMED_ELEMENT(Studio) is load-bearing: bare QML_ELEMENT registers the
  // type as `StudioBridge`, and Main.qml's `Studio.*` bindings would then hit
  // an undefined identifier WITHOUT any QML error -- every affected binding
  // just keeps its default, so the breakage is invisible.
  QML_NAMED_ELEMENT(Studio)
  QML_ELEMENT
  QML_SINGLETON

 public:
  // Default-constructible on purpose: the QML engine instantiates the
  // singleton itself and passes no engine pointer.
  explicit StudioBridge(QObject* parent = nullptr);
  ~StudioBridge() override;

  // --- QML surface ----------------------------------------------------------

  // Every property below is read-only. The status group shares one notify
  // signal because ApplyStatus repaints all of them together; `busy`, `log`
  // and the poster have their own.
  Q_PROPERTY(int engineStatusKind READ engineStatusKind NOTIFY engineStatusChanged)
  Q_PROPERTY(QString engineStatusDetail READ engineStatusDetail NOTIFY engineStatusChanged)
  Q_PROPERTY(QString engineStatusHint READ engineStatusHint NOTIFY engineStatusChanged)
  // Friendly status surface (B2 / todo 4): derived, not stored - see the
  // StatusTitleFor / StatusVideoNameFor free-function contract above.
  Q_PROPERTY(QString statusTitle READ statusTitle NOTIFY engineStatusChanged)
  Q_PROPERTY(QString statusVideoName READ statusVideoName NOTIFY engineStatusChanged)
  Q_PROPERTY(quint64 enginePid READ enginePid NOTIFY engineStatusChanged)
  Q_PROPERTY(bool videoActive READ videoActive NOTIFY engineStatusChanged)
  Q_PROPERTY(bool engineRunning READ engineRunning NOTIFY engineStatusChanged)
  Q_PROPERTY(QString activeVideoPath READ activeVideoPath NOTIFY engineStatusChanged)
  Q_PROPERTY(QString posterPath READ posterPath NOTIFY posterChanged)
  Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
  Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
  Q_PROPERTY(QString settingsPath READ settingsPath CONSTANT)
  Q_PROPERTY(QString version READ version CONSTANT)
  Q_PROPERTY(QString donateUrl READ donateUrl CONSTANT)
  Q_PROPERTY(QString projectUrl READ projectUrl CONSTANT)

  Q_PROPERTY(bool updateAvailable READ updateAvailable NOTIFY updateChanged)
  Q_PROPERTY(bool updateCheckBusy READ updateCheckBusy NOTIFY updateChanged)
  Q_PROPERTY(QString latestVersion READ latestVersion NOTIFY updateChanged)
  Q_PROPERTY(QString latestPageUrl READ latestPageUrl NOTIFY updateChanged)
  // Outcome of the last user-initiated check, for the QML label. Empty until
  // checkForUpdatesInteractive() runs: the automatic start-up check must not
  // put anything here.
  Q_PROPERTY(QString updateCheckMessage READ updateCheckMessage NOTIFY updateChanged)
  Q_INVOKABLE void checkForUpdates();
  Q_INVOKABLE void checkForUpdatesInteractive();
  Q_PROPERTY(QStringList log READ log NOTIFY logChanged)

  // The video picked in the Wallpaper tab, not yet applied. Empty when nothing
  // is picked. QML applies it explicitly via applyWallpaper().
  Q_PROPERTY(QString selectedVideo READ selectedVideo NOTIFY selectedVideoChanged)

  // Quick settings, mirrored from the engine config + the autostart registry.
  Q_PROPERTY(QString quickFit READ quickFit NOTIFY quickSettingsChanged)
  Q_PROPERTY(int quickMonitor READ quickMonitor NOTIFY quickSettingsChanged)
  Q_PROPERTY(bool quickAutostart READ quickAutostart NOTIFY quickSettingsChanged)
  Q_PROPERTY(bool quickBattery READ quickBattery NOTIFY quickSettingsChanged)
  // [{ "text": "Semua layar", "id": -1 }, ...] - index 0 is always "Semua
  // layar" (-1), then one entry per ListMonitors() row, primary first.
  Q_PROPERTY(QVariantList monitorChoices READ monitorChoices NOTIFY quickSettingsChanged)

  // Display model (row 19): the read-only monitor canvas data. Entries are
  // BuildDisplayEntries() maps (see the free-function contract above).
  // Nothing here is writable beyond assignVideoToMonitor /
  // clearMonitorAssignment.
  Q_PROPERTY(QVariantList displays READ displays NOTIFY displaysChanged)
  // True only when the last get_state carried display_capability:1.
  Q_PROPERTY(bool displayCapability READ displayCapability NOTIFY displaysChanged)
  // Non-empty Indonesian IS-7 refusal when DetectKeyCollision reports
  // colliding keys; empty otherwise.
  Q_PROPERTY(QString duplicateModeNotice READ duplicateModeNotice NOTIFY displaysChanged)
  // Plan todo 9: read-only playlist live state from the last get_state ack.
  // Repainted by engineStatusChanged with the rest of the status group.
  Q_PROPERTY(bool playlistLiveEnabled READ playlistLiveEnabled NOTIFY engineStatusChanged)
  Q_PROPERTY(int playlistLiveSize READ playlistLiveSize NOTIFY engineStatusChanged)
  Q_PROPERTY(int playlistLiveIndex READ playlistLiveIndex NOTIFY engineStatusChanged)

  QString selectedVideo() const { return selected_video_; }
  QString quickFit() const { return quick_fit_; }
  int quickMonitor() const { return quick_monitor_; }
  bool quickAutostart() const { return quick_autostart_; }
  bool quickBattery() const { return quick_battery_; }
  QVariantList monitorChoices() const { return monitor_choices_; }
  QVariantList displays() const { return displays_; }
  bool displayCapability() const { return display_capability_; }
  QString duplicateModeNotice() const { return duplicate_mode_notice_; }
  bool playlistLiveEnabled() const { return playlist_live_enabled_; }
  int playlistLiveSize() const { return playlist_live_size_; }
  int playlistLiveIndex() const { return playlist_live_index_; }

  // --- Live engine status (all repainted together by engineStatusChanged) ---

  // BridgeStatusKind as int (see the enum above for the exact order).
  int engineStatusKind() const { return static_cast<int>(status_.kind); }
  // The status sentence shown in the UI. Wording follows brief C-20 /
  // glossary §5 (plan todo 4):
  //   "Wallpaper aktif • <video>" / "Wallpaper aktif tapi tak tampil • <video>" /
  //   "Dijeda — <video>" / "Tidak aktif — pilih video untuk mulai" /
  //   "Terputus — coba lagi"
  QString engineStatusDetail() const { return status_detail_; }
  // Tooltip detail: the full video path (or the same "(belum ada video aktif)"
  // fallback), plus the headless-slot hint on kDegraded, plus the transport
  // error on kDisconnected. Empty while the first poll is still in flight.
  QString engineStatusHint() const { return status_hint_; }
  // Friendly state label / video name, derived from kind+detail+videoActive
  // via the pure free functions (B2). Repainted by engineStatusChanged.
  QString statusTitle() const {
    return StatusTitleFor(static_cast<BridgeStatusKind>(status_.kind),
                          status_detail_, video_active_);
  }
  QString statusVideoName() const {
    return StatusVideoNameFor(static_cast<BridgeStatusKind>(status_.kind),
                              status_detail_, video_active_);
  }
  // Engine PID from the get_state ack; 0 when unknown (not running, or the
  // ack carried no pid).
  quint64 enginePid() const { return status_.pid; }
  // True when the engine reports an active video path.
  bool videoActive() const { return video_active_; }
  // True for Connected/Paused/Degraded, i.e. the pipe answers and the
  // pause/resume invokables are allowed to run.
  bool engineRunning() const { return engine_running_; }
  // Full active-video path (empty when nothing is applied yet).
  QString activeVideoPath() const { return active_video_; }

  // --- Preview / poster -----------------------------------------------------

  // Cached thumbnail JPEG for the shell's preview poster, or empty while it
  // is still being generated (or when the file cannot be stated). Updated by
  // posterChanged once the worker finishes, so the QML Image never blocks the
  // GUI thread.
  QString posterPath() const { return poster_path_; }

  // --- Diagnostics ----------------------------------------------------------

  // Last failed action, in Indonesian, already safe to display. Cleared by
  // every successful action. This is the "surface it in the status text"
  // path: an IPC failure never vanishes, it lands here.
  QString lastError() const { return last_error_; }
  // True while a blocking engine op (start / apply) runs on a worker. Bind to
  // the busy indicator; do not infer busy-ness from lastError.
  bool busy() const { return busy_; }
  // %LOCALAPPDATA%/K6WP/studio_settings.json. Shown in the technical detail
  // line, exactly like the old path_label_ showed the config path. Phase 1
  // also loads the file once (self-healing, see the ctor) so a corrupt
  // settings file is reported at startup instead of at the first settings
  // write in a later phase.
  QString settingsPath() const { return settings_path_; }
  QString version() const { return version_; }
  QString donateUrl() const { return QString::fromUtf8(K6WP_DONATE_URL); }
  QString projectUrl() const { return QString::fromUtf8(K6WP_PROJECT_URL); }
  bool updateAvailable() const { return !latest_version_.isEmpty(); }
  bool updateCheckBusy() const { return update_busy_; }
  QString latestVersion() const { return latest_version_; }
  QString latestPageUrl() const { return latest_page_url_; }
  QString updateCheckMessage() const { return update_check_message_; }
  // The bridge's own log lines (Load() fallback reasons, apply results, the
  // preview poster hand-off). Backs the Wallpaper tab's "Detail teknis (log)"
  // text area, which the old UI read from MainWindow's log_view_.
  QStringList log() const { return log_; }

  // --- Invokables -----------------------------------------------------------

  // Publishes the chosen file to selectedVideo. Does NOT apply: the QML flow is
  // pick-then-apply, so "Terapkan Wallpaper" reads the selection afterwards.
  Q_INVOKABLE void pickVideo();
  // Task 27 (koleksi card select-only): publishes an ALREADY-KNOWN path (a
  // library card's dst) to selectedVideo, exactly like pickVideo does for the
  // dialog path. Still does NOT apply - a card click only selects, and the
  // explicit "Pasang" affordances (card hover button, row menu, right rail)
  // read selectedVideo / applyAt afterwards. Empty path is ignored; clearing
  // stays owned by clearSelectedVideo() ("Hapus").
  Q_INVOKABLE void selectVideo(const QString& path);
  Q_INVOKABLE void clearSelectedVideo();

  // --- Quick settings ("Pengaturan cepat") ----------------------------------
  //
  // Plain GUI-thread config writes, not workers: the engine's own config
  // watcher picks the change up, so there is no blocking call to move off the
  // GUI thread here.
  Q_INVOKABLE void setQuickFit(const QString& fit_mode);
  Q_INVOKABLE void setQuickMonitor(int monitor_id);
  Q_INVOKABLE void setQuickAutostart(bool enabled);
  Q_INVOKABLE void setQuickBattery(bool enabled);

  // Re-reads the quick settings and the monitor list from disk. QML calls it
  // when the Wallpaper tab is shown, so a Pengaturan change shows up here.
  Q_INVOKABLE void refreshQuickSettings();

  // --- Display model (row 19) ------------------------------------------------

  // Re-reads the display model from ListMonitors() + displays.json + the
  // cached get_state display_coverage, then rebuilds displays_ /
  // duplicate_mode_notice_ and emits displaysChanged.
  Q_INVOKABLE void refreshDisplays();

  // Persists the assignment via SaveDisplays (displays.json), then pushes
  // set_display_video over IPC. An unknown key is refused with an Indonesian
  // lastError: no file write, no displaysChanged.
  Q_INVOKABLE void assignVideoToMonitor(const QString& key, const QString& path);

  // Drops the assignment for `key`: SaveDisplays + a clear
  // set_display_video push. Unknown key refuses exactly like assign.
  Q_INVOKABLE void clearMonitorAssignment(const QString& key);

  // Plan todo 11 / brief B7 + C-14 + GATE 0 #3: one-shot "Pasang ke semua
  // layar". ONE C++ action: setQuickMonitor(-1) (global monitor_id) +
  // clearMonitorAssignment for every key with an active assignment in the
  // displays model, each following the existing persist + best-effort IPC
  // pattern. Returns the number of overrides cleared — the CALLER uses it
  // to decide the C-14 confirmation (NeedsAllScreensConfirm(count)). Does
  // NOT play the video: the caller installs it via applyWallpaper(path);
  // this function only moves the target + clears overrides. Empty displays
  // model -> returns 0, no crash.
  Q_INVOKABLE int applyToAllMonitors(const QString& path);

  // Brief B9 / C-15: open Windows Display Settings (ms-settings:display)
  // via ShellExecuteW. Best-effort: a launch failure is logged, not raised
  // as lastError (nothing user-actionable Studio can do about it).
  Q_INVOKABLE void openWindowsDisplaySettings();

  // The one place displays_ / duplicate_mode_notice_ are rebuilt.
  // refreshDisplays() feeds it live data; studio_logic_test feeds fixtures.
  void ApplyDisplayModel(const std::vector<MonitorInfo>& monitors,
                         const DisplaysConfig& store,
                         const std::map<std::string, std::string>& coverage);

  // The one place playlistLive* is rebuilt from a parsed get_state.
  // OnPollDone feeds it live data; studio_logic_test feeds fixtures.
  void ApplyPlaylistLive(const PlaylistLiveState& live);

  // Fires one GetState round-trip on a worker and repaints the properties
  // above when it lands. A no-op while a poll is in flight. The QTimer calls
  // this every 1500 ms; QML may call it too (e.g. after pause/resume).
  Q_INVOKABLE void requestStatusPoll();

  // Graceful engine restart (ApplyManager::RestartEngine) followed by the
  // cancellable ready-wait. Blocking -> runs on a worker; the result arrives
  // as busy=false + lastError + a fresh status poll.
  Q_INVOKABLE void startEngine();

  // The shared apply path: preserve every existing config field, replace only
  // the video, write the engine config, then live-switch over IPC (with the
  // manager's built-in restart recovery when the engine is not listening).
  // Blocking -> worker. An empty or missing path is refused with lastError set.
  Q_INVOKABLE void applyWallpaper(const QString& path);

  // Size in whole MB when `path` is over the compress-first threshold, else -1.
  // QML asks this before applying so a large video can be offered to the
  // compressor first; the offer itself and its wording live in Main.qml.
  Q_INVOKABLE qint64 compressFirstOfferMb(const QString& path) const;

  // IPC pause / resume, then an immediate status refresh so the label flips
  // without waiting up to 1.5s. Workers, like every other blocking call.
  Q_INVOKABLE void pause();
  Q_INVOKABLE void resume();
  // B3 / todo 4: one QML button. Pauses when the engine is active, resumes
  // when paused, reusing the pause/resume IPC path above. Consults the live
  // status kind - never blindly pauses. No-op when the pipe is dead
  // (kNotRunning / kDisconnected): nothing to toggle.
  Q_INVOKABLE void togglePause();
  // B9 / todo 4: puts `text` on the system clipboard for "Salin untuk
  // dukungan". Empty text is ignored (a dialog can open before any
  // technical detail exists) and a missing QGuiApplication clipboard is a
  // silent no-op - neither path may crash.
  Q_INVOKABLE void copyToClipboard(const QString& text);

  // Requests the preview poster for `videoPath` (usually activeVideoPath()).
  // Non-blocking: the cached thumbnail is returned immediately when present,
  // otherwise a worker generates it and posterPath() updates when it lands.
  Q_INVOKABLE void requestPoster(const QString& videoPath);

  // --- Preview widget plumbing (forwarded to the active QmlShell) -----------

  // Called from the placeholder Item's x/y/width/height change handlers in
  // studio/qml/Main.qml with QML SCENE coordinates. Converts them to
  // QMainWindow content coordinates and moves the native PreviewWidget there
  // (it is a real QWidget on top of the QQuickWidget, not a QQuickItem).
  // Values <= 0 in w/h hide the preview rather than creating a 0x0 HWND.
  Q_INVOKABLE void syncPreviewGeometry(int x, int y, int w, int h);

  // Loads `path` into the native preview (PreviewWidget::LoadVideo) and keeps
  // mpv playing. Used for the engine's active video.
  Q_INVOKABLE void loadPreview(const QString& path);

  // PreviewWidget::SetPaused passthrough so the QML play/pause button drives
  // mpv without QML needing to know the widget exists.
  Q_INVOKABLE void setPreviewPaused(bool paused);

 signals:
  // One signal for the whole engine-status group: kind / detail / hint / pid /
  // videoActive / running / activeVideoPath are repainted together by
  // ApplyStatus, so a single notify keeps the QML bindings consistent (and
  // avoids seven re-evaluations racing each other).
  void engineStatusChanged();
  void posterChanged();
  void lastErrorChanged();
  void busyChanged();
  void logChanged();
  void selectedVideoChanged();
  void updateChanged();
  // Repaints quickFit / quickMonitor / quickAutostart / quickBattery /
  // monitorChoices together.
  void quickSettingsChanged();
  // Repaints displays / displayCapability / duplicateModeNotice together.
  void displaysChanged();

 private slots:
  // QFutureWatcher::finished handlers. All run on the GUI thread (queued).
  void OnPollDone();
  void OnEngineStartDone();
  void OnApplyDone();
  void OnPauseResumeDone();
  void OnPosterDone();

 private:
  // Single place where an IpcResult becomes the QML-facing state: the pure
  // decision function, then the Indonesian sentences MainWindow paints.
  void ApplyStatus(const EngineStatusView& view);
  void SetLastError(const QString& error);
  void ClearLastError();
  void AppendLog(const QString& line);
  // Runs a blocking pause/resume on a worker, tagging which one it was so the
  // finished slot knows what to log (the MainWindow::pending_pause_op_ idea).
  void RunPauseResume(bool do_pause);
  void RunUpdateCheck(bool interactive);
  // True when `key` is one of the keys currently modelled in displays_ -
  // the assign/clear unknown-key guard (QML only ever offers modelled keys).
  bool IsKnownDisplayKey(const QString& key) const;
  // Writes the fresh get_state display_coverage tokens onto the existing
  // entries (geometry stays whatever ApplyDisplayModel last built).
  void MergeCoverageIntoDisplays();

  // T15: the ONE IpcClient of this Studio process, a plain value member --
  // IpcClient is NOT a QObject and must never be given Q_PROPERTY access.
  // ApplyManager borrows it via SetIpcClient in the ctor and serializes
  // against it in Send's mutex, exactly like MainWindow.
  IpcClient ipc_;
  // Value member parented to `this` (MainWindow's apply_manager_{this}
  // pattern): QObject parent-child owns the lifetime, the member owns the
  // storage, and the block is destructed only after the last watcher fired.
  ApplyManager apply_manager_{this};
  // Plain value members (not QObjects, no Q_PROPERTY): the shared thumbs cache
  // for the shell's preview poster, and Studio's own preferences.
  Thumbnailer thumb_;
  StudioSettings settings_;
  QString settings_path_;
  QString version_ = QStringLiteral(K6WP_VERSION_STR);
  UpdateChecker* update_checker_ = nullptr;
  QString latest_version_;
  QString latest_page_url_;
  QString update_check_message_;
  bool update_busy_ = false;
  bool update_check_interactive_ = false;

  // HIGH-3 slice B plumbing, mirrored from MainWindow: 1.5s poll timer, a
  // poll_busy_ re-entrancy guard so a slow/dead engine stacks nothing up, an
  // atomic busy flag for the blocking engine ops, and one watcher per blocking
  // path so every worker result lands on the GUI thread.
  QTimer* poll_timer_ = nullptr;
  QFutureWatcher<IpcResult>* poll_watcher_ = nullptr;
  QFutureWatcher<EngineStartOutcome>* start_watcher_ = nullptr;
  QFutureWatcher<ApplyOutcome>* apply_watcher_ = nullptr;
  QFutureWatcher<IpcResult>* pause_watcher_ = nullptr;
  QFutureWatcher<QString>* poster_watcher_ = nullptr;
  bool poll_busy_ = false;
  bool poster_busy_ = false;
  // Latest poster request that arrived while one was already running; re-issued
  // when the in-flight job finishes so the wrong video's poster is never left.
  QString pending_poster_video_;
  // 0 = no pause/resume in flight, 1 = pause, 2 = resume (MainWindow's
  // pending_pause_op_).
  int pending_pause_op_ = 0;
  // Path the in-flight apply is for; the preview follows it on success.
  QString pending_apply_path_;
  // Last video handed to the native preview, so the steady-state 1.5s poll
  // does not re-issue LoadVideo on every tick.
  QString last_preview_video_;
  // Worker-cancel flag for the blocking waits inside startEngine /
  // applyWallpaper (RestartEngine's 8s process wait, the pipe-ready poll and
  // the ready-wait). Set on the GUI thread, read on the worker in short
  // (<=100ms) slices; never TerminateThread.
  std::atomic<bool> op_cancel_{false};

  // Repainted state (see the property getters).
  EngineStatusView status_;
  QString status_detail_;
  QString status_hint_;
  QString active_video_;
  bool video_active_ = false;
  bool engine_running_ = false;
  QString poster_path_;
  QString last_error_;
  bool busy_ = false;
  QStringList log_;
  QString selected_video_;
  QString quick_fit_;
  int quick_monitor_ = -1;
  bool quick_autostart_ = false;
  bool quick_battery_ = false;
  QVariantList monitor_choices_;
  // Display model (row 19). display_capability_/display_coverage_ come from
  // the last get_state (OnPollDone); last_monitors_ is the monitor list the
  // current displays_ entries were built from, so assign/clear can rebuild
  // the model after a successful persist without re-enumerating.
  QVariantList displays_;
  bool display_capability_ = false;
  QString duplicate_mode_notice_;
  std::map<std::string, std::string> display_coverage_;
  std::vector<MonitorInfo> last_monitors_;
  // Plan todo 9: playlist live snapshot from the last get_state (see the
  // PlaylistLiveState contract above).
  bool playlist_live_enabled_ = false;
  int playlist_live_size_ = 0;
  int playlist_live_index_ = -1;
};

}  // namespace k6wp
