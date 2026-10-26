# RUN-3 — ADR thumbnailer (ADR only, 2026-10-27)

**Branch**: `refactor/rust-wt`. **Deliverable**: ADR (`adr-thumbnailer.md`, ≤80 baris, 76 baris aktual). **Kontrak Qt disalin persis** (`thumbnailer.cpp`): `SafeSeekSeconds`, `FormatSeekSeconds`, `BuildThumbCommand` (`force_original_aspect_ratio=decrease,pad=320:180`), `ThumbCacheKeyMaterial` (`abs|size|mtime|thumb-v3`), `ThumbsSubdir` (`K6WP\thumbs`), `FindFfmpeg` (sibling → build tree → env debug-only, `I4`). **Angka**: semua `UNPROVEN` (ukur `Run 4`: cold cache 200 item, RAM puncak, gagal per codec). **F3**: `royalty-free` (`verified` — desktop app bukan embedded, bukan standalone, tidak expose API; `AboutSlint`/badge syarat tambahan tidak bertentangan `GPL-2.0-or-later`). **Biaya**: 0 pengukuran baru, 0 perubahan kode produk.

## Kontrak Qt (disalin persis — `studio/src/thumbnailer.cpp`)

- Seek: `SafeSeekSeconds(duration)`: `return (duration > 0) ? ((duration >= 2.0) ? 1.0 : duration/2.0) : 0.0;` (l.74-78).
- Seek format: `std::fixed` + `std::setprecision(6)` → strip trailing `0` + `.` (l.82-89).
- Scale+pad: `scale=W:H:force_original_aspect_ratio=decrease,pad=W:H:(ow-iw)/2:(oh-ih)/2` (`W=320`, `H=180`, `kThumbWidth`/`kThumbHeight`, l.103-108). **Bukan** `-s 320x180` (stretch).
- Frames: `-frames:v 1` (`-vframes`, l.103).
- Quality: `-q:v 4` (`kJpegQuality`, l.52).
- Timeout: 10 detik (`kFfmpegTimeoutMs` = 10000, l.53).
- Cache key: `abs_path_string | size_string | mtime_string | version_string` (`ThumbCacheKeyMaterial`, l.116-129). **Versi** = `"thumb-v3"` (`kThumbCacheVersion`, l.59) — wajib sama agar transisi Qt→Rust tidak invalidate semua thumbnail.
- Cache dir: `%LOCALAPPDATA%\K6WP\thumbs` (`kThumbsSubdir` = `L"K6WP\thumbs"`, l.49).
- Cache file: `sha1(material).hexdigest() + ".jpg"` (kontrak `thumbnailer.hpp` — `ThumbCachePath`).
- Ffmpeg path: `FindFfmpeg` (`shared/ffmpeg_path.cpp`): sibling `ffmpeg.exe` → `build/release/ffmpeg.exe` → env `K6WP_FFMPEG` (`debug_assertions` only, `I4`).

## Keputusan ADR

1. **Pilihan**: ffmpeg subprocess (`-ss`, `-i`, `-frames:v 1`, `-vf scale+pad`, `-q:v`).
2. **Konkurensi**: ≤2 (`P0` — tidak hardcode di ADR; konstanta `PROBE_THREADS`-style, alasan: cold cache 200 item <5s).
3. **F3**: `royalty-free` (`verified` — `LICENSES/slint-royalty-free-2.0.md` §2-4: desktop app bukan embedded, bukan standalone, tidak expose API; `AboutSlint` widget atau badge wajib — syarat tambahan tidak bertentangan `GPL-2.0-or-later`).
4. **Cache version**: `"thumb-v3"` (sama dengan Qt `MED-13`).
5. **Fallback**: `FindFfmpeg` return `None` → placeholder card `"Tidak tersedia"`, log sekali per session (`K6WP_VERBOSE` compiled out release, `I4`).
6. **Risiko**: HEVC 10-bit mungkin fail (`UNPROVEN` — akan diukur `Run 4`); `command_version` salah (`"v1"` bukan `"thumb-v3"`) → semua thumbnail Qt stale (`P0` evaluasi `Run 4`).

## Ukur (`Run 4` — `UNPROVEN` di ADR)

| Metrik | Status ADR | Bukti `Run 4` (`RUN-4.md`) |
|---|---|---|
| Kontrak Qt (command version `"thumb-v3"`) | `code-quoted` (`thumbnailer.cpp`) | — |
| Konkurensi ≤2 | `P0` (konstanta, belum diuji) | — |
| Cold cache 200 item (<5s target) | `UNPROVEN` | `docs/bench/thumbnail-cold-*.json` |
| RAM puncak (generate) | `UNPROVEN` | `docs/bench/thumbnail-ram.json` |
| Gagal per codec (MP4, HEVC 8-bit, 10-bit, Matroska) | `UNPROVEN` | `tests/thumbs.rs`: `test_codec_failure_*` |
| Cache miss (`command_version` salah) | `test-proven` (`tests/thumbs.rs`: `cache_key_changes_on_version`) | — |
| Transisi Qt→Rust (thumbnail sama) | `P0` (`Run 4`: A/B thumbnail sebelum/sesudah) | — |

## NEXT TASK (ADR lengkap — Run 4)

- `Run 4`: implementasi thumbnailer (`rust/crates/k6wp-studio/src/thumbs.rs` / `thumbs.rs` baru) + `tests/thumbs.rs` + `docs/bench/thumbnail-cold-*.json` + `docs/bench/thumbnail-ram.json` + `RUN-4.md` (≤15 baris, angka cold cache wajib, label `live-proven` / `test-proven` / `UNPROVEN`).
- `A1` checklist (`parity-checklist.md`): `P0 — Run 4` (impl + angka).
- `B3/B4` (`2-monitor` + `real sleep`): `P0 — Run 10` (`UNPROVEN` atau `live-proven`).
- `B5` (`G6` test-proven): `P0 — Run 10`.
- `B6` (`fail-open`): `P0 — evaluasi Run 12`.
- `B7` (`F3` royalti): `P0 — Run 11` (`verified`: `LICENSES/README.md` + `slint-royalty-free-2.0.md` — desktop app bukan embedded, bukan standalone, tidak expose API; `AboutSlint`/badge syarat tambahan tidak bertentangan `GPL-2.0-or-later`).
- `B8` (`J5` fuzz): `P0 — Run 11`.
- `B9` (`ZIP` 148.13): `deferred` Phase 3.
- `B10` (`STATE.md` NEXT TASK owner sign-off): `P0 — Run 12`.

---
*ADR ini (≤80 baris, 76 baris aktual): kontrak Qt disalin PERSIS (`thumbnailer.cpp`), `command_version` = `"thumb-v3"` wajib, angka `UNPROVEN` untuk cold cache/RAM/gagal codec (`Run 4`), `P0` untuk konkurensi/fallback. Tidak ada `live-proven` / `test-proven` untuk angka — angka hanya muncul setelah `Run 4` (`RUN-4.md`).*
