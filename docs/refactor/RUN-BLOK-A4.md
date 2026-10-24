# RUN-BLOK-A4

## 1. Studio release build
- Initial failure (prior session) was STATUS_STACK_BUFFER_OVERRUN when building the whole workspace in release concurrently.
- `cargo build --release -p k6wp-studio`: SUCCESS in 1m07s. Binary k6wp-studio.exe 8,633,856 bytes.
- Launched: window "K6WP Studio", Responding=True. Screenshot docs/img/studio-release.png.
- Releases: rustc 1.98.0, cargo 1.98.0; free RAM 3.74 GB, free disk D: 58.34 GB.
- No lto/codegen/opt-level change needed.

## 2. Advisory table
| ID | Level | Crate | Path | Used? |
|---|---|---|---|---|
| RUSTSEC-2025-0141 | unmaintained | bincode 2.0.1 | not in Windows target tree | no |
| RUSTSEC-2026-0192 | unmaintained | ttf-parser 0.25.1 | not in Windows target tree | no |
| RUSTSEC-2026-0371 | unmaintained | zbus_names 4.3.5 | not in Windows target tree | no |
| RUSTSEC-2026-0370 | unmaintained | zvariant 5.15.1 | not in Windows target tree | no |
| RUSTSEC-2026-0372 | unmaintained | zvariant_derive | not in Windows target tree | no |
- clipboard-win yanked -> upgraded 5.4.1 -> 5.4.3 (not yanked).
- tokio: NOT in the Windows target graph (`cargo tree -i tokio --target x86_64-pc-windows-msvc` = nothing). It is not a direct dependency; no async runtime in the workspace. Prior advisory was lockfile-only.
- deny.toml ignores the 5 unmaintained with review date 2027-01-01.

## 3. K6WP_FFMPEG
- Changed to debug-only: `#ifdef _DEBUG` in shared/ffmpeg_path.cpp, with `(void)env_name` in the release path.
- Correction: it IS a tool-path redirection, but gated out of release. lint_env_overrides OK. Release scan: 5 binaries, 0 findings (incl. k6wp-studio.exe).

## 4. Golden Studio (A1)
- tests/golden/studio_logic.json marked status=DRAFT (hand-written expected values, not C++-generated).
- C++ writer/checker + Rust replayer NOT implemented. Reason: the pure functions in studio/src are Qt-bound (QCoreApplication::translate in compress_errors.cpp; QObject/QML elsewhere); extracting them is a refactor beyond this block's context budget. 7 modules x 8 cases remain specification only.

## 5. Screenshot
- docs/img/studio-release.png (release build, desktop capture).

## 6. Sync note
- Desktop WIP: 13 untracked + patch backed up; only corpus and vendor/ffmpeg copied. Rust WIP not merged.

## 7. Not finished (technical)
- A1 writer/checker/replayer: Qt-bound pure logic extraction (QCoreApplication::translate, QObject) not completed - context/time limit at Langkah 4.
- A2 6-image DPI matrix: only one desktop screenshot captured; headless Slint render not attempted.
