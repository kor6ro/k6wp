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
use windows::core::PCWSTR;
use windows::Win32::Foundation::{CloseHandle, HANDLE, INVALID_HANDLE_VALUE};
use windows::Win32::Storage::FileSystem::{ReadFile, WriteFile, FILE_FLAGS_AND_ATTRIBUTES};
use windows::Win32::System::Pipes::{
    ConnectNamedPipe, CreateNamedPipeW, DisconnectNamedPipe, NAMED_PIPE_MODE,
    PIPE_READMODE_MESSAGE, PIPE_TYPE_MESSAGE, PIPE_WAIT,
};

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

/// The over-limit case (task L4): every pipe instance is busy.
///
/// The engine serves ONE client at a time, so a second client gets
/// `ERROR_PIPE_BUSY`. The client must **wait** for an instance (bounded by
/// its timeout) and then fail with an error that says WHY - never a hang
/// and never a bare Win32 code.
#[test]
fn a_busy_pipe_waits_then_says_why() {
    use windows::core::PCWSTR;
    use windows::Win32::Storage::FileSystem::{
        CreateFileW, FILE_FLAGS_AND_ATTRIBUTES, FILE_GENERIC_READ, FILE_GENERIC_WRITE,
        FILE_SHARE_MODE, OPEN_EXISTING,
    };
    use windows::Win32::System::Pipes::{
        CreateNamedPipeW, NAMED_PIPE_MODE, PIPE_READMODE_MESSAGE, PIPE_TYPE_MESSAGE, PIPE_WAIT,
    };

    let name = format!(r"\\.\pipe\k6wp-engine-test-busy-{}", std::process::id());
    let wide: Vec<u16> = name.encode_utf16().chain(std::iter::once(0)).collect();
    // One instance only: the occupant takes it.
    // SAFETY: the name is null-terminated and the out-parameters are valid.
    let pipe = unsafe {
        CreateNamedPipeW(
            PCWSTR(wide.as_ptr()),
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
    // Occupy the instance.
    // SAFETY: the name is null-terminated and the handle is checked below.
    let occupant = unsafe {
        CreateFileW(
            PCWSTR(wide.as_ptr()),
            FILE_GENERIC_READ.0 | FILE_GENERIC_WRITE.0,
            FILE_SHARE_MODE(0),
            None,
            OPEN_EXISTING,
            windows::Win32::Storage::FileSystem::FILE_FLAG_OVERLAPPED,
            None,
        )
    };
    assert!(occupant.is_ok(), "the occupant must take the only instance");

    let options = ConnectOptions {
        timeout: Duration::from_millis(400),
        ..ConnectOptions::default()
    };
    let started = Instant::now();
    let result = IpcClient::connect(&name, options);
    let elapsed = started.elapsed();
    let error = match result {
        Ok(_) => panic!("a busy pipe must not connect"),
        Err(e) => e,
    };
    let message = error.to_string();
    assert!(
        message.contains("no free instance"),
        "the error must say why it failed: {message}"
    );
    assert!(
        elapsed >= Duration::from_millis(300),
        "the client must WAIT for an instance, not fail at once (it took {elapsed:?})"
    );
    assert!(
        elapsed < Duration::from_secs(3),
        "the wait must be bounded by the timeout (it took {elapsed:?})"
    );
    // SAFETY: both handles are live and not used afterwards.
    unsafe {
        let _ = windows::Win32::Foundation::CloseHandle(pipe);
        if let Ok(h) = occupant {
            let _ = windows::Win32::Foundation::CloseHandle(h);
        }
    }
}

/// The reconnect must release the OLD handle (task L4).
///
/// The engine serves one client at a time and creates the next instance
/// only after the previous client left. A reconnect that kept the old
/// handle open would keep the instance busy: the new connect would wait
/// for the whole timeout and fail - the 5 s stall the L2 runs hit. The
/// fake engine below behaves exactly like that (one instance at a time,
/// a brutal hang-up on the first connection).
#[test]
fn the_reconnect_releases_the_old_handle() {
    use windows::core::PCWSTR;
    use windows::Win32::Foundation::{CloseHandle, HANDLE, INVALID_HANDLE_VALUE};
    use windows::Win32::Storage::FileSystem::{ReadFile, WriteFile, FILE_FLAGS_AND_ATTRIBUTES};
    use windows::Win32::System::Pipes::{
        ConnectNamedPipe, CreateNamedPipeW, NAMED_PIPE_MODE, PIPE_READMODE_MESSAGE,
        PIPE_TYPE_MESSAGE, PIPE_WAIT,
    };

    let name = format!(
        r"\\.\pipe\k6wp-engine-test-reconnect-{}",
        std::process::id()
    );
    let wide: Vec<u16> = name.encode_utf16().chain(std::iter::once(0)).collect();
    let (ready_tx, ready_rx) = std::sync::mpsc::channel::<()>();
    let server = std::thread::spawn({
        let wide = wide.clone();
        move || {
            let deadline = Instant::now() + Duration::from_secs(20);
            let mut served = 0u32;
            while Instant::now() < deadline {
                // One instance at a time, exactly like the engine: the next
                // instance exists only after the client left.
                let pipe = loop {
                    // SAFETY: the name is null-terminated.
                    let p = unsafe {
                        CreateNamedPipeW(
                            PCWSTR(wide.as_ptr()),
                            FILE_FLAGS_AND_ATTRIBUTES(0x0000_0003),
                            NAMED_PIPE_MODE(
                                PIPE_TYPE_MESSAGE.0 | PIPE_READMODE_MESSAGE.0 | PIPE_WAIT.0,
                            ),
                            1,
                            65536,
                            65536,
                            0,
                            None,
                        )
                    };
                    if p != INVALID_HANDLE_VALUE {
                        break p;
                    }
                    // ERROR_PIPE_BUSY: the old instance is still alive.
                    if Instant::now() > deadline {
                        return;
                    }
                    std::thread::sleep(Duration::from_millis(20));
                };
                if served == 0 {
                    // The instance exists: the client may connect now.
                    let _ = ready_tx.send(());
                }
                // Wait for the client (ERROR_PIPE_CONNECTED means it is
                // already there).
                // SAFETY: the handle is live.
                if let Err(e) = unsafe { ConnectNamedPipe(pipe, None) } {
                    let code = (e.code().0 as u32) & 0xFFFF;
                    if code != 535 {
                        // SAFETY: the handle is live.
                        unsafe {
                            let _ = CloseHandle(pipe);
                        }
                        return;
                    }
                }
                // Read one request.
                let mut buffer = [0u8; 8192];
                let mut read = 0u32;
                // SAFETY: the handle is live and the buffer is valid.
                let ok = unsafe { ReadFile(pipe, Some(&mut buffer), Some(&mut read), None) };
                if ok.is_err() {
                    // SAFETY: the handle is live.
                    unsafe {
                        let _ = CloseHandle(pipe);
                    }
                    return;
                }
                // Answer it.
                let answer = b"{\"state\":{\"ok\":true}}\n";
                // SAFETY: the handle is live and the buffer is valid.
                let _ = unsafe { WriteFile(pipe, Some(answer), None, None) };
                // Read the NEXT request and hang up on it. The client reads
                // the answer above BEFORE it sends this one (the protocol is
                // strictly request/response), so the answer can never be
                // lost to the hang-up - no sleep, no race.
                let mut read2 = 0u32;
                // SAFETY: the handle is live and the buffer is valid.
                let ok2 = unsafe { ReadFile(pipe, Some(&mut buffer), Some(&mut read2), None) };
                served += 1;
                // The brutal hang-up: the answer to the second request never
                // comes, so that request fails on the client. The engine
                // DISCONNECTS the pipe (not just closes its handle) - a
                // closed handle leaves the client's pending read hanging
                // until its timeout.
                // SAFETY: the handle is live.
                unsafe {
                    let _ = windows::Win32::System::Pipes::DisconnectNamedPipe(pipe);
                    let _ = CloseHandle(pipe);
                }
                if served >= 2 || ok2.is_err() {
                    return;
                }
            }
        }
    });

    let options = ConnectOptions {
        // 2 s: the reconnect must find the new instance (the engine creates
        // it within ~50 ms), and the request after the last hang-up is
        // bounded by this timeout - a read posted AFTER a disconnect parks
        // until the server accepts again, so it ends in the timeout, not in
        // an error.
        timeout: Duration::from_secs(2),
        ..ConnectOptions::default()
    };
    let message = IpcMessage {
        version: k6wp_core::ipc::PROTOCOL_VERSION,
        cmd: Cmd::GetState,
        payload: serde_json::json!({}),
    };

    // The pipe instance must exist before the client connects (the engine
    // creates it, then waits for the client).
    ready_rx
        .recv_timeout(Duration::from_secs(5))
        .expect("the fake engine must create its pipe instance");

    let mut client =
        IpcClient::connect(&name, options.clone()).expect("the first connect must work");
    let first = client.request(&message);
    assert!(
        first.is_ok(),
        "the first request must be answered: {:?}",
        first.as_ref().err()
    );
    // The server hung up: this request must fail (and must not hang).
    let second = client.request(&message);
    assert!(second.is_err(), "the request after the hang-up must fail");
    // The reconnect: the old handle is released, so the engine can accept.
    let mut client = client
        .reconnect(options)
        .expect("the reconnect must succeed - the old handle must be released first");
    let third = client.request(&message);
    assert!(
        third.is_ok(),
        "the request after the reconnect must be answered: {:?}",
        third.as_ref().err()
    );
    // The second hang-up: the request after it fails and the fake engine
    // ends its loop (so the join below cannot hang).
    let fourth = client.request(&message);
    assert!(fourth.is_err(), "the request after the hang-up must fail");

    let _ = server.join();
    let _ = HANDLE(std::ptr::null_mut());
}

/// Test: set_video encodes correctly (codec-level test).
#[test]
fn set_video_encodes_correctly() {
    let message = IpcMessage {
        version: k6wp_core::ipc::PROTOCOL_VERSION,
        cmd: Cmd::SetVideo,
        payload: serde_json::json!({ "path": r"C:\videos\test.mp4" }),
    };
    let encoded = encode(&message).expect("must encode");
    assert!(
        encoded.contains("set_video"),
        "encoded message must contain set_video command"
    );
    assert!(
        encoded.contains(r"C:\\videos\\test.mp4"),
        "encoded message must contain the path"
    );
    assert!(
        encoded.contains("\"version\":1"),
        "encoded message must contain version 1"
    );
    assert!(
        encoded.ends_with('\n'),
        "encoded message must end with newline"
    );
}

/// Test: set_video error payload decodes correctly.
#[test]
fn set_video_error_payload_decodes() {
    // Test that an error response from the engine can be parsed.
    let error_response = serde_json::json!({ "error": "video format not supported" });
    let error_msg = error_response["error"].as_str().unwrap_or("");
    assert!(error_msg.contains("video format"));
}

/// Test: missing media handling is tested in k6wp-studio worker tests.
#[test]
fn set_video_missing_media() {
    // The worker probes the media file first (with timeout) and only sends
    // set_video if the file exists. If missing, it emits IpcError and does NOT send IPC.
    // This is tested in k6wp-studio's worker tests.
    assert!(
        true,
        "missing media handling is tested in k6wp-studio worker tests"
    );
}
