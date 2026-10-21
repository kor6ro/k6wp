//! The per-monitor assignment store (`displays.json`).
//!
//! The Rust mirror of the frozen C++ contract in
//! `shared/displays_schema.hpp` / `shared/displays_schema.cpp`: the same
//! keys, the same defaults, the same migration and - the important part -
//! the same error behaviour. The golden fixtures
//! (`tests/golden/shared_behavior.json`, the `displays.*` cases) are replayed
//! against both sides.
//!
//! The on-disk shape (the engine reads it, Studio writes it):
//!
//! ```text
//! {
//!   "version": 1,
//!   "assignments": {
//!     "\\\\.\\DISPLAY1": { "path": "C:\\Videos\\a.mp4", "exists": true,
//!                         "key_stable": "ccd:abc" },
//!     "\\\\.\\DISPLAY2": { "path": "C:\\Videos\\b.webm", "exists": false }
//!   },
//!   "displays": [ /* display-only metadata; the engine ignores it */ ]
//! }
//! ```
//!
//! The rules the C++ enforces, and this port keeps:
//!
//! - a MISSING key takes the C++ default; a WRONG TYPE is an error
//!   (nlohmann's `value()` / `get<>()` throw on a type mismatch, and the C++
//!   turns that into a `ConfigError`) - never a silent default;
//! - `key_stable` and `key_aliases` are ADDITIVE and LENIENT: a wrong type
//!   there is ignored, so a hand-edited or legacy file still loads;
//! - `version` outside `[0, 1]` is rejected.

use std::collections::BTreeMap;

use serde_json::{Map, Value};

pub use crate::config::ConfigError;

/// The on-disk schema version of `displays.json` (the C++
/// `kDisplaysSchemaVersion`).
pub const DISPLAYS_SCHEMA_VERSION: u32 = 1;

/// The upper bound on assignment entries (the C++ `kDisplaysMaxEntries`).
/// Paths are strings, so 100 keeps the serialized file well under the 1 MiB
/// config limit even with long paths.
pub const DISPLAYS_MAX_ENTRIES: usize = 100;

/// The nlohmann type name of a JSON value (the C++ `type_name()`).
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

/// The C++ `type_error.302` message, wrapped the way the C++ wraps it.
pub(crate) fn type_error(context: &str, expected: &str, actual: &Value) -> ConfigError {
    ConfigError(format!(
        "{context} field type error: [json.exception.type_error.302] \
         type must be {expected}, but is {}",
        json_type_name(actual)
    ))
}

/// The nlohmann `get<int>()` conversion: integers pass, a float truncates
/// toward zero, anything else is a type error.
pub(crate) fn as_int(value: &Value) -> Option<i64> {
    match value {
        Value::Number(n) => {
            if let Some(i) = n.as_i64() {
                Some(i)
            } else if let Some(u) = n.as_u64() {
                Some(u as i64)
            } else {
                n.as_f64().map(|f| f as i64)
            }
        }
        _ => None,
    }
}

/// One per-monitor assignment: the video file that plays on the monitor
/// whose GDI device name is the map key.
#[derive(Clone, Debug, Default, PartialEq, Eq)]
pub struct DisplayAssignment {
    /// The absolute path of the video file (UTF-8, as on disk).
    pub path: String,
    /// The Studio-side existence hint at write time. The engine re-checks the
    /// filesystem at apply time and does not trust this flag.
    pub exists: bool,
    /// The tiered stable identity (`ccd:...`, `edid:...:1a2b`). Empty = a
    /// legacy entry.
    pub key_stable: String,
}

/// The whole assignment store.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Displays {
    /// Always the current schema version after a migration.
    pub version: u32,
    /// Keyed by the GDI device name (e.g. `\\.\DISPLAY1`). The map is sorted
    /// so the serialization order is deterministic across runs, exactly like
    /// the C++ `std::map`.
    pub assignments: BTreeMap<String, DisplayAssignment>,
}

impl Default for Displays {
    fn default() -> Self {
        Self {
            version: DISPLAYS_SCHEMA_VERSION,
            assignments: BTreeMap::new(),
        }
    }
}

/// Migrates a raw `displays.json` document into the store.
///
/// This is the Rust mirror of the C++ `MigrateDisplays`. The C++ takes an
/// already-parsed `nlohmann::json`; the golden fixtures hand the replayer
/// the RAW document text, so the Rust side parses it here and reports a
/// corrupt document the way the C++ loader does.
///
/// # Errors
///
/// - a corrupt document (`corrupt displays JSON: parse error`);
/// - a root that is not an object (`displays root must be a JSON object`);
/// - a version outside `[0, 1]` (`unsupported displays schema version: N`);
/// - `assignments` that is not an object, an assignment that is not an
///   object, a missing/non-string `path`, a non-array `displays`, or a
///   wrong-typed `version` / `exists`.
pub fn migrate_displays(raw_text: &str) -> Result<Displays, ConfigError> {
    let raw: Value = serde_json::from_str(raw_text)
        .map_err(|_| ConfigError("corrupt displays JSON: parse error".to_string()))?;
    migrate_displays_value(&raw)
}

/// The same migration, on an already-parsed document.
pub fn migrate_displays_value(raw: &Value) -> Result<Displays, ConfigError> {
    let root = raw
        .as_object()
        .ok_or_else(|| ConfigError("displays root must be a JSON object".to_string()))?;

    // The C++ `raw.value("version", 0)`: a missing key defaults to 0, a
    // wrong type is a type error.
    let version = match root.get("version") {
        None => 0i64,
        Some(value) => as_int(value).ok_or_else(|| type_error("displays", "number", value))?,
    };
    if version < 0 || version > DISPLAYS_SCHEMA_VERSION as i64 {
        return Err(ConfigError(format!(
            "unsupported displays schema version: {version}"
        )));
    }

    let mut assignments = BTreeMap::new();

    if let Some(raw_assignments) = root.get("assignments") {
        let map = raw_assignments
            .as_object()
            .ok_or_else(|| ConfigError("displays assignments must be an object".to_string()))?;
        for (key, entry) in map {
            let entry = entry.as_object().ok_or_else(|| {
                ConfigError("displays assignment value must be an object".to_string())
            })?;
            let path = match entry.get("path") {
                Some(value) => value.as_str().ok_or_else(|| {
                    ConfigError("displays assignment path must be a string".to_string())
                })?,
                None => {
                    return Err(ConfigError(
                        "displays assignment path must be a string".to_string(),
                    ))
                }
            };
            // The C++ `value("exists", false)`: a missing key is false, a
            // wrong type is a type error.
            let exists = match entry.get("exists") {
                None => false,
                Some(value) => value
                    .as_bool()
                    .ok_or_else(|| type_error("displays", "boolean", value))?,
            };
            // `key_stable` is additive and lenient: a wrong type is ignored
            // (never a throw), so a legacy file still loads.
            let key_stable = match entry.get("key_stable") {
                Some(Value::String(text)) => text.clone(),
                _ => String::new(),
            };
            assignments.insert(
                key.clone(),
                DisplayAssignment {
                    path: path.to_string(),
                    exists,
                    key_stable,
                },
            );
        }
    }

    if let Some(displays) = root.get("displays") {
        if !displays.is_array() {
            return Err(ConfigError("displays must be an array".to_string()));
        }
    }

    // `key_aliases` is additive and lenient: the engine never reads the
    // map, so it is not carried in the store, and the C++ ignores a
    // missing or wrong-typed value as well (never a throw). Nothing to do
    // here - and nothing that could fail.

    Ok(Displays {
        version: DISPLAYS_SCHEMA_VERSION,
        assignments,
    })
}

/// The store -> JSON mapping, in the shape the golden fixtures record.
///
/// The C++ dump writes `version` plus the `assignments` map, and every
/// assignment carries `path`, `exists` and `key_stable` (the last one even
/// when it is empty - that is what the frozen fixture holds).
pub fn displays_json(displays: &Displays) -> Value {
    let mut assignments = Map::new();
    for (key, assignment) in &displays.assignments {
        let mut entry = Map::new();
        entry.insert("path".to_string(), Value::String(assignment.path.clone()));
        entry.insert("exists".to_string(), Value::Bool(assignment.exists));
        entry.insert(
            "key_stable".to_string(),
            Value::String(assignment.key_stable.clone()),
        );
        assignments.insert(key.clone(), Value::Object(entry));
    }
    let mut out = Map::new();
    out.insert("version".to_string(), Value::from(displays.version));
    out.insert("assignments".to_string(), Value::Object(assignments));
    Value::Object(out)
}

/// True when any component of `path` is exactly `..`.
///
/// A `..` inside a file name ("foo..bar") is fine; only a traversal
/// component is rejected, so a hostile assignment cannot escape the
/// wallpapers root. This is the C++ `HasDotDotComponent`.
fn has_dot_dot_component(path: &str) -> bool {
    path.split(['/', '\\']).any(|part| part == "..")
}

/// Rejects the values outside the accepted ranges - the C++
/// `ValidateDisplays`.
///
/// # Errors
///
/// - a version outside `[0, 1]`;
/// - more than `DISPLAYS_MAX_ENTRIES` assignments;
/// - an empty assignment path;
/// - an assignment path with a `..` component.
pub fn validate_displays(displays: &Displays) -> Result<(), ConfigError> {
    if displays.version > DISPLAYS_SCHEMA_VERSION {
        return Err(ConfigError(format!(
            "displays schema version out of range [0,{DISPLAYS_SCHEMA_VERSION}]: {}",
            displays.version
        )));
    }
    if displays.assignments.len() > DISPLAYS_MAX_ENTRIES {
        return Err(ConfigError(format!(
            "displays exceeds maximum entries ({DISPLAYS_MAX_ENTRIES})"
        )));
    }
    for (key, assignment) in &displays.assignments {
        if assignment.path.is_empty() {
            return Err(ConfigError(format!(
                "displays assignment path must be non-empty (key {key})"
            )));
        }
        if has_dot_dot_component(&assignment.path) {
            return Err(ConfigError(format!(
                "displays assignment path must not contain '..': {}",
                assignment.path
            )));
        }
    }
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn a_v0_document_migrates_to_the_current_version() {
        let displays = migrate_displays(r#"{"version":0}"#).expect("must migrate");
        assert_eq!(displays.version, DISPLAYS_SCHEMA_VERSION);
        assert!(displays.assignments.is_empty());
    }

    #[test]
    fn the_assignments_keep_their_shape() {
        let displays = migrate_displays(
            r#"{"version":1,"assignments":{"\\\\.\\DISPLAY1":{"path":"C:\\a.mp4","exists":true},
                 "\\\\.\\DISPLAY2":{"path":"C:\\b.mp4","exists":false,"key_stable":"ccd:abc"}}}"#,
        )
        .expect("must migrate");
        assert_eq!(displays.assignments.len(), 2);
        let first = &displays.assignments["\\\\.\\DISPLAY1"];
        assert_eq!(first.path, "C:\\a.mp4");
        assert!(first.exists);
        assert_eq!(first.key_stable, "");
        let second = &displays.assignments["\\\\.\\DISPLAY2"];
        assert_eq!(second.key_stable, "ccd:abc");
        assert!(!second.exists);
    }

    #[test]
    fn a_future_version_is_rejected() {
        let err = migrate_displays(r#"{"version":2}"#).expect_err("must fail");
        assert_eq!(err.0, "unsupported displays schema version: 2");
    }

    #[test]
    fn a_non_object_assignment_is_rejected() {
        let err =
            migrate_displays(r#"{"version":1,"assignments":{"a":"oops"}}"#).expect_err("must fail");
        assert_eq!(err.0, "displays assignment value must be an object");
    }

    #[test]
    fn a_missing_path_is_rejected() {
        let err = migrate_displays(r#"{"version":1,"assignments":{"a":{"exists":true}}}"#)
            .expect_err("must fail");
        assert_eq!(err.0, "displays assignment path must be a string");
    }

    #[test]
    fn a_wrong_typed_exists_is_rejected() {
        let err =
            migrate_displays(r#"{"version":1,"assignments":{"a":{"path":"p","exists":"yes"}}}"#)
                .expect_err("must fail");
        assert_eq!(
            err.0,
            "displays field type error: [json.exception.type_error.302] \
             type must be boolean, but is string"
        );
    }

    #[test]
    fn a_wrong_typed_key_stable_is_ignored() {
        let displays =
            migrate_displays(r#"{"version":1,"assignments":{"a":{"path":"p","key_stable":7}}}"#)
                .expect("must migrate");
        assert_eq!(displays.assignments["a"].key_stable, "");
    }

    #[test]
    fn a_non_array_displays_is_rejected() {
        let err = migrate_displays(r#"{"version":1,"displays":{}}"#).expect_err("must fail");
        assert_eq!(err.0, "displays must be an array");
    }

    #[test]
    fn a_corrupt_document_is_rejected() {
        let err = migrate_displays(r#"{"version":1,"#).expect_err("must fail");
        assert_eq!(err.0, "corrupt displays JSON: parse error");
    }

    #[test]
    fn the_json_mapping_carries_every_field() {
        let displays = Displays {
            version: 1,
            assignments: BTreeMap::from([(
                "d1".to_string(),
                DisplayAssignment {
                    path: "C:\\a.mp4".to_string(),
                    exists: true,
                    key_stable: "ccd:abc".to_string(),
                },
            )]),
        };
        let json = displays_json(&displays);
        assert_eq!(json["version"], Value::from(1));
        assert_eq!(json["assignments"]["d1"]["path"], Value::from("C:\\a.mp4"));
        assert_eq!(json["assignments"]["d1"]["exists"], Value::from(true));
        assert_eq!(
            json["assignments"]["d1"]["key_stable"],
            Value::from("ccd:abc")
        );
    }

    #[test]
    fn the_validator_rejects_a_traversal() {
        let displays = Displays {
            version: 1,
            assignments: BTreeMap::from([(
                "d1".to_string(),
                DisplayAssignment {
                    path: "C:\\a\\..\\b.mp4".to_string(),
                    exists: false,
                    key_stable: String::new(),
                },
            )]),
        };
        assert!(validate_displays(&displays).is_err());
    }
}
