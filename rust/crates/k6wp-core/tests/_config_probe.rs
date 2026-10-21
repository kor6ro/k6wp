//! A local replay of the frozen golden cases for the config and the studio
//! settings against the Rust port.
//!
//! The golden file is produced by the C++ `tests/golden_dump.cpp`, whose
//! `DumpConfig` / `DumpStudioSettings` record the MIGRATION result only:
//! the C++ dump never calls `ValidateConfig`, so a migrated config may
//! still carry an out-of-range value. This probe replays the same table
//! with the same semantics and prints the diffs.
//!
//! The two shapes the C++ dump records for the settings are reproduced by
//! the two Rust serializers: `settings_json` is the migrated-settings
//! projection and `settings_report_json` is the full report (the one with
//! the directory basenames).

use k6wp_core::{config, settings};
use serde_json::Value;

fn golden_cases() -> Vec<Value> {
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

/// The C++ `DumpConfig` / `DumpStudioSettings` semantics: the migration
/// alone, no validation.
fn run_case(case: &Value) -> Result<Value, String> {
    let id = case["id"].as_str().unwrap_or_default();
    if id == "config.default" {
        return Ok(config::config_to_json(&config::WallpaperConfig::default()));
    }
    if id == "settings.default" {
        return Ok(settings::settings_report_json(
            &settings::StudioSettings::default(),
        ));
    }
    let raw_text = case["input"].as_str().unwrap_or_default();
    if id.starts_with("config.") {
        let cfg = config::migrate_config(raw_text).map_err(|e| e.to_string())?;
        return Ok(config::config_to_json(&cfg));
    }
    if id.starts_with("settings.") {
        let settings = settings::migrate_studio_settings(raw_text).map_err(|e| e.to_string())?;
        return Ok(settings::settings_json(&settings));
    }
    Err(format!("unknown case: {id}"))
}

fn is_ours(id: &str) -> bool {
    id.starts_with("config.") || id.starts_with("settings.")
}

#[test]
fn the_config_and_settings_golden_cases_match() {
    let cases = golden_cases();
    let mut failed: Vec<String> = Vec::new();
    let mut checked = 0usize;
    for case in &cases {
        let id = case["id"].as_str().unwrap_or_default();
        if !is_ours(id) {
            continue;
        }
        let expected = &case["output"];
        let actual = run_case(case);
        checked += 1;
        match (expected.is_object() || expected.is_array(), actual) {
            (true, Ok(value)) => {
                if &value != expected {
                    failed.push(format!("{id}: expected {expected}, got {value}"));
                }
            }
            (true, Err(e)) => failed.push(format!("{id}: expected {expected}, got the error {e}")),
            (false, Ok(value)) => failed.push(format!("{id}: expected a failure, got {value}")),
            (false, Err(_)) => {}
        }
    }
    assert!(
        checked > 0,
        "the golden file must hold the config/settings cases"
    );
    if !failed.is_empty() {
        for line in &failed {
            eprintln!("[FAIL] {line}");
        }
        panic!("{} of {} golden cases differ", failed.len(), checked);
    }
    eprintln!("probe: {checked} config/settings cases replayed, all match");
}

#[test]
fn the_error_messages_are_the_cpp_ones_verbatim() {
    let cases = golden_cases();
    let mut failed: Vec<String> = Vec::new();
    let mut checked = 0usize;
    for case in &cases {
        let id = case["id"].as_str().unwrap_or_default();
        if !is_ours(id) {
            continue;
        }
        let Some(expected) = case.get("error").and_then(|e| e.as_str()) else {
            continue;
        };
        checked += 1;
        match run_case(case) {
            Ok(value) => failed.push(format!("{id}: expected the error {expected}, got {value}")),
            Err(actual) => {
                if actual != expected {
                    failed.push(format!("{id}: expected the error {expected}, got {actual}"));
                }
            }
        }
    }
    assert!(checked > 0, "the golden file must hold the failing cases");
    if !failed.is_empty() {
        for line in &failed {
            eprintln!("[FAIL] {line}");
        }
        panic!("{} of {} error messages differ", failed.len(), checked);
    }
    eprintln!("probe: {checked} error messages match the C++ ones verbatim");
}

/// The C++ `ValidateConfig` rejects what the migration carries over: the
/// golden fixtures record the migrated value (the dump does not validate),
/// so the validation is a separate, later step.
///
/// The list below is the C++ `ValidateConfig` verdict for every migrated
/// golden case: the cases the C++ rejects are exactly the ones with an
/// out-of-range value or an unknown enum.
#[test]
fn the_validation_agrees_with_the_cpp_validate_config() {
    const REJECTED: [&str; 13] = [
        "config.migrate.unknown_fit",
        "config.migrate.crf_low",
        "config.migrate.crf_high",
        "config.migrate.speed_low",
        "config.migrate.speed_high",
        "config.migrate.fps_zero",
        "config.migrate.fps_high",
        "config.migrate.res_negative",
        "config.migrate.monitor_below_minus1",
        "config.migrate.battery_mode_bogus",
        "config.migrate.affinity_bogus",
        "config.migrate.adapter_bogus",
        "config.migrate.v5_reconcile_bogus",
    ];
    let cases = golden_cases();
    let mut checked = 0usize;
    for case in &cases {
        let id = case["id"].as_str().unwrap_or_default();
        if !id.starts_with("config.") || id == "config.default" {
            continue;
        }
        let raw_text = case["input"].as_str().unwrap_or_default();
        let Ok(cfg) = config::migrate_config(raw_text) else {
            continue;
        };
        checked += 1;
        let expected_valid = !REJECTED.contains(&id);
        assert_eq!(
            config::validate_config(&cfg).is_ok(),
            expected_valid,
            "{id}: the validation must agree with the C++ ValidateConfig"
        );
    }
    assert!(checked > 0, "the golden file must hold the config cases");
}
