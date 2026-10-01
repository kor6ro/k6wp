#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace k6wp::launcher {

// RAII guard for a Win32 HANDLE (mirrors shared's HandleGuard pattern).
class HandleGuard {
 public:
  explicit HandleGuard(HANDLE h) : handle_(h) {}
  HandleGuard(const HandleGuard&) = delete;
  HandleGuard& operator=(const HandleGuard&) = delete;
  HandleGuard(HandleGuard&& other) noexcept : handle_(other.handle_) {
    other.handle_ = nullptr;
  }
  HandleGuard& operator=(HandleGuard&& other) noexcept {
    if (this != &other) {
      if (handle_ != nullptr && handle_ != INVALID_HANDLE_VALUE) {
        CloseHandle(handle_);
      }
      handle_ = other.handle_;
      other.handle_ = nullptr;
    }
    return *this;
  }
  ~HandleGuard() {
    if (handle_ != nullptr && handle_ != INVALID_HANDLE_VALUE) {
      CloseHandle(handle_);
    }
  }
  HANDLE get() const { return handle_; }
  bool valid() const {
    return handle_ != nullptr && handle_ != INVALID_HANDLE_VALUE;
  }

 private:
  HANDLE handle_;
};

}  // namespace k6wp::launcher
