# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

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
