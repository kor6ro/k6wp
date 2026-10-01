#include "preview_widget.hpp"

#include <mpv/client.h>

#include <QEvent>
#include <QFile>
#include <QLabel>
#include <QMouseEvent>
#include <QPixmap>
#include <QStyle>
#include <QTimer>

#include <cstdio>

#include "user_errors.hpp"

// Task 23: production logging. The informational "worker up" mirror goes to
// stderr only in verbose builds (root CMake option K6WP_VERBOSE); the two
// hard-failure lines below stay ungated (error paths are never gated).
#ifndef K6WP_VERBOSE
#define K6WP_VERBOSE 0
#endif

namespace k6wp {

// --- MpvHandleGuard ---------------------------------------------------------

MpvHandleGuard::~MpvHandleGuard() {
  if (handle_) mpv_terminate_destroy(handle_);
}

MpvHandleGuard::MpvHandleGuard(MpvHandleGuard&& other) noexcept
    : handle_(other.release()) {}

MpvHandleGuard& MpvHandleGuard::operator=(MpvHandleGuard&& other) noexcept {
  if (this != &other) reset(other.release());
  return *this;
}

mpv_handle* MpvHandleGuard::release() {
  mpv_handle* h = handle_;
  handle_ = nullptr;
  return h;
}

void MpvHandleGuard::reset(mpv_handle* handle) {
  if (handle_ != handle) {
    if (handle_) mpv_terminate_destroy(handle_);
    handle_ = handle;
  }
}

// --- MpvWorker --------------------------------------------------------------

MpvWorker::MpvWorker(WId wid, std::string video, std::atomic<bool>& started,
                     std::atomic<bool>& stop,
                     std::atomic<PreviewLoadState>& load_state)
    : wid_(wid),
      video_(std::move(video)),
      started_(started),
      stop_(stop),
      load_state_(load_state) {}

MpvWorker::~MpvWorker() = default;

void MpvWorker::run() {
  MpvHandleGuard mpv(mpv_create());
  if (!mpv.get()) {
    std::fprintf(stderr, "[preview] mpv_create failed\n");
    return;
  }

  mpv_set_option_string(mpv.get(), "wid",
                        std::to_string(static_cast<long long>(wid_)).c_str());
  mpv_set_option_string(mpv.get(), "hwdec", "d3d11va");
  mpv_set_option_string(mpv.get(), "audio", "no");
  mpv_set_option_string(mpv.get(), "vo", "gpu");
  mpv_set_option_string(mpv.get(), "keep-open", "yes");
  mpv_set_option_string(mpv.get(), "loop-file", "inf");
  mpv_set_option_string(mpv.get(), "video-unscaled", "no");
  mpv_set_option_string(mpv.get(), "panscan", "0");
  mpv_set_option_string(mpv.get(), "video-zoom", "0");
  mpv_set_option_string(mpv.get(), "video-aspect-override", "0");
  // P1.2 lean subset (preview keeps vo=gpu; video-sync intentionally left
  // at the mpv default — display-desync is engine-only by design).
  mpv_set_option_string(mpv.get(), "scale", "bilinear");
  mpv_set_option_string(mpv.get(), "dscale", "bilinear");
  mpv_set_option_string(mpv.get(), "cscale", "bilinear");
  mpv_set_option_string(mpv.get(), "swapchain-depth", "2");
  mpv_set_option_string(mpv.get(), "load-scripts", "no");
  mpv_set_option_string(mpv.get(), "osc", "no");
  mpv_set_option_string(mpv.get(), "sub-auto", "no");
  mpv_set_option_string(mpv.get(), "audio-file-auto", "no");
  mpv_set_option_string(mpv.get(), "demuxer-max-bytes", "16MiB");
  mpv_set_option_string(mpv.get(), "demuxer-max-back-bytes", "4MiB");

  if (mpv_initialize(mpv.get()) < 0) {
    std::fprintf(stderr, "[preview] mpv_initialize failed\n");
    return;
  }
  mpv_request_log_messages(mpv.get(), "info");

  {
    std::lock_guard<std::mutex> lock(mutex_);
    handle_ = mpv.get();
  }

  if (!video_.empty()) {
    const char* cmd[] = {"loadfile", video_.c_str(), nullptr};
    mpv_command(mpv.get(), cmd);
  }
  started_.store(true);

  // P1.2 evidence line: unconditional (NOT behind K6WP_VERBOSE) so the
  // applied lean subset plus the effective video-sync value are provable
  // in a normal build — this is what shows the engine/preview divergence
  // is real and intentional.
  char* hwdec = mpv_get_property_string(mpv.get(), "hwdec-current");
  char* vsync = mpv_get_property_string(mpv.get(), "video-sync");
  std::fprintf(stderr,
               "[preview] options applied (vo=gpu hwdec=d3d11va "
               "scale/dscale/cscale=bilinear swapchain-depth=2 "
               "load-scripts=no osc=no sub-auto=no audio-file-auto=no "
               "demuxer-max-bytes=16MiB demuxer-max-back-bytes=4MiB "
               "video-sync=%s hwdec-current=%s)\n",
               vsync ? vsync : "?", hwdec ? hwdec : "?");
#if K6WP_VERBOSE
  std::fprintf(stderr, "[preview] worker up (hwdec-current=%s)\n",
               hwdec ? hwdec : "?");
#endif
  mpv_free(hwdec);
  mpv_free(vsync);

  while (!stop_.load()) {
    mpv_event* ev = mpv_wait_event(mpv.get(), 100);
    if (ev->event_id == MPV_EVENT_SHUTDOWN) break;
    if (ev->event_id == MPV_EVENT_FILE_LOADED) {
      load_state_.store(PreviewLoadState::kLive);
      std::lock_guard<std::mutex> lock(mutex_);
      load_error_.clear();
    } else if (ev->event_id == MPV_EVENT_END_FILE) {
      const auto* end = static_cast<mpv_event_end_file*>(ev->data);
      if (end != nullptr && end->reason == MPV_END_FILE_REASON_ERROR) {
        load_state_.store(PreviewLoadState::kError);
        std::lock_guard<std::mutex> lock(mutex_);
        load_error_ = mpv_error_string(end->error);
        // libmpv's text is the diagnostic; the label shows a mapped sentence
        // instead, so the raw string is logged here where it is still useful.
        std::fprintf(stderr, "[preview] mpv load error: %s\n",
                     load_error_.c_str());
        std::fflush(stderr);
      }
    }
  }

  {
    std::lock_guard<std::mutex> lock(mutex_);
    handle_ = nullptr;
    mpv_terminate_destroy(mpv.release());
  }
}

void MpvWorker::LoadVideo(const std::string& path) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!handle_) return;
  load_state_.store(PreviewLoadState::kLoading);
  load_error_.clear();
  const char* cmd[] = {"loadfile", path.c_str(), nullptr};
  mpv_command(handle_, cmd);
}

std::string MpvWorker::LastError() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return load_error_;
}

void MpvWorker::TogglePause() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!handle_) return;
  const char* cmd[] = {"cycle", "pause", nullptr};
  mpv_command(handle_, cmd);
}

void MpvWorker::SetPaused(bool paused) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!handle_) return;
  mpv_set_property_string(handle_, "pause", paused ? "yes" : "no");
}

bool MpvWorker::IsPaused() const {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!handle_) return false;
  int flag = 0;
  if (mpv_get_property(handle_, "pause", MPV_FORMAT_FLAG, &flag) < 0) return false;
  return flag != 0;
}

// --- PreviewWidget ----------------------------------------------------------

PreviewWidget::PreviewWidget(QWidget* parent) : QWidget(parent) {
  setAttribute(Qt::WA_NativeWindow);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  setMinimumSize(320, 180);

  status_label_ = new QLabel(this);
  status_label_->setObjectName(QStringLiteral("preview_status_label"));
  status_label_->setAlignment(Qt::AlignCenter);
  status_label_->setWordWrap(true);
  status_label_->setTextInteractionFlags(Qt::TextSelectableByMouse);

  thumb_label_ = new QLabel(this);
  thumb_label_->setObjectName(QStringLiteral("preview_thumb_label"));
  thumb_label_->setAlignment(Qt::AlignCenter);
  thumb_label_->hide();
  hover_badge_ = new QLabel(this);
  hover_badge_->setObjectName(QStringLiteral("preview_hover_badge"));
  hover_badge_->setAlignment(Qt::AlignCenter);
  hover_badge_->setAttribute(Qt::WA_TransparentForMouseEvents);
  hover_badge_->setStyleSheet(
      QStringLiteral("background-color: rgba(0, 0, 0, 140); color: white; "
                     "border-radius: 32px; font-size: 28px;"));
  hover_badge_->setFixedSize(64, 64);
  hover_badge_->hide();
  setToolTip(tr("Klik untuk jeda/jalan"));

  status_timer_ = new QTimer(this);
  status_timer_->setInterval(50);
  connect(status_timer_, &QTimer::timeout, this, [this] {
    if (started_.load()) {
      status_timer_->stop();
      UpdateStatusLabel();
    } else if (++start_polls_ > 100) {
      status_timer_->stop();
      load_state_.store(PreviewLoadState::kError);
      error_detail_ = tr("backend video gagal dimulai "
                         "(libmpv hilang atau rusak)");
      UpdateStatusLabel();
    }
  });

  preview_timer_ = new QTimer(this);
  preview_timer_->setInterval(250);
  connect(preview_timer_, &QTimer::timeout, this, [this] {
    if (worker_ && started_.load()) {
      // load_state_ is the worker's own shared atomic (passed by reference
      // at construction), so reading it here is the live worker state.
      const PreviewLoadState state = load_state_.load();
      if (state == PreviewLoadState::kError && error_detail_.isEmpty()) {
        const std::string detail = worker_->LastError();
        error_detail_ = FriendlyPreviewError(QString::fromUtf8(detail.c_str()));
      }
      if (state == PreviewLoadState::kLive && thumb_label_ != nullptr) {
        thumb_label_->hide();
      }
      UpdateStatusLabel();
      if (hovered_) {
        RefreshHoverBadge();
      }
    }
  });
  preview_timer_->start();
  UpdateStatusLabel();
}

PreviewWidget::~PreviewWidget() {
  if (status_timer_ != nullptr) status_timer_->stop();
  if (preview_timer_ != nullptr) preview_timer_->stop();
  StopWorker();
}

void PreviewWidget::LoadVideo(const QString& path) {
  if (path.isEmpty()) {
    video_path_.clear();
    load_state_.store(PreviewLoadState::kIdle);
    error_detail_.clear();
    using_thumbnail_ = false;
    if (thumb_label_ != nullptr) thumb_label_->hide();
    UpdateStatusLabel();
    return;
  }
  if (!QFile::exists(path)) {
    video_path_ = path.toUtf8().toStdString();
    load_state_.store(PreviewLoadState::kError);
    error_detail_ = tr("file tidak ditemukan: %1").arg(path);
    using_thumbnail_ = false;
    if (thumb_label_ != nullptr) thumb_label_->hide();
    UpdateStatusLabel();
    return;
  }
  video_path_ = path.toUtf8().toStdString();
  load_state_.store(PreviewLoadState::kLoading);
  error_detail_.clear();

  // Thumbnail-first: the cached frame (thumbnailer.cpp) is already decoded
  // at import time, so showing it avoids a second full mpv decode of the
  // same video. Fall back to mpv when no cached thumbnail exists.
  const std::filesystem::path thumb =
      thumb_.CachedThumb(std::filesystem::path(path.toStdWString()));
  std::error_code ec;
  if (!thumb.empty() && std::filesystem::is_regular_file(thumb, ec) && !ec) {
    using_thumbnail_ = true;
    thumb_video_ = video_path_;
    if (thumb_label_ != nullptr) {
      thumb_label_->setPixmap(
          QPixmap(QString::fromStdWString(thumb.wstring())));
      thumb_label_->setGeometry(rect());
      thumb_label_->show();
      thumb_label_->raise();
    }
    // The raise above buries a badge shown while hovered; in pure thumbnail
    // mode no worker runs, so the 250 ms tick never re-raises it — do it now.
    RefreshHoverBadge();
    StopWorker();
    UpdateStatusLabel();
    // Poster first, motion right after: the cached frame shows instantly
    // while mpv spins up; hide the poster once frames flow (state 2).
    QTimer::singleShot(200, this, [this]() { StartLiveFromPoster(); });
    return;
  }
  using_thumbnail_ = false;
  if (thumb_label_ != nullptr) thumb_label_->hide();
  if (worker_) worker_->LoadVideo(video_path_);
  UpdateStatusLabel();
}

void PreviewWidget::TogglePlayPause() {
  if (using_thumbnail_) {
    StartLiveFromPoster();
    return;
  }
  if (worker_) worker_->TogglePause();
  RefreshHoverBadge();
}

void PreviewWidget::SetPaused(bool paused) {
  if (using_thumbnail_) return;
  if (worker_) worker_->SetPaused(paused);
  RefreshHoverBadge();
}

bool PreviewWidget::IsPlaying() const {
  if (using_thumbnail_) return false;
  if (!worker_ || !started_.load()) return false;
  return !worker_->IsPaused();
}

void PreviewWidget::showEvent(QShowEvent* event) {
  QWidget::showEvent(event);
  if (status_label_ != nullptr) status_label_->setGeometry(rect());
  if (thumb_label_ != nullptr) thumb_label_->setGeometry(rect());
  RefreshHoverBadge();
  StartWorkerIfNeeded();
  // P2.8: resume only if the worker was playing when hidden (one-shot: the
  // flag is consumed here so a paired changeEvent restore is a safe no-op).
  // Lazy-start on first show preserved: wasPlaying_ is false until a
  // hideEvent/changeEvent arms it.
  if (wasPlaying_) {
    wasPlaying_ = false;
    if (!using_thumbnail_ && worker_ && started_.load()) {
      worker_->SetPaused(false);
      std::fprintf(stderr, "[preview] shown -> resume (was playing)\n");
    }
  }
}

void PreviewWidget::hideEvent(QHideEvent* event) {
  QWidget::hideEvent(event);
  // P2.8: hidden (tab switch; also fires on minimize) -> pause the mpv
  // worker so only the engine decode stays active. Latch semantics: arm
  // wasPlaying_ only when actually playing, never clear an armed flag here
  // (a paired WindowStateChange for the same minimize sequence must not wipe
  // it — the matching show/restore consumes it exactly once).
  if (IsPlaying()) {
    wasPlaying_ = true;
    SetPaused(true);
    std::fprintf(stderr, "[preview] hidden -> pause (was playing)\n");
  } else if (wasPlaying_) {
    std::fprintf(stderr, "[preview] hidden (resume still armed)\n");
  } else {
    std::fprintf(stderr, "[preview] hidden (idle/paused, no resume armed)\n");
  }
}

void PreviewWidget::changeEvent(QEvent* event) {
  QWidget::changeEvent(event);
  if (event == nullptr || event->type() != QEvent::WindowStateChange) return;
  // P2.8: minimized -> force pause + arm wasPlaying_ iff actually playing,
  // so restore never phantom-resumes a minimized-while-paused preview.
  if (isMinimized()) {
    if (IsPlaying()) {
      wasPlaying_ = true;
      SetPaused(true);
      std::fprintf(stderr, "[preview] minimized -> pause (was playing)\n");
    } else if (wasPlaying_) {
      std::fprintf(stderr, "[preview] minimized (resume still armed)\n");
    } else {
      std::fprintf(stderr,
                   "[preview] minimized (idle/paused, no resume armed)\n");
    }
    return;
  }
  // Restored without a matching showEvent (widget stayed visible under a
  // minimized top-level): resume here; guarded by !isHidden() so a restore
  // while tabbed away keeps the flag armed for the later showEvent.
  if (wasPlaying_ && !isHidden()) {
    wasPlaying_ = false;
    if (!using_thumbnail_ && worker_ && started_.load()) {
      SetPaused(false);
      std::fprintf(stderr, "[preview] restored -> resume (was playing)\n");
    }
  }
}

void PreviewWidget::resizeEvent(QResizeEvent* event) {
  QWidget::resizeEvent(event);
  if (status_label_ != nullptr) status_label_->setGeometry(rect());
  RefreshHoverBadge();
  if (thumb_label_ != nullptr) {
    thumb_label_->setGeometry(rect());
    const QPixmap pix = thumb_label_->pixmap();
    if (!pix.isNull()) {
      thumb_label_->setPixmap(
          pix.scaled(size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
  }
}

void PreviewWidget::StartWorkerIfNeeded() {
  if (worker_ || using_thumbnail_) return;
  const WId wid = winId();  // forces native window creation (spike Todo 4)
  if (!video_path_.empty()) {
    load_state_.store(PreviewLoadState::kLoading);
  }
  // Required, not redundant: `stop_` is one flag shared by every MpvWorker
  // and StopWorker() latches it true. Without this clear, a worker started
  // after any StopWorker() skips its event loop entirely, so FILE_LOADED never
  // arrives and the status freezes on "Memuat pratinjau...".
  stop_.store(false);
  worker_ = std::make_unique<MpvWorker>(wid, video_path_, started_, stop_,
                                        load_state_);
  worker_->start();
  start_polls_ = 0;
  status_timer_->start();
}

void PreviewWidget::StartLiveFromPoster() {
  if (!using_thumbnail_ || video_path_.empty() ||
      video_path_ != thumb_video_) {
    return;
  }
  using_thumbnail_ = false;
  if (!worker_) {
    StartWorkerIfNeeded();
  } else {
    worker_->LoadVideo(video_path_);
  }
}

void PreviewWidget::mousePressEvent(QMouseEvent* event) {
  QWidget::mousePressEvent(event);
  if (event->button() == Qt::LeftButton) {
    TogglePlayPause();
  }
}

// Task 28 (LOW-25): hover pause/play overlay. enterEvent arms the badge,
// leaveEvent drops it; RefreshHoverBadge centers a semi-transparent
// pause/play glyph (QStyle standard icons) reflecting the actual worker
// pause state, so hover always shows the action a click would take.
void PreviewWidget::enterEvent(QEnterEvent* event) {
  QWidget::enterEvent(event);
  hovered_ = true;
  RefreshHoverBadge();
}

void PreviewWidget::leaveEvent(QEvent* event) {
  QWidget::leaveEvent(event);
  hovered_ = false;
  if (hover_badge_ != nullptr) hover_badge_->hide();
}

void PreviewWidget::RefreshHoverBadge() {
  if (hover_badge_ == nullptr) return;
  // Only when a click would toggle something: poster/thumbnail mode or a
  // live (kLive) worker. Idle/loading/error states show no badge.
  const bool toggleable =
      using_thumbnail_ || load_state_.load() == PreviewLoadState::kLive;
  if (!hovered_ || !toggleable) {
    hover_badge_->hide();
    return;
  }
  // Paused (or poster, not yet live) -> play glyph; playing -> pause glyph.
  bool paused = true;
  if (!using_thumbnail_ && worker_ != nullptr && started_.load()) {
    paused = worker_->IsPaused();
  }
  const QStyle::StandardPixmap glyph =
      paused ? QStyle::SP_MediaPlay : QStyle::SP_MediaPause;
  // DPR-aware raster: without the ratio argument the icon renders at
  // 1.0 and blurs on high-DPI displays. The badge label is
  // WA_TransparentForMouseEvents, so it never shows a tooltip of its own —
  // the widget-level "Klik untuk jeda/jalan" tooltip always applies.
  const QPixmap pix = style()->standardIcon(glyph).pixmap(
      QSize(32, 32), devicePixelRatioF());
  hover_badge_->setPixmap(pix);
  const QSize badge = hover_badge_->size();
  hover_badge_->move((width() - badge.width()) / 2,
                     (height() - badge.height()) / 2);
  hover_badge_->raise();
  hover_badge_->show();
}

void PreviewWidget::StopWorker() {
  if (worker_ == nullptr) return;
  stop_.store(true);
  if (worker_->isRunning()) {
    // The loop polls mpv_wait_event with a 100 ms timeout, so it exits almost
    // immediately once stop_ is set. Never terminate() (unsafe while holding
    // mutex_ / inside mpv) and never reset() a running QThread (Qt aborts): if
    // the graceful wait truly times out, leak the thread object instead.
    worker_->wait(5000);
    if (worker_->isRunning()) {
      std::fprintf(stderr,
                   "[preview] worker did not stop within 5s; leaking it rather "
                   "than terminating\n");
      std::fflush(stderr);
      (void)worker_.release();
      started_.store(false);
      start_polls_ = 0;
      if (status_timer_ != nullptr) status_timer_->stop();
      return;
    }
  }
  worker_.reset();
  started_.store(false);
  start_polls_ = 0;
  if (status_timer_ != nullptr) status_timer_->stop();
}

QString PreviewWidget::statusText() const {
  switch (load_state_.load()) {
    case PreviewLoadState::kLoading:
      return tr("Memuat pratinjau...");
    case PreviewLoadState::kLive:
      return QString();
    case PreviewLoadState::kError:
      // error_detail_ is already a full sentence (FriendlyPreviewError, or the
      // backend-start message), so no "Pratinjau tak tersedia: " prefix here.
      return error_detail_.isEmpty() ? tr("Pratinjau tak tersedia")
                                    : error_detail_;
    case PreviewLoadState::kIdle:
      break;
  }
  return tr("Belum ada pratinjau — pilih video...");
}

void PreviewWidget::UpdateStatusLabel() {
  if (status_label_ == nullptr) return;
  if (using_thumbnail_) {
    status_label_->setVisible(false);
    return;
  }
  const QString text = statusText();
  status_label_->setText(text);
  status_label_->setVisible(!text.isEmpty());
  if (status_label_->isVisible()) status_label_->raise();
}

}  // namespace k6wp