#pragma once
// Compressor ffmpeg job — build argv, run with progress, cancel/timeout.
// This header is windows.h-free; only std types + filesystem.

#include <filesystem>
#include <string>

namespace k6wp::compressor {

struct FfmpegJobOptions {
  std::filesystem::path in;
  std::filesystem::path out;
  int res_w = 0;  // <=0 means keep source size (scale omitted)
  int res_h = 0;
  int fps = 30;
  int crf = 22;
  // Short label ("auto"|"nvenc"|"qsv"|"amf"|"x264") or full ffmpeg
  // encoder name ("h264_nvenc"|"h264_qsv"|"h264_amf"|"libx264").
  std::string encoder = "auto";
  double timeout_sec = 0.0;  // <=0 means no timeout
  // Lockframe mode: extract single frame as JPEG
  bool lockframe = false;
  double offset_sec = 0.0;  // seek offset in seconds
  int quality = 2;          // JPEG quality 1-31 (lower=better)
};

/// Resolve a short label (or "auto" via PickEncoder) to a full ffmpeg
/// encoder name. Full names pass through unchanged.
std::string ResolveEncoderName(const std::string& label_or_auto);

/// Build the full ffmpeg command line for a job — the same builder
/// RunFfmpegJob() uses, shared with the --dry-run path so the printed
/// argv is exactly what would run. Returns false and sets `error` when
/// ffmpeg is missing or the input file does not exist.
bool BuildFfmpegCmdline(const FfmpegJobOptions& opts, std::wstring& cmdline,
                        std::string& error);

/// Run the ffmpeg compression job:
///   -vf scale=W:H,fps=FPS -an -c:v <enc> + per-encoder quality flags,
///   -pix_fmt yuv420p -profile:v high, MP4 output, `-progress - -nostats`.
/// Lockframe mode (opts.lockframe): `-ss <offset> -frames:v 1 -q:v <quality>`
/// JPEG output, written via a .tmp file and atomically moved with MoveFileEx.
/// Emits progress NDJSON to stdout roughly every 500ms:
///   {"progress":0-100,"eta_s":N}
/// On success returns 0 (after emitting {"progress":100,"eta_s":0}).
/// On failure returns nonzero and sets `error`; a killed (cancel/timeout)
/// or failed job also deletes the partial output file.
/// Ctrl+C during the job kills ffmpeg and deletes the partial file.
int RunFfmpegJob(const FfmpegJobOptions& opts, std::string& error);

}  // namespace k6wp::compressor
