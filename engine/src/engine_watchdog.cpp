#include "engine_watchdog.hpp"

#include "crash_dump_prune.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#pragma comment(lib, "dbghelp.lib")
#include <dbghelp.h>

#include <chrono>
#include <cstdio>
#include <string>
#include <thread>
#include <vector>

namespace k6wp {

namespace {

// The path of the sibling launcher (K6WP.exe), used to restart the engine
// after a hang. Empty when it cannot be resolved.
std::wstring SiblingLauncherPath() {
  wchar_t exe_path[MAX_PATH] = {};
  const DWORD len = GetModuleFileNameW(nullptr, exe_path, MAX_PATH);
  if (len == 0 || len >= MAX_PATH) return {};
  const std::wstring candidate =
      std::wstring(exe_path, len) + L"\\..\\K6WP.exe";
  return candidate;
}

// Writes a full minidump of THIS process into %LOCALAPPDATA%\K6WP\crashes\.
// Returns the path (empty on failure). Never throws.
//
// This is the hang counterpart of the crash dump in main.cpp: a hang leaves
// no exception behind, so the dump has to be taken proactively - and it must
// capture the STUCK thread, which is exactly what a hang dump is for.
std::wstring WriteHangMinidump() {
  wchar_t local_app_data[MAX_PATH] = {};
  const DWORD len =
      GetEnvironmentVariableW(L"LOCALAPPDATA", local_app_data, MAX_PATH);
  if (len == 0 || len >= MAX_PATH) return {};

  wchar_t crash_dir[MAX_PATH] = {};
  if (swprintf_s(crash_dir, MAX_PATH, L"%s\\K6WP\\crashes", local_app_data) < 0) {
    return {};
  }
  if (!CreateDirectoryW(crash_dir, nullptr) &&
      GetLastError() != ERROR_ALREADY_EXISTS) {
    return {};
  }
  // Same cap as the crash dumps: the directory never grows without bound.
  PruneOldCrashDumps(crash_dir);

  SYSTEMTIME st = {};
  GetLocalTime(&st);
  wchar_t dump_path[MAX_PATH] = {};
  if (swprintf_s(dump_path, MAX_PATH, L"%s\\engine-hang-%04u%02u%02u-%02u%02u%02u.dmp",
                 crash_dir, st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute,
                 st.wSecond) < 0) {
    return {};
  }

  HANDLE file = CreateFileW(dump_path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL, nullptr);
  if (file == INVALID_HANDLE_VALUE) return {};

  // MiniDumpWithFullMemoryInfo: the stuck thread's stack is the whole point
  // of a hang dump, so the memory is included (a few MB, written once).
  const BOOL ok = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),
                                    file,
                                    static_cast<MINIDUMP_TYPE>(
                                        MiniDumpWithFullMemoryInfo |
                                        MiniDumpWithThreadInfo),
                                    nullptr, nullptr, nullptr);
  CloseHandle(file);
  if (!ok) return {};
  return dump_path;
}

// Spawns the launcher detached: it waits `--delay-ms` and then starts a
// replacement engine. `--restarted` makes the relaunch count against the
// engine's crash-restart backoff.
bool SpawnRestartHelper() {
  const std::wstring launcher = SiblingLauncherPath();
  if (launcher.empty()) return false;
  const std::wstring args = L"--engine --silent --restarted --delay-ms 3000";
  std::wstring cmd = L"\"" + launcher + L"\" " + args;
  std::vector<wchar_t> mutable_cmd(cmd.begin(), cmd.end());
  mutable_cmd.push_back(L'\0');
  STARTUPINFOW si{};
  si.cb = sizeof(si);
  PROCESS_INFORMATION pi{};
  if (!CreateProcessW(launcher.c_str(), mutable_cmd.data(), nullptr, nullptr,
                      FALSE, DETACHED_PROCESS | CREATE_NO_WINDOW, nullptr,
                      nullptr, &si, &pi)) {
    return false;
  }
  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);
  return true;
}

}  // namespace

EngineWatchdog::~EngineWatchdog() { Stop(); }

void EngineWatchdog::Start(std::atomic<unsigned long long>* heartbeat,
                            std::atomic<bool>* parked, int hang_timeout_ms,
                            int check_interval_ms) {
  if (running_.load(std::memory_order_acquire)) return;
  stop_.store(false, std::memory_order_release);
  try {
    std::thread worker([this, heartbeat, parked, hang_timeout_ms,
                         check_interval_ms] {
      unsigned long long last_seen =
          heartbeat != nullptr ? heartbeat->load(std::memory_order_acquire) : 0;
      auto last_change = std::chrono::steady_clock::now();
      while (!stop_.load(std::memory_order_acquire)) {
        // A timed wait (not a plain sleep): the watcher adds NO wakeups to
        // the message loop and no work to the paused engine, and Stop()
        // can still wake it immediately.
        {
          std::unique_lock<std::mutex> lock(wake_mutex_);
          wake_cv_.wait_for(lock, std::chrono::milliseconds(check_interval_ms),
                            [this] { return stop_.load(std::memory_order_acquire); });
        }
        if (stop_.load(std::memory_order_acquire)) break;
        if (heartbeat == nullptr) continue;
        const unsigned long long current =
            heartbeat->load(std::memory_order_acquire);
        const bool is_parked =
            parked != nullptr && parked->load(std::memory_order_acquire);
        if (current != last_seen) {
          last_seen = current;
          last_change = std::chrono::steady_clock::now();
          continue;
        }
        const auto now = std::chrono::steady_clock::now();
        const long long now_ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch())
                .count();
        const long long change_ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                last_change.time_since_epoch())
                .count();
        if (!IsHung(last_seen, current, is_parked, true, now_ms, change_ms,
                    hang_timeout_ms)) {
          continue;
        }
        if (log_) {
          log_("watchdog: the message loop stopped responding - writing a "
               "dump and restarting the engine");
        }
        const std::wstring dump = WriteHangMinidump();
        if (log_) {
          if (dump.empty()) {
            log_("watchdog: the minidump could not be written");
          } else {
            log_("watchdog: hang minidump written");
          }
        }
        if (!SpawnRestartHelper()) {
          if (log_) {
            log_("watchdog: the restart helper could not be started - "
                 "terminating without a replacement");
          }
        }
        // The engine is hung: its own shutdown path cannot run, so the
        // process is terminated here. The helper starts the replacement
        // after the singleton mutex is free.
        TerminateProcess(GetCurrentProcess(), 3);
      }
    });
    thread_ = new std::thread(std::move(worker));
    running_.store(true, std::memory_order_release);
  } catch (...) {
    if (log_) {
      log_("watchdog: the watcher thread could not be started - the engine "
           "runs without hang detection");
    }
  }
}

void EngineWatchdog::Stop() {
  if (!running_.load(std::memory_order_acquire)) return;
  stop_.store(true, std::memory_order_release);
  {
    std::lock_guard<std::mutex> lock(wake_mutex_);
    wake_cv_.notify_all();
  }
  auto* worker = static_cast<std::thread*>(thread_);
  thread_ = nullptr;
  running_.store(false, std::memory_order_release);
  if (worker == nullptr) return;
  // The watcher wakes on the notification above, so the join returns
  // immediately.
  worker->join();
  delete worker;
}

}  // namespace k6wp
