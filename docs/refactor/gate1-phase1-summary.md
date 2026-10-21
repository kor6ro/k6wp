# Gate 1 — the Phase 1 summary

Date: 2026-10-21
Branch: `refactor/rust`
Commits: `340da84` (C1+C2) → `76d1a28` (C6+C7) → `a5b2b0c` (1.3 census
+ parity) → `1633d29` (1.1 D2) → `2860d0b` (1.5) → `dca9f76` (D1/D3/D4/D6).

**Reference: baseline v2** (`docs/bench/phase1-baseline-v2.json`, commit
`340da84`) — the engine after the pacing fix, before the long-pause unload.
Baseline v1 is archived; the 1.2-vs-v1 CPU/GPU delta is **not a release
claim** (v1 ran the wallpaper at 0.70–0.75×, so the two are not
comparable).

## The verdict per task

| task | verdict | the number that decided it |
|---|---|---|
| **1.1** unload after a long pause | **ACCEPT** (D2) | private **161.7 → 60.1 MB (−63 %)**, GPU shared **150.9 → 0 MB**, threads 37 → 19, **0 black frames** in 170 screen samples across 5 real fullscreen exits, the injected surface GONE while Unloaded, Explorer responsive, the animation back, an explicit user pause still shows the picture. Resume latency median **~835 ms** |
| **1.2** the pacing fix | **ACCEPT** (C1 closed) | the desync baseline ran the wallpaper **25 % slow** (rate 0.70–0.75) and delivered 184–197 late frames per 60 s; the fixed build runs at **0.98** with **0** late frames and **0** drops in either. The +35 % GPU decode is explained: the fixed build decodes the source rate (29.3 fps) where the baseline decoded 21.8 fps *because it was slow* |
| **1.3** the bundle | **DEFER** (census + parity done, the swap not applied) | the per-file ZIP census: ffmpeg+ffprobe **70.73 MB (47.8 %)**, libmpv 44.64 (30.1 %), Qt 19.78 (13.4 %) — so **dropping Qt can never reach the old ≥ 30 % ZIP target** (max ~15 %); the target is revised to **≤ 115 MB ZIP / ≤ 292 MB install**. The ffprobe→libmpv swap has **proven parity** (0 disagreements over 5 files × 5 fields) but is **not applied**: the field 1.4 needs is not in libmpv, and the ffprobe spawn is 30 ms (not ~150 ms). **The ZIP stays 148.13 MB — the target is NOT met, recorded as-is** |
| **1.4** the DPB-pinned re-encode | **ACCEPT** | the source of `num_ref_frames` is ffmpeg's own `trace_headers` (measured: ffprobe, ffmpeg's normal output and libmpv all lack it — no hand-written SPS parser). The skip-optimal COPY path is refused above 4 refs. A/B: private **386.8 → 228.2 MB (−41 %)**, GPU shared 467.7 → 432.5 MB, rate 0.99 in both, 0 delayed, parity Δluma 0.04 / Δstddev 0.06, and the re-encoded file still animates live |
| **1.5** the span mode + the old-OS branch | **ACCEPT** | **−345 net LOC** (+69 / −414); per-monitor is the only topology; the pre-1703 DPI fallback is gone (the min-OS is enforced before anything starts) |
| **C6** the minimum OS | **ACCEPT** | `RtlGetVersion` (build ≥ 19041) in the engine, the launcher and the Studio, the manifest's supportedOS ladder, the NSIS installer's refusal, 49/49 ctest |
| **C7** the WorkerW regression test | **ACCEPT** | the host-resolution rule is a pure function with 6 cases, **verified to fail** when the rule is reverted |
| **C8** the session safety | **ACCEPT** | `tools/session_snapshot.ps1` snapshots the display mode, the wallpaper, the still state and the lock-screen backup, and restores them in a `finally`; every live-session test in Phase 1 uses it |
| **C2** the per-thread CPU gate | **ACCEPT** | the aggregate hid a 66 %-of-a-core spin; the gate is now **≤ 25 % of one core** per thread during playback (the fixed build peaks at 10.8 %) |

## The deltas against baseline v2

| metric | baseline v2 | now | delta |
|---|---|---|---|
| private bytes, paused 30 s (an invisible owner) | 161.6 MB | **60.1 MB** | **−63 %** |
| GPU shared, paused 30 s | 150.9 MB | **0 MB** | **−100 %** |
| private bytes, playback 4K/16-ref | 387.2 MB | **228.2 MB** (the DPB-pinned file) | **−41 %** |
| GPU shared, playback 4K/16-ref | 468.1 MB | **432.5 MB** | −7.5 % |
| private bytes, idle | 24.2 MB | 24.2 MB | — |
| CPU, playback 1080p | 1.74 % | 1.74 % | — (the pacing fix is already in v2) |
| max engine thread CPU, playback | 9.3–10.8 % | unchanged | — |
| engine LOC | — | — | **−345** (task 1.5) |
| install size / ZIP | 389.14 MB / 148.13 MB | **unchanged** | the bundle lever is DEFER |
| Studio RAM / startup | 337.53 MB / 1290 ms | unchanged | Phase 2 (Slint, no Qt) |

**Every number above carries its playback rate and its frame counters.** The
baseline v2 states: 1080p rate 1.00 (29.98 fps delivered, cpu/fps 0.06,
gpu/fps 0.72), 4K rate 0.98 (29.54 fps, cpu/fps 0.06, gpu/fps 2.46),
0 delayed frames in both. The 4K state recorded **one of five runs at
0.9694** — marginally under the 0.97 validity bound; recorded, not
smoothed.

## The D2 resume latency (measured, n=5)

The resume after an invisible-owner pause (the fullscreen case), measured
by `tools/bench_unload_scenarios.ps1` on the D2 build:

| cycle | 1 | 2 | 3 | 4 | 5 | **median** |
|---|---|---|---|---|---|---|
| resume latency (ms) | 1061 | 788 | 811 | 762 | 798 | **798 ms** |

(The budget is ≤ 2000 ms; the Phase 0 reference was the 1114–1657 ms
"resume → visible motion" range.)

**And no black frame**: the D2 gate sampled the screen across five REAL
fullscreen exits — 170 samples, **0 black**. The wallpaper underneath is
the shell's own (the per-user still), so the transition reads as "the
wallpaper starts over" rather than a glitch.

## The UNPROVEN list

Single monitor, one rig, one Windows build. **Not** proven here:

- multi-monitor, mixed-DPI, portrait, and a monitor hot-plug while Unloaded;
- HDR / colour handling, AMD and Intel-Arc hardware decode;
- battery / DC, the real sleep→wake duration, the session-lock owner;
- Windows 10 22H2 itself (the min-OS gate is **test-proven** at the unit
  level and the boundary logic is proven, but the gate was never run ON a
  22H2 machine);
- the fullscreen-exit gate on a machine with a real fullscreen **game**
  (the gate used a real borderless fullscreen window, which the engine's
  watcher detects exactly as it detects a game);
- a lean libmpv / lean ffmpeg build (no toolchain, no source — DEFER);
- the mpv-based probe's cost on the compress path (the swap is not applied,
  so the cost was never measured on the real path);
- the ZIP target ≤ 115 MB (not met — the lever is DEFER).

## The known costs (measured, not hidden)

1. **The video restarts from its first frame** after a long invisible-owner
   pause: the renderer is new. The wallpaper underneath is the per-user
   still, which IS the video's first frame, so the transition reads as
   "the wallpaper starts over" rather than a glitch.
2. **The idle headless mpv instance (~36 MB) is still held** while
   Unloaded (60.1 MB total vs the 24.2 MB idle floor). It is shared, so
   handing it to the graveyard needs an exclusive-ownership handoff — a
   follow-up, not a D2 change.
3. **A deep-DPB source is now re-encoded** instead of copied, so the
   compress job takes a little longer (the DPB probe is 39 ms) and the
   output is a lossy re-encode (Δluma 0.04, Δstddev 0.06).

## The test gate and a live engine — MEASURED (G3)

**The suite passes WITH a live engine running.** The earlier note in this
document ("run ctest with NO engine running") was written from the hang
that the pipe-name collision caused, and it was fixed rather than worked
around: the pipe tests now claim a **unique pipe name per test process**
(the `K6WP_ENGINE_PIPE` override, the process id in the name) and the
engine's singleton mutex has the same override (`K6WP_ENGINE_MUTEX`), so a
test can never talk to - or be blocked by - the user's engine.

The measured result on this host, **with the live session's engine
running**:

| run | result | time |
|---|---|---|
| the suite, the engine up | **49/49 pass** | 73.6 s |
| the suite, the engine up (a later run) | **49/49 pass** | 51.2 s |

Before the fix the same situation hung for **30 minutes** (the test bound
a fake pipe server on the live engine's name, the bind failed, and the
client waited for an answer that never came). Every test now also has a
**300 s timeout** (a hung test can no longer hang the suite without a
bound), so even a future collision fails fast instead of hanging.

**So: no precondition. Run the suite with the engine up or down.**

## Gate 1: PASS

Every Phase 1 task is ACCEPT or a DEFER **with its reason measured and
recorded**. No parity gate regressed, no escalation trigger (§11) fired,
and the Phase 2 prerequisites are verified on this machine (rustup, cargo,
the MSVC target, cargo-deny, cargo-audit, and a Slint build that compiles
and runs). **Phase 2 may start.**
