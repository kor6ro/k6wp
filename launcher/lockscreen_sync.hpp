#pragma once

#include <string>

#include "cli.hpp"
#include "exit_codes.hpp"

namespace k6wp::launcher {

enum class ElevatedWaitOutcome {
  kRestored,      // the worker finished the policy restore
  kDeclined,      // ERROR_CANCELLED: the user dismissed the UAC prompt
  kTimedOut,      // the budget expired; the worker was terminated
  kLaunchFailed,  // ShellExecuteExW failed for a non-consent reason
  kWaitFailed,    // WaitForSingleObject itself failed
  kWorkerFailed,  // the worker ran and returned non-zero
};

ElevatedWaitOutcome ClassifyElevatedWait(bool launched, unsigned long launch_error,
                                        unsigned long wait_result,
                                        unsigned long exit_code);
int LockscreenExitCodeFor(ElevatedWaitOutcome outcome);

// What is left behind when the restore did not happen, and the one command that
// fixes it. Empty on success.
std::wstring DescribeLockscreenRestore(int exit_code);

int RunElevateLockscreen(const Options& opts);

}  // namespace k6wp::launcher
