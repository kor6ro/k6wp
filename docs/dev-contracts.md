# Developer contracts — IPC, config schema, contract tests

Test flags are NOT here: see `docs/dev-test-flags.md` (T18; engine QA flags
hidden from `--help`). This doc covers the wire + on-disk contracts and the
suites that lock them. Sources of truth are the files named below, not this
doc — when in doubt, read the code.

## 1. IPC NDJSON v1 (per-session `\\.\pipe\k6wp-engine-<session_id>`)

- Contract: `shared/ipc_protocol.hpp` / `shared/ipc_protocol.cpp`
  (`kProtocolVersion = 1`, `kMaxPayloadBytes = 64 KiB`). Server:
  `engine/src/ipc_server.cpp`. Client (Studio's single shared instance):
  `studio/src/ipc_client.*` (`MainWindow::ipc_`; `ApplyManager` borrows it —
  exactly one `IpcClient` per Studio process).
- Per-session pipe name (MED-12, audit-remediation todo 13): the engine
  serves `\\.\pipe\k6wp-engine-<session_id>` where `<session_id>` is the
  process's Terminal Services session, resolved via
  `ProcessIdToSessionId(GetCurrentProcessId(), ...)` inside
  `k6wp::CurrentSessionPipeName()` (`shared/ipc_protocol.*`;
  `k6wp::PipeNameForSession(session_id)` builds the name for an explicit
  session). Server (`IpcServer::AcceptAndServe`), client (`IpcClient::Connect`),
  and `ApplyManager::WaitForEnginePipe` (+ its `RestartEngine` liveness probe)
  all use the helper — never a bare `\\.\pipe\k6wp-engine` literal, which no
  longer exists as a listening endpoint. Rationale: concurrent sessions (RDP /
  fast-user-switching) each get an isolated engine singleton instead of
  fighting over one pipe.
- Singleton mutex UNCHANGED: `Local\K6WP-Engine-Singleton`
  (`engine/src/engine_app.hpp::kSingletonMutexName`) stays session-agnostic —
  only the pipe name carries the session suffix.
- Framing: one JSON object + `\n` per message. Request shape:
  `{"version":1,"cmd":"<name>","payload":{...}}`.
  `Encode` throws `IpcError` when the payload exceeds 64 KiB; `Decode`
  returns false (never throws) on empty input, malformed JSON, wrong
  version, unknown cmd, or oversize payload.
- Commands and payloads (see `tests/ipc_test.cpp` round-trip cases):

  | cmd | payload | ack |
  | --- | ------- | --- |
  | `set_video` | `{"path":"<utf8 video path>"}` | `{"ok":true}` or `{"error":"..."}` |
  | `set_monitor` | `{"monitor":N}` (alias `monitor_id` accepted) — HONORED since Step 5: `-1` = all screens, `>=0` = only that monitor (absent id = zero slots, engine keeps running); non-integer or `< -1` → `{"error"}` ack | `{"ok":true}` or `{"error":"..."}` |
  | `pause` / `resume` | `{}` (test uses `{"reason":"user"}` for pause) | `{"ok":true}` |
  | `get_state` | `{}` | `{"ok":true,"state":{...}}` |
  | `quit` | `{}` | `{"ok":true}` then graceful shutdown (see below) |

- `quit` semantics: the engine acks `{"ok":true}` FIRST on the IPC worker
  thread, then posts its private shutdown message to the hidden window so
  the existing message loop runs the SAME path as normal exit
  (`ShutdownWallpaperSurface` + `RestoreOsWallpaper` + tray removal + pipe
  teardown). Never destroys state inline on the worker thread. Protocol
  version stays 1: old engines reply `{"error":...}` to `quit` (unknown
  command) and old Studios never send it, so mixed versions degrade
  gracefully. Client timeout expectations: `quit` uses the normal 2 s ack
  deadline (the ack precedes shutdown, so it never times out on a live
  engine); after the ack the client waits up to 8 s for that PID to exit
  before falling back to `TerminateProcess` on that PID only — never by
  image name.

- Ack contract (CRIT-2, audit-remediation todo 11 — "diterima" vs "selesai",
  LOW-15): `set_video` / `set_monitor` handlers run on the IPC worker
  thread but NEVER execute there. They only VALIDATE the payload
  (`engine/src/ipc_marshal.hpp`: `ParseSetMonitorPayload` /
  `ValidateSetVideoPayload`, unit-tested in `tests/ipc_marshal_test.cpp`),
  stash it in a one-slot pending queue (`EngineApp::marshal_mutex_`), and
  `PostMessageW` a private `UINT` (`WM_APP+0x54` set_monitor, `WM_APP+0x55`
  set_video) to the hidden window — the main loop pops the queue in
  `HandleMessage` and runs the real executor (`HandleSetVideo` /
  `HandleSetMonitor`: `SetActiveMonitor` creates/destroys desktop windows,
  `LoadLoopAll` drives every live renderer; both main-thread-only).
  Consequence: `{"ok":true}` means "diterima" (accepted + queued for the
  main loop), NOT "selesai" (applied to the desktop). `{"error"}` means
  "ditolak" (rejected: malformed payload, unknown path, or a post failure
  during shutdown — nothing queued). Clients that need certainty verify via
  `get_state` (`video` / `monitor` fields reflect the APPLIED state).
  Shutdown path: `Shutdown()` stops the IPC server FIRST (joins the worker
  before any surface teardown), so no executor ever runs after teardown;
  destroying the hidden window discards any still-queued post.
  Related CRIT-1 hardening (same todo): `wallpaper_surface_live_`,
  `headless_owns_decode_`, `pin_verify_armed_` are `std::atomic<bool>`;
  `pin_verify_at_` + `pin_reverted_total_` live under `state_mutex_`;
  `options_` lives under `options_mutex_` (main thread uses a snapshot);
  `running_` and `MultiMonitor::active_monitor_` are atomic — no worker
  write race remains.

- `get_state.state` fields (`EngineApp::BuildStateJson`,
  `engine/src/engine_app.cpp`): `running`, `paused`, `pid`,
  `wallpaper_mode`, `video` (utf-8 path), `config` (config path in use),
  `headless_slots` (Step 4, int), `monitor` (Step 5: live target, `-1` =
  all screens). Studio pushes `set_monitor` on change (and
  `ApplyManager::SyncMonitor` skips unchanged values). Additive playlist
  fields (`playlist_enabled` / `playlist_size` / `playlist_index`) are
  documented in §3a.
- Client discipline: `IpcClient::Send` is one atomic transaction
  (connect + write + ack-read under a mutex) with a 2 s ack deadline;
  pipe-drop gets exactly 1 retry on a fresh connection; `kNotRunning`
  means the engine is dead (Studio starts it, then retries) — never
  restart the engine on the normal live path.
- Pipe ACL: `MakeCurrentUserOnlySA()` grants access to the current user's
  SID only.

## 2. Config schema (`%LOCALAPPDATA%\K6WP\config.json`)

- Schema: `shared/config_schema.json` (draft-07, `$id`
  `.../config.schema.json`); loader: `shared/config_schema.cpp`.
  Example: `packaging/config.json.example`.
- Split (T17): `config.json` is pure Engine playback state
  (`video_path`, `fit_mode`, `monitor_id`, `speed`, `fps_cap`,
  `battery_saver`, plus compressor defaults `crf`, `resolution_w/h`
  kept for round-trip). Studio/compressor preferences live separately in
  `studio_settings.json` (see §3). The engine honors `--config <path>`
  (empty = default path); Studio's `ApplyManager` mirrors the same rule
  via `SetConfigPath()`.
- Key defaults: `fit_mode` enum `cover/fill/fit/stretch/center`, default
  `cover` (`fill` = legacy alias of cover); `speed` 1.0 in [0.5, 2.0]
  (RESERVED: persisted and validated, but not applied by the renderer);
  `monitor_id` `-1` = all screens, `>=0` = only that monitor (Step 5);
  `crf` 22 in [16, 28]; `resolution_w/h` 0 = match monitor;
  `fps_cap` 24 in [1, 30]; `battery_saver` false.
- Migration + `.bak` contract: loader migrates v0/v1 → v2 (missing fields
  defaulted; v0/v1 `monitor_id` is FORCED to `-1` because the old id had no
  render effect — preserving it would newly single-out one screen); on
  migration AND on corrupt/invalid input it backs the original bytes up to
  `<config>.bak` (path + `.bak`) before throwing / rewriting. Live-apply
  reuses this: on `ConfigError` it loads `<config>.bak` (last-valid) and
  only falls back to defaults if the backup is unusable — never silently
  clobbering the user file.
- Write ownership + atomicity (HIGH-4, audit-remediation todo 14): every
  config write publishes via `<config>.tmp` → flush + close → `MoveFileExW`
  (`MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH`) in
  `shared/config_schema.cpp::AtomicWriteJson` — a crash/force-kill before
  the rename leaves the old file intact, and an orphan `<config>.tmp` is
  ignored by `LoadConfig` (never shadows the main file). Two writers share
  this path with split ownership: the ENGINE owns single-field persist
  (`k6wp::PersistConfigField`, a JSON-level merge that preserves
  unknown/future keys and skips the write when the field is unchanged) from
  exactly two call-sites — `EngineApp::HandleSetVideo` (`video_path`) and
  `EngineApp::HandleSetMonitor` (`monitor_id`) in
  `engine/src/engine_app.cpp`; the tray MRU quick-switch
  (`OnTrayQuickSwitch`) performs no direct write and routes through
  `HandleSetVideo`. STUDIO owns full-file persist
  (`ApplyManager::WriteConfig` → `k6wp::SaveConfig`, struct rewrite) in
  `studio/src/apply_manager.cpp`. No other TU calls `SaveConfig` on the
  engine path (grep `SaveConfig` in `engine/` is empty outside the shared
  helper).

## 3. Studio settings (`%LOCALAPPDATA%\K6WP\studio_settings.json`)

- Schema: `shared/studio_settings.json`; loader:
  `shared/studio_settings.cpp`. Example:
  `packaging/studio_settings.json.example`.
- Fields: `auto_compress_on_import` (default true),
  `compress_output_dir` (default `%LOCALAPPDATA%\K6WP\wallpapers`, single
  result location), `default_crf` 22 in [16, 28], `default_fps` 30 in
  [1, 30], `default_resolution_mode` in
  `match_monitor/source/720p/1080p/2160p` (default `match_monitor`),
  `start_with_windows` false, `lockscreen_sync` false,
  `lockscreen_offset_sec` 1.0 (>= 0), `compress_advanced_visible` false
  (Compressor-tab advanced-box state, UI-only),
  `cache_dir` (default `%LOCALAPPDATA%\K6WP\cache`).
  (v3 removed the dead `keep_original`/`start_minimized` keys; old files
  carrying them migrate by ignoring those keys.)
- Same `.bak` contract as §2: v0/v1/v2 → v3 migration and corrupt input both
  back up to `<file>.bak` first. Empty-string dirs migrate to the env
  default. Directory defaults resolve at load (expanded paths on disk;
  `%LOCALAPPDATA%` literal form only in the example file).
- Engine-read allowlist (LOW-16, audit-remediation todo 24): the Engine may
  observe EXACTLY these two fields, both read-only through
  `shared/lockscreen.cpp` (which links into the engine via `k6wp_shared`).
  Every other field in this file is Studio-private — the Engine must never
  branch on it.

  | field | type / default | Engine consumer | missing/corrupt behavior |
  | ----- | -------------- | --------------- | ------------------------ |
  | `lockscreen_sync` | bool, `false` | `IsLockscreenSyncEnabled()` — `EngineApp::MaybeTriggerLockscreenSync` (`engine/src/engine_app.cpp`) and `FireLockscreenSyncAsync` both no-op when false | `false` (best-effort read, never throws) |
  | `lockscreen_offset_sec` | double `>= 0`, `1.0` | `LockscreenOffsetSec()` — seek offset for the `compressor --lockframe` spawn inside `FireLockscreenSyncAsync` (negative values clamp to `0.0`; unreadable settings fall back to `1.0`) | `1.0` (best-effort read, never throws) |

  Forbidden (Studio-only, Engine must not read): `auto_compress_on_import`,
  `compress_output_dir`, `default_crf`, `default_fps`,
  `default_resolution_mode`, `start_with_windows`, `cache_dir`,
  `compress_advanced_visible`, `check_updates` (plus the removed v3 keys
  `keep_original` / `start_minimized`, which are ignored on migration).
  Rationale: `studio_settings.json` is Studio preference state, not playback
  state — the Engine's only legitimate interest is whether to spend a frame
  extract on each video change and at which offset. Widening this allowlist
  (e.g. the Engine honoring a Studio compression default) would couple the
  resident render loop to UI preferences; any such widening updates this
  table AND `tests/studio_logic_test.cpp` §6 first.
- Contract test: `tests/studio_logic_test.cpp` §6 (`studio/CMakeLists.txt`,
  `BUILD_TESTING` block) pins the allowlist behaviorally — it points
  `LOCALAPPDATA` at a scratch dir, writes settings where every forbidden
  field carries a non-default sentinel, and asserts the Engine-visible
  behavior (`IsLockscreenSyncEnabled()` / `LockscreenOffsetSec()`) is
  unchanged; flipping only the two allowed fields must move the behavior.
  Missing-file and corrupt-file cases must yield the safe defaults above.

## 3a. Playlist (`%LOCALAPPDATA%\K6WP\playlist.json`)

- Schema: `shared/playlist.hpp` / `shared/playlist.cpp` (schema v1,
  `kPlaylistSchemaVersion`). Engine reader: `EngineApp::MaybeReloadPlaylist` /
  `EngineApp::FireRotation` (`engine/src/engine_app.cpp`); Studio writer:
  `studio/src/playlist_bridge.cpp` (the `Playlist` QML singleton).
- Why a separate file: `config.json` is Engine playback state, and an older
  Studio build rewrites it from its own `WallpaperConfig` struct
  (`ApplyManager::WriteConfig` → `SaveConfig`), which would silently delete a
  playlist stored there. A separate file is never touched by old builds, so
  downgrade cannot corrupt the playlist.
- Fields: `version` (1), `enabled` (bool, default false), `interval_min` (int,
  `[1, 1440]`, default 30), `shuffle` (bool, default false), `order` (array of
  UTF-8 absolute paths; de-duplicated case-insensitively; max 500 entries).
- Same `.bak` + `AtomicWriteJson` contract as §2/§3. The engine reloads when the
  file's mtime/size change (mirrors `ConfigWatcher`) and rotates via the existing
  validated `set_video` path when the interval elapses AND the wallpaper is not
  paused. While paused the loop wait is INFINITE, so rotation is frozen and the
  full interval restarts on resume. The rotation position is derived from
  `config.video_path` (no separate persisted index).
- Location: sibling of `config.json` (`PlaylistPathForConfig`), so an engine
  started with `--config <dir>/config.json` reads `<dir>/playlist.json`.
- Additive `get_state` fields (old clients ignore unknown keys):
  `playlist_enabled` (bool), `playlist_size` (int), `playlist_index` (int,
  -1 when the current video is not in the playlist).
- Tests: `playlist_test` (`shared/CMakeLists.txt`, pure io/validation/helpers)
  and `playlist_bridge_test` (`studio/CMakeLists.txt`, QML-singleton round-trip
  over a scratch file via the `K6WP_PLAYLIST_JSON` override).

## 4. Contract-test inventory (kept — never delete)

| Suite | Source | Gate | Last verified |
| ----- | ------ | ---- | ------------- |
| `config_test` (250 checks) | `tests/config_test.cpp` | `BUILD_TESTING=ON` (`shared/CMakeLists.txt`) | Todo 14: 250 checks, 0 failures (`build/msvc-dev/config_test.exe`) |
| `ipc_test` (100 checks) | `tests/ipc_test.cpp` | `BUILD_TESTING=ON` (`shared/CMakeLists.txt`) | 2026-09-18: 81 checks, 0 failures (`build/msvc-dev/ipc_test.exe`) → quit adds 19 (100 checks, 0 failures) |
| `sha1_test` (6 checks) | `tests/sha1_test.cpp` | `BUILD_TESTING=ON` (`shared/CMakeLists.txt`) | Todo 25: 6 checks, 0 failures — shared SHA1 vectors |
| `proc_util_test` (13 checks) | `tests/proc_util_test.cpp` | `BUILD_TESTING=ON` (`shared/CMakeLists.txt`) | Todo 16: 13 checks, 0 failures — RunCaptured spawn/timeout/kill |
| `occlusion_test` (25 checks) | `tests/occlusion_test.cpp` | `BUILD_TESTING=ON` (`engine/CMakeLists.txt`) | 25 checks, 0 failures — occlusion union/area helpers |
| `ipc_marshal_test` (18 checks) | `tests/ipc_marshal_test.cpp` | `BUILD_TESTING=ON` (`engine/CMakeLists.txt`) | Todo 11: 18 checks, 0 failures — worker-side set_video/set_monitor validators |
| `gpu_pin_test` (31 checks) | `tests/gpu_pin_test.cpp` | `BUILD_TESTING=ON` (`engine/CMakeLists.txt`) | Todo 21: 31 checks, 0 failures — gpu_pin table + semver/sha1/cache-key anchors |
| `probe_json_test` (49 checks) | `compressor/tests/probe_json_test.cpp` | `BUILD_TESTING=ON` (`compressor/CMakeLists.txt`) | Todo 15: 49 checks, 0 failures — single-JSON ffprobe parser |
| `compress_argv_contract` (11 checks) | `compressor/tests/compress_argv_contract_test.cpp` | `BUILD_TESTING=ON` (`compressor/CMakeLists.txt`) | Todo 20: 11 checks, 0 failures — golden Studio↔compressor argv replay (`compressor.exe --dry-run`) |
| `studio_async_test` (16 checks) | `tests/studio_async_test.cpp` | `BUILD_TESTING=ON` (`studio/CMakeLists.txt`) | Todo 18: 16 checks, 0 failures — off-GUI-thread IPC/cancel/retry over fake pipe (`QT_QPA_PLATFORM=offscreen`, `QCoreApplication`) |
| `thumbnailer_test` (24 checks) | `tests/thumbnailer_test.cpp` | `BUILD_TESTING=ON` (`studio/CMakeLists.txt`) | Todo 4: 24 checks, 0 failures — safe seek + aspect-preserving scale/pad |
| `library_crud_test` (42 checks) | `tests/library_crud_test.cpp` | `BUILD_TESTING=ON` (`studio/CMakeLists.txt`) | Todo 21: 42 checks, 0 failures — ReferenceInPlace + shared-manager contract |
| `studio_logic_test` (34 checks) | `tests/studio_logic_test.cpp` | `BUILD_TESTING=ON` (`studio/CMakeLists.txt`) | Todo 22: 17 checks, 0 failures — ApplyManager non-GUI logic (unwired refusals, WriteConfig round-trip, SyncMonitor dedup over fake pipe) + Todo 24: +17 (engine-read settings contract §6, dev-fallback gate §7) |
| `fake_pipe_test` (14 checks) | `tests/fake_pipe_test.cpp` | `BUILD_TESTING=ON` (`studio/CMakeLists.txt`) | Todo 22: 14 checks, 0 failures — Qt-free IpcClient round-trips over in-process fake pipe (`--no-server` proves the kNotRunning path) |
| `playlist_test` (43 checks) | `tests/playlist_test.cpp` | `BUILD_TESTING=ON` (`shared/CMakeLists.txt`) | playlist.json io/validation/migration + `SelectNextIndex` / `PlaylistIndexForPath` (43 checks, 0 failures) |
| `playlist_bridge_test` (26 checks) | `tests/playlist_bridge_test.cpp` | `BUILD_TESTING=ON` (`studio/CMakeLists.txt`) | PlaylistBridge CRUD/persist round-trip over a scratch playlist.json (26 checks, 0 failures) |
| `lockscreen_backup_test` (34 checks) | `tests/lockscreen_backup_test.cpp` | `BUILD_TESTING=ON` (root `CMakeLists.txt`) | Todo 19/22: 34 checks, 0 failures — lockscreen backup escape/decode round-trip (needs `/utf-8`: literals are UTF-8 without BOM) |
| `monitor_dump` | `shared/monitor_dump.cpp` | always built (QA tool, prints monitor list as JSON) | — |

- Run: `cmake --preset msvc-dev` (with `-DBUILD_TESTING=ON` for the
  test targets), then `build\msvc-dev\config_test.exe` and
  `build\msvc-dev\ipc_test.exe` — exit 0 = all pass. The `release`
  preset sets `BUILD_TESTING=OFF` (see `CMakePresets.json`); the wider
  target tidy (T24) owns any further gating.
- Production logging (T23, in flight at time of writing): engine/studio
  TUs already carry `#ifndef K6WP_VERBOSE / #define 0` guards with
  `#if K6WP_VERBOSE` debug paths; the root `option(K6WP_VERBOSE)` is the
  owning task's to land — check `CMakeLists.txt` + `engine/src/log_file.*`
  for current state.

## 5. Canonical silent flags (pointer — full table in T18 doc)

- Engine silent = `--minimized`; launcher/autostart = `K6WP.exe
  --engine --silent`. `--minimized --silent` stays a working alias in both parsers
  for old autostart entries. Unknown `--flag` → usage to stderr, exit 2.
  Details: `docs/dev-test-flags.md`.

## 6. Studio↔compressor argv contract (MED-15, audit-remediation todo 20)

- Single source of truth: `shared/compress_args.hpp` / `shared/compress_args.cpp`
  (`k6wp::CompressArgs`, `k6wp::BuildCompressArgv`, `k6wp::IsKnownCompressorFlag`,
  plus the `kCompressFlag*` spellings, validation bounds
  `kCompressMinCrf`/`kCompressMaxCrf`/`kCompressMaxFps` and defaults
  `kCompressDefaultFps`/`kCompressDefaultCrf`/`kCompressDefaultEncoder`).
  The header is windows.h-free and Qt-free (std only); both sides adapt at
  their boundary. Registered in `shared/CMakeLists.txt` (`k6wp_shared`
  sources); both consumers already link `k6wp_shared`.
- Builder: `CompressService::StartNext` (`studio/src/compress_service.cpp`)
  fills a `CompressArgs` from the `CompressRequest` and converts each
  emitted `std::string` token to `QString` — it never hand-spells a flag.
  Canonical order: `--in/--out/--res WxH/--fps/--crf/--encoder`
  `[--force-long]` (from `CompressRequest::force`) `[--dry-run]` (golden-test
  path only; production jobs never send it). The builder does no validation;
  the parser owns rejection. IPC wire format is NOT involved (argv only).
- Parser: `ParseCli` (`compressor/src/cli.cpp`) shares the flag spellings,
  bounds and defaults from the same header (no local limit literals), and
  its unknown-option branch consults `k6wp::IsKnownCompressorFlag` to tell
  "flag from the future" (contract moved without a parser update — distinct
  `shared argv contract but not handled` error) apart from real typos
  (`unknown option`). Lockframe-mode flags (`--lockframe`, `--offset-s`,
  `--quality`) and `--probe-encoders`/`--help` are known-flag set members
  but are never emitted by the Studio builder.
- Golden test: `compress_argv_contract`
  (`compressor/tests/compress_argv_contract_test.cpp`, registered in
  `compressor/CMakeLists.txt` under `BUILD_TESTING` — compressor scope, not
  root `tests/` scope). Builds a fixture `CompressArgs` (empty temp input +
  `--force-long` so the ffprobe duration probe is skipped while the dry-run
  cmdline build still sees an existing input; encoder `auto` mirrors the
  Studio default), asserts the exact golden argv vector, asserts every
  emitted flag is known, then replays the argv against the real
  `compressor.exe --dry-run` (located as a sibling of the test exe — no
  generator-expression path quoting) and asserts exit 0 + `"dry_run":true`
  on stdout. Adding a flag emission to the builder without a parser update
  turns this test RED (vector mismatch AND parser rejection).
