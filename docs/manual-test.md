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
9. OS wallpaper restored on exit. Note your static desktop wallpaper,
   run the engine, Exit via the tray menu, and confirm the original
   wallpaper is back (engine saves on start, restores on shutdown).
   (task-15-wallpaper-restore.txt)
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
23. MANUAL — per-monitor assignment by drag (needs 2 monitors). In Studio,
    drag a library video onto monitor 2's desktop rect. Only that monitor's
    picture changes; monitor 1 keeps playing. `get_state` must show the map
    under `display_assignments` (keyed by `\\.\DISPLAY2`) and `displays.json`
    must gain that entry. This is the `set_display_video` path driven by the
    Studio drag gate.
24. MANUAL — Duplicate-mode assignment refusal (needs 2 monitors). Switch to
    Duplicate and attempt the same drag onto a monitor. Both monitors report
    one rect, so the assignment is refused — the Duplicate-mode assignment
    refusal recorded in `packaging/known-limitations.md` — with a message
    naming the collision, rather than stacking two videos on one screen.
    `displays.json` must stay unchanged and `get_state`'s
    `display_assignments` must not gain a key. See `docs/runbook-2monitor.md`.
25. MANUAL — clear a per-monitor assignment (needs 2 monitors). With a video
    assigned to monitor 2, clear it in Studio (the `set_display_video` clear
    flag). Monitor 2 must revert to the global video, the
    `display_assignments` entry must disappear from `get_state`, and
    `displays.json` must drop that key. See `docs/runbook-2monitor.md`.
