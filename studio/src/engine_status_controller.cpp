// Engine-status controller implementation — moved verbatim from
// main_window.cpp (MED-5 part 1, pure refactor).

#include "engine_status_controller.hpp"

#include <QThread>

namespace k6wp {

EngineStatusView DecideEngineStatus(const IpcResult& res) {
  EngineStatusView view;
  if (res.status == IpcStatus::kOk) {
    view.kind = EngineStatusView::Kind::kConnected;
    const EngineState state = ParseEngineState(res.raw);
    view.pid = state.pid;
    view.video = QString::fromUtf8(state.video.c_str());
    view.paused = state.paused;
    view.headless = state.headless_slots;
    view.live = state.live;
    if (view.paused) {
      view.kind = EngineStatusView::Kind::kPaused;
    } else if (!view.live || view.headless > 0) {
      view.kind = EngineStatusView::Kind::kDegraded;
    }
    return view;
  }
  if (res.status == IpcStatus::kNotRunning) {
    view.kind = EngineStatusView::Kind::kNotRunning;
    return view;
  }
  view.kind = EngineStatusView::Kind::kDisconnected;
  view.error = res.error;
  return view;
}

bool WaitForEngineReady(IpcClient& ipc, std::atomic<bool>& cancel,
                        int timeout_ms) {
  const int step_ms = 400;
  const int slice_ms = 50;
  int waited = 0;
  while (waited < timeout_ms) {
    if (cancel.load(std::memory_order_relaxed)) {
      return false;
    }
    const IpcResult res = ipc.GetState();
    if (res.status == IpcStatus::kOk) {
      return true;
    }
    int slept = 0;
    while (slept < step_ms && waited + slept < timeout_ms) {
      if (cancel.load(std::memory_order_relaxed)) {
        return false;
      }
      QThread::msleep(static_cast<unsigned long>(slice_ms));
      slept += slice_ms;
    }
    waited += slept;
  }
  if (cancel.load(std::memory_order_relaxed)) {
    return false;
  }
  return ipc.GetState().status == IpcStatus::kOk;
}

}  // namespace k6wp
