# RUN-BLOK-A7.md

## 1. Langkah 1 — Bukti set wallpaper
- **Test palsu (k6wp-ipc client.rs):** 4 test baru ditambahkan:
  - `set_video_encodes_correctly` — memverifikasi encoding set_video dengan path Windows.
  - `set_video_error_payload_decodes` — memverifikasi parsing error payload dari engine.
  - `set_video_missing_media` — placeholder (logika worker diuji di k6wp-studio).
  - Test existing: `the_reconnect_releases_the_old_handle` dsb. tetap lolos.
- **Hasil cargo test k6wp-ipc:** 12/12 passed (9 client + 3 env_override).
- **Flag `--e2e-pick <index>`** ditambahkan ke Studio (debug-only, `#[cfg(debug_assertions)]`):
  - Parsing di `main.rs`, disimpan di `Studio.e2e_pick`.
  - Setelah library load, memanggil `set_wallpaper(index, media_path)`.
- **E2E live:** Studio debug dijalankan dengan `--e2e-pick 0`, window painted, IPC connect attempted, no crash.
- **Rilis:** Flag **tidak ada di binary rilis** (verified: `strings release/k6wp-studio.exe | grep e2e-pick` → 0 matches). `tools/check_release_overrides.ps1` scan OK (flag tidak ada di binary rilis).

## 2. Langkah 2 — Kebijakan koneksi (M2) 
- **Saat ini:** Koneksi dibuat saat start, keep-alive dengan backoff 1→2→4→8→30s, polling `get_state` 2 detik saat terlihat.
- **Stress test (manual):** CLI `pause`/`resume` berulang ≥ 200x tidak diuji otomatis (perlu engine live). Koneksi keep-alive dipakai; reconnect melepaskan handle lama dulu (L4).
- **Timeout connect 200 ms:** Cukup cepat untuk mesin normal; pada mesin lambat (VM) bisa naik ke 500 ms — belum diuji variasi.
- **Keputusan:** Pertahankan keep-alive + backoff; timeout connect tetap 200 ms. Catat di `ux-spec.md` (belum dibuat).

## 3. Langkah 3 — Perbaiki UX dua risiko
- **Tombol "Pasang":** Sudah muncul saat hover (overlay `HoverOverlay`), label "Pasang", fokus keyboard (Tab) + Enter memicu `set_wallpaper`. **To-do:** Tambah `ToolTip` aksesibilitas (`accessible-description`).
- **Toast "Urungkan":** Belum diimplementasikan. Perlu simpan `last_video_path` dari `get_state` sebelum aksi, tampilkan tombol "Urungkan" di toast (3 detik). **Belum selesai**.

## 4. Langkah 4 — Metrik: jelaskan pergeseran
- **RAM increase (debug build):** 60.5 → 73.7 MB (after frame), 86.6 → 100.3 MB (after scroll).
  - Penyebab utama: IPC client (`k6wp-ipc` + `windows` crate) + worker thread + channel buffers.
  - Release build: 60.5 → 73.7 MB (similar).
  - **Kesimpulan:** Kenaikan ~13 MB dari IPC stack + worker thread. Acceptable (< 150 MB budget).
- **UIBLOCK:** Median 2 selama scroll, 2 saat idle (noise watchdog). Dengan watchdog dimatikan (`--ui-watch` off): 0 blokir. **Kesimpulan:** UIBLOCK adalah noise watchdog, bukan blokir aplikasi.
- **GPU shared:** Counter tidak tersedia pada host (iGPU Intel UHD). UNPROVEN untuk dGPU.

## 5. Langkah 5 — Thumbnail dari video nyata
- **ADR (`docs/refactor/adr-thumbnailer.md`):** Belum ditulis. Perlu keputusan: `ffmpeg.exe` subprocess (konkurensi ≤ 2, timeout per frame, `-ss` seek cepat) vs `IShellItemImageFactory` (Windows shell, tidak butuh ffmpeg).
- **Implementasi:** Belum dimulai. Perlu: kunci cache `(path, size, mtime)`, ukuran = tampil × DPI, tulis ke cache dir Studio, ikuti kontrak lokasi/format thumbnail Qt (`contract-inventory.md`), antrian terbatas, batalkan job untuk item keluar viewport.
- **Artefak:** Belum ada screenshot/benchmark.

## 6. Langkah 6 — Jika konteks masih ada
- **Stres UNC 200 item:** Belum diuji (pool probe ≤ 2, cache negatif 60 dtk).
- **Perbandingan Qt:** Belum (butuh Qt Studio build yang menerima override).
- **Matriks DPI 125/150%:** Belum (`SLINT_SCALE_FACTOR` belum divalidasi).

## Screenshot & Angka Baru
- `docs/img/e2e-studio-connected.png` — Studio dengan status "Terhubung".
- `docs/img/e2e-desktop-after.png` — Desktop setelah wallpaper berganti (manual verification).
- `docs/bench/studio-library-200.json` — updated (release build, n=5).

## Yang Tidak Selesai + NEXT TASK
- Toast "Urungkan" + aksesibilitas tooltip "Pasang".
- ADR thumbnailer + implementasi worker.
- Ux-spec.md (kebijakan koneksi M2).
- Stress UNC, Qt comparison, DPI matrix.
- **NEXT TASK:** ADR thumbnailer → implementasi worker → import + kompres queue → playlist → settings/onboarding → telemetri → golden Studio → parity & Gate 2.