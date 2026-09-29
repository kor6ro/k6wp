#include "apply_manager.hpp"

#include <chrono>
#include <filesystem>
#include <thread>

// windows.h for the PID-targeted RestartEngine (OpenProcess /
// WaitForSingleObject / TerminateProcess + WaitNamedPipeW polling).
// Kept in this .cpp only.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFutureWatcher>
#include <QProcess>
#include <QString>
#include <QtConcurrent>

#include "ipc_client.hpp"
#include "lockscreen.hpp"
#include "engine_status_controller.hpp"

namespace k6wp {
namespace {

// RAII guard for a process HANDLE (CloseHandle exactly once).
struct ProcHandle {
  HANDLE h = nullptr;
  explicit ProcHandle(HANDLE handle) : h(handle) {}
  ~ProcHandle() {
    if (h != nullptr && h != INVALID_HANDLE_VALUE) CloseHandle(h);
  }
  ProcHandle(const ProcHandle&) = delete;
  ProcHandle& operator=(const ProcHandle&) = delete;
  HANDLE get() const { return h; }
  bool valid() const { return h != nullptr && h != INVALID_HANDLE_VALUE; }
};

// Poll WaitNamedPipeW in slice_ms slices until total_ms elapses.
// Per-session pipe (MED-12): probes \\.\pipe\k6wp-engine-<session_id>, the
// same name the engine serves and IpcClient dials.
// HIGH-3 slice B: cancel (when non-null) aborts the poll within one slice —
// the worker just returns false, never TerminateThread.
bool WaitForEnginePipe(DWORD total_ms, DWORD slice_ms,
                       const std::atomic<bool>* cancel = nullptr) {
  const std::wstring pipe_name = CurrentSessionPipeName();
  const ULONGLONG start = GetTickCount64();
  for (;;) {
    if (cancel != nullptr &&
        cancel->load(std::memory_order_relaxed)) {
      return false;
    }
    if (WaitNamedPipeW(pipe_name.c_str(), slice_ms) != 0) {
      return true;
    }
    if (GetTickCount64() - start >= total_ms) {
      return false;
    }
  }
}

// HIGH-3 slice B: bounded process-exit wait in short slices. Returns true
// when the process exited, false on timeout OR cancel. Runs on a worker
// thread — the GUI thread never blocks in WaitForSingleObject.
bool WaitForProcessExit(HANDLE proc, DWORD total_ms, DWORD slice_ms,
                        const std::atomic<bool>* cancel) {
  const ULONGLONG start = GetTickCount64();
  for (;;) {
    if (cancel != nullptr &&
        cancel->load(std::memory_order_relaxed)) {
      return false;
    }
    const DWORD rc = WaitForSingleObject(proc, slice_ms);
    if (rc == WAIT_OBJECT_0) {
      return true;
    }
    if (rc != WAIT_TIMEOUT) {
      // Abandoned / failed: treat like exit (nothing left to wait for).
      return true;
    }
    if (GetTickCount64() - start >= total_ms) {
      return false;
    }
  }
}

// Budgets (dev-contracts.md S1: quit ack 2s, then up to 8s for PID exit).
constexpr DWORD kGracefulWaitMs = 8000;
constexpr DWORD kKillWaitMs = 3000;
constexpr DWORD kWaitSliceMs = 100;

}  // namespace

ApplyManager::ApplyManager(QObject* parent) : QObject(parent) {}

ApplyManager::~ApplyManager() {
  // No ipc_ touch here: the borrowed client may already be destroyed
  // (member order). Owners cancel their own workers while alive.
  cancel_.store(true, std::memory_order_relaxed);
}

void ApplyManager::AppendLog(const QString& line) {
  log_.append(line);
  emit LogMessage(line);
}

bool ApplyManager::WriteConfig(const WallpaperConfig& cfg,
                               QString* error_out) {
  // Honor the engine's --config resolution: custom override when set,
  // DefaultConfigPath() otherwise (same rule as engine_app.cpp).
  const std::filesystem::path path = ConfigPath();
  AppendLog(QStringLiteral("Apply: writing config to %1")
                .arg(QString::fromStdWString(path.wstring())));
  try {
    SaveConfig(path, cfg);
  } catch (const ConfigError& e) {
    const QString msg =
        QStringLiteral("Apply: SaveConfig failed: %1")
            .arg(QString::fromUtf8(e.what()));
    AppendLog(msg);
    if (error_out != nullptr) {
      *error_out = msg;
    }
    return false;
  }
  AppendLog(QStringLiteral("Apply: config written OK"));
  return true;
}

bool ApplyManager::IsDevEngineFallbackEnabled() {
#ifdef _DEBUG
  // Explicit opt-in only: exactly "1", anything else (unset/empty/other)
  // keeps the fallback off. Fixed-size stack buffer: longer values report
  // their required size in `n` and fail the `n == 1` check.
  wchar_t buf[8] = {};
  const DWORD n = GetEnvironmentVariableW(L"K6WP_DEV", buf, 8);
  return n == 1 && buf[0] == L'1';
#else
  return false;
#endif
}

QString ApplyManager::ResolveEnginePath() {
  const QString exe_dir = QCoreApplication::applicationDirPath();
  const QString primary = QDir(exe_dir).filePath(QStringLiteral("engine.exe"));
  if (QFile::exists(primary)) {
    return QDir::cleanPath(primary);
  }
#ifdef _DEBUG
  // LOW-16: dev-only fallback, never silent. The literal below exists only
  // in _DEBUG binaries (release builds compile this branch out entirely, so
  // no release studio.exe can reference the dev tree), and even a _DEBUG
  // binary uses it only with K6WP_DEV=1 (IsDevEngineFallbackEnabled).
  if (IsDevEngineFallbackEnabled()) {
    const QString fallback = QDir(exe_dir).filePath(
        QStringLiteral("../../build/msvc-dev/engine.exe"));
    return QDir::cleanPath(fallback);
  }
#endif
  return QDir::cleanPath(primary);
}

bool ApplyManager::RestartEngine(QString* error_out) {
  const QString engine_path = ResolveEnginePath();
  const QString config_path =
      QString::fromStdWString(ConfigPath().wstring());
  AppendLog(QStringLiteral("Apply: engine path = %1").arg(engine_path));

  if (!QFile::exists(engine_path)) {
    const QString msg =
        QStringLiteral("Apply: engine.exe not found at %1").arg(engine_path);
    AppendLog(msg);
    if (error_out != nullptr) {
      *error_out = msg;
    }
    return false;
  }

  DWORD engine_pid = 0;
  if (ipc_ != nullptr) {
    const IpcResult st = ipc_->GetState();
    engine_pid = EnginePidFromState(st);
    if (engine_pid != 0) {
      AppendLog(QStringLiteral("Apply: engine PID %1 from get_state")
                    .arg(engine_pid));
    } else {
      AppendLog(QStringLiteral("Apply: no running engine (get_state failed); "
                               "starting fresh"));
    }
  } else {
    AppendLog(QStringLiteral(
        "Apply: shared IpcClient not wired; skipping graceful stop"));
  }

  if (engine_pid != 0) {
    HANDLE raw = OpenProcess(SYNCHRONIZE | PROCESS_TERMINATE, FALSE, engine_pid);
    if (raw == nullptr) {
      AppendLog(QStringLiteral("Apply: engine PID %1 already gone")
                    .arg(engine_pid));
      engine_pid = 0;
    } else {
      ProcHandle proc(raw);
      AppendLog(QStringLiteral("Apply: requesting graceful shutdown (quit)"));
      if (ipc_ != nullptr) {
        (void)ipc_->Quit();
      }
      // HIGH-3 slice B: cancellable 8s wait in 100ms slices (worker thread).
      if (WaitForProcessExit(proc.get(), kGracefulWaitMs, kWaitSliceMs,
                             &cancel_)) {
        AppendLog(QStringLiteral("Apply: old engine exited gracefully"));
      } else if (cancel_.load(std::memory_order_relaxed)) {
        AppendLog(QStringLiteral("Apply: restart cancelled while waiting "
                                 "for old engine exit"));
        if (error_out != nullptr) {
          *error_out = QStringLiteral("Apply: restart dibatalkan");
        }
        return false;
      } else {
        AppendLog(QStringLiteral(
                      "Apply: engine still alive after 8s; terminating PID %1")
                      .arg(engine_pid));
        if (TerminateProcess(proc.get(), 1) == 0) {
          AppendLog(QStringLiteral("Apply: warning: TerminateProcess failed "
                                   "(error %1)")
                        .arg(GetLastError()));
        } else {
          (void)WaitForProcessExit(proc.get(), kKillWaitMs, kWaitSliceMs,
                                   &cancel_);
          AppendLog(QStringLiteral("Apply: old engine terminated"));
        }
      }
      engine_pid = 0;
    }
  }

  auto spawn_engine = [&]() -> bool {
    AppendLog(QStringLiteral("Apply: starting engine.exe --config %1")
                  .arg(config_path));
    return QProcess::startDetached(
        engine_path, {QStringLiteral("--config"), config_path});
  };

  if (!spawn_engine()) {
    const QString msg = QStringLiteral("Apply: failed to start %1")
                            .arg(engine_path);
    AppendLog(msg);
    if (error_out != nullptr) {
      *error_out = msg;
    }
    return false;
  }

  if (WaitForEnginePipe(10000, 250, &cancel_)) {
    AppendLog(QStringLiteral("Apply: engine restarted OK (pipe ready)"));
    return true;
  }

  bool engine_alive = WaitNamedPipeW(CurrentSessionPipeName().c_str(), 500) != 0;
  if (ipc_ != nullptr) {
    const IpcResult probe = ipc_->GetState();
    engine_alive = probe.status != IpcStatus::kNotRunning;
  }
  if (!engine_alive) {
    AppendLog(QStringLiteral(
        "Apply: engine died during start (mutex race); respawning once ..."));
    if (!spawn_engine()) {
      const QString msg = QStringLiteral("Apply: respawn failed for %1")
                              .arg(engine_path);
      AppendLog(msg);
      if (error_out != nullptr) {
        *error_out = msg;
      }
      return false;
    }
    if (WaitForEnginePipe(5000, 250, &cancel_)) {
      AppendLog(
          QStringLiteral("Apply: engine restarted OK (pipe ready, 2nd try)"));
      return true;
    }
  }
  const QString msg =
      QStringLiteral("Apply: engine started but pipe not ready");
  AppendLog(msg);
  if (error_out != nullptr) {
    *error_out = msg;
  }
  return false;
}

bool ApplyManager::SyncMonitor(int monitor_id, QString* error_out) {
  if (last_monitor_.has_value() && *last_monitor_ == monitor_id) {
    return true;  // unchanged since the last push — no IPC needed.
  }
  if (ipc_ == nullptr) {
    const QString msg = QStringLiteral(
        "Apply: shared IpcClient not wired (call SetIpcClient first)");
    AppendLog(msg);
    if (error_out != nullptr) {
      *error_out = msg;
    }
    return false;
  }
  AppendLog(QStringLiteral("Apply: monitor changed, pushing set_monitor %1")
                .arg(monitor_id));
  const IpcResult res = ipc_->SetMonitor(monitor_id);
  if (res.status == IpcStatus::kOk) {
    last_monitor_ = monitor_id;
    AppendLog(QStringLiteral("Apply: set_monitor OK"));
    return true;
  }
  const QString msg =
      (res.status == IpcStatus::kNotRunning)
          ? tr("Apply: engine mati saat sinkron layar — klik Start Engine dulu")
          : QStringLiteral("Apply: engine rejected set_monitor: %1")
                .arg(QString::fromStdString(res.error));
  AppendLog(msg);
  if (error_out != nullptr) {
    *error_out = msg;
  }
  return false;
}

ApplyResult ApplyManager::Apply(const WallpaperConfig& cfg,
                                              QString* error_out) {
  // Single Apply path (was duplicated in MainWindow::ApplyOnWorker — the
  // MainWindow copy is what shipped and handled recovery/readback, so THOSE
  // semantics won and live here now; the caller writes the config already).
  ApplyResult r;
  AppendLog(QStringLiteral("Apply: begin (IPC live-switch, restart fallback)"));
  cancel_.store(false, std::memory_order_relaxed);
  if (ipc_ == nullptr) {
    const QString msg = QStringLiteral(
        "Apply: shared IpcClient not wired (call SetIpcClient first)");
    AppendLog(msg);
    r.outcome = ApplyResult::Outcome::kRejected;
    r.first_error = msg.toStdString();
    if (error_out != nullptr) {
      *error_out = msg;
    }
    return r;
  }
  ipc_->ClearCancel();
  // Live-switch over the shared client (T15). The 500ms budget covers the
  // render swap, not just the ack (acks alone measure <1ms — Todo 28/30
  // learnings).
  const std::string utf8_path =
      std::filesystem::path(cfg.video_path).u8string();
  AppendLog(QStringLiteral("Apply: trying IPC live-switch ..."));
  const auto t0 = std::chrono::steady_clock::now();
  IpcResult res = ipc_->SetVideo(utf8_path);
  if (res.status == IpcStatus::kError) {
    // T16: pipe drop mid-apply gets exactly ONE retry on a fresh connection,
    // then a polite failure (the caller offers the manual Start/Restart
    // button). No RestartEngine here — restart is only for the dead-engine
    // (kNotRunning) path below.
    r.retried = true;
    AppendLog(QStringLiteral("Apply: live-switch error, retrying once ..."));
    ipc_->Disconnect();
    res = ipc_->SetVideo(utf8_path);
  }
  const long long elapsed_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::steady_clock::now() - t0)
          .count();
  if (res.status == IpcStatus::kOk) {
    r.outcome = ApplyResult::Outcome::kLiveOk;
    AppendLog(QStringLiteral("Apply: IPC live-switch OK (%1 ms)").arg(elapsed_ms));
    r.monitor_ok = SyncMonitor(cfg.monitor_id, &r.monitor_error);
    if (!r.monitor_ok) {
      AppendLog(QStringLiteral("Apply: monitor sync skipped/failed: %1")
                    .arg(r.monitor_error));
    }
    return r;
  }
  if (res.status == IpcStatus::kNotRunning) {
    r.restart_attempted = true;
    AppendLog(tr("Apply: engine mati, memakai jalur mulai ulang"));
    if (!RestartEngine(error_out)) {
      r.outcome = ApplyResult::Outcome::kRestartFailed;
      AppendLog(QStringLiteral("Apply: FAILED at RestartEngine"));
      return r;
    }
    // Fresh engine's pipe is ready (RestartEngine waited); still give the
    // state poll a bounded window before re-sending the video explicitly.
    if (!k6wp::WaitForEngineReady(*ipc_, cancel_, 8000)) {
      r.outcome = ApplyResult::Outcome::kNotReady;
      return r;
    }
    const IpcResult retry = ipc_->SetVideo(utf8_path);
    if (retry.status == IpcStatus::kOk) {
      r.outcome = ApplyResult::Outcome::kAfterRestartOk;
      r.monitor_ok = SyncMonitor(cfg.monitor_id, &r.monitor_error);
      if (!r.monitor_ok) {
        AppendLog(QStringLiteral("Apply: monitor sync skipped/failed: %1")
                      .arg(r.monitor_error));
      }
      try {
        const nlohmann::json state = ipc_->GetState().raw.value(
            "state", nlohmann::json::object());
        r.live_path = QString::fromUtf8(
            state.value("video", std::string()).c_str());
      } catch (const std::exception&) {
      }
      AppendLog(
          QStringLiteral("Apply: IPC live-switch OK (auto-start)"));
      return r;
    }
    r.outcome = ApplyResult::Outcome::kRetryFailed;
    r.retry_error = retry.error;
    return r;
  }
  // T16: live-switch failed even after the single retry (or the engine
  // rejected it): polite failure, manual recovery owned by the GUI slot.
  r.outcome = ApplyResult::Outcome::kRejected;
  r.first_error = res.error;
  if (error_out != nullptr) {
    *error_out = QStringLiteral(
                     "Apply: engine rejected live-switch after 1 retry: %1. "
                     "Klik Start Engine lalu coba Apply lagi.")
                     .arg(QString::fromStdString(res.error));
  }
  return r;
}

void ApplyManager::RequestCancel() {
  cancel_.store(true, std::memory_order_relaxed);
  if (ipc_ != nullptr) {
    ipc_->RequestCancel();
  }
}

}  // namespace k6wp
