//! The G6 client tests that need NO engine.
//!
//! They cover the parts of the client that must hold even when the engine
//! is not running:
//!
//!   - the pipe name (the per-session name, the env override, the naming
//!     rule);
//!   - the TIMEOUT: a pipe instance with nobody on the other end must not
//!     hang the client - the read is bounded;
//!   - the payload limit: a message over 64 KiB is refused by the codec
//!     before it reaches the pipe;
//!   - the connect error: a pipe that does not exist fails fast, with a
//!     clear error (never a hang).
//!
//! The live-engine tests live in tests/live_engine.rs (they need the
//! engine, so they are opt-in).

use std::time::{Duration, Instant};

use k6wp_core::ipc::{encode, Cmd, IpcMessage};
use k6wp_ipc::{current_session_pipe_name, pipe_name_for_session, ConnectOptions, IpcClient};

#[test]
fn the_pipe_name_is_the_per_session_one() {
    // The name must never be a bare literal: it carries the session id.
    let name = current_session_pipe_name();
    assert!(
        name.starts_with(r"\\.\pipe\k6wp-engine"),
        "the pipe name must carry the base: {name}"
    );
    // The session id is the last component.
    let tail = name.rsplit('-').next().unwrap();
    assert!(
        tail.chars().all(|c| c.is_ascii_digit()),
        "the pipe name must end with the session id: {name}"
    );
    // And the explicit builder agrees with it.
    let session: u32 = tail.parse().unwrap();
    assert_eq!(pipe_name_for_session(session), name);
}

#[test]
fn the_env_override_wins() {
    // The override is read on EVERY call (never cached in a static: a static
    // is initialised before main() and the client and the server would
    // disagree about the name - that is what caused the 30-minute hang in
    // the C++ suite).
    let unique = r"\\.\pipe\k6wp-engine-test-override-42";
    std::env::set_var("K6WP_ENGINE_PIPE", unique);
    assert_eq!(current_session_pipe_name(), unique);
    std::env::remove_var("K6WP_ENGINE_PIPE");
    assert_ne!(current_session_pipe_name(), unique);
}

#[test]
fn a_missing_pipe_fails_fast() {
    let options = ConnectOptions {
        timeout: Duration::from_millis(500),
        ..ConnectOptions::default()
    };
    let started = Instant::now();
    let result = IpcClient::connect(r"\\.\pipe\k6wp-engine-does-not-exist-42", options);
    let elapsed = started.elapsed();
    assert!(result.is_err(), "connecting to a missing pipe must fail");
    assert!(
        elapsed < Duration::from_secs(5),
        "the connect must be bounded (it took {elapsed:?})"
    );
}

#[test]
fn a_payload_over_the_limit_is_refused_before_the_pipe() {
    // 64 KiB is the protocol's limit: the codec refuses it, so the client
    // never puts an oversized message on the wire.
    let huge = "x".repeat(k6wp_core::ipc::MAX_PAYLOAD_BYTES + 1024);
    let message = IpcMessage {
        version: k6wp_core::ipc::PROTOCOL_VERSION,
        cmd: Cmd::SetVideo,
        payload: serde_json::json!({ "video": huge }),
    };
    assert!(
        encode(&message).is_err(),
        "a payload over the limit must be refused"
    );
}

#[test]
fn a_silent_server_times_out_instead_of_hanging() {
    // A pipe instance with NO server on the other end: the client connects
    // (the instance exists), writes into the buffer and then waits for an
    // answer that never comes. The read must be bounded.
    use windows::core::PCWSTR;
    use windows::Win32::Foundation::HANDLE;
    use windows::Win32::Storage::FileSystem::FILE_FLAGS_AND_ATTRIBUTES;
    use windows::Win32::System::Pipes::{
        CreateNamedPipeW, NAMED_PIPE_MODE, PIPE_READMODE_MESSAGE, PIPE_TYPE_MESSAGE, PIPE_WAIT,
    };

    let name = format!(r"\\.\pipe\k6wp-engine-test-silent-{}", std::process::id());
    let wide: Vec<u16> = name.encode_utf16().chain(std::iter::once(0)).collect();
    // SAFETY: the name is null-terminated and the out-parameters are valid.
    let pipe = unsafe {
        CreateNamedPipeW(
            PCWSTR(wide.as_ptr()),
            // The open mode (PIPE_ACCESS_DUPLEX = 0x3) and the pipe mode.
            FILE_FLAGS_AND_ATTRIBUTES(0x0000_0003),
            NAMED_PIPE_MODE(PIPE_TYPE_MESSAGE.0 | PIPE_READMODE_MESSAGE.0 | PIPE_WAIT.0),
            1,
            65536,
            65536,
            0,
            None,
        )
    };
    assert_ne!(
        pipe,
        windows::Win32::Foundation::INVALID_HANDLE_VALUE,
        "the test pipe must be creatable"
    );

    let options = ConnectOptions {
        timeout: Duration::from_millis(800),
        // The verification is not the subject of this test; the pipe is the
        // test process itself (the same user, the same session), so the
        // default checks hold.
        ..ConnectOptions::default()
    };
    let mut client = IpcClient::connect(&name, options).expect("the pipe must open");
    let started = Instant::now();
    let result = client.request(&IpcMessage {
        version: k6wp_core::ipc::PROTOCOL_VERSION,
        cmd: Cmd::GetState,
        payload: serde_json::json!({}),
    });
    let elapsed = started.elapsed();
    assert!(
        result.is_err(),
        "a silent server must not produce an answer"
    );
    assert!(
        elapsed < Duration::from_secs(5),
        "the timeout must be bounded (it took {elapsed:?})"
    );
    // SAFETY: the handle is live and not used afterwards.
    unsafe {
        let _ = windows::Win32::Foundation::CloseHandle(pipe);
        let _ = HANDLE(std::ptr::null_mut());
    }
}
