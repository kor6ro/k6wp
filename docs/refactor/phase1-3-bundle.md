# Phase 1.3 — the bundle: what is actually worth cutting

Date: 2026-10-21
Branch: `refactor/rust`
Label: **live-proven (one machine)** — every number below is measured from
the shipped ZIP and the shipped binaries on this host.
Evidence: `docs/bench/bundle-zip.json`, `docs/bench/probe-parity.json`.

## The measured facts first

The install is 389.14 MB; the shipped **ZIP is 148.13 MB** (38.1 % ratio,
1295 entries). The two are different levers, and the group sizes are
measured **per file** from the actual ZIP
(`tools/bench_bundle_zip.ps1`):

| group | files | install MB | **ZIP MB** | ZIP % |
|---|---|---|---|---|
| **ffmpeg + ffprobe** | ffmpeg.exe 97.18 + ffprobe.exe 96.98 | 194.16 | **70.73** | **47.8 %** |
| **mpv** | libmpv-2.dll | 114.77 | **44.64** | **30.1 %** |
| **Qt DLLs** | Qt6*.dll | 48.00 | **19.78** | **13.4 %** |
| shader cache | dxcompiler.dll 13.65 + dxil.dll 1.44 | 15.09 | 6.88 | 4.6 % |
| QML payload | qml/ | 8.14 | ~2.6 | ~1.8 % |
| K6WP exes | engine + studio + K6WP + compressor | 5.44 | 2.08 | 1.4 % |
| MSVC runtime | vcruntime/msvcp | 0.83 | 0.30 | 0.2 % |

**The C3 premise is confirmed by measurement:** dropping Qt entirely saves at
most 19.78 + 2.6 ≈ **22 MB of a 148 MB ZIP (15 %)**. A "≥ 30 % ZIP reduction
by removing Qt" target is **unreachable**. The Qt removal is still the right
change — for **Studio RAM (337 MB → ≤ 150 MB), Studio startup (1290 ms →
≤ 700 ms) and code tidiness** — but it is NOT a bundle-size play, and the
budget has been revised accordingly (`docs/refactor/budgets.md`).

## Lever 1 — ffprobe → libmpv: **SAFE, PROVEN, the biggest win**

`ffprobe.exe` is **96.98 MB of the install (~35 MB of the ZIP, 24 % of the
ZIP on its own)** — a second copy of the ffmpeg build. The product asks it
for exactly five fields:

- `format.duration`
- the first video stream's `codec_name`, `width`, `height`, `r_frame_rate`
- whether an audio stream exists

`libmpv-2.dll` is **already shipped** (the engine and the Studio use it), so
probing through it costs **zero new shipping bytes**.

### The parity experiment (the evidence)

`tools/bench_probe_parity.ps1` asks the **live mpv instance** for the same
five fields and compares them with ffprobe's, field by field, over the
corpus. The engine's hidden `--dump-media-props <path>` QA flag is the
probe's read side.

| file | duration | codec | size | fps | audio | video |
|---|---|---|---|---|---|---|
| anime.mp4 | ✅ 30/30 | ✅ | ✅ | ✅ 30/30 | ✅ | ✅ |
| gaming.mp4 | ✅ 30/30 | ✅ | ✅ | ✅ 30/30 | ✅ | ✅ |
| highdpb_1080p.mp4 | ✅ 20/20 | ✅ | ✅ | ✅ 30/30 | ✅ | ✅ |
| highdpb_4k.mp4 | ✅ 12/12 | ✅ | ✅ | ✅ 30/30 | ✅ | ✅ |
| slideshow.mp4 | ✅ 30/30 | ✅ | ✅ | ✅ 5/5 | ✅ | ✅ |

**0 fields disagree across 5 files** (`docs/bench/probe-parity.json`).

### The three traps the experiment had to solve

1. **mpv's `video-codec` is the human-readable string**
   (`"H.264 / AVC / MPEG-4 AVC / MPEG-4 part 10"`), not ffprobe's short
   name (`"h264"`). The short name is the **`codec` field of the video track
   in `track-list`** — that is the field to use.
2. **`duration` is a double**, so an integer property read reports it as
   unavailable. It must be read as text.
3. **The properties are not there until the demuxer is done.** A dump at
   start-up reads zeros; the dump must wait for `duration` to appear (with
   a cap).
4. **The properties must be read from the renderer that HAS the file** —
   with a live wallpaper surface that is the **slot's** renderer, not the
   headless one (which stays empty in that case).

### The implementation (the next task, spec below)

- New shared module `shared/mpv_probe.{hpp,cpp}`:
  `bool ProbeVideo(const std::filesystem::path&, VideoProps&)` where
  `VideoProps` carries `duration / codec / width / height / fps / has_audio /
  found_video` — the union of the compressor's `VideoProps` and the Studio's
  `VideoMetadata`.
- It creates a **headless** mpv instance (`vo=null`, `audio=no`,
  `idle=yes`), loads the file, waits for `duration`, reads the fields, and
  destroys the instance.
- The **compressor** (`compressor/src/video_probe.cpp`) and the **Studio**
  (`studio/src/ffprobe_helper.cpp`) both switch to it. The compressor must
  link `libmpv` (a build change; the DLL is already shipped).
- `tools/bench_probe_parity.ps1` becomes the **gate**: it must report 0
  disagreements over the corpus, and it must be extended to the user's real
  library before the swap is accepted.
- The installer then drops `ffprobe.exe` (the ZIP's biggest single file
  after libmpv).
- **One open risk to measure:** the probe cost. ffprobe is a short-lived
  spawn (~150 ms); an mpv instance init is heavier. The compressor probes
  once per invocation, so the delta must be measured and must not regress
  the compress round-trip time.

**Verdict: APPROVED to implement — the parity is proven; the implementation
is the next task.** Not shipped in this commit: the swap touches the
compressor's and the Studio's data paths, and the brief demands a
full playback/compress parity pass before a media-path change lands.

## Lever 2 — a lean libmpv build: **DEFER**

`libmpv-2.dll` is 114.77 MB of the install / 44.64 MB of the ZIP (30 %).

**DEFER, with the reason:** this machine has **no meson, no ninja, no
gcc/clang/make** (verified) and the vendor tree carries **prebuilt binaries
only** — there is no source to build from. A minimal mpv build cannot be
produced *or verified* here, and shipping an unverified media build is
exactly the risk the brief forbids. It stays as upside, not as a gate.

When it is done, it needs: a new SHA-256 in `vendor/VERSIONS.md`, an
**identical playback** proof (the rendered-frame gate), and **no worse
RAM** (the private bytes + GPU shared metrics).

## Lever 3 — a lean ffmpeg build: **DEFER**

`ffmpeg.exe` is 97.18 MB of the install / ~35 MB of the ZIP.

**DEFER, for the same reason** (no toolchain, no source) and one more: it
changes the **encoder set** — every codec the product can emit — so it needs
a full compress/playback parity pass, not just a size check. A build limited
to `libx264 + h264 demux/mux` is the right target *if* the product's codec
matrix allows it (the current UI offers more than H.264).

## Revised bundle targets

| metric | baseline | **new target** | reachable by |
|---|---|---|---|
| install, total | 389.14 MB | **≤ 292 MB (−25 %)** | the ffprobe→libmpv swap |
| **ZIP** | 148.13 MB | **≤ 115 MB (−22 %)** | the ffprobe→libmpv swap |
| install, ffprobe | 96.98 MB | **0 MB** | the swap |
| install, mpv group | 114.77 MB | ≤ 114.77 MB (parity) | DEFER (lean build) |
| install, ffmpeg group | 194.16 MB | ≤ 194.16 MB (parity) | DEFER (lean build) |

The deferred levers are **upside, not part of the gate**.

## The D4 verdict (added after the Phase 1.2 brief)

The swap was **not applied**, and the reason is measured, not assumed:

1. **The field task 1.4 needs (`num_ref_frames`) is NOT in libmpv.** The
   whole mpv property list was enumerated: there is no DPB / refs property.
   The same is true of ffprobe (its stream keys carry no refs/dpb field) and
   of ffmpeg's normal output. The value comes from ffmpeg's own
   `trace_headers` bitstream filter instead — so the swap cannot serve task
   1.4, and the brief's fallback applies: **ffprobe stays**, the DPB comes
   from the SPS route.
2. **The cost does not argue for it either.** The ffprobe spawn is **30 ms**
   (the brief assumed ~150 ms — it is 5× that) and the new DPB probe is
   **39 ms**. Both are negligible against an encode, and an mpv-instance
   probe would be heavier than a 30 ms spawn, not lighter.

**Consequence, recorded as-is:** the **ZIP target ≤ 115 MB is NOT met** by
this lever — the ZIP stays **148.13 MB**. The 97 MB ffprobe opportunity
stays documented here (with its proven parity evidence) and can be
re-decided if the bundle win is wanted at the cost of an mpv-based probe.

## What Phase 2 (Slint, no Qt) is actually for

Not the ZIP: it is **Studio RAM (337.53 MB → ≤ 150 MB), Studio startup
(1290 ms → ≤ 700 ms), and code tidiness** (one UI framework instead of
two). The Qt group is 13.4 % of the ZIP — the reason to do it is the
process, not the download.
