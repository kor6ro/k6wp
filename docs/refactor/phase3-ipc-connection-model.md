# Phase 3 - the IPC connection model (the per-worker spec)

**Status:** a specification, not an implementation. It exists because the
**C++ engine serves one client at a time**, and every client-side workaround
(the bounded `WaitNamedPipe` retry, the reconnect that releases the old
handle, the ~0.25 % baseline request loss) is a symptom of that one design
fact.

**Owner:** the Phase 3 engine (the engine in Rust). The Rust Studio (Phase 2)
consumes it unchanged - the wire protocol does not change.

---

## 1. The problem, measured

| fact | where it is measured |
| --- | --- |
| the engine creates **one** pipe instance and serves it until the client disconnects | `engine/src/ipc_server.cpp` (`AcceptAndServe`) |
| the concurrent-serving attempt was **reverted** (the pipe-instance accounting broke) | `STATE.md`, the "IPC server must serve the clients concurrently" row - still **open** |
| 4 clients -> the baseline lost **3 of 1200** requests (0.25 %) | `phase2-k1-ipc-latency.md` §2 |
| a client that waits longer than its timeout **loses** the request | `phase2-l4-ipc-client-retry.md` §5 |

The concurrent-serving revert is why the **client** carries the workaround.
The Phase 3 engine removes the need for the workaround - and the client
workaround stays valid until the Studio ships against the new engine.

## 2. The model

```
        connect                          connect
           |                                |
   +-------v--------+              +--------v--------+
   |  worker slot 0 |              |  worker slot 1 |     ... up to N workers
   | (owns one pipe |              | (owns one pipe |
   |  instance for  |              |  instance for  |
   |  its lifetime) |              |  its lifetime) |
   +-------+--------+              +--------+--------+
           |  answers every request on its own instance
           v
      the request state (shared, lock-free reads + a small mutex where needed)
```

**Rules:**

1. **One instance per worker.** The accept loop creates the instance and
   hands it to a worker; the worker owns it for the connection's lifetime and
   answers every request on it. The instance is released when the
   connection ends.
2. **A request never blocks another client.** The worker count is the
   concurrency; a slow handler occupies one worker, not the whole server.
   The C++ engine's remaining serialisation points (the state build, the
   slot snapshot) become **per-request snapshots**, not a global lock.
3. **The cap is explicit.** `N` workers = `N` live instances. A client
   beyond the cap gets `ERROR_PIPE_BUSY` and **waits** with `WaitNamedPipe`
   and a bounded timeout (the L4 client behaviour), then a clear error.
4. **A reconnect releases the old handle first** (the L4 rule): the
   connection type owns the handle, and dropping the connection closes it
   before the next connect.
5. **No shared mutable session state on the request path.** The workers read
   the state (an `ArcSwap`/atomics snapshot), they do not serialise on it.
   A write (set_video, set_monitor) goes through the main loop's queue, as
   the C++ engine does today.

## 3. What it must not break

- **The wire protocol** stays the same (`shared/ipc_protocol.cpp`): the
  Phase 2 Rust Studio talks to the Phase 3 engine unchanged.
- **The `get_state` budget** (L2): the whole reply build is 0.31-0.36 ms and
  no field is over 1 ms; the per-worker model must not add a lock to that
  path.
- **The verification** (`verify_server`): the client checks the server's
  image, session and owner. The Phase 3 engine is one binary (`k6wp.exe`) -
  the accepted image list already contains it.

## 4. The open questions (decide before 3.7)

| question | why it matters | suggested default |
| --- | --- | --- |
| How many workers? | the concurrency vs the memory (each worker has a stack) | 4 (the pipe's instance cap today) |
| Does a worker own the instance or borrow it? | the reconnect rule (L4) needs the owner to release it | own it |
| What happens to a request in flight when the client disconnects? | the lost-request residual (0.25 %) | drop it; a request id + replay is a protocol change, not a connection change |
| Does the launcher and the engine merge into one process? | the accepted image names already allow `k6wp.exe` | yes (the Phase 3 plan) |

## 5. Evidence / references

- `docs/refactor/phase2-l4-ipc-client-retry.md` - the client rules the
  server must not depend on.
- `docs/refactor/phase2-k1-ipc-latency.md` - the serial server and the
  0.25 % loss.
- `docs/refactor/ipc-poll-cost.md` - the per-field cost of the reply (the
  budget the workers must not inflate).
- `engine/src/ipc_server.cpp` - the current accept loop.
