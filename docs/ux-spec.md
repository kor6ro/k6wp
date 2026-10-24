# UX Spec - K6WP Studio (Slint)

## Goal
Studio is a separate process (no mpv, no HWND-embed, no live preview). Replaces Qt Studio
with lower RAM/startup and identical flows. Targets: new install -> wallpaper visible <= 3 steps;
Library -> set wallpaper <= 2 clicks.

## Surfaces
Header (title, version), Library grid (virtual, 200+ items), Settings/QuickSettings,
Onboarding/first-run, Cache dialog, Log panel, Toast, Warning banner, EmptyState, status bar,
AppDialogs, Playlist UI, per-monitor assignment.

## Tokens
See rust/crates/k6wp-studio/ui/design-tokens.slint. Light/dark follow system color scheme;
font Segoe UI Variable; icons inline SVG; reduced-motion honored.

## Thread policy
UI thread performs NO file/network/IPC/registry I/O. All via worker with timeout. Enforced by
tools/lint_ui_thread.ps1 (self-test present).

## Thumbnailer
Decision pending measurement (ADR): ffmpeg.exe subprocess (concurrency <=2, per-frame timeout)
vs IShellItemImageFactory. Contract: location/format per contract-inventory.md so old library
reads. Cache keyed (path,size,mtime); size = display x DPI; LRU total cap; decode only viewport.

## Frame-strip hover
N=8 small frames, lazy worker, loaded only on hover, released on exit.

## Import dialog
Drag-and-drop + file pick. Probe SPS in worker (k6wp-core::sps, fallback trace_headers).
Show extra RAM estimate (reference pool). Warn on 10-bit. HDR (VUI transfer 16/18, primaries 9)
never auto re-encoded. "Optimize (re-encode)" vs "Keep original": default Optimize only for
8-bit SDR above 64MB byte-DPB; default Keep for 10-bit/HDR/unknown.

## Settings
Two-way binding with validation; save via config_store in worker; honors schema v6, preserves
unknown keys. Quality-affecting changes shown clearly, never silent.

## Accessibility
Keyboard full, AccessKit, high contrast, reduced-motion. i18n en+id.
