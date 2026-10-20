# Baseline build and test record (Phase 0.2)

Date: 2026-10-20
Revision: `9923cc0` (tag `baseline-pre-refactor`), branch `refactor/rust`
Host: Windows 11 Home 10.0.26200, x64
CPU: Intel Core i7-12650H (6P + 4E, 16 logical)
RAM: 16,457,592 KiB visible (~15.7 GiB)
GPU: Intel UHD Graphics (driver 32.0.101.7085) + NVIDIA RTX 2050 (32.0.16.1664)
Display: 1 active monitor, 1920x1080 @ 144 Hz, 100% scale
Power plan: Balanced
Toolchain: CMake 4.4.3, MSVC via Visual Studio 18 2026 generator, Qt 6.8.3
(`QT_ROOT=C:\Qt\6.8.3\msvc2022_64`), Rust 1.98.0, Node 24.13.1

## Build

| Command | Result |
|---|---|
| `cmake --preset release` | exit 0 |
| `cmake --build --preset release` | exit 0 |
| `cmake --build build/msvc-dev --config Debug --target engine` | exit 0 |
| `ctest --test-dir build/msvc-dev -C Debug --output-on-failure` | 47/47 passed, 66.84 s |

Logs: `build/phase0-release-configure.log`, `build/phase0-release-build.log`,
`build/phase0-ctest.log` (build artifacts are git-ignored).

## First ctest attempt (kept for honesty)

The first run was executed while the user's production `engine.exe`
(`build/release`, PID 18864) was resident. Two tests failed for that reason,
not because of the source tree:

- `display_assignment_test`: "a live engine answers the session pipe; stop it
  (IPC quit) before running display_assignment_test".
- `studio_async_test`: timed out at 1500 s; its "clear-cancel" case assumes no
  engine is listening on the session pipe.

After `build/release/K6WP.exe --stop` (graceful IPC quit, wallpaper restored),
the same tree passed 47/47. Baseline numbers below are therefore taken with
the production engine stopped.

## Known baseline gaps

- One physical monitor only. Multi-monitor, mixed-DPI, AMD/Intel encoders and
  battery/DC behavior remain **UNPROVEN** (no rig / no hardware).
- A Virtual Display Driver package exists locally under
  `shadow-evidence/karantina/` but is not installed as an active monitor. It
  is not used as evidence.
- `Engine:FirstFrame` is emitted on the first message-loop iteration, not on
  the first presented frame (existing TODO 10 debt). Phase 0 records it as the
  legacy startup proxy and adds a visual resume marker instead.
