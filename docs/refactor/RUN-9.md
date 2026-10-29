# Run 9: Assign per-monitor + playlist UI

## Ringkasan
Implementasi assign per-monitor + playlist UI dengan 5 test baru, total 46 test pass di k6wp-studio.

## Perubahan Utama

### 1. Settings Page (app.slint) — 3 Tab
- **Tab Umum**: 11 field existing (auto_compress, compress_output_dir, default_crf, default_fps, default_resolution_mode, cache_dir, wallpaper_sync, lockscreen_sync, lockscreen_offset_sec, check_updates, close_to_tray)
- **Tab Monitor**: Daftar monitor dengan assign/hapus video per monitor
- **Tab Playlist**: 4 field (enabled, interval_min, shuffle, order) + file picker

### 2. IPC Protocol (worker.rs)
- `Request::ListMonitors` → `Event::MonitorsLoaded(Vec<MonitorAssignment>)`
- `Request::AssignVideo(device, path)` → `Event::AssignmentResult(device, ok, error)`
- `Request::ClearAssignment(device)` → `Event::AssignmentResult(device, ok, error)`
- `Request::LoadPlaylist` → `Event::PlaylistLoaded(PlaylistConfig)`
- `Request::SavePlaylist(config)` → `Event::PlaylistSaved(ok, error)`

### 3. Core Logic (playlist.rs, settings.rs)
- `save_playlist(path, config)` — atomic write + validation
- `save_studio_settings(path, config)` — includes playlist fields
- `validate_playlist(config)` — interval_min ∈ [1, 1440]

### 4. 5 Test Baru (worker.rs::tests)
1. `assign_video_sends_correct_payload` — verify SetDisplayVideo payload structure
2. `clear_assignment_sends_clear_true` — verify clear: true payload
3. `playlist_rejects_interval_zero` — validate_playlist rejects interval 0 and 1441
4. `playlist_preserves_order` — save_playlist → migrate_playlist round-trip preserves order
5. `save_playlist_round_trip` — production path round-trip with enabled/shuffle/interval

## Test Results
- **k6wp-studio**: 46 passed (41 existing + 5 new)
- **k6wp-core**: 83 passed + 4 integration (2 sps_golden failures pre-existing — missing test corpus)

## Files Changed
- `rust/crates/k6wp-studio/ui/app.slint` — 3-tab settings, MonitorAssignment struct
- `rust/crates/k6wp-studio/src/worker.rs` — IPC handlers, 5 new tests, FakeCompressorBehavior in mod tests
- `rust/crates/k6wp-studio/src/main.rs` — callback handlers for new IPC
- `rust/crates/k6wp-core/src/playlist.rs` — save_playlist, validate_playlist, migrate_playlist
- `rust/crates/k6wp-core/src/settings.rs` — save_studio_settings with playlist fields
- `docs/refactor/parity-checklist.md` — A5, A9, A11 marked done

## Notes
- Struct `MonitorAssignment` digunakan (bukan parallel arrays workaround) — Slint 1.17 generate normal
- `FakeCompressorBehavior` & `make_fake_compressor` dipindahkan ke `mod tests` untuk akses test baru
- sps_golden 2 test fail pre-existing (missing test corpus video files)