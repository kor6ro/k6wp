# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.0.1-alpha] - 2026-10-10

Honest reset: the software is still buggy, so the `v1.2.0-beta` through
`v1.3.0-beta.3` tags and their GitHub Releases were withdrawn and versioning
restarts at `0.0.1-alpha` (tag `v0.0.1-alpha`). No code change in this entry:
`K6WP_VERSION` is now `"0.0.1-alpha"` (numeric spots `0.0.1.0`), `main` takes
`fix/1.3.0-beta.3-audit` with newest-code-wins, and the old `1.x-beta`
sections below are kept as history of the withdrawn line.

## [1.3.0-beta.3] - 2026-10-10

Second audit-remediation release. All findings from the 1.3.0-beta.2 audit
follow-up landed on `fix/1.3.0-beta.3-audit`, verified by a green
`ctest --preset msvc-dev` (33/33 suites, no MSVC warnings). No behavior
contract changed: IPC wire format, config schema (v5), displays.json schema
(v1) and playlist schema (v1) are untouched.

### Fixed

- **An OS wallpaper change during a session is no longer clobbered on exit
  (audit E-01).** `OsWallpaperGuard::Save()` snapshotted the wallpaper once at
  boot; `WM_SETTINGCHANGE (SPI_SETDESKWALLPAPER)` only re-anchored the live
  surface, so quitting after the user picked a new wallpaper in Windows
  Settings restored the stale path over their change. The snapshot now
  re-saves whenever the OS reports a different desktop wallpaper (the engine
  never writes the desktop wallpaper itself, so any change is external).
- **Re-anchoring no longer leaks shell WorkerW windows (audit E-02).** Both
  the 24H2 and the classic Strategy-B attach passes sent the shell `0x052C`
  "spawn WorkerW" message on every pass (Init, display change, TaskbarCreated,
  resume) and then bound to the *oldest* empty Progman-child WorkerW — the
  freshly spawned one was never used or destroyed. `ResolveSharedHost` now
  reuses an existing empty WorkerW and only spawns when none exists.
- **The hwdec fallback chain re-arms after hardware decoding recovers
  (audit E-03).** `hwdec_stage_` advanced one link per fallback but never
  reset when `hwdec-current` reported a working decoder again, so one
  transient "no" reading (e.g. after a device loss) permanently consumed the
  chain and a later loss left the wallpaper black. A healthy hwdec reading
  now resets the chain; the write-only `hwdec_fallback_attempted_` atomic
  was removed.
- **Real device-lost recovery (audit E-04).** `OnDeviceLost()` was reachable
  only from the test simulator. A new `DeviceLostWatch` registers
  `IDXGIFactory7::RegisterAdaptersChangedEvent` (verified against the
  installed SDK; degrades to a logged no-op on older OS) and joins the engine
  loop wait; `RecreateDevice()` now rebuilds the live desktop surface
  (re-resolve host, re-attach slots, re-apply boot assignments) instead of
  only re-issuing `loadfile`.
- **`get_state` no longer hits the disk on every poll (audit E-06).** The IPC
  worker read `displays.json` through `LoadDisplays()` on every Studio poll;
  it now reads a mutex-guarded snapshot maintained by the main loop at every
  assignment change point.
- **The hwdec-change handler snapshots the renderer through
  `AcquireRenderer()` (audit E-07).** `kMpvHwdecChangeMessage` dereferenced
  the `renderer_` member directly, violating the documented worker-visible
  state discipline (benign today, a race waiting for a future writer).
- **Span-mode re-anchor uses the freshly measured virtual-screen origin
  (audit E-08).** `Reanchor()` passed the stored `slot.info.x/y` instead of
  the just-measured geometry, so a shifted virtual screen after sleep/resume
  re-placed the span window at the old origin.

### Added

- **Crash-restart backoff for the WER auto-restart (audit L-01).** Pure
  `CrashRestartAdvance`/`CrashRestartAllowed` state machine
  (`engine/src/crash_restart_guard.{hpp,cpp}`) backed by a small counter file
  at `%LOCALAPPDATA%\K6WP\crash_restart.txt`: after 3 automatic restarts
  inside a 10-minute window the engine gives up with an engine.log line
  instead of crash-looping at every logon; 300 s of clean uptime clears the
  counter. Covered by new checks in `engine_units_test`.
- **`.clang-tidy` configuration (audit C-04).** Maintainer static-analysis
  config (bugprone/performance/modernize/readability subset) at the repo
  root; not wired into CI. Documented in `CONTRIBUTING.md`.

### Changed

- **Engine tray is Indonesian and jargon-free (audit E-05).** Menu entries
  ("Buka K6WP", "Jeda", "Lanjut", "Wallpaper berikutnya", "Pindah cepat",
  "Dukung pengembang", "Keluar") and tooltips now match the 1.3.0 Studio
  de-jargon direction; the error tooltip says "Wallpaper tidak tampil"
  instead of leaking "headless".
- **CI hardening (audit C-01/C-02/C-03).** Actions pinned to commit SHAs
  (`actions/checkout`, `jurplel/install-qt-action`,
  `actions/upload-artifact`), an explicit least-privilege
  `permissions: contents: read` block, and a version-pinned Chocolatey NSIS
  install.
- **Studio i18n completion (audit S-01).** The remaining user-visible strings
  in `StatusBar.qml` and the C++ bridges (`StudioBridge`, `CompressBridge`,
  `SettingsBridge`, `PlaylistBridge`, `LibraryGridModel`, `CacheDirPolicy`,
  `CompressError`) are now inside `qsTr`/`tr`/`QCoreApplication::translate`
  with a matching literal context, and `studio/i18n/studio_en.ts` carries
  the new messages. Diagnostics-only log lines stay untranslated on purpose.

### Docs

- README "Key protocols" corrected: the IPC pipe is per-session
  (`\\.\pipe\k6wp-engine-<session_id>`), the config schema is v5 (audit
  E-09).
- `docs/dev-contracts.md` documents the ConfigWatcher second-file limitation:
  `displays.json` is watched only when it lives in the same directory as the
  active config (no poll fallback otherwise) (audit E-10).
- `docs/manual-test.md` gained manual verification steps for E-01..E-05 and
  L-01 (wallpaper change survival, WorkerW census across re-anchors, hwdec
  re-arm, adapter-removed log line, crash-restart give-up).

## [1.3.0-beta.2] - 2026-10-10

Audit-remediation release. Six fixes landed on `fix/1.3.0-beta.2-audit`,
each verified by a green `ctest --preset msvc-dev` (33/33 suites) and, for
the installer, a local `makensis` compile. No behavior contract changed:
IPC wire format, config schema and displays.json schema are untouched.

### Fixed

- **Battery-saver "cap24" caps decode instead of freezing it (audit H3).**
  On DC power with `battery_mode=cap24` the engine set the `kPausePower`
  pause bit, which froze every visible slot (last frame held), while the
  24 fps cap was applied only to the usually-idle headless renderer — the
  log then claimed "capping to 24fps" over a frozen wallpaper. The cap now
  fans out to every decoder through the new `MultiMonitor::ApplyFpsCapAll`
  (the slots own the visible decode, and every slot renderer has the same
  `SetFpsCap` API as the headless one), and `kPausePower` is reserved for
  the explicit `static` mode. The decision itself is the new pure helper
  `k6wp::DecidePowerCapAction` (`engine/src/power.{hpp,cpp}`), unit-tested
  in `engine_units_test`. A mode flip (static → cap24) while paused on DC
  resumes immediately.
- **IPC worker thread no longer races the engine's slot map (audit H1).**
  `pause`/`resume` (→ `ApplyPauseState` → `PauseAll`/`ResumeAll`) and
  `get_state` (→ `BuildStateJson`) run on the IPC worker thread, while the
  main loop mutates the same `MultiMonitor::slots_` map (set_monitor
  executor, `OnDisplayChange`, `Reanchor`). Map iteration concurrent with
  erase/insert is undefined behavior. `slots_` is now guarded by a
  recursive mutex (`MultiMonitor::slots_mutex_`; recursive because public
  methods nest, and a single lock cannot invert order with anything else),
  and the worker's `get_state` reader goes through the new snapshot API
  `MultiMonitor::SnapshotSlots()` instead of iterating the map directly.
- **Installer upgrades no longer break a running installation (audit H2).**
  Installing over the resident engine failed at the first locked exe
  (`File` extraction) and the rollback then deleted the whole staged set —
  including the previous installation's files. SEC_APP now stops this
  install dir's processes first (`K6WP.exe --stop`, the same scoped helper
  the uninstaller uses) and takes a per-file pre-existence census;
  `RollbackPartial` deletes only files the run actually created, so a
  failed upgrade leaves the previous version installed and uninstallable.
  The root `*.dll` wildcard deletion is gone (leftover versioned DLLs are
  inert next to the old exes).
- **hwdec fallback now chains to software (audit M3).** The fallback was
  one-shot at `dxva2`, so a machine with neither `d3d11va` nor `dxva2`
  stayed black. `MpvRenderer::TryFallbackHwdec` advances the documented
  chain (`d3d11va → dxva2 → software`) one link per observation via the
  new `hwdec_stage_` counter, and logs each link.
- **Compress cancel cleans up after the hard kill (audit M1).**
  `CompressService::CancelCurrent` used `QProcess::kill()`
  (TerminateProcess), which bypasses compressor.exe's own partial-output
  cleanup entirely — every cancelled job left its `_k6wp.mp4` partial
  behind while the log claimed "cleaned up by compressor.exe". Studio now
  removes the partial itself (guarded: never when the output resolves to
  the input file), and the log states which side did the cleanup. The
  normal-failure path (compressor exits non-zero on its own) still relies
  on the compressor's own `remove_partial()`, as before.
- **Docs: test-suite inventory matches CMake (audit M4).** README said
  "24 CTest suites" and the 1.3.0-beta.1 changelog said "32"; the actual
  registered count is **33** (shared 7 / engine 13 / studio 7 / compressor
  4 / launcher 2). The README breakdown now lists every suite.

### Changed

- `playlist_.Reload()` in the engine loop is throttled to 500 ms (the
  ConfigWatcher poll cadence): the loop wakes early on posted messages,
  and the reload stats `playlist.json` on every wake (audit L1).

## [1.3.0-beta.1] - 2026-10-08

> **Beta release.** This is an explicit pre-release. The `v1.2.0` and `v1.2.1`
> tags were originally published without a beta marker even though they
> shipped beta-grade software; they have since been re-pointed to
> `v1.2.0-beta` / `v1.2.1-beta` and their GitHub releases are now marked
> pre-release (old deep links to the un-suffixed tag URLs no longer resolve;
> the asset filenames did not change). The `1.3.0` line is marked pre-release
> from the start. One headline claim is not yet live-proven: the
> multi-monitor **Extend** placement fix is covered by the automated suites,
> but the plan records the live two-monitor acceptance as UNPROVEN until the
> third-party rig run-book bundle comes back. Treat Extend-mode support as
> unverified until then.

### Added

- **Multi-monitor Extend-mode support.** Before this, a live wallpaper on two or
  more monitors only worked when Windows was set to **Duplicate**; in **Extend**
  mode the extra screens stayed blank. The injection path is fixed and each
  monitor can now carry its own wallpaper.
  - **Per-monitor placement fix** (`engine/src/desktop_placement.{hpp,cpp}`,
    `engine/src/desktop_inject.cpp`): the injector resolves one shared host per
    attach pass instead of spawning one per slot, converts screen coordinates
    with `MapWindowPoints` instead of hand-rolled subtraction, uses a single
    z-order contract across the 24H2 and classic branches, and verifies the
    final child rect actually covers its monitor. A false success now degrades
    to the existing headless indicator instead of a silent blank screen.
  - **Per-monitor wallpaper assignments** (`shared/displays_schema.{hpp,cpp}`):
    an assignment map persisted at `%LOCALAPPDATA%\K6WP\displays.json`, keyed by
    the GDI device name (`\\.\DISPLAYn`), with the same atomic write and
    `.bak`-on-corrupt contract as the other settings files. An additive IPC
    command plus a `get_state` capability field expose it, and the engine
    watches the file without adding a periodic timer. An empty map keeps the
    previous single-video behaviour, so single-monitor users see no change.
  - **Studio monitor sub-tabs with click-to-assign.** Studio lists the monitors
    as sub-tabs; select a saved library or playlist video and click a monitor to
    assign it there. The earlier read-only display canvas and drag-to-assign
    were dropped during execution after live testing showed a cross-tab drag
    disappears under the native preview window.
  - **Two-monitor run-book** (`docs/runbook-2monitor.md`,
    `tools/run_2monitor_evidence.ps1`): a non-author can collect the live proof
    bundle in one command, the extended `monitor_dump` JSON, the `placement:`
    engine-log lines, an injected-window rect census, and one screenshot per
    display mode. Until that bundle is pasted back, the Extend fix stays
    UNPROVEN (see the beta note above).
  - Assignments are keyed on `\\.\DISPLAYn`, which is not hardware-stable: a
    dock/undock or port change can renumber the key and require re-assignment,
    and a Duplicate-mode identity collision is detected and refused rather than
    silently overwriting. HDR and colour handling are out of scope and recorded
    as a documented limitation.
- **Studio UI/UX redesign.** The Studio front end is rebuilt around one page and
  plain-language copy, so a non-technical user can pick a video and press
  **Pasang** without learning tabs or jargon.
  - **One-page Beranda.** A landing page with the library gallery and a large
    preview. Cards are select-then-**Pasang**; the separate Kompresor tab and
    the "Tandai" mode are gone, and the technical controls move under
    **Lanjutan**. A "Pasang ke" choice appears only when more than one monitor
    is present.
  - **Material icon pack** replaces the ad-hoc glyphs across cards, menus, the
    sidebar and dialogs.
  - **De-jargon friendly status and copy.** "Engine aktif" becomes "Wallpaper
    aktif", the separate pause and resume controls collapse into one
    **Jeda / Lanjut** button, and status and error strings say what happened
    plus what to do, without CRF / FPS / encoder / IPC / pid / HWND terms. The
    raw details stay reachable through an "Info teknis" dialog with a
    "Salin untuk dukungan" button.
  - **Collapsible sidebar and adaptive Pengaturan.** The sidebar collapses to an
    icon rail at narrow widths, and Settings is grouped into Umum / Tampilan /
    Hemat daya / Lanjutan with a Performa preset (Hemat / Seimbang / Maksimal).
  - **Background auto-compress.** Large or long imports are prepared
    automatically in the background ("Siapkan video otomatis", default on) with
    a friendly progress state; the long-video consent dialog is kept.
  - **Optional tray icon.** A "Tutup ke tray" setting (default off) keeps Studio
    running when the window closes, with a Buka / Jeda / Lanjut / Keluar menu.
    The engine keeps its own separate tray icon.
  - **Theming.** A `Theme.qml` token singleton defines light and dark palettes
    that follow the system, with shared spacing, radius and type tokens, and
    dialogs drawn from the same tokens.
- **Wallpaper playlist + timed rotation.** The engine can now cycle through an
  ordered list of wallpapers on a fixed interval, with optional shuffle.
  - New `shared/playlist.{hpp,cpp}` (`PlaylistConfig`, `LoadPlaylist`,
    `SavePlaylist`, `ValidatePlaylist`, `MigratePlaylist`, plus the pure
    `SelectNextIndex` / `PlaylistIndexForPath` helpers), schema v1, stored at
    `%LOCALAPPDATA%\K6WP\playlist.json` — deliberately NOT in `config.json`, so
    an older Studio build (which rewrites `config.json` from its own
    `WallpaperConfig` struct) can never delete the playlist. Same atomic
    `.tmp` + `MoveFileExW` publish and `.bak`-on-corrupt contract as the other
    settings files.
  - Engine (`engine/src/engine_app.{hpp,cpp}`): `MaybeReloadPlaylist()` watches
    the file (mtime/size, mirroring `ConfigWatcher`) and `FireRotation()` reuses
    the existing validated `set_video` path. Rotation is gated on
    `!SlotsPaused()`, so it freezes while paused (loop wait is INFINITE) and the
    interval restarts on resume; it re-arms after a device-lost reload. It never
    feeds the tray MRU and never re-fires the lock-screen frame extract, and it
    logs at `Log` (not `LogImportant`) to avoid log churn. Tray "Next Wallpaper"
    advances within the playlist when one is active. `get_state` gains the
    additive fields `playlist_enabled`, `playlist_size`, `playlist_index`.
  - Studio: new `Playlist` QML singleton (`studio/src/playlist_bridge.{hpp,cpp}`)
    and a "Daftar putar" panel on the Wallpaper tab (enable, interval, shuffle,
    add/remove/reorder/clear).
  - Tests: `playlist_test` (43 checks) and `playlist_bridge_test` (26 checks).
  - Packaging: `packaging/playlist.json.example` staged in the ZIP and
    installer; stale `packaging/config.json.example` refreshed to schema v5.

- **Multi-monitor placement fix, per-monitor video assignment, and a Studio
  Display panel.** Under Windows Extend a second monitor could stay blank (or
  one window could cover both) because the placement path assumed the host
  window spanned the monitor. The injector now measures the host, converts
  coordinates through Windows, verifies that the child actually covers its
  monitor, and only then reports success. On top of that fix a saved video can
  be assigned to one monitor and survives a restart; a live edit re-applies
  without restarting the engine; and a monitor that disappears degrades to the
  global video instead of going blank. The two-monitor live proof is NOT
  claimed here: it still needs the run-book artifact from a real rig.
  - Shared (`shared/displays_schema.{hpp,cpp,json}`, `shared/monitor_util.*`,
    `shared/monitor_dump.cpp`): new `displays.json` (schema v1, draft-07,
    `kDisplaysSchemaVersion`) stores a per-monitor `assignments` map keyed by
    the GDI device name (`\\.\DISPLAY1`) as `{"path":...,"exists":...}`, with
    `LoadDisplays` / `SaveDisplays` / `ValidateDisplays` / `MigrateDisplays`,
    the same atomic `.tmp` + `MoveFileExW` publish and `.bak`-on-corrupt
    contract as the other settings files, and a `DetectKeyCollision` guard for
    Duplicate/clone mode. `MonitorInfo` gained orientation, refresh rate and
    per-monitor DPI, and `monitor_dump` now emits the full geometry (`x`, `y`,
    `device_name`, orientation, DPI) through a testable formatter instead of
    only `id/width/height/is_primary`.
  - Engine (`engine/src/desktop_inject.{hpp,cpp}`,
    `engine/src/desktop_placement.{hpp,cpp}`,
    `engine/src/multi_monitor.{hpp,cpp}`, `engine/src/engine_app.cpp`):
    placement measures the shared host once, converts child coordinates with
    `MapWindowPoints` instead of hand-rolled arithmetic, refuses to attach to
    an arbitrary non-spanning `WorkerW`, and applies ONE z-order contract
    across the `Progman` and `WorkerW` branches (the 24H2 path parented a level
    too deep and sent the child to the wrong sibling). Every attach logs a
    stable `placement:` line and coverage is checked against the child's
    measured screen rect, so a clipped or out-of-bounds window is logged as
    `RETRY-FALSE-SUCCESS` and retried rather than counted as covered. Per
    monitor assignment is served by the additive `set_display_video` IPC
    command (`{"device":...,"path":...}` to assign, `{"device":...,"clear":true}`
    to drop), marshalled onto the main loop and persisted through
    `SaveDisplays`. `displays.json` is watched through
    `ConfigWatcher::WatchSecondFile` with event-only semantics, so it adds no
    periodic wakeup; on boot the engine re-converges slots, keeps assignments
    whose device name survives, and logs a retention warning with a fall back
    to the global video for a re-keyed monitor. `get_state` gains the additive
    fields `display_capability`, `display_assignments` and `display_coverage`.
  - Studio (`studio/qml/Main.qml`, `studio/qml/DisplayCanvas.qml`): a new
    **Display** tab hosts a read-only `DisplayCanvas` that draws each monitor
    as a correctly-proportioned rectangle at its real virtual-desktop position,
    marks the primary, and shows per-monitor scale, orientation and resolution
    as Windows reports them. Library rows and playlist rows are drag sources
    (custom `application/x-k6wp-assignment` MIME key) and each monitor
    rectangle is a `DropArea`, so a saved video can be dragged onto exactly one
    monitor. Scale and orientation are a readout, never a control.
  - Tests: new CTest suites `displays_schema_test` (73 checks; io, migration,
    validation, the `.bak` contract and `DetectKeyCollision`),
    `monitor_util_test` (41 checks), `desktop_placement_test` (16 checks;
    coverage verdicts), `desktop_zorder_test` (16 checks), `workerw_span_test`
    (3 checks), `multi_monitor_factory_test` (106 checks; the injectable
    `SlotFactory` seam), `multi_monitor_placement_test` (86 checks; negative
    origins, portrait, shared host) and `display_assignment_test` (38 checks;
    assignment lifecycle and legacy regression). `ipc_test`,
    `ipc_marshal_test`, `engine_state_test` and `engine_units_test` gained the
    new-protocol and second-file-watcher cases.
  - Packaging (`docs/runbook-2monitor.md`, `tools/run_2monitor_evidence.ps1`):
    a one-command run-book lets a non-developer collect the two-monitor
    evidence bundle (`monitor_dump.json`, `placement.log`,
    `injected_windows.json`, one screenshot per display mode and the rig's
    Windows build). `docs/dev-contracts.md` now documents `displays.json` and
    `set_display_video`, and `packaging/known-limitations.md` records the
    corrected multi-monitor provenance plus the HDR, duplicate-mode and keying
    limits.

- **The Tampilan canvas tab is replaced by per-monitor sub-tabs in the
  Wallpaper view, and assignment is now click-to-assign.** The fourth Studio
  tab added in the previous entry is gone: its read-only canvas duplicated the
  wallpaper preview and split assignment across two tabs. Each connected
  monitor now gets a sub-tab in the Wallpaper view that scopes both the preview
  and the assignment target to that monitor.
  - Removed (`studio/qml/Main.qml`, `studio/CMakeLists.txt`): the **Tampilan**
    tab, `studio/qml/DisplayCanvas.qml`, and the `display_canvas_test` CTest
    suite. The drag sources and per-rect `DropArea` targets went with the
    canvas: cross-tab drag was impossible once the sources and the canvas lived
    on different tabs, and a dragged delegate vanished under the native
    preview. The contract-test inventory drops back to 32 suites.
  - Assignment (`studio/qml/Main.qml`): click-to-assign. Arming a library or
    playlist item, clicking a monitor sub-tab while armed sends the existing
    `set_display_video` command, scoped to the selected
    sub-tab so only the chosen monitor's picture changes. Clearing uses the same
    `set_display_video` clear flag, and the Duplicate-mode collision refusal is
    unchanged: it still surfaces a message naming the collision instead of
    stacking two videos on one screen.
  - Polish (`studio/qml/Main.qml`, `studio/i18n/studio_en.ts`): the assignment
    affordances, the degraded and coverage readouts, and the per-button tooltip
    gate (hovered and unarmed) were tightened, with the new strings translated.

## [1.2.0] - 2026-09-27

79 commits landed after the `v1.1.0` tag, all between 2026-09-23 and
2026-09-26. Two things dominate. First, the entire Studio UI was rewritten
from Qt Widgets to Qt Quick/QML in a single day: the whole widget tree was
deleted and the three tabs are now served by one `studio/qml/Main.qml` plus
four C++ bridges. Second, a 49-ID audit was remediated item by item;
`docs/compliance-matrix.md` is the authoritative index of what each fix
covered and which evidence file proves it. A third thread runs through both:
the i18n scaffolding that MED-11 deleted in 1.1.0 came back, by owner
decision, with Indonesian as the source language and English as the only
catalogue.

### Added

- **Selectable Studio UI language (Indonesia / English)**, reversing the
  MED-11 decision that 1.1.0 recorded in `CONTRIBUTING.md` ("owner: hapus").
  The chain is back end to end:
  - `studio/i18n/studio_en.ts` (211 messages, none unfinished, declared
    `sourcelanguage="id" language="en"`), the file Qt Linguist edits.
  - `lupdate`, `lrelease` and `translations` targets back in
    `studio/CMakeLists.txt`. Only `lrelease` gates the `studio` target, so a
    plain build compiles `build/i18n/studio_en.qm` and copies it next to
    `studio.exe` without ever rewriting the committed `.ts`. Both tools
    degrade to a clear no-op target plus a configure-time `message(WARNING)`
    when `find_program` cannot find them, because a Studio that cannot
    translate must still be a buildable Studio.
  - `studio/src/ui_language.{hpp,cpp}` plus a `QTranslator` installed in
    `studio/src/main.cpp` **before** `QmlShell::setSource()`. A translator
    added after the shell loads only reaches bindings created later, so the
    practical consequence is that a language change takes effect on the next
    Studio launch rather than live. The choice is read from
    `studio_ui.ini` (key `ui/language`) under `%LOCALAPPDATA%\K6WP`, not from
    `StudioSettings`, because `StudioSettings` lives in `shared/` and is read
    by the engine, so a Studio-only field there would mean a shared schema
    migration. An unknown or corrupt value falls back to Indonesian silently.
  - **No `studio_id.ts` by design.** Every `tr()` / `qsTr()` in
    `studio/src/*.cpp` and `studio/qml/Main.qml` is Indonesian, so Indonesian
    is the source language, the default, and needs neither a `.qm` nor a
    `QTranslator`. Only a non-source language loads a catalogue.
  - Two bugs surfaced only because translations now exist, and both are
    fixed: `PreviewWidget` had no `Q_OBJECT`, so `tr()` resolved against the
    inherited `QObject` context while `lupdate` recorded
    `k6wp::PreviewWidget` and every preview status string silently failed to
    translate (`Q_OBJECT` is now present for the `tr()` context alone, the
    class still declares no signals or slots); and the engine-status plus
    monitor labels in `StudioBridge` were `QStringLiteral`, so they stayed
    Indonesian inside an English UI. `Main.qml` is wrapped in `qsTr()` and
    uses `%1` / `%2` with `.arg()` for percentage-bearing sentences so a
    translator gets one sentence rather than fragments. Enum key arrays
    (`fit_mode` / monitor / cpu lists) and the encoder id list stay
    untranslated on purpose: they are API values matched against C++, not
    prose.
- **Compress-first offer for large videos** restored
  (`studio/src/compress_first_offer.hpp`): importing or applying a video over
  20 MB again asks "compress this to 1080p first?". The feature lived in
  `MainWindow::MaybeOfferCompressFirst` and was deleted along with the widget
  tree; the QML migration never replaced it, so nothing prompted. The
  threshold now lives in one header so the import and apply paths cannot drift
  apart. Accepting pins the job to 1920x1080, because the prompt promises
  1080p while the resolution box may hold something else.
- **12 new CTest suites (3 at the `v1.1.0` tag, 15 now)**, so a plain
  `ctest` is a real gate rather than a two-suite smoke test. The list lives
  in the `add_test(NAME ...)` calls of `shared/CMakeLists.txt` (4:
  `config_test`, `ipc_test`, `sha1_test`, `proc_util_test`),
  `engine/CMakeLists.txt` (3: `occlusion_test`, `ipc_marshal_test`,
  `gpu_pin_test`), `studio/CMakeLists.txt` (5: `studio_async_test`,
  `thumbnailer_test`, `library_crud_test`, `studio_logic_test`,
  `fake_pipe_test`), `compressor/CMakeLists.txt` (2: `probe_json_test`,
  `compress_argv_contract`) and `launcher/CMakeLists.txt` (1:
  `lockscreen_backup_test`). The Qt-based suites run under
  `QT_QPA_PLATFORM=offscreen`, and `studio_logic_test` gets the vendored
  ffmpeg/ffprobe paths plus a copy of `compressor.exe` so its queue test has a
  real job to run.

### Changed

- **Studio rewritten from Qt Widgets to Qt Quick/QML.** This is the largest
  change in the release. Seven `.cpp` files and their seven headers were
  deleted (14 files, roughly 4,600 lines): `main_window.cpp`,
  `settings_widget.cpp`, `wallpaper_tab_widget.cpp`,
  `compressor_tab_widget.cpp`, `library_widget.cpp`, `import_dialog.cpp` and
  `compress_status_widget.cpp`. Before deleting, every includer of those
  headers was checked and each one was itself in the delete set.
  `first_run_wizard.cpp` / `.hpp` survive only because `studio_logic_test`
  compiles them, and `preview_widget.cpp` plus `compress_errors.cpp` survive
  because the native mpv preview and `CompressBridge`'s friendly-error mapping
  still use them. `Qt6::Widgets` therefore stays linked to the `studio`
  target, and that is required rather than incidental: the vendored libmpv has
  no D3D11 render backend (`mpv_render_context_create` with `api="d3d11"`
  returns rc=-19) and the OpenGL path drops `hwdec` to software, so `vo=gpu`
  with `d3d11va` is only reachable by handing libmpv a real `winId()`. The
  preview is a `QQuickWidget` sibling, a native child of the shell window,
  and QML pushes its hole rect through `Studio.syncPreviewGeometry()`.
  - **Phase 1** (`7c8141d`): the QML shell (`studio/src/qml_shell.{hpp,cpp}`,
    `studio/qml/Main.qml`), `StudioBridge` (`studio_bridge.{hpp,cpp}`, owning
    the single `IpcClient`, `ApplyManager` and `StudioSettings` and
    re-exposing them as `Q_PROPERTY` / `Q_INVOKABLE`), and
    `qt_add_qml_module(URI K6WP)` with `QT_RESOURCE_ALIAS` pinning `Main.qml`
    to a bare name. Engine status still comes from the unit-tested
    `DecideEngineStatus` decision function, polled off-thread through
    `QFutureWatcher`. Two bugs were found while verifying: a missing
    `QML_NAMED_ELEMENT(Studio)` made the singleton register as
    `StudioBridge` while `Main.qml` binds to `Studio.*`, which QML does not
    treat as a load error (every affected binding silently keeps its default);
    and `QQuickWidget::status` never reaches `Error` for runtime binding
    errors, so `QmlShell` also reports `QQmlEngine::warnings`.
  - **Phase 2** (`f9ea1a1`): the Wallpaper tab gets working video selection,
    apply, pause/resume and the "Pengaturan cepat" group. Selection is
    pick-then-apply, matching the widget flow, and the quick settings are
    GUI-thread single-field writes because the engine's own config watcher
    applies them. `monitorChoices` is built from `ListMonitors()`.
  - **Phase 3** (`7aee28b`): `CompressBridge` (`compress_bridge.{hpp,cpp}`)
    owns a `CompressController` **by value** (it is not a `QObject`, so it
    cannot carry `Q_PROPERTY` itself) and republishes the queue as
    primitives. Only `int` / `QString` / `bool` cross into QML, because
    `CompressService`'s signals carry `JobMeta` and `CompressOkInfo`, which
    are not registered metatypes and work today only because service,
    receivers and `QProcess` all sit on the GUI thread. The long-video gate
    is a `QMessageBox` that cannot run headless, so the bridge raises
    `consentRequired()` and QML answers through a Material `Dialog`.
  - **Phase 4** (`a65b5cf`): `SettingsBridge` (`settings_bridge.{hpp,cpp}`)
    owns `StudioSettings` plus the engine playback config and keeps the two
    files apart exactly as the on-disk contract requires, with two
    independent writes so a `config.json` failure cannot discard preferences
    that already saved. Autostart is the one field written through
    immediately rather than on apply, because it is an HKCU `Run` value the
    widget checkbox fired on every toggle, and the checkbox re-reads
    `IsAutostart()` so it snaps back when the registry write fails.
  - **Phase 5** (`85aa8fe`): `LibraryGridModel` (`library_grid_model.{hpp,cpp}`)
    exists because `LibraryManager` is a plain class whose
    `Load` / `Add` / `Remove` **throw** `LibraryError` and QML cannot catch a
    C++ exception. The model owns a `LibraryManager`, wraps every call and
    converts a failure into a status line plus `lastError`, so the exception
    can never unwind into the QML engine. Display label and search filter are
    ported verbatim from `LibraryWidget::EntryLabel` / `FilterMatches`. The
    model raises intents (`applyRequested`, `recompressRequested`) instead of
    acting on them, because applying is a `StudioBridge` call and
    re-compressing is a `CompressBridge` call.
  - **Phase 6** (`f03c850`): drag-and-drop import moves onto `QmlShell`
    (`setAcceptDrops` plus `dragEnter` / `dragMove` / `dropEvent`), and the
    widget tree is deleted. The drop target is the library model, which
    publishes itself through `SetImportTarget` in its constructor because the
    QML engine constructs that singleton after the shell exists. A drag is
    only accepted when it carries at least one local video, so the cursor
    shows the copy affordance only for drops Studio will take.
  - **Phase 7** (`81e758a`): Phase 6 silently took the menu bar with it, which
    lost real functionality and not just layout. The MenuBar is back in QML
    with the original Indonesian items (Berkas: Impor Video..., Keluar;
    Bantuan: Check for updates, Tentang K6WP Studio) and the `UpdateChecker`
    that was left compiled but unreachable is now wired: version,
    `donateUrl` / `projectUrl`, `updateAvailable`, `updateCheckBusy`,
    `latestVersion`, `latestPageUrl`, `checkForUpdates()`, lazily created,
    republished as primitives, with the start-up check gated on
    `settings_.check_updates` and an early return when
    `IsUpdateCheckUrlUsable()` is false.
  - **Phase 8** (`34c6099`): the first-run wizard returns as a QML `Dialog`
    with a `StackLayout` in place of `QWizardPage` (which cannot run inside a
    `QQuickWidget`), keeping the original Indonesian wording and the
    Lanjut / Kembali / Selesai / Nanti saja buttons. The gate is still the
    same pure `IsFirstRunCondition` the widget wizard used, still unit-tested
    by `studio_logic_test`, with QML supplying the third argument via
    `!Studio.videoActive`. That property is documented as deliberately
    partial: dropping the QML half would resurface the wizard for users who
    already have a video applied.
  - **Layout passes** (`e6a664a`, then `4993354`): the three tabs go from one
    full-width vertical stack each (on a 1936 px window the right ~75% sat
    empty, with "Folder output:" at x=10 and its Ubah button ~1840 px away)
    to two-column layouts, then to shared sizing tokens. Each form group is
    now one `GridLayout` so labels and controls line up by construction
    rather than by hand-tuned widths; `labelColWidth`, `controlWidth`,
    `narrowControlWidth`, `spinWidth` and the spacing values live on the QML
    root and are shared by all three tabs, so a row cannot drift from its
    neighbours. Buttons dropped `Layout.fillWidth` and size to their content.
- **Library model: one `LibraryManager` per process, shared by reference.**
  `MainWindow`, `LibraryWidget` and `ImportDialog` each kept their own
  manager and compensated by re-`Load()`ing from disk on every read, which
  meant three copies of one index plus a reload on every grid refresh to hide
  the divergence, and a real window where a mutation through one instance was
  invisible to another. `MainWindow` now owns the single instance and hands
  out borrowed pointers (`LibraryWidget::SetLibraryManager` replaces the
  by-value member; `ImportDialog` takes a `LibraryManager*` in its ctor and
  drops both its `Library` and its own manager member).
  - The `Library` storage layer is **removed** (`7c44b08`). `Library` copied
    an imported video into `%LOCALAPPDATA%/K6WP/library/` and generated its
    thumbnail, sitting underneath `LibraryManager`, which persists the
    metadata index above it. Two overlapping storage abstractions described
    the same import twice and the copies disagreed. Imports now reference the
    source file in place (`src == dst`), so the copy step and the second cache
    are gone, and `Thumbnailer` owns the single ffmpeg `CreateProcessW` site.
    `library.cpp` / `library.hpp` are dropped from the `studio` target.
  - `LibraryManager::ReferenceInPlace` (`a46530f`) replaces the copy half of
    the removed `Library::ImportVideo`: it builds a `LibraryEntry` referencing
    an existing video where it sits, with dimensions and duration still
    deferred to `Add()`, and throws `LibraryError` when the source is missing
    or is not a regular file so a directory or a stale path fails at the call
    site instead of producing an entry that can never resolve.
  - Coverage retargeted (`d0a7edc`): `library_import_test` existed only to
    time the function that no longer has a subject, so it is deleted rather
    than repointed, and `library_crud_test` grows from 20 to 42 checks
    covering `ReferenceInPlace` plus the shared-manager contract (Add / Remove
    / Clear through one instance are visible to a reader on that same
    instance without a manual `Load`, which is exactly the property the old
    per-widget copies broke).
- **Pre-QML Studio decomposition, and what survived it.** MED-5 split
  `main_window.cpp` from 2538 to 1275 lines by extracting
  `CompressController`, `WallpaperTabWidget`, `CompressorTabWidget`,
  `FirstRunWizard` and `EngineStatusController`. The QML rewrite then deleted
  the two tab widgets and `MainWindow` itself, so the surviving pieces are
  the controllers: `CompressController` is owned by `CompressBridge`,
  `EngineStatusController` backs the engine-status poll, and
  `FirstRunWizard` is kept alive only as a compile dependency of
  `studio_logic_test`.
- **Apply unified into `ApplyManager`** (`2c369b5`).
  `MainWindow::ApplyOnWorker` had re-implemented `ApplyManager::Apply`
  line-by-line and already drifted (live-path readback, restart + ready-wait +
  explicit `SetVideo` recovery, per-outcome lockscreen sync).
  `ApplyManager::Apply` now returns `ApplyResult` and owns the full shipped
  semantics, `ApplyOnWorker` is deleted, and the dead async-restart machinery
  (`RestartEngineAsync`, `RestartBusy`, `RestartFinished`,
  `OnRestartWorkerDone`, `WaitForAsyncDone`, zero callers) went with it.
  `SetConfigPath` is restored with an honest doc: production never sets it,
  but it is the test seam that keeps `WriteConfig` off the real per-user file.
- **Version single-sourced (MED-14, `9002c29`).** `K6WP_VERSION` in the root
  `CMakeLists.txt` is canonical and `configure_file()` stamps it into
  `build/<cfg>/generated/version.h` (C/C++/RC consumers),
  `packaging/version.nsh` (from the root `version.nsh.in`) and
  `engine/app.manifest` (in place, from `engine/app.manifest.in`). Every
  consumer references those generated files, so there are no hand-synced
  version literals left in the five `.rc` files, the manifest or
  `installer.nsi` (which `!include`s `version.nsh` and derives `APP_VERSION`
  and `VIProductVersion` from it). `packaging/make_zip.ps1` asserts all of it:
  `project()` and `K6WP_VERSION` agree, the generated files carry the
  canonical version, and a hand-written literal in any consumer source fails
  the run. `docs/tech-debt.md` marks the item RESOLVED.
- **Config schema documentation pinned to the code contract** (`3ea53fb`):
  `config_schema.json` moves v4 to v5 (`cpu_affinity` / `gpu_adapter`
  documented, v5 migration history, `speed` marked RESERVED because the
  renderer does not apply it) and a new `config_test` guard asserts
  `version.maximum` and the exact `ConfigToJson` field set against the JSON
  doc, so the two cannot drift.
- **Dead code and hygiene**, each verified by grep before removal (`9429569`,
  `6b1c879`, `6dd5859`): the pass-through `SettingsTabWidget` shim, the
  unreachable fit-mode machinery in `PreviewWidget`, never-emitted
  `LibraryWidget` signals and their handlers, and a dead queue API; a uniform
  `GetWindowLongPtrW` (`GetWindowLongW(` is now zero repo-wide, and the
  single engine-side hit was where the audit said Studio was), 9 named
  `constexpr`, resolved README/SECURITY TODOs, a contact in `SECURITY.md`,
  legacy icons moved to `attic/packaging-icons/`, a `/W4` baseline of 8
  warning lines with no `/WX` in any dev preset, and removal of committed
  session artifacts (a 582 KB `loglog.txt`, a 1 MB `shared_dump.md`, 67
  tracked `.omo/` transcripts, and 1365 attic spike lines that were never in
  CMake). Committed `bench_*.json` evidence is deliberately kept.
- **Stale comments corrected in place** (`3ea53fb`): the pre-CRIT-2
  "IPC worker thread is thread-safe" header comments replaced with the
  current marshal-to-main-thread contract, a false "sole intentional
  polling" claim in `occlusion_watch.hpp` (the fullscreen and config
  fallbacks poll too) and its stale `dwmapi` claim (the module is cached),
  and a pointer at a `mpv_hwdec.cpp` path that had moved to
  `attic/spikes/`.

### Fixed

- **Post-rewrite Studio regressions.** The QML migration shipped working, then
  a run of small fixes closed the gaps the rewrite opened:
  - `auto_compress_on_import` now does something. The setting was fully
    plumbed (schema, persistence, `SettingsBridge` property, the Pengaturan
    checkbox) but nothing ever read it, so the default `true` meant nothing
    and an oversized import still stopped at the offer dialog.
    `compressFirstRequired` is the import signal and
    `maybeOfferCompressFirst` is the apply signal, so it gates only the import
    path: with auto-compress on it runs the job, and with it off the dialog
    still appears, which keeps the apply path's offer reachable instead of
    becoming dead code under the default.
  - Library thumbnails bootstrap. The grid asked the model for a thumbnail
    from `Image.onStatusChanged`, but that only fires once a load has been
    attempted, and a load is only attempted when `thumbUrl` is non-empty, so
    an entry with an empty `thumb` field never got asked for and the cells
    stayed blank for the life of the install. The delegate now asks on
    completion when `thumbUrl` is empty (the status trigger stays for the
    other case, a persisted path whose file has since been deleted), and
    `ensureThumbnail` persists through `LibraryManager::Add` before its
    `reload()`, because `reload()` re-reads `library.json` and would discard a
    thumbnail living only in memory on the next paint.
  - A pending apply is dropped when a compress job yields no output.
    `CompressService::Finished` has three exits and only the success one set a
    result, so the compress-first offer's intent flag outlived the job that
    raised it and could apply an unrelated later compress, the exact failure
    the flag was introduced to prevent. Both no-result exits now emit
    `jobFinishedWithoutResult()` and QML clears the flag on it, so the intent
    dies with the job that created it. Clearing is unconditional rather than
    gated on the queue draining, because `running` / `pending` are not settled
    at `Finished` time and gating would reintroduce a signal-ordering hazard.
  - A refused enqueue is announced too, not just a failed job. Dispatch's
    `kInvalid` branch (`compressor.exe` missing, or an empty in/out path)
    refuses the request outright, so nothing is queued and `CompressService`
    never emits `Finished`, leaving the one-shot apply intent alive.
  - The compress-first offer has its own apply flag. It was borrowing
    `Compress.autoApply`, which is the user's own "Langsung terapkan setelah
    selesai" checkbox, so accepting an offer ticked or unticked a preference
    the user never touched. `applyAfterCompress` is now owned by the offer: set
    when the offer starts a job, spent when a result lands, cleared by the
    Kompres button so a failed job cannot leak an auto-apply into an unrelated
    later compress. `Compress.autoApply` is written from exactly one place
    again. The same commit returns the "Terapkan perubahan engine yang butuh
    restart" tooltip to the Terapkan button it actually describes; it had been
    left on the Restart engine button, leaving Terapkan the only button in
    that row with no tooltip at all.
  - Declining the compress-first offer applies the original video.
    `maybeOfferCompressFirst` returned `true`, which suppressed the caller's
    `Studio.applyWallpaper` and handed the action to a dialog that only had
    `onAccepted`, so answering No (or Esc) closed it and threw the apply away
    with no error and no log line. Every video over the 20 MB threshold was
    impossible to apply. `onRejected` now runs the pending apply; the import
    path sets `offerApplyAfter = false`, so declining is a no-op there and a
    large imported file is still not silently applied behind the user's back.
  - The latched mpv stop flag. The preview froze on "Memuat pratinjau..." and
    never recovered: `stop_` was passed **by reference** into every
    `MpvWorker`, which gates its event loop on `while (!stop_.load())`, and
    nothing ever set it back to false. The first `LoadVideo()` took the cached
    thumbnail path, called `StopWorker()`, and 200 ms later
    `StartLiveFromPoster()` started a worker with the flag still latched. That
    worker created the mpv core and initialised it, then skipped the event
    loop entirely, so `MPV_EVENT_FILE_LOADED` never arrived; `load_state_`
    stayed `kLoading` forever, and because the worker had already set
    `started_ = true` the 5 s "backend gagal dimulai" fallback was cancelled,
    so nothing ever surfaced an error. Pre-existing, not from the layout work:
    it fires on any second video load in a session and only looked new
    because the preview had previously been exercised on a first load.
  - The Compressor tab's status and dialog copy is translated. `CompressBridge`
    had no translation context at all: every user-facing string was
    `QStringLiteral`, so the whole tab stayed Indonesian in an English UI (the
    visible symptom was the idle status line still reading "Belum ada
    kompresi dijalankan" under an English menu bar). Status, detail, queue,
    ETA, the last-error texts and the two file-dialog titles are now `tr()`;
    the `AppendLog` diagnostics ("Compress:", "Enqueue:", "job #N") are
    deliberately left alone as technical log output. `FormatEta` is a free
    function in an anonymous namespace, so it uses
    `QCoreApplication::translate` with a literal context, since `lupdate`
    resolves that call statically. The idle default moved out of a header
    member initializer into the constructor, because `tr()` in a default
    member initializer would tie the header to a translation context and the
    string would silently stop translating.
  - The catalogue declares `sourcelanguage="id"` and its `<location>` entries
    point at real relative source paths. Every clean build had printed
    `lupdate warning: Specified source language 'id' disagrees with existing
    file's language ''`, and the committed `<location filename="">` entries
    (plus a few whitespace-only ones, from a throwaway first-fill generator)
    pointed nowhere, which made the file unusable in Qt Linguist. No
    translation content changed.
- **Studio app icon, in three steps.**
  - `94c86aa`: `icons.qrc` aliased `:/icons/k6wp-on.png` to `../k6wp-on.png`,
    a file that has never been in the tree. `rcc` produced a null resource, so
    both `QIcon` call sites got a null icon and Studio had no titlebar or
    taskbar icon at all. The dead alias is gone; both call sites now point at
    the canonical ICO every other binary already embeds
    (`packaging/k6wp-on.ico`, also the `IDI_APPICON` resource in
    `studio/app.rc`), which `QIcon` loads directly.
  - `3de6eab`: that fixed the resource path but not the resource itself.
    `qt_add_qml_module()` turns `AUTORCC` off for its target, so the
    `icons.qrc` listed in `qt_add_executable` was never compiled: no
    `qrc_icons.cpp` existed and the `QIcon` was null. The `.qrc` is replaced
    with `qt_add_resources()`, which embeds unconditionally and keeps the
    same `:/icons/` path. `setWindowIcon` is re-applied after `show()` so it
    lands on a real HWND, with a recorded note that `WM_GETICON` is not a valid
    probe here (Qt intercepts it and answers from its own state, reporting 0
    even when the icon is correct).
  - `7b5c7ef` set an `AppUserModelID` so the taskbar would show the K6WP
    icon. **This was reverted** in `aa4e364` the same day; the only trace in
    the tree now is the absence of the call. Do not expect the taskbar icon
    from an `AppUserModelID`; it does not ship.
- **Engine renderer, three defects in `MpvRenderer` (`887825f`):**
  - **Bitblt presentation.** `d3d11-flip=no`. The injected window is a
    `WS_EX_LAYERED` child of Progman, and Progman carries
    `WS_EX_NOREDIRECTIONBITMAP` on 24H2, so DWM gives that subtree no
    redirection surface. A flip-model swap chain shares its back buffer with
    DWM and needs one: `Present()` succeeds with a valid front buffer while
    DWM composites nothing, which reads as "decodes fine, the desktop never
    changes". Bitblt presentation does not need the redirection surface.
  - **Non-deprecated aspect override.** `SetFitMode` used
    `video-aspect-override=0` to mean "no override", which mpv 0.41 deprecated,
    so every call logged a deprecation warning and `engine.log` grew to tens
    of thousands of identical lines. Every "no override" branch now goes
    through one `clear_aspect_override` lambda (`override=no` +
    `video-aspect-mode=ignore`).
  - **Live VO logging.** The `vo=gpu-next,gpu` string in the Create log is
    only the *attempted* list, so the log could not say which VO libmpv
    actually brought up. `vo-configured` now reads `current-vo` through
    libmpv and logs it once per real change, tracked by a new
    `last_vo_configured_` member. `current-vo` is read without taking
    `mutex_`, since nesting it under `event_mutex_` would break the
    documented lock order.
  - The same commit inlines the pre-Create pending-HWND apply. It called
    `SetHWND` while already holding `mutex_`, and `SetHWND` re-locks the same
    non-recursive `std::mutex`, so a `SetHWND`-before-`Create` sequence
    self-deadlocked. `initialized_` is true at that point, so the inline form
    is exactly `SetHWND`'s post-initialize branch.
- **Engine finds the 24H2 wallpaper `WorkerW` (`dd6fa1c`).** On 24H2 the
  shell parents its wallpaper `WorkerW` to Progman, so the `EnumWindows`
  sweep in `FindDesktopWindows` structurally could not see it: the window is a
  child, not top-level, so Strategy B never found the layer it asked for and
  always fell through to the layered-child-of-Progman fallback.
  `FindDesktopWindows` now also enumerates `Progman` children into a separate
  `progman_worker_ws` list so Strategy A is unaffected, `FindWorkerWStrategyB`
  prefers an empty Progman-child `WorkerW` before the `GW_HWNDNEXT` sibling
  probe, and the 24H2 primary path uses `0x052C` with the documented spawn
  parameters (the previous `(0, 0)` form is a no-op on 24H2, which is why
  asking for a `WorkerW` produced nothing). With no wallpaper `WorkerW` the
  old layered-child path still applies, so the worst case is unchanged.
- **Audit remediation.** `docs/compliance-matrix.md` is the index: it maps
  each of the 32 plan todos to its audit IDs and to the happy-path plus
  failure-path evidence files, and it records two accepted deviations (todo
  14's `.bak`-semantics note in `accb7e9`, todo 30's substitute live coverage).
  The user-visible and correctness-relevant fixes:
  - **CRIT-1 / CRIT-2** (`7a6f136`): IPC handlers are marshalled onto the main
    message loop. `ipc_server.cpp` now has zero hits for `CreateWindowExW`,
    `SetActiveMonitor` or `LoadLoopAll` (validation plus `PostMessageW`
    `WM_APP+0x54` / `0x55` only), and `wallpaper_surface_live_`,
    `headless_owns_decode_` and `pin_verify_armed_` became `std::atomic`.
    Verified with 100 send_test round-trips, 1000 pipe_fuzz iterations, a 50x
    monitor stress with zero `DestroyWindow ... failed` and a 120x flood
    plus quit.
  - **HIGH-1** (`f686437`): the IPC server is stopped first in `Shutdown`, so
    an in-flight request can no longer touch a torn-down renderer. The
    ack-then-complete semantics are now written down in
    `docs/dev-contracts.md` §1 ("diterima" vs "selesai").
  - **HIGH-2** (`d49a5b2`): the lockscreen backup no longer loses data.
    `EscapeBackupJson` flattened every non-ASCII `wchar` to `'?'`, so a
    `LockScreenImage` path with CJK or accented characters was corrupted
    beyond recovery on restore, and `ReadLockscreenBackup` copied the JSON
    value byte-for-byte, mangling escapes and multi-byte UTF-8. It now
    converts wide to UTF-8 first (surrogate-pair safe) and JSON-escapes only
    `\`, `"` and bytes below `0x20`, reads back with a proper unescape
    (`\\`, `\"`, `\uXXXX` plus surrogate pairs), and a legacy `?`-corrupted
    backup is detected and warned about explicitly instead of silently
    restoring a broken path. 34/34 round-trips pass across CJK, accents,
    emoji and special bytes.
  - **HIGH-3, slice A** (`b491e6e`): thumbnail generation on import is async,
    so `ImportVideo` returns in under 100 ms instead of waiting on ffmpeg.
    Generation runs through `QtConcurrent::run` with a GUI-thread callback;
    a missing ffmpeg now yields an empty thumbnail plus an error log rather
    than a hang.
  - **HIGH-3, slice B** (`34e3508`): `IpcClient::Send`,
    `ApplyManager::RestartEngine`, `WaitForEngineReady` and
    `GracefulStopEngineByPid` all moved off the GUI thread (worker plus queued
    signals), with an atomic cancel flag polled in the 10 ms ack-poll slices
    and the connect-retry gaps, and `RestartEngineAsync` yielding a queued
    `RestartFinished`. The wire format and the 2 s ack deadline are unchanged,
    the single shared `IpcClient` plus its `Send` mutex are kept, Apply is
    disabled during the operation, and cancel is cooperative only, with no
    `TerminateThread` anywhere. Events are now processed during a
    `RestartEngine` wait, so there is no "Not Responding".
  - **HIGH-4** (`a934b07`, `6ba842e`): config and settings writes are atomic.
    `SaveConfig` publishes through a `.tmp` + `MoveFileExW`
    (`REPLACE_EXISTING | WRITE_THROUGH`) and a new `PersistConfigField` helper
    writes a single field (video at `engine_app.cpp`, monitor likewise) so the
    tray no longer rewrites the whole file, with a no-op skip when the value
    is unchanged. `SaveStudioSettings` had the same problem with a plain
    `ofstream(trunc)` and now shares the primitive, which was promoted out of
    `config_schema.cpp`'s anonymous namespace to a declared `k6wp::`
    `AtomicWriteJson` so the copy-paste chain stops there.
  - **MED-6** (`0b97156`): the IPC accept loop is event-driven.
    `WaitWithStop` blocks on
    `WaitForMultipleObjects(2, {op_event, stop_event}, FALSE, INFINITE)` with
    zero wakeups while idle, and `IpcServer::Stop()` sets `stop_event_` so the
    loop wakes instantly instead of hanging (proved by a failure-path probe:
    stop without the event hangs, with it exits clean). The 100 ms
    `kAcceptSliceMs` poll slice and every `Sleep()` in the accept path are
    gone; error backoffs use `WaitForSingleObject(stop_event, 250)`.
    `docs/tech-debt.md` marks the "IPC 10 Hz residual" RESOLVED.
  - **MED-8** (`c9e3fdd`): `ParseSemver` cannot overflow. A `long long`
    intermediate plus a pre-multiply clamp, with golden cases (1.1.0, 1.10.0,
    2.0.0-rc) unchanged.
  - **MED-9** (`4b96048`): `ProbeVideoProps` spawns **one**
    `ffprobe -of json` per probe instead of two, parsed with nlohmann in the
    new `compressor/src/video_probe.*`. Old flags and new `--probe` produce
    identical output on three fixtures, and truncated, empty and garbage JSON
    all yield `INVALID` with `valid=0` and no crash.
  - **MED-10** (`7a3a50c`): the compressor decodes argv as UTF-8 in `Widen`.
    The old `std::wstring(s.begin(), s.end())` widened each byte, and on
    MSVC's signed `char` every byte `>= 0x80` sign-extends (`0xC3` to
    `U+FFC3`) and multibyte sequences are never decoded. The replacement is
    the two-call `MultiByteToWideChar(CP_UTF8, ...)` pattern, deliberately
    lenient (`dwFlags=0`) so one bad byte becomes `U+FFFD` instead of
    dropping the whole command line. All seven call sites were audited (ASCII
    flags today; paths were already `path::wstring()`).
  - **MED-12** (`3752bf7`, `518d153`): the pipe name carries a session suffix
    to match the per-session singleton, via `PipeNameForSession` /
    `CurrentSessionPipeName` in `shared/ipc_protocol.hpp`, used by the server,
    the client, `ApplyManager` and the launcher probes. The `Local\...` mutex
    is untouched. Proved active: `get_state` works on the suffixed pipe while
    a bare-pipe probe gets `ERROR_FILE_NOT_FOUND`, and the launcher no longer
    double-spawns.
  - **MED-13** (`0806877`): a safe thumbnail seek (`min(1.0, dur/2)`) plus an
    aspect-preserving scale and pad, so a 4:3 fixture letterboxes to 320x180
    instead of stretching.
  - **MED-15** (`68b3d93`): one source of truth for compress arguments, in
    the new `shared/compress_args.{hpp,cpp}`. `compress_service.cpp` builds
    through the helper and the CLI validates against the same shared bounds
    and flags, with a golden dry-run contract test (11/11 exit 0) and a
    negative control: a bogus flag makes the builder fail and the CLI exit 2
    with `{"error":"unknown option..."}`.
  - **MED-17** (`873b768`): the occlusion region union uses `std::sort`
    (25/25 `occlusion_test`, 7-fixture union areas byte-identical before and
    after).
  - **MED-18** (`ba0fb8d`): a `file_size` guard plus `kMaxConfigBytes` (1 MB)
    in all three read paths (`config_schema.cpp`, `studio_settings.cpp`,
    `library_manager.cpp`), so a 2 MB fixture returns a `ConfigError` with an
    exact message while a 1 KB valid config still loads clean.
  - **MED-3** (`cd9466a`): the two byte-identical SHA1 copies
    (`compressor/src/cache_manager.cpp`, `studio/src/thumbnailer.cpp`) are
    consolidated into `shared/sha1.{hpp,cpp}` and both call sites now use
    `k6wp::Sha1Hex`. The cache key is bumped `v2` to `v3` and `thumb-v2` to
    `thumb-v3` so pre-refactor artifacts miss and regenerate; an old-vs-new
    hash comparison on identical fixtures shows 0 mismatches.
  - **MED-4** (`2849057`): one central `RunCaptured` subprocess helper in
    `shared/proc_util.*`, used at all four call sites, with
    `kProbeTimeoutMs=10000` and `kCompressTimeoutMs=30000`. A bad exe
    fast-fails below the timeout, a hung exe is killed at the budget, and a
    100x handle loop shows a delta of 0 after warm-up.
  - **LOW-4 / LOW-5 / LOW-6 / LOW-7** (`e8b15df`): `CacheKeyHex` prefers
    mtime/size from the `library.json` fingerprint-invalidated snapshot
    (3N stats down to about one hot stat plus lookups), `DrainChannel` exists
    so cursor parse is O(n) instead of O(n^2) with the line contract
    unchanged, the `IpcClient` keeps its pipe across `Send`s and reaps it after
    5 s idle (with broken-pipe paths still dropping immediately), and the
    `dwmapi` `HMODULE` is cached once behind an RAII holder.
  - **LOW-16** (`ba3b6dd`): the dev engine-path fallback is gated on
    `#ifdef _DEBUG` **and** the `K6WP_DEV` env var, and the engine-readable
    settings contract is written down in `docs/dev-contracts.md` §3 with a
    contract test. `SettingsWidget::OnEngineRestart` had resolved `engine.exe`
    with the `../../build/msvc-dev` fallback unconditionally, so a release
    binary could silently pick up a dev-tree engine. A release `studio.exe`
    now contains zero `msvc-dev...engine.exe` strings. One known residual
    stays: an unguarded `engine.exe` literal at `settings_widget.cpp:864` in
    a release build, which does not re-enable the fallback path.
  - **LOW-1 / LOW-12 / LOW-28** (`d7a189a`): a `DestroyWindow` return plus
    `GetLastError()` diagnostic, and a new `engine/src/timer_ids.hpp` with
    pairwise `static_assert`s so a duplicate timer ID fails the build (proved
    by temporarily introducing one and reading C2338).
  - **LOW-2 / LOW-8 / LOW-9 / LOW-10 / LOW-11 / LOW-13 / LOW-22 / LOW-23**
    (`6b1c879`): the hygiene sweep described under Changed.
  - **LOW-17 / LOW-19 / LOW-20 / LOW-21** (`08c4d22`): policy only, no code
    change for the ACL risk (documented in `SECURITY.md`); the update check
    verified as GET + compare + browser-open and its user-initiated policy
    written down; the libmpv dev-channel rationale plus a monthly and
    pre-release advisory schedule in `vendor/VERSIONS.md`; and the
    restore-from-backup JSON, which was missing in **all three** uninstall
    paths, added as `K6WP.exe --elevate-lockscreen` off shims in
    `packaging/uninstall.bat`, `tools/uninstall.bat` and
    `packaging/installer.nsi`.
  - **LOW-24 / LOW-25 / LOW-26** (`9d823d0`): apply-in-progress feedback
    (Apply disabled plus a progress dialog during a live switch), a pause /
    play hover overlay on the preview, and a queue label so a second compress
    request is not silent.
- **Dead code removal that was also a behavior fix** (`9429569`): the never
  emitted `LibraryWidget` signals and the `FormatEta` duplicate, among others.
- **Studio text encoding.** 18 UTF-8 em-dashes in `studio/src/main_window.cpp`
  were stored double-encoded (the classic `â€"` mojibake:
  `U+00E2 U+20AC U+201D` instead of `U+2014`), so 14 user-visible strings
  rendered with a stray curly quote, e.g. the status bar read `Engine aktif â€" %1`.
  Repaired at the byte level so no other byte moved. **Note:** that fix landed
  at 07:34 on 2026-09-26 and the file it touched was deleted 2h18m later by
  the Phase 6 QML commit (`f03c850`, 09:52 the same day), so it is recorded
  here for history only. The replacement `studio/qml/Main.qml` carries 4
  em-dashes and zero mojibake sequences, verified by scan.

### Known limitations

- **`docs/a11y-audit.md` is void.** The LOW-27 audit scanned
  `studio/src/main_window.cpp`, `settings_widget.cpp`, `library_widget.cpp`,
  `import_dialog.cpp` and `compress_status_widget.cpp` for
  `setAccessibleName`, `setTabOrder` and friends. All five were deleted by the
  QML rewrite, so every finding in that document (2 High, 7 Medium, 3 Low)
  describes files that no longer exist. A fresh accessibility pass over the
  QML UI is outstanding work: it cannot be produced by grepping a `.qml` file
  for the Widgets API, and a fabricated re-audit would be worse than none.
  Until it exists, the Studio UI has **no** verified accessibility baseline.
- **The paused zero-wakeup gate is closed by design but not re-measured.**
  MED-6 removed the 100 ms IPC accept slice that caused the churn, and
  `docs/tech-debt.md` records the item RESOLVED. No post-fix 60 s paused
  context-switch measurement is recorded in this repo, so the ~11.8-13.5/s
  figures in `packaging/known-limitations.md` §11 remain the last numbers on
  record and the `< 1.0/s` gate is still formally NOT MET. Treat the fix as
  unproven until someone reruns the tidsnap harness.
- **GPU @1080p H.264 hwdec is still not measured as a percentage.** The
  `< 5%` budget has no committed measurement. What is on record is that the
  dGPU is not doing the decoding (RTX dec/sm/enc 0% across three samples
  during hwdec-active playback, `docs/bench_phase3.json`) and that the last
  actual GPU-3D figure, roughly 55%, came from a Debug dual-decoder run that
  is not a 1080p single-stream hwdec measurement
  (`packaging/known-limitations.md` §3). The old reason, "no video wired to
  CLI yet", is obsolete: playback shipped in 1.0.0.
- **Engine RAM is still over budget** and nothing in this release moves it:
  roughly 220 MB working set while playing 1080p H.264 against an 80 MB
  budget, with a final clamp-based budget of 1080p <= 220 MB and 4K <= 450 MB
  on the iGPU topology. See `packaging/known-limitations.md` §3a and §7.
- **Unmeasured, unchanged:** 2-monitor RAM, the gaming delta (no game harness
  on the rig), 4K assets (none in the repo), reboot-cold startup, and the QSV
  and AMF encoder legs.
- **Shared-decode remains descoped** for per-monitor N decode. The vendor
  swap plan is parked for a future release; see `docs/tech-debt.md`.
- No screenshots ship with this release. `docs/img/` does not exist yet; the
  README placeholder says so rather than showing a layout that has since been
  rewritten to QML.

## [1.1.0] - 2026-09-22

Ships the Phase 1–4 optimization track (earlier entries below were labeled
1.1.0/1.2.0 trajectories while in flight; consolidated here as 1.1.0).
Version stamps bumped consistently (20 spots: root `K6WP_VERSION`,
`installer.nsi`, four `app.rc`) — `make_zip.ps1` version assert green.

### Added

- Phase 4 final pass (RAM trim, build, packaging):
  - P4.1 Engine one-shot working-set trim: a single 2 s timer armed at the
    first frame marker calls `SetProcessWorkingSetSize(-1, -1)` once, then
    dies (never periodic). Working set before/after logged in KB
    (`engine: working-set trim ws=151584 KB -> 184 KB` during 1080p
    playback); failure is non-fatal (log only). Private bytes are the
    honest RAM metric — the trim evicts residency, not commits.
  - P4.2 Studio update check over WinHTTP (Qt6Network dropped): blocking
    GET on a worker thread (`User-Agent: K6WP/1.1.0`,
    `Accept: application/vnd.github+json`, 10 s timeouts), result posted
    to the Qt event loop via queued invoke; `UpdateAvailable`/
    `CheckFinished` signals, `ResolveUpdateCheckUrl`/
    `IsUpdateCheckUrlUsable`, and silent-on-failure semantics unchanged
    (`main_window` untouched). Deploy carries no `Qt6Network.dll`
    (windeployqt `--exclude-plugins qtuiotouchplugin` — the touch plugin
    was the sole Network consumer): Qt deploy 18→13 files, 86.94→79.98 MB
    Debug; release `Qt6Network.dll` 1.76 MB + bearer/TLS dirs gone.
  - P4.4 ffmpeg re-vendored to gyan 8.1.2 **essentials** (verified before
    swap: `h264_nvenc`, `h264_qsv`, `h264_amf`, `libx264` encoders +
    `scale`/`fps` filters + mp4 `faststart`; post-swap:
    `compressor --probe-encoders` picks nvenc, corpus bench PASS
    4.04/7.06/2.04 s, 1080p→720p24 end-to-end moov-before-mdat).
    ffmpeg.exe 242.50→101.90 MB, ffprobe.exe 242.29→101.69 MB (~281 MB
    saved in repo + deploy; new SHA256 in `vendor/VERSIONS.md`).
  - P4.5 Studio Engine settings for the phase 2–3 policy (Indonesian
    labels, `config.json`-backed, restart required for CPU/GPU flips):
    Mode hemat baterai (`cap24` 24-fps cap vs `static` full pause),
    Inti CPU (`auto` E-core vs `all`), GPU (`auto`/`integrasi`/`diskrit`).
- Release engineering hardening:
  - Version single-source assert in `packaging/make_zip.ps1`: the packaging
    run now fails when `CMakeLists.txt` (`project()` / `K6WP_VERSION`),
    `packaging/installer.nsi` (`APP_VERSION` / `VIProductVersion`) and the
    four `app.rc` `FileVersion`/`ProductVersion` values disagree.
  - Ship-set sync assert in `packaging/make_zip.ps1`: the staged portable ZIP
    file set is compared against the `File`/`RMDir` set in
    `packaging/installer.nsi` and fails on drift in either direction.
  - Engine crash minidump: an unhandled-exception filter writes a timestamped
    `.dmp` (system DbgHelp `MiniDumpWriteDump`) to
    `%LOCALAPPDATA%\K6WP\crashes\` and appends one `engine.log` line with the
    dump path.

### Changed

- Phase 1 performance optimization (1.1.0 trajectory, no version bump):
  - P1.1 Engine mpv lean profile: `vo=gpu-next,gpu`, `hwdec=d3d11va` (fallback
    dxva2 → software), `video-sync=display-desync`, bounded demuxer readahead
    (`demuxer-max-bytes=16MiB`, `demuxer-max-back-bytes=4MiB`), bilinear
    scalers, `swapchain-depth=2`; active `vo/hwdec-current/adapter` log line.
  - P1.2 Studio preview lean subset: preview playback uses the same lean mpv
    options (preview deliberately keeps no `display-desync`).
  - P1.3 `fps_cap` default 24 for new configs; compress enqueue clamps
    `fps = min(source, cap)` with a 4-value log line
    (`Enqueue: fps=… (source=…, cap=…) res=…x…`).
  - P1.4 Decode-friendly ffmpeg output: `-tune fastdecode` (x264 leg),
    `-threads N`, single `-movflags +faststart` (moov before mdat).
  - P1.5 `BELOW_NORMAL_PRIORITY_CLASS` for engine + compressor (ffmpeg child
    inherits); IPC `get_state` ack still < 2 s at the lowered priority.
- Benchmarks: `docs/bench_phase1.json` — idle RAM vs
  `docs/bench_baseline_pre_opt.json`, playback CPU vs baseline, 30fps-source vs
  24fps-compressed decode load, plus display-desync long-loop and demuxer-RAM
  watch notes.
- Phase 2 event-driven engine (1.2.0 trajectory, no version bump):
  - P2.1 Config watcher → `ReadDirectoryChangesW` (overlapped, 64 KiB buffer
    with mtime+size rescan fallback) + 250 ms debounce on the hidden window;
    external edit and atomic temp+rename reload via the event path
    (~250 ms), corrupt JSON keeps last-valid, 10 rapid writes coalesce to
    1 reload; setup failure falls back to the old 500 ms poll with a log line.
  - P2.2 Fullscreen watcher → `SetWinEventHook` (foreground range + minimize
    range, two hooks); callback only posts to the loop thread, detection logic
    (`IsFullscreenCandidate` + 2-confirm debounce) byte-identical; hook failure
    keeps the old 1 s poll with a log line.
  - P2.3 Power split: `PBT_APMPOWERSTATUSCHANGE` keeps the unchanged
    `PowerSaver` state machine; new `GUID_MONITOR_POWER_ON` notification with
    new `kPauseScreenOff = 32` (member of both `UiPaused()` and
    `SlotsPaused()` — disjoint targets, no double-pause); new
    `--simulate-monitor-off-after-ms` test flag.
  - P2.4 Per-renderer mpv event threads (headless + every slot) with mutex
    restructure (`mutex_` guards commands only; event polling lock-free);
    EOF watchdog relocated into the threads (500 ms dynamic wait unpaused,
    `-1` blocking while paused); new `mpv_wakeup()` for IPC fast drain and
    `Pause()`/`Resume()` re-arm; engine loop timeout now state-dependent
    (`INFINITE` paused / 1500 ms unpaused / 50 ms simulate-armed) — the fixed
    250 ms sleep is gone.
  - P2.6 `PauseSlot`/`IsSlotPaused` per-slot granularity (atomic flags; global
    bits override; `ResumeAll` preserves occlusion-paused slots; span mode is
    a single slot at index 0).
  - P2.5 New `occlusion_watch`: 1500 ms SELF-SUSPENDING tick (only while
    unpaused, re-armed by `ApplyPauseState`), region-union coverage (no
    `CombineRgn`, no naive sum-area), 95/90 hysteresis, per-slot pause via
    `PauseSlot` (never touches global bits).
  - P2.7 `battery_mode` (`cap24` default | `static`) as schema v3→v4 migration
    (`.bak` backup, enum validated); `cap24` is byte-identical current
    behavior, `static` pauses all slots on DC via the reused `kPausePower` bit.
  - P2.8 Studio preview hidden/minimized pause (resume only if it was playing;
    engine decode unaffected, separate process).
- Benchmarks: `docs/bench_phase2.json` — explicit thresholds vs
  `docs/bench_phase1.json`: idle RAM within ±2 MB (27.69 vs 27.11, PASS),
  playback CPU ≤ phase-1 (0.01% vs 0.24%, PASS), 30fps vs 24fps decode at the
  CPU floor (PASS, no regression signal), unpaused wakeups at the 1.5 s
  occlusion cadence (0.63/s, PASS), paused 60 s ctx-switch delta FAIL
  (~12.5/s vs < 1.0/s — documented, user-approved, see below).

- Benchmarks: `docs/bench_phase3.json` (tag `"3-lite"`) — idle RAM
  29.02 MB vs phase-2 27.69 MB (PASS, ±2 MB), playback CPU 0.02% at the
  floor with E-core pin + iGPU pin, playback WS 141.83 MB vs 153.19 MB,
  ctest 100% (config_test 217 checks incl. v5 scenarios 38-43),
  all `--simulate-*` green, `live_switch_30.py` PASS with pin active.
- Phase 3-lite per-monitor optimization (shared-decode descoped, see
  `packaging/known-limitations.md` §9 + `docs/tech-debt.md`):
  - Schema v4→v5: `cpu_affinity` (`auto`|`all`, default `auto`) +
    `gpu_adapter` (`auto`|`integrated`|`discrete`, default `auto`);
    full v0→v5 migration chain, `.bak`, enum validation.
  - E-core affinity: `auto` on hybrid CPUs pins the engine to the
    min-EfficiencyClass set via `SetProcessAffinityMask` (verified
    mask `0xf000` on i7-12650H); `all`/non-hybrid = logged no-op.
  - iGPU pin: `DXGI_GPU_PREFERENCE_MINIMUM_POWER` pick
    (VendorId-table fallback) applied as mpv `d3d11-adapter` at VO
    init; post-start verify pass recreates-unpinned + reloads when a
    pinned renderer reports hwdec inactive (`dxva2` counts as active,
    never reverts); revert path executed on both slot + headless in
    dev-runs (bogus adapter substrings are silently ignored by this
    mpv build — recorded, synthetic trigger used).
  - **Behavior change**: default `"auto"` now means E-core affinity +
    iGPU pin for existing hybrid users; opt out with `"all"` /
    `"discrete"` (restart required for either flip).
- Phase 4 release flags (P4.3, Release-only via generator expressions —
  Debug/msvc-dev untouched): compile `/GL /Gy /Gw`, link `/LTCG
  /OPT:REF /OPT:ICF` (`/GL` needs `/LTCG`, set together). Release 4-exe
  sizes: compressor −3.7%, K6WP −2.8%, studio −1.5%, engine +7.5%
  (P4.1 code + `/GL` inline growth combined) — net +0.7%, recorded
  honestly. Release build + `make_zip.ps1` green: staged 350.07 MB /
  30 files (was 618.82 MB / 28), ZIP 133.37 MB (was 238.44 MB, < 250 MB
  assert), ship-set + Qt-free + version asserts all OK.
- Benchmarks: `docs/bench_phase4.json` — final matrix: idle CPU 0.02%
  (PASS < 0.3%); paused-wakeup gate still OPEN (phase-2 row-b ~12.5/s
  carried, IPC 10 Hz untouched); 1-monitor private bytes +0.45 MB idle /
  +7.1 MB playback vs phase 3 (−10 MB target NOT MET, recorded with
  cause); post-trim working set idle 1.63 / playback 2.85 MB
  (residency artifact — private bytes stay the cost metric); 2-monitor
  and gaming rows UNTESTED/UNMEASURED on this rig; occlusion /
  monitor-off / battery-static PASS-by-carry with fresh sim smokes green;
  shared decode DESCOPED (N instances retained).

### Fixed

- Occlusion resume no longer needs user input: closing a
  borderless-fullscreen window stranded the fullscreen bit at 1/2
  debounce confirms (exactly one foreground event on close; the
  INFINITE-while-paused loop runs no 1 s poll to complete it). The engine
  now tracks the fullscreen HWND and its `EVENT_OBJECT_DESTROY` clears
  the bit immediately, plus a debounced 150 ms poke runs one direct
  occlusion coverage check (`CheckNow`, existing hysteresis). A follow-up
  closed the same-family F11-out gap (no destroy/foreground/minimize
  event on rect-only exit) via an `EVENT_OBJECT_LOCATIONCHANGE` hook
  with the same tracked-window immediate re-query. Verified:
  scripted close → clear same-ms, mask 0 in ≤0.6 s; F11-out → clear in
  ~11 ms; Win+D cycle 0.5 s; paused 60 s re-verified ≈13.5/s (no new
  periodic timer). See `packaging/known-limitations.md` §11.

### Known limitations

- Phase 4 accounting note: judge engine RAM by private bytes +
  `PeakWorkingSet64`, never by post-trim working set (one-shot
  `SetProcessWorkingSetSize` evicts residency only; soft-fault retouch
  cost after resume/seek unmeasured). See `docs/bench_phase4.json` +
  `packaging/known-limitations.md` §10.
- Occlusion 1.5 s tick is intentional polling: the unpaused message-loop wake
  doubles as the occlusion cadence and is the sole sanctioned periodic wakeup
  in Phase 2 (self-suspending while any pause owner is active).
- IPC 10 Hz residual: the pre-existing IPC `ServeLoop` 100 ms accept slices
  (`engine/src/ipc_server.cpp`) cause ~10/s context-switch churn in ALL
  states, failing the paused zero-wakeup gate (matrix row b, ~750 switches /
  60 s). Untouched by Phase 2 by scope decision; needs-fix with the exact
  repro in `.omo/evidence/matrix/row_b_notes.md`. No prod change made.

## [1.0.0] - 2026-09-19

### Added

- Per-user NSIS installer (`packaging/installer.nsi`): no-UAC
  (`RequestExecutionLevel user`), installs to `%LOCALAPPDATA%\K6WP`, HKCU-only
  registry writes, Start Menu/Desktop shortcuts, autostart checkbox, wallpaper
  backup/restore, clean uninstall with user-data keep/remove choice.
- Portable ZIP build (`packaging/make_zip.ps1`): release-preset build, staging
  of all four executables, vendored ffmpeg/ffprobe + libmpv-2.dll, windeployqt
  Qt DLLs + plugin dirs, app-local VC runtime, `config.json.example` +
  `uninstall.bat`, dumpbin import check, Qt-free launcher assert, version
  assert, 250 MB size limit.
- Four executables: `K6WP.exe` (Qt-free Win32 launcher, singleton + engine
  dispatch), `engine.exe` (resident wallpaper engine), `studio.exe` (Qt6
  Widgets control UI), `compressor.exe` (ffmpeg compress CLI).
- Two-process architecture: the resident Engine renders 24/7 while Studio
  opens on demand for import/compress/customize and live-switches video
  over IPC (NDJSON named pipe) without restarting the Engine.
- Auto-compress cache: imports are compressed to H.264/MP4 against the
  target monitor (CRF 22 default) with an output cache (instant result on
  repeat settings) plus skip-optimal copy and friendly errors.
- Lockscreen sync: optional static-frame extraction to the Windows lock
  screen image (one-time admin elevate via the launcher).
- App-local VC runtime (vcruntime140.dll + vcruntime140_1.dll + msvcp140.dll)
  for fresh-machine safety without a system-wide redist install.
- Engine: desktop injection (WorkerW/Progman A-B fallback), per-monitor
  multi-window geometry, DPI PerMonitorV2, fullscreen auto-pause, battery
  saver, tray icon + menu, single-instance mutex, IPC server (NDJSON + pipe
  ACL), live video switching, OS wallpaper restore on exit.
- Studio: 3-area layout, drag-and-drop import, thumbnail library with
  metadata cache, live apply without restart, compressor page with progress,
  simple-by-default settings with advanced page, start-with-Windows toggle
  (HKCU, no UAC), i18n infrastructure with English baseline.
- Compressor: ffmpeg job runner with progress + cancel, default CRF 22 / H.264,
  output cache, friendly errors.
- Shared: versioned config schema + migration, canonical silent flags, IPC
  protocol, monitor/DPI utilities, autostart helpers.
- QA tooling: `monitor_dump.exe`, bench scripts, stress scripts, release build
  manual test docs.

### Fixed

- Engine keeps desktop icons above the video layer.
- Engine window stays out of the taskbar and Alt-Tab.
- Resilient re-anchor on Explorer/power/display changes.
- Studio opens via the launcher singleton.
- Residual stdout chatter silenced in production (file log only).

### Known limitations

- See `packaging/known-limitations.md` (engine RAM vs budget, hwdec logging
  gap, hardware variance, unsigned-bundle AV expectations).