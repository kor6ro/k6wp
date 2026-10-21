// dpb_probe.cpp - the decoder's DPB size, via ffmpeg's trace_headers.
//
// The reference parser is ffmpeg's own bitstream filter: the Rust
// `k6wp-core` parser is validated against exactly this output (the golden
// fixtures in tests/golden/sps-dpb.json), so the two must agree.
//
// The fields the trace prints (and the H.264 SPS syntax):
//   max_num_ref_frames                 the DPB size
//   max_dec_frame_buffering            the VUI's own DPB bound
//   sps_max_dec_pic_buffering_minus1   the HEVC DPB bound (-1)
//   pic_width_in_mbs_minus1            the width  = (x + 1) * 16
//   pic_height_in_map_units_minus1     the height = (y + 1) * 16 (x2 if MBAFF)
//   frame_mbs_only_flag                0 = interlaced (MBAFF)
//   level_idc                          reported, NOT a cap (measured)

#include "dpb_probe.hpp"

#include <algorithm>
#include <cstdlib>
#include <string>

#include "ffmpeg_path.hpp"
#include "proc_util.hpp"

namespace k6wp::compressor {
namespace {

constexpr unsigned long kDpbProbeTimeoutMs = 15000;

/// The value of a field on a trace_headers line ("... name  bits = value"),
/// or -1 when the field is not on this line.
int ParseFieldValue(const std::string& line, const std::string& field) {
  const size_t at = line.find(field);
  if (at == std::string::npos) return -1;
  size_t i = line.find('=', at + field.size());
  if (i == std::string::npos) return -1;
  ++i;
  while (i < line.size() && (line[i] == ' ' || line[i] == '\t')) ++i;
  if (i >= line.size() || line[i] < '0' || line[i] > '9') return -1;
  long value = 0;
  while (i < line.size() && line[i] >= '0' && line[i] <= '9') {
    value = value * 10 + (line[i] - '0');
    if (value > 1000000) return -1;  // absurd: treat as not found
    ++i;
  }
  return static_cast<int>(value);
}

}  // namespace

DpbInfo ProbeDpb(const std::filesystem::path& in) {
  DpbInfo info;
  std::filesystem::path ffmpeg;
  try {
    ffmpeg = ::k6wp::FindFfmpeg();
  } catch (...) {
  }
  if (ffmpeg.empty()) return info;

  const std::wstring cmdline =
      L"\"" + ffmpeg.wstring() + L"\" -hide_banner -v verbose -i \"" +
      in.wstring() + L"\" -c copy -bsf:v trace_headers -frames:v 1 -f null -";

  const ::k6wp::ProcResult rr =
      ::k6wp::RunCaptured(ffmpeg, cmdline, kDpbProbeTimeoutMs);
  if (!rr.spawned || rr.output.empty()) return info;

  int width_in_mbs = -1;
  int height_in_map_units = -1;
  int frame_mbs_only = 1;

  size_t pos = 0;
  while (pos < rr.output.size()) {
    size_t eol = rr.output.find('\n', pos);
    if (eol == std::string::npos) eol = rr.output.size();
    const std::string line = rr.output.substr(pos, eol - pos);
    pos = eol + 1;

    int value = ParseFieldValue(line, "max_num_ref_frames");
    if (value >= 0 && info.max_num_ref_frames == 0) {
      info.max_num_ref_frames = value;
      continue;
    }
    value = ParseFieldValue(line, "max_dec_frame_buffering");
    if (value >= 0 && info.vui_max_dec_frame_buffering == 0) {
      info.vui_max_dec_frame_buffering = value;
      continue;
    }
    value = ParseFieldValue(line, "sps_max_dec_pic_buffering_minus1");
    if (value >= 0 && info.hevc_max_dec_pic_buffering == 0) {
      // HEVC stores it as "minus one".
      info.hevc_max_dec_pic_buffering = value + 1;
      continue;
    }
    value = ParseFieldValue(line, "level_idc");
    if (value >= 0 && info.level_idc == 0) {
      info.level_idc = value;
      continue;
    }
    value = ParseFieldValue(line, "pic_width_in_mbs_minus1");
    if (value >= 0 && width_in_mbs < 0) {
      width_in_mbs = value;
      continue;
    }
    value = ParseFieldValue(line, "pic_height_in_map_units_minus1");
    if (value >= 0 && height_in_map_units < 0) {
      height_in_map_units = value;
      continue;
    }
    // The HEVC frame size is in luma samples, not in macroblocks.
    value = ParseFieldValue(line, "pic_width_in_luma_samples");
    if (value >= 0 && info.width == 0) {
      info.width = value;
      continue;
    }
    value = ParseFieldValue(line, "pic_height_in_luma_samples");
    if (value >= 0 && info.height == 0) {
      info.height = value;
      continue;
    }
    value = ParseFieldValue(line, "frame_mbs_only_flag");
    if (value >= 0 && frame_mbs_only < 0) {
      frame_mbs_only = value;
      continue;
    }
  }

  if (width_in_mbs >= 0) {
    info.width = (width_in_mbs + 1) * 16;
  }
  if (height_in_map_units >= 0) {
    const int units = height_in_map_units + 1;
    info.height = units * 16 * (frame_mbs_only == 0 ? 2 : 1);
  }

  // The reference frames the decoder actually holds: the smallest of
  // max_num_ref_frames and the parameter set's own DPB bound (the VUI's
  // max_dec_frame_buffering for H.264, sps_max_dec_pic_buffering for
  // HEVC). The LEVEL is deliberately not a cap (measured: a level-4.2 file
  // with 16 refs still allocates 16 frames).
  int refs = info.max_num_ref_frames;
  if (refs <= 0) refs = info.hevc_max_dec_pic_buffering;
  if (refs <= 0) refs = info.vui_max_dec_frame_buffering;
  const int bound = info.vui_max_dec_frame_buffering > 0
                        ? info.vui_max_dec_frame_buffering
                        : info.hevc_max_dec_pic_buffering;
  if (refs > 0 && bound > 0) refs = std::min(refs, bound);
  info.effective_refs = refs;

  if (refs > 0 && info.width > 0 && info.height > 0) {
    // YUV 4:2:0 -> 1.5 bytes per pixel.
    info.dpb_bytes = static_cast<std::uint64_t>(refs) *
                      static_cast<std::uint64_t>(info.width) *
                      static_cast<std::uint64_t>(info.height) * 3ull / 2ull;
  }
  info.ok = info.effective_refs > 0 && info.dpb_bytes > 0;
  return info;
}

}  // namespace k6wp::compressor
