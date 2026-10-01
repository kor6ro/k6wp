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
  bool elevate_lockscreen = false;  // --elevate-lockscreen on|off
  std::wstring elevate_action;      // L"on" or L"off"
  bool elevated_worker = false;     // hidden --elevated (already elevated)
};

void PrintUsage(FILE* out);

// Parses launcher flags. Engine passthrough flags (--config/--video/
// --wallpaper-mode/--minimized) are forwarded verbatim, never reinterpreted.
// Returns 0 ok, 1 help requested, 2 bad flag.
int ParseArgs(int argc, wchar_t** argv, Options& out);

}  // namespace k6wp::launcher
