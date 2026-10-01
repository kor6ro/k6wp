// Phase 1 of the Widgets -> QML migration: the QML-facing backend.
// See studio_bridge.hpp for the design notes; this file is the QML-visible
// behaviour and deliberately mirrors main_window.cpp's HIGH-3 slice B
// threading (1.5s QTimer -> QtConcurrent worker -> QFutureWatcher slot).

#include "studio_bridge.hpp"

#include <QFileDialog>
#include <QFileInfo>
#include <QTimer>
#include <QVariantMap>

#include <QtConcurrent>

#include <cassert>

#include "autostart.hpp"
#include "compress_first_offer.hpp"
#include "config_schema.hpp"
#include "engine_status_controller.hpp"
#include "links.hpp"
#include "monitor_util.hpp"
#include "qml_shell.hpp"
#include "update_checker.hpp"
#include "user_errors.hpp"

namespace k6wp {

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
}

StudioBridge::~StudioBridge() {
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
    return;
  }
  // The status DECISION is EngineStatusController::DecideEngineStatus (pure,
  // unit-tested by studio_logic_test); this class only maps the resulting view
  // onto QML properties and the Indonesian sentences MainWindow painted.
  ApplyStatus(DecideEngineStatus(res));
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
          tr("Engine aktif tapi tak tampil — %1").arg(short_name);
    } else {
      status_detail_ = tr("Engine aktif — %1").arg(short_name);
    }
    QString tip_detail =
        view.video.isEmpty() ? tr("(belum ada video aktif)") : view.video;
    if (view.kind == EngineStatusView::Kind::kDegraded) {
      tip_detail += tr("\n%1 slot tak menempel (headless) — cek engine.log")
                        .arg(view.headless > 0 ? view.headless : 1);
    }
    status_hint_ = tr("pid %1\n%2").arg(view.pid).arg(tip_detail);
  } else if (view.kind == EngineStatusView::Kind::kNotRunning) {
    status_detail_ = tr("Engine mati — klik Nyalakan Engine");
    status_hint_ = tr("Engine tidak jalan");
  } else {
    status_detail_ = tr("Terputus — coba lagi");
    // The transport error is the "never swallow an IPC failure" path, so it
    // reaches the log verbatim while the tooltip gets a mapped sentence.
    const QString technical = QString::fromStdString(view.error);
    AppendLog(technical);
    status_hint_ = FriendlyIpcError(technical);
  }
  emit engineStatusChanged();

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
  if (last_error_ == error) {
    return;
  }
  last_error_ = error;
  emit lastErrorChanged();
  if (!error.isEmpty()) {
    AppendLog(error);
  }
}

void StudioBridge::ClearLastError() {
  SetLastError(QString());
}

void StudioBridge::AppendLog(const QString& line) {
  constexpr int kMaxLogLines = 500;
  log_.append(line);
  while (log_.size() > kMaxLogLines) {
    log_.removeFirst();
  }
  emit logChanged();
}

// --- video selection --------------------------------------------------------

void StudioBridge::pickVideo() {
  // Same filter the Widgets dialogs used (wallpaper_tab_widget / import_dialog).
  const QString path = QFileDialog::getOpenFileName(
      nullptr, QStringLiteral("Pilih Video"),
      QString(),
      QStringLiteral("Video (*.mp4 *.webm *.avi *.mkv *.mov *.wmv);;"
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
    SetLastError(QStringLiteral("Engine sibuk — tunggu proses berjalan"));
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
    SetLastError(QStringLiteral("Belum Ada yang Dipilih"));
    return;
  }
  if (!QFileInfo::exists(path)) {
    SetLastError(QStringLiteral("Video tidak ditemukan"));
    return;
  }
  if (busy_) {
    SetLastError(QStringLiteral("Engine sibuk — tunggu proses berjalan"));
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
    if (!outcome.ok && outcome.error.isEmpty()) {
      // The manager reported neither a message nor a first_error. Leave it
      // empty: FriendlyApplyError turns that into actionable wording, so the
      // failure is still named and no fake line reaches the log.
      outcome.error.clear();
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
  // "Engine aktif - <file>"), so no extra log line is invented here.
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
      SetLastError(tr("Engine mati — klik Nyalakan Engine dulu"));
    } else {
      const QString technical = QString::fromStdString(res.error);
      if (!technical.isEmpty()) AppendLog(technical);
      SetLastError(tr("Jeda gagal: %1").arg(FriendlyIpcError(technical)));
    }
  } else if (op == 2) {
    if (res.status == IpcStatus::kOk) {
      AppendLog(tr("Engine resumed (IPC)"));
      ClearLastError();
    } else if (res.status == IpcStatus::kNotRunning) {
      SetLastError(tr("Engine mati — klik Nyalakan Engine dulu"));
    } else {
      const QString technical = QString::fromStdString(res.error);
      if (!technical.isEmpty()) AppendLog(technical);
      SetLastError(
          tr("Lanjutkan gagal: %1").arg(FriendlyIpcError(technical)));
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
