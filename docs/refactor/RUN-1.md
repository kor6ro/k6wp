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

## Catatan sebelum Run 2 (4 poin — wajib baca):

A. **G6 fix: `code-quoted` bukan `test-proven`** — `expected_engine_image()` diuji hanya cabang `None` (tes fake-server PASS karena `engine.exe` tidak ditemukan di env tes). Cabang `Some(path)` (production) belum diuji (`tests/live_engine.rs` butuh `K6WP_IPC_LIVE=1`). Status: `code-quoted — test pending (Run 10)`. **Tidak blocker** untuk Run 2 (Gate A/B dokumentasi), tapi wajib diperbaiki sebelum Gate B claim.

B. **Fail-open residual** — jika `engine.exe` hilang (portabel, folder salah), `expected_engine_image()` return `None` → image check skip. Residual risk: user+session tetap dicek, jadi risk rendah. **Catat di Gate B** (`docs/refactor/parity-checklist.md`): "fail-open residual (engine.exe hilang → image check skip); evaluasi Run 12". Tidak diubah sekarang.

C. **Laporan konsistensi** — `git show --stat 520f913` konfirmasi 6 file: `HANDOFF-DIGEST.md`, `ledger.md`, `STATE.md`, `phase1-4-sps-dpb.md`, `rust/crates/k6wp-core/src/sps.rs`, `rust/crates/k6wp-studio/src/worker.rs`. `HANDOFF-DIGEST.md` ≤3000 token: belum dicek ulang setelah edit; jika >3000, pangkas manual di Run 11 (F3 verifikasi).

D. **Run 2 deliverable: Gate A/B dulu, ADR thumbnailer opsional** (`COST-RULES.md` §4: satu deliverable per run, tanpa side quest). Urutan: (1) `STATE.md` section Gate A/B, (2) `parity-checklist.md` final, (3) commit. ADR thumbnailer (`adr-thumbnailer.md`) = Run 3 (hanya jika context cukup setelah Gate A/B). Tanpa Gate A/B, Run 9 (Gate A verifikasi) tidak punya acuan.

**NEXT TASK (tidak berubah dari STATE.md)**: ADR thumbnailer (`adr-thumbnailer.md`) → implementasi worker thumbnailer → import + antrian kompres → ... → Gate 2.
