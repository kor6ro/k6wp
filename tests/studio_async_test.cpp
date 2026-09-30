// studio_async_test.cpp — HIGH-3 slice B: IPC/process waits off the GUI thread.
//
// Covers (offscreen Qt, no engine required):
//  1. IpcClient::RequestCancel aborts a worker Send fast (pre-set + mid-flight
//     during the 2s ack poll against a never-acking fake pipe server).
//  2. Errors return through the queued QFutureWatcher signal path (the exact
//     GUI pattern: QtConcurrent worker + finished() slot).
//  3. The event loop keeps processing while a worker blocks in Send
//     (QTimer counter rises during a ~2s ack-timeout wait).
//  4. A kError ack mid-apply gets exactly ONE retry (fake server rejects
//     the first connection, acks the second; Apply returns true with 2
//     accepts).
//
// The fake server listens on the REAL session pipe name, so every server
// test first probes for a live engine and SKIPs when one answers (never
// steal the real engine's endpoint). Wire format + 2s ack deadline are
// asserted intact, never changed.

#include "apply_manager.hpp"
#include "ipc_client.hpp"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <string>
#include <thread>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <QCoreApplication>
#include <QEventLoop>
#include <QFutureWatcher>
#include <QTimer>
#include <QtConcurrent>

namespace {

int g_checks = 0;
int g_failures = 0;

void Check(bool cond, const std::string& name) {
  ++g_checks;
  if (cond) {
    std::printf("PASS %s\n", name.c_str());
  } else {
    ++g_failures;
    std::printf("FAIL %s\n", name.c_str());
  }
}

bool HasSubstr(const std::string& hay, const std::string& needle) {
  return hay.find(needle) != std::string::npos;
}

// True when a live engine answers the session pipe (then the fake-server
// tests must SKIP — never hijack the real endpoint).
bool EngineAlive() {
  const std::wstring name = k6wp::CurrentSessionPipeName();
  HANDLE h =
      CreateFileW(name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                  OPEN_EXISTING, 0, nullptr);
  if (h == INVALID_HANDLE_VALUE) {
    return false;
  }
  CloseHandle(h);
  return true;
}

HANDLE MakePipeInstance() {
  const std::wstring name = k6wp::CurrentSessionPipeName();
  return CreateNamedPipeW(name.c_str(),
                          PIPE_ACCESS_DUPLEX,
                          PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE |
                              PIPE_WAIT,
                          1, 65536, 65536, 0, nullptr);
}

// Reads one message; false on disconnect/failure.
bool ReadOne(HANDLE pipe, std::string* out) {
  char buf[65536];
  DWORD got = 0;
  if (ReadFile(pipe, buf, sizeof(buf), &got, nullptr) == 0 || got == 0) {
    return false;
  }
  out->assign(buf, got);
  return true;
}

bool WriteOne(HANDLE pipe, const std::string& msg) {
  DWORD written = 0;
  return WriteFile(pipe, msg.data(), static_cast<DWORD>(msg.size()),
                   &written, nullptr) != 0 &&
         written == msg.size();
}

// Blocks a QEventLoop until the watcher finishes or the guard fires.
template <typename T>
T WaitForWatcher(QFutureWatcher<T>* watcher, int timeout_ms) {
  QEventLoop loop;
  QTimer guard;
  guard.setSingleShot(true);
  QObject::connect(&guard, &QTimer::timeout, &loop, &QEventLoop::quit);
  QObject::connect(watcher, &QFutureWatcher<T>::finished, &loop,
                   &QEventLoop::quit);
  guard.start(timeout_ms);
  if (!watcher->isFinished()) {
    loop.exec();
  }
  return watcher->result();
}

}  // namespace

int main(int argc, char** argv) {
  setvbuf(stdout, nullptr, _IONBF, 0);
  // QCoreApplication, not QApplication: this test creates no widgets and
  // must not load a QPA platform plugin (only qwindows is deployed; the
  // offscreen plugin is absent and its load failure hangs init).
  QCoreApplication app(argc, argv);

  // 1. Pre-set cancel: no engine, Send must abort the connect-retry gaps
  //    fast with a "cancelled" error. The status is kError, not kNotRunning:
  //    the caller cancelled, and reporting "engine not running" would tell the
    //    user their engine is down when they only asked to stop waiting. The
    //    engine really being absent is covered by the clear-cancel leg below.
    {
      k6wp::IpcClient client;
      client.RequestCancel();
      const ULONGLONG t0 = GetTickCount64();
      const k6wp::IpcResult r = client.Send(k6wp::Cmd::get_state);
      const ULONGLONG dt = GetTickCount64() - t0;
      std::printf("[info] preset-cancel: status=%d dt=%llums err=%s\n",
                  static_cast<int>(r.status), dt, r.error.c_str());
      Check(r.status == k6wp::IpcStatus::kError,
          "preset-cancel maps to kError, not kNotRunning");
    Check(HasSubstr(r.error, "cancelled"), "preset-cancel error says cancelled");
    Check(dt < 800, "preset-cancel returns fast (<800ms, no 500ms gaps)");
    client.ClearCancel();
    const k6wp::IpcResult r2 = client.Send(k6wp::Cmd::get_state);
    Check(r2.status == k6wp::IpcStatus::kNotRunning,
          "clear-cancel still kNotRunning without engine");
    Check(!HasSubstr(r2.error, "cancelled"),
          "clear-cancel error has no cancelled marker");
  }

  const bool skip_server = EngineAlive();
  std::printf("[info] engine alive: %d (server tests %s)\n",
              static_cast<int>(skip_server),
              skip_server ? "SKIPPED" : "running");

  // 2+3. Never-acking server: mid-flight cancel + event-loop liveness + the
  // intact 2s ack deadline (three sequential connections, one server).
  if (!skip_server) {
    std::atomic<int> accepts{0};
    std::atomic<bool> stop_server{false};
    std::thread server([&]() {
      for (int i = 0; i < 3 && !stop_server.load(); ++i) {
        HANDLE pipe = MakePipeInstance();
        if (pipe == INVALID_HANDLE_VALUE) {
          return;
        }
        const BOOL ok =
            ConnectNamedPipe(pipe, nullptr) != 0 ||
            GetLastError() == ERROR_PIPE_CONNECTED;
        if (!ok) {
          CloseHandle(pipe);
          return;
        }
        ++accepts;
        std::string req;
        if (!ReadOne(pipe, &req)) {
          CloseHandle(pipe);
          continue;
        }
        if (i < 2) {
          // First two connections: read the request, never ack. The client
          // either cancels (test 2) or rides the full 2s deadline (test 3);
          // the second ReadFile unblocks when the client disconnects.
          std::string dummy;
          (void)ReadOne(pipe, &dummy);
        } else {
          // Third connection: stay silent too (deadline path re-verified
          // implicitly); just drain until the client goes away.
          std::string dummy;
          (void)ReadOne(pipe, &dummy);
        }
        CloseHandle(pipe);
      }
    });

    // 2b. A cancel requested BEFORE Send must also map to kError, not
    //     kNotRunning. Connect() returns a bare bool, so a pre-set cancel is
    //     indistinguishable from an absent pipe inside Send; reporting
    //     kNotRunning there tells the user the engine is down when it is up
    //     and they merely asked to stop waiting. This is the deterministic
    //     twin of the mid-flight case below, which only lands here when the
    //     cancel happens to race the connect.
    {
      k6wp::IpcClient client;
      client.RequestCancel();
      const k6wp::IpcResult r = client.Send(k6wp::Cmd::get_state);
      Check(r.status == k6wp::IpcStatus::kError,
            "pre-set cancel maps to kError, not kNotRunning");
    }

    // 2. Mid-flight cancel during the ack poll: cancel at ~300ms, Send must
    //    return kError "cancelled" well before the 2s deadline.
    {
      k6wp::IpcClient client;
      QFutureWatcher<k6wp::IpcResult> watcher;
      watcher.setFuture(QtConcurrent::run([&]() {
        return client.Send(k6wp::Cmd::get_state);
      }));
      std::this_thread::sleep_for(std::chrono::milliseconds(300));
      const ULONGLONG t0 = GetTickCount64();
      client.RequestCancel();
      const k6wp::IpcResult r = WaitForWatcher(&watcher, 5000);
      const ULONGLONG dt = GetTickCount64() - t0;
      std::printf("[info] midflight-cancel: status=%d dt=%llums err=%s\n",
                  static_cast<int>(r.status), dt, r.error.c_str());
      Check(watcher.isFinished(), "midflight-cancel watcher finished");
      Check(r.status == k6wp::IpcStatus::kError,
            "midflight-cancel maps to kError");
      Check(HasSubstr(r.error, "cancelled"),
            "midflight-cancel error says cancelled");
      Check(dt < 1500, "midflight-cancel beats the 2s deadline");
    }

    // 3. Event loop stays live during a full 2s worker Send: a 50ms QTimer
    //    must fire repeatedly while the worker blocks, and the ack-timeout
    //    contract (2s deadline, error text) must be intact.
    {
      k6wp::IpcClient client;
      int ticks = 0;
      QTimer ticker;
      ticker.setInterval(50);
      QObject::connect(&ticker, &QTimer::timeout, [&]() { ++ticks; });
      ticker.start();
      QFutureWatcher<k6wp::IpcResult> watcher;
      const ULONGLONG t0 = GetTickCount64();
      watcher.setFuture(QtConcurrent::run([&]() {
        return client.Send(k6wp::Cmd::get_state);
      }));
      const k6wp::IpcResult r = WaitForWatcher(&watcher, 8000);
      const ULONGLONG dt = GetTickCount64() - t0;
      ticker.stop();
      std::printf("[info] responsive-wait: ticks=%d dt=%llums status=%d\n",
                  ticks, dt, static_cast<int>(r.status));
      Check(ticks >= 10,
            "responsive-wait event loop ticked during worker block");
      Check(r.status == k6wp::IpcStatus::kError,
            "responsive-wait ack timeout maps to kError");
      Check(HasSubstr(r.error, "ack timeout (2s)"),
            "responsive-wait keeps the 2s ack deadline contract");
      Check(dt >= 1800 && dt < 6000,
            "responsive-wait took ~2s (deadline honored, no hang)");
    }

    stop_server.store(true);
    // Unblock the server if it still waits for a third connection: a
    // pre-cancelled Send still connects + writes (waking ConnectNamedPipe)
    // then aborts the ack poll immediately — no 2s stall.
    {
      k6wp::IpcClient probe;
      probe.RequestCancel();
      (void)probe.Send(k6wp::Cmd::get_state);
    }
    server.join();
    std::printf("[info] server accepts=%d\n", accepts.load());
  }

  // 4. Exactly one retry on a kError ack: server rejects connection #1 with
  // {"error":...}, acks #2 with {"ok":true}; Apply must succeed with 2
  // accepts. (A raw handle-drop maps to kNotRunning → the restart path, not
  // the T16 retry — so the retry is driven by a proper error ack here.)
  if (!skip_server) {
    std::atomic<int> accepts{0};
    std::thread server([&]() {
      for (int i = 0; i < 2; ++i) {
        HANDLE pipe = MakePipeInstance();
        if (pipe == INVALID_HANDLE_VALUE) {
          return;
        }
        const BOOL ok =
            ConnectNamedPipe(pipe, nullptr) != 0 ||
            GetLastError() == ERROR_PIPE_CONNECTED;
        if (!ok) {
          CloseHandle(pipe);
          return;
        }
        ++accepts;
        std::string req;
        if (!ReadOne(pipe, &req)) {
          CloseHandle(pipe);
          continue;
        }
        if (i == 0) {
          (void)WriteOne(pipe, "{\"error\":\"boom\"}");
        } else {
          (void)WriteOne(pipe, "{\"ok\":true}");
        }
        std::string dummy;
        (void)ReadOne(pipe, &dummy);
        CloseHandle(pipe);
      }
    });

    k6wp::IpcClient client;
    k6wp::ApplyManager mgr;
    mgr.SetIpcClient(&client);
    k6wp::WallpaperConfig cfg;
    cfg.video_path = L"C:\\videos\\wallpaper.mp4";
    QString error;
    QFutureWatcher<bool> watcher;
    watcher.setFuture(QtConcurrent::run([&]() {
      return static_cast<bool>(mgr.Apply(cfg, &error));
    }));
    const bool ok = WaitForWatcher(&watcher, 15000);
    server.join();
    std::printf("[info] retry-1x: ok=%d accepts=%d err=%s\n",
                static_cast<int>(ok), accepts.load(),
                error.toStdString().c_str());
    Check(ok, "retry-1x apply succeeds after one kError");
    Check(accepts.load() == 2, "retry-1x server saw exactly 2 connections");
  }

  std::printf("checks=%d failures=%d\n", g_checks, g_failures);
  return g_failures == 0 ? 0 : 1;
}
