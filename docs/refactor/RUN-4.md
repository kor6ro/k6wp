# RUN-4 — Thumbnailer (impl + test, Opsi A split, 2026-10-27)

**Branch**: `refactor/rust-wt`. **Deliverable**: `thumbnailer.rs` baru + `tests/thumbnailer.rs` + `parity-checklist.md` A1 update (`P0 — Run 4` + `P0 — Run 4b`). **ADR** (`adr-thumbnailer.md`) sudah `done` (Run 3, 76 baris, `thumb-v3`, `UNPROVEN` angka). Kontrak Qt (`thumbnailer.cpp`) disalin persis — **C1 verified** (`thumb-v3`, `BuildThumbCommand` `W:H` dinamis `int width, height`, `ThumbCacheKeyMaterial` `abs|size|mtime|version`, `FindFfmpeg` `None` → placeholder). **Tidak** hardcode `320:180` (dinamis dari parameter). **F3**: `verified` (`royalty-free` kompatibel `GPL-2.0`). **Biaya**: 0 pengukuran baru (angka `UNPROVEN` — `Run 4b`).

---

## Bagian 1 — Implementasi (`thumbnailer.rs` baru)

File baru: `rust/crates/k6wp-studio/src/thumbnailer.rs`. **Bukan** modifikasi `thumbs.rs` (decode + cache tetap terpisah).

```rust
pub enum ThumbError { FfmpegNotFound, Timeout, SpawnFailed(String), CacheError(String), InvalidMedia(String) }
pub fn generate_thumbnail(media_path: &Path, cache_dir: &Path, width: u32, height: u32) -> Result<PathBuf, ThumbError>;
pub fn find_ffmpeg() -> Option<PathBuf>; // sibling engine.exe dir -> build/release -> K6WP_FFMPEG env (debug-only, I4)
```

**Parameter ffmpeg** (`-vf`): `scale={width}:{height}:force_original_aspect_ratio=decrease,pad={width}:{height}:(ow-iw)/2:(oh-ih)/2` (`BuildThumbCommand` l.104-108, `thumbnailer.cpp`). **Bukan** `-s 320x180` (stretch).

**Cache key**: `sha1(material).hexdigest() + ".jpg"` (`ThumbCacheKeyMaterial` l.116-129). `material` = `abs_path_string | size_string | mtime_string | version_string`. `version` = `kThumbCacheVersion` (`thumbnailer.cpp` l.59) = `"thumb-v3"`. **Wajib sama** (`ADR` sudah tulis; kalau beda → semua thumbnail Qt stale saat transisi).

**Konkurensi**: `Mutex<u32>` counter atau `std::sync::Arc<Semaphore>` manual (≤2). Timeout: 10 detik (`kFfmpegTimeoutMs` l.53). Fallback (`find_ffmpeg()` `None`): placeholder card `"Tidak tersedia"`, log sekali per session (`LogThumbError` hanya jika `K6WP_VERBOSE`, `I4`: compiled out release).

**Worker wiring** (`main.rs` / `worker.rs`): `Request::Thumbnail { index, media_path, width, height }` (baru, beda dari `Thumb`). Handler: panggil `generate_thumbnail`, jika sukses → `Event::Thumb` + `Thumb` decode (`thumbs.rs`); jika gagal → `Event::Thumb { index, result: Err(msg) }`.

---

## Bagian 2 — Test (`tests/thumbnailer.rs`)

5 test minimum (`test-proven`):
1. `generate_from_real_video` — `tests/corpus/anime.mp4` → `.jpg` ada, pixel non-flat (`thumbs::decode_to_cover` verifikasi).
2. `cache_hit_no_spawn` — 2x panggil (`media_path` sama, `mtime` sama), panggilan ke-2 return cached, tidak spawn ffmpeg 2x.
3. `mtime_change_invalidates_cache` — ubah `mtime` (`std::fs::File::set_modified_time` atau `touch`) → cache miss (file baru).
4. `missing_file_errors` — `ThumbError::InvalidMedia` (tidak butuh ffmpeg; jalan selalu).
5. `timeout_kills_ffmpeg` — fake ffmpeg (batch sleep 60s atau binary palsu) → timeout 10s, child kill, `ThumbError::Timeout`.

Skip test 1, 2, 3, 5 jika `find_ffmpeg()` = `None` (`eprintln!` + `return`). Test 4 (missing file) **harus** jalan tanpa ffmpeg.

---

## Bagian 3 — Cold cache 200 (`P0` — `Run 4b`, `UNPROVEN` di ADR)

**Tidak diukur di Run 4** (`UNPROVEN`). Tulis eksplisit di `RUN-4.md`:
- Setup: `tools/gen_library_200.ps1` (`hardlink`, bukan `copy`; `tests/corpus/` 17 file `gitignored`, regenerasi `make_corpus.ps1`).
- Timing: cold 200 <5s target (`live-proven` jika tercapai, `UNPROVEN` jika gagal — angka aktual wajib dicatat, bukan target saja).
- RAM puncak (`Process.PrivateMemorySize64` sebelum/sesudah).
- `K6WP_THUMBS_DIR` override (`debug_assertions` only, `I4`) untuk isolasi.
- `parity-checklist.md` `A1` update: `P0 — Run 4` (`impl+test`) + `P0 — Run 4b` (`cold cache 200 + RAM`).

---

## Verifikasi sebelum commit

1. C1: `grep` `thumbnailer.cpp` (`thumb-v3`, `BuildThumbCommand` persis, `ThumbCacheKeyMaterial`).
2. `thumbnailer.rs` baru (`thumbnailer.rs`) → `thumbs.rs` tidak dimodifikasi (`thumbs.rs` tetap `decode_to_cover`).
3. `cargo test -p k6wp-studio -- --test-threads=1`: 4 fake-server OK + 5 thumbnail OK (jika ffmpeg ada) / skip OK (jika tidak).
4. `cargo clippy -p k6wp-studio -- -D warnings`: clean.
5. `cargo fmt --check`: clean.
6. `RUN-4.md` ≤15 baris (`wajib`: angka `UNPROVEN` atau `live-proven`, `command_version` `"thumb-v3"`, `P0` `Run 4b`).
7. `git status`: hanya `thumbnailer.rs` baru, `tests/thumbnailer.rs`, `worker.rs` (update `Request::Thumbnail`), `main.rs` (handler), `RUN-4.md`, `parity-checklist.md` (`A1` update).

---

## NEXT TASK (Run 4 → Run 4b, jika Opsi A split)

- **Run 4** (`P0`): implementasi thumbnailer + test (`RUN-4.md`: angka `UNPROVEN` wajib).
- **Run 4b** (`P0` jika split): cold cache 200 item (`thumbnail-cold-200.json`, `live-proven` / `UNPROVEN` — angka aktual wajib).
- `B3/B4` (`2-monitor` + `real sleep`): `P0 — Run 10` (`UNPROVEN` atau `live-proven`).
- `B5` (`G6` test-proven): `P0 — Run 10` (`tests/live_engine.rs` `K6WP_IPC_LIVE=1`).
- `B6` (`fail-open`): `P0 — evaluasi Run 12`.
- `B7` (`F3` royalti): `P0 — Run 11` (`verified`: `LICENSES/README.md` + `slint-royalty-free-2.0.md` — desktop app bukan embedded, bukan standalone, tidak expose API; `AboutSlint`/badge syarat tambahan tidak bertentangan `GPL-2.0-or-later`; `LICENSE` tetap).
- `B8` (`J5` fuzz): `P0 — Run 11`.
- `B9` (`ZIP` 148.13): `deferred` Phase 3 (`budgets.md` L10).
- `B10` (`STATE.md` NEXT TASK owner sign-off): `P0 — Run 12`.

---
*ADR ini (`docs/refactor/adr-thumbnailer.md`, 76 baris): kontrak `thumbnailer.cpp` disalin PERSIS (`thumb-v3`, `pad=W:H`, `SafeSeekSeconds`, `command_version` wajib), angka `UNPROVEN` (`Run 4b`), `P0` konkurensi/fallback (`Run 4`). Tidak ada `live-proven` / `test-proven` untuk angka — angka hanya setelah `Run 4` (`RUN-4.md`).*
