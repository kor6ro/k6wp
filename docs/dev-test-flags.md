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
