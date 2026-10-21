# Developer test flags (engine.exe) — NOT in `--help`

These flags exist for automated QA only (T1–T6 patterns). They keep working
but are hidden from user-facing help and UI. User-facing silent start is
`--minimized` (engine) / `--engine --silent` (launcher autostart);
`--minimized --silent` remains a working alias for old autostart entries and scripts.

| Flag | Form | Meaning |
| ---- | ---- | ------- |
| `--exit-after-ms` | `--exit-after-ms <N>` | shut down cleanly after N ms (0 = run forever) |
| `--simulate-device-lost-after-ms` | `--simulate-device-lost-after-ms <N>` | fire OnDeviceLost() after N ms |
| `--simulate-suspend-after-ms` | `--simulate-suspend-after-ms <N>` | post PBT_APMSUSPEND after N ms, PBT_APMRESUMEAUTOMATIC +2 s |
| `--simulate-dc-after-ms` | `--simulate-dc-after-ms <N>` | latch forced-DC + post PBT_APMPOWERSTATUSCHANGE after N ms, AC restore +2 s |
| `--simulate-monitor-off-after-ms` | `--simulate-monitor-off-after-ms <N>` | send PBT_POWERSETTINGCHANGE Data=0 (monitor off) after N ms, Data=1 (on) +2 s |
| `--capture-frames` | `--capture-frames <prefix>` | write the RENDERED frames to `<prefix>1.jpg`, `<prefix>2.jpg`, ... (QA only) |
| `--capture-frames-count` | `--capture-frames-count <n>` | how many frames to write (default 3) |
| `--capture-frames-interval-ms` | `--capture-frames-interval-ms <n>` | delay between frames (default 1000) |
| `--capture-frames-seek` | `--capture-frames-seek <sec>` | seek to this position and pause before the first frame, so the captured frame is deterministic |
| `--fps-cap` | `--fps-cap <n>` | force the playback frame-rate cap at start-up (the engine otherwise caps only on the DC power path) |
| `--dump-mpv-counters` | `--dump-mpv-counters <path>` | sample the live mpv frame-quality counters into a CSV (drops, mistimed, delayed, frames consumed, playback rate) |
| `--dump-mpv-counters-count` | `--dump-mpv-counters-count <n>` | how many samples to take (default 60) |
| `--dump-mpv-counters-interval-ms` | `--dump-mpv-counters-interval-ms <n>` | delay between samples (default 1000) |
| `--dump-mpv-properties` | `--dump-mpv-properties <path>` | write the mpv property list (every readable property name) — used to discover which counters a given mpv build exposes |
| `--unload-after-pause-ms` | `--unload-after-pause-ms <n>` | unload the decode pipeline N ms after the engine enters the paused state (drives the SAME code path the `pause_unload_after_sec` config uses) |
| `--dump-media-props` | `--dump-media-props <path>` | write the media metadata the live mpv instance reports (duration, codec, size, fps, has_audio) as JSON — the read side of the ffprobe→libmpv parity check |

The `--dump-mpv-counters` family exists because a pacing change has to be
judged on QUALITY and not only on CPU/GPU numbers: the counters say whether
frames were dropped, delivered late, or played at the wrong speed. Note
that `vo-drop-frame-count` does **not** exist in mpv 0.41 — the VO drop
counter is `frame-drop-count`.

The `--dump-media-props` flag is the read side of the ffprobe→libmpv
parity experiment (`docs/refactor/phase1-3-bundle.md`): the same five
fields ffprobe reports must come out of the live mpv instance, or the
97 MB ffprobe binary cannot be dropped from the install.

The `--capture-frames` family exists because a screen capture of the desktop
only proves what the COMPOSITE shows: an always-on-top overlay, a covered
desktop or a paused engine all make it record the plain desktop instead of
the wallpaper, which silently turns a visual-parity gate into a no-op. The
engine-side frame is the picture the renderer actually produced, so it is
comparable between two builds. The capture seeks and pauses first, so the
frame is the SAME source frame on every run.

These flags also make the message loop tick (500 ms instead of INFINITE)
while a capture is pending — a `WM_TIMER` cannot do that, because
`MsgWaitForMultipleObjects(..., QS_ALLINPUT)` does not wake on a posted
timer message.

Parsing lives in `engine/src/cli_options.cpp`; unknown `--flag` prints usage
to stderr and exits 2.

Legacy/compat: `--engine` and `--silent` remain working aliases of
`--minimized` for old autostart entries and scripts, and `--restarted` is
accepted and ignored (posted by `RegisterApplicationRestart` on OS-initiated
restarts, so it must stay parseable).

Wire + on-disk contracts (IPC NDJSON v1, config schema, studio settings,
`.bak` rule, contract-test inventory): `docs/dev-contracts.md`.
