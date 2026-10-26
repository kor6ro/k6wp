//! Prints one `get_state` answer as JSON - the read-only probe the A-12b e2e
//! uses to check which video the engine is playing, without changing anything.
//!
//! USAGE
//!   cargo run --example get_state_once
//!   cargo run --example get_state_once -- --pipe '\\.\pipe\...' --json out.json

use k6wp_core::ipc::{Cmd, IpcMessage, PROTOCOL_VERSION};
use k6wp_ipc::{current_session_pipe_name, ConnectOptions, IpcClient};

fn main() {
    let args: Vec<String> = std::env::args().collect();
    let mut pipe = current_session_pipe_name(None);
    let mut json_out: Option<String> = None;
    let mut i = 1;
    while i < args.len() {
        match args[i].as_str() {
            "--pipe" => {
                if let Some(value) = args.get(i + 1) {
                    pipe = value.clone();
                }
                i += 2;
            }
            "--json" => {
                json_out = args.get(i + 1).cloned();
                i += 2;
            }
            other => {
                eprintln!("get_state_once: the unknown argument {other}");
                std::process::exit(2);
            }
        }
    }

    let mut client = match IpcClient::connect(
        &pipe,
        ConnectOptions {
            timeout: std::time::Duration::from_secs(5),
            expected_server_image: None,
            require_current_user: true,
            require_same_session: true,
        },
    ) {
        Ok(client) => client,
        Err(error) => {
            eprintln!("get_state_once: connect failed: {error}");
            std::process::exit(1);
        }
    };
    let message = IpcMessage {
        version: PROTOCOL_VERSION,
        cmd: Cmd::GetState,
        payload: serde_json::json!({}),
    };
    match client.request(&message) {
        Ok(answer) => {
            let text = serde_json::to_string(&answer).unwrap_or_else(|_| answer.to_string());
            println!("{text}");
            if let Some(path) = json_out {
                if let Err(error) = std::fs::write(&path, &text) {
                    eprintln!("get_state_once: cannot write {path}: {error}");
                    std::process::exit(1);
                }
            }
        }
        Err(error) => {
            eprintln!("get_state_once: request failed: {error}");
            std::process::exit(1);
        }
    }
}
