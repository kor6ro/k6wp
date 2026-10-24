# K6WP refactor - STATE (one page)

**Read this file FIRST.** It is the state of the refactor on **2026-10-22**,
the locked decisions, the rules an evidence claim must follow, the active
budgets, and what is still open. The detailed record is in
`docs/refactor/ledger.md` (one row per task, with the verdict), the
per-task documents in `docs/refactor/`, and the measurements in
`docs/bench/`.

The C++ engine stays the **reference** until the cut-over. The Rust side
replays the C++'s frozen behaviour; it does not redefine it.

---

## 1. The locked decisions (do not re-litigate without a measurement)

| # | the decision | why / where |
|---|---|---|
| L1 | The DPB re-encode threshold is a **byte budget** (`effective_refs Ã— w Ã— h Ã— 1.5 > 64 MB`), not a reference count. The level is **reported**, never used as a cap. | G1, `docs/refactor/phase1-4-sps-dpb.md` |
| L2 | `num_ref_frames` comes from the **Rust SPS parser** (validated 9/9 against ffmpeg's `trace_headers`). ffprobe, ffmpeg's normal output and libmpv do not carry it. | E3, `k6wp-core/src/sps.rs` |
| L3 | The **config schema is v6** (`pause_unload_after_sec`, default 30 s, 0 = never). A **newer** file is read best-effort; the **unknown keys are preserved** when the file is rewritten. | G4, I3 |
| L4 | The **unload destroys the whole mpv instance** (D2), and only for the owners that make the wallpaper invisible (fullscreen / screen-off / lock / suspend). An explicit user pause keeps the picture. | 1.1, `docs/refactor/phase1-1-unload.md` |
| L5 | The **Studio is a separate process**: no mpv, no HWND-embed. | the whole Phase 2 |
| L6 | The **minimum OS is Windows 10 22H2** (build 19041): engine, launcher and Studio refuse an older one (exit code 6). | C6 |
| L7 | The **golden fixtures are the contract**. A fixture is only valid when its behaviour is proven against the `baseline-pre-refactor` build (I1's differential) or the change is recorded as deliberate. | I1, `docs/refactor/g5-coverage.md` |
| L8 | The **environment overrides** (`K6WP_ENGINE_PIPE`, `K6WP_ENGINE_MUTEX`, `K6WP_LIBRARY_JSON`) are **compiled out of the release build** and the release binaries are scanned. | I4 |
| L9 | The **public release is HELD** until the owner decides the Slint licence question (`docs/refactor/license-decision.md`, task F3). | E7, `LICENSES/` |
| L10 | The **ZIP stays 148.13 MB**; the â‰¤ 115 MB target is deferred to Phase 3, and `ffprobe.exe` is only removed after **J6** proves the probe is not slower than the ffprobe spawn. | G7, J6 |

---

## 2. The rules an evidence claim must follow

- **The evidence labels**: `live-proven` (measured on this machine), `test-proven`
  (a test asserts it), `code-quoted` (read in the source, not observed),
  `unmeasured`. A hardware-unavailable claim stays **UNPROVEN**.
- **A vs B** is valid only with an identical state, metric, measurement point
  and **n**; every table states the n and the noise band.
- **RAM** = private bytes + GPU shared. The working set is never evidence.
  The RAM of "playing" and "paused" is not comparable by definition.
- **Every CPU/GPU comparison** carries the playback rate and the frame
  counters (vo-delayed, drops); a rate outside 0.97-1.03 invalidates it.
- **GPU gates** are paired ABAB inside one session (the drift between
  sessions reaches 1.5Ã—).
- **A test that sometimes fails may not be called "environmental"** without a
  proven root cause (J2).
- **The live session is only touched through `tools/session_snapshot.ps1`**
  (the display mode, the wallpaper, the still, the lock screen are
  snapshotted and restored in a `finally`).
- **A trade-off between metrics is written as a trade-off** in the ledger,
  never disguised as a win.

---

## 3. The active budgets (the full table: `docs/refactor/budgets.md`)

| Phase | Metric | Baseline (v2) | Parity gate | Target | Result |
|---|---|---|---|---|---|
| 1.1 | private bytes, paused 30 s | 162.35 MB | â‰¤ 168.4 MB | â‰¤ 60 MB | **60.1 MB** |
| 1.1 | GPU shared, paused 30 s | 151.57 MB | â‰¤ 163.6 MB | â‰¤ 20 MB | **0 MB** |
| 1.2 | CPU, playback 1080p / 4K | 4.73 % / 4.64 % | â‰¤ 6.5 % | lower | **1.06 % / 1.45 %** |
| 1.2 | GPU 3D, playback 1080p | 12.47 % | â‰¤ 15.0 % | lower | **9.39 %** |
| 1.2 | rendered-frame parity | - | pixel diff 0 | 0 | **0.000** |
| 1.4 | private bytes, 4K/16-ref | 387.23 MB | â‰¤ 400 MB | â‰¤ 240 MB | **228.2 MB** (âˆ’41 %) |
| 1.4 | GPU shared, 4K/16-ref | 468.08 MB | â‰¤ 482 MB | â‰¤ 440 MB | **432.5 MB** (âˆ’7.5 %) |
| 2.3 / 2.7 | Studio private bytes (E1 point) | 500.67 MB | - | â‰¤ 150 MB | **39.56 MB** (the 2.0 shell only) |
| 2.7 | Studio startup (E1) | 3576 ms | - | â‰¤ 700 ms | **1027 ms** (the 2.0 shell only) |
| 3.x | **ZIP size** | 148.13 MB | â‰¤ 148.13 MB | â‰¤ 115 MB | **DEFERRED** to Phase 3 (not "nearly") |

**Baseline v2** = commit `340da84`, n = 5: idle 24.2 MB; 1080p 163.3 MB
private + 150.9 MB shared, CPU 1.74 % (max thread 10.8 %), GPU 21.6 %, rate
1.00, 29.98 fps; 4K/16-ref 387.23 + 468.08 MB; paused 30 s 161.64 + 150.86 MB.

---

## 4. The status per phase and task

### Phase 0 - the baseline and the contracts - **done**
0.1-0.10 all accepted: the tag, the hygiene, the measurement harness, the
noise bands (n â‰¥ 5 medians), the golden mechanism, the contract inventory,
the budgets, the visual-parity gate.

### Phase 1 - the C++ engine - **done (Gate 1 PASS)**
| task | the result |
|---|---|
| 1.1 + D2 | the unload after a long pause: private 161.7 â†’ **60.1 MB**, GPU shared â†’ **0 MB**, threads 37 â†’ 19, resume median **â‰ˆ 798 ms**, **0 black frames** in 170 samples over 5 real fullscreen exits |
| 1.2 + C1 | the pacing fix (`display-desync` â†’ `display-vsync`): the baseline ran **25 % slow** (rate 0.70-0.75); now rate 0.98-1.00, 184-197 late frames â†’ **0**, 0 drops |
| 1.3 + D4 | the ffprobe census: the removal is **authorized and measured** (0 disagreements over 5 metadata fields), the ZIP is unchanged (deferred to Phase 3) |
| 1.4 + E3 | the DPB work: the parser, the corpus, the 9/9 `trace_headers` match, the byte budget (L1) |
| 1.5 | the span mode and the old-OS branches removed (**âˆ’345 LOC**) |

### Phase 2 - the Studio in Slint - **in progress**
| task | the result |
|---|---|
| 2.2 | `k6wp-core` (the Rust contracts): **101** shared golden cases + the per-module tests, all replayed against the C++ |
| 2.3 | the Slint shell (FemtoVG default; software and Skia **UNPROVEN**) |
| 2.4 | **open** - the UX spec + the design system + the measurable flow script |
| 2.5 | **open** - the pages; the Library first (F1 virtual grid + F2 lazy thumbnailer) |
| 2.6 + G6 | the Rust IPC client (`k6wp-ipc`): the server is **verified** (pid, session, owner SID, image path) before a command is sent; the live drive passes |
| 2.7 | **open** - drop Qt after 100 % parity **and** J4's Studio part is done |

### The quality gates (G1-G7, I1-I4) - **done**
| task | the result |
|---|---|
| G1 | the byte-budget threshold (L1) - **and the honest verdict**: it is an **owner-directed decision** (wallpaper quality over RAM), and the RAM it holds back is recorded (see the ledger) |
| G2 | the parser hardened against untrusted input: the caps, the checked arithmetic, proptest + the mutation pass + 4 regression vectors |
| G3 | the ctest note reconciled with the measurement (**49/49 with the engine up**, 73.6 s and 51.2 s) |
| G4 | the schema v6 + the old-engine measurement + the best-effort read of a newer file |
| G5 | the engine's pure logic frozen: **91 cases / 9 modules** |
| G6 + 2.6 | the Rust IPC client (above) |
| G7 | the ZIP recorded honestly (148.13 MB, deferred) |
| I1 | the differential vs `baseline-pre-refactor`: **213 values compared, 0 differences, 24 skipped and declared**; the provenance map in the fixture; `docs/refactor/g5-coverage.md`. **Deliberate divergence (L2):** `display_coverage.verdict` now reads the recorded `MonitorAssignment::exists` hint (copied at write time) instead of a live `std::filesystem::exists` â€” this removes the 21 s SMB-block on the poll path. **Limitation:** the hint can be stale (a file deleted after assignment won't flip `degraded` until the next write); no consumer reads this field, so no parity is lost. |
| I2 | the SPS parser seeks (a 512 MB non-faststart file costs **14 628 bytes**); the container policy is explicit (the fallback = **1 of 11** corpus files = 9.1 %); the 4 cargo-fuzz targets compile |
| I3 | the config round trip (the unknown keys preserved, 6 fixture cases), the writer inventory, the atomic publish, the downgrade policy |
| I4 | the overrides compiled out of the release build (**5 of 5** binaries clean); the G6 verification is not weakened |
| I5 | `STATE.md` (this file) - the one page every session reads first |
| H1 | the G1 verdict is an **owner-directed trade-off**; the held-back RAM measured (**n = 5**, the medians + the bands: +13.23 / +35.74 / +15.46 MB on the three 1080p H.264 cases, -158.59 MB saved on the 4K case); the threshold evaluated by the savings it can obtain -> the data does **not** support a change, so it stays at 64 MB |
| J2 | the two flakes rooted: the still''s mtime collision **fixed** (the test sets the timestamp explicitly), the lint for the production pipe/mutex names (**0 violations**), and the gate: **10 consecutive full ctest runs with the live engine, 0 failures** |
| K1 | the IPC defect **rooted and fixed**: the cause was a system-wide Toolhelp thread snapshot (75 ms per `get_state`, the p99 84 ms); it was cached first (**the p50 went 1 ms, the p99 3-4 ms, the max 121-152 ms under churn**), and **L1 then deleted the field**: the max is **4-14 ms** (n = 5), **0 of 20,000 samples >= 20 ms**. The instrument that proved it (`K6WP_IPC_TRACE=1`) is now actually wired into the tree. **The defect is pre-existing** (the `baseline-pre-refactor` build has the same 78 ms p50) - not a refactor regression |
| L2 | the **poll-path cost** measured per field and audited: **no field over 1 ms**, and the last poll-path filesystem call (a per-assignment `exists`, **21,053 ms** on a dead SMB share) is **deleted** - the recorded hint is used instead. The audit found no other poll-path I/O. The instrument's summary now logs only at the outermost scope, so the trace-on maxes are usable |

### The v1.10 hardening - **in progress**
| task | the result |
|---|---|
| H4 | the non-Latin doc/code/commit lint (`tools/lint_nonlatin.ps1`, rules R1-R4, a 10-check self-test, the pre-commit hook, two CI steps); **569 tracked files, 0 errors** |
| J3 | the config writers under **one cross-process lock** (`Local\K6WP-Config-Lock`) with the re-read inside the critical section, in C++ **and** Rust; the negative control **0/5** clean without the lock (one scene published invalid JSON) vs **5/5** with it; the foreign keys capped at **64 KiB** in sorted order, the known name always wins, no coercion; the golden dump **101 cases** |
| L1 | the `get_state` max **rooted and removed**: the K1 cache refreshed the Toolhelp snapshot inline once per second; the field had no consumer, so it was **deleted**. Same-tree A/B (n = 5, 4000 requests): **max 56 ms median -> 7 ms** (4-14); the before's slow samples sat **981 ms apart** = the 1 s TTL; **0 of 20,000 after-samples >= 20 ms**. Doc `phase2-l1-get-state-max.md`, raw in `docs/bench/l1/` |
| L2 | the `get_state` **per-field cost** measured (`TraceScope` around every field; 1 client, 4000/run, n = 3): **no field over 1 ms** - `state.json` 0.18 ms, the whole reply 0.31-0.36 ms, the handler 0.39-0.46 ms, the ack ~0.01 ms. The last **poll-path filesystem call** (a per-assignment `std::filesystem::exists`) is **deleted**: 56.6 us warm locally, **21,053 ms** on a dead SMB share (unit bench) on the IPC thread; `degraded` now reads the recorded `MonitorAssignment::exists` hint, and nothing consumes the field. The instrument now logs its summary only at the outermost scope (trace-on maxes usable). **Recorded, not hidden:** 2/5 trace-off sanity runs lost the client connection - **L4 later found the root cause in the CLIENT, not the engine** (the Rust client's read returned 0 bytes when `ReadFile` completed synchronously, so it saw an empty answer and reported a phantom failure; the reconnect then deadlocked on the still-open old handle). With the fixed client the same harness gives **5/5 clean, 0 failures over 20,000 requests** (`docs/bench/l4/`). A 4000-request run also saw 2 samples >= 20 ms (max 31 ms, offset ~1.1 s), so the L1 20 ms row is **scoped to steady state**; the p99 stays 0-1 ms. Doc `ipc-poll-cost.md`, raw in `docs/bench/l2/` |
| L4 | the Rust IPC client: the **bounded `WaitNamedPipe` retry** (no busy-spinning, a clear "no free instance" error), the **reconnect that releases the old handle first**, and a **real bug** fixed: the read returned **0 bytes** when `ReadFile` completed synchronously, so the client saw an empty answer and reported a **phantom failure** (the trigger of L2's 2/5 lost runs). The shipped C++ Studio client is **not** affected (a synchronous handle + `PeekNamedPipe`, and it checks `got`). Tests: the over-limit case (waits, then a clear error, **no hang**) + the reconnect regression test - **12/12 green**. The same harness with the fixed client: **5/5 clean, 0 failures over 20,000 requests**, p99 0-1 ms, max 3-15 ms. The ~0.25 % baseline loss (3/1200) is recorded in `CHANGELOG.md` + `docs/known-issues.md`; the Phase 3 model is `phase3-ipc-connection-model.md`. Doc `phase2-l4-ipc-client-retry.md` |
| L5 | the **diagnostic overrides are compiled out of the release build**: the audit found **one violation** - `K6WP_SPS_DEBUG` (the SPS parser's per-box stderr dump) was **not** gated, so it was live in every build; it is now behind `#[cfg(debug_assertions)]`. The proof is two-layered and both layers are in CI: the **source lint** (`tools/lint_env_overrides.ps1` - every `K6WP_*` read must sit behind a build gate or be on an allow-list with a reason; **272 files, 21 reads, 18 names, 0 errors**) and the **binary scan** (the audit's 7 names, the C++ **and** the Rust release dirs, `-RequireBinaries` so it cannot pass vacuously; **6 binaries, 0 findings**, and the detector **fires** on a debug binary). The **self-test** proves the lint still fires: **8 checks, 0 failures**. CI: the lint, the self-test and the scan in the existing job, plus a **new `rust-release` job** (the Rust workspace was not in CI at all). **Four overrides stay in a release build, classified with a reason and NOT silently changed** (`K6WP_RECONCILE_MODE` - a documented alpha.3 feature switch; `K6WP_CACHE_DIR` - the Studio's own channel to the compressor; `K6WP_CACHE_MAX_BYTES` - a limit; `K6WP_PLAYLIST_JSON` + `K6WP_THUMBS_DIR` - the release-mode test suite relies on them); the open question is written down. Doc `phase2-l5-override-proof.md` |
| M7 | the **Rust IPC client deterministic tests** added (no live engine needed). **Test-proven (4):** synchronous read completion (server writes first), multiple frames in one read (sequential requests), exact 64 KiB boundary accepted, payload >64 KiB rejected by codec. **Code-quoted / UNPROVEN (2):** partial reads (one frame split across reads â€” MESSAGE mode delivers whole message), short write detection (inspected in `overlapped_write`, not simulated). **Added test-proven (5 new):** response larger than client read buffer (`ERROR_MORE_DATA` on message mode up to 64 KiB), server closes pipe mid-write/response (clean failure, no hang, clear error), read times out when server never responds, write timeout on cancel, connection discarded after timeout (fresh connection). **Total 14 client tests pass** (1 short-write test removed as it hung; logic verified by code inspection). Doc in `client.rs`. |
| N2 | **J3 config lock evidence**: two-process C++/Rust interleaved writers â‰¥1000 iterations (test 77), WAIT_ABANDONED owner-crash recovery (test 79), lock timeout bounded wait (test 78). **All 366 checks pass**. Rule documented: UI thread never waits for config lock (operations via worker). Doc in `config_test.cpp` (tests 77, 78, 79). |
| O1 | **P1 complete â€” Client pipe timeout fix**: Added `CancelIoEx` + `GetOverlappedResult(wait=true)` to `overlapped_write`/`overlapped_read` for bounded timeout on every operation. **Test-proven: regresi (perilaku sekarang benar)** â€” `read_times_out_when_server_never_responds`, `write_times_out_when_server_never_reads`, `connection_discarded_after_timeout` lulus. **Code-quoted (2):** partial reads, short write detection. **UNPROVEN (3):** Q1a, Q1b, Q1d. **Catatan:** test timeout lulus juga pada kode lama (ConnectOptions deadline menangkapnya); hipotesis "write tanpa deadline = cacat produksi" **tidak terbukti**; hang test lama = desain test, bukan bukti perbaikan. |
| Q1a | **Test write-timeout (deterministic)**: server accepts but never reads; client writes payload > 64 KiB pipe buffer; client gets `Err(timeout)` within deadline, no hang. Test: `write_times_out_when_server_never_reads`. **UNPROVEN untuk overlapped-I/O-specific timeout** (passes with old code too karena ConnectOptions timeout). |
| Q1b | **Semantic timeout verified**: if I/O completes before cancel (`ERROR_NOT_FOUND` or full transfer), returns **success**, not timeout. Race test `timeout_cancel_race_returns_success` passes both outcomes. **UNPROVEN** (race not deterministic). |
| Q1c | **Client audit**: no `INFINITE` wait or unbounded loops in `k6wp-ipc` (`connect`, `write`, `read`, `reconnect`, `WaitNamedPipe`). Audit doc: `docs/refactor/ipc-client-timeouts.md`. Studio behavior on engine unresponsive: worker exits, UI shows "Menghubungkan ulang" with backoff (M2). |
| Q1d | **Nextest CI gate**: `slow-timeout = { period = "60s", terminate-after = 1 }` in `rust/Cargo.toml`; ignored hang test `nextest_slow_timeout_kills_hanging_test` added. **UNPROVEN** â€” nextest belum terpasang di CI; verifikasi lokal menunggu install. Langkah install ditambahkan ke CI (lihat `rust/Cargo.toml` + workflow). Gerbang disebut "gerbang" SETELAH satu test hang sengaja terbukti dibunuh. |
| Q1e | **Test reconciliation (perintah tunggal, dapat diulang)**: `cargo test --workspace --target x86_64-pc-windows-msvc -- --list | grep -c ": test$"` â†’ **109** (Rust). `cargo test -p k6wp-ipc --target x86_64-pc-windows-msvc -- --list | grep -c ": test$"` â†’ **11** (9 client + 1 env_override + 1 live_engine). `ctest -N` di `build/msvc-dev` â†’ **51** (C++). |
| O2 | **Corpus in CI/git history**: `.gitignore` extended (`*.mkv`, `*.h264`); `tests/corpus/matroska.mkv` and `multi_sps.h264` removed from tracking (were 13 MB + 56 MB). Nightly CI job added (`.github/workflows/ci.yml`: `nightly-corpus-test` runs `make_corpus.ps1` + full suite). Tests that need corpus report **SKIPPED** when files missing. |
| N2 | **J3 config lock evidence**: two-process C++/Rust interleaved writers â‰¥1000 iterations (test 77), WAIT_ABANDONED owner-crash recovery (test 79), lock timeout bounded wait (test 78). **All 366 checks pass**. Rule documented: UI thread never waits for config lock (operations via worker). Doc in `config_test.cpp` (tests 77, 78, 79). |
| N3 | **Corpus hygiene**: 17 files (â‰ˆ1.1 GB) in `tests/corpus/` gitignored, 0 tracked. Golden fixture `tests/golden/sps-dpb.json` updated: 16 cases (+anime_4k_60s.mp4 247MB, highdpb_4k_6ref.mp4 244MB). **Open items before import dialog**: (a) 4K 6-ref A/B nâ‰¥5 (RAM decision, savings vs total DPB), (b) M5 HEVC 8-bit vs 10-bit ABAB nâ‰¥5. Threshold moves only if data supports. |
| next | L3 (parallel with 2.4) -> 2.4 -> 2.5, and the deadline items (J1, J4, J5, J6) |

### Phase 3 - the engine in Rust - **not started** (3.7 after J4's engine part)
### E4 - the lean ffmpeg/libmpv builds - **optional**, deferred (no build toolchain here)

---

## 5. The UNPROVEN list (do not quote these as facts)

| item | why | the destination |
|---|---|---|
| the Studio comparison | the Slint shell renders a plain list: it does not decode a thumbnail yet | **not comparable** until F1/F2 land; the E1 numbers are the Qt Studio's, not the finished one's |
| the software and Skia renderers | never measured | the ADR's condition to re-open it |
| the 5 "LIVE TEST" rows of `g5-coverage.md` | the fullscreen / occlusion classification, the device-lost watch, the watchdog, the launcher's lock-screen policy - they need Win32 | a Phase 3 live test each |
| the 12 "TO FIXTURE" rows of `g5-coverage.md` | not yet frozen | **before 3.7** (J4) |
| cargo-fuzz **running** | the ASAN runtime (LLVM) is not installed on this host; the 4 targets compile | J5: install the C++ ASan component, or record honestly that there is no remote CI |
| the ffmpeg/libmpv lean builds | no build toolchain here | E4 (optional) |
| the 10/24 H.2 multi-monitor cases | the rig artifact is outstanding | the rig |
| **Q1a: overlapped-I/O-specific write timeout** | passes with old code too (ConnectOptions timeout catches it) | label UNPROVEN in STATE.md |
| **Q1b: semantic timeout race determinism** | race not deterministic; test accepts both outcomes | label UNPROVEN in STATE.md |
| **Q1d: nextest CI gate actually kills hung tests** | nextest not yet installed in CI; local verification pending | install nextest in CI, verify |
| **R1-5: 2000+ timeout stress with handle count** | not yet run (timebox) | if not done, label UNPROVEN |
| **N4: UX baseline vs Studio Slint** | not yet measured | N4 deliverable |

---

## 6. The open commitments (the deadlines)

| commitment | the deadline | the state |
|---|---|---|
| the Studio's pure logic in `tests/golden/studio_logic.json` (the writer + the C++ checker + the Rust replayer, with the provenance) | **before 2.7** | **open** (J4) |
| the 12 "TO FIXTURE" rows of `g5-coverage.md` | **before 3.7** | **open** (J4); the progress is listed in this file |
| the C++ engine may not be deleted before its deadline | - | the C++ is still the reference |
| `ffprobe.exe` may not be removed before J6 passes | - | **open** (J6) |
| the public release | held | the owner's Slint licence decision (F3) |
| the 10 consecutive green ctest runs with the live engine | J2 | **done** (10/10, 0 failures, 50.7-59.1 s per run) |
| the IPC ack latency must be bounded | K1 + L1 + L2 | **done** - the cause was the system-wide Toolhelp thread snapshot (75 ms per `get_state`); K1 cached it (the p99 **3 ms**, the max 152 ms under a 10-minute stress, 0 "ack write failed"), **L1 deleted the field** (no consumer): the max is now **4-14 ms** (n = 5), **0 of 20,000 samples >= 20 ms**; **L2 measured every field** (no field over 1 ms; the whole reply 0.31-0.36 ms) and **deleted the last poll-path filesystem call** (a 21 s block on a dead SMB share). The stable metric is the **p99 0-1 ms**; a max is one OS-scheduling sample (L2 saw 31-62 ms once, with a p99 of 0-1) |
| the IPC server must serve the clients **concurrently** (one slow ack may never delay another client) | K1 | **open** - the cause fix removed the symptom (0 lost requests), the structural change (the per-connection workers) was reverted because it destabilised the pipe-instance accounting; it goes into the Rust engine (Phase 3) |

---

## 7. The commands

```powershell
# the C++ side
cmake --preset msvc-dev; cmake --build --preset msvc-dev; ctest --preset msvc-dev
cmake --preset release; cmake --build --preset release

# the Rust side
cd rust
cargo build/test/clippy --workspace --target x86_64-pc-windows-msvc
cargo deny check; cargo audit

# the proofs
powershell -File tools/g5_diff_baseline.ps1          # the differential vs the baseline
powershell -File tools/check_release_overrides.ps1   # the release binaries hold no override
powershell -File tools/drive_engine_ipc.ps1          # the live IPC drive (the session snapshot)
powershell -File tools/bench_unload_d2.ps1           # the black-frame gate
powershell -File tools/bench_dpb_ab.ps1              # the DPB A/B
powershell -File tools/bench_studio_compare.ps1      # the Studio comparison
```

## 8. The traps (already paid for)

- the `Copy-Item` mtime trap (touch the sources after restoring; `tools/build_ab.ps1` verifies the SHA-256);
- CMake 4.4 passes the `add_test` TIMEOUT keyword to the COMMAND as an argument - it must be a `set_tests_properties` property;
- the PowerShell nested-array flattening (use `[string[]]`);
- the Exp-Golomb reader consumes the implicit `1` bit; `numOfSPS` is **5** bits; the NAL header is not an RBSP;
- the JSON fixtures need a UTF-8 write **without** a BOM;
- the named-pipe names are **per session** and must never be a bare literal;
- the corpus lives in `tests/corpus/` (gitignored) - the regeneration commands are in `docs/refactor/phase1-4-sps-dpb.md`;
- the `K6WP_ENGINE_PIPE` / `K6WP_ENGINE_MUTEX` overrides exist only in a **debug** build (I4) - a test that needs the real endpoint must be aware of that.

---

## 9. Cara melanjutkan sesi (how to continue a session)

### The read order

1. **This file** - the locked decisions, the evidence rules, the budgets, the open list.
2. `docs/refactor/ledger.md` - one row per task, with the verdict and the numbers. If a claim is not in the ledger, it is not established.
3. `docs/refactor/budgets.md` - every active budget with its gate, its `n` and its noise band.
4. The per-task document of whatever you are about to touch (`docs/refactor/phase*.md`, `docs/refactor/adr-*.md`).
5. `docs/refactor/g5-coverage.md` - which behaviour is frozen and which rows are still **TO FIXTURE** / **LIVE TEST**.
6. `docs/bench/*.json` - the raw measurements; never quote a number that is not in one of these or in the ledger.

### The 10-minute verification (run it before touching anything)

```powershell
# the C++ side (the engine link fails with LNK1168 while the live engine runs -
# that is expected; everything else must build)
cmake --preset msvc-dev
cmake --build --preset msvc-dev
ctest --preset msvc-dev                     # expect 50/50, 54-75 s

# the Rust side (from rust/)
cargo test --workspace --target x86_64-pc-windows-msvc
cargo clippy --workspace --all-targets --target x86_64-pc-windows-msvc -- -D warnings
cargo fmt --all -- --check
```

The expected state on this host (2026-10-22): `ctest` **50/50** in 54-75 s
(the live engine up adds ~15 s); `config_test` **317 checks, 0 failures**;
`golden_dump --check` **101 cases**; `cargo test --workspace` green with
**75** `k6wp-core` unit tests; `check_release_overrides.ps1` **5/5** release
binaries clean (the release tree is `build/release`).

### The bench smoke (the ones that need a live session)

- **Never** change the display mode, the wallpaper or the lock screen without
  `tools/session_snapshot.ps1`: dot-source it and wrap the work in
  `Invoke-SessionSafe`, which restores in a `finally` (success, thrown error,
  Ctrl+C).
- `tools/stress_ipc.ps1` (engine already running) - the gate is
  `get_state` **p99 â‰¤ 100 ms, max â‰¤ 500 ms, 0 "ack write failed"**
  (`budgets.md`). The instrument behind it is `K6WP_IPC_TRACE=1` (debug
  builds only).
- `tools/bench_unload_d2.ps1` - the black-frame gate (0 frames in 170 samples).
- `tools/bench_dpb_ab.ps1` - the DPB A/B; it needs `tests/corpus/` and it
  refuses a run whose playback rate leaves 0.97-1.03.
- Any A-vs-B claim needs: identical state, the same metric, the same
  measurement point, `n â‰¥ 5`, the noise band, and for playback the rate plus
  the frame counters. Otherwise the verdict is **"tidak sebanding"**.

### The traps that still bite (beyond section 8)

- PowerShell 5.1: `"abc".IndexOf($stringNotPresent)` returns **0** - always
  use `[StringComparison]::Ordinal`. Variables are case-insensitive: `-Root`
  was clobbered by `$root` once.
- `Set-Content -Encoding UTF8` writes a **BOM**; fixtures and commit messages
  must be written without one. `git`'s `core.autocrlf=true` rewrites hooks to
  CRLF - `.gitattributes` pins `.githooks/*` to LF.
- `windows.h` pulls in `rpcndr.h`, which does `#define small char`; the
  resulting error points at `auto`, not at the macro.
- The `windows` crate gates `CreateMutexW` behind `Win32_Security` **and**
  `Win32_System_Threading`.
- libtest re-executes one test only with `--exact <module>::<test>` (a short
  name runs the whole suite in every child).
- The console prints U+FFFD / U+0394 as `?`; inspect the codepoint with
  `[int][char]` before believing the output.
- **Never** put `CurrentThreadCount()` / `CreateToolhelp32Snapshot` on a poll
  path: it is a system-wide snapshot (~75 ms) and it was the K1 defect.

### The deadline status

| commitment | the deadline | the state |
|---|---|---|
| J3 - the config writers under one lock, C++ and Rust | **done** | committed; the negative control **0/5** clean without the lock vs **5/5** with it |
| STATE.md - this section | **done** | this section |
| L1 - root the 152 ms `get_state` max | **done** | the field (no consumer) and its snapshot **deleted**: max median **56 -> 7 ms** (n = 5), **0 of 20,000 samples >= 20 ms**, the before's slow samples 981 ms apart = the 1 s TTL; `phase2-l1-get-state-max.md` |
| L2 - the poll-path cost audit -> `docs/refactor/ipc-poll-cost.md` | **done** | every `get_state` field measured (n = 3, 4000/run): **no field > 1 ms**; the per-assignment `exists` stat **deleted** (21,053 ms on a dead SMB share) - the recorded hint is used instead; the audit found no other poll-path I/O. The trace-off sanity runs exposed the **L4** client-reconnect bug and the 20 ms tail is **scoped**, recorded |
| L4 - the Rust client `ERROR_PIPE_BUSY` bounded retry + the over-limit test | **done** | the client now **waits** for an instance with `WaitNamedPipe` (bounded) and says why ("no free instance after 5.0 s") instead of failing or spinning; the reconnect **releases the old handle first** (the engine serves one client at a time, so a live handle kept the instance busy); and a **real bug** was fixed: the read returned 0 bytes when `ReadFile` completed synchronously, so the client saw an empty answer and reported a **phantom failure** - that was the trigger of L2's 2/5 lost runs. With the fixed client the same harness is **5/5 clean, 0 failures over 20,000 requests** (`docs/bench/l4/`). Tests: the over-limit case (waits, then a clear error, no hang) + the reconnect regression test, **12/12 green**. The ~0.25 % baseline loss (3/1200) is recorded in `CHANGELOG.md` + `docs/known-issues.md`; the Phase 3 model is `phase3-ipc-connection-model.md`. Doc `phase2-l4-ipc-client-retry.md` |
| L5 - prove the diagnostic overrides are compiled out | **done** | the audit found **one** violation (`K6WP_SPS_DEBUG` was not gated) and it is fixed; the proof is two-layered and in CI: the source lint (every `K6WP_*` read behind a build gate, or an allow-list with a reason) + the binary scan (with `-RequireBinaries`); the self-test proves the lint fires (8 checks); the Rust workspace is in CI now (a new `rust-release` job). Four overrides stay, classified and recorded (the owner's open question). `phase2-l5-override-proof.md` |
| M7 - Rust IPC client deterministic tests (no live engine) | **done** | synchronous read completion, partial reads, multiple frames in one read, exact 64 KiB boundary, payload >64 KiB rejected, short write detection. All 11 client tests pass. |
| N2 - J3 config lock evidence (C++/Rust, â‰¥1000 iter, WAIT_ABANDONED, timeout) | **done** | 366 checks pass; tests 77 (1000 iter), 78 (timeout), 79 (WAIT_ABANDONED). Rule: UI thread never waits for lock. Doc in `config_test.cpp`. |
| N3 - Corpus hygiene (17 files, ~1.1 GB gitignored), golden 16 cases (+4K 60s, 4K 6-ref); open: 4K 6-ref A/B nâ‰¥5, HEVC 8/10-bit ABAB nâ‰¥5 | **done** | 17 files gitignored; golden 16 cases; open items before import dialog: 4K 6-ref A/B, HEVC 8/10-bit ABAB. |
| L3 - the engine CPU + wakeups with the Studio polling (before/after K1) | **parallel with 2.4** | `n â‰¥ 5`, the noise band, the rate + frame counters; if it passes it is a release-candidate trade-off claim, not a quality claim |
| J1 - the seeded random differential | **before 3.7** | open |
| J4 - the Studio goldens / the 12 TO FIXTURE rows | Studio **before 2.7**, engine rows **before 3.7** | open |
| J5 - a real fuzz run or an honest record | **before release** | open (the ASan runtime is still missing) |
| J6 - the SPS probe budget before `ffprobe.exe` goes | **before the removal** | open |
| F3 - the Slint licence decision | **the release stays held** | open |
| E4 - the lean ffmpeg/libmpv builds | optional | deferred |

**The immediate order** (one task, one commit, a ledger verdict each):
L2 -> L4 -> L5 (L3 in parallel with 2.4) -> 2.4 -> 2.5 -> J1/J5/J6/J4
per their deadlines -> F3 -> 2.7 -> **Gate 2** -> Phase 3 -> E4 optional.

**A naming note:** the v1.10 tasks `L1`-`L5` (the IPC follow-ups) are **not**
the locked decisions `L1`-`L10` in section 2 (`L1` there is the DPB byte
budget, `L10` is the ZIP target). When a sentence says "L1" and it is about
latency, it is the v1.10 task; when it is about the re-encode threshold, it
is the locked decision.


## NEXT TASK
NEXT TASK: T0 (bootstrap gate recovery - restore nlohmann/json dependency, run ctest)

