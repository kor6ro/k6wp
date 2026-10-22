# The SPS parser - the read strategy, the container policy and the fuzzing

Task I2. This file describes **how** the Rust SPS parser reads a file, what
it does with a container it does not know, and how the fuzzing is arranged.

## 1. The read strategy: SEEK, do not read the prefix

The parser is asked one question: *how big is the decoder's DPB for this
file?* The answer lives in the parameter sets, and the parameter sets live
in the file's **header**. So the parser must not read the body.

The container walk therefore **seeks**:

| step | what is read | what is skipped |
|---|---|---|
| the head | 193 bytes (the container magic + the `ftyp` brand) | - |
| the top-level boxes | 8 bytes per box header | every box the parser does not need is **seeked over** - not read |
| the `ftyp` box | its payload (a few dozen bytes) | - |
| the `moov` box | its payload (the parameter sets) | - |
| everything else | **nothing** | the `mdat` (the media data - the big part) |

**The measured cost** (this host, the corpus + a synthetic fixture):

| file | the size | the bytes read | what was read |
|---|---|---|---|
| slideshow.mp4 | 10 KB | 14 628 | the `moov` + its samples |
| hevc_1080p.mp4 | 3.45 MB | 2 601 658 | the `moov` + its samples |
| anime.mp4 | 32.4 MB | 32 655 074 | the `moov` + its samples |
| highdpb_4k.mp4 | 38.4 MB | 37 597 559 | the `moov` + its samples |
| multi_sps.mp4 | 53.6 MB | 54 812 871 | the `moov` + its samples |
| **gaming.mp4** | **423 MB** | **32 984 910 (7.8 %)** | the `moov` + the samples inside the window |
| **a synthetic 512 MB MP4 with the `moov` at the end** | **512 MB** | **14 628** | the `moov` only (it has no sample table) |
| matroska.mkv (not parsed) | 10 KB | 193 | the head only |
| multi_sps.h264 (a raw stream) | 53.5 MB | 56 137 498 | the bounded prefix scan |

**Two reads, two rules:**

1. **The container walk SEEKS** - 8 bytes per box header, and every box
   the parser does not need is skipped **without being read**. Only the
   `ftyp` (a few dozen bytes) and the `moov` (the parameter sets and the
   sample table) are loaded.
2. **The in-band scan reads the SAMPLES, not the media data.** The samples
   are located EXACTLY through the sample table (`stsz` / `stsc` / `stco`),
   so the parser reads the samples it needs. The window is the first
   `MAX_SCAN_BYTES` (64 MB) of the file - **the same window as before the
   refactor**, so the behaviour is unchanged (the golden fixture still
   matches ffmpeg 9/9).

**The property:** a file bigger than the window is **never read whole** -
the biggest file in the corpus (423 MB) costs 33 MB of reads, and the
synthetic 512 MB fixture costs 14 KB.

The report carries `bytes_read`, so the cost is **observable**, not a
promise: `DpbReport::bytes_read`.

**The non-faststart case** is the normal case for a file that was never
optimized (`+faststart`): the `moov` is at the END, after the `mdat`. The
parser finds it because it walks the boxes by seeking - the fixture in
`tests/sps_read_strategy.rs` builds exactly that shape (a 512 MB sparse
`mdat` + the `moov` at the end) and asserts that the parameter sets are
found and that the file costs less than 1 MB of reads.

**The raw Annex-B streams** have no boxes: the parameter sets are in band,
so the parser scans a **bounded prefix** (64 MB, `MAX_SCAN_BYTES`). A raw
stream bigger than that is truncated (and the caller's fallback covers the
rest). The one raw file in the corpus (`multi_sps.h264`, 53.5 MB) is
therefore read up to its second parameter set - that is the nature of an
in-band scan, and it is bounded.

## 2. The container policy (explicit)

| container | the magic | what happens |
|---|---|---|
| the MP4 family (isom, mp41, mp42, dash, avc1, ...) | `ftyp` at the offset 4 | **parsed** (the `moov` box) |
| a raw H.264 / HEVC Annex-B stream | the NAL start code `00 00 01` | **parsed** (the in-band scan) |
| Matroska / WebM | the EBML magic `1A 45 DF A3` | **fallback**: ffmpeg's `trace_headers` |
| AVI | `RIFF....AVI ` | **fallback**: ffmpeg's `trace_headers` |
| MPEG-TS | the sync byte `0x47` at 0 and at 188 | **fallback**: ffmpeg's `trace_headers` |
| anything else | - | **fallback**: ffmpeg's `trace_headers` |

The fallback is a **decision, not an accident**: `DpbReport::needs_fallback`
is true and the report carries no parameter set. The parser **never
guesses** and never rewrites the file.

**The measured fallback rate over the corpus** (11 files, this host):
**1 of 11 = 9.1 %** - and the one is the Matroska sample. Every MP4 and the
raw H.264 stream are parsed by the module itself. The sweep is a test
(`the_corpus_sweep_reports_the_fallback_percentage`), so the rate is
re-measured with every run.

## 3. `ffmpeg.exe` is still needed - only `ffprobe.exe` goes

This is worth stating plainly, because the two are easy to confuse:

| binary | the status | why |
|---|---|---|
| `ffprobe.exe` | **REMOVED** (task 1.3 / D4) | the 5 metadata fields are read through libmpv (0 disagreements over the corpus) and the DPB comes from the Rust SPS parser (9/9 against ffmpeg's `trace_headers`) |
| `ffmpeg.exe` | **STAYS** | (a) the **fallback**: a Matroska / an AVI / an MPEG-TS file is probed with `ffmpeg -i ... -c copy -bsf:v trace_headers`; (b) the **compressor**: the re-encode itself is an ffmpeg run |

So the size win is the ffprobe binary (and its process spawn), **not** the
removal of ffmpeg. The ZIP stays at 148.13 MB (the target is deferred to
Phase 3, see `docs/refactor/budgets.md`).

## 4. The fuzzing

### 4.1 What runs where

| layer | what | when | the toolchain |
|---|---|---|---|
| the property fuzzing | proptest over arbitrary bytes + a deterministic mutation pass over the real corpus + the fixed regression vectors | every `cargo test` (13 s) | **stable** - it runs everywhere |
| the sanitizer fuzzing | cargo-fuzz over 4 targets (the SPS parser, the box parser, the library codec, the IPC codec) | the scheduled CI job (`.github/workflows/fuzz.yml`, every night) | **nightly** (cargo-fuzz needs `-Zsanitizer=address`) |

### 4.2 The targets

| target | what it drives | why it matters |
|---|---|---|
| `sps_parser` | `probe_dpb_bytes` | the whole probe: the container detection, the box walk, the parameter-set parsers |
| `box_parser` | `container_parameter_sets` | the box walk alone: it reads the most attacker-controlled numbers (the box sizes, the table counts) |
| `library_codec` | `LibraryIndex::parse` + the round trip | the Studio reads a JSON file the user can edit |
| `ipc_codec` | `ipc::decode` + the round trip | a frame arrives from another process on a named pipe |

The engine is built with `panic = "abort"`, so a panic in a parser is a
crash of the whole engine: the fuzzing is a safety requirement, not a
nicety.

### 4.3 The seed corpus

`tools/seed_fuzz_corpus.ps1` writes **synthetic** seeds (a small MP4-shaped
byte string with an `avcC`, plus the hostile shapes: a truncated box, a huge
box size, an all-ones buffer, a broken JSON, a garbage IPC frame). They are
committed and contain **no user data** - the real corpus holds the user's
videos and is not in git, so the CI cannot use it. With `-WithRealCorpus`
the local run also adds the headers of the real corpus (a broader local
run than the CI run).

### 4.4 The failing samples

A failing sample lands in `rust/fuzz/artifacts/<target>/` and is uploaded
as a CI artifact. It must then be **moved** into the regression fixtures
(`tests/golden/sps-regressions/` for the SPS parser) and committed - the CI
cannot do that automatically, because a sample is a decision.

### 4.5 The local blocker (honest)

**cargo-fuzz cannot run on this host**: the ASAN runtime DLL
(`clang_rt.asan_dynamic-x86_64.dll`) ships with LLVM, and neither LLVM nor
the ASAN runtime is installed here. The nightly toolchain and cargo-fuzz
0.13.2 are installed, and the four targets **compile** (the build only
fails at the link step, on the missing ASAN runtime). The CI job installs
LLVM when the runner image does not provide it.

The local evidence is therefore the **stable** property fuzzing (the
proptest + the mutation pass + the 4 regression vectors) - which is the
layer that actually guards the release build.
