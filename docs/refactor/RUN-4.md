# RUN-4 — Thumbnailer (impl + test, Opsi A split, 2026-10-27)

**Branch**: `refactor/rust-wt`. **ADR** (`adr-thumbnailer.md`, `bd213b0` koreksi): `GenerateTo` hanya `(video, thumb)` → `BuildThumbCommand` dipanggil dengan `kThumbWidth=320, kThumbHeight=180` (`hardcode`, bukan `int width,height` dinamis). Cache key (`ThumbCacheKeyMaterial`): `abs_path|size|mtime|thumb-v3` — **tanpa w×h** (match Qt `thumbnailer.cpp`, `ADR` §4, C1 verified). **Biaya**: 1 run (`P0` — Run 4: impl + test), `P0` — Run 4b (cold cache 200 `UNPROVEN`). **Opsi A split**: total 13 run (`~$9.75` < `$10`, buffer `$0.25`). `B2`: syarat jujur — `UNPROVEN` dicatat eksplisit (`ADR` §7, `RUN-4.md` §3). **Tidak rework** (budget tight — tanpa buffer untuk error besar).

---

## 1. Implementasi (`rust/crates/k6wp-studio/src/thumbnailer.rs` baru, `B3`)

`thumbnailer.rs` baru (`B3`: spawn ffmpeg terpisah dari `thumbs.rs` decode pure):
- `pub fn generate_thumbnail(media_path: &Path, cache_dir: &Path) -> Result<PathBuf, ThumbError>` (`generate_thumbnail` tanpa `width, height` param — `GenerateTo` hardcode `kThumbWidth`/`kThumbHeight`).
- Cache key: `SHA256(format!("{}|{}|{}|{}", abs_string, size, mtime, "thumb-v3"))` (`version_string` = `"thumb-v3"`, wajib sama `kThumbCacheVersion` `l.59`).
- `pub fn find_ffmpeg()` (`FindFfmpeg` persis: sibling `engine.exe` dir → `build/release/ffmpeg.exe` → `K6WP_FFMPEG` env `debug_assertions` only, `I4`).
- Konkurensi ≤2 (`Mutex<u32>` counter manual, bukan `tokio`). Timeout 10 detik (`kFfmpegTimeoutMs` = 10000). Fallback (`find_ffmpeg()` `None`): placeholder card, log sekali per session (`K6WP_VERBOSE` compiled out release).
- `Request::Thumbnail { index: usize, media_path: PathBuf }` (baru, beda dari `Request::Thumb` decode JPG/PNG existing). Handler (`main.rs` / `worker.rs`): `thumbnailer::generate_thumbnail` → `decode_to_cover` → `Event::Thumb`.

---

## 2. Test (`tests/thumbnailer.rs` — 5 test `test-proven`)

1. `generate_from_real_video` — `tests/corpus/anime.mp4` → `.jpg` ada, decode non-flat (`thumbs::decode_to_cover`). Skip jika `find_ffmpeg()` `None` (`eprintln!` + `return`).
2. `cache_hit_no_spawn` — 2× panggil (`media_path` sama, `mtime` sama) → panggilan ke-2 cached, spawn count = 1 (counter test-only).
3. `mtime_change_invalidates_cache` — ubah `mtime` (`File::set_modified_time`) → cache miss (file baru).
4. `missing_file_errors` — `ThumbError::InvalidMedia` (`find_ffmpeg()` tidak perlu; selalu jalan).
5. `timeout_kills_ffmpeg` — fake `.bat` sleep 60s → timeout 10s, child kill (`process.kill()`), `ThumbError::Timeout`. Skip jika `find_ffmpeg()` `None`.

## 3. Ukur (`P0` — `Run 4b`, semua `UNPROVEN` di ADR)

`RUN-4.md` wajib sebut (`≤15` baris):
- Cold cache 200 item: `UNPROVEN` (`docs/bench/thumbnail-cold-200.json` belum ada; `P0` `Run 4b`; setup `tools/gen_library_200.ps1` hardlink; `K6WP_THUMBS_DIR` `debug_assertions` `I4`).
- RAM puncak (`thumbnail-ram.json`): `UNPROVEN` (`P0` `Run 4b`).
- Gagal per codec (`tests/thumbs.rs` `test_codec_failure_*`): `UNPROVEN` (`P0` `Run 4b`).
- Transisi Qt→Rust: `P0` (`Run 4`: A/B thumbnail sebelum/sesudah — `parity-checklist.md` A1 `P0` `Run 4`).

## B2 — syarat jujur (context habis)

Kalau context habis di tengah: `RUN-4.md` wajib tulis `UNPROVEN (Run 4b — context exhausted)`. Jangan klaim "semua deliverable". `NEXT TASK`: `Run 4b: cold cache 200`. Budget total: 14 run (`~$10.50`), buffer `$0` → tight, tapi cukup (tidak rework).

---

## NEXT TASK

`Run 4b`: `thumbnail-cold-200.json` (`live-proven` atau `UNPROVEN`) + `thumbnail-ram.json`. `B3/B4` (`2-monitor` + `real sleep`): `UNPROVEN` (`Run 10`). `B5` (`G6` `test-proven`): `Run 10`. `B6` (`fail-open`): `Run 12`. `B7` (`F3`): `Run 11` (`verified`). `B8` (`J5`): `Run 11`. `B9` (`ZIP`): `deferred`. `B10` (`STATE.md` NEXT TASK): `Run 12`. `A2-A5` (`import`/`compress`/`settings`/`assign`): `P0` `Run 5-8`.
