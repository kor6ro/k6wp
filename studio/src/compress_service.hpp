#pragma once

// Single queued compression runner (Step 7.1): exactly ONE QProcess for
// compressor.exe plus a FIFO job queue, so two compressions can never run
// concurrently. CompressBridge owns exactly one of these.
//
// Wire contract: unchanged. compressor.exe argv and its NDJSON stdout
// ({"progress":0-100,"eta_s":N}, {"ok":true,...}) / stderr ({"error":...})
// are parsed exactly like the old bridge; only the ownership moves here.
//
// Header stays Win32-free (Qt + std only).

#include <QObject>
#include <QProcess>
#include <QString>

#include <deque>

namespace k6wp {

// Compression request: mirrors compressor CLI flags
// (--in/--out/--res WxH/--fps/--crf/--encoder).
struct CompressRequest {
  QString in_path;
  QString out_path;
  int res_w = 0;
  int res_h = 0;
  int fps = 30;
  int crf = 22;
  QString encoder = QStringLiteral("auto");
  bool force = false;   // --force-long: allow inputs longer than 10 minutes
  QString out_dir;      // override output directory (empty = default)
};

// Outcome detail of a finished job (subset of the compressor {"ok"} line).
struct CompressOkInfo {
  QString out;
  QString encoder;
  bool cache_hit = false;
  bool skip_optimal = false;
};

// Per-job routing metadata (who enqueued it, what to do on completion).
struct JobMeta {
  enum class Origin { kTab, kImport, kRecompress, kFirstRun };
  int job_id = 0;  // assigned by Enqueue; 0 = not yet queued
  QString label;
  QString src;
  QString pending_out;
  bool register_library = false;
  bool apply_after = false;
  Origin origin = Origin::kTab;
};

// Long-video consent threshold (10 minutes): jobs longer than this are
// refused unless the request carries force = true (--force-long) and the
// user confirms the dialog. Shared by the Compressor tab, ImportDialog and
// LibraryWidget.
inline constexpr double kLongVideoConsentSeconds = 600.0;

// Deduped output name: strips repeated "_k6wp[N]" suffixes from the input
// stem, then appends "_k6wp.mp4" (or "_k6wp2", "_k6wp3", ...) until the
// candidate is neither the input itself nor an existing file. Never yields
// "_k6wp_k6wp".
QString UniqueOutPath(const QString& dir, const QString& in_path);

class CompressService final : public QObject {
  Q_OBJECT

 public:
  explicit CompressService(QObject* parent = nullptr);
  ~CompressService() override;

  CompressService(const CompressService&) = delete;
  CompressService& operator=(const CompressService&) = delete;

  // Resolves compressor.exe next to the studio exe, falling back to the
  // build tree (mirrors ApplyManager::ResolveEnginePath).
  static QString ResolveCompressorPath();

  // Detailed outcome of EnqueueDetailed:
  //   kStarted     - the job launched immediately (compressor.exe started)
  //   kQueued      - the job is waiting behind a running one
  //   kInvalid     - request/launch invalid; not queued; *error_out set
  //   kStartFailed - the job was dequeued but compressor.exe failed to
  //                  launch; its Finished(false, ...) is already queued
  enum class EnqueueResult { kStarted, kQueued, kInvalid, kStartFailed };

  // Dispatch, with the outcome spelled out for callers that need to
  // distinguish launched-now / queued / launch-failed.
  EnqueueResult EnqueueDetailed(const CompressRequest& req, JobMeta& meta,
                                QString* error_out = nullptr);

  // Cache directory override for spawned compressor.exe processes
  // (K6WP_CACHE_DIR env, same key the compressor's cache manager reads).
  // Empty = inherit the process environment (no override).
  void SetCacheDir(const QString& dir) { cache_dir_ = dir; }

  // Kills the running job (its Finished(false, "Cancelled", ...) still
  // fires exactly once). No-op when idle.
  void CancelCurrent();
  // Drops every queued job, then CancelCurrent(). Queued jobs never started
  // get no Finished signal.
  void CancelAll();

  bool IsRunning() const { return running_; }
  // Number of jobs waiting (excludes the running one).
  int PendingCount() const { return static_cast<int>(queue_.size()); }

 signals:
  void Started(const JobMeta& meta);
  void Progress(int percent, int eta_s);
  void Finished(const JobMeta& meta, bool ok, const QString& technical,
                const CompressOkInfo& info);
  // Emitted once per batch when the queue drains (idle again).
  void QueueSummary(int ok_count, int fail_count);
  void LogMessage(const QString& line);

 private slots:
  void OnReadyReadStdout();
  void OnReadyReadStderr();
  void OnProcessFinished(int exit_code, QProcess::ExitStatus exit_status);

 private:
  struct Job {
    CompressRequest req;
    JobMeta meta;
  };

  bool StartNext();  // true iff a compressor.exe process was launched
  void HandleStartFailure(const QString& msg);
  void AppendLog(const QString& line);
  void DrainChannel(QProcess::ProcessChannel channel, QString& buf);
  void HandleStdoutLine(const QString& line);
  void HandleStderrLine(const QString& line);

  QProcess process_;  // the single runner: no ownership juggling, dtor kills
  QString cache_dir_;  // K6WP_CACHE_DIR override for spawned jobs (empty = none)
  std::deque<Job> queue_;
  bool running_ = false;
  bool has_current_ = false;
  Job current_;
  int next_job_id_ = 1;
  // Per-run parse state (reset in StartNext).
  bool cancelled_ = false;
  bool saw_ok_ = false;
  CompressOkInfo ok_info_;
  QString error_text_;
  QString stdout_buf_;
  QString stderr_buf_;
  // Batch accounting for QueueSummary.
  bool batch_active_ = false;
  int batch_ok_ = 0;
  int batch_fail_ = 0;
};

}  // namespace k6wp
