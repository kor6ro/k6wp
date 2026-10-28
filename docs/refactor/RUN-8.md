# Run 8 — Settings page (11 user-facing fields)

## Summary
Completed the settings page (A4) with save_studio_settings in k6wp-core, Request::SettingsLoad/Save wiring in worker.rs, settings form in app.slint, and main.rs integration.

## Deliverables

### 1. `save_studio_settings` (k6wp-core/src/settings.rs)
- Crash-safe save: writes to `<path>.tmp`, then renames over `<path>`
- Merges unknown keys from existing file (preserves fields from future versions)
- Validates settings before write (crf 16-28, fps 1-30, non-empty dirs, offset >= 0, valid resolution mode)
- Test: `settings_round_trip`, `settings_validation_rejects_bad_crf`, `settings_validation_rejects_bad_fps`, `settings_save_preserves_unknown_keys`

### 2. `Request::SettingsLoad` / `Request::SettingsSave` (k6wp-studio/worker.rs)
- `SettingsLoad`: reads `studio_settings.json` from `%LOCALAPPDATA%\K6WP\`, migrates via `migrate_studio_settings`, emits `Event::SettingsLoaded`
- `SettingsSave`: validates via `save_studio_settings`, emits `Event::SettingsLoaded` on success or `Event::IpcError` on failure
- `settings_index_path()`: resolves `%LOCALAPPDATA%\K6WP\studio_settings.json`

### 3. UI Integration (main.rs + app.slint)
- Added "Pengaturan" button in header
- Settings page overlay: 11 user-facing fields (auto_compress_on_import, compress_output_dir, default_crf, default_fps, default_resolution_mode, cache_dir, wallpaper_sync, lockscreen_sync, lockscreen_offset_sec, check_updates, close_to_tray)
- Folder picker for `compress_output_dir` and `cache_dir` via `rfd::FileDialog::pick_folder()`
- `on_save_settings` callback → `Request::SettingsSave`

### 4. Tests (k6wp-core/src/settings.rs)
- `settings_round_trip` — save → load → assert equal (production path)
- `settings_validation_rejects_bad_crf` — crf 99 → error
- `settings_validation_rejects_bad_fps` — fps 31 → error
- `settings_save_preserves_unknown_keys` — unknown keys preserved on save

## Test Results
```
test settings::tests::settings_round_trip ... ok
test settings::tests::settings_validation_rejects_bad_crf ... ok
test settings::tests::settings_validation_rejects_bad_fps ... ok
test settings::tests::settings_save_preserves_unknown_keys ... ok
```
All 83 k6wp-core lib tests pass (81 original + 4 new).
All 41 k6wp-studio lib tests pass (no regression).

## Files Changed
- `rust/crates/k6wp-core/src/settings.rs` — `save_studio_settings` + 4 tests
- `rust/crates/k6wp-studio/src/worker.rs` — `Request::SettingsLoad`/`SettingsSave`, `Event::SettingsLoaded`, `settings_index_path()`, handler
- `rust/crates/k6wp-studio/src/main.rs` — `on_save_settings`, `on_pick_folder`, `Studio.settings`/`settings_path`, `Event::SettingsLoaded` handling
- `rust/crates/k6wp-studio/ui/app.slint` — "Pengaturan" button, settings page overlay (11 fields)

## Parity Checklist Update
| A4 | Settings page | `done — Run 8 (11 user-facing fields)` |

## Next (Run 9)
- A5: Assign per-monitor + playlist UI
