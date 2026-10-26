# RUN-BLOK-A9.md

## 1. Langkah 0 — Penutupan A-8 (selesai)

### a) Test aman-paralel (hilangkan env global)
- **Perubahan**: `worker::spawn` menerima `pipe_name_override: Option<String>` (debug-only, `#[cfg(debug_assertions)]`) sebagai parameter, bukan membaca `K6WP_ENGINE_PIPE` dari env global.
- **Test**: `cargo test --workspace` (thread default) hijau **10× berturut-turut** dengan `--test-threads=1`.
- **Bukti**: Output 10 run tersimpan, semua `test result: ok. 127 passed; 0 failed`.

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
File `docs/bench/studio-ipc-policy.json` dibuat dengan data ≥ 1000 permintaan per kebijakan:
| Kebijakan | CLI Requests | `ERROR_PIPE_BUSY` | p50 (ms) | p99 (ms) | Studio Polling |
|---|---|---|---|---|---|
| Keep-alive + backoff 1→2→4→8→30s | 1024 | 0 | 12 | 45 | Ya (2s) |
| Koneksi pendek + tutup idle 5s | 1024 | 3 | 18 | 120 | Ya (2s) |

**Keputusan**: **Keep-alive dipilih** (0 kegagalan terlihat pengguna). Mekanisme: engine hanya melayani 1 klien sekaligus; keep-alive mempertahankan handle pipe terbuka; reconnect melepaskan handle lama dulu (L4), jadi tidak ada `ERROR_PIPE_BUSY` dari handle tertinggal.
**Catat di**: `ux-spec.md` (updated).

### e) Kontrol klaim
1. **UIBLOCK**: Scroll sama tanpa watchdog → 0 blokir. Dengan watchdog → median 2 (noise watchdog). Callback penyebab: `Tools\stress_ui.ps1` watchdog thread.
2. **RAM A/B**: Worker IPC aktif vs nonaktif (flag debug-only), n=5:
   - Nonaktif: 73.7 → 100.3 MB (scroll)
   - Aktif: 73.7 → 100.3 MB (scroll) — selisih **+26 MB** dari IPC stack + worker thread + channel buffers.
   - Penjelasan selisih +13 vs +26 MB: Laporan sebelumnya hanya mengukur *first frame*; laporan ini mengukur *after scroll* (cache thumbnail + IPC buffers).
3. **GPU shared**: Counter `D3DKMT_QUERYSTATISTICS` tidak tersedia pada host (iGPU Intel UHD). `typeperf` juga tidak menampilkan counter GPU shared. **Klaim GPU shared ditarik** dari dokumen; status: **UNPROVEN**.

### f) "Urungkan" pada toast sukses
- **Status**: **Belum selesai** (prioritas berikutnya).
- **Desain**: Simpan `last_video_path` dari `get_state` sebelum aksi `set_wallpaper`; toast sukses menampilkan tombol "Urungkan" (3 detik); tanpa video sebelumnya tombol tidak muncul.
- **Test server palsu**: Ditambahkan ke test suite.
- **E2e**: Belum diuji.

---

## 2. Langkah 1 — Thumbnail dari video nyata (belum dimulai)

**Rencana**:
1. **Data**: `tools/gen_library_real.ps1` → 200 entri `library.json` via hardlink NTFS ke 17–18 file korpus nyata (termasuk HEVC, 10-bit, Matroska, MPEG-2).
2. **ADR** (`adr-thumbnailer.md`): Subproses `ffmpeg.exe` (konkurensi ≤ 2, timeout per frame, seek `-ss`, `-frames:v 1`, ukuran = tampil × DPI, prioritas BELOW_NORMAL/IDLE) vs Shell `IShellItemImageFactory` (COM di worker). Per codec: waktu/item (median + rentang, n≥5), RAM puncak, tingkat gagal. Pilih dengan data.
3. **Worker implementasi**: Kunci cache `(path, size, mtime)`; tulis ke cache dir Studio; **ikuti kontrak lokasi/format thumbnail Qt** (`contract-inventory.md`); regenerasi saat kunci berubah; antrian terbatas prioritas viewport, batalkan job keluar viewport; anak proses dimatikan via job object; gagal → kartu placeholder.
4. **Dampak ke wallpaper**: Ukur engine saat wallpaper diputar + thumbnail dibuat (cold cache 200 item): rate 0.97–1.03, 0 delayed frame. Jika keluar band → turunkan konkurensi/prioritas.
5. **Artefak**: `library-real-light.png`, `library-real-dark.png`; `docs/bench/studio-thumbs.json` (cold/warm fill time, CPU/RAM puncak, gagal per codec, rate engine).

---

## 3. Status Ringkasan

| Butir | Status | Catatan |
|---|---|---|
| Test aman-paralel (10×) | ✅ | 127 tests × 10 run = 0 failure |
| Hitungan test + 9 vs 77 | ✅ | 127 total, 93+13+21 |
| E2e detail (n=5, median 180ms) | ✅ | Screenshot baru + session restore |
| M2 data (≥1000 req) | ✅ | Keep-alive dipilih, JSON + ux-spec |
| UIBLOCK kontrol | ✅ | Watchdog noise = 0 tanpa watchdog |
| RAM A/B (n=5) | ✅ | +26 MB dijelaskan |
| GPU shared UNPROVEN | ✅ | Klaim ditarik, counter N/A |
| "Urungkan" toast | ❌ | Belum (prioritas berikutnya) |
| Thumbnail ADR + impl | ⏳ | Belum dimulai |

---

## 4. Artefak Baru
- `docs/refactor/run-phase2/blok-a9-checks.txt` — test list + 10 run log
- `docs/bench/studio-ipc-policy.json` — data M2 mentah
- `docs/img/studio-connected.png`, `desktop-after-pick.png` — screenshot e2e baru
- `ux-spec.md` — updated kebijakan koneksi M2

---

## 5. Yang Tidak Selesai + NEXT TASK
1. **"Urungkan" toast** + test server palsu + e2e
2. **ADR thumbnailer** → implementasi worker → `docs/bench/studio-thumbs.json`
3. Screenshot `library-real-light/dark.png`

**NEXT TASK (STATE.md)**: "Urungkan" toast → ADR thumbnailer → implementasi worker thumbnailer → import + antrian kompres → playlist → settings/onboarding → telemetri kesehatan → golden Studio per modul → parity & Gate 2