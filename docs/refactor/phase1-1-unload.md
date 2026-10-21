# Phase 1.1 — unload the decode pipeline after a long pause

Date: 2026-10-21
Branch: `refactor/rust`
Label: **live-proven (one machine)** — every number below was measured on
this host with the real engine over the real IPC path.
Evidence: `docs/bench/unload.json`, `docs/bench/unload-scenarios.json`.

## The problem

A paused wallpaper held the whole decode pipeline. Measured (1080p, the
Phase 0 baseline):

| | private bytes | GPU shared | engine threads |
|---|---|---|---|
| playing | 162.3 MB | 151.6 MB | 37 |
| **paused 30 s** | **162.3 MB** | **151.6 MB** | 37 |
| idle (no video) | 23.8 MB | 0 MB | — |

The pause freed nothing. A user who leaves a fullscreen game running (or
locks the machine, or lets the screen off) still has ~314 MB of RAM and
~152 MB of video memory held by a wallpaper nobody is looking at.

## The design

```
Active  ──pause owner appears──►  Paused  ──deadline (30 s)──►  Unloaded
   ▲                                  │                            │
   └──────────── pause owner clears ──┴────────── resume ──────────┘
```

- **Active** — a video is loaded and playing.
- **Paused** — a pause owner holds the mask; the file is still loaded.
- **Unloaded** — the pause lasted longer than `pause_unload_after_sec`; the
  file is released, so the decoder and its GPU surfaces are gone.

The IPC command values are **UNCHANGED**. The state is reported as a new
**additive** `get_state` key:

```json
{"decode_state": "active" | "paused" | "unloaded"}
```

`DecodeState` lives in `EngineApp`; the transition is driven from
`ApplyPauseState` (the single place the pause mask changes) and the deadline
check runs in the message loop.

### Only the owners that hide the wallpaper unload

`mpv`'s `"stop"` releases the decoder **and clears the VO surface**, so an
unload while the wallpaper is on screen would black it out. The unload
therefore only runs for the owners that make the wallpaper **invisible
anyway** (`EngineApp::PauseHidesWallpaper`):

| owner | wallpaper visible? | unload? |
|---|---|---|
| `kUser` (an explicit pause) | yes | **no** — the frozen picture stays |
| `kFullscreen` (a fullscreen app) | no | yes |
| `kScreenOff` (the monitor is off) | no | yes |
| `kSessionLock` (the session is locked) | no | yes |
| `kSuspend` (the machine sleeps) | no | yes |
| `kPower` (the DC static mode) | yes | **no** |

An explicit user pause is the one case where the user is looking at the
wallpaper, so the picture stays exactly as it is today. The control case is
measured: after a 3 s user pause the surface still carries the picture
(mean luma **127.97**, the video's own content).

## The measurement (C4: private bytes + GPU shared)

Real engine, isolated `%LOCALAPPDATA%`, a REAL borderless fullscreen window
(the engine's fullscreen watcher detects it exactly as it would a game), the
real IPC path:

| phase | private bytes | GPU shared | threads |
|---|---|---|---|
| playing | 163.0 MB | 151.6 MB | 37 |
| **unloaded** | **69.0 MB** | **0 MB** | **28** |
| resumed | 141.0 MB | 151.6 MB | 37 |

- **private bytes: −94 MB (−57 %)**
- **GPU shared: −152 MB (−100 %)**
- The resumed figure is slightly *below* the original 163 MB (the
  working-set trim), and the thread count returns to 37 — **no leak** across
  the cycle.

## The required scenario tests

`tools/bench_unload_scenarios.ps1` drives the real engine over the real IPC
path. Results:

| scenario | result |
|---|---|
| **resume latency (n=5)** | **median 840 ms** (min 766, max 1056); the animation is back in every cycle |
| **no black frame** | **5 black frames (one per resume)** — see the cost below |
| **the video changes while Unloaded** | acked; the new path is RECORDED and loaded at the resume; the state stays `unloaded`; no new renderer is created |
| **quit while Unloaded** | acked, exited in **141 ms**, exit code 0 — no teardown hang |
| **the user pause is not unloaded** | state `paused`, the surface still carries the picture (luma 127.97) |
| **a fullscreen app in and out** | unload on the way in, reload on the way out, 5 cycles in a row |

## The cost: a black surface during the resume — SOLVED (D2)

**The first implementation had a real, measured cost:** mpv's `"stop"` clears
the VO surface, so between the unload and the first decoded frame the
wallpaper surface was **black** (the measured resume latency was 840 ms).

**D2 removed it.** The mechanism:

1. **Hide mpv's VO child window inside OUR OWN surface** with
   `SetWindowPos(child, nullptr, 0, 0, 0, 0, SWP_HIDEWINDOW | SWP_NOMOVE |
   SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_ASYNCWINDOWPOS)`.
   The injected surface is a `WS_EX_LAYERED` window: where it paints nothing
   it is **transparent**, so the shell's own wallpaper (the per-user still —
   the video's first frame) shows through it instead of a black rectangle.
   The child belongs to **this** process and the call is **asynchronous**, so
   no window-position message is delivered synchronously and **nothing can
   block on Explorer's UI thread**. The shell's own windows are never
   touched.
2. **Destroy the whole mpv instance**, not just its file: the mpv handle
   AND the surface window go to the renderer graveyard (destroyed on the
   background worker, never on the message loop).
3. **The resume** creates a fresh surface + a fresh renderer (the same
   machinery the re-anchor uses) and re-issues the video. The new surface is
   empty until the first frame arrives — but the shell's wallpaper is
   underneath the whole time, so **there is no black frame there either**.

### The D2 gate (n=5, `tools/bench_unload_d2.ps1`)

| gate | result |
|---|---|
| **no black frame** (the screen sampled across a REAL fullscreen exit, 170 samples) | **0 black samples**, min luma 67.4 |
| **the static wallpaper underneath is visible while Unloaded** | **the injected surface is GONE** (0 surfaces) → the shell's wallpaper shows |
| **no Explorer UI hang** (a `WM_NULL` with a 2 s timeout to the taskbar = Explorer's UI thread) | responsive before, during and after every run |
| **the animation is back after the resume** | yes, every run |
| **an explicit user pause still shows the picture** | yes (luma 78.4) |

The resume latency: **median ~835 ms** (n=5: 772–1155 ms) — unchanged by
the D2 mechanism.

### The memory, after the full destroy

| phase | private bytes | GPU shared | threads |
|---|---|---|---|
| playing | 161.7 MB | 151.6 MB | 37 |
| **unloaded** | **60.1 MB** | **0 MB** | **19** |
| resumed | ~139 MB | 151.6 MB | 38 |

**private −102 MB (−63 %)** — better than the 69 MB residue of the
mpv-`stop` variant, because the whole instance goes instead of only its
file. The remaining 60 MB is the engine itself (24 MB at idle) plus the
**idle headless mpv instance** the engine keeps as its fallback (~36 MB) —
see the follow-up note below.

### The trade-offs D2 introduces (recorded honestly)

- **The video restarts from its first frame** after the resume: the renderer
  is new. That is consistent with what is shown underneath (the per-user
  still IS the video's first frame), so the transition reads as "the
  wallpaper starts over" rather than a glitch. It is still a behaviour
  change against the frozen-frame behaviour of the pause.
- **The still underneath is the video's FIRST frame**, so a video that was
  paused at frame N shows frame 0 while Unloaded. (The still is refreshed
  by the product's own still machinery on a video change.)
- **The idle headless mpv instance (~36 MB) is still held** while Unloaded.
  Destroying it would need the exclusive-ownership handoff the graveyard
  provides, and the headless renderer is shared (`shared_ptr`) — a
  follow-up, not a D2 change.

## Two alternatives that were measured and REJECTED

Both were implemented, measured, and removed again — the numbers are why.

1. **Loading a still IMAGE in the video's place** (capture the frame with
   mpv's screenshot, then `loadfile` the JPEG so the picture stays):
   the image decoder kept the VIDEO's resources alive — the private bytes
   only fell to **164.5 MB** instead of 69 MB — and the resume jumped to
   **300 MB**. It did not free the memory, which is the whole point.
2. **Hiding the injected SURFACE with `ShowWindow`** so the shell's own
   wallpaper shows through: `ShowWindow` on a window parented to the
   shell's WorkerW blocks on Explorer's UI thread and **HUNG the engine**
   (the IPC stopped answering). The D2 mechanism replaces it with an
   **asynchronous** `SetWindowPos` on **our own child window**, which
   cannot block.

## Configuration (schema v6)

New key in `config.json`:

```json
"pause_unload_after_sec": 30
```

- Range **0..3600** (0 = never unload), default **30**.
- **Schema v5 → v6**: a v5 file migrates with the default, is backed up
  byte-for-byte to `<config>.bak` and rewritten at the current schema — the
  `.bak` self-heal contract is unchanged (`tests/config_test.cpp` cases
  47-50, the golden fixtures regenerated).
- The field is also settable per run with the hidden QA flag
  `--unload-after-pause-ms <N>`, which drives the SAME code path (so the
  measurement exercises the real mechanism, not a copy of it).

## The wakeup cost of the deadline

The loop parks INFINITE while paused (the zero-wakeup design) and a
`WM_TIMER` cannot wake it (`MsgWaitForMultipleObjects` with `QS_ALLINPUT`
does not include timer messages). The wait is therefore bounded by the time
left until the deadline: **one wakeup per pause episode**, not a periodic
one. The measured wakeup floor while paused is unchanged (33/s, Phase 0).

## Rust port spec (Phase 3)

The Phase 3 engine must implement the same contract:

1. The state machine `Active → Paused → Unloaded` with the same
   transitions, driven by the same pause-owner mask.
2. The unload releases the decoder AND its GPU surfaces. In Rust that means
   dropping the `mpv` handle (or the `libmpv` decoder) — the
   `mpv` instance, the VO and the window may survive, but the FILE must go.
3. The unload only runs for the owners that make the wallpaper invisible.
4. `get_state` gains the additive `decode_state` key. The IPC command values
   are unchanged.
5. `pause_unload_after_sec` in `config.json` (schema v6), range 0..3600,
   default 30, with the same migration and `.bak` rules.

**Open for the port**: the black-surface window during the resume. A Rust
engine that keeps the last frame (e.g. by rendering into a texture it owns
instead of relying on mpv's VO) would close it; the C++ engine cannot,
because `mpv`'s `"stop"` owns the surface.
