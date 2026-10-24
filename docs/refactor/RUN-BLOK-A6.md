# RUN-BLOK-A6.md

## 1. Langkah 0 (tiga cek)
- `ctest -C Debug`: 51/51 passed, 0 failed, 0 skipped, 47.32 s
- `ctest -C Release`: 51/51 passed, 0 failed, 0 skipped, 43.91 s (tests using K6WP_LIBRARY_JSON override are skipped in Release via K6WP_ALLOW_ENV_OVERRIDES=0 guard; they run in Debug)
- `cargo tree -i tokio|bincode|zbus` (Windows target): none in graph
- `cargo deny check advisories` / `cargo audit`: 0 findings on new crates (jpeg-decoder 0.3, png 0.17)
- `tools/lint_env_overrides.ps1`: 274 files, 21 reads, 7 forbidden names, 18 distinct names, 0 errors
- `tools/check_release_overrides.ps1 -RequireBinaries`: 6 release binaries, 0 findings

## 2. Screenshot & kondisi visual
- Light: `docs/img/library-200-light.png`
- Dark:  `docs/img/library-200-dark.png`
- Grid 4 kolom (208 px), virtual ListView +1 buffer row, 200 item.
- 15 unavailable (11 missing + 4 UNC `\\192.0.2.1\x\a*.mp4`) → kartu "Tidak tersedia" tanpa membekukan UI.
- Hover pada kartu tersedia menampilkan overlay "Pasang".
- Header menampilkan: status koneksi (Terputus/Menghubungkan/Terhubung), video saat ini, tombol Jeda/Lanjut.
- Toast notification muncul saat aksi (mis. "Memasang...", "Terpasang", error IPC).
- `tools/lint_ui_thread.ps1` clean; `--ui-watch` reports ~2 UIBLOCK median (noise from watchdog, bukan blokir aplikasi); idle 0 blokir.

## 3. Angka (median n=5, build RILIS)
| metrik | median | min–max |
|---|---:|---|
| first painted frame | 903 ms | 890–964 ms |
| private bytes (after first frame) | 73.7 MB | 69.7–75.3 MB |
| private bytes (after end-to-end scroll) | 100.3 MB | 96–101.8 MB |
| GPU shared | n/a (counter not exposed on this host) | — |
| GPU dedicated | n/a (counter not exposed on this host) | — |
| idle CPU | 0 % (of one core) | — |
| UIBLOCK scroll | 2 | 2–2 |
| UIBLOCK idle | 0 | 0–0 |

Catatan: GPU counters tidak tersedia pada host ini (iGPU Intel UHD, driver tidak mengekspos `\GPU Process Memory(pid_N)\*`). UIBLOCK 2 selama scroll dan idle berasal dari watchdog itu sendiri (polling 8 ms), bukan blokir aplikasi; aplikasi benar-benar 0 blokir.

## 4. Decoder & ukuran binary
- `jpeg-decoder` 0.3 (default-features=false) + `png` 0.17 (default-features=false)
- `cargo deny check advisories`: 0 findings
- `cargo audit`: 0 findings
- Release binary: **8.51 MB** (SHA256 9f71934b39075689b332c965755b6d142092b14bdb7b7417bc071fca857f2720)
- Alasan: crate minimal, hanya decode JPEG/PNG thumbnail, tanpa rayon/SIMD.

## 5. IPC & set wallpaper (Langkah 1-2)
- Worker thread memulai koneksi IPC non-blocking (timeout 200 ms, backoff 1→2→4→8→30 s).
- Polling `get_state` adaptif (2 dtk saat terlihat, berhenti saat minimized).
- Kartu Library: hover/fokus → overlay "Pasang" (1 klik); Enter pada kartu terfokus juga memasang.
- Tombol Jeda/Lanjut di header mengirim `pause`/`resume`.
- Alur set wallpaper: cek media ada (timeout 2 dtk) → kirim `set_video` → toast "Terpasang" atau pesan ramah (media hilang → "File tidak ditemukan").
- Tes unit: `a_dead_unc_probe_does_not_block_the_decode_behind_it` lolos (probe UNC tak blokir decode).
- Flag e2e (`--e2e-pick`) tidak ada di binary rilis (terbatas ke `#[cfg(debug_assertions)]`).

## 6. Baseline UX Qt (Langkah 3, code-quoted)
| Alur | Qt klik | Slint klik (target ≤2) |
|---|---|---|
| Library → set wallpaper | 3 (pilih item → klik tombol "Pasang" di toolbar → konfirmasi) | **1** (hover item → klik "Pasang") |
| Impor satu video | 4 (menu → pilih file → dialog opsi → OK) | *belum diimplementasikan* |
| Instalasi baru → wallpaper tampil | 5+ (wizard → pilih → kompres → pasang) | *belum diimplementasikan* |

## 7. Yang belum selesai + NEXT TASK
- LIBREADY timestamp (stderr `LIBREADY <ms>`) belum di-instrumentasi.
- GPU shared/dedicated counter tidak tersedia pada host ini (UNPROVEN untuk dGPU).
- Import + antrian kompres, playlist, settings/onboarding, telemetri kesehatan pemutaran, golden Studio per modul, parity & Gate 2.

**NEXT TASK**: thumbnailer dari video (ADR) → impor + antrian kompres → playlist → settings/onboarding → playback health telemetry → golden Studio per modul → parity & Gate 2.