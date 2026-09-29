// Unit tests for shared/sha1.hpp (Todo 25, MED-3).
// No external test framework: plain asserts with a pass/fail counter,
// same style as tests/config_test.cpp. Exit code 0 = all pass.
#include <cstdio>
#include <string>

#include "sha1.hpp"

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

// --- FIPS 180-1 / RFC 3174 test vectors --------------------------------

void TestVectors() {
  // "abc" -> a9993e364706816aba3e25717850c26c9cd0d89d (RFC 3174 A.1).
  Check(k6wp::Sha1Hex("abc") == "a9993e364706816aba3e25717850c26c9cd0d89d",
        "SHA1(\"abc\") matches RFC 3174 vector");

  // Empty input -> da39a3ee5e6b4b0d3255bfef95601890afd80709 (RFC 3174 A.2).
  Check(k6wp::Sha1Hex("") == "da39a3ee5e6b4b0d3255bfef95601890afd80709",
        "SHA1(\"\") matches RFC 3174 empty-string vector");

  // 56-char message (fits one block) -> RFC 3174 A.3.
  Check(k6wp::Sha1Hex("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq") ==
            "84983e441c3bd26ebaae4aa1f95129e5e54670f1",
        "SHA1(56-char message) matches RFC 3174 vector");

  // 1000 x 'a' -> multi-block path (16 blocks) exercises the
  // block-boundary + length-padding logic.
  Check(k6wp::Sha1Hex(std::string(1000, 'a')) ==
            "291e9a6c66994949b57ba5e650361e98fc36b1ba",
        "SHA1(1000 x 'a') matches multi-block vector");

  // Determinism + lowercase hex shape.
  const std::string h = k6wp::Sha1Hex("abc");
  Check(h.size() == 40, "SHA1 output is 40 chars");
  Check(h.find_first_not_of("0123456789abcdef") == std::string::npos,
        "SHA1 output is lowercase hex");
}

}  // namespace

int main() {
  TestVectors();

  std::printf("%d checks, %d failures\n", g_checks, g_failures);
  return g_failures == 0 ? 0 : 1;
}