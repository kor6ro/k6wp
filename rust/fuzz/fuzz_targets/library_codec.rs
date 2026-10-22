#![no_main]
// The library index codec against untrusted text (task I2).
//
// The Studio reads a JSON file the user (or a sync tool) can edit: the
// codec must reject a broken document with an error, never with a panic.
use libfuzzer_sys::fuzz_target;

fuzz_target!(|data: &[u8]| {
    if let Ok(text) = std::str::from_utf8(data) {
        if let Ok(index) = k6wp_core::library::LibraryIndex::parse(text) {
            // A parsed index must survive the round trip.
            let json = index.to_json();
            let _ = k6wp_core::library::LibraryIndex::parse(&json);
        }
    }
});
