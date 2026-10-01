// Phase 3 of the Widgets -> QML migration: the Kompresor tab's backend.
// See compress_bridge.hpp for the two design constraints (primitives-only QML
// surface, and the consent gate driven from QML).

#include "compress_bridge.hpp"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QUrl>

#include <cmath>
#include <filesystem>

#include "compress_errors.hpp"
#include "config_schema.hpp"
#include "probe_async.hpp"

namespace k6wp {

namespace {

// Not a member, so tr() is out of scope. The context must be a literal:
// lupdate resolves QCoreApplication::translate statically, so a constexpr
// variable would drop these two strings from the catalogue.
QString FormatEta(int eta_s) {
  if (eta_s < 0) {
    return QCoreApplication::translate("k6wp::CompressBridge", "ETA --");
  }
  return QCoreApplication::translate("k6wp::CompressBridge", "ETA %1s")
      .arg(eta_s);
}

}  // namespace

CompressBridge::CompressBridge(QObject* parent) : QObject(parent) {
  status_text_ = tr("Belum ada kompresi dijalankan");
  CompressService* service = controller_.service();
  connect(service, &CompressService::Started, this,
          [this](const JobMeta& meta) {
            status_text_ = tr("Mengompres %1 ...")
                              .arg(meta.label.isEmpty() ? meta.src : meta.label);
            detail_text_.clear();
            progress_ = 0;
            eta_s_ = -1;
            emit statusTextChanged();
            emit progressChanged();
            RefreshQueue();
          });
  connect(service, &CompressService::Progress, this,
          [this](int percent, int eta_s) {
            progress_ = percent;
            eta_s_ = eta_s;
            emit progressChanged();
          });
  connect(service, &CompressService::Finished, this,
          [this](const JobMeta& meta, bool ok, const QString& technical,
                 const CompressOkInfo& info) {
            CompressRequest req;
            controller_.TakeRequest(meta.job_id, &req);
            progress_ = ok ? 100 : 0;
            eta_s_ = 0;
            if (!ok) {
              // Technical detail goes to the log only; the user sees the
              // friendly message (same split the Widgets handler used).
              AppendLog(QStringLiteral("Compress: job #%1 FAILED: %2")
                            .arg(meta.job_id)
                            .arg(technical));
              const QString friendly = FriendlyCompressError(technical);
              status_text_ =
                  tr("Kompres gagal: %1").arg(friendly);
              detail_text_ = friendly;
              SetLastError(friendly);
              emit statusTextChanged();
              emit progressChanged();
              RefreshQueue();
              emit jobFinishedWithoutResult();
              return;
            }
            const QString out = meta.pending_out;
            if (out.isEmpty() || !QFile::exists(out)) {
              AppendLog(
                  QStringLiteral("Compress: job #%1 reported OK but output is "
                                 "missing: %2")
                      .arg(meta.job_id)
                      .arg(out));
              status_text_ = QStringLiteral(
                  "Kompres lapor OK tetapi hasilnya hilang: %1").arg(out);
              detail_text_ = status_text_;
              emit statusTextChanged();
              emit progressChanged();
              RefreshQueue();
              emit jobFinishedWithoutResult();
              return;
            }
            if (meta.register_library) {
              // Registers the COMPRESSED result: src stays the original input,
              // dst is the compressor output. Dimensions come from the request
              // rather than a second probe. LibraryManager throws, and this is
              // C++ so the exception is caught here - it can never cross into
              // QML.
              try {
                LibraryEntry entry;
                if (req.in_path.isEmpty()) {
                  entry.src = std::filesystem::path(out.toStdWString());
                } else {
                  entry.src = std::filesystem::path(req.in_path.toStdWString());
                  entry.crf = req.crf;
                  entry.fps = req.fps;
                  entry.encoder = req.encoder.toStdString();
                  if (req.res_w > 0 && req.res_h > 0) {
                    entry.width = req.res_w;
                    entry.height = req.res_h;
                    entry.res =
                        std::to_string(req.res_w) + "x" + std::to_string(req.res_h);
                  }
                }
                entry.dst = std::filesystem::path(out.toStdWString());
                // This is a second LibraryManager in the process that starts
                // empty. Add() persists the WHOLE in-memory index, so without
                // loading the on-disk index first this write would truncate the
                // user's library to the single just-compressed entry.
                library_mgr_.Load();
                library_mgr_.Add(entry);
                AppendLog(
                    QStringLiteral("Compress: imported to library: %1").arg(out));
              } catch (const std::exception& e) {
                AppendLog(QStringLiteral("Compress: library import failed: %1")
                              .arg(QString::fromUtf8(e.what())));
              }
            }
            if (info.skip_optimal) {
              status_text_ = QStringLiteral(
                  "Sudah optimal (H.264 tanpa audio, resolusi/fps sesuai) — "
                  "disalin, bukan dikompres");
              detail_text_ = status_text_;
            } else if (info.cache_hit) {
              status_text_ = QStringLiteral(
                  "Hasil instan dari cache (setting sama sudah pernah)");
              detail_text_ = status_text_;
            } else {
              status_text_ =
                  tr("Selesai: %1 (%2)").arg(out, info.encoder);
              detail_text_ = tr("Encoder: %1").arg(
                  info.encoder.isEmpty() ? QStringLiteral("--") : info.encoder);
            }
            result_path_ = out;
            emit resultChanged();
            emit statusTextChanged();
            emit progressChanged();
            ClearLastError();
            RefreshQueue();
          });
  connect(service, &CompressService::QueueSummary, this,
          [this](int ok_count, int fail_count) {
            status_text_ =
                fail_count == 0
                    ? tr("Antrian selesai: %1 sukses").arg(ok_count)
                    : tr("Antrian selesai: %1 sukses, %2 gagal")
                          .arg(ok_count)
                          .arg(fail_count);
            emit statusTextChanged();
            RefreshQueue();
          });
  connect(service, &CompressService::LogMessage, this,
          [this](const QString& line) { AppendLog(line); });

  refreshDefaults();
}

CompressBridge::~CompressBridge() = default;

// --- inputs -----------------------------------------------------------------

void CompressBridge::refreshDefaults() {
  try {
    settings_ = LoadStudioSettings(DefaultStudioSettingsPath());
  } catch (const ConfigError&) {
    // Keep the in-memory defaults; same as the Widgets handler.
  }
  crf_ = settings_.default_crf > 0 ? settings_.default_crf : 22;
  fps_ = settings_.default_fps > 0 ? settings_.default_fps : 30;
  out_dir_ = CompressController::ResolveTabOutDir(
      QString(), QString::fromStdWString(settings_.compress_output_dir));
  if (out_dir_.isEmpty()) {
    out_dir_ = CompressController::DefaultWallpapersDir();
  }
  controller_.SetCacheDir(QString::fromStdWString(settings_.cache_dir));
  emit inputsChanged();
}

void CompressBridge::pickSource() {
  const QString path = QFileDialog::getOpenFileName(
      nullptr, tr("Pilih Video untuk Kompres"), QString(),
      tr("Video (*.mp4 *.webm *.avi *.mkv *.mov *.wmv);;"
         "Semua File (*)"));
  if (!path.isEmpty()) {
    setSourcePath(path);
  }
}

void CompressBridge::setSourcePath(const QString& path) {
  if (source_path_ == path) {
    return;
  }
  source_path_ = path;
  emit inputsChanged();
}

void CompressBridge::pickOutDir() {
  const QString dir = QFileDialog::getExistingDirectory(
      nullptr, tr("Pilih Folder Output"), out_dir_);
  if (dir.isEmpty()) {
    return;
  }
  out_dir_ = dir;
  emit inputsChanged();
}

void CompressBridge::openOutDir() {
  if (!out_dir_.isEmpty()) {
    QDesktopServices::openUrl(
        QUrl::fromLocalFile(QDir::toNativeSeparators(out_dir_)));
  }
}

void CompressBridge::openResultDir() {
  if (result_path_.isEmpty()) {
    return;
  }
  QDesktopServices::openUrl(
      QUrl::fromLocalFile(QFileInfo(result_path_).absolutePath()));
}

void CompressBridge::setAdvanced(bool on) {
  if (advanced_ == on) {
    return;
  }
  advanced_ = on;
  emit inputsChanged();
}

void CompressBridge::setCrf(int value) {
  // Schema range; the compressor rejects anything outside it.
  if (value < 16 || value > 28 || value == crf_) {
    return;
  }
  crf_ = value;
  emit inputsChanged();
}

void CompressBridge::setFps(int value) {
  if (value <= 0 || value == fps_) {
    return;
  }
  fps_ = value;
  emit inputsChanged();
}

void CompressBridge::setEncoder(const QString& name) {
  if (name.isEmpty() || encoder_ == name) {
    return;
  }
  encoder_ = name;
  emit inputsChanged();
}

void CompressBridge::setResolutionText(const QString& text) {
  if (resolution_text_ == text) {
    return;
  }
  resolution_text_ = text;
  emit inputsChanged();
}

void CompressBridge::setForceLong(bool on) {
  if (force_long_ == on) {
    return;
  }
  force_long_ = on;
  emit inputsChanged();
}

void CompressBridge::setAutoApply(bool on) {
  if (auto_apply_ == on) {
    return;
  }
  auto_apply_ = on;
  emit autoApplyChanged();
}

// --- queue helpers ----------------------------------------------------------

QString CompressBridge::etaText() const { return FormatEta(eta_s_); }

QString CompressBridge::queueText() const {
  const int waiting = controller_.PendingCount();
  return waiting > 0 ? tr("%1 menunggu").arg(waiting) : QString();
}

void CompressBridge::RefreshQueue() { emit queueChanged(); }

void CompressBridge::AppendLog(const QString& line) {
  constexpr int kMaxLogLines = 500;
  log_.append(line);
  while (log_.size() > kMaxLogLines) {
    log_.removeFirst();
  }
  emit logChanged();
}

void CompressBridge::SetLastError(const QString& error) {
  if (last_error_ == error) {
    return;
  }
  last_error_ = error;
  emit lastErrorChanged();
  if (!error.isEmpty()) {
    AppendLog(error);
  }
}

void CompressBridge::ClearLastError() { SetLastError(QString()); }

void CompressBridge::clearResult() {
  if (result_path_.isEmpty()) {
    return;
  }
  result_path_.clear();
  emit resultChanged();
}

// --- enqueue ----------------------------------------------------------------

void CompressBridge::start() {
  if (source_path_.isEmpty()) {
    SetLastError(tr("Pilih video dulu dengan Impor Video atau Pilih Video."));
    return;
  }
  // Re-read so a Kompresor default just saved in Pengaturan applies now.
  refreshDefaults();

  int res_w = 1280;
  int res_h = 720;
  bool need_res = false;
  if (advanced_) {
    const QString res_text = resolution_text_.trimmed().toLower();
    const int x = res_text.indexOf(QLatin1Char('x'));
    if (x > 0) {
      const int w = res_text.left(x).toInt();
      const int h = res_text.mid(x + 1).toInt();
      if (w > 0 && h > 0) {
        res_w = w;
        res_h = h;
      }
    }
  } else if (settings_.default_resolution_mode == "source") {
    need_res = true;
  } else {
    CompressController::ResolveSimpleRes(settings_.default_resolution_mode,
                                         nullptr, res_w, res_h);
  }
  EnqueueWithProbe(source_path_, res_w, res_h, need_res);
}

void CompressBridge::EnqueueWithProbe(const QString& src, int res_w, int res_h,
                                      bool need_res) {
  status_text_ = tr("Menyiapkan...");
  emit statusTextChanged();
  ProbeVideoAsync(
      std::filesystem::path(src.toStdWString()),
      [this, src, res_w, res_h, need_res](bool ok,
                                           const VideoMetadata& probed) mutable {
        if (need_res && ok && probed.width > 0 && probed.height > 0) {
          res_w = probed.width;
          res_h = probed.height;
        }
        const double duration = ok ? probed.duration : 0.0;
        if (duration > kLongVideoConsentSeconds && !force_long_) {
          // Hand the decision to QML; AskLongVideoConsent is a QMessageBox and
          // cannot run headless here.
          consent_src_ = src;
          consent_w_ = res_w;
          consent_h_ = res_h;
          consent_need_res_ = need_res;
          consent_probed_ok_ = ok;
          consent_fps_source_ = probed.fps;
          emit consentRequired(duration / 60.0);
          return;
        }
        Dispatch(src, res_w, res_h, ok, probed.fps);
      },
      this);
}

void CompressBridge::resolveConsent(bool approved) {
  if (consent_src_.isEmpty()) {
    return;
  }
  const QString src = consent_src_;
  const int w = consent_w_;
  const int h = consent_h_;
  const bool probed_ok = consent_probed_ok_;
  const double fps_source = consent_fps_source_;
  consent_src_.clear();
  if (!approved) {
    status_text_ = tr("Dibatalkan: video panjang tanpa izin.");
    emit statusTextChanged();
    return;
  }
  // One-shot consent: it must not flip the user-facing forceLong toggle, and
  // the probe result is reused instead of falling back to the engine cap.
  consent_force_ = true;
  Dispatch(src, w, h, probed_ok, fps_source);
  consent_force_ = false;
}

void CompressBridge::Dispatch(const QString& src, int res_w, int res_h, bool probed_ok,
                              double fps_source) {
  const bool force_effective = force_long_ || consent_force_;
  CompressRequest req;
  {
    TabRequestInputs in;
    in.src = src;
    in.res_w = res_w;
    in.res_h = res_h;
    in.advanced = advanced_;
    in.adv_fps = fps_;
    in.adv_crf = crf_;
    in.adv_encoder = encoder_;
    in.adv_force = force_effective;
    in.simple_fps = fps_;
    in.simple_crf = crf_;
    in.out_dir = out_dir_.isEmpty() ? CompressController::DefaultWallpapersDir()
                                    : out_dir_;
    req = CompressController::BuildTabRequest(in);
  }
  req.force = force_effective;

  const int engine_fps_cap = CompressController::ResolveEngineFpsCap();
  const double usable_fps = (probed_ok && fps_source > 0.0 && std::isfinite(fps_source))
                                ? fps_source
                                : 0.0;
  req.fps = CompressController::ComputeEnqueueFps(usable_fps, engine_fps_cap);
  AppendLog(QStringLiteral("Enqueue: fps=%1 (source=%2, cap=%3) res=%4x%5")
                .arg(req.fps)
                .arg(usable_fps > 0.0 ? QString::number(usable_fps, 'f', 2)
                                      : QStringLiteral("?"))
                .arg(engine_fps_cap)
                .arg(req.res_w)
                .arg(req.res_h));

  JobMeta meta;
  meta.origin = JobMeta::Origin::kTab;
  meta.label = QFileInfo(src).fileName();
  meta.src = src;
  meta.register_library = true;
  meta.apply_after = false;  // QML calls Studio.applyWallpaper via autoApply
  meta.pending_out = req.out_path;

  QString error;
  const CompressService::EnqueueResult dispatch =
      controller_.EnqueueWithStatus(req, meta, &error);
  if (dispatch == CompressService::EnqueueResult::kInvalid) {
    // CompressService::fail already logged the technical string; map only what
    // the user reads.
    const QString friendly = FriendlyCompressError(error);
    status_text_ =
        tr("Kompres gagal dimulai: %1").arg(friendly);
    emit statusTextChanged();
    SetLastError(friendly);
    emit jobFinishedWithoutResult();
    return;
  }
  ClearLastError();
  // kStartFailed already has its own Finished queued; do not overwrite it.
  if (dispatch != CompressService::EnqueueResult::kStartFailed) {
    status_text_ =
        tr("Mengompres %1 ...").arg(QFileInfo(src).fileName());
    AppendLog(QStringLiteral("Mengompres di latar..."));
    emit statusTextChanged();
  }
  RefreshQueue();
}

void CompressBridge::cancel() {
  if (!controller_.IsRunning() && controller_.PendingCount() == 0) {
    return;
  }
  // Cancel means the whole batch: CancelCurrent alone lets the queue's next
  // job start as soon as the running one dies.
  controller_.CancelAll();
  AppendLog(QStringLiteral("Dibatalkan"));
  RefreshQueue();
}

}  // namespace k6wp
