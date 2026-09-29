#pragma once

// Central subprocess helper (MED-4): the single CreateProcessW site for all
// probe/detect spawns. The four former per-file spawn blocks
// (compressor ProbeDuration / RunCommand / ProbeVideoProps, studio
// FfprobeHelper::Probe) now delegate here, so timeout, orphan-kill, and
// handle-lifetime policy live in exactly one place.
//
// Header stays Win32-free (same pattern as ffmpeg_path.hpp /
// encoder_detect.hpp); windows.h lives only in proc_util.cpp. exit_code is
// therefore `unsigned long` (= DWORD on Windows).

#include <filesystem>
#include <string>

namespace k6wp {

// Timeout budget for every probe/detect spawn (ffprobe metadata probes,
// `ffmpeg -encoders` list, 1-frame encoder probes). Single constant —
// call-sites must not carry their own magic millisecond literals.
constexpr unsigned long kProbeTimeoutMs = 10000;

// Budget reserved for heavy compress-path spawns only (real multi-second
// ffmpeg encodes, e.g. the encoder-detect 1-frame probes that must survive
// slow hardware-encoder init). Probe/detect paths use kProbeTimeoutMs; do
// not copy this value as a magic number elsewhere.
constexpr unsigned long kCompressTimeoutMs = 30000;

// Captured result of one RunCaptured spawn.
struct ProcResult {
  bool spawned = false;    // false: CreateProcessW failed (bad exe path...)
  bool timed_out = false;  // true: child killed via TerminateProcess after
                           // timeout_ms and reaped (no orphan, no leak)
  unsigned long exit_code = 1;
  std::string output;  // merged stdout+stderr bytes
};

// Spawns `exe` with `cmdline` (caller builds the full command line, quoting
// `exe` as argv[0]; `exe` is also passed as lpApplicationName so no cmd.exe
// quote-stripping is involved), captures merged stdout+stderr, and waits up
// to timeout_ms. On timeout the child is killed (TerminateProcess) and
// reaped, timed_out is set, and exit_code is the terminate code (1).
// Handles are RAII-managed (no leak on any path, including timeout); the
// pipe is drained incrementally while waiting so a verbose child can never
// deadlock against a full pipe buffer. Never throws. Windows-only.
[[nodiscard]] ProcResult RunCaptured(const std::filesystem::path& exe,
                                     const std::wstring& cmdline,
                                     unsigned long timeout_ms);

}  // namespace k6wp
