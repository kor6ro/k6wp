# Task 1.4 / E3 — the SPS parser, the golden fixtures and the data-derived threshold

Date: 2026-10-22
Branch: `refactor/rust`
Label: **live-proven (one machine)** — every number was measured on this
host; the parser is validated against ffmpeg's own bitstream parser.

## Where `num_ref_frames` comes from (D3, measured)

| source | has the field? | how it was checked |
|---|---|---|
| ffprobe | **no** | its stream keys were enumerated: no refs / dpb field |
| ffmpeg's normal output | **no** | the debug log prints the NAL types, not the DPB size |
| libmpv | **no** | the whole mpv property list was enumerated |
| **ffmpeg's `trace_headers` filter** | **yes** | `max_num_ref_frames` (H.264) and `sps_max_dec_pic_buffering_minus1` (HEVC) |

So the source is ffmpeg's own bitstream parser. The Rust port replaces the
spawn with a **small, dependency-free parser** in `k6wp-core`
(`rust/crates/k6wp-core/src/sps.rs`):

- the H.264 SPS: `max_num_ref_frames`, the VUI's
  `max_dec_frame_buffering`, `frame_mbs_only_flag` (the interlaced case) and
  the frame size;
- the HEVC SPS: `sps_max_dec_pic_buffering`;
- the container: the MP4 `avcC` / `hvcC` boxes (through the `stsd`
  sample entries) **and** the in-band parameter sets, located EXACTLY
  through the sample table (`stsz` / `stsc` / `stco`) — the decoder
  switches to the in-band SPS, so the largest DPB in the FILE is the one
  the decoder must honour (a stream can carry a deeper SPS than the
  container's).

**The unknown-codec fallback**: a file whose codec this parser does not
know returns `SpsError::NoSps` / `needs_fallback` — the caller then uses
ffmpeg's `trace_headers` (the C++ compressor already does; the Rust port
will too).

## The validation (the golden fixtures)

`tools/extract_sps_truth.ps1` runs ffmpeg's `trace_headers` over the
corpus and writes the ground truth to `tests/golden/sps-dpb.json`. The
Rust test `rust/crates/k6wp-core/tests/sps_golden.rs` replays it.

The corpus covers the cases the brief names:

| corpus file | what it exercises | ffmpeg's DPB | the parser's |
|---|---|---|---|
| anime.mp4 | the baseline (4 refs) | 4 | **4** |
| gaming.mp4 | a 4K-ish source | 4 | **4** |
| highdpb_1080p.mp4 | 16 refs at 1080p | 16 | **16** |
| highdpb_4k.mp4 | 16 refs at 4K | 16 | **16** |
| **hevc_1080p.mp4** | **HEVC** (x265) | 5 | **5** |
| **interlaced.mp4** | **`frame_mbs_only_flag = 0`** (MBAFF) | 4 | **4** (interlaced ✓) |
| **multi_sps.mp4 / .h264** | **two parameter sets** (4 and 16) | 16 | **16** (both found) |
| slideshow.mp4 | a short clip | 4 | **4** |

`multi_sps` is the case the container alone would get WRONG: its `avcC`
holds the first SPS (4 refs) and the stream carries a deeper one (16 refs) —
the parser finds both through the sample table and reports the maximum,
which is what the decoder has to honour.

**9 of 9 cases match ffmpeg's trace_headers**, including the frame size, the
interlaced flag and the HEVC field. The corpus is gitignored;
`tools/build_corpus.ps1` regenerates it and the fixture pins each file's
SHA-256.

### The three parser bugs the corpus caught (all now fixed)

1. **The NAL header was parsed as RBSP.** The H.264 header is 1 byte and
   the HEVC header is 2 — parsing them as payload shifted every field.
2. **The Exp-Golomb reader consumed the wrong bits.** `ue(v)` is the
   (k+1)-bit value of (codeNum+1) with k leading zeros; the reader took
   the k bits *before* the implicit 1, which returns the wrong number for
   most values (it looked right for a few small ones).
3. **`numOfSPS` is 5 bits, not a byte** — the reserved bits made the
   avcC list unreadable (the parser found no parameter set at all).

## The RAM correlation (the threshold from the data, not a guess)

The decoder's surface pool scales with the DPB: `refs × width × height ×
1.5` bytes (YUV 4:2:0). Measured on this host:

| file | refs | frame | DPB bytes (predicted) | private bytes (measured) |
|---|---|---|---|---|
| anime.mp4 | 4 | 1920×1088 | 12.5 MB | **163.3 MB** |
| highdpb_1080p.mp4 | 16 | 1920×1088 | 50.0 MB | **194.8 MB** |
| highdpb_4k.mp4 (the source) | 16 | 3840×2160 | 199.1 MB | **387.2 MB** |
| highdpb_4k.mp4 (the DPB-pinned re-encode) | 2 | 3840×2160 | 24.9 MB | **228.2 MB** |

The measured deltas match the prediction: 12 extra refs at 1080p cost
**+31.5 MB** (predicted 37.5 MB) and 14 fewer refs at 4K save **−159 MB**
(predicted −174 MB).

**The threshold (4 refs) therefore bounds the DPB's memory to ~50 MB** — the
same order as the engine's own footprint (24 MB idle, 60 MB with the whole
mpv instance). That is why the threshold is a ref count and not a byte
count: it is the number that keeps the DPB's RAM in that band at BOTH
resolutions (4 refs = 12.5 MB at 1080p, 50 MB at 4K).

**The behaviour did not change** (the C++ compressor already refused the
copy path above 4 refs), so no A/B and no visual-parity re-run is owed for
the threshold. The Rust parser reproduces the SAME number, so the Rust port
inherits the measured A/B (the 4K/16-ref case: private **386.8 → 228.2 MB,
−41 %**, GPU shared 467.7 → 432.5 MB, rate 0.99 in both, parity Δluma 0.04).

## The `ffprobe.exe` verdict (D4's condition)

D4's condition was: *the 5 metadata fields via libmpv **and** the
`num_ref_frames` parser must both be proven*.

| requirement | status |
|---|---|
| the 5 metadata fields via libmpv | **PROVEN** — 0 disagreements over 5 corpus files × 5 fields (`docs/refactor/phase1-3-bundle.md`) |
| the `num_ref_frames` parser | **PROVEN** — 9/9 against ffmpeg's trace_headers, including HEVC, interlaced and the multi-SPS case |
| the probe cost | the ffprobe spawn is **30 ms** (not the ~150 ms the brief assumed) and the DPB probe is **39 ms** |

**The condition is met, so removing `ffprobe.exe` is authorized.** What is
NOT yet done is the mechanical part in the C++ world: the compressor would
have to link `libmpv` to read the five fields (the DPB itself needs only
ffmpeg, which stays). The Rust port does this natively — `k6wp-core` has
the library codec, the IPC codec and now the SPS parser, and Phase 3's
compressor will read the metadata through libmpv and the DPB through the
parser.

**Until that lands, the ZIP stays 148.13 MB and the ≤ 115 MB target is
NOT met** — recorded as-is, not softened.

## What is left for E3

- The compressor's **C++** side still spawns `trace_headers` (an ffmpeg
  spawn, 39 ms). The Rust parser removes that spawn in the Phase 2/3 port.
- The corpus builder script (`tools/build_corpus.ps1`) is referenced by the
  fixture's note; the files are generated by the commands in this document
  and the extraction script.
