// engine/src/mpv_renderer.cpp
// RAII mpv renderer — renders video via libmpv into an HWND with hardware
// decode fallback chain (d3d11va → dxva2 → software), infinite loop, no audio.
//
// Pattern validated by the mpv_hwdec spike (Todo 4, archived at
// attic/spikes/mpv_hwdec.cpp).

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include "mpv_renderer.hpp"

#include "log_file.hpp"

#include <mpv/client.h>
#include <clocale>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <mutex>
#include <string>
#include <system_error>

namespace k6wp {
namespace {

std::mutex g_mpv_log_mutex;

void Log(const char* fmt, ...) {
  std::lock_guard<std::mutex> lock(g_mpv_log_mutex);
  char msg[2048] = {};
  va_list args;
  va_start(args, fmt);
  std::vsnprintf(msg, sizeof(msg), fmt, args);
  va_end(args);
  AppendEngineLogLine(msg);
}

// State transitions (hwdec changes, re-loops) flush immediately so a
// taskkill never loses the diagnostic that explains it. mpv warn/error
// lines need no marking: the log sink auto-flushes those by level.
void LogImportant(const char* fmt, ...) {
  std::lock_guard<std::mutex> lock(g_mpv_log_mutex);
  char msg[2048] = {};
  va_list args;
  va_start(args, fmt);
  std::vsnprintf(msg, sizeof(msg), fmt, args);
  va_end(args);
  AppendEngineLogLine(msg, true);
}

}  // namespace

MpvRenderer::MpvRenderer() = default;

MpvRenderer::~MpvRenderer() {
  // Shutdown order (P2.4): quit flag -> mpv_wakeup -> join FIRST (no mutex_
  // held — the event thread briefly takes mutex_ for watchdog/fallback
  // commands), and ONLY after the join destroy the mpv instance under
  // mutex_. Joining while holding mutex_ would deadlock.
  StopEventThread();
  std::lock_guard<std::mutex> lock(mutex_);
  if (mpv_) {
    mpv_terminate_destroy(mpv_);
    mpv_ = nullptr;
  }
}

void MpvRenderer::StopEventThread() {
  if (!event_thread_.joinable()) return;
  if (event_thread_.get_id() == std::this_thread::get_id()) return;
  quit_.store(true, std::memory_order_relaxed);
  Wakeup();  // break the mpv_wait_event block so the loop sees quit_
  const auto t0 = std::chrono::steady_clock::now();
  event_thread_.join();  // blocking; never TerminateThread
  const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::steady_clock::now() - t0)
                      .count();
  if (ms > 200) {
    LogImportant(
        "critical: mpv event thread join took %lld ms (>200 ms budget); "
        "kept blocking-joining for correctness (never TerminateThread)",
        static_cast<long long>(ms));
  } else {
    Log("mpv: event thread joined in %lld ms", static_cast<long long>(ms));
  }
}

void MpvRenderer::Wakeup() {
  mpv_handle* ctx = nullptr;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    ctx = mpv_;
  }
  if (ctx) mpv_wakeup(ctx);
}

void MpvRenderer::SetMessageWindow(void* hwnd) {
  message_hwnd_.store(hwnd, std::memory_order_relaxed);
  // Re-arm immediately so a queued PROPERTY_CHANGE is picked up even when
  // the thread currently sits in its -1 paused block.
  Wakeup();
}

bool MpvRenderer::Create(void* hwnd) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (initialized_) return false;

  // libmpv requires LC_NUMERIC = "C" (client.h docs).
  std::setlocale(LC_NUMERIC, "C");

  mpv_ = mpv_create();
  if (!mpv_) return false;

  // Embedding: wid = HWND as decimal string (per spike pattern).
  if (hwnd) {
    std::string wid_str =
        std::to_string(reinterpret_cast<std::intptr_t>(hwnd));
    mpv_set_option_string(mpv_, "wid", wid_str.c_str());
  }

  // Video output: gpu-next with gpu fallback (P1.1 optional robustness;
  // A/B 2026-09-20: -62 MB vs gpu @4K H264 d3d11va, rendering verified
  // on-desktop via screenshot — gpu-next stays primary for RAM reasons).
  const char* vo_attempted = "gpu-next,gpu";
  mpv_set_option_string(mpv_, "vo", vo_attempted);

  // Hardware decoding: try d3d11va first, fallback chain in HandleEvent.
  mpv_set_option_string(mpv_, "hwdec", "d3d11va");

  // P3L.3: adapter pin at VO-init level (PATCH A: a runtime swap is not
  // honored). Empty = unpinned (log keeps the historical adapter=default).
  if (!pending_adapter_.empty()) {
    pin_applied_ = pending_adapter_;
    mpv_set_option_string(mpv_, "d3d11-adapter", pin_applied_.c_str());
  }

  // No audio — wallpaper engine is silent.
  mpv_set_option_string(mpv_, "audio", "no");

  // Infinite loop.
  mpv_set_option_string(mpv_, "loop-file", "inf");

  // Keep open when file ends (prevents VO from closing).
  mpv_set_option_string(mpv_, "keep-open", "yes");

  // Stay idle when no file loaded.
  mpv_set_option_string(mpv_, "idle", "yes");

  // Bound demuxer readahead: local looped files need no deep cache.
  // P1.1 spec values: 16MiB/4MiB (up from the 8M/2M RAM clamp). The RAM
  // delta is recorded in docs/bench_phase1.json rather than assumed safe.
  // Deliberately NOT set: vd-queue-enable (experimental path, A/B showed
  // +23 MB on 1080p) and hwdec-extra-frames (allocates EXTRA decoder
  // surfaces — the opposite of saving). vd-lavc-threads=4 bounds the
  // ffmpeg decoder thread pool. Failures logged, non-fatal.
  auto set_opt = [&](const char* key, const char* val) {
    const int rc = mpv_set_option_string(mpv_, key, val);
    if (rc < 0) Log("mpv: option '%s=%s' refused (%s)", key, val, mpv_error_string(rc));
  };
  set_opt("demuxer-max-bytes", "16MiB");
  set_opt("demuxer-max-back-bytes", "4MiB");
  set_opt("ad-queue-enable", "no");
  set_opt("ad-queue-max-bytes", "1M");
  set_opt("ad-queue-max-samples", "4");
  set_opt("ad-queue-max-secs", "1");
  set_opt("vd-lavc-threads", "4");
  // Render floor (Step 1A): kill expensive scaling/dither passes that a
  // wallpaper never needs. bilinear scalers + no dither/interpolation/
  // deinterlace remove full-screen shader passes (imperceptible on a
  // wallpaper; re-enable dither first if gradient banding is reported).
  // vo stays gpu-next and demuxer stays 8M/2M (A/B-measured, tighter than
  // the 32M/8M proposed upstream — do not raise without a new A/B).
  set_opt("scale", "bilinear");
  set_opt("cscale", "bilinear");
  set_opt("dscale", "bilinear");
  set_opt("dither", "no");
  set_opt("interpolation", "no");
  set_opt("deinterlace", "no");
  set_opt("framedrop", "vo");
  // Pacing: P1.1 switches vblank-locked display-vsync to display-desync
  // (lower render overhead for a wallpaper; long-loop behavior recorded
  // in docs/bench_phase1.json as a watch item, not assumed safe).
  set_opt("video-sync", "display-desync");
  // P1.1 lean profile (new knobs only; hwdec/loop/audio=no above untouched).
  set_opt("gpu-api", "d3d11");
  // Bitblt presentation, never flip model. The injected window is a
  // WS_EX_LAYERED child of Progman, and Progman has WS_EX_NOREDIRECTIONBITMAP
  // on 24H2, so DWM gives that subtree no redirection surface. Flip-model swap
  // chains share back buffers with DWM and need one: Present() would succeed
  // (valid frame in the front buffer) while DWM composites nothing, which
  // reads as "decodes fine, desktop never changes".
  set_opt("d3d11-flip", "no");
  set_opt("swapchain-depth", "2");
  set_opt("osc", "no");
  set_opt("sub-auto", "no");
  set_opt("audio-file-auto", "no");
  set_opt("hwdec-codecs", "all");
  {
    wchar_t lad[MAX_PATH] = {};
    std::string cache_dir = "shadercache";
    if (GetEnvironmentVariableW(L"LOCALAPPDATA", lad, MAX_PATH) > 0) {
      char lad_utf8[MAX_PATH * 2] = {};
      WideCharToMultiByte(CP_UTF8, 0, lad, -1, lad_utf8,
                          static_cast<int>(sizeof(lad_utf8)), nullptr, nullptr);
      cache_dir = std::string(lad_utf8) + "\\K6WP\\shadercache";
    }
    std::error_code ec;
    const bool existed =
        std::filesystem::exists(std::filesystem::u8path(cache_dir), ec);
    std::filesystem::create_directories(std::filesystem::u8path(cache_dir), ec);
    Log("mpv: gpu-shader-cache-dir='%s' status=%s", cache_dir.c_str(),
        ec ? ("failed:" + ec.message()).c_str()
           : (existed ? "exists" : "created"));
    set_opt("gpu-shader-cache-dir", cache_dir.c_str());
  }
  // Wallpaper never uses these: OSD, console, lua scripts (incl. ytdl
  // hooks/stats), and input bindings. load-scripts=no also kills the lua
  // engine + ytdl stat() calls at loadfile time.
  set_opt("osd-level", "0");
  set_opt("terminal", "no");
  set_opt("load-scripts", "no");
  set_opt("ytdl", "no");
  set_opt("input-default-bindings", "no");
  set_opt("input-vo-keyboard", "no");
  set_opt("input-cursor", "no");
  // Aggressive floor (Step 1A optional): most-basic render path + skip
  // deblocking/fast sw-decode shortcuts (sw-fallback only). Refusals are
  // logged + non-fatal; drop first if artefacts appear.
  set_opt("gpu-dumb-mode", "yes");
  set_opt("vd-lavc-skiploopfilter", "all");
  set_opt("vd-lavc-fast", "yes");

  if (mpv_initialize(mpv_) < 0) {
    mpv_terminate_destroy(mpv_);
    mpv_ = nullptr;
    return false;
  }

  // Request log messages at info level — needed for "hardware decoding" log.
  mpv_request_log_messages(mpv_, "info");

  // Observe hwdec-current property — the reliable hwdec signal (Todo 4).
  mpv_observe_property(mpv_, 0, "hwdec-current", MPV_FORMAT_STRING);
  // P2.4: also observe vo-configured so the event thread can notify the
  // main thread when the video output reconfigures (same PostMessage path).
  mpv_observe_property(mpv_, 0, "vo-configured", MPV_FORMAT_FLAG);

  initialized_ = true;

  {
    char* cur = mpv_get_property_string(mpv_, "hwdec-current");
    LogImportant("mpv: start vo=%s hwdec-current=%s adapter=%s "
                 "(req hwdec=d3d11va gpu-api=d3d11 video-sync=display-desync; "
                 "active hwdec follows via observer)",
                 vo_attempted, cur ? cur : "unknown",
                 pin_applied_.empty() ? "default" : pin_applied_.c_str());
    if (cur) mpv_free(cur);
  }

  // Inlined, not SetHWND(): mutex_ is already held and SetHWND re-locks the
  // same non-recursive std::mutex -> self-deadlock. initialized_ is true here,
  // so this is exactly SetHWND's post-initialize branch.
  if (pending_hwnd_) {
    const std::string pending_wid =
        std::to_string(reinterpret_cast<std::intptr_t>(pending_hwnd_));
    mpv_set_property_string(mpv_, "wid", pending_wid.c_str());
    pending_hwnd_ = nullptr;
  }

  // P2.4: each renderer owns one event thread (headless + every slot).
  // Started while holding mutex_ — the thread blocks in mpv_wait_event and
  // only briefly takes mutex_ for watchdog/fallback commands, so there is
  // no lock-ordering issue with the Create path finishing here.
  quit_.store(false, std::memory_order_relaxed);
  paused_.store(false, std::memory_order_relaxed);
  event_thread_ = std::thread(&MpvRenderer::EventThreadLoop, this);
  Log("mpv: event thread started");

  return true;
}

void MpvRenderer::SetAdapterPin(const std::string& substr) {
  std::lock_guard<std::mutex> lock(mutex_);
  // PATCH A: the d3d11-adapter option is only honored at VO-init time
  // (mpv_create -> mpv_initialize inside Create()). A runtime swap after
  // Create is NOT honored by libmpv, so a post-Create call is a deliberate
  // no-op -- log it so a caller expecting a live re-pin sees why nothing
  // changed (recreate the renderer to re-pin).
  if (initialized_) {
    Log("mpv: SetAdapterPin('%s') ignored -- adapter pin is init-level only "
        "(PATCH A); recreate the renderer to re-pin",
        substr.c_str());
    return;
  }
  pending_adapter_ = substr;
}

bool MpvRenderer::pin_active() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return initialized_ && !pin_applied_.empty();
}

void MpvRenderer::SetHWND(void* hwnd) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!mpv_) {
    // Store for later — Create() will apply it.
    pending_hwnd_ = hwnd;
    return;
  }

  std::string wid_str =
      std::to_string(reinterpret_cast<std::intptr_t>(hwnd));

  if (!initialized_) {
    // Before mpv_initialize: set as option.
    mpv_set_option_string(mpv_, "wid", wid_str.c_str());
  } else {
    // After mpv_initialize: set as property (triggers VO restart).
    mpv_set_property_string(mpv_, "wid", wid_str.c_str());
  }
}

bool MpvRenderer::LoadLoop(const std::string& path, bool force) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!mpv_ || !initialized_) return false;
  if (!force && !last_path_.empty() && path == last_path_) {
    // Same path: skip the reload unless the file bytes changed (a
    // re-compress overwrite keeps the path). Stat failure reloads — a
    // missed reload is worse than a redundant one.
    std::error_code ec;
    const std::uintmax_t size =
        std::filesystem::file_size(std::filesystem::u8path(path), ec);
    if (!ec) {
      const auto mtime = std::filesystem::last_write_time(
          std::filesystem::u8path(path), ec);
      if (!ec && size == last_size_ && mtime == last_mtime_) {
        return true;
      }
    }
  }
  const char* cmd[] = {"loadfile", path.c_str(), nullptr};
  if (mpv_command(mpv_, cmd) < 0) {
    return false;
  }
  last_path_ = path;
  std::error_code ec;
  last_size_ = std::filesystem::file_size(std::filesystem::u8path(path), ec);
  if (ec) last_size_ = 0;
  last_mtime_ = std::filesystem::last_write_time(std::filesystem::u8path(path), ec);
  {
    // P2.4: eof state lives under event_mutex_ (shared with the event
    // thread's watchdog). This mutex_->event_mutex_ nesting is the single
    // allowed order — no path takes mutex_ while holding event_mutex_.
    std::lock_guard<std::mutex> ev_lock(event_mutex_);
    eof_pending_ = false;
  }
  return true;
}

void MpvRenderer::SetFitMode(const std::string& fit_mode,
                             double window_aspect) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!mpv_ || !initialized_) return;
  // mpv 0.41 deprecated video-aspect-override; its documented replacement for
  // "no override" is override=no + video-aspect-mode=ignore. Passing "0"
  // instead logs one deprecation warning per call, which grew engine.log to
  // tens of thousands of identical lines. Every branch that means "no
  // override" must call this, not just the ones that used to pass "0".
  const auto clear_aspect_override = [this]() {
    mpv_set_property_string(mpv_, "video-aspect-override", "no");
    mpv_set_property_string(mpv_, "video-aspect-mode", "ignore");
  };
  if (fit_mode == "cover" || fit_mode == "fill") {
    // Fullscreen fill: scale to cover the window, no black bars. "fill" is
    // the schema default; "cover" is kept as an accepted alias.
    mpv_set_property_string(mpv_, "video-unscaled", "no");
    mpv_set_property_string(mpv_, "panscan", "1.0");
    mpv_set_property_string(mpv_, "video-zoom", "0");
    clear_aspect_override();
  } else if (fit_mode == "stretch" && window_aspect > 0.0) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.6f", window_aspect);
    mpv_set_property_string(mpv_, "video-unscaled", "no");
    mpv_set_property_string(mpv_, "panscan", "0");
    mpv_set_property_string(mpv_, "video-zoom", "0");
    mpv_set_property_string(mpv_, "video-aspect-override", buf);
    mpv_set_property_string(mpv_, "video-aspect-mode", "ignore");
  } else if (fit_mode == "center") {
    mpv_set_property_string(mpv_, "video-unscaled", "yes");
    mpv_set_property_string(mpv_, "panscan", "0");
    mpv_set_property_string(mpv_, "video-zoom", "0");
    clear_aspect_override();
  } else {
    // fit (contain): whole video visible, letterboxed (panscan 0 / default).
    mpv_set_property_string(mpv_, "video-unscaled", "no");
    mpv_set_property_string(mpv_, "panscan", "0");
    mpv_set_property_string(mpv_, "video-zoom", "0");
    clear_aspect_override();
  }
}

void MpvRenderer::Pause() {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!mpv_ || !initialized_) return;
    int flag = 1;
    mpv_set_property(mpv_, "pause", MPV_FORMAT_FLAG, &flag);
  }
  // P2.4: flip the atomic first so the event thread switches to its -1
  // blocking wait (zero periodic wakeups while paused), then wake it out of
  // the current 500 ms wait so the new cadence applies immediately.
  paused_.store(true, std::memory_order_relaxed);
  Wakeup();
}

void MpvRenderer::Resume() {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!mpv_ || !initialized_) return;
    int flag = 0;
    mpv_set_property(mpv_, "pause", MPV_FORMAT_FLAG, &flag);
  }
  // P2.4: clear the atomic, then wake the thread out of its -1 block so the
  // 500 ms watchdog cadence re-arms immediately — otherwise hwdec-fallback /
  // EOF-watchdog would stay dormant after resume until an unrelated event.
  paused_.store(false, std::memory_order_relaxed);
  Wakeup();
}

void MpvRenderer::SetFpsCap(int fps) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!mpv_ || !initialized_) return;
  if (fps <= 0) {
    mpv_set_property_string(mpv_, "vf", "");
  } else {
    const std::string vf = "fps=" + std::to_string(fps);
    mpv_set_property_string(mpv_, "vf", vf.c_str());
  }
}

void MpvRenderer::EventThreadLoop() {
  // P2.4: per-renderer event thread. Dynamic wait timeout — 500 ms while
  // unpaused (the EOF watchdog is checked on every wake), -1 blocking while
  // paused (zero periodic wakeups; a paused stream cannot hit EOF).
  // mutex_ is NEVER held across mpv_wait_event here: the handle is
  // snapshotted under a brief lock, and watchdog/fallback commands take
  // mutex_ only for their own short critical section afterwards.
  // mpv_terminate_destroy runs only after this loop is joined (see dtor),
  // so the snapshotted handle stays valid for the loop's lifetime.
  while (!quit_.load(std::memory_order_relaxed)) {
    mpv_handle* ctx = nullptr;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (!mpv_ || !initialized_) break;
      ctx = mpv_;
    }
    const double timeout =
        paused_.load(std::memory_order_relaxed) ? -1.0 : 0.5;
    mpv_event* ev = mpv_wait_event(ctx, timeout);
    if (quit_.load(std::memory_order_relaxed)) break;
    if (ev->event_id != MPV_EVENT_NONE) {
      HandleEvent(ev);
      for (int i = 0; i < 15; ++i) {
        mpv_event* extra = mpv_wait_event(ctx, 0);  // Drain, non-blocking.
        if (extra->event_id == MPV_EVENT_NONE) break;
        HandleEvent(extra);
      }
    }
    // Watchdog runs ONLY when unpaused; while paused there are no periodic
    // wakes and no EOF check. The first 500 ms wake after resume re-arms it.
    if (!paused_.load(std::memory_order_relaxed)) CheckEofWatchdog();
  }
}

void MpvRenderer::HandleEvent(mpv_event* ev) {
  bool need_fallback = false;
  {
    std::lock_guard<std::mutex> lock(event_mutex_);
    switch (ev->event_id) {
      case MPV_EVENT_LOG_MESSAGE: {
        auto* lm =
            reinterpret_cast<mpv_event_log_message*>(ev->data);
        if (lm->text && std::strstr(lm->text, "hardware decoding")) {
          hwdec_active_.store(true, std::memory_order_relaxed);
          hwdec_fallback_attempted_.store(false, std::memory_order_relaxed);
          if (last_hwdec_logged_.empty()) {
            last_hwdec_logged_ = "d3d11va(log)";
            LogImportant("mpv: hwdec active (log: hardware decoding)");
          }
        }
        // Surface real failures: without this, decode/VO errors are
        // silent and the wallpaper just stays black. info/verbose stay
        // out of the log to avoid spam.
        if (lm->level && lm->text &&
            (std::strcmp(lm->level, "fatal") == 0 ||
             std::strcmp(lm->level, "error") == 0 ||
             std::strcmp(lm->level, "warn") == 0)) {
          Log("mpv [%s/%s]: %s", lm->prefix ? lm->prefix : "?",
              lm->level, lm->text);
        }
        break;
      }
      case MPV_EVENT_START_FILE:
      case MPV_EVENT_FILE_LOADED:
        eof_pending_ = false;  // native (re)start: watchdog stands down
        ever_started_.store(true, std::memory_order_relaxed);
        break;
      case MPV_EVENT_END_FILE: {
        auto* ef = reinterpret_cast<mpv_event_end_file*>(ev->data);
        const char* reason = "unknown";
        bool is_eof = false;
        if (ef != nullptr) {
          switch (ef->reason) {
            case MPV_END_FILE_REASON_EOF:
              reason = "eof";
              is_eof = true;
              break;
            case MPV_END_FILE_REASON_ERROR:
              reason = "error";
              break;
            case MPV_END_FILE_REASON_STOP:
              reason = "stop";
              break;
            case MPV_END_FILE_REASON_QUIT:
              reason = "quit";
              break;
            case MPV_END_FILE_REASON_REDIRECT:
              reason = "redirect";
              break;
            default:
              break;
          }
        }
        Log("mpv: end-file (reason=%s%s%s)", reason,
            (ef != nullptr && ef->reason == MPV_END_FILE_REASON_ERROR)
                ? ", detail="
                : "",
            (ef != nullptr && ef->reason == MPV_END_FILE_REASON_ERROR)
                ? mpv_error_string(ef->error)
                : "");
        if (is_eof) {
          // Armed: cleared by the loop's own START_FILE, or consumed by the
          // watchdog when no restart arrives (see CheckEofWatchdog).
          eof_pending_ = true;
          eof_at_ = std::chrono::steady_clock::now();
        }
        break;
      }
      case MPV_EVENT_PROPERTY_CHANGE: {
        auto* p = reinterpret_cast<mpv_event_property*>(ev->data);
        const std::string name = p->name ? p->name : "";
        const bool is_hwdec = (name == "hwdec-current");
        const bool is_vo = (name == "vo-configured");
        if (!is_hwdec && !is_vo) break;
        // The "vo=gpu-next,gpu" string in the Create log is only the
        // ATTEMPTED list, so engine.log otherwise cannot say which VO libmpv
        // actually brought up. current-vo is read through libmpv directly
        // (thread-safe); taking mutex_ here would nest it under event_mutex_
        // and break the documented lock order.
        if (is_vo && p->format == MPV_FORMAT_FLAG && p->data) {
          const int configured = *reinterpret_cast<const int*>(p->data);
          if (configured != last_vo_configured_) {
            last_vo_configured_ = configured;
            char* live_vo = mpv_get_property_string(mpv_, "current-vo");
            LogImportant("mpv: vo-configured=%d current-vo=%s", configured,
                         live_vo ? live_vo : "(none)");
            if (live_vo) mpv_free(live_vo);
          }
        }
        void* hwnd = message_hwnd_.load(std::memory_order_relaxed);
        if (hwnd != nullptr) {
          // Todo 7 path: the main thread owns fallback decisions.
          ::PostMessageW(reinterpret_cast<HWND>(hwnd), kMsgHwdecChange,
                         is_hwdec ? 1 : 2, 0);
          break;
        }
        // Transition path (no message window wired yet): apply inline so
        // the d3d11va->dxva2 fallback keeps working exactly as before.
        if (is_hwdec && p->format == MPV_FORMAT_STRING && p->data) {
          const char* val = *reinterpret_cast<const char**>(p->data);
          need_fallback = ApplyHwdecValue(val ? val : "no");
        }
        break;
      }
      default:
        break;
    }
  }
  // Runs AFTER event_mutex_ is released: TryFallbackHwdec takes mutex_,
  // and the two locks must never nest (see header lock-order note).
  if (need_fallback) TryFallbackHwdec();
}

bool MpvRenderer::ApplyHwdecValue(const std::string& cur) {
  // Moved (not rewritten) from the old PollEvents PROPERTY_CHANGE branch:
  // same hwdec chain semantics, same log lines. Only the call site changed
  // (event thread inline path + main-thread OnHwdecPropertyChange).
  if (cur != "no") {
    hwdec_active_.store(true, std::memory_order_relaxed);
    hwdec_fallback_attempted_.store(false, std::memory_order_relaxed);
  } else {
    hwdec_active_.store(false, std::memory_order_relaxed);
    if (cur != last_hwdec_logged_) {
      last_hwdec_logged_ = cur;
      LogImportant("mpv: hwdec-current=%s (hwdec INACTIVE-software)",
                   cur.c_str());
    }
    return true;  // caller runs TryFallbackHwdec() after unlocking
  }
  if (cur != last_hwdec_logged_) {
    last_hwdec_logged_ = cur;
    LogImportant("mpv: hwdec-current=%s (hwdec active)", cur.c_str());
  }
  return false;
}

void MpvRenderer::OnHwdecPropertyChange() {
  // Main-thread hook for the Todo 7 PostMessage path: re-query the property
  // under mutex_ (authoritative value at handling time), then run the moved
  // ApplyHwdecValue decision under event_mutex_. Locks taken sequentially,
  // never nested.
  std::string cur = "no";
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!mpv_ || !initialized_) return;
    char* val = mpv_get_property_string(mpv_, "hwdec-current");
    if (val) {
      cur = val;
      mpv_free(val);
    }
  }
  bool need_fallback = false;
  {
    std::lock_guard<std::mutex> lock(event_mutex_);
    need_fallback = ApplyHwdecValue(cur);
  }
  if (need_fallback) TryFallbackHwdec();
}

void MpvRenderer::CheckEofWatchdog() {
  // EOF watchdog: native loop-file=inf restarts within milliseconds (which
  // emits START_FILE and disarms eof_pending_). Only a truly stuck EOF —
  // no restart after the grace period — gets a manual re-loop here, so a
  // finished video can never silently drop back to the OS wallpaper.
  // 2000 ms: far above a healthy restart, far below a noticeable black gap.
  // Held across the check+command like the old Tick() did: the single
  // allowed nesting order mutex_ -> event_mutex_ (same as LoadLoop), brief
  // and never across mpv_wait_event.
  std::lock_guard<std::mutex> lock(mutex_);
  if (!mpv_ || !initialized_ || last_path_.empty()) return;
  {
    std::lock_guard<std::mutex> ev_lock(event_mutex_);
    if (!eof_pending_) return;
    const auto idle_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - eof_at_)
                             .count();
    if (idle_ms < 2000) return;
    eof_pending_ = false;  // re-armed by the next END_FILE if this fails too
  }
  const char* cmd[] = {"loadfile", last_path_.c_str(), nullptr};
  if (mpv_command(mpv_, cmd) >= 0) {
    LogImportant("mpv: watchdog re-loop '%s' (native loop did not restart)",
                 last_path_.c_str());
  } else {
    LogImportant("warning: mpv watchdog re-loop refused for '%s'",
                 last_path_.c_str());
  }
}

void MpvRenderer::TryFallbackHwdec() {
  hwdec_fallback_attempted_.store(true, std::memory_order_acq_rel);
  std::lock_guard<std::mutex> lock(mutex_);
  if (!mpv_) return;

  // M3 (1.3.0-beta.2): advance the chain ONE link per call. The old
  // one-shot exchange stopped at dxva2, so a machine with neither
  // d3d11va nor dxva2 stayed black. Stage 2 is terminal: nothing left to
  // request, the log already names software.
  const char* next = nullptr;
  switch (hwdec_stage_) {
    case 0:
      next = "dxva2";
      break;
    case 1:
      next = "no";  // software decode — keeps the wallpaper visible
      break;
    default:
      return;
  }
  ++hwdec_stage_;
  LogImportant("mpv: hwdec inactive, falling back to %s (chain link %d/3)",
               next, hwdec_stage_ + 1);
  mpv_set_property_string(mpv_, "hwdec", next);
}

}  // namespace k6wp
