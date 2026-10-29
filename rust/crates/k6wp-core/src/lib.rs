//! The K6WP contracts in Rust (Phase 2, slice 2.2).
//!
//! Every module here mirrors a C++ contract that is FROZEN: the same keys,
//! the same defaults, the same migration and the same error behaviour. The
//! C++ side stays the reference until the cut-over; the golden fixtures
//! (`tests/golden/shared_behavior.json`) are replayed against both.

pub mod compress_args;
pub mod config;
#[cfg(windows)]
pub mod config_store;
pub mod displays;
pub mod engine_logic;
pub mod ipc;
pub mod library;
pub mod playlist;
pub mod settings;
pub mod sps;
pub mod version;

pub use config::{ConfigError, WallpaperConfig};
#[cfg(windows)]
pub use config_store::{
    persist_config_field, save_config, ConfigFileLock, LOCK_TIMEOUT, MAX_CONFIG_BYTES,
};
pub use library::{LibraryEntry, LibraryIndex};
pub use playlist::{save_playlist, PlaylistConfig};
pub use settings::StudioSettings;
pub use settings::{migrate_studio_settings, save_studio_settings};
pub use sps::{probe_dpb, Codec, DpbReport, SpsError, SpsInfo};
