# L1 - the `get_state` max: the once-per-second snapshot spike

Task L1 (v1.10). K1 cached the thread count behind a 1 s TTL; the p99 fell
to 3-4 ms but the **max** stayed at 121-152 ms under the churn stress. This
file proves what that max was, removes it, and records the gate.

---

## 1. The hypothesis, and why it was testable

The K1 trace had already shown that the `get_state` JSON build was dominated
by `CurrentThreadCount()`: a `CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0)`
walks **every thread of every process** in the machine (75 ms per call on
this host). K1 wrapped it in `CurrentThreadCountCached()` (a 1 s TTL), so the
snapshot ran **inline, on the handler, once per second**.

If that is the max, the slow samples must be **periodic with a ~1 s spacing**,
and they must disappear when the call is gone. Both were measured.

## 2. The A/B that proves it

Two binaries, **the same source tree, the same build directory, the same
compiler and flags**; the only difference is the L1 edit:

| binary | `thread_count` in the image | the trace scopes | the difference |
|---|---|---|---|
| `build/l1-before/engine.exe` | **yes** | yes | HEAD (the K1 cache, unchanged) |
| `build/l1/engine.exe` | **no** | yes | HEAD + L1 (the field and its callers deleted) |

The check is a byte scan of the two PE files (`thread_count` is a literal):
before `thread_count=True`, after `thread_count=False`; both contain
`ipc-trace`, so the instrumentation is on both sides.

The harness is `tools/bench_get_state_max.ps1`: it starts a **second** engine
beside the user's live one, with a unique singleton mutex and pipe (the
debug-only overrides) and a config with **no video** and `monitor_id: 99` (no
display matches, so the instance owns zero wallpaper slots and paints
nothing). The live session is not touched.

The load: **1 client, 4000 `get_state` requests per run, n = 5 per binary**,
the trace **off** (the trace's own summary write lands inside the scope -
see section 6). Both sides ran back to back on the same machine.

### The result

| run | before: p50 / p99 / max | after: p50 / p99 / max |
|---|---|---|
| 1 | 0 / 1 / **69** ms | 0 / 1 / **5** ms |
| 2 | 0 / 0 / **51** ms | 0 / 0 / **4** ms |
| 3 | 0 / 0 / **56** ms | 0 / 0 / **7** ms |
| 4 | 0 / 0 / **50** ms | 0 / 1 / **7** ms |
| 5 | 0 / 0 / **57** ms | 0 / 0 / **14** ms |
| **median** | p50 0, p99 0, **max 56** (50-69) | p50 0, p99 0, **max 7** (4-14) |

No failures on either side (20,000 requests each). The raw per-run JSONs are
in `docs/bench/l1/`.

### The shape of the spike (the proof of the cause)

Run 1 of each side also wrote **every** sample (`--samples`):

- **before**: 4000 samples, **2 samples >= 20 ms** -
  at offset **965 ms -> 69 ms** and offset **1946 ms -> 50 ms**.
  The spacing is **981 ms**, i.e. the 1 s TTL of the K1 cache. The p99.9
  is 7 ms, so the max is not the tail of a distribution; it is a **periodic
  event**.
- **after**: 4000 samples, **0 samples >= 20 ms**; the max is 5 ms and the
  only two samples >= 3 ms are 5 and 4 ms, at 1928 and 1942 ms (not
  periodic). The p99.9 is 2 ms.

Over the five runs that is **0 of 20,000** after-samples >= 20 ms, against a
before-rate of about **1 per second**.

## 3. The fix: delete the field

`thread_count` was **emitted but never consumed**:
`docs/refactor/contract-inventory.md` lists it under "keys emitted by the
engine but NOT consumed by `ParseEngineState`", and a repository-wide search
finds no reader in the Studio, the Rust client, the tests or the tools. (The
`thread_count` in `docs/bench/pacing-*.json` is the bench harness's own
`Get-Process` count, not the IPC field.)

So the fix is the smallest one that removes the cause:

- `engine/src/engine_app.cpp`: `CurrentThreadCount()` and
  `CurrentThreadCountCached()` are **deleted**, and so is
  `{"thread_count", ...}` in `BuildStateJson()`;
- `<tlhelp32.h>` is no longer included (it had no other user).

The alternatives were considered and rejected:

| alternative | why not |
|---|---|
| keep the 1 s cache | still a 50-70 ms spike once per second in the poll path (measured above) |
| refresh from a background thread | removes the spike from the handler but still burns ~75 ms of CPU **per second** (7.5 % of a core) to feed a field nobody reads |
| count only our own threads | a different metric - libmpv's demux/decode/VO threads would vanish from the number, so the field would lie |
| find a cheaper Win32 call | there is no per-process thread count without a system-wide enumeration; `GetProcessHandleCount` is the cheap neighbour and it stays |

`handle_count` (a `GetProcessHandleCount` call, microseconds) stays.

## 4. The gate and the verdict

**The gate** (`STATE.md` section 9, `budgets.md`): `max` in the p99 class,
recorded with `n` and the noise band.

**The verdict: PASS, with the residual stated.** The 50-69 ms periodic spike
class is gone; 0 of 20,000 samples cross 20 ms. The after-max is 4-14 ms,
which is **client-side scheduling jitter**, not a handler cost: it is not
periodic, it appears once per run at most (14 ms in run 5), and the same
harness measured 4-7 ms in the other four runs. An exact `max == p99` is not
claimed - the max is a single sample and the p99 is 0-1 ms at this
resolution - but the max no longer contains a **separate** cost above the
noise.

**What this does NOT claim:**

- It does not re-run the 4-thread churn stress. The 4-client connect
  currently loses clients to `ERROR_PIPE_BUSY` (the known K1/L4 baseline
  defect, 3 of 1200 requests in the 10-minute run); the client retry is
  **L4**. The 1-client stress is deliberate: it measures the handler, and
  the server is serial anyway.
- It does not claim the old 152 ms max would now be 7 ms **under churn**.
  The churn adds queueing and renderer work; the claim is that the
  **snapshot spike** is gone, which is the component L1 owned.

## 5. What changed in the tree

- `engine/src/engine_app.cpp` - the field and the two functions deleted.
- `engine/src/ipc_server.cpp` + `engine/src/engine_app.cpp` - the K1
  accumulator (`ipc_trace.hpp`) is now **actually wired**: `ipc.handler`,
  `ipc.ack`, `get_state.json`. K1 shipped the header without a single
  `TraceScope`; the breakdown in `phase2-k1-ipc-latency.md` came from a local
  patch. It is reproducible now.
- `rust/crates/k6wp-ipc/examples/stress_client.rs` - `--samples <csv>` writes
  `offset_ms,latency_ms` for every request (the series above).
- `tools/bench_get_state_max.ps1` - the private-instance harness.
- `docs/bench/l1/` - the 10 raw runs and the two sample series.

## 6. The instrument caveat (for L2) - **resolved by L2**

`TraceScope`'s destructor logged the summary **inline** every 200 samples
(`ipc_trace.hpp`), so with `K6WP_IPC_TRACE=1` the 200th request paid the
log write, and the write landed **inside** the very scope it is summarising.
That is acceptable for the **means** L2 needs and **not** acceptable for a
max gate - which is why the L1 gate ran with the trace off.

**L2 fixed it** (the cheap option, not a background writer): the summary is
now logged only when the **outermost** scope exits (a thread-local depth
counter), after that scope's own duration was taken. No measured block
encloses the flush, so a trace-on **max is usable** - the whole build's
trace-on max fell from 9-15 ms (flush) to 1.3-2.2 ms (real). The L1 gate
numbers below stay trace-off; they are unaffected. The trace-off path also
got cheaper: the scope no longer calls the clock when the trace is disabled.

**One caveat remains** (and it is the one that matters for L2's numbers): the
per-label **means** include the Debug-build instrument (a clock pair, a
`std::map` lookup and a mutex per scope), so they are **upper bounds** on the
release cost. The trace-off run remains the latency gate.

**A second caveat, found by L2's own runs:** the L1 gate's "0 of 20,000
samples >= 20 ms" was true **for L1's runs** but is not a universal
guarantee - a later L2 run saw 2 of 4,000 samples at 21 and 31 ms (the 31 ms
at offset ~1.1 s, not at startup) and clean harness runs had maxima of
34/57/62 ms, always with a p99 of 0-1 ms. The **p99 is the stable metric**;
the max is one OS-scheduling sample under a live session. `budgets.md` now
scopes the 20 ms row accordingly - see `docs/refactor/ipc-poll-cost.md` §7.

`K6WP_IPC_TRACE` is now **compiled out of the release build**: it obeys the
same `build_flags.hpp` gate as the pipe and mutex names, and the name is in
`tools/check_release_overrides.ps1`. The Release `engine.exe` contains
neither the name nor the `ipc-trace` format (verified by a byte scan), so a
shipped engine cannot be traced by an environment variable. **L5** still owns
the broader sweep (the CI wiring and the audit of every remaining override).

## 7. Reproduce

```powershell
# the two binaries: build/l1-before must be built from HEAD with the trace
# scopes, build/l1 from HEAD + L1 (see the commit) - same build dir options
powershell -File tools/bench_get_state_max.ps1 `
  -EngineExe build\l1-before\engine.exe -Label before-1 `
  -OutJson docs\bench\l1\before-1.json -Threads 1 -Requests 4000 `
  -SamplesOut $env:TEMP\samples-before.csv
powershell -File tools/bench_get_state_max.ps1 `
  -EngineExe build\l1\engine.exe -Label after-1 `
  -OutJson docs\bench\l1\after-1.json -Threads 1 -Requests 4000 `
  -SamplesOut $env:TEMP\samples-after.csv
```

Run each five times; the gate is the medians in section 2.

## 8. The traps this task paid for

- **PowerShell parameter variables are typed.** A `[switch]$Trace` parameter
  makes `$trace` (case-insensitive) a `SwitchParameter`; assigning an
  `OrderedDictionary` to it fails with "Cannot convert
  OrderedDictionary to SwitchParameter". The switch is `-WithTrace` now.
- **PS 5.1 turns a native command's stderr into a terminating error** when
  `$ErrorActionPreference = 'Stop'`. The Rust client reports on stderr, so
  every call runs under `EAP=Continue` with the exit code read explicitly.
- **`git diff ... > file` writes UTF-16** in PS 5.1, and `git apply` refuses
  it ("No valid patches"). Write patches with
  `[IO.File]::WriteAllText(..., UTF8 no BOM)` or avoid the round trip.
- A private engine with `monitor_id: 99` matches no display
  (`FilterMonitors`), so `InitWallpaperSurface` creates **zero slots** - the
  safe way to run a second engine beside the live session.
- The stress client's 4-thread connect can lose clients to
  `ERROR_PIPE_BUSY` (231); that is the L4 defect, not an L1 regression.
