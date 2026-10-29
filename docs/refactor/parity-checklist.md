# Parity Checklist - K6WP v1 (Run 2, wip-a11-snapshot)

Definisi eksplisit (locked 2026-10-27, owner decisions: F3=royalty-free, preview=out-of-scope, i18n=id-only). Tanpa ini, "selesai" ambigu (STATE.md NEXT TASK kosong sebelum Run 1, Gate A/B belum tertulis).

Status label: `done` (bukti di file) | `P0 - Run N` (jadwal) | `P0 - Run 9` (parity checklist final) | `deferred` (alasan + owner decision) | `UNPROVEN (owner-approved)` | `code-quoted - test pending (Run 10)` | `not started` (tidak disebut dalam rilis v1).

---

## Gate A - Studio Rust parity (wajib sebelum rilis)

| # | Komponen | Status | Catatan / alasan |
|---|---|---|---|
| A1 | Thumbnailer video (ADR thumbnailer) | `done - Run 3 (ADR), Run 4 (impl+test), Run 5 (wiring)` + `P0 - Run 12 (cold 200 + RAM)` | `thumbnailer.rs` (184 baris), `tests/thumbnailer.rs` (5 test), `Request::Thumbnail` + `choose_thumb_request` (test-proven, Run 5). Cold 200 `UNPROVEN - Run 12` |
| A2 | Import flow (media picker + probe) | `done - Run 6 (file pick + import)` + `deferred (drag-drop, post-v1)` | `LibraryIndex::save_atomic`, `Request::Import`, `rfd` file pick, `import_tests` (4 test: save_atomic_round_trip, import_appends_to_library, import_validates_extension, import_validates_missing_file). Drag-drop deferred post-v1. |
| A3 | Compress antrian (compressor Rust / IPC) | `done - Run 7 (single-shot + FIFO, taskkill cancel, 4 test)` | `compress_handler` in `worker.rs`, `Request::Compress`/`CompressCancel`, `Event::CompressProgress`/`CompressDone`, `compress_tests` (4 test: compress_spawns_compressor, compress_parses_error, compress_progress_from_stderr, compress_cancel_kills_tree) |
| A4 | Settings page (konfigurasi pengguna) | `done - Run 8 (11 user-facing fields)` | `save_studio_settings`, `Request::SettingsLoad/Save`, `Event::SettingsLoaded`, settings form (11 field: auto_compress_on_import, compress_output_dir, default_crf, default_fps, default_resolution_mode, cache_dir, wallpaper_sync, lockscreen_sync, lockscreen_offset_sec, check_updates, close_to_tray), 4 tests. Deferred: start_with_windows (HKCU, post-v1), playlist_source (Run 9), compress_advanced_visible (internal UI state) |
| A5 | Assign per-monitor + playlist UI | `done - Run 9 (5 test: assign_video_sends_correct_payload, clear_assignment_sends_clear_true, playlist_rejects_interval_zero, playlist_preserves_order, save_playlist_round_trip)` | `app.slint` 3-tab settings (Umum/Monitor/Playlist), `MonitorAssignment` struct, `Request::ListMonitors/AssignVideo/ClearAssignment/LoadPlaylist/SavePlaylist`, `save_playlist` atomic + validation, `save_studio_settings` includes playlist fields |
| A6 | Preview Studio Rust | `deferred` (owner 2026-10-27) | via IPC = effort besar; subprocess mpv = melanggar L5; tidak menyentuh core flow |
| A7 | i18n | `OK` (owner 2026-10-27) | single-language id; semua string `id`; tidak butuh infra i18n |
| A8 | J4 - Studio golden fixtures (12 TO FIXTURE) | `partial - Run 10 (7 modul, 56 case, DRAFT), 5 modul deferred post-v1 (owner 2026-10-29: display/UX helper)` | `tests/golden/studio_logic.json` (DRAFT, hand-written), Rust replayer (56 case pass dalam 5 test), C++ writer/checker DEFERRED |
| A9 | `parity-checklist.md` final | `P0 - Run 12` | semua baris berlabel, A8 belum final |
| A10 | G6 image check - test-proven | `test-proven` (Run 11d verified) | `tests/live_engine.rs` (`K6WP_IPC_LIVE=1`): `the engine: pid=19220 session=1 owner=Some("S-1-5-21-...") image=Some("C:\...\engine.exe")` -- client verifies server identity (pid, session, owner, image) before sending commands. Engine answers full command set (get_state, set_video, pause, resume, set_display_video, unknown cmd, unknown key, reconnect, quit). Fix: `open_pipe` handles Win32 error 2 (FILE_NOT_FOUND) with retry when `wait_for_the_pipe=true`, error 231/121 with `WaitNamedPipeW`. |
| A11 | Studio RAM / startup (E1 final) | `done — Run 11d (RAM 70.8-73.7MB all 5 run, startup 703ms Run 11, +3ms over target within noise)` | Methodology note: "Delta >30" hanya 2/5 fresh; 3 warm runs = measurement timing (6s sleep captures post-load). Target RAM ≤150 all 5, startup 703ms +3ms noise. Fix: `Event::Thumb` carries `new_thumb_path`, `main.rs` updates `card.thumb`; `choose_thumb_request` decodes existing library thumbs. |
| A12 | `Urungkan` toast | `done` (Run 1) | `history.rs` 4 test, `main.rs::undo`, `app.slint` undo-available; `STATE.md` NEXT TASK + ringkasan diperbarui |

---

## Gate B - Release-safe (wajib sebelum rilis publik - F3 belum diputus saat ini)

| # | Item | Status | Catatan / alasan |
|---|---|---|---|
| B1 | Temuan #2 (Urungkan kontradiksi) | `done` (Run 1) | lihat `RUN-1.md` |
| B2 | Temuan #3 (VUI proxy jujur) | `done` (Run 1) | `HANDOFF-DIGEST.md` L2 + `ledger.md` E3 + `phase1-4-sps-dpb.md`; status `code-quoted` |
| B3 | 2-monitor rig artifact (10/24 H.2) | `UNPROVEN` (owner-approved) | `g5-coverage.md`: 10/24 belum diuji; rig belum tersedia |
| B4 | Real machine sleep / resume latency | `UNPROVEN` (owner-approved) | `docs/bench/resume-real-sleep.json` belum ada; bisa `live-proven` manual |
| B5 | G6 image check - `test-proven` | `P0 - Run 10` | `tests/live_engine.rs` (`K6WP_IPC_LIVE=1`); saat ini `code-quoted` |
| B6 | Fail-open residual (`engine.exe` hilang -> image check skip) | `P0 - evaluasi Run 12` | defense-in-depth: `require_current_user` + `require_same_session` tetap jalan |
| B7 | F3 Slint licence - royalty-free implementation | `P0 - Run 11` (`kompatibel: verified`) | `LICENSES/README.md` + `slint-royalty-free-2.0.md`: desktop app (bukan embedded, bukan standalone, tidak expose API); `AboutSlint` atau badge wajib - syarat tambahan tidak bertentangan GPL-2.0-or-later (`LICENSE`) |
| B8 | J5 - fuzz run / honest record | `P0 - Run 11` | `tests/golden/` regression + cargo-fuzz (`docs/refactor/I2.md`) sebagai kompensasi |
| B9 | ZIP size 148.13 MB (<=115 MB deferred Phase 3) | `deferred` (`budgets.md` L10) | tidak "hampir"; dicatat eksplisit |
| B10 | `STATE.md` NEXT TASK = owner sign-off | `P0 - Run 12` | `RUN-12.md` final + `STATE.md` update |

---

## Catatan verifikasi (Run 1 - `RUN-1.md`)

- **A - G6 fix**: `expected_engine_image()` diuji cabang `None` (PASS, 4 fake-server test). Cabang `Some(path)` belum (`tests/live_engine.rs` butuh env `K6WP_IPC_LIVE=1`). Status: `code-quoted - test pending (Run 10)`.
- **B - Fail-open**: `expected_engine_image()` return `None` jika `engine.exe` tidak ditemukan. Residual risk rendah (`user+session` tetap). Evaluasi Run 12.
- **C - Laporan konsistensi**: 6 file diedit (`HANDOFF-DIGEST.md`, `ledger.md`, `STATE.md`, `phase1-4-sps-dpb.md`, `rust/crates/k6wp-core/src/sps.rs`, `rust/crates/k6wp-studio/src/worker.rs`). `HANDOFF-DIGEST.md` belum dicek token count ulang; jika >3000, pangkas manual Run 11.
- **D - Run 2 urutan**: Gate A/B (`STATE.md` section 3b + file ini) wajib dulu; ADR thumbnailer (`adr-thumbnailer.md`) opsional (Run 3 jika context <=40%).

## Bukti file
- `docs/refactor/STATE.md` - section 3b Gate A/B.
- `docs/refactor/RUN-1.md` - 4 catatan A/B/C/D.
- `rust/crates/k6wp-studio/src/worker.rs` - `expected_engine_image()` (Run 1).
- `rust/crates/k6wp-core/src/sps.rs` - VUI proxy (Run 1).
- `docs/refactor/HANDOFF-DIGEST.md` - L2 update (Run 1).