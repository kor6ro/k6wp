// proc_util.cpp — central captured-subprocess helper (MED-4).
//
// Single CreateProcessW site for all probe/detect spawns. RAII handle
// wrapper; poll-drain wait loop (PeekNamedPipe + 100 ms WaitForSingleObject
// slices) so (a) a never-exiting child is killed exactly at timeout_ms and
// (b) a verbose child can never deadlock against a full pipe buffer — the
// old "read-to-EOF then WaitForSingleObject" order blocked in ReadFile
// forever on a hung child and never reached its timeout.

#include "proc_util.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <algorithm>
#include <vector>

namespace k6wp {
namespace {

// RAII wrapper for a Win32 HANDLE (CloseHandle on scope exit / reset).
// Move-only; default state is null (nothing to close).
class Handle {
 public:
  Handle() = default;
  explicit Handle(HANDLE h) : h_(h) {}
  ~Handle() { reset(); }

  Handle(const Handle&) = delete;
  Handle& operator=(const Handle&) = delete;

  Handle(Handle&& other) noexcept : h_(other.h_) { other.h_ = nullptr; }
  Handle& operator=(Handle&& other) noexcept {
    if (this != &other) {
      reset();
      h_ = other.h_;
      other.h_ = nullptr;
    }
    return *this;
  }

  HANDLE get() const { return h_; }
  void reset(HANDLE h = nullptr) {
    if (h_ != nullptr && h_ != INVALID_HANDLE_VALUE) {
      CloseHandle(h_);
    }
    h_ = h;
  }

 private:
  HANDLE h_ = nullptr;
};

// Drain any bytes already sitting in the pipe buffer without blocking.
// Returns false only when the pipe is broken (child gone, no more data).
bool DrainAvailable(HANDLE read_pipe, std::string& out) {
  DWORD avail = 0;
  if (!PeekNamedPipe(read_pipe, nullptr, 0, nullptr, &avail, nullptr)) {
    return false;
  }
  while (avail > 0) {
    char buf[4096];
    const DWORD want = (std::min)(avail, static_cast<DWORD>(sizeof(buf)));
    DWORD n = 0;
    if (!ReadFile(read_pipe, buf, want, &n, nullptr) || n == 0) {
      return false;
    }
    out.append(buf, n);
    avail -= n;
    if (n < want) break;
    DWORD more = 0;
    if (!PeekNamedPipe(read_pipe, nullptr, 0, nullptr, &more, nullptr)) {
      return false;
    }
    avail = more;
  }
  return true;
}

// After the child is gone (exited or killed), read to EOF: every write end
// is closed by now, so ReadFile returns the buffered remainder then FALSE
// (ERROR_BROKEN_PIPE) instead of blocking.
void DrainToEof(HANDLE read_pipe, std::string& out) {
  char buf[4096];
  DWORD n = 0;
  while (ReadFile(read_pipe, buf, sizeof(buf), &n, nullptr) && n > 0) {
    out.append(buf, n);
  }
}

}  // namespace

ProcResult RunCaptured(const std::filesystem::path& exe,
                       const std::wstring& cmdline,
                       unsigned long timeout_ms) {
  ProcResult r;
  try {
    if (exe.empty() || cmdline.empty()) return r;

    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    HANDLE raw_read = nullptr;
    HANDLE raw_write = nullptr;
    if (!CreatePipe(&raw_read, &raw_write, &sa, 0)) return r;
    Handle read_pipe(raw_read);
    Handle write_pipe(raw_write);

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = write_pipe.get();
    si.hStdError = write_pipe.get();
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);

    // CreateProcessW parses quotes with standard C rules (no cmd.exe
    // quote-stripping), so a quoted exe + quoted args in cmdline is safe.
    // lpApplicationName pins the exe (avoids PATH search games).
    std::vector<wchar_t> cmd_buf(cmdline.begin(), cmdline.end());
    cmd_buf.push_back(L'\0');
    PROCESS_INFORMATION pi{};
    const std::wstring exe_str = exe.wstring();
    if (!CreateProcessW(exe_str.c_str(), cmd_buf.data(), nullptr, nullptr,
                        TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
      return r;
    }
    Handle process(pi.hProcess);
    Handle thread(pi.hThread);
    thread.reset();  // we never touch the primary thread; close immediately

    // Parent drops its copy of the write end so ReadFile sees EOF once the
    // child (the only remaining writer) exits or is killed.
    write_pipe.reset();

    // Poll-drain loop: 100 ms wait slices, draining between slices.
    const ULONGLONG deadline = GetTickCount64() +
                               static_cast<ULONGLONG>(timeout_ms);
    bool exited = false;
    for (;;) {
      if (!DrainAvailable(read_pipe.get(), r.output)) {
        // Pipe broken: child likely already gone; confirm via a zero wait.
        if (WaitForSingleObject(process.get(), 0) == WAIT_OBJECT_0) {
          exited = true;
          break;
        }
        // Transient peek failure while the child lives on: keep waiting.
      }
      const ULONGLONG now = GetTickCount64();
      if (now >= deadline) break;
      ULONGLONG remain = deadline - now;
      if (remain > 100) remain = 100;
      const DWORD w =
          WaitForSingleObject(process.get(), static_cast<DWORD>(remain));
      if (w == WAIT_OBJECT_0) {
        exited = true;
        break;
      }
      if (w != WAIT_TIMEOUT) {
        // WAIT_FAILED / WAIT_ABANDONED: stop waiting; reap below without
        // claiming a timeout.
        break;
      }
    }

    if (!exited) {
      const bool expired = GetTickCount64() >= deadline;
      // Orphan-kill: a hung child must never outlive the wait (the old
      // encoder_detect RunCommand had no TerminateProcess here — a hung
      // ffmpeg lingered and the caller read exit code 259 STILL_ACTIVE).
      TerminateProcess(process.get(), 1);
      WaitForSingleObject(process.get(), 1000);
      r.timed_out = expired;
    }
    DrainToEof(read_pipe.get(), r.output);
    DWORD code = 1;
    if (!GetExitCodeProcess(process.get(), &code)) code = 1;
    r.exit_code = static_cast<unsigned long>(code);
    r.spawned = true;
    return r;
  } catch (...) {
    return r;
  }
}

}  // namespace k6wp
