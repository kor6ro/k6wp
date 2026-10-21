# Phase 0 budgets (task 0.9)

Budgets are **derived from the measured noise bands** in
`docs/refactor/phase0-baseline.md`, not chosen by wishful thinking. A budget
that sits inside the noise band is not a gate — it is a coin flip.

## How the thresholds were derived

For each protected metric the Phase 0 harness measured 5 independent runs
and reported the median plus the min..max band. The rule used here:

> **A regression gate = the measured band + a margin.**
> The margin must be larger than the run-to-run noise (so a real regression
> cannot hide inside it) and small enough that the improvement is still worth
> shipping.

Concretely, the noise band `B` is `(max - min) / median` of the 5 per-run
medians. The gate is set at `max(2 x B, floor)` of the baseline median, where
`floor` is the smallest change that is even worth the risk of a behavioural
regression.

Two metrics are exempt from the band rule and are called out explicitly:

- **working set** — the engine trims its own working set
  (`engine/src/working_set_trim.cpp:50`), so it measures the trim, not the
  memory use. Working set is **not a protected metric**; private bytes and
  GPU shared memory are.
- **CPU %** — the band is wide (13 % at 1080p, 29 % at 4K) because the sample
  window is 10–12 s and shares the machine with the OS. CPU is gated with a
  generous absolute margin, not a percentage band.

## Protected metrics and gates

Baseline = Phase 0 median (`docs/bench/phase0.json`).

| # | Metric | State | Baseline | Noise band | **Gate** | Meaning |
|---|---|---|---|---|---|---|
| 0 | **max engine thread CPU** | playback | 10.2 % of one core (vsync) | — | **≤ 25 % of one core** | **C2 gate — the process aggregate hides a spinning thread. No engine thread may exceed 25 % of one core during playback.** The measured vsync build peaks at 13.4 %; the desync build hit 58 %. 25 % leaves ~2× headroom over the good build while still catching a busy-wait |
| 1 | private bytes | idle | 23.84 MB | 2.3 % | **≤ 24.7 MB** (+3.5 %) | no regression at rest |
| 2 | private bytes | 1080p | 162.48 MB | 4.0 % | **≤ 168.8 MB** (+4 %) | no regression while playing |
| 3 | private bytes | 4K/16ref | 387.89 MB | 0.2 % | **≤ 401.4 MB** (+3.5 %) | no regression on the heavy case |
| 4 | private bytes | paused 30 s | 162.35 MB | 3.7 % | **≤ 168.4 MB** (+3.7 %) | paused must not grow |
| 5 | GPU shared | 1080p | 151.57 MB | 2.0 % | **≤ 156.2 MB** (+3 %) | no VRAM/UMA regression |
| 6 | GPU shared | 4K/16ref | 468.43 MB | 0.1 % | **≤ 482.5 MB** (+3 %) | no VRAM/UMA regression |
| 7 | CPU | 1080p | 4.63 % | 18 % | **≤ 6.5 %** (+40 %) | absolute margin; the band is too wide for a % |
| 8 | CPU | 4K/16ref | 4.82 % | 24 % | **≤ 6.5 %** (+35 %) | same |
| 9 | GPU total | 1080p | 22.05 % | 18 % | **≤ 27.1 %** (+23 %) | absolute margin |
| 10 | GPU total | 4K/16ref | 58.50 % | 17 % | **≤ 70.2 %** (+20 %) | absolute margin |
| 11 | wakeups/s | idle | 39.8 | 40 % | **≤ 55** | the idle floor must not regress |
| 12 | wakeups/s | paused | 30.1 | 11 % | **≤ 45** | paused must stay near the event-driven floor |
| 13 | wakeups/s | 1080p | 1273 | 11 % | **≤ 1400** | no polling regression |
| 14 | wakeups/s | 4K/16ref | 1294 | 10 % | **≤ 1450** | no polling regression |
| 15 | Studio startup | — | 1290 ms | 1226–1558 ms | **≤ 1600 ms** | the app must not open slower |
| 16 | Studio private bytes | idle | 337.53 MB | — | **≤ 348 MB** (+3 %) | Studio must not grow |
| 17 | Studio idle CPU | — | 0.01 % | — | **≤ 0.5 %** | the Studio must stay idle |
| 18 | Install size | — | 389.14 MB | — | **≤ 389.14 MB** | any phase that adds bytes must justify it |
| 19 | Engine startup | — | (see `bench_startup`) | — | **< 2000 ms** | existing budget, unchanged |
| 20 | Visual parity | — | luma 128.48 / stddev 66.69 / motion 84.87 | — | **luma ±12, stddev ±12, motion > 0** | the picture must not change |
| 21 | Resume → visible motion | — | 1114–1657 ms | — | **≤ 2500 ms** | the wallpaper must come back promptly |
| 22 | **Playback rate** | playback, 60 s | 0.98 (vsync) | — | **0.97 – 1.03** | **the wallpaper must run at real time.** The measured desync baseline was **0.70–0.75**: the wallpaper ran a quarter slow, a visible quality defect. A rate below 0.97 fails |
| 23 | **VO delayed frames** | playback, 60 s | 0 (vsync) | — | **≤ 5** | frames must not be delivered late (the desync build showed 184–197) |
| 24 | **VO frame drops** | playback, 60 s | 0 | — | **0** | no dropped frames |
| 25 | **Decoder frame drops** | playback, 60 s | 0 | — | **0** | no dropped frames |

### C4 — the honest RAM metric

**Private bytes + GPU shared memory.** The working set is NOT evidence: the
engine trims its own working set two seconds after start
(`engine/src/working_set_trim.cpp:50`), so the working set measures the trim,
not the memory in use. It is still recorded, but it never decides a verdict.

### C5 — the GPU gate is paired

The GPU numbers drift up to **1.5×** between sessions on this host (the same
binary measured 22.6 % and 35.6 % GPU on two different days). Therefore:

- A GPU gate is **always** an A/B comparison of the two builds measured
  **alternately (ABAB) inside one session** — never a candidate number
  compared against a stored baseline.
- `tools/bench_ab.ps1` and `tools/bench_pacing.ps1` both alternate the arms.
- `tools/build_ab.ps1` verifies the two binaries really differ (SHA-256)
  before any comparison runs — a "comparison" of a build with itself is void.

## Where the budget says "improve", not just "do not regress"

The gates above are parity gates. The refactor's *purpose* is improvement, so
each phase also carries a **target** that is deliberately tighter than the
parity gate. A phase that hits its parity gate but misses its target is
"accepted with debt", not "done".

| Phase | Metric | Baseline | Parity gate | **Target** | **Result** |
|---|---|---|---|---|---|
| 1.1 | private bytes, paused 30 s | 162.35 MB | ≤ 168.4 MB | **≤ 60 MB** (mpv unloaded) | pending |
| 1.1 | GPU shared, paused 30 s | 151.57 MB | ≤ 163.6 MB | **≤ 20 MB** | pending |
| 1.2 | wakeups/s, playback | 1273 | ≤ 1400 | **≤ 700** (event-driven) | **1103** — the desync busy-wait is gone (`phase1-2-vsync.md`) |
| 1.2 | CPU, playback 1080p | 4.73 % | ≤ 6.5 % | lower | **1.06 %** (−72.5 %) |
| 1.2 | CPU, playback 4K | 4.64 % | ≤ 6.5 % | lower | **1.45 %** (−64.9 %) |
| 1.2 | GPU 3D, playback 1080p | 12.47 % | ≤ 15.0 % | lower | **9.39 %** (−62.7 %) |
| 1.2 | rendered-frame parity | — | pixel diff 0 | 0 | **0.000** (byte-identical) |
| 1.3 | install size, mpv group | 114.77 MB | ≤ 114.77 MB | **≤ 60 MB** (lean libmpv build) | pending |
| 2.3 | Studio private bytes | 337.53 MB | ≤ 348 MB | **≤ 150 MB** (Slint, no Qt) | pending |
| 2.7 | install size, qt group | 48.0 MB + 8.1 MB payload | ≤ 56.1 MB | **0 MB** | pending |
| 2.7 | Studio startup | 1290 ms | ≤ 1600 ms | **≤ 700 ms** | pending |
| 3.x | install size, ffmpeg group | 194.16 MB | ≤ 194.16 MB | **≤ 100 MB** (or 0 if ffmpeg is dropped) | pending |
| 3.x | install size, total | 389.14 MB | ≤ 389.14 MB | **≤ 200 MB** | pending |

## Bundle budget rationale

The 389 MB install is dominated by two vendor binaries that are **not** the
application:

- ffmpeg 194 MB (50 %) — used only by the compressor.
- libmpv 115 MB (30 %) — used by the engine and the Studio preview.

A 30-second H.264 clip is transcoded by a full-featured ffmpeg build that
carries every encoder, filter and protocol. A build limited to
`libx264 + h264 demux/mux` would be a fraction of the size. That is the
single largest lever in the whole refactor, and it is **independent** of the
C++ → Rust work — it can ship in Phase 1.

## Rules for using these numbers

1. **Re-measure, do not extrapolate.** Every gate is checked by re-running
   the Phase 0 harness on the candidate build, on the same host, with the
   same n ≥ 5.
2. **Same conditions or no verdict.** If the candidate run had a different
   power plan, a different monitor layout, or a visible foreground window, the
   numbers are not comparable and the gate is *unmeasured*, not *passed*.
3. **A missing number is not a pass.** If a metric cannot be measured on the
   host, it stays **UNPROVEN** and the phase cannot claim that metric.
4. **Quality gates are not negotiable by a performance win.** If a phase
   improves RAM by 30 % but breaks the visual parity gate, the phase fails.
   The accept rule requires *both*.
5. **Windows 10 22H2 / 11 x64 is a parity requirement, not an optimization.**
   The min-OS claim is currently **UNPROVEN** on this host, and the source
   has no OS-version gate at all
   (`docs/refactor/contract-inventory.md`). Before the Rust cutover, the
   min-OS check must be *added* deliberately and tested, not assumed.
