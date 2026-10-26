//! The IPC stress client for pause/resume commands (Blok A-11 Langkah 1).
//!
//! It hammers `pause`/`resume` from N threads with random intervals and
//! reports the latency distribution - the p50, the p99 and the maximum -
//! plus the failures (a timeout, a broken pipe, a refused server).
//!
//! USAGE
//!   cargo run --release --example stress_pause_resume -- --threads 4 --requests 250
//!
//! The options:
//!   --threads N     the parallel clients (default 4)
//!   --requests N    the requests per client (default 250) -> total = threads * requests
//!   --pipe NAME     the pipe name (default: this session's)
//!   --image PATH    the engine image the server must have (default: none -
//!                   the verification of the image is skipped)
//!   --json PATH     the report file (default: stdout only)
//!   --policy NAME   connection policy: "keep-alive" (default) or "short"
//!   --interval-min MS minimum random interval between requests (default 20)
//!   --interval-max MS maximum random interval between requests (default 200)

use std::sync::atomic::{AtomicBool, AtomicU64, Ordering};
use std::sync::Arc;
use std::thread;
use std::time::{Duration, Instant, SystemTime, UNIX_EPOCH};

use k6wp_core::ipc::{Cmd, IpcMessage, PROTOCOL_VERSION};
use k6wp_ipc::{current_session_pipe_name, ConnectOptions, IpcClient};
use serde_json;

// Simple Xorshift64* RNG
struct SimpleRng(u64);

impl SimpleRng {
    fn new() -> Self {
        let seed = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .unwrap()
            .as_nanos() as u64;
        Self(seed)
    }
    fn u64(&mut self, range: std::ops::RangeInclusive<u64>) -> u64 {
        // Xorshift64*
        self.0 ^= self.0 << 13;
        self.0 ^= self.0 >> 7;
        self.0 ^= self.0 << 17;
        let val = self.0.wrapping_mul(0x2545F4914F6CDD1D);
        range.start() + (val % (range.end() - range.start() + 1))
    }
}

#[derive(Clone, Copy, Debug)]
enum Policy {
    KeepAlive,
    Short,
}

fn main() {
    let mut threads: usize = 4;
    let mut requests: usize = 250; // 4 * 250 = 1000 total
    let mut pipe = current_session_pipe_name(None);
    let mut image: Option<String> = None;
    let mut json_out: Option<String> = None;
    let mut policy = Policy::KeepAlive;
    let mut interval_min_ms: u64 = 20;
    let mut interval_max_ms: u64 = 200;

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
            "--json" => {
                json_out = args.get(i + 1).cloned();
                i += 2;
            }
            "--policy" => {
                let policy_str = args.get(i + 1).map_or(String::new(), |v| v.to_string());
                policy = match policy_str.as_str() {
                    "short" => Policy::Short,
                    _ => Policy::KeepAlive,
                };
                i += 2;
            }
            "--interval-min" => {
                interval_min_ms = args
                    .get(i + 1)
                    .and_then(|v| v.parse().ok())
                    .unwrap_or(interval_min_ms);
                i += 2;
            }
            "--interval-max" => {
                interval_max_ms = args
                    .get(i + 1)
                    .and_then(|v| v.parse().ok())
                    .unwrap_or(interval_max_ms);
                i += 2;
            }
            other => {
                eprintln!("stress_pause_resume: unknown argument {other}");
                std::process::exit(2);
            }
        }
        // i += 1;  // Remove this - we increment inside each match arm
    }

    if interval_min_ms > interval_max_ms {
        eprintln!("interval-min must be <= interval-max");
        std::process::exit(2);
    }

    let options = ConnectOptions {
        timeout: Duration::from_secs(5),
        expected_server_image: image.map(std::path::PathBuf::from),
        require_current_user: true,
        require_same_session: true,
    };

    let failures = Arc::new(AtomicU64::new(0));
    let pipe_busy_count = Arc::new(AtomicU64::new(0));
    let latencies = Arc::new(std::sync::Mutex::new(Vec::new()));

    let started = Instant::now();
    let mut handles = Vec::new();

    for _ in 0..threads {
        let pipe = pipe.clone();
        let options = options.clone();
        let failures = failures.clone();
        let pipe_busy_count = pipe_busy_count.clone();
        let latencies = latencies.clone();

        handles.push(std::thread::spawn(move || {
            let mut client = match IpcClient::connect(&pipe, options) {
                Ok(client) => client,
                Err(e) => {
                    eprintln!("stress_pause_resume: connect failed: {e}");
                    failures.fetch_add(1, Ordering::Relaxed);
                    return;
                }
            };

            let mut rng = SimpleRng::new();

            for index in 0..requests {
                // Random interval between requests
                let interval_ms = rng.u64(interval_min_ms..=interval_max_ms);
                thread::sleep(Duration::from_millis(interval_ms));

                let cmd = if index % 2 == 0 {
                    Cmd::Pause
                } else {
                    Cmd::Resume
                };
                let message = IpcMessage {
                    version: PROTOCOL_VERSION,
                    cmd,
                    payload: serde_json::json!({}),
                };

                let t0 = Instant::now();
                match client.request(&message) {
                    Ok(_) => {
                        let elapsed = t0.elapsed().as_millis() as u64;
                        latencies.lock().unwrap().push(elapsed);
                    }
                    Err(e) => {
                        let elapsed = t0.elapsed().as_millis() as u64;
                        latencies.lock().unwrap().push(elapsed);
                        eprintln!("Request failed: {e}");
                        let err_str = format!("{e}");
                        if err_str.contains("ERROR_PIPE_BUSY") || err_str.contains("231") {
                            pipe_busy_count.fetch_add(1, Ordering::Relaxed);
                        }
                        failures.fetch_add(1, Ordering::Relaxed);
                        // Reconnect for next request
                        match client.reconnect(ConnectOptions {
                            timeout: Duration::from_secs(5),
                            expected_server_image: None,
                            require_current_user: true,
                            require_same_session: true,
                        }) {
                            Ok(next) => client = next,
                            Err(_) => break,
                        }
                    }
                }

                // Small sleep to prevent overwhelming
                thread::sleep(Duration::from_millis(1));
            }
        }));
    }

    for handle in handles {
        let _ = handle.join();
    }

    let total_failures = failures.load(Ordering::Relaxed);
    let pipe_busy_failures = pipe_busy_count.load(Ordering::Relaxed);
    let total_requests = threads * requests;

    let mut all_latencies = latencies.lock().unwrap();
    all_latencies.sort_unstable();
    let percentile = |p: usize| -> u64 {
        if all_latencies.is_empty() {
            return 0;
        }
        let index = (all_latencies.len() * p / 100).min(all_latencies.len() - 1);
        all_latencies[index]
    };
    let p50 = percentile(50);
    let p95 = percentile(95);
    let p99 = percentile(99);
    let max = all_latencies.last().copied().unwrap_or(0);
    let count = all_latencies.len();

    eprintln!(
        "stress_pause_resume: {} requests, policy={:?}, p50={}ms p95={}ms p99={}ms max={}ms, failures={}, pipe_busy={}",
        count,
        policy,
        p50, p95, p99, max,
        total_failures,
        pipe_busy_failures
    );

    if let Some(path) = json_out {
        let report = serde_json::json!({
            "requests": count,
            "threads": threads,
            "policy": match policy { Policy::KeepAlive => "keep-alive", Policy::Short => "short" },
            "p50_ms": p50,
            "p95_ms": p95,
            "p99_ms": p99,
            "max_ms": max,
            "failures": total_failures,
            "pipe_busy_failures": pipe_busy_failures,
            "total_requests": total_requests,
        });
        if let Err(e) = std::fs::write(
            &path,
            format!("{}\n", serde_json::to_string_pretty(&report).unwrap()),
        ) {
            eprintln!("Failed to write report: {e}");
        }
    }
}
