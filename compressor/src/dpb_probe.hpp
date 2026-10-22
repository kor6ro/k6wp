#pragma once

// Task 1.4 (D3/G1): the decoder's DPB size.
//
// WHY IT MATTERS
// The decoder's surface pool scales with the DPB: it holds up to
// `max_num_ref_frames` reference frames, each `width x height x 1.5` bytes
// (YUV 4:2:0). Measured on this host:
//
//   1080p,  4 refs -> 12.5 MB of DPB -> the engine holds 163.3 MB
//   1080p,  8 refs -> 25.0 MB of DPB -> the engine holds 169.6 MB
//   1080p, 16 refs -> 50.0 MB of DPB -> the engine holds 195.2 MB
//    4K,   16 refs -> 199.1 MB of DPB -> the engine holds 387.2 MB
//    4K,    2 refs -> 24.9 MB of DPB -> the engine holds 228.2 MB
//
// (The 4K re-encode row is the DPB-pinned output: the same picture, the
// same CRF, the same resolution and FPS, only the reference list pinned.)
//
// So the threshold is a BYTE budget, not a reference count: the old "4 refs"
// rule re-encoded a 1080p clip whose decoder only holds 8-16 frames - a
// lossy re-encode for a few megabytes.
//
// THE LEVEL DOES NOT CAP THE ALLOCATION (measured, not assumed): the
// 1080p file with 16 refs declares level 4.2 (whose MaxDpbFrames is 8) and
// still costs exactly 8 reference frames more than the 8-ref file
// (195.16 MB vs 169.55 MB = +25.6 MB = 8 frames x 3.13 MB). The level is
// therefore REPORTED but not used as a cap.

#include <cstdint>
#include <filesystem>
#include <string>

namespace k6wp::compressor {

/// What the SPS says about the decoder's DPB.
struct DpbInfo {
  /// True when the parameter set was found and parsed.
  bool ok = false;
  /// The H.264 `max_num_ref_frames` (0 = unknown).
  int max_num_ref_frames = 0;
  /// The H.264 VUI `max_dec_frame_buffering` (0 = absent).
  int vui_max_dec_frame_buffering = 0;
  /// The HEVC `sps_max_dec_pic_buffering` (0 = absent).
  int hevc_max_dec_pic_buffering = 0;
  /// The H.264 `level_idc` (reported, not a cap - see the header).
  int level_idc = 0;
  /// The frame size in pixels (0 = unknown).
  int width = 0;
  int height = 0;
  /// The chroma format (0 = monochrome, 1 = 4:2:0, 2 = 4:2:2, 3 = 4:4:4).
  /// 0 means "absent" - the profile implies 4:2:0 (task H2).
  int chroma_format_idc = 0;
  /// The luma bit depth (8 when the field is absent - the baseline and the
  /// main profiles of H.264 do not carry it, task H2).
  int bit_depth_luma = 8;
  /// The reference frames the decoder actually holds: the smallest of
  /// `max_num_ref_frames` and the VUI's / HEVC's own DPB bound.
  int effective_refs = 0;
  /// The DPB's memory in bytes (0 = unknown).
  std::uint64_t dpb_bytes = 0;
};

/// The size of ONE decoded frame in bytes (0 = unknown).
///
/// The same formula as the Rust side (`k6wp-core`'s `SpsInfo::frame_bytes`),
/// so the two never disagree: the samples per pixel of the chroma format
/// times the bytes per sample of the bit depth.
///
///   | chroma            | samples/pixel | 8-bit | 10-bit |
///   | ----------------- | ------------: | ----: | -----: |
///   | 4:0:0 (monochrome) |           1 |     1 |      2 |
///   | 4:2:0              |         1.5 |   1.5 |      3 |
///   | 4:2:2              |           2 |     2 |      4 |
///   | 4:4:4              |           3 |     3 |      6 |
inline std::uint64_t FrameBytesFor(int width, int height, int chroma_format_idc,
                                   int bit_depth_luma) {
  if (width <= 0 || height <= 0) return 0;
  // The samples per pixel, in halves, so the arithmetic stays exact.
  std::uint64_t samples_x2 = 3;  // 4:2:0 - the default
  switch (chroma_format_idc) {
    case 0: samples_x2 = 2; break;  // monochrome
    case 2: samples_x2 = 4; break;  // 4:2:2
    case 3: samples_x2 = 6; break;  // 4:4:4
    default: samples_x2 = 3; break; // 4:2:0
  }
  // The bytes per sample: 1 for 8-bit, 2 for 9-16 bit (the samples live in
  // 16-bit words).
  const std::uint64_t bytes_per_sample = bit_depth_luma > 8 ? 2u : 1u;
  const std::uint64_t pixels =
      static_cast<std::uint64_t>(width) * static_cast<std::uint64_t>(height);
  return pixels * samples_x2 / 2u * bytes_per_sample;
}

/// The DPB budget: a file whose DPB costs more than this is re-encoded
/// (with the DPB pinned), so the decoder's pool stays bounded.
///
/// 64 MB: above the 1080p worst case (50 MB at 16 refs) and below the 4K
/// cases (100 MB at 8 refs, 199 MB at 16 refs) - so a 1080p clip is never
/// re-encoded for a few megabytes, while a 4K clip with a deep reference
/// list is.
inline constexpr std::uint64_t kMaxDpbBytesForCopy = 64ull * 1024 * 1024;

/// Reads the DPB information of a video file.
///
/// The source is ffmpeg's `trace_headers` bitstream filter (the same
/// reference the Rust `k6wp-core` parser is validated against).
/// Returns `ok = false` when the file cannot be read or holds no
/// parameter set this probe understands - the caller must then treat the
/// DPB as UNKNOWN (and fall back), never as "small".
DpbInfo ProbeDpb(const std::filesystem::path& in);

/// The DPB size the decoder holds, for a file whose parameter set cannot be
/// read: UNKNOWN (0). Never a guess.
inline constexpr int kUnknownDpbBytes = 0;

}  // namespace k6wp::compressor
