// Engine entry point (Todo 11: /SUBSYSTEM:WINDOWS, no console window).
//
// wWinMain converts the wide command line to UTF-8 narrow args once and
// forwards them to EngineApp::Init — ParseCli keeps its (argc, char**)
// contract, so --help / --exit-after-ms and all pipe QA flows are unchanged
// (redirected stdout still works under the WINDOWS subsystem).
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <shellapi.h>

#include <algorithm>
#include <cwchar>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "crash_dump_prune.hpp"
#include "crash_restart_guard.hpp"
#include "engine_app.hpp"
#include "log_file.hpp"
#include "monitor_util.hpp"

// --- Crash minidump (release hardening) --------------------------------------
// Installed via SetUnhandledExceptionFilter in wWinMain. On an unhandled
// exception it writes a timestamped .dmp (system DbgHelp MiniDumpWriteDump)
// to %LOCALAPPDATA%\K6WP\crashes\ and appends one engine.log line with the
// dump path, then returns EXCEPTION_CONTINUE_SEARCH so the OS default
// termination still runs. The directory is capped (kMaxCrashDumps) the same
// way engine.log is capped in log_file.cpp. QE-guarded: every call is
// null-checked and allocation-free (no CRT heap, no std::filesystem, no
// mutex) so the filter itself can never fault a second time.
#pragma comment(lib, "dbghelp.lib")
#include <dbghelp.h>

namespace {

LONG WINAPI CrashDumpHandler(EXCEPTION_POINTERS* exception_info) {
  // Drain the batched log first so the .dmp has its diagnostics beside it.
  // Try-lock only: never wait in a filter when the crashed thread may hold
  // the log mutex.
  k6wp::FlushEngineLogFromCrash();
  if (exception_info == nullptr || exception_info->ExceptionRecord == nullptr) {
    return EXCEPTION_CONTINUE_SEARCH;
  }

  wchar_t local_app_data[MAX_PATH] = {};
  const DWORD len =
      GetEnvironmentVariableW(L"LOCALAPPDATA", local_app_data, MAX_PATH);
  if (len == 0 || len >= MAX_PATH) {
    return EXCEPTION_CONTINUE_SEARCH;
  }

  wchar_t crash_dir[MAX_PATH] = {};
  if (swprintf_s(crash_dir, MAX_PATH, L"%s\\K6WP\\crashes", local_app_data) < 0) {
    return EXCEPTION_CONTINUE_SEARCH;
  }
  if (!CreateDirectoryW(crash_dir, nullptr) &&
      GetLastError() != ERROR_ALREADY_EXISTS) {
    return EXCEPTION_CONTINUE_SEARCH;
  }
  // Prune before writing, not after: the dump about to be created is then never
  // itself a delete candidate (a backwards clock cannot make it look oldest),
  // and a failed prune still costs nothing but leftover files. See
  // crash_dump_prune.hpp - the prune leaves one slot free, so the directory
  // never holds more than kMaxCrashDumps dumps after a crash.
  k6wp::PruneOldCrashDumps(crash_dir);

  SYSTEMTIME st = {};
  GetLocalTime(&st);
  wchar_t dump_path[MAX_PATH] = {};
  if (swprintf_s(dump_path, MAX_PATH,
                 L"%s\\engine-%04u%02u%02u-%02u%02u%02u.dmp", crash_dir,
                 st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute,
                 st.wSecond) < 0) {
    return EXCEPTION_CONTINUE_SEARCH;
  }

  HANDLE file = CreateFileW(dump_path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL, nullptr);
  if (file == INVALID_HANDLE_VALUE) {
    return EXCEPTION_CONTINUE_SEARCH;
  }

  MINIDUMP_EXCEPTION_INFORMATION mei = {};
  mei.ThreadId = GetCurrentThreadId();
  mei.ExceptionPointers = exception_info;
  mei.ClientPointers = FALSE;
  const BOOL ok = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),
                                    file, MiniDumpNormal, &mei, nullptr,
                                    nullptr);
  CloseHandle(file);
  if (!ok) {
    return EXCEPTION_CONTINUE_SEARCH;
  }

  // One engine.log line with the dump path. Direct append (FILE_APPEND_DATA +
  // FILE_SHARE_WRITE): no mutex, no CRT file I/O, so this can never deadlock
  // a crashed process.
  wchar_t log_path[MAX_PATH] = {};
  if (swprintf_s(log_path, MAX_PATH, L"%s\\K6WP\\engine.log", local_app_data) >= 0) {
    wchar_t line_wide[MAX_PATH + 64] = {};
    if (swprintf_s(line_wide, MAX_PATH + 64, L"[crash] minidump written: %s",
                   dump_path) > 0) {
      char line_utf8[(MAX_PATH + 64) * 2] = {};
      const int n = WideCharToMultiByte(CP_UTF8, 0, line_wide, -1, line_utf8,
                                        static_cast<int>(sizeof(line_utf8)),
                                        nullptr, nullptr);
      if (n > 0) {
        HANDLE log = CreateFileW(log_path, FILE_APPEND_DATA,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                                 OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (log != INVALID_HANDLE_VALUE) {
          DWORD written = 0;
          WriteFile(log, line_utf8, static_cast<DWORD>(n - 1), &written,
                    nullptr);
          WriteFile(log, "\n", 1, &written, nullptr);
          CloseHandle(log);
        }
      }
    }
  }

  return EXCEPTION_CONTINUE_SEARCH;
}

// --- Crash-restart backoff (L-01) -------------------------------------------
// EngineApp::Init registers RegisterApplicationRestart(L"--restarted"), so
// Windows Error Reporting relaunches the engine after ANY unhandled exception.
// Without a budget a reproducible crash (a bad video decoded at every logon)
// crash-loops forever. Only a "--restarted" relaunch counts; the state file is
// a single "count|tick" line (tick = GetTickCount64, which survives a process
// restart but resets on reboot — a reboot then opens a fresh window, which is
// correct). CrashRestartAdvance/CrashRestartAllowed are the unit-tested pure
// rules; this TU owns only the file I/O. All file errors are non-fatal.
constexpr int kMaxAutoRestarts = 3;
constexpr long long kAutoRestartWindowMs = 10 * 60 * 1000;  // 10 min

std::filesystem::path CrashRestartStateFilePath() {
  wchar_t local_app_data[MAX_PATH] = {};
  const DWORD len =
      GetEnvironmentVariableW(L"LOCALAPPDATA", local_app_data, MAX_PATH);
  if (len == 0 || len >= MAX_PATH) return {};
  return std::filesystem::path(local_app_data) / L"K6WP" / L"crash_restart.txt";
}

k6wp::CrashRestartState LoadCrashRestartState() {
  k6wp::CrashRestartState state;
  const std::filesystem::path path = CrashRestartStateFilePath();
  if (path.empty()) return state;
  std::ifstream in(path, std::ios::binary);
  if (!in) return state;
  long long count = 0;
  long long tick = 0;
  char sep = 0;
  if (in >> count >> sep >> tick && sep == '|' && count >= 0) {
    state.count = static_cast<int>(count);
    state.window_start_ms = tick;
  }
  return state;  // corrupt/missing -> default {0,0} -> fresh window
}

void SaveCrashRestartState(const k6wp::CrashRestartState& state) {
  const std::filesystem::path path = CrashRestartStateFilePath();
  if (path.empty()) return;
  std::error_code ec;
  std::filesystem::create_directories(path.parent_path(), ec);
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  if (!out) return;  // best-effort: a stale count only tightens the budget
  out << state.count << '|' << state.window_start_ms << '\n';
}

}  // namespace

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    LPWSTR lpCmdLine, int nShowCmd) {
  // DLL-planting hardening: per-user install dir is user-writable; restrict
  // DLL search to the application directory and System32 only (engine/launcher
  // run at logon). Ignore return value explicitly.
  (void)SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_APPLICATION_DIR |
                                 LOAD_LIBRARY_SEARCH_SYSTEM32);
  (void)hInstance;
  (void)hPrevInstance;
  (void)lpCmdLine;
  (void)nShowCmd;
  // Crash minidump (release hardening): install before any engine logic so an
  // unhandled exception anywhere writes a .dmp + engine.log line.
  SetUnhandledExceptionFilter(&CrashDumpHandler);
  // PerMonitorV2 before any window is created. The embedded app.manifest
  // already declares PerMonitorV2, so these calls are defense-in-depth and
  // return FALSE as a no-op when the manifest set the context — never a
  // downgrade. SetProcessDPIAware covers pre-1703 Windows.
  if (!k6wp::SetProcessDpiAwarenessContextPMDA()) {
    ::SetProcessDPIAware();
  }
  // P1.5: run the whole process (and every child it spawns) at BELOW_NORMAL
  // priority. Never fatal: a failed class change logs and continues.
  if (!SetPriorityClass(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS)) {
    const std::string priority_error =
        "warning: SetPriorityClass(BELOW_NORMAL_PRIORITY_CLASS) failed (error " +
        std::to_string(static_cast<unsigned long>(GetLastError())) + ")";
    k6wp::AppendEngineLogLine(priority_error.c_str(), true);
  } else {
    k6wp::AppendEngineLogLine("priority: BELOW_NORMAL_PRIORITY_CLASS set", true);
  }
  int wargc = 0;
  LPWSTR* wargv = CommandLineToArgvW(GetCommandLineW(), &wargc);
  if (wargv == nullptr) {
    return 2;
  }
  std::vector<std::string> utf8_args;
  utf8_args.reserve(static_cast<size_t>(wargc > 0 ? wargc : 0));
  for (int i = 0; i < wargc; ++i) {
    const int needed = WideCharToMultiByte(CP_UTF8, 0, wargv[i], -1, nullptr,
                                           0, nullptr, nullptr);
    if (needed <= 0) {
      LocalFree(wargv);
      return 2;
    }
    std::string s(static_cast<size_t>(needed), '\0');
    if (WideCharToMultiByte(CP_UTF8, 0, wargv[i], -1, s.data(), needed,
                            nullptr, nullptr) <= 0) {
      LocalFree(wargv);
      return 2;
    }
    s.pop_back();  // drop the null written by WideCharToMultiByte
    utf8_args.push_back(std::move(s));
  }
  LocalFree(wargv);
  std::vector<char*> argv;
  argv.reserve(utf8_args.size());
  for (auto& s : utf8_args) {
    argv.push_back(s.data());
  }
  // L-01: a WER relaunch passes --restarted; only then does the restart count
  // against the backoff budget. Over budget → log, flush, and exit WITHOUT
  // booting the engine (the previous crash would just repeat).
  const bool engine_restarted =
      std::find(utf8_args.begin(), utf8_args.end(),
                std::string("--restarted")) != utf8_args.end();
  if (engine_restarted) {
    const k6wp::CrashRestartState advanced = k6wp::CrashRestartAdvance(
        LoadCrashRestartState(), static_cast<long long>(GetTickCount64()),
        kMaxAutoRestarts, kAutoRestartWindowMs);
    if (!k6wp::CrashRestartAllowed(advanced, kMaxAutoRestarts)) {
      k6wp::AppendEngineLogLine(
          "crash-restart: giving up after repeated crashes in 10 min "
          "(run K6WP manually)",
          true);
      k6wp::FlushEngineLog();
      return 0;
    }
    SaveCrashRestartState(advanced);
  }
  k6wp::EngineApp app;
  if (!app.Init(static_cast<int>(argv.size()), argv.data())) {
    return app.InitExitCode();
  }
  return app.Run();
}
