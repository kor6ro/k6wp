// Engine file-log sink implementation (Todo 11). See log_file.hpp.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include "log_file.hpp"

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <mutex>
#include <share.h>
#include <string>
#include <vector>

namespace k6wp {
namespace {

// Batching (Step 1C): lines accumulate here and reach disk when the buffer
// fills (kFlushBytes) or an important line arrives. A full-buffer flush
// drains every earlier line too, so boot context rides along with the first
// state-transition flush.
std::mutex g_file_mutex;
FILE* g_log_file = nullptr;
bool g_tried_open = false;
std::filesystem::path g_log_path;  // remembered so rotation can reopen
std::string g_buf;
constexpr size_t kFlushBytes = 8 * 1024;

// engine.log is capped at 10 MiB: beyond it the file is renamed to
// engine.log.1 (previous backup replaced) and a fresh file is opened.
constexpr std::uintmax_t kMaxLogBytes = 10ull * 1024 * 1024;

void DebugOut(const char* line) {
  const int wlen =
      MultiByteToWideChar(CP_UTF8, 0, line, -1, nullptr, 0);
  if (wlen > 0) {
    std::vector<wchar_t> wbuf(static_cast<size_t>(wlen));
    if (MultiByteToWideChar(CP_UTF8, 0, line, -1, wbuf.data(), wlen) > 0) {
      OutputDebugStringW(wbuf.data());
      OutputDebugStringW(L"\n");
      return;
    }
  }
  OutputDebugStringA(line);
  OutputDebugStringA("\n");
}

void TryOpenOnce() {
  wchar_t local_app_data[MAX_PATH] = {};
  const DWORD len =
      GetEnvironmentVariableW(L"LOCALAPPDATA", local_app_data, MAX_PATH);
  if (len == 0 || len >= MAX_PATH) {
    return;
  }
  std::error_code ec;
  const std::filesystem::path dir =
      std::filesystem::path(local_app_data) / L"K6WP";
  std::filesystem::create_directories(dir, ec);
  if (ec) {
    return;
  }
  const std::filesystem::path log_path = dir / L"engine.log";
  g_log_path = log_path;
  // _SH_DENYNO: external readers (PowerShell, Notepad) can read the log
  // while the engine holds it open. Plain fopen locks exclusively, which
  // made Get-Content fail with "being used by another process".
  FILE* f = _wfsopen(log_path.c_str(), L"a", _SH_DENYNO);
  if (f == nullptr) {
    return;
  }
  g_log_file = f;
}

// Caller holds g_file_mutex. When engine.log exceeds kMaxLogBytes, move it
// to engine.log.1 (replacing any previous backup) and reopen a fresh
// engine.log so a 24/7 run never grows unbounded. Rename can fail only in
// exotic cases (antivirus lock); a failed rotate keeps the current file.
void MaybeRotateLog() {
  if (g_log_file == nullptr || g_log_path.empty()) {
    return;
  }
  std::error_code ec;
  const std::uintmax_t size = std::filesystem::file_size(g_log_path, ec);
  if (ec || size < kMaxLogBytes) {
    return;
  }
  // Once a rotation attempt fails (AV/indexer holding engine.log.1), back off
  // for a minute instead of close/reopen-cycling on every important flush.
  static ULONGLONG next_attempt_ms = 0;
  const ULONGLONG now_ms = GetTickCount64();
  if (now_ms < next_attempt_ms) {
    return;
  }
  std::fclose(g_log_file);
  g_log_file = nullptr;
  std::filesystem::path rotated = g_log_path;
  rotated += L".1";
  std::filesystem::remove(rotated, ec);
  std::error_code ren_ec;
  std::filesystem::rename(g_log_path, rotated, ren_ec);
  next_attempt_ms = ren_ec ? (now_ms + 60000) : 0;
  FILE* f = _wfsopen(g_log_path.c_str(), L"a", _SH_DENYNO);
  if (f != nullptr) {
    g_log_file = f;
  }
}

// Caller holds g_file_mutex. Drains the buffer to disk (with rotation).
// Without a file the batch mirrors to the debugger and is dropped — same
// fallback policy as the old per-line path.
void FlushLocked() {
  if (g_buf.empty()) {
    return;
  }
  if (g_log_file == nullptr) {
    DebugOut(g_buf.c_str());
    g_buf.clear();
    return;
  }
  MaybeRotateLog();
  if (g_log_file == nullptr) {
    DebugOut(g_buf.c_str());
    g_buf.clear();
    return;
  }
  std::fwrite(g_buf.data(), 1, g_buf.size(), g_log_file);
  std::fflush(g_log_file);
  g_buf.clear();
}

// Warn/error levels always flush immediately, so a fatal line is never
// stuck behind the batch. Substring match on the codebase's lowercase
// logging vocabulary ("warning:", "error:", mpv [prefix/warn], "failed",
// "rejected").
bool IsImportantLine(const char* line) {
  return std::strstr(line, "warn") != nullptr ||
         std::strstr(line, "error") != nullptr ||
         std::strstr(line, "fatal") != nullptr ||
         std::strstr(line, "fail") != nullptr ||
         std::strstr(line, "reject") != nullptr;
}

}  // namespace

void AppendEngineLogLine(const char* line, bool important) {
  if (line == nullptr) {
    return;
  }
  std::lock_guard<std::mutex> lock(g_file_mutex);
  if (!g_tried_open) {
    g_tried_open = true;
    TryOpenOnce();
  }
  g_buf += line;
  g_buf += '\n';
  if (important || IsImportantLine(line) || g_buf.size() >= kFlushBytes) {
    FlushLocked();
  }
}

void FlushEngineLog() {
  std::lock_guard<std::mutex> lock(g_file_mutex);
  FlushLocked();
}

void FlushEngineLogFromCrash() {
  std::unique_lock<std::mutex> lock(g_file_mutex, std::try_to_lock);
  if (!lock.owns_lock()) {
    return;  // crashed while holding the log mutex: never wait in a filter
  }
  if (g_log_file == nullptr || g_buf.empty()) {
    return;
  }
  std::fwrite(g_buf.data(), 1, g_buf.size(), g_log_file);
  std::fflush(g_log_file);
  g_buf.clear();
}

}  // namespace k6wp
