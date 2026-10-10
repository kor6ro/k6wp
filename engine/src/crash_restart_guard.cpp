// Crash-restart backoff (L-01). See crash_restart_guard.hpp for the contract.
//
// The decision logic (CrashRestartAdvance / CrashRestartAllowed) is pure and
// unit-tested from engine_units_test. Only ClearCrashRestartFile() touches
// Win32 — windows.h is confined to this TU so the header stays windows.h-free
// (the power.hpp split), and the file-I/O dependency is kernel32 only, which
// keeps crash_restart_guard.cpp linkable into the test binary with no extra
// libraries.

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <cwchar>

#include "crash_restart_guard.hpp"

namespace k6wp {
namespace {

// "%LOCALAPPDATA%\K6WP\crash_restart.txt". False (out untouched) when
// LOCALAPPDATA is unset — the caller then skips the clear silently.
bool CrashRestartPath(wchar_t (&out)[MAX_PATH]) {
  wchar_t local_app_data[MAX_PATH] = {};
  const DWORD len =
      GetEnvironmentVariableW(L"LOCALAPPDATA", local_app_data, MAX_PATH);
  if (len == 0 || len >= MAX_PATH) return false;
  return swprintf_s(out, MAX_PATH, L"%s\\K6WP\\crash_restart.txt",
                    local_app_data) >= 0;
}

}  // namespace

CrashRestartState CrashRestartAdvance(CrashRestartState s, long long now_ms,
                                      int max_restarts, long long window_ms) {
  // Budget is enforced by CrashRestartAllowed(); Advance only counts.
  (void)max_restarts;
  // A non-positive window disables the sliding behaviour: every restart is its
  // own window (count resets to 1 each time) — the safe interpretation of a
  // misconfigured window, never an unbounded counter.
  const bool new_window = s.count <= 0 || window_ms <= 0 ||
                          now_ms < s.window_start_ms ||
                          now_ms - s.window_start_ms >= window_ms;
  if (new_window) {
    s.count = 1;
    s.window_start_ms = now_ms;
  } else {
    s.count += 1;
  }
  return s;
}

bool CrashRestartAllowed(const CrashRestartState& s, int max_restarts) {
  return s.count <= max_restarts;
}

void ClearCrashRestartFile() {
  wchar_t path[MAX_PATH] = {};
  if (!CrashRestartPath(path)) return;
  // DELETE is the contract: a missing file (first clean run) or a locked file
  // (AV) is a best-effort no-op, never fatal.
  DeleteFileW(path);
}

}  // namespace k6wp
