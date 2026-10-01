#include "working_set_trim.hpp"

#include "timer_ids.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <psapi.h>

namespace k6wp {

void WorkingSetTrim::Arm(void* hwnd) {
  // One-shot only: never re-arm after the trim ran or while pending.
  if (done_ || armed_) return;
  if (hwnd == nullptr) {
    if (log_) log_("warning: working-set trim not armed (no message window)");
    return;
  }
  if (SetTimer(static_cast<HWND>(hwnd),
               static_cast<UINT_PTR>(kWorkingSetTrimTimerId), 2000, nullptr) ==
      0) {
    if (log_) {
      log_("warning: working-set trim SetTimer failed (error %lu), skipping trim",
           GetLastError());
    }
    return;
  }
  armed_ = true;
}

void WorkingSetTrim::RunOnce() {
  // Fires exactly once per process: mark done FIRST so every path below
  // (including failure) can never re-trim. Periodic trim would thrash
  // paged-in pages back out, so there is intentionally no re-arm here.
  if (done_) return;
  done_ = true;
  armed_ = false;
  PROCESS_MEMORY_COUNTERS_EX before{};
  before.cb = sizeof(before);
  SIZE_T ws_before_kb = 0;
  if (GetProcessMemoryInfo(GetCurrentProcess(),
                           reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&before),
                           sizeof(before))) {
    ws_before_kb = static_cast<SIZE_T>(before.WorkingSetSize / 1024);
  }
  if (!SetProcessWorkingSetSize(GetCurrentProcess(), (SIZE_T)-1,
                                (SIZE_T)-1)) {
    if (log_) {
      log_("warning: working-set trim failed (error %lu), continuing",
           GetLastError());
    }
    return;
  }
  PROCESS_MEMORY_COUNTERS_EX after{};
  after.cb = sizeof(after);
  if (GetProcessMemoryInfo(GetCurrentProcess(),
                           reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&after),
                           sizeof(after))) {
    const SIZE_T ws_after_kb =
        static_cast<SIZE_T>(after.WorkingSetSize / 1024);
    if (log_) {
      log_("engine: working-set trim ws=%llu KB -> %llu KB",
           static_cast<unsigned long long>(ws_before_kb),
           static_cast<unsigned long long>(ws_after_kb));
    }
  } else {
    if (log_) {
      log_("engine: working-set trim done (ws before=%llu KB)",
           static_cast<unsigned long long>(ws_before_kb));
    }
  }
}

}  // namespace k6wp
