#pragma once

// Engine-status controller (MED-5 part 1): extracted from main_window.{hpp,cpp}
// as a pure refactor — no behavior change. MainWindow keeps the wiring (the
// 1.5s QTimer, the QFutureWatcher plumbing, the widget painting with its
// MainWindow tr() context); the status *decision* and the worker-only
// ready-wait live here so they are unit-testable without widgets.
//
// Status contract (docs/known-issues.md item 1): the first get_state after
// pipe-up may report running:false until the video actually starts; Studio
// maps both to Connected (only the paused flag splits the label).

#include <QString>

#include <atomic>
#include <string>

#include "ipc_client.hpp"

namespace k6wp {

// What the status label shows. Painting (colors, translated strings,
// tooltips, start-button visibility) stays in MainWindow::OnPollDone.
struct EngineStatusView {
  enum class Kind {
    kConnected,    // engine answering, live, attached
    kPaused,       // engine answering, paused flag set
    kDegraded,     // engine answering but !live or headless slots > 0
    kNotRunning,   // engine dead (offer Start) — gray
    kDisconnected  // transport failure / malformed ack — red
  };
  Kind kind = Kind::kDisconnected;
  unsigned long long pid = 0;
  QString video;  // full active-video path, may be empty (tolerated)
  bool paused = false;
  int headless = 0;
  bool live = true;
  std::string error;  // kError message for the tooltip
};

// Pure mapping from one GetState result to a view (was the decision half of
// MainWindow::OnPollDone). Never throws: malformed state JSON falls back to
// defaults, exactly like the old inline try/catch.
EngineStatusView DecideEngineStatus(const IpcResult& res);

// HIGH-3 slice B worker body (was MainWindow::WaitForEngineReady): polls
// GetState until kOk or timeout in cancellable short slices. BLOCKING —
// never call on the GUI thread. GetState serializes in Send's mutex; the
// 400ms steps sleep in 50ms slices so cancel aborts promptly.
bool WaitForEngineReady(IpcClient& ipc, std::atomic<bool>& cancel,
                        int timeout_ms);

}  // namespace k6wp
