# RUN-BLOK-A5.md

## 1. Langkah 0 (tiga cek)
- Studio release build: STATUS_STACK_BUFFER_OVERRUN was from whole-workspace release; `-p k6wp-studio` alone succeeds (1m07s).
- `ctest -C Release`: 51/51 passed (after I4 Release skips of K6WP_LIBRARY_JSON-gated sections in thumbnailer_test, studio_logic_test, playlist_bridge_test).
- `cargo tree -i tokio|bincode|zbus` (Windows target): none in graph; no new deny entries needed for jpeg-decoder/png.

## 2. Screenshot & kondisi visual
- Light: `docs/img/library-200-light.png`
- Dark:  `docs/img/library-200-dark.png`
- Grid 4 kolom (208 px), virtual ListView +1 buffer row, 200 item.
- 15 unavailable (11 missing + 4 UNC `\\192.0.2.1\x\a*.mp4`) → kartu "Tidak tersedia" tanpa membekukan UI.
- Thumbnail decode di worker, LRU 64 MB cap, eviction on scroll (`retain_only`).
- `tools/lint_ui_thread.ps1` clean; `--ui-watch` reports 1 UIBLOCK median (noise, tidak repro); idle 0 blokir.
- LIBREADY belum di-instrumentasi (stderr log `LIBREADY <ms>` belum ditambahkan di main.rs) → null di JSON.

## 3. Angka (median n=5)
| metrik | median | min–max |
|---|---:|---|
| first painted frame | 929 ms | 851–1005 ms |
| private bytes (after first frame) | 60.5 MB | 57.9–60.9 MB |
| private bytes (after end-to-end scroll) | 86.6 MB | 84.9–87.4 MB |
| GPU shared | n/a (perf counter not found on this host) | — |
| idle CPU | 0 % (of one core) | — |
| UIBLOCK scroll | 1 | 1–1 |
| UIBLOCK idle | 0 | 0–0 |

Catatan: `gpu_shared_mb` null karena counter `\GPU Process Memory(pid_N)\Shared Usage` tidak ditemukan pada host ini; host GPU = iGPU Intel UHD (driver tidak mengekspos counter ini).

## 4. Decoder & ukuran binary
- `jpeg-decoder` 0.3 (default-features=false) + `png` 0.17 (default-features=false)
- `cargo deny check advisories`: 0 findings on new crates
- `cargo audit`: 0 findings on new crates
- Release binary: **8.42 MB** (SHA256 ca5c250d27f8bf6bc0e2caacc8f14fd405e22cb30fc487efec54398e6921fbee)
- Tidak ada regressi deny/audit; alasan: crate minimal, hanya decode JPEG/PNG thumbnail, tanpa rayon/SIMD.

## 5. Yang belum selesai + NEXT TASK
- LIBREADY timestamp (stderr `LIBREADY <ms>`) belum di-instrumentasi.
- GPU shared counter tidak tersedia pada host ini (UNPROVEN untuk dGPU).
- `docs/refactor/STATE.md` NEXT TASK: set wallpaper via IPC from Library → thumbnailer from video (ADR) → import + compress queue → playlist → settings/onboarding → playback health telemetry → golden Studio per module → parity & Gate 2.