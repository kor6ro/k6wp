//! The wallpaper configuration schema (`config.json`).
//!
//! This is the Rust mirror of the C++ contract in `shared/config_schema.hpp`
//! and `shared/config_schema.cpp`. The behaviour is FROZEN: the same keys,
//! the same defaults, the same per-version migration rules, the same
//! validation ranges and the same error cases. The C++ side stays the
//! reference until the cut-over, and the golden fixtures
//! (`tests/golden/shared_behavior.json`) are replayed against both sides.
//!
//! The three rules that make the port behaviour-preserving:
//!
//! * A MISSING key takes the C++ default.
//! * A key with the WRONG TYPE is an ERROR. The C++ `raw.value(key,
//!   default)` throws a `type_error` on a mismatch, so a hand-edited file
//!   with `{"speed": "fast"}` fails the load instead of silently taking the
//!   default. The Rust side rejects it the same way.
//! * The migrated config is always written at the CURRENT schema version,
//!   whatever the file said.

use std::path::PathBuf;

use serde_json::{Map, Value};

/// The on-disk schema version of `config.json` (the C++
/// `kConfigSchemaVersion`).
pub const CONFIG_SCHEMA_VERSION: i64 = 6;

/// The error context the config contract uses in its type errors.
const CONTEXT: &str = "config";

/// The structured error the config and settings contracts report - the Rust
/// mirror of the C++ `ConfigError` (a `std::runtime_error` carrying a
/// human-readable message).
///
/// The message is verbatim the C++ `what()` string, so a caller that logs
/// the error prints the same text on both sides.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct ConfigError(pub String);

impl std::fmt::Display for ConfigError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.write_str(&self.0)
    }
}

impl std::error::Error for ConfigError {}

/// The on-disk wallpaper configuration - the Rust mirror of the C++
/// `WallpaperConfig`.
///
/// Every field has a default, so a v0/legacy file (or a file missing
/// fields) migrates without crashing.
#[derive(Clone, Debug, PartialEq)]
pub struct WallpaperConfig {
    /// The schema version - always the current one after a migration.
    pub version: i64,
    /// The wallpaper video. UTF-8 on disk (the JSON form); the C++ keeps a
    /// wide string in memory for the Win32 calls.
    pub video_path: PathBuf,
    /// `cover` | `fill` (legacy alias) | `fit` | `stretch` | `center`.
    pub fit_mode: String,
    /// 0.5 - 2.0 (RESERVED: not applied by the renderer).
    pub speed: f64,
    /// -1 = all screens; >= 0 = that monitor only.
    pub monitor_id: i64,
    /// 16 - 28.
    pub crf: i64,
    /// 0 = monitor native.
    pub resolution_w: i64,
    /// 0 = monitor native.
    pub resolution_h: i64,
    /// 1 - 30.
    pub fps_cap: i64,
    /// Whether the battery policy is active.
    pub battery_saver: bool,
    /// `cap24` | `static` (the DC policy, master-gated by `battery_saver`).
    pub battery_mode: String,
    /// `auto` | `all` (P3L: E-core pin on a hybrid CPU when `auto`).
    pub cpu_affinity: String,
    /// `auto` | `integrated` | `discrete` (P3L: iGPU pin on a hybrid GPU
    /// when `auto`).
    pub gpu_adapter: String,
    /// The display-resilience reconcile mode: `legacy` | `shadow` | `on`.
    pub reconcile_mode: String,
    /// Unload the decode pipeline after this many seconds of continuous
    /// pause. 0 = never unload; the range is 0..3600 s.
    pub pause_unload_after_sec: i64,
}

impl Default for WallpaperConfig {
    /// The C++ default constructor: every field takes its declared default.
    fn default() -> Self {
        WallpaperConfig {
            version: CONFIG_SCHEMA_VERSION,
            video_path: PathBuf::new(),
            fit_mode: "cover".to_string(),
            speed: 1.0,
            monitor_id: -1,
            crf: 22,
            resolution_w: 0,
            resolution_h: 0,
            fps_cap: 24,
            battery_saver: false,
            battery_mode: "cap24".to_string(),
            cpu_affinity: "auto".to_string(),
            gpu_adapter: "auto".to_string(),
            reconcile_mode: "legacy".to_string(),
            pause_unload_after_sec: 30,
        }
    }
}

/// The fields whose absence means the file predates the current schema and
/// has to be rewritten (the C++ `NeedsMigration` field list).
///
/// `reconcile_mode` is deliberately NOT in the list: it is an additive field,
/// so a file without it loads clean (no `.bak`, no version bump) and simply
/// takes the default.
const MIGRATION_FIELDS: [&str; 13] = [
    "video_path",
    "fit_mode",
    "speed",
    "monitor_id",
    "crf",
    "resolution_w",
    "resolution_h",
    "fps_cap",
    "battery_saver",
    "battery_mode",
    "cpu_affinity",
    "gpu_adapter",
    "pause_unload_after_sec",
];

/// Parses a raw config document and migrates it to the current schema.
///
/// The C++ `LoadConfig` splits this into "read the file", "parse", "migrate"
/// and "validate"; this function is the "parse + migrate" half, and
/// [`validate_config`] is the other half. The golden fixtures record the
/// migration alone, so the two are kept apart on purpose.
///
/// # Errors
///
/// * A corrupt document -> `ConfigError("corrupt config JSON: parse error")`.
/// * A root that is not an object -> `ConfigError("config root must be a
///   JSON object")`.
/// * A schema version outside `0..=6` -> `ConfigError("unsupported config
///   schema version: N")`.
/// * A type-corrupt field -> `ConfigError("config field type error: ...")`.
pub fn migrate_config(raw_text: &str) -> Result<WallpaperConfig, ConfigError> {
    let raw: Value = serde_json::from_str(raw_text)
        .map_err(|_| ConfigError("corrupt config JSON: parse error".to_string()))?;
    migrate_config_value(&raw)
}

/// The C++ `MigrateConfig`: turns a parsed JSON object into a
/// [`WallpaperConfig`].
///
/// Missing fields are filled with the defaults. The per-version rules:
///
/// * v0/v1 -> v2 always writes `monitor_id = -1` (the old id had no render
///   effect, so keeping it would newly single out one screen). A v2 file
///   keeps its stored value.
/// * A file older than v3 with `fit_mode = "fill"` normalizes to `"cover"`.
/// * Every file is written at the current schema version.
pub fn migrate_config_value(raw: &Value) -> Result<WallpaperConfig, ConfigError> {
    let object = raw
        .as_object()
        .ok_or_else(|| ConfigError("config root must be a JSON object".to_string()))?;

    let version = read_i64(object, "version", 0, CONTEXT)?;
    if version < 0 {
        return Err(ConfigError(format!(
            "unsupported config schema version: {version}"
        )));
    }
    // A NEWER file (written by a newer engine) is read BEST-EFFORT: the
    // known fields keep their values and the ones this engine does not know
    // fall back to their defaults. The older engine must NOT reset the
    // user's settings to the defaults - that is a data loss the user cannot
    // see coming.
    //
    // The keys this engine does not know are DISCARDED when the file is
    // rewritten at the current schema (the .bak keeps the original bytes
    // until the next write) - the config is a closed schema.
    if version > CONFIG_SCHEMA_VERSION {
        // fall through: the fields below are read with the current defaults
    }

    // The fields are read in the C++ order, so a file with two type-corrupt
    // fields reports the same first error as the C++.
    let defaults = WallpaperConfig::default();
    let video_path = PathBuf::from(read_string(object, "video_path", "", CONTEXT)?);
    let mut fit_mode = read_string(object, "fit_mode", &defaults.fit_mode, CONTEXT)?;
    if version < 3 && fit_mode == "fill" {
        fit_mode = "cover".to_string();
    }

    Ok(WallpaperConfig {
        version: CONFIG_SCHEMA_VERSION,
        video_path,
        fit_mode,
        speed: read_f64(object, "speed", defaults.speed, CONTEXT)?,
        monitor_id: if version < 2 {
            -1
        } else {
            read_i64(object, "monitor_id", defaults.monitor_id, CONTEXT)?
        },
        crf: read_i64(object, "crf", defaults.crf, CONTEXT)?,
        resolution_w: read_i64(object, "resolution_w", defaults.resolution_w, CONTEXT)?,
        resolution_h: read_i64(object, "resolution_h", defaults.resolution_h, CONTEXT)?,
        fps_cap: read_i64(object, "fps_cap", defaults.fps_cap, CONTEXT)?,
        battery_saver: read_bool(object, "battery_saver", defaults.battery_saver, CONTEXT)?,
        battery_mode: read_string(object, "battery_mode", &defaults.battery_mode, CONTEXT)?,
        cpu_affinity: read_string(object, "cpu_affinity", &defaults.cpu_affinity, CONTEXT)?,
        gpu_adapter: read_string(object, "gpu_adapter", &defaults.gpu_adapter, CONTEXT)?,
        reconcile_mode: read_string(object, "reconcile_mode", &defaults.reconcile_mode, CONTEXT)?,
        pause_unload_after_sec: read_i64(
            object,
            "pause_unload_after_sec",
            defaults.pause_unload_after_sec,
            CONTEXT,
        )?,
    })
}

/// Rejects the values outside the accepted ranges - the Rust mirror of the
/// C++ `ValidateConfig`:
///
/// * `crf` not in `[16, 28]`
/// * `speed` not in `[0.5, 2.0]`
/// * `fps_cap` outside `[1, 30]`
/// * a negative resolution
/// * `monitor_id` below -1
/// * an unknown `fit_mode`, `battery_mode`, `cpu_affinity`, `gpu_adapter` or
///   `reconcile_mode`
/// * `pause_unload_after_sec` outside `[0, 3600]`
///
/// The C++ takes a non-const reference "so future normalizing rules can
/// rewrite in place"; the config contract normalizes nothing today, so the
/// Rust signature takes a shared reference.
pub fn validate_config(cfg: &WallpaperConfig) -> Result<(), ConfigError> {
    if cfg.crf < 16 || cfg.crf > 28 {
        return Err(ConfigError(format!(
            "crf out of range [16,28]: {}",
            cfg.crf
        )));
    }
    if cfg.speed < 0.5 || cfg.speed > 2.0 {
        return Err(ConfigError(format!(
            "speed out of range [0.5,2.0]: {}",
            format_cpp_double(cfg.speed)
        )));
    }
    if cfg.fps_cap < 1 || cfg.fps_cap > 30 {
        return Err(ConfigError(format!(
            "fps_cap out of range [1,30]: {}",
            cfg.fps_cap
        )));
    }
    if cfg.resolution_w < 0 || cfg.resolution_h < 0 {
        return Err(ConfigError("resolution must be >= 0".to_string()));
    }
    if cfg.monitor_id < -1 {
        return Err(ConfigError(format!(
            "monitor_id must be >= -1 (-1 = all screens): {}",
            cfg.monitor_id
        )));
    }
    if !matches!(
        cfg.fit_mode.as_str(),
        "cover" | "fill" | "fit" | "stretch" | "center"
    ) {
        return Err(ConfigError(format!("unknown fit_mode: {}", cfg.fit_mode)));
    }
    if !matches!(cfg.battery_mode.as_str(), "cap24" | "static") {
        return Err(ConfigError(format!(
            "unknown battery_mode: {}",
            cfg.battery_mode
        )));
    }
    if !matches!(cfg.cpu_affinity.as_str(), "auto" | "all") {
        return Err(ConfigError(format!(
            "unknown cpu_affinity: {}",
            cfg.cpu_affinity
        )));
    }
    if !matches!(cfg.gpu_adapter.as_str(), "auto" | "integrated" | "discrete") {
        return Err(ConfigError(format!(
            "unknown gpu_adapter: {}",
            cfg.gpu_adapter
        )));
    }
    if !matches!(cfg.reconcile_mode.as_str(), "legacy" | "shadow" | "on") {
        return Err(ConfigError(format!(
            "unknown reconcile_mode: {}",
            cfg.reconcile_mode
        )));
    }
    if cfg.pause_unload_after_sec < 0 || cfg.pause_unload_after_sec > 3600 {
        return Err(ConfigError(format!(
            "pause_unload_after_sec out of range [0,3600]: {}",
            cfg.pause_unload_after_sec
        )));
    }
    Ok(())
}

/// The struct -> on-disk JSON mapping - the Rust mirror of the C++
/// `ConfigToJson`.
///
/// This is the single definition site for the config field set, so the
/// writer and the tests always agree on the key set.
/// The merge that preserves the unknown keys (task I3).
///
/// The result is `known` plus every key of `existing` that `known` does not
/// contain. The KNOWN keys always win: they are what this engine
/// understands. The unknown keys are the fields a NEWER engine wrote - an
/// older engine must not drop them when it rewrites the file.
///
/// A `existing` that is not a JSON object contributes nothing (a corrupt
/// file must not break a save).
///
/// The golden fixture `config.roundtrip.*` pins this, and the C++ side
/// replays the same cases.
/// The cap on what an older engine carries forward from a file it does not
/// fully understand (task J3) - the same 64 KiB the C++ `MergeUnknownKeys`
/// uses. The file read itself is bounded (1 MiB), but every rewrite would
/// copy that payload forever, so the foreign part is bounded. Keys are
/// visited in sorted order, so which ones fit is deterministic on both sides.
pub const MAX_FOREIGN_KEY_BYTES: usize = 64 * 1024;

pub fn merge_unknown_keys(existing: &Value, known: &Value) -> Value {
    let mut out = known.clone();
    let Some(existing) = existing.as_object() else {
        return out;
    };
    let Some(out_obj) = out.as_object_mut() else {
        return out;
    };
    let mut keys: Vec<&String> = existing.keys().collect();
    keys.sort();
    let mut budget = MAX_FOREIGN_KEY_BYTES;
    for key in keys {
        // A foreign value that collides with a KNOWN name is discarded,
        // whatever its type: the known key keeps the known type. No coercion.
        if out_obj.contains_key(key) {
            continue;
        }
        let value = &existing[key];
        // The cost is the on-disk size: the key bytes plus the serialized
        // value bytes - the same measurement the C++ side takes with dump().
        let Ok(serialized) = serde_json::to_string(value) else {
            continue;
        };
        let cost = key.len() + serialized.len();
        if cost > budget {
            continue;
        }
        budget -= cost;
        out_obj.insert(key.clone(), value.clone());
    }
    out
}

pub fn config_to_json(cfg: &WallpaperConfig) -> Value {
    let mut out = Map::new();
    out.insert("version".to_string(), Value::from(cfg.version));
    out.insert(
        "video_path".to_string(),
        Value::from(cfg.video_path.to_string_lossy().into_owned()),
    );
    out.insert("fit_mode".to_string(), Value::from(cfg.fit_mode.clone()));
    out.insert("speed".to_string(), Value::from(cfg.speed));
    out.insert("monitor_id".to_string(), Value::from(cfg.monitor_id));
    out.insert("crf".to_string(), Value::from(cfg.crf));
    out.insert("resolution_w".to_string(), Value::from(cfg.resolution_w));
    out.insert("resolution_h".to_string(), Value::from(cfg.resolution_h));
    out.insert("fps_cap".to_string(), Value::from(cfg.fps_cap));
    out.insert("battery_saver".to_string(), Value::from(cfg.battery_saver));
    out.insert(
        "battery_mode".to_string(),
        Value::from(cfg.battery_mode.clone()),
    );
    out.insert(
        "cpu_affinity".to_string(),
        Value::from(cfg.cpu_affinity.clone()),
    );
    out.insert(
        "gpu_adapter".to_string(),
        Value::from(cfg.gpu_adapter.clone()),
    );
    out.insert(
        "reconcile_mode".to_string(),
        Value::from(cfg.reconcile_mode.clone()),
    );
    out.insert(
        "pause_unload_after_sec".to_string(),
        Value::from(cfg.pause_unload_after_sec),
    );
    Value::Object(out)
}

/// True when `raw` predates the current schema: an explicit older version,
/// or any known field absent (e.g. a v1 file written before
/// `battery_saver` existed). The C++ calls this after the migration
/// succeeded, so the version is already known-good there.
///
/// A version that is not a number cannot be a known-good version, so it is
/// reported as "needs migration" instead of panicking.
pub fn needs_migration(raw: &Value) -> bool {
    let Some(object) = raw.as_object() else {
        return true;
    };
    let version = match object.get("version") {
        Some(Value::Number(number)) => number_to_i64(number),
        _ => 0,
    };
    if version != CONFIG_SCHEMA_VERSION {
        return true;
    }
    MIGRATION_FIELDS
        .iter()
        .any(|field| !object.contains_key(*field))
}

// --- the shared JSON reading rules -----------------------------------------
//
// The C++ leans on nlohmann's `value(key, default)`: a missing key takes
// the default, a present key with the wrong type throws a `type_error`.
// These helpers are the Rust equivalent and are shared with the studio
// settings contract (the C++ `studio_settings.cpp` includes
// `config_schema.hpp` for the same reason).

/// The JSON type name nlohmann uses in its type errors.
pub(crate) fn json_type_name(value: &Value) -> &'static str {
    match value {
        Value::Null => "null",
        Value::Bool(_) => "boolean",
        Value::Number(_) => "number",
        Value::String(_) => "string",
        Value::Array(_) => "array",
        Value::Object(_) => "object",
    }
}

/// The nlohmann `type_error.302` the C++ surfaces as a `ConfigError`.
pub(crate) fn type_error(context: &str, expected: &str, actual: &str) -> ConfigError {
    ConfigError(format!(
        "{context} field type error: [json.exception.type_error.302] type must be {expected}, but is {actual}"
    ))
}

/// Reads an integer field: absent -> the default, wrong type -> an error.
pub(crate) fn read_i64(
    object: &Map<String, Value>,
    key: &str,
    default: i64,
    context: &str,
) -> Result<i64, ConfigError> {
    match object.get(key) {
        None => Ok(default),
        Some(Value::Number(number)) => Ok(number_to_i64(number)),
        Some(other) => Err(type_error(context, "number", json_type_name(other))),
    }
}

/// Reads a floating point field: absent -> the default, wrong type -> an
/// error.
pub(crate) fn read_f64(
    object: &Map<String, Value>,
    key: &str,
    default: f64,
    context: &str,
) -> Result<f64, ConfigError> {
    match object.get(key) {
        None => Ok(default),
        Some(Value::Number(number)) => Ok(number_to_f64(number)),
        Some(other) => Err(type_error(context, "number", json_type_name(other))),
    }
}

/// Reads a boolean field: absent -> the default, wrong type -> an error.
pub(crate) fn read_bool(
    object: &Map<String, Value>,
    key: &str,
    default: bool,
    context: &str,
) -> Result<bool, ConfigError> {
    match object.get(key) {
        None => Ok(default),
        Some(Value::Bool(value)) => Ok(*value),
        Some(other) => Err(type_error(context, "boolean", json_type_name(other))),
    }
}

/// Reads a string field: absent -> the default, wrong type -> an error.
pub(crate) fn read_string(
    object: &Map<String, Value>,
    key: &str,
    default: &str,
    context: &str,
) -> Result<String, ConfigError> {
    match object.get(key) {
        None => Ok(default.to_string()),
        Some(Value::String(value)) => Ok(value.clone()),
        Some(other) => Err(type_error(context, "string", json_type_name(other))),
    }
}

/// The tolerant enum read the C++ uses for the plan-todo-8 string fields: a
/// missing key, a non-string value or a value outside `allowed` all take
/// the default. An old file must never fail to load because of a
/// hand-edited preset name.
pub(crate) fn read_tolerant_enum(
    object: &Map<String, Value>,
    key: &str,
    default: &str,
    allowed: &[&str],
) -> String {
    match object.get(key) {
        Some(Value::String(value)) if allowed.contains(&value.as_str()) => value.clone(),
        _ => default.to_string(),
    }
}

/// The C++ `static_cast<int>` of a JSON number: a float truncates towards
/// zero, an integer passes through. Rust saturates where the C++ would
/// overflow, so an absurd on-disk value can never panic.
fn number_to_i64(number: &serde_json::Number) -> i64 {
    if let Some(value) = number.as_i64() {
        value
    } else if let Some(value) = number.as_u64() {
        i64::try_from(value).unwrap_or(i64::MAX)
    } else {
        // A float: the C++ `static_cast<int>` truncates towards zero. Rust's
        // `as` does the same and saturates instead of overflowing.
        number_to_f64(number) as i64
    }
}

/// The C++ `static_cast<double>` of a JSON number.
fn number_to_f64(number: &serde_json::Number) -> f64 {
    number.as_f64().unwrap_or(0.0)
}

/// The C++ `std::to_string(double)`: six decimals, so an error message
/// reads the same on both sides.
fn format_cpp_double(value: f64) -> String {
    format!("{value:.6}")
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn the_default_is_the_cpp_default() {
        let cfg = WallpaperConfig::default();
        assert_eq!(cfg.version, 6);
        assert_eq!(cfg.fit_mode, "cover");
        assert_eq!(cfg.speed, 1.0);
        assert_eq!(cfg.monitor_id, -1);
        assert_eq!(cfg.crf, 22);
        assert_eq!(cfg.fps_cap, 24);
        assert_eq!(cfg.battery_mode, "cap24");
        assert_eq!(cfg.cpu_affinity, "auto");
        assert_eq!(cfg.gpu_adapter, "auto");
        assert_eq!(cfg.reconcile_mode, "legacy");
        assert_eq!(cfg.pause_unload_after_sec, 30);
        assert!(validate_config(&cfg).is_ok());
    }

    #[test]
    fn a_missing_key_takes_the_default() {
        let cfg = migrate_config("{}").expect("the empty config must migrate");
        assert_eq!(cfg.crf, 22);
        assert_eq!(cfg.version, CONFIG_SCHEMA_VERSION);
    }

    #[test]
    fn a_wrong_type_is_an_error_not_a_default() {
        let err = migrate_config(r#"{"version":5,"speed":"fast"}"#).expect_err("must fail");
        assert_eq!(
            err.to_string(),
            "config field type error: [json.exception.type_error.302] type must be number, but is string"
        );
        let err = migrate_config(r#"{"version":"5"}"#).expect_err("must fail");
        assert!(err
            .to_string()
            .contains("type must be number, but is string"));
    }

    #[test]
    fn a_corrupt_version_is_an_error_and_a_newer_one_is_not() {
        // A NEGATIVE version is corrupt: it must fail.
        let err = migrate_config(r#"{"version":-1}"#).expect_err("must fail");
        assert_eq!(err.to_string(), "unsupported config schema version: -1");
        // A NEWER version (written by a newer engine) is read BEST-EFFORT:
        // the known fields keep their values, the unknown ones take their
        // defaults. The older engine must NOT reset the user's settings to
        // the defaults (task G4).
        let cfg = migrate_config(r#"{"version":7,"video_path":"C:\\v\\new.mp4","crf":19}"#)
            .expect("a newer file must migrate");
        assert_eq!(cfg.version, CONFIG_SCHEMA_VERSION);
        assert_eq!(cfg.video_path, std::path::PathBuf::from("C:\\v\\new.mp4"));
        assert_eq!(cfg.crf, 19);
    }

    #[test]
    fn a_corrupt_document_is_an_error() {
        let err = migrate_config(r#"{"version":5,"#).expect_err("must fail");
        assert_eq!(err.to_string(), "corrupt config JSON: parse error");
        let err = migrate_config("[1,2,3]").expect_err("must fail");
        assert_eq!(err.to_string(), "config root must be a JSON object");
    }

    #[test]
    fn a_v0_or_v1_file_forces_the_monitor_id() {
        let cfg = migrate_config(r#"{"version":1,"monitor_id":2}"#).expect("must migrate");
        assert_eq!(cfg.monitor_id, -1);
        let cfg = migrate_config(r#"{"version":2,"monitor_id":1}"#).expect("must migrate");
        assert_eq!(cfg.monitor_id, 1);
    }

    #[test]
    fn a_legacy_fill_becomes_cover() {
        let cfg = migrate_config(r#"{"version":2,"fit_mode":"fill"}"#).expect("must migrate");
        assert_eq!(cfg.fit_mode, "cover");
        let cfg = migrate_config(r#"{"version":3,"fit_mode":"fill"}"#).expect("must migrate");
        assert_eq!(cfg.fit_mode, "fill");
    }

    #[test]
    fn the_validation_ranges_are_the_cpp_ones() {
        let bad_crf = WallpaperConfig {
            crf: 15,
            ..WallpaperConfig::default()
        };
        assert!(validate_config(&bad_crf).is_err());
        let bad_speed = WallpaperConfig {
            speed: 0.4,
            ..WallpaperConfig::default()
        };
        assert!(validate_config(&bad_speed).is_err());
        let bad_fps = WallpaperConfig {
            fps_cap: 31,
            ..WallpaperConfig::default()
        };
        assert!(validate_config(&bad_fps).is_err());
        let bad_monitor = WallpaperConfig {
            monitor_id: -2,
            ..WallpaperConfig::default()
        };
        assert!(validate_config(&bad_monitor).is_err());
        let bad_fit = WallpaperConfig {
            fit_mode: "squish".to_string(),
            ..WallpaperConfig::default()
        };
        assert!(validate_config(&bad_fit).is_err());
        let bad_pause = WallpaperConfig {
            pause_unload_after_sec: 3601,
            ..WallpaperConfig::default()
        };
        assert!(validate_config(&bad_pause).is_err());
    }

    #[test]
    fn a_migrated_config_may_still_be_invalid() {
        // The C++ migration does NOT validate: an out-of-range value is
        // carried over and only `ValidateConfig` rejects it. The golden
        // fixture records exactly that.
        let cfg = migrate_config(r#"{"version":5,"crf":15}"#).expect("must migrate");
        assert_eq!(cfg.crf, 15);
        assert!(validate_config(&cfg).is_err());
    }

    #[test]
    fn the_migration_trigger_matches_the_cpp_field_list() {
        let raw: Value = serde_json::from_str("{}").expect("json");
        assert!(needs_migration(&raw));
        let raw: Value = serde_json::from_str(r#"{"version":6}"#).expect("json");
        assert!(needs_migration(&raw));
        // `reconcile_mode` is additive: it never triggers a migration.
        let raw: Value = serde_json::from_str(
            r#"{"version":6,"video_path":"","fit_mode":"cover","speed":1.0,"monitor_id":-1,
                "crf":22,"resolution_w":0,"resolution_h":0,"fps_cap":24,"battery_saver":false,
                "battery_mode":"cap24","cpu_affinity":"auto","gpu_adapter":"auto",
                "pause_unload_after_sec":30}"#,
        )
        .expect("json");
        assert!(!needs_migration(&raw));
    }
}
