# G5 coverage - what is frozen, what is not, and how the two are proven

Task I1. This file is the honest inventory of the engine's **decision
logic**: the parts a Phase 3 port must not change silently.

The rules this file follows:

- a golden fixture is only worth something when its behaviour is **proven**
  to be the same as the build it is compared against (the differential test
  against the `baseline-pre-refactor` tag) or when the change is **recorded
  as deliberate**;
- a decision that cannot be frozen as a fixture (it needs the clock, Win32,
  a process or the filesystem) is **not** "covered" - it is an **UNPROVEN**
  item with a **Phase 3 live test** as its destination;
- the numbers below are counts, not claims: the counts come from the
  fixture files themselves.

## 1. What IS frozen today

### 1.1 The engine's pure logic - `tests/golden/engine_logic.json`

**91 cases in 9 modules.** The table and the evaluator live in ONE place
(`tests/engine_logic_cases.hpp`) and call the **production** functions, so
the writer (`engine_logic_dump`) and the checker (the ctest
`engine_logic_test`) cannot drift apart. The Rust replayer
(`k6wp-core/tests/engine_logic_golden.rs`) runs the **same** fixture through
the Rust port: a difference fails on **both** sides.

| module | cases | what it freezes |
|---|---|---|
| `desktop_placement` | 9 | the coverage verdict of the surface on the monitor (the negative origin, the portrait, a smaller surface, a disjoint one) |
| `workerw_host` | 6 | which host window may hold our surface (task C7) |
| `retry_schedule` | 8 | the load retry offsets (0 / 250 / 1000 / 3000 ms) |
| `crash_restart_guard` | 8 | the automatic-restart backoff (3 in 10 minutes, the window boundary, the zero window, the clock going backwards) |
| `pause_controller` | 10 | the UI pause, the slot pause and the owner that hides the wallpaper |
| `topology_diff` | 10 | added / removed / moved / resized / dpi / orientation / primary / hash |
| `reconcile_policy` | 14 | 9 trigger-to-action mappings + 5 topology-stability cases |
| `unload_state` | 18 | the task 1.1 unload state machine |
| `pin_verify_schedule` | 8 | the due decision + the lifetime revert counter |

### 1.2 The shared contracts - `tests/golden/shared_behavior.json`

**93 cases:** the config (30: the defaults + 29 migrations), the Studio
settings (19), the playlist (11), the displays schema (6) and the IPC
protocol (27: the command vocabulary, the encode, the decode, the state
parser).

### 1.3 The other Rust-side fixtures

- `tests/golden/sps-dpb.json` - the SPS parser against ffmpeg's
  `trace_headers` (9/9) and the DPB byte budget.
- `tests/golden/library.json` - the library index codec.
- `rust/crates/k6wp-core/tests/` - the per-module Rust unit tests (the
  config, the settings, the displays, the playlist, the library, the IPC
  codec, the compress args).

## 2. The differential proof (the baseline)

`tools/g5_diff_baseline.ps1` builds the **same** harness inside a worktree
at the `baseline-pre-refactor` tag and compares the two outputs case by
case. The report is `docs/bench/g5-differential.json`.

**The measured result (this host):**

| item | value |
|---|---|
| the baseline commit | `9923cc0` (`baseline-pre-refactor`) |
| the compared values | **213** |
| the skipped cases | **24** (18 `unload_state` + 6 `workerw_host`) |
| the differences | **0** |
| the verdict | **IDENTICAL** |

**What is excluded, and why (never silently):**

| excluded | why |
|---|---|
| the `unload_state` module (18 cases) | the unload after a long pause is the **task 1.1** feature: the baseline has no counterpart to compare against. It is a **deliberate** change. |
| the `workerw_host` module (6 cases) | the WorkerW host rule became a pure function in **task C7**; in the baseline the check was inline in `desktop_inject.cpp`. The baseline shim returns `false` on purpose, so if the exclusion is ever removed the diff fails loudly instead of passing silently. |
| the `hides_wallpaper` field of `pause_controller` | the rule was extracted out of `EngineApp` in G5; the baseline has no such helper. The other three fields of that module ARE compared. |

**The three C++ refactors that G5 introduced** (each one is a pure
extraction, and each one is now the single definition):

| refactor | from | to | proven by |
|---|---|---|---|
| the "this owner hides the wallpaper" rule | `EngineApp::PauseHidesWallpaper` (a method) | `pause_controller.hpp` (`PauseHidesWallpaper`, a free function) | the fixture + the differential (the other fields) + the live black-frame gate |
| the unload state machine | `EngineApp::OnPauseStateChanged` / `MaybeUnloadAfterPause` (a method with side effects) | `engine/src/unload_state.hpp` (`UnloadOnPauseChanged` / `UnloadOnTick`, pure) | the fixture (18 cases) + the live black-frame gate (5 runs, 0 black frames) |
| the pin-verify decisions | the class' own clock | `pin_verify_schedule.hpp` (`PinVerifyDueAt` / `PinVerifyAddReverts`, pure) | the fixture (8 cases) + the differential |

## 3. What is NOT frozen yet (the inventory)

Every row is a decision a user can feel. The **status** says what has to
happen before the C++ engine may be deleted (task 2.7).

| decision | where | pure? | status |
|---|---|---|---|
| the IPC payload validators (`set_video`, `set_monitor`, `set_display_video`) | `engine/src/ipc_marshal.hpp` | **yes** (JSON only) | **TO FIXTURE** (Phase 3 prep) - the codec is frozen, but the *validation* of a payload is not |
| the battery saver decision (the reading -> the action: none / cap fps / freeze) | `engine/src/power.hpp` | **yes** (the reading is a parameter) | **TO FIXTURE** (Phase 3 prep) |
| the fps-cap mapping (the config -> the mpv option) | inline in `engine/src/engine_app.cpp` | **yes** (a mapping) | **TO FIXTURE** - it is inline today, so it must be extracted first |
| the GPU adapter selection (the config -> the VO init) | inline in `engine/src/engine_app.cpp` | **yes** (a mapping) | **TO FIXTURE** - same, and it only applies at boot (a flip needs a restart) |
| the working-set trim threshold | `engine/src/working_set_trim.hpp` | **partly** (the threshold is pure, the call is not) | **TO FIXTURE** (the threshold) |
| the occlusion poke schedule | `engine/src/occlusion_poke_scheduler.hpp` | **partly** | **TO FIXTURE** (the schedule) |
| the autostart command + the path rules | `shared/autostart.hpp` | **partly** (the command string is pure, the registry is not) | **TO FIXTURE** (the command string) |
| the legacy lock-screen cleanup decision | `shared/legacy_lockscreen_policy.hpp` | **partly** (the paths and the outcome are pure, the deletion is not) | **TO FIXTURE** (the decision) |
| the ffmpeg path resolution (the candidate list) | `shared/ffmpeg_path.hpp` | **partly** (the list is pure, the probing is not) | **TO FIXTURE** (the candidate order) |
| the cache directory choice | `shared/cache_dir_policy.hpp` | **yes** (a path) | **TO FIXTURE** |
| the video path rules | `shared/video_paths.hpp` | **yes** (a path) | **TO FIXTURE** |
| the playlist controller's pause interaction | `engine/src/playlist_controller.hpp` | **yes** | **TO FIXTURE** (the pure `SelectNextIndex` is already frozen) |
| the fullscreen classification (is this window fullscreen?) | `engine/src/fullscreen_watch.hpp` | **no** (Win32) | **LIVE TEST in Phase 3 - UNPROVEN** |
| the occlusion classification (is the surface covered?) | `engine/src/occlusion_watch.hpp` | **no** (Win32) | **LIVE TEST in Phase 3 - UNPROVEN** |
| the device-lost watch | `engine/src/device_lost_watch.hpp` | **no** (Win32) | **LIVE TEST in Phase 3 - UNPROVEN** |
| the engine watchdog | `engine/src/engine_watchdog.hpp` | **no** (the clock + a process) | **LIVE TEST in Phase 3 - UNPROVEN** |
| the launcher's lock-screen policy (the WinRT still + the lock-screen exit) | `launcher/` | **no** (WinRT + the shell) | **LIVE TEST in Phase 3 - UNPROVEN** |
| the compressor's codec choice (re-encode or copy) | `compressor/src/` | **partly** | the byte-budget decision is frozen in `k6wp-core` (`sps.rs`); the C++ probe still spawns ffmpeg - **the Rust probe is the reference** (the 9/9 `trace_headers` match) |
| the encode preference (the codec, the crf, the preset) | `compressor/src/compress_args.*` | **yes** | **frozen** (the Rust port + the golden) |

## 4. The rules for the remaining work

1. Every "TO FIXTURE" row gets a module in `engine_logic_cases.hpp` (or in
   the shared fixture) **before** the C++ is deleted - the same machinery,
   the same differential test.
2. Every "LIVE TEST" row gets a Phase 3 test that measures the behaviour on
   the running engine, and it is labelled **UNPROVEN** until it has run.
3. A new module in the fixture must state its provenance: `baseline` (the
   differential proves it) or a task label (a deliberate change). The
   fixture's `provenance` map holds that statement, and the differential
   script refuses to compare a module it does not know.

## 5. How to re-run the proof

```powershell
# the C++ side checks the fixture against the current code
ctest --preset msvc-dev -R engine_logic_test

# the Rust side replays the same fixture
cd rust; cargo test -p k6wp-core --test engine_logic_golden

# the differential against the baseline build
powershell -File tools/g5_diff_baseline.ps1
```
