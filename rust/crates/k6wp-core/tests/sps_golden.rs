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
