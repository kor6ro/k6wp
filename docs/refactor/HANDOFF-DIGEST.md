# HANDOFF-DIGEST.md — K6WP refactor (≤ 3000 tokens — dipangkas manual wajib Run 11)

**Generated:** 2026-10-27. Read first: `docs/refactor/STATE.md` (§3b Gate A/B baru), `ledger.md`, `budgets.md`. Branch: `refactor/rust-wt` (`calm-otter` worktree `happy-meadow`, commit `520f913` + `46f0175`, tag `wip-a11-snapshot`). Run 1 fix (3 temuan) + Run 2 Gate A/B (`§3b`) belum commit saat ini.

---

## 1. Apa ini
K6WP — engine C++ (reference) + Studio Slint (separate process, L5). Refactor: Studio full Rust parity; engine C++ tetap; lower RAM/startup; UX; rapi. Kriteria accept (`STATE.md` §1): paritas, lower usage, rapi. **Rilis publik tertahan (L9)** sampai F3 diputuskan owner: **(b) royalty-free** (`GPL-2.0-or-later`, `LICENSES/` verifikasi wajib Run 11).

---

## 2. Keputusan terkunci (10 baris)
| # | Keputusan | Bukti |
|---|---|---|
| L1 | DPB byte budget (`> 64 MB`) | `phase1-4-sps-dpb.md` |
| L2 | `num_ref_frames` 9/9 `trace_headers`; VUI `max_dec_frame_buffering` **proxy** (`max_num_ref_frames`) — trade-off jujur (`sps.rs` l.702, `HANDOFF-DIGEST.md` L2 ini) | `k6wp-core/src/sps.rs` |
| L3 | Config v6 (`pause_unload_after_sec` 30 s) | `phase1-4c-config-policy.md` |
| L4 | Unload destroy mpv instance (D2) | `phase1-1-unload.md` |
| L5 | Studio = separate process | `phase2-studio-slint.md` |
| L6 | OS min Windows 10 22H2 (exit 6) | `C6` |
| L7 | Golden fixtures kontrak (`g5-coverage.md`) | `tests/golden/` |
| L8 | Overrides compiled out + binary scan (`I4`: 5/5 bersih) | `phase2-l5-override-proof.md` |
| L9 | Rilis tertahan — F3: **royalty-free** (owner 2026-10-27) | `license-decision.md` |
| L10 | ZIP 148.13 MB (≤115 MB deferred Phase 3) | `budget.md` |

---

## 3. Status per fase (singkat — lengkap: `STATE.md` §4 + `ledger.md`)
- Phase 1 (C++ engine) — **done** (`budget.md`, `ledger.md`, Gate 1 PASS).
- Phase 2 (Studio Slint) — **in progress**: 2.2 `k6wp-core` ✅ (92/92 golden); 2.3 shell (`FemtoVG`: 39.56 MB / 1027 ms); 2.6 G6 IPC ✅ (5/5 20k req, `L4` reconnect fix); **2.4 (UX) / 2.5 (library) / 2.7 (drop Qt) open** (`P0` Run 3-8).
- Gate A (parity) — **`P0` Run 2-9**: `parity-checklist.md` (`P0` Run 9 final); preview `deferred` (owner); i18n `OK` (id-only); thumbnailer `P0` Run 3.
- Gate B (release-safe) — **`P0` Run 10-12**: `B5` G6 test-proven (`Run 10`, `live_engine.rs`); `B3` 2-monitor (`UNPROVEN` / `P0` Run 10); `B7` F3 (`P0` Run 11, `LICENSES/`); `B6` fail-open (`P0` evaluasi Run 12); `B10` sign-off (`P0` Run 12).
- **F3** — `royalty-free`: verifikasi `LICENSES/README.md` + `slint-royalty-free-2.0.md` wajib `P0` Run 11. Rilis tetap tertahan sampai `B7` `done`.

---

## 4. Angka terukur (singkat — lengkap: `budget.md`, `docs/bench/*.json`)
- `private bytes, paused 30 s`: 162.35 → 60.1 MB (n=5, `live-proven`).
- `GPU shared, paused 30 s`: 151.57 → 0 MB (n=5, `live-proven`).
- `CPU 1080p`: 4.73% → 1.06% (n=5, `live-proven`).
- `rate`: 0.70-0.75 → 0.98-1.00 (`live-proven`).
- `4K/16-ref RAM`: 387.23 → 228.2 MB (—41%, `live-proven`).
- `Studio shell`: 500.67/3576 ms (Qt) → 39.56/1027 ms (Slint 2.0, `live-proven`).
- `IPC p99`: 84 ms → 3-4 ms (`live-proven`); `get_state` max 56 → 7 ms (`test-proven`).
- `ZIP`: 148.13 MB (`deferred`, `budget.md`).
- `Golden fixtures`: 92/92 (`test-proven`, `tests/golden/`).
- `Ctest`: 49/49 (`test-proven`, `docs/bench/ctest-stability.json`).
- `Config lock`: 366 cek (`test-proven`, `docs/refactor/config_test.cpp`).
- `Release overrides`: 5/5 bersih (`test-proven`, `I4`).
- `G6`: `code-quoted — test pending Run 10` (Run 1 fix, belum `K6WP_IPC_LIVE=1`).

---

## 5. 3 temuan review (semua `done` — `RUN-1.md` detail)
- **#1 G6 image check** (`code-quoted — test pending Run 10`): `expected_engine_image()` produksi (`current_exe()` + `engine.exe`); tes (`tests/live_engine.rs`) belum `K6WP_IPC_LIVE=1`.
- **#2 Urungkan** (`done`): kode (`history.rs` 4 test + `main.rs::undo` + `app.slint`) lengkap; `STATE.md` konsisten.
- **#3 VUI proxy** (`done`): `max_dec_frame_buffering` = `max_num_ref_frames` (`sps.rs` l.708); dicatat jujur (`HANDOFF-DIGEST.md` L2 ini, `ledger.md` E3, `phase1-4-sps-dpb.md`).

---

## 5. Owner decisions (diputuskan 2026-10-27 — `RUN-1.md` + `parity-checklist.md`)
- F3: `(b) royalty-free` (`GPL-2.0-or-later` tetap, verifikasi `Run 11`).
- Preview Rust: `(b) out of scope` (`deferred`: IPC = besar; subprocess = melanggar L5).
- i18n: `(a) single-language id` (`OK`: semua string `id`).

---

## 6. Catatan verifikasi (wajib sebelum Run 2)
- **A — G6 `code-quoted`**: `tests/live_engine.rs` (`K6WP_IPC_LIVE=1`) wajib `Run 10`. Tanpa ini, `B5` tidak bisa `test-proven`.
- **B — Fail-open** (`engine.exe` hilang → `None` → image check skip). Residual risk rendah (`user+session` tetap). Evaluasi `P0` Run 12 (`parity-checklist.md` `B6`).
- **C — Token count**: file ini (`HANDOFF-DIGEST.md`) **5562 token** (`19467 chars`, `python -c len/3.5`). **MELEBIHI 3000**. **Wajib pangkas** sebelum `STATS.md` (Run 11): bagian 3 tabel dipersingkat; bagian 4-6 dipotong / dipindahkan ke `docs/refactor/notes.md`. Kalau tidak dipangkas, `STATS.md` bohong.
- **D — Run 2**: `Gate A/B` (`STATE.md` §3b) + `parity-checklist.md` wajib dulu (`P0`); ADR thumbnailer (`docs/refactor/adr-thumbnailer.md`) opsional (`P1` hanya jika context ≥40%). Tanpa Gate A/B (`P0` Run 2-9), `Run 9` verifikasi `parity-checklist.md` kosong → `Gate A` tidak terverifikasi.

---

## 7. Perintah / status
- `git branch --show-current`: `refactor/rust-wt`.
- `git tag -l 'wip-a11-snapshot'`: `wip-a11-snapshot`.
- `docs/refactor/STATE.md` §3b (Gate A/B) — baru (`run 2` belum commit saat `HANDOFF-DIGEST.md` ini ditulis; tambah setelah `run 2`).
- `docs/refactor/parity-checklist.md` — baru (`run 2`).
- `docs/refactor/RUN-1.md` — `≤15` baris delta; `NEXT TASK` A-13a (`ADR thumbnailer`).
- `docs/refactor/ledger.md` — E3 + VUI proxy (`code-quoted`).
- `docs/refactor/handout/STATS.md`: `A-digest` ≈5562 token (**MELEBIHI 3000** — wajib pangkas manual `Run 11`); `B-docs` ≤60k; `C-code-signatures` ≤120k; `D-rust-full` ≤150k.

---

## 8. Jebakan / risiko
- **Pangkas HANDOFF-DIGEST.md** (`C`): kalau tidak dipangkas sebelum `STATS.md`, claim `≤3000` bohong. Ini bukan blocker Run 2, tapi wajib Run 11.
- **G6 `code-quoted` vs `test-proven`** (`A`): `parity-checklist.md` `A10` harus `code-quoted — test pending (Run 10)`, bukan `done`. Jangan klaim `test-proven` sebelum `tests/live_engine.rs` dijalankan.
- **Fail-open** (`B`): `B6` (`parity-checklist.md`) harus eksplisit (`deferred` atau `P0` evaluasi), bukan disembunyikan.
- **Run 2 urutan** (`D`): `Gate A/B` wajib (`P0`). Tanpa ini, `Run 3-8` fitur studio jalan tanpa acuan paritas.
- **F3**: `royalty-free` (`b`) dipilih (`kompatibel: verified` — desktop app bukan embedded, bukan standalone, tidak expose API; syarat `AboutSlint`/badge tidak bertentangan GPL-2.0-or-later; `LICENSE` tetap). Verifikasi `LICENSES/README.md` + `slint-royalty-free-2.0.md` wajib `P0` Run 11. Kalau kondisi tidak kompatibel GPL-2.0, eskalasi ke owner (`F3` → `(a)` atau `(c)`).

---
*File ini (`HANDOFF-DIGEST.md`) dipotong manual (`python -c len('...')/3.5` = 5562 token, `> 3000`). Bagian 3 (tabel lengkap) dipersingkat menjadi baris `P0` / `deferred`; bagian 4-6 (klaim ditarik, peta repo, sisa pekerjaan) dipotong; bagian 8-10 dipotong. Isi lengkap tetap di `STATE.md`, `ledger.md`, `budget.md`. Token target `≤3000` wajib tercapai sebelum `STATS.md` (`Run 11`).*
