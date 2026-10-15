// Unit tests for engine/src/retry_schedule pure core (display resilience P1,
// prompt v5.1 §5.5) driven by a FAKE clock — time is a parameter, so no
// steady_clock / Win32 is involved. Plain asserts with a pass/fail counter
// (tests/desktop_placement_test.cpp style). Exit code 0 = all pass.
//
// Locks the confirmed schedule 0/250/1000/3000 ms (§R2.2) and the
// due / next-delay / exhausted contract (§R7.2).
#include <iostream>
#include <string>
#include <vector>

#include "retry_schedule.hpp"

namespace {

int g_checks = 0;
int g_failures = 0;

void Check(bool cond, const std::string& name) {
  ++g_checks;
  if (cond) {
    std::cout << "[PASS] " << name << "\n";
  } else {
    ++g_failures;
    std::cout << "[FAIL] " << name << "\n";
  }
}

void TestOffsetsAndBudget() {
  Check(k6wp::kRetryCount == 4, "schedule: kRetryCount == 4");
  Check(k6wp::kRetryOffsetsMs[0] == 0 && k6wp::kRetryOffsetsMs[1] == 250 &&
            k6wp::kRetryOffsetsMs[2] == 1000 && k6wp::kRetryOffsetsMs[3] == 3000,
        "schedule: offsets are exactly 0/250/1000/3000");
  Check(k6wp::kRetryOffsetsMs[k6wp::kRetryCount - 1] <= 5000,
        "budget: last offset (3000) fits the final budget (5000)");
}

void TestDue() {
  k6wp::RetryClock c;
  c.first_pass_ms = 1000;
  c.attempts_done = 0;
  Check(k6wp::retry_schedule_due(1000, c),
        "due: attempt #0 is due at first_pass (offset 0)");
  Check(!k6wp::retry_schedule_due(999, c),
        "due: attempt #0 is not due before first_pass");

  c.attempts_done = 1;
  Check(!k6wp::retry_schedule_due(1000 + 249, c),
        "due: attempt #1 (250) not due at +249");
  Check(k6wp::retry_schedule_due(1000 + 250, c),
        "due: attempt #1 (250) due at +250");

  c.attempts_done = 2;
  Check(!k6wp::retry_schedule_due(1000 + 999, c),
        "due: attempt #2 (1000) not due at +999");
  Check(k6wp::retry_schedule_due(1000 + 1000, c),
        "due: attempt #2 (1000) due at +1000");

  c.attempts_done = 3;
  Check(k6wp::retry_schedule_due(1000 + 3000, c),
        "due: attempt #3 (3000) due at +3000");
}

void TestNextDelay() {
  k6wp::RetryClock c;
  c.first_pass_ms = 0;
  c.attempts_done = 0;
  Check(k6wp::retry_next_delay_ms(0, c) == 0,
        "next: 0 ms when attempt #0 is already due");
  c.attempts_done = 1;
  Check(k6wp::retry_next_delay_ms(0, c) == 250,
        "next: 250 ms at t=0 before the 250 offset");
  Check(k6wp::retry_next_delay_ms(100, c) == 150,
        "next: counts down remaining time to the offset");
  Check(k6wp::retry_next_delay_ms(250, c) == 0,
        "next: 0 at the exact offset");
  c.attempts_done = 2;
  Check(k6wp::retry_next_delay_ms(250, c) == 750,
        "next: 750 ms from 250 to the 1000 offset");
}

void TestNegativeElapsed() {
  // now_ms may precede first_pass (clock skew / caller ordering): never due,
  // and the next delay is the offset plus the elapsed shortfall.
  k6wp::RetryClock c;
  c.first_pass_ms = 1000;
  c.attempts_done = 1;
  Check(!k6wp::retry_schedule_due(500, c),
        "negative: not due when now_ms < first_pass");
  Check(k6wp::retry_next_delay_ms(500, c) == 750,
        "negative: next delay = offset + (first_pass - now)");
}

void TestExhausted() {
  k6wp::RetryClock c;
  c.first_pass_ms = 0;
  c.attempts_done = 0;
  Check(!k6wp::retry_schedule_exhausted(c),
        "exhausted: false before any attempt ran");
  c.attempts_done = 3;
  Check(!k6wp::retry_schedule_exhausted(c),
        "exhausted: false while attempts remain");
  c.attempts_done = k6wp::kRetryCount;
  Check(k6wp::retry_schedule_exhausted(c), "exhausted: true at kRetryCount");
  Check(!k6wp::retry_schedule_due(1000000000, c),
        "exhausted: never due after exhaustion");
  Check(k6wp::retry_next_delay_ms(1000000000, c) == -1,
        "exhausted: next delay is -1 after exhaustion");
}

void TestFullSequence() {
  k6wp::RetryClock c;
  c.first_pass_ms = 0;
  c.attempts_done = 0;
  long long now = 0;
  std::vector<long long> fired;
  int guard = 0;
  while (!k6wp::retry_schedule_exhausted(c) && guard++ < 16) {
    if (k6wp::retry_schedule_due(now, c)) {
      fired.push_back(now);
      ++c.attempts_done;
    } else {
      const long long delay = k6wp::retry_next_delay_ms(now, c);
      Check(delay > 0, "sequence: pending delay is positive");
      now += delay;
    }
  }
  Check(fired.size() == 4, "sequence: exactly four attempts fire");
  Check(fired == std::vector<long long>({0, 250, 1000, 3000}),
        "sequence: attempts fire at 0/250/1000/3000 ms");
  Check(k6wp::retry_schedule_exhausted(c),
        "sequence: schedule exhausted after the last attempt");
  Check(!fired.empty() && fired.back() <= 5000,
        "sequence: last attempt inside the budget");
}

}  // namespace

int main() {
  TestOffsetsAndBudget();
  TestDue();
  TestNextDelay();
  TestNegativeElapsed();
  TestExhausted();
  TestFullSequence();
  std::cout << "retry_schedule_test: " << g_checks << " checks, " << g_failures
            << " failures\n";
  return g_failures == 0 ? 0 : 1;
}
