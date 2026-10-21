//! The G5 replay: the Rust engine logic against the C++ golden fixtures.
//!
//! The fixture (tests/golden/engine_logic.json) is written by the C++ side
//! (engine_logic_dump) and checked by the C++ side (the ctest
//! "engine_logic_test"). This test replays the SAME fixture through the
//! Rust port, so a difference in either implementation fails both gates:
//! the C++ one for a drift from the frozen behaviour, this one for a port
//! that does not match the C++.
//!
//! The case count and the per-module counts are asserted here, so a fixture
//! that silently loses cases cannot pass.

use k6wp_core::engine_logic::{
    covers_monitor, crash_restart_advance, crash_restart_allowed, decide_reconcile_action,
    diff_topology, pause, pin_verify_add_reverts, pin_verify_due_at, reconcile_action_name,
    reconcile_action_needs_stable_topology, retry_next_delay_ms, retry_schedule_due,
    retry_schedule_exhausted, topology_signature, topology_stable, unload_on_pause_changed,
    unload_on_tick, workerw_accepts_child_classes, CoverageVerdict, CrashRestartState, DecodeState,
    MonitorInfo, PlacementRect, ReconcileAction, RetryClock, TopologyMonitor, TopologySnapshot,
    UnloadMachine,
};
use serde_json::Value;

fn topology_monitor(value: &Value) -> TopologyMonitor {
    TopologyMonitor {
        stable_key: value["stable_key"].as_str().unwrap().to_string(),
        x: value["x"].as_i64().unwrap() as i32,
        y: value["y"].as_i64().unwrap() as i32,
        width: value["width"].as_i64().unwrap() as i32,
        height: value["height"].as_i64().unwrap() as i32,
        scale_pct: value["scale_pct"].as_i64().unwrap() as i32,
        orientation: value["orientation"].as_i64().unwrap() as i32,
        is_primary: value["is_primary"].as_bool().unwrap(),
        degraded: value["degraded"].as_bool().unwrap(),
    }
}

fn topology_snapshot(value: &Value) -> TopologySnapshot {
    TopologySnapshot {
        monitors: value["monitors"]
            .as_array()
            .unwrap()
            .iter()
            .map(topology_monitor)
            .collect(),
        virtual_x: value["virtual_x"].as_i64().unwrap() as i32,
        virtual_y: value["virtual_y"].as_i64().unwrap() as i32,
        virtual_width: value["virtual_width"].as_i64().unwrap() as i32,
        virtual_height: value["virtual_height"].as_i64().unwrap() as i32,
        hash_snapshot: value["hash_snapshot"].as_str().unwrap().to_string(),
    }
}

fn monitor_info(value: &Value) -> MonitorInfo {
    MonitorInfo {
        id: value["id"].as_i64().unwrap() as i32,
        x: value["x"].as_i64().unwrap() as i32,
        y: value["y"].as_i64().unwrap() as i32,
        width: value["width"].as_i64().unwrap() as i32,
        height: value["height"].as_i64().unwrap() as i32,
        is_primary: value["is_primary"].as_bool().unwrap(),
        device_name: value["device_name"].as_str().unwrap().to_string(),
        orientation: value["orientation"].as_i64().unwrap() as i32,
        refresh_hz: value["refresh_hz"].as_i64().unwrap() as i32,
        scale_pct: value["scale_pct"].as_i64().unwrap() as i32,
        stable_key: value["stable_key"].as_str().unwrap().to_string(),
        identity_degraded: value["identity_degraded"].as_bool().unwrap(),
    }
}

fn fixture_path() -> std::path::PathBuf {
    std::path::Path::new(env!("CARGO_MANIFEST_DIR")).join("../../../tests/golden/engine_logic.json")
}

fn rect(value: &Value) -> PlacementRect {
    PlacementRect {
        left: value["left"].as_i64().unwrap() as i32,
        top: value["top"].as_i64().unwrap() as i32,
        right: value["right"].as_i64().unwrap() as i32,
        bottom: value["bottom"].as_i64().unwrap() as i32,
    }
}

fn evaluate(case: &Value) -> Value {
    let module = case["module"].as_str().unwrap();
    let input = &case["input"];
    match module {
        "desktop_placement" => {
            let verdict = covers_monitor(rect(&input["child"]), rect(&input["monitor"]));
            let token = match verdict {
                CoverageVerdict::Covered => "placement: covered",
                CoverageVerdict::ClippedLeft => "placement: CLIPPED-left",
                CoverageVerdict::ClippedTop => "placement: CLIPPED-top",
                CoverageVerdict::ClippedRight => "placement: CLIPPED-right",
                CoverageVerdict::ClippedBottom => "placement: CLIPPED-bottom",
                CoverageVerdict::OutOfBounds => "placement: OUT-OF-BOUNDS",
            };
            serde_json::json!({ "verdict": token })
        }
        "workerw_host" => {
            let children: Vec<String> = input["child_classes"]
                .as_array()
                .unwrap()
                .iter()
                .map(|c| c.as_str().unwrap().to_string())
                .collect();
            let own = input["own_class"].as_str().unwrap();
            serde_json::json!({
                "accepted": workerw_accepts_child_classes(&children, own)
            })
        }
        "retry_schedule" => {
            let clock = RetryClock {
                first_pass_ms: input["first_pass_ms"].as_i64().unwrap(),
                attempts_done: input["attempts_done"].as_i64().unwrap() as i32,
            };
            let now_ms = input["now_ms"].as_i64().unwrap();
            serde_json::json!({
                "due": retry_schedule_due(now_ms, &clock),
                "next_delay_ms": retry_next_delay_ms(now_ms, &clock),
                "exhausted": retry_schedule_exhausted(&clock),
            })
        }
        "crash_restart_guard" => {
            let state = CrashRestartState {
                count: input["count"].as_i64().unwrap() as i32,
                window_start_ms: input["window_start_ms"].as_i64().unwrap(),
            };
            let advanced = crash_restart_advance(
                state,
                input["now_ms"].as_i64().unwrap(),
                input["max_restarts"].as_i64().unwrap() as i32,
                input["window_ms"].as_i64().unwrap(),
            );
            serde_json::json!({
                "count": advanced.count,
                "window_start_ms": advanced.window_start_ms,
                "allowed": crash_restart_allowed(&advanced, input["max_restarts"].as_i64().unwrap() as i32),
            })
        }
        "pause_controller" => {
            let mask = input["mask"].as_i64().unwrap() as i32;
            serde_json::json!({
                "mask": mask,
                "ui_paused": pause::ui_paused(mask),
                "slots_paused": pause::slots_paused(mask),
                "hides_wallpaper": pause::hides_wallpaper(mask),
            })
        }
        "topology_diff" => {
            let old_s = topology_snapshot(&input["old"]);
            let new_s = topology_snapshot(&input["new"]);
            let d = diff_topology(&old_s, &new_s);
            serde_json::json!({
                "added_keys": d.added_keys,
                "removed_keys": d.removed_keys,
                "moved_keys": d.moved_keys,
                "resized_keys": d.resized_keys,
                "dpi_keys": d.dpi_keys,
                "orientation_keys": d.orientation_keys,
                "primary_changed": d.primary_changed,
                "hash_changed": d.hash_changed,
                "empty": d.is_empty(),
            })
        }
        "reconcile_policy" => {
            if let Some(trigger) = input.get("trigger").and_then(Value::as_str) {
                let action = decide_reconcile_action(trigger);
                let _ = ReconcileAction::None;
                serde_json::json!({
                    "action": reconcile_action_name(action),
                    "needs_stable_topology": reconcile_action_needs_stable_topology(action),
                })
            } else {
                let first: Vec<MonitorInfo> = input["first"]
                    .as_array()
                    .unwrap()
                    .iter()
                    .map(monitor_info)
                    .collect();
                let second: Vec<MonitorInfo> = input["second"]
                    .as_array()
                    .unwrap()
                    .iter()
                    .map(monitor_info)
                    .collect();
                serde_json::json!({
                    "stable": topology_stable(&first, &second),
                    "signature": topology_signature(&first),
                })
            }
        }
        "unload_state" => {
            let state = match input["state"].as_str().unwrap() {
                "paused" => DecodeState::Paused,
                "unloaded" => DecodeState::Unloaded,
                _ => DecodeState::Active,
            };
            let machine = UnloadMachine {
                state,
                pause_started_ms: input["pause_started_ms"].as_i64().unwrap(),
                unload_after_ms: input["unload_after_ms"].as_i64().unwrap() as i32,
            };
            let now_ms = input["now_ms"].as_i64().unwrap();
            let decision = if input["op"].as_str().unwrap() == "tick" {
                unload_on_tick(&machine, input["mask"].as_i64().unwrap() as i32, now_ms)
            } else {
                unload_on_pause_changed(&machine, input["paused"].as_bool().unwrap(), now_ms)
            };
            serde_json::json!({
                "state": decision.state.as_str(),
                "pause_started_ms": decision.pause_started_ms,
                "unload_due": decision.unload_due,
                "reload_due": decision.reload_due,
            })
        }
        "pin_verify_schedule" => {
            if let Some(armed) = input.get("armed").and_then(Value::as_bool) {
                serde_json::json!({
                    "due": pin_verify_due_at(
                        armed,
                        input["deadline_ms"].as_i64().unwrap(),
                        input["now_ms"].as_i64().unwrap(),
                    )
                })
            } else {
                serde_json::json!({
                    "total": pin_verify_add_reverts(
                        input["total"].as_i64().unwrap() as i32,
                        input["reverts"].as_i64().unwrap() as i32,
                    )
                })
            }
        }
        other => panic!("the fixture holds an unknown module: {other}"),
    }
}

#[test]
fn the_rust_port_matches_the_cpp_golden_fixtures() {
    let path = fixture_path();
    let text = std::fs::read_to_string(&path)
        .unwrap_or_else(|e| panic!("cannot read {}: {e}", path.display()));
    let doc: Value = serde_json::from_str(&text).expect("the fixture must be valid JSON");
    let cases = doc["cases"]
        .as_array()
        .expect("the fixture must hold cases");
    assert!(!cases.is_empty(), "the fixture holds no cases");

    let mut per_module = std::collections::BTreeMap::new();
    for case in cases {
        let module = case["module"].as_str().unwrap();
        let name = case["name"].as_str().unwrap();
        let expected = &case["output"];
        let actual = evaluate(case);
        assert_eq!(
            &actual, expected,
            "the Rust port differs from the C++ in {module}/{name}"
        );
        *per_module.entry(module.to_string()).or_insert(0usize) += 1;
    }

    // The per-module case counts (the G5 record). If a module is added, the
    // count changes here on purpose - the record stays honest.
    let expected_counts: std::collections::BTreeMap<&str, usize> = [
        ("crash_restart_guard", 8),
        ("desktop_placement", 9),
        ("pause_controller", 10),
        ("pin_verify_schedule", 8),
        ("reconcile_policy", 14),
        ("retry_schedule", 8),
        ("topology_diff", 10),
        ("unload_state", 18),
        ("workerw_host", 6),
    ]
    .into_iter()
    .collect();
    assert_eq!(
        per_module,
        expected_counts
            .iter()
            .map(|(k, v)| (k.to_string(), *v))
            .collect::<std::collections::BTreeMap<String, usize>>(),
        "the per-module case counts changed"
    );
}
