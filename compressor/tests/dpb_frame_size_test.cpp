// dpb_frame_size_test - the frame size of the DPB (task H2).
//
// WHY
//   The decoder holds 16-bit samples for a 10-bit file and 3 samples per
//   pixel for 4:4:4, so the frame size is NOT `pixels * 1.5` - that
//   constant was only the 8-bit 4:2:0 case, and it UNDER-counted a 10-bit
//   file by half. The copy-vs-re-encode decision hangs on this number, so
//   it is locked here.
//
//   The Rust side (`k6wp-core`'s `SpsInfo::frame_bytes`) has the same
//   table, and the two sides must never disagree: the C++ reads ffmpeg's
//   trace_headers, the Rust parses the bitstream, and both are checked
//   against the same truth (tests/golden/sps-dpb.json).
//
// WHAT IT ASSERTS
//   1. the five pixel formats (4:0:0, 4:2:0, 4:2:2, 4:4:4, and the
//      8-bit/10-bit pairs),
//   2. that an unknown size is 0 (never a guess),
//   3. that the 10-bit frame is exactly twice the 8-bit one.

#include "dpb_probe.hpp"

#include <cstdio>
#include <cstdint>

namespace {

int g_checks = 0;
int g_failures = 0;

void Check(bool ok, const char* what) {
  ++g_checks;
  if (ok) {
    std::printf("PASS %s\n", what);
  } else {
    ++g_failures;
    std::printf("FAIL %s\n", what);
  }
}

}  // namespace

int main() {
  using k6wp::compressor::FrameBytesFor;

  // 1920x1080, one pixel per sample.
  const std::uint64_t pixels = 1920ull * 1080ull;

  // 8-bit 4:2:0: 1.5 bytes per pixel.
  Check(FrameBytesFor(1920, 1080, 1, 8) == pixels * 3 / 2,
        "the 8-bit 4:2:0 frame is 1.5 bytes per pixel");
  // 10-bit 4:2:0: the samples live in 16-bit words, so it is TWICE.
  Check(FrameBytesFor(1920, 1080, 1, 10) == pixels * 3,
        "the 10-bit 4:2:0 frame is 3 bytes per pixel");
  // 4:2:2: 2 samples per pixel.
  Check(FrameBytesFor(1920, 1080, 2, 8) == pixels * 2,
        "the 8-bit 4:2:2 frame is 2 bytes per pixel");
  Check(FrameBytesFor(1920, 1080, 2, 10) == pixels * 4,
        "the 10-bit 4:2:2 frame is 4 bytes per pixel");
  // 4:4:4: 3 samples per pixel.
  Check(FrameBytesFor(1920, 1080, 3, 8) == pixels * 3,
        "the 8-bit 4:4:4 frame is 3 bytes per pixel");
  Check(FrameBytesFor(1920, 1080, 3, 10) == pixels * 6,
        "the 10-bit 4:4:4 frame is 6 bytes per pixel");
  // Monochrome: 1 sample per pixel.
  Check(FrameBytesFor(1920, 1080, 0, 8) == pixels,
        "the 8-bit monochrome frame is 1 byte per pixel");
  Check(FrameBytesFor(1920, 1080, 0, 10) == pixels * 2,
        "the 10-bit monochrome frame is 2 bytes per pixel");

  // The 10-bit frame is exactly twice the 8-bit one (the 4:2:0 case).
  Check(FrameBytesFor(1920, 1080, 1, 10) == FrameBytesFor(1920, 1080, 1, 8) * 2,
        "the 10-bit frame is exactly twice the 8-bit one");

  // An unknown size is 0, never a guess.
  Check(FrameBytesFor(0, 1080, 1, 8) == 0, "a width of 0 gives no estimate");
  Check(FrameBytesFor(1920, 0, 1, 8) == 0, "a height of 0 gives no estimate");
  Check(FrameBytesFor(-1, 1080, 1, 8) == 0, "a negative width gives no estimate");

  std::printf("checks=%d failures=%d\n", g_checks, g_failures);
  if (g_failures > 0) {
    std::printf("RESULT: THE DPB FRAME-SIZE CHECKS FAILED\n");
    return 1;
  }
  std::printf("RESULT: ALL THE DPB FRAME-SIZE CHECKS PASSED\n");
  return 0;
}
