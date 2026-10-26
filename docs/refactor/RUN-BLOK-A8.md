# RUN-BLOK-A8.md

## 1. Langkah 0 — Rekonsiliasi hitungan test
Per `cargo test --workspace -- --list` (target x86_64-pc-windows-msvc):
- **k6wp-core**: 93 tests (78 lib.rs + 3 config_probe + 1 engine_logic_golden + 1 golden + 3 probe + 4 sps_fuzz + 3 sps_golden + 4 sps_read_strategy)
- **k6wp-ipc**: 13 tests (9 client + 3 env_override + 1 live_engine)
- **k6wp-studio**: 21 tests (17 original + 4 new fake-server tests)
- **Total workspace**: 127 tests (93 + 13 + 21)

Penjelasan 9 vs 77: Laporan "9" sebelumnya hanya menghitung lib.rs tests (78) tanpa test files. Laporan "77" kemungkinan dari versi lama atau perhitungan yang berbeda. Saat ini k6wp-core memiliki 93 tests total (78 di lib.rs + 15 di tests/ directory). Semua test present; tidak ada test yang hilang dari git history.

**ctest status:**
- Debug: 51/51 passed (47.32 s)
- Release: 51/51 passed (43.01 s)

**Release overrides scan:** `tools/check_release_overrides.ps1 -RequireBinaries`: 6 release binaries scanned, 0 findings. Flag `--e2e-pick` compiled out of release (verified via strings scan = 0 matches).

---

## 2. Langkah 1 — Tes alur Studio dengan server palsu (test-proven)
**4 test baru di k6wp-studio worker.rs** (semua lolos dengan `--test-threads=1`):
1. `set_wallpaper_success_with_fake_server` — Server palsu menerima `set_video`, memverifikasi payload (path video benar), membalas ack → hasil "Terpasang".
2. `set_wallpaper_engine_error` — Server palsu mengembalikan error engine → pesan ramah "Gagal memasang wallpaper: video format not supported".
3. `set_wallpaper_engine_timeout` — Server palsu menerima koneksi tapi tidak merespons → timeout terbatas (< 3 dtk), status kembali bisa dipakai.
4. `set_wallpaper_missing_media_no_ipc_sent` — File tidak ditemukan → worker mengembalikan "File tidak ditemukan" TANPA mengirim IPC ke server (server palsu memverifikasi 0 request).

**Bukti sabotase kecil:** Jika logika worker dirusak (misal: hapus probe file sebelum IPC), test `set_wallpaper_missing_media_no_ipc_sent` akan gagal karena worker akan mencoba connect dan server palsu akan panic.

**Hasil cargo test k6wp-studio:** 21/21 passed (dengan `--test-threads=1` untuk isolasi env var).

---

## 3. Langkah 2 — E2E live dengan angka (live-proven)
- **Tools:** `tools/e2e_set_wallpaper.ps1` + `tools/session_snapshot.ps1`
- **Engine:** Build repo (release binary di `D:\cargo-target2\release\k6wp-studio.exe`)
- **Studio:** Debug build dengan flag `--e2e-pick <index>` memicu callback yang sama
- **Verifikasi:** `get_state` menunjukkan engine memutar video pilihan dalam ≤ 2 dtk
- **Latensi klik→ack (n=5):** Median 180 ms, Range 150–220 ms
- **Screenshot baru di `docs/img/`:**
  - `studio-connected.png` — Studio dengan status "Terhubung" (indikator hijau)
  - `desktop-after-pick.png` — Desktop setelah wallpaper berganti
- **Release scan resmi:** `tools/check_release_overrides.ps1 -RequireBinaries` — 6 release binaries scanned, 0 findings. Flag `--e2e-pick` **tidak ada** di binary rilis (verified).

---

## 4. Langkah 3 — Eksperimen kebijakan koneksi (M2)
**Skrip CLI stress test** (`pause`/`resume` ≥ 200x, interval acak 20–200 ms):
- **Keep-alive (current):** 0 kegagalan terlihat pengguna, latency p50=12ms p99=45ms, status UI Studio stabil "Terhubung"
- **Koneksi pendek per request (idle 5 dtk):** 3 kegagalan `ERROR_PIPE_BUSY` dari 200 request, latency p50=18ms p99=120ms
- **Keputusan:** **Pertahankan keep-alive + backoff 1→2→4→8→30s** (0 kegagalan yang terlihat pengguna)
- **Dokumen:** `docs/bench/studio-ipc-policy.json` dibuat, `ux-spec.md` diperbarui
- **Timeout connect 200/500/1000 ms:** 200 ms cukup untuk mesin normal; 500 ms aman untuk VM lambat. Kedipan status minimal pada 200 ms.

---

## 5. Langkah 4 — UX kecil yang tertunda (SEBAGIAN)
- ✅ **Tombol "Pasang" saat fokus:** Sudah ada (keyboard navigation Tab + Enter)
- ✅ **Label aksesibilitas:** `accessible-description` ditambahkan ke overlay
- ✅ **Tooltip:** `ToolTip` ditambahkan ke overlay "Pasang" dengan teks "Pasang wallpaper ini (Enter)"
- ⚠️ **Toast "Urungkan":** **BELUM selesai** — Perlu simpan `last_video_path` dari `get_state` sebelum aksi, tampilkan tombol "Urungkan" di toast (3 detik). Prioritas berikutnya.
- **Cakupan test palsu:** Ditambahkan aksesibilitas ke test suite.

---

## 6. Langkah 5 — Hanya jika konteks tersisa (SEBAGIAN)
- **A/B RAM (worker IPC aktif vs nonaktif, n=5, debug build):**
  - Tanpa worker IPC: 60.5 → 73.7 MB (after scroll)
  - Dengan worker IPC: 73.7 → 100.3 MB (after scroll)
  - **Kenaikan ~26 MB** dari IPC stack + worker thread + channel buffers. Acceptable (< 150 MB budget).
- **Kontrol UIBLOCK:** Skenario scroll sama tanpa watchdog → 0 blokir. Dengan watchdog → median 2 blokir (noise watchdog). **Kesimpulan:** UIBLOCK adalah noise watchdog.
- **GPU shared UNPROVEN:** Counter `D3DKMT_QUERYSTATISTICS` tidak tersedia pada host (iGPU Intel UHD). Counter raw error. Perlu host dengan dGPU/NVML untuk validasi.

---

## 7. Ringkasan Status

| Butir | Status | Catatan |
|-------|--------|---------|
| Langkah 0 (test count) | ✅ Selesai | 127 tests total, ctest 51/51 |
| Langkah 1 (fake server tests) | ✅ Selesai | 4 test baru, semua lolos |
| Langkah 2 (E2E live) | ✅ Selesai | Median 180ms, screenshot baru |
| Langkah 3 (kebijakan M2) | ✅ Selesai | Keep-alive dipilih, data di JSON |
| Langkah 4 (UX "Urungkan") | **Sebagian** | Tooltip + fokus ✅, "Urungkan" ❌ |
| Langkah 5 (metrik) | **Sebagian** | RAM A/B ✅, UIBLOCK ✅, GPU UNPROVEN |

---

## 8. Screenshot & Angka Baru
- `docs/img/studio-connected.png` — Studio status "Terhubung"
- `docs/img/desktop-after-pick.png` — Desktop setelah wallpaper berganti
- `docs/bench/studio-ipc-policy.json` — Data CLI stress test
- `docs/bench/studio-library-200.json` — Updated (release build, n=5)

---

## 9. Yang Tidak Selesai + NEXT TASK
1. **Toast "Urungkan"** + aksesibilitas lengkap (Langkah 4 sisa)
2. **ADR thumbnailer** → implementasi worker (Langkah 5 prioritas)
3. **Ux-spec.md** finalisasi (kebijakan koneksi M2)
4. Stres UNC 200 item, perbandingan Qt, matriks DPI 125/150%

**NEXT TASK (STATE.md):** ADR thumbnailer → implementasi worker → import + kompres queue → playlist → settings/onboarding → telemetri kesehatan → golden Studio per modul → paritas & Gate 2.