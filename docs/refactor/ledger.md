# Refactor ledger

One row per task. Every verdict follows §9 of the refactor brief:
`VERDICT: ACCEPT | REVERT | DEFER — criterion — before/after — evidence label`.

Evidence labels: **live-proven** (observed on this machine), **test-proven**
(automated test), **code-quoted** (read from source), **unmeasured**.

## Phase 0 — measure and freeze the baseline

| ID | Hypothesis | Status | Evidence |
|---|---|---|---|
| 0.1 | Tag + branch isolate the baseline; hygiene does not change binaries | pending | |
| 0.2 | Release + Debug build green; ctest result recorded without fixes | pending | |
| 0.3 | Existing harness covers CPU/RAM/startup; GPU, wakeups, resume latency, Studio, bundle and visual parity need new samplers | pending | |
| 0.4 | A 1080p and a 4K high-DPB clip expose decode-pool cost | pending | |
| 0.5 | n≥5 medians establish noise bands for every protected metric | pending | |
| 0.6 | Pure C++ behaviour can be frozen as golden JSON and replayed by Rust | pending | |
| 0.7 | CLI/exit-code/tray/config/Studio inventory is complete enough for parity | pending | |
| 0.8 | Repo hygiene removes noise without deleting evidence | applied | `docs/refactor/repo-hygiene.md` (code-quoted) |
| 0.9 | Budgets are realistic against measured noise | pending | |

## Phase 1 — quick wins in C++

| ID | Hypothesis | Status | Evidence |
|---|---|---|---|
| 1.1 | Unloading mpv after a long pause returns RAM to near idle | pending | |
| 1.2 | Wakeups are explainable and can reach ~0 when paused | pending | |
| 1.3 | A minimal libmpv build cuts bundle size without playback regression | pending | |
| 1.4 | DPB-pinned re-encode lowers playback RAM for high-ref imports | pending | |
| 1.5 | Removing span + old-OS branches reduces code and claims | pending | |

## Phase 2 — Studio in Rust + Slint

| ID | Hypothesis | Status | Evidence |
|---|---|---|---|
| 2.1 | Cargo workspace + CI gates are stable alongside CMake | pending | |
| 2.2 | Shared codecs port 1:1 against golden fixtures | pending | |
| 2.3 | A Slint backend exists with 0% idle CPU and lower RAM than Qt | pending | |
| 2.4 | The UX spec meets the ≤3-step / ≤2-click targets | pending | |
| 2.5 | Page-by-page parity is testable and screenshot-backed | pending | |
| 2.6 | Rust Studio drives the C++ engine over the unchanged IPC | pending | |
| 2.7 | Qt removal shrinks the bundle and startup | pending | |

## Phase 3 — engine, launcher, compressor in Rust

| ID | Hypothesis | Status | Evidence |
|---|---|---|---|
| 3.1 | mpv option dump is byte-identical to C++ | pending | |
| 3.2 | Win32 ports preserve teardown and placement behaviour | pending | |
| 3.3 | Event-driven engine meets Phase 1 wakeups | pending | |
| 3.4 | Launcher merge keeps the autostart contract | pending | |
| 3.5 | Compressor subcommand keeps argv/exit-code contract | pending | |
| 3.6 | IPC parity scenarios match C++ | pending | |
| 3.7 | Cutover installer/ZIP upgrade in place | pending | |
| R1 | Shared N-monitor decode needs an upstream/vendor build; likely DEFER | pending | |
