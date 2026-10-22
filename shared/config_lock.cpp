#include "config_lock.hpp"

#include <windows.h>

#include <string>

#include "build_flags.hpp"
#include "config_schema.hpp"

namespace k6wp::detail {

namespace {
constexpr wchar_t kLockName[] = L"Local\\K6WP-Config-Lock";
}  // namespace

ConfigFileLock::ConfigFileLock(std::chrono::milliseconds timeout) {
#if K6WP_ALLOW_ENV_OVERRIDES
  // The two-writer test runs the same scene with and without the lock, so it
  // can show that the lock is what prevents the lost update. DEBUG only.
  char flag[8] = {};
  if (GetEnvironmentVariableA("K6WP_TEST_NO_CONFIG_LOCK", flag,
                              sizeof(flag)) > 0 &&
      flag[0] == '1') {
    return;
  }
#endif

  HANDLE mutex = CreateMutexW(nullptr, FALSE, kLockName);
  if (mutex == nullptr) {
    throw ConfigError("config lock: CreateMutexW failed (err=" +
                      std::to_string(GetLastError()) + ")");
  }
  DWORD wait_ms = static_cast<DWORD>(timeout.count());
  if (timeout.count() < 0) {
    wait_ms = 0;
  }
  const DWORD state = WaitForSingleObject(mutex, wait_ms);
  if (state != WAIT_OBJECT_0 && state != WAIT_ABANDONED) {
    const DWORD err = GetLastError();
    CloseHandle(mutex);
    if (state == WAIT_TIMEOUT) {
      throw ConfigError("config lock timeout after " +
                        std::to_string(wait_ms) + " ms");
    }
    throw ConfigError("config lock: WaitForSingleObject failed (err=" +
                      std::to_string(err) + ")");
  }
  // WAIT_ABANDONED means the previous owner died while holding the lock: the
  // mutex is ours now and the file may hold a half-merged state, which the
  // caller re-reads from disk - the atomic publish means the file is either
  // the old bytes or the new bytes, never a mixture.
  handle_ = mutex;
  held_ = true;
}

ConfigFileLock::~ConfigFileLock() {
  if (!held_ || handle_ == nullptr) {
    return;
  }
  ReleaseMutex(static_cast<HANDLE>(handle_));
  CloseHandle(static_cast<HANDLE>(handle_));
}

}  // namespace k6wp::detail
