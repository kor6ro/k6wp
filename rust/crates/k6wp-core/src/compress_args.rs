//! The Studio <-> compressor argv contract.
//!
//! The Rust mirror of the frozen C++ contract in
//! `shared/compress_args.hpp` / `shared/compress_args.cpp`: the single
//! source of truth for the `compressor.exe` command line.
//!
//! - the BUILDER side is the Studio's compress service, which must build
//!   its argument list from [`build_argv`] instead of hand-spelling
//!   "--in" / "--out" / ...;
//! - the PARSER side is the compressor CLI, which shares the flag spellings,
//!   the validation bounds and the defaults below, and rejects an unknown
//!   option through [`is_known_compressor_flag`] - so a builder-side flag
//!   addition without a parser update is rejected loudly.
//!
//! The emission order is part of the contract: do NOT change it without
//! bumping the golden vector in the compressor's argv contract test.

/// The input path flag.
pub const COMPRESS_FLAG_IN: &str = "--in";
/// The output path flag.
pub const COMPRESS_FLAG_OUT: &str = "--out";
/// The resolution flag.
pub const COMPRESS_FLAG_RES: &str = "--res";
/// The frame-rate flag.
pub const COMPRESS_FLAG_FPS: &str = "--fps";
/// The constant-quality factor flag.
pub const COMPRESS_FLAG_CRF: &str = "--crf";
/// The encoder flag.
pub const COMPRESS_FLAG_ENCODER: &str = "--encoder";
/// The "allow an input longer than 10 minutes" flag.
pub const COMPRESS_FLAG_FORCE_LONG: &str = "--force-long";
/// The "validate only" flag.
pub const COMPRESS_FLAG_DRY_RUN: &str = "--dry-run";
/// The "list the available encoders" flag.
pub const COMPRESS_FLAG_PROBE_ENCODERS: &str = "--probe-encoders";
/// The help flag.
pub const COMPRESS_FLAG_HELP: &str = "--help";
/// The lock-frame mode flag.
pub const COMPRESS_FLAG_LOCKFRAME: &str = "--lockframe";
/// The lock-frame offset flag.
pub const COMPRESS_FLAG_OFFSET_S: &str = "--offset-s";
/// The quality preset flag.
pub const COMPRESS_FLAG_QUALITY: &str = "--quality";

/// The smallest accepted CRF.
pub const COMPRESS_MIN_CRF: i32 = 16;
/// The largest accepted CRF.
pub const COMPRESS_MAX_CRF: i32 = 28;
/// The largest accepted frame rate.
pub const COMPRESS_MAX_FPS: i32 = 30;
/// The default frame rate.
pub const COMPRESS_DEFAULT_FPS: i32 = 30;
/// The default CRF.
pub const COMPRESS_DEFAULT_CRF: i32 = 22;
/// The default encoder.
pub const COMPRESS_DEFAULT_ENCODER: &str = "auto";

/// The Studio-side job parameters - the Rust mirror of the C++
/// `CompressArgs`.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct CompressArgs {
    /// The input file (the C++ field is `in`; `in` is a Rust keyword).
    pub in_: String,
    /// The output file.
    pub out: String,
    /// The target width (0 = keep the source width).
    pub res_w: i32,
    /// The target height (0 = keep the source height).
    pub res_h: i32,
    /// The target frame rate.
    pub fps: i32,
    /// The constant-quality factor.
    pub crf: i32,
    /// The encoder name.
    pub encoder: String,
    /// Allow an input longer than 10 minutes.
    pub force_long: bool,
    /// Validate only (the golden-test path).
    pub dry_run: bool,
}

impl Default for CompressArgs {
    fn default() -> Self {
        Self {
            in_: String::new(),
            out: String::new(),
            res_w: 0,
            res_h: 0,
            fps: COMPRESS_DEFAULT_FPS,
            crf: COMPRESS_DEFAULT_CRF,
            encoder: COMPRESS_DEFAULT_ENCODER.to_string(),
            force_long: false,
            dry_run: false,
        }
    }
}

/// Builds the compressor argv (without argv[0]) in the canonical order:
/// `--in` / `--out` / `--res WxH` / `--fps` / `--crf` / `--encoder`
/// `[+ --force-long]` `[+ --dry-run]`.
///
/// This is pure formatting - there is NO validation here (the parser owns
/// the rejection) and the resolution is formatted verbatim as `<w>x<h>`.
pub fn build_argv(args: &CompressArgs) -> Vec<String> {
    let mut argv: Vec<String> = Vec::with_capacity(14);
    argv.push(COMPRESS_FLAG_IN.to_string());
    argv.push(args.in_.clone());
    argv.push(COMPRESS_FLAG_OUT.to_string());
    argv.push(args.out.clone());
    argv.push(COMPRESS_FLAG_RES.to_string());
    argv.push(format!("{}x{}", args.res_w, args.res_h));
    argv.push(COMPRESS_FLAG_FPS.to_string());
    argv.push(args.fps.to_string());
    argv.push(COMPRESS_FLAG_CRF.to_string());
    argv.push(args.crf.to_string());
    argv.push(COMPRESS_FLAG_ENCODER.to_string());
    argv.push(args.encoder.clone());
    if args.force_long {
        argv.push(COMPRESS_FLAG_FORCE_LONG.to_string());
    }
    if args.dry_run {
        argv.push(COMPRESS_FLAG_DRY_RUN.to_string());
    }
    argv
}

/// True for every flag spelling the compressor CLI accepts (the Studio
/// subset plus `--probe-encoders` / `--help` and the lock-frame-mode
/// flags).
///
/// The parser's unknown-option branch consults this, so the error can
/// distinguish "a flag from the future" (the contract moved, the parser fell
/// behind) from a real typo.
pub fn is_known_compressor_flag(flag: &str) -> bool {
    matches!(
        flag,
        COMPRESS_FLAG_IN
            | COMPRESS_FLAG_OUT
            | COMPRESS_FLAG_RES
            | COMPRESS_FLAG_FPS
            | COMPRESS_FLAG_CRF
            | COMPRESS_FLAG_ENCODER
            | COMPRESS_FLAG_FORCE_LONG
            | COMPRESS_FLAG_DRY_RUN
            | COMPRESS_FLAG_PROBE_ENCODERS
            | COMPRESS_FLAG_HELP
            | COMPRESS_FLAG_LOCKFRAME
            | COMPRESS_FLAG_OFFSET_S
            | COMPRESS_FLAG_QUALITY
    )
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn the_argv_is_the_golden_vector() {
        let args = CompressArgs {
            in_: "C:\\in.mp4".to_string(),
            out: "C:\\out.mp4".to_string(),
            res_w: 1280,
            res_h: 720,
            fps: 30,
            crf: 22,
            encoder: "auto".to_string(),
            force_long: true,
            dry_run: true,
        };
        assert_eq!(
            build_argv(&args),
            vec![
                "--in",
                "C:\\in.mp4",
                "--out",
                "C:\\out.mp4",
                "--res",
                "1280x720",
                "--fps",
                "30",
                "--crf",
                "22",
                "--encoder",
                "auto",
                "--force-long",
                "--dry-run",
            ]
        );
    }

    #[test]
    fn the_optional_flags_are_omitted() {
        let args = CompressArgs {
            in_: "a".to_string(),
            out: "b".to_string(),
            ..CompressArgs::default()
        };
        let argv = build_argv(&args);
        // Six flags with their six values.
        assert_eq!(argv.len(), 12);
        assert!(!argv.iter().any(|t| t == "--force-long"));
        assert!(!argv.iter().any(|t| t == "--dry-run"));
        assert_eq!(argv[4], "--res");
        assert_eq!(argv[5], "0x0");
    }

    #[test]
    fn every_emitted_flag_is_known() {
        let args = CompressArgs {
            in_: "a".to_string(),
            out: "b".to_string(),
            force_long: true,
            dry_run: true,
            ..CompressArgs::default()
        };
        for token in build_argv(&args) {
            if token.starts_with("--") {
                assert!(is_known_compressor_flag(&token), "unknown flag: {token}");
            }
        }
        assert!(!is_known_compressor_flag("--nope"));
    }
}
