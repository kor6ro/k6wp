//! The wallpaper playlist (`playlist.json`) and its pure rotation helpers.
//!
//! The Rust mirror of the frozen C++ contract in `shared/playlist.hpp` /
//! `shared/playlist.cpp`: the same keys, the same defaults, the same
//! migration and the same error behaviour. The golden fixtures
//! (`tests/golden/shared_behavior.json`, the `playlist.*` cases) are replayed
//! against both sides.
//!
//! The on-disk shape:
//!
//! ```text
//! { "version": 1, "enabled": false, "interval_min": 30,
//!   "shuffle": false, "order": ["C:\\a.mp4", "C:\\b.mp4"] }
//! ```
//!
//! The rules the C++ enforces, and this port keeps:
//!
//! - a MISSING key takes the C++ default; a WRONG TYPE is an error
//!   (nlohmann's `value()` / `get<>()` throw on a type mismatch, and the C++
//!   turns that into a `ConfigError`) - never a silent default;
//! - the `order` list is NORMALIZED on load: the empty entries are dropped
//!   and the duplicates are removed case-insensitively (the first occurrence
//!   wins), so the order is preserved;
//! - the rotation interval must be in `[1, 1440]` minutes.

use std::cell::Cell;
use std::collections::HashSet;

use serde_json::{Map, Value};

pub use crate::config::ConfigError;

// The shared JSON reading rules (the nlohmann `value()` semantics: a missing
// key takes the default, a wrong type is an error) live with the displays
// contract - the C++ shares them the same way, through config_schema.hpp.
use crate::displays::{as_int, type_error};

/// The on-disk schema version of `playlist.json` (the C++
/// `kPlaylistSchemaVersion`).
pub const PLAYLIST_SCHEMA_VERSION: u32 = 1;

/// The smallest rotation interval, in minutes (the C++
/// `kPlaylistMinIntervalMin`).
pub const PLAYLIST_MIN_INTERVAL_MIN: i32 = 1;

/// The largest rotation interval, in minutes (the C++
/// `kPlaylistMaxIntervalMin`) - 24 h.
pub const PLAYLIST_MAX_INTERVAL_MIN: i32 = 1440;

/// The upper bound on the playlist length (the C++ `kPlaylistMaxEntries`).
pub const PLAYLIST_MAX_ENTRIES: usize = 500;

/// The default rotation interval, in minutes (the C++ `PlaylistConfig`).
pub const PLAYLIST_DEFAULT_INTERVAL_MIN: i32 = 30;

/// The default shuffle seed: the splitmix64 golden-gamma constant the C++
/// engine uses for its own RNG state.
pub const DEFAULT_SHUFFLE_SEED: u64 = 0x9E37_79B9_7F4A_7C15;

thread_local! {
    /// The shuffle RNG state. The C++ keeps it in the caller
    /// (`std::uint64_t& rng_state`, mutated in place so successive calls
    /// differ); the Rust API takes the order slice instead, so the state
    /// lives here. It is per-thread on purpose: two threads never share a
    /// sequence.
    static SHUFFLE_STATE: Cell<u64> = const { Cell::new(DEFAULT_SHUFFLE_SEED) };
}

/// The on-disk wallpaper playlist.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct PlaylistConfig {
    /// Always the current schema version after a migration.
    pub version: u32,
    /// Whether the rotation is on.
    pub enabled: bool,
    /// The rotation interval in minutes, in `[1, 1440]`.
    pub interval_min: i32,
    /// Whether the rotation picks a random entry.
    pub shuffle: bool,
    /// The absolute video paths, in play order.
    pub order: Vec<String>,
}

impl Default for PlaylistConfig {
    fn default() -> Self {
        Self {
            version: PLAYLIST_SCHEMA_VERSION,
            enabled: false,
            interval_min: PLAYLIST_DEFAULT_INTERVAL_MIN,
            shuffle: false,
            order: Vec::new(),
        }
    }
}

/// Lowercases the ASCII letters of a string for the case-insensitive
/// compare.
///
/// Windows path comparisons are case-insensitive; the ASCII range is enough
/// for the de-dup and the index lookup (the full Unicode folding is out of
/// scope, and the engine only ever compares paths it wrote itself). This is
/// the C++ `FoldAscii`.
fn fold_ascii(text: &str) -> String {
    text.chars()
        .map(|c| {
            if c.is_ascii_uppercase() {
                c.to_ascii_lowercase()
            } else {
                c
            }
        })
        .collect()
}

/// Normalizes an order list: drops the empty entries, de-duplicates
/// case-insensitively (the first occurrence wins) and preserves the order.
///
/// This is the C++ `NormalizeOrder`. The path spelling is not resolved or
/// normalized - only the list is.
pub fn normalize_order(order: Vec<String>) -> Vec<String> {
    let mut seen: HashSet<String> = HashSet::with_capacity(order.len());
    let mut out: Vec<String> = Vec::with_capacity(order.len());
    for entry in order {
        if entry.is_empty() {
            continue;
        }
        if seen.insert(fold_ascii(&entry)) {
            out.push(entry);
        }
    }
    out
}

/// The index of `current` in `order`, compared case-insensitively.
///
/// Returns `-1` when the path is absent. This is the C++
/// `PlaylistIndexForPath`.
pub fn playlist_index_for_path(order: &[String], current: &str) -> i64 {
    let needle = fold_ascii(current);
    for (index, entry) in order.iter().enumerate() {
        if fold_ascii(entry) == needle {
            return index as i64;
        }
    }
    -1
}

/// The next rotation index of a playlist of `count` entries whose current
/// position is `current`.
///
/// - `count == 0` -> `0` (the C++ returns "no rotation");
/// - not shuffling: `(current + 1) % count`, wrapping. An out-of-range
///   `current` is treated as "the end of the list";
/// - shuffling: a uniform pick in `[0, count)` that never equals `current`
///   (so a shuffle step always moves), driven by the splitmix64 state in
///   `rng_state` (mutated in place, so successive calls differ and are
///   reproducible from a fixed seed).
///
/// This is the C++ `SelectNextIndex`, byte for byte.
pub fn select_next_index_with_state(
    current: usize,
    count: usize,
    shuffle: bool,
    rng_state: &mut u64,
) -> usize {
    if count == 0 {
        return 0;
    }
    if !shuffle {
        let base = if current < count { current } else { count - 1 };
        return (base + 1) % count;
    }
    if count == 1 {
        return 0;
    }
    // splitmix64 step: advance the caller's state so successive calls differ.
    *rng_state = rng_state.wrapping_add(0x9E37_79B9_7F4A_7C15);
    let mut z = *rng_state;
    z = (z ^ (z >> 30)).wrapping_mul(0xBF58_476D_1CE4_E5B9);
    z = (z ^ (z >> 27)).wrapping_mul(0x94D0_49BB_1331_11EB);
    z ^= z >> 31;
    // Pick uniformly among the count-1 entries that are NOT `current`.
    let mut pick = (z % (count - 1) as u64) as usize;
    let cur = if current < count { current } else { 0 };
    if pick >= cur {
        pick += 1;
    }
    pick
}

/// The next rotation index of `order`, whose current position is `index`.
///
/// The Rust API takes the order itself (the C++ takes the count), so the
/// returned value is an index INTO `order`.
///
/// `-1` means "there is nothing to select" - an empty playlist. The shuffle
/// uses the per-thread splitmix64 state (see `set_shuffle_seed`).
pub fn select_next_index(order: &[String], index: usize, shuffle: bool) -> i64 {
    let count = order.len();
    if count == 0 {
        return -1;
    }
    if !shuffle {
        let base = if index < count { index } else { count - 1 };
        return ((base + 1) % count) as i64;
    }
    if count == 1 {
        return 0;
    }
    let mut state = shuffle_state();
    let next = select_next_index_with_state(index, count, true, &mut state);
    SHUFFLE_STATE.with(|slot| slot.set(state));
    next as i64
}

/// The current per-thread shuffle RNG state.
pub fn shuffle_state() -> u64 {
    SHUFFLE_STATE.with(|state| state.get())
}

/// Seeds the per-thread shuffle RNG state.
///
/// The golden fixture replays the shuffle sequence from a fixed seed
/// (`0x0123456789ABCDEF`), so a replayer must be able to set it.
pub fn set_shuffle_seed(seed: u64) {
    SHUFFLE_STATE.with(|state| state.set(seed));
}

/// Migrates a raw `playlist.json` document into the playlist.
///
/// This is the Rust mirror of the C++ `MigratePlaylist`. The C++ takes an
/// already-parsed `nlohmann::json`; the golden fixtures hand the replayer
/// the RAW document text, so the Rust side parses it here.
///
/// # Errors
///
/// - a corrupt document (`corrupt playlist JSON: parse error`);
/// - a root that is not an object (`playlist root must be a JSON object`);
/// - a version outside `[0, 1]` (`unsupported playlist schema version: N`);
/// - an `order` that is not an array, an order entry that is not a string,
///   or a wrong-typed `version` / `enabled` / `interval_min` / `shuffle`.
pub fn migrate_playlist(raw_text: &str) -> Result<PlaylistConfig, ConfigError> {
    let raw: Value = serde_json::from_str(raw_text)
        .map_err(|_| ConfigError("corrupt playlist JSON: parse error".to_string()))?;
    migrate_playlist_value(&raw)
}

/// The same migration, on an already-parsed document.
pub fn migrate_playlist_value(raw: &Value) -> Result<PlaylistConfig, ConfigError> {
    let root = raw
        .as_object()
        .ok_or_else(|| ConfigError("playlist root must be a JSON object".to_string()))?;

    // The C++ `raw.value("version", 0)`: a missing key defaults to 0, a
    // wrong type is a type error.
    let version = match root.get("version") {
        None => 0i64,
        Some(value) => as_int(value).ok_or_else(|| type_error("playlist", "number", value))?,
    };
    if version < 0 || version > PLAYLIST_SCHEMA_VERSION as i64 {
        return Err(ConfigError(format!(
            "unsupported playlist schema version: {version}"
        )));
    }

    let enabled = match root.get("enabled") {
        None => false,
        Some(value) => value
            .as_bool()
            .ok_or_else(|| type_error("playlist", "boolean", value))?,
    };
    let interval_min = match root.get("interval_min") {
        None => PLAYLIST_DEFAULT_INTERVAL_MIN,
        Some(value) => as_int(value).ok_or_else(|| type_error("playlist", "number", value))? as i32,
    };
    let shuffle = match root.get("shuffle") {
        None => false,
        Some(value) => value
            .as_bool()
            .ok_or_else(|| type_error("playlist", "boolean", value))?,
    };

    let order = if let Some(order) = root.get("order") {
        let entries = order
            .as_array()
            .ok_or_else(|| ConfigError("playlist order must be an array".to_string()))?;
        let mut paths = Vec::with_capacity(entries.len());
        for entry in entries {
            let path = entry
                .as_str()
                .ok_or_else(|| ConfigError("playlist order entries must be strings".to_string()))?;
            paths.push(path.to_string());
        }
        normalize_order(paths)
    } else {
        Vec::new()
    };

    Ok(PlaylistConfig {
        version: PLAYLIST_SCHEMA_VERSION,
        enabled,
        interval_min,
        shuffle,
        order,
    })
}

/// The playlist -> JSON mapping, in the shape the golden fixtures record.
pub fn playlist_json(cfg: &PlaylistConfig) -> Value {
    let mut out = Map::new();
    out.insert("version".to_string(), Value::from(cfg.version));
    out.insert("enabled".to_string(), Value::Bool(cfg.enabled));
    out.insert("interval_min".to_string(), Value::from(cfg.interval_min));
    out.insert("shuffle".to_string(), Value::Bool(cfg.shuffle));
    out.insert(
        "order".to_string(),
        Value::Array(cfg.order.iter().map(|p| Value::String(p.clone())).collect()),
    );
    Value::Object(out)
}

/// Rejects the values outside the accepted ranges - the C++
/// `ValidatePlaylist`.
///
/// # Errors
///
/// - an `interval_min` outside `[1, 1440]`;
/// - more than `PLAYLIST_MAX_ENTRIES` entries.
pub fn validate_playlist(cfg: &PlaylistConfig) -> Result<(), ConfigError> {
    if cfg.interval_min < PLAYLIST_MIN_INTERVAL_MIN || cfg.interval_min > PLAYLIST_MAX_INTERVAL_MIN
    {
        return Err(ConfigError(format!(
            "interval_min out of range [1,1440]: {}",
            cfg.interval_min
        )));
    }
    if cfg.order.len() > PLAYLIST_MAX_ENTRIES {
        return Err(ConfigError(format!(
            "playlist exceeds maximum entries ({PLAYLIST_MAX_ENTRIES})"
        )));
    }
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;

    fn order_of(count: usize) -> Vec<String> {
        (0..count).map(|i| format!("C:\\{i}.mp4")).collect()
    }

    #[test]
    fn the_order_drops_the_empty_entries_and_the_duplicates() {
        let order = vec![
            "C:\\a.mp4".to_string(),
            String::new(),
            "C:\\A.mp4".to_string(),
            "C:\\b.mp4".to_string(),
            "C:\\b.mp4".to_string(),
        ];
        assert_eq!(
            normalize_order(order),
            vec!["C:\\a.mp4".to_string(), "C:\\b.mp4".to_string()]
        );
    }

    #[test]
    fn the_rotation_wraps() {
        let order = order_of(3);
        assert_eq!(select_next_index(&order, 0, false), 1);
        assert_eq!(select_next_index(&order, 2, false), 0);
        // An out-of-range current is treated as "the end of the list".
        assert_eq!(select_next_index(&order, 9, false), 0);
    }

    #[test]
    fn an_empty_playlist_has_nothing_to_select() {
        assert_eq!(select_next_index(&[], 0, false), -1);
        assert_eq!(select_next_index(&[], 0, true), -1);
    }

    #[test]
    fn the_shuffle_never_picks_the_current_entry() {
        set_shuffle_seed(0x0123_4567_89AB_CDEF);
        let order = order_of(8);
        for _ in 0..64 {
            let next = select_next_index(&order, 3, true);
            assert_ne!(next, 3);
            assert!((0..8).contains(&next));
        }
    }

    #[test]
    fn the_shuffle_sequence_is_the_frozen_one() {
        let mut state = 0x0123_4567_89AB_CDEF;
        // The C++ dump: count 0, 1, 3, 8 - the sequential calls first, the
        // shuffle call last (only the shuffle calls advance the state).
        let mut rows = Vec::new();
        for count in [0usize, 1, 3, 8] {
            let sequential_from_0 = select_next_index_with_state(0, count, false, &mut state);
            let sequential_from_2 = select_next_index_with_state(2, count, false, &mut state);
            let shuffle_from_0 = select_next_index_with_state(0, count, true, &mut state);
            rows.push((count, sequential_from_0, sequential_from_2, shuffle_from_0));
        }
        assert_eq!(
            rows,
            vec![(0, 0, 0, 0), (1, 0, 0, 0), (3, 1, 0, 2), (8, 1, 3, 4),]
        );
    }

    #[test]
    fn the_path_lookup_is_case_insensitive() {
        let order = vec![
            "C:\\Videos\\A.mp4".to_string(),
            "C:\\Videos\\b.mp4".to_string(),
        ];
        assert_eq!(playlist_index_for_path(&order, "C:\\Videos\\A.mp4"), 0);
        assert_eq!(playlist_index_for_path(&order, "c:\\videos\\B.MP4"), 1);
        assert_eq!(playlist_index_for_path(&order, "C:\\other.mp4"), -1);
        assert_eq!(playlist_index_for_path(&[], "C:\\other.mp4"), -1);
    }

    #[test]
    fn a_v0_document_migrates_to_the_current_version() {
        let cfg = migrate_playlist(r#"{"version":0}"#).expect("must migrate");
        assert_eq!(cfg.version, PLAYLIST_SCHEMA_VERSION);
        assert!(!cfg.enabled);
        assert_eq!(cfg.interval_min, 30);
        assert!(!cfg.shuffle);
        assert!(cfg.order.is_empty());
    }

    #[test]
    fn a_future_version_is_rejected() {
        let err = migrate_playlist(r#"{"version":2}"#).expect_err("must fail");
        assert_eq!(err.0, "unsupported playlist schema version: 2");
    }

    #[test]
    fn the_order_is_normalized_on_load() {
        let cfg =
            migrate_playlist(r#"{"version":1,"order":["C:\\a.mp4","C:\\a.mp4","C:\\b.mp4"]}"#)
                .expect("must migrate");
        assert_eq!(
            cfg.order,
            vec!["C:\\a.mp4".to_string(), "C:\\b.mp4".to_string()]
        );
    }

    #[test]
    fn a_non_string_order_entry_is_rejected() {
        let err = migrate_playlist(r#"{"version":1,"order":[7]}"#).expect_err("must fail");
        assert_eq!(err.0, "playlist order entries must be strings");
    }

    #[test]
    fn a_wrong_typed_interval_is_rejected() {
        let err = migrate_playlist(r#"{"version":1,"interval_min":"x"}"#).expect_err("must fail");
        assert_eq!(
            err.0,
            "playlist field type error: [json.exception.type_error.302] \
             type must be number, but is string"
        );
    }

    #[test]
    fn the_validator_rejects_an_out_of_range_interval() {
        let cfg = PlaylistConfig {
            interval_min: 0,
            ..PlaylistConfig::default()
        };
        assert!(validate_playlist(&cfg).is_err());
    }
}
