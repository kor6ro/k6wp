#pragma once

#include <cstdio>
#include <string>

#include "exit_codes.hpp"

namespace k6wp::launcher {

struct Options {
  bool want_studio = true;   // default + --studio
  bool show_help = false;
  bool minimized = false;       // passthrough marker for engine
  bool silent = false;          // ensure engine only, open nothing
  bool stop = false;            // --stop: uninstall helper (see RunStop)
  std::wstring engine_extra;  // forwarded engine flags (verbatim)
  bool elevate_lockscreen = false;  // --elevate-lockscreen off (legacy cleanup)
  std::wstring elevate_action;      // L"off" (L"on" was removed with the policy flow)
  bool elevated_worker = false;     // hidden --elevated (already elevated)
  // --apply-still: the per-user still helper (shared/still.hpp). It owns the
  // wallpaper + lock-screen work; the Engine and Studio only spawn it.
  bool apply_still = false;
  bool still_restore = false;    // --restore: put the originals back
  bool still_wallpaper = false;  // --wallpaper: limit to the wallpaper side
  bool still_lockscreen = false; // --lockscreen: limit to the lock-screen side
  std::wstring still_video;      // --video value captured for --apply-still
  std::wstring still_res;        // --res WxH ("" = the primary monitor)
  bool still_has_offset = false; // --offset-s override present
  double still_offset_sec = 0.0;
  // --delay-ms <n>: wait n ms before doing anything. Used by the engine's
  // hang watchdog: it spawns the launcher with a delay so the new engine
  // starts AFTER the hung one released the singleton mutex (starting it
  // while the mutex is still held would make the new instance bail out).
  int delay_ms = 0;
};

void PrintUsage(FILE* out);

// Parses launcher flags. Engine passthrough flags (--config/--video/
// --wallpaper-mode/--minimized) are forwarded verbatim, never reinterpreted.
// Returns 0 ok, 1 help requested, 2 bad flag.
int ParseArgs(int argc, wchar_t** argv, Options& out);

}  // namespace k6wp::launcher
