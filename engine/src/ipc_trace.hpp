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
//   With the variable UNSET the instrumentation costs one static bool: no
//   timer, no branch and no log in the release build's hot path.

#include <algorithm>
#include <chrono>
#include <cstdarg>
#include <cstdint>
#include <cstdlib>
#include <map>
#include <mutex>
#include <string>

#include "log_file.hpp"

namespace k6wp {

// True when the trace is on. The environment is read ONCE (the engine's
// environment does not change while it runs).
inline bool IpcTraceEnabled() {
  static const bool enabled = [] {
    const char* value = std::getenv("K6WP_IPC_TRACE");
    return value != nullptr && value[0] != '\0' && value[0] != '0';
  }();
  return enabled;
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

// A scoped stopwatch that records its duration in the per-label accumulator.
class TraceScope {
 public:
  explicit TraceScope(const char* label) : label_(label) {
    if (!IpcTraceEnabled()) return;
    start_ = std::chrono::steady_clock::now();
  }
  ~TraceScope() {
    if (!IpcTraceEnabled()) return;
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
    if (total % kTraceSummaryEvery == 0) {
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
  std::chrono::steady_clock::time_point start_ = std::chrono::steady_clock::now();
};

}  // namespace k6wp
