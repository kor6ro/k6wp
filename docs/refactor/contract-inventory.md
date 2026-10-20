# K6WP — Externally-Observable Contract Inventory (Phase 0 / task 0.7)

**Status:** frozen inventory, read from source only. Every claim carries a `file:line` citation.
Where a fact cannot be established from the source it is marked **NOT ESTABLISHED** — nothing here is inferred.

**Scope:** contracts a user, a script, another process, or the filesystem can observe:
CLI surfaces, IPC wire, on-disk state, tray/UI observables, behavioural invariants.

**Binaries:** `K6WP.exe` (launcher), `engine.exe`, `studio.exe`, `compressor.exe`.
Naming: the launcher is `K6WP.exe`; the launcher and engine are Qt-free Win32 (`/SUBSYSTEM:WINDOWS`); studio is Qt Widgets + QML; compressor is a console app.

---

## AREA 1 — CLI SURFACES

### 1.1 Exit-code constants (single source of truth)

Defined once in `shared/lockscreen_exit.hpp:14-22`; re-exported by `launcher/exit_codes.hpp:12-19` (the launcher and Studio must agree — the Studio maps the launcher's exit code onto the lock-screen toggle result message, `shared/lockscreen_exit.hpp:4-12`).

| Constant | Value | Meaning | Citation |
|---|---|---|---|
| `kExitOk` | 0 | success | `shared/lockscreen_exit.hpp:16` |
| `kExitBadFlag` | 2 | bad/unknown flag, missing value, out-of-range value, invalid flag combination | `shared/lockscreen_exit.hpp:17` |
| `kExitSpawnFailure` | 3 | missing sibling binary / spawn failure / a still-helper step failed / the elevated legacy cleanup failed | `shared/lockscreen_exit.hpp:18` |
| `kExitElevateTimeout` | 4 | the elevated worker outlived its bounded wait budget | `shared/lockscreen_exit.hpp:19` |
| `kExitElevateDeclined` | 5 | the UAC prompt was dismissed | `shared/lockscreen_exit.hpp:20` |

Exit codes 3/4/5 are the contract the uninstallers read (`launcher/exit_codes.hpp:3-6`; `packaging/installer.nsi:537-548`; `packaging/uninstall.bat:90-95`).

### 1.2 `K6WP.exe` — the launcher

Parser: `launcher/cli.cpp:60-214` (`ParseArgs`). Struct: `launcher/cli.hpp:10-35` (`Options`). Dispatch: `launcher/main.cpp:249-356` (`Run`). Entry point `wWinMain` (`launcher/main.cpp:362-381`).

**Accepted flags (exact spelling, arity, default, effect):**

| Flag | Accepted form | Arity | Default when omitted | Effect | Citation |
|---|---|---|---|---|---|
| `--help`, `-h` | `--help` / `-h` | 0 | — | print usage to **stdout**, exit 0. Parsing **stops immediately** (early `return 1`), so later args are not validated | `launcher/cli.cpp:63-65`, `launcher/main.cpp:252-255` |
| `--studio` | `--studio` | 0 | `want_studio = true` (default) | ensure engine, then launch studio | `launcher/cli.cpp:66-67`, `launcher/cli.hpp:11` |
| `--engine` | `--engine` | 0 | `want_studio = true` | `want_studio = false`: ensure engine resident, open nothing | `launcher/cli.cpp:68-69` |
| `--silent` | `--silent` | 0 | `silent = false` | `want_studio = false`, `silent = true` | `launcher/cli.cpp:70-72` |
| `--minimized` | `--minimized` | 0 | `minimized = false` | sets `minimized` and **appends ` --minimized` to the forwarded engine argv** | `launcher/cli.cpp:73-75` |
| `--apply-still` | `--apply-still` | 0 | `apply_still = false` | `apply_still = true`, `want_studio = false`, `silent = true` | `launcher/cli.cpp:76-79` |
| `--restore` | `--restore` | 0 | `still_restore = false` | restore the saved originals; **only valid with `--apply-still`** | `launcher/cli.cpp:80-81`, `launcher/cli.cpp:191-198` |
| `--wallpaper` | `--wallpaper` | 0 | `still_wallpaper = false` | limit the still helper to the wallpaper side; only valid with `--apply-still` | `launcher/cli.cpp:82-83`, `launcher/cli.cpp:191-198` |
| `--lockscreen` | `--lockscreen` | 0 | `still_lockscreen = false` | limit the still helper to the lock-screen side; only valid with `--apply-still` | `launcher/cli.cpp:84-85`, `launcher/cli.cpp:191-198` |
| `--offset-s <n>` | `--offset-s` + value | 1 | studio setting `lockscreen_offset_sec` (0.0 = first frame) | frame seek offset; **double in [0, 3600]** | `launcher/cli.cpp:86-100`, `launcher/still_apply.cpp:466-468` |
| `--res <WxH>` | `--res` + value | 1 | the primary monitor's resolution | still resolution; validated by `k6wp::ParseStillRes` (positive ints only) | `launcher/cli.cpp:101-115`, `launcher/still_apply.cpp:470-475` |
| `--config <path>` | `--config <path>` **or** `--config=<path>` | 1 | absent → not forwarded | forwarded **verbatim** to `engine.exe` (value quoted) | `launcher/cli.cpp:116-130`, `launcher/cli.cpp:176-178` |
| `--video <path>` | `--video <path>` **or** `--video=<path>` | 1 | absent → not forwarded | forwarded verbatim to `engine.exe` **and** captured as the `--apply-still` input | `launcher/cli.cpp:116-130`, `launcher/cli.cpp:128`, `launcher/cli.cpp:179-181` |
| `--wallpaper-mode <m>` | `--wallpaper-mode <m>` **or** `--wallpaper-mode=<m>` | 1 | absent → not forwarded | forwarded verbatim to `engine.exe`; the launcher does **not** validate the value | `launcher/cli.cpp:116-130`, `launcher/cli.cpp:176-178` |
| `--elevate-lockscreen off` | `--elevate-lockscreen` + value | 1 | absent → no legacy cleanup | value must be **exactly** `off`; anything else → exit 2. `on` was removed with the HKLM policy flow | `launcher/cli.cpp:131-149` |
| `--elevated` | `--elevated` | 0 | `elevated_worker = false` | **HIDDEN** (not in the usage text): marks "already elevated" worker | `launcher/cli.cpp:150-151`, `launcher/cli.hpp:19` |
| `--restarted` | `--restarted` | 0 | absent | forwarded to `engine.exe` so the restart counts against the crash-restart backoff | `launcher/cli.cpp:152-156` |
| `--delay-ms <n>` | `--delay-ms` + value | 1 | `delay_ms = 0` | sleep n ms before acting; **long in [0, 600000]** | `launcher/cli.cpp:157-171`, `launcher/cli.hpp:34` |
| `--stop` | `--stop` | 0 | `stop = false` | `stop = true`, `want_studio = false`, `silent = true` | `launcher/cli.cpp:172-175` |

**Only `--config`, `--video`, `--wallpaper-mode` accept the `--flag=value` inline form** (`launcher/cli.cpp:176-182`). `--offset-s`, `--res`, `--delay-ms`, `--elevate-lockscreen` do **not** (they are matched only as a separate token) — `launcher/cli.cpp:86,101,116,131,157`.

**Any other token** (including a bare positional argument) → stderr `K6WP: error: unknown flag '<arg>'` → **exit 2** (`launcher/cli.cpp:183-186`).

**Cross-flag validation (all → exit 2):**
- `--restore` / `--wallpaper` / `--lockscreen` / `--offset-s` / `--res` without `--apply-still` → `launcher/cli.cpp:191-198`.
- `--restore` combined with `--offset-s`/`--res` → `launcher/cli.cpp:200-205`.
- `--apply-still` with neither `--video <path>` nor `--restore` → `launcher/cli.cpp:206-211`.

**`--help` / usage text, verbatim** (`launcher/cli.cpp:14-57`; printed via `fputws` so the literal `%PROGRAMDATA%` is not a conversion specifier):

```
K6WP launcher (Qt-free dispatcher)
Usage: K6WP.exe [options]
  (no args)            ensure engine, then launch studio
  --studio             same as default
  --engine             ensure engine resident, open nothing
  --engine --silent    autostart path (engine only)
  --silent             ensure engine only (autostart path)
  --minimized --silent alias of --engine --silent
  --minimized          passthrough marker forwarded to engine
  --stop               uninstall helper: stop the K6WP processes that live in this K6WP.exe's own folder (engine, Studio, launcher) - never by image name. Sends the engine an IPC quit first so it can restore the OS wallpaper, then escalates per PID.
  --config <path>      forwarded to engine.exe
  --video <path>       forwarded to engine.exe (or the video for --apply-still)
  --wallpaper-mode <m> forwarded to engine.exe
  --apply-still        per-user still helper: apply the first frame of --video as the desktop wallpaper and/or the lock-screen image (no administrator rights, no UAC)
  --apply-still --restore
                       put the saved original wallpaper and lock-screen image back
  --apply-still --wallpaper | --lockscreen
                       limit the operation to one side
  --offset-s <n>       frame seek offset (default: the studio setting, 0 = first frame)
  --res <WxH>          still resolution (default: the primary monitor)
  --elevate-lockscreen off
                       legacy cleanup only: removes the old HKLM LockScreenImage policy (UAC prompt)
  --delay-ms <n>       wait n ms (0..600000) before acting (used by the engine's hang watchdog)
  --restarted          forwarded to engine.exe so the restart counts against the crash-restart backoff
  --help, -h           show this help and exit
Exit codes: 0 ok, 2 bad flag, 3 missing sibling / spawn failure (also: the elevated legacy cleanup failed), 4 the elevated helper was still running when its wait budget expired, 5 the UAC prompt was declined.
```

**Full `K6WP.exe` exit-code contract:**

| Exit | Condition | Citation |
|---|---|---|
| 0 | success; `--help` (usage on stdout); engine ensured + studio launched (or `--engine`/`--silent` returning after the engine is resident); a second launcher that focuses the existing Studio | `launcher/main.cpp:252-255,313,326,355` |
| 2 | any bad flag / missing value / out-of-range value / invalid combination | `launcher/main.cpp:256-259`, `launcher/cli.cpp:89,97,104,114,121,135,145,160,169,185,197,204,210` |
| 3 | sibling `engine.exe` not found next to `K6WP.exe` | `launcher/main.cpp:174-178` |
| 3 | `CreateProcessW(engine.exe)` failed | `launcher/main.cpp:186-191` |
| 3 | sibling `studio.exe` not found | `launcher/main.cpp:331-335` |
| 3 | `CreateProcessW(studio.exe)` failed | `launcher/main.cpp:345-351` |
| 3 | `CommandLineToArgvW` failed | `launcher/main.cpp:375-377` |
| 3 | `--apply-still` helper failed (busy helper mutex, missing video, frame-extract failure, wallpaper/lock-screen apply failure) | `launcher/still_apply.cpp:413,442,460,484,514` |
| 3 | `--stop` could not stop one or more K6WP processes | `launcher/process_stop.cpp:327,349` |
| 3 | the elevated legacy cleanup failed (worker failed / launch failed / wait failed) | `launcher/lockscreen_sync.cpp:125,167,175,187,207,214,220` |
| 3 | `--elevate-lockscreen off` invoked without admin rights when a legacy policy exists | `launcher/lockscreen_sync.cpp:164-168` |
| 4 | the elevated worker was still running when its wait budget expired (it is terminated with exit code 4) | `launcher/lockscreen_sync.cpp:63-71,120` |
| 5 | the UAC prompt was declined (`ERROR_CANCELLED`) | `launcher/lockscreen_sync.cpp:48-55,99,118` |

**Launcher behavioural notes that are externally observable:**
- The launcher is **Qt-free** and links no Qt DLL (a release-pipeline assert enforces it, `packaging/known-limitations.md:247-249`).
- Engine start is *probe-then-spawn*: two `WaitNamedPipeW` probes at `kPipeProbeTimeoutMs` each, a 500 ms confirm gap, then a zero-timeout classify probe (`launcher/main.cpp:146-171`); the pipe-ready budget is 10 000 ms polled every 250 ms (`launcher/main.cpp:75-76,98-113`).
- On a silent/minimized start the launcher appends ` --minimized` to the engine argv if not already present (`launcher/main.cpp:181-185`).
- The launcher holds `Local\K6WP-Studio-Singleton` for the whole life of the spawned Studio, so a second `K6WP.exe` focuses the existing window and exits 0 without spawning an engine (`launcher/main.cpp:292-319`).
- `--stop` matches processes by **full path** under the launcher's own directory, never by image name (`launcher/main.cpp:24-28`, `launcher/process_stop.cpp:321-353`).

### 1.3 `engine.exe`

Parser: `engine/src/cli_options.cpp:39-188` (`ParseCli`). Options struct: `engine/src/cli_options.hpp:18-28`. Hidden-flag doc: `docs/dev-test-flags.md:1-25`.

**Accepted flags — every flag accepts BOTH `--flag value` and `--flag=value`** (`engine/src/cli_options.cpp:50-57`).

| Flag | Arity | Accepted values / effect | Exit on bad input | Citation |
|---|---|---|---|---|
| `--help`, `-h` | 0 | print usage to **stdout**; `ParseCli` returns 1 → the process exits **0** | — | `engine/src/cli_options.cpp:42-45`, `engine/src/engine_app.cpp:136` |
| `--video <path>` | 1 | mpv source path; UTF-8 → wide via `std::filesystem::u8path` | missing value → **2** | `engine/src/cli_options.cpp:67-70` |
| `--config <path>` | 1 | config JSON path; empty → `DefaultConfigPath()` | missing value → **2** | `engine/src/cli_options.cpp:71-74`, `engine/src/engine_app.cpp:1224-1230` |
| `--wallpaper-mode <m>` | 1 | exactly `auto` \| `workerw` \| `progman` | unknown value → **2** | `engine/src/cli_options.cpp:75-88` |
| `--minimized` | 0 | tray-only start (no window); canonical silent spelling | inline value → **2** | `engine/src/cli_options.cpp:89-94` |
| `--engine` | 0 | **alias of `--minimized`** (compat) | inline value → *not* checked (no `inline_value` guard) | `engine/src/cli_options.cpp:95-97` |
| `--silent` | 0 | **alias of `--minimized`** (compat) | inline value → *not* checked | `engine/src/cli_options.cpp:98-100` |
| `--restarted` | 0 | accepted and **intentionally ignored** (WER restart marker) | inline value → **2** | `engine/src/cli_options.cpp:101-107`, `engine/src/cli_options.hpp:15-17` |
| `--exit-after-ms <N>` | 1 | **HIDDEN**; shut down cleanly after N ms (0 = forever) | missing value / non-integer / `< 0` → **2** | `engine/src/cli_options.cpp:108-120`, `docs/dev-test-flags.md:10` |
| `--simulate-device-lost-after-ms <N>` | 1 | **HIDDEN**; fire `OnDeviceLost()` after N ms | missing value / non-integer / `< 0` → **2** | `engine/src/cli_options.cpp:121-134`, `docs/dev-test-flags.md:11` |
| `--simulate-suspend-after-ms <N>` | 1 | **HIDDEN**; post `PBT_APMSUSPEND` after N ms, `PBT_APMRESUMEAUTOMATIC` +2 s | missing value / non-integer / `< 0` → **2** | `engine/src/cli_options.cpp:135-148`, `docs/dev-test-flags.md:12` |
| `--simulate-dc-after-ms <N>` | 1 | **HIDDEN**; latch forced-DC + post `PBT_APMPOWERSTATUSCHANGE` after N ms, AC restore +2 s | missing value / non-integer / `< 0` → **2** | `engine/src/cli_options.cpp:149-162`, `docs/dev-test-flags.md:13` |
| `--simulate-monitor-off-after-ms <N>` | 1 | **HIDDEN**; `PBT_POWERSETTINGCHANGE` Data=0 (off) after N ms, Data=1 (on) +2 s | missing value / non-integer / `< 0` → **2** | `engine/src/cli_options.cpp:163-176`, `docs/dev-test-flags.md:14` |
| `--capture-frames <prefix>` | 1 | **HIDDEN**; write the RENDERED frames to `<prefix><n>.jpg` (QA only) | missing value → **2** | `engine/src/cli_options.cpp`, `docs/dev-test-flags.md` |
| `--capture-frames-count <n>` | 1 | **HIDDEN**; how many frames to write (default 3) | missing value / non-integer / `< 0` → **2** | `engine/src/cli_options.cpp`, `docs/dev-test-flags.md` |
| `--capture-frames-interval-ms <n>` | 1 | **HIDDEN**; delay between frames (default 1000) | missing value / non-integer / `< 0` → **2** | `engine/src/cli_options.cpp`, `docs/dev-test-flags.md` |
| `--capture-frames-seek <sec>` | 1 | **HIDDEN**; seek to this position and pause before the first frame | missing value / non-numeric / `< 0` → **2** | `engine/src/cli_options.cpp`, `docs/dev-test-flags.md` |

**Added after the first freeze (Phase 1.2).** The `--capture-frames` family
exists so the visual-parity gate compares the picture the RENDERER produced,
not what the desktop composite shows (an always-on-top overlay, a covered
desktop or a paused engine all make a screen grab record the plain desktop,
which silently turns the gate into a no-op). The capture seeks and pauses
first, so the frame is the same source frame on every run.

**Unknown `--flag` → stderr `error: unknown flag '<arg>'` + the full usage text on stderr → exit 2** (`engine/src/cli_options.cpp:177-185`).
**Bare positional argument (no `--` prefix) → warning `warning: ignoring unknown argument '<arg>'` and it is SKIPPED (exit code unaffected)** (`engine/src/cli_options.cpp:46-49`). This is a deliberate compat difference from the launcher, which rejects the same input with exit 2.

**`engine.exe` usage text, verbatim** (`engine/src/cli_options.cpp:14-22`):

```
K6WP engine
Usage: engine.exe [options]
  --video <path>                 video file to render
  --config <path>                config JSON path
  --wallpaper-mode <m>           auto | workerw | progman (injection strategy)
  --minimized                    tray-only start (autostart marker, no window)
  --engine, --silent             aliases of --minimized (compat, keep working)
  --help                         show this help and exit
```

The five hidden `--simulate-*` / `--exit-after-ms` flags are **never** printed by `--help` (`engine/src/cli_options.cpp:177-181`, `docs/dev-test-flags.md:1-6`).

**Full `engine.exe` exit-code contract:**

| Exit | Condition | Citation |
|---|---|---|
| 0 | normal run (message loop ends → `Shutdown()` → `return 0`) | `engine/src/engine_app.cpp:925-926` |
| 0 | `--help` was shown (`ParseCli` → 1 → `init_exit_code_ = 0`) | `engine/src/engine_app.cpp:135-138` |
| 0 | a second instance detected: the singleton mutex already exists → logs and exits 0 **without binding a second pipe** | `engine/src/engine_app.cpp:150-157` |
| 0 | crash-restart budget exhausted (WER relaunch): gives up and exits 0 without booting | `engine/src/main.cpp:247-258` |
| 2 | malformed arguments (unknown `--flag`, missing value, bad value, negative integer) | `engine/src/engine_app.cpp:135-138`, `engine/src/cli_options.cpp:69,73,77,87,91,105,110,114,118,122,127,131,136,141,145,150,155,159,165,169,184` |
| 2 | `CommandLineToArgvW` failed, or a wide→UTF-8 conversion failed | `engine/src/main.cpp:213-231` |

### 1.4 `compressor.exe`

Parser: `compressor/src/cli.cpp:155-325` (`ParseCli`). Options struct: `compressor/src/cli.cpp.hpp` → `compressor/src/cli.hpp:12-27`. Canonical flag spellings and bounds: `shared/compress_args.hpp:28-48` (single source of truth shared with the Studio argv builder).

**`compressor.exe` does NOT support the `--flag=value` inline form** — every value is the next argv token (`compressor/src/cli.cpp:159-165`).

| Flag | Arity | Accepted values / effect | Validation / error | Citation |
|---|---|---|---|---|
| `--help` | 0 | print usage to stdout, exit **0** | — | `compressor/src/cli.cpp:167-168`, `compressor/src/main.cpp:132-135` |
| `--in <path>` | 1 | input video | missing value → error; a `"` in the path is **rejected** (child-argv injection guard) | `compressor/src/cli.cpp:169-180` |
| `--out <path>` | 1 | output video; a `"` is rejected | same | `compressor/src/cli.cpp:181-188` |
| `--res <WxH>` | 1 | `WxH`, positive ints only | invalid → error | `compressor/src/cli.cpp:189-196`, `compressor/src/cli.cpp:34-39` |
| `--fps <n>` | 1 | int > 0, **≤ 30** | invalid → `invalid --fps value`; `> 30` → `fps must be <= 30` | `compressor/src/cli.cpp:197-207`, `shared/compress_args.hpp:45` |
| `--crf <n>` | 1 | int in **[16, 28]**, default 22 | out of range → `crf must be in range 16-28` | `compressor/src/cli.cpp:208-218`, `shared/compress_args.hpp:43-44,47` |
| `--encoder <name>` | 1 | `auto` \| `nvenc` \| `qsv` \| `amf` \| `x264`; default `auto` | unknown → error | `compressor/src/cli.cpp:41-43,219-227`, `shared/compress_args.hpp:48` |
| `--force-long` | 0 | allow inputs > 10 min | — | `compressor/src/cli.cpp:228-229` |
| `--dry-run` | 0 | validate only; print the ffmpeg cmdline, exit 0 | — | `compressor/src/cli.cpp:230-231`, `compressor/src/main.cpp:234-257` |
| `--probe-encoders` | 0 | list encoders as JSON, exit 0; **does not require `--in`** | — | `compressor/src/cli.cpp:232-233,273-279`, `compressor/src/main.cpp:141-175` |
| `--lockframe` | 0 | lockframe mode: output must be `.jpg`/`.jpeg` | non-jpg `--out` → error | `compressor/src/cli.cpp:234-235,310-322` |
| `--offset-s <n>` | 1 | double ≥ 0; default 0 | invalid → error; `> duration` → `offset-s exceeds video duration` | `compressor/src/cli.cpp:236-245,304-307` |
| `--quality <n>` | 1 | int **1..31**, default 2 (lower = better) | out of range → error | `compressor/src/cli.cpp:246-255`, `compressor/src/cli.hpp:26` |

**Post-parse validation** (`compressor/src/cli.cpp:273-322`): `--in` is required unless `--probe-encoders`; `--out` must not resolve to the same file as `--in` (lexical + NTFS-identity check, `compressor/src/cli.cpp:118-129`); duration ≤ 600 s unless `--force-long`; `--offset-s` must not exceed the duration.

**Unknown option** → stderr `{"error":"unknown option '<arg>'"}` → **exit 2**. A flag that IS in the shared argv contract but is not handled by this parser gets a **distinct** message (`"option '<arg>' is in the shared argv contract but not handled by this parser …"`) and still exits 2 (`compressor/src/cli.cpp:256-270`, `shared/compress_args.hpp:70-74`).

**`compressor.exe` usage text, verbatim** (`compressor/src/cli.cpp:131-153`):

```
Usage: compressor [options]

Compress a video file with ffmpeg.

Options:
  --in <path>          Input video file
  --out <path>         Output video file
  --res <WxH>          Target resolution, e.g. 1920x1080
  --fps <n>            Target frame rate (default 30, max 30)
  --crf <n>            CRF quality 16-28 (default 22)
  --encoder <name>     auto|nvenc|qsv|amf|x264 (default auto)
  --force-long         Allow inputs longer than 10 minutes
  --dry-run            Validate only; do not compress
  --probe-encoders     List available encoders and exit
  --help               Show this help and exit

Lockframe mode (extract a single frame as a still image):
  --lockframe          Enable lockframe mode (output is .jpg)
  --offset-s <n>       Seek offset in seconds (default 0)
  --res <WxH>          Target resolution (optional, scales frame)
  --quality <n>        JPEG quality 1-31, lower=better (default 2)
```

**Full `compressor.exe` exit-code contract:**

| Exit | Condition | Citation |
|---|---|---|
| 0 | success; `--help`; `--probe-encoders`; `--dry-run` | `compressor/src/main.cpp:132-135,174,256,293,312,337` |
| 1 | ffmpeg job failed / lockframe extract failed | `compressor/src/main.cpp:203-207,316-320` |
| 1 | "optimal" fast-path copy failed | `compressor/src/main.cpp:277-281` |
| 1 | cache fetch failed | `compressor/src/main.cpp:301-306` |
| 1 | `--out` resolves to the input file (checked again after `--out` defaulting) | `compressor/src/main.cpp:224-232` |
| 1 | building the ffmpeg cmdline failed (dry-run) | `compressor/src/main.cpp:250-254` |
| **2** | **invalid arguments** — any parse/validation error; stderr is a single JSON line `{"error":"<msg>"}` | `compressor/src/main.cpp:136-139` |

**Error output format** (externally observable): every failure prints exactly one JSON object on **stderr**: `{"error":"<escaped message>"}\n` (`compressor/src/main.cpp:137`, `compressor/src/main.cpp:204`, `compressor/src/main.cpp:226-230`, `compressor/src/main.cpp:251-253`, `compressor/src/main.cpp:278-280`, `compressor/src/main.cpp:303-305`, `compressor/src/main.cpp:317-319`). Success prints one JSON object on **stdout**: `{"ok":true,…}` (`compressor/src/main.cpp:208-209,235-237,288-292,307-311,332-336`).

**Default output path** (when `--out` is omitted): `%LOCALAPPDATA%\K6WP\wallpapers\<input filename>` (`compressor/src/main.cpp:61-76,213-218`).
**Default lockframe output** (when `--lockframe` without `--out`): `%LOCALAPPDATA%\K6WP\lockframe.jpg` (`compressor/src/main.cpp:78-92,179-181`).

### 1.5 `studio.exe`

**`studio.exe` accepts NO application CLI flags.** `studio/src/main.cpp:94` — `int main(int argc, char* argv[])`; `argv` is never inspected by the application. Only Qt's own `QApplication` argument handling (`studio/src/main.cpp:123`) consumes the command line. There is no `--help`, no `--version`, and no flag table anywhere in `studio/src/`.

**Exit codes:** `0` when a second instance detects the singleton mutex and focuses the existing window (`studio/src/main.cpp:105-110`); otherwise the process returns `QApplication::exec()`'s value (`studio/src/main.cpp:174`, default 0).

> Note: the file named `studio/src/main_window.cpp` in the task brief does **not exist**; the Studio window shell is `studio/src/qml_shell.cpp` (see §4.3).

---

## AREA 2 — IPC WIRE CONTRACT

### 2.1 Transport, pipe name, framing

| Property | Value | Citation |
|---|---|---|
| Transport | Win32 named pipe, `PIPE_ACCESS_DUPLEX \| FILE_FLAG_OVERLAPPED`, `PIPE_TYPE_MESSAGE \| PIPE_READMODE_MESSAGE \| PIPE_WAIT` | `engine/src/ipc_server.cpp:214-218` |
| Pipe name base | `\\.\pipe\k6wp-engine` | `shared/ipc_protocol.hpp:31` |
| Pipe name (per session) | `\\.\pipe\k6wp-engine-<session_id>` | `shared/ipc_protocol.hpp:33-36`, `shared/ipc_protocol.cpp:152-154` |
| Session resolution | `ProcessIdToSessionId(GetCurrentProcessId(), …)`; **falls back to session 0** on failure (never throws, never empty) | `shared/ipc_protocol.cpp:156-162`, `shared/ipc_protocol.hpp:38-42` |
| Where the engine binds it | `IpcServer::AcceptAndServe` resolves it **per accept** so a session change across restarts can never serve a stale name | `engine/src/ipc_server.cpp:209-213` |
| Where the client binds it | `IpcClient::ConnectLocked` | `studio/src/ipc_client.cpp:186-204` |
| Where the launcher probes it | `EngineAlive()` / `WaitForEnginePipe()` | `launcher/main.cpp:92-113` |
| Pipe instances | max 4 concurrent | `engine/src/ipc_server.cpp:32` |
| Pipe buffer | 128 KiB in / 128 KiB out (fits a 64 KiB payload + envelope in one datagram) | `engine/src/ipc_server.cpp:28` |
| Pipe ACL | current user's SID only (`GENERIC_ALL` allow ACE) | `shared/ipc_protocol.cpp:164-219`, `shared/ipc_protocol.hpp:152-157` |
| Default timeout | `nDefaultTimeOut = 0` | `engine/src/ipc_server.cpp:218` |

**Framing rule:** newline-terminated **NDJSON**, one JSON object per message-mode datagram. `Encode` returns `frame + "\n"` (`shared/ipc_protocol.cpp:97`). The server requires the trailing `'\n'`: **a frameless datagram (no trailing newline) gets NO reply** — the client times out (`engine/src/ipc_server.cpp:139-146`). An empty datagram likewise gets no reply (`engine/src/ipc_server.cpp:139`, `engine/src/ipc_server.cpp:347`).

**Envelope** (`shared/ipc_protocol.hpp:76-82`; built at `shared/ipc_protocol.cpp:86-90`):

```
{"version":1,"cmd":"<name>","payload":{...}}\n
```

- `version` — integer, must equal `kProtocolVersion = 1` (`shared/ipc_protocol.hpp:21`). A different value ⇒ `Decode` returns false ⇒ the server answers `{"error":"malformed IPC message …"}` (`shared/ipc_protocol.cpp:122-128`, `engine/src/ipc_server.cpp:148-151`).
- `cmd` — string; the wire name of the `Cmd` enum value.
- `payload` — object; **may be absent** (then it defaults to `{}`, `shared/ipc_protocol.cpp:136`).
- **Serialized key order is ALPHABETICAL**, not the initializer order: `nlohmann::json`'s default `object_t` is `std::map` (`shared/thirdparty/json.hpp:3399-3400,19635-19639`), so `dump()` emits `{"cmd":…,"payload":…,"version":1}`. A Rust port must not assume the initializer order.

**Maximum size:** `kMaxPayloadBytes = 64 * 1024 = 65536` bytes (`shared/ipc_protocol.hpp:44-46`). `Encode` throws `IpcError` if the **payload** exceeds it (`shared/ipc_protocol.cpp:83-85`) **and** if the whole **frame** (payload + envelope overhead) exceeds it (`shared/ipc_protocol.cpp:91-96`). `Decode` returns false if the payload exceeds it (`shared/ipc_protocol.cpp:137-139`). A datagram larger than the 128 KiB pipe buffer is drained and answered with `{"error":"message exceeds maximum size"}` (`engine/src/ipc_server.cpp:304-316,321-333`).

### 2.2 Commands

Wire names and numeric values (`shared/ipc_protocol.cpp:31-39`, `shared/ipc_protocol.hpp:58-74`; the numeric values are locked by `static_assert`, `shared/ipc_protocol.hpp:68-74`):

| # | Enum | Wire string | Notes |
|---|---|---|---|
| 0 | `Cmd::set_video` | `set_video` | |
| 1 | `Cmd::pause` | `pause` | |
| 2 | `Cmd::resume` | `resume` | |
| 3 | `Cmd::set_monitor` | `set_monitor` | |
| 4 | `Cmd::get_state` | `get_state` | default value of `IpcMessage::cmd` (`shared/ipc_protocol.hpp:80`) |
| 5 | `Cmd::quit` | `quit` | |
| 6 | `Cmd::set_display_video` | `set_display_video` | **appended after `quit`** (additive; `kProtocolVersion` stays 1) (`shared/ipc_protocol.hpp:50-57`) |

Unknown command string ⇒ `Decode` returns false ⇒ the server answers `{"error":"malformed IPC message (bad JSON, version, cmd, size, or frame)"}` — never a crash (`shared/ipc_protocol.cpp:129-135`, `engine/src/ipc_server.cpp:148-151`).

### 2.3 Payload schema per command

| cmd | payload | Validation (server side) | Citation |
|---|---|---|---|
| `set_video` | `{"path":"<utf-8 path to an existing regular file>"}` | `path` must exist, be a string, be non-empty, and point to an existing **regular file**; otherwise the command is rejected | `engine/src/ipc_marshal.cpp:52-73` |
| `set_monitor` | `{"monitor": N}` — alias `{"monitor_id": N}` | N must be an integer in **[-1, 2147483647]**; `-1` = all screens, `>= 0` = that monitor only. Non-integer or `< -1` ⇒ reject. (The range check exists because `get<int>()` on an out-of-range value wraps, e.g. `4294967295` → `-1` = "all screens") | `engine/src/ipc_marshal.cpp:27-50` |
| `set_display_video` | `{"device":"\\\\.\\DISPLAY<n>","path":"<utf-8 path>"}` **or** `{"device":"\\\\.\\DISPLAY<n>","clear":true}` | `device` must be a string matching `\\.\DISPLAY` + digits only; `path` must be a non-empty string; `clear` must be a boolean if present. **`clear` + `path` together is a STRICT reject.** `path` must be a non-empty string (the file's existence is re-checked on the main loop) | `engine/src/ipc_marshal.cpp:15-23,75-107` |
| `pause` | `{}` (the Studio sends an empty payload) | none | `studio/src/ipc_client.cpp:442` |
| `resume` | `{}` | none | `studio/src/ipc_client.cpp:444` |
| `get_state` | `{}` | none | `studio/src/ipc_client.cpp:446` |
| `quit` | `{}` | none | `studio/src/ipc_client.cpp:448` |

### 2.4 Ack shapes

| Ack | Exact bytes | Citation |
|---|---|---|
| success (all commands except `get_state`) | `{"ok":true}\n` | `engine/src/ipc_server.cpp:158,164,170,175,179,189` |
| success (`get_state`) | `{"ok":true,"state":{…}}\n` — an empty handler result becomes `{}` | `engine/src/ipc_server.cpp:180-185` |
| error | `{"error":"<escaped message>"}\n` — minimal JSON string escaping (`"`, `\`, `\n`, `\r`, `\t`, and `\u00xx` for other control chars) | `engine/src/ipc_server.cpp:72-98` |
| malformed frame | `{"error":"malformed IPC message (bad JSON, version, cmd, size, or frame)"}\n` | `engine/src/ipc_server.cpp:148-151` |
| handler threw | `{"error":"handler threw: <what>"}\n` / `{"error":"handler threw an unknown exception"}\n` | `engine/src/ipc_server.cpp:192-196` |
| oversize datagram | `{"error":"message exceeds maximum size"}\n` | `engine/src/ipc_server.cpp:311,329` |
| handler not installed | `{"error":"<cmd> handler not installed"}\n` | `engine/src/ipc_server.cpp:156,162,168,173,177,181,187` |
| handler rejected | `{"error":"<cmd> handler rejected the command"}\n` | `engine/src/ipc_server.cpp:157,163,169` |
| unreachable fallback | `{"error":"unknown command"}\n` | `engine/src/ipc_server.cpp:197` |

**"diterima" vs "selesai" contract (externally observable and important for a port):** `{"ok":true}` on `set_video` / `set_monitor` / `set_display_video` means **accepted + queued**, NOT applied. The IPC worker thread only *validates* and stashes the payload, then `PostMessageW`s a private `UINT` to the hidden window; the main loop pops it and runs the real executor (`engine/src/ipc_command_marshal.hpp:18-37`, `engine/src/ipc_command_marshal.cpp:7-83`, `engine/src/ipc_marshal.hpp`, `engine/src/ipc_server.hpp:26-35`). Clients that need certainty must call `get_state`.
Private message IDs: `kSetMonitorMessage = WM_APP+0x54`, `kSetVideoMessage = WM_APP+0x55`, `kSetDisplayVideoMessage = WM_APP+0x56` (pairwise-distinct `static_assert`s) (`engine/src/ipc_command_marshal.hpp:21-34`).

**`quit` semantics:** the engine acks `{"ok":true}` **first**, then the handler triggers shutdown asynchronously (never destroying state on the IPC worker thread) (`engine/src/ipc_server.hpp:45-48`, `engine/src/ipc_server.cpp:186-190`).

### 2.5 `get_state` response — every key and its type

Built by `EngineApp::BuildStateJson` (`engine/src/engine_app.cpp:1671-1793`). The state object is a flat append-only list; old clients ignore unknown keys.

| Key | Type | Source | Citation |
|---|---|---|---|
| `running` | bool | `running_` atomic | `engine/src/engine_app.cpp:1746` |
| `paused` | bool | `PauseController::UiPaused()` (mask & (user\|fullscreen\|suspend\|session-lock\|screen-off)) | `engine/src/engine_app.cpp:1747`, `engine/src/pause_controller.hpp:22-26` |
| `pause_mask` | int | raw pause-owner bitmask; bits: user=1, fullscreen=2, suspend=4, power=8, session-lock=16, screen-off=32 | `engine/src/engine_app.cpp:1749-1750`, `engine/src/pause_controller.hpp:13-20` |
| `pid` | unsigned 64-bit | `GetCurrentProcessId()` | `engine/src/engine_app.cpp:1751` |
| `wallpaper_mode` | string | `"auto"` \| `"workerw"` \| `"progman"` | `engine/src/engine_app.cpp:1752`, `engine/src/cli_options.cpp:27-37` |
| `video` | string (UTF-8) | current video path, `""` when none | `engine/src/engine_app.cpp:1753` |
| `config` | string (UTF-8) | resolved config path (empty `--config` ⇒ `DefaultConfigPath()`) | `engine/src/engine_app.cpp:1754`, `engine/src/engine_app.cpp:1683-1694` |
| `headless_slots` | int | number of headless slots | `engine/src/engine_app.cpp:1756` |
| `live` | bool | `slot_count() > 0` | `engine/src/engine_app.cpp:1757` |
| `monitor` | int | live monitor target, `-1` = all screens | `engine/src/engine_app.cpp:1759` |
| `thread_count` | int | current thread count | `engine/src/engine_app.cpp:1760` |
| `handle_count` | int | current handle count | `engine/src/engine_app.cpp:1761` |
| `hwdec_active` | bool | mpv `hwdec-current` | `engine/src/engine_app.cpp:1762` |
| `gpu_pin` | string | resolved d3d11-adapter pin (`""` = unpinned) | `engine/src/engine_app.cpp:1765` |
| `gpu_pin_reverts` | int | lifetime revert count | `engine/src/engine_app.cpp:1766` |
| `playlist_enabled` | bool | playlist snapshot | `engine/src/engine_app.cpp:1768` |
| `playlist_size` | int | playlist entry count | `engine/src/engine_app.cpp:1769` |
| `playlist_index` | int | current index, `-1` when not in the playlist | `engine/src/engine_app.cpp:1770` |
| `reconcile` | **object** | reconcile status snapshot | `engine/src/engine_app.cpp:1772-1783` |
| `reconcile.mode` | string | `"legacy"` \| `"shadow"` \| `"on"` | `engine/src/engine_app.cpp:1775` |
| `reconcile.last_trigger` | string | last trigger name | `engine/src/engine_app.cpp:1776` |
| `reconcile.last_verdict` | string | last verdict | `engine/src/engine_app.cpp:1777` |
| `reconcile.retries` | int | retry count | `engine/src/engine_app.cpp:1778` |
| `reconcile.degraded_keys` | array of string | degraded slot keys | `engine/src/engine_app.cpp:1779-1781` |
| `display_capability` | int | **always 1** in the current engine; `0` = engine predates the display fields | `engine/src/engine_app.cpp:1788`, `shared/ipc_protocol.hpp:97-101` |
| `display_assignments` | object: `{GDI device name → utf-8 video path}` | from the in-memory `displays.json` snapshot | `engine/src/engine_app.cpp:1708-1721,1789` |
| `display_coverage` | object: `{GDI device name → verdict}` | per-slot placement verdict | `engine/src/engine_app.cpp:1722-1744,1790` |

**`display_coverage` verdict vocabulary:** `"covered"`, `"clipped-left"`, `"clipped-top"`, `"clipped-right"`, `"clipped-bottom"`, `"headless"`, and the additive `"degraded"` (the assignment is recorded but the file no longer exists on disk) (`engine/src/engine_app.cpp:86-96,1733-1742`).

**Keys emitted by the engine but NOT consumed by `ParseEngineState`** (a port may keep them; the Studio's typed view ignores them): `running`, `pause_mask`, `wallpaper_mode`, `config`, `monitor`, `thread_count`, `handle_count`, `hwdec_active`, `gpu_pin`, `gpu_pin_reverts` — compare the emitted list (`engine/src/engine_app.cpp:1745-1791`) with the parsed list (`shared/ipc_protocol.cpp:221-302`).

**`ParseEngineState` defaulting rules** (Studio-side, `shared/ipc_protocol.cpp:221-302`): a missing key or a wrong-typed value falls back to the documented default (`pid` 0, `video` `""`, `paused` false, `headless_slots` 0, `live` true, `display_capability` 0, empty maps, `playlist_enabled` false, `playlist_size` 0, `playlist_index` -1, `reconcile_*` empty/0). Parsing never throws (`shared/ipc_protocol.cpp:299-301`).

### 2.6 Client-side discipline (observable)

- `IpcClient::Send` is one atomic transaction (connect + write + ack-read) under a mutex (`studio/src/ipc_client.cpp:263-271`).
- Ack deadline: **2 s** (`studio/src/ipc_client.cpp:330-331,362-370`; `kReadDeadlineMs`).
- Connect: `WaitNamedPipeW` 500 ms per attempt, N attempts, 10 ms × 50 cancellable retry-gap slices (`studio/src/ipc_client.cpp:193-243`).
- The client forces `PIPE_READMODE_MESSAGE` on the handle so `ReadFile` returns whole NDJSON ack lines (`studio/src/ipc_client.cpp:207-214`).
- On a broken pipe the client drops the handle and reports `kNotRunning`; on a cancel it reports `kError` and also drops the handle (a late ack would desync the next command) (`studio/src/ipc_client.cpp:284-288,317-323,335-343,353-358,367-370`).
- The client first tries `Decode()` on the ack and falls back to raw `{"ok":…}` / `{"error":…}` JSON (`studio/src/ipc_client.cpp:395-419`).

### 2.7 Documented edge behaviour

- **Frameless datagram ⇒ no reply** (client times out). This is the QA-fail path and must never crash or reply (`engine/src/ipc_server.cpp:136-146`).
- **Empty datagram ⇒ no reply** (`engine/src/ipc_server.cpp:139`).
- A datagram > 128 KiB is drained and answered `{"error":"message exceeds maximum size"}` (`engine/src/ipc_server.cpp:304-316,321-333`).
- A brutal client disconnect (`ERROR_BROKEN_PIPE` / `ERROR_PIPE_NOT_CONNECTED`) never escapes: the worker disconnects and re-listens (`engine/src/ipc_server.cpp:299-303,334-338`).
- `ERROR_PIPE_BUSY` (all 4 instances in use) is logged and retried after 250 ms (`engine/src/ipc_server.cpp:221-228`).
- The engine accepts **one client at a time** (`engine/src/ipc_server.hpp:5-7`).
- An unknown `cmd` string is rejected at decode time, so an old engine answers an error ack rather than crashing (`shared/ipc_protocol.hpp:50-57`).
- The engine never binds a second pipe when a duplicate instance is rejected by the singleton mutex (`engine/src/engine_app.cpp:150-157`).

---

## AREA 3 — ON-DISK CONTRACTS

### 3.1 `config.json` (Engine playback state)

Loader/writer: `shared/config_schema.cpp` / `.hpp`.

**Path resolution** (`DefaultConfigPath`, `shared/config_schema.cpp:278-289`):
1. `%LOCALAPPDATA%\K6WP\config.json` if `LOCALAPPDATA` is set and `< MAX_PATH`; else
2. `%USERPROFILE%\AppData\Local\K6WP\config.json` if `USERPROFILE` is set and `< MAX_PATH`; else
3. **throws `ConfigError("LOCALAPPDATA and USERPROFILE are both unset")`**.

An explicit `--config <path>` overrides the default (`engine/src/engine_app.cpp:1224-1230`).

**Schema version: `5`** (`shared/config_schema.hpp:25`). Version history: v0 legacy, v1 `monitor_id` without render effect, v2 monitor targeting, v3 legacy `"fill"` → `"cover"`, v4 `battery_mode`, v5 `cpu_affinity` + `gpu_adapter` (`shared/config_schema.hpp:18-25`).

**Every field** (`ConfigToJson`, `shared/config_schema.cpp:82-99`; struct defaults `shared/config_schema.hpp:37-57`; load defaults `MigrateConfig`, `shared/config_schema.cpp:245-269`):

| JSON key | Type | Default | Validation rule | Citation |
|---|---|---|---|---|
| `version` | int | `5` | must be in `[0, 5]`; a value > 5 or < 0 ⇒ `ConfigError("unsupported config schema version: …")` | `shared/config_schema.cpp:84,236-239` |
| `video_path` | string (UTF-8 path) | `""` | — | `shared/config_schema.cpp:85,245` |
| `fit_mode` | string | `"cover"` | one of `cover` \| `fill` \| `fit` \| `stretch` \| `center`; else `ConfigError("unknown fit_mode: …")` | `shared/config_schema.cpp:86,210-213` |
| `speed` | double | `1.0` | `[0.5, 2.0]`; RESERVED (persisted + validated, not applied by the renderer) | `shared/config_schema.cpp:87,197-199`, `shared/config_schema.hpp:41` |
| `monitor_id` | int | `-1` | `>= -1`; `-1` = all screens | `shared/config_schema.cpp:88,206-209` |
| `crf` | int | `22` | `[16, 28]` | `shared/config_schema.cpp:89,194-196` |
| `resolution_w` | int | `0` | `>= 0` (0 = monitor native) | `shared/config_schema.cpp:90,203-205` |
| `resolution_h` | int | `0` | `>= 0` (0 = monitor native) | `shared/config_schema.cpp:91,203-205` |
| `fps_cap` | int | `24` | `[1, 30]` | `shared/config_schema.cpp:92,200-202` |
| `battery_saver` | bool | `false` | — | `shared/config_schema.cpp:93,257` |
| `battery_mode` | string | `"cap24"` | `cap24` \| `static` | `shared/config_schema.cpp:94,214-216` |
| `cpu_affinity` | string | `"auto"` | `auto` \| `all` | `shared/config_schema.cpp:95,217-219` |
| `gpu_adapter` | string | `"auto"` | `auto` \| `integrated` \| `discrete` | `shared/config_schema.cpp:96,220-223` |
| `reconcile_mode` | string | `"legacy"` | `legacy` \| `shadow` \| `on` | `shared/config_schema.cpp:97,224-227` |

**Migration rules** (`MigrateConfig`, `shared/config_schema.cpp:230-276`):

| Rule | Citation |
|---|---|
| `version` outside `[0, 5]` ⇒ `ConfigError` | `shared/config_schema.cpp:236-239` |
| v0/v1 (`version < 2`) ⇒ `monitor_id` is **FORCED to `-1`** (the old id had no render effect, so preserving it would newly single out one screen) | `shared/config_schema.cpp:240-252` |
| `version < 3` and `fit_mode == "fill"` ⇒ normalized to `"cover"` | `shared/config_schema.cpp:246-250` |
| v4 and older ⇒ `battery_mode` defaults to `"cap24"` | `shared/config_schema.cpp:258-262` |
| v4 and older ⇒ `cpu_affinity` / `gpu_adapter` default to `"auto"` | `shared/config_schema.cpp:258-264` |
| `reconcile_mode` is **additive**: a v5 file without the key loads clean (no `.bak`, no version bump) and defaults to `"legacy"` — it is deliberately NOT in `NeedsMigration` | `shared/config_schema.cpp:265-269`, `shared/config_schema.hpp:51-56`, `shared/config_schema.cpp:24-28` |
| The config is **always persisted as the current version** (5) | `shared/config_schema.cpp:244` |
| A type-corrupt field ⇒ `ConfigError("config field type error: …")` | `shared/config_schema.cpp:272-274` |

**Self-healing migration trigger** (`NeedsMigration`, `shared/config_schema.cpp:17-35`): true when the root is not an object, **or** `version != 5`, **or** any of these 12 fields is absent: `video_path, fit_mode, speed, monitor_id, crf, resolution_w, resolution_h, fps_cap, battery_saver, battery_mode, cpu_affinity, gpu_adapter`.

**`.bak` backup rule** (`detail::BackupFile`, `shared/settings_io.cpp:28-35`): `copy_file(path, path + ".bak", overwrite_existing)`, best-effort, **never throws** (errors ignored). The `.bak` is **overwritten** on every backup (no rotation). `BackupFile` is called at:
- corrupt JSON (`shared/config_schema.cpp:109`)
- unknown schema version / type-corrupt field (`shared/config_schema.cpp:118`)
- migration needed (`shared/config_schema.cpp:125`)
- validation failure (`shared/config_schema.cpp:132`)
- `PersistConfigField` on a missing/corrupt/invalid file (`shared/config_schema.cpp:172`)

**Atomic-write rule** (`AtomicWriteJson`, `shared/config_schema.cpp:45-76`; contract `shared/config_schema.hpp:78-92`):
1. `create_directories(parent)` (best effort).
2. Write the JSON to **`<path>.tmp`** with `j.dump(2)` (**2-space indent**) in binary+trunc mode; `flush()`; `close()`; the stream state is checked after each step ⇒ `ConfigError` on any failure.
3. `MoveFileExW(tmp, path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)`.
4. On rename failure: remove the `.tmp` and throw `ConfigError("atomic json replace failed (err=…): …")`.

A crash/force-kill before the rename leaves the old file intact; a leftover `<config>.tmp` is ignored by `LoadConfig` and truncated on the next save — it never shadows the main file.

**Corrupt-JSON behaviour** (`LoadConfig`, `shared/config_schema.cpp:101-147`):
- `nlohmann::json::parse` throws ⇒ `BackupFile(path)` then `ConfigError("corrupt config JSON: <what>")` is thrown. The caller falls back to last-valid / safe defaults.
- `ReadFile` throws `ConfigError` when the file is larger than `kMaxConfigBytes = 1 MiB` or cannot be opened (`shared/settings_io.hpp`, `shared/settings_io.cpp:11-26`, `shared/config_schema.hpp:12-16`).
- On a successful load of a *migrated* file, the loader **rewrites** the file at the current schema (self-healing, best-effort — a read-only disk must not turn a clean load into an error) (`shared/config_schema.cpp:135-145`).

**Write ownership** (externally observable via file mtime):
- ENGINE owns single-field persist: `PersistConfigField(config_path, "video_path", …)` (`engine/src/engine_app.cpp:1286-1290`) and `PersistConfigField(config_path, "monitor_id", …)` (`engine/src/engine_app.cpp:1359`). A JSON-level merge that preserves unknown/future keys and **skips the write when the value is unchanged** (no mtime bump) (`shared/config_schema.cpp:156-191`, `shared/config_schema.hpp:94-112`).
- STUDIO owns full-file persist (`SaveConfig` → struct rewrite) (`shared/config_schema.cpp:149-154`).

### 3.2 `studio_settings.json` (Studio preferences)

Loader/writer: `shared/studio_settings.cpp` / `.hpp`.

**Path:** `%LOCALAPPDATA%\K6WP\studio_settings.json`, else `%USERPROFILE%\AppData\Local\K6WP\studio_settings.json`, else `ConfigError` (`shared/studio_settings.cpp:313-325`).

**Schema version: `5`** (`shared/studio_settings.hpp:27`).

**Every field** (`SaveStudioSettings`, `shared/studio_settings.cpp:164-182`; struct defaults `shared/studio_settings.hpp:33-81`):

| JSON key | Type | Default | Validation | Citation |
|---|---|---|---|---|
| `version` | int | `5` | `[0, 5]` | `shared/studio_settings.cpp:165,236-239` |
| `auto_compress_on_import` | bool | `true` | — | `shared/studio_settings.cpp:166,244-245` |
| `compress_output_dir` | string (path) | `%LOCALAPPDATA%\K6WP\wallpapers` | must not be empty (else `ConfigError`); an empty string in the file is replaced by the env default on load | `shared/studio_settings.cpp:167-168,200-202,246-252,298-300`, `shared/studio_settings.cpp:78-80` |
| `default_crf` | int | `22` | `[16, 28]` | `shared/studio_settings.cpp:169,192-195` |
| `default_fps` | int | `30` | `[1, 30]` | `shared/studio_settings.cpp:170,196-199` |
| `default_resolution_mode` | string | `"match_monitor"` | `match_monitor` \| `source` \| `720p` \| `1080p` \| `2160p` | `shared/studio_settings.cpp:171,209-213` |
| `start_with_windows` | bool | `false` | — (RESERVED: autostart is read from the HKCU Run value, not this field) | `shared/studio_settings.cpp:172,41` |
| `wallpaper_sync` | bool | `true` | — | `shared/studio_settings.cpp:173,265` |
| `lockscreen_sync` | bool | `true` | — (a stored value is NEVER flipped by the migration) | `shared/studio_settings.cpp:174,266` |
| `lockscreen_offset_sec` | double | `0.0` | `>= 0` | `shared/studio_settings.cpp:175,206-208` |
| `lockscreen_offer_shown` | bool | `false` | — | `shared/studio_settings.cpp:176,277-278` |
| `compress_advanced_visible` | bool | `false` | — | `shared/studio_settings.cpp:177,279-280` |
| `check_updates` | bool | `true` | — | `shared/studio_settings.cpp:178,281` |
| `closeToTray` | bool | `false` | — (camelCase key, unlike the other keys) | `shared/studio_settings.cpp:179,284` |
| `playlistSource` | string | `"all"` | `all` \| `custom`; out-of-set **normalizes** to `"all"` (never throws) | `shared/studio_settings.cpp:180,285-286`, `shared/studio_settings.cpp:218-221` |
| `performancePreset` | string | `"Seimbang"` | `Hemat` \| `Seimbang` \| `Maksimal`; out-of-set **normalizes** to `"Seimbang"` | `shared/studio_settings.cpp:181,287-289`, `shared/studio_settings.cpp:222-226` |
| `cache_dir` | string (path) | `%LOCALAPPDATA%\K6WP\cache` | must not be empty; empty string → env default | `shared/studio_settings.cpp:182,203-205,290-295,301-303`, `shared/studio_settings.cpp:82-84` |

**Migration rules** (`MigrateStudioSettings`, `shared/studio_settings.cpp:229-311`):
- `version` outside `[0, 5]` ⇒ `ConfigError("unsupported studio settings schema version: …")` (`shared/studio_settings.cpp:236-239`).
- v5 offset rule: `version <= 4` **and** `lockscreen_offset_sec == 1.0` **exactly** ⇒ `0.0`; any other value is preserved (`shared/studio_settings.cpp:269-274`).
- v3 removed `keep_original` / `start_minimized`: old files may still carry them, they are **deliberately not read** (`shared/studio_settings.cpp:257-258`).
- `playlistSource` / `performancePreset` use `TolerantEnumString`: a missing key, a non-string value, or an out-of-set value all normalize to the default and **never fail the load** (`shared/studio_settings.cpp:40-60,285-289`).
- `wallpaper_sync` missing (v ≤ 4) ⇒ `true`; `lockscreen_sync` **keeps** its stored value (`shared/studio_settings.cpp:261-266`).
- The file is **always persisted as version 5** (`shared/studio_settings.cpp:243`).
- A type-corrupt field ⇒ `ConfigError("studio settings field type error: …")` (`shared/studio_settings.cpp:306-308`).

**Self-healing migration trigger** (`NeedsSettingsMigration`, `shared/studio_settings.cpp:15-38`): `version != 5` **or** any of these 15 fields absent: `auto_compress_on_import, compress_output_dir, default_crf, default_fps, default_resolution_mode, start_with_windows, cache_dir, lockscreen_sync, lockscreen_offset_sec, wallpaper_sync, compress_advanced_visible, check_updates, closeToTray, playlistSource, performancePreset`.

**`.bak` / atomic-write / corrupt-JSON behaviour:** identical to `config.json` — `BackupFile` before the throw, `AtomicWriteJson` publish, self-healing rewrite on migration (`shared/studio_settings.cpp:110-156,188`).

**Engine-read allowlist (LOW-16):** the Engine may observe **exactly three** fields of this file — `wallpaper_sync`, `lockscreen_sync`, `lockscreen_offset_sec` — read through one cached view (`ReadStillSettingsCached`, `shared/still.hpp:61-85`). A missing or unreadable file yields **both flags OFF** (`readable = false`); a missing key in an existing file yields the per-key default. Every other field is Studio-private (`shared/still.hpp:61-65`).

### 3.3 Other on-disk state (path pattern + writer)

All under `%LOCALAPPDATA%\K6WP\` unless noted. The path helper pattern is `LOCALAPPDATA` → `USERPROFILE\AppData\Local` → temp dir.

| Path | Written by | Notes / contract | Citation |
|---|---|---|---|
| `K6WP\config.json` | Engine (single field), Studio (full file) | §3.1 | `shared/config_schema.cpp:278-289` |
| `K6WP\config.json.tmp` | `AtomicWriteJson` | transient; ignored by the loader | `shared/config_schema.cpp:51-52,78-82` |
| `K6WP\config.json.bak` | `detail::BackupFile` | last backup; overwritten each time | `shared/settings_io.cpp:28-35` |
| `K6WP\studio_settings.json` | Studio | §3.2 | `shared/studio_settings.cpp:313-325` |
| `K6WP\studio_settings.json.bak` / `.tmp` | Studio / `AtomicWriteJson` | same contract as config.json | `shared/studio_settings.cpp:118,134,142,188` |
| `K6WP\displays.json` | Engine (`SaveDisplays`) / Studio | **sibling of `config.json`** ⇒ `<config dir>/displays.json` | `shared/displays_schema.cpp:209-211` |
| `K6WP\playlist.json` | Studio (writer), Engine (reader) | sibling of `config.json`; `PlaylistPathForConfig(config_path)` = `config_path.parent_path()/playlist.json` | `shared/playlist.cpp:209-216` |
| `K6WP\engine.log` | Engine | appended; opened with `_SH_DENYNO` so external readers are not blocked | `engine/src/log_file.cpp:59-83,137-138` |
| `K6WP\engine.log.1` | Engine | rotation at **10 MiB**; the previous backup is deleted first; a failed rotation keeps the current file and backs off 60 s | `engine/src/log_file.cpp:42,89-117` |
| `K6WP\crashes\engine-YYYYMMDD-HHMMSS.dmp` | Engine crash filter | minidump; directory capped at `kMaxCrashDumps = 10`; the prune runs **before** the write | `engine/src/main.cpp:44-131,60-83`, `engine/src/crash_dump_prune.hpp:23,32` |
| `K6WP\crash_restart.txt` | Engine | single `count|tick` line; `count|tick` format | `engine/src/main.cpp:145-177` |
| `K6WP\stills\still_<20-digit FILETIME>.jpg` | `K6WP.exe --apply-still` helper | one unique file per apply; the 2 newest are kept, the rest pruned | `launcher/still_apply.cpp:86-96,50,105-131` |
| `K6WP\still_status.json` | helper (writer), Studio (reader) | schema version `1`; keys `version, ts, ok, wallpaper_ok, lockscreen_ok, error, lockscreen_mode_hint` | `shared/still.cpp:24,427-437`, `shared/still.hpp:42-44,145-161` |
| `K6WP\original_wallpaper.json` | helper | the pre-K6WP desktop wallpaper path (PERSISTENT) | `shared/still.hpp:46-50`, `shared/still.cpp:162-166` |
| `K6WP\original_lockscreen.json` | helper | previous lock-screen mode (spotlight/slideshow registry values) | `shared/still.hpp:52-55`, `shared/still.cpp:172-176` |
| `K6WP\original_lockscreen.jpg` | helper | the previous lock-screen image | `shared/still.hpp:57-59`, `shared/still.cpp:182-186` |
| `K6WP\wallpapers\` | compressor (default `--out` dir) | `%LOCALAPPDATA%\K6WP\wallpapers` | `compressor/src/main.cpp:61-76,213-218` |
| `K6WP\cache\<hex>.mp4` | compressor cache | root `%LOCALAPPDATA%\K6WP\cache`; override `K6WP_CACHE_DIR`; budget 5 GiB, override `K6WP_CACHE_MAX_BYTES` | `compressor/src/cache_manager.cpp:28-59,104`, `compressor/src/cache_manager.hpp:12-32` |
| `K6WP\library.json` | Studio | the video library index | `studio/src/library_manager.cpp:37`, `studio/src/thumbnailer.cpp:156-161` |
| `K6WP\thumbs\` | Studio thumbnailer | `%LOCALAPPDATA%\K6WP\thumbs` | `studio/src/thumbnailer.cpp:273-281` |
| `K6WP\shadercache\` | Engine (mpv `gpu-shader-cache-dir`) | `%LOCALAPPDATA%\K6WP\shadercache`; if `LOCALAPPDATA` is unset the mpv default relative dir `shadercache` is used instead | `engine/src/mpv_renderer.cpp:208-223` |
| `K6WP\lockframe.jpg` | compressor (lockframe default `--out`) | fallback only | `compressor/src/main.cpp:78-92` |
| `%PROGRAMDATA%\K6WP\lockscreen.jpg` + `lockscreen.backup` | LEGACY (old HKLM policy flow) | removed by `K6WP.exe --elevate-lockscreen off` | `launcher/lockscreen_policy.cpp:62`, `launcher/lockscreen_sync.cpp:86-91,177-178` |
| `HKCU\Software\K6WP\WallpaperBackup` value `OsWallpaper` | NSIS installer | install-time OS-wallpaper backup, restored on uninstall | `packaging/installer.nsi:31,91,502-503` |

**Env-var overrides that are externally observable:** `K6WP_RECONCILE_MODE` (`legacy`/`shadow`/`on`, wins over the config value and locks it) (`engine/src/engine_app.cpp:249-259`), `K6WP_CACHE_DIR`, `K6WP_CACHE_MAX_BYTES` (`compressor/src/cache_manager.cpp:29,47`), `K6WP_LIBRARY_JSON` (overrides the `library.json` path — `studio/src/thumbnailer.cpp:150-153`, `studio/src/library_manager.cpp:97`), `K6WP_PLAYLIST_JSON` (overrides the `playlist.json` path — `studio/src/playlist_bridge.cpp:19`).

---

## AREA 4 — TRAY / UI OBSERVABLES

> The file `engine/src/tray_menu*` does not exist; the engine's tray menu is built inline in `engine/src/tray.cpp`. There is no `studio/src/main_window.cpp`; the Studio shell is `studio/src/qml_shell.cpp`.

### 4.1 Engine tray menu

Built in `TrayIcon::OnTrayNotify` (`engine/src/tray.cpp:228-252`). **All labels are Indonesian** (the engine has no translation infra by design, `engine/src/tray.cpp:234-235`).

| Order | Exact label | Command id | Verb / effect | Enabled state | Citation |
|---|---|---|---|---|---|
| 1 | `&Buka K6WP` | `kTrayCmdOpenStudio` = 1002 | Open Studio: `K6WP.exe --studio` (launcher singleton) → falls back to focusing the window titled `K6WP Studio` → falls back to spawning `studio.exe` directly | always | `engine/src/tray.cpp:235`, `engine/src/tray.hpp:48`, `engine/src/tray_actions.cpp:21-90` |
| 2 | `&Jeda` | `kTrayCmdPause` = 1005 | Pause (same callback as Resume, guarded by the live paused state) | **grayed** when already paused | `engine/src/tray.cpp:239-240`, `engine/src/tray.hpp:55`, `engine/src/tray.cpp:298-304` |
| 3 | `&Lanjut` | `kTrayCmdResume` = 1006 | Resume | **grayed** when already running | `engine/src/tray.cpp:241-242`, `engine/src/tray.hpp:56`, `engine/src/tray.cpp:305-311` |
| 4 | `Wallpaper &berikutnya` | `kTrayCmdNextWallpaper` = 1004 | Next wallpaper: cycles the MRU (`index = (current + 1) % size`; an unknown current wraps to index 0 = newest) | **grayed** when the MRU is empty | `engine/src/tray.cpp:245-246`, `engine/src/tray.hpp:50`, `engine/src/tray.cpp:318-345` |
| 5 | `Pindah &cepat` (submenu) | `kTrayCmdQuickBase` = 1100 … 1104 (`kTrayCmdQuickMax` = 5) | Quick-switch to MRU entry *i*; the label is the **file name only** | the submenu is created **only when the MRU is non-empty** | `engine/src/tray.cpp:62-82,248`, `engine/src/tray.hpp:60-61`, `engine/src/tray.cpp:349-353` |
| 6 | *(separator)* | — | — | — | `engine/src/tray.cpp:249` |
| 7 | `&Dukung pengembang` | `kTrayCmdSupport` = 1007 | Open the donation URL (`K6WP_DONATE_URL`) in the browser | always | `engine/src/tray.cpp:250`, `engine/src/tray.hpp:59`, `engine/src/tray_actions.cpp:92-113` |
| 8 | `&Keluar` | `kTrayCmdExit` = 1003 | Quit the engine | always | `engine/src/tray.cpp:251`, `engine/src/tray.hpp:49`, `engine/src/tray.cpp:346-348` |

**Tray icon observables** (`engine/src/tray.cpp`):
- Tooltip (exactly one of three, error wins over paused) (`engine/src/tray.cpp:390-392`):
  - normal: `K6WP Engine`
  - paused: `K6WP Engine - Dijeda`
  - error: `K6WP Engine - Wallpaper tidak tampil`
- Icon: resource `101` = running, `102` = paused (`engine/src/tray.hpp:43-44`); the error state keeps icon `101` (`engine/src/tray.cpp:149-154`). Fallback to the stock shared icons `IDI_APPLICATION` (32512) / `IDI_EXCLAMATION` (32515) when the `.rc` is missing (`engine/src/tray.cpp:33-36,111-112`).
- **Left click does nothing**; only `WM_RBUTTONUP` / `WM_CONTEXTMENU` opens the menu (`engine/src/tray.cpp:207,264-277`). `WM_LBUTTONDBLCLK` opens Studio (`engine/src/tray.cpp:264-277`).
- The menu is shown at the cursor with `TPM_RIGHTBUTTON | TPM_BOTTOMALIGN | TPM_LEFTALIGN` (`engine/src/tray.cpp:258-259`).
- The icon is re-added on `TaskbarCreated` (Explorer restart) (`engine/src/tray.cpp:361-372`).
- The MRU keeps at most 5 entries, de-duplicated, newest first (`engine/src/tray.cpp:32,189-201`).

### 4.2 Launcher tray

**The launcher has NO tray icon.** `launcher/` contains no `Shell_NotifyIcon` usage at all (the only `tray` matches in `launcher/*.cpp` are comments) — the launcher is a short-lived Qt-free dispatcher (`launcher/main.cpp:1-28`). The only resident tray icon in the product is the engine's.

### 4.3 Studio pages / sections a user can reach

Studio window: `QmlShell` (a `QMainWindow`), title `K6WP Studio`, initial size 960×640, `QQuickWidget` min size 480×320, shown maximized (`studio/src/qml_shell.cpp:78-100,169`; `studio/src/main.cpp:124,168-173`).

**Top-level navigation = a collapsible left sidebar with exactly TWO entries** (`studio/qml/Main.qml:79-86,543-558`):

| # | Label | Page component | Citation |
|---|---|---|---|
| 0 | `Beranda` | `WallpaperPage` | `studio/qml/Main.qml:84,543-548,584-603` |
| 1 | `Pengaturan` | `SettingsPage` | `studio/qml/Main.qml:85,553-558,608-626` |

(The `Koleksi` gallery is **part of the `Beranda` page**, not a separate sidebar entry — `studio/qml/Main.qml:80-83`. The former "compressor tab" is gone; compression is a background job surfaced by the ToastBar — `studio/qml/Main.qml:82-83`.)

**`Beranda` (WallpaperPage) sub-regions** (`studio/qml/WallpaperPage.qml:4-6,507,532,546`):
- Left column: `CollectionPage` — the `Koleksi` gallery + the `Ganti otomatis` panel (`AutoSwitchPanel`) + `VideoGrid` (`studio/qml/CollectionPage.qml:162,199`).
- Right rail: `StatusBar` (the one action row: Jeda/Lanjut, Pilih video, Pasang, Hapus, Aktifkan lagi) and `QuickSettingsPanel` (Tampilan / Isi layar) (`studio/qml/WallpaperPage.qml:532,546`; `studio/qml/ActionButtons.qml:35-66`; `studio/qml/QuickSettingsPanel.qml:93-98`).
- An `EmptyState` overlay when the library is empty (`studio/qml/EmptyState.qml:62-138`).
- A `PreviewHole` that hosts the native mpv preview surface (`studio/qml/PreviewHole.qml:34`).

**`Pengaturan` (SettingsPage) sections** (`studio/qml/SettingsPage.qml`):

| Section | Exact title | Citation |
|---|---|---|
| 1 | `Umum` | `studio/qml/SettingsPage.qml:635` |
| 2 | `Tampilan` | `studio/qml/SettingsPage.qml:750` |
| 3 | `Hemat daya` | `studio/qml/SettingsPage.qml:838` |
| 4 | `Lanjutan` | `studio/qml/SettingsPage.qml:947` |

Sub-sections inside `Lanjutan`: `Nilai mentah` (953), `Perkecil manual` (1198), `Bawaan impor` (1339), `Cache` (1436), `Diagnostik` (1475) — `studio/qml/SettingsPage.qml:953,1198,1339,1436,1475`.

**Overlays / dialogs**:
- First-run onboarding overlay (`OnboardingView`) — one full-window screen, not a page (`studio/qml/Main.qml:661-676`; `studio/qml/OnboardingView.qml:311-419`).
- `AppDialogs`: `Info teknis` (221), `Video panjang` (375), `Video besar` (462), `Tentang K6WP Studio` (579), `Pembaruan` (706) — `studio/qml/AppDialogs.qml`.
- `CacheDialog`: `Bersihkan cache` (`studio/qml/CacheDialog.qml:21`).
- `AssignPopup` / `AssignRow`: `Pasang ke` (`studio/qml/AssignPopup.qml:151`; `studio/qml/AssignRow.qml:65`).
- `LogPanel`: `Info teknis` (`studio/qml/LogPanel.qml:27`).
- `ToastBar` at the window bottom edge (`studio/qml/Main.qml:632-639`).

**Studio tray** (only present when `closeToTray` is ON; `studio/src/qml_shell.cpp:204-216,218-254`):

| Order | Exact label | Action | Citation |
|---|---|---|---|
| 1 | `Buka K6WP Studio` | show/raise/activate the window | `studio/src/qml_shell.cpp:227,235` |
| 2 | `Jeda` | pause the engine (IPC) | `studio/src/qml_shell.cpp:228,236-241` |
| 3 | `Lanjut` | resume the engine (IPC) | `studio/src/qml_shell.cpp:229,242-247` |
| 4 | *(separator)* | — | `studio/src/qml_shell.cpp:230` |
| 5 | `Keluar` | release the singleton mutex, then `qApp->quit()` | `studio/src/qml_shell.cpp:231,248,275-280` |

Tooltip: `K6WP Studio`, or `K6WP Studio — <status>` when a status is available (`studio/src/qml_shell.cpp:256-266`). The Studio tray deliberately does **not** carry the engine's quick-switch MRU (`studio/src/qml_shell.cpp:224-226`).

### 4.4 Injected wallpaper window chrome (engine)

Class `K6WP.DesktopInject.1`, window title `K6WP Wallpaper` (`engine/src/desktop_inject.cpp:64,402`).

| Property | Value | Citation |
|---|---|---|
| WndProc | `InjectedWndProc` → `DefWindowProcW` (no custom handling) | `engine/src/desktop_inject.cpp:256-261` |
| Class background brush | `GetStockObject(BLACK_BRUSH)` | `engine/src/desktop_inject.cpp:368` |
| Create window style | `WS_POPUP \| WS_CLIPCHILDREN` | `engine/src/desktop_inject.cpp:403` |
| Create window ex-style | layered ? `WS_EX_LAYERED \| WS_EX_TOOLWINDOW \| WS_EX_NOACTIVATE` : `0` | `engine/src/desktop_inject.cpp:401` |
| **Style enforcement** (`EnforceFramelessStyle`) | `style \|= WS_POPUP \| WS_CLIPCHILDREN`; `style &= ~(WS_OVERLAPPEDWINDOW \| WS_CAPTION \| WS_SYSMENU \| WS_THICKFRAME \| WS_MINIMIZEBOX \| WS_MAXIMIZEBOX)` | `engine/src/desktop_inject.cpp:239-243` |
| **Ex-style enforcement** | `ex \|= WS_EX_TOOLWINDOW \| WS_EX_NOACTIVATE`; `ex \|= WS_EX_LAYERED` when layered; `ex &= ~WS_EX_APPWINDOW` | `engine/src/desktop_inject.cpp:244-248` |
| Non-client refresh | `SetWindowPos(…, SWP_NOMOVE \| SWP_NOSIZE \| SWP_NOZORDER \| SWP_NOACTIVATE \| SWP_FRAMECHANGED)` | `engine/src/desktop_inject.cpp:249-251` |
| Layered opacity | `SetLayeredWindowAttributes(wnd, 0, 255, LWA_ALPHA)` | `engine/src/desktop_inject.cpp:410-412` |
| Parenting | created top-level `WS_POPUP` first, then `SetParent(wnd, target)` (cross-process `CreateWindowEx` with a foreign parent fails with error 5); `WS_EX_LAYERED`/`WS_EX_TOOLWINDOW` are **re-applied after `SetParent`** because `SetParent` strips them | `engine/src/desktop_inject.cpp:5-11,380-382,409-423` |
| Z-order | `SetWindowPos(wnd, z_insert_after, …)`; `z_insert_after` must be a **sibling** of the target (the child is `WS_POPUP`, so `GetParent` on it returns its OWNER, not the host) — otherwise `HWND_BOTTOM` with reason `z-order: sibling-mismatch-fallback` | `engine/src/desktop_inject.cpp:34-52,447` |
| Purpose of the styles | no taskbar button, no Alt-Tab entry, no activation, no caption | `engine/src/desktop_inject.cpp:232-236` |

**Engine's hidden message window** (also observable via `FindWindow`): class `K6WP.Engine.MessageWindow.1`, title `K6WP Engine`, `dwExStyle = 0`, `dwStyle = WS_POPUP`, 0×0, never shown (`engine/src/engine_app.hpp:119`, `engine/src/engine_app.cpp:2254-2257`). It is a real top-level window (not `HWND_MESSAGE`) so it receives `TaskbarCreated`, `WM_DISPLAYCHANGE` and `WM_POWERBROADCAST` (`engine/src/engine_app.cpp:2249-2253`).

---

## AREA 5 — BEHAVIOURAL INVARIANTS

### 5.1 Singleton mutex names

| Mutex name | Created by | Purpose | Citation |
|---|---|---|---|
| `Local\K6WP-Engine-Singleton` | **Engine** (`EngineApp::Init`) | engine single instance; a duplicate exits 0 without binding a second pipe | `engine/src/engine_app.hpp:120-121`, `engine/src/engine_app.cpp:145-157` |
| `Local\K6WP-Studio-Singleton` | **Launcher** | per-user launcher/studio singleton; held for the launcher's lifetime so a second `K6WP.exe` focuses the existing Studio and exits 0 | `launcher/main.cpp:62,292-319` |
| `Local\K6WP-Studio-App-Singleton` | **Studio** (`studio.exe`) | studio's own singleton; a second instance focuses the existing `QWindowIcon` window and exits 0 | `studio/src/main.cpp:20-23,101-117` |
| `Local\K6WP-Still-Helper` | **Launcher** (`--apply-still` path) | serialises still helpers; wait budget `kHelperMutexWaitMs = 120000` ms; on timeout the helper publishes a "busy" status and exits 3 | `launcher/still_apply.cpp:49,404-414` |

All four are in the `Local\` namespace ⇒ per-session, per-user.

### 5.2 Autostart mechanism

| Property | Value | Citation |
|---|---|---|
| Registry location | `HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Run` | `shared/autostart.cpp:17-18` |
| Value name | `K6WP` (REG_SZ) | `shared/autostart.hpp:19`, `shared/autostart.cpp:148-151` |
| Value written (normal) | `"<path>\K6WP.exe" --engine --silent` | `shared/autostart.cpp:112-119` |
| Value written (legacy fallback, only when no sibling `K6WP.exe` exists) | `"<path>\engine.exe" --minimized` | `shared/autostart.cpp:101-107,116-118` |
| Target selection | if the caller **is** `K6WP.exe` use itself; else prefer `<exe_dir>\K6WP.exe`; else fall back to `<exe_dir>\engine.exe` | `shared/autostart.cpp:80-109` |
| Who writes it | **`shared/autostart.cpp::SetAutostart`** — called only from the Studio: `studio/src/settings_bridge.cpp:265` and `studio/src/studio_bridge.cpp:779`. The **engine never touches it** (no `SetAutostart` call in `engine/`) | `shared/autostart.cpp:133-180` |
| Who reads it | `IsAutostart()` (HKCU Run value exists) — `studio/src/settings_bridge.cpp:159`, `studio/src/studio_bridge.cpp:530` | `shared/autostart.cpp:122-131` |
| Disable | `RegDeleteValueW`; deleting a missing value succeeds (returns true) | `shared/autostart.cpp:161-179` |
| Installer equivalent | `WriteRegStr HKCU "${RUN_SUBKEY}" "${RUN_VALUE}" '"$INSTDIR\K6WP.exe" --engine --silent'` (installer checkbox, default on) | `packaging/installer.nsi:24-25,88-89,352-356` |
| Uninstaller | `reg delete "HKCU\Software\Microsoft\Windows\CurrentVersion\Run" /v K6WP /f` | `packaging/uninstall.bat:61-67` |
| No shortcut-based autostart | the installer writes **Start Menu + Desktop** shortcuts whose target is `K6WP.exe` only (never `engine.exe`), but autostart itself is the Run value, not a shortcut | `packaging/installer.nsi:12,346-349` |
| Note | `studio_settings.json::start_with_windows` is **RESERVED** — persisted but NOT read by production code; the HKCU Run value is the single source of truth | `shared/studio_settings.hpp:41`, `shared/studio_settings.cpp:259-260` |

### 5.3 Engine restore behaviour on exit

| Rule | Detail | Citation |
|---|---|---|
| Normal exit (tray `Keluar`, restart, `WM_CLOSE`) | `Shutdown()` calls `RequestStillRestore(/*wallpaper=*/true, /*lockscreen=*/true)` — **detached, never waited** | `engine/src/engine_app.cpp:987-993` |
| Session end (shutdown / logoff) | **NO restore.** `WM_QUERYENDSESSION` sets `session_ending_ = true`; `WM_ENDSESSION` with `wParam != 0` does the same. `Shutdown()` then skips the restore entirely. Rationale: the desktop the user returns to is the one Windows persists | `engine/src/engine_app.cpp:2045-2062`, `engine/src/engine_app.cpp:988-993` |
| What the engine does **not** do | since v5 the engine **no longer snapshots or restores the OS wallpaper** itself; the per-user still helper owns it through the PERSISTENT store (`original_wallpaper.json`) | `engine/src/engine_app.cpp:159-162` |
| What the restore actually is | `RequestStillRestore` spawns a detached `K6WP.exe --apply-still --restore` (plus `--wallpaper` or `--lockscreen` when only one side is requested); it returns false when both flags are false | `shared/still.cpp:491-501` |
| What the helper does on restore | wallpaper side: `RestoreOriginalWallpaper()` (only if the store is non-empty); lock-screen side: `RestoreLockscreen()`; writes `still_status.json`; exit 0 on success, 3 on failure | `launcher/still_apply.cpp:421-443` |
| Studio-side restore | the Studio also calls `RequestStillRestore(true,false)` (`studio/src/settings_bridge.cpp:371`) and `RequestStillRestore(false,true)` (`studio/src/settings_bridge.cpp:395`) when the user toggles the two switches | `shared/still.hpp:163-172` |
| Teardown order in `Shutdown()` | hang watchdog → **IPC server first** (joins the worker so no handler can touch a destroyed renderer) → config watcher → headless renderer → `ShutdownWallpaperSurface()` → `tray_.Remove()` → `fullscreen_watch_.Stop()` → `UnregisterPowerSettingNotification` → destroy the hidden window → unregister the window class → **then** the still restore → `FlushEngineLog()` | `engine/src/engine_app.cpp:929-996` |
| Uninstaller restore | `K6WP.exe --apply-still --restore` is run by the NSIS uninstaller and by `uninstall.bat` before the data folder is removed | `packaging/installer.nsi:511-518`, `packaging/uninstall.bat:69-81` |

### 5.4 Compressor exit code for invalid arguments — VERIFIED

**Contract: invalid arguments ⇒ exit 2.** Quoted from the source:

```cpp
  if (result == CliResult::Error) {
    std::fprintf(stderr, "{\"error\":\"%s\"}\n", JsonEscape(error).c_str());
    return 2;
  }
```
— `compressor/src/main.cpp:136-139`

The error is emitted as exactly one JSON object on **stderr** (`{"error":"…"}`). Note the contrast: a **runtime** failure (ffmpeg job failed, cache fetch failed, in-place guard) exits **1** (`compressor/src/main.cpp:203-207,277-281,301-306,316-320,224-232`). The golden contract test replays `--dry-run` and asserts exit 0 (`docs/dev-contracts.md:518-529`); a bogus flag produces `{"error":"unknown option…"}` with exit 2 (`docs/compliance-matrix.md:44`).

### 5.5 Engine minimum-OS version check

**NOT ESTABLISHED — no runtime minimum-OS version check exists in the source.**

Evidence of the search:
- No `GetVersionExW`, `RtlGetVersion`, `VerifyVersionInfoW`, `IsWindows10OrGreater`, `OSVERSIONINFO`, or any build-number constant (`0x0A000002`, `22000`, `19041`) appears anywhere in `shared/`, `engine/`, `launcher/`, `studio/`, or `compressor/` (grep for `GetVersion|OSVERSION|IsWindows|minimum|min_os|MinOS` matched only unrelated comments, e.g. `compressor/src/encoder_detect.cpp:119-120` and `studio/src/qml_shell.cpp:95`).
- The only version requirement in the repo is **documentation**: `README.md:83` — "Requirements: Windows 10 1703+ (PerMonitorV2) / Windows 11, 64-bit."
- The closest runtime logic is a **layout** detection, not a version gate: `IsWin11_24H2(progman)` checks `WS_EX_NOREDIRECTIONBITMAP` (0x00200000) on the Progman window to pick the 24H2 layered-child injection path (`engine/src/desktop_inject.cpp:213-218,71-73`). `packaging/known-limitations.md:16-19` explicitly labels the Win10 / pre-24H2 branches as **VM-only / code-quoted** (never run on this machine), i.e. the product relies on the *layout* fallback rather than a version check.

**Parity implication for the Rust port:** a Rust engine must reproduce the *layout detection* (24H2 vs classic) but there is **no version gate to reproduce** — a port that adds one would be a behaviour change.

---

## APPENDIX — Cross-reference of contract sources

| Contract | Source of truth |
|---|---|
| Exit codes | `shared/lockscreen_exit.hpp:14-22` (re-exported by `launcher/exit_codes.hpp:12-19`) |
| Launcher CLI | `launcher/cli.cpp:60-214`, `launcher/cli.hpp:10-35` |
| Engine CLI | `engine/src/cli_options.cpp:39-188`, `engine/src/cli_options.hpp:18-28` |
| Compressor CLI | `compressor/src/cli.cpp:155-325`, `shared/compress_args.hpp:28-48` |
| IPC envelope / commands / size | `shared/ipc_protocol.hpp:20-150`, `shared/ipc_protocol.cpp:31-150` |
| IPC server / acks | `engine/src/ipc_server.cpp:96-198` |
| IPC payload validators | `engine/src/ipc_marshal.cpp:15-107` |
| `get_state` shape | `engine/src/engine_app.cpp:1671-1793` (emitter), `shared/ipc_protocol.cpp:221-302` (parser) |
| `config.json` | `shared/config_schema.cpp:45-289`, `shared/config_schema.hpp:12-132` |
| `studio_settings.json` | `shared/studio_settings.cpp:15-327`, `shared/studio_settings.hpp:10-140` |
| `.bak` / read cap | `shared/settings_io.cpp:11-35` |
| Autostart | `shared/autostart.cpp:73-180`, `shared/autostart.hpp:7-34` |
| Still / restore | `shared/still.hpp:37-181`, `shared/still.cpp:143-505`, `launcher/still_apply.cpp:401-515` |
| Engine tray menu | `engine/src/tray.cpp:203-359`, `engine/src/tray.hpp:36-72` |
| Studio shell / pages | `studio/src/qml_shell.cpp:78-354`, `studio/qml/Main.qml:79-120,440-729` |
| Injected window chrome | `engine/src/desktop_inject.cpp:232-252,362-449` |
| Prior prose contracts (keep in sync) | `docs/dev-contracts.md`, `docs/dev-test-flags.md`, `packaging/known-limitations.md` |
