# Parity Checklist - Studio Rust vs Studio Qt

Each item: done + evidence label, or UNPROVEN with reason.

## Library
- [ ] Virtual grid reads same library.json via k6wp-core codec
- [ ] 200 items scroll smooth, no bitmap held off-viewport
- [ ] Thumbnails load lazy, LRU cap, cache keyed (path,size,mtime)
- [ ] Missing/unreachable media shows "not available", no UI freeze (incl UNC 192.0.2.1)
- [ ] Frame-strip hover (8 frames) lazy load/release
- [ ] Undo delete, search + sort

## Import + compress
- [ ] Drag-and-drop + file pick
- [ ] SPS probe in worker with timeout, RAM estimate shown
- [ ] 10-bit warning, HDR never auto re-encoded
- [ ] Optimize vs Keep original defaults per bit-depth
- [ ] Compressor spawn, progress, cancel (process tree), exit codes -> friendly messages
- [ ] --cache-dir explicit arg

## Wallpaper
- [ ] Set from Library via IPC set_video
- [ ] Pause/resume
- [ ] Per-monitor assign (set_display_video, displays.json)
- [ ] display_assignments, display_coverage, degraded, headless
- [ ] Connection policy: no ERROR_PIPE_BUSY, reconnect backoff
- [ ] Adaptive polling (stop when minimized/hidden)

## Playlist
- [ ] Enable/disable, interval, shuffle, drag-reorder
- [ ] Status from get_state, write playlist.json via engine path

## Telemetry
- [ ] hwdec_active + playback_health exposed (additive)
- [ ] Warning with follow-up action on hwdec fallback / rate out of band

## Settings/Onboarding
- [ ] Two-way binding + validation, save via worker, schema v6 preserved
- [ ] Onboarding <= 3 steps
- [ ] Cache dialog, Log panel, Toast, Warning banner, EmptyState, status bar, AppDialogs

## i18n/a11y/DPI
- [ ] en + id
- [ ] Keyboard full, AccessKit, high contrast, reduced-motion
- [ ] Screenshots all pages light/dark x DPI 100/125/150%

## UNPROVEN (hardware/rig)
- second monitor, mixed DPI, HDR, sleep->wake, DPI scaling in live session
