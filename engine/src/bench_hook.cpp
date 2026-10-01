#include "bench_hook.hpp"
#include "log_file.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <atomic>
#include <cstdio>

namespace k6wp {
namespace {

std::atomic<bool> g_first_frame_emitted{false};

}  // namespace

void MarkFirstFrame() {
  bool expected = false;
  if (!g_first_frame_emitted.compare_exchange_strong(expected, true)) {
    return;  // already emitted
  }

  LARGE_INTEGER ticks{};
  LARGE_INTEGER freq{};
  QueryPerformanceCounter(&ticks);
  QueryPerformanceFrequency(&freq);

  char line[256] = {};
  std::snprintf(line, sizeof(line), "Engine:FirstFrame qpc:%lld qpf:%lld",
                static_cast<long long>(ticks.QuadPart),
                static_cast<long long>(freq.QuadPart));
  // Todo 11: mirror every line to engine.log (flushed immediately - this is a
  // start-up gate marker, so it must survive a kill).
  AppendEngineLogLine(line, /*important=*/true);

  // Bench output: always emitted to stdout. tools/bench_startup.ps1 reads the
  // redirected stdout for this marker; the old K6WP_VERBOSE gate (default OFF)
  // meant the poll timed out and the script could report median 0 / pass. The
  // engine is a GUI-subsystem exe in production (no stdout), so this is a
  // harmless no-op there.
  std::fprintf(stdout, "%s\n", line);
  std::fflush(stdout);
}

}  // namespace k6wp
