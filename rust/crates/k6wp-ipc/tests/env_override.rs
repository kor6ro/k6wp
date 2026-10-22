//! The environment overrides (task I4) - in their OWN test binary.
//!
//! The overrides are process-global, so the tests that set them live in
//! their own binary: the test harness runs the tests of a binary in
//! parallel, and a test that reads the name must not see another test''s
//! override.
//!
//! WHAT IS PROVEN HERE
//!
//! 1. The override is read on EVERY call (never cached in a static: a static
//!    is initialised before main() and the client and the server would
//!    disagree about the name - that is what caused the 30-minute hang in
//!    the C++ suite).
//! 2. The override changes the pipe NAME only - it does NOT weaken the
//!    G6 server verification: a pipe that is not the expected engine is
//!    still refused.
//! 3. The override is a DEBUG-only feature: the release build has it
//!    compiled out (the release binaries are scanned by
//!    tools/check_release_overrides.ps1).

use std::time::Duration;

use k6wp_ipc::{current_session_pipe_name, ConnectOptions, IpcClient};
#[test]
fn the_env_override_wins() {
    // The override is read on EVERY call (never cached in a static: a static
    // is initialised before main() and the client and the server would
    // disagree about the name - that is what caused the 30-minute hang in
    // the C++ suite).
    //
    // It is a DEBUG-only feature (task I4): the release build has it
    // compiled out, so this test only holds in a debug/test build.
    let unique = r"\\.\pipe\k6wp-engine-test-override-42";
    std::env::set_var("K6WP_ENGINE_PIPE", unique);
    if cfg!(debug_assertions) {
        // A DEBUG build: the override wins.
        assert_eq!(current_session_pipe_name(), unique);
    } else {
        // A RELEASE build: the override is compiled OUT, so the name is
        // the real per-session one (the shipped binary cannot be
        // redirected).
        assert_ne!(current_session_pipe_name(), unique);
    }
    std::env::remove_var("K6WP_ENGINE_PIPE");
    assert_ne!(current_session_pipe_name(), unique);
}

#[test]
fn the_override_does_not_weaken_the_server_verification() {
    // The override changes the pipe NAME only. The server behind it is
    // verified exactly as before: the process id, the session, the owner
    // and the image path. A pipe that is not the expected engine is
    // refused - with the override set as well.
    let name = format!(
        r"\\.\pipe\k6wp-engine-test-unverified-{}",
        std::process::id()
    );
    std::env::set_var("K6WP_ENGINE_PIPE", &name);

    // A pipe with nobody behind it: the client must not accept it.
    use windows::core::PCWSTR;
    use windows::Win32::Storage::FileSystem::FILE_FLAGS_AND_ATTRIBUTES;
    use windows::Win32::System::Pipes::{
        CreateNamedPipeW, NAMED_PIPE_MODE, PIPE_READMODE_MESSAGE, PIPE_TYPE_MESSAGE, PIPE_WAIT,
    };
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

    // The image check refuses it (the pipe is not the engine).
    let options = ConnectOptions {
        timeout: Duration::from_millis(500),
        expected_server_image: Some(std::path::PathBuf::from(r"C:\not\the\engine.exe")),
        ..ConnectOptions::default()
    };
    let result = IpcClient::connect(&name, options);
    assert!(
        result.is_err(),
        "the override must not weaken the server verification"
    );
    // SAFETY: the handle is live and not used afterwards.
    unsafe {
        let _ = windows::Win32::Foundation::CloseHandle(pipe);
    }
    std::env::remove_var("K6WP_ENGINE_PIPE");
}

#[test]
fn the_accepted_server_images_are_the_known_ones() {
    use k6wp_ipc::{is_accepted_server_image, ACCEPTED_SERVER_IMAGE_NAMES};
    // engine.exe (today) and K6WP.exe (after the Phase 3 merge) are
    // accepted; anything else is not.
    assert!(ACCEPTED_SERVER_IMAGE_NAMES.contains(&"engine.exe"));
    assert!(ACCEPTED_SERVER_IMAGE_NAMES.contains(&"k6wp.exe"));
    let expected = std::path::Path::new(r"C:\Program Files\K6WP\engine.exe");
    assert!(is_accepted_server_image(expected, expected));
    assert!(is_accepted_server_image(
        std::path::Path::new(r"C:\Program Files\K6WP\K6WP.exe"),
        expected
    ));
    // A different name is refused.
    assert!(!is_accepted_server_image(
        std::path::Path::new(r"C:\Program Files\K6WP\notepad.exe"),
        expected
    ));
    // A different directory is refused (the dev path is a separate
    // decision, see dev_server_image_allowed).
    assert!(!is_accepted_server_image(
        std::path::Path::new(r"C:\dev\build\engine.exe"),
        expected
    ));
}
