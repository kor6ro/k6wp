# RUN-1 — 3 review findings fixed (2026-10-27)

**Branch**: `refactor/rust-wt` (new from HEAD `happy-meadow`/`calm-otter`, tag `wip-a11-snapshot`). **Deliverable**: fix 3 temuan review + laporan delta. **Biaya**: 1 run, 1 commit (`520f913`).

## Fix (6 file, +36/-14):

**#1 — G6 image check (BUG NYATA, produksi)** `rust/crates/k6wp-studio/src/worker.rs`: `expected_server_image` bukan `None`. Fungsi `expected_engine_image()` (l.318-334) baca jalur biner engine dari `std::env::current_exe()` (tidak bergantung env override I4). `attempt_connect` (l.348): `expected_server_image: expected_engine_image()`. Bukti: `tests/live_engine.rs` menggunakan `K6WP_IPC_ENGINE_EXE`; produksi sekarang setara.

**#2 — Kontradiksi dokumen "Urungkan"** `docs/refactor/STATE.md`: NEXT TASK (l.331) hapus "Urungkan"; ringkasan A-8 (l.338), A-9 (l.347), A-11 (l.355) diperbarui → ✅. `docs/refactor/ledger.md` E3 (l.42) + catatan "fitur kode lengkap". `HANDOFF-DIGEST.md` L2 (l.16) + catatan VUI proxy. Kontradiksi diselesaikan: fitur ada di kode (`history.rs` 4 test + `main.rs::undo` + `app.slint` undo-available), STATE.md kini konsisten.

**#3 — VUI `max_dec_frame_buffering` (proxy jujur)** `rust/crates/k6wp-core/src/sps.rs`: komentar diperbarui (l.702-708) — "read but NOT used directly"; trade-off dicatat (`HANDOFF-DIGEST.md` L2, `ledger.md` E3, `phase1-4-sps-dpb.md` tabel l.181). `STATE.md` §1b menyebut kontradiksi ini diselesaikan. Parser tidak diubah (risiko misalignment VUI tail tetap), tapi status dicatat eksplisit.

## Verifikasi:
- `cargo fmt --check`: hijau.
- `cargo check -p k6wp-studio`: hijau (hanya warning Slint, bukan error).
- `git status`: bersih; commit `520f913`; push ke `origin/refactor/rust-wt` + tag `wip-a11-snapshot`.
- `docs/refactor/STATE.md`: NEXT TASK sudah `ADR thumbnailer` (tanpa "Urungkan").

## Owner decisions (sudah diputuskan sebelum Run 2):
- F3: (b) royalty-free (`LICENSES/` verifikasi wajib Run 11).
- Preview Studio Rust: (b) out of scope (`parity-checklist.md` akan catat `deferred`).
- i18n: (a) single-language id (`parity-checklist.md`: `OK`).

**NEXT TASK (tidak berubah dari STATE.md)**: ADR thumbnailer (`adr-thumbnailer.md`) → implementasi worker thumbnailer → import + antrian kompres → ... → Gate 2.
