# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

The v1.10 hardening work (the IPC follow-ups and the poll-path audit). The
public release is still **held** (the Slint licence decision, F3).

### Fixed

- **The Rust IPC client waits for a free pipe instance instead of failing.**
  The engine serves one client at a time, so a second client gets
  `ERROR_PIPE_BUSY` (231) until the first one disconnects. The client now
  blocks on `WaitNamedPipe` with a bounded timeout (no 50 ms polling) and,
  when the deadline passes, fails with a message that says why - "the pipe
  ... has no free instance after 5.0 s (the Win32 error 121): every instance
  is in use" - instead of a bare Win32 code. The over-limit case is covered
  by a test that proves it does not hang.
- **The Rust IPC client no longer loses an answer that arrived
  synchronously.** The overlapped read asked for no byte count, so a
  `ReadFile` that completed synchronously (the answer was already in the
  pipe buffer - the common case, because the engine answers fast) reported
  **0 bytes**: the client saw an empty reply, failed to parse it and
  counted a phantom failure. The count is now requested, and a short write
  is an error instead of a silent truncation. This was the trigger of the
  lost runs in the L2 latency gate.
- **The reconnect releases the old pipe handle first.** The engine serves
  one client at a time, so a still-open old handle kept the only instance
  busy and the reconnect waited for the whole timeout (5 s) before failing.
  `IpcClient::reconnect` now consumes the client, so the handle is closed
  before the new connect starts.

### Changed

- The engine's `get_state` does **no filesystem I/O** any more: the
  "degraded" verdict for a per-monitor assignment reads the existence hint
  recorded when the assignment was written (Studio) or applied (engine)
  instead of stat-ing the path on every poll. The stat was 57 us on a local
  volume but **21 seconds** on an unreachable network share, on the IPC
  thread - where the Studio's request timeout turns it into "Disconnected".
  No consumer read the field.
- The IPC latency trace now logs its summary only when the outermost scope
  exits, so a trace-on maximum is meaningful (before, the log flush landed
  inside the measured scope and made the maximum 9-15 ms of fake cost).
- The SPS parser's per-box debug dump (`K6WP_SPS_DEBUG`) is now **compiled
  out of a release build** (it was live in every build, including the
  release ones). A new lint, `tools/lint_env_overrides.ps1`, checks that
  every `K6WP_*` environment read sits behind a build gate - or is on an
  explicit allow-list with a reason - and a self-test proves the lint still
  fires. Both run in CI, together with the release-binary scan.

### Known limitations

- The IPC pipe serves one client at a time; a client that gives up before
  its timeout loses that request. The pre-refactor baseline lost **3 of
  1200 requests (0.25 %)** with 4 clients. The client waits properly now,
  but a request that was in flight when the pipe broke is still lost: the
  protocol has no request id and no replay. The per-worker connection model
  that removes the wait is specified for Phase 3
  (`docs/refactor/phase3-ipc-connection-model.md`).

## [0.0.1-alpha.3] - 2026-10-20

Third alpha pre-release. The elevated lock-screen mechanism is replaced by a
per-user **still** helper (the first frame of the active video becomes the
real Windows wallpaper and the lock-screen image, no UAC), and the
display-resilience regressions found after alpha.2 are fixed.

### Fixed

- **The engine no longer freezes when the monitor count drops.** The whole
  slot reconciliation used to run inside the `WM_DISPLAYCHANGE` handler
  (which the system delivers with `SendMessage`), and the renderer teardown
  waited without a timeout for an mpv VO thread that can be stuck in
  `Present()` on a display that just disappeared — the symptom was "the
  tray icon is there but nothing responds". Now: one debounced reconcile
  door outside the handler, the blocking renderer teardown moved to a
  background worker (the renderer graveyard), the injected surface is
  released instead of destroyed under a live VO, and the slots follow the
  monitor's stable identity instead of its position index (unplugging a
  middle monitor used to swap the renderers between monitors). The
  per-monitor assignment survives an unplug/replug cycle.
- **The adapter-removed watch re-arms after every event.** The DXGI event was
  registered on one `IDXGIFactory7`; once the adapter it watched disappeared
  that factory was no longer current and delivered no further events, so the
  SECOND adapter change went unnoticed and the wallpaper stayed black. The
  watch now registers the same event handle on a fresh factory after every
  signal (`DeviceLostWatch::Rearm`), and a device-lost burst cancels the
  pending topology reconcile (one surface rebuild per burst instead of a
  rebuild plus a re-anchor).
- `wallpaper_surface_live_` is re-synced with the slot census after every
  reconcile pass: when the last monitor disappears the headless renderer
  takes the decode over instead of `set_video` being rejected.
- The shadow reconcile pass computes the topology diff against the PREVIOUS
  snapshot (it was always empty), and the coverage census no longer runs
  while the reconcile mutex is held.
- The two settings the engine reads are parsed once per event instead of
  twice.
- The debounced topology door no longer loops after a wallpaper change: the
  "wait for two equal enumerations" retry kept invalidating the first
  signature, so a `setting-change` re-anchor re-armed every ~400 ms forever
  (the per-user still made this visible on every apply). The retry now keeps
  the signature; the first genuinely different enumeration starts a fresh
  proof.
- **The Studio preview survives the screen it runs on disappearing** (Bug B):
  when a screen is unplugged, a projector or a dock is removed, the preview
  now stops its mpv worker with the bounded wait, moves the window to a
  screen that still exists and restarts the preview once the geometry has
  settled. The same path covers a primary-screen change.
- **`compressor --lockframe` no longer fails with EINVAL** while writing the
  temporary JPEG: the temp name keeps the image extension
  (`<stem>.tmp.jpg`, not `<out>.jpg.tmp`) so ffmpeg's image2 muxer can infer
  the format.
- The still helper asks for the **physical primary resolution**
  (`EnumDisplaySettingsW`) instead of the DPI-virtualized `GetSystemMetrics`
  size, so the extracted still matches the real monitor pixels on displays
  with a scale factor other than 100%.

### Added

- **Per-user wallpaper + lock-screen still.** The first frame of the active
  video is extracted to a unique file under `%LOCALAPPDATA%\K6WP\stills` and
  applied as the real Windows desktop wallpaper
  (`SPI_SETDESKWALLPAPER`) and as the lock-screen image (WinRT
  `LockScreen::SetImageFileAsync`) — no HKLM policy, no ACL grant, no
  administrator rights, no UAC. The engine spawns one detached
  below-normal-priority helper (`K6WP.exe --apply-still`) and never waits;
  the helper writes `still_status.json`, which is the only status source the
  Studio reads.
- The previous wallpaper is captured once to a persistent
  `original_wallpaper.json` (our own stills are never captured, J1), and the
  previous lock-screen image + rotating-mode registry values are saved to
  `original_lockscreen.*`, so both can be restored exactly.
- Stills use unique timestamped names and are pruned to the two newest files,
  so the stills directory cannot grow without bound; only one helper runs at
  a time (bounded named mutex), so concurrent applies cannot interleave their
  files or stores.
- Two Studio toggles ("Jadikan frame pertama video sebagai wallpaper
  Windows" / "…sebagai gambar layar kunci"), a one-time onboarding offer
  (default checked), and a read-only legacy-policy notice with one
  "Bersihkan" button.
- A hang watchdog: the message loop publishes a heartbeat, and a stuck
  engine writes a minidump and is restarted (with the existing
  crash-restart backoff, so it cannot loop).
- New suites: `reconcile_policy_test` (the pure reconcile decision core),
  `still_test` (paths, throttle policy, three-field settings, original
  stores, status JSON, legacy cleanup classification) and the hang-detector
  checks in `engine_units_test`. `engine_no_winrt_test` parses the PE import
  table and fails if `engine.exe` ever links WinRT/COM.

### Changed

- `studio_settings.json` is schema **v5**: adds `wallpaper_sync` (default
  true), keeps `lockscreen_sync` with its new per-user meaning (stored
  values preserved), and moves the `lockscreen_offset_sec` default from 1.0
  to 0.0 (only a value of exactly 1.0 — the old hard-coded default — is
  migrated). A fresh install defaults both toggles to true. The engine reads
  exactly three fields of that file (LOW-16).
- The old elevated path is gone: `--elevate-lockscreen on`,
  `GrantK6wpFolderWriteAccess`, `%PROGRAMDATA%\K6WP\lockscreen.jpg` and the
  HKLM policy writes were removed. `--elevate-lockscreen off` remains
  cleanup-only (one UAC, deletes only our own legacy policy/ACL/files) and
  is what the uninstallers call.
- `compressor --lockframe` without `--out` now writes to
  `%LOCALAPPDATA%\K6WP\lockframe.jpg` instead of the old
  `%PROGRAMDATA%\K6WP\lockscreen.jpg` (the still helper always passes an
  explicit unique output path).
- The lock screen is no longer refreshed on lock, suspend or session end:
  the still is applied on boot with a video, on a manual pick, on a
  throttled playlist rotation, and on a Studio apply. Shutdown/logoff never
  restores the original wallpaper (the store stays for the next explicit
  restore); tray Quit and `K6WP.exe --apply-still --restore`
  (`--wallpaper` / `--lockscreen` for one side only) put the originals back.
- The installer and uninstallers are per-user and UAC-free:
  `RequestExecutionLevel user`, `InstallDir $LOCALAPPDATA\K6WP`, HKCU-only
  registry writes, and the uninstall path calls the cleanup-only
  `--elevate-lockscreen off` for legacy residue.

### Known limitations

- Bundles remain unsigned — SmartScreen / AV may warn.
- The still is per-user and applies only while K6WP is running with an active
  video; it is intentionally not refreshed on lock or suspend. Windows can
  re-encode the lock-screen image (size and hash differ from the still file —
  compare visually, not by hash).
- If Windows Spotlight or a rotating lock-screen mode is re-enabled by the
  user or another tool, the lock screen can show something else; the helper
  saves, disables and restores the rotating/slideshow values it changes.
- The still is a single image sized to the primary display (multi-monitor
  phase 1); the physical second-monitor case is still untested, as are older
  Windows builds without the WinRT `LockScreen` API.

## [0.0.1-alpha.2] - 2026-10-17

Second alpha pre-release: display-resilience groundwork plus a studio UI
polish pass. The shadow reconcile pipeline is opt-in
(`K6WP_RECONCILE_MODE=shadow`); the default `legacy` behavior is unchanged.

### Added

- Tiered per-monitor identity (CCD `monitorDevicePath` → EDID from the
  `Enum\DISPLAY` registry → GDI device name) with a stable key that is used
  only when unique in the topology snapshot; ambiguous tiers degrade to the
  GDI fallback instead of guessing. `monitor_dump` emits `stable_key`;
  `displays.json` gains optional additive `key_stable` (per assignment) and
  `key_aliases` (top level) — legacy files round-trip byte-for-byte.
- Shadow reconcile pipeline behind `K6WP_RECONCILE_MODE=shadow`: pure
  topology diff core with property tests, debounced reconcile scheduling
  with bounded retry, coverage verification, and debug lock-order
  assertions.
- Characterization golden tests for the legacy placement path plus
  dedicated suites (`reconcile_golden_test`, `reconcile_red_test`,
  `reconcile_shadow_test`, `lock_order_test`, `stable_key_test`,
  `topology_diff_test`, `topology_property_test`, `retry_schedule_test`).

### Changed

- Studio feedback round: button glyph+label groups are centred again across
  dialogs, video cards and the status rail; popups pin the Material theme so
  dark-mode labels/radio buttons stay readable; the About close action moved
  into the header; the duplicated log block left Settings; the redundant
  monitor selector left the Wallpaper quick panel; the Wallpaper page rail
  no longer absorbs spare width (pinned to its 340px design width).

### Known limitations

- Unchanged from `0.0.1-alpha` (see `packaging/known-limitations.md`).
  Bundles remain unsigned. The shadow reconcile pipeline has been exercised
  on emulated multi-monitor configurations (virtual display driver) only,
  not yet on a physical second monitor.

## [0.0.1-alpha] - 2026-10-10

Initial alpha release. Pre-release software under active development:
expect bugs and breaking changes between updates.

### Included

- Two-process architecture: resident `engine.exe` (desktop video rendering,
  config watch, IPC, tray, battery saver, fullscreen auto-pause) plus
  on-demand Qt 6 QML `studio.exe` (library, compress, settings, live-switch
  over IPC), `compressor.exe` (ffmpeg compress CLI) and the Qt-free
  Win32 launcher `K6WP.exe`.
- Per-monitor wallpapers with Extend-mode support, wallpaper playlist with
  timed rotation, hardware decode with fallback (`d3d11va` → `dxva2` →
  software), single-instance engine, OS-wallpaper save/restore.
- Per-user installer (`K6WP-Setup.exe`, no UAC) and portable ZIP
  (`K6WP-portable-0.0.1-alpha.zip`).

### Known limitations

- See `packaging/known-limitations.md` (engine RAM vs budget, hwdec logging
  gap, hardware variance, unsigned-bundle SmartScreen/AV expectations).
