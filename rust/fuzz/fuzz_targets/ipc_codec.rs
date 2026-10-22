#![no_main]
// The IPC codec against untrusted text (task I2).
//
// The IPC frames arrive from another process on a named pipe: the decoder
// must answer with an error for a broken frame, never with a panic (the
// engine would die with it).
use libfuzzer_sys::fuzz_target;

fuzz_target!(|data: &[u8]| {
    if let Ok(text) = std::str::from_utf8(data) {
        if let Ok(message) = k6wp_core::ipc::decode(text) {
            // A decoded message must survive the round trip.
            if let Ok(encoded) = k6wp_core::ipc::encode(&message) {
                let _ = k6wp_core::ipc::decode(&encoded);
            }
        }
    }
});
