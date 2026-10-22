#pragma once

// The IPC latency instrumentation (task K1).
//
// WHY
//   The engine's IPC thread serves the clients one after another. A slow
//   `get_state` blocks every other client, and the Studio then shows
//   "Disconnected". To fix that we first have to KNOW where the time goes -
//   a guess is not a fix.
//
// WHAT IT DOES
//   With `K6WP_IPC_TRACE=1` in the environment, every measured section
//   adds its duration to a per-label accumulator (the count, the total and
//   the maximum), and the engine logs a summary of the accumulators every
//   `kTraceSummaryEvery` samples.
//
//   WHY AN ACCUMULATOR AND NOT A LOG LINE PER SAMPLE: the log's write
//   latency (a flush on a slow disk) would land INSIDE the next
//   measurement - the instrument would measure itself. The accumulator has
//   no I/O at all.
//
//   The instrumentation is a DEBUG-ONLY facility (build_flags.hpp): in a
//   release build `IpcTraceEnabled()` is a constant false, so the compiler
//   removes the timers, the labels and the log calls - the shipped binary
//   does not even contain the variable name. In a debug build with the
//   variable unset it costs one static bool.
//
// CAVEAT (L1): the summary is logged from a scope's DESTRUCTOR, i.e. after its
// duration was taken but still inside every ENCLOSING section. A cascade of
// child summaries at the same boundary used to land inside `get_state.json`
// and `ipc.handler`, so a trace-on max was not usable.
//
// L2 FIX: only the OUTERMOST active scope logs (a thread-local depth counter).
// The flush therefore happens after the outermost measurement ended and no
// measured block encloses it - a trace-on MAX is usable now. The means still
// include the debug-build clock + map + mutex overhead, so quote means as
// upper bounds; the trace-OFF run remains the latency source of truth.

#include <algorithm>
#include <chrono>
#include <cstdarg>
#include <cstdint>
#include <cstdlib>
#include <map>
#include <mutex>
#include <string>

#include "build_flags.hpp"
#include "log_file.hpp"

namespace k6wp {

// True when the trace is on. The environment is read ONCE (the engine's
// environment does not change while it runs).
//
// DEBUG ONLY: `K6WP_IPC_TRACE` is an environment override like the pipe and
// mutex names, so it obeys the same rule (build_flags.hpp) - the release
// build does not contain the getenv, the string or the branch, and
// `check_release_overrides.ps1` can scan for the name.
inline bool IpcTraceEnabled() {
#if K6WP_ALLOW_ENV_OVERRIDES
  static const bool enabled = [] {
    const char* value = std::getenv("K6WP_IPC_TRACE");
    return value != nullptr && value[0] != '\0' && value[0] != '0';
  }();
  return enabled;
#else
  return false;
#endif
}

// The shared log sink (the same one every engine TU writes to).
inline void IpcTraceLog(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  // IMPORTANT: the summary must be visible while the engine runs (the
  // stress test reads it).
  EngineLogfV(fmt, args, true);
  va_end(args);
}

// The per-label accumulator.
struct TraceStat {
  std::uint64_t count = 0;
  std::uint64_t total_us = 0;
  std::uint64_t max_us = 0;
};

// The accumulator table (the IPC thread is the only writer, but the
// summary is logged from the same thread - the mutex is cheap and keeps it
// correct if the server ever grows a second thread).
inline std::map<std::string, TraceStat>& TraceStats() {
  static std::map<std::string, TraceStat> stats;
  return stats;
}

inline std::mutex& TraceStatsMutex() {
  static std::mutex mutex;
  return mutex;
}

// How many samples between two summaries.
inline constexpr std::uint64_t kTraceSummaryEvery = 200;

// The current thread's open-scope count (L2). Only the outermost scope logs;
// see the caveat at the top.
inline int& TraceDepth() {
  static thread_local int depth = 0;
  return depth;
}

// A scoped stopwatch that records its duration in the per-label accumulator.
class TraceScope {
 public:
  explicit TraceScope(const char* label) : label_(label) {
    if (!IpcTraceEnabled()) return;
    active_ = true;
    ++TraceDepth();
    start_ = std::chrono::steady_clock::now();
  }
  ~TraceScope() {
    if (!active_) return;
    const auto microseconds =
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - start_)
            .count();
    std::uint64_t total = 0;
    {
      std::lock_guard<std::mutex> lock(TraceStatsMutex());
      TraceStat& stat = TraceStats()[label_];
      stat.count += 1;
      stat.total_us += static_cast<std::uint64_t>(microseconds);
      stat.max_us = std::max(stat.max_us, static_cast<std::uint64_t>(microseconds));
      total += stat.count;
    }
    // L2: log only when the outermost scope exits, AFTER its duration was
    // taken. No measured block encloses the flush, so trace-on maxes are
    // usable (the child labels have already accumulated their samples).
    if (--TraceDepth() == 0 && total % kTraceSummaryEvery == 0) {
      std::lock_guard<std::mutex> lock(TraceStatsMutex());
      for (const auto& [label, stat] : TraceStats()) {
        const double mean = stat.count ? (static_cast<double>(stat.total_us) /
                                         static_cast<double>(stat.count))
                                       : 0.0;
        IpcTraceLog("ipc-trace: %s: n=%llu the mean=%lld us the max=%llu us",
                    label.c_str(), static_cast<unsigned long long>(stat.count),
                    static_cast<long long>(mean),
                    static_cast<unsigned long long>(stat.max_us));
      }
    }
  }
  TraceScope(const TraceScope&) = delete;
  TraceScope& operator=(const TraceScope&) = delete;

 private:
  const char* label_;
  bool active_ = false;
  std::chrono::steady_clock::time_point start_{};
};

}  // namespace k6wp
