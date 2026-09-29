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

#ifndef K6WP_VERBOSE
#define K6WP_VERBOSE 0
#endif

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
  // Todo 11: mirror every line to engine.log (append, flushed; ODS fallback inside).
  AppendEngineLogLine(line);

  // Bench output (always emitted for observability, gated for stdout)
#if K6WP_VERBOSE
  std::fprintf(stdout, "%s\n", line);
  std::fflush(stdout);
#endif
}

}  // namespace k6wp
