#pragma once

// Pure retry schedule (design 02 §R7.2 / §R12.3; brief §4.6).
//
// windows.h-free; time is a PARAMETER (no clock calls inside the core). The
// confirmed schedule is 0/250/1000/3000 ms (§R2.2); the caller owns the real
// steady clock and passes now_ms + the RetryClock. Pattern: the pure/IO split
// in desktop_placement.hpp and pin_verify_schedule (there the clock lives in
// the .cpp wrapper; here it lives in the caller).

#include <cstdint>

namespace k6wp {

inline constexpr int kRetryCount = 4;
inline constexpr long long kRetryOffsetsMs[kRetryCount] = {0, 250, 1000, 3000};

struct RetryClock {
  long long first_pass_ms = 0;  // steady-clock ms at pass end (caller-supplied)
  int attempts_done = 0;        // due-attempts already executed (0..kRetryCount)
};

// Pure: true when attempt #(attempts_done) is due at now_ms.
bool retry_schedule_due(long long now_ms, const RetryClock& c);

// Pure: ms until the next due attempt (0 when already due; -1 when exhausted).
long long retry_next_delay_ms(long long now_ms, const RetryClock& c);

// True when every scheduled attempt has been executed.
bool retry_schedule_exhausted(const RetryClock& c);

}  // namespace k6wp
