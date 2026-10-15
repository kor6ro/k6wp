// Pure retry schedule (design 02 §R7.2). No Win32, no clock, no logging: the
// time is a parameter so the suite drives a fake clock headless. <windows.h>
// is deliberately NOT included.

#include "retry_schedule.hpp"

namespace k6wp {

bool retry_schedule_due(long long now_ms, const RetryClock& c) {
  if (c.attempts_done < 0 || c.attempts_done >= kRetryCount) return false;
  return (now_ms - c.first_pass_ms) >= kRetryOffsetsMs[c.attempts_done];
}

long long retry_next_delay_ms(long long now_ms, const RetryClock& c) {
  if (c.attempts_done < 0 || c.attempts_done >= kRetryCount) return -1;
  const long long elapsed = now_ms - c.first_pass_ms;
  const long long delay = kRetryOffsetsMs[c.attempts_done] - elapsed;
  return delay > 0 ? delay : 0;
}

bool retry_schedule_exhausted(const RetryClock& c) {
  return c.attempts_done >= kRetryCount;
}

}  // namespace k6wp
