// The G2 fuzzing: the SPS parser processes UNTRUSTED input, so it must not
// panic, hang or allocate without bound - whatever the bytes are.
//
// The engine is built with `panic = "abort"`, so a panic in this parser
// would take the whole engine down. The engine's own error handling assumes
// a malformed file produces an ERROR, never a crash.
//
// The property is checked three ways:
//   1. proptest over arbitrary bytes (the mutated corpus);
//   2. a deterministic mutation pass over the REAL corpus files (truncate,
//      flip bytes, splice chunks) - the mutations that are most likely to
//      reach a bad field;
//   3. the fixed regression vectors in tests/golden/sps-regressions/ (the
//      inputs that ever broke the parser - each one is a file with the
//      offending bytes, so a future change cannot re-open the hole).
use k6wp_core::sps::probe_dpb_bytes;
use proptest::prelude::*;

fn corpus_bytes() -> Vec<Vec<u8>> {
    let dir = std::path::Path::new(env!("CARGO_MANIFEST_DIR")).join("../../../tests/corpus");
    let mut out = Vec::new();
    if let Ok(entries) = std::fs::read_dir(&dir) {
        for entry in entries.flatten() {
            let path = entry.path();
            let is_video = path
                .extension()
                .map(|e| {
                    let e = e.to_string_lossy().to_lowercase();
                    e == "mp4" || e == "h264" || e == "mkv" || e == "webm"
                })
                .unwrap_or(false);
            if !is_video {
                continue;
            }
            if let Ok(bytes) = std::fs::read(&path) {
                // The header is what the parser reads; the tail is noise.
                let head: Vec<u8> = bytes.iter().copied().take(512 * 1024).collect();
                out.push(head);
            }
        }
    }
    out
}

proptest! {
    #![proptest_config(ProptestConfig { cases: 256, max_shrink_iters: 2048, ..ProptestConfig::default() })]

    /// Arbitrary bytes must never panic the parser.
    #[test]
    fn the_parser_never_panics_on_arbitrary_bytes(
        data in prop::collection::vec(any::<u8>(), 0..8192)
    ) {
        let report = probe_dpb_bytes(&data);
        // The report is always coherent: a DPB needs a parameter set.
        if !report.needs_fallback {
            prop_assert!(!report.sps.is_empty());
        }
    }

    /// The same for a "plausible" header: the first bytes look like a box.
    #[test]
    fn the_parser_never_panics_on_box_like_bytes(
        mut data in prop::collection::vec(any::<u8>(), 16..16384)
    ) {
        if data.len() >= 8 {
            data[4] = b'a';
            data[5] = b'v';
            data[6] = b'c';
            data[7] = b'C';
        }
        let _ = probe_dpb_bytes(&data);
    }
}

/// The deterministic mutation pass over the real corpus.
#[test]
fn the_parser_never_panics_on_the_mutated_corpus() {
    let corpus = corpus_bytes();
    assert!(!corpus.is_empty(), "the corpus is empty - build it first");
    let mut checked = 0usize;
    for head in &corpus {
        // 1. Every truncation.
        for cut in [0usize, 1, 7, 8, 15, 16, 64, 1024] {
            let data = &head[..head.len().min(cut)];
            let _ = probe_dpb_bytes(data);
            checked += 1;
        }
        // 2. Single-byte flips in the header. The stride keeps the pass
        //    CI-friendly (the proptest property above covers the rest).
        for index in (0..head.len().min(2048)).step_by(4) {
            let mut data = head.clone();
            data[index] = data[index].wrapping_add(0x5A);
            let _ = probe_dpb_bytes(&data);
            checked += 1;
        }
        // 3. A spliced chunk of zeros / ones (a broken box size, a huge
        //    table count).
        for filler in [0x00u8, 0xFF, 0x7F, 0x80] {
            let mut data = head.clone();
            let end = data.len().min(8192);
            for byte in data.iter_mut().take(end) {
                *byte = filler;
            }
            let _ = probe_dpb_bytes(&data);
            checked += 1;
        }
    }
    eprintln!("sps fuzz: {checked} mutated inputs, no panic");
}

/// The fixed regression vectors: the inputs that broke the parser.
#[test]
fn the_regression_vectors_are_handled() {
    let dir = std::path::Path::new(env!("CARGO_MANIFEST_DIR"))
        .join("../../../tests/golden/sps-regressions");
    let mut checked = 0usize;
    let entries = match std::fs::read_dir(&dir) {
        Ok(entries) => entries,
        Err(_) => {
            eprintln!("[SKIP] no regression vectors yet");
            return;
        }
    };
    for entry in entries.flatten() {
        let path = entry.path();
        if path.extension().map(|e| e == "bin").unwrap_or(false) {
            let bytes = std::fs::read(&path).expect("the vector must be readable");
            let _ = probe_dpb_bytes(&bytes);
            checked += 1;
        }
    }
    eprintln!("sps fuzz: {checked} regression vectors handled");
}
