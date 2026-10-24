//! The Studio worker: every touch of the file system, the registry or a
//! remote path happens here, never on the Slint event loop.
//!
//! `tools/lint_ui_thread.ps1` enforces the split: a line in this file that
//! performs I/O is marked `// worker`, and the same line in `main.rs` would be
//! a lint failure. The worker is one thread with a bounded-size request queue;
//! the only place it spawns a helper thread is the existence probe, because a
//! dead UNC path can block for a minute and a half and must not stall the
//! decodes behind it.

use std::path::{Path, PathBuf};
use std::sync::mpsc::{self, Receiver, RecvTimeoutError, Sender};
use std::time::Duration;

use k6wp_core::LibraryIndex;

use crate::thumbs::{decode_to_cover, Pixels};

/// How long an unreachable path may take before the card says
/// "Tidak tersedia". The Qt Studio waited on the UI thread; this is the
/// replacement for that freeze.
pub const PROBE_TIMEOUT: Duration = Duration::from_secs(2);

/// One library entry, reduced to what the grid draws.
#[derive(Clone, Debug)]
pub struct Entry {
    pub name: String,
    pub meta: String,
    pub thumb: PathBuf,
    pub media: PathBuf,
    /// The index already knows the copy is gone (written by the C++ side).
    pub broken: bool,
}

/// A request from the UI thread to the worker.
#[derive(Debug)]
pub enum Request {
    /// Read and parse `library.json`.
    Load { path: PathBuf },
    /// Does the media file still exist?
    Probe { index: usize, path: PathBuf },
    /// Decode one thumbnail at the size the grid will draw it.
    Thumb {
        index: usize,
        generation: u64,
        path: PathBuf,
        width: u32,
        height: u32,
    },
    /// The light/dark and reduced-motion preferences of the system.
    Theme,
    /// Stop the worker (sent on shutdown; the loop also ends on hang-up).
    Stop,
}

/// A result from the worker.
#[derive(Debug)]
pub enum Event {
    Library(Result<Vec<Entry>, String>),
    Probe {
        index: usize,
        exists: bool,
    },
    Thumb {
        index: usize,
        generation: u64,
        result: Result<Pixels, String>,
    },
    Theme {
        dark: bool,
        reduced_motion: bool,
    },
}

/// The number of threads that answer "does this file exist". A dead UNC path
/// can hold a stat call for the SMB timeout, so the probes never run on the
/// decode thread: four dead servers must not delay a thumbnail by eight
/// seconds. Two threads keep two probes in flight and stay small.
const PROBE_THREADS: usize = 2;

/// The event callback, shareable across the worker and the probe threads.
///
/// The caller hands in a closure that posts to the Slint event loop, which is
/// not `Sync`; a short-lived mutex around the call is enough to share it, and
/// the call itself never blocks.
#[derive(Clone)]
struct Sink(std::sync::Arc<std::sync::Mutex<Box<dyn Fn(Event) + Send>>>);

impl Sink {
    fn emit(&self, event: Event) {
        if let Ok(callback) = self.0.lock() {
            callback(event);
        }
    }
}

/// Starts the worker thread. The callback runs on the worker thread; the
/// caller is responsible for hopping back to the UI loop.
pub fn spawn(on_event: impl Fn(Event) + Send + 'static) -> Sender<Request> {
    let (tx, rx) = mpsc::channel::<Request>();
    let sink = Sink(std::sync::Arc::new(std::sync::Mutex::new(
        Box::new(on_event) as Box<dyn Fn(Event) + Send>,
    )));
    let spawned = std::thread::Builder::new()
        .name("k6wp-studio-worker".to_string())
        .spawn(move || run(rx, sink));
    if spawned.is_err() {
        eprintln!("studio: the worker thread could not start; the library will stay empty");
    }
    tx
}

fn run(rx: Receiver<Request>, sink: Sink) {
    // One queue per probe thread: a shared `Receiver` behind a mutex would
    // serialise the probes (one thread would hold the lock while it blocks).
    let probe_queues: Vec<Sender<(usize, PathBuf)>> = (0..PROBE_THREADS)
        .map(|n| {
            let (tx, rx) = mpsc::channel::<(usize, PathBuf)>();
            let sink = sink.clone();
            let spawned = std::thread::Builder::new()
                .name(format!("k6wp-studio-probe-{n}"))
                .spawn(move || probe_loop(rx, sink));
            if spawned.is_err() {
                eprintln!(
                    "studio: a probe thread could not start; remote media will show as missing"
                );
            }
            tx
        })
        .collect();
    let mut next_probe = 0usize;

    loop {
        match rx.recv() {
            Ok(Request::Stop) | Err(_) => break,
            Ok(Request::Load { path }) => sink.emit(Event::Library(load_library(&path))),
            Ok(Request::Probe { index, path }) => {
                if probe_queues.is_empty() {
                    sink.emit(Event::Probe {
                        index,
                        exists: probe(&path, PROBE_TIMEOUT),
                    });
                } else {
                    let queue = &probe_queues[next_probe % probe_queues.len()];
                    next_probe = next_probe.wrapping_add(1);
                    let _ = queue.send((index, path));
                }
            }
            Ok(Request::Thumb {
                index,
                generation,
                path,
                width,
                height,
            }) => {
                let result = read_and_decode(&path, width, height);
                sink.emit(Event::Thumb {
                    index,
                    generation,
                    result,
                });
            }
            Ok(Request::Theme) => {
                let (dark, reduced_motion) = system_appearance();
                sink.emit(Event::Theme {
                    dark,
                    reduced_motion,
                });
            }
        }
    }
}

fn probe_loop(rx: Receiver<(usize, PathBuf)>, sink: Sink) {
    while let Ok((index, path)) = rx.recv() {
        let exists = probe(&path, PROBE_TIMEOUT);
        sink.emit(Event::Probe { index, exists });
    }
}

/// Reads the index and turns it into grid entries. Any failure is reported as
/// a message the empty state can show verbatim.
pub fn load_library(path: &Path) -> Result<Vec<Entry>, String> {
    let text = std::fs::read_to_string(path) // worker
        .map_err(|e| format!("{}: {e}", path.display()))?;
    let index = LibraryIndex::parse(&text).map_err(|e| e.to_string())?;
    Ok(index.entries.iter().map(entry_view).collect())
}

fn entry_view(entry: &k6wp_core::LibraryEntry) -> Entry {
    let name = entry
        .dst
        .file_name()
        .map(|n| n.to_string_lossy().to_string())
        .unwrap_or_else(|| entry.dst.to_string_lossy().to_string());
    let mut meta = String::new();
    if entry.duration > 0.0 {
        meta.push_str(&format!("{:.1} s", entry.duration));
    }
    if entry.width > 0 && entry.height > 0 {
        if !meta.is_empty() {
            meta.push_str(" - ");
        }
        meta.push_str(&format!("{}x{}", entry.width, entry.height));
    }
    if !entry.codec.is_empty() {
        if !meta.is_empty() {
            meta.push_str(" - ");
        }
        meta.push_str(&entry.codec);
    }
    Entry {
        name,
        meta,
        thumb: entry.thumb.clone(),
        media: entry.dst.clone(),
        broken: entry.broken,
    }
}

/// Does the media file exist, without ever blocking longer than `timeout`?
///
/// A drive-letter path is a stat call and is safe inline. Anything else (a
/// UNC path, a mapped network drive) gets its own thread: a dead server can
/// hold `exists()` open for the SMB timeout, and the probe must not stop the
/// thumbnails queued behind it.
pub fn probe(path: &Path, timeout: Duration) -> bool {
    let local = path
        .components()
        .next()
        .map(|c| matches!(c, std::path::Component::Prefix(_)))
        .unwrap_or(false)
        && path.to_string_lossy().as_bytes().get(1) == Some(&b':');
    if local {
        return path.exists(); // worker
    }
    let owned = path.to_path_buf();
    let (tx, rx) = mpsc::channel();
    let _ = std::thread::Builder::new()
        .name("k6wp-studio-probe".to_string())
        .spawn(move || {
            let _ = tx.send(owned.exists()); // worker
        });
    match rx.recv_timeout(timeout) {
        Ok(exists) => exists,
        Err(RecvTimeoutError::Timeout) => false,
        Err(RecvTimeoutError::Disconnected) => false,
    }
}

fn read_and_decode(path: &Path, width: u32, height: u32) -> Result<Pixels, String> {
    let bytes = std::fs::read(path) // worker
        .map_err(|e| format!("{}: {e}", path.display()))?;
    decode_to_cover(&bytes, width, height)
}

/// The system light/dark preference and the "show animations" preference.
fn system_appearance() -> (bool, bool) {
    (system_is_dark(), !system_animations_enabled())
}

#[cfg(windows)]
fn registry_dword(subkey: &str, value: &str) -> Option<u32> {
    use windows::Win32::Foundation::WIN32_ERROR;
    use windows::Win32::System::Registry::{RegGetValueW, HKEY_CURRENT_USER, RRF_RT_REG_DWORD};

    let subkey: Vec<u16> = subkey.encode_utf16().chain(std::iter::once(0)).collect();
    let value: Vec<u16> = value.encode_utf16().chain(std::iter::once(0)).collect();
    let mut data: u32 = 0;
    let mut size = std::mem::size_of::<u32>() as u32;
    let status: WIN32_ERROR = unsafe {
        RegGetValueW(
            HKEY_CURRENT_USER,
            windows::core::PCWSTR::from_raw(subkey.as_ptr()),
            windows::core::PCWSTR::from_raw(value.as_ptr()),
            RRF_RT_REG_DWORD,
            None,
            Some(&mut data as *mut u32 as *mut core::ffi::c_void),
            Some(&mut size),
        )
    };
    if status == WIN32_ERROR(0) {
        Some(data)
    } else {
        None
    }
}

#[cfg(windows)]
fn system_is_dark() -> bool {
    // AppsUseLightTheme: 0 = dark, 1 = light, absent = light.
    match registry_dword(
        r"Software\Microsoft\Windows\CurrentVersion\Themes\Personalize",
        "AppsUseLightTheme",
    ) {
        Some(v) => v == 0,
        None => false,
    }
}

#[cfg(windows)]
fn system_animations_enabled() -> bool {
    use windows::Win32::UI::WindowsAndMessaging::{
        SystemParametersInfoW, SPI_GETCLIENTAREAANIMATION, SYSTEM_PARAMETERS_INFO_UPDATE_FLAGS,
    };
    let mut enabled = windows::core::BOOL(1);
    let ok = unsafe {
        SystemParametersInfoW(
            SPI_GETCLIENTAREAANIMATION,
            0,
            Some(&mut enabled as *mut _ as *mut core::ffi::c_void),
            SYSTEM_PARAMETERS_INFO_UPDATE_FLAGS(0),
        )
    };
    ok.is_ok() && enabled.as_bool()
}

#[cfg(not(windows))]
fn system_is_dark() -> bool {
    false
}

#[cfg(not(windows))]
fn system_animations_enabled() -> bool {
    true
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn a_missing_index_is_an_error_not_a_panic() {
        let err = load_library(Path::new(r"Z:\definitely\not\here\library.json"));
        assert!(err.is_err());
    }

    #[test]
    fn a_drive_letter_path_is_probed_inline() {
        // The temp directory exists; the drive-letter fast path must find it.
        let dir = std::env::temp_dir();
        assert!(probe(&dir, PROBE_TIMEOUT));
    }

    #[test]
    fn an_unreachable_probe_gives_up_after_the_timeout() {
        let started = std::time::Instant::now();
        let exists = probe(
            Path::new(r"\\192.0.2.1\x\a.mp4"),
            Duration::from_millis(150),
        );
        assert!(!exists);
        assert!(
            started.elapsed() < Duration::from_secs(3),
            "the probe must return on the timeout, not on the SMB timeout"
        );
    }

    #[test]
    fn a_dead_unc_probe_does_not_block_the_decode_behind_it() {
        // The real worker, the real queues: a probe of a dead server is
        // dispatched to its own thread, so the thumbnail request queued
        // behind it is answered immediately. With an inline probe this test
        // fails after the full 2 s timeout.
        let (events_tx, events_rx) = mpsc::channel::<Event>();
        let requests = spawn(move |event| {
            let _ = events_tx.send(event);
        });
        requests
            .send(Request::Probe {
                index: 0,
                path: PathBuf::from(r"\\192.0.2.1\x\a.mp4"),
            })
            .expect("the worker must accept a probe");
        requests
            .send(Request::Thumb {
                index: 1,
                generation: 0,
                path: PathBuf::from(r"Z:\definitely\not\here.png"),
                width: 8,
                height: 8,
            })
            .expect("the worker must accept a decode");
        let first = events_rx
            .recv_timeout(Duration::from_millis(500))
            .expect("the decode must answer while the probe is still in flight");
        match first {
            Event::Thumb { index, result, .. } => {
                assert_eq!(index, 1);
                assert!(
                    result.is_err(),
                    "a missing thumbnail is an error, not a panic"
                );
            }
            other => panic!("the decode must answer first, got {other:?}"),
        }
        let _ = requests.send(Request::Stop);
    }

    #[test]
    fn entry_meta_is_built_from_the_codec_fields() {
        let entry = k6wp_core::LibraryEntry {
            dst: PathBuf::from(r"C:\x\clip.mp4"),
            duration: 12.5,
            width: 1920,
            height: 1080,
            codec: "h264".to_string(),
            ..Default::default()
        };
        let view = entry_view(&entry);
        assert_eq!(view.name, "clip.mp4");
        assert_eq!(view.meta, "12.5 s - 1920x1080 - h264");
    }
}
