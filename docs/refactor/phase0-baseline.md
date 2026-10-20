# Phase 0 measurement baseline (task 0.5)

Date: 2026-10-21
Revision: `9923cc0` (tag `baseline-pre-refactor`), branch `refactor/rust`
Label: **live-proven** — every number below was observed on this machine by
the harness in `tools/`, unless the row says otherwise.

## Host

| Field | Value |
|---|---|
| OS | Windows 11 Home 10.0.26200 build 26200 |
| CPU | 12th Gen Intel Core i7-12650H, 10 cores / 16 logical |
| RAM | 16,457,592 KiB visible (~15.7 GiB) |
| GPU | Intel UHD Graphics (iGPU) + NVIDIA RTX 2050 (dGPU) + Virtual Display Driver |
| Display | 1 active monitor, 1920x1080 @ 144 Hz, 100% scale, stable key `ccd:\?display#cmn1521...` |
| Power plan | Balanced |
| Toolchain | CMake 4.4.3, MSVC (VS 18 2026 generator), Qt 6.8.3, Rust 1.98.0 |

## Method

Real `engine.exe` / `studio.exe` processes, never instrumented binaries.

- **Isolation** — every run gets its own `%LOCALAPPDATA%` (temp dir), so the
  user's real config, settings and library are never touched.
- **Occlusion** — all top-level windows are minimized before sampling and
  restored afterwards. The engine pauses playback when a window covers the
  desktop (`occlusion: slot 0 coverage=100.0% pausing`); without this, every
  GPU number reads 0 %.
- **Sampling** — `.NET` process counters (private bytes, working set, CPU
  delta, threads, handles) plus `typeperf` CSV for GPU utilization per engine
  type and thread context-switch rate.
- **n ≥ 5** per state; the reported band is min..max across the 5 per-run
  medians. The first `typeperf` sample is discarded (the counter is not yet
  warm — it reads blank/0).

## Engine playback baseline

`docs/bench/phase0.json` (aggregate) and `docs/bench/phase0.raw.json`
(per-run, 20 runs total).

| State | private MB | working set MB | CPU % | GPU total % | GPU decode % | GPU 3D % | GPU shared MB | wakeups/s |
|---|---|---|---|---|---|---|---|---|
| idle (no video) | **23.84** | 2.24 | 0.00 | – | – | – | 0 | **39.8** |
| playback 1080p | **162.48** | 16.57 | 4.63 | 22.05 | 9.58 | 12.38 | 151.57 | **1273** |
| playback 4K (16 ref) | **387.89** | 77.09 | 4.82 | 58.50 | 38.00 | 20.23 | 468.43 | **1294** |
| paused 30 s | 162.35 | 17.55 | 0.00 | 0.00 | 0.00 | 0.00 | 151.57 | **30.1** |

Noise bands (min..max across the 5 per-run medians):

| State | private | CPU | GPU total | GPU decode | wakeups/s |
|---|---|---|---|---|---|
| idle | 23.58 – 24.13 (2.3 %) | 0 | – | – | 31.9 – 47.1 (40 %) |
| playback 1080p | 156.86 – 163.39 (4.0 %) | 4.27 – 5.13 (18 %) | 19.85 – 23.64 (18 %) | 8.66 – 10.62 (21 %) | 1200 – 1335 (11 %) |
| playback 4K | 387.08 – 388.02 (0.2 %) | 4.24 – 5.30 (24 %) | 53.80 – 63.40 (17 %) | 35.11 – 40.66 (15 %) | 1233 – 1358 (10 %) |
| paused 30 s | 156.70 – 162.69 (3.7 %) | 0 | 0 | 0 | 28.5 – 31.7 (11 %) |

### Reading these numbers honestly

1. **Working set is a vanity metric here.** The engine trims its own working
   set ~2 s after start (`engine/src/working_set_trim.cpp:50`,
   `SetProcessWorkingSetSize`), which is why the working set reads 2 MB while
   private bytes read 162 MB. **Private bytes + GPU shared memory is the
   honest RAM number**; the working set is reported for completeness only.
2. **Paused still holds the decoder.** A paused 1080p session keeps 162 MB
   private and 151 MB GPU shared. The video is not unloaded — that is the
   target of Phase 1.1 (unload mpv after a long pause).
3. **CPU is flat between 1080p and 4K** (4.63 % vs 4.82 %). The decode cost
   lands on the GPU, not the CPU — so a 4K/16-reference clip is a *GPU memory*
   problem (468 MB shared), not a CPU problem.
4. **Wakeups are the real idle cost.** 30–40/s idle and paused versus
   1273/s during 1080p playback. The paused figure is already near the
   event-driven floor; the playback figure is mpv's render loop, not the
   engine's own polling.
5. **hwdec is active**: `hwdec-current=d3d11va` during both playback states
   (iGPU), `no (INACTIVE-software)` when idle.

## Studio baseline

`docs/bench/studio.json`, n = 3, real `studio.exe`, isolated `%LOCALAPPDATA%`.

| Metric | Value |
|---|---|
| Startup to first visible window | **1226 / 1290 ms** (max 1558 ms) |
| Idle private bytes (median) | **337.53 MB** |
| Idle working set | 17.6 MB (trimmed, same caveat as the engine) |
| Idle CPU | **0.01 %** |
| Threads / handles (last sample) | 55 / 1564 |

Startup is measured as process start → first visible top-level window, which
is what a user perceives as "the app opened". There is no stdout marker for
the Studio (unlike the engine's `Engine:FirstFrame`).

## Bundle baseline

`docs/bench/bundle.json`. The staging set mirrors `packaging/installer.nsi`:
libmpv and the VC runtime come from `vendor/`, the Qt payload from the
release build tree.

| Group | MB | % of install |
|---|---|---|
| **ffmpeg** (ffmpeg.exe + ffprobe.exe) | **194.16** | **49.9 %** |
| **mpv** (libmpv-2.dll) | **114.77** | **29.5 %** |
| qt (Qt6*.dll) | 48.00 | 12.3 % |
| qml payload | 8.14 | 2.1 % |
| k6wp exes (K6WP/engine/studio/compressor) | 5.44 | 1.4 % |
| shader cache (dxcompiler + dxil) | 15.09 | 3.9 % |
| msvc runtime | 0.83 | 0.2 % |
| **Total** | **389.14** | 100 % |

1,306 files. The two dominant levers are **ffmpeg (194 MB)** and
**libmpv (115 MB)** — together 79 % of the install. Qt is only 12 %, so
"removing Qt" alone is not a bundle win; it is a Studio-RAM and startup win.

Shipped ZIP for reference: `dist/K6WP-portable-0.0.1-alpha.3.zip` = 155.60 MB
(compressed).

## Resume / suspend baseline

`docs/bench/resume.json`, n = 3. The engine's own
`--simulate-suspend-after-ms` posts the **real** `PBT_APMSUSPEND` /
`PBT_APMRESUMEAUTOMATIC` path.

| Metric | Value |
|---|---|
| Control-path latency (engine log timestamps) | 1998 – 2011 ms |
| Visual change after resume (proxy) | 1114 / 1331 / 1657 ms |
| `paused` after resume | false (playback resumed correctly) |

**Caveat (important):** the control-path number is ~2000 ms because the
simulator posts the resume exactly 2 s after the suspend — it measures the
*simulator*, not the engine. The honest reading is the visual number: after a
resume the wallpaper is visibly moving again within **1.1–1.7 s**. A real
machine sleep is not reproduced by this test and stays **UNPROVEN**.

## Visual parity baseline

`docs/bench/visual/baseline/` (4 full-screen PNGs + `signature.json`).

| Metric | Value |
|---|---|
| mean luma | 128.48 (median) |
| stddev | 66.69 |
| Frame-to-frame motion diff | 84.87 |

The gate is **proven to fail when it should**: `tools/verify_visual_negative.ps1`
captures the desktop with no engine running and the comparator rejects it
(mean_luma delta 59.5, tolerance 12). A gate that never fails is not a gate.

## Corpus

Generated locally by ffmpeg lavfi, **not committed** (repo policy: no binary
corpus in git).

| File | Properties |
|---|---|
| `tests/corpus/anime.mp4` | 1920x1080, 30 fps, 30 s, H.264 (testsrc2 — high temporal complexity) |
| `tests/corpus/gaming.mp4` | 1920x1080, 30 fps, 30 s, H.264 (SMPTE bars + noise) |
| `tests/corpus/slideshow.mp4` | 1920x1080, 5 fps, 30 s, H.264 (3 solid slides — low complexity) |
| `tests/corpus/highdpb_1080p.mp4` | 1920x1080, 16 reference frames |
| `tests/corpus/highdpb_4k.mp4` | 3840x2160, 30 fps, 12 s, 16 reference frames |

The 4K/16-ref clip is what makes the 387 MB / 468 MB GPU-shared numbers
visible; a low-DPB 4K clip would hide the decode-pool cost.

## Golden behaviour fixtures (task 0.6)

`tests/golden/shared_behavior.json` — 92 cases, generated by
`tests/golden_dump.cpp` and enforced by the `golden_test` ctest.

| Function under test | Cases |
|---|---|
| `MigrateConfig` | 28 |
| `MigrateStudioSettings` | 18 |
| `MigratePlaylist` | 8 |
| `MigrateDisplays` | 6 |
| `Decode` (accept/reject table) | 14 |
| `Encode` (exact wire bytes per command) | 7 |
| `ParseEngineState` (old-engine compat) | 5 |
| `CmdToString` / `CmdFromString` | 1 |
| `SelectNextIndex` / `PlaylistIndexForPath` / `NormalizeOrder` | 3 |
| defaults (`WallpaperConfig`, `StudioSettings`) | 2 |

Each case records the **exact input** and the **exact output** (or the exact
error message), so the Rust port in Phase 2/3 replays the same document and
diffs against it. `golden_test` fails the build when the C++ behaviour drifts
without an intentional golden update.

## Not measured on this host (UNPROVEN)

| Item | Why |
|---|---|
| Multi-monitor, mixed-DPI, portrait, HDR | 1 physical monitor only |
| AMD/Intel-Arc hardware decode | no such GPU here |
| Battery / DC policy | no battery in this rig |
| Real sleep → wake duration | only the in-process power broadcast is simulated |
| Hardware-accelerated encode | the compressor path was not re-measured here |

These stay **UNPROVEN** rather than estimated. A Windows 10 22H2 / 11 x64
minimum-OS claim is likewise **UNPROVEN** on this host (Win11 26200 only);
the engine's own minimum-OS check was found to be **absent** from the source
(see `docs/refactor/contract-inventory.md`).
