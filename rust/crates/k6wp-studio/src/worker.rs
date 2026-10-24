//! The Studio worker: every touch of the file system, the registry, the
//! network, or the IPC pipe happens here, never on the Slint event loop.
//!
//! `tools/lint_ui_thread.ps1` enforces the split: a line in this file that
//! performs I/O is marked `// worker`, and the same line in `main.rs` would be
//! a lint failure. The worker is one thread with a bounded-size request queue;
//! the only place it spawns a helper thread is the existence probe, because a
//! dead UNC path can block for a minute and a half and must not stall the
//! decodes behind it.

use std::path::{Path, PathBuf};
use std::sync::mpsc::{self, Receiver, RecvTimeoutError, Sender};
use std::time::{Duration, Instant};

use k6wp_core::{ipc::IpcMessage, LibraryIndex};
use k6wp_ipc::{current_session_pipe_name, ConnectOptions, IpcClient, IpcError};
use serde_json::json;

use crate::thumbs::{decode_to_cover, Pixels};

/// How long an unreachable path may take before the card says
/// "Tidak tersedia". The Qt Studio waited on the UI thread; this is the
/// replacement for that freeze.
pub const PROBE_TIMEOUT: Duration = Duration::from_secs(2);

/// Backoff schedule for IPC reconnection: 1→2→4→8→30 seconds, then repeat.
const RECONNECT_BACKOFF: &[Duration] = &[
    Duration::from_secs(1),
    Duration::from_secs(2),
    Duration::from_secs(4),
    Duration::from_secs(8),
    Duration::from_secs(30),
];

/// Polling interval for get_state when the Studio is visible.
const POLL_INTERVAL_VISIBLE: Duration = Duration::from_secs(2);

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
    /// Set the wallpaper to the given library entry (by index and media path).
    SetWallpaper { index: usize, media: PathBuf },
    /// Pause the current wallpaper.
    Pause,
    /// Resume the current wallpaper.
    Resume,
    /// Stop the worker (sent on shutdown; the loop also ends on hang-up).
    Stop,
}

/// The current IPC connection status.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum IpcStatus {
    Disconnected,
    Connecting,
    Connected,
    Reconnecting,
}

/// The engine state from get_state polling.
#[derive(Debug, Clone, Default)]
pub struct EngineState {
    pub video_name: Option<String>,
    pub is_paused: bool,
    pub is_connected: bool,
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
    /// IPC connection status changed.
    IpcStatus(IpcStatus),
    /// Engine state from get_state polling.
    EngineState(EngineState),
    /// An IPC error occurred (shown as toast).
    IpcError(String),
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

    // IPC connection manager state
    let mut ipc_client: Option<IpcClient> = None;
    let mut ipc_status = IpcStatus::Reconnecting; // Start with Reconnecting so main loop attempts connection
    let mut backoff_index = 0;
    let mut next_connect_attempt = Instant::now(); // Allow immediate first attempt
    let mut last_poll = Instant::now();
    let mut engine_state = EngineState::default();
    let mut studio_visible = true; // updated by UI via a new Request variant if needed

    loop {
        // Adaptive polling: check if it's time to poll get_state
        let should_poll =
            ipc_client.is_some() && studio_visible && last_poll.elapsed() >= POLL_INTERVAL_VISIBLE;

        // Use recv_timeout to allow periodic polling without blocking indefinitely
        let timeout = if should_poll {
            Duration::from_millis(100)
        } else {
            Duration::from_millis(500)
        };

        match rx.recv_timeout(timeout) {
            Ok(Request::Stop) | Err(RecvTimeoutError::Disconnected) => break,
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
            Ok(Request::SetWallpaper { index, media }) => {
                if let Some(client) = &mut ipc_client {
                    handle_set_wallpaper(client, index, &media, &sink);
                } else {
                    sink.emit(Event::IpcError(
                        "Tidak terhubung ke engine. Menghubungkan ulang...".to_string(),
                    ));
                    let _ = attempt_connect(
                        &sink,
                        &mut ipc_client,
                        &mut ipc_status,
                        &mut backoff_index,
                    );
                }
            }
            Ok(Request::Pause) => {
                if let Some(client) = &mut ipc_client {
                    handle_pause(client, &sink);
                }
            }
            Ok(Request::Resume) => {
                if let Some(client) = &mut ipc_client {
                    handle_resume(client, &sink);
                }
            }
            Err(RecvTimeoutError::Timeout) => {
                // Timeout - check if we should poll get_state
                if should_poll {
                    last_poll = Instant::now();
                    if let Some(client) = &mut ipc_client {
                        poll_get_state(client, &sink, &mut engine_state);
                    }
                }
            }
        }

        // If disconnected, attempt reconnection with backoff (non-blocking)
        if ipc_client.is_none() && ipc_status != IpcStatus::Disconnected {
            if Instant::now() >= next_connect_attempt {
                let _ =
                    attempt_connect(&sink, &mut ipc_client, &mut ipc_status, &mut backoff_index);
                // Schedule next attempt with backoff
                let backoff = RECONNECT_BACKOFF[backoff_index.min(RECONNECT_BACKOFF.len() - 1)];
                next_connect_attempt = Instant::now() + backoff;
            }
        }
    }
}

fn attempt_connect(
    sink: &Sink,
    client: &mut Option<IpcClient>,
    status: &mut IpcStatus,
    backoff_index: &mut usize,
) -> Result<(), IpcError> {
    *status = IpcStatus::Connecting;
    sink.emit(Event::IpcStatus(IpcStatus::Connecting));

    let pipe_name = current_session_pipe_name();
    // Use a short timeout for connection attempts so the main loop can continue
    // processing other requests (probes, thumbnails) while we're trying to connect.
    let options = ConnectOptions {
        timeout: Duration::from_millis(200),
        expected_server_image: None, // Allow any accepted image
        require_current_user: true,
        require_same_session: true,
    };

    match IpcClient::connect(&pipe_name, options) {
        Ok(c) => {
            *client = Some(c);
            *status = IpcStatus::Connected;
            *backoff_index = 0;
            sink.emit(Event::IpcStatus(IpcStatus::Connected));
            Ok(())
        }
        Err(e) => {
            *status = IpcStatus::Reconnecting;
            *backoff_index = (*backoff_index + 1).min(RECONNECT_BACKOFF.len());
            sink.emit(Event::IpcStatus(IpcStatus::Reconnecting));
            Err(e)
        }
    }
}

fn handle_set_wallpaper(client: &mut IpcClient, index: usize, media: &PathBuf, sink: &Sink) {
    // First, probe the media file exists (with timeout).
    if !probe(media, PROBE_TIMEOUT) {
        sink.emit(Event::IpcError(format!(
            "File tidak ditemukan: {}",
            media.display()
        )));
        return;
    }
    let msg = IpcMessage {
        version: k6wp_core::ipc::PROTOCOL_VERSION,
        cmd: k6wp_core::ipc::Cmd::SetVideo,
        payload: serde_json::json!({ "path": media.to_string_lossy() }),
    };
    match client.request(&msg) {
        Ok(answer) => {
            if answer.get("error").is_some() {
                sink.emit(Event::IpcError(format!(
                    "Gagal memasang wallpaper: {}",
                    answer["error"].as_str().unwrap_or("unknown error")
                )));
            } else {
                sink.emit(Event::IpcError("Terpasang".to_string()));
            }
        }
        Err(e) => {
            sink.emit(Event::IpcError(format!("Kesalahan IPC: {e}")));
        }
    }
}

fn handle_pause(client: &mut IpcClient, sink: &Sink) {
    let msg = IpcMessage {
        version: k6wp_core::ipc::PROTOCOL_VERSION,
        cmd: k6wp_core::ipc::Cmd::Pause,
        payload: serde_json::json!({}),
    };
    match client.request(&msg) {
        Ok(answer) => {
            if answer.get("error").is_some() {
                sink.emit(Event::IpcError(format!(
                    "Gagal menjeda: {}",
                    answer["error"].as_str().unwrap_or("unknown error")
                )));
            }
        }
        Err(e) => {
            sink.emit(Event::IpcError(format!("Kesalahan IPC: {e}")));
        }
    }
}

fn handle_resume(client: &mut IpcClient, sink: &Sink) {
    let msg = IpcMessage {
        version: k6wp_core::ipc::PROTOCOL_VERSION,
        cmd: k6wp_core::ipc::Cmd::Resume,
        payload: serde_json::json!({}),
    };
    match client.request(&msg) {
        Ok(answer) => {
            if answer.get("error").is_some() {
                sink.emit(Event::IpcError(format!(
                    "Gagal melanjutkan: {}",
                    answer["error"].as_str().unwrap_or("unknown error")
                )));
            }
        }
        Err(e) => {
            sink.emit(Event::IpcError(format!("Kesalahan IPC: {e}")));
        }
    }
}

fn poll_get_state(client: &mut IpcClient, sink: &Sink, state: &mut EngineState) {
    let msg = IpcMessage {
        version: k6wp_core::ipc::PROTOCOL_VERSION,
        cmd: k6wp_core::ipc::Cmd::GetState,
        payload: serde_json::json!({}),
    };
    match client.request(&msg) {
        Ok(answer) => {
            // Parse the engine state from the answer
            if let Some(video_name) = answer.get("video_name").and_then(|v| v.as_str()) {
                state.video_name = Some(video_name.to_string());
            }
            if let Some(paused) = answer.get("paused").and_then(|v| v.as_bool()) {
                state.is_paused = paused;
            }
            state.is_connected = true;
            sink.emit(Event::EngineState(state.clone()));
        }
        Err(e) => {
            state.is_connected = false;
            sink.emit(Event::IpcError(format!("Gagal memuat status: {e}")));
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
        // Skip IPC status events that are emitted during startup
        let first = loop {
            let event = events_rx
                .recv_timeout(Duration::from_millis(500))
                .expect("the decode must answer while the probe is still in flight");
            if matches!(event, Event::Thumb { .. }) {
                break event;
            }
        };
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
}
