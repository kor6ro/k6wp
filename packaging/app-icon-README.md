# K6WP app icon

`k6wp-on.ico` and `k6wp-off.ico` are the canonical K6WP app icons. Master
copies live in `brand/icons/`; build copies live in `packaging/` (on the RC
include path, so the `.rc` files reference them by bare name) and `engine/`.

## What they are
- `k6wp-on.ico` — running state: indigo rounded square, white "K6" glyph.
- `k6wp-off.ico` — paused state: amber rounded square, "K6" glyph plus two
  pause bars (visually distinct from the running icon even at 16 px).
- Both are multi-size ICOs (9 entries: 16/24/32/48/64/72/96/128/256 px,
  32bpp, 432254 bytes each).

## How they are used
- `engine/app.rc` embeds `k6wp-on.ico` as resource `IDI_APPICON` (101) and
  `k6wp-off.ico` as `IDI_APPICON_PAUSED` (102) into `engine.exe`.
- `studio/app.rc` and `launcher/app.rc` embed `k6wp-on.ico` as
  `IDI_APPICON` (101).
- `packaging/installer.nsi` uses `k6wp-on.ico` for the installer icon and
  `k6wp-off.ico` for the uninstaller icon.
- `engine/src/tray.cpp` loads them via `LoadImageW` (sized to the system
  small-icon metric, 16 px at 100% DPI / 32 px at 200% DPI). If the resource
  is missing (e.g. a build without the .rc) it falls back to the stock shared
  icons, so the engine always runs.

## Replacing them (no code changes)
1. Overwrite `packaging/k6wp-on.ico` and/or `packaging/k6wp-off.ico`
   (same filenames, any multi-size ICO format), and mirror the change to the
   master copies in `brand/icons/` and the build copy in `engine/`.
2. Rebuild: `cmake --build --preset msvc-dev --target engine`.
3. Restart the engine. No source changes needed.

## History
The original worker-generated placeholders were `app.ico` / `app_paused.ico`
(16/32/48/256 px), produced by `packaging/make_app_icon.py` (Python + Pillow;
`pip install pillow`). They were superseded by the canonical `k6wp-on.ico` /
`k6wp-off.ico` artwork; the old files now live in `attic/packaging-icons/`
(LOW-22: moved out of `packaging/` so the ship dir holds only live assets).