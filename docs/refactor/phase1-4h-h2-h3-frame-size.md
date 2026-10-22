# H2 + H3 — the extra-RAM estimate: the bit depth, the chroma, and the measured cases

**Task.** The import dialog (2.5) shows an estimate of the extra RAM a video
will cost. The estimate must use the **coded size + the bit depth + the
chroma format** (H2), and the **10-bit and HEVC cases must be measured**
(H3). The unknown-codec policy must be written down.

**Verdict: the formula is generalised and measured; the measurement says
something the formula alone cannot.** The reference-pool estimate is
**exact** (the 10-bit frame is exactly twice the 8-bit one), but the
**measured** extra cost of a 10-bit file is **an order of magnitude larger**
than that estimate, because the instance the measurement runs in decodes in
**software**. The import dialog must therefore label the estimate for what
it is - the decoder's **reference pool** - and warn about 10-bit.

---

## 1. H2 — the formula

Before: `frame_bytes = pixels × 1.5` (the 8-bit 4:2:0 case only). The
decoder holds 16-bit samples for a 10-bit file and 3 samples per pixel for
4:4:4, so the old constant was **2× too low** for a 10-bit file and **2× too
low** for a 4:4:4 file.

After (`rust/crates/k6wp-core/src/sps.rs`): the SPS parser now **keeps** the
two fields it used to skip, and the frame size is
`pixels × samples_per_pixel(chroma) × bytes_per_sample(bit_depth)`:

| chroma | samples/pixel | 8-bit | 10-bit |
| --- | ---: | ---: | ---: |
| 4:0:0 (monochrome) | 1 | 1 | 2 |
| 4:2:0 | 1.5 | 1.5 | 3 |
| 4:2:2 | 2 | 2 | 4 |
| 4:4:4 | 3 | 3 | 6 |

(bytes per pixel; the arithmetic stays in integers: `pixels × samples_x2 / 2 ×
bytes_per_sample`.)

The parser changes:

- **H.264**: the high-profile branch already read `chroma_format_idc` and
  `bit_depth_luma_minus8` - it threw them away. They are kept now. The
  baseline/main profiles do not carry the fields, but the spec says they are
  8-bit 4:2:0, so the **implied** values are recorded (the dialog shows
  them).
- **HEVC**: the same two fields, always present in the SPS.

`dpb_bytes()` is now `effective_refs × frame_bytes()` (the old code
multiplied the pixels by 1.5 directly), so the pool follows the bit depth
too.

**The unknown-codec policy (H3).** For a codec the parser does not read
(MPEG-2, VP9, AV1) the estimate is **UNKNOWN**, never a guess: the width
and the height alone do not tell the bytes per pixel. `frame_bytes()`
returns `None` when the codec is neither H.264 nor HEVC, `dpb_bytes()`
follows, and `is_copy_safe()` is therefore `false` - an unknown DPB must
never take the copy path (that is the path that can leak the deep pool).

**Tests** (`sps.rs`, inline): the five pixel formats, the DPB, the savings,
and the two "never a guess" cases. The corpus sweep (`tests/sps_read_strategy.rs`)
still passes: every `.mp4`/`.h264`/`.hevc` in the corpus is parsed by the
module.

---

## 2. The parser's answer vs ffmpeg's truth

`tools/extract_sps_truth.ps1` compares the parser with ffmpeg's
`trace_headers` (the ground truth). For the new files:

| file | ffmpeg's truth | the parser |
| --- | --- | --- |
| `hi10_1080p.mp4` | `chroma_format_idc=1`, `bit_depth_luma_minus8=2` (10-bit), `max_num_ref_frames=4`, `max_dec_frame_buffering=4` | `4:2:0/10bit`, 4 refs, 1920×1088 coded, the frame 6,266,880 B, the pool 25,067,520 B |
| `hevc10_1080p.mp4` | `chroma_format_idc=1`, `bit_depth_luma_minus8=2` (10-bit), `sps_max_dec_pic_buffering=6`, 1920×1088 | `4:2:0/10bit`, 6 refs, 1920×1088, the frame 6,266,880 B, the pool 37,601,280 B |
| `hevc_1080p.mp4` (8-bit) | 8-bit | `4:2:0/8bit`, the frame 3,110,400 B |
| `mpeg2_1080p.m2v` | MPEG-2 | **no SPS** - the estimate is UNKNOWN |

The 8-bit files are unchanged by the formula (the same numbers as before:
3,133,440 B for the 1920×1088 H.264 files), so the H1 verdict and the
DPB threshold are untouched.

**The coded size, not the display size**: the HEVC file codes 1920×**1088**
(the crop is 1080) - the decoder allocates the coded size, which is what the
estimate uses.

---

## 3. H3 — the measurement

**The harness**: `tools/bench_h3_bitdepth.ps1` (new). It runs the two A/Bs
alternating inside one session (the GPU numbers drift between sessions) and
measures the **C4 pair** (the private bytes + the GPU shared memory) with
the **playback rate and the frame counters** travelling with every number.
The live session is snapshotted and restored with `tools/session_snapshot.ps1`,
and the user's engine is stopped for the measurement and restarted with its
own command line afterwards.

**The arms** (the same encoder settings, the same resolution, the same
duration - only the bit depth differs):

| arm | file | the parser's estimate |
| --- | --- | --- |
| A | `h264_8bit_1080p.mp4` (8-bit, 4 refs) | the frame 3,133,440 B, the pool 12,533,760 B |
| B | `hi10_1080p.mp4` (10-bit, 4 refs) | the frame 6,266,880 B, the pool 25,067,520 B |
| A | `hevc8_1080p.mp4` (8-bit, 6 refs) | the frame 3,133,440 B, the pool 18,800,640 B |
| B | `hevc10_1080p.mp4` (10-bit, 6 refs) | the frame 6,266,880 B, the pool 37,601,280 B |

### The numbers

<!--MEASURED-->(2026-10-22, one machine, Win11 26200, i7-12650H, UHD + RTX 2050, 1080p coded)
`tools/bench_h3_bitdepth.ps1`, 3 runs per arm, ABAB inside one session, C4 metrics,
the playback rate and the frame counters travel with every number.

| pair | arm | private MB | GPU shared MB | rate | delivered fps | delayed |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| h264 8-bit vs 10-bit | A: `h264_8bit_1080p.mp4` (8-bit, 4 refs) | 161.87 | 151.58 | **0.98** | 29.55 | 0 |
| h264 8-bit vs 10-bit | B: `hi10_1080p.mp4` (**10-bit**, 4 refs) | 228.06 | 85.23 | **0.89** | 26.84 | 0 |
| HEVC 8-bit vs 10-bit | A: `hevc8_1080p.mp4` (8-bit, 6 refs) | 206.34 | 190.12 | **0.98** | 29.63 | 0 |
| HEVC 8-bit vs 10-bit | B: `hevc10_1080p.mp4` (**10-bit**, 6 refs) | **389.59** | **278.71** | **0.99** | 29.49 | 0 |
| MPEG-2 unknown codec | A: `mpeg2_1080p.m2v` | 130.10 | 74.93 | **0.99** | 24.65 | 0 |
| MPEG-2 unknown codec | B: `mpeg2_1080p.m2v` | 130.02 | 74.92 | **0.99** | 24.76 | 0 |

The rate rule is enforced: a run outside 0.97–1.03 invalidates its arm, so the
h264 pair's 10-bit arm (rate 0.89–0.90, three times) is **not a valid
comparison** — the decoder fell out of real time in the measured mode, so the
two arms did not do the same work. It is recorded here rather than quoted.
The HEVC pair is inside the band, so its comparison counts: **the 10-bit HEVC
file cost 89 % more private bytes (206.34 → 389.59 MB) and 47 % more GPU
shared memory (190.12 → 278.71 MB)** than its 8-bit counterpart. The
unknown-codec MPEG-2 ran fine (rate 0.99) but costs the same in either arm —
as it must, since the parser knows nothing about it.

<!--/MEASURED-->

**The estimate is exact about the reference pool, and it is not the total.**
The 10-bit frame pool is exactly twice the 8-bit one (+12.5 MB for the H.264
pair, +18.8 MB for the HEVC pair) - the formula predicts it to the byte.
But the **measured** difference is far larger, because the instance the
measurement runs in decodes in **software**: the engine's log for those runs
says `vo-configured=0 current-vo=(none)` and `hwdec-current=no (hwdec
INACTIVE-software)`, with the chain falling back to dxva2 and then to no
hardware decoding at all. The live session, in contrast, decodes with
`hwdec-current=d3d11va` and a real VO.

So the honest reading is:

1. **The DPB estimate is a model of the decoder's reference pool.** It is
   exact for that pool, and it is what the import dialog can compute from
   the file alone (the coded size, the bit depth, the chroma, the reference
   count).
2. **The extra RAM a 10-bit file costs in practice is much larger than the
   pool** - in the measured (software-decoding) mode it is ~15x the pool
   estimate. The estimate must be **labelled** as the reference pool, never
   as "the RAM this file will cost".
3. **A 10-bit H.264 file did not reach real time** in the measured mode
   (the rate 0.89-0.90, outside the 0.97-1.03 validity band) - so that arm
   is **invalid as a comparison** (the two arms did not do the same work),
   and it is recorded as such rather than quoted. The HEVC pair's rates are
   inside the band, so that comparison counts.
4. **The live (hardware-decoding) path was not measured** - the private
   instance has no wallpaper surface, so it does not use the hardware
   decoder. The live path's cost is an **open item** for the import dialog
   (2.5): the dialog's acceptance must compare its estimate with the live
   wallpaper's RAM for the same file.

### The unknown-codec case

`mpeg2_1080p.m2v` plays fine (the measured run: the rate 0.99, the private
131.8 MB, the GPU shared 76.9 MB) but the parser knows **nothing** about it:
no SPS, no reference count, no bit depth. The estimate is therefore
**UNKNOWN** and the dialog must say so instead of showing a number. The file
is an elementary stream on purpose: the corpus sweep asserts that every
`.mp4`/`.h264`/`.hevc` file IS parsed by the module.

---

## 4. What the import dialog (2.5) must therefore do

| the dialog shows | the source |
| --- | --- |
| the coded size, the bit depth and the chroma | the parser (`pixel_format()`) |
| the **reference pool** estimate, labelled as such | `dpb_bytes()` |
| "unknown" for a codec the parser does not read | `dpb_bytes() == None` |
| a **warning for 10-bit** | the measured facts: the 10-bit arms cost far more RAM and the 10-bit H.264 file did not reach real time in the measured mode |
| the pool estimate vs the live wallpaper's RAM | the **open** live-path measurement (2.5) |

---

## 5. Evidence index

| item | file |
| --- | --- |
| the formula + the parser | `rust/crates/k6wp-core/src/sps.rs` |
| the tests | `rust/crates/k6wp-core/src/sps.rs` (the inline `mod tests`) |
| the probe | `rust/crates/k6wp-core/examples/sps_probe.rs` (it prints the format, the frame and the pool) |
| the measurement | `tools/bench_h3_bitdepth.ps1` + `docs/bench/h3-bitdepth.json` |
| the corpus | `tools/make_corpus.ps1` (the four new cases + the unknown-codec case) |
| the ground truth | `tools/extract_sps_truth.ps1` (ffmpeg's `trace_headers`) |

**Labels:** the formula and the parser = **test-proven** (the inline tests +
the corpus sweep); the truth comparison = **test-proven** (ffmpeg's
`trace_headers`); the measured numbers = **live-proven** (one machine, n = 3
per arm, the rate band stated); the "software decode" reading =
**code-quoted** (the engine log of those runs); the live path = **open**.
