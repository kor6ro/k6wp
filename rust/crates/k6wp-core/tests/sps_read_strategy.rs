//! Task I2: the parser's READ STRATEGY and the container policy.
//!
//! WHAT IS PROVEN HERE
//!
//! 1. The container walk SEEKS: a file whose `moov` is at the END (a
//!    non-faststart MP4 - the normal case for a file that was never
//!    optimized) is still found, and a file with a 512 MB `mdat` costs
//!    KILOBYTES of reads. The report carries `bytes_read`, so the cost is
//!    observable and not a promise.
//! 2. The container policy is EXPLICIT: the MP4 family and the raw Annex-B
//!    streams are parsed; Matroska / WebM, AVI and MPEG-TS FALL BACK to
//!    ffmpeg's `trace_headers` (the parser never guesses).
//! 3. The corpus sweep: how many of the real files fall back, and which.

use std::io::{Seek, SeekFrom, Write};
use std::path::{Path, PathBuf};

use k6wp_core::sps::{detect_container, probe_dpb, ContainerKind};

fn corpus_dir() -> PathBuf {
    Path::new(env!("CARGO_MANIFEST_DIR")).join("../../../tests/corpus")
}

/// The `moov` box of an MP4 (its bytes), if the file has one.
fn moov_bytes(data: &[u8]) -> Option<Vec<u8>> {
    let mut pos = 0usize;
    while pos + 8 <= data.len() {
        let size =
            u32::from_be_bytes([data[pos], data[pos + 1], data[pos + 2], data[pos + 3]]) as usize;
        let kind = &data[pos + 4..pos + 8];
        if size < 8 || pos + size > data.len() {
            return None;
        }
        if kind == b"moov" {
            return Some(data[pos..pos + size].to_vec());
        }
        pos += size;
    }
    None
}

/// Writes an MP4 with a SPARSE 512 MB `mdat` and the `moov` at the END -
/// the non-faststart layout, with a body big enough that reading the file
/// whole would be visible.
fn write_non_faststart(source: &Path, target: &Path) -> std::io::Result<u64> {
    let data = std::fs::read(source)?;
    let moov = moov_bytes(&data).expect("the source must have a moov");
    // The ftyp box (the head of the file).
    let ftyp_len = if data.len() >= 8 && &data[4..8] == b"ftyp" {
        let size = u32::from_be_bytes([data[0], data[1], data[2], data[3]]) as usize;
        size.min(data.len())
    } else {
        0
    };

    let mut file = std::fs::File::create(target)?;
    file.write_all(&data[..ftyp_len])?;

    // The mdat: a 64-bit size header, then a sparse hole.
    let mdat_body = 512u64 * 1024 * 1024;
    let mdat_size = 16u64 + mdat_body;
    file.write_all(&[0, 0, 0, 1])?;
    file.write_all(b"mdat")?;
    file.write_all(&mdat_size.to_be_bytes())?;
    // The hole: the file length grows, the bytes stay unwritten.
    file.seek(SeekFrom::Start(ftyp_len as u64 + mdat_size))?;

    // The moov at the END.
    file.write_all(&moov)?;
    file.flush()?;
    Ok(file.metadata()?.len())
}

#[test]
fn a_non_faststart_file_is_found_by_seeking_and_costs_kilobytes() {
    let source = corpus_dir().join("slideshow.mp4");
    if !source.exists() {
        eprintln!("[SKIP] the corpus is missing (tests/corpus)");
        return;
    }
    let target = std::env::temp_dir().join(format!("k6wp-nonfaststart-{}.mp4", std::process::id()));
    let length = write_non_faststart(&source, &target).expect("the fixture must be written");
    assert!(
        length > 512 * 1024 * 1024,
        "the fixture must be bigger than 512 MB (it is {length})"
    );

    let report = probe_dpb(&target).expect("the probe must run");
    assert_eq!(report.container, "mp4", "the container must be detected");
    assert!(
        !report.needs_fallback,
        "the parameter sets must be found: {report:?}"
    );
    assert!(
        !report.sps.is_empty(),
        "the moov at the END must be found by seeking"
    );
    // The point of the whole exercise: the file is 512 MB, the read is
    // kilobytes.
    assert!(
        report.bytes_read < 1024 * 1024,
        "a 512 MB file must not be read whole (it cost {} bytes)",
        report.bytes_read
    );
    eprintln!(
        "the 512 MB non-faststart file cost {} bytes of reads",
        report.bytes_read
    );
    let _ = std::fs::remove_file(&target);
}

#[test]
fn the_container_policy_is_explicit() {
    // Matroska / WebM: the EBML magic.
    let mut mkv = vec![0x1A, 0x45, 0xDF, 0xA3, 0x01, 0x02, 0x03, 0x04];
    mkv.extend_from_slice(&[0u8; 200]);
    let kind = detect_container(&mkv);
    assert_eq!(kind, ContainerKind::Matroska);
    assert!(
        !kind.is_parsed(),
        "Matroska is not parsed: the fallback is used"
    );

    // AVI: "RIFF"...."AVI ".
    let mut avi = b"RIFF".to_vec();
    avi.extend_from_slice(&[0u8; 4]);
    avi.extend_from_slice(b"AVI ");
    avi.extend_from_slice(&[0u8; 200]);
    assert_eq!(detect_container(&avi), ContainerKind::Avi);

    // MPEG-TS: the sync byte at 0 and at 188.
    let mut ts = vec![0u8; 400];
    ts[0] = 0x47;
    ts[188] = 0x47;
    assert_eq!(detect_container(&ts), ContainerKind::MpegTs);

    // The MP4 family: "ftyp" at the offset 4.
    let mut mp4 = vec![0u8; 64];
    mp4[4..8].copy_from_slice(b"ftyp");
    assert_eq!(detect_container(&mp4), ContainerKind::Mp4);
    assert!(ContainerKind::Mp4.is_parsed());

    // A raw Annex-B stream: the NAL start code.
    assert_eq!(
        detect_container(&[0, 0, 0, 1, 0x67]),
        ContainerKind::RawStream
    );
    assert_eq!(detect_container(&[0, 0, 1, 0x67]), ContainerKind::RawStream);

    // Something else: unknown -> the fallback.
    assert_eq!(detect_container(&[0xAA; 64]), ContainerKind::Unknown);
}

#[test]
fn a_matroska_file_falls_back_and_never_guesses() {
    let path = corpus_dir().join("matroska.mkv");
    if !path.exists() {
        eprintln!("[SKIP] the Matroska sample is missing (tests/corpus)");
        return;
    }
    let report = probe_dpb(&path).expect("the probe must run");
    assert_eq!(report.container, "matroska");
    assert!(
        report.needs_fallback,
        "a Matroska file must fall back to trace_headers"
    );
    assert!(
        report.sps.is_empty(),
        "the parser must never invent a parameter set"
    );
}

#[test]
fn the_corpus_sweep_reports_the_fallback_percentage() {
    let dir = corpus_dir();
    if !dir.exists() {
        eprintln!("[SKIP] the corpus is missing (tests/corpus)");
        return;
    }
    let mut total = 0usize;
    let mut fallback = 0usize;
    let mut lines = Vec::new();
    let mut entries: Vec<_> = std::fs::read_dir(&dir)
        .expect("the corpus must be readable")
        .flatten()
        .map(|e| e.path())
        .collect();
    entries.sort();
    for path in &entries {
        let extension = path
            .extension()
            .map(|e| e.to_string_lossy().to_lowercase())
            .unwrap_or_default();
        if !matches!(
            extension.as_str(),
            "mp4" | "mkv" | "h264" | "hevc" | "webm" | "ts" | "avi"
        ) {
            continue;
        }
        total += 1;
        let report = match probe_dpb(path) {
            Ok(report) => report,
            Err(e) => {
                lines.push(format!("  {}: the probe failed ({e})", path.display()));
                fallback += 1;
                continue;
            }
        };
        if report.needs_fallback {
            fallback += 1;
        }
        lines.push(format!(
            "  {}: the container={} the fallback={} the bytes_read={} the SPS={}",
            path.file_name().unwrap().to_string_lossy(),
            report.container,
            report.needs_fallback,
            report.bytes_read,
            report.sps.len()
        ));
    }
    for line in &lines {
        eprintln!("{line}");
    }
    let percentage = if total == 0 {
        0.0
    } else {
        (fallback as f64) * 100.0 / (total as f64)
    };
    eprintln!("the corpus: {fallback} of {total} files fall back ({percentage:.1} %)");
    assert!(total > 0, "the corpus must hold video files");
    // The policy: only the containers this module does not parse fall back.
    for path in entries {
        let name = path.file_name().unwrap().to_string_lossy().to_lowercase();
        if name.ends_with(".mp4") || name.ends_with(".h264") || name.ends_with(".hevc") {
            let report = probe_dpb(&path).expect("the probe must run");
            assert!(
                !report.needs_fallback,
                "{} must be parsed by the module itself",
                path.display()
            );
        }
    }
}
