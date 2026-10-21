//! The golden-fixture probe for the contracts this agent owns
//! (displays, playlist, ipc, version).
//!
//! It replays `tests/golden/shared_behavior.json` against the Rust
//! implementations, exactly like the C++ `tests/golden_dump.cpp` produced
//! them, and prints every difference. Zero differences = the port is
//! behaviour-preserving.
//!
//! The `golden.rs` harness covers the whole document; this probe covers the
//! slice of it this agent owns, with the C++ dump's exact shapes (the
//! harness and the fixture disagree on a few case shapes - see the report).

use std::collections::BTreeMap;

use k6wp_core::{displays, ipc, playlist, version};
use serde_json::{json, Map, Value};

fn cases() -> Vec<Value> {
    // The crate lives in <repo>/rust/crates/k6wp-core, so the fixtures are
    // three levels up.
    let path = concat!(
        env!("CARGO_MANIFEST_DIR"),
        "/../../../tests/golden/shared_behavior.json"
    );
    let text = std::fs::read_to_string(path).expect("the golden fixtures must be readable");
    let root: Value = serde_json::from_str(&text).expect("the golden file must be JSON");
    root["cases"].as_array().cloned().unwrap_or_default()
}

/// The `state` object of a get_state answer, in the C++ dump's shape.
fn engine_state_json(state: &ipc::EngineState) -> Value {
    let mut out = Map::new();
    out.insert("pid".into(), json!(state.pid));
    out.insert("video".into(), json!(state.video));
    out.insert("paused".into(), json!(state.paused));
    out.insert("headless_slots".into(), json!(state.headless_slots));
    out.insert("live".into(), json!(state.live));
    out.insert("display_capability".into(), json!(state.display_capability));
    out.insert(
        "display_assignments".into(),
        json!(state.display_assignments),
    );
    out.insert("display_coverage".into(), json!(state.display_coverage));
    out.insert("playlist_enabled".into(), json!(state.playlist_enabled));
    out.insert("playlist_size".into(), json!(state.playlist_size));
    out.insert("playlist_index".into(), json!(state.playlist_index));
    out.insert("reconcile_mode".into(), json!(state.reconcile_mode));
    out.insert(
        "reconcile_last_trigger".into(),
        json!(state.reconcile_last_trigger),
    );
    out.insert(
        "reconcile_last_verdict".into(),
        json!(state.reconcile_last_verdict),
    );
    out.insert("reconcile_retries".into(), json!(state.reconcile_retries));
    out.insert(
        "reconcile_degraded_keys".into(),
        json!(state.reconcile_degraded_keys),
    );
    Value::Object(out)
}

/// The command vocabulary, in the C++ dump's shape.
fn cmd_vocabulary_json() -> Value {
    let mut out = Map::new();
    for (name, cmd) in ipc::cmd_vocabulary() {
        out.insert(name.to_string(), json!(cmd as i32));
    }
    let mut from_string = Map::new();
    for name in [
        "set_video",
        "pause",
        "resume",
        "set_monitor",
        "get_state",
        "quit",
        "set_display_video",
        "nope",
        "",
    ] {
        let key = if name.is_empty() { "<empty>" } else { name };
        let value = ipc::cmd_from_string(name).map(|c| c as i32).unwrap_or(-1);
        from_string.insert(key.to_string(), json!(value));
    }
    out.insert("from_string".into(), Value::Object(from_string));
    out.insert("protocol_version".into(), json!(ipc::PROTOCOL_VERSION));
    out.insert("max_payload_bytes".into(), json!(ipc::MAX_PAYLOAD_BYTES));
    Value::Object(out)
}

/// The rotation rows, in the C++ dump's shape (the shuffle state is carried
/// across the calls, in order, from the fixture's seed).
fn select_next_index_json() -> Value {
    let mut state = 0x0123_4567_89AB_CDEF_u64;
    let mut rows = Vec::new();
    for count in [0usize, 1, 3, 8] {
        let mut row = Map::new();
        row.insert("count".into(), json!(count));
        row.insert(
            "sequential_from_0".into(),
            json!(playlist::select_next_index_with_state(
                0, count, false, &mut state
            )),
        );
        row.insert(
            "sequential_from_2".into(),
            json!(playlist::select_next_index_with_state(
                2, count, false, &mut state
            )),
        );
        row.insert(
            "shuffle_from_0".into(),
            json!(playlist::select_next_index_with_state(
                0, count, true, &mut state
            )),
        );
        rows.push(Value::Object(row));
    }
    Value::Array(rows)
}

/// The result of one case: the value, or the failure message.
fn run_case(case: &Value) -> Result<Value, String> {
    let id = case["id"].as_str().unwrap_or_default().to_string();
    let input = &case["input"];

    if id.starts_with("displays.") {
        let text = input.as_str().unwrap_or_default();
        let displays = displays::migrate_displays(text).map_err(|e| e.to_string())?;
        return Ok(displays::displays_json(&displays));
    }

    if id.starts_with("playlist.") {
        if id == "playlist.normalize_order" {
            let order: Vec<String> = input["order"]
                .as_array()
                .map(|a| {
                    a.iter()
                        .map(|v| v.as_str().unwrap_or_default().to_string())
                        .collect()
                })
                .unwrap_or_default();
            return Ok(json!(playlist::normalize_order(order)));
        }
        if id == "playlist.select_next_index" {
            return Ok(select_next_index_json());
        }
        if id == "playlist.index_for_path" {
            let order: Vec<String> = input["order"]
                .as_array()
                .map(|a| {
                    a.iter()
                        .map(|v| v.as_str().unwrap_or_default().to_string())
                        .collect()
                })
                .unwrap_or_default();
            let queries: Vec<String> = input["queries"]
                .as_array()
                .map(|a| {
                    a.iter()
                        .map(|v| v.as_str().unwrap_or_default().to_string())
                        .collect()
                })
                .unwrap_or_default();
            let query =
                |index: usize| -> String { queries.get(index).cloned().unwrap_or_default() };
            let mut out = Map::new();
            out.insert(
                "found_exact".into(),
                json!(playlist::playlist_index_for_path(&order, &query(0))),
            );
            out.insert(
                "found_case_insensitive".into(),
                json!(playlist::playlist_index_for_path(&order, &query(1))),
            );
            out.insert(
                "not_found".into(),
                json!(playlist::playlist_index_for_path(&order, &query(2))),
            );
            out.insert(
                "empty_order".into(),
                json!(playlist::playlist_index_for_path(&[], &query(2))),
            );
            return Ok(Value::Object(out));
        }
        let text = input.as_str().unwrap_or_default();
        let cfg = playlist::migrate_playlist(text).map_err(|e| e.to_string())?;
        return Ok(playlist::playlist_json(&cfg));
    }

    if id.starts_with("ipc.") {
        if id == "ipc.cmd_vocabulary" {
            return Ok(cmd_vocabulary_json());
        }
        if let Some(cmd_name) = id.strip_prefix("ipc.encode.") {
            let cmd = ipc::cmd_from_string(cmd_name)
                .ok_or_else(|| format!("unknown command: {cmd_name}"))?;
            let msg = ipc::IpcMessage {
                version: ipc::PROTOCOL_VERSION,
                cmd,
                payload: input.clone(),
            };
            return Ok(json!(ipc::encode(&msg).map_err(|e| e.to_string())?));
        }
        if id.starts_with("ipc.decode.") {
            let text = input.as_str().unwrap_or_default();
            let msg = ipc::decode(text).map_err(|e| e.to_string())?;
            return Ok(json!({
                "version": msg.version,
                "cmd": ipc::cmd_to_string(msg.cmd),
                "payload": msg.payload,
            }));
        }
        if id.starts_with("ipc.parse_state.") {
            let text = input.as_str().unwrap_or_default();
            return Ok(engine_state_json(&ipc::parse_engine_state_text(text)));
        }
        return Err(format!("unknown IPC case: {id}"));
    }

    if id.starts_with("version.") {
        let a = input["a"].as_str().unwrap_or_default();
        let b = input["b"].as_str().unwrap_or_default();
        return Ok(json!(version::compare_versions(a, b)));
    }

    Err(format!("unknown case: {id}"))
}

#[test]
fn the_golden_fixtures_match_on_the_rust_side() {
    let all = cases();
    assert!(!all.is_empty(), "the golden file must not be empty");
    let mut failed: Vec<String> = Vec::new();
    let mut checked = 0usize;
    let mut matched: Vec<String> = Vec::new();

    for case in &all {
        let id = case["id"].as_str().unwrap_or_default();
        if !(id.starts_with("displays.")
            || id.starts_with("playlist.")
            || id.starts_with("ipc.")
            || id.starts_with("version."))
        {
            continue;
        }
        checked += 1;
        let has_expected_output = case.get("output").is_some();
        let actual = run_case(case);
        match (has_expected_output, actual) {
            (true, Ok(value)) => {
                if value == case["output"] {
                    matched.push(id.to_string());
                } else {
                    failed.push(format!("{id}: expected {}, got {}", case["output"], value));
                }
            }
            (true, Err(e)) => failed.push(format!("{id}: expected a value, got the error {e}")),
            (false, Ok(value)) => failed.push(format!("{id}: expected a failure, got {value}")),
            (false, Err(_)) => matched.push(id.to_string()),
        }
    }

    for line in &failed {
        eprintln!("[FAIL] {line}");
    }
    eprintln!("probe: {checked} cases replayed, {} match", matched.len());
    assert!(
        failed.is_empty(),
        "{} of {} golden cases differ on the Rust side",
        failed.len(),
        checked
    );
}

/// The order slice API must agree with the count API everywhere the C++
/// playlist is not empty.
#[test]
fn the_order_api_agrees_with_the_count_api() {
    playlist::set_shuffle_seed(0x0123_4567_89AB_CDEF);
    for count in [1usize, 3, 8] {
        let order: Vec<String> = (0..count).map(|i| format!("C:\\{i}.mp4")).collect();
        assert_eq!(
            playlist::select_next_index(&order, 0, false),
            1 % count as i64
        );
        assert_eq!(playlist::select_next_index(&order, count - 1, false), 0);
    }
    // An empty playlist has nothing to select.
    assert_eq!(playlist::select_next_index(&[], 0, false), -1);
    assert_eq!(playlist::select_next_index(&[], 0, true), -1);
}

/// The `Displays` map is sorted, so the JSON is deterministic.
#[test]
fn the_displays_json_is_sorted() {
    let mut assignments = BTreeMap::new();
    for key in ["\\\\.\\DISPLAY2", "\\\\.\\DISPLAY1"] {
        assignments.insert(
            key.to_string(),
            displays::DisplayAssignment {
                path: "C:\\a.mp4".to_string(),
                exists: true,
                key_stable: String::new(),
            },
        );
    }
    let store = displays::Displays {
        version: 1,
        assignments,
    };
    let json = displays::displays_json(&store);
    let keys: Vec<&String> = json["assignments"]
        .as_object()
        .expect("an object")
        .keys()
        .collect();
    assert_eq!(keys, vec!["\\\\.\\DISPLAY1", "\\\\.\\DISPLAY2"]);
}
