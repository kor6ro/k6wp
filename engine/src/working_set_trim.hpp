#pragma once

namespace k6wp {

// P4.1 one-shot working-set trim: arm a ~2 s timer after the first frame,
// fire once, never re-arm (periodic trim would thrash pages back out).
class WorkingSetTrim {
 public:
  using LogFn = void (*)(const char* fmt, ...);

  explicit WorkingSetTrim(LogFn log = nullptr) : log_(log) {}

  // Arms the trim timer on hwnd. No-op when already armed/done, or hwnd null.
  void Arm(void* hwnd);

  // Runs the trim exactly once (idempotent); clears the armed flag.
  void RunOnce();

  // Clears a pending arm (shutdown path).
  void Cancel() { armed_ = false; }

 private:
  LogFn log_;
  bool armed_ = false;
  bool done_ = false;
};

}  // namespace k6wp
