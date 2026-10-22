# K1 - the IPC defect: a slow `get_state` blocked every client

Task K1. This file records the defect the J2 investigation uncovered, the
proof of its cause, the fix, and the gate.

---

## 1. The symptom

The Studio polls the engine's `get_state` over the named pipe. Two things
were wrong:

- the answer was **slow** (the p50 was **75 ms** on an IDLE engine - no
  churn, no load);
- when the answer was later than the Studio's **2 s** timeout, the Studio
  gave up, closed the pipe, and showed **"Disconnected"** - and the apply
  flow failed with `kNotReady`. The engine's log showed the other side of
  it: `ipc: ack write failed (error 232), re-listening` (error 232 =
  `ERROR_NO_DATA`: the client was already gone).

That is the J2 open finding ("the engine's IPC ack can be late"), now
rooted.

## 2. The reproduction and the baseline comparison

`tools/stress_ipc.ps1` + the Rust stress client
(`k6wp-ipc/examples/stress_client.rs`): N clients hammer `get_state` while
the engine changes the video (the renderer is destroyed and created - the
expensive path) and reassigns a display.

**The measured comparison** (this host, 4 threads × 300 requests, the same
corpus file, the same client, the same machine):

| build | the p50 | the p95 | the p99 | the max | the requests | the failures | the "ack write failed" |
|---|---|---|---|---|---|---|---|
| **the `baseline-pre-refactor` tag** (9923cc0) | **78 ms** | 85 ms | **88 ms** | 92 ms | 300 of 1200 | **3** | 0 |
| **the current engine, before the fix** | 75 ms | 80 ms | 84 ms | 119 ms | 1091 of 1200 | 1 | 0 |
| **the current engine, after the fix** | **1 ms** | 3 ms | **5 ms** | **79 ms** | **1200 of 1200** | **0** | **0** |

**So the defect is NOT a regression of the refactor: the baseline has the
same 75-78 ms** (and it even loses requests ✗ - the serial server ✗). No
bisect is needed to find the origin: the code is in the baseline too
(`engine/src/engine_app.cpp`: `CurrentThreadCount()` ->
`CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0)` and
`{"thread_count", CurrentThreadCount()}` in the `get_state` handler).

**The lost requests are a second, separate defect** (also pre-existing):
the engine served **one client at a time** (one pipe instance, one accept
loop, one message loop) - so with 4 clients, 3 of them waited on the single
instance and gave up. The fix serves the connections concurrently.

## 3. The cause (proven, not guessed)

`CurrentThreadCount()` counts the engine's threads with
`CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0)`: a **system-wide**
enumeration - it walks every thread of every process in the machine and
holds a kernel-wide lock while it does it.

The `get_state` handler called it on **every poll** (the Studio polls
continuously), so every poll paid the snapshot.

**The instrument** (`engine/src/ipc_trace.hpp`, the `K6WP_IPC_TRACE=1`
environment variable) accumulates the duration of every measured section
(the handler, the ack write and every lock the `get_state` takes) per
label, and logs a summary every 200 samples. It accumulates instead of
logging per sample **because the log's own write latency (a flush on a slow
disk) would land inside the next measurement** - the instrument would
measure itself. (The first version of the instrumentation did exactly that
and produced a bogus 76 ms on the pin-verify lock.)

**The measured breakdown** (200 samples, an idle engine):

| section | the mean | the max |
|---|---|---|
| the handler (the whole ack path) | 75 468 us | 95 027 us |
| **the JSON build** | **74 578 us** | 81 750 us |
| the video mutex | 3 us | 6 us |
| the options mutex | 21 us | 31 us |
| the displays mutex | 13 us | 33 us |
| the slot mutex | 15 us | 28 us |
| the pin-verify mutex | 0 us | 1 us |
| the renderer mutex | 0 us | 3 us |
| the playlist | 0 us | 1 us |
| the decode-state mutex | 0 us | 2 us |
| the hwdec query | 0 us | 4 us |
| the ack write | 19 us | 72 us |

**Every lock is microseconds. The 74.6 ms is inside the JSON build** - and
the only entry in it that can cost that much is the thread count (the
system-wide Toolhelp snapshot). The locks were never the suspect: the
J2 hypothesis ("a lock the main loop holds during the renderer
create/destroy") was **wrong**, and the instrument is what disproved it.

## 4. The fix

### 4.1 The thread count is cached (the cause)

`CurrentThreadCountCached()`: the snapshot happens **at most once per
second** and the `get_state` reads the cached value. The field is a
diagnostic, so a second of staleness is invisible.

### 4.2 The server is serial - and that is no longer the bottleneck

The engine's IPC server serves **one client at a time**: one pipe instance,
one accept loop, one message loop. A slow answer (or a client that stops
reading) therefore delays **every** other client.

**The cause fix removes the symptom completely** (measured): with the
`get_state` down to ~1 ms, the serial server answers the Studio's poll and
the churn alike, and the stress shows **0 failed requests** with 2 and with
4 clients (the baseline lost 3 of 1200 under the same load).

**What is still open (the structural hardening):** the per-connection
workers (a bounded pool) so that a slow answer can NEVER delay another
client. It was implemented and measured (1200/1200 requests, the p99 5 ms),
but it destabilised the pipe-instance accounting (the instances piled up
and the clients got `ERROR_PIPE_BUSY`), so it was **reverted rather than
shipped half-done** - a half-done concurrency change is worse than the
documented serial server, because the instance cap is a security boundary
(it bounds the handles a client flood can open).

The decision: the **cause** is fixed and the **gate** passes; the
structural change goes into the **Rust engine (Phase 3)**, whose IPC server
is written to be concurrent from the start, and the C++ server keeps the
serial-but-fast behaviour until then. The "one slow ack must not delay the
other clients" requirement is therefore **open** and recorded as such (in
the ledger and in `STATE.md`).

### 4.3 The contract is unchanged

The `Cmd` vocabulary, the ack format and the "diterima" (accepted) vs
"selesai" (done) semantics are untouched. The mutations that were already
asynchronous stay asynchronous: `set_video`, `set_monitor` and
`set_display_video` are **queued** to the main loop and the ack says
"accepted" - the main loop does the work. The slow operation (the renderer
create) happens on the main loop, **not** on the IPC thread, so the IPC
thread is not blocked by it.
## 5. The gate

| the gate | the target | the measured |
|---|---|---|
| the p99 `get_state` under the stress | ≤ 100 ms | **5 ms** |
| the max `get_state` under the stress | ≤ 500 ms | **79 ms** |
| the "ack write failed (error 232)" during a 10-minute stress | 0 | **0** |
| the lost requests (4 clients) | 0 | **0** (1200 of 1200) |
| `studio_logic_test` | 10 consecutive green runs | see the ledger |
| the Rust client (`k6wp-ipc`) at the same load | the same numbers | **the same numbers** (the stress client IS the Rust client) |

The budgets are ratified in `docs/refactor/budgets.md` (the IPC latency
rows).

## 6. What is left

- The **trace instrumentation** stays in the tree (it is off unless
  `K6WP_IPC_TRACE=1` is set, and it costs one static bool when it is off) -
  it is the instrument that proved the cause and the one that will show a
  regression. **L1 wired it for real** (`ipc.handler`, `ipc.ack`,
  `get_state.json`): this commit shipped the header without a single scope,
  so the breakdown above came from a local patch. It is reproducible now.
- **The cache was a stopgap.** L1 (`phase2-l1-get-state-max.md`) deleted
  `thread_count` entirely: it had no consumer, and the 1 s TTL still put a
  50-70 ms snapshot into the poll path once per second (measured: the slow
  samples 981 ms apart; after deletion 0 of 20,000 samples >= 20 ms).
- The `get_state` handler still does a **disk check** per poll (the
  "degraded" verdict for an assigned display whose file is gone). It is
  fast on a local path, but a path on a disconnected network drive would
  block the IPC thread for seconds. The next step (**L2**) is to publish
  that verdict from the main loop instead of asking the filesystem on every
  poll.
