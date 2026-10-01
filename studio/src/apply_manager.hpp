#pragma once

#include <QStringList>

#include <QObject>

#include <atomic>
#include <optional>

#include "config_schema.hpp"
#include "ipc_client.hpp"

class QString;

namespace k6wp {

// Live-switch apply path (Todo 30, Wave 4). The caller writes the wallpaper
// config to ConfigPath() via WriteConfig/SaveConfig, then Apply() hot-swaps
// the running engine via IpcClient::SetVideo (no process restart, new frame
// <=500ms). When the engine is not listening (IpcStatus::kNotRunning) it
// recovers through RestartEngine (the Todo 25 path) plus one SetVideo retry;
// a kError ack fails the Apply with the engine's message surfaced.
//
// T15 shared-client ownership (one IpcClient per Studio process, for Wave-E):
//
//   StudioBridge owns:  IpcClient ipc_   (member, lives with the bridge)
//        | borrows (non-owning raw ptr, set once in the StudioBridge ctor)
//        v
//   ApplyManager::ipc_  ── never new/delete, never null after wiring ──
//        |
//        +-- Apply()/WriteConfig()/RestartEngine() use it for SetVideo/
//            SetMonitor/GetState. Status poll, pause/resume use the same
//            shared client through StudioBridge.
//   LibraryGridModel holds NO client: its row actions emit applyRequested /
//   recompressRequested; StudioBridge::applyWallpaper does the single IPC
//   live-switch. Exactly-once at the connection level (T13).
//
// Lifetime: StudioBridge (QObject parent) outlives every apply call; the
// shared-client transactions serialize inside IpcClient::Send (mutex_).
//
// HIGH-3 slice B: every blocking op (Apply, RestartEngine's 8s process wait
// and pipe-ready poll) runs on a CALLER-OWNED QtConcurrent worker, never on
// the GUI thread. StudioBridge runs Apply on its watcher and paints the
// returned ApplyResult; the GUI disables its buttons and shows an
// indeterminate progress bar while busy. Cancellation is cooperative: an
// atomic flag polled in short (<=100ms) slices — never TerminateThread.

// Structured outcome of Apply() (moved from MainWindow — it paints the
// result, ApplyManager computes it). kLiveOk / kAfterRestartOk are the two
// success shapes; explicit operator bool() covers them so callers can use
// the result directly in a boolean context.
struct ApplyResult {
  enum class Outcome {
    kLiveOk,
    kRestartFailed,
    kNotReady,
    kAfterRestartOk,
    kRetryFailed,
    kRejected,
  };
  Outcome outcome = Outcome::kRejected;
  bool retried = false;
  bool restart_attempted = false;
  std::string first_error;
  QString restart_error;
  std::string retry_error;
  QString live_path;
  bool monitor_ok = false;
  QString monitor_error;

  explicit operator bool() const {
    return outcome == Outcome::kLiveOk || outcome == Outcome::kAfterRestartOk;
  }
};

class ApplyManager final : public QObject {
  Q_OBJECT

  public:
   explicit ApplyManager(QObject* parent = nullptr);
   ~ApplyManager() override;

  // Points the manager at the process-wide shared client (MainWindow::ipc_).
  // Must be called before any IPC-touching call; the pointer is borrowed,
  // never owned.
  void SetIpcClient(IpcClient* shared) { ipc_ = shared; }

  // Overrides the engine config path (the `--config` value the engine was
  // started with). Empty (the default) means DefaultConfigPath(), which is
  // exactly how the engine resolves an empty --config (engine_app.cpp).
  // Production never sets it today; studio_logic_test/studio_async_test use
  // it to redirect WriteConfig away from the real %LOCALAPPDATA% file.
  void SetConfigPath(const std::filesystem::path& path) {
    config_path_override_ = path;
  }
  std::filesystem::path ConfigPath() const {
    return config_path_override_.empty() ? DefaultConfigPath()
                                         : config_path_override_;
  }

  // Writes cfg to the engine config path (see ConfigPath()). Returns true
  // on success; on failure returns false and fills *error_out (when
  // non-null). Logs each step.
  bool WriteConfig(const WallpaperConfig& cfg, QString* error_out = nullptr);

  // Graceful restart: get_state -> engine PID, IPC quit, wait up to 8s
  // for that PID to exit (graceful Shutdown runs RestoreOsWallpaper),
  // TerminateProcess on that PID only when still alive, then start fresh
  // with `--config <ConfigPath()>`. Never kills by image name. If the
  // engine was not running, it is started and that fact is logged. Returns
  // true ONLY when the fresh engine's pipe is ready; false + *error_out
  // otherwise. Logs each step.
  //
  // BLOCKING — call only from a worker thread. The 8s process wait
  // and the pipe-ready poll run in short slices that check the atomic
  // cancel flag (RequestCancel), so a stale op aborts promptly.
  bool RestartEngine(QString* error_out = nullptr);

  // The single Apply path (unified with what MainWindow's worker used to
  // re-implement line-by-line): IPC live-switch over the shared client,
  // exactly ONE retry on a pipe error, monitor sync on success, and on
  // kNotRunning a full RestartEngine + ready-wait + one SetVideo retry with
  // live-path readback. The CALLER must have written `cfg` to ConfigPath()
  // first (WriteConfig above) — the fresh-engine recovery path boots from
  // that file. Blocking: run on a worker; results are painted by the caller
  // from the returned ApplyResult. Lockscreen sync stays caller-owned (the
  // GUI fires it per outcome). Cooperative cancel via RequestCancel.
  ApplyResult Apply(const WallpaperConfig& cfg, QString* error_out = nullptr);

  // Pushes the target monitor over IPC only when it changed since the last
  // successful push (T16 "set_monitor bila ganti"). First call always pushes
  // (last_monitor_ starts disengaged); a rebooted engine resets it via
  // RestartEngine. kNotRunning → false (caller owns the start-engine path);
  // kError → false + *error_out. Uses the shared client, never a new one.
  bool SyncMonitor(int monitor_id, QString* error_out = nullptr);

  // Resolves engine.exe: `<studio_exe_dir>/engine.exe` first (both exes are
  // flattened into build/msvc-dev/ in dev), falling back to
  // `<studio_exe_dir>/../../build/msvc-dev/engine.exe` ONLY when the dev
  // fallback is enabled (see IsDevEngineFallbackEnabled). Otherwise a
  // missing primary yields a clear "engine.exe not found at <path>" error
  // from RestartEngine.
  static QString ResolveEnginePath();

  // LOW-16 dev-fallback gate (dev-contracts.md §3 + §7): true only in a
  // _DEBUG build AND when the process environment carries K6WP_DEV=1.
  // Release builds never enable it (the fallback literal is compiled out
  // under `#ifdef _DEBUG`, so no release binary can reference the dev
  // path); _DEBUG builds without the env var resolve to the primary path,
  // so the fallback is never used silently. Test seam: studio_logic_test
  // §7 pins both sides of the gate.
  static bool IsDevEngineFallbackEnabled();

  const QStringList& log() const { return log_; }
  void ClearLog() { log_.clear(); }

  // Cooperatively aborts any in-flight blocking operation (RestartEngine /
  // Apply) running on a caller-owned worker; short-poll slices observe it
  // within ~100ms. Safe to call from the GUI thread.
  void RequestCancel();

 signals:
  void LogMessage(const QString& line);

 private:
  void AppendLog(const QString& line);

  // Borrowed shared client (see class doc). Raw pointer by design: MainWindow
  // owns the object and outlives all apply calls; Apply() refuses loudly
  // when it was never wired so a missing SetIpcClient surfaces in tests.
  IpcClient* ipc_ = nullptr;
  // Last monitor_id successfully pushed via SyncMonitor. Disengaged =
  // unknown (push on next apply so a fresh process always converges with
  // the engine). std::optional because -1 is a valid target (all screens),
  // never an "unknown" sentinel.
  std::optional<int> last_monitor_;
  QStringList log_;
  // Empty = DefaultConfigPath() (mirrors the engine's empty---config
  // resolution). Set via SetConfigPath when the engine uses --config (or in
  // tests, to keep writes off the real per-user file).
  std::filesystem::path config_path_override_;
  // Worker-cancel flag polled by the blocking waits in short slices; set
  // on the GUI thread, read on the worker.
  std::atomic<bool> cancel_{false};
};

}  // namespace k6wp
