// Pure-logic coverage slice A (MED-2 todo 21): gpu_pin heuristic (incl. the
// documented RTX 2050 case), version_compare golden table (incl. MED-8
// overflow anchors), Sha1Hex "abc" anchor, compressor CacheKeyHex
// determinism.
//
// No external test framework: plain asserts with a pass/fail counter, same
// style as tests/config_test.cpp. Exit code 0 = all pass.
//
// The gpu_pin heuristic lives in an anonymous namespace inside
// engine/src/gpu_pin.cpp, so this TU #includes the REAL .cpp
// (thumbnailer_test-style single-TU pattern). Perturbing the heuristic
// (e.g. flipping the 0x10DE NVIDIA branch) turns the RTX 2050 checks RED.
// compressor/src/cache_manager.cpp is compiled as a separate source (it is
// windows.h-free) and linked against k6wp_shared for k6wp::Sha1Hex.
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

#include "../engine/src/gpu_pin.cpp"

#include "cache_manager.hpp"
#include "sha1.hpp"
#include "version_compare.hpp"

namespace {

int g_checks = 0;
int g_failures = 0;

void Check(bool cond, const std::string& name) {
  ++g_checks;
  if (cond) {
    std::printf("[PASS] %s\n", name.c_str());
  } else {
    ++g_failures;
    std::printf("[FAIL] %s\n", name.c_str());
  }
}

constexpr unsigned long long kMb = 1024ULL * 1024ULL;

// --- gpu_pin: VendorId table (gpu_pin.hpp documents the RTX 2050 case) -----
//
// Spike A2 verified on RTX 2050 + Intel UHD: the bare Shared>Dedicated rule
// misclassifies the RTX 2050 (dedicated 3962MB < shared 8035MB), so the
// table pins NVIDIA 0x10DE to discrete unconditionally.

void TestGpuPinRtx2050() {
  // The documented rig: RTX 2050, dedicated 3962MB, shared 8035MB.
  // Shared > dedicated here — the naive rule says "integrated", the table
  // must still say discrete.
  Check(k6wp::IsIntegrated(0x10DE, "NVIDIA GeForce RTX 2050", 3962ULL * kMb) ==
            false,
        "gpu_pin RTX_2050_is_discrete_not_integrated");
  Check(8035ULL > 3962ULL &&
            k6wp::IsIntegrated(0x10DE, "NVIDIA GeForce RTX 2050",
                               3962ULL * kMb) == false,
        "gpu_pin RTX_2050_SharedGtDedicated_pitfall_stays_discrete");
}

void TestGpuPinTable() {
  Check(k6wp::IsIntegrated(0x8086, "Intel(R) UHD Graphics", 128ULL * kMb),
        "gpu_pin Intel UHD is integrated");
  Check(k6wp::IsIntegrated(0x8086, "Intel(R) Arc(TM) Graphics", 2048ULL * kMb) ==
            false,
        "gpu_pin Intel Arc is discrete");
  Check(k6wp::IsIntegrated(0x8086, "Intel(R) ARC Graphics", 2048ULL * kMb) ==
            false,
        "gpu_pin Intel ARC uppercase is discrete");
  Check(k6wp::IsIntegrated(0x1002, "AMD Radeon(TM) Graphics",
                           512ULL * kMb - 1) == true,
        "gpu_pin AMD APU under 512MB is integrated");
  Check(k6wp::IsIntegrated(0x1002, "AMD Radeon(TM) Graphics", 512ULL * kMb) ==
            false,
        "gpu_pin AMD at exactly 512MB is discrete (boundary)");
  Check(k6wp::IsIntegrated(0x1002, "AMD Radeon RX 6600", 8192ULL * kMb) ==
            false,
        "gpu_pin AMD card is discrete");
  Check(k6wp::IsIntegrated(0x1234, "Unknown GPU 9000", 8192ULL * kMb) == false,
        "gpu_pin unknown vendor never claims integrated");
}

// --- version_compare golden table -------------------------------------------
// Canonical MED-8 overflow coverage lives in tests/config_test.cpp section
// 31; the two overflow anchors below pin the same vectors in this suite so
// the golden table is self-contained. The remaining rows are genuinely new
// (whitespace/V-prefix tolerance, prerelease-vs-prerelease, trailing junk,
// empty-component tolerance) — no overlap with section 31.

void TestSemverGolden() {
  Check(k6wp::CompareSemver("1.2", "1.2.0") == 0,
        "semver missing component equals zero");
  Check(k6wp::CompareSemver("  1.2.3  ", "v1.2.3") == 0,
        "semver whitespace and v-prefix stripped");
  Check(k6wp::CompareSemver("V2.0.0", "2.0.0") == 0,
        "semver uppercase V-prefix stripped");
  Check(k6wp::CompareSemver("1.0.0-alpha", "1.0.0-beta") == 0,
        "semver two prereleases with equal cores compare equal");
  Check(k6wp::IsNewerVersion("1.0.0", "1.0.0-rc1"),
        "semver release beats prerelease");
  Check(k6wp::CompareSemver("1.2.3junk", "1.2.3") == 0,
        "semver trailing junk inside component ignored");
  Check(k6wp::CompareSemver("1..2", "1.2.0") == 0,
        "semver empty component tolerated");
  Check(k6wp::CompareSemver("", "1.0.0") == 0,
        "semver empty latest never newer");
  Check(k6wp::CompareSemver("abc", "def") == 0,
        "semver two invalid versions compare equal");
  // MED-8 anchors (canonical coverage: config_test.cpp section 31).
  Check(k6wp::CompareSemver("1.4294967309", "1.13") > 0,
        "semver MED-8 long component clamps no int overflow");
  Check(k6wp::CompareSemver("1.2147483648", "1.0") > 0,
        "semver MED-8 INT_MAX+1 component clamps no wrap");
  const k6wp::ParsedSemver pv = k6wp::ParseSemver("1.99999999999999999999");
  Check(pv.valid && pv.parts.size() == 2 && pv.parts[1] == 1000000000,
        "semver ParseSemver clamps absurd component to 1e9");
}

// --- Sha1Hex "abc" anchor ----------------------------------------------------
// Canonical vectors live in tests/sha1_test.cpp (abc, empty, 56-char,
// 1000x'a'); this single anchor keeps the slice self-contained and pins the
// hash the cache keys build on.

void TestSha1Abc() {
  Check(k6wp::Sha1Hex("abc") == "a9993e364706816aba3e25717850c26c9cd0d89d",
        "sha1 abc matches RFC 3174 A.1");
  const std::string h = k6wp::Sha1Hex("abc");
  Check(h.size() == 40, "sha1 output is 40 chars");
  Check(h.find_first_not_of("0123456789abcdef") == std::string::npos,
        "sha1 output is lowercase hex");
}

// --- compressor CacheKeyHex determinism --------------------------------------

void TestCacheKeyHexDeterminism() {
  auto dir = std::filesystem::temp_directory_path() / "k6wp_gpu_pin_cachekey";
  std::error_code ec;
  std::filesystem::remove_all(dir, ec);
  std::filesystem::create_directories(dir, ec);
  const auto file = dir / "clip.mp4";
  {
    std::ofstream out(file, std::ios::binary | std::ios::trunc);
    out << "fake video bytes";
  }
  const std::string k1 =
      k6wp::compressor::CacheKeyHex(file, 1280, 720, 30, 23, "libx264");
  Check(k1.size() == 40, "cachekey returns 40 hex chars");
  Check(k1.find_first_not_of("0123456789abcdef") == std::string::npos,
        "cachekey is lowercase hex");
  Check(k6wp::compressor::CacheKeyHex(file, 1280, 720, 30, 23, "libx264") ==
            k1,
        "cachekey deterministic for same file and params");
  Check(k6wp::compressor::CacheKeyHex(file, 1280, 720, 30, 20, "libx264") !=
            k1,
        "cachekey changes when crf changes");
  Check(k6wp::compressor::CacheKeyHex(file, 1920, 1080, 30, 23, "libx264") !=
            k1,
        "cachekey changes when resolution changes");
  {
    std::ofstream out(file, std::ios::binary | std::ios::trunc);
    out << "fake video bytes, now longer";
  }
  Check(k6wp::compressor::CacheKeyHex(file, 1280, 720, 30, 23, "libx264") !=
            k1,
        "cachekey changes when file content changes");
  Check(k6wp::compressor::CacheKeyHex(dir / "k6wp_no_such_file_xyz.mp4", 1280,
                                      720, 30, 23, "libx264")
            .empty(),
        "cachekey empty for missing input");
  std::filesystem::remove_all(dir, ec);
}

}  // namespace

int main() {
  TestGpuPinRtx2050();
  TestGpuPinTable();
  TestSemverGolden();
  TestSha1Abc();
  TestCacheKeyHexDeterminism();

  std::printf("%d checks, %d failures\n", g_checks, g_failures);
  return g_failures == 0 ? 0 : 1;
}
