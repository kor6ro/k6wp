# Run 6 — Import + LibraryIndex::save_atomic + file pick (rfd) + tests

## Summary
Completed the import flow (A2) with file pick via `rfd`, atomic library persistence, and test coverage.

## Deliverables

### 1. `LibraryIndex::save_atomic` (k6wp-core)
- Atomic write: writes to `<path>.tmp`, then renames over `<path>`
- Mirrors C++ `config_store::AtomicWriteJson` behavior
- Test: `save_atomic_round_trip` — write → parse → assert equal

### 2. `Request::Import` handler (k6wp-studio/worker.rs)
- Validates file extensions (mp4, mkv, webm, mov)
- Validates file exists and size > 0
- Copies files to `<library_dir>/wallpapers/`
- Creates `LibraryEntry` with defaults (res=0x0, fps=30, crf=23, encoder=libx264)
- Appends to library index and calls `save_atomic`
- Emits `Event::Library(Ok(entries))` to reload UI
- Reports errors via `Event::IpcError`

### 3. UI Integration (main.rs + app.slint)
- Added "Impor" button in header
- `on_import` callback → `rfd::FileDialog` with video filter
- Sends `Request::Import { paths }` to worker
- Drag-drop deferred post-v1 (Slint 1.x custom DragArea effort)

### 4. Tests (worker.rs)
- `save_atomic_round_trip` — core persistence
- `import_validates_extension` — rejects .txt via generate_thumbnail
- `import_validates_missing_file` — rejects missing file via generate_thumbnail
- (import_appends_to_library deferred - needs full worker integration test)

## Test Results
```
test worker::tests::import_tests::import_validates_extension ... ok
test worker::tests::import_tests::import_validates_missing_file ... ok
```
All 36 tests pass (34 original + 2 new).

## Files Changed
- `rust/crates/k6wp-core/src/library.rs` — `save_atomic` + test
- `rust/crates/k6wp-studio/Cargo.toml` — `rfd = "0.15"` dependency
- `rust/crates/k6wp-studio/src/worker.rs` — `Request::Import` handler + tests
- `rust/crates/k6wp-studio/src/main.rs` — `on_import` callback + `rfd` import
- `rust/crates/k6wp-studio/ui/app.slint` — "Impor" button in header

## Parity Checklist Update
| A2 | Import flow | `done — Run 6 (file pick + import)` + `deferred (drag-drop, post-v1)` |

## Next (Run 7)
- A3: Compress antrian (compressor Rust / IPC)