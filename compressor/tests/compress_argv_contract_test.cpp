// compress_argv_contract_test.cpp — golden Studio<->compressor argv contract
// (MED-15, audit-remediation todo 20).
//
// Builds the compressor argv from the single source of truth
// (k6wp::BuildCompressArgv in shared/compress_args.hpp — the same builder
// studio/src/compress_service.cpp::StartNext uses) and replays it against
// the REAL compressor.exe parser via --dry-run:
//
//   1. exact golden-vector match on the emitted argv (order + spelling);
//   2. every "--*" token is known to k6wp::IsKnownCompressorFlag;
//   3. compressor.exe --dry-run with that argv exits 0 and reports
//      "dry_run":true (parse accepted, cmdline built).
//
// Failure drill: adding a bogus flag emission to BuildCompressArgv (without
// a ParseCli update) must turn this test RED — step 1 mismatches and step 3
// is rejected by the parser.
//
// Step 4/5 cover the in-place-compress data-loss guard: `--in X --out X` used
// to parse cleanly, ffmpeg then ran with -y over the source, and
// ffmpeg_job.cpp's remove_partial() issued std::filesystem::remove(opts.out)
// on any cancel/timeout/failure — deleting the user's ORIGINAL video. Step 4
// replays the collision against the real binary and asserts EXIT 2 + the
// {"error":...} bad-option convention + byte-identical input survival; step 5
// unit-matrices PathsReferToSameFile over the lexical layers (case folding,
// "..", slash direction, relative-vs-absolute) and NTFS file identity
// (hardlink).
//
// Fixture notes: --in points at a real (empty) temp file with --force-long,
// so ParseCli skips the ffprobe duration probe (which would fail on an empty
// file) while the dry-run cmdline build still sees an existing input.
// Encoder "auto" mirrors the Studio default (exercises PickEncoder, hence
// the generous 120 s spawn budget).
//
// Exit 0 = contract holds; nonzero = contract broken.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "compress_args.hpp"
#include "proc_util.hpp"

#include "../src/cli.hpp"

namespace {

int g_checks = 0;
int g_failures = 0;

void Check(bool cond, const char* what) {
  ++g_checks;
  std::printf("%s %s\n", cond ? "PASS" : "FAIL", what);
  if (!cond) ++g_failures;
}

std::wstring WidenUtf8(const std::string& s) {
  if (s.empty()) return {};
  const int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(),
                                    static_cast<int>(s.size()), nullptr, 0);
  if (n <= 0) return {};
  std::wstring out(static_cast<std::size_t>(n), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, s.c_str(), static_cast<int>(s.size()),
                      out.data(), n);
  return out;
}

// Quote one argv token for RunCaptured (no cmd.exe involved; quoting only
// needs to survive CommandLineToArgvW rules — no embedded quotes allowed).
bool QuoteArg(const std::string& tok, std::wstring& quoted) {
  if (tok.find('"') != std::string::npos) return false;
  quoted = L"\"" + WidenUtf8(tok) + L"\"";
  return true;
}

// Spawn the real compressor.exe with `argv` (argv[0] implied, quoted per
// QuoteArg). `build_ok` is set false when a token cannot be quoted, in which
// case the returned ProcResult is meaningless. Shared by step 3 and step 4
// so both spawn the binary through one code path.
k6wp::ProcResult RunCompressor(const std::filesystem::path& exe,
                               const std::vector<std::string>& argv,
                               bool& build_ok) {
  std::wstring cmdline = L"\"" + exe.wstring() + L"\"";
  for (const std::string& tok : argv) {
    std::wstring q;
    if (!QuoteArg(tok, q)) {
      build_ok = false;
      return {};
    }
    cmdline += L" " + q;
  }
  return k6wp::RunCaptured(exe, cmdline, 120000);
}

// Whole-file bytes, for the data-loss assertion: the guard's real contract is
// that the input survives BYTE-IDENTICAL, not merely that it still exists.
std::string ReadAllBytes(const std::filesystem::path& p) {
  std::ifstream f(p, std::ios::binary);
  if (!f) return {};
  return std::string((std::istreambuf_iterator<char>(f)),
                     std::istreambuf_iterator<char>());
}

// Points the spawned compressor at the pinned vendored ffmpeg via
// K6WP_FFMPEG. Without this the test depends on shared/ffmpeg_path.cpp's
// directory guessing (exe_dir\..\..\vendor\ffmpeg), which holds in a dev tree
// but not on a clean CI checkout, and the "exits 0" assertions then fail for a
// reason that has nothing to do with the contract under test. Searches upward
// from the cwd so it works from any build dir inside the repo.
bool PinFfmpegForChild() {
  std::error_code ec;
  std::filesystem::path dir = std::filesystem::current_path(ec);
  if (ec) return false;
  for (int up = 0; up < 8 && !dir.empty(); ++up) {
    const std::filesystem::path candidate =
        dir / "vendor" / "ffmpeg" / "ffmpeg.exe";
    if (std::filesystem::exists(candidate, ec) && !ec) {
      SetEnvironmentVariableW(L"K6WP_FFMPEG", candidate.wstring().c_str());
      return true;
    }
    const std::filesystem::path parent = dir.parent_path();
    if (parent == dir) break;
    dir = parent;
  }
  std::printf(
      "     (no vendor/ffmpeg/ffmpeg.exe found walking up from %s - "
      "run tools\\fetch_vendor.ps1)\n",
      std::filesystem::current_path().string().c_str());
  return false;
}

}  // namespace

int main(int argc, char** argv) {
  // ---- fixture: temp dir + empty input file ----
  std::error_code ec;
  const std::filesystem::path tmp =
      std::filesystem::temp_directory_path(ec) / "k6wp_argv_contract";
  Check(!ec, "temp_directory_path resolves");
  if (ec) return 1;
  std::filesystem::create_directories(tmp, ec);
  Check(!ec, "fixture dir created");
  const std::filesystem::path in_file = tmp / "contract_in.mp4";
  const std::filesystem::path out_file = tmp / "contract_out.mp4";
  {
    std::ofstream touch(in_file, std::ios::binary | std::ios::trunc);
    Check(static_cast<bool>(touch), "fixture input file created");
  }
  std::filesystem::remove(out_file, ec);  // dry-run must not need it

  // ---- step 1: build argv from the shared helper, assert golden vector ----
  k6wp::CompressArgs args;
  args.in = in_file.u8string();
  args.out = out_file.u8string();
  args.res_w = 1280;
  args.res_h = 720;
  args.fps = 30;
  args.crf = 22;
  args.encoder = "auto";
  args.force_long = true;
  args.dry_run = true;

  const std::vector<std::string> built = k6wp::BuildCompressArgv(args);
  const std::vector<std::string> golden = {
      "--in", args.in, "--out", args.out, "--res", "1280x720",
      "--fps", "30", "--crf", "22", "--encoder", "auto",
      "--force-long", "--dry-run",
  };
  Check(built == golden, "BuildCompressArgv matches golden vector");

  // ---- step 2: every flag token is in the shared known-flag set ----
  bool all_known = true;
  for (const std::string& tok : built) {
    if (tok.rfind("--", 0) == 0 && !k6wp::IsKnownCompressorFlag(tok)) {
      std::printf("FAIL unknown contract flag: %s\n", tok.c_str());
      all_known = false;
    }
  }
  Check(all_known, "all emitted flags known to IsKnownCompressorFlag");

  // ---- step 3: replay against the real compressor.exe --dry-run ----
  std::filesystem::path self =
      (argc > 0) ? std::filesystem::path(argv[0]) : std::filesystem::path();
  const std::filesystem::path compressor =
      self.parent_path() / "compressor.exe";
  Check(std::filesystem::exists(compressor, ec) && !ec,
        "compressor.exe next to test exe");
  Check(PinFfmpegForChild(), "pinned ffmpeg located for the child process");
  if (g_failures > 0) {
    std::printf("RESULT: %d checks, %d failures (pre-spawn)\n", g_checks,
                g_failures);
    return 1;
  }

  std::wstring cmdline = L"\"" + compressor.wstring() + L"\"";
  bool quote_ok = true;
  for (const std::string& tok : built) {
    std::wstring q;
    if (!QuoteArg(tok, q)) {
      quote_ok = false;
      break;
    }
    cmdline += L" " + q;
  }
  Check(quote_ok, "argv quoted for spawn");
  if (!quote_ok) return 1;

  const k6wp::ProcResult rr =
      k6wp::RunCaptured(compressor, cmdline, 120000);
  Check(rr.spawned, "compressor.exe spawned");
  Check(!rr.timed_out, "dry-run did not time out");
  Check(rr.spawned && !rr.timed_out && rr.exit_code == 0,
        "dry-run parse accepted (exit 0)");
  Check(rr.output.find("\"dry_run\":true") != std::string::npos,
        "dry-run reports dry_run:true");

  // ---- step 4: same-path guard (in-place compress = data loss) ----
  // `--in X --out X` used to parse cleanly. ffmpeg then ran with -y straight
  // over the input, and ffmpeg_job.cpp's remove_partial() issued
  // std::filesystem::remove(opts.out) on any cancel/timeout/non-zero exit --
  // deleting the user's ORIGINAL video. ParseCli must now reject the
  // collision up front, before any filesystem or ffmpeg work.
  const std::filesystem::path same = tmp / "same_in.mp4";
  const std::string kPayload = "k6wp in-place guard fixture 0123456789";
  {
    std::ofstream f(same, std::ios::binary | std::ios::trunc);
    f << kPayload;
    Check(static_cast<bool>(f), "same-path fixture created");
  }
  Check(ReadAllBytes(same) == kPayload, "same-path fixture payload written");
  std::error_code same_ec;

  // --force-long skips the ffprobe duration check (which would reject a
  // non-video fixture for an unrelated reason and make this step pass for the
  // wrong cause); --dry-run keeps ffmpeg from actually encoding.
  bool build_ok = true;
  const k6wp::ProcResult same_rr = RunCompressor(
      compressor,
      {"--in", same.u8string(), "--out", same.u8string(), "--force-long",
       "--dry-run"},
      build_ok);
  Check(build_ok, "same-path argv built");
  Check(same_rr.spawned, "same-path invocation spawned");
  Check(!same_rr.timed_out, "same-path invocation did not time out");
  Check(same_rr.spawned && !same_rr.timed_out && same_rr.exit_code == 2,
        "same-path invocation rejected with exit 2");
  Check(same_rr.output.find("{\"error\"") != std::string::npos,
        "same-path rejection is a machine-parseable {\"error\":...}");
  Check(same_rr.output.find("\"dry_run\":true") == std::string::npos,
        "same-path rejection lands before the dry-run is accepted");
  Check(std::filesystem::exists(same, same_ec) && !same_ec,
        "same-path input still exists after rejection");
  Check(ReadAllBytes(same) == kPayload,
        "same-path input bytes unchanged after rejection");

  // Windows paths are case-insensitive, so a case-variant --out is the SAME
  // directory entry (removing one removes the other) and must be rejected too
  // -- a naive string compare misses this entirely.
  const std::filesystem::path upper = tmp / L"SAME_IN.MP4";
  build_ok = true;
  const k6wp::ProcResult upper_rr = RunCompressor(
      compressor,
      {"--in", same.u8string(), "--out", upper.u8string(), "--force-long",
       "--dry-run"},
      build_ok);
  Check(upper_rr.spawned && !upper_rr.timed_out && upper_rr.exit_code == 2,
        "case-variant output path rejected with exit 2");
  Check(ReadAllBytes(same) == kPayload,
        "case-variant rejection left input bytes unchanged");

  // The guard must not over-block: a sibling output path in the same
  // directory is still a legal invocation.
  const std::filesystem::path sibling = tmp / "same_out.mp4";
  build_ok = true;
  const k6wp::ProcResult sib_rr = RunCompressor(
      compressor,
      {"--in", same.u8string(), "--out", sibling.u8string(), "--force-long",
       "--dry-run"},
      build_ok);
  Check(sib_rr.spawned && !sib_rr.timed_out && sib_rr.exit_code == 0,
        "distinct sibling output path still accepted (exit 0)");

  // ---- step 5: PathsReferToSameFile unit matrix (the lexical layers the
  // end-to-end spawn above cannot reach) ----
  using k6wp::compressor::PathsReferToSameFile;
  Check(PathsReferToSameFile(std::filesystem::path(L"C:\\vids\\clip.mp4"),
                             std::filesystem::path(L"C:\\vids\\clip.mp4")),
        "same-file: identical spellings");
  Check(PathsReferToSameFile(std::filesystem::path(L"C:\\vids\\clip.mp4"),
                             std::filesystem::path(L"c:\\VIDS\\CLIP.MP4")),
        "same-file: case-folded (Windows is case-insensitive)");
  Check(PathsReferToSameFile(std::filesystem::path(L"C:\\vids\\sub\\..\\clip.mp4"),
                             std::filesystem::path(L"C:\\vids\\clip.mp4")),
        "same-file: '..' segment collapsed");
  Check(PathsReferToSameFile(std::filesystem::path(L"C:/vids/./clip.mp4"),
                             std::filesystem::path(L"C:\\vids\\clip.mp4")),
        "same-file: forward slashes and '.' folded to backslashes");
  Check(!PathsReferToSameFile(std::filesystem::path(L"C:\\vids\\"),
                              std::filesystem::path(L"C:\\vids\\other.mp4")),
        "not-same-file: sibling filename");
  Check(!PathsReferToSameFile(std::filesystem::path(),
                              std::filesystem::path(L"C:\\vids\\clip.mp4")),
        "not-same-file: empty side is never a collision");

  // A '..'-bearing spelling of the fixture, built from the fixture itself
      // so it exercises the collapse layer deterministically. It replaces an
      // earlier std::filesystem::relative() round-trip, which made the result
      // depend on the process cwd and failed on CI for that reason alone.
      {
        const std::filesystem::path abs = same;
        const std::filesystem::path spelled =
            abs.parent_path() / ".." / abs.parent_path().filename() /
            abs.filename();
        Check(PathsReferToSameFile(abs, spelled),
              "same-file: '..' round-trip spelling of a real file");
      }

  // Layer 2 (NTFS file identity): a hardlink is a different directory entry
  // pointing at the same file index, which no amount of string normalisation
  // can see. Layer 1 alone would let this through and ffmpeg would still
  // destroy the input.
  {
    const std::filesystem::path link = tmp / "same_in_link.mp4";
    std::error_code link_ec;
    std::filesystem::remove(link, link_ec);
    link_ec.clear();
    std::filesystem::create_hard_link(same, link, link_ec);
    Check(!link_ec, "hardlink fixture created");
    if (!link_ec) {
      Check(PathsReferToSameFile(same, link),
            "same-file: hardlink resolved to the input's file identity");
      std::filesystem::remove(link, same_ec);
    }
    Check(!std::filesystem::exists(link, same_ec), "hardlink fixture removed");
  }

  // A path that does not exist must be reported as "not the same file"
  // without throwing -- equivalent() fails there and that is the right answer.
  Check(!PathsReferToSameFile(same, tmp / "definitely_absent_9f2b.mp4"),
        "not-same-file: absent output does not throw and is not a collision");

  // ---- fixture cleanup (this step's own files only) ----
  std::error_code clean_ec;
  std::filesystem::remove(same, clean_ec);
  std::filesystem::remove(sibling, clean_ec);
  Check(!std::filesystem::exists(same, clean_ec) &&
            !std::filesystem::exists(sibling, clean_ec),
        "step-4 fixtures removed");

  std::printf("RESULT: %d checks, %d failures\n", g_checks, g_failures);
  return g_failures == 0 ? 0 : 1;
}
