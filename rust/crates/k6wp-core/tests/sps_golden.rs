// The E3 golden test: the Rust SPS parser against ffmpeg's trace_headers.
//
// The ground truth is tests/golden/sps-dpb.json, produced by
// tools/extract_sps_truth.ps1 (which runs ffmpeg's own bitstream parser
// over the corpus). The corpus itself is gitignored - tools/build_corpus.ps1
// regenerates it - so a case whose file is missing is SKIPPED, loudly; if
// every case is skipped the test FAILS (a corpus that is not built would
// otherwise make the check look green).

use k6wp_core::sps::probe_dpb;
use std::path::PathBuf;

fn fixture() -> Vec<serde_json::Value> {
    let path = PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("../../../tests/golden/sps-dpb.json");
    let text = std::fs::read_to_string(&path)
        .unwrap_or_else(|e| panic!("the SPS fixtures must be readable: {e}"));
    let root: serde_json::Value = serde_json::from_str(&text).expect("valid JSON");
    root["cases"]
        .as_array()
        .cloned()
        .unwrap_or_else(|| panic!("the fixture must have a 'cases' array"))
}

#[test]
fn the_sps_parser_agrees_with_ffmpeg() {
    let corpus = PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("../../../tests/corpus");
    let cases = fixture();
    assert!(!cases.is_empty(), "the fixture must not be empty");

    let mut checked = 0usize;
    let mut skipped: Vec<String> = Vec::new();
    let mut failures: Vec<String> = Vec::new();

    for case in &cases {
        let name = case["file"].as_str().unwrap_or_default();
        let file = corpus.join(name);
        if !file.exists() {
            skipped.push(name.to_string());
            continue;
        }
        let expected = &case["expected"];
        match probe_dpb(&file) {
            Ok(report) => {
                // The DPB the decoder has to be able to hold.
                let expected_dpb = expected["max_num_ref_frames"]
                    .as_u64()
                    .or(expected["sps_max_dec_pic_buffering"].as_u64());
                if let Some(expected_dpb) = expected_dpb {
                    let got = report.max_dpb.unwrap_or(0) as u64;
                    if got != expected_dpb {
                        failures.push(format!(
                            "{name}: the DPB is {got}, ffmpeg's trace_headers says {expected_dpb}"
                        ));
                    }
                }
                // The VUI's own DPB field, when the codec has one.
                if let Some(expected_vui) = expected["max_dec_frame_buffering"].as_u64() {
                    let got = report
                        .sps
                        .iter()
                        .filter_map(|s| s.max_dec_frame_buffering)
                        .max()
                        .unwrap_or(0) as u64;
                    if got != expected_vui {
                        failures.push(format!(
                            "{name}: the VUI max_dec_frame_buffering is {got}, expected {expected_vui}"
                        ));
                    }
                }
                // The interlaced flag.
                if let Some(expected_interlaced) = expected["interlaced"].as_bool() {
                    let got = report
                        .sps
                        .iter()
                        .filter_map(|s| s.frame_mbs_only_flag)
                        .any(|mbs| mbs == 0);
                    if got != expected_interlaced {
                        failures.push(format!(
                            "{name}: interlaced is {got}, expected {expected_interlaced}"
                        ));
                    }
                }
                checked += 1;
            }
            Err(e) => failures.push(format!("{name}: the parser failed: {e}")),
        }
    }

    for line in &failures {
        eprintln!("[FAIL] {line}");
    }
    for name in &skipped {
        eprintln!("[SKIP] {name} is not in the corpus (run tools\\build_corpus.ps1)");
    }
    if checked == 0 {
        panic!(
            "no corpus file was available - the SPS check would be vacuous ({} skipped)",
            skipped.len()
        );
    }
    assert!(
        failures.is_empty(),
        "{} of {} SPS cases differ from ffmpeg",
        failures.len(),
        checked
    );
    eprintln!("sps: {checked} cases match ffmpeg's trace_headers");
}

/// The DPB byte budget (G1) - the rule the compressor uses to decide
/// between copying a file and re-encoding it.
///
/// The expected values are the measured ones (the DPB bytes and the
/// measured engine RAM), so a change in the parser or in the rule shows up
/// here.
#[test]
fn the_dpb_byte_budget_is_pinned() {
    // file -> (the DPB bytes, the copy decision)
    let expected: &[(&str, u64, bool)] = &[
        // 4 refs x 1920 x 1088 x 1.5 = 12.5 MB -> inside the budget.
        ("anime.mp4", 12_533_760, true),
        // 8 refs -> 25.1 MB -> inside.
        ("meddpb_1080p.mp4", 25_067_520, true),
        // 16 refs -> 50.1 MB -> inside (the 64 MB budget).
        ("highdpb_1080p.mp4", 50_135_040, true),
        // 16 refs at 4K -> 199.1 MB -> OVER the budget -> re-encoded.
        ("highdpb_4k.mp4", 199_065_600, false),
        // HEVC: 5 refs x 1920 x 1080 x 1.5 = 15.6 MB -> inside.
        ("hevc_1080p.mp4", 15_552_000, true),
        // The interlaced file: 4 refs -> 12.5 MB -> inside.
        ("interlaced.mp4", 12_533_760, true),
        // Two SPSs (4 and 16 refs): the LARGER one decides -> 50.1 MB.
        ("multi_sps.mp4", 50_135_040, true),
    ];
    let corpus = PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("../../../tests/corpus");
    let mut checked = 0usize;
    for (name, want_bytes, want_copy) in expected {
        let file = corpus.join(name);
        if !file.exists() {
            eprintln!("[SKIP] {name} is not in the corpus");
            continue;
        }
        let report = probe_dpb(&file).unwrap_or_else(|e| panic!("{name}: {e}"));
        let bytes = report
            .sps
            .iter()
            .filter_map(|s| s.dpb_bytes())
            .max()
            .unwrap_or(0);
        assert_eq!(bytes, *want_bytes, "{name}: the DPB bytes changed");
        let copy_safe = report.sps.iter().any(|s| s.is_copy_safe());
        assert_eq!(copy_safe, *want_copy, "{name}: the copy decision changed");
        checked += 1;
    }
    assert!(checked > 0, "no corpus file was available");
    eprintln!("sps: {checked} DPB budgets pinned");
}
