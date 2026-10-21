// dpb_probe.cpp - the decoder's DPB size, via ffmpeg's trace_headers.
//
// The filter prints the parsed SPS fields, e.g.
//   [trace_headers @ ...] 47  max_num_ref_frames    000010001 = 16
//   [trace_headers @ ...] 191 max_dec_frame_buffering 000010001 = 16
// (H.264) and for HEVC:
//   sps_max_dec_pic_buffering_minus1  = 15
// The value after the "= " is the number.

#include "dpb_probe.hpp"

#include <cstdlib>
#include <string>

#include "ffmpeg_path.hpp"
#include "proc_util.hpp"

namespace k6wp::compressor {
namespace {

// One spawn budget. The filter only parses the headers (it copies the
// stream), so a few seconds is generous; a timeout kills the child (no
// orphan) and the probe reports "unknown".
//
// `-frames:v 1` bounds the output: without it the filter prints EVERY
// slice header (95 KB for one frame of a 4K clip, unbounded with the
// clip's length). The SPS is parsed before the first frame, so one frame
// is enough.
constexpr unsigned long kDpbProbeTimeoutMs = 15000;

// Extracts the number after the last "= " on a line that carries the given
// field name. Returns -1 when the field is not on the line.
int ParseFieldValue(const std::string& line, const std::string& field) {
  const size_t at = line.find(field);
  if (at == std::string::npos) return -1;
  const size_t eq = line.find('=', at + field.size());
  if (eq == std::string::npos) return -1;
  // The value is the number after the "= ".
  size_t i = eq + 1;
  while (i < line.size() && (line[i] == ' ' || line[i] == '\t')) ++i;
  if (i >= line.size() || line[i] < '0' || line[i] > '9') return -1;
  long value = 0;
  while (i < line.size() && line[i] >= '0' && line[i] <= '9') {
    value = value * 10 + (line[i] - '0');
    if (value > 100000) return -1;  // absurd: treat as not found
    ++i;
  }
  return static_cast<int>(value);
}

}  // namespace

int ProbeMaxRefFrames(const std::filesystem::path& in) {
  std::filesystem::path ffmpeg;
  try {
    ffmpeg = ::k6wp::FindFfmpeg();
  } catch (...) {
  }
  if (ffmpeg.empty()) return 0;

  const std::wstring cmdline =
      L"\"" + ffmpeg.wstring() + L"\" -hide_banner -v verbose -i \"" +
      in.wstring() + L"\" -c copy -bsf:v trace_headers -frames:v 1 -f null -";

  const ::k6wp::ProcResult rr =
      ::k6wp::RunCaptured(ffmpeg, cmdline, kDpbProbeTimeoutMs);
  if (!rr.spawned || rr.output.empty()) return 0;

  // H.264 first: max_num_ref_frames IS the DPB size.
  // max_dec_frame_buffering is the same value under a different name in
  // some ffmpeg versions, and it is a safe fallback.
  const char* const kH264Fields[] = {"max_num_ref_frames",
                                    "max_dec_frame_buffering"};
  // HEVC: the DPB is sps_max_dec_pic_buffering_minus1 + 1.
  const char* const kHevcFields[] = {"sps_max_dec_pic_buffering_minus1"};

  size_t pos = 0;
  while (pos < rr.output.size()) {
    size_t eol = rr.output.find('\n', pos);
    if (eol == std::string::npos) eol = rr.output.size();
    const std::string line = rr.output.substr(pos, eol - pos);
    pos = eol + 1;

    for (const char* field : kH264Fields) {
      const int value = ParseFieldValue(line, field);
      if (value >= 0) return value;
    }
    for (const char* field : kHevcFields) {
      const int value = ParseFieldValue(line, field);
      // HEVC stores it as "minus one".
      if (value >= 0) return value + 1;
    }
  }
  return 0;
}

}  // namespace k6wp::compressor
