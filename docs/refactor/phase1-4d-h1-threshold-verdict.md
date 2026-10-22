# H1 - the G1 threshold: the honest verdict, the held-back RAM, and the
# threshold evaluated by the savings it can obtain

Task H1 (v1.6). G1 decided the DPB re-encode threshold as a **byte
budget**. The brief asks for three things, and this file answers them:

1. the verdict is recorded as an **owner-directed decision** (wallpaper
   quality over RAM), not as "better on all sides";
2. the RAM that is **held back** by that decision is stated with **n ≥ 5,
   the medians and the noise band**;
3. the threshold is evaluated by the **savings that can be obtained**
   (`(refs − pin_refs) × frame size`), not by the total DPB - and it is
   changed **only if the data supports it**.

---

## 1. The verdict: a trade-off, decided by the owner

**The decision:** a file whose decoder pool costs more than **64 MB** is
re-encoded (with the DPB pinned to 2 reference frames) so the decoder's
pool stays bounded; a file below that is **copied** byte-for-byte.

**What that buys:** the 4K case with a 16-frame reference list drops from
**386.79 MB** private + **467.73 MB** GPU shared to **228.20 MB** +
**432.53 MB** (the DPB-pinned re-encode), with the rate 0.99 and 0 delayed
frames.

**What that costs:** three 1080p H.264 files are now **copied instead of
re-encoded**, so they keep their original picture (no second lossy
encode) **and** they keep their deeper decoder pool. The RAM that is held
back is measured below.

**This is a trade-off, not a win on every axis.** The previous rule (a
reference count > 4) re-encoded those 1080p files: a **lossy** re-encode
for a few megabytes of RAM. The byte-budget rule keeps the picture and
spends the RAM. **The owner decides which side matters** - and the decision
recorded here is: **the wallpaper quality wins over the RAM** for the
1080p files, while the 4K case (where the pool is enormous) is still
re-encoded.

**The honest summary for the ledger:** *the byte-budget rule is an
owner-directed decision; it holds back +X MB on the three 1080p H.264
cases and saves 158.6 MB on the 4K case.*

## 2. The held-back RAM, measured (n = 5, the medians and the bands)

The two arms are the **same content**:

- **arm A** = the corpus file, copied (the byte-budget rule: no re-encode);
- **arm B** = the same file re-encoded with the DPB pinned (the old
  reference-count rule's outcome).

The arms alternate **ABAB inside one session** (`tools/bench_dpb_ab.ps1`),
the metric is private bytes + GPU shared, and the playback rate and the
frame counters travel with every number. The re-encoded arm was produced
by the **current** binary with the new diagnostic flag
`--force-dpb-reencode` (a measurement must not need a modified build).

| case | A: private (median) | A: band | B: private (median) | B: band | **the held-back RAM** | A rate | B rate |
|---|---|---|---|---|---|---|---|
| `meddpb_1080p` | **170.40 MB** | 9.73 MB | **157.17 MB** | 1.11 MB | **+13.23 MB** | 1.00 | 1.00 |
| `highdpb_1080p` | **194.02 MB** | 0.82 MB | **158.28 MB** | 2.05 MB | **+35.74 MB** | 1.00 | 1.00 |
| `multi_sps` | **173.16 MB** | 3.28 MB | **157.70 MB** | 3.91 MB | **+15.46 MB** | 1.00 | 1.00 |
| `highdpb_4k` (the case the rule targets) | 386.79 MB | - | 228.20 MB | - | **-158.59 MB** (saved) | 0.99 | 0.99 |

**The band** is the spread of the five samples (max - min) - a
conservative noise band: a difference has to be larger than it to mean
something. Every difference above is larger than the band of its arm
(the meddpb case is the tightest: 13.23 MB against a 9.73 MB band).

**The GPU shared** moves the same way: +2.35 MB (meddpb), +6.01 MB
(highdpb_1080p), +9.27 MB (multi_sps) held back.

**The rate rule:** every arm above is inside 0.97-1.03 ✓, so every number
is valid.

**One measurement was invalid first and was re-measured (the cause is
proven, not waved away):** the first `meddpb_1080p` run had a rate of
0.89-0.97 in both arms, which invalidates it by the rate rule. The cause:
**the file is only ≈ 5.6 s long** and the measurement window (6 s settle +
8 s sample = 14 s) runs past the end of the file, so the rate is computed
over a partial playback. The window was shortened to 1 s + 3 s (still
inside the file) and the case was measured again - the rate is 0.985-1.013
✓ valid. The other two cases are long enough for the default window.

**The discrepancy with the G1 numbers (+4.9 / +29.4 / +11.1 MB) is
recorded, not hidden:** those came from a single run whose report was not
saved and whose window was not recorded; the numbers above are the
reproducible ones (n = 5, the medians, the bands, the reports in
`docs/bench/h1-ab-*.json`). The G1 numbers are **withdrawn** in favour of
these.

## 3. The threshold evaluated by the savings it can obtain

**The rule under test (the brief's formulation):** a re-encode is worth it
when the RAM it saves is worth the picture it costs. The savings that can
be obtained are

```
savings = (effective_refs − pin_refs) × frame_bytes
```

**not** the total DPB: the total DPB says how big the pool is, the savings
say how much the re-encode would actually **recover**. A file whose pool is
48 MB and whose pinned pool is 5 MB saves 43 MB; a file whose pool is
48 MB and whose pinned pool is 24 MB saves 24 MB. The two differ whenever
the pinned pool is not negligible - and the pinned pool is **not**
negligible at 4K (2 frames of 4K = 24.9 MB).

**The evaluation over the corpus** (the numbers are the DPB data from
`docs/refactor/phase1-4-sps-dpb.md`; the 4:2:0 8-bit factor = 1.5
byte/pixel - H2 generalises it to the bit depth and the chroma format):

| file | effective refs | the total DPB | the pinned DPB | **the savings obtainable** | the current rule (the total > 64 MB) | the savings rule (the saving > 64 MB) |
|---|---|---|---|---|---|---|
| `anime` | 4 | 12.5 MB | 6.3 MB | 6.3 MB | copy | copy |
| `meddpb_1080p` | 8 | 25.1 MB | 6.3 MB | 18.8 MB | copy | copy |
| `highdpb_1080p` | 16 | 50.1 MB | 6.3 MB | 43.9 MB | copy | copy |
| `multi_sps` | 16 | 50.1 MB | 6.3 MB | 43.9 MB | copy | copy |
| `highdpb_4k` | 16 | 199.1 MB | 24.9 MB | **174.2 MB** | **re-encode** | **re-encode** |

**The two formulations agree on every corpus case.** The data therefore
does **not** support a change: **the threshold stays at 64 MB**, and no
A/B or parity re-run is needed (the behaviour is unchanged).

**But they are not the same rule, and the owner should know where they
diverge.** The corpus has no case in the divergence zone, so the data
cannot speak about it. The divergence:

| file | the total DPB | the total rule (today) | the savings rule | the difference |
|---|---|---|---|---|
| a 4K file with 6 refs | 74.7 MB | **re-encode** (74.7 > 64) | copy (the savings 49.8 MB < 64) | the total rule re-encodes **more** often |
| a 4K file with 16 refs | 199.1 MB | re-encode | re-encode (the savings 174.2 MB) | the same |

So the total rule is the **more conservative** one at 4K: it re-encodes in
the zone where the savings are only moderate. The corpus cannot say
whether that is right (no such file exists in it) - so **the threshold
stays as it is** (the total-DPB byte budget, the owner-directed decision
of section 1) and the savings evaluation is the **tool** for a future
revision, not a silent change.

**The implementation** exposes the savings next to the total
(`SpsInfo::savings_obtainable()`, pinned by a test against the corpus), so
the evaluation above is reproducible and a future revision is a one-line
change **plus** the A/B and the parity re-run (which is what the brief
demands: a threshold change is only accepted with a re-measured A/B and a
re-verified parity).

**What would change the threshold:** a corpus case whose savings are just
above/below 64 MB while its total DPB is on the other side of the line -
none exists today. If one appears, the A/B **and** the parity are re-run
before the change is accepted.

## 4. The import dialog (task 2.5)

The Studio's import flow (2.5) must show the **estimated extra RAM** the
parser reports for the file being imported, and let the user choose:

- **"Optimalkan (re-encode)"** - the file is re-encoded (the DPB pinned),
  the RAM is bounded, the picture takes a second lossy encode;
- **"Pertahankan asli"** - the file is copied, the picture is untouched,
  the decoder's pool stays as big as the file's SPS says.

**The default follows the H1 decision:** the byte-budget rule's own
verdict - re-encode only when the file's DPB costs more than 64 MB, and
otherwise keep the original. The dialog exists so the user can **override**
that default per file; the estimated RAM must come from the **parser** (the
same numbers the threshold uses), never from a guess.

**The flow budget (2.4):** the choice must not add a step to the common
path - importing a file that needs no decision must stay at **≤ 2 clicks**
from the library, and the dialog itself is one modal with two buttons.

## 5. The files

| file | what |
|---|---|
| `docs/bench/h1-ab-meddpb_1080p.json` | the A/B report (n = 5, the medians, the bands, every run) |
| `docs/bench/h1-ab-highdpb_1080p.json` | the same |
| `docs/bench/h1-ab-multi_sps.json` | the same |
| `tools/bench_dpb_ab.ps1` | the A/B harness (now session-safe: the wallpaper is restored, the live engine is restarted, and the report carries the noise band) |
| `compressor/src/cli.{hpp,cpp}` | the `--force-dpb-reencode` diagnostic flag (the re-encoded arm of the measurement, from the current binary) |
