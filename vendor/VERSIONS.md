# Vendor Versions — k6wp-v1

Pinned third-party binaries and headers for the K6WP build. All hashes are SHA256.
Download date: **2026-09-16**.

## Binaries

| Artifact | Version | Source URL | SHA256 |
|----------|---------|------------|--------|
| `ffmpeg/ffmpeg.exe` | 8.1.2-essentials_build (gyan.dev, gcc 16.1.0) | https://github.com/GyanD/codexffmpeg/releases/download/8.1.2/ffmpeg-8.1.2-essentials_build.zip | `1326DDE4C84FF1F96FE6B8916C5BED29E163E9B5DCCF995F6F3DB069D143EC5E` |
| `ffmpeg/ffprobe.exe` | 8.1.2-essentials_build (gyan.dev, gcc 16.1.0) | https://github.com/GyanD/codexffmpeg/releases/download/8.1.2/ffmpeg-8.1.2-essentials_build.zip | `B49CCC7C6547B141AD5A2F6EC69CC04323D7133D7704D70B331B904C63EECB07` |
| `libmpv/bin/libmpv-2.dll` | 0.41.0-dev (git `69e63f425a`, shinchiro build 20260903) | https://github.com/shinchiro/mpv-winbuild-cmake/releases/download/20260903/mpv-dev-x86_64-20260903-git-69e63f425a.7z | `673E6397920AB64A9C5B3A618F7F16D38854EFE72B58665F1F84E4E873B763A4` |
| `libmpv/lib/mpv.lib` | MSVC x64 import lib, generated from libmpv-2.dll exports (54 `mpv_*` symbols) | generated via `dumpbin /exports` + `lib /def /machine:x64` (VS 18 BuildTools 14.51.36231) | `9D4734F0655C62B1C5AB7DD00970F9941D74DCBCF1A91BC149D42EA3D0FAE8CA` |
| `shared/thirdparty/json.hpp` | nlohmann/json 3.11.3 | https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp | `9BEA4C8066EF4A1C206B2BE5A36302F8926F7FDC6087AF5D20B417D0CF103EA6` |

## libmpv headers (part of the shinchiro dev package above)

| Header | SHA256 |
|--------|--------|
| `libmpv/include/mpv/client.h` | `1ACF99EE77C8C2A6F1D1993BD81BBC8A91D27FB5924E80171670E6139A4BD353` |
| `libmpv/include/mpv/render.h` | `192691941602052F00DF0587F126246C48785A1CF21DE68D22A92EA1908D1C55` |
| `libmpv/include/mpv/render_gl.h` | `48662C0ED9872A14DD9E1684105C97F69F94A0414709C254C5D372ADC41D2E69` |
| `libmpv/include/mpv/stream_cb.h` | `188E58B6D14383E15A5DFFDC4ECEBBDFBF2E412B9C099FF21B571F747A8CE32D` |

## Notes

- **ffmpeg (P4.4, 2026-09-22)**: re-vendored from `full_build` to gyan
  `essentials_build` 8.1.2 (same upstream 8.1.2, gcc 16.1.0). Verified
  before swap: `-encoders` lists `h264_nvenc`, `h264_qsv`, `h264_amf`,
  `libx264`; `-filters` lists `scale` + `fps`; `mp4` muxer supports
  `faststart`. Post-swap: `compressor --probe-encoders` OK (nvenc chosen),
  corpus bench (attic `bench_compress.ps1` logic, fresh cache) PASS
  4.04/7.06/2.04 s nvenc, and a 1080p→720p24 CLI encode verified
  moov-before-mdat. Sizes: ffmpeg.exe 101.90 MB (was 242.50 MB), ffprobe.exe
  101.69 MB (was 242.29 MB) — ~281 MB saved in repo + deploy. `ffplay.exe`
  in the essentials zip is NOT vendored (never referenced).
- **ffmpeg (pre-P4.4)**: copied from winget install `Gyan.FFmpeg (package dir `ffmpeg-8.1.2-full_build`). Config includes `--enable-nvenc --enable-libvpl --enable-amf --enable-libx264` (verified: `h264_nvenc`, `h264_qsv`, `h264_amf`, `libx264` all listed by `-encoders`).
- **ffprobe**: same winget package as ffmpeg.exe; copied alongside it (Todo 14 — the compressor CLI needs it for the >10-min duration check).
- **libmpv**: official mpv release zips (v0.41.0 and git-release tags) are **player-only** (mpv.exe, no `include/`/`lib/`/`bin/`). Used shinchiro's `mpv-dev-x86_64` package instead — it ships the 4 client headers + `libmpv-2.dll` (MSVC-consumable: depends only on system DLLs + UCRT, no MinGW runtime). Version is 0.41.0-dev (commit `69e63f425a` is after tag v0.41.0, before any 0.42.0 tag).
- **mpv.lib**: shinchiro ships only a MinGW `libmpv.dll.a`. The MSVC import library was generated from the DLL's exports (54 `mpv_*` symbols; bundled-JRE `Java_*` exports excluded) via `dumpbin /exports` → `.def` → `lib /def /machine:x64`. Verified x64 (`8664 machine (x64)`).
- **json.hpp**: single-header nlohmann/json 3.11.3 (version macros verified in-file).
- **libmpv dev-channel policy (LOW-20)**: the pin above is intentionally a
  dev-channel build (`0.41.0-dev`, shinchiro `20260903`, git `69e63f425a`),
  not a stable tag. Reason: official mpv release zips are player-only
  (`mpv.exe`, no `include/`/`lib/`/`bin/`), so no stable-tagged dev package
  exists to pin; shinchiro's `mpv-dev-x86_64` is the only MSVC-consumable
  source (system DLLs + UCRT only, no MinGW runtime). Tradeoff accepted:
  a dev snapshot can carry pre-release ABI churn, contained here by pinning
  the exact build + SHA256 and generating `mpv.lib` from that DLL's own
  exports. Advisory review schedule: re-check the shinchiro release feed
  and upstream mpv security advisories on the first week of each month and
  before every K6WP release; re-pin + re-hash + regenerate `mpv.lib` when
  moving, and record the new build here.

## Distribution (task 26 — what ships in the release bundle)

- **ZIP (`packaging/make_zip.ps1`) and installer (`packaging/installer.nsi`)
  stage the same file set from `build/release/`**: `K6WP.exe`,
  `engine.exe`, `studio.exe`, `compressor.exe` (+ `monitor_dump.exe` only
  when the build produced it — absent under `BUILD_TESTING=OFF`),
  `vendor/ffmpeg/ffmpeg.exe`, `vendor/ffmpeg/ffprobe.exe`,
  `vendor/libmpv/bin/libmpv-2.dll`, the windeployqt output next to
  `studio.exe` (root `*.dll` + every plugin subdir containing `*.dll`),
  `packaging/config.json.example`, `packaging/uninstall.bat`.
- **Why essentials (P4.4, supersedes the full_build note below)**: the
  compressor's encoder auto-detect (`NVENC → QSV → AMF → x264`) needs all
  four encoders present in one binary — the essentials build above is
  verified to list `h264_nvenc`, `h264_qsv`, `h264_amf`, `libx264`, plus the
  only filters/muxer flags the pipeline uses (`scale`, `fps`, `faststart`).
  Full-build-only codecs/filters were never referenced by any call site.
- **Why full_build (pre-P4.4, historical)**: the compressor's encoder
  auto-detect (`NVENC → QSV → AMF → x264`) needs all four encoders present
  in one binary. The gyan `full_build` was verified to list `h264_nvenc`,
  `h264_qsv`, `h264_amf`, `libx264`; at the time a smaller
  (essentials-style) ffmpeg was *assumed* to drop the QSV/AMF legs. P4.4
  disproved that for 8.1.2 (all four present, verified above), so the size
  cost was retired — see the essentials note.
- **Measured sizes (2026-09-18, release staging)**: ffmpeg.exe 231.26 MB,
  ffprobe.exe 231.07 MB, libmpv-2.dll 114.77 MB, Qt runtime + plugins
  ~32 MB, our five exes ~1.2 MB. Staged total 618.82 MB / 28 files;
  Deflate ZIP 238.44 MB — under the 250 MB `make_zip.ps1` assert, but
  with only ~12 MB of headroom. If the assert ever trips, the fix is the
  vendor choice (smaller ffmpeg), never silently raising the limit.
- **Licensing note (unreviewed by counsel)**: gyan full_build ffmpeg and
  shinchiro libmpv are GPL-licensed binaries. Shipping them in
  `dist/K6WP-portable-*.zip` / `dist/K6WP-Setup.exe` distributes GPL
  software alongside our exes — keep this section's source URLs + hashes
  intact so recipients can fetch the corresponding sources, and get a
  proper license review before any public release.
- **nlohmann/json.hpp and mpv.lib are build-time only** (header / import
  lib) and never ship in the bundle.