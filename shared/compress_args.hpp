#pragma once

// Studio<->compressor argv contract (MED-15, audit-remediation todo 20):
// single source of truth for the compressor.exe command line.
//
// - The BUILDER side is studio/src/compress_service.cpp::StartNext, which
//   must construct its QStringList from k6wp::BuildCompressArgv (converting
//   each std::string token) instead of hand-spelling "--in"/"--out"/...
// - The PARSER side is compressor/src/cli.cpp::ParseCli, which shares the
//   flag spellings (kCompressFlag*), the validation bounds
//   (kCompressMinCrf/kCompressMaxCrf/kCompressMaxFps) and the defaults below,
//   and routes its unknown-option rejection through
//   k6wp::IsKnownCompressorFlag so a builder-side flag addition without a
//   parser update is rejected loudly (and the golden dry-run CTest
//   `compress_argv_contract` goes RED).
//
// Header stays windows.h-free and Qt-free (std only); both consumers adapt
// at their boundary (QString conversion on the Studio side, char* parsing
// on the compressor side). Do NOT change the emission order without bumping
// the golden vector in compressor/tests/compress_argv_contract_test.cpp.

#include <string>
#include <vector>

namespace k6wp {

// ---- canonical flag spellings (exactly one literal per flag) ----
inline constexpr char kCompressFlagIn[] = "--in";
inline constexpr char kCompressFlagOut[] = "--out";
inline constexpr char kCompressFlagRes[] = "--res";
inline constexpr char kCompressFlagFps[] = "--fps";
inline constexpr char kCompressFlagCrf[] = "--crf";
inline constexpr char kCompressFlagEncoder[] = "--encoder";
inline constexpr char kCompressFlagForceLong[] = "--force-long";
inline constexpr char kCompressFlagDryRun[] = "--dry-run";
inline constexpr char kCompressFlagProbeEncoders[] = "--probe-encoders";
inline constexpr char kCompressFlagHelp[] = "--help";
inline constexpr char kCompressFlagLockframe[] = "--lockframe";
inline constexpr char kCompressFlagOffsetS[] = "--offset-s";
inline constexpr char kCompressFlagQuality[] = "--quality";

// ---- shared validation bounds (documented here, enforced by ParseCli) ----
inline constexpr int kCompressMinCrf = 16;
inline constexpr int kCompressMaxCrf = 28;
inline constexpr int kCompressMaxFps = 30;
inline constexpr int kCompressDefaultFps = 30;
inline constexpr int kCompressDefaultCrf = 22;
inline constexpr char kCompressDefaultEncoder[] = "auto";

// Studio-side job parameters (Qt-free mirror of
// studio/src/compress_service.hpp::CompressRequest).
struct CompressArgs {
  std::string in;
  std::string out;
  int res_w = 0;
  int res_h = 0;
  int fps = kCompressDefaultFps;
  int crf = kCompressDefaultCrf;
  std::string encoder = kCompressDefaultEncoder;
  bool force_long = false;  // --force-long: allow inputs longer than 10 min
  bool dry_run = false;     // --dry-run: validate only (golden-test path)
};

// Build the compressor argv (without argv[0]) in canonical order:
// --in/--out/--res WxH/--fps/--crf/--encoder [+ --force-long] [+ --dry-run].
// Pure formatting — no validation (the parser owns rejection); res is
// formatted verbatim as "<w>x<h>".
std::vector<std::string> BuildCompressArgv(const CompressArgs& args);

// True for every flag spelling ParseCli accepts (the Studio subset above
// plus --probe-encoders/--help and the lockframe-mode flags). The parser's
// unknown-option branch consults this so the error distinguishes "flag from
// the future" (contract moved, parser fell behind) from real typos.
bool IsKnownCompressorFlag(const std::string& flag);

}  // namespace k6wp
