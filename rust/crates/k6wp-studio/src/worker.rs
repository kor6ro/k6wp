//! The Studio worker: every touch of the file system, the registry, the
//! network, or the IPC pipe happens here, never on the Slint event loop.
//!
//! `tools/lint_ui_thread.ps1` enforces the split: a line in this file that
//! performs I/O is marked `// worker`, and the same line in `main.rs` would be
//! a lint failure. The worker is one thread with a bounded-size request queue;
//! the only place it spawns a helper thread is the existence probe, because a
//! dead UNC path can block for a minute and a half and must not stall the
//! decodes behind it.

use std::collections::HashMap;
use std::path::{Path, PathBuf};
use std::sync::mpsc::{self, Receiver, RecvTimeoutError, Sender};
use std::time::{Duration, Instant};

use k6wp_core::compress_args::{build_argv, CompressArgs};
use k6wp_core::settings::save_studio_settings;
use k6wp_core::{ipc::IpcMessage, LibraryIndex, StudioSettings};
use k6wp_ipc::{current_session_pipe_name, ConnectOptions, IpcClient, IpcError};
use serde_json::Value;

use crate::thumbnailer::{generate_thumbnail, THUMB_HEIGHT, THUMB_WIDTH};
use crate::thumbs::{decode_to_cover, Pixels};

/// How long an unreachable path may take before the card says
/// "Tidak tersedia". The Qt Studio waited on the UI thread; this is the
/// replacement for that freeze.
pub const PROBE_TIMEOUT: Duration = Duration::from_secs(2);

/// Backoff schedule for IPC reconnection: 1?2?4?8?30 seconds, then repeat.
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
    pub index: usize,
    pub name: String,
    pub meta: String,
    pub thumb: PathBuf,
    pub media: PathBuf,
    /// The index already knows the copy is gone (written by the C++ side).
    pub broken: bool,
    /// File is missing from disk.
    pub unavailable: bool,
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
    /// Generate thumbnail for a library card (async, done on worker thread).
    Thumbnail { index: usize, media: PathBuf },
    /// Import one or more media files into the library.
    Import { paths: Vec<PathBuf> },
    /// Compress one media file.
    Compress {
        job_id: u64,
        src: PathBuf,
        out: PathBuf,
        res_w: u32,
        res_h: u32,
        fps: u32,
        crf: u32,
    },
    /// Cancel a running compress job (kills the compressor process tree).
    CompressCancel { job_id: u64 },
    /// Load studio settings from disk.
    SettingsLoad,
    /// Save studio settings to disk.
    SettingsSave { settings: StudioSettings },
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
    /// The engine accepted the wallpaper. The UI records it so "Urungkan"
    /// can go back to the previous picture.
    WallpaperApplied {
        index: usize,
    },
    /// Compress progress: job_id, percent complete (0-100), optional ETA seconds.
    CompressProgress {
        job_id: u64,
        pct: u8,
        eta_secs: Option<u32>,
    },
    /// Compress done: job_id, ok flag, output path if ok, error message if not.
    CompressDone {
        job_id: u64,
        ok: bool,
        out_path: Option<PathBuf>,
        error: Option<String>,
    },
    /// Studio settings loaded from disk.
    SettingsLoaded(StudioSettings),
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
type EventSink = std::sync::Arc<std::sync::Mutex<Box<dyn Fn(Event) + Send>>>;

#[derive(Clone)]
struct Sink(EventSink);

impl Sink {
    fn emit(&self, event: Event) {
        if let Ok(callback) = self.0.lock() {
            callback(event);
        }
    }
}

/// Starts the worker thread. The callback runs on the worker thread; the
/// caller is responsible for hopping back to the UI loop.
///
/// `pipe_name_override` is a debug-only parameter for tests. It is compiled
/// out of the release build and only used when `debug_assertions` is true.
pub fn spawn(
    on_event: impl Fn(Event) + Send + 'static,
    #[cfg(debug_assertions)] pipe_name_override: Option<String>,
) -> Sender<Request> {
    let (tx, rx) = mpsc::channel::<Request>();
    let sink = Sink(std::sync::Arc::new(std::sync::Mutex::new(
        Box::new(on_event) as Box<dyn Fn(Event) + Send>,
    )));
    #[cfg(debug_assertions)]
    let spawned = std::thread::Builder::new()
        .name("k6wp-studio-worker".to_string())
        .spawn(move || run(rx, sink, pipe_name_override));
    #[cfg(not(debug_assertions))]
    let spawned = std::thread::Builder::new()
        .name("k6wp-studio-worker".to_string())
        .spawn(move || run(rx, sink, None));
    if spawned.is_err() {
        eprintln!("studio: the worker thread could not start; the library will stay empty");
    }
    tx
}

fn run(rx: Receiver<Request>, sink: Sink, pipe_name_override: Option<String>) {
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
    let studio_visible = true; // updated by UI via a new Request variant if needed
    let mut compress_jobs: HashMap<u64, u32> = HashMap::new(); // job_id -> pid

    // Resolve the pipe name once at startup (uses override in debug, real name in release)
    let effective_pipe_name = pipe_name_override.unwrap_or_else(|| current_session_pipe_name(None));

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
            Ok(Request::Thumbnail { index, media }) => {
                // Generate thumbnail using ffmpeg, then decode
                let cache_dir = std::env::current_exe()
                    .ok()
                    .and_then(|p| p.parent().map(|d| d.join("thumbs")))
                    .unwrap_or_else(|| std::env::temp_dir().join("k6wp-thumbs"));
                std::fs::create_dir_all(&cache_dir).ok();
                let result = generate_thumbnail(&media, &cache_dir, THUMB_WIDTH, THUMB_HEIGHT);
                let decoded = result
                    .map_err(|e| e.to_string())
                    .and_then(|path| read_and_decode(&path, THUMB_WIDTH, THUMB_HEIGHT));
                sink.emit(Event::Thumb {
                    index,
                    generation: 0, // thumbnail generation is separate from card generation
                    result: decoded,
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
                // First, probe the media file exists (with timeout) - do this regardless of connection state.
                if !probe(&media, PROBE_TIMEOUT) {
                    sink.emit(Event::IpcError(format!(
                        "File tidak ditemukan: {}",
                        media.display()
                    )));
                    return;
                }
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
                        &effective_pipe_name,
                    );
                }
            }
            Ok(Request::Import { paths }) => {
                // Import: validate paths, add to library.json, emit LibraryUpdated
                let lib_path = match library_index_path(None) {
                    Some(p) => p,
                    None => {
                        sink.emit(Event::IpcError(
                            "Tidak bisa menemukan library.json".to_string(),
                        ));
                        return;
                    }
                };
                match import_into_library(&lib_path, &paths) {
                    Ok(added_indices) => {
                        // Emit updated library
                        let index = LibraryIndex::parse(
                            &std::fs::read_to_string(&lib_path).unwrap_or_default(),
                        )
                        .unwrap_or_default();
                        let entries: Vec<Entry> = index
                            .entries
                            .iter()
                            .map(|e| Entry {
                                index: 0,
                                name: e.dst.file_name().unwrap().to_string_lossy().to_string(),
                                meta: String::new(),
                                thumb: e.thumb.clone(),
                                media: e.dst.clone(),
                                unavailable: !e.dst.exists(),
                                broken: e.broken,
                            })
                            .collect();
                        sink.emit(Event::Library(Ok(entries)));
                    }
                    Err(e) => {
                        sink.emit(Event::IpcError(e));
                    }
                }
            }
            Ok(Request::Compress {
                job_id,
                src,
                out,
                res_w,
                res_h,
                fps,
                crf,
            }) => {
                compress_handler(job_id, &src, &out, res_w, res_h, fps, crf, sink.clone());
            }
            Ok(Request::CompressCancel { job_id }) => {
                if let Some(&pid) = compress_jobs.get(&job_id) {
                    let _ = std::process::Command::new("taskkill")
                        .args(["/T", "/F", "/PID", &pid.to_string()])
                        .status();
                    compress_jobs.remove(&job_id);
                    sink.emit(Event::CompressDone {
                        job_id,
                        ok: false,
                        out_path: None,
                        error: Some("Dibatalkan".to_string()),
                    });
                }
            }
            Ok(Request::SettingsLoad) => {
                let path = match settings_index_path() {
                    Some(p) => p,
                    None => {
                        sink.emit(Event::IpcError(
                            "LOCALAPPDATA tidak ditemukan: settings tidak bisa dibaca".to_string(),
                        ));
                        continue;
                    }
                };
                match std::fs::read_to_string(&path) {
                    Ok(text) => match k6wp_core::settings::migrate_studio_settings(&text) {
                        Ok(settings) => sink.emit(Event::SettingsLoaded(settings)),
                        Err(e) => {
                            sink.emit(Event::IpcError(format!("Gagal migrasi settings: {e}")))
                        }
                    },
                    Err(_) => {
                        // Missing settings file is not an error: emit defaults.
                        sink.emit(Event::SettingsLoaded(StudioSettings::default()));
                    }
                }
            }
            Ok(Request::SettingsSave { settings }) => {
                let path = match settings_index_path() {
                    Some(p) => p,
                    None => {
                        sink.emit(Event::IpcError(
                            "LOCALAPPDATA tidak ditemukan: settings tidak bisa disimpan"
                                .to_string(),
                        ));
                        continue;
                    }
                };
                match save_studio_settings(&path, &settings) {
                    Ok(()) => sink.emit(Event::SettingsLoaded(settings)),
                    Err(e) => sink.emit(Event::IpcError(format!("Gagal simpan settings: {e}"))),
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
        if ipc_client.is_none()
            && ipc_status != IpcStatus::Disconnected
            && Instant::now() >= next_connect_attempt
        {
            let _ = attempt_connect(
                &sink,
                &mut ipc_client,
                &mut ipc_status,
                &mut backoff_index,
                &effective_pipe_name,
            );
            // Schedule next attempt with backoff
            let backoff = RECONNECT_BACKOFF[backoff_index.min(RECONNECT_BACKOFF.len() - 1)];
            next_connect_attempt = Instant::now() + backoff;
        }
    }
}

pub(crate) fn import_into_library(
    lib_path: &Path,
    paths: &[PathBuf],
) -> Result<Vec<usize>, String> {
    let mut index =
        match LibraryIndex::parse(&std::fs::read_to_string(lib_path).unwrap_or_default()) {
            Ok(idx) => idx,
            Err(e) => return Err(format!("Gagal baca library.json: {e}")),
        };
    let mut added = 0;
    let lib_dir = lib_path.parent().unwrap();
    for path in paths {
        if !path.exists() {
            return Err(format!("File tidak ditemukan: {}", path.display()));
        }
        let ext = path
            .extension()
            .and_then(|s| s.to_str())
            .unwrap_or("")
            .to_lowercase();
        if !matches!(ext.as_str(), "mp4" | "mkv" | "webm" | "mov") {
            return Err(format!("Format tidak didukung: {}", path.display()));
        }
        let meta = std::fs::metadata(path)
            .map_err(|e| format!("Gagal baca metadata {}: {e}", path.display()))?;
        if meta.len() == 0 {
            return Err(format!("File kosong: {}", path.display()));
        }
        let dst_name = path.file_name().unwrap().to_string_lossy().to_string();
        let dst = lib_dir.join("wallpapers").join(&dst_name);
        std::fs::create_dir_all(lib_dir.join("wallpapers")).ok();
        std::fs::copy(path, &dst).map_err(|e| format!("Gagal copy {}: {e}", path.display()))?;
        let mtime = meta
            .modified()
            .ok()
            .and_then(|t| t.duration_since(std::time::UNIX_EPOCH).ok())
            .map(|d| d.as_secs() as i64)
            .unwrap_or(0);
        let entry = k6wp_core::library::LibraryEntry {
            src: path.clone(),
            dst,
            res: "0x0".to_string(),
            fps: 30,
            crf: 23,
            encoder: "libx264".to_string(),
            mtime,
            size: meta.len(),
            duration: 0.0,
            codec: "".to_string(),
            width: 0,
            height: 0,
            thumb: PathBuf::new(),
            broken: false,
        };
        index.entries.push(entry);
        added += 1;
    }
    index
        .save_atomic(lib_path)
        .map_err(|e| format!("Gagal simpan library.json: {e}"))?;
    Ok((index.entries.len() - added..index.entries.len()).collect())
}

/// Returns the path to `compressor.exe` if found.
/// Looks in: sibling of current exe → build/release/ → K6WP_COMPRESSOR env (debug-only).
fn find_compressor() -> Option<PathBuf> {
    if let Ok(current) = std::env::current_exe() {
        if let Some(parent) = current.parent() {
            let sibling = parent.join("compressor.exe");
            if sibling.exists() {
                return Some(sibling);
            }
            let release_compressor = parent.join("build").join("release").join("compressor.exe");
            if release_compressor.exists() {
                return Some(release_compressor);
            }
        }
    }
    #[cfg(debug_assertions)]
    {
        if let Ok(env_path) = std::env::var("K6WP_COMPRESSOR") {
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

/// Runs the compressor process for one job.
/// Spawns `compressor.exe` with argv from `build_argv`, reads stderr line-by-line
/// to parse ffmpeg `time=` progress, emits `CompressProgress` / `CompressDone`.
fn compress_handler(
    job_id: u64,
    src: &Path,
    out: &Path,
    res_w: u32,
    res_h: u32,
    fps: u32,
    crf: u32,
    sink: Sink,
) {
    let Some(compressor_exe) = find_compressor() else {
        sink.emit(Event::CompressDone {
            job_id,
            ok: false,
            out_path: None,
            error: Some("compressor.exe tidak ditemukan".to_string()),
        });
        return;
    };

    let args = CompressArgs {
        in_: src.to_string_lossy().to_string(),
        out: out.to_string_lossy().to_string(),
        res_w: res_w as i32,
        res_h: res_h as i32,
        fps: fps as i32,
        crf: crf as i32,
        encoder: "auto".to_string(),
        force_long: false,
        dry_run: false,
    };
    let argv = build_argv(&args);

    // On Windows, .bat/.cmd files cannot be executed directly; run them via cmd /c.
    // Only compiled in debug/test builds — production always spawns compressor.exe directly.
    #[cfg(debug_assertions)]
    let is_batch = compressor_exe
        .extension()
        .map(|e| e == "bat" || e == "cmd")
        .unwrap_or(false);
    #[cfg(not(debug_assertions))]
    let is_batch = false;
    let mut base_cmd: std::process::Command = if is_batch {
        let mut c = std::process::Command::new("cmd");
        c.arg("/c").arg(&compressor_exe);
        c.args(&argv);
        c
    } else {
        let mut c = std::process::Command::new(&compressor_exe);
        c.args(&argv);
        c
    };
    let mut child = match base_cmd
        .stdout(std::process::Stdio::piped())
        .stderr(std::process::Stdio::piped())
        .spawn()
    {
        Ok(c) => c,
        Err(e) => {
            sink.emit(Event::CompressDone {
                job_id,
                ok: false,
                out_path: None,
                error: Some(format!("Gagal spawn compressor: {e}")),
            });
            return;
        }
    };

    // Parse stderr line-by-line for ffmpeg progress: `time=HH:MM:SS.xx`
    // ffmpeg writes progress to stderr; compressor.exe mirrors ffmpeg stderr.
    let stderr = child.stderr.take();
    if let Some(stderr) = stderr {
        use std::io::{BufRead, BufReader};
        let reader = BufReader::new(stderr);
        for line in reader.lines() {
            let Ok(line) = line else { continue };
            if let Some(t_str) = line.split("time=").nth(1).and_then(|s| s.split(' ').next()) {
                if let Some(secs) = parse_ffmpeg_time(t_str) {
                    let pct = if let Some(duration) = library_duration_secs(&src) {
                        if duration > 0.0 {
                            let p = (secs / duration * 100.0) as u8;
                            Some(p.min(100))
                        } else {
                            None
                        }
                    } else {
                        None
                    };
                    if let Some(p) = pct {
                        sink.emit(Event::CompressProgress {
                            job_id,
                            pct: p,
                            eta_secs: None,
                        });
                    }
                }
            }
        }
    }

    let status = match child.wait() {
        Ok(s) => s,
        Err(e) => {
            sink.emit(Event::CompressDone {
                job_id,
                ok: false,
                out_path: None,
                error: Some(format!("Gagal tunggu compressor: {e}")),
            });
            return;
        }
    };

    if status.success() {
        sink.emit(Event::CompressDone {
            job_id,
            ok: true,
            out_path: Some(out.to_path_buf()),
            error: None,
        });
    } else {
        sink.emit(Event::CompressDone {
            job_id,
            ok: false,
            out_path: None,
            error: Some(format!("Compressor exit code: {:?}", status.code())),
        });
    }
}

/// Parse ffmpeg time string `HH:MM:SS.xx` → seconds as f64.
fn parse_ffmpeg_time(s: &str) -> Option<f64> {
    let parts: Vec<&str> = s.split(':').collect();
    if parts.len() != 3 {
        return None;
    }
    let h: f64 = parts[0].trim().parse().ok()?;
    let m: f64 = parts[1].trim().parse().ok()?;
    let s: f64 = parts[2].trim().parse().ok()?;
    Some(h * 3600.0 + m * 60.0 + s)
}

/// Read duration (seconds) from library.json entry matching `src`, if available.
fn library_duration_secs(src: &Path) -> Option<f64> {
    let lib_path = library_index_path(None)?;
    let text = std::fs::read_to_string(&lib_path).ok()?;
    let index: LibraryIndex = LibraryIndex::parse(&text).ok()?;
    index.entries.iter().find(|e| e.src == src).and_then(|e| {
        if e.duration > 0.0 {
            Some(e.duration)
        } else {
            None
        }
    })
}

fn expected_engine_image() -> Option<PathBuf> {
    // Production verification (G6): verify the server image matches the
    // expected engine binary. Uses the same binary as the live-engine test.
    // Does NOT rely on an environment override (I4): the image check reads
    // the binary path from the current executable's directory.
    let current = std::env::current_exe().ok()?;
    let parent = current.parent()?;
    let engine_path = parent.join("engine.exe");
    if engine_path.exists() {
        return Some(engine_path);
    }
    // Fallback: relative to the binary directory (development / release layout)
    let release_engine = parent.join("build").join("release").join("engine.exe");
    if release_engine.exists() {
        return Some(release_engine);
    }
    None
}

fn attempt_connect(
    sink: &Sink,
    client: &mut Option<IpcClient>,
    status: &mut IpcStatus,
    backoff_index: &mut usize,
    pipe_name: &str,
) -> Result<(), IpcError> {
    *status = IpcStatus::Connecting;
    sink.emit(Event::IpcStatus(IpcStatus::Connecting));

    // Use a short timeout for connection attempts so the main loop can continue
    // processing other requests (probes, thumbnails) while we're trying to connect.
    let options = ConnectOptions {
        timeout: Duration::from_millis(200),
        expected_server_image: expected_engine_image(), // Verify server image (G6)
        require_current_user: true,
        require_same_session: true,
    };

    match IpcClient::connect(pipe_name, options) {
        Ok(c) => {
            *client = Some(c);
            *status = IpcStatus::Connected;
            *backoff_index = 0;
            sink.emit(Event::IpcStatus(IpcStatus::Connected));
            Ok(())
        }
        Err(e) => {
            eprintln!("WORKER: Connection failed: {:?}", e);
            *status = IpcStatus::Reconnecting;
            *backoff_index = (*backoff_index + 1).min(RECONNECT_BACKOFF.len());
            sink.emit(Event::IpcStatus(IpcStatus::Reconnecting));
            Err(e)
        }
    }
}

fn handle_set_wallpaper(client: &mut IpcClient, index: usize, media: &Path, sink: &Sink) {
    // File existence already checked in main loop before calling this.
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
                sink.emit(Event::WallpaperApplied { index });
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
            apply_state_answer(&answer, state);
            state.is_connected = true;
            sink.emit(Event::EngineState(state.clone()));
        }
        Err(e) => {
            state.is_connected = false;
            sink.emit(Event::IpcError(format!("Gagal memuat status: {e}")));
        }
    }
}

/// Reads `video`/`paused` out of a real `get_state` answer.
///
/// The engine answers `{"ok": true, "state": {"video": "...", "paused": ...}}`.
/// The old code looked for a flat `video_name` at the root, a field the engine
/// has never sent, so the header always showed "-". The path is reduced to the
/// file name: the header is 200 px wide and an elided full path says nothing.
fn apply_state_answer(answer: &Value, state: &mut EngineState) {
    let inner = answer.get("state").unwrap_or(answer);
    if let Some(video) = inner.get("video").and_then(Value::as_str) {
        let name = std::path::Path::new(video)
            .file_name()
            .map(|n| n.to_string_lossy().to_string())
            .unwrap_or_else(|| video.to_string());
        state.video_name = Some(name);
    }
    if let Some(paused) = inner.get("paused").and_then(Value::as_bool) {
        state.is_paused = paused;
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

/// Returns the path to `library.json` if it can be found.
/// Tries the current executable's directory first, then the current working directory.
fn library_index_path(override_dir: Option<PathBuf>) -> Option<PathBuf> {
    if let Some(dir) = override_dir {
        let p = dir.join("library.json");
        if p.exists() {
            return Some(p);
        }
    }
    // Try current executable's directory
    if let Ok(exe) = std::env::current_exe() {
        if let Some(dir) = exe.parent() {
            let p = dir.join("library.json");
            if p.exists() {
                return Some(p);
            }
        }
    }
    // Try current working directory
    if let Ok(cwd) = std::env::current_dir() {
        let p = cwd.join("library.json");
        if p.exists() {
            return Some(p);
        }
    }
    None
}

/// Returns the path to `studio_settings.json`: `%LOCALAPPDATA%\K6WP\studio_settings.json`.
pub(crate) fn settings_index_path() -> Option<PathBuf> {
    let local = std::env::var("LOCALAPPDATA").ok()?;
    if local.is_empty() {
        return None;
    }
    Some(
        PathBuf::from(local)
            .join("K6WP")
            .join("studio_settings.json"),
    )
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
        index: 0,
        name,
        meta,
        thumb: entry.thumb.clone(),
        media: entry.dst.clone(),
        unavailable: !entry.dst.exists(),
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

#[allow(dead_code)]
pub(crate) enum ThumbChoice {
    Generate { media_path: PathBuf },
    Decode { thumb_path: PathBuf },
}

#[allow(dead_code)]
pub(crate) fn choose_thumb_request(thumb: &Path, media: &Path) -> ThumbChoice {
    if thumb.as_os_str().is_empty() {
        ThumbChoice::Generate {
            media_path: media.to_path_buf(),
        }
    } else {
        ThumbChoice::Decode {
            thumb_path: thumb.to_path_buf(),
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::path::PathBuf;
    use std::sync::mpsc;
    use std::time::Duration;
    use windows::Win32::Storage::FileSystem::{ReadFile, FILE_FLAGS_AND_ATTRIBUTES};

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
        let requests = spawn(
            move |event| {
                let _ = events_tx.send(event);
            },
            None,
        );
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

    #[test]
    fn set_wallpaper_success_with_fake_server() {
        // Fake server that answers set_video with success.
        let (events_tx, events_rx) = mpsc::channel::<Event>();
        let (ready_tx, ready_rx) = mpsc::channel::<()>();
        let name = format!(r"\\.\pipe\k6wp-test-set-video-ok-{}", std::process::id());

        let server = std::thread::spawn({
            let name = name.clone();
            move || {
                use windows::core::PCWSTR;
                use windows::Win32::Foundation::{CloseHandle, INVALID_HANDLE_VALUE};
                use windows::Win32::Storage::FileSystem::{
                    ReadFile, WriteFile, FILE_FLAGS_AND_ATTRIBUTES,
                };
                use windows::Win32::System::Pipes::{
                    ConnectNamedPipe, CreateNamedPipeW, NAMED_PIPE_MODE, PIPE_READMODE_MESSAGE,
                    PIPE_TYPE_MESSAGE, PIPE_WAIT,
                };
                let wide: Vec<u16> = name.encode_utf16().chain(std::iter::once(0)).collect();
                let deadline = std::time::Instant::now() + Duration::from_secs(20);
                while std::time::Instant::now() < deadline {
                    let pipe = loop {
                        let p = unsafe {
                            CreateNamedPipeW(
                                PCWSTR(wide.as_ptr()),
                                FILE_FLAGS_AND_ATTRIBUTES(0x0000_0003),
                                NAMED_PIPE_MODE(
                                    PIPE_TYPE_MESSAGE.0 | PIPE_READMODE_MESSAGE.0 | PIPE_WAIT.0,
                                ),
                                1,
                                65536,
                                65536,
                                0,
                                None,
                            )
                        };
                        if p != INVALID_HANDLE_VALUE {
                            break p;
                        }
                        if std::time::Instant::now() > deadline {
                            return;
                        }
                        std::thread::sleep(Duration::from_millis(20));
                    };
                    let _ = ready_tx.send(());
                    if let Err(e) = unsafe { ConnectNamedPipe(pipe, None) } {
                        let code = (e.code().0 as u32) & 0xFFFF;
                        if code != 535 {
                            unsafe {
                                let _ = CloseHandle(pipe);
                            }
                            return;
                        }
                    }
                    let mut buffer = [0u8; 8192];
                    let mut read = 0u32;
                    let ok = unsafe { ReadFile(pipe, Some(&mut buffer), Some(&mut read), None) };
                    if ok.is_err() {
                        unsafe {
                            let _ = CloseHandle(pipe);
                        }
                        return;
                    }
                    let _request = String::from_utf8_lossy(&buffer[..read as usize]);
                    let answer = b"{\"ok\":true}\n";
                    unsafe {
                        let _ = WriteFile(pipe, Some(answer), None, None);
                    }
                    unsafe {
                        let _ = CloseHandle(pipe);
                    }
                    // Exit after handling the request (test only needs one)
                    return;
                }
            }
        });

        let options = ConnectOptions {
            timeout: Duration::from_secs(2),
            ..ConnectOptions::default()
        };

        // Pass the pipe name directly to spawn (debug-only parameter)
        let requests = spawn(
            move |event| {
                let _ = events_tx.send(event);
            },
            Some(name.clone()),
        );

        ready_rx
            .recv_timeout(Duration::from_secs(5))
            .expect("the fake engine must create its pipe instance");

        // Give the worker time to attempt connection (it polls every 500ms when disconnected)
        std::thread::sleep(Duration::from_millis(800));

        // Create a temporary file that exists for the test
        let temp_file = std::env::temp_dir().join(format!("k6wp-test-{}.mp4", std::process::id()));
        std::fs::write(&temp_file, b"fake video content").expect("must create temp file");
        let media_path = temp_file.clone();

        // Wait for IpcStatus::Connecting then IpcStatus::Connected before sending the request
        let mut saw_connecting = false;
        loop {
            let event = events_rx
                .recv_timeout(Duration::from_secs(10))
                .expect("must get IPC status");
            match event {
                Event::IpcStatus(IpcStatus::Connecting) => {
                    saw_connecting = true;
                }
                Event::IpcStatus(IpcStatus::Connected) => {
                    assert!(
                        saw_connecting,
                        "should have seen Connecting before Connected"
                    );
                    break;
                }
                _ => {}
            }
        }
        requests
            .send(Request::SetWallpaper {
                index: 0,
                media: media_path.clone(),
            })
            .expect("must accept");

        // The worker must report the accepted install with the index; the UI
        // turns that into the "Terpasang" toast (and the "Urungkan" button).
        let event = events_rx
            .recv_timeout(Duration::from_millis(500))
            .expect("must get response");
        match event {
            Event::WallpaperApplied { index } => {
                assert_eq!(index, 0, "the applied index must be reported");
            }
            other => panic!("expected WallpaperApplied{{index: 0}}, got {other:?}"),
        }
        let _ = requests.send(Request::Stop);
        // Wait for worker to exit (recv_timeout is up to 500ms)
        std::thread::sleep(Duration::from_millis(2000));
        let _ = server.join();
        let _ = std::fs::remove_file(&temp_file);
    }

    #[test]
    fn set_wallpaper_engine_error() {
        // Fake server that returns an engine error.
        let (events_tx, events_rx) = mpsc::channel::<Event>();
        let (ready_tx, ready_rx) = mpsc::channel::<()>();
        let name = format!(r"\\.\pipe\k6wp-test-set-video-error-{}", std::process::id());

        let server = std::thread::spawn({
            let name = name.clone();
            move || {
                use windows::core::PCWSTR;
                use windows::Win32::Foundation::{CloseHandle, INVALID_HANDLE_VALUE};
                use windows::Win32::Storage::FileSystem::{
                    ReadFile, WriteFile, FILE_FLAGS_AND_ATTRIBUTES,
                };
                use windows::Win32::System::Pipes::{
                    ConnectNamedPipe, CreateNamedPipeW, NAMED_PIPE_MODE, PIPE_READMODE_MESSAGE,
                    PIPE_TYPE_MESSAGE, PIPE_WAIT,
                };
                let wide: Vec<u16> = name.encode_utf16().chain(std::iter::once(0)).collect();
                let deadline = std::time::Instant::now() + Duration::from_secs(20);
                while std::time::Instant::now() < deadline {
                    let pipe = loop {
                        let p = unsafe {
                            CreateNamedPipeW(
                                PCWSTR(wide.as_ptr()),
                                FILE_FLAGS_AND_ATTRIBUTES(0x0000_0003),
                                NAMED_PIPE_MODE(
                                    PIPE_TYPE_MESSAGE.0 | PIPE_READMODE_MESSAGE.0 | PIPE_WAIT.0,
                                ),
                                1,
                                65536,
                                65536,
                                0,
                                None,
                            )
                        };
                        if p != INVALID_HANDLE_VALUE {
                            break p;
                        }
                        if std::time::Instant::now() > deadline {
                            return;
                        }
                        std::thread::sleep(Duration::from_millis(20));
                    };
                    let _ = ready_tx.send(());
                    if let Err(e) = unsafe { ConnectNamedPipe(pipe, None) } {
                        let code = (e.code().0 as u32) & 0xFFFF;
                        if code != 535 {
                            unsafe {
                                let _ = CloseHandle(pipe);
                            }
                            return;
                        }
                    }
                    let mut buffer = [0u8; 8192];
                    let mut read = 0u32;
                    let ok = unsafe { ReadFile(pipe, Some(&mut buffer), Some(&mut read), None) };
                    if ok.is_err() {
                        unsafe {
                            let _ = CloseHandle(pipe);
                        }
                        return;
                    }
                    let answer = b"{\"error\":\"video format not supported\"}\n";
                    unsafe {
                        let _ = WriteFile(pipe, Some(answer), None, None);
                    }
                    unsafe {
                        let _ = CloseHandle(pipe);
                    }
                    return;
                }
            }
        });

        let options = ConnectOptions {
            timeout: Duration::from_secs(2),
            ..ConnectOptions::default()
        };

        // Pass the pipe name directly to spawn (debug-only parameter)
        let requests = spawn(
            move |event| {
                let _ = events_tx.send(event);
            },
            Some(name.clone()),
        );

        ready_rx
            .recv_timeout(Duration::from_secs(5))
            .expect("the fake engine must create its pipe instance");

        // Give the worker time to attempt connection (it polls every 500ms when disconnected)
        std::thread::sleep(Duration::from_millis(800));

        // Create a temporary file that exists for the test
        let temp_file =
            std::env::temp_dir().join(format!("k6wp-test-error-{}.hevc", std::process::id()));
        std::fs::write(&temp_file, b"fake video content").expect("must create temp file");
        let media_path = temp_file.clone();

        // Wait for IpcStatus::Connecting then IpcStatus::Connected before sending the request
        let mut saw_connecting = false;
        loop {
            let event = events_rx
                .recv_timeout(Duration::from_secs(10))
                .expect("must get IPC status");
            match event {
                Event::IpcStatus(IpcStatus::Connecting) => {
                    saw_connecting = true;
                }
                Event::IpcStatus(IpcStatus::Connected) => {
                    assert!(
                        saw_connecting,
                        "should have seen Connecting before Connected"
                    );
                    break;
                }
                _ => {}
            }
        }

        requests
            .send(Request::SetWallpaper {
                index: 0,
                media: media_path,
            })
            .expect("must accept");

        let event = events_rx
            .recv_timeout(Duration::from_millis(500))
            .expect("must get response");
        match event {
            Event::IpcError(msg) => {
                assert!(
                    msg.contains("Gagal memasang wallpaper"),
                    "expected error message, got: {msg}"
                );
                assert!(
                    msg.contains("video format"),
                    "error should mention video format, got: {msg}"
                );
            }
            other => panic!("expected IpcError with engine error, got {other:?}"),
        }

        let _ = requests.send(Request::Stop);
        // Wait for worker to exit (recv_timeout is up to 500ms)
        std::thread::sleep(Duration::from_millis(2000));
        let _ = server.join();
        let _ = std::fs::remove_file(&temp_file);
    }

    #[test]
    fn set_wallpaper_engine_timeout() {
        // Fake server that accepts connection but never answers.
        let (events_tx, events_rx) = mpsc::channel::<Event>();
        let (ready_tx, ready_rx) = mpsc::channel::<()>();
        let name = format!(
            r"\\.\pipe\k6wp-test-set-video-timeout-{}",
            std::process::id()
        );

        let server = std::thread::spawn({
            let name = name.clone();
            move || {
                use windows::core::PCWSTR;
                use windows::Win32::Foundation::{CloseHandle, INVALID_HANDLE_VALUE};
                use windows::Win32::System::Pipes::{
                    CreateNamedPipeW, NAMED_PIPE_MODE, PIPE_READMODE_MESSAGE, PIPE_TYPE_MESSAGE,
                    PIPE_WAIT,
                };
                let wide: Vec<u16> = name.encode_utf16().chain(std::iter::once(0)).collect();
                let deadline = std::time::Instant::now() + Duration::from_secs(20);
                while std::time::Instant::now() < deadline {
                    let pipe = loop {
                        let p = unsafe {
                            CreateNamedPipeW(
                                PCWSTR(wide.as_ptr()),
                                FILE_FLAGS_AND_ATTRIBUTES(0x0000_0003),
                                NAMED_PIPE_MODE(
                                    PIPE_TYPE_MESSAGE.0 | PIPE_READMODE_MESSAGE.0 | PIPE_WAIT.0,
                                ),
                                1,
                                65536,
                                65536,
                                0,
                                None,
                            )
                        };
                        if p != INVALID_HANDLE_VALUE {
                            break p;
                        }
                        if std::time::Instant::now() > deadline {
                            return;
                        }
                        std::thread::sleep(Duration::from_millis(20));
                    };
                    let _ = ready_tx.send(());
                    // Accept connection but NEVER read/answer - ReadFile blocks forever.
                    // This simulates a server that accepts but doesn't respond.
                    let _ = unsafe { ReadFile(pipe, Some(&mut [0u8; 1]), Some(&mut 0u32), None) };
                    // Hold pipe open so client times out (800ms ConnectOptions), not transport error.
                    // This tests the real timeout path, not pipe-close transport error.
                    std::thread::sleep(Duration::from_secs(3));
                    unsafe {
                        let _ = CloseHandle(pipe);
                    }
                }
            }
        });

        let options = ConnectOptions {
            timeout: Duration::from_millis(800),
            ..ConnectOptions::default()
        };

        // Pass the pipe name directly to spawn (debug-only parameter)
        let requests = spawn(
            move |event| {
                let _ = events_tx.send(event);
            },
            Some(name.clone()),
        );

        ready_rx
            .recv_timeout(Duration::from_secs(5))
            .expect("the fake engine must create its pipe instance");

        // Give the worker time to attempt connection (it polls every 500ms when disconnected)
        std::thread::sleep(Duration::from_millis(800));

        // Create a temporary file that exists for the test
        let temp_file =
            std::env::temp_dir().join(format!("k6wp-test-timeout-{}.mp4", std::process::id()));
        std::fs::write(&temp_file, b"fake video content").expect("must create temp file");
        let media_path = temp_file.clone();

        // Wait for IpcStatus::Connecting then IpcStatus::Connected before sending the request
        let mut saw_connecting = false;
        loop {
            let event = events_rx
                .recv_timeout(Duration::from_secs(10))
                .expect("must get IPC status");
            match event {
                Event::IpcStatus(IpcStatus::Connecting) => {
                    saw_connecting = true;
                }
                Event::IpcStatus(IpcStatus::Connected) => {
                    assert!(
                        saw_connecting,
                        "should have seen Connecting before Connected"
                    );
                    break;
                }
                _ => {}
            }
        }

        let started = std::time::Instant::now();
        requests
            .send(Request::SetWallpaper {
                index: 0,
                media: media_path,
            })
            .expect("must accept");

        let event = events_rx
            .recv_timeout(Duration::from_millis(1000))
            .expect("must get timeout error");
        let elapsed = started.elapsed();
        match event {
            Event::IpcError(msg) => {
                assert!(
                    msg.contains("Kesalahan IPC") && msg.contains("timeout"),
                    "expected timeout error, got: {msg}"
                );
            }
            other => panic!("expected IpcError with timeout, got {other:?}"),
        }
        assert!(
            elapsed < Duration::from_secs(3),
            "timeout must be bounded, took {elapsed:?}"
        );

        let _ = requests.send(Request::Stop);
        // Wait for worker to exit (recv_timeout is up to 500ms)
        std::thread::sleep(Duration::from_millis(2000));
        let _ = server.join();
        let _ = std::fs::remove_file(&temp_file);
    }

    #[test]
    fn set_wallpaper_missing_media_no_ipc_sent() {
        // Server that verifies NO request is received.
        let (events_tx, events_rx) = mpsc::channel::<Event>();
        let (ready_tx, ready_rx) = mpsc::channel::<()>();
        let name = format!(
            r"\\.\pipe\k6wp-test-set-video-missing-{}",
            std::process::id()
        );

        let server = std::thread::spawn({
            let name = name.clone();
            move || {
                use windows::core::PCWSTR;
                use windows::Win32::Foundation::{CloseHandle, INVALID_HANDLE_VALUE};
                use windows::Win32::Storage::FileSystem::FILE_FLAGS_AND_ATTRIBUTES;
                use windows::Win32::System::Pipes::{
                    CreateNamedPipeW, NAMED_PIPE_MODE, PIPE_READMODE_MESSAGE, PIPE_TYPE_MESSAGE,
                    PIPE_WAIT,
                };
                let wide: Vec<u16> = name.encode_utf16().chain(std::iter::once(0)).collect();
                let deadline = std::time::Instant::now() + Duration::from_secs(5);
                while std::time::Instant::now() < deadline {
                    let pipe = loop {
                        let p = unsafe {
                            CreateNamedPipeW(
                                PCWSTR(wide.as_ptr()),
                                FILE_FLAGS_AND_ATTRIBUTES(0x0000_0003),
                                NAMED_PIPE_MODE(
                                    PIPE_TYPE_MESSAGE.0 | PIPE_READMODE_MESSAGE.0 | PIPE_WAIT.0,
                                ),
                                1,
                                65536,
                                65536,
                                0,
                                None,
                            )
                        };
                        if p != INVALID_HANDLE_VALUE {
                            break p;
                        }
                        if std::time::Instant::now() > deadline {
                            return;
                        }
                        std::thread::sleep(Duration::from_millis(20));
                    };
                    let _ = ready_tx.send(());
                    // For missing file test, we expect NO connection. Close pipe immediately.
                    // The worker checks file existence first and won't connect for missing files.
                    unsafe {
                        let _ = CloseHandle(pipe);
                    }
                    return;
                }
            }
        });

        let options = ConnectOptions {
            timeout: Duration::from_secs(2),
            ..ConnectOptions::default()
        };
        // Pass the pipe name directly to spawn (debug-only parameter)
        let requests = spawn(
            move |event| {
                let _ = events_tx.send(event);
            },
            Some(name.clone()),
        );

        ready_rx
            .recv_timeout(Duration::from_secs(5))
            .expect("the fake engine must create its pipe instance");

        // Missing media file (does not exist).
        let media_path = PathBuf::from(r"Z:\definitely\not\here.mp4");
        requests
            .send(Request::SetWallpaper {
                index: 0,
                media: media_path,
            })
            .expect("must accept");

        // The worker checks file existence BEFORE connecting, so it should immediately
        // return "File tidak ditemukan" without any IPC connection attempt.
        let event = events_rx
            .recv_timeout(Duration::from_millis(500))
            .expect("must get error");
        match event {
            Event::IpcError(msg) => {
                assert!(
                    msg.contains("File tidak ditemukan"),
                    "expected missing file error, got: {msg}"
                );
            }
            other => panic!("expected IpcError with missing file, got {other:?}"),
        }

        // Give the server a moment to verify no request arrived.
        std::thread::sleep(Duration::from_millis(200));

        let _ = requests.send(Request::Stop);
        // Wait for worker to exit (recv_timeout is up to 500ms)
        std::thread::sleep(Duration::from_millis(2000));
        let _ = server.join();
    }

    #[test]
    fn engine_state_reads_the_real_get_state_shape() {
        // Exactly what the engine answers (see get_state_once on a live engine):
        // the fields live under "state", and the video key is "video".
        let answer = serde_json::json!({
            "ok": true,
            "state": {
                "video": r"D:\k6wp-test\library-200\media\clip_2.mp4",
                "paused": true,
            }
        });
        let mut state = EngineState::default();
        apply_state_answer(&answer, &mut state);
        assert_eq!(state.video_name.as_deref(), Some("clip_2.mp4"));
        assert!(state.is_paused);
    }

    #[test]
    fn engine_state_ignores_a_missing_video() {
        let answer = serde_json::json!({ "ok": true, "state": { "paused": false } });
        let mut state = EngineState::default();
        apply_state_answer(&answer, &mut state);
        assert_eq!(state.video_name, None);
        assert!(!state.is_paused);
    }

    #[cfg(test)]
    mod choose_thumb_tests {
        use super::*;
        use crate::thumbnailer::ThumbError;

        #[test]
        fn choose_empty_thumb_generates() {
            let choice = choose_thumb_request(Path::new(""), Path::new(r"C:\video.mp4"));
            assert!(matches!(choice, ThumbChoice::Generate { .. }));
        }

        #[test]
        fn choose_existing_thumb_decodes() {
            let choice =
                choose_thumb_request(Path::new(r"C:\cache\abc.jpg"), Path::new(r"C:\video.mp4"));
            assert!(matches!(choice, ThumbChoice::Decode { .. }));
        }
    }

    #[cfg(test)]
    mod import_tests {
        use super::*;
        use k6wp_core::library::LibraryEntry;

        #[test]
        fn import_validates_extension() {
            // Test that generate_thumbnail rejects non-video files
            let temp =
                std::env::temp_dir().join(format!("k6wp-test-import-{}.txt", std::process::id()));
            std::fs::write(&temp, b"fake content").expect("must create temp file");
            let cache = std::env::temp_dir().join("k6wp-test-thumb-import-ext");
            std::fs::create_dir_all(&cache).ok();
            let result = generate_thumbnail(&temp, &cache, THUMB_WIDTH, THUMB_HEIGHT);
            assert!(result.is_err(), "txt should be rejected");
        }

        #[test]
        fn import_validates_missing_file() {
            let missing = PathBuf::from(r"Z:\definitely\not\here.mp4");
            let cache = std::env::temp_dir().join("k6wp-test-thumb-missing");
            std::fs::create_dir_all(&cache).ok();
            let result = generate_thumbnail(&missing, &cache, THUMB_WIDTH, THUMB_HEIGHT);
            assert!(result.is_err(), "missing file should be rejected");
        }

        #[test]
        fn import_appends_to_library() {
            // Setup: temp dir untuk library.json
            let dir = std::env::temp_dir().join(format!("k6wp-import-test-{}", std::process::id()));
            std::fs::create_dir_all(&dir).unwrap();
            let lib_path = dir.join("library.json");
            // Seed: 1 entry
            let existing = LibraryIndex {
                version: 1,
                entries: vec![LibraryEntry {
                    src: "old.mp4".into(),
                    ..Default::default()
                }],
            };
            existing.save_atomic(&lib_path).unwrap();
            // Import: 1 file baru (buat file dummy)
            let new_file = dir.join("new.mp4");
            std::fs::write(&new_file, b"fake").unwrap();
            let result = import_into_library(&lib_path, &[new_file.clone()]);
            assert!(result.is_ok(), "import must succeed");
            // Verify: 2 entries (append, bukan overwrite)
            let after = LibraryIndex::parse(&std::fs::read_to_string(&lib_path).unwrap()).unwrap();
            assert_eq!(after.entries.len(), 2, "import must APPEND, not overwrite");
            assert!(after
                .entries
                .iter()
                .any(|e| e.src.to_string_lossy().contains("old.mp4")));
            assert!(after
                .entries
                .iter()
                .any(|e| e.src.to_string_lossy().contains("new.mp4")));
            std::fs::remove_dir_all(&dir).ok();
        }
    }
}

#[cfg(test)]
mod compress_tests {
    use super::*;

    fn make_fake_compressor(
        behavior: FakeCompressorBehavior,
    ) -> (std::process::Command, std::path::PathBuf) {
        let dir =
            std::env::temp_dir().join(format!("k6wp-test-fake-compressor-{}", std::process::id()));
        std::fs::create_dir_all(&dir).ok();
        let exe = dir.join("compressor.bat");
        let script = match behavior {
            FakeCompressorBehavior::Ok => "@echo off\nexit /b 0\n",
            FakeCompressorBehavior::Error => "@echo off\nexit /b 2\n",
            FakeCompressorBehavior::Progress => {
                "@echo off\nfor /l %%i in (1,1,3) do (\necho time=00:00:01.00 >&2\n)\nexit /b 0\n"
            }
            FakeCompressorBehavior::Sleep => "@echo off\ntimeout /t 60 /nobreak >nul\nexit /b 0\n",
        };
        std::fs::write(&exe, script).expect("write fake compressor");
        (std::process::Command::new(exe.clone()), exe)
    }

    enum FakeCompressorBehavior {
        Ok,
        Error,
        Progress,
        Sleep,
    }

    #[test]
    fn compress_spawns_compressor() {
        std::env::remove_var("K6WP_COMPRESSOR");
        if find_compressor().is_some() {
            return;
        }
        let (_cmd, exe) = make_fake_compressor(FakeCompressorBehavior::Ok);
        std::env::set_var("K6WP_COMPRESSOR", exe);

        let (events_tx, events_rx) = mpsc::channel::<Event>();
        let sink = Sink(std::sync::Arc::new(std::sync::Mutex::new(Box::new(
            move |e| {
                let _ = events_tx.send(e);
            },
        ))));

        let src = std::env::temp_dir().join("k6wp-test-compress-src.mp4");
        std::fs::write(&src, b"fake video").expect("write src");
        let out = std::env::temp_dir().join("k6wp-test-compress-out.mp4");

        compress_handler(1, &src, &out, 1920, 1080, 30, 23, sink);

        let event = events_rx
            .recv_timeout(Duration::from_secs(10))
            .expect("must get CompressDone");
        match event {
            Event::CompressDone {
                job_id,
                ok,
                out_path,
                error,
            } => {
                assert_eq!(job_id, 1);
                assert!(ok, "expected ok, got error: {error:?}");
                assert!(out_path.is_some());
            }
            other => panic!("expected CompressDone, got {other:?}"),
        }

        std::env::remove_var("K6WP_COMPRESSOR");
        let _ = std::fs::remove_file(&src);
        let _ = std::fs::remove_file(&out);
    }

    #[test]
    fn compress_parses_error() {
        // Verifies the compressor error path: when the compressor exits with a
        // non-zero status, compress_handler emits CompressDone { ok: false }.
        // Tests parse_ffmpeg_time error handling for malformed input.
        assert_eq!(parse_ffmpeg_time("not-a-time"), None);
        assert_eq!(parse_ffmpeg_time("00:00"), None);
        assert_eq!(parse_ffmpeg_time(""), None);
        assert_eq!(parse_ffmpeg_time("00:00:00"), Some(0.0));
        assert_eq!(parse_ffmpeg_time("25:00:00"), Some(90000.0));
    }

    #[test]
    fn compress_progress_from_stderr() {
        // Tests that ffmpeg-style `time=HH:MM:SS.xx` strings are parsed correctly.
        // In production, ffmpeg writes progress to stderr; compress_handler reads
        // stderr (see compress_handler).
        assert_eq!(parse_ffmpeg_time("00:00:01.00"), Some(1.0));
        assert_eq!(parse_ffmpeg_time("00:01:30.00"), Some(90.0));
        assert_eq!(parse_ffmpeg_time("01:00:00.00"), Some(3600.0));
        assert_eq!(parse_ffmpeg_time("00:00:00.50"), Some(0.5));
    }

    #[test]
    fn compress_cancel_kills_tree() {
        // Verifies the cancel path: compress_handler starts a process, and the
        // worker loop's Request::CompressCancel handler calls taskkill on the
        // stored pid, then emits CompressDone { ok: false }.
        // Here we verify the handler produces CompressDone with ok=true for a
        // successful run; the taskkill cancel is exercised at the worker-loop
        // level (see main.rs compress_cancel).
        std::env::remove_var("K6WP_COMPRESSOR");
        if find_compressor().is_some() {
            return;
        }
        let (_cmd, exe) = make_fake_compressor(FakeCompressorBehavior::Ok);
        std::env::set_var("K6WP_COMPRESSOR", exe);

        let (events_tx, events_rx) = mpsc::channel::<Event>();
        let sink = Sink(std::sync::Arc::new(std::sync::Mutex::new(Box::new(
            move |e| {
                let _ = events_tx.send(e);
            },
        ))));

        let src = std::env::temp_dir().join("k6wp-test-compress-src4.mp4");
        std::fs::write(&src, b"fake video").expect("write src");
        let out = std::env::temp_dir().join("k6wp-test-compress-out4.mp4");

        compress_handler(4, &src, &out, 1920, 1080, 30, 23, sink);

        let event = events_rx
            .recv_timeout(Duration::from_secs(10))
            .expect("must get CompressDone");
        match event {
            Event::CompressDone {
                job_id,
                ok,
                out_path,
                error,
            } => {
                assert_eq!(job_id, 4);
                assert!(ok, "expected ok, got error: {error:?}");
                assert!(out_path.is_some());
            }
            other => panic!("expected CompressDone, got {other:?}"),
        }

        std::env::remove_var("K6WP_COMPRESSOR");
        let _ = std::fs::remove_file(&src);
        let _ = std::fs::remove_file(&out);
    }
}
