# K6WP — known bugs / limitations (§16.7)

Task-26 release-bundle list. Every item is labeled exactly:
**live-proven** (observed on this dev machine), **code-quoted** (mechanism
read in source, not observed live), or **VM-only** (needs a VM / hardware
not present here). Sources point at `.omo/evidence/` + notepad learnings.
No item is invented: untested legs are listed as untested.

## 1. Windows 11 24H2 (Progman without WorkerW)

- The injector tries WorkerW → Progman → 24H2-layered hosts
  (`engine/src/desktop_inject.cpp`). **Live-proven on this machine**:
  24H2 z-order measured `SHELLDLL_DefView (icons) → … →
  K6WP.DesktopInject.1 (video)` — icons above video, PASS
  (learnings T5; `task-5-restore-icons.txt`).
- **VM-only**: Windows 10 and pre-24H2 Windows 11 still create the
  classic WorkerW path. The Strategy-A/B/fallback branches for those
  hosts are **code-quoted** — no machine here runs them. If the video
  lands above the icons on an older build, that branch is the suspect.

## 2. Multi-monitor / DPI edges (dev-box verified; rig artifact outstanding)

Test machine: single 1920x1080 monitor, 100% scaling. The placement rewrite
is exercised on that setup and by the headless placement matrix; the rest is
honestly marked. No 2-physical-monitor run exists yet, so the headline
multi-monitor claim is not live-proven. See the capture instruction at the
end of this section.

- **Live-proven on the dev box (single monitor)**: inject rect == monitor
  rect 0,0,1920,1080; PerMonitorV2 manifest + runtime awareness `2`
  (`task-4-dpi-geometry*.txt`); synthetic same-resolution WM_DISPLAYCHANGE
  re-enumerates with no crash/duplicates; Explorer restart re-anchors to the
  fresh Progman HWND (learnings T3). The 24H2 WorkerW path emits exactly one
  `placement: host-resolution` line per attach pass and one
  `reason=placement: covered` line per monitor, deterministic across
  consecutive `--exit-after-ms` runs (task-7 evidence).
- **Test-proven, not live**: the multi-monitor placement matrix
  (`multi_monitor_placement_test`, `desktop_placement_test`,
  `multi_monitor_factory_test`, `desktop_zorder_test`, `workerw_span_test`)
  locks negative virtual coordinates, portrait rects, a non-spanning host and
  the single z-order contract with synthetic monitor fixtures. A non-covering
  child logs `placement: RETRY-FALSE-SUCCESS` and degrades to headless
  (surfaced as the existing `kDegraded` indicator). This proves the
  mechanism; it does not replace a real two-monitor observation.
- **VM-only / code-quoted**: real resolution or DPI change at runtime (the
  re-attach resize leg) and true mixed-DPI multi-monitor (125/150%) have no
  live run on any machine here. The DPI>100% path is argued from
  PerMonitorV2 physical pixels + runtime awareness `2` (learnings T4
  follow-up), never observed.
- **Code-quoted**: span mode (`MultiMonitorMode::Span`) has no CLI flag and
  is engine-code-only; its geometry (virtual-screen union) was verified by
  reading the metrics, not by displaying. Automatic span is out of scope for
  this release (see §10).
- **Out of scope by decision: HDR / colour handling.** No HDR metadata, no
  tone mapping and no colour-management path ships; SDR video is mapped as
  before. This is a recorded scope boundary (plan Must-NOT-have), not a
  defect: do not file an HDR fidelity report as a regression.
- **Duplicate-mode per-monitor assignment is refused, not supported.** Under
  Duplicate both monitors report the same rect, so `DetectKeyCollision`
  refuses a per-monitor map that would stack two videos on one rect. The
  refusal is engine-side, not only in the Studio UI. A Duplicate-mode rig
  keeps today's single global video.
- **`\\.\DISPLAYn` assignment keys renumber on a port change and require
  re-assignment.** `displays.json` is keyed on the GDI device name, which is
  not hardware-stable; docking/undocking or a port change can renumber it. An
  assignment whose key no longer matches logs a retention/re-key warning and
  falls back to the global video rather than landing on the wrong screen.
  Re-assign after such a change.
- **Awaiting rig artifact**: the 2-physical-monitor run has not happened yet,
  so no third-party bundle is in the evidence ledger and IS-1 stays UNPROVEN.
  When it lands, replace this placeholder with the real path:
  `<rig-bundle>: .omo/evidence/2monitor-<timestamp>/summary.txt` (produced by
  `tools/run_2monitor_evidence.ps1` per `docs/runbook-2monitor.md`). Until
  then the 2-monitor claim stays unproven; wait for the real artifact.
- If a secondary monitor shows the video at the wrong offset or with black
  bars, run `tools/run_2monitor_evidence.ps1` and send back the bundle
  (extended `monitor_dump.exe` JSON + `injected_windows.json` rect census +
  `placement.log`), per `docs/runbook-2monitor.md`.

## 3. Hardware variance (NVENC / QSV / AMF / x264, perf)

- Encoder priority is hardcoded `NVENC → QSV → AMF → x264`
  (`compressor/src/encoder_detect.cpp`). **Live-proven**: NVENC leg +
  `--probe-encoders` on this machine; x264 fallback encodes (CPU).
  **VM-only / other-hardware**: QSV (Intel) and AMF (AMD) legs have
  never run here — a machine with that hardware owns the proof.
- Perf acceptance (CPU <2%, RAM 60–80 MB, GPU <5% @1080p H264 hwdec)
  is **NOT met by the Debug build on this machine and NOT re-measured
  for Release**: Debug dual-decoder showed ~0.77% of 16 cores, ~350 MB
  RAM, GPU 3D ~55% (learnings T6). Release shrinks this (0.31 MB engine
  vs Debug) but ships no perf counter proof — treat the §Success-G
  numbers as aspirational until a Release hwdec run is measured.
- **Unmeasured**: 4K assets (none in repo), reboot-cold startup (measured
  warm-cache only, median 1337 ms; true reboot is manual-test item 6),
  battery/DC transition beyond the `--simulate-dc-after-ms` harness.

## 3a. RAM budget per resolusi (FINAL 2026-09-20 — Release, iGPU-routed)

- **Live-proven** (A/B `tools/bench_ram_self.ps1`, Release, d3d11va,
  settled working-set avg, single 1920x1080 monitor; raw runs in
  `docs/bench_ram_clamp.json`). Engine binary + file effect terisolasi:
  | Konfigurasi | 1080p H264 | 4K H264 |
  |---|---|---|
  | vo=gpu, file lama (baseline) | 216.9 MB | 638.9 MB |
  | vo=gpu-next, file lama | 196.5 MB (−20) | 577.2 MB (−62, render terverifikasi via screenshot) |
  | vo=gpu-next + file DPB-pinned (compressor refs=2/bf=1) | 189.5 MB (≈netral, pool 1080p di bawah noise ±20 MB) | **396.6 MB (−181)** |
- Mechanism (**live-proven**, mengoreksi §7): surface decode hidup di
  pool driver D3D11 (shared GPU ~667 MB @4K file lama), bukan heap mpv.
  Ukuran pool ditentukan SPS `num_ref_frames` stream — file internet
  (refs besar) → pool besar. Clamp antrian mpv (demuxer/vd-queue/
  ad-queue/vd-lavc-threads): netral (±5 MB). `hwdec-extra-frames` dan
  `vd-queue-enable=yes`: perusak (+23 MB @1080p) — dilarang, lihat
  komentar di `mpv_renderer.cpp`. `swapchain-depth=2`: netral (−0.5 MB,
  di-revert). Debug ≈ Release (±3 MB) — base bukan CRT debug heap,
  prediksi interim sebelumnya salah dan dicabut.
- **Live-proven** (topologi): `nvidia-smi dmon` menunjukkan dec=0% di
  dGPU saat 4K jalan → decode di iGPU/shared memory (display
  digerakkan iGPU). Pindah decode ke dGPU = cross-adapter present
  (risiko lebih buruk) → **no-go**, ini fisika topologi, bukan utang.
- **Live-proven** (kasus "426 threads"): 5 menit Studio-terbuka
  (~200 poll GetState 1.5s) → threads 36→31 (turun), handles datar
  414–428. Tidak ada leak IPC. Angka 426 adalah kolom Handles yang
  salah baca (handles kini 423–428). Kasus **tertutup**.
- Compressor DPB pinning (`-refs 2 -bf 1`, NVENC + libx264; QSV/AMF
  disengaja tidak diubah — leg itu tidak pernah jalan di mesin ini)
  + cache generation bump `v2` (file cache lama otomatis re-encode).
  Kualitas: CRF sama, konten loop — perbedaan praktis tak terukur.
- Budget final (topologi iGPU, file DPB-pinned): **1080p ≤ 220 MB,
  4K ≤ 450 MB**. File luar (rip internet, DPB besar): ≤ 650 MB —
  dimitigasi oleh alur compress-on-import yang memang sudah ada.
  Target agresif (4K < 300 MB RAM sistem) hanya via arsitektur:
  render-API single-decode (N monitor) / unload-on-long-pause —
  roadmap, bukan tuning.

## 4. AV / SmartScreen (unsigned bundle)

- **Code-quoted, no AV-vendor test performed**: the exes are
  **unsigned** — expect Windows SmartScreen "Unknown publisher" on
  first run, and possible heuristic flags because the engine injects a
  window into the desktop (SetParent to Progman/WorkerW) and the app
  writes an HKCU `...\Run` autostart value. Both behaviors are
  legitimate (uninstall removes the Run value; injector is TOOLWINDOW +
  NOACTIVATE, never a keylogger shape) but match common heuristics.
- Mitigation status: per-user install (no admin/UAC), HKCU-only,
  stock NSIS plugins, no bundled third-party runtime beyond Qt +
  documented vendor binaries. Signing (cert + timestamp) is the real
  fix and is **not done** — do not promise clean AV verdicts.

## 5. Installer / bundle failure legs (proven)

- **Missing DLL**: the ZIP pipeline fails closed — `dumpbin
  /dependents` per staged exe lists every unresolved hard import and
  aborts instead of shipping (task-26 evidence: full-import OK on the
  release file set; throwaway incomplete-staging probe proves the FAIL
  leg). Delay-load misses are warnings, not fatal.
- **Unwritable install dir**: the NSIS writability pre-check aborts
  with a message before any file is copied (proven by debug replica,
  `task-27-installer.txt` §7: guard exit 2, zero partial state).
- **Qt-free launcher**: `K6WP.exe` must pull zero `Qt6*.dll`
  (task-26 evidence: `QTFREE_OK` on release binaries). If this assert
  ever fires, the launcher regressed — do not ship.
- **Headless-session caveat** (environmental, not a defect):
  `SPI_GETDESKWALLPAPER` returns 0 with no wallpaper provider, so the
  installer skips the OS-wallpaper backup gracefully and uninstall
  skips the restore when the value is absent (task-27 §7).

## 6. Minor / cosmetic (carried over)
- First `get_state` after pipe-up may report `running:false` until the
  video starts; Studio maps both to Connected (docs/known-issues.md).
- `lupdate.exe` in Qt 6.8.3 here cannot run (missing Linguist runtime);
  the `translations` target degrades gracefully (same file).
- **Version is single-sourced, not hand-synced** (MED-14, done 2026-09-23;
  `docs/tech-debt.md` marks it RESOLVED). `K6WP_VERSION` in the root
  `CMakeLists.txt` is canonical and the only place a version is typed.
  `configure_file()` stamps it into three generated consumers:
  - `build/<cfg>/generated/version.h` from `version.h.in`, consumed by
    every `.rc` (all four `app.rc` plus `shared/monitor_dump.rc` via
    `FILEVERSION` / `PRODUCTVERSION` and the `FileVersion` / `ProductVersion`
    strings) plus `K6WP_VERSION_STR` for the Studio update-check
    `User-Agent` and the About dialog. The root `CMakeLists.txt` puts
    `${CMAKE_BINARY_DIR}/generated` on the include path, so `rc.exe` and
    every C++ TU can `#include "version.h"`.
  - `packaging/version.nsh` from the root `version.nsh.in`, which
    `packaging/installer.nsi` `!include`s; `APP_VERSION`,
    `VIProductVersion` and the `VIAddVersionKey` values are all derived
    from it, so `installer.nsi` carries no version literal.
  - `engine/app.manifest` in place from `engine/app.manifest.in`
    (`version="@K6WP_VERSION_4@"`).
  `packaging/make_zip.ps1` asserts the whole chain and fails the run on
  drift: `project()` VERSION and `K6WP_VERSION` must agree, all three
  generated files must carry the canonical version, and a hand-written
  version literal in any consumer source is a hard failure rather than a
  warning. The old "three `.rc` files + manifest + `installer.nsi`" list is
  therefore obsolete; the current risk is a stale *generated* file, which
  the assert message names explicitly ("run cmake configure first").

## 7. RAM Engine (~220MB vs budget 80MB)

- **Live-proven** (Step 9.4, `docs/bench_idle_video_1min.json`):
  engine playing 1080p H.264 (`build/spikes/test_1080p.mp4`, autoplay at
  boot, `Engine:FirstFrame` in log) averaged **0.42% CPU over 16 cores**
  (peak 1.52%, 116 samples @500ms, 1 min) — **CPU PASS (<2%)** — but
  **~220 MB working set** (avg private 222.6 MB, peak 224.0 MB) against
  the 80 MB budget from planning §6.
- Mechanism (**code-quoted**): mpv D3D11 decoder context + the layered
  desktop surface pre-allocation (`engine/src/mpv_renderer.cpp`,
  `engine/src/desktop_inject.cpp`) dominate; the MsgWait idle loop
  itself is near-free (Step 9.4: 100 → 250 ms tick).
- Deeper optimization needs mpv-internal tuning (decoder surface
  budgeting, VO options) — **out of scope for v1.0**. The §3 numbers
  above stay aspirational until a Release hwdec run is measured.

## 8. hwdec logging gap ("unknown" in automated benches)

- **Live-proven**: `engine.log` does not expose the mpv
  `hwdec-current` property — the Step 9.4 bench log contains only
  `mpv [vd/warn]` video-aspect lines, so automated tooling honestly
  reports `"hwdec": "unknown"` (`docs/bench_idle_video_1min.json`).
- Visual/manual runs still show hardware decode active (video plays
  at 0.42% CPU, consistent with hwdec, not software decode).
- Fix direction (not done): mirror `hwdec-current` into `engine.log`
  on change (one line in `MpvRenderer::PollEvents`). Until then,
  bench JSONs keep the explicit `"hwdec_source"` note instead of
  guessing.

## 9. Multi-monitor = per-monitor N decode (Phase 3-lite descoping)

- **Decision (2026-09-22, spikes A/A2)**: single-decode fan-out is
  **descoped for 1.1.0**. GL-WGL sharing proved the architecture
  (decode=1, 2 windows) but forced software decode (`hwdec=no`,
  CPU +8.5% vs hwdec-dual) — trade rejected. Native D3D11 render API
  is `MPV_ERROR_NOT_IMPLEMENTED` in vendored libmpv (blocked on
  upstream PR #17764, see tech-debt). ANGLE rejected (no pinnable
  source vs `vendor/VERSIONS.md` policy).
- **Live-proven on the dev box**: multi-monitor stays N mpv instances (one
  decode per monitor). Mitigations that ship: per-slot occlusion pause (P2.5),
  lean mpv profile (P1.1), E-core affinity + iGPU pin (Phase 3-lite —
  `docs/bench_phase3.json`: 1080p playback 0.02% CPU, 141.8 MB WS,
  hwdec d3d11va on Intel UHD, RTX dec 0%). Per-monitor *video assignment*
  now ships (`displays.json`, `set_display_video`, Studio monitor sub-tabs), but
  it is still one decode per monitor.
- **Still no 2-physical-monitor run**: the placement rewrite and the
  per-monitor assignment map are covered by the headless placement matrix and
  the dev-box single-monitor run only (`multi_monitor_placement_test` and
  friends, one `reason=placement: covered` line per monitor in `engine.log`).
  A third-party two-monitor artifact is **awaiting collection**: the run-book
  (`docs/runbook-2monitor.md`) and `tools/run_2monitor_evidence.ps1` exist to
  produce it (see §2's placeholder). IS-1 stays UNPROVEN until that artifact
  is pasted back; no unproven 2-monitor claim is made here.

## 10. Phase 4 final-pass notes (2026-09-22, `docs/bench_phase4.json`)

- **Occlusion polling 1.5 s remains BY DESIGN** (P2.5): the unpaused
  message-loop wait doubles as the occlusion cadence — the sole sanctioned
  periodic wakeup, self-suspending while any pause owner is active. No
  event-driven replacement exists for maximized-window coverage (DWM
  thumbnail/region queries have no notification API); the paused state
  spends zero wakeups on it (loop wait goes INFINITE).
- **iGPU heuristic, not a guarantee**: `auto` picks
  `DXGI_GPU_PREFERENCE_MINIMUM_POWER` with a VendorId-table fallback, and
  the post-start verify pass recreates-unpinned + reloads when a pinned
  renderer reports hwdec inactive. A wrong pick costs one flicker + reload,
  never a stuck software-decode session. Override with
  `"gpu_adapter": "discrete"` / `"integrated"` (restart required).
- **Span-auto conditions**: span mode (`MultiMonitorMode::Span`) is still
  engine-code-only with no CLI/Studio flag; per-monitor (`-1` = all screens)
  is the only shipped topology, and the new per-monitor video assignment
  (`displays.json`, §2) does not change that. Automatic span (one decode
  across the virtual-screen union) arrives only with the single-decode render
  API (§9); until then every monitor owns its decode instance.
- **ffmpeg essentials scope**: the 8.1.2 essentials build carries exactly
  what the pipeline calls — `h264_nvenc`, `h264_qsv`, `h264_amf`, `libx264`
  encoders, `scale`/`fps` filters, mp4 `faststart`. Anything outside that
  set (other codecs, filters, protocols) is absent by choice: a compress
  job needing them fails with the existing friendly error, never silently.
  `ffplay.exe` from the upstream zip is deliberately NOT vendored.
- **Working-set trim accounting**: the one-shot trim collapses resident
  set (idle ~29.9→0.2 MB, playback ~151.6→0.2 MB) while private bytes are
  untouched (idle 24.2 MB, playback ~150.8 MB). Judge RAM by private bytes
  + `PeakWorkingSet64`, never by post-trim working set; the soft-fault
  cost of re-touching evicted pages on resume/seek is unmeasured.

## 11. Paused wakeups — documented limitation (pre-release diagnosis, 2026-09-22)

- **Repro**: engine + video, manual IPC `pause` (mask=1, `slots=1`),
  60 s window, per-TID `Win32_PerfRawData_PerfProc_Thread` raw-delta
  method (`.omo/evidence/matrix/tidsnap.py`, the phase-2 row-b tool):
  **11.8/s process-wide** (phase-2 row-b: 11.9–13.5/s — reproduced).
  Top thread **9.67/s @ 0.002% CPU** = the IPC `ServeLoop` 100 ms accept
  slice (`engine/src/ipc_server.cpp:28` + `WaitWithStop`), plus two
  libmpv-internal threads at ~1.01/s each @ 0.000%. Everything else
  ≤0.11/s. Zero file I/O + zero periodic log lines in the window: decode
  is fully stopped, the churn is pure wait-slice wakeups at ~0 CPU.
- **Ordered suspects checked and cleared**: (1) config watcher runs the
  `event=rdevchange` path (log: watch armed, no fallback); (2) occlusion
  `Check` early-outs while disarmed (`occlusion_armed_`, zero occlusion
  lines in the window); (3) only two `SetTimer` sites exist engine-wide
  (config debounce + one-shot WS trim, both dead in steady state).
  `MsgWait` INFINITE + `mpv_wait_event(-1)` confirmed silent (main loop
  and both mpv event threads 0.00/s).
- **Why not fixed**: silencing the 10 Hz slice means an event-driven
  accept loop — an IPC-server redesign touching accept latency and the
  protected IPC v1 path. Out of scope for 1.1.0 by design; the gate
  (< 1.0/s) stays recorded as NOT MET.
- **Method confound on record**: any run with `--exit-after-ms` (or any
  `--simulate-*`) forces the 50 ms loop cadence via `AnySimulateArmed()`
  and measures ~80/s instead — those numbers are superseded for cadence
  purposes, including a P4-time 60 s typeperf run that briefly read
  79.8/s before the confound was identified.
- **HOTFIX (occlusion resume, 2026-09-22, on master post-1.1.0-tag)**:
  closing a borderless-fullscreen window stranded the fullscreen bit at
  1/2 debounce confirms (close yields exactly one foreground event; the
  INFINITE-while-paused loop runs no 1 s poll to complete it), so decode
  stayed paused until the next user input. Fix: track the fullscreen
  HWND; its `EVENT_OBJECT_DESTROY` clears the bit immediately (destroyed
  ≠ blip) + a debounced 150 ms poke runs one direct occlusion check.
  Proven: scripted close → clear same-ms, mask 0 in ≤0.6 s; Win+D cycle
  0.5 s; captioned-maximize still never trips the guard. Paused 60 s
  re-verified 13.5/s vs 11.8/s (≈, zero CPU, transient-only). No new
  periodic timer (conditional/transient `SetTimer` sites only).
- **HOTFIX follow-up (F11-out, same branch)**: exiting fullscreen via F11
  fires NO destroy/foreground/minimize event (window alive + focused,
  rect only) — same strand family, confirmed stuck-or-lucky by test
  (first run healed at 1.5 s via unidentified wake, not a trigger).
  Fix: `EVENT_OBJECT_LOCATIONCHANGE` hook (top-level only); the tracked
  holder's own rect flip re-queries and clears at once. Proven: F11-out
  → clear in 11 ms, mask 0 ≤0.5 s, full chain logged (locationchange →
  immediate clear + poke → check). F11-in cascades now complete the
  2-confirm in ~2 ms (was ~2.5 s by poll luck). No SHOW/HIDE hook
  (tooltip/menu storm volume judged not cheap).
  Residual: hide-via-API without focus change takes the same strand path
  (accepted: rarer than close, same pre-fix behavior).
