# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

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
- `wallpaper_surface_live_` is re-synced with the slot census after every
  reconcile pass: when the last monitor disappears the headless renderer
  takes the decode over instead of `set_video` being rejected.
- The shadow reconcile pass computes the topology diff against the PREVIOUS
  snapshot (it was always empty), and the coverage census no longer runs
  while the reconcile mutex is held.
- The lock screen is refreshed when the session locks, when the machine
  suspends, when the session ends and on a playlist rotation — not only on
  boot and on a manual video change.
- The OS wallpaper is restored on shutdown/logoff (the `WM_QUERYENDSESSION`
  / `WM_ENDSESSION` handlers were missing).
- The two settings the engine reads are parsed once per event instead of
  twice, and the lock-screen debounce is keyed on the file's content — a
  re-compressed wallpaper with the same path is no longer swallowed.

### Added

- **The lock-screen toggle now works end to end.** It runs
  `K6WP.exe --elevate-lockscreen on|off` and only stays ON when the policy
  really got registered: there is no longer a state "ON in the settings but
  no policy" (the engine used to refresh a picture Windows never showed).
  The first-run wizard offers the feature once (default on), the Studio
  reports the honest Windows-edition support, and the lock-screen frame is
  the LIVE picture on screen (falling back to the file-based extract).
- A hang watchdog: the message loop publishes a heartbeat, and a stuck
  engine writes a minidump and is restarted (with the existing
  crash-restart backoff, so it cannot loop).
- New suites: `reconcile_policy_test` (the pure reconcile decision core),
  `lockscreen_policy_test` (the trigger, elevation and Windows-edition
  policies) and the hang-detector checks in `engine_units_test`.

### Changed

- `studio_settings.json` is schema **v4** (adds the Studio-private
  `lockscreen_offer_shown`). The migration never changes a user's
  `lockscreen_sync` decision: the feature needs an administrator
  registration, so flipping it silently would produce exactly the state
  this release removes. The engine still reads exactly two fields of that
  file (LOW-16 unchanged).

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
