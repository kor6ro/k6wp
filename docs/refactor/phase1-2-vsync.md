# Phase 1.2 — the `display-desync` busy-wait (VERDICT: ACCEPT)

Date: 2026-10-21
Branch: `refactor/rust`
Baseline: `build/ab/engine-baseline.exe` (desync) vs `build/ab/engine-candidate.exe`
(vsync) — both built from the same tree by `tools/build_ab.ps1`, which
**verifies that the two binaries differ** (see "The measurement trap" below).
Evidence: `docs/bench/ab-vsync.json`, `docs/bench/ab-vsync-4k.json`,
`docs/bench/frames/frames.json`.

## The mpv options, before and after (C1)

Every option the engine sets on the mpv instance, verbatim. Only ONE line
changed; everything else is identical in both builds, so the comparison is
attributable.

| mpv option | baseline (`display-desync`) | candidate (`display-vsync`) |
|---|---|---|
| `wid` | the injected HWND as a decimal string | same |
| `vo` | `gpu-next,gpu` | same |
| `hwdec` | `d3d11va` | same |
| `d3d11-adapter` | the pinned adapter (empty = default) | same |
| `audio` | `no` | same |
| `loop-file` | `inf` | same |
| `keep-open` | `yes` | same |
| `idle` | `yes` | same |
| `demuxer-max-bytes` | `16MiB` | same |
| `demuxer-max-back-bytes` | `4MiB` | same |
| `ad-queue-enable` / `ad-queue-max-bytes` / `ad-queue-max-samples` / `ad-queue-max-secs` | `no` / `1M` / `4` / `1` | same |
| `vd-lavc-threads` | `4` | same |
| `scale` / `cscale` / `dscale` | `bilinear` | same |
| `dither` | `no` | same |
| `interpolation` | `no` | same |
| `deinterlace` | `no` | same |
| `framedrop` | `vo` | same |
| **`video-sync`** | **`display-desync`** | **`display-vsync`** ← THE ONLY CHANGE |
| `gpu-api` | `d3d11` | same |
| `d3d11-flip` | `no` | same |
| `swapchain-depth` | `2` | same |
| `osc` | `no` | same |
| `sub-auto` | `no` | same |
| `audio-file-auto` | `no` | same |
| `hwdec-codecs` | `all` | same |
| `gpu-shader-cache-dir` | `%LOCALAPPDATA%\K6WP\shadercache` | same |
| `osd-level` | `0` | same |
| `terminal` | `no` | same |
| `load-scripts` | `no` | same |
| `ytdl` | `no` | same |
| `input-default-bindings` / `input-vo-keyboard` / `input-cursor` | `no` | same |

Forbidden options (`hwdec-extra-frames`, `vd-queue-enable=yes`) are **not**
set in either build, and the decode stays on the iGPU in both.

**C2 note (for the Rust port):** port the set ABOVE — the measured one — and
specifically `video-sync=display-vsync`. Do **not** copy `display-desync`
because it is what the pre-refactor code had: it is the option that caused
the busy-wait and the slow-motion playback.

## C1 — the quality proof

The change was accepted on CPU/GPU numbers. That is not enough: a cheaper
CPU can also mean dropped or late frames, or a wallpaper that runs in slow
motion. So the pacing was measured with the **live mpv counters** over
**60 s per mode**, at the **fps cap 24 and 30**, at the **display's 144 Hz**
and at **60 Hz** (`tools/bench_pacing.ps1`, ABAB-paired inside one session).

The counters (discovered with `--dump-mpv-properties`: `vo-drop-frame-count`
does **not** exist in mpv 0.41, the VO drop counter is `frame-drop-count`):

| counter | meaning |
|---|---|
| `frame-drop-count` | frames the VO dropped |
| `decoder-frame-drop-count` | frames the decoder dropped |
| `mistimed-frame-count` | frames presented at the wrong time |
| `vo-delayed-frame-count` | frames presented late |
| `estimated-frame-number` | frame index consumed (delta = frames decoded) |
| `estimated-vf-fps`, `playback-time`, `vf` | the rate, the media clock, the active filter chain |

### 144 Hz (the display's native rate), 60 s per run

| fps cap | metric | desync | vsync |
|---|---|---|---|
| 24 | max engine thread CPU | 55.3 % of a core | **10.2 %** |
| 24 | process CPU | 4.0 % | **1.1 %** |
| 24 | GPU total | 34.9 % | **22.1 %** |
| 24 | GPU decode | 10.5 % | 14.4 % |
| 24 | GPU 3D | 24.4 % | **7.8 %** |
| 24 | **frames decoded** | 1322 | **1775** |
| 24 | **decoded fps** | 21.8 | **29.4** |
| 24 | **playback rate (media clock ÷ wall clock)** | **0.73** | **0.98** |
| 24 | **VO frame drops** | 0 | 0 |
| 24 | **decoder frame drops** | 0 | 0 |
| 24 | **VO delayed frames** | 184 | **0** |
| 30 | max engine thread CPU | 56.9 % | **11.3 %** |
| 30 | **playback rate** | **0.72** | **0.98** |
| 30 | **VO delayed frames** | 195 | **0** |
| 30 | **VO / decoder frame drops** | 0 / 0 | 0 / 0 |

### 60 Hz, 60 s per run

| fps cap | metric | desync | vsync |
|---|---|---|---|
| 24 | max engine thread CPU | 54.7 % | **7.1 %** |
| 24 | **playback rate** | **0.73** | **0.98** |
| 24 | **decoded fps** | 22.1 | 29.5 |
| 24 | **VO delayed frames** | 197 | **0** |
| 30 | max engine thread CPU | 58.3 % | **13.4 %** |
| 30 | **playback rate** | **0.70** | **0.98** |
| 30 | **VO delayed frames** | 193 | **0** |
| 30 | **VO / decoder frame drops** | 0 / 0 | 0 / 0 |

### What the counters say

1. **The candidate drops NOTHING.** `frame-drop-count` = 0 and
   `decoder-frame-drop-count` = 0 in every run of both builds.
2. **The candidate delivers every frame ON TIME.** `vo-delayed-frame-count` =
   0 in every candidate run, against 184–197 in the desync runs.
3. **The candidate plays the video at the RIGHT SPEED; the baseline did
   not.** The media clock advanced 0.98 × wall-clock in the candidate and
   **0.70–0.75 ×** in the baseline. The baseline was playing the wallpaper
   **a quarter slow** — the desync busy-wait was not merely burning a core,
   it was distorting the playback rate. That is a visible quality defect
   (a slow-motion wallpaper) that the CPU/GPU numbers alone would never
   have revealed.
4. **Therefore the GPU decode increase is NOT extra work for the same
   result.** The candidate decodes 29.3–29.5 frames/s — the source rate of
   the 30 fps clip — while the baseline decoded only 21.5–22.4 frames/s
   *because it was running slow*. The +35 % decode utilization is the
   candidate doing the work the video actually contains. The frame-drop
   counters prove no frames were skipped to achieve it: both builds dropped
   **zero** frames, so the decoder delivered every frame it decoded.

### Verdict

**ACCEPT — the change is a quality improvement, not a trade-off.** No
regression on any quality counter, and the baseline's slow-motion defect is
fixed. Label: **live-proven (one machine)** — the numbers come from this
host, this clip, this display.

### Still UNPROVEN

- Multi-monitor, mixed-DPI, and hardware other than this iGPU+dGPU pair
  (single monitor, one rig).
- A 30/50/59.94 Hz display (only 60 Hz and 144 Hz exist here).
- The behaviour when the display's vblank is unavailable (e.g. a
  virtual display driver): the VO would block in the present. The VO
  keeping the GPU busy (the decode work is sustained across every sample)
  is evidence it does not stall here, but it is not proof on other hardware.

`engine/src/mpv_renderer.cpp` set `video-sync=display-desync` (a P1.1 change
from `display-vsync`). mpv's *desync* mode has no vblank to sleep on, so it
**busy-waits** for the frame deadline in its VO thread.

Measured on this host (1080p playback, the engine's own threads):

| | baseline (desync) | candidate (vsync) |
|---|---|---|
| hottest engine thread | **66 % of one core, state `Running`** | 9.7 %, state `Wait` |
| process CPU (all 16 threads) | 3.86 % | **1.06 %** |
| engine thread context switches/s | 1381 | **1103** |
| GPU 3D utilization | 25.2 % | **9.4 %** |
| GPU total utilization | 36.3 % | **23.8 %** |
| GPU decode utilization | 10.8 % | 14.5 % (the one regression) |

At 4K (16-reference clip): CPU **4.13 % → 1.45 %**, GPU 3D **31.6 % → 17.6 %**,
GPU total unchanged (70.6 % → 75.2 %, inside the 11.6 % noise band), working
set **72.6 → 50.7 MB**, private bytes unchanged, GPU shared unchanged.

The busy-wait was invisible in the aggregate: the engine's *total* CPU looked
like a modest 4 %, and the process never looked "hot" in Task Manager. It was
one thread spinning to hit a frame deadline that nothing required.

## Why `display-vsync` is the right fix (not a guess)

`display-vsync` blocks in the present call until the display's vblank, which
is exactly what a wallpaper wants: one frame per refresh, no tearing, and the
CPU sleeps between frames. The engine renders into a layered window without a
redirection surface (`d3d11-flip=no` is deliberate, see the comment in
`mpv_renderer.cpp`), so the present path is the blit path — and blocking in
DWM's present is well-defined there.

**Proof that the VO keeps presenting** (the failure mode a vblank wait on a
window without a redirection surface would cause is a frozen wallpaper):
mpv's decoder is pull-driven by the VO, so a stalled present starves the
decode. The candidate's decode work is **sustained across the whole
playback window** (14.5 % at 1080p, 57.6 % at 4K, every one-second sample),
not a spike at the start. A stalled VO would read ~0 %.

## Render parity (the acceptance bar)

`tools/bench_frames.ps1` compares the frames the engine **renders** — not
what the desktop composite shows — using the hidden `--capture-frames` QA
flag (mpv's own decoded-frame screenshot, at a deterministic seek position
with playback paused so the frame cannot drift).

```
frame size      baseline_luma candidate_luma luma_delta baseline_rgb         candidate_rgb        pixel_diff pass
    1 1920x1080        128.19         128.19          0 125.11,129.36,125.69 125.11,129.36,125.69          0 True
    2 1920x1080        128.19         128.19          0 125.11,129.36,125.69 125.11,129.36,125.69          0 True
    3 1920x1080        128.19         128.19          0 125.11,129.36,125.69 125.11,129.36,125.69          0 True
```

**Byte-identical rendered frames** (pixel diff 0.000, identical mean luma and
per-channel RGB). The change touches presentation *timing*, not the decode,
scaler or colour path.

### The measurement trap that made the first comparison void

The first frame comparison reported "pixel diff 0.000 — PASS" **while the two
binaries were byte-identical**: `Copy-Item` preserves the source file's
mtime, so restoring `mpv_renderer.cpp` from a `.bak` left it older than its
object file, MSBuild skipped the recompile, and the "new" build was the old
binary. The comparison had compared a build with itself.

`tools/build_ab.ps1` now exists to make that impossible: it patches the
source, **touches** it (so MSBuild really recompiles), and **verifies the two
binaries differ by SHA-256** before the A/B is allowed to run. It aborts
otherwise.

## The one regression, and why it is accepted

GPU **decode** utilization rises: +35 % at 1080p, +44 % at 4K (both well
outside the measured noise). GPU *total* falls 34 % at 1080p and is flat at
4K.

The decode work is the same work — it moves from the CPU's spin-wait into the
GPU's pipeline instead of being throttled by a stalled CPU-side deadline. The
protected metric is **total** GPU utilization, and it improves (or stays flat
inside the noise). CPU drops 65–73 %. Accepting a +44 % decode figure to buy a
−65 % CPU figure would be the wrong trade for a background process that is
supposed to cost the user nothing.

## What is NOT proven

- **A screen-level "the picture on the desktop moves" check is impossible in
  this session.** The agent harness's own always-on-top cursor overlay covers
  the whole screen, and the engine correctly treats a fully covered desktop
  as occluded and pauses the wallpaper (the occlusion watcher). A paused
  engine holds a still frame, so a desktop grab records the plain desktop.
  The check was therefore made against the **injected wallpaper window
  itself** (PrintWindow on the `K6WP.DesktopInject.1` surface), which is
  what the engine actually presents:
  `tools/check_live_animation.ps1` → frame-to-frame diff **18.687** — the
  live wallpaper **is animating** with the vsync build.
- Multi-monitor, mixed-DPI, and hardware other than this iGPU+dGPU pair are
  **UNPROVEN** (single monitor, one rig).

## Changes

- `engine/src/mpv_renderer.cpp` — `video-sync` = `display-vsync`; the log line
  now reports the mode that is actually requested.
- `engine/src/mpv_renderer.hpp/.cpp` — `MpvRenderer::SeekTo(double)` (QA
  support for the deterministic frame capture).
- `engine/src/multi_monitor.hpp/.cpp` — `MultiMonitor::SeekAllTo(double)`.
- `engine/src/cli_options.hpp/.cpp` — hidden QA flags `--capture-frames
  <prefix>`, `--capture-frames-count <n>`, `--capture-frames-interval-ms
  <n>`, `--capture-frames-seek <sec>`.
- `engine/src/engine_app.hpp/.cpp` — the capture step, the QA-only
  `CaptureFramesPending()` wait bound, and the shutdown cleanup.
- `engine/src/timer_ids.hpp` — (no new timer id was needed: a `WM_TIMER`
  cannot wake the parked loop, see the note in the code).

## Harness added

| Tool | Purpose |
|---|---|
| `tools/build_ab.ps1` | builds a verified-different baseline/candidate pair |
| `tools/bench_ab.ps1` | paired A/B over the Phase 0 metrics, noise-aware |
| `tools/bench_frames.ps1` | engine-side rendered-frame parity (the gate) |
| `tools/bench_wallpaper_frames.ps1` | captures the injected wallpaper window itself (PrintWindow) |
| `tools/check_live_animation.ps1` | one-shot "is the live wallpaper moving?" check against the running engine |
| `tools/bench_vo_sustained.ps1` | proves the VO keeps presenting (sustained GPU work) |
| `tools/diag_wakeups3.ps1` | per-thread CPU/context-switch attribution |
| `tools/list_windows.ps1` | enumerates the visible top-level windows |

## Also fixed: the wallpaper WorkerW host resolution

Found while chasing the visual check. `desktop_inject.cpp` picked the shell's
wallpaper WorkerW only when it was **empty**, but a re-anchor pass resolves
the host **before** the old surface is released — so the WorkerW still held
*our own* stale injected window, the "is it empty" test failed, and the
engine fell back to the layered-Progman-child branch, which sits below the
shell's DefView and is **invisible**. The wallpaper therefore disappeared a
fraction of a second after every start.

`WorkerWAcceptsSurface()` now accepts a WorkerW whose only children are our
own injected surfaces. The log confirms the bogus fallback is gone: the
engine now performs a single injection into the wallpaper WorkerW instead of
injecting and then re-injecting into the invisible branch.
