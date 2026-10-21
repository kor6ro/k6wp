//! The library index codec (`library.json`).
//!
//! This is the C++ contract from `studio/src/library_json_codec.cpp` and
//! `studio/src/library_manager.cpp`, byte-for-byte in behaviour: the same
//! keys, the same defaults and the same "missing key = default" rule. The
//! Studio's library must mean exactly the same thing on both sides, so
//! the codec is shared, not re-invented.
//!
//! The file shape:
//!
//! ```json
//! { "version": 1, "entries": [ { "src": "...", "dst": "...", ... } ] }
//! ```

use serde::{Deserialize, Serialize};
use std::path::PathBuf;

/// The on-disk schema version of `library.json`.
pub const LIBRARY_SCHEMA_VERSION: u32 = 1;

/// One library entry - the Rust mirror of the C++ `LibraryEntry`.
#[derive(Clone, Debug, Default, PartialEq, Serialize, Deserialize)]
pub struct LibraryEntry {
    /// The original source path (the identity for the cache key).
    #[serde(default)]
    pub src: PathBuf,
    /// The library copy (the unique key).
    #[serde(default)]
    pub dst: PathBuf,
    /// e.g. "1920x1080" (cache key material).
    #[serde(default = "default_res")]
    pub res: String,
    /// Cache key material.
    #[serde(default = "default_fps")]
    pub fps: i64,
    /// Cache key material.
    #[serde(default = "default_crf")]
    pub crf: i64,
    /// Cache key material.
    #[serde(default = "default_encoder")]
    pub encoder: String,
    /// The copy's last-write time (file-time ticks, informational).
    #[serde(default)]
    pub mtime: i64,
    /// The copy's size in bytes (informational).
    #[serde(default)]
    pub size: u64,
    /// The probed duration in seconds (0 = unknown).
    #[serde(default)]
    pub duration: f64,
    /// e.g. "h264", "hevc", "vp9".
    #[serde(default)]
    pub codec: String,
    #[serde(default)]
    pub width: i64,
    #[serde(default)]
    pub height: i64,
    /// The cached thumbnail (a .jpg sibling); may be empty.
    #[serde(default)]
    pub thumb: PathBuf,
    /// Computed: the copy is missing on disk.
    #[serde(default)]
    pub broken: bool,
}

fn default_res() -> String {
    "0x0".to_string()
}
fn default_fps() -> i64 {
    30
}
fn default_crf() -> i64 {
    23
}
fn default_encoder() -> String {
    "libx264".to_string()
}

/// The whole index file.
#[derive(Clone, Debug, Default, PartialEq, Serialize, Deserialize)]
pub struct LibraryIndex {
    #[serde(default = "default_library_version")]
    pub version: u32,
    #[serde(default)]
    pub entries: Vec<LibraryEntry>,
}

fn default_library_version() -> u32 {
    LIBRARY_SCHEMA_VERSION
}

/// The error the codec reports - the Rust mirror of the C++ `LibraryError`.
#[derive(Debug)]
pub enum LibraryError {
    /// The file is not valid JSON.
    Json(String),
    /// The shape is wrong (e.g. "entries" is not an array).
    Shape(String),
    /// The file could not be read.
    Io(String),
}

impl std::fmt::Display for LibraryError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            LibraryError::Json(m) => write!(f, "library json: {m}"),
            LibraryError::Shape(m) => write!(f, "library shape: {m}"),
            LibraryError::Io(m) => write!(f, "library io: {m}"),
        }
    }
}

impl std::error::Error for LibraryError {}

impl LibraryIndex {
    /// Parses the index file.
    ///
    /// The C++ side throws `LibraryError` for a malformed file; the Rust
    /// side returns the same three kinds of error, so the callers report
    /// the same messages.
    pub fn parse(text: &str) -> Result<Self, LibraryError> {
        let value: serde_json::Value =
            serde_json::from_str(text).map_err(|e| LibraryError::Json(e.to_string()))?;
        if !value.is_object() {
            return Err(LibraryError::Shape("the root must be an object".into()));
        }
        let items: Vec<serde_json::Value> = match value.get("entries") {
            None => Vec::new(),
            Some(v) if v.is_array() => v.as_array().cloned().unwrap_or_default(),
            Some(_) => {
                return Err(LibraryError::Shape(
                    "library \"entries\" must be an array".into(),
                ))
            }
        };
        let version = value
            .get("version")
            .and_then(|v| v.as_u64())
            .unwrap_or(LIBRARY_SCHEMA_VERSION as u64) as u32;
        let mut out = LibraryIndex {
            version,
            entries: Vec::with_capacity(items.len()),
        };
        for (i, item) in items.iter().enumerate() {
            if !item.is_object() {
                return Err(LibraryError::Shape(format!("entry {i} must be an object")));
            }
            // A missing key takes the default; a WRONG TYPE is an error -
            // the same rule the C++ codec applies (nlohmann's value() with
            // a default throws on a type mismatch).
            out.entries.push(
                serde_json::from_value::<LibraryEntry>(item.clone())
                    .map_err(|e| LibraryError::Shape(format!("entry {i}: {e}")))?,
            );
        }
        Ok(out)
    }

    /// Serializes the index (the key set and the field names are the C++
    /// contract's).
    pub fn to_json(&self) -> String {
        serde_json::to_string_pretty(self).unwrap_or_else(|_| "{}".to_string())
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn parses_the_cpp_shape() {
        let text = r#"{"version":1,"entries":[{"src":"C:/a.mp4","dst":"C:/b.mp4",
            "res":"1920x1080","fps":30,"crf":23,"encoder":"libx264","mtime":7,
            "size":123,"duration":12.5,"codec":"h264","width":1920,"height":1080,
            "thumb":"C:/b.jpg","broken":false}]}"#;
        let index = LibraryIndex::parse(text).expect("the index must parse");
        assert_eq!(index.version, 1);
        assert_eq!(index.entries.len(), 1);
        let e = &index.entries[0];
        assert_eq!(e.res, "1920x1080");
        assert_eq!(e.codec, "h264");
        assert_eq!(e.width, 1920);
        assert_eq!(e.duration, 12.5);
        assert!(!e.broken);
    }

    #[test]
    fn missing_keys_take_the_defaults() {
        let index = LibraryIndex::parse(r#"{"entries":[{}]}"#).expect("parse");
        let e = &index.entries[0];
        assert_eq!(e.res, "0x0");
        assert_eq!(e.fps, 30);
        assert_eq!(e.crf, 23);
        assert_eq!(e.encoder, "libx264");
        assert_eq!(e.width, 0);
        assert!(!e.broken);
    }

    #[test]
    fn a_missing_entries_array_is_empty() {
        let index = LibraryIndex::parse(r#"{"version":1}"#).expect("parse");
        assert!(index.entries.is_empty());
    }

    #[test]
    fn a_non_array_entries_is_an_error() {
        let err = LibraryIndex::parse(r#"{"entries":{}}"#).expect_err("must fail");
        assert!(matches!(err, LibraryError::Shape(_)));
    }

    #[test]
    fn broken_json_is_an_error() {
        let err = LibraryIndex::parse("{not json").expect_err("must fail");
        assert!(matches!(err, LibraryError::Json(_)));
    }

    #[test]
    fn the_round_trip_keeps_the_fields() {
        let text = r#"{"version":1,"entries":[{"src":"a","dst":"b","res":"1x1","fps":24,
            "crf":22,"encoder":"libx264","mtime":1,"size":2,"duration":3.5,
            "codec":"h264","width":4,"height":5,"thumb":"t","broken":true}]}"#;
        let index = LibraryIndex::parse(text).expect("parse");
        let again = LibraryIndex::parse(&index.to_json()).expect("re-parse");
        assert_eq!(index, again);
    }
}
