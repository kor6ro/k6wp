#include "cli.hpp"

#include "compress_args.hpp"
#include "ffmpeg_path.hpp"
#include "proc_util.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

namespace k6wp::compressor {
namespace {

// MED-15 argv contract: validation bounds/defaults live in
// shared/compress_args.hpp (single source of truth shared with the Studio
// builder). No local limit literals here.
constexpr int kMinCrf = k6wp::kCompressMinCrf;
constexpr int kMaxCrf = k6wp::kCompressMaxCrf;
constexpr int kMaxFps = k6wp::kCompressMaxFps;
constexpr double kMaxDurationSec = 600.0;  // 10 minutes

bool ParseInt(const std::string& s, int& out) {
  if (s.empty()) return false;
  char* end = nullptr;
  long v = std::strtol(s.c_str(), &end, 10);
  if (end == s.c_str() || *end != '\0') return false;
  out = static_cast<int>(v);
  return true;
}

bool ParseRes(const std::string& s, int& w, int& h) {
  const auto x = s.find('x');
  if (x == std::string::npos) return false;
  if (!ParseInt(s.substr(0, x), w) || !ParseInt(s.substr(x + 1), h)) return false;
  return w > 0 && h > 0;
}

bool IsValidEncoder(const std::string& e) {
  return e == "auto" || e == "nvenc" || e == "qsv" || e == "amf" || e == "x264";
}

std::wstring FindFfprobe() {
  try {
    const std::filesystem::path p = ::k6wp::FindFfprobe();
    if (p.empty()) return L"";
    return p.wstring();
  } catch (...) {
    return L"";
  }
}

// Returns duration in seconds, or -1.0 on failure (missing ffprobe, corrupt
// input, or non-numeric output).
double ProbeDuration(const std::filesystem::path& in) {
  const std::wstring ffprobe = FindFfprobe();
  if (ffprobe.empty()) return -1.0;

  // The child command line follows standard C quoting rules (no cmd.exe
  // quote-stripping), so quoting the exe + input path is safe. Spawn goes
  // through the central k6wp::RunCaptured helper (single spawn site, kProbeTimeoutMs budget, orphan-kill on timeout); the old inline
  // block waited INFINITE and could hang the CLI forever on a stuck ffprobe.
  const std::wstring cmdline =
      L"\"" + ffprobe +
      L"\" -v error -show_entries format=duration "
      L"-of default=noprint_wrappers=1:nokey=1 \"" +
      in.wstring() + L"\"";

  const k6wp::ProcResult rr = k6wp::RunCaptured(
      std::filesystem::path(ffprobe), cmdline, k6wp::kProbeTimeoutMs);
  if (!rr.spawned || rr.timed_out || rr.exit_code != 0) return -1.0;
  std::string out = rr.output;

  while (!out.empty() &&
         (out.back() == '\n' || out.back() == '\r' || out.back() == ' ')) {
    out.pop_back();
  }
  if (out.empty()) return -1.0;
  char* end = nullptr;
  const double d = std::strtod(out.c_str(), &end);
  if (end == out.c_str() || *end != '\0') return -1.0;
  return d;
}

// Layer 1 of the same-file comparison: the absolute path with the lexical
// "." / ".." noise removed, case-folded, so every spelling of one path folds
// to one string. Purely lexical -- no filesystem access, so it also holds for
// an output that does not exist yet and cannot throw.
//
// lexically_normal() is kept even though MSVC's absolute() already collapses
// ".." for an absolute input: the standard specifies absolute(p) == p when p is
// already absolute, so that collapsing is an implementation detail rather than
// a contract. lexically_normal() is the specified way to get it, and it also
// guarantees backslash separators ("the returned path has backslashes"), which
// is why no separate '/' -> '\' fold is needed here.
//
// Bytes >= 0x80 are left alone: under the "C" locale (the compressor never
// calls setlocale) std::tolower passes UTF-8 continuation bytes through
// unchanged, so non-ASCII filenames still compare correctly.
std::string FoldedAbsolute(const std::filesystem::path& p) {
  std::error_code ec;
  std::filesystem::path abs = std::filesystem::absolute(p, ec);
  if (ec) abs = p;  // no cwd available: compare the raw spelling
  std::string folded = abs.lexically_normal().u8string();
  std::transform(folded.begin(), folded.end(), folded.begin(), [](char c) {
    return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  });
  return folded;
}

}  // namespace

bool PathsReferToSameFile(const std::filesystem::path& a,
                          const std::filesystem::path& b) {
  if (a.empty() || b.empty()) return false;
  if (FoldedAbsolute(a) == FoldedAbsolute(b)) return true;
  // Layer 2: NTFS file identity (volume serial + file index). This is what
  // actually decides "same file" on Windows, and it closes the aliasing holes
  // lexical normalisation cannot see: symlinks, junctions, 8.3 short names
  // (CLIP~1.MP4) and hardlinks. The error_code overload reports false when
  // either side does not exist, which is the answer we want there anyway.
  std::error_code ec;
  return std::filesystem::equivalent(a, b, ec) && !ec;
}

const char* CliUsage() {
  return "Usage: compressor [options]\n"
         "\n"
         "Compress a video file with ffmpeg.\n"
         "\n"
         "Options:\n"
         "  --in <path>          Input video file\n"
         "  --out <path>         Output video file\n"
         "  --res <WxH>          Target resolution, e.g. 1920x1080\n"
         "  --fps <n>            Target frame rate (default 30, max 30)\n"
         "  --crf <n>            CRF quality 16-28 (default 22)\n"
         "  --encoder <name>     auto|nvenc|qsv|amf|x264 (default auto)\n"
         "  --force-long         Allow inputs longer than 10 minutes\n"
         "  --dry-run            Validate only; do not compress\n"
         "  --probe-encoders     List available encoders and exit\n"
         "  --help               Show this help and exit\n"
         "\n"
         "Lockframe mode (extract single frame as lockscreen image):\n"
         "  --lockframe          Enable lockframe mode (output is .jpg)\n"
         "  --offset-s <n>       Seek offset in seconds (default 0)\n"
         "  --res <WxH>          Target resolution (optional, scales frame)\n"
         "  --quality <n>        JPEG quality 1-31, lower=better (default 2)\n";
}

CliResult ParseCli(int argc, char** argv, CliOptions& out,
                   std::string& error) {
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    const auto next = [&](const char* name) -> const char* {
      if (i + 1 >= argc) {
        error = std::string("missing value for ") + name;
        return nullptr;
      }
      return argv[++i];
    };

    if (arg == "--help") {
      return CliResult::Help;
    } else if (arg == "--in") {
      const char* v = next("--in");
      if (!v) return CliResult::Error;
      // A '"' in a path would break out of the child command-line
      // quoting (CommandLineToArgvW rules) and inject arguments into the
      // ffmpeg/ffprobe child. '"' is not a valid Win32 filename character,
      // so reject-and-log instead of mangling the path.
      if (std::strchr(v, '"') != nullptr) {
        error = "invalid --in value: path contains a double quote";
        return CliResult::Error;
      }
      out.in = v;
    } else if (arg == "--out") {
      const char* v = next("--out");
      if (!v) return CliResult::Error;
      if (std::strchr(v, '"') != nullptr) {
        error = "invalid --out value: path contains a double quote";
        return CliResult::Error;
      }
      out.out = v;
    } else if (arg == "--res") {
      const char* v = next("--res");
      if (!v) return CliResult::Error;
      if (!ParseRes(v, out.res_w, out.res_h)) {
        error = std::string("invalid --res value '") + v +
                "' (expected WxH, e.g. 1920x1080)";
        return CliResult::Error;
      }
    } else if (arg == "--fps") {
      const char* v = next("--fps");
      if (!v) return CliResult::Error;
      if (!ParseInt(v, out.fps) || out.fps <= 0) {
        error = std::string("invalid --fps value '") + v + "'";
        return CliResult::Error;
      }
      if (out.fps > kMaxFps) {
        error = "fps must be <= 30";
        return CliResult::Error;
      }
    } else if (arg == "--crf") {
      const char* v = next("--crf");
      if (!v) return CliResult::Error;
      if (!ParseInt(v, out.crf)) {
        error = std::string("invalid --crf value '") + v + "'";
        return CliResult::Error;
      }
      if (out.crf < kMinCrf || out.crf > kMaxCrf) {
        error = "crf must be in range 16-28";
        return CliResult::Error;
      }
    } else if (arg == "--encoder") {
      const char* v = next("--encoder");
      if (!v) return CliResult::Error;
      out.encoder = v;
      if (!IsValidEncoder(out.encoder)) {
        error = std::string("unknown --encoder value '") + v +
                "' (expected auto|nvenc|qsv|amf|x264)";
        return CliResult::Error;
      }
    } else if (arg == "--force-long") {
      out.force_long = true;
    } else if (arg == "--dry-run") {
      out.dry_run = true;
    } else if (arg == "--probe-encoders") {
      out.probe_encoders = true;
    } else if (arg == "--lockframe") {
      out.lockframe = true;
    } else if (arg == "--offset-s") {
      const char* v = next("--offset-s");
      if (!v) return CliResult::Error;
      char* end = nullptr;
      double d = std::strtod(v, &end);
      if (end == v || *end != '\0' || d < 0.0) {
        error = std::string("invalid --offset-s value '") + v + "'";
        return CliResult::Error;
      }
      out.offset_sec = d;
    } else if (arg == "--quality") {
      const char* v = next("--quality");
      if (!v) return CliResult::Error;
      int q = 0;
      if (!ParseInt(v, q) || q < 1 || q > 31) {
        error = std::string("invalid --quality value '") + v +
                "' (expected 1-31)";
        return CliResult::Error;
      }
      out.lockframe_q = q;
    } else {
      // Unknown to this parser. Cross-check against the shared argv
      // contract: a flag known to shared/compress_args.hpp but unhandled
      // above means the contract moved without a parser update — reject
      // loudly (with a distinct message) so the golden dry-run CTest goes
      // RED instead of silently drifting from the Studio builder.
      if (k6wp::IsKnownCompressorFlag(arg)) {
        error = std::string("option '") + arg +
                "' is in the shared argv contract but not handled by this "
                "parser (parser fell behind shared/compress_args.hpp)";
      } else {
        error = std::string("unknown option '") + arg + "'";
      }
      return CliResult::Error;
    }
  }

  // --probe-encoders does not require --in/--out.
  if (!out.probe_encoders) {
    if (out.in.empty()) {
      error = "missing required option --in";
      return CliResult::Error;
    }
  }

  // In-place-compress guard, checked before any filesystem or ffprobe work so
  // a rejected invocation cannot touch the file it names. With --out naming
  // the input, ffmpeg ran with -y straight over the source, and
  // ffmpeg_job.cpp's remove_partial() then std::filesystem::remove()d that
  // same path on cancel/timeout/failure -- deleting the user's original.
  if (PathsReferToSameFile(out.in, out.out)) {
    error =
        "--out must differ from --in (refusing to compress a file onto itself)";
    return CliResult::Error;
  }

  // Duration check: only when the input file actually exists (a --dry-run
  // against a not-yet-created file must still pass validation).
  if (!out.force_long && std::filesystem::exists(out.in)) {
    const double dur = ProbeDuration(out.in);
    if (dur < 0.0) {
      error = "failed to read duration from input (ffprobe error or corrupt file)";
      return CliResult::Error;
    }
    if (dur > kMaxDurationSec) {
      error = "input is longer than 10 minutes; pass --force-long to override";
      return CliResult::Error;
    }
    if (out.lockframe && out.offset_sec > dur) {
      error = "offset-s exceeds video duration";
      return CliResult::Error;
    }
  }

  if (out.lockframe) {
    if (out.in.empty()) {
      error = "lockframe mode requires --in";
      return CliResult::Error;
    }
    if (!out.out.empty()) {
      const std::string out_ext = out.out.extension().string();
      if (out_ext != ".jpg" && out_ext != ".jpeg") {
        error = "lockframe mode output must be a .jpg file";
        return CliResult::Error;
      }
    }
  }

  return CliResult::Ok;
}

}  // namespace k6wp::compressor