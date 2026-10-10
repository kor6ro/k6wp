#pragma once

// Crash-restart backoff (L-01, audit remediation).
//
// The engine installs a minidump filter (main.cpp) and registers
// RegisterApplicationRestart(L"--restarted", 0) (engine_app.cpp), so Windows
// Error Reporting relaunches it automatically after ANY unhandled exception.
// Without a counter a reproducible crash (a bad video decoded at every logon)
// crash-loops forever with no user-visible backoff. This unit owns the PURE
// decision — windows.h-free, exactly the power.hpp split — so the window/count
// rules are unit-tested without a process. The persistence helper lives in the
// .cpp because it only wraps Win32 file I/O.
//
// State model: a sliding window of `window_ms`. The first restart after a
// window expiry (or the very first ever) opens a new window at now_ms with
// count 1; every further restart inside the window increments count. When
// count exceeds `max_restarts` the caller gives up and exits WITHOUT booting
// the engine (see main.cpp). The caller supplies a MONOTONIC clock
// (GetTickCount64 = system uptime, so it survives a process restart but resets
// on reboot — a reboot then looks like an expired window, which is correct).

namespace k6wp {

struct CrashRestartState {
  int count = 0;                  // automatic restarts inside the window
  long long window_start_ms = 0;  // monotonic ms when the window opened
};

// Advances the state for one automatic restart at monotonic `now_ms`
// (caller-supplied, normally GetTickCount64()). A zero count, an expired window
// (now - start >= window_ms), or a now_ms that went BACKWARDS (system reboot
// reset GetTickCount64) opens a fresh window at count 1; anything else
// increments. `max_restarts` does not clamp here — CrashRestartAllowed() is the
// budget check, so the caller can log the exact over-budget count.
CrashRestartState CrashRestartAdvance(CrashRestartState s, long long now_ms,
                                      int max_restarts, long long window_ms);

// True while the state is inside the allowed restart budget
// (count <= max_restarts). max_restarts == 0 refuses every automatic restart.
bool CrashRestartAllowed(const CrashRestartState& s, int max_restarts);

// Deletes %LOCALAPPDATA%\K6WP\crash_restart.txt so a clean run (engine alive
// >= 300 s) resets the counter and a later crash gets a fresh budget.
// Best-effort and never fatal: a missing or locked file is left alone — the
// worst case is a stale count, which only makes the backoff slightly stricter.
void ClearCrashRestartFile();

}  // namespace k6wp
