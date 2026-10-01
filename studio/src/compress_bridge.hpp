#pragma once

// Phase 3 of the Widgets -> QML migration: the Kompresor tab's backend.
//
// CompressBridge owns a CompressController BY VALUE (CompressController is NOT
// a QObject, so it cannot carry Q_PROPERTY itself) and republishes the queue as
// primitives. This mirrors how StudioBridge holds IpcClient and ApplyManager.
//
// Two deliberate constraints:
//
//  1. Only primitives cross into QML. CompressService's signals carry JobMeta
//     and CompressOkInfo, which are not registered metatypes - they work today
//     only because CompressService, its receivers and QProcess all live on the
//     GUI thread, so the connections are direct. Republishing them to QML would
//     need Q_DECLARE_METATYPE, so the signal wiring stays in this .cpp and QML
//     sees int / QString / bool.
//
//  2. The long-video consent gate is a QMessageBox in compress_errors.cpp, so
//     it cannot run here. Instead the gate raises consentRequired(); QML shows a
//     Material Dialog and calls resolveConsent(), which continues or abandons
//     the enqueue.

#include <QObject>
#include <QString>
#include <QStringList>

#include <QtQml/qqmlregistration.h>

#include "compress_controller.hpp"
#include "library_manager.hpp"
#include "studio_settings.hpp"

namespace k6wp {

// NOT `final`: QML type registration instantiates a QQmlElement<T> subclass, so
// a final QML_ELEMENT class does not compile (C3246).
class CompressBridge : public QObject {
  // Registered as `Compress`; QML_NAMED_ELEMENT is required because bare
  // QML_ELEMENT would register it as `CompressBridge` and every `Compress.*`
  // binding would silently keep its default with no QML error.
  Q_OBJECT
  QML_NAMED_ELEMENT(Compress)
  QML_ELEMENT
  QML_SINGLETON

 public:
  explicit CompressBridge(QObject* parent = nullptr);
  ~CompressBridge() override;

  // --- Inputs ---------------------------------------------------------------

  Q_PROPERTY(QString sourcePath READ sourcePath NOTIFY inputsChanged)
  Q_PROPERTY(QString outDir READ outDir NOTIFY inputsChanged)
  Q_PROPERTY(bool advanced READ advanced NOTIFY inputsChanged)
  Q_PROPERTY(int crf READ crf NOTIFY inputsChanged)
  Q_PROPERTY(int fps READ fps NOTIFY inputsChanged)
  Q_PROPERTY(QString encoder READ encoder NOTIFY inputsChanged)
  Q_PROPERTY(QString resolutionText READ resolutionText WRITE setResolutionText
                 NOTIFY inputsChanged)
  Q_PROPERTY(bool forceLong READ forceLong NOTIFY inputsChanged)
  Q_PROPERTY(bool autoApply READ autoApply NOTIFY autoApplyChanged)

  QString sourcePath() const { return source_path_; }
  QString outDir() const { return out_dir_; }
  bool advanced() const { return advanced_; }
  int crf() const { return crf_; }
  int fps() const { return fps_; }
  QString encoder() const { return encoder_; }
  QString resolutionText() const { return resolution_text_; }
  bool forceLong() const { return force_long_; }
  bool autoApply() const { return auto_apply_; }

  // --- Queue state ----------------------------------------------------------

  Q_PROPERTY(bool running READ running NOTIFY queueChanged)
  Q_PROPERTY(int pending READ pending NOTIFY queueChanged)
  Q_PROPERTY(int progress READ progress NOTIFY progressChanged)
  Q_PROPERTY(QString etaText READ etaText NOTIFY progressChanged)
  Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
  Q_PROPERTY(QString detailText READ detailText NOTIFY statusTextChanged)
  Q_PROPERTY(QString queueText READ queueText NOTIFY queueChanged)
  // Output of the last successful job; empty when there is nothing to apply.
  Q_PROPERTY(QString resultPath READ resultPath NOTIFY resultChanged)
  Q_PROPERTY(bool hasResult READ hasResult NOTIFY resultChanged)
  Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
  Q_PROPERTY(QStringList log READ log NOTIFY logChanged)

  bool running() const { return controller_.IsRunning(); }
  int pending() const { return controller_.PendingCount(); }
  int progress() const { return progress_; }
  QString etaText() const;
  QString statusText() const { return status_text_; }
  QString detailText() const { return detail_text_; }
  QString queueText() const;
  QString resultPath() const { return result_path_; }
  bool hasResult() const { return !result_path_.isEmpty(); }
  QString lastError() const { return last_error_; }
  QStringList log() const { return log_; }

  // --- Invokables -----------------------------------------------------------

  Q_INVOKABLE void pickSource();
  Q_INVOKABLE void setSourcePath(const QString& path);
  Q_INVOKABLE void pickOutDir();
  Q_INVOKABLE void openOutDir();
  Q_INVOKABLE void openResultDir();

  Q_INVOKABLE void setAdvanced(bool on);
  Q_INVOKABLE void setCrf(int value);
  Q_INVOKABLE void setFps(int value);
  Q_INVOKABLE void setEncoder(const QString& name);
  Q_INVOKABLE void setResolutionText(const QString& text);
  Q_INVOKABLE void setForceLong(bool on);
  Q_INVOKABLE void setAutoApply(bool on);

  // Re-reads the Kompresor defaults from studio_settings.json so a change saved
  // in the Pengaturan tab applies to the next job without a restart.
  Q_INVOKABLE void refreshDefaults();

  // Probes, then enqueues. Blocking work (ffprobe) runs off the GUI thread.
  Q_INVOKABLE void start();
  Q_INVOKABLE void cancel();
  // Answer to consentRequired(): true continues with force-long, false abandons.
  Q_INVOKABLE void resolveConsent(bool approved);
  Q_INVOKABLE void clearResult();

 signals:
  void inputsChanged();
  void queueChanged();
  void progressChanged();
  void statusTextChanged();
  void resultChanged();
  // resultChanged() never fires for a job that ends without an output, so
  // QML needs a separate hook to drop a pending one-shot apply intent.
  void jobFinishedWithoutResult();
  void lastErrorChanged();
  void autoApplyChanged();
  void logChanged();
  // durationMinutes > 10: the UI must confirm before the job is queued.
  void consentRequired(double durationMinutes);

 private:
  void AppendLog(const QString& line);
  void SetLastError(const QString& error);
  void ClearLastError();
  void RefreshQueue();
  // Shared tail of start(): builds the request, probes, gates, and enqueues.
  void EnqueueWithProbe(const QString& src, int res_w, int res_h, bool need_res);
  // probed_ok=false makes the enqueue fps fall back to the engine cap.
  void Dispatch(const QString& src, int res_w, int res_h, bool probed_ok,
                double fps_source);

  CompressController controller_;
  LibraryManager library_mgr_;
  StudioSettings settings_;

  QString source_path_;
  QString out_dir_;
  bool advanced_ = false;
  int crf_ = 22;
  int fps_ = 30;
  QString encoder_ = QStringLiteral("auto");
  QString resolution_text_;
  bool force_long_ = false;
  bool auto_apply_ = false;

  int progress_ = 0;
  int eta_s_ = -1;
  // Set in the constructor: user-facing text must go through tr() so it lands
  // in this class's translation context.
  QString status_text_;
  QString detail_text_;
  QString result_path_;
  QString last_error_;
  QStringList log_;

  // Long-video gate state, held between consentRequired() and resolveConsent().
  QString consent_src_;
  int consent_w_ = 0;
  int consent_h_ = 0;
  bool consent_need_res_ = false;
  bool consent_probed_ok_ = false;
  double consent_fps_source_ = 0.0;
  bool consent_force_ = false;
};

}  // namespace k6wp
