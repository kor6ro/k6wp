#pragma once

// CompressController (MED-5 part 2, commit 1): thin owner around the single
// queued CompressService. Pure refactor — no behavior change.
//
// MainWindow keeps all widget wiring and completion slots; this class owns:
//   - the CompressService instance (borrowed by LibraryWidget/ImportDialog
//     via service(), same lifetime as before when it was a direct member),
//   - the per-job CompressRequest ledger (was MainWindow::compress_reqs_),
//   - the tab request-building pure logic (was MainWindow::BuildTabRequest /
//     ResolveTabOutDir / ResolveSimpleRes / DefaultWallpapersDir).
//
// Header stays Win32-free (Qt + std only, like compress_service.hpp).

#include <QString>

#include <map>
#include <string>

#include "compress_service.hpp"

namespace k6wp {

struct VideoMetadata;

// Explicit inputs for building a Compressor-tab request. MainWindow fills
// these from its widgets + StudioSettings; the controller stays widget-free
// so the mapping is unit-testable without QApplication.
struct TabRequestInputs {
  QString src;
  int res_w = 1280;
  int res_h = 720;
  bool advanced = false;  // true = use adv_* below; false = simple-mode safe
                          // defaults (CRF 22, FPS 30, encoder auto)
  int adv_fps = 30;
  int adv_crf = 22;
  QString adv_encoder = QStringLiteral("auto");
  bool adv_force = false;
  QString out_name_override;  // filename override (empty = auto UniqueOutPath)
  int simple_fps = 30;
  int simple_crf = 22;
  QString out_dir;  // resolved output directory (never empty)
};

class CompressController {
 public:
  CompressController() = default;
  ~CompressController() = default;

  CompressController(const CompressController&) = delete;
  CompressController& operator=(const CompressController&) = delete;

  // The single queued service (borrowed pointers stay valid for the
  // controller's lifetime; identical to the old by-value member).
  CompressService* service() { return &service_; }
  const CompressService* service() const { return &service_; }

  // Enqueue wrapper: delegates to the service and records the request in
  // the ledger on success (was the two-line sequence in
  // MainWindow::PrepareAndEnqueue). Returns false + *error_out without
  // queueing when the request/launch is invalid.
  bool Enqueue(const CompressRequest& req, JobMeta& meta,
               QString* error_out = nullptr);

  // Same dispatch as Enqueue, with the outcome spelled out so callers can
  // distinguish launched-now / queued / launch-failed (the launch-failed
  // job's Finished(false, ...) is already queued by the service).
  CompressService::EnqueueResult EnqueueWithStatus(
      const CompressRequest& req, JobMeta& meta,
      QString* error_out = nullptr);

  // Takes (and removes) the ledger entry for a finished job; *out is empty
  // when the id is unknown (was the find-or-empty lookup in
  // MainWindow::OnServiceFinished).
  bool TakeRequest(int job_id, CompressRequest* out);

  // Single-job queue introspection (pass-throughs; the queue stays
  // single-job: exactly one QProcess plus a FIFO, owned by the service).
  bool IsRunning() const { return service_.IsRunning(); }
  int PendingCount() const { return service_.PendingCount(); }
  void CancelCurrent() { service_.CancelCurrent(); }

  // Pushes the studio cache dir into the service (K6WP_CACHE_DIR override).
  void SetCacheDir(const QString& dir) { service_.SetCacheDir(dir); }

  // Default output folder (%LOCALAPPDATA%/K6WP/wallpapers, profile/temp
  // fallbacks). Was MainWindow::DefaultWallpapersDir.
  static QString DefaultWallpapersDir();

  // Resolves the effective output dir: explicit override wins, then the
  // settings dir, then the default above (created on disk). Was
  // MainWindow::ResolveTabOutDir.
  static QString ResolveTabOutDir(const QString& override_dir,
                                  const QString& settings_dir);

  // Simple-mode target resolution from a resolution-mode string:
  // match_monitor = primary monitor native size, source = probed input size,
  // 720p/1080p/2160p = fixed. Falls back to 1280x720. Probe-free unless mode
  // is "source" (only "source" consults probed; nullptr = no probe ran).
  // Was the anonymous-namespace ResolveSimpleRes in main_window.cpp.
  static void ResolveSimpleRes(const std::string& mode,
                               const VideoMetadata* probed, int& w, int& h);

  // P1.3 enqueue fps policy: min(source, engine fps_cap), rounded, clamped
  // to >= 1; unknown/unparseable/<=0/NaN source -> engine cap (never
  // hardcoded 30, never 0/negative in argv).
  static int ComputeEnqueueFps(double fps_source, int engine_fps_cap);

  // Fresh-read of the engine fps cap so external config edits apply without
  // a restart; a corrupt config falls back to the "<config>.bak"
  // (keep-last-valid path), then to the schema default. Shared by
  // CompressorTabWidget and ImportDialog (was duplicated in both).
  static int ResolveEngineFpsCap();

  // Builds a tab compress request (output dir, quality, resolution). The
  // compressor CLI requires an explicit positive WxH (it rejects 0x0), so
  // the caller resolves "source" mode via ResolveSimpleRes first. Was
  // MainWindow::BuildTabRequest.
  static CompressRequest BuildTabRequest(const TabRequestInputs& in);

 private:
  CompressService service_;  // the single runner: one QProcess + FIFO queue
  // Live request per queued job (for library registration on completion).
  std::map<int, CompressRequest> requests_;
};

}  // namespace k6wp
