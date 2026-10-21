//! The small semver compare the update check uses.
//!
//! The Rust mirror of the frozen C++ contract in
//! `shared/version_compare.hpp` (header-only in C++, so the shared unit
//! tests and the Studio update check use the exact same comparison).
//!
//! The rules, deliberately small and with no build-metadata handling:
//!
//! - the leading/trailing ASCII whitespace and ONE leading `v` / `V` are
//!   stripped, so "v1.2.3" equals "1.2.3";
//! - the numeric components compare left to right and a missing component is
//!   0, so "1.2" equals "1.2.0". The comparison is NUMERIC, so "1.10.0" is
//!   newer than "1.9.0";
//! - each dot-separated component contributes its leading digit run
//!   (clamped); the trailing junk inside a component is ignored;
//! - everything from the first `-` on is a prerelease marker: with equal
//!   numeric cores the plain release sorts above the prerelease ("1.0.0" is
//!   newer than "1.0.0-beta"), and two prereleases with equal cores compare
//!   equal;
//! - a version with no digits at all is INVALID, and any comparison that
//!   involves an invalid version reports EQUAL (never newer), so a corrupt
//!   tag can never trigger the "new version" indicator.
//!
//! Nothing here throws.

/// A parsed version - the Rust mirror of the C++ `ParsedSemver`.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct ParsedSemver {
    /// The numeric components.
    pub parts: Vec<i32>,
    /// Whether the version carries a prerelease marker.
    pub prerelease: bool,
    /// Whether the version could be parsed at all.
    pub valid: bool,
}

/// The C `isspace` set (the C++ uses `std::isspace`).
fn is_c_space(byte: u8) -> bool {
    matches!(byte, b' ' | b'\t' | b'\n' | 0x0b | 0x0c | b'\r')
}

/// Parses a version string - the C++ `ParseSemver`.
///
/// The comparison works on BYTES, exactly like the C++ (which works on a
/// `std::string`), so a non-ASCII byte makes the version invalid.
pub fn parse_semver(text: &str) -> ParsedSemver {
    let bytes = text.as_bytes();
    let mut out = ParsedSemver {
        parts: Vec::new(),
        prerelease: false,
        valid: false,
    };

    let mut beg = 0usize;
    while beg < bytes.len() && is_c_space(bytes[beg]) {
        beg += 1;
    }
    let mut end = bytes.len();
    while end > beg && is_c_space(bytes[end - 1]) {
        end -= 1;
    }
    if beg < end && (bytes[beg] == b'v' || bytes[beg] == b'V') {
        beg += 1;
    }

    // Split off the prerelease marker at the first '-'.
    let mut core_end = end;
    if let Some(offset) = bytes[beg..end].iter().position(|byte| *byte == b'-') {
        out.prerelease = true;
        core_end = beg + offset;
    }

    // Parse the dot-separated components; each non-empty one must start with
    // a digit, otherwise the whole version is invalid.
    let mut i = beg;
    while i < core_end {
        if bytes[i] == b'.' {
            i += 1; // Tolerate the empty components ("1..2" parses as 1, 2).
            continue;
        }
        if !bytes[i].is_ascii_digit() {
            out.parts.clear();
            out.valid = false;
            return out;
        }
        let mut acc: i64 = 0;
        while i < core_end && bytes[i].is_ascii_digit() {
            let digit = (bytes[i] - b'0') as i64;
            if acc > (1_000_000_000 - digit) / 10 {
                acc = 1_000_000_000;
                i += 1;
                while i < core_end && bytes[i].is_ascii_digit() {
                    i += 1;
                }
                break;
            }
            acc = acc * 10 + digit;
            i += 1;
        }
        out.parts.push(acc as i32);
        // Skip the trailing junk inside the component, up to the next dot.
        while i < core_end && bytes[i] != b'.' {
            i += 1;
        }
    }

    out.valid = !out.parts.is_empty();
    if !out.valid {
        out.parts.clear();
    }
    out
}

/// Compares two versions - the C++ `CompareSemver`.
///
/// `-1` when `a` is older, `0` when they are equal (or when one of them is
/// invalid) and `1` when `a` is newer.
pub fn compare_versions(a: &str, b: &str) -> i32 {
    let pa = parse_semver(a);
    let pb = parse_semver(b);
    if !pa.valid || !pb.valid {
        return 0;
    }
    let count = pa.parts.len().max(pb.parts.len());
    for index in 0..count {
        let va = pa.parts.get(index).copied().unwrap_or(0);
        let vb = pb.parts.get(index).copied().unwrap_or(0);
        if va < vb {
            return -1;
        }
        if va > vb {
            return 1;
        }
    }
    if pa.prerelease == pb.prerelease {
        return 0;
    }
    if pa.prerelease {
        -1
    } else {
        1
    }
}

/// True when `latest` is newer than `current` - the C++
/// `IsNewerVersion`.
pub fn is_newer_version(latest: &str, current: &str) -> bool {
    compare_versions(latest, current) > 0
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn the_numeric_parts_compare() {
        assert_eq!(compare_versions("1.2.3", "1.2.3"), 0);
        assert_eq!(compare_versions("1.10.0", "1.9.0"), 1);
        assert_eq!(compare_versions("1.9.0", "1.10.0"), -1);
        assert_eq!(compare_versions("1.2", "1.2.0"), 0);
        assert_eq!(compare_versions("2.0.0", "1.9.9"), 1);
    }

    #[test]
    fn the_whitespace_and_the_v_prefix_are_stripped() {
        assert_eq!(compare_versions("  v1.2.3 ", "1.2.3"), 0);
        assert_eq!(compare_versions("V1.2.3", "1.2.3"), 0);
    }

    #[test]
    fn the_prerelease_sorts_below_the_release() {
        assert_eq!(compare_versions("1.0.0", "1.0.0-beta"), 1);
        assert_eq!(compare_versions("1.0.0-beta", "1.0.0"), -1);
        assert_eq!(compare_versions("1.0.0-beta", "1.0.0-beta"), 0);
    }

    #[test]
    fn an_invalid_version_compares_equal() {
        assert_eq!(compare_versions("", "1.0.0"), 0);
        assert_eq!(compare_versions("abc", "1.0.0"), 0);
        assert_eq!(compare_versions("1.0.0", "abc"), 0);
        assert!(!is_newer_version("abc", "1.0.0"));
    }

    #[test]
    fn the_junk_inside_a_component_is_ignored() {
        assert_eq!(compare_versions("1.2.3beta", "1.2.3"), 0);
        assert_eq!(compare_versions("1..2", "1.2"), 0);
    }

    #[test]
    fn the_component_is_clamped() {
        let parsed = parse_semver("99999999999.0.0");
        assert!(parsed.valid);
        assert_eq!(parsed.parts[0], 1_000_000_000);
    }
}
