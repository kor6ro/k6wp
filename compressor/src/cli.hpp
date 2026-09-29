#pragma once
// Compressor CLI — argument parsing and validation.
// This header is windows.h-free; only std types + filesystem.

#include <filesystem>
#include <string>

#include "compress_args.hpp"

namespace k6wp::compressor {

struct CliOptions {
  std::filesystem::path in;
  std::filesystem::path out;
  int res_w = 0;
  int res_h = 0;
  int fps = k6wp::kCompressDefaultFps;
  int crf = k6wp::kCompressDefaultCrf;
  std::string encoder = k6wp::kCompressDefaultEncoder;
  bool force_long = false;
  bool dry_run = false;
  bool probe_encoders = false;
  // Lockframe mode: extract a single frame as lockscreen image
  bool lockframe = false;
  double offset_sec = 0.0;  // --offset-s N (seconds from start)
  int lockframe_q = 2;      // JPEG quality 1-31 (lower=better), default 2
};

enum class CliResult { Ok, Help, Error };

/// True when `a` and `b` denote the same file, i.e. writing output to `b`
/// would destroy `a`. ParseCli rejects such a pair up front, because
/// ffmpeg_job.cpp writes with -y and then removes the output path on
/// cancel/timeout/failure -- with a same-file output that removal takes the
/// user's original video with it.
///
/// Never throws, never touches the filesystem on the lexical path, and
/// returns false when either side is empty. Compares case-insensitively
/// (Windows paths are case-insensitive) after lexical normalisation, then
/// falls back to NTFS file identity when the input exists.
[[nodiscard]] bool PathsReferToSameFile(const std::filesystem::path& a,
                                        const std::filesystem::path& b);

/// Parse argc/argv into `out`. On Error, `error` carries a human-readable
/// message suitable for JSON emission. Returns Help when --help is passed.
CliResult ParseCli(int argc, char** argv, CliOptions& out,
                   std::string& error);

/// Return a usage string.
const char* CliUsage();

}  // namespace k6wp::compressor
