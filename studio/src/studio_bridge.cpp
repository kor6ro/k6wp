// Phase 1 of the Widgets -> QML migration: the QML-facing backend.
// See studio_bridge.hpp for the design notes; this file is the QML-visible
// behaviour and deliberately mirrors main_window.cpp's HIGH-3 slice B
// threading (1.5s QTimer -> QtConcurrent worker -> QFutureWatcher slot).

#include "studio_bridge.hpp"
#include "bridge_diagnostics.hpp"

#include <QClipboard>
#include <QCoreApplication>
#include <QFileDialog>
#include <QFileInfo>
#include <QGuiApplication>
#include <QTimer>
#include <QVariantMap>

#include <QtConcurrent>

#include <cassert>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shellapi.h>

#include "autostart.hpp"
#include "compress_first_offer.hpp"
#include "config_schema.hpp"
#include "displays_schema.hpp"
#include "engine_status_controller.hpp"
#include "links.hpp"
#include "monitor_util.hpp"
#include "qml_shell.hpp"
#include "update_checker.hpp"
#include "user_errors.hpp"

namespace k6wp {

// Plan todo 17 / B10: Studio tray hooks (see studio_bridge.hpp). One process-
// global slot pair; the tray reads copies at menu-popup time so the menu
// never holds a live reference across a bridge rebuild.
namespace {
StudioTrayHooks g_studio_tray_hooks;
std::function<void()> g_tray_status_listener;
}  // namespace

void SetStudioTrayHooks(StudioTrayHooks hooks) { g_studio_tray_hooks = std::move(hooks); }

void ClearStudioTrayHooks() {
  g_studio_tray_hooks = StudioTrayHooks{};
  g_tray_status_listener = nullptr;
}

StudioTrayHooks GetStudioTrayHooks() { return g_studio_tray_hooks; }

void SetStudioTrayStatusListener(std::function<void()> listener) {
  g_tray_status_listener = std::move(listener);
}

void NotifyStudioTrayStatusChanged() {
  if (g_tray_status_listener) {
    g_tray_status_listener();
  }
}

namespace {

// Reads a finished watcher's value, absorbing an exception that escaped the
// QtConcurrent lambda.
//
// QFutureWatcher::result() re-THROWS whatever left the worker. Nothing here is
// expected to throw: IpcClient reports every failure through IpcResult (Send
// catches IpcError and std::exception internally and returns kError /
// kNotRunning) and DecideEngineStatus is documented never to throw. But an
// exception unwinding out of a slot on a QML-facing object would take the GUI
// thread down with no explanation at all, so it becomes an explicit error
// value the caller can display. The catch blocks are not empty: they fill
// *error_out (possibly with an empty string, meaning "the exception carried
// no message") and report failure so the caller supplies its own wording.
template <typename T>
bool TakeResult(QFutureWatcher<T>* watcher, T* out, QString* error_out) {
  try {
    *out = watcher->result();
    return true;
  } catch (const std::exception& e) {
    *error_out = QString::fromUtf8(e.what());
    return false;
  } catch (...) {
    *error_out = QString();
    return false;
  }
}

}  // namespace

// --- display model (row 19) --------------------------------------------------

QVariantList BuildDisplayEntries(
    const std::vector<MonitorInfo>& monitors, const DisplaysConfig& store,
    const std::map<std::string, std::string>& coverage) {
  QVariantList out;
  for (const MonitorInfo& m : monitors) {
    const std::string key8 = std::filesystem::path(m.device_name).u8string();
    QString label =
        QCoreApplication::translate("StudioBridge", "Layar %1").arg(m.id);
    if (m.is_primary) {
      label += QCoreApplication::translate("StudioBridge", " (utama)");
    }
    const auto assign_it = store.assignments.find(m.device_name);
    const bool has_assign = assign_it != store.assignments.end();
    const auto cov_it = coverage.find(key8);
    out.append(QVariantMap{
        {QStringLiteral("key"), QString::fromStdWString(m.device_name)},
        {QStringLiteral("label"), label},
        {QStringLiteral("x"), m.x},
        {QStringLiteral("y"), m.y},
        {QStringLiteral("width"), m.width},
        {QStringLiteral("height"), m.height},
        {QStringLiteral("isPrimary"), m.is_primary},
        {QStringLiteral("orientation"), m.IsPortrait()
                                            ? QStringLiteral("portrait")
                                            : QStringLiteral("landscape")},
        {QStringLiteral("scalePercent"), m.ScalePercent()},
        {QStringLiteral("refreshHz"), m.refresh_hz},
        {QStringLiteral("resolutionLabel"),
         QStringLiteral("%1x%2").arg(m.width).arg(m.height)},
        {QStringLiteral("assignedPath"), has_assign
                                             ? QString::fromStdWString(
                                                   assign_it->second.path)
                                             : QString()},
        {QStringLiteral("assignedExists"),
         has_assign && assign_it->second.exists},
        {QStringLiteral("coverage"),
         cov_it != coverage.end() ? QString::fromStdString(cov_it->second)
                                  : QString()},
    });
  }
  return out;
}

QString DuplicateModeNoticeText(const std::vector<std::wstring>& keys) {
  if (keys.empty()) {
    return QString();
  }
  QStringList names;
  names.reserve(static_cast<int>(keys.size()));
  for (const std::wstring& k : keys) {
    names.append(QString::fromStdWString(k));
  }
  return QCoreApplication::translate(
             "StudioBridge",
             "Mode duplikat terdeteksi (%1). Penugasan video per layar "
             "dinonaktifkan sampai tampilan Windows diubah ke mode Perluas.")
      .arg(names.join(QStringLiteral(", ")));
}

bool DisplayCapabilityFromState(const EngineState& state) {
  return state.display_capability == 1;
}

bool NeedsAllScreensConfirm(int assignment_count) {
  return assignment_count > 0;
}

PlaylistLiveState PlaylistLiveFromState(const EngineState& state) {
  PlaylistLiveState out;
  out.enabled = state.playlist_enabled;
  out.size = state.playlist_size < 0 ? 0 : state.playlist_size;
  out.index = state.playlist_index;
  return out;
}

// --- friendly status surface (plan todo 4 / B2 / brief C-3 + C-20 + glossary §5) --

QString StatusVideoNameFor(BridgeStatusKind kind, const QString& detail,
                           bool video_active) {
  Q_UNUSED(kind);
  if (!video_active) {
    return QString();
  }
  const QStringList separators{QStringLiteral(" • "), QStringLiteral(" — "),
                               QStringLiteral(" - ")};
  for (const QString& sep : separators) {
    const int idx = detail.lastIndexOf(sep);
    if (idx >= 0) {
      const QString name = detail.mid(idx + sep.size()).trimmed();
      if (name.isEmpty() ||
          name == QStringLiteral("(belum ada video aktif)")) {
        return QString();
      }
      return name;
    }
  }
  return QString();
}

QString StatusTitleFor(BridgeStatusKind kind, const QString& detail,
                       bool video_active) {
  switch (kind) {
    case BridgeStatusKind::kConnected:
    case BridgeStatusKind::kDegraded:
      // Degraded still paints the wallpaper as active in the detail line
      // ("tapi tak tampil"), so the title agrees with the engine being alive.
      return video_active
                 ? QCoreApplication::translate("StudioBridge", "Wallpaper aktif")
                 : QCoreApplication::translate("StudioBridge", "Tidak aktif");
    case BridgeStatusKind::kPaused:
      return QCoreApplication::translate("StudioBridge", "Dijeda");
    case BridgeStatusKind::kNotRunning:
    case BridgeStatusKind::kDisconnected:
      return QCoreApplication::translate("StudioBridge", "Tidak aktif");
    case BridgeStatusKind::kCount:
    default:
      // Unknown kind (a future EngineStatusView::Kind): never invent a
      // label - hand the raw detail through so nothing user-visible is
      // lost; empty detail falls back to the idle label.
      return detail.isEmpty()
                 ? QCoreApplication::translate("StudioBridge", "Tidak aktif")
                 : detail;
  }
}

StudioBridge::StudioBridge(QObject* parent) : QObject(parent) {
  // T15: ONE IpcClient per Studio process. It is a plain value member (not a
  // QObject) and is only ever touched from a worker thread; ApplyManager
  // borrows the very same object and serializes against it inside Send, so
  // a poll and an apply can never interleave bytes on the pipe.
  apply_manager_.SetIpcClient(&ipc_);

  // Studio preferences. Same self-healing contract as
  // MainWindow::LoadStudioSettingsOrDefault, minus the first-run wizard: a
  // missing file is silent (defaults, file stays absent), a corrupt one is
  // reported so Phase 1 surfaces it at startup instead of at the first
  // settings write in a later phase. Phase 1 only reads the path, but loading
  // here keeps the file validated and migrated exactly once.
  settings_ = DefaultStudioSettings();
  try {
    settings_path_ =
        QString::fromStdWString(DefaultStudioSettingsPath().wstring());
    std::error_code ec;
    if (std::filesystem::exists(DefaultStudioSettingsPath(), ec) && !ec) {
      try {
        settings_ = LoadStudioSettings(DefaultStudioSettingsPath());
      } catch (const ConfigError& e) {
        // LoadStudioSettings already preserved the bad bytes as
        // studio_settings.json.bak - fall back to defaults and log.
        settings_ = DefaultStudioSettings();
        AppendLog(QStringLiteral("Pengaturan studio rusak, kembali ke bawaan: %1")
                      .arg(QString::fromUtf8(e.what())));
      }
    }
  } catch (const ConfigError&) {
    // No resolvable LOCALAPPDATA: keep built-in defaults silently.
    settings_path_.clear();
  }

  // The apply manager's own step log feeds the QML log view, so the QML side
  // shows the same detail the old log_view_ did.
  connect(&apply_manager_, &ApplyManager::LogMessage, this,
          &StudioBridge::AppendLog);

  // Status poll. Interval and the immediate first poll are MainWindow's
  // (1500 ms, <=2s budget); a tick that lands while a poll is in flight is
  // skipped by poll_busy_, never queued.
  poll_timer_ = new QTimer(this);
  poll_timer_->setInterval(1500);
  connect(poll_timer_, &QTimer::timeout, this, &StudioBridge::requestStatusPoll);

  // One watcher per blocking path. Each is a Qt child, so finished() fires
  // queued on the GUI thread and the slots below can touch QML properties
  // safely while the workers block.
  poll_watcher_ = new QFutureWatcher<IpcResult>(this);
  connect(poll_watcher_, &QFutureWatcher<IpcResult>::finished, this,
          &StudioBridge::OnPollDone);
  start_watcher_ = new QFutureWatcher<EngineStartOutcome>(this);
  connect(start_watcher_, &QFutureWatcher<EngineStartOutcome>::finished, this,
          &StudioBridge::OnEngineStartDone);
  apply_watcher_ = new QFutureWatcher<ApplyOutcome>(this);
  connect(apply_watcher_, &QFutureWatcher<ApplyOutcome>::finished, this,
          &StudioBridge::OnApplyDone);
  pause_watcher_ = new QFutureWatcher<IpcResult>(this);
  connect(pause_watcher_, &QFutureWatcher<IpcResult>::finished, this,
          &StudioBridge::OnPauseResumeDone);
  poster_watcher_ = new QFutureWatcher<QString>(this);
  connect(poster_watcher_, &QFutureWatcher<QString>::finished, this,
          &StudioBridge::OnPosterDone);

  poll_timer_->start();
  requestStatusPoll();
  refreshQuickSettings();
  if (settings_.check_updates) {
    checkForUpdates();
  }
  // Plan todo 17: publish the C-19 tray hooks. QmlShell's tray reads copies
  // at menu-popup time; this registration is what makes Jeda/Lanjut/status
  // live once the (optional) tray exists.
  SetStudioTrayHooks(
      {[this]() { pause(); },
       [this]() { resume(); },
       [this]() { return statusTitle(); }});
}

StudioBridge::~StudioBridge() {
  // Plan todo 17: drop the tray hooks before members die so a still-visible
  // tray menu can never invoke a destroyed bridge.
  ClearStudioTrayHooks();
  // The QML engine owns this singleton, so the destruction order is: engine
  // teardown -> here. Stop the timer so no NEW poll launches, then cancel and
  // drain every in-flight worker BEFORE members die: each lambda captures
  // `this` and touches ipc_/thumb_, and the watcher QObject children are
  // destroyed after the members, so a still-running worker would use freed
  // state. RequestCancel() aborts the cancellable IPC waits quickly.
  poll_timer_->stop();
  ipc_.RequestCancel();
  if (poll_watcher_) poll_watcher_->waitForFinished();
  if (start_watcher_) start_watcher_->waitForFinished();
  if (apply_watcher_) apply_watcher_->waitForFinished();
  if (pause_watcher_) pause_watcher_->waitForFinished();
  if (poster_watcher_) poster_watcher_->waitForFinished();
}

// --- status ----------------------------------------------------------------

void StudioBridge::requestStatusPoll() {
  // Re-entrancy guard (MainWindow::poll_busy_): a slow or dead engine must
  // never stack polls up on the global thread pool.
  if (poll_busy_) {
    return;
  }
  poll_busy_ = true;
  // The GUI thread only hands the worker a reference to the shared client;
  // the GetState transaction itself runs off-thread.
  poll_watcher_->setFuture(QtConcurrent::run([this]() {
    return ipc_.GetState();
  }));
}

void StudioBridge::OnPollDone() {
  IpcResult res;
  QString thrown;
  const bool ok_read = TakeResult(poll_watcher_, &res, &thrown);
  poll_busy_ = false;
  if (!ok_read) {
    AppendLog(thrown.isEmpty() ? QStringLiteral("Poll status gagal") : thrown);
    // Paint a disconnected state without a view to derive it from.
    EngineStatusView failed;
    failed.kind = EngineStatusView::Kind::kDisconnected;
    ApplyStatus(failed);
    display_capability_ = false;
    display_coverage_.clear();
    ApplyPlaylistLive(PlaylistLiveState{});
    MergeCoverageIntoDisplays();
    emit displaysChanged();
    return;
  }
  // The status DECISION is EngineStatusController::DecideEngineStatus (pure,
  // unit-tested by studio_logic_test); this class only maps the resulting view
  // onto QML properties and the Indonesian sentences MainWindow painted.
  ApplyStatus(DecideEngineStatus(res));
  // Row 19 + plan todo 9: the additive display + playlist fields ride the
  // same get_state ack. DecideEngineStatus parses EngineState internally but
  // only carries the status view across, so those fields are parsed once more
  // here (ParseEngineState never throws and defaults old-engine absences to
  // capability 0 / empty maps / playlist off).
  const EngineState state = ParseEngineState(res.raw);
  ApplyPlaylistLive(PlaylistLiveFromState(state));
  display_capability_ = DisplayCapabilityFromState(state);
  display_coverage_ = state.display_coverage;
  MergeCoverageIntoDisplays();
  emit displaysChanged();
}

void StudioBridge::ApplyStatus(const EngineStatusView& view) {
  // The C++ -> QML kind mapping is a hard contract (studio/qml/Main.qml
  // switches on these ints). Fail the build rather than silently render the
  // wrong branch if EngineStatusView grows a kind without a twin here.
  assert(static_cast<int>(view.kind) >= 0 &&
         static_cast<int>(view.kind) <
             static_cast<int>(BridgeStatusKind::kCount));
  assert(static_cast<int>(EngineStatusView::Kind::kConnected) ==
         static_cast<int>(BridgeStatusKind::kConnected));
  assert(static_cast<int>(EngineStatusView::Kind::kPaused) ==
         static_cast<int>(BridgeStatusKind::kPaused));
  assert(static_cast<int>(EngineStatusView::Kind::kDegraded) ==
         static_cast<int>(BridgeStatusKind::kDegraded));
  assert(static_cast<int>(EngineStatusView::Kind::kNotRunning) ==
         static_cast<int>(BridgeStatusKind::kNotRunning));
  assert(static_cast<int>(EngineStatusView::Kind::kDisconnected) ==
         static_cast<int>(BridgeStatusKind::kDisconnected));

  status_ = view;
  // The pipe answers => pause/resume are meaningful. kDegraded still counts:
  // the engine is alive, it just has a headless slot.
  engine_running_ = view.kind == EngineStatusView::Kind::kConnected ||
                    view.kind == EngineStatusView::Kind::kPaused ||
                    view.kind == EngineStatusView::Kind::kDegraded;
  active_video_ = view.video;
  video_active_ = !view.video.isEmpty();

  // Wording copied verbatim from MainWindow::OnPollDone so Phase 1 speaks
  // exactly like the widget UI did.
  if (engine_running_) {
    const QString short_name =
        view.video.isEmpty()
            ? tr("(belum ada video aktif)")
            : QFileInfo(view.video).fileName();
    if (view.kind == EngineStatusView::Kind::kPaused) {
      status_detail_ = tr("Dijeda — %1").arg(short_name);
    } else if (view.kind == EngineStatusView::Kind::kDegraded) {
      status_detail_ =
          tr("Wallpaper aktif tapi tak tampil • %1").arg(short_name);
    } else {
      status_detail_ = tr("Wallpaper aktif • %1").arg(short_name);
    }
    QString tip_detail =
        view.video.isEmpty() ? tr("(belum ada video aktif)") : view.video;
    if (view.kind == EngineStatusView::Kind::kDegraded) {
      tip_detail += tr("\n%1 slot tak menempel (headless) — cek engine.log")
                        .arg(view.headless > 0 ? view.headless : 1);
    }
    status_hint_ = tr("pid %1\n%2").arg(view.pid).arg(tip_detail);
  } else if (view.kind == EngineStatusView::Kind::kNotRunning) {
    status_detail_ = tr("Tidak aktif — pilih video untuk mulai");
    status_hint_ = tr("Wallpaper tidak aktif");
  } else {
    status_detail_ = tr("Terputus — coba lagi");
    // The transport error is the "never swallow an IPC failure" path, so it
    // reaches the log verbatim while the tooltip gets a mapped sentence.
    const QString technical = QString::fromStdString(view.error);
    AppendLog(technical);
    status_hint_ = FriendlyIpcError(technical);
  }
  emit engineStatusChanged();
  // Plan todo 17: refresh the Studio tray tooltip (friendly statusTitle,
  // never the word "engine") whenever the polled status repaints.
  NotifyStudioTrayStatusChanged();

  // The native preview follows the engine's active video - that is the whole
  // reason PreviewWidget is embedded in the shell. Only on change, so the
  // steady-state 1.5s poll does not re-issue LoadVideo every tick.
  if (active_video_ != last_preview_video_) {
    last_preview_video_ = active_video_;
    if (!active_video_.isEmpty()) {
      loadPreview(active_video_);
      requestPoster(active_video_);
    }
  }
}

void StudioBridge::SetLastError(const QString& error) {
  if (!SetChangedError(last_error_, error)) {
    return;
  }
  emit lastErrorChanged();
  if (!error.isEmpty()) {
    AppendLog(error);
  }
}

void StudioBridge::ClearLastError() {
  SetLastError(QString());
}

void StudioBridge::AppendLog(const QString& line) {
  AppendCapped(log_, line, 500);
  emit logChanged();
}

// --- video selection --------------------------------------------------------

void StudioBridge::pickVideo() {
  // Same filter the Widgets dialogs used (wallpaper_tab_widget / import_dialog).
  const QString path = QFileDialog::getOpenFileName(
      nullptr, tr("Pilih Video"), QString(),
      tr("Video (*.mp4 *.webm *.avi *.mkv *.mov *.wmv);;"
         "Semua File (*)"));
  if (path.isEmpty()) {
    return;  // cancelled: leave the selection untouched
  }
  if (selected_video_ == path) {
    return;
  }
  selected_video_ = path;
  AppendLog(QStringLiteral("Dipilih: %1").arg(path));
  emit selectedVideoChanged();
}

// Task 27: the card click / Enter / Space path. Same publish semantics as
// pickVideo (never applies, logs "Dipilih: ..."), but for a path the caller
// already holds. An empty path keeps the current selection untouched.
void StudioBridge::selectVideo(const QString& path) {
  if (path.isEmpty()) {
    return;
  }
  if (selected_video_ == path) {
    return;
  }
  selected_video_ = path;
  AppendLog(QStringLiteral("Dipilih: %1").arg(path));
  emit selectedVideoChanged();
}

void StudioBridge::clearSelectedVideo() {
  if (selected_video_.isEmpty()) {
    return;
  }
  AppendLog(
      QStringLiteral("Video saat ini dihapus dari daftar: %1").arg(selected_video_));
  selected_video_.clear();
  emit selectedVideoChanged();
}

// --- quick settings ---------------------------------------------------------

void StudioBridge::refreshQuickSettings() {
  WallpaperConfig cfg;
  try {
    cfg = LoadConfig(DefaultConfigPath());
  } catch (const ConfigError&) {
    // A missing or corrupt config leaves the built-in defaults in place, which
    // is what the Widgets quick-settings refresh did too.
  }
  quick_fit_ = QString::fromStdString(cfg.fit_mode);
  quick_monitor_ = cfg.monitor_id;
  quick_battery_ = cfg.battery_saver;
  quick_autostart_ = IsAutostart();

  QVariantList choices;
  choices.append(QVariantMap{{QStringLiteral("text"),
                              tr("Semua layar")},
                             {QStringLiteral("id"), -1}});
  for (const MonitorInfo& m : ListMonitors()) {
    choices.append(QVariantMap{
        {QStringLiteral("text"),
         tr("Layar %1 — %2x%3%4")
             .arg(m.id)
             .arg(m.width)
             .arg(m.height)
             .arg(m.is_primary ? tr(" (utama)") : QString())},
        {QStringLiteral("id"), m.id}});
  }
  monitor_choices_ = choices;
  emit quickSettingsChanged();
}

// --- display model (row 19) --------------------------------------------------

void StudioBridge::refreshDisplays() {
  std::vector<MonitorInfo> monitors = ListMonitors();
  DisplaysConfig store;
  try {
    store = LoadDisplays(DefaultDisplaysPath());
  } catch (const ConfigError&) {
    // Missing or corrupt displays.json: the same empty-store contract the
    // engine applies when it cannot read the file (engine_app.cpp).
    store = DisplaysConfig{};
  }
  ApplyDisplayModel(monitors, store, display_coverage_);
}

void StudioBridge::ApplyDisplayModel(
    const std::vector<MonitorInfo>& monitors, const DisplaysConfig& store,
    const std::map<std::string, std::string>& coverage) {
  last_monitors_ = monitors;
  displays_ = BuildDisplayEntries(monitors, store, coverage);
  duplicate_mode_notice_ =
      DuplicateModeNoticeText(DetectKeyCollision(store, monitors));
  emit displaysChanged();
}

void StudioBridge::ApplyPlaylistLive(const PlaylistLiveState& live) {
  playlist_live_enabled_ = live.enabled;
  playlist_live_size_ = live.size;
  playlist_live_index_ = live.index;
  emit engineStatusChanged();
}

bool StudioBridge::IsKnownDisplayKey(const QString& key) const {
  for (const QVariant& v : displays_) {
    if (v.toMap().value(QStringLiteral("key")).toString() == key) {
      return true;
    }
  }
  return false;
}

void StudioBridge::MergeCoverageIntoDisplays() {
  for (QVariant& v : displays_) {
    QVariantMap m = v.toMap();
    const std::string key8 =
        m.value(QStringLiteral("key")).toString().toStdString();
    const auto it = display_coverage_.find(key8);
    m[QStringLiteral("coverage")] =
        it != display_coverage_.end() ? QString::fromStdString(it->second)
                                      : QString();
    v = QVariant(m);
  }
}

void StudioBridge::assignVideoToMonitor(const QString& key,
                                        const QString& path) {
  if (key.isEmpty()) {
    SetLastError(tr("Kunci monitor kosong."));
    return;
  }
  if (path.isEmpty()) {
    SetLastError(tr("Video kosong."));
    return;
  }
  if (!IsKnownDisplayKey(key)) {
    SetLastError(
        tr("Monitor tidak dikenal: %1. Segarkan daftar layar dulu.").arg(key));
    return;
  }
  DisplaysConfig store;
  try {
    store = LoadDisplays(DefaultDisplaysPath());
  } catch (const ConfigError&) {
    store = DisplaysConfig{};
  }
  MonitorAssignment assignment;
  assignment.path = path.toStdWString();
  assignment.exists = QFileInfo::exists(path);
  store.assignments[key.toStdWString()] = assignment;
  try {
    SaveDisplays(DefaultDisplaysPath(), store);
  } catch (const ConfigError& e) {
    SetLastError(tr("Gagal menyimpan displays.json: %1")
                     .arg(QString::fromUtf8(e.what())));
    return;
  }
  // IPC push after the persist (SettingsBridge::setMonitorId's
  // mutate-then-notify shape, with the SaveDisplays write in front of it).
  // Best-effort: the engine also re-reads displays.json through its own
  // watcher, so a dead engine never loses the assignment.
  nlohmann::json payload;
  payload["device"] = key.toStdString();
  payload["path"] = path.toStdString();
  const IpcResult res = ipc_.Send(Cmd::set_display_video, payload);
  if (res.status == IpcStatus::kOk) {
    AppendLog(tr("Penugasan layar dikirim ke engine: %1").arg(key));
  } else if (res.status == IpcStatus::kNotRunning) {
    AppendLog(tr("Engine mati — penugasan tersimpan di displays.json saja"));
  } else {
    AppendLog(tr("IPC set_display_video ditolak: %1")
                  .arg(QString::fromStdString(res.error)));
  }
  ClearLastError();
  ApplyDisplayModel(last_monitors_, store, display_coverage_);
}

void StudioBridge::clearMonitorAssignment(const QString& key) {
  if (key.isEmpty()) {
    SetLastError(tr("Kunci monitor kosong."));
    return;
  }
  if (!IsKnownDisplayKey(key)) {
    SetLastError(
        tr("Monitor tidak dikenal: %1. Segarkan daftar layar dulu.").arg(key));
    return;
  }
  DisplaysConfig store;
  try {
    store = LoadDisplays(DefaultDisplaysPath());
  } catch (const ConfigError&) {
    store = DisplaysConfig{};
  }
  const auto it = store.assignments.find(key.toStdWString());
  if (it == store.assignments.end()) {
    // Nothing persisted for this key: idempotent no-op, no file write.
    ApplyDisplayModel(last_monitors_, store, display_coverage_);
    return;
  }
  store.assignments.erase(it);
  try {
    SaveDisplays(DefaultDisplaysPath(), store);
  } catch (const ConfigError& e) {
    SetLastError(tr("Gagal menyimpan displays.json: %1")
                     .arg(QString::fromUtf8(e.what())));
    return;
  }
  nlohmann::json payload;
  payload["device"] = key.toStdString();
  payload["clear"] = true;
  const IpcResult res = ipc_.Send(Cmd::set_display_video, payload);
  if (res.status == IpcStatus::kOk) {
    AppendLog(tr("Penugasan layar dihapus: %1").arg(key));
  } else if (res.status == IpcStatus::kNotRunning) {
    AppendLog(tr("Engine mati — penghapusan tersimpan di displays.json saja"));
  } else {
    AppendLog(tr("IPC set_display_video (clear) ditolak: %1")
                  .arg(QString::fromStdString(res.error)));
  }
  ClearLastError();
  ApplyDisplayModel(last_monitors_, store, display_coverage_);
}

int StudioBridge::applyToAllMonitors(const QString& path) {
  // Brief C-14 / GATE 0 #3: "Semua layar" = monitor_id -1 + DELETE every
  // per-key override. The video itself is installed by the CALLER via
  // applyWallpaper(path); this function only moves the global target and
  // clears overrides. Returns the number of overrides cleared so the caller
  // can decide the C-14 confirmation (NeedsAllScreensConfirm(count)).
  Q_UNUSED(path);
  setQuickMonitor(-1);
  QStringList keys;
  for (const QVariant& v : displays_) {
    const QVariantMap m = v.toMap();
    if (!m.value(QStringLiteral("assignedPath")).toString().isEmpty()) {
      keys.append(m.value(QStringLiteral("key")).toString());
    }
  }
  for (const QString& key : keys) {
    clearMonitorAssignment(key);
  }
  if (!keys.isEmpty()) {
    AppendLog(tr("Pasang ke semua layar: %1 override dihapus")
                  .arg(keys.size()));
  }
  return keys.size();
}

void StudioBridge::openWindowsDisplaySettings() {
  // Brief B9 / C-15: open Windows Display Settings. Best-effort: a launch
  // failure is logged, not raised as lastError (nothing user-actionable).
  const HINSTANCE rc = ShellExecuteW(nullptr, L"open", L"ms-settings:display",
                                      nullptr, nullptr, SW_SHOWNORMAL);
  if (reinterpret_cast<INT_PTR>(rc) <= 32) {
    AppendLog(tr("Gagal membuka Pengaturan Layar Windows (ShellExecute %1)")
                  .arg(static_cast<qint64>(reinterpret_cast<INT_PTR>(rc))));
  }
}

void StudioBridge::setQuickFit(const QString& fit_mode) {
  if (fit_mode.isEmpty()) {
    return;
  }
  WallpaperConfig cfg;
  try {
    cfg = LoadConfig(DefaultConfigPath());
  } catch (const ConfigError&) {
  }
  cfg.fit_mode = fit_mode.toStdString();
  try {
    SaveConfig(DefaultConfigPath(), cfg);
    AppendLog(QStringLiteral("Pengaturan cepat: mode layar = %1").arg(fit_mode));
  } catch (const ConfigError& e) {
    AppendLog(QStringLiteral("Gagal menyimpan mode layar: %1")
                  .arg(QString::fromUtf8(e.what())));
  }
  quick_fit_ = fit_mode;
  emit quickSettingsChanged();
}

void StudioBridge::setQuickMonitor(int monitor_id) {
  WallpaperConfig cfg;
  try {
    cfg = LoadConfig(DefaultConfigPath());
  } catch (const ConfigError&) {
  }
  cfg.monitor_id = monitor_id;
  try {
    SaveConfig(DefaultConfigPath(), cfg);
    AppendLog(QStringLiteral("Pengaturan cepat: layar = %1").arg(monitor_id));
  } catch (const ConfigError& e) {
    AppendLog(
        QStringLiteral("Gagal menyimpan layar: %1").arg(QString::fromUtf8(e.what())));
  }
  quick_monitor_ = monitor_id;
  emit quickSettingsChanged();
}

void StudioBridge::setQuickAutostart(bool enabled) {
  std::string err;
  if (!SetAutostart(enabled, &err)) {
    AppendLog(QStringLiteral("Gagal mengubah mulai otomatis: %1")
                  .arg(QString::fromStdString(err)));
  } else {
    AppendLog(QStringLiteral("Pengaturan cepat: mulai otomatis %1")
                  .arg(enabled ? QStringLiteral("ON") : QStringLiteral("OFF")));
  }
  // Re-read rather than trusting `enabled`: on failure the registry still holds
  // the old value, and the checkbox must snap back to reality.
  quick_autostart_ = IsAutostart();
  emit quickSettingsChanged();
}

void StudioBridge::setQuickBattery(bool enabled) {
  WallpaperConfig cfg;
  try {
    cfg = LoadConfig(DefaultConfigPath());
  } catch (const ConfigError&) {
  }
  cfg.battery_saver = enabled;
  try {
    SaveConfig(DefaultConfigPath(), cfg);
    AppendLog(QStringLiteral("Pengaturan cepat: hemat baterai %1")
                  .arg(enabled ? QStringLiteral("ON") : QStringLiteral("OFF")));
  } catch (const ConfigError& e) {
    AppendLog(QStringLiteral("Gagal menyimpan hemat baterai: %1")
                  .arg(QString::fromUtf8(e.what())));
  }
  quick_battery_ = enabled;
  emit quickSettingsChanged();
}

// --- engine ops (all blocking work runs on a worker) ------------------------
// busy_, pending_pause_op_ and pending_apply_path_ are plain GUI-thread state:
// every Q_INVOKABLE and every watcher slot runs on the GUI thread, and only
// the QtConcurrent lambdas below read the shared client.

void StudioBridge::startEngine() {
  if (busy_) {
    SetLastError(tr("Engine sibuk — tunggu proses berjalan"));
    return;
  }
  busy_ = true;
  op_cancel_.store(false, std::memory_order_relaxed);
  ipc_.ClearCancel();
  emit busyChanged();
  // RestartEngine waits up to 8s for the old PID to exit and then polls the
  // pipe; WaitForEngineReady adds the cancellable 8s ready-wait. Both observe
  // the same atomic cancel flag in short (<=100ms) slices.
  start_watcher_->setFuture(QtConcurrent::run([this]() {
    EngineStartOutcome r;
    r.restarted = apply_manager_.RestartEngine(&r.restart_error);
    r.ready = r.restarted && !op_cancel_.load(std::memory_order_relaxed)
                  ? WaitForEngineReady(ipc_, op_cancel_, 8000)
                  : false;
    return r;
  }));
}

void StudioBridge::OnEngineStartDone() {
  EngineStartOutcome r;
  QString thrown;
  const bool ok_read = TakeResult(start_watcher_, &r, &thrown);
  busy_ = false;
  emit busyChanged();
  if (!ok_read) {
    if (!thrown.isEmpty()) AppendLog(thrown);
    SetLastError(FriendlyApplyError(thrown));
    return;
  }
  if (!r.restarted) {
    // RestartEngine already filled restart_error with the reason (engine.exe
    // missing, the 8s wait gave up, the pipe never came up).
    if (!r.restart_error.isEmpty()) AppendLog(r.restart_error);
    SetLastError(FriendlyApplyError(r.restart_error));
    return;
  }
  if (!r.ready) {
    // Started but not answering yet; the 1.5s poll keeps retrying and the
    // status sentence flips on its own.
    AppendLog(QStringLiteral(
        "Engine: sudah dinyalakan tapi belum siap; akan dicoba lagi otomatis"));
    return;
  }
  AppendLog(QStringLiteral("Engine: started OK"));
  ClearLastError();
  requestStatusPoll();
}

qint64 StudioBridge::compressFirstOfferMb(const QString& path) const {
  return CompressFirstOfferMb(path);
}

void StudioBridge::applyWallpaper(const QString& path) {
  if (path.isEmpty()) {
    SetLastError(tr("Belum Ada yang Dipilih"));
    return;
  }
  if (!QFileInfo::exists(path)) {
    SetLastError(tr("Video tidak ditemukan"));
    return;
  }
  if (busy_) {
    SetLastError(tr("Engine sibuk — tunggu proses berjalan"));
    return;
  }

  // Preserve every existing config field; only the video changes. T16
  // last-valid: LoadConfig backs a corrupt file up to .bak before throwing, so
  // fall back to that copy instead of silently resetting the user's settings to
  // defaults - the same recovery MainWindow::ApplyVideoPath performs.
  WallpaperConfig cfg;
  QString recovery;
  try {
    cfg = LoadConfig(DefaultConfigPath());
  } catch (const ConfigError& e) {
    recovery = QStringLiteral("Apply: config korup (%1), memakai salinan "
                              "terakhir yang valid")
                   .arg(QString::fromUtf8(e.what()));
    const std::filesystem::path bak = DefaultConfigPath().wstring() + L".bak";
    try {
      cfg = LoadConfig(bak);
      recovery += QStringLiteral(" | Apply: salinan .bak OK");
    } catch (const ConfigError&) {
      recovery += QStringLiteral(
          " | Apply: salinan .bak tidak bisa dipakai, memakai bawaan");
    }
  }
  cfg.video_path = path.toStdWString();
  if (!recovery.isEmpty()) {
    AppendLog(recovery);
  }

  // Config sync first: even when the engine is dead the new video persists for
  // boot. This is a small local file write, done on the GUI thread exactly
  // like MainWindow::ApplyVideoPath.
  QString error;
  if (!apply_manager_.WriteConfig(cfg, &error)) {
    SetLastError(error);
    return;
  }

  busy_ = true;
  op_cancel_.store(false, std::memory_order_relaxed);
  ipc_.ClearCancel();
  pending_apply_path_ = path;
  emit busyChanged();
  // The live-switch (plus its built-in restart recovery and the retry) blocks;
  // it runs on a worker so a dead/slow engine never freezes Studio.
  apply_watcher_->setFuture(QtConcurrent::run([this, cfg]() {
    ApplyOutcome outcome;
    QString apply_error;
    const ApplyResult res = apply_manager_.Apply(cfg, &apply_error);
    outcome.ok = static_cast<bool>(res);  // kLiveOk / kAfterRestartOk
    outcome.restarted = res.restart_attempted;
    outcome.error = apply_error;
    if (!outcome.ok && outcome.error.isEmpty()) {
      outcome.error = QString::fromStdString(res.first_error);
    }
    return outcome;
  }));
}

void StudioBridge::OnApplyDone() {
  ApplyOutcome outcome;
  QString thrown;
  const bool ok_read = TakeResult(apply_watcher_, &outcome, &thrown);
  busy_ = false;
  emit busyChanged();
  if (!ok_read) {
    if (!thrown.isEmpty()) AppendLog(thrown);
    SetLastError(FriendlyApplyError(thrown));
    return;
  }
  if (!outcome.ok) {
    // The engine's / manager's own message reaches the log; the status line
    // gets a mapped sentence.
    if (!outcome.error.isEmpty()) AppendLog(outcome.error);
    SetLastError(FriendlyApplyError(outcome.error));
    return;
  }
  ClearLastError();
  // The live status sentence is the success feedback (it flips to
  // "Wallpaper aktif • <file>"), so no extra log line is invented here.
  loadPreview(pending_apply_path_);
  requestPoster(pending_apply_path_);
  requestStatusPoll();
}

void StudioBridge::RunPauseResume(bool do_pause) {
  if (pending_pause_op_ != 0) {
    return;
  }
  pending_pause_op_ = do_pause ? 1 : 2;
  // The 2s ack deadline lives inside Send, so this must stay off the GUI
  // thread (MainWindow::OnPauseClicked does the same).
  pause_watcher_->setFuture(QtConcurrent::run([this, do_pause]() {
    return do_pause ? ipc_.Pause() : ipc_.Resume();
  }));
}

void StudioBridge::pause() {
  RunPauseResume(/*do_pause=*/true);
}

void StudioBridge::resume() {
  RunPauseResume(/*do_pause=*/false);
}

void StudioBridge::togglePause() {
  // B3: one button, toggle - consult the live kind, never blindly pause.
  if (status_.kind == EngineStatusView::Kind::kPaused) {
    resume();
    return;
  }
  if (engine_running_) {
    pause();
  }
  // kNotRunning / kDisconnected: nothing to toggle; no IPC, no log noise.
}

void StudioBridge::copyToClipboard(const QString& text) {
  // Empty text is a valid QML call (e.g. a dialog opened before any
  // technical detail existed) and must never reach the clipboard API.
  if (text.isEmpty()) {
    return;
  }
  // QGuiApplication::clipboard() is not safe when only a QCoreApplication
  // exists (studio_logic_test runs that way on purpose - no QPA plugin).
  // The real shell is a QApplication, so the cast succeeds there.
  auto* gui = qobject_cast<QGuiApplication*>(QCoreApplication::instance());
  if (gui == nullptr) {
    return;
  }
  QClipboard* clipboard = gui->clipboard();
  if (clipboard == nullptr) {
    return;
  }
  clipboard->setText(text);
}

void StudioBridge::OnPauseResumeDone() {
  IpcResult res;
  QString thrown;
  const bool ok_read = TakeResult(pause_watcher_, &res, &thrown);
  const int op = pending_pause_op_;
  pending_pause_op_ = 0;
  if (!ok_read) {
    SetLastError(thrown);
    requestStatusPoll();
    return;
  }
  if (op == 1) {
    if (res.status == IpcStatus::kOk) {
      AppendLog(tr("Engine paused (IPC)"));
      ClearLastError();
    } else if (res.status == IpcStatus::kNotRunning) {
      SetLastError(tr("Tidak ada wallpaper aktif. Pilih video untuk mulai, "
                      "lalu coba lagi."));
    } else {
      const QString technical = QString::fromStdString(res.error);
      if (!technical.isEmpty()) AppendLog(technical);
      SetLastError(tr("Jeda gagal: %1. Coba lagi.")
                       .arg(FriendlyIpcError(technical)));
    }
  } else if (op == 2) {
    if (res.status == IpcStatus::kOk) {
      AppendLog(tr("Engine resumed (IPC)"));
      ClearLastError();
    } else if (res.status == IpcStatus::kNotRunning) {
      SetLastError(tr("Tidak ada wallpaper aktif. Pilih video untuk mulai, "
                      "lalu coba lagi."));
    } else {
      const QString technical = QString::fromStdString(res.error);
      if (!technical.isEmpty()) AppendLog(technical);
      SetLastError(tr("Lanjutkan gagal: %1. Coba lagi.")
                       .arg(FriendlyIpcError(technical)));
    }
  }
  // Refresh right away instead of waiting up to 1.5s for the label to flip.
  requestStatusPoll();
}

// --- preview poster --------------------------------------------------------

void StudioBridge::requestPoster(const QString& video_path) {
  if (video_path.isEmpty()) {
    if (!poster_path_.isEmpty()) {
      poster_path_.clear();
      emit posterChanged();
    }
    return;
  }
  const std::filesystem::path video(video_path.toStdWString());
  // Fast path: CachedThumb is documented read-only, non-generating and
  // non-logging, so it is safe on the GUI thread and keeps the common case
  // (grid already warmed the cache) free of a worker round-trip.
  const std::filesystem::path cached = thumb_.CachedThumb(video);
  if (!cached.empty()) {
    const QString path = QString::fromStdWString(cached.wstring());
    if (poster_path_ != path) {
      poster_path_ = path;
      emit posterChanged();
    }
    return;
  }
  if (poster_busy_) {
    pending_poster_video_ = video_path;
    return;
  }
  pending_poster_video_.clear();
  poster_busy_ = true;
  // Cache miss: GetThumb spawns ffmpeg, so it runs on a worker. It returns an
  // empty path on ANY failure and never throws, so the QML Image just stays
  // blank and the placeholder shows.
  poster_watcher_->setFuture(QtConcurrent::run([this, video]() {
    return QString::fromStdWString(thumb_.GetThumb(video).wstring());
  }));
}

void StudioBridge::OnPosterDone() {
  QString path;
  QString thrown;
  const bool ok_read = TakeResult(poster_watcher_, &path, &thrown);
  poster_busy_ = false;
  if (!ok_read) {
    if (!thrown.isEmpty()) {
      AppendLog(thrown);
    }
  } else if (poster_path_ != path) {
    poster_path_ = path;
    emit posterChanged();
  }
  // A request that arrived while this job ran was deferred; re-issue it now so
  // the poster follows the newest selection instead of the finished video.
  if (!pending_poster_video_.isEmpty()) {
    const QString next = pending_poster_video_;
    pending_poster_video_.clear();
    requestPoster(next);
  }
}

// --- update check -----------------------------------------------------------

void StudioBridge::checkForUpdates() {
  RunUpdateCheck(/*interactive=*/false);
}

void StudioBridge::checkForUpdatesInteractive() {
  RunUpdateCheck(/*interactive=*/true);
}

void StudioBridge::RunUpdateCheck(bool interactive) {
  if (update_busy_) {
    return;
  }
  const QString url = ResolveUpdateCheckUrl();
  if (interactive) {
    update_check_message_.clear();
    if (!IsUpdateCheckUrlUsable(url)) {
      update_check_message_ =
          tr("Cek pembaruan dimatikan di Pengaturan \u2192 Pembaruan.");
      emit updateChanged();
      return;
    }
  } else if (!IsUpdateCheckUrlUsable(url)) {
    return;
  }
  if (update_checker_ == nullptr) {
    update_checker_ = new UpdateChecker(version_, this);
    update_checker_->SetReleasesPageUrl(
        QString::fromUtf8(K6WP_RELEASES_URL));
    connect(update_checker_, &UpdateChecker::UpdateAvailable, this,
            [this](const QString& latest, const QString& page) {
              latest_version_ = latest;
              latest_page_url_ = page;
              emit updateChanged();
            });
    connect(update_checker_, &UpdateChecker::CheckFinished, this,
            [this](UpdateCheckOutcome outcome) {
              update_busy_ = false;
              const bool available =
                  outcome == UpdateCheckOutcome::kUpdateAvailable;
              if (!available) {
                latest_version_.clear();
                latest_page_url_.clear();
              }
              // Only a user-initiated check narrates; the start-up check
              // leaves updateCheckMessage empty.
              if (update_check_interactive_) {
                update_check_interactive_ = false;
                switch (outcome) {
                  case UpdateCheckOutcome::kDisabled:
                    update_check_message_ =
                        tr("Cek pembaruan dimatikan di Pengaturan \u2192 Pembaruan.");
                    break;
                  case UpdateCheckOutcome::kUpToDate:
                    update_check_message_ = tr("K6WP kamu sudah versi terbaru.");
                    break;
                  case UpdateCheckOutcome::kUpdateAvailable:
                    update_check_message_ =
                        tr("Versi %1 tersedia.").arg(latest_version_);
                    break;
                  case UpdateCheckOutcome::kFailed:
                    update_check_message_ = tr(
                        "Gagal menghubungi server pembaruan. Cek koneksi lalu "
                        "coba lagi.");
                    break;
                }
              }
              emit updateChanged();
            });
  }
  update_busy_ = true;
  emit updateChanged();
  if (interactive) {
    update_check_interactive_ = true;
    update_checker_->CheckInteractive(url);
  } else {
    update_checker_->Check(url);
  }
}

// --- preview widget plumbing (forwarded to the active QmlShell) ------------

void StudioBridge::syncPreviewGeometry(int x, int y, int w, int h) {
  QmlShell* shell = ActiveQmlShell();
  if (shell == nullptr) {
    return;
  }
  shell->syncPreviewGeometry(x, y, w, h);
}

void StudioBridge::loadPreview(const QString& path) {
  if (path.isEmpty()) {
    return;
  }
  QmlShell* shell = ActiveQmlShell();
  if (shell == nullptr) {
    return;
  }
  shell->loadPreview(path);
}

void StudioBridge::setPreviewPaused(bool paused) {
  QmlShell* shell = ActiveQmlShell();
  if (shell == nullptr) {
    return;
  }
  shell->setPreviewPaused(paused);
}

}  // namespace k6wp
