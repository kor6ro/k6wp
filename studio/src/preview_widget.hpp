#pragma once

#include <QThread>
#include <QWidget>

#include <atomic>
#include <memory>
#include <mutex>
#include <string>

#include "thumbnailer.hpp"

struct mpv_handle;
class QLabel;
class QTimer;
class QShowEvent;
class QHideEvent;
class QEvent;
class QEnterEvent;
class QResizeEvent;
class QMouseEvent;

namespace k6wp {

// Preview load states (Todo 22), shared between the worker thread and the
// widget through std::atomic<PreviewLoadState>. kLive = mpv FILE_LOADED
// (rendering; covers paused too).
enum class PreviewLoadState {
  kIdle = 0,
  kLoading = 1,
  kLive = 2,
  kError = 3
};

// RAII guard for an mpv_handle: calls mpv_terminate_destroy() in the dtor.
// Move-only (project rule: no raw new/delete, no copies).
class MpvHandleGuard final {
 public:
  MpvHandleGuard() = default;
  explicit MpvHandleGuard(mpv_handle* handle) : handle_(handle) {}
  ~MpvHandleGuard();
  MpvHandleGuard(const MpvHandleGuard&) = delete;
  MpvHandleGuard& operator=(const MpvHandleGuard&) = delete;
  MpvHandleGuard(MpvHandleGuard&& other) noexcept;
  MpvHandleGuard& operator=(MpvHandleGuard&& other) noexcept;

  mpv_handle* get() const { return handle_; }
  mpv_handle* release();
  void reset(mpv_handle* handle = nullptr);

 private:
  mpv_handle* handle_ = nullptr;
};

// mpv playback worker: plain QThread subclass (run() override), no Q_OBJECT.
// The mpv_handle is created and destroyed inside run(); commands are
// dispatched from any thread through a mutex that also serializes against
// mpv_terminate_destroy() (mpv forbids concurrent calls on the same context
// while it is being destroyed).
// Preview load states shared with the widget via the enum-typed atomic; the
// human-readable detail lives in LastError()/error_detail_ (tr() applied
// widget-side, never in the worker).
class MpvWorker final : public QThread {
 public:
  MpvWorker(WId wid, std::string video, std::atomic<bool>& started,
            std::atomic<bool>& stop,
            std::atomic<PreviewLoadState>& load_state);
  ~MpvWorker() override;

  void LoadVideo(const std::string& path);
  void TogglePause();
  void SetPaused(bool paused);
  bool IsPaused() const;
  std::string LastError() const;

 protected:
  void run() override;

 private:
  WId wid_;
  std::string video_;
  std::atomic<bool>& started_;
  std::atomic<bool>& stop_;
  std::atomic<PreviewLoadState>& load_state_;
  std::string load_error_;        // ASCII detail, guarded by mutex_
  mutable std::mutex mutex_;  // guards handle_ + load_error_, serializes API
  mpv_handle* handle_ = nullptr;
};

// Video preview surface: a QWidget that embeds mpv via wid (winId) and
// exposes play/pause. No audio (audio=no). The video always letterboxes
// (mpv defaults: video-unscaled=no, panscan=0, no aspect override). The mpv
// worker starts lazily on the first showEvent (winId() must be called after
// the widget is shown, per spike Todo 4).
class PreviewWidget final : public QWidget {
  // Not for signals/slots (there are none): this is what makes the tr() calls
  // in statusText() resolve against "k6wp::PreviewWidget" instead of the
  // inherited "QObject", which is the context lupdate records them under.
  Q_OBJECT

 public:
  explicit PreviewWidget(QWidget* parent = nullptr);
  ~PreviewWidget() override;

  void LoadVideo(const QString& path);
  void TogglePlayPause();
  void SetPaused(bool paused);
  bool IsPlaying() const;
  QString statusText() const;

 protected:
  void showEvent(QShowEvent* event) override;
  void hideEvent(QHideEvent* event) override;
  void changeEvent(QEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void enterEvent(QEnterEvent* event) override;
  void leaveEvent(QEvent* event) override;

 private:
  void StartWorkerIfNeeded();
  // Poster-to-motion handoff: drop the thumbnail gate and start the worker
  // for the poster video (guarded so a stale timer never hijacks a newer
  // LoadVideo call).
  void StartLiveFromPoster();
  void StopWorker();
  void UpdateStatusLabel();
  void RefreshHoverBadge();

  std::string video_path_;
  int start_polls_ = 0;
  std::atomic<bool> started_{false};
  std::atomic<bool> stop_{false};
  std::atomic<PreviewLoadState> load_state_{PreviewLoadState::kIdle};
  QString error_detail_;
  std::unique_ptr<MpvWorker> worker_;
  QTimer* status_timer_ = nullptr;  // Qt parent-child owned
  QTimer* preview_timer_ = nullptr;
  QLabel* status_label_ = nullptr;
  QLabel* thumb_label_ = nullptr;  // cached-thumbnail mode (no mpv decode)
  QLabel* hover_badge_ = nullptr;
  bool hovered_ = false;
  Thumbnailer thumb_;
  bool using_thumbnail_ = false;
  // Poster video guarded by the 200 ms handoff timer (stale timers never
  // hijack a newer LoadVideo call).
  std::string thumb_video_;
  // P2.8: true when the mpv worker was playing at hide/minimize time, arming
  // a one-shot resume on the matching show/restore. GUI-thread only (all
  // hide/show/change events run on the GUI thread); never armed while
  // paused, so restore never phantom-resumes.
  bool wasPlaying_ = false;
};

}  // namespace k6wp