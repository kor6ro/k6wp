//! The G6 live-engine test: the Rust client against the REAL engine.
//!
//! It is opt-in (K6WP_IPC_LIVE=1) because it needs a running engine - and it
//! drives the whole command set against it:
//!
//!   get_state -> set_video -> pause -> resume -> get_state ->
//!   set_display_video -> an unknown cmd -> a payload with an unknown key ->
//!   quit
//!
//! What it proves:
//!
//!   - the client VERIFIES the server before it sends anything (the process
//!     id, the session, the owner and the image path);
//!   - a REFUSED server is refused (the expected image is wrong -> the
//!     client never sends a command);
//!   - the engine answers every command of the frozen vocabulary;
//!   - an UNKNOWN command gets an error ack (the engine must not crash);
//!   - a payload with an UNKNOWN key is ignored (the forward-compatibility
//!     of the protocol);
//!   - the client can RECONNECT after the pipe was closed.
//!
//! The quit is the last step: the engine exits, so the wrapper (tools/
//! drive_engine_ipc.ps1) restarts it in its `finally`.

use std::path::PathBuf;
use std::time::Duration;

use k6wp_core::ipc::{Cmd, IpcMessage, PROTOCOL_VERSION};
use k6wp_ipc::{current_session_pipe_name, ConnectOptions, IpcClient};

fn live_enabled() -> bool {
    std::env::var("K6WP_IPC_LIVE").as_deref() == Ok("1")
}

fn options(expected_image: Option<PathBuf>) -> ConnectOptions {
    ConnectOptions {
        timeout: Duration::from_secs(5),
        expected_server_image: expected_image,
        require_current_user: true,
        require_same_session: true,
    }
}

fn message(cmd: Cmd, payload: serde_json::Value) -> IpcMessage {
    IpcMessage {
        version: PROTOCOL_VERSION,
        cmd,
        payload,
    }
}

#[test]
fn the_live_engine_answers_the_whole_command_set() {
    if !live_enabled() {
        eprintln!("[SKIP] the live engine test needs K6WP_IPC_LIVE=1");
        return;
    }

    let engine_exe = std::env::var("K6WP_IPC_ENGINE_EXE").ok().map(PathBuf::from);
    assert!(
        engine_exe.is_some(),
        "K6WP_IPC_ENGINE_EXE must point at the running engine (the image check must not be skipped)"
    );

    // 1. A server whose image is NOT the expected one is REFUSED - the
    //    client must not send a single command to it.
    let wrong = options(Some(PathBuf::from(r"C:\not\the\engine.exe")));
    let refused = IpcClient::connect_current_session(wrong);
    assert!(
        refused.is_err(),
        "a server with the wrong image must be refused"
    );

    // Give the engine time to recreate the pipe instance after the refused connection.
    std::thread::sleep(Duration::from_millis(200));

    // 2. The real engine: the connection is verified.
    let mut client = IpcClient::connect_current_session(options(engine_exe.clone()))
        .expect("the engine must accept the connection");
    let identity = client.server_identity().clone();
    assert!(identity.process_id > 0, "the server pid must be known");
    assert!(
        identity.owner_sid.is_some(),
        "the server owner must be known"
    );
    eprintln!(
        "the engine: pid={} session={} owner={:?} image={:?}",
        identity.process_id, identity.session_id, identity.owner_sid, identity.image_path
    );

    // 3. get_state: the state must come back with the known fields.
    let state = client
        .request(&message(Cmd::GetState, serde_json::json!({})))
        .expect("get_state must answer");
    assert!(
        state.get("state").is_some(),
        "the get_state answer must carry the state: {state}"
    );
    let inner = &state["state"];
    eprintln!("the state: {inner}");
    // The display capability is part of the frozen contract (0 = no
    // capability).
    assert!(
        inner.get("display_capability").is_some(),
        "the state must carry display_capability: {inner}"
    );

    // 4. set_video: the engine must accept the command.
    let ack = client
        .request(&message(
            Cmd::SetVideo,
            serde_json::json!({ "path": r"C:\Users\axioo\AppData\Local\K6WP\wallpapers\1657392155-1657392155-desktop-esdeath-akame-ga-kill-live-wallpaper_k6wp7.mp4" }),
        ))
        .expect("set_video must answer");
    assert_eq!(
        ack.get("ok").and_then(|v| v.as_bool()),
        Some(true),
        "set_video must be accepted: {ack}"
    );

    // 5. pause / resume.
    let ack = client
        .request(&message(Cmd::Pause, serde_json::json!({})))
        .expect("pause must answer");
    assert_eq!(
        ack.get("ok").and_then(|v| v.as_bool()),
        Some(true),
        "pause: {ack}"
    );
    let ack = client
        .request(&message(Cmd::Resume, serde_json::json!({})))
        .expect("resume must answer");
    assert_eq!(
        ack.get("ok").and_then(|v| v.as_bool()),
        Some(true),
        "resume: {ack}"
    );

    // 6. set_display_video: the assignment must be accepted.
    let ack = client
        .request(&message(
            Cmd::SetDisplayVideo,
            serde_json::json!({ "device": "\\\\.\\DISPLAY1", "path": "C:\\wallpapers\\assigned.mp4" }),
        ))
        .expect("set_display_video must answer");
    assert_eq!(
        ack.get("ok").and_then(|v| v.as_bool()),
        Some(true),
        "set_display_video: {ack}"
    );

    // 7. A payload with an UNKNOWN key: the engine must ignore it and still
    //    answer (the forward compatibility of the protocol).
    let ack = client
        .request(&message(
            Cmd::SetVideo,
            serde_json::json!({ "path": r"C:\Users\axioo\AppData\Local\K6WP\wallpapers\1657392155-1657392155-desktop-esdeath-akame-ga-kill-live-wallpaper_k6wp7.mp4", "a_key_from_the_future": 42 }),
        ))
        .expect("a payload with an unknown key must still be answered");
    assert_eq!(
        ack.get("ok").and_then(|v| v.as_bool()),
        Some(true),
        "an unknown key must be ignored: {ack}"
    );

    // 8. An UNKNOWN command: the engine must answer with an error ack and
    //    keep running (it must not crash).
    client
        .write_line(b"{\"version\":1,\"cmd\":\"a_command_from_the_future\",\"payload\":{}}\n")
        .expect("the unknown command must be sent");
    let answer = client.read_line().expect("the engine must answer");
    let text = String::from_utf8_lossy(&answer).to_string();
    eprintln!("the unknown command was answered with: {text}");
    let ack: serde_json::Value =
        serde_json::from_str(text.trim()).expect("the answer must be JSON");
    assert!(
        ack.get("error").is_some(),
        "an unknown command must get an error ack: {ack}"
    );

    // 9. The engine is still alive after the unknown command.
    let state = client
        .request(&message(Cmd::GetState, serde_json::json!({})))
        .expect("the engine must still answer");
    assert!(
        state.get("state").is_some(),
        "the engine must still be alive"
    );

    // 10. RECONNECT: close the pipe and connect again.
    // The engine quits and restarts with --restarted; wait for it to be ready.
    client.close();
    std::thread::sleep(Duration::from_millis(2000));
    let mut again =
        IpcClient::connect_current_session(options(engine_exe)).expect("the reconnect must work");
    let state = again
        .request(&message(Cmd::GetState, serde_json::json!({})))
        .expect("the reconnected client must get an answer");
    assert!(state.get("state").is_some());

    // 11. The drive must not leave a side effect: the assignment it made
    //     is dropped again (the engine accepts {"device","clear":true}).
    let ack = again
        .request(&message(
            Cmd::SetDisplayVideo,
            serde_json::json!({ "device": "\\\\.\\DISPLAY1", "clear": true }),
        ))
        .expect("the assignment must be clearable");
    assert_eq!(
        ack.get("ok").and_then(|v| v.as_bool()),
        Some(true),
        "clearing the assignment: {ack}"
    );

    // 12. quit: the engine exits (the wrapper restarts it).
    let ack = again
        .request(&message(Cmd::Quit, serde_json::json!({})))
        .expect("quit must answer");
    eprintln!("the quit was answered with: {ack}");
    let name = current_session_pipe_name(None);
    eprintln!("the pipe was {name}");
}
