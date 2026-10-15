// Pure retry schedule (design 02 §R7.2). No Win32, no clock, no logging: the
// time is a parameter so the suite drives a fake clock headless. <windows.h>
// is deliberately NOT included.

#include "retry_schedule.hpp"

namespace k6wp {

// RED (P1 red commit): behavior intentionally unimplemented so the P1 tests
// fail on their intended assertions. Implemented for real in the green commit.

bool retry_schedule_due(long long /*now_ms*/, const RetryClock& /*c*/) {
  return false;
}

long long retry_next_delay_ms(long long /*now_ms*/, const RetryClock& /*c*/) {
  return -1;
}

bool retry_schedule_exhausted(const RetryClock& /*c*/) { return true; }

}  // namespace k6wp
