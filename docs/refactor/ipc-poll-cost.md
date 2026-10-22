# L2 — the per-field `get_state` cost and the poll-path audit

**Task.** After K1/L1 removed the once-per-second Toolhelp snapshot from
`get_state`, measure **every field** of the reply, audit the other IPC
handlers and the timers/poll loops for hidden work of the same class, and fix
any field that costs more than 1 ms **on the poll path**. One task, one
commit, the verdict in the ledger.

**Verdict: PASS in class.** No field costs more than 1 ms (the largest mean is
`state.json` at 0.17-0.20 ms, and even the whole reply is 0.31-0.36 ms). The
audit found exactly **one** real I/O call left on the poll path -
`std::filesystem::exists` per assigned display - and it was a **21-second**
block when the recorded path lives on a dead SMB share. It is **removed**:
the `degraded` verdict now reads the `exists` hint that the writer recorded
(Studio at write time, engine at apply time), so `get_state` does **no
filesystem I/O at all**. No consumer reads the field, so no parity is lost.

---

## 1. Method

**Instrument.** `k6wp::TraceScope` (the K1 accumulator, `engine/src/ipc_trace.hpp`)
wraps each part of `BuildStateJson` (`engine/src/engine_app.cpp`, the
`display_*` block at 2245-2290 and the fields around it):

| label | what it covers |
| --- | --- |
| `get_state.json` | the whole `BuildStateJson()` (kept from K1 as the total) |
| `state.video` | the `video` string copy under the video mutex |
| `state.options` | the options snapshot (fit mode, speed, ...) |
| `state.displays` | the `displays.json` snapshot copy + the assignment/path maps |
| `state.coverage` | the per-slot `display_coverage` loop |
| `state.exists` | the per-assignment filesystem stat (**this task removed it**) |
| `state.json` | the JSON object assembly (the ~30 scalar fields) |
| `state.dump` | the debug dump/playlist/engine-name fields |
| `ipc.handler` | the server's command dispatch around the handler |
| `ipc.ack` | the ack write on the server side |

**Runs.** `tools/bench_get_state_max.ps1` against a **private** Debug engine
(`build/l1/engine.exe`) beside the live session: unique singleton mutex and
pipe, `monitor_id: 99` (no display matches), no video - zero wallpaper slots,
the live desktop untouched. **1 client, 4000 `get_state` per run, 3 runs,
trace on** (`-WithTrace`). Raw reports: `docs/bench/l2/fields-{1,2,3}.json`.
The harness parses **every** trace label (no whitelist), so new fields show up
without editing the tool.

**What the numbers are.** The per-label **mean** is the honest number here:
the accumulator adds a `steady_clock::now()` pair, a `std::map` lookup and a
mutex per scope, and the engine is a **Debug** build, so every mean is an
**upper bound** on the release cost. The trace-on max is usable now (the L2
depth gate below), but a single sample is still a scheduling event, not a
field cost - the client's p99 is quoted alongside.

---

## 2. The per-field cost (µs, n = 4000 per run)

| label | run 1 | run 2 | run 3 | median | trace-on max (run 1 / 2 / 3) |
| --- | ---: | ---: | ---: | ---: | ---: |
| `get_state.json` (whole) | 327 | 358 | 305 | **327** | 2170 / 1357 / 1281 |
| `state.json` | 178 | 195 | 165 | **178** | 1949 / 1008 / 410 |
| `state.dump` | 43 | 48 | 41 | **43** | 229 / 301 / 317 |
| `state.displays` | 34 | 38 | 32 | **34** | 426 / 327 / 185 |
| `state.options` | 5 | 6 | 5 | **5** | 99 / 127 / 52 |
| `state.coverage` | 1 | 1 | 1 | **1** | 71 / 105 / 29 |
| `state.video` | 0 | 0 | 0 | **0** | 25 / 9 / 30 |
| `ipc.handler` | 421 | 463 | 393 | **421** | 2267 / 1659 / 1433 |
| `ipc.ack` | 12 | 10 | 9 | **10** | 139 / 244 / 133 |

*No field is over 1 ms.* The whole reply build is **0.31-0.36 ms**; the
handler around it (build + encode + dispatch) is **0.39-0.46 ms**; the ack
write is **~10 µs**. The client saw **p50 0 ms, p99 1 ms, max 4-15 ms** in the
same three runs, **0 failures** in 12,000 requests.

The children sum to ~0.26-0.32 ms of the 0.31-0.36 ms parent: the remainder is
the JSON object glue (the `display_assignments`/`display_coverage` objects)
plus the children's destructor bookkeeping, which is charged to the parent.
The means include the Debug instrument; a Release build does not carry it at
all (`K6WP_IPC_TRACE` is compiled out, see L5).

`state.coverage` at 1 µs is the loop over **zero slots** in the private
instance - it does not measure the per-slot cost. The live session has one
slot; the loop body is a `SnapshotSlots()` copy plus a string verdict, the
same class as `state.displays` (tens of µs), and the verdict chain is
`covered|clipped-*|headless` from the slot's recorded reason - no I/O.

---

## 3. The one I/O call left, and the 21-second tail

The old coverage loop stat'ed the assigned path **on every poll** for every
display that had an assignment:

```cpp
// before L2
if (display_assignments.contains(s.device)) {
  std::error_code ec;
  const std::filesystem::path assigned_path(
      std::filesystem::u8path(display_assignments.at(s.device).get<std::string>()));
  if (!std::filesystem::exists(assigned_path, ec) || ec) verdict = "degraded";
}
```

The private harness has **zero slots**, so the loop never ran there and
`state.exists` has `n = 0` in the trace. The cost was measured at unit level
instead (`[IO.File]::Exists`, 2000 calls per case, `docs/refactor/STATE.md`
records the method):

| case | mean | note |
| --- | ---: | --- |
| warm local file (`tests/corpus/meddpb_1080p.mp4`) | **56.6 µs** | 2000 calls, cached metadata |
| first call in a fresh process | **2.22-2.49 ms** | cold path/DLL work |
| `\\192.0.2.1\share\missing.mp4` (dead SMB) | **21,053.2 ms** | the SMB timeout |

55 µs is invisible. **21 seconds is not**: this runs on the engine's **IPC
worker thread**, and the Studio's request timeout is 5 s, so a single
assignment pointing at an unreachable share turns every poll into
"Disconnected" - the same user-visible failure class as K1, with a much
longer block. A recorded path can come from a library scan or a hand-edited
`displays.json`; the engine must not depend on the share being up to answer a
status poll.

---

## 4. The fix: report the recorded hint, never stat

`MonitorAssignment::exists` is written by the **Studio** when it assigns (it
has already checked the file) and by the **engine** at apply time. That is the
only existence information the UI needs for the `degraded` diagnostic. L2
copies it into a `std::map<std::string, bool>` while the `displays_cfg`
snapshot is already being copied, and the coverage loop reads it:

```cpp
std::map<std::string, bool> assignment_exists;  // L2: recorded, never stat'ed
...
const std::string device = NarrowUtf8(key);
display_assignments[device] = NarrowUtf8(a.path);
assignment_exists[device] = a.exists;
...
const auto hint = assignment_exists.find(s.device);
if (hint != assignment_exists.end() && !hint->second) verdict = "degraded";
```

`state.exists` is gone from the code and from the trace. `get_state` now does
**no filesystem call** of any kind: a mutex copy of the in-memory snapshot,
the JSON build and the ack.

**Trade-off, stated not hidden.** A file deleted **after** the assignment no
longer flips to `degraded` within a poll; it flips when the assignment is
written/applied again (or when the Studio's library scan notices). This is
acceptable because:

- **No consumer reads it.** `display_coverage` is emitted but not parsed by
  `ParseEngineState` (`shared/ipc_protocol.cpp`), and no Studio, Rust crate or
  tool reads it (contract-inventory §2.5). It is a diagnostic for humans and
  tests, not a UI state.
- The Studio already marks broken files in the library; the slot itself keeps
  its placement verdict (`covered|clipped-*|headless`).
- The alternative (an async stat worker) adds a thread and a wakeup source for
  a field nobody reads, and still has a stale window. Deleted is simpler.

If a future UI consumes the field and wants live existence, the correct design
is a **watcher/scan result**, not a per-poll stat - noted in
`contract-inventory.md`.

---

## 5. The poll-path audit

Every IPC handler, timer and poll loop that can run while the Studio polls:

| site | work | verdict |
| --- | --- | --- |
| IPC server, `Cmd::set_video` / `set_monitor` / `set_display_video` | `ipc_marshal_.Queue*` only (engine_app.cpp:405-411) | no I/O, no lock beyond the queue |
| `Cmd::pause` / `resume` | flip a pause owner | atomic/mask update |
| `Cmd::quit` | signal the loop | no I/O |
| `Cmd::get_state` | the table above | **no I/O after L2** |
| `IsHwdecActive()` | atomic read | no lock |
| slot count / `SnapshotSlots()` | mutex + small vector copy | tens of µs |
| config watch | one-shot/debounced (no periodic full parse) | not on the poll path |
| topology reconcile | one-shot, 400 ms debounce | not on the poll path |
| reconcile debounce | 500 ms | not on the poll path |
| working-set trim | one-shot at 2000 ms, then off | not on the poll path |
| Qt Studio poll | `get_state` every **1500 ms** (`studio/src/studio_bridge.cpp:269-271`); an in-flight tick is skipped, never queued | bounded |
| Slint Studio (`studio-slint/main.rs`) | no polling loop | nothing to bound |
| launcher | `CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS)` only in the **process-stop** path (`launcher/process_stop.cpp:258`), not a poll | out of the poll path |

There is **no other `CreateToolhelp32Snapshot`** in `engine/`, `studio/` or
`launcher/` (grep, 2026-10-22). The K1-class system-wide snapshot now exists
only where a user action (stop) needs it.

**L3 input (Studio polling).** The Qt timer is a fixed 1500 ms and skips
in-flight ticks; L3 measures the engine CPU/wakeup cost with the Studio open
and decides the adaptive interval/backoff, and the Rust polling design goes
into `ux-spec.md`. L2 changes neither.

---

## 6. The instrument fix (why trace-on maxes are usable now)

L1 recorded a caveat: the accumulator's summary was logged **inside** the
scope that hit the 200-sample boundary, so a trace-on max paid for the log
flush (9-15 ms of fake cost, cascading through the child labels). The L2
instrument logs only when the **outermost** scope exits (a thread-local depth
counter), after its own duration was taken. No measured block encloses the
flush:

- trace-on maxima are now **1.28-2.17 ms** for the whole build (they were
  9-15 ms of flush), and the per-field maxima are 0.03-0.43 ms;
- the trace-off path is cheaper than before: the old `start_` default
  initializer called `steady_clock::now()` even when disabled; now the ctor
  returns after the `IpcTraceEnabled()` check and does nothing else.

The **means** still include the Debug clock/map/mutex overhead and are quoted
as upper bounds; the trace-off runs remain the latency gate.

---

## 7. Gate, noise band, and the contradiction with L1's 20 ms tail

The trace-on evidence above is 3/3 runs, 0 failures. The **trace-off** sanity
runs (`docs/bench/l2/gate-{1..5}.json`, same harness, 4000 requests):

| run | requests | failures | p50 / p99 | max |
| --- | ---: | ---: | ---: | ---: |
| gate-1 | 4000 | 0 | 0 / 0 | 34 |
| gate-2 | 985 | 1 | 0 / 1 | 11 |
| gate-3 | 4000 | 0 | 0 / 0 | 57 |
| gate-4 | 4000 | 0 | 0 / 0 | 62 |
| gate-5 | 227 | 1 | 0 / 0 | 0 |

**Two runs lost the connection** after an immediate request error; the client
then tried to reconnect **while still holding the old pipe handle**, the
single-instance server was still serving that old connection, and the new
connect timed out after 5 s (`ERROR_PIPE_BUSY`; the client's error branch
reconnects without closing the old client - this is **L4's** bounded-retry
task). The engine stayed alive. The 3 clean runs have p99 0 and max 34-62 ms;
the client saw the spikes as single samples.

**One clean run with per-sample CSV** (`-SamplesOut`) put the maximum
(**31 ms**) at **offset ~1.0-1.2 s**, not in the first requests (the first
five were 0-1 ms), with 2 of 4000 samples >= 20 ms. That is an OS/scheduler
stall, not a startup race and not a field cost. A manual steady-state private
engine (2 s settle, no probe) ran **3 x 4000** requests: **max 11 / 16 / 11
ms, 0 failures**.

**This contradicts L1's universal phrasing.** L1 recorded "0 of 20,000
after-samples >= 20 ms"; a later run saw **2 of 4,000** >= 20 ms (21 and 31
ms) and the clean harness runs saw maxima of 34/57/62 ms. The L1 decision
(the mean 56 -> 7 ms, the periodic class gone) stands, but the **max row is
not a hard 20 ms guarantee**: under a live session the tail is OS scheduling,
and the stable metric is **p99 = 0-1 ms**. `budgets.md` is amended to say so
and to scope the 20 ms row to L1's measurement conditions. Hiding this would
be exactly the kind of clean-looking number this refactor must not ship.

---

## 8. Evidence index

| item | file |
| --- | --- |
| per-field trace, 3 runs x 4000 | `docs/bench/l2/fields-{1,2,3}.json` |
| trace-off sanity runs | `docs/bench/l2/gate-{1..5}.json` |
| the fix | `engine/src/engine_app.cpp` (`BuildStateJson`, 2245-2290) |
| the hint contract | `shared/displays_schema.hpp` (`MonitorAssignment::exists`) |
| the simulation test | `tests/display_assignment_test.cpp` (`HandleGetState`, case (d)) |
| the instrument | `engine/src/ipc_trace.hpp` (depth-gated summary) |
| K1/L1 context | `docs/refactor/phase2-l1-get-state-max.md` |
| the budget rows | `docs/refactor/budgets.md` (2.6) |
| the contract note | `docs/refactor/contract-inventory.md` (§2.5) |

**Labels:** per-field numbers = **test-proven** (private engine, n stated);
the 21 s = **test-proven** (unit bench); "no filesystem call" =
**code-quoted**; the tail/contradiction = **test-proven, recorded**; the
Studio consumers = **code-quoted** (`ParseEngineState` + grep).
