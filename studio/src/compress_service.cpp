#include "compress_service.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QProcessEnvironment>

#include "compress_args.hpp"

namespace k6wp {

QString UniqueOutPath(const QString& dir, const QString& in_path) {
  QString stem = QFileInfo(in_path).completeBaseName();
  QString base = stem;
  while (!base.isEmpty()) {
    const QString lower = base.toLower();
    const int idx = lower.lastIndexOf(QStringLiteral("_k6wp"));
    if (idx < 0) {
      break;
    }
    bool digits = true;
    for (int i = idx + 5; i < base.size(); ++i) {
      if (!base.at(i).isDigit()) {
        digits = false;
        break;
      }
    }
    if (!digits) {
      break;
    }
    base = base.left(idx);
  }
  if (base.trimmed().isEmpty()) {
    base = QStringLiteral("video");
  }
  const QDir out_dir(dir);
  const QString clean_in = QDir::cleanPath(in_path);
  QString candidate = base + QStringLiteral("_k6wp.mp4");
  int n = 2;
  while (QDir::cleanPath(out_dir.filePath(candidate)) == clean_in ||
         QFile::exists(out_dir.filePath(candidate))) {
    candidate = QStringLiteral("%1_k6wp%2.mp4").arg(base).arg(n);
    ++n;
  }
  return out_dir.filePath(candidate);
}

CompressService::CompressService(QObject* parent) : QObject(parent) {
  connect(&process_, &QProcess::readyReadStandardOutput, this,
          &CompressService::OnReadyReadStdout);
  connect(&process_, &QProcess::readyReadStandardError, this,
          &CompressService::OnReadyReadStderr);
  connect(&process_, &QProcess::finished, this,
          &CompressService::OnProcessFinished);
}

CompressService::~CompressService() {
  if (process_.state() != QProcess::NotRunning) {
    process_.kill();
    process_.waitForFinished(1000);
  }
}

QString CompressService::ResolveCompressorPath() {
  const QString exe_dir = QCoreApplication::applicationDirPath();
  const QString primary =
      QDir(exe_dir).filePath(QStringLiteral("compressor.exe"));
  if (QFile::exists(primary)) {
    return QDir::cleanPath(primary);
  }
  const QString fallback = QDir(exe_dir).filePath(
      QStringLiteral("../../build/msvc-dev/compressor.exe"));
  return QDir::cleanPath(fallback);
}

CompressService::EnqueueResult CompressService::EnqueueDetailed(
    const CompressRequest& req, JobMeta& meta, QString* error_out) {
  const auto fail = [this, error_out](const QString& msg) {
    AppendLog(msg);
    if (error_out != nullptr) {
      *error_out = msg;
    }
  };
  const QString exe_path = ResolveCompressorPath();
  if (!QFile::exists(exe_path)) {
    fail(QStringLiteral("Compress: compressor.exe not found at %1")
             .arg(exe_path));
    return EnqueueResult::kInvalid;
  }
  if (req.in_path.isEmpty() || req.out_path.isEmpty()) {
    fail(QStringLiteral("Compress: input/output path must not be empty"));
    return EnqueueResult::kInvalid;
  }
  meta.job_id = next_job_id_++;
  queue_.push_back(Job{req, meta});
  if (!batch_active_) {
    batch_active_ = true;
    batch_ok_ = 0;
    batch_fail_ = 0;
  }
  AppendLog(QStringLiteral("Compress: queued #%1 %2 (%3 waiting)")
                .arg(meta.job_id)
                .arg(meta.label.isEmpty() ? meta.src : meta.label)
                .arg(running_ ? static_cast<int>(queue_.size()) : 0));
  if (!running_) {
    return StartNext() ? EnqueueResult::kStarted
                       : EnqueueResult::kStartFailed;
  }
  return EnqueueResult::kQueued;
}

void CompressService::CancelCurrent() {
  if (!running_ || process_.state() == QProcess::NotRunning) {
    return;
  }
  AppendLog(QStringLiteral("Compress: cancelling job #%1 (kill) ...")
                .arg(current_.meta.job_id));
  cancelled_ = true;
  // M1 (1.3.0-beta.2): process_.kill() is TerminateProcess — it bypasses
  // compressor.exe's Ctrl+C handler and remove_partial() entirely (see the
  // same note in compressor/src/ffmpeg_job.cpp), and ffmpeg writes the
  // non-lockframe output directly to the final path. The partial file would
  // survive every cancel, so remove it HERE. Guard: never delete when the
  // output resolves to the input file (the compressor CLI refuses that pair,
  // but this delete must not be able to eat the user's source video).
  const QString partial = current_.req.out_path;
  const QString source = current_.req.in_path;
  process_.kill();
  process_.waitForFinished(1000);
  if (partial.isEmpty()) return;
  const QFileInfo partial_info(partial);
  if (!partial_info.exists()) return;
  if (!source.isEmpty() &&
      partial_info.absoluteFilePath().compare(
          QFileInfo(source).absoluteFilePath(), Qt::CaseInsensitive) == 0) {
    AppendLog(QStringLiteral("Compress: keeping '%1' (same file as input)")
                  .arg(partial));
    return;
  }
  if (QFile::remove(partial)) {
    AppendLog(QStringLiteral("Compress: partial output removed: %1")
                  .arg(partial));
  } else {
    AppendLog(QStringLiteral(
                  "Compress: partial output could not be removed (locked?): %1")
                  .arg(partial));
  }
}

void CompressService::CancelAll() {
  const int dropped = static_cast<int>(queue_.size());
  queue_.clear();
  if (dropped > 0) {
    AppendLog(QStringLiteral("Compress: dropped %1 queued job(s)").arg(dropped));
  }
  CancelCurrent();
  if (!running_ && batch_active_) {
    batch_active_ = false;
    emit QueueSummary(batch_ok_, batch_fail_);
  }
}

bool CompressService::StartNext() {
  if (running_ || queue_.empty()) {
    return false;
  }
  current_ = queue_.front();
  queue_.pop_front();
  has_current_ = true;

  const QString exe_path = ResolveCompressorPath();
  const CompressRequest& req = current_.req;
  // MED-15 argv contract: flag spelling/order lives in
  // shared/compress_args.hpp (single source of truth) — never hand-spell
  // "--in"/"--out"/... here. The golden dry-run CTest
  // (compress_argv_contract) replays this exact builder against the real
  // compressor.exe parser.
  k6wp::CompressArgs contract_args;
  contract_args.in = req.in_path.toStdString();
  contract_args.out = req.out_path.toStdString();
  contract_args.res_w = req.res_w;
  contract_args.res_h = req.res_h;
  contract_args.fps = req.fps;
  contract_args.crf = req.crf;
  contract_args.encoder = req.encoder.toStdString();
  contract_args.force_long = req.force;
  QStringList args;
  for (const std::string& tok : k6wp::BuildCompressArgv(contract_args)) {
    args << QString::fromStdString(tok);
  }
  AppendLog(QStringLiteral("Compress: starting #%1 %2 %3")
                .arg(current_.meta.job_id)
                .arg(exe_path, args.join(QLatin1Char(' '))));

  cancelled_ = false;
  saw_ok_ = false;
  ok_info_ = CompressOkInfo{};
  error_text_.clear();
  stdout_buf_.clear();
  stderr_buf_.clear();

  process_.setProgram(exe_path);
  process_.setArguments(args);
  process_.setProcessChannelMode(QProcess::SeparateChannels);
  if (!cache_dir_.isEmpty()) {
    // Studio cache_dir override: the compressor's cache manager reads this
    // same key (K6WP_CACHE_DIR wins over its built-in default).
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("K6WP_CACHE_DIR"), cache_dir_);
    process_.setProcessEnvironment(env);
  }
  process_.start();
  if (!process_.waitForStarted(5000)) {
    const QString msg = QStringLiteral("Compress: failed to start %1: %2")
                            .arg(exe_path, process_.errorString());
    AppendLog(msg);
    HandleStartFailure(msg);
    return false;
  }
  running_ = true;
  emit Started(current_.meta);
  return true;
}

// Launch-failure path: record the batch failure, drop the current job, and
// queue the Finished emission plus either the next job or the batch
// summary. Emissions are queued, not emitted, because StartNext may run
// inside Enqueue()'s call stack (called synchronously when idle): emitting
// directly would run their slots before Enqueue returns — the controller
// records its ledger entry only after Enqueue returns, and the tab widget
// still has post-enqueue UI writes pending — both would be
// overwritten/seen-missing. Posting order (per job first, summary last) is
// preserved by the event loop.
void CompressService::HandleStartFailure(const QString& msg) {
  ++batch_fail_;
  const JobMeta failed = current_.meta;
  has_current_ = false;
  current_ = Job{};
  QMetaObject::invokeMethod(
      this, [this, failed, msg]() {
        emit Finished(failed, false, msg, CompressOkInfo{});
      },
      Qt::QueuedConnection);
  if (!queue_.empty()) {
    StartNext();
  } else {
    batch_active_ = false;
    const int ok = batch_ok_;
    const int fail = batch_fail_;
    QMetaObject::invokeMethod(
        this, [this, ok, fail]() { emit QueueSummary(ok, fail); },
        Qt::QueuedConnection);
  }
}

void CompressService::OnReadyReadStdout() {
  DrainChannel(QProcess::StandardOutput, stdout_buf_);
}

void CompressService::OnReadyReadStderr() {
  DrainChannel(QProcess::StandardError, stderr_buf_);
}

void CompressService::OnProcessFinished(int exit_code,
                                        QProcess::ExitStatus exit_status) {
  if (!has_current_) {
    return;
  }
  // Drain anything left in the pipes before judging the outcome.
  DrainChannel(QProcess::StandardOutput, stdout_buf_);
  DrainChannel(QProcess::StandardError, stderr_buf_);
  running_ = false;
  has_current_ = false;
  const JobMeta meta = current_.meta;
  current_ = Job{};

  if (cancelled_) {
    cancelled_ = false;
    // M1 (1.3.0-beta.2): the old text claimed compressor.exe cleaned up the
    // partial — false on the kill path, which skips its cleanup entirely.
    // CancelCurrent removes it (and logs the outcome) before we get here.
    AppendLog(QStringLiteral(
                  "Compress: job #%1 cancelled by user; partial output "
                  "removed by Studio (the hard kill bypasses the "
                  "compressor's own cleanup)")
                  .arg(meta.job_id));
    ++batch_fail_;
    emit Finished(meta, false, QStringLiteral("Cancelled"), CompressOkInfo{});
  } else if (exit_status == QProcess::NormalExit && exit_code == 0 && saw_ok_) {
    ++batch_ok_;
    AppendLog(QStringLiteral("Compress: job #%1 finished OK: %2")
                  .arg(meta.job_id)
                  .arg(ok_info_.out));
    emit Finished(meta, true, QString(), ok_info_);
  } else {
    const QString detail =
        !error_text_.isEmpty()
            ? error_text_
            : QStringLiteral("compressor exited with code %1").arg(exit_code);
    AppendLog(QStringLiteral("Compress: job #%1 FAILED: %2; partial output "
                             "cleaned up by compressor.exe (Todo 16 verified)")
                  .arg(meta.job_id)
                  .arg(detail));
    ++batch_fail_;
    emit Finished(meta, false, detail, CompressOkInfo{});
  }

  if (!queue_.empty()) {
    StartNext();
  } else if (batch_active_) {
    batch_active_ = false;
    emit QueueSummary(batch_ok_, batch_fail_);
  }
}

void CompressService::AppendLog(const QString& line) {
  emit LogMessage(line);
}

void CompressService::DrainChannel(QProcess::ProcessChannel channel,
                                   QString& buf) {
  // LOW-5: cursor parse instead of remove(0, n) per line. The old loop
  // memmoved the whole remainder on every line (O(bytes * lines) on verbose
  // ffmpeg stderr); the cursor advances through the appended chunk once and
  // a single remove() drops the consumed prefix (O(bytes) total). Line
  // handling is unchanged: split on '\n', trim, skip empties, route stdout
  // lines to HandleStdoutLine and stderr lines to HandleStderrLine; a
  // trailing partial line stays buffered for the next ready-read.
  process_.setReadChannel(channel);
  buf.append(QString::fromUtf8(process_.readAll()));
  int pos = 0;
  int nl = -1;
  while ((nl = buf.indexOf(QLatin1Char('\n'), pos)) >= 0) {
    const QString line = buf.mid(pos, nl - pos).trimmed();
    pos = nl + 1;
    if (line.isEmpty()) {
      continue;
    }
    if (channel == QProcess::StandardOutput) {
      HandleStdoutLine(line);
    } else {
      HandleStderrLine(line);
    }
  }
  if (pos > 0) {
    buf.remove(0, pos);
  }
}

void CompressService::HandleStdoutLine(const QString& line) {
  QJsonParseError err{};
  const QJsonDocument doc =
      QJsonDocument::fromJson(line.toUtf8(), &err);
  if (err.error != QJsonParseError::NoError || !doc.isObject()) {
    return;  // non-JSON chatter (ffmpeg logs go to stderr anyway)
  }
  const QJsonObject obj = doc.object();
  if (obj.contains(QStringLiteral("progress"))) {
    const int pct = obj.value(QStringLiteral("progress")).toInt(-1);
    const int eta = obj.value(QStringLiteral("eta_s")).toInt(-1);
    if (pct < 0 || pct > 100) {
      return;
    }
    emit Progress(pct, eta);
    return;
  }
  if (obj.value(QStringLiteral("ok")).toBool(false)) {
    saw_ok_ = true;
    ok_info_.out = obj.value(QStringLiteral("out")).toString();
    ok_info_.encoder = obj.value(QStringLiteral("encoder")).toString();
    ok_info_.cache_hit = obj.value(QStringLiteral("cache_hit")).toBool(false);
    ok_info_.skip_optimal =
        obj.value(QStringLiteral("skip_optimal")).toBool(false);
    return;
  }
  if (obj.contains(QStringLiteral("error"))) {
    error_text_ = obj.value(QStringLiteral("error")).toString();
  }
}

void CompressService::HandleStderrLine(const QString& line) {
  // Compressor failures arrive as {"error":"..."} on stderr (exit 2/1).
  QJsonParseError err{};
  const QJsonDocument doc =
      QJsonDocument::fromJson(line.toUtf8(), &err);
  if (err.error == QJsonParseError::NoError && doc.isObject()) {
    const QJsonObject obj = doc.object();
    if (obj.contains(QStringLiteral("error"))) {
      error_text_ = obj.value(QStringLiteral("error")).toString();
      return;
    }
  }
  // Fill the fallback only when no structured {"error"} was captured, and keep
  // the FIRST line: later ffmpeg chatter (size=... time=...) must not overwrite
  // the actual reason the friendly-error mapping looks for.
  if (error_text_.isEmpty() && !line.isEmpty()) {
    error_text_ = line;
  }
}

}  // namespace k6wp
