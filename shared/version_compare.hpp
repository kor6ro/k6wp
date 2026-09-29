#pragma once

#include <cctype>
#include <string>
#include <vector>

namespace k6wp {

// Small semver compare helper for the optional Studio update check.
// Header-only so the shared unit tests (config_test) and the Studio
// UpdateChecker use the exact same comparison without a new link unit.
//
// Comparison rules (deliberately small, no build-metadata handling):
//   - Leading/trailing ASCII whitespace and one leading 'v'/'V' are
//     stripped, so "v1.2.3" equals "1.2.3".
//   - Numeric components compare left to right; missing components are 0,
//     so "1.2" equals "1.2.0". Comparison is numeric, so "1.10.0" is
//     newer than "1.9.0".
//   - Each dot-separated component contributes its leading digit run
//     (clamped); trailing junk inside a component is ignored.
//   - Anything from the first '-' on is a prerelease marker: with equal
//     numeric cores the plain release sorts above the prerelease
//     ("1.0.0" is newer than "1.0.0-beta"); two prereleases with equal
//     cores compare equal.
//   - A version with no digits at all is invalid. Any comparison involving
//     an invalid version reports equal (never newer), so a corrupt tag can
//     never trigger the "new version" indicator. Nothing here throws.
struct ParsedSemver {
  std::vector<int> parts;
  bool prerelease = false;
  bool valid = false;
};

inline ParsedSemver ParseSemver(const std::string& text) {
  ParsedSemver out;
  size_t beg = 0;
  while (beg < text.size() &&
         std::isspace(static_cast<unsigned char>(text[beg])) != 0) {
    ++beg;
  }
  size_t end = text.size();
  while (end > beg &&
         std::isspace(static_cast<unsigned char>(text[end - 1])) != 0) {
    --end;
  }
  if (beg < end && (text[beg] == 'v' || text[beg] == 'V')) {
    ++beg;
  }
  // Split off the prerelease marker at the first '-'.
  size_t core_end = end;
  for (size_t i = beg; i < end; ++i) {
    if (text[i] == '-') {
      out.prerelease = true;
      core_end = i;
      break;
    }
  }
  // Parse dot-separated components; each non-empty one must start with a
  // digit, otherwise the whole version is invalid.
  size_t i = beg;
  while (i < core_end) {
    if (text[i] == '.') {
      ++i;  // Tolerate empty components ("1..2" parses as 1, 2).
      continue;
    }
    if (text[i] < '0' || text[i] > '9') {
      out.parts.clear();
      out.valid = false;
      return out;
    }
    long long acc = 0;
    while (i < core_end && text[i] >= '0' && text[i] <= '9') {
      const int digit = text[i] - '0';
      if (acc > (1000000000LL - digit) / 10) {
        acc = 1000000000;
        ++i;
        while (i < core_end && text[i] >= '0' && text[i] <= '9') {
          ++i;
        }
        break;
      }
      acc = acc * 10 + digit;
      ++i;
    }
    out.parts.push_back(acc);
    // Skip trailing junk inside the component up to the next dot.
    while (i < core_end && text[i] != '.') {
      ++i;
    }
  }
  out.valid = !out.parts.empty();
  if (!out.valid) {
    out.parts.clear();
  }
  return out;
}

inline int CompareSemver(const std::string& a, const std::string& b) {
  const ParsedSemver pa = ParseSemver(a);
  const ParsedSemver pb = ParseSemver(b);
  if (!pa.valid || !pb.valid) {
    return 0;
  }
  size_t n = pa.parts.size();
  if (pb.parts.size() > n) {
    n = pb.parts.size();
  }
  for (size_t k = 0; k < n; ++k) {
    const int va = k < pa.parts.size() ? pa.parts[k] : 0;
    const int vb = k < pb.parts.size() ? pb.parts[k] : 0;
    if (va < vb) {
      return -1;
    }
    if (va > vb) {
      return 1;
    }
  }
  if (pa.prerelease == pb.prerelease) {
    return 0;
  }
  return pa.prerelease ? -1 : 1;
}

inline bool IsNewerVersion(const std::string& latest,
                           const std::string& current) {
  return CompareSemver(latest, current) > 0;
}

}  // namespace k6wp
