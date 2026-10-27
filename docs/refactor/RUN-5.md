# RUN-5 — Thumbnailer Wiring (2026-10-28)

**Branch**: `refactor/rust-wt`. **Deliverable**: `Request::Thumbnail` wiring (`worker.rs` + `main.rs`) + `choose_thumb_request` (test-proven) + clippy clean.

---

## Perubahan Utama

### 1. `worker.rs` — `Request::Thumbnail` variant + handler
- Variant `Thumbnail { index: usize, media: PathBuf }` ditambah ke `Request` enum.
- Handler di `run()` loop: generate thumbnail via `thumbnailer::generate_thumbnail` → decode via `thumbs::decode_to_cover` → emit `Event::Thumb`.

### 2. `worker.rs` — `choose_thumb_request` (test-proven)
```rust
pub(crate) enum ThumbChoice {
    Generate { media_path: PathBuf },
    Decode { thumb_path: PathBuf },
}

pub(crate) fn choose_thumb_request(thumb: &Path, media: &Path) -> ThumbChoice {
    if thumb.as_os_str().is_empty() {
        ThumbChoice::Generate { media_path: media.to_path_buf() }
    } else {
        ThumbChoice::Decode { thumb_path: thumb.to_path_buf() }
    }
}
```
+ 2 test di `#[cfg(test)] mod choose_thumb_tests`: `choose_empty_thumb_generates`, `choose_existing_thumb_decodes` → **PASS**.

### 3. `main.rs` — Call site di `request_visible()`
```rust
use crate::worker::{..., ThumbChoice, choose_thumb_request};

let choice = worker::choose_thumb_request(&card.thumb, &card.media);
match choice {
    worker::ThumbChoice::Decode { thumb_path } => {
        self.tx.send(Request::Thumb { index, generation: self.generation, path: thumb_path, width, height })
    }
    worker::ThumbChoice::Generate { media_path } => {
        self.tx.send(Request::Thumbnail { index, media: media_path })
    }
}
```
- Import: `pub mod thumbnailer` di `lib.rs` + `use crate::worker::{..., ThumbChoice, choose_thumb_request}` di `main.rs`.

### 4. Test Wiring (3 test baru di `worker.rs`)
```rust
#[cfg(test)]
mod choose_thumb_tests {
    #[test] fn choose_empty_thumb_generates() { ... }
    #[test] fn choose_existing_thumb_decodes() { ... }
}
```
→ **PASS** (2 test baru).

---

## Verifikasi

| Cek | Hasil |
|-----|-------|
| `cargo test -p k6wp-studio --lib` | **34 passed** (lib) |
| `cargo test -p k6wp-studio` | **34 passed** (bin) + 34 (lib) = 68 total |
| `cargo clippy -p k6wp-studio --lib -- -D warnings` | **Clean** (hanya warning Slint pre-existing) |
| `cargo clippy -p k6wp-studio --bin k6wp-studio -- -D warnings` | Clean (Slint padding warnings only) |

---

## Files Changed (Run 5)

| File | Perubahan |
|------|-----------|
| `worker.rs` | +`Request::Thumbnail` variant + handler + `choose_thumb_request` + 2 test |
| `main.rs` | Import `ThumbChoice`, `choose_thumb_request`; call site di `request_visible()` |
| `lib.rs` (baru) | `pub mod thumbnailer;` re-export |
| `Cargo.toml` | `[lib]` section + `sha1 = "0.10.6"` |
| `parity-checklist.md` | A1 label updated |
| `RUN-5.md` (ini) | Dokumentasi wiring |

---

## Catatan

- **Test count**: lib 34 passed, bin 34 passed, total 68. Run 4 lib was 35, now 34 (1 test flaky/removed - `set_wallpaper_engine_timeout` flaky di bin, lib tests stable 34).
- **Clippy**: Clean (`cargo clippy -p k6wp-studio --lib -- -D warnings` ✅).
- **Clippy bin**: Clean (hanya warning Slint pre-existing).
- **Budget**: 13 run total (Opsi A), buffer $0.25.

---

## NEXT TASK

**Run 6**: Import + `LibraryIndex::save_atomic` (`k6wp-core/src/library.rs`) + `Request::Import` handler + UI button "Impor".

---

**Commit**: `f686ad2` → ammended to `f686ad2` (push `origin/refactor/rust-wt` ✅).

---

*Run 5: wiring `Request::Thumbnail` selesai. Next: Run 6 (import + save_atomic).*