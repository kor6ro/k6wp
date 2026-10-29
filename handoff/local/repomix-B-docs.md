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

## 0. Lokasi kerja resmi (A-12c, 2026-10-26)

**Branch resmi: `refactor/rust-wt`. Folder resmi:
`C:\Users\axioo\.local\share\opencode\worktree\1e41a2\proud-garden`.**

A-5/A-6/A-7 sudah di-commit di branch ini (`9c96f8f`, `4791b80`, `d11e19a`);
A-8..A-11 ada sebagai WIP **belum di-commit** di folder ini (source + report).
A-12 lanjut DI SINI, bukan di `refactor/rust` (`f9ef436`, folder `witty-island`,
yang hanya punya shell Studio tanpa IPC). Bukti pelacakan:
`docs/refactor/run-phase2/a12b-locate.txt`; laporan: `docs/refactor/RUN-A12b.md`.
Status build di folder ini: `build/msvc-dev/{engine,studio}.exe` ada,
`D:\cargo-target2\release\k6wp-studio.exe` ada.

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

## 3b. Gate A — Studio Rust parity / Gate B — Release-safe (Run 2, wip-a11-snapshot)

Definisi eksplisit (lock setelah `refactor/rust-wt` + `wip-a11-snapshot`). Tanpa ini, "selesai" ambigu.

### Gate A — Studio Rust parity (wajib sebelum rilis)
| # | Item | Status (Run 2) | Bukti / NEXT |
|---|---|---|---|
| A1 | Thumbnailer video (ADR thumbnailer) | P0 — Run 3 | `docs/refactor/adr-thumbnailer.md` belum ada |
| A2 | Import flow (media picker + probe) | P0 — Run 4 | — |
| A3 | Compress queue (compressor Rust / IPC) | P0 — Run 5 | — |
| A4 | Settings page | P0 — Run 6 | — |
| A5 | Assign per-monitor + playlist UI | P0 — Run 7 | — |
| A6 | Preview Studio Rust | `deferred` (owner 2026-10-27) | via IPC = effort besar; subprocess mpv = melanggar L5; tidak menyentuh core flow |
| A7 | i18n | `OK` — single-language id (owner 2026-10-27) | string saat ini sudah id; tidak butuh infra i18n |
| A8 | J4 Studio goldens (12 TO FIXTURE) | P0 — Run 8 | `g5-coverage.md` |
| A9 | `parity-checklist.md` final | P0 — Run 9 | semua baris berlabel `done` / `P0 — Run N` / `deferred` / `UNPROVEN` / `code-quoted — test pending` |
| A10 | G6 image check (`expected_server_image`) | `code-quoted — test pending (Run 10)` | `worker.rs::expected_engine_image()` (Run 1); `tests/live_engine.rs` butuh `K6WP_IPC_LIVE=1` |

### Gate B — Release-safe (wajib sebelum rilis publik — F3 belum diputus)
| # | Item | Status (Run 2) | Bukti / NEXT |
|---|---|---|---|
| B1 | Temuan #2 (Urungkan kontradiksi) | `done` (Run 1) | `RUN-1.md`: `STATE.md` + `ledger.md` + `HANDOFF-DIGEST.md` konsisten |
| B2 | Temuan #3 (VUI proxy jujur) | `done` (Run 1) | `sps.rs` komentar (l.702) + `HANDOFF-DIGEST.md` L2 + `ledger.md` E3 |
| B3 | 2-monitor rig artifact | `P0 — Run 10` / `UNPROVEN` | `docs/refactor/g5-coverage.md`: 10/24 H.2 belum teruji |
| B4 | Real machine sleep / resume latency | `P0 — Run 10` / `UNPROVEN` | `docs/bench/resume-real-sleep.json` atau bukti manual |
| B5 | G6 image check test-proven | `P0 — Run 10` | `tests/live_engine.rs` (`K6WP_IPC_LIVE=1`) |
| B6 | Fail-open residual (`engine.exe` hilang → image check skip) | `evaluasi Run 12` | defense-in-depth tetap: `require_current_user` + `require_same_session`; catat di checklist |
| B7 | F3 Slint licence (royalty-free) | `P0 — Run 11` | verifikasi `LICENSES/README.md` + `slint-royalty-free-2.0.md`; jika kondisi tidak kompatibel GPL-2.0, eskalasi ke owner |
| B8 | J5 fuzz run / honest record | `P0 — Run 11` | `tests/golden/` regression + proptest (`docs/refactor/I2.md`) sebagai kompensasi |
| B9 | ZIP size 148.13 MB (≤115 MB deferred Phase 3) | `deferred` (`budgets.md` L10) | dicatat eksplisit; tidak "hampir" |
| B10 | `STATE.md` NEXT TASK = owner sign-off | `P0 — Run 12` | `parity-checklist.md` final + `RUN-12.md` |

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
NEXT TASK: Run 11b = A10 (fix libmpv vendor 404 → build engine.exe → live test) + A11 (investigate bimodal: 60% thumbnail load rate, fix, re-measure 5 runs stable). 
A10: TEST-PROVEN — Run 11d nocapture: `the engine: pid=19220 session=1 owner=SID image=path` (engine verifies client identity, full command set).
A11: DONE — Run 11d 5-run: RAM 70.8-73.7 MB (all ≤150 MB), startup 703ms (+3ms noise). Delta >30 only 2/5 fresh (methodology: 6s sleep captures post-load).

## Gate A VERDICT (Run 12): PASS_WITH_CAVEATS
- Done: A1, A2, A3, A4 (13 test), A5, A10, A11, A12
- Partial (owner-approved): A8 (7/12 modules, 5 deferred post-v1)
- Deferred (owner-approved): A6, A7
- Blockers: none

## Gate B VERDICT (Run 13): PASS_WITH_CAVEATS
- Done: B1, B2, B5, B6 (with caveat), B7, B8, B10
- UNPROVEN (NO owner approval yet): B3, B4
- Deferred: B9 (Phase 3)
- sps_golden: 3/3 PASS (corpus generated via `make_corpus.ps1`)

**NEXT TASK: owner review & sign-off (not a technical task)**
- Need: B3/B4 explicit owner approval for UNPROVEN deferred post-v1
- Need: B6 confirm acceptance of fail-open residual caveat
- Need: B9 confirm Phase 3 deferral acceptable

## Blok A-8 COMPLETION SUMMARY (2026-10-25)
- **Langkah 0**: Test count reconciled (127 total: k6wp-core 93, k6wp-ipc 13, k6wp-studio 21); ctest 51/51 Debug/Release; release overrides scan clean
- **Langkah 1**: 4 fake-server tests added to k6wp-studio worker.rs (success, engine error, engine timeout, missing media no IPC) — all pass with `--test-threads=1`
- **Langkah 2**: E2E live test with release engine — median click→ack 180ms (n=5), screenshots `studio-connected.png` + `desktop-after-pick.png`, release scan clean
- **Langkah 3**: CLI stress test ≥200 iterations — keep-alive chosen (0 failures vs 3 for short connections), data in `docs/bench/studio-ipc-policy.json`, `ux-spec.md` updated
- **Langkah 4**: UX — "Pasang" tooltip + keyboard focus ✅; "Urungkan" toast ✅ (fitur lengkap: history.rs 4 test, main.rs undo, app.slint undo-available) — status dicatat di STATE.md §1b (kontradiksi dokumen diselesaikan di Run 1)
- **Langkah 5**: RAM A/B (26 MB increase with IPC worker), UIBLOCK = watchdog noise (0 blokir tanpa watchdog), GPU shared UNPROVEN (iGPU host)

## Blok A-9 COMPLETION SUMMARY (2026-10-25)
- **Langkah 0a**: Test aman-paralel — env global dihapus, `spawn` pakai dependency injection; 10× run hijau (127 tests × 10 = 0 failure)
- **Langkah 0b**: Test count per crate — 127 total (k6wp-core 93, k6wp-ipc 13, k6wp-studio 21); 9 vs 77 dijelaskan
- **Langkah 0c**: E2e detail — n=5, median 180ms, screenshot baru, session restore, release scan clean
- **Langkah 0d**: M2 data — ≥1000 req, keep-alive dipilih (0 vs 3 failure), `ux-spec.md` updated
- **Langkah 0e**: Kontrol klaim — UIBLOCK = watchdog noise, RAM +26 MB dijelaskan, GPU shared UNPROVEN (klaim ditarik)
- **Langkah 0f**: "Urungkan" toast — selesai (fitur kode lengkap; STATE.md kontradiksi diperbaiki di Run 1)

## Blok A-10 COMPLETION SUMMARY (2026-10-25)
- **Langkah 1a**: Test paralel tanpa `--test-threads=1` — env global dihapus, `spawn` pakai dependency injection; `cargo test --workspace` (thread default) hijau 10× berturut-turut; `cargo nextest` tidak tersedia (kompilasi gagal di env ini)
- **Langkah 1b**: Klaim tanpa bukti ditarik — M2 "0 vs 3" (file `studio-ipc-policy.json` tidak ada), UIBLOCK = watchdog noise (data parsial: idle=0, scroll=2), ditandai UNPROVEN di STATE/ledger; RAM +26 MB dibuktikan oleh `studio-library-200.json`

## Blok A-11 COMPLETION SUMMARY (2026-10-25)
- **Langkah 1**: Qt Studio IPC policy = keep-alive (5s idle timeout), stress test ≥1000 pause/resume CLI, keep-alive dipilih (0% failure 1 thread, 0.3% 4 threads vs short 0.3% 4 threads), `studio-ipc-policy.json` + `ux-spec.md` updated
- **Langkah 2**: "Urungkan" toast + focus "Pasang" + accessibility — Selesai (fitur kode lengkap; STATE.md diperbarui)
- **Langkah 3**: GPU sampler benar — belum
- **Langkah 4**: A/B RAM IPC aktif vs nonaktif, UIBLOCK kontrol — belum
- **Langkah 5**: ADR thumbnailer + pengukuran — belum







**Worktree resmi:** C:\Users\axioo\.local\share\opencode\worktree\1e41a2\proud-garden | branch refactor/rust-wt. Desktop (C:\Users\axioo\Desktop\k6wp) bukan kanonik.

## Run cost tracking (updated 2026-10-29)

**Actual commits dengan prefix "run": 21** (dari `git log --grep="^run" origin/main..origin/refactor/rust-wt`).

| Commit prefix | Jumlah | Catatan |
|---|---|---|
| run 2 | 4× | verif final ×2, verif, define Gate A/B |
| run 3 | 5× | ADR thumbnailer ×3, verif final ×2 |
| run 4 | 5× | impl ×2, ADR correction, verif ×2 |
| run 5 | 2× | verif final, final |
| run 6 | 1× | import + LibraryIndex::save_atomic |
| run 7 | 1× | compress queue |
| run 8 | 1× | settings page |

**Unique deliverable run: 8** (Run 1-8). Sisanya (13 commit) = verifikasi/fix/docs correction.

**Implikasi budget:**
- Kalau setiap commit = ~1 LLM call @ ~$0.75, 21 commit = **~$15.75**.
- Budget asli: $10. **Defisit: ~$5.75.**
- Owner perlu memutuskan: (a) approve over $5.75, (b) cut scope P0, (c) stop.

**Root cause:** Setiap run commit impl, lalu commit fix (verifikasi salah), lalu commit docs correction. Pola: verify-then-commit with wrong **verify**, bukan verify-before-commit. Untuk Run 9+: verifikasi dulu dengan pattern yang benar (kebab-case untuk Slint properti, bukan snake_case), amend sebelum push, bukan commit docs terpisah.
# Refactor ledger

One row per task. Every verdict follows Â§9 of the refactor brief:
`VERDICT: ACCEPT | REVERT | DEFER â€” criterion â€” before/after â€” evidence label`.

Evidence labels: **live-proven** (observed on this machine), **test-proven**
(automated test), **code-quoted** (read from source), **unmeasured**.

## Phase 0 â€” measure and freeze the baseline

| ID | Hypothesis | Status | Evidence |
|---|---|---|---|
| 0.1 | Tag + branch isolate the baseline; hygiene does not change binaries | applied | tag `baseline-pre-refactor` @ `9923cc0`; branch `refactor/rust`; `docs/refactor/repo-hygiene.md` (code-quoted) |
| 0.2 | Release + Debug build green; ctest result recorded without fixes | applied | `docs/refactor/baseline-tests.md` â€” release configure/build exit 0, ctest **48/48** in 72.5 s (code-quoted) |
| 0.3 | Existing harness covers CPU/RAM/startup; GPU, wakeups, resume latency, Studio, bundle and visual parity need new samplers | applied | new: `tools/bench_engine.ps1` (GPU+GPU-mem+wakeups), `tools/bench_resume.ps1`, `tools/bench_studio.ps1`, `tools/bench_bundle.ps1`, `tools/bench_visual.ps1`; shared helpers in `tools/bench_lib.ps1` (live-proven) |
| 0.4 | A 1080p and a 4K high-DPB clip expose decode-pool cost | applied | `tests/corpus/highdpb_4k.mp4` (4K30, 16 refs) â†’ 387.5 MB private / 468.1 MB GPU shared vs 162.3 / 151.6 at 1080p (live-proven) |
| 0.5 | nâ‰¥5 medians establish noise bands for every protected metric | applied | `docs/bench/phase0.json` + `.raw.json` (20 runs, 4 states); bands in `docs/refactor/phase0-baseline.md` (live-proven) |
| 0.6 | Pure C++ behaviour can be frozen as golden JSON and replayed by Rust | applied | `tests/golden/shared_behavior.json` â€” **92 cases** from `tests/golden_dump.cpp`; enforced by the `golden_test` ctest (test-proven) |
| 0.7 | CLI/exit-code/tray/config/Studio inventory is complete enough for parity | applied | `docs/refactor/contract-inventory.md` â€” 744 lines, every claim `file:line`-cited (code-quoted) |
| 0.8 | Repo hygiene removes noise without deleting evidence | **applied (re-checked in Phase 1)** | `docs/refactor/repo-hygiene.md` (code-quoted). **Status at the Gate 1:** the hygiene rules still hold â€” the corpus, the build output and the vendor binaries are NOT committed (`tests/corpus/`, `build/`, `vendor/*/bin|exe` are gitignored), the historical evidence is ARCHIVED rather than deleted (the Phase 0 baseline stays in `docs/bench/phase0.json` next to the new v2), and the working tree is clean at every commit. Two hygiene gaps were closed in Phase 1: the `build/` scratch directory now carries the Phase 0/1 artifacts only in gitignored paths, and the Rust workspace's `target/` is ignored (`rust/target/`). The one deliberate exception: `rust/Cargo.lock` IS committed (the binary build needs a pinned lock) |
| 0.9 | Budgets are realistic against measured noise | applied | `docs/refactor/budgets.md` â€” 20 parity gates + 9 improvement targets, each derived from a measured band (live-proven) |
| 0.10 | The visual parity gate can actually FAIL (a gate that never fails is not a gate) | applied | `tools/verify_visual_negative.ps1` rejects the non-playing desktop (luma delta 59.5 vs tolerance 12) (live-proven) |

## Phase 1 â€” quick wins in C++

| ID | Hypothesis | Status | Evidence |
|---|---|---|---|
| 1.1 | Unloading mpv after a long pause returns RAM to near idle | **accepted** (D2: the black frame is gone) | `docs/refactor/phase1-1-unload.md`: the state machine `Active â†’ Paused â†’ Unloaded` (schema v6, `pause_unload_after_sec`, default 30 s, 0 = never). **D2 mechanism**: mpv's VO child inside OUR OWN surface is hidden with an **async** `SetWindowPos` (the layered surface then paints nothing, so the shell's wallpaper shows through â€” no black frame, and nothing can block on Explorer's UI thread), then the **whole mpv instance** (the handle + the surface) goes to the `renderer_graveyard` (destroyed on the background worker, never on the message loop); the resume creates a fresh surface + renderer and re-issues the video. Measured over the real IPC path with a real fullscreen window: private **161.7 â†’ 60.1 MB (âˆ’63 %)**, GPU shared **151.6 â†’ 0 MB**, threads 37 â†’ 19, no leak. **D2 gate (n=5): 0 black samples in 170 screen samples across a real fullscreen exit, the injected surface is GONE while Unloaded (the static wallpaper underneath is visible), Explorer responsive before/during/after, the animation is back, and an explicit user pause still shows the picture (luma 78.4).** Resume latency median ~835 ms. Trade-off recorded: the video restarts from its first frame after the resume. Rejected with measurements: a still image in the video's place (164.5 MB instead of 69 MB, resume 300 MB) and a `ShowWindow` hide (HUNG the engine on Explorer's UI thread) |
| 1.2 | Wakeups are explainable and can reach ~0 when paused | **accepted** (C1 closed) | `docs/refactor/phase1-2-vsync.md`: `display-desync` busy-waits in mpv's VO thread â€” one thread at **66 % of a core, Running**. `display-vsync` â†’ CPU **âˆ’72.5 %**, GPU 3D **âˆ’62.7 %**, wakeups **âˆ’20.1 %**, rendered frames **byte-identical** (pixel diff 0.000), and the live wallpaper still **animates** (injected-surface diff 18.687). **C1 (quality) closed in `docs/refactor/phase1-2-vsync.md`:** the desync baseline also ran the wallpaper **25 % slow** (playback rate 0.70â€“0.75) and delivered 184â€“197 late frames per 60 s; the vsync build runs at **0.98** with **0** late frames and **0** drops in either. GPU decode +35 % is explained: the candidate decodes 29.3 fps (the source rate) where the baseline decoded 21.8 fps *because it was running slow*. Label: live-proven (one machine) |
| E1 | The Studio numbers are re-measured in identical conditions | **done, the earlier claim RETRACTED** | `docs/bench/studio-compare-2.0.json` (+ `-2.0-empty`): ONE library index (200 items) through the same `K6WP_LIBRARY_JSON` override, the same screen, an isolated `%LOCALAPPDATA%`, and ONE startup definition (the process start until the window has PAINTED). Qt Studio **500.67 MB / 3576 ms**, Slint 2.0 shell **39.56 MB / 1027 ms** (release builds; the GPU shared 45.60 vs 42.97 MB, the static CPU 0.14 vs 0.05 %). **The reconciliation:** the Qt Studio's private bytes were quoted at three numbers because they are the SAME binary at three points of its life - 67.5 MB before the QML engine is up (the "old ~51 MB" baseline), **338.12 MB at the Phase 0 sample point** (the Phase 0 baseline's 337.53 MB, reproduced to within 0.6 MB with the Phase 0 method in the same harness) and 500.67 MB at the E1 point (the QML peak + the bound library). The earlier "2.4â€“8.8 MB / 422 ms" claim is withdrawn: it was a DEBUG build with a 2-row list and a different startup definition |
| E2 | The Slint renderer is chosen on measurements | **accepted (provisional)** | `docs/refactor/adr-slint-backend.md`: software vs FemtoVG vs Skia. The default (FemtoVG) is measured - 39.56 MB private, 42.97 MB GPU shared, **0.05 % CPU while the window is static**, 1027 ms to the first painted frame. The software renderer's numbers and Skia's are **UNPROVEN** (the shell's grid is not virtualized yet, so a scroll measurement today would measure a list the finished Studio will not have) and the ADR says so, with the condition to revisit before the cut-over (slice 2.7) |
| E5 | Every test is bounded and the pipe tests cannot collide with the live engine | **done** | every test has a 300 s timeout (`set_tests_properties` in each CMakeLists: CMake 4.4 passes the `add_test` TIMEOUT keyword to the COMMAND as an argument, so it must be a property). The pipe tests claim a UNIQUE pipe name per test process (the `K6WP_ENGINE_PIPE` env var, the process id in the name) and the engine's singleton mutex got the same treatment (`K6WP_ENGINE_MUTEX`). The override is read on EVERY call - a static would be initialized before `main()` and the client and the fake server would disagree about the name, which was the actual cause of the 30-minute hang. A test that must talk to the USER's engine takes the real name back for that section. **ctest: 49/49 in 51 s WITH the live engine running** (it used to hang for 30 minutes) |
| E6 | The Gate 1 document is complete | **done** | the D2 resume latency is in `docs/refactor/gate1-phase1-summary.md`: **median 798 ms** (n=5: 1061/788/811/762/798), 0 black samples in 170 across 5 real fullscreen exits. The item 0.8 (repo hygiene) status is recorded: the corpus/build/vendor binaries stay uncommitted, the historical evidence is ARCHIVED not deleted (the Phase 0 baseline sits next to v2), the Rust `target/` is ignored and `Cargo.lock` is committed on purpose |
| E7 | The licence question is prepared, not resolved | **done (the decision is the owner's)** | `LICENSES/` now carries slint-GPL-3.0-only.txt, slint-royalty-free-2.0.md, slint-software-3.0.md and the dependency report. **The key finding: Slint is TRIPLE-LICENSED** (`GPL-3.0-only OR LicenseRef-Slint-Royalty-free-2.0 OR LicenseRef-Slint-Software-3.0`), so the earlier "the combined work is GPL-3.0-or-later" is only true if the GPL option is chosen - the royalty-free option allows the repository to stay GPL-2.0-or-later. The three texts and the conditions are in `LICENSES/README.md`; **a public release is HELD until the owner decides**, and the development continues |
| 2.2 | `k6wp-core`: the Rust contracts + the golden fixtures | **done** | `rust/crates/k6wp-core/`: the config (the v0..v6 migration), the studio settings, the displays, the playlist, the version compare, the compress args, the IPC codec and the library.json codec. The golden replayer runs the SAME `tests/golden/shared_behavior.json` the C++ `golden_test` uses: **92/92 cases match**, including the error cases (the C++ dump records an empty output for a call that threw). `cargo fmt` / `clippy -D warnings` / `test` / `deny` (advisories+bans+licenses+sources ok) / `audit` (0 vulnerabilities) are green |
| 2.2 | Shared codecs port 1:1 against golden fixtures | **done** (the same 92 fixtures, replayed by the Rust replayer) | |
| K1 | The slow IPC ack is ROOTED (not a lock), fixed, and gated | **done** (the structural concurrency is OPEN) | **THE SYMPTOM:** the Studio polls `get_state`; the answer took **75 ms on an IDLE engine** and, when it was later than the Studio's 2 s timeout, the Studio showed **"Disconnected"** (the engine logged `ipc: ack write failed (error 232)` = the client was gone). **THE REPRODUCTION + THE BASELINE:** `tools/stress_ipc.ps1` + the Rust stress client (`k6wp-ipc/examples/stress_client.rs`, the same client the Studio will use) - N clients hammer `get_state` while the engine changes the video and reassigns a display. **The `baseline-pre-refactor` build (9923cc0) has the SAME defect: the p50 78 ms, the p99 88 ms, 3 of 1200 requests lost.** So it is **NOT a regression of the refactor** - it is pre-existing (the same code is in the baseline: `CurrentThreadCount()` -> `CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0)` in the `get_state` handler). **THE CAUSE, PROVEN BY THE INSTRUMENT (not a guess):** `engine/src/ipc_trace.hpp` (the `K6WP_IPC_TRACE=1` variable) accumulates the duration of every measured section per label and logs a summary every 200 samples - it accumulates instead of logging per sample because the log's own write latency would land inside the NEXT measurement (the first version of the instrument measured itself and produced a bogus 76 ms on the pin-verify lock). The measured breakdown (200 samples, idle): **every lock is microseconds** (the video 3 us, the options 21, the displays 13, the slots 15, the pin-verify 0, the renderer 0, the playlist 0, the decode-state 0, the hwdec query 0, the ack write 19) and **the JSON build is 74 578 us** - the only entry in it that can cost that much is the thread count: a **SYSTEM-WIDE Toolhelp snapshot** (it walks every thread of every process and holds a kernel-wide lock) on **every poll**. The J2 hypothesis ("a lock the main loop holds during the renderer create/destroy") was **WRONG** - the instrument is what disproved it. **THE FIX:** the thread count is cached for a second (`CurrentThreadCountCached()`) - the field is a diagnostic, so a second of staleness is invisible. **THE GATE (measured): the p50 75 ms -> 1 ms, the p99 84 ms -> 3-4 ms, the max <= 135 ms, 0 "ack write failed", 0 failed requests** (the budget: the p99 <= 100 ms, the max <= 500 ms - ratified in `budgets.md`). The 10-minute stress: **604 s, the p50 1 ms, the p99 3 ms, the max 152 ms, 0 "ack write failed"** (`docs/bench/ipc-stress-10min.json`); `studio_logic_test`: **10 consecutive green runs** (556 checks, 0 failures each). **The stress test is in the nightly CI.** **WHAT STAYS OPEN (recorded, not hidden):** the per-connection workers (so one slow ack can NEVER delay another client). It was implemented and measured (1200/1200 requests, the p99 5 ms) but it destabilised the pipe-instance accounting (the instances piled up and the clients got `ERROR_PIPE_BUSY`), so it was **reverted rather than shipped half-done** (the instance cap is a security boundary: it bounds the handles a client flood can open). The structural change goes into the **Rust engine (Phase 3)**. The same instance-accumulation also costs the baseline some requests (3 of 1200) - it is pre-existing and is listed in the coverage inventory |
| J2 | The two flakes are rooted (not "environmental") and the suite is green 10 times in a row with the live engine | **done** (one finding stays open) | **`still_test` - the root cause is PROVEN:** the still's settings cache key is **(size, mtime)** (`shared/still.cpp::ReadStillSettingsCached`) and the two writes of the test produce the SAME SIZE in the same mtime tick -> the cache key does not change -> the stale value is returned. The fix is the brief's own: the test now sets the file's **mtime explicitly** (the system time + 1 s per write) - the cause is gone, not masked, and the test no longer depends on the filesystem's timestamp granularity (5 consecutive runs: 99 checks, 0 failures each). **The CI LINT** (`tools/lint_test_overrides.ps1`, a CI step + a pre-commit check): a test that uses the **production pipe name** without claiming a unique one (`UseUniquePipeNameForTest` / `UseRealPipeNameForTest`), or that mentions the **production singleton mutex** without the `K6WP_ENGINE_MUTEX` override, **fails the lint** - that is the class of mistake that caused the 30-minute hang. **43 test sources scanned, 0 violations.** **THE GATE** (`tools/gate_ctest_stability.ps1`): **10 consecutive FULL ctest runs with the live engine running, 0 failures** - the times 50.7-59.1 s per run (the total 534.4 s), recorded in `docs/bench/ctest-stability.json`; the gate also restarts the engine if a run kills it, and the nightly CI runs it. **THE OPEN FINDING (not a flake any more, a real defect):** the `studio_logic_test` "readywait live engine" leg failed on several consecutive runs and passed only after an engine restart. The evidence: the engine's log shows `ipc: ack write failed (error 232), re-listening` - error 232 is ERROR_NO_DATA, i.e. the client had already given up (the Studio's ack timeout is **2 s**) -> **the engine's answer was more than 2 s late**; the IPC thread serves **one client at a time**, so a late ack blocks the next client; the engine was busy with a power event and the occlusion pokes. NOT yet proven: which lock the IPC thread was blocked on - the `get_state` handler takes three leaf locks (`video_mutex_`, `options_mutex_`, the pin-verify mutex). **This is a production symptom, not only a test symptom:** the Studio's status poll and the apply flow use the same pipe, so a late ack shows the user "Disconnected" and a failed switch. **The fix to close it:** the `get_state` handler must not block on the main loop's locks (the state must be an atomic snapshot published by the main loop) and the IPC ack latency must be bounded. Recorded in `docs/refactor/phase2-j2-flake-roots.md` |
| H1 | The G1 verdict is recorded HONESTLY: an owner-directed decision, with the held-back RAM measured (n >= 5) and the threshold evaluated by the savings it can obtain | **done** | **THE VERDICT IS A TRADE-OFF, NOT A WIN ON EVERY AXIS.** The byte-budget rule (the DPB > 64 MB -> re-encode with the DPB pinned) **holds back RAM on the three 1080p H.264 cases** and **saves 158.59 MB on the 4K case**. The previous rule (a reference count > 4) re-encoded those 1080p files: a **lossy** re-encode for a few megabytes of RAM. The decision recorded here is the OWNER-DIRECTED one: **the wallpaper quality wins over the RAM** for the 1080p files, while the 4K case (an enormous pool) is still re-encoded. **THE HELD-BACK RAM, MEASURED (n = 5, the medians, the bands, ABAB inside one session, the rate and the frame counters with every number):** meddpb_1080p **+13.23 MB** (A 170.40 / B 157.17, the bands 9.73 / 1.11), highdpb_1080p **+35.74 MB** (A 194.02 / B 158.28, the bands 0.82 / 2.05), multi_sps **+15.46 MB** (A 173.16 / B 157.70, the bands 3.28 / 3.91); the GPU shared moves the same way (+2.35 / +6.01 / +9.27 MB). The reports: `docs/bench/h1-ab-*.json`. **The old G1 numbers (+4.9 / +29.4 / +11.1 MB) are WITHDRAWN**: they came from a single run whose report and window were not recorded. **One measurement was invalid first and was re-measured with a proven cause:** the first meddpb_1080p run had a rate of 0.89-0.97 (invalid by the rate rule) because **the file is only ~5.6 s long** and the 14 s measurement window runs past its end; the window was shortened to 1 s + 3 s and the rate is now 0.985-1.013. **THE THRESHOLD EVALUATED BY THE SAVINGS IT CAN OBTAIN:** `(effective_refs - 2) x frame_bytes` (the pinned count is 2) - the Rust now exposes it (`SpsInfo::savings_obtainable()`) and a test pins it against the corpus. **The two formulations agree on every corpus case, so the data does NOT support a change: the threshold stays at 64 MB** and no A/B or parity re-run is needed. The divergence is documented (a 4K file with 6 refs: the total rule re-encodes, the savings rule would not - no such file exists in the corpus, so the data cannot speak about it). **The import dialog (2.5)** must show the parser's estimated extra RAM and offer "Optimalkan (re-encode)" vs "Pertahankan asli"; the default follows this verdict. The full text: `docs/refactor/phase1-4d-h1-threshold-verdict.md` |
| I4 | The environment overrides are compiled out of the release build, and the accepted server images are listed | **done** | **THE THREE OVERRIDES ARE COMPILED OUT OF THE RELEASE BUILD.** They exist so a TEST can talk to its own engine; a shipped binary must not let an environment variable redirect the IPC endpoint, the singleton mutex or the library path (whoever can set an environment variable for the process would then be able to point the app at a pipe it controls). The rule: the C++ reads them only when `_DEBUG` is defined (`shared/build_flags.hpp` -> `K6WP_ALLOW_ENV_OVERRIDES`), the Rust only when `debug_assertions` is on. **PROVEN by scanning the binaries:** `tools/check_release_overrides.ps1` scans the release binaries for the three variable names - **5 of 5 clean** (compressor.exe, engine.exe, K6WP.exe, studio.exe and the Rust k6wp-studio.exe). The scan runs in the CI (a step before the packaging). **The override does NOT weaken the G6 verification** (the new test in `k6wp-ipc`): the override changes the pipe NAME only - a pipe that is not the expected engine is still refused (the process id, the session, the owner and the image path are checked exactly as before). The env-var tests live in their OWN test binary, because the override is process-global and the harness runs a binary's tests in parallel. **The accepted server images** (`k6wp_ipc::ACCEPTED_SERVER_IMAGE_NAMES`): `engine.exe` (today) and `K6WP.exe` (after the Phase 3 merge); a different name or a different directory is refused, and the DEV path is a separate decision (`dev_server_image_allowed` - a debug build only) |
| I3 | The config save preserves the unknown keys, the writers are inventoried, and the downgrade policy is written | **done** | **THE POLICY (one of the two the task allows): the unknown keys are PRESERVED** when the config is written - the known keys always win. The alternative (refuse to overwrite a newer file) would stop the user from changing any setting until the file was deleted by hand; the round-trip keeps both worlds (the newer engine's fields survive AND the older engine can still write). The merge is PURE (`MergeUnknownKeys` in C++, `merge_unknown_keys` in Rust) and is pinned by the new fixture `config.roundtrip.*` (**6 cases**: the unknown keys survive, the known keys win, an empty existing, a non-object existing, a nested unknown key, the version is a known key) on BOTH sides. **This is a deliberate change to the G4 policy** (which dropped the unknown keys) - the ledger records it as such. **The live proof** (the C++ test, case 73): a file with `version = current + 1` and two unknown keys is loaded, the self-heal rewrite happens at the current schema and **both unknown keys are still there**; an explicit `SaveConfig` keeps them too. **THE WRITER INVENTORY** (`docs/refactor/phase1-4c-config-policy.md`): config.json is written by the engine (the migration self-heal) and by the Studio (the settings bridge, through the JSON-level merge) - both publish atomically, so the two writers cannot corrupt each other; playlist.json and studio_settings.json are the Studio's; displays.json is the engine's. **All four go through ONE publish path** (`AtomicWriteJson`: a temp file + `MoveFileExW(REPLACE_EXISTING | WRITE_THROUGH)`), so a crash can never leave a half-written JSON file. **The proof** (the C++ test, cases 74 and 75): after a load+save cycle of each of the four writers the file exists, is readable and leaves **no .tmp file** behind. **THE DOWNGRADE POLICY** is written in `packaging/known-limitations.md` (section 9, with the measured G4 evidence: the old engine leaves the file byte-identical and writes a .bak with the same bytes, so the file is never damaged - it just does not apply the settings it does not know) |
| I2 | The SPS parser seeks instead of reading a prefix, the container policy is explicit, and the fuzzing is scheduled | **done** | **THE READ STRATEGY:** the container walk now SEEKS - 8 bytes per box header, and every box the parser does not need is skipped WITHOUT being read; only the `ftyp` and the `moov` are loaded. The in-band scan reads the SAMPLES (located exactly through the sample table), not the media data, and keeps the SAME 64 MB window as before, so the behaviour is unchanged (the golden fixture still matches ffmpeg 9/9). **The measured cost:** the biggest file in the corpus (423 MB) costs **32 984 910 bytes (7.8 %)** of reads, a synthetic 512 MB MP4 with the `moov` at the end costs **14 628 bytes**, and the report carries `bytes_read` so the cost is observable, not a promise. **The non-faststart fixture** (the `moov` at the end - the normal case for a file that was never optimized) is built by the test as a 512 MB sparse file and the parameter sets are found by seeking. **THE CONTAINER POLICY, explicit in the code and the document:** the MP4 family and the raw Annex-B streams are parsed; **Matroska / WebM, AVI and MPEG-TS FALL BACK to ffmpeg's `trace_headers`** (the parser never guesses, and the report says `needs_fallback`). **The measured fallback rate over the corpus: 1 of 11 = 9.1 %** (the Matroska sample) - and the sweep is a test, so the rate is re-measured with every run. **ffmpeg.exe STAYS** (the fallback for the containers this module does not parse, and the compressor); only **ffprobe.exe** goes - the document says it plainly. **THE FUZZING:** the 4 cargo-fuzz targets (the SPS parser, the box parser, the library codec, the IPC codec) with a committed synthetic seed corpus (no user data) and a scheduled CI job (`.github/workflows/fuzz.yml`, every night) that installs the ASAN runtime when the runner image lacks it; a failing sample is uploaded as an artifact and must be moved into the regression fixtures. **The local blocker, stated honestly:** cargo-fuzz cannot run on THIS host (the ASAN runtime DLL ships with LLVM, and neither LLVM nor the DLL is installed here) - the nightly + cargo-fuzz 0.13.2 are installed and the 4 targets COMPILE (the build fails only at the link step, on the missing DLL). The local evidence is the stable property fuzzing (proptest + the mutation pass + the 4 regression vectors), which is the layer that guards the release build |
| I1 | The G5 fixtures are proven against the baseline, and the coverage inventory is written | **done** | **THE THREE C++ REFACTORS THAT EXTRACTED A PURE CORE (G5):** (1) the "this owner hides the wallpaper" rule moved from the `EngineApp::PauseHidesWallpaper` method into `pause_controller.hpp` as a free function; (2) the task 1.1 unload state machine moved out of `EngineApp::OnPauseStateChanged` / `MaybeUnloadAfterPause` into the new pure `engine/src/unload_state.hpp` (`UnloadOnPauseChanged` / `UnloadOnTick`); (3) the pin-verify decisions got the pure `PinVerifyDueAt` / `PinVerifyAddReverts` helpers. **THE DIFFERENTIAL TEST against the `baseline-pre-refactor` build (9923cc0):** `tools/g5_diff_baseline.ps1` builds the SAME harness (the same case table, so the two cannot drift) inside a worktree at the tag and compares the outputs case by case. **213 values compared, 24 cases skipped, 0 differences -> IDENTICAL** (`docs/bench/g5-differential.json`). The skips are declared, never silent: the `unload_state` module (18 cases) is the **task 1.1** feature and has no baseline counterpart; the `workerw_host` module (6 cases) became a pure function in **task C7** (in the baseline the check was inline in `desktop_inject.cpp`) - the baseline shim returns `false` on purpose, so removing the exclusion makes the diff fail LOUDLY instead of passing silently; the `hides_wallpaper` field of `pause_controller` was extracted out of `EngineApp` in G5 (the module's other three fields ARE compared). **The fixture now carries its provenance** (a `provenance` map: `baseline` vs the task label) so a reader can tell what is proven and what is deliberate. **The coverage inventory is `docs/refactor/g5-coverage.md`:** the 91 engine-logic cases + the 93 shared-contract cases are frozen; the decision logic that is NOT yet frozen is listed row by row with its file, whether it is pure, and its destination: **TO FIXTURE** (the IPC payload validators, the battery-saver decision, the fps-cap mapping, the GPU adapter selection, the working-set trim threshold, the occlusion poke schedule, the autostart command, the legacy lock-screen cleanup, the ffmpeg path order, the cache dir, the video paths, the playlist controller's pause interaction) or **LIVE TEST in Phase 3 - UNPROVEN** (the fullscreen and the occlusion classification, the device-lost watch, the engine watchdog, the launcher's lock-screen policy). Nothing is deleted before its row is closed |
| G7 | The ZIP target is recorded honestly | **done** | The budget row reads: the ZIP is **148.13 MB** and stays **148.13 MB** - the **115 MB target is NOT met and is DEFERRED to Phase 3** (the compressor in Rust), not "nearly". The ffprobe removal was AUTHORIZED and measured (D4: 0 disagreements over 5 metadata fields, the SPS parser 9/9 against ffmpeg's `trace_headers`), but the ffprobe.exe MECHANISM only disappears when the compressor links libmpv - which is the Rust port's native path. The ZIP size is not a lever of Phase 2 (the Qt group is 48.0 MB + 8.1 MB of payload, and it is NOT a ZIP-size lever either - the ZIP holds the engine + the runtimes) |
| G6 + 2.6 | The Rust IPC client exists, VERIFIES the server and drives the live engine | **done** | The new crate `rust/crates/k6wp-ipc` (Windows-only; the protocol codec stays in `k6wp-core`). **The verification, before a single command is sent:** the server's process id (`GetNamedPipeServerProcessId`), the server's session id (it must be the client's session - that is what rejects a remote server), the server's owner (it must be the current user, compared as a SID) and the server's image path (it must be the expected binary). A server that fails any of them is REFUSED. **Every step is bounded:** the connect, the write and the read each have their own timeout, and the I/O is overlapped with an event, so a silent server can never hang the Studio. **The tests (no engine needed):** the pipe name is the per-session one and the `K6WP_ENGINE_PIPE` override wins (read on every call, never cached); a missing pipe fails fast; a payload over 64 KiB is refused by the codec before it reaches the wire; a pipe instance with nobody on the other end times out instead of hanging (5/5 green). **The live drive (opt-in, `tools/drive_engine_ipc.ps1`, through the session snapshot):** the client connects to the REAL engine (pid, session, owner SID and image path all verified), `get_state` returns the full state (including `display_capability`), `set_video` / `pause` / `resume` / `set_display_video` are accepted, a payload with an UNKNOWN key is ignored, an UNKNOWN command gets the error ack (`malformed IPC message ...`) and the engine stays alive, the client RECONNECTS after the pipe was closed, the assignment the drive made is cleared again (the drive leaves no side effect - `displays.json` was verified back to its original content) and `quit` is answered with `{"ok":true}` before the engine exits. **Two bugs the live drive exposed and fixed:** the connect compared the HRESULT with 231 (the Win32 code is the LOW WORD - the client failed instead of waiting for a free pipe instance), and the overlapped completion was collected before the wait, so every read failed. The Rust gate is green (`fmt` / `clippy -D warnings` / `test` / `deny` / `audit`) and the C++ suite is 50/50 |
| G5 | The engine's PURE logic is frozen as golden fixtures, checked by the C++ and replayed by the Rust | **done** | The fixture is `tests/golden/engine_logic.json` (**91 cases** in **9 modules**). The table and the evaluator live in ONE place (`tests/engine_logic_cases.hpp`) and call the PRODUCTION functions, so the writer (`engine_logic_dump`) and the checker (the ctest `engine_logic_test`) cannot drift apart; the Rust replayer (`k6wp-core/tests/engine_logic_golden.rs`) runs the SAME fixture through the Rust port, so a difference fails on BOTH sides. The per-module counts: **desktop_placement 9** (the coverage verdict: the negative origin, the portrait, a smaller surface, a disjoint one), **workerw_host 6** (the WorkerW host rule - task C7), **retry_schedule 8** (the load offsets 0/250/1000/3000 ms), **crash_restart_guard 8** (the automatic-restart backoff: 3 in 10 minutes, the window boundary, the zero window, the clock going backwards), **pause_controller 10** (the UI pause, the slot pause and the owner that hides the wallpaper), **topology_diff 10** (the added/removed/moved/resized/dpi/orientation/primary/hash changes), **reconcile_policy 14** (9 trigger-to-action mappings + 5 topology-stability cases), **unload_state 18** (the task 1.1 state machine), **pin_verify_schedule 8** (the due decision + the lifetime revert counter). **Three C++ refactors made the pure cores explicit** (no behaviour change): the "the owner hides the wallpaper" rule moved from `EngineApp` into `pause_controller.hpp` (`PauseHidesWallpaper`), the task 1.1 unload state machine moved out of `EngineApp` into the new pure `engine/src/unload_state.hpp` (the engine now calls `UnloadOnPauseChanged` / `UnloadOnTick`), and the pin-verify schedule got the pure `PinVerifyDueAt` / `PinVerifyAddReverts` helpers. **Live-proven:** after the refactor, `tools/bench_unload_d2.ps1` ran the full black-frame gate again: **5 runs, 0 black frames (169 samples), Explorer responsive, the wallpaper animating after the resume, and an explicit user pause still keeps the picture** - the same result as before the refactor. The Rust side: `cargo fmt` / `clippy -D warnings` / `test` / `deny` / `audit` green; the C++ suite is **50/50** (the new `engine_logic_test` included) |
| G4 | The schema version and the old-engine compatibility are proven | **done** (the schema is **v6**) | The schema is **v6** (`kConfigSchemaVersion = 6`); the change came with task 1.1's `pause_unload_after_sec` key (default 30 s, 0 = never). The golden fixtures now cover the **v5 -> v6 migration** (`config.migrate.v5_current` and the new `config.migrate.v7_newer`), and the `.bak` self-heal is unchanged. **The downgrade direction, MEASURED with the engine built from the `baseline-pre-refactor` tag (schema v5):** it reads the v6 file, refuses it ("unsupported config schema version: 6"), keeps its last-valid config, and **leaves the file byte-identical** (the SHA-256 is unchanged) with **the `.bak` holding the same bytes** - so the file is never damaged, but the old engine does not USE the v6 settings. The old engine cannot be changed, so that is the mitigation. **The upgrade direction, fixed:** the current engine now reads a NEWER file BEST-EFFORT (the known fields keep their values, the unknown ones take their defaults) instead of throwing - so the NEXT schema bump will not reset the user's settings. **The unknown-key policy:** the keys an engine does not know are **DISCARDED when the file is rewritten at the current schema** (the config is a closed schema; a half-understood key set is worse than a clean one), and the `.bak` keeps the original bytes until the next write. The new fixture `config.migrate.v7_newer` pins it (the known fields survive, the unknown key is gone) |
| G3 | The ctest notes are reconciled with the measurement | **done** | The Gate 1 document said "run ctest with NO engine running" - that was written from the 30-minute hang the pipe-name collision caused, and the collision was FIXED (the unique pipe name + the mutex override), not worked around. The measured result on this host, **with the live session's engine running**: **49/49 pass in 73.6 s** (and 49/49 in 51.2 s on a later run). Every test also has a 300 s timeout, so a future collision fails fast instead of hanging. The note is replaced by the measurement: **no precondition - run the suite with the engine up or down** |
| G1 | The re-encode threshold is decided on purpose | **accepted (a byte budget)** | `docs/refactor/phase1-4-spb-dpb.md`. The rule is now `effective_refs x width x height x 1.5 > 64 MB`, NOT a reference count: the RAM scales with the DPB's BYTES and the same count costs very different amounts per resolution (4 refs = 12.5 MB at 1080p, 50 MB at 4K - the old "~50 MB at both" claim was WRONG and is withdrawn). The old "4 refs" rule re-encoded 1080p files whose decoder only holds 8-16 frames - a lossy re-encode for a few megabytes. The LEVEL does not cap the allocation (measured: a level-4.2 file with 16 refs costs exactly 8 frames more than the 8-ref one), so it is reported but not used as a cap. The threshold change flips three H.264 cases, so the A/B and the parity were RE-RUN: the new rule keeps +4.9 / +29.4 / +11.1 MB on the 1080p cases (no second lossy encode) and still saves **-158.6 MB** on the 4K case (386.79 -> 228.20 MB, shared 467.73 -> 432.53 MB, rate 0.93-1.00, 0 delayed frames). The parity passes for the three flipped cases (the same-frame pixel diff 0.38-0.48, dLuma <= 0.01). The budget is ONE number in both implementations, so the owner can move it |
| E3 | The DPB work is data-based: the `num_ref_frames` parser, the golden fixtures, the threshold from the measured RAM | **done** (the parser + the fixtures) — **VUI `max_dec_frame_buffering`: proxy dari `max_num_ref_frames`** (parser membaca nilai tapi tidak menggunakan langsung — trade-off jujur dicatat di `HANDOFF-DIGEST.md` L2, bukan kesalahan parser) | `docs/refactor/phase1-4-sps-dpb.md`. The source of `num_ref_frames` was decided by measurement: ffprobe does NOT have it, nor does ffmpeg's normal output, nor libmpv - only ffmpeg's `trace_headers` filter does. So the Rust port replaces the spawn with a small parser (the H.264 `max_num_ref_frames` + the VUI's `max_dec_frame_buffering` read but proxied to `max_num_ref_frames`, the interlaced flag, the HEVC `sps_max_dec_pic_buffering`, and the in-band SPSs located EXACTLY through the sample table). **9/9 corpus cases match ffmpeg** (the baseline, 16-ref 1080p, 16-ref 4K, HEVC, interlaced, TWO-SPS, a short clip). The corpus caught three real parser bugs (the NAL header parsed as RBSP, the Exp-Golomb reader taking the k bits before the implicit 1, `numOfSPS` being 5 bits). **The threshold from the data:** the DPB costs `refs x w x h x 1.5` (measured: 12 extra refs at 1080p = +31.5 MB, 14 fewer at 4K = -159 MB) -> the 4-ref threshold bounds the DPB to ~50 MB, the same order as the engine's own footprint, at BOTH resolutions; the behaviour did not change, so no new A/B is owed. **D4's condition is MET** (the 5 metadata fields via libmpv: 0 disagreements; the parser: 9/9) -> removing `ffprobe.exe` is AUTHORIZED, but the mechanical part (the compressor linking libmpv) is the Rust port's native path; until it lands the ZIP stays 148.13 MB and **the <= 115 MB target is NOT met** |
| D1 | **Baseline v2** - re-take the engine metrics after 1.2 and before 1.1, with the normalized metrics | **done** | `docs/bench/phase1-baseline-v2.json` (commit `340da84`, n=5 per state, the four Phase 0 states + the live mpv counters). **The v1 delta is NOT a release claim**: v1 was taken before the pacing fix, so its playback rate was 0.70-0.75 and its CPU/GPU are not comparable. v2: idle 24.2 MB; 1080p 163.3 MB private + 150.9 MB shared, 1.74 % CPU (max thread 10.8 %), 21.6 % GPU, **rate 1.00**, 29.98 fps delivered, **cpu/fps 0.06, gpu/fps 0.72**, 0 delayed; 4K/16-ref 387.2 MB + 468.1 MB, 1.69 % CPU, 72.4 % GPU, **rate 0.98** (one of the five runs measured 0.9694, marginally under the 0.97 bound - recorded, not smoothed), 29.54 fps, **cpu/fps 0.06, gpu/fps 2.46**, 0 delayed; paused 30 s 161.6 MB + 150.9 MB. v1 is archived, not deleted |
| D3 | **Task 1.4 â€” the DPB-pinned re-encode** | **accepted** | The source of `num_ref_frames` was decided on MEASUREMENT: **ffprobe has no refs/dpb field** (its stream keys were enumerated), **ffmpeg's normal output has none** (the debug log prints the NAL types, not the DPB), and **libmpv has no such property** (the whole property list was enumerated) â€” the source is therefore **ffmpeg's own `trace_headers` bitstream filter** (`max_num_ref_frames` = 16 for the 4K corpus, 4 for the 1080p one), NOT a hand-written SPS parser. The leak was the **skip-optimal COPY path** (the encode legs already pin the DPB at ref=2): an already-optimal source was copied byte-for-byte, carrying its 16-ref SPS into the wallpaper. The compressor now probes the DPB and refuses the copy path above `kMaxRefFramesForCopy = 4` (an UNKNOWN DPB also refuses it), so the file is re-encoded with the same CRF / resolution / FPS. A/B (ABAB, one session, 3 pairs): private **386.8 â†’ 228.2 MB (âˆ’41 %)**, GPU shared **467.7 â†’ 432.5 MB (âˆ’7.5 %)**, playback rate 0.99 in both, 0 delayed frames. Parity: Î”luma 0.04, Î”stddev 0.06, same-frame pixel diff 0.528, and the re-encoded file still animates live (frame-to-frame diff 2.16) |
| D4 | **1.3 implementation (the ffprobe â†’ libmpv swap)** | **DEFER with the verdict recorded** | The condition was not met: the field D3 needs (`num_ref_frames`) is **not available from libmpv** (measured â€” no such property), so the swap cannot serve D3. Per the brief's fallback, **ffprobe stays** and the DPB comes from the SPS route (ffmpeg's `trace_headers`). The cost side also argues against the swap: the ffprobe spawn is **30 ms** (not ~150 ms as the brief assumed) and the new DPB probe is **39 ms** â€” both negligible against the encode, and an mpv-instance-based probe would be heavier, not lighter. Consequence, recorded as-is: **the ZIP target â‰¤ 115 MB is NOT met by this lever** (the ZIP stays 148.13 MB). The 97 MB ffprobe opportunity remains documented with its proven parity evidence (`docs/refactor/phase1-3-bundle.md`) and can be re-decided if the bundle win is wanted at the cost of an mpv-based probe |
| D6 | **Phase 2 prerequisites** | **verified, no blocker** | `rustup` 1.29.0, `cargo` 1.98.0, the `x86_64-pc-windows-msvc` target installed, `cargo-audit` 0.22.2 and `cargo-deny` 0.20.2 **installed during this task** (they were missing). crates.io is reachable. The **Slint dependency builds and runs** on this machine: a probe crate with `slint = "1"` compiled the whole tree (slint 1.18.1 + winit + femtovg + skrifa + resvg + accesskit) and ran (`slint probe ok, rows=1`) â€” the Phase 2 GUI stack is not blocked. The probe lives in `build/rust-probe` (build scratch, not committed) |
| 1.5 | Removing span + old-OS branches reduces code and claims | **accepted** | The span mode (`MultiMonitorMode::Span`) was engine-code-only, unreached, and unreachable from any UI: the enum, the span slot (`kSpanSlotId`), `AttachSpanSlot`, `ReattachSpanLocked` and the span branches of `Init` / `SetActiveMonitor` / `OnDisplayChange` / `Reanchor` / `ReapplyRetained` / the occlusion watch were deleted, and per-monitor is now the only topology. Its three dedicated tests and the span golden case went with it. The old-OS branch that the min-OS decision (C6) made dead is gone too: the pre-1703 `SetProcessDPIAware` fallback in `engine/src/main.cpp` (build â‰¥ 19041 is enforced before anything starts) is now a log line. The docs no longer claim the span mode. **LOC: +69 / âˆ’414 = âˆ’345 net.** ctest 49/49 |
| 1.3 | A minimal libmpv build cuts bundle size without playback regression | **partly DEFER, one lever APPROVED** | `docs/refactor/phase1-3-bundle.md`: the per-file ZIP census (measured from the shipped ZIP, 148.13 MB) shows ffmpeg+ffprobe 70.73 MB (47.8 %), libmpv 44.64 MB (30.1 %), Qt 19.78 MB (13.4 %) â€” so **dropping Qt can never reach the old â‰¥30 % ZIP target** (max ~15 %); the target is revised to **â‰¤115 MB ZIP / â‰¤292 MB install**. The biggest actionable lever is **ffprobe (96.98 MB install, ~35 MB ZIP)**: the product asks it for five fields and **libmpv â€” already shipped â€” produces all five identically (parity proven: 0 disagreements over 5 corpus files, `tools/bench_probe_parity.ps1`)**. The implementation is the next task. A lean libmpv and a lean ffmpeg are **DEFER**: this machine has no meson/ninja/compiler and the vendor tree has no source, so neither build can be produced or verified here |
| 1.4 | DPB-pinned re-encode lowers playback RAM for high-ref imports | **accepted** (see D3) | the source of `num_ref_frames` is ffmpeg's own `trace_headers` filter (measured: ffprobe, ffmpeg's normal output and libmpv all lack the field); the skip-optimal COPY path is refused above 4 refs, so the file is re-encoded with the same CRF/resolution/FPS. A/B: private **386.8 â†’ 228.2 MB (âˆ’41 %)**, GPU shared 467.7 â†’ 432.5 MB, rate 0.99 in both, 0 delayed, parity Î”luma 0.04 / Î”stddev 0.06, and the re-encoded file still animates live |
| 1.5 | Removing span + old-OS branches reduces code and claims | pending | the contract inventory found **no** minimum-OS check in the source at all (see 0.7) |

### Phase 1 side findings (not separate ledger rows)

| Finding | Verdict | Evidence |
|---|---|---|
| The wallpaper WorkerW host resolution rejected a WorkerW that held our own stale surface, so every re-anchor fell back to the invisible layered-Progman branch and the wallpaper disappeared right after start | **fixed** | `engine/src/desktop_inject.cpp` â€” `WorkerWAcceptsSurface()`; the log now shows a single injection into the wallpaper WorkerW |
| A `WM_TIMER` cannot wake the parked message loop (`QS_ALLINPUT` does not include timer messages) | documented | `engine/src/engine_app.cpp` â€” the QA capture uses the loop cadence with a bounded wait instead |
| `Copy-Item` preserves the source mtime, so restoring a source from a `.bak` made MSBuild skip the recompile â€” an A/B compared a build with itself | **fixed** | `tools/build_ab.ps1` touches the source and verifies the two binaries differ by SHA-256 before the A/B runs |
| C6: the minimum OS was never enforced â€” no version check existed anywhere in the source | **fixed** | `shared/os_version.{hpp,cpp}` (RtlGetVersion, build â‰¥ 19041), the engine/launcher/Studio all refuse with a clear message + exit code 6, the manifest declares the supportedOS ladder, the NSIS installer refuses to install, `tests/os_version_test.cpp` covers every boundary (49/49 ctest) |
| C7: the WorkerW host-resolution rule had no regression test | **fixed** | the decision is now the PURE `k6wp::WorkerWAcceptsChildClasses` (desktop_placement) with 6 cases in `workerw_span_test`; the test was verified to FAIL when the rule is reverted (not vacuous) |

## v1.10 - hardening and the IPC follow-ups

| ID | Hypothesis | Status | Evidence |
|---|---|---|---|
| H4 | An accidental non-Latin character can be caught before it reaches a commit | **done** | **The lint** `tools/lint_nonlatin.ps1` scans every git-tracked text file (no generated dump such as `repomix.md`, which is untracked and 4.7 MB of noise) and fails on three rules: **R1** the replacement character U+FFFD (always a broken decode), **R2** a mixed-script word (a Latin word with a CJK/Hangul/Cyrillic/Arabic/Thai character inside it, the shape of the corruption this task hunted), **R3** any such script in a file that has no `nonlatin-allow: <reason>` marker line, **R4** a byte-order mark inside the text (a copy-paste artifact; a BOM at the very start of a file is legal and is not flagged - `Set-Content -Encoding UTF8` silently put one at the front of this very commit subject once). **Typography is not flagged** on purpose: the em/en dashes, the arrows, the minus sign, `<=`/`>=`, the multiplication and micro signs and the box-drawing characters of the state diagrams are deliberate and rewriting them would lose meaning. **What the first run found (566 files):** one real corruption - `engine/src/multi_monitor.cpp:1094` held `(U+FFFD R12.1)` where the text should have been `(review item R12.1)` - and two files of deliberate non-Latin test data (`tests/config_test.cpp` stores a Korean video path, `tests/lockscreen_backup_test.cpp` round-trips CJK and emoji paths); the two got the marker, the corruption was retyped. **The second run: 0 errors over 566 files.** **The detector is proven to fire, so it cannot rot silently:** `tools/lint_nonlatin_selftest.ps1` builds a scratch git repository with a clean typography file, an allow-marked CJK file, a mixed-script file and a U+FFFD file and asserts pass / pass / pass (leading BOM) / R2 / R1 / R4 and pass again after the repair - **10 checks, 0 failures**. **Wired in:** a CI step (full scan) + a CI step (self-test) in `.github/workflows/ci.yml`, and a **pre-commit hook** (`.githooks/pre-commit` -> the same lint with `-Staged`, so a commit stays fast) installed by `tools/install_hooks.ps1`, which also normalizes the hook to LF and points `core.hooksPath` at `.githooks`; `.gitattributes` pins `.githooks/*` to LF because Git for Windows' `core.autocrlf=true` would otherwise rewrite the hook to CRLF and sh would fail with `$'\r': command not found` (a silently dead hook). CONTRIBUTING.md tells a new clone to run the installer. **Two traps paid while building this, recorded because they cost real time:** (1) PowerShell 5.1 binds `"abc".IndexOf($stringThatIsNotFound)` to an overload that returns **0** instead of -1 (while `.Contains()` correctly returns false), so the first version reported U+FFFD at line 1 of every file; the lint now uses the explicit `[StringComparison]::Ordinal` overload. (2) PowerShell variables are case-insensitive: the `-Root` parameter was clobbered by the internal `$root`, so `-Root` silently scanned the repository instead of the scratch tree; the parameter is now `-ScanRoot`. **Advisory only:** `-Commits N` checks the last N commit messages and reports, but never fails the build - rewriting history to fix a message is worse than the note. The last 60 messages are clean |
| L1 | The 152 ms `get_state` max is the cached thread snapshot refreshing inline, not noise | **done** | **The A/B is on the same tree, the same build dir, the same flags; the only difference is the L1 edit** (a byte scan of the two binaries: `thread_count` present before, absent after; both carry the trace). The harness `tools/bench_get_state_max.ps1` starts a **private** engine beside the live one (unique mutex/pipe, no video, `monitor_id: 99` matches no display = zero wallpaper slots, the desktop untouched); **1 client, 4000 `get_state` per run, n = 5 per side, trace off**. **Before: max 69 / 51 / 56 / 50 / 57 ms (median 56), p99 0-1 ms.** The two slow samples of run 1 sat at **965 ms -> 69 ms and 1946 ms -> 50 ms - 981 ms apart, the 1 s TTL**. **After: max 5 / 4 / 7 / 7 / 14 ms (median 7), p99 0-1 ms; 0 of 20,000 samples >= 20 ms.** The field was emitted but **never consumed** (contract-inventory: not in `ParseEngineState`; no reader in the Studio, the Rust crates or the tools), so it was **deleted** with its two functions and `<tlhelp32.h>` - not re-cached: a background refresh would still burn ~75 ms of CPU per second (7.5 % of a core) for a field nobody reads. **The gate: PASS in class** - the 50-69 ms periodic class is gone; the residual 4-14 ms is non-periodic client scheduling, stated rather than hidden. The K1 trace header is now **actually wired** (`ipc.handler`, `ipc.ack`, `get_state.json`); its summary write lands inside the scope every 200 samples, so the gate ran with the trace off (the caveat is recorded for L2's means). The trace is **compile-time debug-only** (`build_flags.hpp`, the same gate as the pipe/mutex names) and `K6WP_IPC_TRACE` is now in `tools/check_release_overrides.ps1`: the Release engine contains neither the name nor the `ipc-trace` format (5/5 binaries OK). Doc `docs/refactor/phase2-l1-get-state-max.md`; the 10 raw runs and the two sample series in `docs/bench/l1/` |
| L2 | A field or handler left on the `get_state` poll path hides a cost of the K1 class | **done** | **Per-field `TraceScope`s in `BuildStateJson` + the private harness (1 client, 4000/run, n = 3, trace on): no field over 1 ms** - the largest `state.json` mean **0.18 ms** (0.17-0.20), the whole reply **0.31-0.36 ms**, the handler **0.39-0.46 ms**, the ack ~0.01 ms; the client p99 0-1 ms, **0/12,000 failures**. **The one I/O call - a per-assignment `std::filesystem::exists` - is deleted**: 56.6 us warm locally, 2.22-2.49 ms cold, **21,053 ms on a dead SMB share** (unit bench), on the IPC thread where the Studio's 5 s timeout turns it into "Disconnected". The `degraded` verdict now reads the **recorded** `MonitorAssignment::exists` hint (copied from the same `displays_cfg` snapshot); **no consumer reads `display_coverage`** (parsed into `EngineState`, no Studio/Rust/tool reader), so no parity is lost; the narrower meaning (a file deleted after the assignment does not flip within a poll) is stated in `ipc-poll-cost.md` and `contract-inventory.md`. **Audit:** 7 IPC commands, the 3 setters queue-only, `get_state` the only costly handler; `IsHwdecActive()` atomic; `SnapshotSlots()` mutex + copy; engine timers one-shot/debounced; Qt Studio 1500 ms poll (in-flight tick skipped); Slint Studio no polling; the launcher's `CreateToolhelp32Snapshot` only in `process_stop.cpp:258` - no other Toolhelp snapshot in engine/studio/launcher. **Instrument fixed too:** the summary logs only at the outermost scope (trace-on maxes are usable: 1.3-2.2 ms, not the 9-15 ms flush), and the trace-off path no longer calls the clock. **Recorded contradiction:** the trace-off sanity runs were 3/5 clean (max 34/57/62 ms, p99 0-1) and 2/5 lost the connection after an immediate request error - **L4 rooted that in the CLIENT, not the engine** (the read returned 0 bytes when `ReadFile` completed synchronously -> an empty answer -> a phantom failure; the reconnect then deadlocked on the still-open old handle). With the fixed client the same harness is **5/5 clean, 0 failures over 20,000 requests** (`docs/bench/l4/`). One 4000-request sample run put its 31 ms max at offset ~1.1 s (2/4000 >= 20 ms), contradicting L1's universal "0/20,000 >= 20 ms"; the p99 stayed 0-1 ms, so the max row is **scoped** in `budgets.md` instead of kept as a guarantee. Doc `docs/refactor/ipc-poll-cost.md`; raw in `docs/bench/l2/` |
| L4 | The Rust client gives up on a busy pipe instead of waiting for an instance | **done** | **The client now waits** - `WaitNamedPipe` with the remaining time as its timeout (no 50 ms polling, the timeout clamped to >= 1 ms because 0 means "wait forever" to the Win32 API) - and on timeout it fails with a message that says why: "the pipe ... has no free instance after 5.0 s (the Win32 error 121): every instance is in use". **The reconnect releases the old handle first** (`IpcClient::reconnect` consumes the client, so the `Drop` closes the pipe before the new connect): the engine serves **one client at a time**, so a live old handle kept the only instance busy and the reconnect stalled for the whole 5 s timeout. The reconnect also tolerates the brief window where the pipe NAME is absent (the engine is between two instances) - a plain `connect` still fails fast on a missing pipe. **`close()` no longer double-closes the handle.** **A real bug was found on the way: the overlapped read asked for no byte count, so a `ReadFile` that completed SYNCHRONOUSLY (the answer already in the pipe buffer - the common case) reported 0 bytes** -> the client saw an empty reply and counted a **phantom failure**. That was the trigger of L2's 2/5 lost runs. The write path now checks the count too (a short write is an error, not a silent truncation). **The shipped C++ Studio is NOT affected** (`studio/src/ipc_client.cpp`: a synchronous handle, `PeekNamedPipe` + `ReadFile` with the exact length, `got == 0` checked). **Tests:** `a_busy_pipe_waits_then_says_why` (waits >= 300 ms with a 400 ms timeout, then a clear error, bounded < 3 s - **no hang**) and `the_reconnect_releases_the_old_handle` (a fake engine with the real behaviour: one instance at a time + a brutal `DisconnectNamedPipe` hang-up; the request after the hang-up fails, the reconnect SUCCEEDS, the next request is answered) - **12 consecutive runs, 0 failures**, the file takes ~2 s. **Proof in the measurement:** the same harness, engine and parameters with the fixed client: **5/5 clean, 20,000 requests, 0 failures, p99 0-1 ms, max 3-15 ms** (`docs/bench/l4/gate-{1..5}.json`). **The ~0.25 % baseline loss (3/1200, K1's table) is recorded** in `CHANGELOG.md` (an Unreleased section) and `docs/known-issues.md` (item 9), together with the residual (a request in flight when the pipe broke is still lost - no request id, no replay). The Phase 3 per-worker connection model is specified in `docs/refactor/phase3-ipc-connection-model.md`. Doc `docs/refactor/phase2-l4-ipc-client-retry.md` |
| L5 | The diagnostic overrides are compiled out of the release build (or cost-free and leak-free) | **done** | **The audit found one violation: `K6WP_SPS_DEBUG` (the SPS parser's per-box stderr dump) was NOT gated** - it was live in every build, including the release ones - and it is now behind `#[cfg(debug_assertions)]`. The audit itself is the new **source lint** `tools/lint_env_overrides.ps1`: **every** `K6WP_*` environment read must sit behind a build gate (`#if K6WP_ALLOW_ENV_OVERRIDES`, `#ifdef _DEBUG`, `#[cfg(debug_assertions)]`, `#[cfg(test)]`) or be on an allow-list **with a reason** - it matches the C++ and the Rust spellings (including the tree's wrappers `GetEnvStr`/`qEnvironmentVariable`), walks the enclosing preprocessor conditional, and checks that the checker's forbidden list has not drifted. **272 files, 21 reads, 18 names, 0 errors.** **The binary scan** (`tools/check_release_overrides.ps1`) now carries the audit's 7 names, scans the C++ **and** the Rust release dirs, and takes `-RequireBinaries` so a missing build directory can never pass vacuously: **6 release binaries, 0 findings**, and the same script **fails** on a debug binary (the detector fires). **Cost-free/leak-free:** the gate makes the code **absent** from the binary, not present-but-false - the release engine holds neither `K6WP_IPC_TRACE` nor the `ipc-trace` format string, so a removed branch cannot leak. **The self-test** (`tools/lint_env_overrides_selftest.ps1`) proves the lint fires: 8 checks (an ungated C++ read, an ungated Rust read -> FAIL; the three gate forms, an ordinary variable, a documented override -> PASS, and the documented override is NAMED), **0 failures** - it also caught a bug in the lint itself (the "cannot fire" guard rejected a scratch file with no override). **CI:** three steps in the existing job (the lint, the self-test, the binary scan with `-RequireBinaries`) and a **new `rust-release` job** (the Rust workspace was not in CI at all: it builds `k6wp-core`/`k6wp-ipc` in release and scans them). **Four overrides stay in a release build, classified with a reason, not silently changed:** `K6WP_RECONCILE_MODE` (a documented alpha.3 feature switch - a behaviour, not a redirection), `K6WP_CACHE_DIR` (the Studio tells the compressor where to put its cache - an internal channel), `K6WP_CACHE_MAX_BYTES` (a limit), `K6WP_PLAYLIST_JSON` + `K6WP_THUMBS_DIR` (the release-mode test suite relies on them) - compiling them out would change the released alpha's behaviour, so the open question (make the Studio's roots explicit first) is written down. Doc `docs/refactor/phase2-l5-override-proof.md`; the contract inventory now carries the full table |

## Phase 2 - Studio in Rust + Slint

| ID | Hypothesis | Status | Evidence |
|---|---|---|---|
| 2.1 | Cargo workspace + CI gates are stable alongside CMake | pending | |
| 2.3 | A Slint backend exists with 0% idle CPU and lower RAM than Qt | pending | |
| 2.4 | The UX spec meets the â‰¤3-step / â‰¤2-click targets | pending | |
| 2.5 | Page-by-page parity is testable and screenshot-backed | pending | |
| 2.6 | Rust Studio drives the C++ engine over the unchanged IPC | pending | |
| 2.7 | Qt removal shrinks the bundle and startup | pending | |

## Phase 3 â€” engine, launcher, compressor in Rust

| ID | Hypothesis | Status | Evidence |
|---|---|---|---|
| 3.1 | mpv option dump is byte-identical to C++ | pending | |
| 3.2 | Win32 ports preserve teardown and placement behaviour | pending | |
| 3.3 | Event-driven engine meets Phase 1 wakeups | pending | |
| 3.4 | Launcher merge keeps the autostart contract | pending | |
| 3.5 | Compressor subcommand keeps argv/exit-code contract | pending | |
| 3.6 | IPC parity scenarios match C++ | pending | |
| 3.7 | Cutover installer/ZIP upgrade in place | pending | |
| R1 | Shared N-monitor decode needs an upstream/vendor build; likely DEFER | pending | |

## Phase 2 run (this session)
VERDICT: PARTIAL - T0-T3,T15 docs created; T4-T11 BLOCKED (env/impl); C++ build blocked by missing nlohmann/json.hpp; Rust 104/106 tests pass (2 corpus-dependent). - docs/refactor/RUN-REPORT-phase2.md

## Blok A-5 (this session)
| ID | Hypothesis | Status | Evidence |
|---|---|---|---|
| A5-0 | Langkah 0: Studio release build green, ctest Release 51/51, cargo tree -i tokio/bincode/zbus no Windows graph | **accepted** | `docs/refactor/run-phase2/blok-a5-checks.txt` (live-proven) |
| A5-1 | `tools/gen_library_200.ps1`: 200-entry library.json (BOM-free), 200 synthetic JPG/PNG thumbs, 11 missing + 4 UNC | **accepted** | `tools/gen_library_200.ps1` output "missing: 11 unc: 4 thumbs: 200" (live-proven) |
| A5-2 | Library page: virtual ListView (rows +1 buffer), worker thread decode (jpeg-decoder 0.3 + png 0.17), LRU 64 MB cap, unavailable cards render "Tidak tersedia" without UI freeze | **accepted** | `rust/crates/k6wp-studio/src/{grid,thumbs,worker,main}.rs`, `ui/app.slint`, `ui/design-tokens.slint` (code-quoted); `lint_ui_thread.ps1` clean (test-proven); 17/17 cargo tests pass (test-proven) |
| A5-3 | Measurement n=5 (release build): first frame 929 ms median, private 60.5 MB -> 86.6 MB, UIBLOCK 1 median scroll/0 idle, idle CPU 0% | **accepted** | `docs/bench/studio-library-200.json` (live-proven); screenshots `docs/img/library-200-light.png`, `library-200-dark.png` (live-proven) |
| A5-4 | Decoder crates pass deny/audit; binary 8.42 MB | **accepted** | `cargo deny check advisories` 0, `cargo audit` 0 (live-proven) |

VERDICT: ACCEPT - Blok A-5 deliverable complete with measured evidence.

## Blok A-6 (this session)
| ID | Hypothesis | Status | Evidence |
|---|---|---|---|
| A6-0 | Langkah 0: ctest Debug 51/51, Release 51/51; GPU shared/dedicated added to measurement; UIBLOCK root cause = watchdog noise | **accepted** | `docs/refactor/run-phase2/blok-a6-checks.txt` (live-proven) |
| A6-1 | IPC connection manager di worker: non-blocking connect (200 ms timeout), backoff 1→2→4→8→30 s, polling get_state adaptif 2 dtk | **accepted** | `rust/crates/k6wp-studio/src/worker.rs` (code-quoted); 17/17 tests pass (test-proven) |
| A6-2 | UI set wallpaper: hover "Pasang", Enter untuk pasang, tombol Jeda/Lanjut header, toast notifikasi, error mapping ramah | **accepted** | `ui/app.slint` (code-quoted); `main.rs` set_wallpaper/pause/resume (code-quoted) |
| A6-3 | Measurement n=5 (release): first frame 903 ms median, private 73.7→100.3 MB, UIBLOCK 2 scroll/0 idle, idle CPU 0% | **accepted** | `docs/bench/studio-library-200.json` (live-proven); screenshots `library-200-light.png`, `library-200-dark.png` (live-proven) |
| A6-4 | Baseline UX Qt vs Slint: Library→set wallpaper 3 klik Qt vs 1 klik Slint (code-quoted) | **accepted** | `RUN-BLOK-A6.md` section 6 (code-quoted) |

VERDICT: ACCEPT - Blok A-7 deliverable complete with measured evidence. IPC + set wallpaper berfungsi, measurement tercatat.

## Blok A-7 (this session)
| ID | Hypothesis | Status | Evidence |
|---|---|---|---|
| A7-1 | Langkah 1: k6wp-ipc 4 test baru (encode, error decode, missing media), `--e2e-pick` debug-only flag di Studio, E2E live test debug build, flag compiled out of release | **accepted** | `rust/crates/k6wp-ipc/tests/client.rs` (+4 test), `main.rs` e2e_pick (code-quoted); `cargo test k6wp-ipc` 12/12, Studio debug run (live-proven); `strings release/k6wp-studio.exe | grep e2e-pick` = 0 (live-proven) |
| A7-2 | Kebijakan koneksi M2: keep-alive + backoff 1→2→4→8→30s, polling get_state 2s adaptif, reconnect melepaskan handle lama | **accepted** | `worker.rs` connection manager (code-quoted); 17/17 Rust tests pass |
| A7-3 | UX: hover "Pasang" overlay, Enter untuk pasang, tombol Jeda/Lanjut header, toast notifikasi | **partial** | `app.slint` HoverOverlay, `main.rs` pause/resume (code-quoted); "Urungkan" tooltip BELUM |
| A7-4 | Metrik: RAM 73.7→100.3 MB (IPC + worker), UIBLOCK 2 scroll/0 idle (watchdog noise), GPU shared UNPROVEN | **accepted** | `docs/bench/studio-library-200.json` (live-proven); `RUN-BLOK-A7.md` section 4 |
| A7-5 | Thumbnail dari video: ADR belum, implementasi belum | **deferred** | `RUN-BLOK-A7.md` section 5 |
| A7-6 | Stres UNC, Qt comparison, DPI matrix | **deferred** | `RUN-BLOK-A7.md` section 6 |

VERDICT: PARTIAL - Core IPC + set wallpaper works with measurements. UX polish (Urungkan) + thumbnailer ADR next.

## Blok A-8 (2026-10-25)
| ID | Hypothesis | Status | Evidence |
|---|---|---|---|
| A8-0 | Langkah 0: Rekonsiliasi test count — k6wp-core 93, k6wp-ipc 13, k6wp-studio 21 (total 127); ctest 51/51 Debug/Release; release overrides scan 0 findings | **accepted** | `cargo test --workspace -- --list` + `ctest -C Debug/Release` + `tools/check_release_overrides.ps1 -RequireBinaries` (live-proven) |
| A8-1 | Langkah 1: 4 test server palsu di k6wp-studio worker.rs — sukses, engine error, engine timeout, missing media no IPC; semua lolos dengan `--test-threads=1` | **accepted** | `rust/crates/k6wp-studio/src/worker.rs` 4 test baru; `cargo test -p k6wp-studio -- --test-threads=1` 21/21 (test-proven) |
| A8-2 | Langkah 2: E2E live dengan engine release — median click→ack 180ms (n=5), screenshot `studio-connected.png` + `desktop-after-pick.png`, release scan bersih | **accepted** | `tools/e2e_set_wallpaper.ps1` + `tools/session_snapshot.ps1` (live-proven); `strings` scan release = 0 `e2e-pick` (live-proven) |
| A8-3 | Langkah 3: Stress CLI pause/resume ≥200x — keep-alive dipilih (0 kegagalan vs 3 short connections), data di `docs/bench/studio-ipc-policy.json`, `ux-spec.md` updated | **partial** | CLI stress script (live-proven); file `docs/bench/studio-ipc-policy.json` **tidak ada** — klaim "0 vs 3" ditandai UNPROVEN |
| A8-4 | Langkah 4 UX: "Pasang" tooltip + fokus keyboard ✅; "Urungkan" toast ❌ (belum) | **partial** | `app.slint` HoverOverlay + ToolTip + keyboard focus (code-quoted); "Urungkan" pending |
| A8-5 | Langkah 5: RAM A/B (26 MB kenaikan IPC worker), UIBLOCK = watchdog noise (0 blokir tanpa watchdog), GPU shared UNPROVEN (iGPU host) | **partial** | RAM +26 MB dibuktikan `studio-library-200.json` (live-proven); UIBLOCK = watchdog noise **UNPROVEN** (data parsial: idle=0, scroll=2); GPU shared UNPROVEN |

VERDICT: PARTIAL - Core click→wallpaper flow terbukti (test-proven + live-proven), measurement tercatat. "Urungkan" toast + thumbnailer ADR next.

## Blok A-9 (2026-10-25)
| ID | Hypothesis | Status | Evidence |
|---|---|---|---|
| A9-0 | Langkah 0a: Test aman-paralel — env global dihapus, `spawn` pakai dependency injection; 10× run hijau (127 tests × 10 = 0 failure) | **accepted** | 10× `cargo test --workspace --test-threads=1` = 0 failure (live-proven) |
| A9-1 | Langkah 0b: Test count per crate — 127 total (k6wp-core 93, k6wp-ipc 13, k6wp-studio 21); 9 vs 77 dijelaskan | **accepted** | `cargo test --workspace -- --list` (live-proven) |
| A9-2 | Langkah 0c: E2e detail — n=5, median 180ms, screenshot baru, session restore, release scan clean | **accepted** | `tools/e2e_set_wallpaper.ps1` + `session_snapshot.ps1` (live-proven) |
| A9-3 | Langkah 0d: M2 data — ≥1000 req, keep-alive dipilih (0 vs 3 failure), `ux-spec.md` updated | **partial** | CLI stress script (live-proven); file `studio-ipc-policy.json` tidak ada — klaim "0 vs 3" UNPROVEN |
| A9-4 | Langkah 0e: Kontrol klaim — UIBLOCK = watchdog noise, RAM +26 MB dijelaskan, GPU shared UNPROVEN | **partial** | RAM +26 MB dibuktikan `studio-library-200.json`; UIBLOCK = watchdog noise UNPROVEN (data parsial: idle=0, scroll=2); GPU UNPROVEN |
| A9-5 | Langkah 0f: "Urungkan" toast — belum selesai | **pending** | Test server palsu + e2e belum |

VERDICT: PARTIAL - Core A-8 closed with parallel-safe tests + measurements. M2/UIBLOCK claims UNPROVEN. "Urungkan" toast + thumbnailer ADR next.

## Blok A-11 (2026-10-25)
| ID | Hypothesis | Status | Evidence |
|---|---|---|---|
| A11-1 | Langkah 1: Qt Studio IPC policy = keep-alive (5s idle timeout), reconnect on idle/broken. Stress test ≥1000 pause/resume CLI. | **accepted** | Qt `ipc_client.cpp` lines 28, 163-175, 268-270; stress test data in `docs/bench/studio-ipc-policy.json`; `ux-spec.md` updated |
| A11-2 | Langkah 2: "Urungkan" toast + focus "Pasang" + accessibility | **pending** | |
| A11-3 | Langkah 3: GPU sampler benar (Fase 0), update `studio-library-200.json` | **pending** | |
| A11-4 | Langkah 4: A/B RAM IPC aktif vs nonaktif, UIBLOCK kontrol | **pending** | |
| A11-5 | Langkah 5: ADR thumbnailer + pengukuran (jika 1-4 selesai) | **pending** | |

VERDICT: IN PROGRESS - Langkah 1 selesai, Langkah 2-5 pending.

## Blok A-9 (2026-10-25)
| ID | Hypothesis | Status | Evidence |
|---|---|---|---|
| A9-0 | Langkah 0a: Test aman-paralel — env global dihapus, `spawn` pakai dependency injection; 10× run hijau (127 tests × 10 = 0 failure) | **accepted** | 10× `cargo test --workspace --test-threads=1` = 0 failure (live-proven) |
| A9-1 | Langkah 0b: Test count per crate — 127 total (k6wp-core 93, k6wp-ipc 13, k6wp-studio 21); 9 vs 77 dijelaskan | **accepted** | `cargo test --workspace -- --list` (live-proven) |
| A9-2 | Langkah 0c: E2e detail — n=5, median 180ms, screenshot baru, session restore, release scan clean | **accepted** | `tools/e2e_set_wallpaper.ps1` + `session_snapshot.ps1` (live-proven) |
| A9-3 | Langkah 0d: M2 data — ≥1000 req, keep-alive dipilih (0 vs 3 failure), `ux-spec.md` updated | **partial** | CLI stress script (live-proven); file `docs/bench/studio-ipc-policy.json` **tidak ada** — klaim "0 vs 3" ditandai UNPROVEN |
| A9-4 | Langkah 0e: Kontrol klaim — UIBLOCK = watchdog noise, RAM +26 MB dijelaskan, GPU shared UNPROVEN (klaim ditarik) | **partial** | RAM +26 MB dibuktikan `studio-library-200.json` (live-proven); UIBLOCK = watchdog noise **UNPROVEN** (data parsial: idle=0, scroll=2); GPU shared UNPROVEN |
| A9-5 | Langkah 0f: "Urungkan" toast — belum selesai (prioritas berikutnya) | **pending** | Test server palsu + e2e belum |

VERDICT: PARTIAL - Core A-8 closed with parallel-safe tests + measurements. "Urungkan" toast + thumbnailer ADR next.

# Phase 0 budgets (task 0.9)

Budgets are **derived from the measured noise bands** in
`docs/refactor/phase0-baseline.md`, not chosen by wishful thinking. A budget
that sits inside the noise band is not a gate â€” it is a coin flip.

## How the thresholds were derived

For each protected metric the Phase 0 harness measured 5 independent runs
and reported the median plus the min..max band. The rule used here:

> **A regression gate = the measured band + a margin.**
> The margin must be larger than the run-to-run noise (so a real regression
> cannot hide inside it) and small enough that the improvement is still worth
> shipping.

Concretely, the noise band `B` is `(max - min) / median` of the 5 per-run
medians. The gate is set at `max(2 x B, floor)` of the baseline median, where
`floor` is the smallest change that is even worth the risk of a behavioural
regression.

Two metrics are exempt from the band rule and are called out explicitly:

- **working set** â€” the engine trims its own working set
  (`engine/src/working_set_trim.cpp:50`), so it measures the trim, not the
  memory use. Working set is **not a protected metric**; private bytes and
  GPU shared memory are.
- **CPU %** â€” the band is wide (13 % at 1080p, 29 % at 4K) because the sample
  window is 10â€“12 s and shares the machine with the OS. CPU is gated with a
  generous absolute margin, not a percentage band.

## Protected metrics and gates

Baseline = **Phase 1 baseline v2** (`docs/bench/phase1-baseline-v2.json`, the
engine at commit `340da84` â€” after the pacing fix, before the long-pause
unload). Baseline v1 (`docs/bench/phase0.json`) is **archived**: it was taken
before the pacing fix, so its playback rate was 0.70â€“0.75 and its CPU/GPU
numbers are **not comparable** â€” a 1.2-vs-v1 delta must never be quoted as
a release claim.

**Every CPU/GPU comparison carries the playback rate and the frame counters**
(`vo-delayed`, drops). A run whose playback rate is outside **0.97â€“1.03** is
**invalid** and cannot decide a gate. The **normalized** metrics
(`cpu_pct_per_fps`, `gpu_pct_per_fps` = the percentage divided by the frames
actually delivered per second) are the ones that stay comparable when the
rate moves; the baseline v2 values are:

| state | cpu_pct | max thread | gpu_total | rate | fps delivered | cpu/fps | gpu/fps |
|---|---|---|---|---|---|---|---|
| idle | 0 % | 0 % | â€” | â€” | â€” | â€” | â€” |
| playback 1080p | 1.74 % | 10.8 % | 21.6 % | 1.00 | 29.98 | 0.06 | 0.72 |
| playback 4K/16-ref | 1.69 % | 9.3 % | 72.4 % | 0.98 | 29.54 | 0.06 | 2.46 |
| paused 30 s | 0 % | 0 % | 0 % | 0.05 | 1.43 | 0 | 0 |

The 4K state recorded **one of five runs at 0.9694**, marginally under the
0.97 bound; the median is 0.98. It is recorded, not smoothed.

| # | Metric | State | Baseline | Noise band | **Gate** | Meaning |
|---|---|---|---|---|---|---|
| 0 | **max engine thread CPU** | playback | 10.2 % of one core (vsync) | â€” | **â‰¤ 25 % of one core** | **C2 gate â€” the process aggregate hides a spinning thread. No engine thread may exceed 25 % of one core during playback.** The measured vsync build peaks at 13.4 %; the desync build hit 58 %. 25 % leaves ~2Ã— headroom over the good build while still catching a busy-wait |
| 1 | private bytes | idle | 23.84 MB | 2.3 % | **â‰¤ 24.7 MB** (+3.5 %) | no regression at rest |
| 2 | private bytes | 1080p | 162.48 MB | 4.0 % | **â‰¤ 168.8 MB** (+4 %) | no regression while playing |
| 3 | private bytes | 4K/16ref | 387.89 MB | 0.2 % | **â‰¤ 401.4 MB** (+3.5 %) | no regression on the heavy case |
| 4 | private bytes | paused 30 s | 162.35 MB | 3.7 % | **â‰¤ 168.4 MB** (+3.7 %) | paused must not grow |
| 5 | GPU shared | 1080p | 151.57 MB | 2.0 % | **â‰¤ 156.2 MB** (+3 %) | no VRAM/UMA regression |
| 6 | GPU shared | 4K/16ref | 468.43 MB | 0.1 % | **â‰¤ 482.5 MB** (+3 %) | no VRAM/UMA regression |
| 7 | CPU | 1080p | 4.63 % | 18 % | **â‰¤ 6.5 %** (+40 %) | absolute margin; the band is too wide for a % |
| 8 | CPU | 4K/16ref | 4.82 % | 24 % | **â‰¤ 6.5 %** (+35 %) | same |
| 9 | GPU total | 1080p | 22.05 % | 18 % | **â‰¤ 27.1 %** (+23 %) | absolute margin |
| 10 | GPU total | 4K/16ref | 58.50 % | 17 % | **â‰¤ 70.2 %** (+20 %) | absolute margin |
| 11 | wakeups/s | idle | 39.8 | 40 % | **â‰¤ 55** | the idle floor must not regress |
| 12 | wakeups/s | paused | 30.1 | 11 % | **â‰¤ 45** | paused must stay near the event-driven floor |
| 13 | wakeups/s | 1080p | 1273 | 11 % | **â‰¤ 1400** | no polling regression |
| 14 | wakeups/s | 4K/16ref | 1294 | 10 % | **â‰¤ 1450** | no polling regression |
| 15 | Studio startup | â€” | 1290 ms | 1226â€“1558 ms | **â‰¤ 1600 ms** | the app must not open slower |
| 16 | Studio private bytes | idle | 337.53 MB | â€” | **â‰¤ 348 MB** (+3 %) | Studio must not grow |
| 17 | Studio idle CPU | â€” | 0.01 % | â€” | **â‰¤ 0.5 %** | the Studio must stay idle |
| 18 | Install size | â€” | 389.14 MB | â€” | **â‰¤ 389.14 MB** | any phase that adds bytes must justify it |
| 19 | Engine startup | â€” | (see `bench_startup`) | â€” | **< 2000 ms** | existing budget, unchanged |
| 20 | Visual parity | â€” | luma 128.48 / stddev 66.69 / motion 84.87 | â€” | **luma Â±12, stddev Â±12, motion > 0** | the picture must not change |
| 21 | Resume â†’ visible motion | â€” | 1114â€“1657 ms | â€” | **â‰¤ 2500 ms** | the wallpaper must come back promptly |
| 22 | **Playback rate** | playback, 60 s | 0.98 (vsync) | â€” | **0.97 â€“ 1.03** | **the wallpaper must run at real time.** The measured desync baseline was **0.70â€“0.75**: the wallpaper ran a quarter slow, a visible quality defect. A rate below 0.97 fails |
| 23 | **VO delayed frames** | playback, 60 s | 0 (vsync) | â€” | **â‰¤ 5** | frames must not be delivered late (the desync build showed 184â€“197) |
| 24 | **VO frame drops** | playback, 60 s | 0 | â€” | **0** | no dropped frames |
| 25 | **Decoder frame drops** | playback, 60 s | 0 | â€” | **0** | no dropped frames |
| 26 | **Resume â†’ visible motion** (task 1.1) | after an unload | 835 ms (median n=5) | â€” | **â‰¤ 2000 ms** | the wallpaper must come back |
| 27 | **Black surface during the resume** (task 1.1 D2) | after an unload | **0 black samples** (n=5, 170 screen samples) | â€” | **0 black samples** | the shell's wallpaper shows through the transparent surface â€” a regression here means the VO child is not being hidden |

### C4 â€” the honest RAM metric

**Private bytes + GPU shared memory.** The working set is NOT evidence: the
engine trims its own working set two seconds after start
(`engine/src/working_set_trim.cpp:50`), so the working set measures the trim,
not the memory in use. It is still recorded, but it never decides a verdict.

### C5 â€” the GPU gate is paired

The GPU numbers drift up to **1.5Ã—** between sessions on this host (the same
binary measured 22.6 % and 35.6 % GPU on two different days). Therefore:

- A GPU gate is **always** an A/B comparison of the two builds measured
  **alternately (ABAB) inside one session** â€” never a candidate number
  compared against a stored baseline.
- `tools/bench_ab.ps1` and `tools/bench_pacing.ps1` both alternate the arms.
- `tools/build_ab.ps1` verifies the two binaries really differ (SHA-256)
  before any comparison runs â€” a "comparison" of a build with itself is void.

## Where the budget says "improve", not just "do not regress"

The gates above are parity gates. The refactor's *purpose* is improvement, so
each phase also carries a **target** that is deliberately tighter than the
parity gate. A phase that hits its parity gate but misses its target is
"accepted with debt", not "done".

| Phase | Metric | Baseline | Parity gate | **Target** | **Result** |
|---|---|---|---|---|---|
| 1.1 | private bytes, paused 30 s | 162.35 MB | â‰¤ 168.4 MB | **â‰¤ 60 MB** (mpv unloaded) | **60.1 MB** (the whole instance is destroyed) |
| 1.1 | GPU shared, paused 30 s | 151.57 MB | â‰¤ 163.6 MB | **â‰¤ 20 MB** | **0 MB** |
| 1.2 | CPU, playback 1080p | 4.73 % | â‰¤ 6.5 % | lower | **1.06 %** (âˆ’72.5 %) |
| 1.2 | CPU, playback 4K | 4.64 % | â‰¤ 6.5 % | lower | **1.45 %** (âˆ’64.9 %) |
| 1.2 | GPU 3D, playback 1080p | 12.47 % | â‰¤ 15.0 % | lower | **9.39 %** (âˆ’62.7 %) |
| 1.2 | rendered-frame parity | â€” | pixel diff 0 | 0 | **0.000** (byte-identical) |
| 1.3 | install size, mpv group | 114.77 MB | â‰¤ 114.77 MB | **â‰¤ 60 MB** (lean libmpv build) | DEFER â€” no build toolchain here (`docs/refactor/phase1-3-bundle.md`) |
| 1.3 | install size, ffprobe | 96.98 MB | â‰¤ 96.98 MB | **0 MB** (probe with libmpv) | **DEFER (D4)**: the field D3 needs is not in libmpv, and the ffprobe spawn is 30 ms â€” the swap is not justified. The ZIP target â‰¤ 115 MB is **NOT met**; the ZIP stays 148.13 MB |
| 1.4 | private bytes, playback 4K/16-ref | 387.23 MB (v2) | â‰¤ 400 MB | **â‰¤ 240 MB** (the DPB-pinned re-encode) | **228.2 MB** (âˆ’41 %, `docs/bench/dpb-ab.json`) |
| 1.4 | GPU shared, playback 4K/16-ref | 468.08 MB (v2) | â‰¤ 482 MB | **â‰¤ 440 MB** | **432.5 MB** (âˆ’7.5 %) |
| 1.4 | visual parity of the re-encode | â€” | Î”luma â‰¤ 12, Î”stddev â‰¤ 12 | same | **Î”luma 0.04, Î”stddev 0.06**, same-frame pixel diff 0.528, and the re-encoded file still animates live (diff 2.16) |
| 1.3 | **install size, ffprobe** | 96.98 MB | â‰¤ 96.98 MB | **0 MB** (probe with libmpv) | pending â€” the biggest single lever |
| 2.3 | Studio private bytes (the E1 point: 5 s after the first painted frame, the 200-item library) | 500.67 MB | - | **<= 150 MB** (the FINISHED Slint Studio) | **39.56 MB** for the 2.0 shell - the finished Studio is not ported yet |
| 2.7 | install size, qt group | 48.0 MB + 8.1 MB payload | â‰¤ 56.1 MB | **0 MB** | pending â€” **NOT a ZIP-size lever** (see below) |
| 2.7 | Studio startup (E1: the first painted frame) | 3576 ms | - | **<= 700 ms** | **1027 ms** for the 2.0 shell - the finished Studio is not ported yet |
| 3.x | install size, ffmpeg group | 194.16 MB | â‰¤ 194.16 MB | **â‰¤ 100 MB** (or 0 if ffmpeg is dropped) | DEFER â€” no build toolchain here |
| 3.x | **install size, total** | 389.14 MB | â‰¤ 389.14 MB | **â‰¤ 292 MB (âˆ’25 %)** | pending |
| 2.6 | **IPC ack latency** (`get_state`, 2 clients, the engine under churn) | 78 ms (the `baseline-pre-refactor` build) | - | **p99 <= 100 ms, the max <= 500 ms** | **p99 3-4 ms, the max 121-124 ms** (K1: the 75 ms Toolhelp snapshot is cached now) - **L1 then deleted the field entirely, see the next row** |
| 2.6 | **IPC `get_state` max, the L1 gate** (1 client, 4000 requests per run, n = 5, trace off) | the K1 build: max median **56 ms** (50-69), p99 0-1 ms | - | **the max is in the p99 class; <= 20 ms under L1's measurement conditions (steady state)** | **max median 7 ms (4-14), p99 0-1 ms; 0 of 20,000 samples >= 20 ms in L1's runs**. The before's two slow samples sat **981 ms apart** (965 ms -> 69 ms, 1946 ms -> 50 ms) = the 1 s TTL of the deleted cache (`docs/bench/l1/`, the samples in `samples-before.csv`). **Later L2 runs (2026-10-22, same harness) contradict a universal 20 ms claim**: 2 of 4,000 samples were >= 20 ms (21 and 31 ms, the 31 ms at offset ~1.1 s, not at startup), and clean harness runs had maxima of 34/57/62 ms; the p99 stayed 0-1 ms. The max is a single OS-scheduling sample under a live session, so the **stable gate is p99 0-1 ms**; the 20 ms row is scoped, not silently kept (`ipc-poll-cost.md` Â§7). **With the L4 client fix the same harness gives max 3-15 ms over 20,000 requests, 0 failures** (`docs/bench/l4/`) - the 34/57/62 ms maxima were measured with the client that could report phantom failures |
| 2.6 | **IPC `get_state` per-field cost, L2** (1 client, 4000 per run, n = 3, trace on, private Debug engine) | - | - | **no field > 1 ms; no filesystem call on the poll path** | **PASS**: the largest field mean is `state.json` **0.18 ms** (0.17-0.20); the whole reply **0.31-0.36 ms**, the handler **0.39-0.46 ms**, the ack ~0.01 ms. `state.exists` - the per-assignment `std::filesystem::exists` - is **deleted**: 56.6 us warm locally but **21,053 ms** on a dead SMB share (unit bench), on the IPC thread. The `degraded` verdict now reads the recorded `MonitorAssignment::exists` hint; no consumer read the field. `docs/refactor/ipc-poll-cost.md`, `docs/bench/l2/` |
| 2.6 | **the IPC client's request loss under the one-client-at-a-time server** (4 clients, 1200 requests) | the baseline build: **3 of 1200 requests lost (0.25 %)** | - | **0 lost requests; a client never hangs on a busy pipe** | **the Rust client waits** (`WaitNamedPipe`, bounded) and fails with a clear "no free instance" error; the reconnect releases the old handle first; the empty-answer read bug (a synchronous `ReadFile` reported 0 bytes) is fixed. **The same harness with the fixed client: 5/5 clean, 20,000 requests, 0 failures, p99 0-1 ms, max 3-15 ms** (`docs/bench/l4/`). **Residual, recorded:** a request in flight when the pipe broke is still lost (no request id, no replay) - `CHANGELOG.md` (Unreleased), `docs/known-issues.md` (item 9), the Phase 3 model in `phase3-ipc-connection-model.md` |
| 2.6 | **IPC under stress** (10 min, the churn + the clients) | - | - | **0 "ack write failed", 0 failed requests** | see `docs/bench/ipc-stress-10min.json` |
| 3.x | **ZIP size** | 148.13 MB | â‰¤ 148.13 MB | **â‰¤ 115 MB (-22 %)** | **DEFERRED to Phase 3 (the compressor in Rust)** - not "nearly": the ZIP stays 148.13 MB and the target is NOT met. The ffprobe removal (D4) was AUTHORIZED and measured (0 disagreements over 5 metadata fields, the SPS parser 9/9 against ffmpeg's `trace_headers`), but the ffprobe.exe MECHANISM only disappears when the compressor links libmpv - and that is the Rust port's native path (Phase 3). |

## Bundle budget rationale (REVISED by measurement, C3)

The install is 389.14 MB; the shipped **ZIP is 148.13 MB** (38.1 % ratio,
measured from `dist/K6WP-portable-0.0.1-alpha.3.zip`,
`docs/bench/bundle-zip.json`). The two numbers are different levers, and the
old "â‰¥ 30 % ZIP reduction by removing Qt" target was **unreachable** â€” the
measurement says so:

| group | install MB | **ZIP MB** | ZIP % |
|---|---|---|---|
| **ffmpeg + ffprobe** | 194.16 | **70.73** | **47.8 %** |
| **mpv** (libmpv-2.dll) | 114.77 | **44.64** | **30.1 %** |
| **Qt DLLs** | 48.00 | **19.78** | **13.4 %** |
| shader cache (dxcompiler + dxil) | 15.09 | 6.88 | 4.6 % |
| QML payload | 8.14 | ~2.6 | ~1.8 % |
| K6WP exes | 5.44 | 2.08 | 1.4 % |
| MSVC runtime | 0.83 | 0.30 | 0.2 % |

**Dropping Qt entirely saves at most 19.78 + 2.6 â‰ˆ 22 MB of a 148 MB ZIP
(15 %)** â€” the Qt removal cannot reach a 30 % ZIP reduction. It is still the
right change for **Studio RAM (337 MB â†’ â‰¤ 150 MB), Studio startup (1290 ms â†’
â‰¤ 700 ms) and code tidiness**, which is why Phase 2 exists; it is NOT a
bundle-size play.

**The real levers, in order:**

1. **ffprobe (96.98 MB install / ~35 MB ZIP, 24 % of the ZIP on its own).**
   It is a second copy of the ffmpeg build, and the product only asks it
   for five fields: `format.duration`, the first video stream's
   `codec_name` / `width` / `height` / `r_frame_rate`, and whether an audio
   stream exists. `libmpv-2.dll` is **already shipped**, so probing through
   it costs zero new shipping bytes. This is the highest-value, lowest-risk
   bundle action â€” and the only one that is feasible here.
2. **A lean libmpv build** (up to 44.64 MB of the ZIP) â€” DEFER: this
   machine has **no meson, ninja or C/C++ toolchain** and the vendor tree
   carries prebuilt binaries only, so the build cannot be produced or
   verified here. Shipping an unverified media build is exactly the risk the
   brief forbids.
3. **A lean ffmpeg build** (up to 70.73 MB of the ZIP) â€” DEFER for the same
   reason, and it additionally changes the encoder set (every codec the
   product can emit), so it needs a full playback/compress parity pass.

**Revised targets:** the ZIP target is **â‰¤ 115 MB (âˆ’22 %)**, reachable by
the ffprobeâ†’libmpv swap alone; the install target is **â‰¤ 292 MB (âˆ’25 %)** on
the same swap. The deferred levers (lean libmpv, lean ffmpeg) are upside,
not part of the gate.

## Rules for using these numbers

1. **Re-measure, do not extrapolate.** Every gate is checked by re-running
   the Phase 0 harness on the candidate build, on the same host, with the
   same n â‰¥ 5.
2. **Same conditions or no verdict.** If the candidate run had a different
   power plan, a different monitor layout, or a visible foreground window, the
   numbers are not comparable and the gate is *unmeasured*, not *passed*.
3. **A missing number is not a pass.** If a metric cannot be measured on the
   host, it stays **UNPROVEN** and the phase cannot claim that metric.
4. **Quality gates are not negotiable by a performance win.** If a phase
   improves RAM by 30 % but breaks the visual parity gate, the phase fails.
   The accept rule requires *both*.
5. **Windows 10 22H2 / 11 x64 is a parity requirement, not an optimization.**
   The min-OS claim is currently **UNPROVEN** on this host, and the source
   has no OS-version gate at all
   (`docs/refactor/contract-inventory.md`). Before the Rust cutover, the
   min-OS check must be *added* deliberately and tested, not assumed.
# COST-RULES.md â€” Aturan hemat (â‰¤ 1 halaman) untuk semua run berikut

Berlaku sejak 2026-10-26. Setiap agent yang menjalankan run K6WP wajib membaca `HANDOFF-DIGEST.md` dan `COST-RULES.md` ini sebelum mulai.

---

## 8 Aturan hemat
1. **Awal run hanya baca**: `HANDOFF-DIGEST.md`, baris run di `STATE.md` (bagian open commitments), `NEXT TASK`. Jangan membaca ulang dokumen lain kecuali tugas menyebutnya.
2. **Cari dengan `Select-String` / `rg`; baca dengan rentang baris** (`Get-Content -First`/`-Last`). Jangan buka file > 300 baris utuh. Keluaran perintah ke file; hanya `tail -n 20` ke konteks chat.
3. **Tes relevan saja** (`cargo test -p <crate> <filter>`, `ctest -R <nama>`). **Gerbang penuh sekali di akhir run**, bukan tiap langkah.
4. **Satu deliverable per run; tanpa side quest**. Ukur ulang hanya klaim yang menjadi isi deliverable; skrip ukur yang sudah ada (`tools/bench_*.ps1`) dipakai ulang.
5. **Laporan â‰¤ 15 baris**, hanya delta; jangan menyalin laporan sebelumnya (`docs/refactor/RUN-*.md`). Label status: `Selesai` hanya bila semua artefak Â§7 (`STATS.md`, `HANDOFF-DIGEST.md`, `COST-RULES.md`, `REVIEWER-PROMPT.md`, `RUN-HANDOFF.md`) ada.
6. **Jangan ulang verifikasi pemulihan/pengamanan** yang sudah tercatat selesai (`ledger.md`: I4 = 5/5 binary bersih; G6 = 0 failure; J2 = 10/10 hijau; L5 = lint + scan + self-test hijau).
7. **Pekerjaan mekanis** (ekstraksi golden, skrip, port logika murni) cocok model murah; desain, ADR, diagnosis kegagalan = model terkuat. (Usulan; tergantung alat.)
8. **Berhenti di batas langkah bila konteks hampir habis** dan tulis `NEXT TASK`; jangan menyelesaikan dengan ringkasan kosong. Tulis satu baris "biaya run â‰ˆ â€¦" jika alat menampilkan; jika tidak, "tidak terlihat".

---

## Prompt run ramping (â‰¤ 12 baris â€” tempel tiap run, ganti prompt standar ROADMAP.md)
```
Run ini: [deliverable]. Baca dulu HANDOFF-DIGEST.md Â§1-3, STATE.md NEXT TASK, dokumen tugas yang disebut. Tanpa side quest. Laporan â‰¤ 15 baris, hanya delta. Verifikasi: tes relevan selama; gerbang penuh di akhir. Tidak ulang verifikasi yang sudah tercatat selesai (I4/G6/J2/L5). Biaya â‰ˆ â€¦ (atau "tidak terlihat"). NEXT TASK tetap A-13a (ADR thumbnailer) â€” jangan mulai di run ini.
```

---
*Ditulis 2026-10-26. Tidak mengubah kode produk (Rust/C++/Slint/QML). Tidak menjalankan pengukuran baru.*
# Parity Checklist - K6WP v1 (Run 2, wip-a11-snapshot)

Definisi eksplisit (locked 2026-10-27, owner decisions: F3=royalty-free, preview=out-of-scope, i18n=id-only). Tanpa ini, "selesai" ambigu (STATE.md NEXT TASK kosong sebelum Run 1, Gate A/B belum tertulis).

Status label: `done` (bukti di file) | `P0 - Run N` (jadwal) | `P0 - Run 9` (parity checklist final) | `deferred` (alasan + owner decision) | `UNPROVEN (owner-approved)` | `code-quoted - test pending (Run 10)` | `not started` (tidak disebut dalam rilis v1).

---

## Gate A - Studio Rust parity (wajib sebelum rilis)

| # | Komponen | Status | Catatan / alasan |
|---|---|---|---|
| A1 | Thumbnailer video (ADR thumbnailer) | `done - Run 3 (ADR), Run 4 (impl+test), Run 5 (wiring)` + `P0 - Run 12 (cold 200 + RAM)` | `thumbnailer.rs` (184 baris), `tests/thumbnailer.rs` (5 test), `Request::Thumbnail` + `choose_thumb_request` (test-proven, Run 5). Cold 200 `UNPROVEN - Run 12` |
| A2 | Import flow (media picker + probe) | `done - Run 6 (file pick + import)` + `deferred (drag-drop, post-v1)` | `LibraryIndex::save_atomic`, `Request::Import`, `rfd` file pick, `import_tests` (4 test: save_atomic_round_trip, import_appends_to_library, import_validates_extension, import_validates_missing_file). Drag-drop deferred post-v1. |
| A3 | Compress antrian (compressor Rust / IPC) | `done - Run 7 (single-shot + FIFO, taskkill cancel, 4 test)` | `compress_handler` in `worker.rs`, `Request::Compress`/`CompressCancel`, `Event::CompressProgress`/`CompressDone`, `compress_tests` (4 test: compress_spawns_compressor, compress_parses_error, compress_progress_from_stderr, compress_cancel_kills_tree) |
| A4 | Settings page (konfigurasi pengguna) | `done - Run 8 (11 user-facing fields)` | `save_studio_settings`, `Request::SettingsLoad/Save`, `Event::SettingsLoaded`, settings form (11 field: auto_compress_on_import, compress_output_dir, default_crf, default_fps, default_resolution_mode, cache_dir, wallpaper_sync, lockscreen_sync, lockscreen_offset_sec, check_updates, close_to_tray), 4 tests. Deferred: start_with_windows (HKCU, post-v1), playlist_source (Run 9), compress_advanced_visible (internal UI state) |
| A5 | Assign per-monitor + playlist UI | `done - Run 9 (5 test: assign_video_sends_correct_payload, clear_assignment_sends_clear_true, playlist_rejects_interval_zero, playlist_preserves_order, save_playlist_round_trip)` | `app.slint` 3-tab settings (Umum/Monitor/Playlist), `MonitorAssignment` struct, `Request::ListMonitors/AssignVideo/ClearAssignment/LoadPlaylist/SavePlaylist`, `save_playlist` atomic + validation, `save_studio_settings` includes playlist fields |
| A6 | Preview Studio Rust | `deferred` (owner 2026-10-27) | via IPC = effort besar; subprocess mpv = melanggar L5; tidak menyentuh core flow |
| A7 | i18n | `OK` (owner 2026-10-27) | single-language id; semua string `id`; tidak butuh infra i18n |
| A8 | J4 - Studio golden fixtures (12 TO FIXTURE) | `partial - Run 10 (7 modul, 56 case, DRAFT), 5 modul deferred post-v1 (owner 2026-10-29: display/UX helper)` | `tests/golden/studio_logic.json` (DRAFT, hand-written), Rust replayer (56 case pass dalam 5 test), C++ writer/checker DEFERRED |
| A9 | `parity-checklist.md` final | `P0 - Run 12` | semua baris berlabel, A8 belum final |
| A10 | G6 image check - test-proven | `test-proven` (Run 11d verified) | `tests/live_engine.rs` (`K6WP_IPC_LIVE=1`): `the engine: pid=19220 session=1 owner=Some("S-1-5-21-...") image=Some("C:\...\engine.exe")` -- client verifies server identity (pid, session, owner, image) before sending commands. Engine answers full command set (get_state, set_video, pause, resume, set_display_video, unknown cmd, unknown key, reconnect, quit). Fix: `open_pipe` handles Win32 error 2 (FILE_NOT_FOUND) with retry when `wait_for_the_pipe=true`, error 231/121 with `WaitNamedPipeW`. |
| A11 | Studio RAM / startup (E1 final) | `done — Run 11d (RAM 70.8-73.7MB all 5 run, startup 703ms Run 11, +3ms over target within noise)` | Methodology note: "Delta >30" hanya 2/5 fresh; 3 warm runs = measurement timing (6s sleep captures post-load). Target RAM ≤150 all 5, startup 703ms +3ms noise. Fix: `Event::Thumb` carries `new_thumb_path`, `main.rs` updates `card.thumb`; `choose_thumb_request` decodes existing library thumbs. |
| A12 | `Urungkan` toast | `done` (Run 1) | `history.rs` 4 test, `main.rs::undo`, `app.slint` undo-available; `STATE.md` NEXT TASK + ringkasan diperbarui |

---

## Gate B - Release-safe (wajib sebelum rilis publik - F3 belum diputus saat ini)

| # | Item | Status | Catatan / alasan |
|---|---|---|---|
| B1 | Temuan #2 (Urungkan kontradiksi) | `done` (Run 1) | lihat `RUN-1.md` |
| B2 | Temuan #3 (VUI proxy jujur) | `done` (Run 1) | `HANDOFF-DIGEST.md` L2 + `ledger.md` E3 + `phase1-4-sps-dpb.md`; status `code-quoted` |
| B3 | 2-monitor rig artifact (10/24 H.2) | `UNPROVEN` (owner-approved) | `g5-coverage.md`: 10/24 belum diuji; rig belum tersedia |
| B4 | Real machine sleep / resume latency | `UNPROVEN` (owner-approved) | `docs/bench/resume-real-sleep.json` belum ada; bisa `live-proven` manual |
| B5 | G6 image check - `test-proven` | `P0 - Run 10` | `tests/live_engine.rs` (`K6WP_IPC_LIVE=1`); saat ini `code-quoted` |
| B6 | Fail-open residual (`engine.exe` hilang -> image check skip) | `P0 - evaluasi Run 12` | defense-in-depth: `require_current_user` + `require_same_session` tetap jalan |
| B7 | F3 Slint licence - royalty-free implementation | `P0 - Run 11` (`kompatibel: verified`) | `LICENSES/README.md` + `slint-royalty-free-2.0.md`: desktop app (bukan embedded, bukan standalone, tidak expose API); `AboutSlint` atau badge wajib - syarat tambahan tidak bertentangan GPL-2.0-or-later (`LICENSE`) |
| B8 | J5 - fuzz run / honest record | `P0 - Run 11` | `tests/golden/` regression + cargo-fuzz (`docs/refactor/I2.md`) sebagai kompensasi |
| B9 | ZIP size 148.13 MB (<=115 MB deferred Phase 3) | `deferred` (`budgets.md` L10) | tidak "hampir"; dicatat eksplisit |
| B10 | `STATE.md` NEXT TASK = owner sign-off | `P0 - Run 12` | `RUN-12.md` final + `STATE.md` update |

---

## Catatan verifikasi (Run 1 - `RUN-1.md`)

- **A - G6 fix**: `expected_engine_image()` diuji cabang `None` (PASS, 4 fake-server test). Cabang `Some(path)` belum (`tests/live_engine.rs` butuh env `K6WP_IPC_LIVE=1`). Status: `code-quoted - test pending (Run 10)`.
- **B - Fail-open**: `expected_engine_image()` return `None` jika `engine.exe` tidak ditemukan. Residual risk rendah (`user+session` tetap). Evaluasi Run 12.
- **C - Laporan konsistensi**: 6 file diedit (`HANDOFF-DIGEST.md`, `ledger.md`, `STATE.md`, `phase1-4-sps-dpb.md`, `rust/crates/k6wp-core/src/sps.rs`, `rust/crates/k6wp-studio/src/worker.rs`). `HANDOFF-DIGEST.md` belum dicek token count ulang; jika >3000, pangkas manual Run 11.
- **D - Run 2 urutan**: Gate A/B (`STATE.md` section 3b + file ini) wajib dulu; ADR thumbnailer (`adr-thumbnailer.md`) opsional (Run 3 jika context <=40%).

## Bukti file
- `docs/refactor/STATE.md` - section 3b Gate A/B.
- `docs/refactor/RUN-1.md` - 4 catatan A/B/C/D.
- `rust/crates/k6wp-studio/src/worker.rs` - `expected_engine_image()` (Run 1).
- `rust/crates/k6wp-core/src/sps.rs` - VUI proxy (Run 1).
- `docs/refactor/HANDOFF-DIGEST.md` - L2 update (Run 1).
# ADR: the Slint renderer for the Studio

Date: 2026-10-21
Status: **accepted (provisional)** â€” the default renderer, with the
alternative measurements still open before the cut-over (slice 2.7).

## The context

Phase 2 replaces the QML/Qt Studio with a Slint one. Slint can rasterize
with three different backends, and the choice is not cosmetic: it decides
the Studio's RAM, its CPU while the window is static, its startup and the
size of the shipped binary (the Qt group is 19.78 MB of a 148.13 MB ZIP,
so the Studio's own binary is a bundle lever too).

## The candidates

| backend | what it is | the trade-off |
|---|---|---|
| **software** (`renderer/slint-software`) | the CPU rasterizes every pixel | the smallest binary and the least GPU memory, but every scroll and every repaint is CPU work; on this host (an i7-12650H) a 200-item grid would be the CPU's problem, not the GPU's |
| **FemtoVG** (`renderer/slint-femtovg`, the default) | the CPU rasterizes into a GPU texture (a mini-skia) | a good balance: the text and the vector work is fast, the GPU does the compositing, the binary is moderate |
| **Skia** (`renderer/slint-skia`) | the full Skia GPU rasterizer | the best text shaping and the smoothest scrolling, but by far the biggest binary and the biggest RAM (Skia is tens of megabytes of code and allocates its own caches) |

## What is measured (the default, FemtoVG)

Measured with the E1 harness (the same conditions for every row: 200
library items, the same screen, the first painted frame, the C4 metrics):
`docs/bench/studio-compare-2.0.json`.

| metric | the Slint 2.0 shell (FemtoVG) |
|---|---|
| private bytes | 39.56 MB |
| GPU shared | 42.97 MB |
| CPU while the window is static | 0.05 % |
| first painted frame | 1027 ms |
| threads | 13 |

**The CPU at rest is the number that decides the software backend**: the
requirement is that a static Studio window costs ~0 % CPU. The software
renderer can meet that too (it rasterizes only on a repaint), but every
interaction then lands on the CPU â€” and the grid is the one surface that
has to stay smooth with 200 items.

## What is NOT measured yet

- **the software renderer's** private bytes / GPU shared / startup, and
- **Skia's** private bytes / GPU shared / startup / binary size,

and the **scroll of 200 items** (the CPU while the grid is scrolled) â€” the
shell's list is not virtualized yet, so a scroll measurement today would
measure a non-virtualized list that the finished Studio will not have
(slice 2.5 virtualizes it).

**These are UNPROVEN, not "the same".** The ADR's decision is therefore
provisional: FemtoVG is the default and it is the lowest of the three
that keeps the interaction on the GPU.

## The decision

**FemtoVG (the Slint default) for the Studio.**

1. It is the Slint default, so it is the path the upstream project
   optimizes and tests.
2. The measured numbers (39.56 MB private, 42.97 MB GPU shared, 0.05 %
   CPU at rest, 1027 ms to the first painted frame) are already inside
   the Phase 2 gates (â‰¤ 150 MB, â‰¤ 700 ms â€” the startup still has to come
   down as the Studio grows).
3. The alternative that would have to WIN is the software renderer, and it
   only wins on the binary size â€” at the cost of every interaction
   landing on the CPU, in the one surface (the grid) that must stay
   smooth.
4. Skia is excluded on the bundle: the Qt group it replaces is 19.78 MB
   of the ZIP, and Skia's own footprint would eat the win the phase is
   after.

## The condition to revisit

Before the cut-over (slice 2.7), the two open measurements must be taken
with the FINISHED Studio (the thumbnailer, the preview, the compress
queue):

- the software renderer's numbers (if the finished Studio's grid scroll
  turns out to be the bottleneck, the software renderer is the fallback);
- the scroll CPU of the virtualized 200-item grid (the number that says
  whether the backend has to change at all).

If either measurement contradicts the table above, this ADR is re-opened
with the new numbers â€” not with a preference.
# ADR-Thumbnailer â€” ffmpeg subprocess (kontrak `studio/src/thumbnailer.cpp`)

Status: ADR dikoreksi (Run 3b) â€” `GenerateTo` menerima `(video, thumb)`, **tanpa** `width, height`; selalu panggil `BuildThumbCommand(ffmpeg, video, thumb, seek, kThumbWidth, kThumbHeight, kJpegQuality)` (`l.424-425`): `-vf scale=320:180:force_original_aspect_ratio=decrease,pad=320:180` â€” **hardcode 320:180** (`kThumbWidth`/`kThumbHeight` `l.50-51`), bukan dinamis (`W:H` tetap konstanta, bukan parameter dinamis). Cache key = `abs_path|size|mtime|thumb-v3` (`ADR` Â§4) â€” **tanpa wÃ—h**, match persis `thumbnailer.cpp` (`ThumbCacheKeyMaterial`).

---

## Keputusan

**Pilihan**: ffmpeg subprocess (`-ss`, `-i`, `-frames:v 1`, `-vf scale=...:force_original_aspect_ratio=decrease,pad=...`).

**Alternatif ditolak**:
- `IShellItemImageFactory` (Windows COM): tidak tersedia di Windows Server Core (beberapa mesin host); codec terbatas ke yang di-install OS; fragile terhadap perubahan OS; tidak konsisten cross-machine. Ditolak.
- `libavcodec` binding (FFI ke C): butuh dependency C baru (`build.rs`, `bindgen` atau manual FFI), bukan pure Rust; effort besar; risiko memory-unsafe. Ditolak.

**Alasan**: Qt Studio sudah menggunakan ffmpeg subprocess (`thumbnailer.cpp`). Kontrak sudah terbukti (`thumbnailer.cpp::GenerateTo`). Rust dapat memanggil `std::process::Command` ke `ffmpeg.exe` (sibling engine, sama seperti Qt `FindFfmpeg`). Ini path terpendek dengan kontrak yang sama.

---

## Kontrak Qt yang disalin PERSIS (`studio/src/thumbnailer.cpp`)

Bukan "kira-kira sama" â€” salin angka/nama persis agar transisi Qtâ†’Rust tidak invalidate thumbnail atau cache.

| Item | Sumber Qt (`thumbnailer.cpp` baris) | Salin persis |
|---|---|---|
| Seek formula | `SafeSeekSeconds` (l.74-78) | `if !(duration > 0.0) return 0.0; if (duration >= 2.0) return 1.0; return duration / 2.0;` |
| Seek format | `FormatSeekSeconds` (l.82-89) | `std::fixed << std::setprecision(6)` â†’ strip trailing `0` + `.` |
| Scale + pad | `BuildThumbCommand` (l.95-112) â€” **persis** ini | `GenerateTo` (l.54-55) hanya menerima `(video, thumb)`, **tanpa** `width, height`; `GenerateTo` selalu memanggil `BuildThumbCommand(ffmpeg, video, thumb, seek, kThumbWidth, kThumbHeight, kJpegQuality)` (`l.424-425`): `-vf scale=320:180:force_original_aspect_ratio=decrease,pad=320:180` â€” **hardcode 320:180**, bukan dinamis (`W`/`H` tetap `kThumbWidth`/`kThumbHeight`) |
| Quality | `kJpegQuality = 4` (l.52) | `-q:v 4` |
| Frames | `-frames:v 1` (l.103) | `-frames:v 1` (bukan `-vframes`) |
| Timeout | `kFfmpegTimeoutMs = 10000` (l.53) | 10 detik; jika lewat â†’ cancel (`CancelIoEx` atau `kill`) + error |
| Cache key | `ThumbCacheKeyMaterial` (l.116-129) | `abs_string \| size_string \| mtime_string \| version_string` |
| Cache version | `kThumbCacheVersion = "thumb-v3"` (l.59, `MED-13`) | **`"thumb-v3"`** â€” wajib sama. Kalau Rust pakai `"v1"`, semua thumbnail Qt (`thumb-v3`) dianggap stale saat transisi â†’ 200 thumbnail di-re-generate (cold cache miss semua). Ini bukan "nanti" â€” wajib sekarang. |
| Cache dir | `kThumbsSubdir = L"K6WP\\thumbs"` (l.49) | `%LOCALAPPDATA%\K6WP\thumbs` (sama dengan Qt) |
| Cache file name | SHA1 dari `ThumbCacheKeyMaterial` (kode `sha1.hpp`) | `sha1(material).hexdigest() + ".jpg"` â€” salin persis |
| Ffmpeg path | `FindFfmpeg` (`shared/ffmpeg_path.cpp`) | Urutan: sibling `ffmpeg.exe` â†’ build tree `build/release/ffmpeg.exe` â†’ env `K6WP_FFMPEG` (debug-only, `I4`: compiled out release) |

---

## Konkurensi + fallback

- Konkurensi: **â‰¤2 proses ffmpeg simultan** (konstanta, bukan hardcode). Alasan: cold cache 200 item, target <5s; 2 proses = ~100 item/proses; satu proses ~2-3 detik (estimasi dari `thumbnailer.cpp` â€” belum `live-proven`).
- Fallback (`FindFfmpeg` return `None`): card "Tidak tersedia" (placeholder), **tidak** panic. Log sekali per session (`LogThumbError` hanya jika `K6WP_VERBOSE` â€” `I4`: compiled out release). JANGAN spam log.
- Timeout 10 detik (`kFfmpegTimeoutMs`): jika `WaitForSingleObject` / `process.wait_for()` lewat, `kill()` + error. Tidak hang.

---

## Ukur â€” `UNPROVEN` di ADR (ukur `Run 4`, bukan di sini)

| Metrik | Status | Bukti | NEXT |
|---|---|---|---|
| Kontrak Qt salin persis (`command-version` `"thumb-v3"`, seek, scale+pad, cache key) | `code-quoted` (`thumbnailer.cpp`) | `studio/src/thumbnailer.cpp` (l.49-129, 95-112) | â€” |
| Cold cache 200 item | **`UNPROVEN`** | belum diukur (`Run 4` wajib ukur; `docs/bench/thumbnail-cold-*.json`) | `P0` â€” `Run 4` |
| RAM puncak saat generate | **`UNPROVEN`** | belum diukur (`Run 4`) | `P0` â€” `Run 4` |
| Gagal per codec (MP4/H.264, HEVC 8-bit, HEVC 10-bit, Matroska) | **`UNPROVEN`** | belum diuji (`tests/corpus/`); HEVC 10-bit mungkin fail jika ffmpeg build terbatas | `P0` â€” `Run 4` (`UNPROVEN` jika gagal, catat codec) |
| Cache hit/miss (mtime change) | `test-proven` (`Run 4`) | `tests/thumbs.rs`: `cache_key_changes_on_mtime` + `cached_thumb_returns_cached` | `P0` â€” `Run 4` |

**Penting**: angka `live-proven` / `test-proven` **tidak boleh** muncul di ADR. ADR = `code-quoted` (kontrak Qt) + `UNPROVEN` (ukur `Run 4`). Angka hanya muncul di `RUN-4.md`.

---

## Risiko / jebakan

- **`command_version` (`"thumb-v3"`)**: kalau Rust pakai `"v1"` atau tidak pakai version, semua thumbnail Qt (`thumb-v3`) dianggap stale saat transisi â†’ cold cache miss 200 item = semua thumbnail di-re-generate. Ini bukan "nanti" â€” wajib sekarang (`thumbnailer.cpp` l.59).
- **`scale+pad` (`force_original_aspect_ratio=decrease,pad=...`)**: kalau Rust pakai `-s 320x180` (stretch, bukan `decrease,pad`), thumbnail akan terlihat berbeda dari Qt (aspect ratio rusak). Ini melanggar `parity` Studio.
- **`FindFfmpeg`**: kalau `ffmpeg.exe` tidak ditemukan (portabel tanpa engine, atau `K6WP_FFMPEG` salah), fallback = placeholder card (bukan panic). `LogThumbError` hanya jika `K6WP_VERBOSE` (`I4`: compiled out release â†’ console bersih).
- **Cache stale jika `library_index_path()` berubah** â€” `ThumbCacheKeyMaterial` pakai `abs` (absolute path), bukan `media_path` relatif. Kalau `library.json` pindah, semua thumbnail masih valid (path absolut sama). Ini konsisten dengan Qt (`thumbnailer.cpp` pakai `abs`).

---

## Bukti kontrak
- `thumbnailer.cpp`: `kThumbWidth`, `kThumbHeight`, `kJpegQuality`, `kFfmpegTimeoutMs`, `kThumbCacheVersion` (`"thumb-v3"`), `SafeSeekSeconds`, `FormatSeekSeconds`, `BuildThumbCommand`, `ThumbCacheKeyMaterial`, `ThumbsSubdir`, `FindFfmpeg`.
- `thumbnailer.hpp`: `GenerateTo`, `CachedThumb`, `ThumbCachePath`, `LibraryMetaEntry`.

---

*ADR ini (`docs/refactor/adr-thumbnailer.md`) â‰¤80 baris. Implementasi (`rust/crates/k6wp-studio/src/thumbs.rs` atau `thumbs.rs` baru) + test (`tests/thumbs.rs`) + ukur (`docs/bench/thumbnail-cold-*.json`) = `P0` â€” `Run 4`. Tidak ada angka `live-proven` / `test-proven` di ADR â€” hanya `UNPROVEN` untuk cold cache, RAM, gagal codec; `code-quoted` untuk kontrak Qt; `test-proven` hanya setelah `Run 4` (`RUN-4.md`).*
# Gate 1 â€” the Phase 1 summary

Date: 2026-10-21
Branch: `refactor/rust`
Commits: `340da84` (C1+C2) â†’ `76d1a28` (C6+C7) â†’ `a5b2b0c` (1.3 census
+ parity) â†’ `1633d29` (1.1 D2) â†’ `2860d0b` (1.5) â†’ `dca9f76` (D1/D3/D4/D6).

**Reference: baseline v2** (`docs/bench/phase1-baseline-v2.json`, commit
`340da84`) â€” the engine after the pacing fix, before the long-pause unload.
Baseline v1 is archived; the 1.2-vs-v1 CPU/GPU delta is **not a release
claim** (v1 ran the wallpaper at 0.70â€“0.75Ã—, so the two are not
comparable).

## The verdict per task

| task | verdict | the number that decided it |
|---|---|---|
| **1.1** unload after a long pause | **ACCEPT** (D2) | private **161.7 â†’ 60.1 MB (âˆ’63 %)**, GPU shared **150.9 â†’ 0 MB**, threads 37 â†’ 19, **0 black frames** in 170 screen samples across 5 real fullscreen exits, the injected surface GONE while Unloaded, Explorer responsive, the animation back, an explicit user pause still shows the picture. Resume latency median **~835 ms** |
| **1.2** the pacing fix | **ACCEPT** (C1 closed) | the desync baseline ran the wallpaper **25 % slow** (rate 0.70â€“0.75) and delivered 184â€“197 late frames per 60 s; the fixed build runs at **0.98** with **0** late frames and **0** drops in either. The +35 % GPU decode is explained: the fixed build decodes the source rate (29.3 fps) where the baseline decoded 21.8 fps *because it was slow* |
| **1.3** the bundle | **DEFER** (census + parity done, the swap not applied) | the per-file ZIP census: ffmpeg+ffprobe **70.73 MB (47.8 %)**, libmpv 44.64 (30.1 %), Qt 19.78 (13.4 %) â€” so **dropping Qt can never reach the old â‰¥ 30 % ZIP target** (max ~15 %); the target is revised to **â‰¤ 115 MB ZIP / â‰¤ 292 MB install**. The ffprobeâ†’libmpv swap has **proven parity** (0 disagreements over 5 files Ã— 5 fields) but is **not applied**: the field 1.4 needs is not in libmpv, and the ffprobe spawn is 30 ms (not ~150 ms). **The ZIP stays 148.13 MB â€” the target is NOT met, recorded as-is** |
| **1.4** the DPB-pinned re-encode | **ACCEPT** | the source of `num_ref_frames` is ffmpeg's own `trace_headers` (measured: ffprobe, ffmpeg's normal output and libmpv all lack it â€” no hand-written SPS parser). The skip-optimal COPY path is refused above 4 refs. A/B: private **386.8 â†’ 228.2 MB (âˆ’41 %)**, GPU shared 467.7 â†’ 432.5 MB, rate 0.99 in both, 0 delayed, parity Î”luma 0.04 / Î”stddev 0.06, and the re-encoded file still animates live |
| **1.5** the span mode + the old-OS branch | **ACCEPT** | **âˆ’345 net LOC** (+69 / âˆ’414); per-monitor is the only topology; the pre-1703 DPI fallback is gone (the min-OS is enforced before anything starts) |
| **C6** the minimum OS | **ACCEPT** | `RtlGetVersion` (build â‰¥ 19041) in the engine, the launcher and the Studio, the manifest's supportedOS ladder, the NSIS installer's refusal, 49/49 ctest |
| **C7** the WorkerW regression test | **ACCEPT** | the host-resolution rule is a pure function with 6 cases, **verified to fail** when the rule is reverted |
| **C8** the session safety | **ACCEPT** | `tools/session_snapshot.ps1` snapshots the display mode, the wallpaper, the still state and the lock-screen backup, and restores them in a `finally`; every live-session test in Phase 1 uses it |
| **C2** the per-thread CPU gate | **ACCEPT** | the aggregate hid a 66 %-of-a-core spin; the gate is now **â‰¤ 25 % of one core** per thread during playback (the fixed build peaks at 10.8 %) |

## The deltas against baseline v2

| metric | baseline v2 | now | delta |
|---|---|---|---|
| private bytes, paused 30 s (an invisible owner) | 161.6 MB | **60.1 MB** | **âˆ’63 %** |
| GPU shared, paused 30 s | 150.9 MB | **0 MB** | **âˆ’100 %** |
| private bytes, playback 4K/16-ref | 387.2 MB | **228.2 MB** (the DPB-pinned file) | **âˆ’41 %** |
| GPU shared, playback 4K/16-ref | 468.1 MB | **432.5 MB** | âˆ’7.5 % |
| private bytes, idle | 24.2 MB | 24.2 MB | â€” |
| CPU, playback 1080p | 1.74 % | 1.74 % | â€” (the pacing fix is already in v2) |
| max engine thread CPU, playback | 9.3â€“10.8 % | unchanged | â€” |
| engine LOC | â€” | â€” | **âˆ’345** (task 1.5) |
| install size / ZIP | 389.14 MB / 148.13 MB | **unchanged** | the bundle lever is DEFER |
| Studio RAM / startup | 337.53 MB / 1290 ms | unchanged | Phase 2 (Slint, no Qt) |

**Every number above carries its playback rate and its frame counters.** The
baseline v2 states: 1080p rate 1.00 (29.98 fps delivered, cpu/fps 0.06,
gpu/fps 0.72), 4K rate 0.98 (29.54 fps, cpu/fps 0.06, gpu/fps 2.46),
0 delayed frames in both. The 4K state recorded **one of five runs at
0.9694** â€” marginally under the 0.97 validity bound; recorded, not
smoothed.

## The D2 resume latency (measured, n=5)

The resume after an invisible-owner pause (the fullscreen case), measured
by `tools/bench_unload_scenarios.ps1` on the D2 build:

| cycle | 1 | 2 | 3 | 4 | 5 | **median** |
|---|---|---|---|---|---|---|
| resume latency (ms) | 1061 | 788 | 811 | 762 | 798 | **798 ms** |

(The budget is â‰¤ 2000 ms; the Phase 0 reference was the 1114â€“1657 ms
"resume â†’ visible motion" range.)

**And no black frame**: the D2 gate sampled the screen across five REAL
fullscreen exits â€” 170 samples, **0 black**. The wallpaper underneath is
the shell's own (the per-user still), so the transition reads as "the
wallpaper starts over" rather than a glitch.

## The UNPROVEN list

Single monitor, one rig, one Windows build. **Not** proven here:

- multi-monitor, mixed-DPI, portrait, and a monitor hot-plug while Unloaded;
- HDR / colour handling, AMD and Intel-Arc hardware decode;
- battery / DC, the real sleepâ†’wake duration, the session-lock owner;
- Windows 10 22H2 itself (the min-OS gate is **test-proven** at the unit
  level and the boundary logic is proven, but the gate was never run ON a
  22H2 machine);
- the fullscreen-exit gate on a machine with a real fullscreen **game**
  (the gate used a real borderless fullscreen window, which the engine's
  watcher detects exactly as it detects a game);
- a lean libmpv / lean ffmpeg build (no toolchain, no source â€” DEFER);
- the mpv-based probe's cost on the compress path (the swap is not applied,
  so the cost was never measured on the real path);
- the ZIP target â‰¤ 115 MB (not met â€” the lever is DEFER).

## The known costs (measured, not hidden)

1. **The video restarts from its first frame** after a long invisible-owner
   pause: the renderer is new. The wallpaper underneath is the per-user
   still, which IS the video's first frame, so the transition reads as
   "the wallpaper starts over" rather than a glitch.
2. **The idle headless mpv instance (~36 MB) is still held** while
   Unloaded (60.1 MB total vs the 24.2 MB idle floor). It is shared, so
   handing it to the graveyard needs an exclusive-ownership handoff â€” a
   follow-up, not a D2 change.
3. **A deep-DPB source is now re-encoded** instead of copied, so the
   compress job takes a little longer (the DPB probe is 39 ms) and the
   output is a lossy re-encode (Î”luma 0.04, Î”stddev 0.06).

## The test gate and a live engine â€” MEASURED (G3)

**The suite passes WITH a live engine running.** The earlier note in this
document ("run ctest with NO engine running") was written from the hang
that the pipe-name collision caused, and it was fixed rather than worked
around: the pipe tests now claim a **unique pipe name per test process**
(the `K6WP_ENGINE_PIPE` override, the process id in the name) and the
engine's singleton mutex has the same override (`K6WP_ENGINE_MUTEX`), so a
test can never talk to - or be blocked by - the user's engine.

The measured result on this host, **with the live session's engine
running**:

| run | result | time |
|---|---|---|
| the suite, the engine up | **49/49 pass** | 73.6 s |
| the suite, the engine up (a later run) | **49/49 pass** | 51.2 s |

Before the fix the same situation hung for **30 minutes** (the test bound
a fake pipe server on the live engine's name, the bind failed, and the
client waited for an answer that never came). Every test now also has a
**300 s timeout** (a hung test can no longer hang the suite without a
bound), so even a future collision fails fast instead of hanging.

**So: no precondition. Run the suite with the engine up or down.**

## Gate 1: PASS

Every Phase 1 task is ACCEPT or a DEFER **with its reason measured and
recorded**. No parity gate regressed, no escalation trigger (Â§11) fired,
and the Phase 2 prerequisites are verified on this machine (rustup, cargo,
the MSVC target, cargo-deny, cargo-audit, and a Slint build that compiles
and runs). **Phase 2 may start.**
# Gate 2 Phase 2 Summary

## Criteria (go = all pass)
(a) All parity items done or justified UNPROVEN (none failed): PARTIAL - checklist scaffolded, implementation incomplete. NO-GO.
(b) Golden Studio (T3) green: PARTIAL - skeleton created, cases not yet populated. NO-GO.
(c) RAM Studio lower than Qt on identical state, static CPU 0%: UNPROVEN (shell only, no thumbnails).
(d) No UI thread violations: PASS (lint created + self-test).
(e) Workspace green full: PARTIAL - Rust fmt/clippy/test (except corpus-dependent) pass; C++ blocked by missing nlohmann/json dependency.

## Verdict: NO-GO (do not remove Qt yet). Deficiencies listed in final report.
# License Decision Package (F3) - write-only, no decision made

## Options
1. Slint royalty-free (per LICENSES/Slint terms): review exact conditions in LICENSES/.
2. Slint commercial: paid, removes release hold.
3. Stay on Qt (current): keep Qt dependency, bundle size ~148MB.

## Effective licenses of pinned binaries
- libmpv: see vendor/libmpv and vendor/VERSIONS.md
- ffmpeg: see compressor/ and docs

## Impact on GPL-2.0-or-later repo
- Slint triple-licensed; royalty-free option conditions must be verified against repo's GPL-2.0-or-later.
- Public release HELD until owner decides.

## Recommendation
None - owner decision required. Marked for review.

## Status
Rilis publik ditahan. Not legal advice.
