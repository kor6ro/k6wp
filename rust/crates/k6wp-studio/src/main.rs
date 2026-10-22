// The K6WP Studio - Phase 2 shell.
//
// This is the Slint shell that replaces the QML Studio: the same window,
// the same settings surface, without Qt. The shell is deliberately thin -
// the Phase 2 slices add the library, the thumbnailer, the preview, the
// compress queue and the settings bindings one at a time, each with its
// own gate.
//
// The startup budget is 700 ms (baseline v2: the Qt Studio took 1290 ms
// with 337 MB of private bytes); the RAM budget is 150 MB.

slint::include_modules!();

use k6wp_core::{LibraryEntry, LibraryIndex};
use slint::{ComponentHandle, ModelRc, SharedString, VecModel};
use std::path::PathBuf;

/// The library index the Studio reads - the SAME file the Qt Studio reads
/// (the C++ override first, then %LOCALAPPDATA%\K6WP\library.json).
///
/// The override is what makes the Phase 2 comparison (E1) possible: both
/// Studios are pointed at ONE index file, so the state is identical.
fn library_index_path() -> Option<PathBuf> {
    // The override is compiled OUT of the release build (task I4): a
    // shipped binary must not let an environment variable redirect the
    // library path. The release binaries are scanned by
    // tools/check_release_overrides.ps1.
    #[cfg(debug_assertions)]
    {
        if let Ok(override_path) = std::env::var("K6WP_LIBRARY_JSON") {
            if !override_path.is_empty() {
                return Some(PathBuf::from(override_path));
            }
        }
    }
    let local = std::env::var("LOCALAPPDATA").ok()?;
    if local.is_empty() {
        return None;
    }
    Some(PathBuf::from(local).join("K6WP").join("library.json"))
}

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let ui = AppWindow::new()?;

    // The library model: the SAME index file the Qt Studio reads, through
    // the shared codec (k6wp-core).
    let rows = VecModel::<LibraryRow>::default();
    let mut loaded = 0usize;
    if let Some(path) = library_index_path() {
        match std::fs::read_to_string(&path) {
            Ok(text) => match LibraryIndex::parse(&text) {
                Ok(index) => {
                    for entry in &index.entries {
                        rows.push(row_from(entry));
                    }
                    loaded = index.entries.len();
                }
                Err(e) => {
                    eprintln!("studio: the library index is malformed: {e}");
                }
            },
            Err(e) => {
                eprintln!("studio: the library index could not be read: {e}");
            }
        }
    }
    ui.set_library(ModelRc::new(rows));
    ui.set_library_count(loaded as i32);

    // The version line: the same information the Qt Studio showed.
    ui.set_version(SharedString::from(env!("CARGO_PKG_VERSION")));

    ui.run()?;
    Ok(())
}

fn row_from(entry: &LibraryEntry) -> LibraryRow {
    LibraryRow {
        name: SharedString::from(
            entry
                .dst
                .file_name()
                .map(|n| n.to_string_lossy().to_string())
                .unwrap_or_default(),
        ),
        path: SharedString::from(entry.dst.to_string_lossy().to_string()),
        duration_s: entry.duration as f32,
    }
}
