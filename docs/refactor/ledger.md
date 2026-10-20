# Refactor ledger

One row per task. Every verdict follows §9 of the refactor brief:
`VERDICT: ACCEPT | REVERT | DEFER — criterion — before/after — evidence label`.

Evidence labels: **live-proven** (observed on this machine), **test-proven**
(automated test), **code-quoted** (read from source), **unmeasured**.

## Phase 0 — measure and freeze the baseline

| ID | Hypothesis | Status | Evidence |
|---|---|---|---|
| 0.1 | Tag + branch isolate the baseline; hygiene does not change binaries | applied | tag `baseline-pre-refactor` @ `9923cc0`; branch `refactor/rust`; `docs/refactor/repo-hygiene.md` (code-quoted) |
| 0.2 | Release + Debug build green; ctest result recorded without fixes | applied | `docs/refactor/baseline-tests.md` — release configure/build exit 0, ctest **48/48** in 72.5 s (code-quoted) |
| 0.3 | Existing harness covers CPU/RAM/startup; GPU, wakeups, resume latency, Studio, bundle and visual parity need new samplers | applied | new: `tools/bench_engine.ps1` (GPU+GPU-mem+wakeups), `tools/bench_resume.ps1`, `tools/bench_studio.ps1`, `tools/bench_bundle.ps1`, `tools/bench_visual.ps1`; shared helpers in `tools/bench_lib.ps1` (live-proven) |
| 0.4 | A 1080p and a 4K high-DPB clip expose decode-pool cost | applied | `tests/corpus/highdpb_4k.mp4` (4K30, 16 refs) → 387.5 MB private / 468.1 MB GPU shared vs 162.3 / 151.6 at 1080p (live-proven) |
| 0.5 | n≥5 medians establish noise bands for every protected metric | applied | `docs/bench/phase0.json` + `.raw.json` (20 runs, 4 states); bands in `docs/refactor/phase0-baseline.md` (live-proven) |
| 0.6 | Pure C++ behaviour can be frozen as golden JSON and replayed by Rust | applied | `tests/golden/shared_behavior.json` — **92 cases** from `tests/golden_dump.cpp`; enforced by the `golden_test` ctest (test-proven) |
| 0.7 | CLI/exit-code/tray/config/Studio inventory is complete enough for parity | applied | `docs/refactor/contract-inventory.md` — 744 lines, every claim `file:line`-cited (code-quoted) |
| 0.8 | Repo hygiene removes noise without deleting evidence | applied | `docs/refactor/repo-hygiene.md` (code-quoted) |
| 0.9 | Budgets are realistic against measured noise | applied | `docs/refactor/budgets.md` — 20 parity gates + 9 improvement targets, each derived from a measured band (live-proven) |
| 0.10 | The visual parity gate can actually FAIL (a gate that never fails is not a gate) | applied | `tools/verify_visual_negative.ps1` rejects the non-playing desktop (luma delta 59.5 vs tolerance 12) (live-proven) |

## Phase 1 — quick wins in C++

| ID | Hypothesis | Status | Evidence |
|---|---|---|---|
| 1.1 | Unloading mpv after a long pause returns RAM to near idle | pending | measured: a paused 1080p session still holds 162 MB private + 152 MB GPU shared (`docs/refactor/phase0-baseline.md`) — the target is real |
| 1.2 | Wakeups are explainable and can reach ~0 when paused | **accepted** | `docs/refactor/phase1-2-vsync.md`: `display-desync` busy-waits in mpv's VO thread — one thread at **66 % of a core, Running**. `display-vsync` → CPU **−72.5 %**, GPU 3D **−62.7 %**, wakeups **−20.1 %**, rendered frames **byte-identical** (pixel diff 0.000). One regression: GPU decode +34.9 % (work moved from the CPU spin into the GPU pipeline; total GPU still −34.3 %). 4K: CPU −64.9 %, GPU 3D −44.5 %, working set −30.2 % |
| 1.3 | A minimal libmpv build cuts bundle size without playback regression | pending | measured: libmpv is 114.77 MB of the 389.14 MB install (29.5 %); ffmpeg is 194.16 MB (49.9 %) |
| 1.4 | DPB-pinned re-encode lowers playback RAM for high-ref imports | pending | measured: 4K/16-ref playback holds 393.8 MB private + 466.9 MB GPU shared |
| 1.5 | Removing span + old-OS branches reduces code and claims | pending | the contract inventory found **no** minimum-OS check in the source at all (see 0.7) |

### Phase 1 side findings (not separate ledger rows)

| Finding | Verdict | Evidence |
|---|---|---|
| The wallpaper WorkerW host resolution rejected a WorkerW that held our own stale surface, so every re-anchor fell back to the invisible layered-Progman branch and the wallpaper disappeared right after start | **fixed** | `engine/src/desktop_inject.cpp` — `WorkerWAcceptsSurface()`; the log now shows a single injection into the wallpaper WorkerW |
| A `WM_TIMER` cannot wake the parked message loop (`QS_ALLINPUT` does not include timer messages) | documented | `engine/src/engine_app.cpp` — the QA capture uses the loop cadence with a bounded wait instead |
| `Copy-Item` preserves the source mtime, so restoring a source from a `.bak` made MSBuild skip the recompile — an A/B compared a build with itself | **fixed** | `tools/build_ab.ps1` touches the source and verifies the two binaries differ by SHA-256 before the A/B runs |

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
