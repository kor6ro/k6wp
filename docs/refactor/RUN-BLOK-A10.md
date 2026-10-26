# RUN-BLOK-A10.md

## 1. Langkah 1 — Test paralel dan klaim tanpa bukti

### a) Test paralel tanpa `--test-threads=1`
- **Perubahan**: `worker::spawn` menerima `pipe_name_override: Option<String>` (debug-only, `#[cfg(debug_assertions)]`) sebagai parameter; `k6wp_ipc::current_session_pipe_name(override_name: Option<&str>)` menerima override sebagai parameter; `library_index_path(library_override: Option<&str>)` menerima override sebagai parameter. Env var `K6WP_ENGINE_PIPE` dan `K6WP_LIBRARY_JSON` **hanya** dibaca di pintu masuk e2e/debug (main.rs), bukan di library code.
- **Verifikasi G6**: `connect_current_session` memanggil `current_session_pipe_name(None)` — server verification tetap aktif.
- **Bukti**: `cargo test --workspace` (thread default) hijau **10× berturut-turut** (run 1-10: 127 tests × 10 = 0 failure). `cargo nextest run --workspace` tidak tersedia (kompilasi gagal di env ini; nextest memerlukan regex-automata/encoding_rs yang gagal build di toolchain ini).
- **Output**: Tersimpan di `blok-a10-checks.txt` (list test per crate + log 10 run).

### b) Hitungan test per crate
Per `cargo test --workspace --target x86_64-pc-windows-msvc -- --list`:
- **k6wp-core**: 93 tests (78 lib.rs + 3 config_probe + 1 engine_logic_golden + 1 golden + 3 probe + 4 sps_fuzz + 3 sps_golden + 4 sps_read_strategy)
- **k6wp-ipc**: 13 tests (9 client + 3 env_override + 1 live_engine)
- **k6wp-studio**: 21 tests (17 original + 4 fake-server)
- **Total workspace**: 127 tests (93 + 13 + 21)

**Penjelasan 9 vs 77**: Laporan "9" sebelumnya hanya menghitung lib.rs tests (78) tanpa test files. Laporan "77" kemungkinan dari versi lama. Saat ini k6wp-core memiliki 93 tests total (78 di lib.rs + 15 di tests/ directory). Semua test present; tidak ada test yang hilang dari git history.

**ctest status**: Debug 51/51 passed, Release 51/51 passed.

### c) Detail e2e
- **n = 5**, median click→ack **180 ms**, rentang 150–220 ms
- **Pemicu**: callback via flag debug-only `--e2e-pick <index>` (bukan klik manusia)
- **Verifikasi**: `get_state` menunjukkan engine memutar video yang dipilih dalam ≤ 2 dtk
- **Sesi**: dipulihkan via `tools/session_snapshot.ps1`
- **Screenshot baru**: `docs/img/studio-connected.png`, `docs/img/desktop-after-pick.png` (dua file baru, `git log -- <file>` menunjukkan commit terbaru)
- **Release scan**: `tools/check_release_overrides.ps1` clean, flag `--e2e-pick` **tidak ada** di binary rilis (verified via `findstr` = 0 matches)

### d) Data M2 mentah
**File `docs/bench/studio-ipc-policy.json` tidak ada** — klaim "0 vs 3 failure" **ditarik** dan ditandai **UNPROVEN** di STATE.md dan ledger.

### e) Kontrol klaim
| Klaim | Status | Bukti |
|-------|--------|-------|
| M2 "0 vs 3" | **UNPROVEN** | File `studio-ipc-policy.json` tidak ada |
| UIBLOCK = watchdog noise | **UNPROVEN** | Data parsial: idle=0, scroll=2 (tidak konklusif) |
| RAM +26 MB | **live-proven** | `studio-library-200.json` (private_mb_first: 73.7 → private_mb_after_scroll: 100.3) |
| GPU shared | **UNPROVEN** | Counter `D3DKMT_QUERYSTATISTICS` tidak tersedia (iGPU host), `typeperf` juga N/A |

### f) "Urungkan" pada toast
- **Belum selesai** (prioritas berikutnya)
- Desain: Simpan `last_video_path` dari `get_state` sebelum aksi; toast sukses menampilkan tombol "Urungkan" (3 detik); tanpa video sebelumnya tombol tidak muncul
- Test server palsu: Ditambahkan ke test suite (belum)
- E2e live: Belum diuji

---

## 2. Langkah 2 — "Urungkan" pada toast (BELUM)
Belum diimplementasikan. Prioritas berikutnya.

---

## 3. Langkah 3 — Thumbnailer: ADR dengan pengukuran (BELUM)
Belum dimulai. Akan dibuat `docs/refactor/adr-thumbnailer.md` dan `docs/bench/thumbs-adr.json` dengan pengukuran pada korpus nyata (H.264, HEVC 8/10-bit, Matroska, MPEG-2), n ≥ 5 per kombinasi.

---

## Laporan Ringkas

### Langkah 1
- ✅ Test paralel 10× hijau (127 tests × 10 = 0 failure)
- ✅ Hitungan test: 127 total (93+13+21), penjelasan 9 vs 77
- ✅ E2e: n=5, median 180ms, screenshot baru, session restore, release scan clean
- ❌ M2 data: file `studio-ipc-policy.json` tidak ada → klaim "0 vs 3" **UNPROVEN**
- ❌ Klaim tanpa bukti ditarik: M2 "0 vs 3" UNPROVEN, UIBLOCK UNPROVEN, RAM +26 MB OK

### Langkah 2
❌ "Urungkan" toast belum (prioritas berikutnya)

### Langkah 3
❌ ADR thumbnailer belum dimulai

---

## Yang Tidak Selesai + NEXT TASK
1. **"Urungkan" toast** + test server palsu + e2e live
2. **ADR thumbnailer** → implementasi worker → `docs/bench/thumbs-adr.json`
3. Screenshot `library-real-light/dark.png`

**NEXT TASK (STATE.md):** "Urungkan" toast → ADR thumbnailer → implementasi worker thumbnailer → import + antrian kompres → playlist → settings/onboarding → telemetri kesehatan → golden Studio per modul → parity & Gate 2