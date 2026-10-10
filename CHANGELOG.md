# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

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
