# Contributing to K6WP

Thanks for helping with K6WP. This doc covers setup, conventions (from the
repo docs), testing, and PR expectations.

## Build setup

- Toolchain: Visual Studio 2022+ (MSVC x64), CMake 3.25+, Qt 6.8.x
  `msvc2022_64` with `QT_ROOT` set (e.g. `$env:QT_ROOT = "C:\Qt\6.8.3\msvc2022_64"`).
- Vendored deps are **fetched, not committed**: run `.\tools\fetch_vendor.ps1`
  once after cloning (it verifies each file against `vendor/VERSIONS.md` and
  needs 7-Zip for the libmpv dev package). It fetches ffmpeg 8.1.2, libmpv
  0.41.0-dev and `shared/thirdparty/json.hpp` (nlohmann/json 3.11.3) — ~311 MB
  in total, too large for git because `libmpv-2.dll` is over GitHub's 100 MB
  per-file limit. Committed instead: `vendor/VERSIONS.md` (the pin manifest),
  `vendor/vc_runtime/` (app-local MS CRT) and `vendor/libmpv/lib/mpv.lib`
  (generated locally from the DLL's exports, not downloadable).
  `vcpkg.json` is a catalog placeholder only, not build instructions — vcpkg is
  intentionally not used for builds.
- Configure + build + test (from the repo root):

  ```powershell
  cmake --preset msvc-dev
  cmake --build --preset msvc-dev
  ctest --preset msvc-dev
  ```

- The `release` preset sets `BUILD_TESTING=OFF` (no test targets) by design;
  use `msvc-dev` for test runs.
- Release packaging: `powershell -ExecutionPolicy Bypass -File packaging\make_zip.ps1`
  then makensis on `packaging\installer.nsi`. `make_zip.ps1` asserts (a) version
  consistency across `CMakeLists.txt` / `installer.nsi` / `app.rc` files and
  (b) that the staged ZIP file set matches `installer.nsi`'s `File` set —
  changing any shipped file requires updating BOTH scripts or the build fails.

## Code conventions

Sources of truth: `docs/dev-contracts.md` (IPC, config schema, contract tests),
`docs/dev-test-flags.md` (engine QA flags), `docs/release-build.md`.

- Language: C++17, MSVC `/W4`, warnings-clean. No raw `new`/`delete` — RAII
  guards for all Win32/mpv handles. Headers use `#pragma once`.
- Namespace `k6wp`. Win32 headers only inside `.cpp` files, never in headers.
- Errors: recoverable = `bool` + out-param; unrecoverable = structured
  exceptions (`ConfigError`, `IpcError`, `LibraryError`). `Decode` never throws
  (returns `false`); `Encode` throws `IpcError` on oversize payload.
- IPC: NDJSON v1 over `\\.\pipe\k6wp-engine` — one JSON object + `\n` per
  message, payload cap 64 KiB, unknown commands get `{"error":…}`.
  Pipe ACL is current-user-only; client discipline is one atomic
  connect+write+ack-read transaction (2 s ack deadline, 1 retry on drop).
- Config: `%LOCALAPPDATA%\K6WP\config.json` is engine playback state only;
  Studio/compressor prefs live in `studio_settings.json`. Both migrate forward
  and back the original bytes up to `<file>.bak` before rewriting.
- Logging: production is file-only (`engine.log`); stdout/stderr mirrors exist
  only under `-DK6WP_VERBOSE=ON`. The `K6WP_VERBOSE` option block in the root
  `CMakeLists.txt` must stay BEFORE `add_subdirectory()` or it silently no-ops
  (marked preserve/NOTE — do not move it).
- Comments marked `preserve`/`NOTE` must be kept.
- Keep diffs minimal and focused; no unrelated reformatting.

## UI language (MED-11 reversed: i18n is back)

- Bahasa UI Studio bisa **Indonesia** (bawaan) atau **English**, dipilih di
  Pengaturan → Bahasa. Pilihan berlaku penuh mulai Studio dijalankan
  berikutnya: `QTranslator` hanya dipasang sekali di `main.cpp`, sebelum
  `QmlShell` memanggil `setSource()`.
- **Indonesia adalah bahasa sumber.** String `tr()` / `qsTr()` di
  `studio/src/*.cpp` dan `studio/qml/Main.qml` berisi teks Indonesia, jadi
  bahasa itu tidak butuh katalog `.qm` sama sekali. Karena itu tidak ada
  `studio_id.ts` — menerjemahkan Indonesia ke Indonesia hanya menambah file
  yang bisa melenceng dari sumbernya.
  Katalog English: `studio/i18n/studio_en.ts` (193 pesan).
- Target CMake: `lupdate` (ekstrak ulang `.ts`), `lrelease` (kompilasi `.qm`),
  `translations` (keduanya). Hanya `lrelease` yang menjadi dependensi target
  `studio`, jadi build biasa mengompilasi katalog tanpa pernah menimpa `.ts`
  yang dikomit. Setelah mengubah string UI, jalankan
  `cmake --build --preset msvc-dev --target translations`.
- Pilihan bahasa disimpan di `studio_ui.ini` (key `ui/language`) di
  `%LOCALAPPDATA%\K6WP`, **bukan** field `StudioSettings`. Alasannya: `main.cpp`
  harus membaca bahasa sebelum bridge/QML ada, dan `StudioSettings` di
  `shared/` dibaca engine (`shared/lockscreen.cpp` untuk `lockscreen_sync`)
  sehingga menambah field di sana berarti migrasi schema bersama.
- Jebakan yang sudah pernah menyesatkan: `tr()` di kelas yang **tidak** punya
  `Q_OBJECT` resolve ke context `QObject`, sedangkan `lupdate` mencatatnya
  di bawah nama kelas - sehingga lookup tidak akan pernah cocok dan string
  tetap tidak diterjemahkan diam-diam. `PreviewWidget` pernah seperti itu;
  `Q_OBJECT`-nya sekarang hanya untuk itu (kelas ini tidak punya signal/slot).


## Running tests

- Unit suites (kept — never delete): `config_test` + `ipc_test`, run via
  `ctest --preset msvc-dev` (or the exes directly in `build/msvc-dev/`).
  Both must exit 0.
- QA helpers: `monitor_dump.exe` (monitor list as JSON), `tools/bench_*.ps1`,
  `tools/verify_wallpaper.ps1`. Bench corpus is generated locally via
  `tools/make_corpus.ps1` and never committed (`tests/corpus/*.mp4` is gitignored).
- Manual checklists: `docs/manual-test.md`, `docs/single-app-test.md`.
- Known limitations worth reading before debugging perf/AV: `docs/known-issues.md`,
  `packaging/known-limitations.md`. `planning.md` / `repomix.md` are local
  scratch (gitignored) — do not rely on them in PRs.

## PR expectations

- One focused change per PR; describe what/why, not just what.
- `/W4`-clean, `ctest --preset msvc-dev` green — state the result in the PR.
- If you touch the ship set (any staged/installed file), update
  `packaging/make_zip.ps1` AND `packaging/installer.nsi` together.
- If you touch IPC or config schema, update `docs/dev-contracts.md` and the
  contract tests in the same PR.
- Do not commit: `build/`, `dist/`, `vendor/bin/`, `tests/corpus/*.mp4`,
  `planning.md`, `repomix.md`, or machine-specific absolute paths.
- License: contributions fall under GPL-2.0-or-later (see `LICENSE`);
  third-party notices live in `LICENSES/`.
