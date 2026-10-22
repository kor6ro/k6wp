#![no_main]
// The SPS parser against untrusted bytes (task I2).
//
// The engine is built with `panic = "abort"`, so a panic here is a crash of
// the whole process. The parser must answer (an error, "unknown" or a
// value) for EVERY input - never panic, never hang, never allocate
// without bound.
use libfuzzer_sys::fuzz_target;

fuzz_target!(|data: &[u8]| {
    let report = k6wp_core::sps::probe_dpb_bytes(data);
    // The report is always coherent: a DPB needs a parameter set.
    if !report.needs_fallback {
        assert!(!report.sps.is_empty());
    }
});
