// widen_utf8_test.cpp — QA harness for MED-10 (Widen UTF-8 decode).
//
// Includes the REAL compressor/src/ffmpeg_job.cpp so the anonymous-namespace
// Widen() under test is the actual production code, not a copy. External
// symbols the TU references (FindFfmpeg, PickEncoder) are stubbed below —
// they are never exercised by these assertions.
//
// Usage:
//   widen_utf8_test.exe happy    -> asserts the fixed Widen decodes UTF-8
//                                   correctly (non-ASCII path -> correct UTF-16)
//   widen_utf8_test.exe failure  -> demonstrates the OLD byte-wise Widen
//                                   produced wrong wchar for multibyte input
//
// Exit 0 = all assertions held; nonzero = an assertion failed.

#include "../src/ffmpeg_job.cpp"

#include <cstdio>
#include <cstring>

// ---- stubs for external symbols referenced by ffmpeg_job.cpp ----
namespace k6wp {
std::filesystem::path FindFfmpeg() { return {}; }
}  // namespace k6wp

namespace k6wp::compressor {
std::string PickEncoder() { return "libx264"; }
}  // namespace k6wp::compressor

// ---- the OLD byte-wise implementation (pre-MED-10), verbatim ----
static std::wstring WidenOld(const std::string& s) {
  return std::wstring(s.begin(), s.end());
}

static int g_failures = 0;

static void Check(bool ok, const char* what) {
  std::printf("%s %s\n", ok ? "PASS" : "FAIL", what);
  if (!ok) ++g_failures;
}

static void DumpWchars(const char* label, const std::wstring& ws) {
  std::printf("%s (%zu wchar):", label, ws.size());
  for (wchar_t c : ws) std::printf(" U+%04X", static_cast<unsigned>(c));
  std::printf("\n");
}

int main(int argc, char** argv) {
  const bool happy = (argc < 2) || std::strcmp(argv[1], "happy") == 0;

  // "C:\Users\José\vid.mp4" as explicit UTF-8 bytes (é = 0xC3 0xA9) so the
  // source encoding can never change the input.
  const std::string utf8_path = "C:\\Users\\Jos" "\xC3\xA9" "\\vid.mp4";
  const std::wstring expected = L"C:\\Users\\Jos\u00E9\\vid.mp4";

  if (happy) {
    std::printf("=== MED-10 happy: fixed Widen decodes UTF-8 to correct UTF-16 ===\n");
    std::printf("input UTF-8 bytes: C:\\Users\\Jos C3 A9 \\vid.mp4\n");

    const std::wstring got = k6wp::compressor::Widen(utf8_path);
    DumpWchars("k6wp::compressor::Widen(utf8_path)", got);

    Check(got == expected, "k6wp::compressor::Widen(utf8_path) == L\"C:\\Users\\Jos\\u00E9\\vid.mp4\"");
    Check(got.size() == expected.size(),
          "wchar count matches (single U+00E9, not two bytes)");
    Check(got.find(L'\u00E9') != std::wstring::npos, "contains U+00E9");
    Check(got.find(L'\u00C3') == std::wstring::npos,
          "no raw 0xC3 byte widened as wchar");
    Check(got.find(L'\u00A9') == std::wstring::npos,
          "no raw 0xA9 byte widened as wchar");
    Check(got.find(L'\uFFC3') == std::wstring::npos,
          "no sign-extended 0xC3 (U+FFC3)");
    Check(got.find(L'\uFFA9') == std::wstring::npos,
          "no sign-extended 0xA9 (U+FFA9)");

    // Regression: every current Widen() call-site passes ASCII-only flag
    // strings (encoder names, numeric quality flags, vf/tune/dpb/seek).
    const std::string ascii = "-crf 22 -preset veryfast";
    Check(k6wp::compressor::Widen(ascii) == L"-crf 22 -preset veryfast",
          "ASCII flag string still widens 1:1 (call-site regression)");
    Check(k6wp::compressor::Widen("") == L"", "empty string -> empty wstring");

    std::printf(g_failures == 0 ? "RESULT: ALL HAPPY CHECKS PASSED\n"
                                : "RESULT: %d HAPPY CHECK(S) FAILED\n",
                g_failures);
    return g_failures == 0 ? 0 : 1;
  }

  // failure mode: prove the OLD byte-wise widening was wrong for multibyte
  // UTF-8, i.e. the fix is meaningful.
  std::printf("=== MED-10 failure: OLD byte-wise Widen was wrong for multibyte UTF-8 ===\n");
  std::printf("input UTF-8 bytes: C:\\Users\\Jos C3 A9 \\vid.mp4\n");

  const std::wstring old_got = WidenOld(utf8_path);
  DumpWchars("WidenOld(utf8_path)", old_got);

  Check(old_got != expected, "old Widen != correct UTF-16 (it was broken)");
  Check(old_got.find(L'\uFFC3') != std::wstring::npos,
        "old Widen sign-extended 0xC3 into a standalone wchar U+FFC3");
  Check(old_got.find(L'\uFFA9') != std::wstring::npos,
        "old Widen sign-extended 0xA9 into a standalone wchar U+FFA9");
  Check(old_got.find(L'\u00E9') == std::wstring::npos,
        "old Widen never produced the real U+00E9");

  std::printf(g_failures == 0 ? "RESULT: OLD BEHAVIOR CONFIRMED BROKEN (fix justified)\n"
                              : "RESULT: %d FAILURE-MODE CHECK(S) FAILED\n",
              g_failures);
  return g_failures == 0 ? 0 : 1;
}