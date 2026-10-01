# K6WP — Lightweight Live-Wallpaper Engine for Windows

K6WP is a lightweight live-wallpaper engine for Windows (C++17, CMake 3.25+, MSVC).
It plays video as your desktop wallpaper with minimal resident overhead.

Two-process architecture:

- **Engine** (`engine.exe`) — resident 24/7. Renders video onto the desktop
  (WorkerW/Progman injection), watches config, serves IPC, handles tray,
  fullscreen auto-pause, and battery saver. Kept as small as possible.
- **Studio** (`studio.exe`) — on-demand Qt 6 Quick/QML (Material) control UI.
  Import videos, manage the library, compress, tweak settings, and live-switch
  the playing video over IPC without restarting the engine.

Plus two helpers: **compressor** (`compressor.exe`, ffmpeg compress CLI) and
**launcher** (`K6WP.exe`, Qt-free Win32 dispatcher / autostart entry point).

## Screenshots

**No screenshots ship with this release.** `docs/img/` does not exist yet, and
this is a known gap rather than a broken link: the Studio UI was rewritten
from Qt Widgets to Qt Quick/QML for 1.2.0, so any screenshot taken of the old
widget layout would be actively misleading. New captures (Studio window with
the three tabs, engine status, the library grid, and desktop playback) are
outstanding work and belong under `docs/img/` once they exist.

## Features

- Hardware decode with fallback (`d3d11va` → `dxva2` → software), `vo=gpu`, loop playback via libmpv.
- Multi-monitor: per-monitor windows, DPI PerMonitorV2, span mode (engine-code-only).
- Fullscreen auto-pause: foreground fullscreen app pauses rendering, resumes after.
- Battery saver: FPS cap (default 30, down to 24 on DC) + restore on AC.
- Auto-compress to monitor resolution: H.264/MP4 (default CRF 22) with LRU output
  cache (5 GiB), skip-optimal copy, encoder auto-detect (NVENC → QSV → AMF → x264).
- Video library: copy + thumbnail cache + JSON metadata index, double-click live-switch.
- Wallpaper playlist: ordered rotation with a configurable interval and optional
  shuffle, edited in Studio and driven by the engine from `playlist.json`
  (freezes while paused; tray "Next Wallpaper" advances within it).
- Tray icon with quick-switch (MRU) and error-status tooltip.
- Single-instance engine (named mutex), OS-wallpaper save/restore on exit,
  config migration with `.bak` self-healing.

## Performance

Current numbers come from `docs/bench_phase4.json` (phase-4 final gate,
2026-09-22), the newest matrix in the repo. Read this table with one caveat
that has not changed since 1.0: `docs/bench_final.json` and
`docs/bench_cpu_mem.json` (fase-6 gate, 2026-09-16) measured the **skeleton
idle loop** (no video, no mpv instance), so their CPU and GPU rows are harness
artifacts rather than render load. That is why the idle-CPU row below now
reads 0.02% against the same metric those two files read 37.6% and 63.5%
against: the phase-2 event-driven engine replaced the hot
`PeekMessage` + `Sleep(10)` loop those files sampled, not a measurement
accident. Both older figures are kept in the row so a stale bug report can be
recognised as stale.

Real playback numbers (1080p H.264 actually playing, 1 min) live in
`docs/bench_idle_video_1min.json` and `docs/bench_phase2.json` /
`bench_phase3.json` / `bench_phase4.json`; honest analysis in
`packaging/known-limitations.md` §§3, 3a, 7, 8.

| Metric | Budget | Measured | Verdict |
|---|---|---|---|
| Cold start (pipe-ack + settle) | < 2000 ms | median 116.5 ms (260.8 / 116.5 / 111.7 ms, n=3, `bench_final.json`); a separate same-day 2-run measurement in `bench_startup.json` read median 169.4 ms | PASS |
| Engine RAM, skeleton idle (no video) | < 80 MB | avg 24.16 MB, peak 24.21 MB private bytes (`bench_phase4.json`); the 15.8 MB / 15.9 MB in `bench_fase1.json` is the older 2026-09-16 reading, since superseded | PASS |
| Engine CPU, skeleton idle (no video) | < 2% | avg 0.02%, peak 0.19% over 16 cores (`bench_phase4.json`, gate < 0.3). Superseded earlier reading: 37.6% (5-min) / 63.5% (1-min) in `bench_fase1.json` + `bench_cpu_mem.json`, measured against the old `PeekMessage` + `Sleep(10)` hot idle loop | PASS |
| GPU @1080p H.264 hwdec | < 5% | **still no percentage measurement.** What is on record: the dGPU is not decoding (RTX dec/sm/enc 0% over 3 samples during hwdec-active playback, `bench_phase3.json`), and the only real GPU-3D figure, ~55%, came from a Debug dual-decoder run that is not a 1080p single-stream hwdec measurement (`known-limitations.md` §3) | NOT MEASURED |
| Compress (NVENC, 720p30, CRF 23) | < 30 s | anime 4.10 s / gaming 7.06 s / slideshow 2.03 s (`bench_phase4.json`; the 2026-09-16 `bench_compress.json` run read 5.06 / 10.11 / 3.03 s) | PASS |
| Playback, 1080p H.264 (1 min) | < 2% CPU, 80 MB RAM | CPU 0.01% avg / 0.19% peak over 16 cores with E-core + iGPU pin (`bench_phase4.json`); RAM ~150 MB private bytes, of which `PeakWorkingSet64` 158.92 MB. The separate 2026-09-20 run in `bench_idle_video_1min.json` read 0.42% CPU and ~220 MB working set (avg private 222.6 MB) | CPU PASS / RAM over budget (see `known-limitations.md` §7 for the 80 MB miss and §3a for the 1080p <= 220 MB clamp-based budget) |

The private-bytes / working-set split matters if you read the raw JSON: the
one-shot `SetProcessWorkingSetSize` trim collapses resident working set to
~2.9 MB during playback, and that is a residency artifact, not a saving.
Private bytes are the honest cost metric. See `known-limitations.md` §10.

Reproduce: `tools/bench_startup.ps1`, `tools/bench_cpu_mem.ps1`, `tools/bench_compress.ps1`.
Corpus is generated locally (`tools/make_corpus.ps1`) and never committed.

## Install

Requirements: Windows 10 1703+ (PerMonitorV2) / Windows 11, 64-bit.

**Option A — portable ZIP:**

1. Download `K6WP-portable-<version>.zip` from Releases.
2. Extract to any folder.
3. Run `K6WP.exe` (the launcher starts the engine; Studio opens on demand).
4. Optional: `uninstall.bat` in the folder removes the autostart entry and,
   on confirmation, `%LOCALAPPDATA%\K6WP` data. Delete the folder to finish.

**Option B — installer:**

1. Download `K6WP-Setup.exe` from Releases.
2. Run it — per-user install, no UAC prompt, installs to `%LOCALAPPDATA%\K6WP`.
3. Optional autostart checkbox (default on), Start Menu / Desktop shortcuts.
4. Uninstall via Apps → K6WP → Uninstall (offers to keep or remove your data;
   restores your previous OS wallpaper).

> The installer is not a prebuilt binary checked into the repo. It is compiled
> from `packaging/installer.nsi` with NSIS (`makensis`, e.g.
> `makensis packaging\installer.nsi` from the repo root) and written to
> `dist\K6WP-Setup.exe` as part of the release process, which also runs
> `packaging\make_zip.ps1` for the portable ZIP. Both get the version from
> `packaging/version.nsh`, which CMake generates from the root
> `CMakeLists.txt` `K6WP_VERSION`, so a stale `K6WP-Setup.exe` on your disk is
> a stale build rather than a stale source. If the Release page for this
> version has no installer attached yet, Option A is the working path.

> Unsigned build: expect a Windows SmartScreen "Unknown publisher" prompt on
> first run. See `packaging/known-limitations.md` §4.

## Build from source

Prerequisites:

- Visual Studio 2022+ (MSVC x64, BuildTools work).
- Qt 6.8.x `msvc2022_64` — set the `QT_ROOT` env var, e.g.
  `$env:QT_ROOT = "C:\Qt\6.8.3\msvc2022_64"` (the presets read `$env{QT_ROOT}`).
- CMake 3.25+.
- 7-Zip (only to unpack the libmpv dev package).
- Vendored deps are **fetched, not committed** (ffmpeg 8.1.2, libmpv
  0.41.0-dev, nlohmann/json 3.11.3 total ~311 MB, and `libmpv-2.dll` alone is
  over GitHub's 100 MB per-file limit). Run once after cloning:

  ```powershell
  .\tools\fetch_vendor.ps1
  ```

  It verifies every file against the SHA256s in `vendor/VERSIONS.md` and
  refuses to install on mismatch. CI runs it automatically.

  Two things *are* committed: `vendor/VERSIONS.md` (the pin manifest) and
  `vendor/vc_runtime/` (app-local MS CRT), plus `vendor/libmpv/lib/mpv.lib`,
  which is generated locally from the DLL's own exports and is not
  downloadable.

```powershell
# Dev build + tests
cmake --preset msvc-dev
cmake --build --preset msvc-dev
ctest --preset msvc-dev

# Release build (no test targets by design)
cmake --preset release
cmake --build --preset release
```

Unit tests (`BUILD_TESTING=ON`, msvc-dev preset only): **22 CTest suites**,
all of which must exit 0. The authoritative list is the `add_test(NAME ...)`
calls in each module's `CMakeLists.txt`:

- `shared/CMakeLists.txt` (5): `config_test` (config schema
  load/migrate/validate, incl. the v0→v5 chain and a schema-doc sync guard),
  `ipc_test` (NDJSON protocol round-trips), `sha1_test` (RFC 3174 vectors),
  `proc_util_test` (the `RunCaptured` subprocess helper), `playlist_test`
  (playlist.json io/validation/migration + rotation helpers).
- `engine/CMakeLists.txt` (5): `occlusion_test` (occlusion_watch pure
  helpers), `ipc_marshal_test` (the PostMessage marshal payload),
  `gpu_pin_test` (the gpu_pin VendorId table), `tray_menu_test` (tray popup
  construction, incl. USER-handle balance), `crash_dump_prune_test` (the
  crash-dump retention cap).
- `studio/CMakeLists.txt` (6): `studio_async_test` (offscreen Qt proof that
  IPC/engine waits do not block the GUI thread), `thumbnailer_test`,
  `library_crud_test` (LibraryManager metadata-index CRUD plus
  `ReferenceInPlace` and the shared-manager contract), `studio_logic_test`
  (Studio non-GUI logic, incl. `IsFirstRunCondition` and the CompressBridge
  no-result contract; needs the vendored ffmpeg/ffprobe and a local
  `compressor.exe`), `fake_pipe_test` (in-process fake IPC server),
  `playlist_bridge_test` (PlaylistBridge CRUD/persist round-trip).
- `compressor/CMakeLists.txt` (4): `probe_json_test` (the single-JSON ffprobe
  parser), `compress_argv_contract` (golden Studio↔compressor argv dry-run,
  plus the in-place-compress guard), `compress_friendly_error` (the
  technical→friendly error mapping), `widen_utf8_test` (the production
  `Widen()` UTF-8 decode).
- `launcher/CMakeLists.txt` (2): `lockscreen_backup_test` (launcher
  lockscreen backup UTF-8 round-trip), `launcher_stop_test` (the uninstaller
  stop path: own-image matching and the elevated-restore outcome mapping).

The Qt-based suites run under `QT_QPA_PLATFORM=offscreen`, so `ctest` works
headless. Run them all with `ctest --preset msvc-dev` (or the exes directly
in `build/msvc-dev/`).

Release packaging: `packaging\make_zip.ps1` (asserts version consistency +
ZIP↔installer file-set sync + import closure + 250 MB limit) and
`packaging\installer.nsi` (makensis → `dist\K6WP-Setup.exe`).

## Architecture

Condensed from `repomix.md` Part 1 (full dump) — contracts in
`docs/dev-contracts.md` (sources of truth are the code files named there).

```text
┌──────────────────────────────┐   Named pipe    ┌─────────────────────────────┐
│ ENGINE (resident 24/7)       │◄───────────────►│ STUDIO (on-demand)          │
│ C++17 + libmpv               │  \\.\pipe\k6wp- │ C++17 + Qt6 Quick/QML      │
│ desktop injection            │      engine     │ import / library / compress │
│ per-monitor render slots     │                 │ settings / live-switch      │
│ config watcher (500 ms poll) │                 │ single shared IpcClient     │
│ IPC server: set_video,       │                 │                             │
│   pause/resume, set_monitor, │                 │                             │
│   get_state                  │                 │                             │
│ tray + fullscreen watch      │                 │                             │
│ power saver + re-anchor      │                 │                             │
└──────────────────────────────┘                 └─────────────────────────────┘
            │                                                  │
            └──────────────► config.json ◄─────────────────────┘
              %LOCALAPPDATA%\K6WP\config.json (engine playback state)
              studio_settings.json (Studio/compressor prefs, separate file)
```

- `shared/` — static lib `k6wp_shared`: versioned config schema + migration,
  NDJSON IPC protocol (`{"version":1,"cmd":…,"payload":{…}}\n`, 64 KiB cap),
  monitor/DPI utilities, autostart (HKCU Run) helpers.
- `engine/` — `EngineApp` message loop owns renderer, injector, watchers, IPC
  server, tray, power saver. Hot-swaps video without restart.
- `studio/` — Material QML shell (Wallpaper / Kompresor / Pengaturan) with
  drag-and-drop import, thumbnail + metadata cache, compress queue with live
  progress, and a library grid over a `QAbstractListModel`. The C++ backends
  stay pure logic and reach QML through four bridges: `StudioBridge` (engine
  status, apply, pause/resume), `CompressBridge` (owns a `CompressController` by
  value), `SettingsBridge` (studio + engine settings) and `LibraryGridModel`
  (contains `LibraryManager`'s throwing API). The mpv preview remains a native
  child widget: the vendored libmpv has no D3D11 render backend and the OpenGL
  path drops `hwdec` to software, so `vo=gpu` + `d3d11va` is only reachable
  through `wid`/`winId()`.
- `compressor/` — ffmpeg subprocess runner (progress + cancel), encoder
  auto-detect, LRU cache, friendly errors.
- `launcher/` — Qt-free Win32 `K6WP.exe`: singleton + engine dispatch
  (`--engine --silent` for autostart).
- `packaging/` — `make_zip.ps1` + `installer.nsi` (kept in sync by build asserts).

Key protocols: IPC NDJSON v1 over `\\.\pipe\k6wp-engine` (current-user-only ACL,
2 s ack deadline, 1 retry on pipe-drop); config v2 with `.bak` backup on
migration or corrupt input; canonical silent flags `--minimized` (engine) /
`--engine --silent` (launcher). Details: `docs/dev-contracts.md`,
`docs/dev-test-flags.md`.

## Privacy

K6WP collects nothing: no telemetry, no accounts, no data leaves your machine
except the optional update check below.

Update check (Studio only, on by default, disableable in Settings → Lanjutan):
once per Studio start (plus manual Help → "Check for updates") Studio sends a
single HTTPS GET to api.github.com (GitHub Releases API), compares the
latest release tag against its own version, and — only when newer — shows a
non-intrusive "new version available" indicator (status bar + About dialog)
with an "Open release page" button. Nothing is ever downloaded or installed
automatically, network failures are silent, and everything else — playback,
library, compression, settings — works fully offline.

## License

GPL-2.0-or-later — see [`LICENSE`](LICENSE).
Third-party components (libmpv/ffmpeg binaries, Qt runtime DLLs,
nlohmann/json header) remain under their own licenses; see
[`LICENSES/`](LICENSES/).

## Support development

- Support development: [Ko-fi](https://ko-fi.com/kor6ro)
- Project page: [GitHub](https://github.com/kor6ro/k6wp)
