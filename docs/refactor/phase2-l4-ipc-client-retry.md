# L4 — the Rust IPC client: the bounded retry, the over-limit test, and a real bug

**Task.** The Rust client (`k6wp-ipc`) must **wait** for a free pipe instance
(`ERROR_PIPE_BUSY`) with `WaitNamedPipe` and a timeout instead of failing (or
spinning); the over-limit case must be covered by a test that proves **no
hang** and a **clear error**; the ~0.25 % baseline request loss must be
recorded in the changelog and the known-limitations doc; and the Phase 3
per-worker connection model must be specified.

**Verdict: DONE, and it found a shipped-code bug.** While reproducing the
failure, the client was found to **lose the answer of a request that was
answered synchronously** - the read returned 0 bytes when the data was
already in the pipe buffer, so the client saw an empty reply and reported a
phantom failure. That bug was the trigger of the 2/5 lost runs in L2's sanity
gate (§6). Both defects are fixed, and the same measurement now runs
**5/5 clean, 0 failures over 20,000 requests**.

---

## 1. What the client does today (the C++ engine serves ONE client at a time)

The engine's IPC server creates **one** pipe instance, accepts one client and
serves it until it disconnects; only then does it create the next instance
(`engine/src/ipc_server.cpp`, `AcceptAndServe`). The concurrent-serving
change was tried and **reverted** (the pipe-instance accounting broke), so
this is the current, intended behaviour - and the reason a client must be
patient:

- `kMaxPipeInstances = 4` is the *ceiling* the pipe is created with, not the
  number of live instances: the server creates one at a time.
- A second client therefore gets `ERROR_PIPE_BUSY` (231) until the current
  client leaves.
- The K1 measurement showed the consequence: the **baseline build lost 3 of
  1200 requests (0.25 %)** under 4 clients, because the waiting clients gave
  up (`docs/refactor/phase2-k1-ipc-latency.md`, the table in §2).

---

## 2. The changes

### 2.1 `open_pipe`: `WaitNamedPipe` instead of a 50 ms poll

**Before:** `CreateFileW` -> `ERROR_PIPE_BUSY` -> `sleep(50 ms)` -> retry,
until the deadline; the error was a bare Win32 code.

**After:** `CreateFileW` -> `ERROR_PIPE_BUSY` -> **`WaitNamedPipeW`** with the
remaining time as its timeout -> retry. `WaitNamedPipe` blocks until an
instance becomes free (or the wait times out), so the client does not poll
and does not burn CPU while it waits. The timeout is clamped to **at least
1 ms** - a 0 would mean "wait forever" to the Win32 API.

The failure message now says what happened and how long it waited:

```
the pipe \\.\pipe\k6wp-engine-1 has no free instance after 5.0 s
(the Win32 error 121): every instance is in use
```

`ERROR_FILE_NOT_FOUND` (2) is **not** retried by a plain `connect` (a missing
pipe means the engine is not running - the caller wants to know now), but it
**is** retried by a reconnect (§2.2), where the engine is expected to be
between two instances.

### 2.2 `IpcClient::reconnect`: the old handle is released FIRST

The reconnect consumes `self`, so the `Drop` impl closes the pipe **before**
the new connect starts:

```rust
pub fn reconnect(self, options: ConnectOptions) -> Result<IpcClient, IpcError> {
    let pipe_name = self.pipe_name.clone();
    drop(self); // the Drop impl closes the handle
    Self::connect_with(&pipe_name, options, true)
}
```

This is the fix for the deadlock the L2 runs hit: the engine serves one
client at a time, so a **live** old handle keeps the only instance busy and
the new connect waits for the whole timeout (5 s) and then fails. With the
handle released first, the server sees the disconnect and creates the next
instance - and the reconnect tolerates the brief window where the pipe NAME
does not exist (`wait_for_the_pipe`).

`IpcClient::close` now only drops the client: it used to `CloseHandle` and
then let the `Drop` impl close the **same handle a second time**.

### 2.3 The read bug (the important one)

`overlapped_read` passed `None` for the byte count:

```rust
let read = 0u32;                       // always zero
let ok = unsafe { ReadFile(handle, Some(&mut buffer[..]), None, Some(&mut overlapped)) };
if ok.is_ok() {
    return Ok(read as usize);          // -> 0 bytes, even when the read succeeded
}
```

With an overlapped handle, `ReadFile` returns **TRUE** when the operation
completes **synchronously** - and in that case the number of bytes read
arrives **only** through `lpNumberOfBytesRead`, which the client did not ask
for. The synchronous path is the *common* one when the answer is already in
the pipe buffer (the engine answers fast), so the client regularly got
**0 bytes**, failed to parse the empty reply ("the answer is not JSON: EOF")
and reported a **phantom failure**. The fix passes the count out-parameter
and returns it; the write path now checks the count too (a short write is an
error, not a silent truncation).

**The shipped C++ Studio client is NOT affected**: it uses a synchronous
handle, `PeekNamedPipe` + `ReadFile` with the exact length, and it checks
`got == 0` (`studio/src/ipc_client.cpp:326-380`). The bug was in the Rust
client only - the stress client and the (not yet shipped) Rust Studio.

---

## 3. The tests

Two new tests in `rust/crates/k6wp-ipc/tests/client.rs`:

| test | what it proves |
| --- | --- |
| `a_busy_pipe_waits_then_says_why` | the over-limit case: every instance is busy -> the client **waits** (>= 300 ms with a 400 ms timeout), then fails with an error that contains "no free instance", bounded by the timeout (< 3 s). **No hang, a clear error.** |
| `the_reconnect_releases_the_old_handle` | the regression test: a fake engine that behaves like the real one (one instance at a time, a brutal `DisconnectNamedPipe` hang-up) - the request after the hang-up fails, the **reconnect succeeds** (it could not if the old handle were still open), and the request after the reconnect is answered. |

The fake engine is deterministic: it answers the first request and reads the
second one before hanging up, so the answer can never be lost to the
disconnect (the client reads the answer before it sends the next request -
the protocol is strictly request/response). No sleeps, no timing guesswork.

**Stability: 12 consecutive runs of the test file, 0 failures** (the file
takes ~2 s). The C++ side is untouched by this task.

---

## 4. The measurement that proves it (the L2 gate, re-run)

The L2 sanity gate (`docs/bench/l2/gate-{1..5}.json`, trace off, 1 client,
4000 requests) was **3/5 clean**: two runs lost the client connection after
an immediate request error and then stalled for 5 s on the reconnect.

With the **fixed client** and the **same engine, the same harness, the same
parameters** (`docs/bench/l4/gate-{1..5}.json`):

| run | requests | failures | p50 | p99 | max |
| --- | ---: | ---: | ---: | ---: | ---: |
| l4gate-1 | 4000 | 0 | 0 | 0 | 3 |
| l4gate-2 | 4000 | 0 | 0 | 1 | 15 |
| l4gate-3 | 4000 | 0 | 0 | 0 | 14 |
| l4gate-4 | 4000 | 0 | 0 | 0 | 7 |
| l4gate-5 | 4000 | 0 | 0 | 0 | 11 |

**5/5 clean, 20,000 requests, 0 failures, p99 0-1 ms, max 3-15 ms.** The only
change between the two sets is the client. That is the proof that the L2
gate's failures were **client** defects (the empty answer + the reconnect
deadlock) and not engine behaviour.

---

## 5. The recorded limitation (the ~0.25 %)

The baseline build lost **3 of 1200 requests (0.25 %)** under 4 clients: the
engine served one client at a time and the waiting clients gave up. The same
shape is still possible today - a client that gives up before its timeout
still loses its request - so the limitation is recorded rather than hidden:

- `CHANGELOG.md` (the Unreleased section): the client waits for an instance
  now, with the bounded retry and the clear error.
- `docs/known-issues.md` (item 9): the limitation, the reason, the fix and
  the residual (a client that is killed mid-request still loses that
  request; the engine's one-client-at-a-time service is the cause and goes
  to Phase 3).

**The residual, stated:** the request that was in flight when the pipe broke
is still lost - a protocol-level retry (a request id + an idempotent
replay) is **not** implemented. That is a Phase 3 decision (§6), not a
silent gap.

---

## 6. Phase 3: the per-worker connection model

The full spec is `docs/refactor/phase3-ipc-connection-model.md`. The short
version:

- **One pipe instance per worker thread.** The Phase 3 engine accepts a
  connection and hands it to a worker; the worker owns the instance for its
  lifetime and answers every request on it.
- **A request never blocks another client.** A slow handler (the C++ engine
  still has the whole `get_state` build on the IPC thread) cannot delay a
  second client.
- **The instance cap is explicit**: the number of workers is the cap, and a
  client beyond it waits with `WaitNamedPipe` and a clear timeout error.
- **A reconnect releases the old handle first** - the rule L4 established,
  now a property of the connection type.

---

## 7. Evidence index

| item | file |
| --- | --- |
| the client | `rust/crates/k6wp-ipc/src/lib.rs` (`open_pipe`, `connect_with`, `reconnect`, `overlapped_read`, `overlapped_write`) |
| the tests | `rust/crates/k6wp-ipc/tests/client.rs` (`a_busy_pipe_waits_then_says_why`, `the_reconnect_releases_the_old_handle`) |
| the stress client | `rust/crates/k6wp-ipc/examples/stress_client.rs` (the reconnect path) |
| the clean re-runs | `docs/bench/l4/gate-{1..5}.json` |
| the failed runs (L2) | `docs/bench/l2/gate-{2,5}.json` |
| the baseline loss | `docs/refactor/phase2-k1-ipc-latency.md` (§2, the table) |
| the Phase 3 spec | `docs/refactor/phase3-ipc-connection-model.md` |

**Labels:** the client defects = **test-proven** (the two new tests + the
5/5 re-runs); "the C++ Studio is not affected" = **code-quoted**
(`studio/src/ipc_client.cpp`); the 0.25 % baseline loss = **test-proven**
(K1's table, unchanged); the residual = **code-quoted** (no request id in
the protocol).
