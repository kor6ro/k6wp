# Release build

Verified 2026-09-17: configure exit 0, build exit 0, all three exes
Windows-GUI subsystem, `K6WP.exe --help` prints usage and exits.

Prerequisites: Visual Studio 18 2026 BuildTools (x64), Qt 6.8.3
`msvc2022_64`, CMake at `C:\Program Files\CMake\bin\cmake.exe`
(not on PATH). Set `QT_ROOT` to the Qt install dir, e.g.
`C:\Qt\6.8.3\msvc2022_64` — the preset reads `$env{QT_ROOT}`.

Copy-paste (run from the repo root):

```powershell
& 'C:\Program Files\CMake\bin\cmake.exe' --preset release
& 'C:\Program Files\CMake\bin\cmake.exe' --build --preset release
```

Notes:

- Preset `release` (CMakePresets.json): generator Visual Studio 18 2026,
  x64, `CMAKE_BUILD_TYPE=Release`, `BUILD_TESTING=OFF`, binary dir
  `build/release`. Test targets (`config_test`, `ipc_test`) are not
  created in this configuration by design.
- A configure-time `translations: 'lupdate' ... failed to run` notice is
  expected on machines whose Qt install lacks the Linguist runtime (see
  docs/known-issues.md). It is harmless: the committed English baseline
  is used and the build still exits 0.
- Outputs land directly in `build/release/`: `engine.exe` (wallpaper
  engine), `studio.exe` (settings UI), `K6WP.exe` (launcher/dispatcher),
  plus `compressor.exe` and `monitor_dump.exe`.

Confirm the GUI subsystem without a VS shell (PE byte-check, expects 2
= Windows GUI for each exe):

```powershell
foreach ($n in 'engine.exe','studio.exe','K6WP.exe') {
  $b = [IO.File]::ReadAllBytes("build/release/$n")
  [BitConverter]::ToUInt16($b, [BitConverter]::ToInt32($b, 0x3C) + 92)
}
```

Observed on the verification run: engine.exe 325632 B, studio.exe
515072 B, K6WP.exe 64512 B, subsystem 2 for all three,
FileDescription K6WP Engine / K6WP Studio / K6WP Launcher.
Sizes will drift with code changes; subsystem 2 must not.
