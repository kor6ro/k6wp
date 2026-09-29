// fake_pipe_test.cpp — MED-2 slice B: in-process fake named-pipe server for
// IpcClient (Studio IPC client round-trips without a live engine).
//
// Qt-free single TU compiling the REAL studio/src/ipc_client.cpp (same
// pattern as studio_async_test.cpp, minus the Qt/async parts). Covers:
//  1. Pipe-name helpers: PipeNameForSession literal shape + the session
//     suffix on CurrentSessionPipeName (per-session pipe, MED-12).
//  2. Round-trips over a fake server on the REAL session pipe name:
//     set_video/get_state/set_monitor acks -> kOk (+ raw state pid),
//     {"error":...} ack -> kError, malformed ack -> kError. The server also
//     asserts the NDJSON wire it receives (version + cmd name intact).
//  3. No server -> kNotRunning (controlled connect budget, engine-dead path
//     the UI maps to "offer Start").
//
// Modes: default runs everything (server sections SKIP when a live engine
// answers the session pipe — never steal the real endpoint). `--no-server`
// runs only sections 1+3 (QA failure: fake server down -> kNotRunning).

#include "ipc_client.hpp"

#include <atomic>
#include <cstdio>
#include <string>
#include <thread>
#include <vector>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

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
// tests must SKIP — never hijack the real endpoint). Retried: a resident
// engine's single pipe instance is transiently BUSY while another client
// holds it — one probe then looks "dead" and the sections below race the
// real engine for the name.
bool EngineAlive() {
  for (int i = 0; i < 5; ++i) {
    const std::wstring name = k6wp::CurrentSessionPipeName();
    HANDLE h =
        CreateFileW(name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                    OPEN_EXISTING, 0, nullptr);
    if (h != INVALID_HANDLE_VALUE) {
      CloseHandle(h);
      return true;
    }
    if (GetLastError() == ERROR_PIPE_BUSY) {
      return true;  // someone holds it -> an engine is serving
    }
    Sleep(100);
  }
  return false;
}

HANDLE MakePipeInstance() {
  const std::wstring name = k6wp::CurrentSessionPipeName();
  return CreateNamedPipeW(name.c_str(), PIPE_ACCESS_DUPLEX,
                          PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                          1, 65536, 65536, 0, nullptr);
}

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
  return WriteFile(pipe, msg.data(), static_cast<DWORD>(msg.size()), &written,
                   nullptr) != 0 &&
         written == msg.size();
}

}  // namespace

int main(int argc, char** argv) {
  setvbuf(stdout, nullptr, _IONBF, 0);
  const bool no_server =
      argc > 1 && std::string(argv[1]) == "--no-server";

  // 1. Pipe-name helpers (no server needed).
  {
    const std::wstring s7 = k6wp::PipeNameForSession(7);
    Check(s7 == L"\\\\.\\pipe\\k6wp-engine-7", "pipe-name session 7 literal");
    const std::wstring cur = k6wp::CurrentSessionPipeName();
    Check(HasSubstr(std::string(cur.begin(), cur.end()), "k6wp-engine-"),
          "pipe-name current carries session suffix");
    Check(cur == k6wp::CurrentSessionPipeName(),
          "pipe-name current stable across calls");
  }

  const bool skip_server = EngineAlive();
  std::printf("[info] engine alive: %d no-server-mode: %d (server tests %s)\n",
              static_cast<int>(skip_server), static_cast<int>(no_server),
              (skip_server || no_server) ? "SKIPPED" : "running");

  // 3 (before any server exists). No listener -> Connect exhausts its
  // retry budget -> kNotRunning (never kError, never a hang).
  if (!skip_server) {
    k6wp::IpcClient client;
    const ULONGLONG t0 = GetTickCount64();
    const k6wp::IpcResult r = client.Send(k6wp::Cmd::get_state);
    const ULONGLONG dt = GetTickCount64() - t0;
    std::printf("[info] no-server: status=%d dt=%llums err=%s\n",
                static_cast<int>(r.status), dt, r.error.c_str());
    Check(r.status == k6wp::IpcStatus::kNotRunning,
          "no-server maps to kNotRunning");
    Check(dt < 8000, "no-server connect budget controlled (<8s, no hang)");
  }
  if (no_server) {
    std::printf("checks=%d failures=%d\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
  }

  // 2. Scripted round-trips over the fake server.
  if (!skip_server) {
    const std::vector<std::string> acks = {
        "{\"ok\":true}",
        "{\"ok\":true,\"state\":{\"pid\":4242,\"running\":true}}",
        "{\"ok\":true}",
        "{\"error\":\"paused-denied\"}",
        "this is not json{{{",
    };
    const std::vector<std::string> want_cmd = {
        "set_video",
        "get_state",
        "set_monitor",
        "pause",
        "resume",
    };
    std::vector<std::string> seen;
    std::vector<std::string> wire_ok;
    std::thread server([&]() {
      // Serves the 5 scripted acks in order across whatever connection
      // shape the client uses: one held connection (connection reuse —
      // all 5 requests on the first accept) or one connection per Send
      // (release-per-transaction — one request per accept). Script steps
      // are indexed by REQUEST, not by connection, so retries/reconnects
      // cannot misalign cmds and acks. Exits once all 5 are acked; the
      // client Disconnect() below releases a held pipe so join() returns.
      size_t step = 0;
      for (int c = 0; c < 5 && step < acks.size(); ++c) {
        HANDLE pipe = MakePipeInstance();
        if (pipe == INVALID_HANDLE_VALUE) {
          return;
        }
        const BOOL ok = ConnectNamedPipe(pipe, nullptr) != 0 ||
                        GetLastError() == ERROR_PIPE_CONNECTED;
        if (!ok) {
          CloseHandle(pipe);
          return;
        }
        for (;;) {
          std::string req;
          if (!ReadOne(pipe, &req)) {
            break;
          }
          seen.push_back(req);
          wire_ok.push_back(HasSubstr(req, "\"version\":1") &&
                                     HasSubstr(req, "\"" + want_cmd[step] + "\"")
                                 ? "1"
                                 : "0");
          if (!WriteOne(pipe, acks[step])) {
            break;
          }
          ++step;
          if (step >= acks.size()) {
            break;
          }
        }
        CloseHandle(pipe);
      }
    });

    k6wp::IpcClient client;
    {
      const k6wp::IpcResult r =
          client.SetVideo(std::string("C:\\videos\\wallpaper.mp4"));
      Check(r.status == k6wp::IpcStatus::kOk, "round-trip set_video kOk");
    }
    {
      const k6wp::IpcResult r = client.GetState();
      Check(r.status == k6wp::IpcStatus::kOk, "round-trip get_state kOk");
      Check(r.raw.is_object() && r.raw.contains("state") &&
                r.raw["state"].is_object() &&
                r.raw["state"].value("pid", 0) == 4242,
            "round-trip get_state raw carries pid 4242");
    }
    {
      const k6wp::IpcResult r = client.SetMonitor(2);
      Check(r.status == k6wp::IpcStatus::kOk, "round-trip set_monitor kOk");
    }
    {
      const k6wp::IpcResult r = client.Pause();
      Check(r.status == k6wp::IpcStatus::kError, "error-ack maps to kError");
      Check(HasSubstr(r.error, "paused-denied"),
            "error-ack surfaces engine message");
    }
    {
      const k6wp::IpcResult r = client.Resume();
      Check(r.status == k6wp::IpcStatus::kError, "malformed-ack maps to kError");
    }
    client.Disconnect();  // release a held connection so the server drains
    server.join();

    Check(seen.size() == acks.size(), "fake server saw all 5 requests");
    bool wire_all = seen.size() == acks.size();
    for (const auto& w : wire_ok) {
      wire_all = wire_all && w == "1";
    }
    Check(wire_all, "wire intact: version + cmd on every request");
  }

  std::printf("checks=%d failures=%d\n", g_checks, g_failures);
  return g_failures == 0 ? 0 : 1;
}
