# Manual test checklist

Build the release preset first (docs/release-build.md). Each item lists
the evidence file it was cross-checked against in `.omo/evidence/`.
Item 6 is a MANUAL reboot step: procedure plus expected evidence, not a
claimed result.

1. Apply switches video live. In Studio, pick a library video and press
   Apply (or double-click it). Expect the wallpaper to change with the
   same engine pid still serving `get_state` — no taskkill, no restart.
   If the engine is down, Studio saves the choice and boots a fresh
   engine on it instead. (task-23-library-thumbnails.txt)
2. Tray menu works. Right-click the tray icon: Pause/Resume toggles
   playback (Studio status flips to Paused within ~2 s and back on
   resume), Next Wallpaper advances, Exit shuts the engine down cleanly.
   (task-12-dblclick-studio.txt, task-24-status-states.txt)
3. Tray double-click opens Studio. Double-click the tray icon: Studio
   appears. Repeat while Studio is open: still exactly one Studio
   window (same HWND), focused. (task-12-dblclick-studio.txt,
   task-13-studio-singleton.txt)
4. Singletons hold both directions. Start a second engine while one
   runs: it must exit 0, serve no second pipe, and leave one engine
   process. Repeated Open-Studio requests must never stack Studio
   windows (see item 3). (task-14-singleton.txt,
   task-13-studio-singleton.txt)
5. Start-with-Windows toggle writes HKCU only. In Studio settings, check
   Start with Windows and Apply, then `reg query
   HKCU\Software\Microsoft\Windows\CurrentVersion\Run /v K6WP` must show
   `"...\K6WP.exe" --engine --silent`. Uncheck and Apply: the value is
   gone. No UAC prompt at any point. (task-17-autostart-chain.txt,
   task-19-autostart-toggle.txt)
6. MANUAL — reboot with autostart on. Enable Start with Windows, reboot,
   log in, do not launch anything. Expect: engine running with tray
   icon, no Studio window, wallpaper playing, cold start under 2 s
   (reference median 1337 ms over 5 warm runs, task-18-coldstart.txt).
   Record the observed start time and engine log lines; this step has no
   pre-recorded pass result.
7. No taskbar button, no Alt+Tab entry, tray icon present. While the
   engine runs, confirm the wallpaper window appears in neither the
   taskbar nor Alt+Tab, and the tray icon is visible. Run
   `tools/verify_wallpaper.ps1 -VideoA <a> -VideoB <b>` (engine must
   already run; the script never starts it) and expect
   `RESULT: PASS (20/20 acks, ...)`. (task-7-toolwindow.txt,
   task-8-fallback-reanchor.txt, task-10-zorder.txt,
   task-11-stress.txt)
8. Multi-monitor cover. With two monitors attached, start the engine and
   confirm each monitor is fully covered edge to edge, desktop icons stay
   above the video, and the engine log shows one `attached ... to
   Progman, layered=YES` line per monitor plus `TOOLWINDOW=YES`.
   (task-9-fullscreen-cover.txt, task-10-zorder.txt)
9. Wallpaper restore on explicit exit. Start from a fresh data folder, note
   your static desktop wallpaper A, run the engine with the wallpaper still
   on, then Exit via the tray menu. A must be back (the engine spawns the
   per-user restore on a normal exit). A shutdown/logoff deliberately does
   NOT restore (T6). (task-15-wallpaper-restore.txt)
10. Drag-drop import. Drag one or more video files (plus a non-video file
    as a negative control) onto the Studio window or library. Expect the
    videos added to the library, the first valid one selected, the
    non-video file rejected with a friendly message, and no autoplay.
    (task-21-dragdrop.txt)
11. Preview with honest errors. Select a valid video: a real frame shows.
    Point Studio at a missing file: a `Preview unavailable: file not
    found` label shows instead of a crash. An unplayable file shows the
    unsupported-file label. (task-22-preview.txt)
12. Four status states. Studio status bar must show Connected (green)
    while playing, Paused (orange) while tray-paused, Not running (gray)
    with the engine stopped, and Disconnected (red) when the engine
    rejects a request. (task-24-status-states.txt)
13. MANUAL — Win+L pauses, unlock resumes. With a video playing, press
    Win+L: playback must pause (Studio status flips to Paused within
    ~2 s, engine log shows `session: WTS_SESSION_LOCK received`). Unlock:
    playback resumes with no user action (status back to Connected,
    `WTS_SESSION_UNLOCK received` in the log).
14. MANUAL — user pause survives lock/unlock. Tray-pause playback, then
    Win+L, then unlock: playback must stay paused (status Paused
    throughout, no decode behind the lock screen). Tray-resume after
    unlock resumes normally.
15. MANUAL — maximized-window occlusion pauses decode. With a video
    playing, maximize any window so it fully covers the wallpaper monitor
    for >2 s: decode must pause (engine log shows the occlusion pause;
    Studio status flips to Paused within ~2 s). Restore/minimize the
    window: playback resumes with no user action. (P2.5: 1500 ms
    self-suspending tick, 95/90 hysteresis — brief overlaps must NOT
    flicker playback.)
16. MANUAL — screen-off pauses, screen-on resumes. With a video playing,
    turn the monitor off (power button or idle timeout): decode must pause
    (engine log shows `power: monitor off (Data=0), pausing decode`).
    Turn the monitor back on: playback resumes (`monitor on (Data=1)`).
    Headless equivalent: `engine.exe --simulate-monitor-off-after-ms 2000
    --exit-after-ms 9000` exits 0 with the same log lines.
17. MANUAL — battery static mode. In Studio Settings → Engine, set Mode
    hemat baterai to "Jeda penuh saat baterai" and enable Hemat baterai,
    then unplug (DC): decode must pause fully (last frame shown, status
    Paused). Replug (AC): playback resumes. With "Batasi 24 fps" instead,
    playback continues at 24 fps on DC. (Master gate: the basic-tab Hemat
    baterai switch; CPU/GPU picks in the same box need an engine restart.)
18. MANUAL — multi-monitor per-slot pause (needs 2 monitors). With a video
    on both monitors, fully cover ONE monitor with a maximized window:
    only that monitor's decode pauses; the other keeps playing (per-slot
    `PauseSlot`, never global). Note: 1.1.0 still decodes per monitor (N
    instances, no shared decode — see packaging/known-limitations.md §9);
    this item checks independent pause, not instance count.
19. MANUAL — Extend-mode coverage (needs 2 monitors). Set Windows to Extend
    (Win+P), start the engine, and confirm every monitor is covered edge to
    edge with desktop icons above the video. The engine log must show one
    `placement: covered` line per monitor plus one `attached ... to Progman,
    layered=YES` line per monitor — the item-8 assertion, now under Extend.
    Collect the bundle with `tools/run_2monitor_evidence.ps1` and check
    `monitor_dump.exe` reports both monitors, per `docs/runbook-2monitor.md`.
20. MANUAL — Duplicate-mode coverage (needs 2 monitors). Press Win+P and
    choose Duplicate, then restart the engine if it stopped. Re-check the same
    coverage assertion: both screens filled, icons above, one
    `placement: covered` line per reported monitor and no crash. Run
    `tools/run_2monitor_evidence.ps1` a second time and keep both bundles
    (Extend and Duplicate), as `docs/runbook-2monitor.md` Step 4 asks.
21. MANUAL — portrait secondary (needs 2 monitors). In Display settings rotate
    one monitor to portrait. Its wallpaper must fill the rotated rect edge to
    edge, with no letterboxing and no video rotation: the picture stays
    upright, scaled to the tall rect. The engine log's placement line must
    report that monitor covered, and `monitor_dump.exe` must report the tall
    rect. Record which monitor is portrait, as `docs/runbook-2monitor.md`
    Step 5 asks.
22. MANUAL — hotplug and arrangement change (needs 2 monitors). With videos
    on both monitors, unplug the secondary monitor and replug it (or move it
    to the other side in Display settings). The engine must not crash, and
    the retained per-monitor assignment must re-apply to the same monitor.
    If its `\\.\DISPLAYn` key renumbered, the engine falls back to the global
    video and logs a re-key warning instead of landing on the wrong screen.
    `get_state` must still answer. See `docs/runbook-2monitor.md`.
23. MANUAL — per-monitor assignment by click (needs 2 monitors). In Studio's
    Wallpaper view, arm a library (or playlist) video, then click monitor 2's
    sub-tab. Only that monitor's picture
    changes; monitor 1 keeps playing. `get_state` must show the map under
    `display_assignments` (keyed by `\\.\DISPLAY2`) and `displays.json` must
    gain that entry. This is the `set_display_video` path driven by the arming
    gate.
24. MANUAL — Duplicate-mode assignment refusal (needs 2 monitors). Switch to
    Duplicate and attempt the same click-assign on a monitor. Both monitors
    report one rect, so the assignment is refused — the Duplicate-mode
    assignment refusal recorded in `packaging/known-limitations.md` — with a
    message naming the collision, rather than stacking two videos on one screen.
    `displays.json` must stay unchanged and `get_state`'s
    `display_assignments` must not gain a key. See `docs/runbook-2monitor.md`.
25. MANUAL — clear a per-monitor assignment (needs 2 monitors). With a video
    assigned to monitor 2, clear it from monitor 2's sub-tab in Studio (the
    `set_display_video` clear flag). Monitor 2 must revert to the global video,
    the `display_assignments` entry must disappear from `get_state`, and
    `displays.json` must remove that key. See `docs/runbook-2monitor.md`.
26. MANUAL - sub-tab assignment acceptance (needs 2 monitors). Assign a video
    to monitor 1 through its sub-tab, then select monitor 2's sub-tab and
    confirm the preview and the sub-tab state reflect monitor 2 only, with no
    assignment leaking to monitor 1. `get_state`'s `display_assignments` must
    hold exactly one key per assigned monitor.
27. MANUAL - click-to-assign refusal acceptance (needs 2 monitors). Click a
    monitor sub-tab with nothing armed: no assignment is sent and
    `display_assignments` stays unchanged. Then switch to Duplicate, arm a
    video and click a sub-tab: the collision refusal appears and
    `displays.json` stays unchanged. Clearing the arm before selecting a
    sub-tab must also leave the assignment untouched.
28. MANUAL - sleep/resume keeps playback (power-resume regression). With a
    video playing on every monitor, sleep the PC (Start -> Power -> Sleep or
    close the lid), wait a few seconds, then wake it. Playback must continue
    on every monitor within ~2 s with no black/frozen wallpaper and no user
    action. `engine.log` must show `power: PBT_APMRESUMEAUTOMATIC received`
    followed by `pause: owner=suspend ... slots=0` and then motion again; it
    must NOT show `mpv: end-file (reason=quit)` in that resume pass. On an
    Explorer restart (real re-anchor) the log may show one re-attach followed
    by `loadfile` — that is the expected recovery path.
29. MANUAL - lock screen is not overlaid. Press Win+L while a video plays.
    The Windows lock screen (secure desktop) must be shown normally: no
    wallpaper video, no K6WP window, no frozen frame on top of it. Unlock and
    confirm playback resumes (item 13). The injected surface is a
    Progman/WorkerW child, not a topmost window, so it must never cover the
    lock UI; this item records the check.

## 1.3.0-beta.3 engine fixes

Verify the engine-hardening fixes in 1.3.0-beta.3. Items marked optional are
best-effort on the matching hardware.

1. E-01 — an OS wallpaper change is not clobbered on exit. Set OS wallpaper A,
   start the engine, change the OS wallpaper to B (Windows Settings →
   Personalization → Background), then quit K6WP from the tray menu. The
   wallpaper must stay B: the engine saved A at start, so a stale restore would
   wrongly bring A back.
2. E-02 — re-anchoring does not leak WorkerW windows (needs 24H2). On a 24H2
   machine, count WorkerW-class windows (Spy++ or `tools\verify_wallpaper.ps1`
   if it dumps windows) before and after ~10 re-anchors (lock/unlock the
   session, or toggle Duplicate/Extend). The count must not grow.
3. E-03 — hwdec fallback re-arms after recovery (needs a hwdec-capable
   machine). Note the `hwdec-current=` lines in `engine.log`; force a transient
   failure (or start the engine with `--simulate-device-lost-after-ms`) and let
   playback recover. The next decode failure must still fall back: the
   `falling back to` chain lines must appear again, not just once at boot.
4. E-04 — device-lost watch armed (optional). If a TDR simulation is possible,
   or on a DXGI 1.6-capable machine, confirm `engine.log` shows the boot line
   `device-lost: adapter-removed watch armed`.
5. L-01 — crash-restart give-up (optional). Force repeated crashes and confirm
   `engine.log` shows the crash-restart give-up line once the restart budget is
   exhausted.

## Display resilience + per-user still (Bug B / Bug A)

### Monitor removal (no hang, no lost wallpaper)

The virtual-display driver in `shadow-evidence/` makes these reproducible
without a second physical monitor. The scripts (`set-display.ps1`,
`test-disable*.ps1`, `test-sdc*.ps1`, `testmode*.ps1`) and the walkthrough
live in `docs/runbook-2monitor.md`.

1. **Unplug a secondary monitor.** Start the engine with a video on all
   screens, then disable the secondary display. The engine must NOT freeze:
   the tray icon stays, the menu still opens, and the wallpaper on the
   remaining monitor keeps playing the RIGHT video. `engine.log` must show
   ONE `display: topology reconcile pass` for the burst (not one per
   event). There must be no `critical:` line about a stuck teardown.
2. **Unplug the primary monitor.** Same expectations; the survivor takes
   over as primary and keeps its own video (the per-monitor assignment must
   NOT jump to the other monitor).
3. **Unplug a middle monitor from a 3-monitor setup.** The two survivors
   must keep their own videos. This is the case the old position-index
   matching got wrong: the renderers were swapped between monitors.
4. **Replug the monitor.** The slot comes back and its per-monitor
   assignment is re-applied (`display: applied <path> to <device>` in the
   log). The wallpaper on the re-plugged monitor shows that video again.
5. **Suspend / resume with a monitor gone.** Put the machine to sleep with
   the secondary disabled, then wake it. The wallpaper must resume on the
   remaining monitor, and the log must show the re-anchor waiting for a
   stable enumeration (`display: topology still moving`) before it touches
   a slot.

### The hang watchdog (C1)

6. **A stuck engine is restarted.** Hard to force on purpose; if a hang can
   be provoked (e.g. by unplugging a monitor during a heavy decode), the
   engine must recover on its own within ~20 s: `engine.log` shows the
   `watchdog: the message loop stopped responding` line, a hang minidump
   appears in `%LOCALAPPDATA%\K6WP\crashes\engine-hang-*.dmp`, and a fresh
   engine takes over (the wallpaper comes back). The restart counts against
   the crash-restart backoff, so it cannot loop forever.

### The per-user still (T1–T10)

These replace the old HKLM-policy checks. Run them on a fresh data folder
(move `%LOCALAPPDATA%\K6WP` aside) with at least one video imported. "Status"
means `%LOCALAPPDATA%\K6WP\still_status.json` — Studio's Settings page shows
the same result. Inspect `wallpaper_ok`, `lockscreen_ok`, `error` and
`lockscreen_mode_hint`.

1. **T1 — boot apply.** Start the engine with a video configured. Within a
   few seconds the desktop wallpaper must be the video's FIRST frame
   (`offset 0`, not one second in). Status: `ok=true`, both sides true. If in
   doubt compare with
   `compressor.exe --lockframe --offset-s 0 --res <W>x<H>` output.
2. **T2 — manual pick.** Pick another video in Studio (and once from the tray
   MRU). Wallpaper and lock screen must both change, with no UAC; the status
   `ts` advances and `error` is empty.
3. **T3 — playlist rotation.** Enable the playlist with a 1-minute interval
   and wait for a rotation. The still must refresh for the new video. The
   5-second rotation throttle itself is unit-tested (`still_test`); the log
   line `still: rotation refresh skipped` must NOT appear for a normal
   1-minute rotation, and a manual pick must never be throttled.
4. **T4 — the two toggles are per-user and reversible.** Turn the wallpaper
   toggle off: the original wallpaper returns immediately, no UAC. Turn it
   back on: a fresh still is applied. Repeat for the lock-screen toggle: the
   previous image and the rotating-mode registry values return, then a still
   is applied again. Verify `HKCU\...\ContentDeliveryManager\
   RotatingLockScreenEnabled` and `HKCU\...\Lock Screen\SlideshowEnabled` are
   restored to their pre-K6WP values.
5. **T5 — Win+L shows the still.** Lock the screen: it must show a frame of
   the CURRENT video. The status hint must be `picture` (not `spotlight` or
   `slideshow`). Unlock, rotate the playlist, lock again.
6. **T6 — shutdown/logoff does NOT restore (J3).** With a still active, sign
   out or shut down. At the next logon the desktop must still show the still
   until the engine re-applies, and `original_wallpaper.json` must still
   exist. A tray Quit, by contrast, DOES restore (item 9).
7. **T7 — J1 original store.** With sync on, set a different wallpaper by
   hand in Windows Settings. The engine log must record the external change
   and update the store; a later toggle-off must restore THAT wallpaper, never
   our still. Re-apply a still afterwards; the store must never contain a
   `%LOCALAPPDATA%\K6WP\stills` path.
8. **T8 — unique stills + prune.** Trigger ~5 applies. The `stills` folder
   must hold at most two `still_*.jpg`, each a complete, openable JPEG (no
   truncated file, filenames never reused). The active wallpaper must never
   point at a missing file.
9. **T9 — no admin in the normal flow.** Set UAC to "always notify", apply a
   video and flip both toggles: no prompt may appear. Check that
   `reg query HKLM\SOFTWARE\Policies\Microsoft\Windows\Personalization /v
   LockScreenImage` is unchanged (or absent) and that
   `%PROGRAMDATA%\K6WP\lockscreen.jpg` is not recreated.
10. **T10 — legacy cleanup.** Seed a fake old install: create
    `%PROGRAMDATA%\K6WP\lockscreen.jpg` and, from an elevated prompt, set the
    HKLM policy to that path. Restart Studio → Settings shows the read-only
    legacy notice and a "Bersihkan" button. Click it: exactly one UAC.
    Accept: the policy/backup/our files are gone. Decline: everything remains
    and the message says so. A policy pointing somewhere else must never be
    offered (and `K6WP.exe --elevate-lockscreen off` must exit 0 with no
    prompt).
