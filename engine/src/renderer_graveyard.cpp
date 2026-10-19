#include "renderer_graveyard.hpp"

#include <chrono>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <thread>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace k6wp {

namespace {

struct GraveyardItem {
  std::unique_ptr<MpvRenderer> renderer;
  void* window = nullptr;  // HWND to destroy after the renderer is gone
};

// Process-wide state. The graveyard is a singleton on purpose: the
// renderers and windows come from different call sites (MultiMonitor's slot
// teardown, the engine's headless renderer) and must share ONE worker so
// the teardown order (renderer first, window second) stays observable.
struct GraveyardState {
  std::mutex mutex;
  std::condition_variable wake;
  std::condition_variable idle;
  std::deque<GraveyardItem> queue;
  std::deque<void*> ready_windows;  // renderer gone -> safe to DestroyWindow
  std::thread worker;
  bool stop_requested = false;
  bool worker_running = false;
  bool worker_busy = false;
  unsigned long long submitted = 0;
  unsigned long long destroyed = 0;
};

GraveyardState& State() {
  // LEAKY SINGLETON ON PURPOSE. The worker thread outlives the process's
  // static destruction order (it may still be inside a renderer destructor
  // when main() returns), so the state must NOT be destroyed at exit: a
  // destroyed mutex/condvar under a waiting worker is undefined behaviour
  // (observed as a hang at process exit). The process reclaims the memory
  // when it terminates.
  static GraveyardState* state = new GraveyardState();
  return *state;
}

void WorkerMain() {
  GraveyardState& s = State();
  for (;;) {
    GraveyardItem item;
    {
      std::unique_lock<std::mutex> lock(s.mutex);
      s.worker_busy = false;
      s.idle.notify_all();
      s.wake.wait(lock, [&s] { return s.stop_requested || !s.queue.empty(); });
      if (s.queue.empty()) {
        if (s.stop_requested) return;
        continue;
      }
      item = std::move(s.queue.front());
      s.queue.pop_front();
      s.worker_busy = true;
    }
    // The blocking call: ~MpvRenderer -> mpv_terminate_destroy() waits for
    // the VO thread, which can be stuck in Present() on a removed display.
    // THAT is why it runs here and not on the message-loop thread.
    if (item.renderer) {
      item.renderer.reset();
      std::lock_guard<std::mutex> lock(s.mutex);
      ++s.destroyed;
    }
    if (item.window != nullptr) {
      // The renderer is gone: the window may now be destroyed - but only on
      // the thread that created it (the message-loop thread), so it is
      // parked in the ready list and picked up by Drain().
      std::lock_guard<std::mutex> lock(s.mutex);
      s.ready_windows.push_back(item.window);
    }
  }
}

}  // namespace

bool RendererGraveyard::Submit(std::unique_ptr<MpvRenderer> renderer,
                               void* window_to_destroy_after, LogFn log) {
  GraveyardState& s = State();
  if (!renderer && window_to_destroy_after == nullptr) {
    return true;  // nothing to do
  }
  std::lock_guard<std::mutex> lock(s.mutex);
  if (!s.worker_running) {
    // Start the worker. If the thread cannot be created the caller keeps
    // ownership - we must not half-take the renderer.
    try {
      s.worker = std::thread(WorkerMain);
    } catch (...) {
      if (log) {
        log("graveyard: worker thread could not be started; the renderer stays "
            "with the caller");
      }
      return false;
    }
    s.worker_running = true;
  }
  s.queue.push_back(GraveyardItem{std::move(renderer), window_to_destroy_after});
  ++s.submitted;
  s.wake.notify_one();
  return true;
}

int RendererGraveyard::Drain() {
  GraveyardState& s = State();
  std::deque<void*> windows;
  {
    std::lock_guard<std::mutex> lock(s.mutex);
    windows.swap(s.ready_windows);
  }
  int destroyed = 0;
  for (void* raw : windows) {
    HWND hwnd = static_cast<HWND>(raw);
    if (hwnd == nullptr) continue;
    if (IsWindow(hwnd)) {
      // A window that cannot be destroyed (already gone, or owned by
      // another thread) is not fatal - the slot is already gone.
      DestroyWindow(hwnd);
    }
    ++destroyed;
  }
  return destroyed;
}

bool RendererGraveyard::WaitIdle(int timeout_ms) {
  GraveyardState& s = State();
  std::unique_lock<std::mutex> lock(s.mutex);
  return s.idle.wait_for(lock, std::chrono::milliseconds(timeout_ms), [&s] {
    return s.queue.empty() && !s.worker_busy;
  });
}

bool RendererGraveyard::Shutdown(int timeout_ms, LogFn log) {
  GraveyardState& s = State();
  std::thread worker;
  {
    std::lock_guard<std::mutex> lock(s.mutex);
    if (!s.worker_running) return true;
    s.stop_requested = true;
    s.wake.notify_all();
    worker = std::move(s.worker);
    s.worker_running = false;
  }
  if (!worker.joinable()) return true;

  // std::thread::join() has no timeout, so the join is done by a helper
  // thread and waited for with a deadline. The helper state is heap
  // allocated: when the worker is stuck the helper outlives this call and
  // must not touch a dangling stack object.
  struct JoinState {
    std::mutex mutex;
    std::condition_variable cv;
    bool done = false;
  };
  auto join_state = std::make_shared<JoinState>();
  std::thread joiner([worker = std::move(worker), join_state]() mutable {
    worker.join();
    {
      std::lock_guard<std::mutex> lock(join_state->mutex);
      join_state->done = true;
    }
    join_state->cv.notify_all();
  });
  joiner.detach();

  bool clean = true;
  {
    std::unique_lock<std::mutex> lock(join_state->mutex);
    const bool finished = join_state->cv.wait_for(
        lock, std::chrono::milliseconds(timeout_ms),
        [&join_state] { return join_state->done; });
    clean = finished;
  }
  if (!clean && log) {
    log("graveyard: worker still busy after %d ms; leaving it detached (a stuck "
        "renderer teardown must not block the exit)",
        timeout_ms);
  }
  return clean;
}

unsigned long long RendererGraveyard::submitted_count() {
  GraveyardState& s = State();
  std::lock_guard<std::mutex> lock(s.mutex);
  return s.submitted;
}

unsigned long long RendererGraveyard::destroyed_count() {
  GraveyardState& s = State();
  std::lock_guard<std::mutex> lock(s.mutex);
  return s.destroyed;
}

int RendererGraveyard::pending_window_count() {
  GraveyardState& s = State();
  std::lock_guard<std::mutex> lock(s.mutex);
  return static_cast<int>(s.ready_windows.size());
}

}  // namespace k6wp
