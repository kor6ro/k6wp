//! The K6WP contracts in Rust (Phase 2, slice 2.2).
//!
//! Every module here mirrors a C++ contract that is FROZEN: the same keys,
//! the same defaults, the same migration and the same error behaviour. The
//! C++ side stays the reference until the cut-over; the golden fixtures
//! (`tests/golden/shared_behavior.json`) are replayed against both.

pub mod library;

pub use library::{LibraryEntry, LibraryIndex};
