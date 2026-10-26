# Thumbnailer — Rust thumbnail generator (ADR kontrak Qt `thumbnailer.cpp`)

Status: ADR `docs/refactor/adr-thumbnailer.md` (Run 3, 76 baris) sudah `done`. Kontrak Qt (`thumbnailer.cpp`) disalin persis (`thumb-v3`, `pad=W:H` dinamis `int width,height` dari `BuildThumbCommand`, `SafeSeekSeconds`, `ThumbCacheKeyMaterial`). File ini (`thumbnailer.rs`) baru (`B3` — spawn ffmpeg terpisah dari `thumbs.rs` decode pure).

**Angka**: semua `UNPROVEN` (`P0` — `Run 4b`). Tidak ada `live-proven` / `test-proven` sebelum implementasi + test (`tests/thumbnailer.rs`).

---

## 1. Kontrak (disalin PERSIS dari `thumbnailer.cpp` — C1 verified)

- Seek: `SafeSeekSeconds(duration)`: `if !(duration > 0.0) return 0.0; if (duration >= 2.0) return 1.0; return duration / 2.0;` (l.74-78).
- Seek format: `std::fixed << std::setprecision(6)` → strip trailing `0` + `.` (l.82-89).
- Scale+pad: `scale={width}:{height}:force_original_aspect_ratio=decrease,pad={width}:{height}:(ow-iw)/2:(oh-ih)/2` (`W=320`=`kThumbWidth`, `H=180`=`kThumbHeight`, `int width, int height` dinamis, `l.95-112`). **Bukan** `-s 320x180` (stretch).
- Frames: `-frames:v 1` (`l.103`, bukan `-vframes`).
- Quality: `-q:v 4` (`kJpegQuality`, `l.52`).
- Timeout: 10 detik (`kFfmpegTimeoutMs` = 10000, `l.53`).
- Cache key: `abs_path_string | size_string | mtime_string | version_string` (`ThumbCacheKeyMaterial`, `l.116-129`). **Versi** = `"thumb-v3"` (`kThumbCacheVersion`, `l.59`, `MED-13`) — wajib sama agar transisi Qt→Rust tidak invalidate semua thumbnail.
- Cache dir: `%LOCALAPPDATA%\K6WP\thumbs` (`kThumbsSubdir` = `L"K6WP\thumbs"`, `l.49`).
- Cache file: `sha1(material).hexdigest() + ".jpg"` (`ThumbCachePath`, `thumbnailer.hpp`).
- Ffmpeg path: `FindFfmpeg` (`shared/ffmpeg_path.cpp`): sibling `ffmpeg.exe` → `build/release/ffmpeg.exe` → env `K6WP_FFMPEG` (`debug_assertions` only, `I4`: compiled out release).

## 2. Implementasi (`thumbnailer.rs` baru — `P0` — `Run 4`)

Struktur:

```rust
pub enum ThumbError { FfmpegNotFound, Timeout, SpawnFailed(String), CacheError(String), InvalidMedia(String) }
pub fn generate_thumbnail(media_path: &Path, cache_dir: &Path, width: u32, height: u32) -> Result<PathBuf, ThumbError>;
pub fn find_ffmpeg() -> Option<PathBuf>;
```

Implementasi `generate_thumbnail`:
1. Cek `find_ffmpeg()` → `ThumbError::FfmpegNotFound` jika `None` (fallback placeholder card).
2. Cache key: `sha1(path_string + '|' + size_string + '|' + mtime_string + '|thumb-v3')` → hex → `cache_dir.join(hex + ".jpg")`.
3. Cache hit (`.exists()`): return path langsung (tidak spawn ffmpeg).
4. Cache miss: spawn ffmpeg (`std::process::Command::new(ffmpeg)`) dengan `-ss`, `-i`, `-frames:v 1`, `-vf scale={w}:{h}:force_original_aspect_ratio=decrease,pad={w}:{h}`, `-q:v 4`, dst=`thumb_path`.
5. Konkurensi: `Mutex<u32>` counter (manual semaphore) — ≤2 proses simultan. `wait_for_child` loop 50ms; kill setelah 10 detik (`kFfmpegTimeoutMs`).
6. Timeout → `ThumbError::Timeout` (`CancelIoEx` / `kill`). Tidak hang.

File baru (`B3` — `thumbnailer.rs`, bukan `thumbs.rs`): spawn ffmpeg (`process` I/O) terpisah dari `thumbs.rs` (pure decode `ImageCache`). `thumbs.rs` tetap (`decode_to_cover`, `ImageCache`). `main.rs` / `worker.rs`: `Request::Thumbnail` baru (beda dari `Thumb`).

## 3. Test (`tests/thumbnailer.rs` — `P0` — `Run 4`)

5 test (`test-proven`):
1. `generate_from_real_video`: `tests/corpus/anime.mp4` → `.jpg` ada, pixel non-flat (`thumbs::decode_to_cover` verifikasi). Skip jika `find_ffmpeg()` `None`.
2. `cache_hit_no_spawn`: panggil 2x, `mtime` sama → panggilan ke-2 return cached, tidak spawn ffmpeg 2x.
3. `mtime_change_invalidates_cache`: ubah `mtime` (`File::set_modified_time`) → cache miss.
4. `missing_file_errors`: `ThumbError::InvalidMedia` (tidak butuh ffmpeg — selalu jalan).
5. `timeout_kills_ffmpeg`: fake ffmpeg (`.bat` sleep 60s) → timeout 10s, child kill, `ThumbError::Timeout`. Skip jika `find_ffmpeg()` `None`.

## 4. Ukur (`P0` — `Run 4b` — `UNPROVEN` di ADR, `live-proven` / `test-proven` setelah `Run 4`)

Tidak ada angka `live-proven` / `test-proven` di ADR. Angka wajib `RUN-4.md`:

| Metrik | Status ADR | Bukti `RUN-4.md` |
|---|---|---|
| Kontrak Qt (`thumb-v3`, seek, scale+pad, cache key) | `code-quoted` (`thumbnailer.cpp`) | — |
| Konkurensi ≤2 | `P0` (konstanta, belum diuji) | — |
| Cold cache 200 item (<5s target) | `UNPROVEN` | `docs/bench/thumbnail-cold-200.json` |
| RAM puncak (generate) | `UNPROVEN` | `docs/bench/thumbnail-ram.json` |
| Gagal per codec (MP4/H.264, HEVC 8-bit, 10-bit?, Matroska) | `UNPROVEN` | `tests/thumbs.rs`: `test_codec_failure_*` |
| Cache hit (version `thumb-v3` = `v3`) | `test-proven` (`tests/thumbs.rs`: `cache_key_changes_on_version`) | — |
| Transisi Qt→Rust (thumbnail sama) | `P0` (`Run 4`: A/B thumbnail sebelum/sesudah) | — |

`K6WP_THUMBS_DIR` (`debug_assertions` only, `I4`): override untuk isolasi measurement. Release: `build/msvc-dev` / `D:\cargo-target2\release` tetap bersih (`I4`: compiled out).

## NEXT TASK (`RUN-4.md` — `P0`)

- `Run 4`: implementasi (`thumbnailer.rs`) + test (`tests/thumbs.rs`) + kompilasi (`cargo check`/`clippy`) + `RUN-4.md` (≤15 baris, angka `UNPROVEN` wajib dicatat).
- `Run 4b`: cold cache 200 item (`thumbnail-cold-200.json`, `live-proven` atau `UNPROVEN`) + RAM (`thumbnail-ram.json`).
- `A1` checklist (`parity-checklist.md`): `P0 — Run 4` (impl+test) + `P0 — Run 4b` (angka).
- `B3/B4` (`2-monitor` + `real sleep`): `UNPROVEN` (`P0` `Run 10` atau manual).
- `B5` (`G6` test-proven): `P0 — Run 10` (`K6WP_IPC_LIVE=1`).
- `B6` (`fail-open`): `P0 — evaluasi Run 12` (`parity-checklist.md` `B6`).
- `B7` (`F3` royalti): `P0 — Run 11` (`verified`: `LICENSES/README.md` + `slint-royalty-free-2.0.md` — desktop app bukan embedded, bukan standalone, tidak expose API; `AboutSlint` widget atau badge wajib — syarat tambahan tidak bertentangan `GPL-2.0-or-later`; `LICENSE` tetap).
- `B8` (`J5` fuzz): `P0 — Run 11`.
- `B9` (`ZIP` 148.13 MB): `deferred` Phase 3 (`budgets.md` L10).
- `B10` (`STATE.md` NEXT TASK owner sign-off): `P0 — Run 12`.
- `A11`: `Studio RAM/startup E1 final` (`P0` `Run 9` — `budget.md` `2.7` / `2.3`).
