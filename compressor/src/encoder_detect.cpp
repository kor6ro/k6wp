#include "encoder_detect.hpp"

#include "ffmpeg_path.hpp"
#include "proc_util.hpp"

#include <cstdio>
#include <filesystem>
#include <string>

namespace k6wp::compressor {
namespace {

// ---- ffmpeg path resolution (centralized in k6wp_shared) ----

std::wstring FindFfmpeg() {
  try {
    const std::filesystem::path p = ::k6wp::FindFfmpeg();
    if (p.empty()) return L"";
    return p.wstring();
  } catch (...) {
    return L"";
  }
}

// ---- subprocess helper ----

struct RunResult {
  std::string output;
  unsigned long exit_code = 1;
};

/// Run a command via the central k6wp::RunCaptured helper (single spawn
/// site), capturing stdout+stderr.
/// Uses lpApplicationName for the exe (avoids cmd.exe quote-stripping).
/// Budget is kCompressTimeoutMs: these are real ffmpeg encode probes
/// (hardware-encoder init can take seconds), not cheap metadata reads.
/// Unlike the old inline block, a hung ffmpeg is killed on timeout instead
/// of lingering as an orphan while the caller reads STILL_ACTIVE (259).
RunResult RunCommand(const std::wstring& exe_path,
                     const std::wstring& cmdline) {
  RunResult r;
  const k6wp::ProcResult pr = k6wp::RunCaptured(std::filesystem::path(exe_path),
                                                cmdline,
                                                k6wp::kCompressTimeoutMs);
  if (!pr.spawned) return r;
  r.output = pr.output;
  r.exit_code = pr.exit_code;
  return r;
}

// ---- candidate encoders in priority order ----

struct EncoderCandidate {
  const char* name;  // ffmpeg encoder name
  const char* label; // short CLI label
};

constexpr EncoderCandidate kCandidates[] = {
    {"h264_nvenc", "nvenc"},
    {"h264_qsv",   "qsv"},
    {"h264_amf",   "amf"},
    {"libx264",    "x264"},
};

/// Check if a line from `ffmpeg -encoders` output contains the encoder name.
bool LineContainsEncoder(const std::string& line, const char* name) {
  // Encoder names appear as whole words in the output, e.g.:
  // " V..... h264_nvenc ..."
  // We do a simple substring search; encoder names are unique enough.
  return line.find(name) != std::string::npos;
}

}  // namespace

std::vector<EncoderInfo> ListEncoders() {
  const std::wstring ffmpeg = FindFfmpeg();
  if (ffmpeg.empty()) {
    // Return all candidates as unavailable.
    std::vector<EncoderInfo> result;
    for (const auto& c : kCandidates) {
      result.push_back({c.name, c.label, false, false});
    }
    return result;
  }

  const std::wstring cmdline =
      L"\"" + ffmpeg + L"\" -hide_banner -encoders";
  const RunResult rr = RunCommand(ffmpeg, cmdline);

  // Parse the output line by line.
  std::vector<EncoderInfo> result;
  for (const auto& c : kCandidates) {
    bool found = false;
    // Scan output for a line containing the encoder name.
    std::string remaining = rr.output;
    while (!remaining.empty()) {
      const auto nl = remaining.find('\n');
      const std::string line =
          (nl != std::string::npos) ? remaining.substr(0, nl) : remaining;
      if (LineContainsEncoder(line, c.name)) {
        found = true;
        break;
      }
      if (nl == std::string::npos) break;
      remaining = remaining.substr(nl + 1);
    }
    result.push_back({c.name, c.label, found, false});
  }
  return result;
}

bool Probe1Frame(const std::string& encoder_name) {
  const std::wstring ffmpeg = FindFfmpeg();
  if (ffmpeg.empty()) return false;

  // Build the 1-frame encode command.
  // testsrc generates a synthetic pattern; -frames:v 1 limits to one frame;
  // -f null discards the output. Exit 0 = encoder works.
  // Size 320x240: NVENC rejects frames below its minimum dimension (64x64
  // fails with "Frame Dimension less than the minimum supported value").
  const std::wstring wenc(encoder_name.begin(), encoder_name.end());
  const std::wstring cmdline =
      L"\"" + ffmpeg +
      L"\" -hide_banner -y -f lavfi "
      L"-i testsrc=duration=0.1:size=320x240:rate=1 "
      L"-frames:v 1 -c:v " +
      wenc +
      L" -f null -";

  const RunResult rr = RunCommand(ffmpeg, cmdline);
  return rr.exit_code == 0;
}

std::string PickEncoder() {
  for (const auto& c : kCandidates) {
    if (Probe1Frame(c.name)) {
      return c.name;
    }
  }
  // All HW encoders failed — fall back to libx264.
  std::fprintf(stderr,
               "warning: all hardware encoders failed 1-frame probe; "
               "falling back to libx264\n");
  return "libx264";
}

}  // namespace k6wp::compressor
