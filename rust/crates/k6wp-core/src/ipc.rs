//! The Engine <-> Studio IPC codec: the command vocabulary and the NDJSON
//! frame.
//!
//! The Rust mirror of the frozen C++ contract in `shared/ipc_protocol.hpp` /
//! `shared/ipc_protocol.cpp`. The Win32 pipe-name helpers of that .cpp are
//! NOT ported here: only the message codec is a contract.
//!
//! One message is one NDJSON line:
//!
//! ```text
//! {"cmd":"set_video","payload":{"path":"C:\\v\\a.mp4"},"version":1}
//! ```
//!
//! The rules the C++ enforces, and this port keeps:
//!
//! - the `Cmd` vocabulary is APPEND-ONLY: the numeric values never move and
//!   never get reordered, so an old engine and a new Studio still agree;
//! - a frame is rejected (never a crash) when it is empty, is not JSON, is
//!   not an object, has no `version`, has a `version` that is not the
//!   protocol version, has no `cmd`, has a `cmd` that is not a string, or
//!   names a command that does not exist;
//! - the payload is capped at 64 KiB.

use std::collections::BTreeMap;
use std::fmt;

use serde_json::{Map, Value};

/// The protocol version. Bump it when the wire format changes
/// incompatibly.
pub const PROTOCOL_VERSION: u32 = 1;

/// The maximum serialized payload size (64 KiB). A bigger payload is
/// rejected by `encode` and by `decode`.
pub const MAX_PAYLOAD_BYTES: usize = 64 * 1024;

/// The commands the engine understands.
///
/// The discriminants are the wire contract: they are APPEND-ONLY, so a new
/// command takes the next free number and never renumbers an old one.
#[repr(i32)]
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub enum Cmd {
    SetVideo = 0,
    Pause = 1,
    Resume = 2,
    SetMonitor = 3,
    GetState = 4,
    Quit = 5,
    SetDisplayVideo = 6,
}

/// The wire vocabulary, in the append-only order (the C++ `kCmdNames`).
const CMD_NAMES: [(&str, Cmd); 7] = [
    ("set_video", Cmd::SetVideo),
    ("pause", Cmd::Pause),
    ("resume", Cmd::Resume),
    ("set_monitor", Cmd::SetMonitor),
    ("get_state", Cmd::GetState),
    ("quit", Cmd::Quit),
    ("set_display_video", Cmd::SetDisplayVideo),
];

/// Maps a command to its wire string.
pub fn cmd_to_string(cmd: Cmd) -> &'static str {
    match cmd {
        Cmd::SetVideo => "set_video",
        Cmd::Pause => "pause",
        Cmd::Resume => "resume",
        Cmd::SetMonitor => "set_monitor",
        Cmd::GetState => "get_state",
        Cmd::Quit => "quit",
        Cmd::SetDisplayVideo => "set_display_video",
    }
}

/// Parses a wire string back to a command.
///
/// `None` for a name that is not in the vocabulary - the C++
/// `CmdFromString` returns false, and the frame is then rejected.
pub fn cmd_from_string(name: &str) -> Option<Cmd> {
    CMD_NAMES
        .iter()
        .find(|(wire, _)| *wire == name)
        .map(|(_, cmd)| *cmd)
}

/// Every command name, in the stable append-only order.
pub fn cmd_vocabulary() -> Vec<(&'static str, Cmd)> {
    CMD_NAMES.to_vec()
}

/// One NDJSON message.
#[derive(Clone, Debug, PartialEq)]
pub struct IpcMessage {
    /// The protocol version of the frame.
    pub version: u32,
    /// The command.
    pub cmd: Cmd,
    /// The command payload. An empty payload is an empty object.
    pub payload: Value,
}

impl Default for IpcMessage {
    fn default() -> Self {
        Self {
            version: PROTOCOL_VERSION,
            cmd: Cmd::GetState,
            payload: Value::Object(Map::new()),
        }
    }
}

/// The structured error the codec reports.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct IpcError(pub String);

impl fmt::Display for IpcError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str(&self.0)
    }
}

impl std::error::Error for IpcError {}

/// Serializes a message to a single NDJSON line (the JSON plus the `'\n'`).
///
/// # Errors
///
/// - a payload (or a whole frame) bigger than `MAX_PAYLOAD_BYTES`.
pub fn encode(msg: &IpcMessage) -> Result<String, IpcError> {
    let payload_json = serde_json::to_string(&msg.payload)
        .map_err(|e| IpcError(format!("Encode: the payload is not serializable: {e}")))?;
    if payload_json.len() > MAX_PAYLOAD_BYTES {
        return Err(IpcError(
            "Encode: payload exceeds kMaxPayloadBytes (64 KiB)".to_string(),
        ));
    }
    let mut root = Map::new();
    root.insert("version".to_string(), Value::from(msg.version));
    root.insert(
        "cmd".to_string(),
        Value::String(cmd_to_string(msg.cmd).to_string()),
    );
    root.insert("payload".to_string(), msg.payload.clone());
    // The documented 64 KiB limit is on the NDJSON LINE, and the envelope
    // adds the key/version overhead - so the frame is capped too.
    let frame = serde_json::to_string(&Value::Object(root))
        .map_err(|e| IpcError(format!("Encode: the frame is not serializable: {e}")))?;
    if frame.len() > MAX_PAYLOAD_BYTES {
        return Err(IpcError(
            "Encode: frame exceeds kMaxPayloadBytes (64 KiB)".to_string(),
        ));
    }
    Ok(frame + "\n")
}

/// Parses one NDJSON line.
///
/// A line with or without a trailing `'\n'` is accepted (the encoder always
/// emits one); an embedded newline (a multi-line input) is rejected.
///
/// # Errors
///
/// - an empty line, a multi-line input, a malformed JSON, a root that is not
///   an object, a missing/non-integer `version`, a version that is not
///   `PROTOCOL_VERSION`, a missing/non-string `cmd`, a command that is not in
///   the vocabulary, or a payload bigger than `MAX_PAYLOAD_BYTES`.
pub fn decode(text: &str) -> Result<IpcMessage, IpcError> {
    if text.is_empty() {
        return Err(IpcError("Decode: the frame is empty".to_string()));
    }
    // One NDJSON line: strip a single trailing '\n' (and an optional '\r'),
    // then reject anything that still contains a newline.
    let mut line = text;
    if let Some(stripped) = line.strip_suffix('\n') {
        line = stripped;
    }
    if let Some(stripped) = line.strip_suffix('\r') {
        line = stripped;
    }
    if line.is_empty() || line.contains('\n') {
        return Err(IpcError(
            "Decode: the frame is not a single line".to_string(),
        ));
    }

    let root: Value = serde_json::from_str(line)
        .map_err(|e| IpcError(format!("Decode: the frame is not valid JSON: {e}")))?;
    let object = root
        .as_object()
        .ok_or_else(|| IpcError("Decode: the frame is not a JSON object".to_string()))?;

    let version = match object.get("version") {
        Some(Value::Number(number)) if number.is_i64() || number.is_u64() => number
            .as_i64()
            .unwrap_or_else(|| number.as_u64().unwrap_or(0) as i64),
        Some(_) => {
            return Err(IpcError(
                "Decode: the version is not an integer".to_string(),
            ))
        }
        None => return Err(IpcError("Decode: the frame has no version".to_string())),
    };
    if version != PROTOCOL_VERSION as i64 {
        return Err(IpcError(format!(
            "Decode: unsupported protocol version: {version}"
        )));
    }

    let cmd_name = match object.get("cmd") {
        Some(Value::String(name)) => name.as_str(),
        Some(_) => return Err(IpcError("Decode: the command is not a string".to_string())),
        None => return Err(IpcError("Decode: the frame has no command".to_string())),
    };
    let cmd = cmd_from_string(cmd_name)
        .ok_or_else(|| IpcError(format!("Decode: unknown command: {cmd_name}")))?;

    // A missing payload is an empty object; a payload of another type is
    // kept as it is (the C++ `value()` copies the JSON value as it is).
    let payload = object
        .get("payload")
        .cloned()
        .unwrap_or_else(|| Value::Object(Map::new()));
    let payload_json = serde_json::to_string(&payload)
        .map_err(|e| IpcError(format!("Decode: the payload is not serializable: {e}")))?;
    if payload_json.len() > MAX_PAYLOAD_BYTES {
        return Err(IpcError(
            "Decode: the payload exceeds kMaxPayloadBytes (64 KiB)".to_string(),
        ));
    }

    Ok(IpcMessage {
        version: version as u32,
        cmd,
        payload,
    })
}

/// The fields of a `get_state` ack's `state` object.
///
/// Missing keys keep the defaults; a wrong-typed value falls back to the
/// default too. This is the C++ `EngineState`.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct EngineState {
    /// The engine process id.
    pub pid: u64,
    /// The current video path (UTF-8).
    pub video: String,
    /// Whether the engine is paused.
    pub paused: bool,
    /// The number of headless slots.
    pub headless_slots: i32,
    /// Whether the engine is live.
    pub live: bool,
    /// 0 = the engine predates the display fields, 1 = they are meaningful.
    pub display_capability: i32,
    /// The per-monitor assignment map (GDI device name -> UTF-8 video path).
    pub display_assignments: BTreeMap<String, String>,
    /// The per-device placement verdict ("covered", "clipped-left", ...).
    pub display_coverage: BTreeMap<String, String>,
    /// Whether the playlist rotation is on.
    pub playlist_enabled: bool,
    /// The playlist length.
    pub playlist_size: i32,
    /// The current playlist index (-1 = none).
    pub playlist_index: i32,
    /// The reconcile mode ("legacy", "shadow", "on").
    pub reconcile_mode: String,
    /// What triggered the last reconcile pass.
    pub reconcile_last_trigger: String,
    /// The verdict of the last reconcile pass.
    pub reconcile_last_verdict: String,
    /// The retry count of the last reconcile pass.
    pub reconcile_retries: i32,
    /// The degraded keys of the last reconcile pass.
    pub reconcile_degraded_keys: Vec<String>,
}

impl Default for EngineState {
    fn default() -> Self {
        Self {
            pid: 0,
            video: String::new(),
            paused: false,
            headless_slots: 0,
            live: true,
            display_capability: 0,
            display_assignments: BTreeMap::new(),
            display_coverage: BTreeMap::new(),
            playlist_enabled: false,
            playlist_size: 0,
            playlist_index: -1,
            reconcile_mode: String::new(),
            reconcile_last_trigger: String::new(),
            reconcile_last_verdict: String::new(),
            reconcile_retries: 0,
            reconcile_degraded_keys: Vec::new(),
        }
    }
}

/// The nlohmann `get<unsigned long long>` conversion.
fn as_u64(value: &Value) -> Option<u64> {
    match value {
        Value::Number(number) => {
            if let Some(u) = number.as_u64() {
                Some(u)
            } else if let Some(i) = number.as_i64() {
                Some(i as u64)
            } else {
                number.as_f64().map(|f| f as u64)
            }
        }
        _ => None,
    }
}

/// The nlohmann `get<int>` conversion.
fn as_i32(value: &Value) -> Option<i32> {
    match value {
        Value::Number(number) => {
            if let Some(i) = number.as_i64() {
                Some(i as i32)
            } else if let Some(u) = number.as_u64() {
                Some(u as i32)
            } else {
                number.as_f64().map(|f| f as i32)
            }
        }
        _ => None,
    }
}

/// True for a JSON integer (the C++ `is_number_integer()`: a float is not
/// an integer).
fn is_integer(value: &Value) -> bool {
    matches!(value, Value::Number(number) if number.is_i64() || number.is_u64())
}

/// The `state.value(key, default)` of the C++: a missing key takes the
/// default, a wrong type is an error (and aborts the whole parse, exactly
/// like the C++ does).
fn state_value<T>(
    state: &Value,
    key: &str,
    default: T,
    read: impl Fn(&Value) -> Option<T>,
) -> Result<T, ()> {
    let object = state.as_object().ok_or(())?;
    match object.get(key) {
        None => Ok(default),
        Some(value) => read(value).ok_or(()),
    }
}

/// Parses the `state` object of a `get_state` answer.
///
/// This is the Rust mirror of the C++ `ParseEngineState`: it NEVER throws.
/// A wrong-typed value falls back to the default, and - like the C++ - the
/// parse stops at the first wrong-typed value, keeping the fields it had
/// already read.
pub fn parse_engine_state(raw: &Value) -> EngineState {
    let mut state = EngineState::default();
    // The C++ `raw.value("state", json::object())` throws when the root is
    // not an object, and the catch keeps the defaults.
    let Some(object) = raw.as_object() else {
        return state;
    };
    let inner = object
        .get("state")
        .cloned()
        .unwrap_or_else(|| Value::Object(Map::new()));

    // The C++ reads the fields in this order and aborts on the first type
    // mismatch, keeping what it has already assigned.
    let _ = (|| -> Result<(), ()> {
        state.pid = state_value(&inner, "pid", 0u64, as_u64)?;
        state.video = state_value(&inner, "video", String::new(), |v| {
            v.as_str().map(str::to_string)
        })?;
        state.paused = state_value(&inner, "paused", false, |v| v.as_bool())?;
        state.headless_slots = state_value(&inner, "headless_slots", 0i32, as_i32)?;
        state.live = state_value(&inner, "live", true, |v| v.as_bool())?;

        if let Some(capability) = inner.get("display_capability") {
            if is_integer(capability) {
                if let Some(value) = as_i32(capability) {
                    state.display_capability = value;
                }
            }
        }
        if let Some(assignments) = inner.get("display_assignments") {
            if let Some(map) = assignments.as_object() {
                for (device, path) in map {
                    if let Some(path) = path.as_str() {
                        state
                            .display_assignments
                            .insert(device.clone(), path.to_string());
                    }
                }
            }
        }
        if let Some(coverage) = inner.get("display_coverage") {
            if let Some(map) = coverage.as_object() {
                for (device, verdict) in map {
                    if let Some(verdict) = verdict.as_str() {
                        state
                            .display_coverage
                            .insert(device.clone(), verdict.to_string());
                    }
                }
            }
        }
        if let Some(enabled) = inner.get("playlist_enabled") {
            if let Some(enabled) = enabled.as_bool() {
                state.playlist_enabled = enabled;
            }
        }
        if let Some(size) = inner.get("playlist_size") {
            if is_integer(size) {
                if let Some(value) = as_i32(size) {
                    state.playlist_size = value;
                }
            }
        }
        if let Some(index) = inner.get("playlist_index") {
            if is_integer(index) {
                if let Some(value) = as_i32(index) {
                    state.playlist_index = value;
                }
            }
        }
        if let Some(reconcile) = inner.get("reconcile") {
            if let Some(map) = reconcile.as_object() {
                if let Some(mode) = map.get("mode").and_then(Value::as_str) {
                    state.reconcile_mode = mode.to_string();
                }
                if let Some(trigger) = map.get("last_trigger").and_then(Value::as_str) {
                    state.reconcile_last_trigger = trigger.to_string();
                }
                if let Some(verdict) = map.get("last_verdict").and_then(Value::as_str) {
                    state.reconcile_last_verdict = verdict.to_string();
                }
                if let Some(retries) = map.get("retries") {
                    if is_integer(retries) {
                        if let Some(value) = as_i32(retries) {
                            state.reconcile_retries = value;
                        }
                    }
                }
                if let Some(degraded) = map.get("degraded_keys") {
                    if let Some(list) = degraded.as_array() {
                        for key in list {
                            if let Some(key) = key.as_str() {
                                state.reconcile_degraded_keys.push(key.to_string());
                            }
                        }
                    }
                }
            }
        }
        Ok(())
    })();

    state
}

/// The same parse, on a raw document. A corrupt document yields the
/// defaults (the parse never throws).
pub fn parse_engine_state_text(text: &str) -> EngineState {
    match serde_json::from_str::<Value>(text) {
        Ok(raw) => parse_engine_state(&raw),
        Err(_) => EngineState::default(),
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn the_vocabulary_is_append_only() {
        assert_eq!(cmd_to_string(Cmd::SetVideo), "set_video");
        assert_eq!(cmd_to_string(Cmd::SetDisplayVideo), "set_display_video");
        assert_eq!(cmd_from_string("set_video"), Some(Cmd::SetVideo));
        assert_eq!(
            cmd_from_string("set_display_video"),
            Some(Cmd::SetDisplayVideo)
        );
        assert_eq!(cmd_from_string("nope"), None);
        assert_eq!(cmd_from_string(""), None);
        assert_eq!(cmd_vocabulary().len(), 7);
        assert_eq!(cmd_vocabulary()[0], ("set_video", Cmd::SetVideo));
        assert_eq!(
            cmd_vocabulary()[6],
            ("set_display_video", Cmd::SetDisplayVideo)
        );
    }

    #[test]
    fn the_frame_is_the_frozen_byte_sequence() {
        let msg = IpcMessage {
            version: PROTOCOL_VERSION,
            cmd: Cmd::SetVideo,
            payload: serde_json::json!({ "path": "C:\\v\\a.mp4" }),
        };
        assert_eq!(
            encode(&msg).expect("must encode"),
            "{\"cmd\":\"set_video\",\"payload\":{\"path\":\"C:\\\\v\\\\a.mp4\"},\"version\":1}\n"
        );
    }

    #[test]
    fn a_frame_round_trips() {
        let frame =
            "{\"version\":1,\"cmd\":\"set_video\",\"payload\":{\"path\":\"C:\\\\v\\\\a.mp4\"}}\n";
        let msg = decode(frame).expect("must decode");
        assert_eq!(msg.version, 1);
        assert_eq!(msg.cmd, Cmd::SetVideo);
        assert_eq!(msg.payload["path"], Value::from("C:\\v\\a.mp4"));
    }

    #[test]
    fn the_malformed_frames_are_rejected() {
        for frame in [
            "",
            "\n",
            "{\"version\":1,",
            "[1,2,3]",
            "{\"cmd\":\"get_state\"}",
            "{\"version\":\"1\",\"cmd\":\"get_state\"}",
            "{\"version\":2,\"cmd\":\"get_state\"}",
            "{\"version\":1}",
            "{\"version\":1,\"cmd\":5}",
            "{\"version\":1,\"cmd\":\"explode\"}",
            "{\"version\":1,\"cmd\":\"get_state\"}\n{\"version\":1,\"cmd\":\"quit\"}",
        ] {
            assert!(decode(frame).is_err(), "must reject: {frame:?}");
        }
    }

    #[test]
    fn a_frame_without_a_payload_gets_an_empty_one() {
        let msg = decode("{\"version\":1,\"cmd\":\"quit\"}").expect("must decode");
        assert_eq!(msg.payload, Value::Object(Map::new()));
    }

    #[test]
    fn the_state_defaults_are_kept() {
        let state = parse_engine_state_text("{}");
        assert_eq!(state.pid, 0);
        assert!(state.live);
        assert_eq!(state.playlist_index, -1);
        assert!(state.display_assignments.is_empty());
    }

    #[test]
    fn a_wrong_typed_state_falls_back_to_the_defaults() {
        let state = parse_engine_state_text(
            r#"{"state":{"pid":"x","paused":"yes","display_capability":"y",
                 "display_assignments":[],"playlist_size":"z","reconcile":"nope"}}"#,
        );
        assert_eq!(state, EngineState::default());
    }
}
