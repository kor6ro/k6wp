#![no_main]
// The MP4 box parser against untrusted bytes (task I2).
//
// The box walk is the part that reads the most attacker-controlled numbers
// (the box sizes, the table counts, the parameter-set arrays), so it is
// fuzzed on its own: it must never panic, hang or allocate without bound.
use libfuzzer_sys::fuzz_target;

fuzz_target!(|data: &[u8]| {
    let _ = k6wp_core::sps::container_parameter_sets(data);
});
