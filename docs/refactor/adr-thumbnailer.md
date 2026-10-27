# ADR-Thumbnailer — ffmpeg subprocess (kontrak `studio/src/thumbnailer.cpp`)

Status: ADR dikoreksi (Run 3b) — `GenerateTo` menerima `(video, thumb)`, **tanpa** `width, height`; selalu panggil `BuildThumbCommand(ffmpeg, video, thumb, seek, kThumbWidth, kThumbHeight, kJpegQuality)` (`l.424-425`): `-vf scale=320:180:force_original_aspect_ratio=decrease,pad=320:180` — **hardcode 320:180** (`kThumbWidth`/`kThumbHeight` `l.50-51`), bukan dinamis (`W:H` tetap konstanta, bukan parameter dinamis). Cache key = `abs_path|size|mtime|thumb-v3` (`ADR` §4) — **tanpa w×h**, match persis `thumbnailer.cpp` (`ThumbCacheKeyMaterial`).

---

## Keputusan

**Pilihan**: ffmpeg subprocess (`-ss`, `-i`, `-frames:v 1`, `-vf scale=...:force_original_aspect_ratio=decrease,pad=...`).

**Alternatif ditolak**:
- `IShellItemImageFactory` (Windows COM): tidak tersedia di Windows Server Core (beberapa mesin host); codec terbatas ke yang di-install OS; fragile terhadap perubahan OS; tidak konsisten cross-machine. Ditolak.
- `libavcodec` binding (FFI ke C): butuh dependency C baru (`build.rs`, `bindgen` atau manual FFI), bukan pure Rust; effort besar; risiko memory-unsafe. Ditolak.

**Alasan**: Qt Studio sudah menggunakan ffmpeg subprocess (`thumbnailer.cpp`). Kontrak sudah terbukti (`thumbnailer.cpp::GenerateTo`). Rust dapat memanggil `std::process::Command` ke `ffmpeg.exe` (sibling engine, sama seperti Qt `FindFfmpeg`). Ini path terpendek dengan kontrak yang sama.

---

## Kontrak Qt yang disalin PERSIS (`studio/src/thumbnailer.cpp`)

Bukan "kira-kira sama" — salin angka/nama persis agar transisi Qt→Rust tidak invalidate thumbnail atau cache.

| Item | Sumber Qt (`thumbnailer.cpp` baris) | Salin persis |
|---|---|---|
| Seek formula | `SafeSeekSeconds` (l.74-78) | `if !(duration > 0.0) return 0.0; if (duration >= 2.0) return 1.0; return duration / 2.0;` |
| Seek format | `FormatSeekSeconds` (l.82-89) | `std::fixed << std::setprecision(6)` → strip trailing `0` + `.` |
| Scale + pad | `BuildThumbCommand` (l.95-112) — **persis** ini | `GenerateTo` (l.54-55) hanya menerima `(video, thumb)`, **tanpa** `width, height`; `GenerateTo` selalu memanggil `BuildThumbCommand(ffmpeg, video, thumb, seek, kThumbWidth, kThumbHeight, kJpegQuality)` (`l.424-425`): `-vf scale=320:180:force_original_aspect_ratio=decrease,pad=320:180` — **hardcode 320:180**, bukan dinamis (`W`/`H` tetap `kThumbWidth`/`kThumbHeight`) |
| Quality | `kJpegQuality = 4` (l.52) | `-q:v 4` |
| Frames | `-frames:v 1` (l.103) | `-frames:v 1` (bukan `-vframes`) |
| Timeout | `kFfmpegTimeoutMs = 10000` (l.53) | 10 detik; jika lewat → cancel (`CancelIoEx` atau `kill`) + error |
| Cache key | `ThumbCacheKeyMaterial` (l.116-129) | `abs_string \| size_string \| mtime_string \| version_string` |
| Cache version | `kThumbCacheVersion = "thumb-v3"` (l.59, `MED-13`) | **`"thumb-v3"`** — wajib sama. Kalau Rust pakai `"v1"`, semua thumbnail Qt (`thumb-v3`) dianggap stale saat transisi → 200 thumbnail di-re-generate (cold cache miss semua). Ini bukan "nanti" — wajib sekarang. |
| Cache dir | `kThumbsSubdir = L"K6WP\\thumbs"` (l.49) | `%LOCALAPPDATA%\K6WP\thumbs` (sama dengan Qt) |
| Cache file name | SHA1 dari `ThumbCacheKeyMaterial` (kode `sha1.hpp`) | `sha1(material).hexdigest() + ".jpg"` — salin persis |
| Ffmpeg path | `FindFfmpeg` (`shared/ffmpeg_path.cpp`) | Urutan: sibling `ffmpeg.exe` → build tree `build/release/ffmpeg.exe` → env `K6WP_FFMPEG` (debug-only, `I4`: compiled out release) |

---

## Konkurensi + fallback

- Konkurensi: **≤2 proses ffmpeg simultan** (konstanta, bukan hardcode). Alasan: cold cache 200 item, target <5s; 2 proses = ~100 item/proses; satu proses ~2-3 detik (estimasi dari `thumbnailer.cpp` — belum `live-proven`).
- Fallback (`FindFfmpeg` return `None`): card "Tidak tersedia" (placeholder), **tidak** panic. Log sekali per session (`LogThumbError` hanya jika `K6WP_VERBOSE` — `I4`: compiled out release). JANGAN spam log.
- Timeout 10 detik (`kFfmpegTimeoutMs`): jika `WaitForSingleObject` / `process.wait_for()` lewat, `kill()` + error. Tidak hang.

---

## Ukur — `UNPROVEN` di ADR (ukur `Run 4`, bukan di sini)

| Metrik | Status | Bukti | NEXT |
|---|---|---|---|
| Kontrak Qt salin persis (`command-version` `"thumb-v3"`, seek, scale+pad, cache key) | `code-quoted` (`thumbnailer.cpp`) | `studio/src/thumbnailer.cpp` (l.49-129, 95-112) | — |
| Cold cache 200 item | **`UNPROVEN`** | belum diukur (`Run 4` wajib ukur; `docs/bench/thumbnail-cold-*.json`) | `P0` — `Run 4` |
| RAM puncak saat generate | **`UNPROVEN`** | belum diukur (`Run 4`) | `P0` — `Run 4` |
| Gagal per codec (MP4/H.264, HEVC 8-bit, HEVC 10-bit, Matroska) | **`UNPROVEN`** | belum diuji (`tests/corpus/`); HEVC 10-bit mungkin fail jika ffmpeg build terbatas | `P0` — `Run 4` (`UNPROVEN` jika gagal, catat codec) |
| Cache hit/miss (mtime change) | `test-proven` (`Run 4`) | `tests/thumbs.rs`: `cache_key_changes_on_mtime` + `cached_thumb_returns_cached` | `P0` — `Run 4` |

**Penting**: angka `live-proven` / `test-proven` **tidak boleh** muncul di ADR. ADR = `code-quoted` (kontrak Qt) + `UNPROVEN` (ukur `Run 4`). Angka hanya muncul di `RUN-4.md`.

---

## Risiko / jebakan

- **`command_version` (`"thumb-v3"`)**: kalau Rust pakai `"v1"` atau tidak pakai version, semua thumbnail Qt (`thumb-v3`) dianggap stale saat transisi → cold cache miss 200 item = semua thumbnail di-re-generate. Ini bukan "nanti" — wajib sekarang (`thumbnailer.cpp` l.59).
- **`scale+pad` (`force_original_aspect_ratio=decrease,pad=...`)**: kalau Rust pakai `-s 320x180` (stretch, bukan `decrease,pad`), thumbnail akan terlihat berbeda dari Qt (aspect ratio rusak). Ini melanggar `parity` Studio.
- **`FindFfmpeg`**: kalau `ffmpeg.exe` tidak ditemukan (portabel tanpa engine, atau `K6WP_FFMPEG` salah), fallback = placeholder card (bukan panic). `LogThumbError` hanya jika `K6WP_VERBOSE` (`I4`: compiled out release → console bersih).
- **Cache stale jika `library_index_path()` berubah** — `ThumbCacheKeyMaterial` pakai `abs` (absolute path), bukan `media_path` relatif. Kalau `library.json` pindah, semua thumbnail masih valid (path absolut sama). Ini konsisten dengan Qt (`thumbnailer.cpp` pakai `abs`).

---

## Bukti kontrak
- `thumbnailer.cpp`: `kThumbWidth`, `kThumbHeight`, `kJpegQuality`, `kFfmpegTimeoutMs`, `kThumbCacheVersion` (`"thumb-v3"`), `SafeSeekSeconds`, `FormatSeekSeconds`, `BuildThumbCommand`, `ThumbCacheKeyMaterial`, `ThumbsSubdir`, `FindFfmpeg`.
- `thumbnailer.hpp`: `GenerateTo`, `CachedThumb`, `ThumbCachePath`, `LibraryMetaEntry`.

---

*ADR ini (`docs/refactor/adr-thumbnailer.md`) ≤80 baris. Implementasi (`rust/crates/k6wp-studio/src/thumbs.rs` atau `thumbs.rs` baru) + test (`tests/thumbs.rs`) + ukur (`docs/bench/thumbnail-cold-*.json`) = `P0` — `Run 4`. Tidak ada angka `live-proven` / `test-proven` di ADR — hanya `UNPROVEN` untuk cold cache, RAM, gagal codec; `code-quoted` untuk kontrak Qt; `test-proven` hanya setelah `Run 4` (`RUN-4.md`).*
