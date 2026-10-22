//! The golden-fixture replayer (Phase 2, slice 2.2).
//!
//! The C++ side owns the frozen behaviour: `tests/golden/shared_behavior.json`
//! is produced by `tests/golden_dump.cpp` and checked by the `golden_test`
//! ctest gate. This test replays the SAME file against the Rust
//! implementations, so a Rust port that differs in any way - a default, a
//! migration rule, an error case - fails here exactly as it would fail the
//! C++ gate.
//!
//! The case shape:
//!   { "fn": "...", "id": "...", "input": ..., "output": ... }
//! `input` is the RAW document text (a JSON string) for the migrate
//! functions and an object for the pure ones. `output` is the expected
//! result; an EMPTY output means the call must FAIL.

use std::collections::BTreeMap;

use k6wp_core::{config, displays, ipc, playlist, settings, version};
use serde_json::{json, Value};

fn cases() -> Vec<Value> {
    let path = concat!(
        env!("CARGO_MANIFEST_DIR"),
        "/../../../tests/golden/shared_behavior.json"
    );
    let text = std::fs::read_to_string(path)
        .unwrap_or_else(|e| panic!("the golden fixtures must be readable: {e}"));
    let root: Value = serde_json::from_str(&text).expect("the golden file must be JSON");
    root["cases"]
        .as_array()
        .cloned()
        .unwrap_or_else(|| panic!("the golden file must have a 'cases' array"))
}

/// A string list from the case input.
fn string_list(input: &Value, key: &str) -> Vec<String> {
    input[key]
        .as_array()
        .map(|a| {
            a.iter()
                .map(|v| v.as_str().unwrap_or_default().to_string())
                .collect()
        })
        .unwrap_or_default()
}

/// Does the case expect the call to FAIL?
///
/// The C++ golden dump records an EMPTY output for a call that threw - and
/// an empty output is an empty string (the dump writes the returned text),
/// so a non-empty string IS a result.
fn expects_failure(expected: &Value) -> bool {
    match expected {
        Value::String(s) => s.is_empty(),
        Value::Null => true,
        _ => false,
    }
}

/// The JSON the C++ side records for a migrated config.
fn config_json(cfg: &config::WallpaperConfig) -> Value {
    config::config_to_json(cfg)
}

fn run_case(case: &Value) -> Result<Value, String> {
    let id = case["id"].as_str().unwrap_or_default().to_string();
    let input = &case["input"];

    // --- the config ------------------------------------------------------
    if id.starts_with("config.") {
        if id == "config.default" {
            let cfg = config::WallpaperConfig::default();
            return Ok(config_json(&cfg));
        }
        // The round trip: the merge that preserves the unknown keys.
        if id.starts_with("config.roundtrip.") {
            let existing = &input["existing"];
            let known = &input["known"];
            return Ok(config::merge_unknown_keys(existing, known));
        }
        let raw_text = input
            .as_str()
            .ok_or_else(|| "the config input must be the raw document text".to_string())?;
        // The C++ golden dump records ConfigToJson(MigrateConfig(raw)) - the
        // migration ONLY. ValidateConfig is a separate step (the loader
        // calls it), so the fixture does NOT expect the invalid values to
        // be rejected here.
        let cfg = config::migrate_config(raw_text).map_err(|e| e.to_string())?;
        return Ok(config_json(&cfg));
    }

    // --- the studio settings ---------------------------------------------
    if id.starts_with("settings.") {
        if id == "settings.default" {
            let s = settings::StudioSettings::default();
            // The default case is recorded in the C++ "report" shape (17
            // keys: it adds start_with_windows and the two basenames); the
            // migrate cases use the 14-key projection.
            return Ok(settings::settings_report_json(&s));
        }
        let raw_text = input
            .as_str()
            .ok_or_else(|| "the settings input must be the raw document text".to_string())?;
        let s = settings::migrate_studio_settings(raw_text).map_err(|e| e.to_string())?;
        return Ok(settings::settings_json(&s));
    }

    // --- the displays ----------------------------------------------------
    if id.starts_with("displays.") {
        let raw_text = input
            .as_str()
            .ok_or_else(|| "the displays input must be the raw document text".to_string())?;
        let d = displays::migrate_displays(raw_text).map_err(|e| e.to_string())?;
        return Ok(displays::displays_json(&d));
    }

    // --- the playlist ----------------------------------------------------
    if id.starts_with("playlist.") {
        if id == "playlist.normalize_order" {
            let order = string_list(input, "order");
            return Ok(json!(playlist::normalize_order(order)));
        }
        if id == "playlist.select_next_index" {
            // The C++ SelectNextIndex carries a shuffle state across the
            // calls - the fixture records the four calls in order.
            let mut state = 0x0123_4567_89AB_CDEF_u64;
            let mut rows = Vec::new();
            for count in [0usize, 1, 3, 8] {
                let mut row = serde_json::Map::new();
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
            return Ok(Value::Array(rows));
        }
        if id == "playlist.index_for_path" {
            let order = string_list(input, "order");
            let queries = string_list(input, "queries");
            let q = |i: usize| queries.get(i).cloned().unwrap_or_default();
            return Ok(json!({
                "found_exact": playlist::playlist_index_for_path(&order, &q(0)),
                "found_case_insensitive": playlist::playlist_index_for_path(&order, &q(1)),
                "not_found": playlist::playlist_index_for_path(&order, &q(2)),
                "empty_order": playlist::playlist_index_for_path(&[], &q(2)),
            }));
        }
        if id.starts_with("playlist.migrate.") {
            let raw_text = input
                .as_str()
                .ok_or_else(|| "the playlist input must be the raw document text".to_string())?;
            let cfg = playlist::migrate_playlist(raw_text).map_err(|e| e.to_string())?;
            return Ok(playlist::playlist_json(&cfg));
        }
        return Err(format!("unknown playlist case: {id}"));
    }

    // --- the IPC ---------------------------------------------------------
    if id.starts_with("ipc.") {
        if id == "ipc.cmd_vocabulary" {
            let mut map = BTreeMap::new();
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
                map.insert(
                    key.to_string(),
                    ipc::cmd_from_string(name).map(|c| c as i32).unwrap_or(-1),
                );
            }
            return Ok(json!({
                "from_string": map,
                "get_state": ipc::Cmd::GetState as i32,
                "max_payload_bytes": ipc::MAX_PAYLOAD_BYTES,
                "pause": ipc::Cmd::Pause as i32,
                "protocol_version": ipc::PROTOCOL_VERSION,
                "quit": ipc::Cmd::Quit as i32,
                "resume": ipc::Cmd::Resume as i32,
                "set_display_video": ipc::Cmd::SetDisplayVideo as i32,
                "set_monitor": ipc::Cmd::SetMonitor as i32,
                "set_video": ipc::Cmd::SetVideo as i32,
            }));
        }
        if id.starts_with("ipc.encode.") {
            let cmd_name = id.trim_start_matches("ipc.encode.");
            let cmd = ipc::cmd_from_string(cmd_name).ok_or("an unknown command")?;
            let msg = ipc::IpcMessage {
                version: ipc::PROTOCOL_VERSION,
                cmd,
                payload: input.clone(),
            };
            let frame = ipc::encode(&msg).map_err(|e| e.to_string())?;
            return Ok(json!(frame));
        }
        if id.starts_with("ipc.decode.") {
            let text = input
                .as_str()
                .ok_or_else(|| "the IPC input must be the raw document text".to_string())?;
            let msg = ipc::decode(text).map_err(|e| e.to_string())?;
            return Ok(json!({
                "cmd": ipc::cmd_to_string(msg.cmd),
                "version": msg.version,
                "payload": msg.payload,
            }));
        }
        if id.starts_with("ipc.parse_state.") {
            let text = input.as_str().unwrap_or_default();
            let state = ipc::parse_engine_state_text(text);
            return Ok(json!({
                "pid": state.pid,
                "video": state.video,
                "paused": state.paused,
                "headless_slots": state.headless_slots,
                "live": state.live,
                "display_capability": state.display_capability,
                "display_assignments": state.display_assignments,
                "display_coverage": state.display_coverage,
                "playlist_enabled": state.playlist_enabled,
                "playlist_size": state.playlist_size,
                "playlist_index": state.playlist_index,
                "reconcile_mode": state.reconcile_mode,
                "reconcile_last_trigger": state.reconcile_last_trigger,
                "reconcile_last_verdict": state.reconcile_last_verdict,
                "reconcile_retries": state.reconcile_retries,
                "reconcile_degraded_keys": state.reconcile_degraded_keys,
            }));
        }
        return Err(format!("unknown IPC case: {id}"));
    }

    // --- the version compare ---------------------------------------------
    if id.starts_with("version.") {
        let a = input["a"].as_str().unwrap_or_default();
        let b = input["b"].as_str().unwrap_or_default();
        return Ok(json!(version::compare_versions(a, b)));
    }

    Err(format!("unknown case: {id}"))
}

#[test]
fn the_golden_fixtures_pass_on_the_rust_side() {
    let all = cases();
    assert!(!all.is_empty(), "the golden file must not be empty");
    let mut failed: Vec<String> = Vec::new();
    let mut checked = 0usize;
    for case in &all {
        let id = case["id"].as_str().unwrap_or_default();
        let expected = &case["output"];
        let actual = run_case(case);
        checked += 1;
        match (expects_failure(expected), actual) {
            // The expectation is a result.
            (false, Ok(value)) => {
                if &value != expected {
                    failed.push(format!("{id}: expected {expected}, got {value}"));
                }
            }
            (false, Err(e)) => {
                failed.push(format!("{id}: expected {expected}, got the error {e}"));
            }
            // The expectation is a FAILURE (the C++ dump records an empty
            // output for a call that threw).
            (true, Ok(value)) => {
                failed.push(format!("{id}: expected a failure, got {value}"));
            }
            (true, Err(_)) => {}
        }
    }
    if !failed.is_empty() {
        for line in &failed {
            eprintln!("[FAIL] {line}");
        }
        panic!(
            "{} of {} golden cases differ on the Rust side",
            failed.len(),
            checked
        );
    }
    eprintln!("golden: {checked} cases replayed, all match");
}
