# Phase 1.2 — the `display-desync` busy-wait (VERDICT: ACCEPT)

Date: 2026-10-21
Branch: `refactor/rust`
Baseline: `build/ab/engine-baseline.exe` (desync) vs `build/ab/engine-candidate.exe`
(vsync) — both built from the same tree by `tools/build_ab.ps1`, which
**verifies that the two binaries differ** (see "The measurement trap" below).
Evidence: `docs/bench/ab-vsync.json`, `docs/bench/ab-vsync-4k.json`,
`docs/bench/frames/frames.json`.

## The finding

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

- **The on-screen animation could not be captured in this session.** The agent
  harness's own always-on-top cursor overlay covers the whole screen, and the
  engine correctly treats a fully covered desktop as "occluded" and pauses the
  wallpaper (the occlusion watcher). A paused engine holds a still frame, so
  a screen-level "do the frames move?" check is impossible here. The engine
  was verified to be *rendering* (sustained decode work) and the rendered
  frames are identical, but "the picture on the desktop moves" stays
  **UNPROVEN in this environment** — it is not proven *broken* either.
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
