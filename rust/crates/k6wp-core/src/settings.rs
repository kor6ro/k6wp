//! The Studio settings schema (`studio_settings.json`).
//!
//! This is the Rust mirror of the C++ contract in
//! `shared/studio_settings.hpp` and `shared/studio_settings.cpp`. The
//! behaviour is FROZEN: the same keys, the same defaults, the same
//! per-version migration rules and the same error cases. The C++ side stays
//! the reference until the cut-over, and the golden fixtures
//! (`tests/golden/shared_behavior.json`) are replayed against both sides.
//!
//! These are the STUDIO preferences, deliberately separate from the
//! wallpaper configuration (`config.json`): the Engine never reads this
//! file and the Studio never writes playback state here.
//!
//! The reading rules are the same as the config contract's: a missing key
//! takes the C++ default, a key with the wrong type is an error - except
//! for the two plan-todo-8 enum fields (`playlistSource` and
//! `performancePreset`), which are read tolerantly and normalize to their
//! safe default instead of failing the load.

use std::fs;
use std::path::{Path, PathBuf};

use serde_json::{Map, Value};

use crate::config::merge_unknown_keys;

pub use crate::config::ConfigError;

use crate::config::{read_bool, read_f64, read_i64, read_string, read_tolerant_enum};

/// The on-disk schema version of `studio_settings.json` (the C++
/// `kStudioSettingsSchemaVersion`).
pub const STUDIO_SETTINGS_SCHEMA_VERSION: i64 = 5;

/// The error context the settings contract uses in its type errors.
const CONTEXT: &str = "studio settings";

/// The Studio-side preferences - the Rust mirror of the C++
/// `StudioSettings`.
#[derive(Clone, Debug, PartialEq)]
pub struct StudioSettings {
    /// The schema version - always the current one after a migration.
    pub version: i64,
    /// Compress a video right after it is imported.
    pub auto_compress_on_import: bool,
    /// Where the compressor writes its output (default
    /// `%LOCALAPPDATA%/K6WP/wallpapers`).
    pub compress_output_dir: PathBuf,
    /// 16 - 28 (agrees with the T7 CRF 22 default).
    pub default_crf: i64,
    /// 1 - 30.
    pub default_fps: i64,
    /// `match_monitor` | `source` | `720p` | `1080p` | `2160p`.
    pub default_resolution_mode: String,
    /// RESERVED: the autostart is read from HKCU Run, not from this field.
    pub start_with_windows: bool,
    /// The cache root (default `%LOCALAPPDATA%/K6WP/cache`).
    pub cache_dir: PathBuf,
    /// Apply the first frame as the desktop wallpaper.
    pub wallpaper_sync: bool,
    /// Apply the per-user lock-screen image.
    pub lockscreen_sync: bool,
    /// The frame seek offset in seconds (>= 0). 0.0 = the FIRST frame.
    pub lockscreen_offset_sec: f64,
    /// The one-time lock-screen onboarding offer has been shown.
    pub lockscreen_offer_shown: bool,
    /// The compressor tab's advanced box is expanded (session UI only).
    pub compress_advanced_visible: bool,
    /// Check for a new version on start (Studio only: it never downloads).
    pub check_updates: bool,
    /// Exit to the tray instead of quitting (the JSON key is `closeToTray`).
    pub close_to_tray: bool,
    /// The rotation source, `all` | `custom` (the JSON key is
    /// `playlistSource`).
    pub playlist_source: String,
    /// `Hemat` | `Seimbang` | `Maksimal` (the JSON key is
    /// `performancePreset`).
    pub performance_preset: String,
}

impl Default for StudioSettings {
    /// The C++ `DefaultStudioSettings()`: the declared defaults with the two
    /// environment-resolved directories filled in.
    ///
    /// Note the C++ has two "defaults": the default *constructor* leaves the
    /// two directories empty, while `DefaultStudioSettings()` resolves them
    /// from the environment. The frozen fixture records the latter, so this
    /// is the one the Rust `Default` mirrors.
    fn default() -> Self {
        StudioSettings {
            version: STUDIO_SETTINGS_SCHEMA_VERSION,
            auto_compress_on_import: true,
            compress_output_dir: default_compress_output_dir(),
            default_crf: 22,
            default_fps: 30,
            default_resolution_mode: "match_monitor".to_string(),
            start_with_windows: false,
            cache_dir: default_cache_dir(),
            wallpaper_sync: true,
            lockscreen_sync: true,
            lockscreen_offset_sec: 0.0,
            lockscreen_offer_shown: false,
            compress_advanced_visible: false,
            check_updates: true,
            close_to_tray: false,
            playlist_source: "all".to_string(),
            performance_preset: "Seimbang".to_string(),
        }
    }
}

/// The default compressor output directory - `%LOCALAPPDATA%/K6WP/wallpapers`
/// (the C++ `DefaultCompressOutputDir`).
pub fn default_compress_output_dir() -> PathBuf {
    k6wp_local_dir("wallpapers")
}

/// The default cache directory - `%LOCALAPPDATA%/K6WP/cache` (the C++
/// `DefaultStudioCacheDir`).
pub fn default_cache_dir() -> PathBuf {
    k6wp_local_dir("cache")
}

/// `%LOCALAPPDATA%/K6WP/<leaf>`, falling back to
/// `%USERPROFILE%/AppData/Local/K6WP/<leaf>` and then to the temp
/// directory.
fn k6wp_local_dir(leaf: &str) -> PathBuf {
    if let Some(local_app_data) = non_empty_env("LOCALAPPDATA") {
        return PathBuf::from(local_app_data).join("K6WP").join(leaf);
    }
    if let Some(user_profile) = non_empty_env("USERPROFILE") {
        return PathBuf::from(user_profile)
            .join("AppData")
            .join("Local")
            .join("K6WP")
            .join(leaf);
    }
    std::env::temp_dir().join("K6WP").join(leaf)
}

/// A non-empty environment variable, if it is set.
fn non_empty_env(name: &str) -> Option<String> {
    match std::env::var(name) {
        Ok(value) if !value.is_empty() => Some(value),
        _ => None,
    }
}

/// Parses a raw settings document and migrates it to the current schema.
///
/// # Errors
///
/// * A corrupt document -> `ConfigError("corrupt settings JSON: parse
///   error")`.
/// * A root that is not an object -> `ConfigError("studio settings root
///   must be a JSON object")`.
/// * A schema version outside `0..=5` -> `ConfigError("unsupported studio
///   settings schema version: N")`.
/// * A type-corrupt field -> `ConfigError("studio settings field type
///   error: ...")`.
pub fn migrate_studio_settings(raw_text: &str) -> Result<StudioSettings, ConfigError> {
    let raw: Value = serde_json::from_str(raw_text)
        .map_err(|_| ConfigError("corrupt settings JSON: parse error".to_string()))?;
    migrate_studio_settings_value(&raw)
}

/// The C++ `MigrateStudioSettings`: turns a parsed JSON object into a
/// [`StudioSettings`].
///
/// Missing fields are filled with the defaults. The per-version rules:
///
/// * A file written before v5 keeps both stored sync flags exactly as the
///   user left them; only a `lockscreen_offset_sec` of exactly `1.0` (the
///   old hard-coded default) moves to `0.0` (the first frame).
/// * `keep_original` and `start_minimized` were removed in v3 and are
///   ignored when an old file still carries them.
/// * An out-of-set `playlistSource` / `performancePreset` normalizes to its
///   safe default instead of failing the load.
pub fn migrate_studio_settings_value(raw: &Value) -> Result<StudioSettings, ConfigError> {
    let object = raw
        .as_object()
        .ok_or_else(|| ConfigError("studio settings root must be a JSON object".to_string()))?;

    let defaults = StudioSettings::default();
    let version = read_i64(object, "version", 0, CONTEXT)?;
    if !(0..=STUDIO_SETTINGS_SCHEMA_VERSION).contains(&version) {
        return Err(ConfigError(format!(
            "unsupported studio settings schema version: {version}"
        )));
    }

    let mut settings = defaults.clone();
    settings.version = STUDIO_SETTINGS_SCHEMA_VERSION;
    settings.auto_compress_on_import = read_bool(
        object,
        "auto_compress_on_import",
        defaults.auto_compress_on_import,
        CONTEXT,
    )?;

    // An empty directory means "unset" (a hand-edited file): it resolves to
    // the environment default instead of persisting an unusable empty path.
    let compress_output_dir = read_string(object, "compress_output_dir", "", CONTEXT)?;
    settings.compress_output_dir = if compress_output_dir.is_empty() {
        defaults.compress_output_dir.clone()
    } else {
        PathBuf::from(compress_output_dir)
    };

    settings.default_crf = read_i64(object, "default_crf", defaults.default_crf, CONTEXT)?;
    settings.default_fps = read_i64(object, "default_fps", defaults.default_fps, CONTEXT)?;
    settings.default_resolution_mode = read_string(
        object,
        "default_resolution_mode",
        &defaults.default_resolution_mode,
        CONTEXT,
    )?;
    settings.start_with_windows = read_bool(
        object,
        "start_with_windows",
        defaults.start_with_windows,
        CONTEXT,
    )?;
    settings.wallpaper_sync =
        read_bool(object, "wallpaper_sync", defaults.wallpaper_sync, CONTEXT)?;
    settings.lockscreen_sync =
        read_bool(object, "lockscreen_sync", defaults.lockscreen_sync, CONTEXT)?;
    settings.lockscreen_offset_sec = read_f64(
        object,
        "lockscreen_offset_sec",
        defaults.lockscreen_offset_sec,
        CONTEXT,
    )?;
    // v5 offset migration: 1.0 was the old hard-coded default and 0.0 (the
    // FIRST frame) is the new one. Only an exactly-1.0 value from an old
    // file moves; any other user choice is preserved.
    if version <= 4 && settings.lockscreen_offset_sec == 1.0 {
        settings.lockscreen_offset_sec = 0.0;
    }
    settings.lockscreen_offer_shown = read_bool(
        object,
        "lockscreen_offer_shown",
        defaults.lockscreen_offer_shown,
        CONTEXT,
    )?;
    settings.compress_advanced_visible = read_bool(
        object,
        "compress_advanced_visible",
        defaults.compress_advanced_visible,
        CONTEXT,
    )?;
    settings.check_updates = read_bool(object, "check_updates", defaults.check_updates, CONTEXT)?;
    settings.close_to_tray = read_bool(object, "closeToTray", defaults.close_to_tray, CONTEXT)?;
    settings.playlist_source =
        read_tolerant_enum(object, "playlistSource", "all", &["all", "custom"]);
    settings.performance_preset = read_tolerant_enum(
        object,
        "performancePreset",
        "Seimbang",
        &["Hemat", "Seimbang", "Maksimal"],
    );

    let cache_dir = read_string(object, "cache_dir", "", CONTEXT)?;
    settings.cache_dir = if cache_dir.is_empty() {
        defaults.cache_dir.clone()
    } else {
        PathBuf::from(cache_dir)
    };

    Ok(settings)
}

/// Rejects the values outside the accepted ranges and normalizes the two
/// plan-todo-8 enums in place - the Rust mirror of the C++
/// `ValidateStudioSettings`:
///
/// * `default_crf` not in `[16, 28]`
/// * `default_fps` not in `[1, 30]`
/// * an empty `compress_output_dir` / `cache_dir`
/// * a negative `lockscreen_offset_sec`
/// * an unknown `default_resolution_mode`
/// * a `playlist_source` outside `{all, custom}` -> `"all"`
/// * a `performance_preset` outside `{Hemat, Seimbang, Maksimal}` ->
///   `"Seimbang"`
///
/// The enums normalize and never throw: an out-of-set value from a
/// hand-edited file becomes the safe default instead of failing the whole
/// load. The C++ takes a non-const reference "so future normalizing rules
/// can rewrite in place", and so does the Rust side.
pub fn validate_studio_settings(settings: &mut StudioSettings) -> Result<(), ConfigError> {
    if settings.default_crf < 16 || settings.default_crf > 28 {
        return Err(ConfigError(format!(
            "default_crf out of range [16,28]: {}",
            settings.default_crf
        )));
    }
    if settings.default_fps < 1 || settings.default_fps > 30 {
        return Err(ConfigError(format!(
            "default_fps out of range [1,30]: {}",
            settings.default_fps
        )));
    }
    if settings.compress_output_dir.as_os_str().is_empty() {
        return Err(ConfigError(
            "compress_output_dir must not be empty".to_string(),
        ));
    }
    if settings.cache_dir.as_os_str().is_empty() {
        return Err(ConfigError("cache_dir must not be empty".to_string()));
    }
    if settings.lockscreen_offset_sec < 0.0 {
        return Err(ConfigError(
            "lockscreen_offset_sec must be >= 0".to_string(),
        ));
    }
    if !matches!(
        settings.default_resolution_mode.as_str(),
        "match_monitor" | "source" | "720p" | "1080p" | "2160p"
    ) {
        return Err(ConfigError(format!(
            "unknown default_resolution_mode: {}",
            settings.default_resolution_mode
        )));
    }
    if !matches!(settings.playlist_source.as_str(), "all" | "custom") {
        settings.playlist_source = "all".to_string();
    }
    if !matches!(
        settings.performance_preset.as_str(),
        "Hemat" | "Seimbang" | "Maksimal"
    ) {
        settings.performance_preset = "Seimbang".to_string();
    }
    Ok(())
}

/// The migrated-settings JSON projection - the Rust mirror of the shape the
/// C++ golden dump records for every migrated settings case.
///
/// The C++ records the two environment-dependent directories (and the
/// reserved `start_with_windows` flag) only in the "default settings"
/// report; the migrated-settings projection carries the stable fields
/// only. [`settings_report_json`] is the full report.
pub fn settings_json(settings: &StudioSettings) -> Value {
    let mut out = Map::new();
    out.insert("version".to_string(), Value::from(settings.version));
    out.insert(
        "auto_compress_on_import".to_string(),
        Value::from(settings.auto_compress_on_import),
    );
    out.insert("default_crf".to_string(), Value::from(settings.default_crf));
    out.insert("default_fps".to_string(), Value::from(settings.default_fps));
    out.insert(
        "default_resolution_mode".to_string(),
        Value::from(settings.default_resolution_mode.clone()),
    );
    out.insert(
        "wallpaper_sync".to_string(),
        Value::from(settings.wallpaper_sync),
    );
    out.insert(
        "lockscreen_sync".to_string(),
        Value::from(settings.lockscreen_sync),
    );
    out.insert(
        "lockscreen_offset_sec".to_string(),
        Value::from(settings.lockscreen_offset_sec),
    );
    out.insert(
        "lockscreen_offer_shown".to_string(),
        Value::from(settings.lockscreen_offer_shown),
    );
    out.insert(
        "compress_advanced_visible".to_string(),
        Value::from(settings.compress_advanced_visible),
    );
    out.insert(
        "check_updates".to_string(),
        Value::from(settings.check_updates),
    );
    out.insert(
        "close_to_tray".to_string(),
        Value::from(settings.close_to_tray),
    );
    out.insert(
        "playlist_source".to_string(),
        Value::from(settings.playlist_source.clone()),
    );
    out.insert(
        "performance_preset".to_string(),
        Value::from(settings.performance_preset.clone()),
    );
    Value::Object(out)
}

/// The full settings report - the Rust mirror of the shape the C++ golden
/// dump records for the "default settings" case.
///
/// It adds the reserved `start_with_windows` flag and the two directory
/// basenames. The basenames are recorded as a BYTE ARRAY (the C++
/// `std::wstring` has no nlohmann serializer, so it is serialized as its
/// code units): `"cache"` becomes `[99, 97, 99, 104, 101]`.
pub fn settings_report_json(settings: &StudioSettings) -> Value {
    let mut out = match settings_json(settings) {
        Value::Object(map) => map,
        _ => Map::new(),
    };
    out.insert(
        "start_with_windows".to_string(),
        Value::from(settings.start_with_windows),
    );
    out.insert(
        "compress_output_dir_basename".to_string(),
        basename_bytes(&settings.compress_output_dir),
    );
    out.insert(
        "cache_dir_basename".to_string(),
        basename_bytes(&settings.cache_dir),
    );
    Value::Object(out)
}

/// Crash-safe save of studio settings: serializes via [`settings_json`],
/// merges unknown keys from the existing file (preserving fields from
/// future versions), writes `"<path>.tmp"`, then renames over `path`.
///
/// # Errors
///
/// [`ConfigError`] when validation fails, the temp file cannot be written,
/// or the atomic rename fails.
pub fn save_studio_settings(path: &Path, settings: &StudioSettings) -> Result<(), ConfigError> {
    let mut settings = settings.clone();
    validate_studio_settings(&mut settings)?;
    let existing = read_existing_json(path);
    let known = settings_json(&settings);
    let doc = merge_unknown_keys(&existing, &known);
    if let Some(parent) = path.parent() {
        if !parent.as_os_str().is_empty() {
            fs::create_dir_all(parent)
                .map_err(|e| ConfigError(format!("cannot create settings directory: {e}")))?;
        }
    }
    let mut tmp = path.as_os_str().to_owned();
    tmp.push(".tmp");
    let tmp = PathBuf::from(tmp);
    let text = serde_json::to_string_pretty(&doc)
        .map_err(|e| ConfigError(format!("cannot serialize settings: {e}")))?;
    fs::write(&tmp, text).map_err(|e| {
        ConfigError(format!(
            "cannot write settings temp file: {} ({e})",
            tmp.display()
        ))
    })?;
    fs::rename(&tmp, path).map_err(|e| {
        let _ = fs::remove_file(&tmp);
        ConfigError(format!(
            "atomic settings replace failed: {} ({e})",
            path.display()
        ))
    })
}

/// Read the existing settings file as a JSON object; an empty object when
/// the file is missing, unreadable or not a JSON object.
fn read_existing_json(path: &Path) -> Value {
    fs::read_to_string(path)
        .ok()
        .and_then(|text| serde_json::from_str(&text).ok())
        .filter(Value::is_object)
        .unwrap_or_else(|| Value::Object(Map::new()))
}

/// The last path segment of `path` as an array of code units - the C++
/// `std::filesystem::path(...).filename().wstring()` that the golden dump
/// serializes as a byte array.
///
/// The separators are the Windows ones (`\` and `/`) on every platform, so
/// the value does not depend on where the test runs.
fn basename_bytes(path: &Path) -> Value {
    let text = path.to_string_lossy();
    let trimmed = text.trim_end_matches(['\\', '/']);
    let name = match trimmed.rfind(['\\', '/']) {
        Some(index) => &trimmed[index + 1..],
        None => trimmed,
    };
    // The C++ hands nlohmann a `std::wstring` (UTF-16 on Windows), and
    // nlohmann serializes it as its code units.
    Value::Array(
        name.encode_utf16()
            .map(|unit| Value::from(unit as u32))
            .collect(),
    )
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn the_default_is_the_cpp_default() {
        let settings = StudioSettings::default();
        assert_eq!(settings.version, 5);
        assert!(settings.auto_compress_on_import);
        assert_eq!(settings.default_crf, 22);
        assert_eq!(settings.default_fps, 30);
        assert_eq!(settings.default_resolution_mode, "match_monitor");
        assert!(settings.wallpaper_sync);
        assert!(settings.lockscreen_sync);
        assert_eq!(settings.lockscreen_offset_sec, 0.0);
        assert!(!settings.lockscreen_offer_shown);
        assert!(!settings.compress_advanced_visible);
        assert!(settings.check_updates);
        assert!(!settings.close_to_tray);
        assert_eq!(settings.playlist_source, "all");
        assert_eq!(settings.performance_preset, "Seimbang");
        let mut settings = settings;
        assert!(validate_studio_settings(&mut settings).is_ok());
    }

    #[test]
    fn a_missing_key_takes_the_default() {
        let settings = migrate_studio_settings("{}").expect("the empty file must migrate");
        assert_eq!(settings.default_crf, 22);
        assert_eq!(settings.version, STUDIO_SETTINGS_SCHEMA_VERSION);
    }

    #[test]
    fn a_wrong_type_is_an_error_not_a_default() {
        let err =
            migrate_studio_settings(r#"{"version":5,"default_crf":"x"}"#).expect_err("must fail");
        assert_eq!(
            err.to_string(),
            "studio settings field type error: [json.exception.type_error.302] type must be number, but is string"
        );
    }

    #[test]
    fn an_unknown_version_is_an_error() {
        let err = migrate_studio_settings(r#"{"version":6}"#).expect_err("must fail");
        assert_eq!(
            err.to_string(),
            "unsupported studio settings schema version: 6"
        );
    }

    #[test]
    fn the_v5_offset_rule_moves_only_an_exactly_one() {
        let settings = migrate_studio_settings(r#"{"version":4,"lockscreen_offset_sec":1.0}"#)
            .expect("must migrate");
        assert_eq!(settings.lockscreen_offset_sec, 0.0);
        let settings = migrate_studio_settings(r#"{"version":4,"lockscreen_offset_sec":0.5}"#)
            .expect("must migrate");
        assert_eq!(settings.lockscreen_offset_sec, 0.5);
        let settings = migrate_studio_settings(r#"{"version":5,"lockscreen_offset_sec":1.0}"#)
            .expect("must migrate");
        assert_eq!(settings.lockscreen_offset_sec, 1.0);
    }

    #[test]
    fn the_stored_sync_flags_are_never_flipped() {
        let settings = migrate_studio_settings(r#"{"version":1,"lockscreen_sync":false}"#)
            .expect("must migrate");
        assert!(!settings.lockscreen_sync);
        assert!(settings.wallpaper_sync);
    }

    #[test]
    fn the_plan_todo_8_enums_are_tolerant() {
        let settings = migrate_studio_settings(r#"{"version":5,"playlistSource":"nope"}"#)
            .expect("must migrate");
        assert_eq!(settings.playlist_source, "all");
        let settings = migrate_studio_settings(r#"{"version":5,"performancePreset":"nope"}"#)
            .expect("must migrate");
        assert_eq!(settings.performance_preset, "Seimbang");
        let settings =
            migrate_studio_settings(r#"{"version":5,"playlistSource":42}"#).expect("must migrate");
        assert_eq!(settings.playlist_source, "all");
    }

    #[test]
    fn the_removed_keys_are_ignored() {
        let settings =
            migrate_studio_settings(r#"{"version":3,"keep_original":true,"start_minimized":true}"#)
                .expect("must migrate");
        assert_eq!(settings.default_crf, 22);
    }

    #[test]
    fn the_basename_is_a_byte_array() {
        let settings = StudioSettings::default();
        let report = settings_report_json(&settings);
        assert_eq!(
            report["cache_dir_basename"],
            Value::Array(vec![
                Value::from(99),
                Value::from(97),
                Value::from(99),
                Value::from(104),
                Value::from(101)
            ])
        );
    }

    #[test]
    fn settings_round_trip() {
        let dir =
            std::env::temp_dir().join(format!("k6wp_settings_roundtrip_{}", std::process::id()));
        let _ = fs::remove_dir_all(&dir);
        fs::create_dir_all(&dir).expect("scratch dir");
        let path = dir.join("studio_settings.json");

        let mut settings = StudioSettings::default();
        settings.default_crf = 20;
        settings.default_fps = 24;
        settings.default_resolution_mode = "1080p".to_string();
        settings.wallpaper_sync = false;
        settings.lockscreen_sync = false;
        settings.check_updates = false;

        save_studio_settings(&path, &settings).expect("save must succeed");

        let loaded = migrate_studio_settings(&fs::read_to_string(&path).expect("read settings"))
            .expect("load must succeed");
        assert_eq!(loaded, settings);
        assert_eq!(loaded.default_crf, 20);
        assert_eq!(loaded.default_fps, 24);
        assert_eq!(loaded.default_resolution_mode, "1080p");
        assert!(!loaded.wallpaper_sync);
        assert!(!loaded.lockscreen_sync);
        assert!(!loaded.check_updates);

        let _ = fs::remove_dir_all(&dir);
    }

    #[test]
    fn settings_validation_rejects_bad_crf() {
        let dir = std::env::temp_dir().join(format!("k6wp_settings_badcrf_{}", std::process::id()));
        let _ = fs::remove_dir_all(&dir);
        fs::create_dir_all(&dir).expect("scratch dir");
        let path = dir.join("studio_settings.json");

        let mut settings = StudioSettings::default();
        settings.default_crf = 99;
        let err = save_studio_settings(&path, &settings).expect_err("crf 99 must fail");
        assert!(err.to_string().contains("default_crf out of range"));

        let _ = fs::remove_dir_all(&dir);
    }

    #[test]
    fn settings_validation_rejects_bad_fps() {
        let dir = std::env::temp_dir().join(format!("k6wp_settings_badfps_{}", std::process::id()));
        let _ = fs::remove_dir_all(&dir);
        fs::create_dir_all(&dir).expect("scratch dir");
        let path = dir.join("studio_settings.json");

        let mut settings = StudioSettings::default();
        settings.default_fps = 31;
        let err = save_studio_settings(&path, &settings).expect_err("fps 31 must fail");
        assert!(err.to_string().contains("default_fps out of range"));

        let _ = fs::remove_dir_all(&dir);
    }

    #[test]
    fn settings_save_preserves_unknown_keys() {
        let dir = std::env::temp_dir().join(format!("k6wp_settings_unknownkeys_{}", std::process::id()));
        let _ = fs::remove_dir_all(&dir);
        fs::create_dir_all(&dir).expect("scratch dir");
        let path = dir.join("studio_settings.json");

        // Seed with an unknown key from the future.
        let mut seed = Map::new();
        seed.insert("version".to_string(), Value::from(5));
        seed.insert("auto_compress_on_import".to_string(), Value::from(true));
        seed.insert("future_key".to_string(), Value::from(42));
        fs::write(&path, serde_json::to_string(&seed).unwrap()).expect("write seed");

        let settings = StudioSettings::default();
        save_studio_settings(&path, &settings).expect("save must succeed");

        let doc: Value = serde_json::from_str(&fs::read_to_string(&path).expect("read settings"))
            .expect("valid json");
        assert_eq!(doc["future_key"], 42, "unknown key must be preserved");
        assert_eq!(doc["version"], 5, "version must be current schema");

        let _ = fs::remove_dir_all(&dir);
    }
}
