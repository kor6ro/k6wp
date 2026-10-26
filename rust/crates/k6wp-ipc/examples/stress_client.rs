//! The IPC stress client (task K1).
//!
//! It hammers `get_state` from N threads and reports the latency
//! distribution - the p50, the p99 and the maximum - plus the failures
//! (a timeout, a broken pipe, a refused server).
//!
//! WHY
//!   The engine's IPC thread serves the clients one after another. A slow
//!   `get_state` (one that waits for a lock the main loop holds during a
//!   renderer create/destroy) blocks EVERY other client - the Studio then
//!   shows "Disconnected". This client is the measuring instrument for that
//!   defect: the same load against the baseline build and against the
//!   current one tells us whether it is a regression.
//!
//! USAGE
//!   cargo run --release --example stress_client -- --threads 4 --requests 200
//!
//! The options:
//!   --threads N     the parallel clients (default 4)
//!   --requests N    the requests per client (default 200)
//!   --pipe NAME     the pipe name (default: this session's)
//!   --image PATH    the engine image the server must have (default: none -
//!                   the verification of the image is skipped)
//!   --json PATH     the report file (default: stdout only)

use std::sync::atomic::{AtomicBool, AtomicU64, Ordering};
use std::sync::Arc;
use std::time::{Duration, Instant};

use k6wp_core::ipc::{Cmd, IpcMessage, PROTOCOL_VERSION};
use k6wp_ipc::{current_session_pipe_name, ConnectOptions, IpcClient};

fn main() {
    let mut threads: usize = 4;
    let mut requests: usize = 200;
    let mut pipe = current_session_pipe_name(None);
    let mut image: Option<String> = None;
    let mut json_out: Option<String> = None;
    let mut samples_out: Option<String> = None;
    let mut churn: Option<String> = None;
    let mut churn_every: usize = 25;

    let args: Vec<String> = std::env::args().collect();
    let mut i = 1;
    while i < args.len() {
        match args[i].as_str() {
            "--threads" => {
                threads = args
                    .get(i + 1)
                    .and_then(|v| v.parse().ok())
                    .unwrap_or(threads);
                i += 2;
            }
            "--requests" => {
                requests = args
                    .get(i + 1)
                    .and_then(|v| v.parse().ok())
                    .unwrap_or(requests);
                i += 2;
            }
            "--pipe" => {
                pipe = args.get(i + 1).cloned().unwrap_or(pipe);
                i += 2;
            }
            "--image" => {
                image = args.get(i + 1).cloned();
                i += 2;
            }
            "--churn" => {
                churn = args.get(i + 1).cloned();
                i += 2;
            }
            "--churn-every" => {
                churn_every = args
                    .get(i + 1)
                    .and_then(|v| v.parse().ok())
                    .unwrap_or(churn_every);
                i += 2;
            }
            "--json" => {
                json_out = args.get(i + 1).cloned();
                i += 2;
            }
            // L1: write every sample as CSV (offset_ms,latency_ms), so a
            // spike can be attributed in time instead of only summarised by
            // the max. The offset is milliseconds since the run started.
            "--samples" => {
                samples_out = args.get(i + 1).cloned();
                i += 2;
            }
            other => {
                eprintln!("stress_client: the unknown argument {other}");
                std::process::exit(2);
            }
        }
    }

    let options = ConnectOptions {
        timeout: Duration::from_secs(5),
        expected_server_image: image.map(std::path::PathBuf::from),
        require_current_user: true,
        require_same_session: true,
    };

    let churn_shared = churn.clone();
    let stop = Arc::new(AtomicBool::new(false));
    let failures = Arc::new(AtomicU64::new(0));
    let mut latencies: Vec<u64> = Vec::with_capacity(threads * requests);

    let started = Instant::now();
    let mut handles = Vec::new();
    for _ in 0..threads {
        let churn_shared = churn_shared.clone();
        let pipe = pipe.clone();
        let options = options.clone();
        let stop = stop.clone();
        let failures = failures.clone();
        handles.push(std::thread::spawn(move || {
            // L1: (offset_ms, latency_ms) per sample, so the report can show
            // WHEN a spike happened (a once-per-second spike has a shape).
            let mut samples: Vec<(u64, u64)> = Vec::new();
            let thread_started = started;
            let mut client = match IpcClient::connect(&pipe, options) {
                Ok(client) => client,
                Err(e) => {
                    eprintln!("stress_client: the connect failed: {e}");
                    failures.fetch_add(1, Ordering::Relaxed);
                    return samples;
                }
            };
            let churn_file = churn_shared.clone();
            for index in 0..requests {
                if stop.load(Ordering::Relaxed) {
                    break;
                }
                // The CHURN: a video change (the engine destroys and creates
                // the renderer - the expensive path) and a display
                // assignment. The clients keep hammering get_state while the
                // engine works: that is the load the defect showed up in.
                if let Some(path) = churn_file.as_ref() {
                    if index > 0 && index % churn_every == 0 {
                        let set_video = IpcMessage {
                            version: PROTOCOL_VERSION,
                            cmd: Cmd::SetVideo,
                            payload: serde_json::json!({ "path": path }),
                        };
                        let _ = client.request(&set_video);
                        let assign = IpcMessage {
                            version: PROTOCOL_VERSION,
                            cmd: Cmd::SetDisplayVideo,
                            payload: serde_json::json!({
                                "device": "\\\\.\\DISPLAY1",
                                "path": path,
                            }),
                        };
                        let _ = client.request(&assign);
                    }
                }
                let message = IpcMessage {
                    version: PROTOCOL_VERSION,
                    cmd: Cmd::GetState,
                    payload: serde_json::json!({}),
                };
                let t0 = Instant::now();
                match client.request(&message) {
                    Ok(ack) => {
                        let elapsed = t0.elapsed().as_millis() as u64;
                        samples.push((thread_started.elapsed().as_millis() as u64, elapsed));
                        if ack.get("state").is_none() {
                            failures.fetch_add(1, Ordering::Relaxed);
                        }
                    }
                    Err(_) => {
                        let elapsed = t0.elapsed().as_millis() as u64;
                        samples.push((thread_started.elapsed().as_millis() as u64, elapsed));
                        failures.fetch_add(1, Ordering::Relaxed);
                        // A broken pipe: reconnect for the next request. The
                        // reconnect releases the OLD handle first (it
                        // consumes the client) - the engine serves one
                        // client at a time, so a live handle keeps the pipe
                        // instance busy and the new connect would wait for
                        // the whole timeout for nothing (task L4).
                        let options = ConnectOptions {
                            timeout: Duration::from_secs(5),
                            expected_server_image: None,
                            require_current_user: true,
                            require_same_session: true,
                        };
                        match client.reconnect(options) {
                            Ok(next) => client = next,
                            Err(_) => break,
                        }
                    }
                }
            }
            samples
        }));
    }

    let mut series: Vec<(u64, u64)> = Vec::with_capacity(threads * requests);
    for handle in handles {
        if let Ok(samples) = handle.join() {
            for (offset_ms, latency_ms) in samples {
                latencies.push(latency_ms);
                series.push((offset_ms, latency_ms));
            }
        }
    }
    let wall = started.elapsed();

    latencies.sort_unstable();
    let percentile = |p: usize| -> u64 {
        if latencies.is_empty() {
            return 0;
        }
        let index = (latencies.len() * p / 100).min(latencies.len() - 1);
        latencies[index]
    };
    let p50 = percentile(50);
    let p95 = percentile(95);
    let p99 = percentile(99);
    let max = latencies.last().copied().unwrap_or(0);
    let failures = failures.load(Ordering::Relaxed);
    let count = latencies.len();

    eprintln!(
        "stress_client: {count} requests, the p50 {p50} ms, the p95 {p95} ms, the p99 {p99} ms, the max {max} ms, {failures} failures, the wall {:.1} s", wall.as_secs_f64()
    );

    if let Some(path) = json_out {
        let report = serde_json::json!({
            "requests": count,
            "threads": threads,
            "p50_ms": p50,
            "p95_ms": p95,
            "p99_ms": p99,
            "max_ms": max,
            "failures": failures,
            "wall_seconds": wall.as_secs_f64(),
        });
        if let Err(e) = std::fs::write(&path, format!("{report}\n")) {
            eprintln!("stress_client: the report could not be written: {e}");
            std::process::exit(2);
        }
    }

    if let Some(path) = samples_out {
        series.sort_by_key(|sample| sample.0);
        let mut text = String::from("offset_ms,latency_ms\n");
        for (offset_ms, latency_ms) in &series {
            text.push_str(&format!("{offset_ms},{latency_ms}\n"));
        }
        if let Err(e) = std::fs::write(&path, text) {
            eprintln!("stress_client: the samples could not be written: {e}");
            std::process::exit(2);
        }
    }
}
