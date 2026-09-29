#pragma once
// Compressor encoder detection — list, 1-frame probe, priority pick.
// This header is windows.h-free; only std types.

#include <string>
#include <vector>

namespace k6wp::compressor {

struct EncoderInfo {
  std::string name;   // ffmpeg encoder name (e.g. "h264_nvenc")
  std::string label;  // short label (e.g. "nvenc")
  bool available;     // listed by ffmpeg -encoders
  bool works;         // passes 1-frame encode test
};

/// Run `ffmpeg -hide_banner -encoders` and parse the list of H.264 encoders.
/// Returns info for each candidate in priority order: NVENC, QSV, AMF, libx264.
std::vector<EncoderInfo> ListEncoders();

/// Test a single encoder with a 1-frame encode of a synthetic test source.
/// Returns true if ffmpeg exits 0.
bool Probe1Frame(const std::string& encoder_name);

/// Try encoders in priority order (NVENC→QSV→AMF→libx264).
/// Returns the first that passes Probe1Frame, or "libx264" as fallback.
std::string PickEncoder();

}  // namespace k6wp::compressor
