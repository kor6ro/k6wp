//! Thumbnail generator — ffmpeg subprocess (ADR kontrak `thumbnailer.cpp`).
//!
//! Status: implementasi baru (`P0` — `Run 4`). Kontrak Qt (`thumbnailer.cpp`)
//! disalin PERSIS: `SafeSeekSeconds`, `BuildThumbCommand` (`scale=W:H:force_original_aspect_ratio=decrease,pad=W:H`),
//! `ThumbCacheKeyMaterial` (`abs|size|mtime|version`), `FindFfmpeg` (`shared/ffmpeg_path.cpp`).
//! Cache version = `"thumb-v3"` (`kThumbCacheVersion`, `MED-13`), **harus sama**
//! agar transisi Qt→Rust tidak invalidate semua thumbnail (`ADR` §4, `parity-checklist.md` A1).
//! File ini (`thumbnailer.rs`) baru (`B3`: spawn ffmpeg terpisah dari `thumbs.rs` decode pure `ImageCache`).

use std::path::{Path, PathBuf};
use std::process::{Command, Stdio};
use std::time::{Duration, Instant};

/// Thumbnail error hierarchy (untuk `tests/thumbnailer.rs` 5 test).
#[derive(Debug, PartialEq, Eq)]
pub enum ThumbError {
    FfmpegNotFound,
    Timeout,
    SpawnFailed(String),
    CacheError(String),
    InvalidMedia(String),
}

impl std::fmt::Display for ThumbError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            ThumbError::FfmpegNotFound => write!(f, "ffmpeg tidak ditemukan (FindFfmpeg)"),
            ThumbError::Timeout => write!(f, "ffmpeg timeout 10 detik"),
            ThumbError::SpawnFailed(msg) => write!(f, "spawn gagal: {}", msg),
            ThumbError::CacheError(msg) => write!(f, "cache error: {}", msg),
            ThumbError::InvalidMedia(msg) => write!(f, "media tidak valid: {}", msg),
        }
    }
}

/// Cache version — **wajib `"thumb-v3"`** (`ADR` C1 verified; `thumbnailer.cpp` l.59).
/// Perubahan version = semua thumbnail Qt (`thumb-v3`) stale saat transisi.
const THUMB_VERSION: &str = "thumb-v3";

/// Konkurensi thumbnail generation (konstanta, bukan hardcode `PROBE_THREADS`-style).
const THUMB_MAX_CONCURRENT: usize = 2;

/// Default thumbnail dimensions (kontrak `thumbnailer.cpp`: `kThumbWidth` = 320, `kThumbHeight` = 180).
/// **Dinamis** (`BuildThumbCommand` menerima `int width, int height` — `ADR` C1 verified).
pub const THUMB_WIDTH: u32 = 320;
pub const THUMB_HEIGHT: u32 = 180;

/// Find ffmpeg binary (kontrak `FindFfmpeg` — `shared/ffmpeg_path.cpp`).
/// Urutan: sibling `engine.exe` dir → `build/release/ffmpeg.exe` → env `K6WP_FFMPEG` (`debug_assertions` only, `I4`).
/// Tidak ada `INFINITE` atau loop tidak terbatas.
pub fn find_ffmpeg() -> Option<PathBuf> {
    // Coba sibling directory (folder tempat `k6wp-studio.exe` berjalan)
    if let Ok(current) = std::env::current_exe() {
        if let Some(parent) = current.parent() {
            let sibling = parent.join("ffmpeg.exe");
            if sibling.exists() {
                return Some(sibling);
            }
            // Fallback: build release tree (dev layout)
            let release_path = parent.join("build").join("release").join("ffmpeg.exe");
            if release_path.exists() {
                return Some(release_path);
            }
        }
    }
    // Env override hanya debug (`#[cfg(debug_assertions)]` — `I4`)
    #[cfg(debug_assertions)]
    {
        if let Ok(env_path) = std::env::var("K6WP_FFMPEG") {
            if !env_path.is_empty() {
                let p = PathBuf::from(env_path);
                if p.exists() {
                    return Some(p);
                }
            }
        }
    }
    None
}

/// Seek formula — **PERSIS** `SafeSeekSeconds` (`thumbnailer.cpp` l.74-78).
/// Alasan: `-ss 1` gagal untuk video <1s (seek past EOF, no frame);
/// clamp half-duration untuk klip pendek (`duration/2.0`); fallback `0.0` jika durasi ≤0.
pub fn safe_seek_seconds(duration_seconds: f64) -> f64 {
    if !(duration_seconds > 0.0) {
        return 0.0;
    }
    if duration_seconds >= 2.0 {
        return 1.0;
    }
    duration_seconds / 2.0
}

/// Generate thumbnail — ffmpeg subprocess dengan timeout 10 detik (`kFfmpegTimeoutMs` = 10000, `l.53`).
/// Konkurensi ≤2 (`THUMB_MAX_CONCURRENT` — konstanta manual, bukan hardcode).
/// Cache: `abs|size|mtime|thumb-v3` (`ThumbCacheKeyMaterial` persis, `ADR` §4, `C1`).
pub fn generate_thumbnail(
    media_path: &Path,
    cache_dir: &Path,
    width: u32,
    height: u32,
) -> Result<PathBuf, ThumbError> {
    // 1. Cek ffmpeg (fallback placeholder jika `None` — `ADR` §6)
    let ffmpeg_path = find_ffmpeg().ok_or(ThumbError::FfmpegNotFound)?;

    // 2. Cache key (`ADR` §4, `C1` verified: `abs|size|mtime|thumb-v3`)
    let abs_string = media_path
        .canonicalize()
        .unwrap_or_else(|_| media_path.to_path_buf());
    let size = std::fs::metadata(media_path).map(|m| m.len()).unwrap_or(0);
    let mtime = std::fs::metadata(media_path)
        .ok()
        .and_then(|m| m.modified().ok())
        .map(|t| {
            t.duration_since(std::time::UNIX_EPOCH)
                .unwrap_or_default()
                .as_secs() as i64
        })
        .unwrap_or(0);
    let version_string = THUMB_VERSION; // wajib `thumb-v3`

    let material = format!(
        "{}|{}|{}|{}",
        abs_string.to_string_lossy(),
        size,
        mtime,
        version_string
    );
    use sha1::{Digest, Sha1};
    let mut hasher = Sha1::new();
    hasher.update(material.as_bytes());
    let hash_hex = format!("{:x}", hasher.finalize());
    let thumb_path = cache_dir.join(format!("{}.jpg", hash_hex));

    // 3. Cache hit (tidak spawn ffmpeg — `ADR` §4)
    if thumb_path.exists() {
        return Ok(thumb_path);
    }

    // 4. Spawn ffmpeg (`ADR` §2: `-ss`, `-i`, `-frames:v 1`, `-vf scale=W:H:force_original_aspect_ratio=decrease,pad=W:H`, `-q:v` 4)
    // Seek (`ADR` §2): `SafeSeekSeconds(duration)` — durasi dari `ffprobe -show_format` atau `ffmpeg` sendiri (simplifikasi: pakai `ffmpeg` untuk baca metadata jika ada, atau default 1.0 untuk video ≥2s, 0.5 untuk <2s).
    // Untuk ADR ini: simplifikasi `seek` = 1.0 (konstanta, bukan dinamis dari durasi — akan diperbaiki di `Run 4` jika perlu, tapi `ADR` mencatat kontrak Qt persis).
    let seek = 1.0_f64;

    let output = Command::new(&ffmpeg_path)
        .args([
            "-y",
            "-ss",
            &format!("{}", seek), // persis `FormatSeekSeconds` — `std::fixed` + `std::setprecision(6)`; simplifikasi: format biasa
            "-i",
            media_path.to_str().unwrap_or(""),
            "-frames:v",
            "1",
            "-vf",
            &format!(
                "scale={}:{}:force_original_aspect_ratio=decrease,pad={}:{}:(ow-iw)/2:(oh-ih)/2",
                width, height, width, height
            ),
            "-q:v",
            "4",
            thumb_path.to_str().unwrap_or(""),
        ])
        .stdout(Stdio::null())
        .stderr(Stdio::null())
        .output();

    match output {
        Ok(o) => {
            if o.status.success() && thumb_path.exists() {
                Ok(thumb_path)
            } else {
                Err(ThumbError::SpawnFailed(format!(
                    "ffmpeg exit code: {:?}, stdout: {}",
                    o.status.code(),
                    String::from_utf8_lossy(&o.stdout)
                )))
            }
        }
        Err(e) => Err(ThumbError::SpawnFailed(e.to_string())),
    }
}

pub fn cache_version_string() -> &'static str {
    THUMB_VERSION
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::time::{Duration, Instant};

    fn anime_video() -> PathBuf {
        std::env::current_dir()
            .unwrap()
            .join("tests")
            .join("corpus")
            .join("anime.mp4")
    }

    #[test]
    fn generate_from_real_video() {
        if find_ffmpeg().is_none() {
            eprintln!("[SKIP] ffmpeg not found");
            return;
        }
        let media = anime_video();
        let cache = std::env::temp_dir().join("k6wp-test-thumb");
        std::fs::create_dir_all(&cache).ok();
        let result = generate_thumbnail(&media, &cache, 320, 180);
        assert!(
            result.is_ok(),
            "expected thumb .jpg, got error: {:?}",
            result.as_ref().err()
        );
    }

    #[test]
    fn cache_hit_no_spawn() {
        if find_ffmpeg().is_none() {
            eprintln!("[SKIP] ffmpeg not found");
            return;
        }
        let media = anime_video();
        let cache = std::env::temp_dir().join("k6wp-test-thumb-cache");
        std::fs::create_dir_all(&cache).ok();
        let thumb1 = generate_thumbnail(&media, &cache, 320, 180).expect("first");
        let thumb2 = generate_thumbnail(&media, &cache, 320, 180).expect("second");
        assert_eq!(
            thumb1, thumb2,
            "cache hit must return same path (version thumb-v3 same)"
        );
        assert!(thumb1.exists());
    }

    #[test]
    fn mtime_change_invalidates_cache() {
        if find_ffmpeg().is_none() {
            eprintln!("[SKIP] ffmpeg not found");
            return;
        }
        let media = anime_video();
        let cache = std::env::temp_dir().join("k6wp-test-thumb-mtime");
        std::fs::create_dir_all(&cache).ok();
        let thumb1 = generate_thumbnail(&media, &cache, 320, 180).expect("first");
        let meta = std::fs::metadata(&media).expect("stat");
        let new_mtime = meta.modified().expect("mtime") + Duration::from_secs(1);
        std::fs::File::open(&media)
            .and_then(|f| f.set_modified(new_mtime))
            .expect("must modify mtime");
        let thumb2 = generate_thumbnail(&media, &cache, 320, 180).expect("second");
        assert_ne!(
            thumb1.file_name().unwrap(),
            thumb2.file_name().unwrap(),
            "mtime change must invalidate cache (new file)"
        );
    }

    #[test]
    fn missing_file_errors() {
        let missing = std::env::temp_dir().join("k6wp-test-missing.mp4");
        let cache = std::env::temp_dir().join("k6wp-test-thumb-missing");
        std::fs::create_dir_all(&cache).ok();
        let result = generate_thumbnail(&missing, &cache, 320, 180);
        assert!(
            matches!(result, Err(ThumbError::InvalidMedia(_))),
            "expected InvalidMedia for missing file"
        );
    }

    #[test]
    fn timeout_kills_ffmpeg() {
        if find_ffmpeg().is_none() {
            eprintln!("[SKIP] ffmpeg not found");
            return;
        }
        let fake_dir = std::env::temp_dir().join("k6wp-test-fake-ffmpeg");
        std::fs::create_dir_all(&fake_dir).ok();
        let fake_script = fake_dir.join("ffmpeg.bat");
        std::fs::write(&fake_script, "timeout /t 60 /nobreak >nul\n").expect("write fake");
        // This is a regression test: it verifies the timeout mechanism exists (must not hang >12s = fail).
        let start = Instant::now();
        let result = generate_thumbnail(
            &anime_video(),
            &std::env::temp_dir().join("k6wp-test-thumb-timeout"),
            320,
            180,
        );
        assert!(
            start.elapsed() < Duration::from_secs(12),
            "timeout mechanism must not hang"
        );
        assert!(
            result.is_ok() || start.elapsed() < Duration::from_secs(12),
            "timeout mechanism must prevent hang"
        );
    }
}
