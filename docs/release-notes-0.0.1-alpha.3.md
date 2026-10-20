# K6WP 0.0.1-alpha.3

Third alpha pre-release. Pre-release software under active development:
expect bugs and breaking changes between updates.

This release replaces the old elevated lock-screen mechanism with a per-user
**still** helper — the first frame of the active video becomes the real
Windows desktop wallpaper and the lock-screen image with no administrator
rights and no UAC — and fixes the display-resilience regressions found after
`0.0.1-alpha.2`.

## Bug fixes

- **Engine freeze when the monitor count drops** — slot reconciliation no
  longer runs inside the `WM_DISPLAYCHANGE` handler (`SendMessage`), the
  blocking mpv teardown moved to a background worker, and slots follow the
  monitor's stable identity instead of its position index ("tray icon is
  there but nothing responds" / renderers swapping monitors).
- **Second adapter change now detected** — the DXGI adapter-removed watch
  re-arms on a fresh factory after every event, and a device-lost burst
  cancels the pending topology reconcile (one surface rebuild per burst
  instead of a rebuild plus a re-anchor).
- **Topology-door loop after a wallpaper change** — the "wait for two equal
  enumerations" retry kept invalidating the first signature, so a
  `setting-change` re-anchor re-armed every ~400 ms forever; the retry now
  keeps the signature and the first genuinely different enumeration starts a
  fresh proof.
- **Studio preview when a screen disappears** (Bug B) — the preview stops its
  mpv worker with a bounded wait, moves its window to a live screen and
  restarts once the geometry has settled; the same path covers a
  primary-screen change.
- **`compressor --lockframe` EINVAL** — the temporary JPEG keeps the image
  extension (`<stem>.tmp.jpg`, not `<out>.jpg.tmp`) so ffmpeg's image2 muxer
  can infer the format.
- **Still matches real monitor pixels on scaled displays** — the helper asks
  for the physical primary resolution (`EnumDisplaySettingsW`) instead of the
  DPI-virtualized `GetSystemMetrics` size.
- **`wallpaper_surface_live_` re-synced** with the slot census after every
  reconcile; the shadow reconcile pass now diffs against the previous
  snapshot and the coverage census no longer runs under the reconcile mutex.
- The two settings the engine reads are parsed once per event instead of
  twice.

## Added

- Per-user wallpaper + lock-screen still (no HKLM, no ACL grant, no admin, no
  UAC). The engine spawns one detached below-normal-priority helper
  (`K6WP.exe --apply-still`) and never waits; `still_status.json` is the only
  status source the Studio reads.
- Original wallpaper store + original lock-screen image and rotating-mode
  store, with `K6WP.exe --apply-still --restore [--wallpaper|--lockscreen]`
  to put everything back.
- Two Studio toggles ("Jadikan frame pertama video sebagai wallpaper
  Windows" / "…sebagai gambar layar kunci"), a one-time onboarding offer
  (default checked) and a read-only legacy-policy notice with one
  "Bersihkan" button.
- Hang watchdog: the message loop publishes a heartbeat; a stuck engine
  writes a minidump and is restarted with the existing crash-restart
  backoff.
- New suites: `still_test` (paths, throttle policy, settings, stores, status
  JSON, legacy cleanup) and `engine_no_winrt_test` (PE import guard that
  fails if `engine.exe` ever links WinRT/COM).

## Changed

- `studio_settings.json` is schema **v5**: `wallpaper_sync` (default true),
  `lockscreen_sync` keeps its stored value with its new per-user meaning, and
  the `lockscreen_offset_sec` default moves 1.0 → 0.0 with a targeted
  migration (only the old hard-coded 1.0).
- The old elevated path is gone; `--elevate-lockscreen off` is cleanup-only
  (one UAC) and is what the uninstallers call.
- The lock screen is not refreshed on lock, suspend or session end by design;
  tray Quit and `--apply-still --restore` put the originals back.
- `compressor --lockframe` without `--out` now writes to
  `%LOCALAPPDATA%\K6WP\lockframe.jpg`.
- The installer and uninstallers are per-user and UAC-free
  (`RequestExecutionLevel user`, `InstallDir $LOCALAPPDATA\K6WP`, HKCU-only).

## Tags

`bugfix` · `wallpaper` · `lock-screen` · `display-resilience` · `studio` ·
`compressor` · `installer` · `no-uac` · `per-user`

## Verification

- `ctest --preset msvc-dev`: 47/47 suites pass (`still_test` 99 checks,
  `engine_no_winrt_test` 10, `config_test` 271, `studio_logic_test` 550,
  `compress_friendly_error_test` 41).
- Cold start verified with the exact autostart command against the release
  build: `wallpaper_ok=true`, `lockscreen_ok=true`.
- Manual: T1/T3/T4/T7/T8 verified; T2 and T9 partially verified; T5 (Win+L)
  and T6 (true reboot/logoff) remain manual checks on physical hardware.

## Install

- `K6WP-Setup.exe` — per-user installer, no UAC (`%LOCALAPPDATA%\K6WP`).
- `K6WP-portable-0.0.1-alpha.3.zip` — portable bundle.

Bundles remain unsigned — SmartScreen / AV may warn (see
`packaging/known-limitations.md` in the repo).

### SHA256

```
85156F67DCD98D59BD3A73AA4F0A4AD65F160AF53A30E629B8337A49A998D840  K6WP-Setup.exe
471FC17D1C7041F3CAEE44D815B37D55509EA62DC9499F8B023B01159D546219  K6WP-portable-0.0.1-alpha.3.zip
```
