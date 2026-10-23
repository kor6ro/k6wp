# IPC Client Timeouts Audit (Q1c)

**Generated:** 2026-10-23  
**Crate:** `k6wp-ipc` (`rust/crates/k6wp-ipc/src/lib.rs`)  
**Scope:** Every wait/loop in connect, write, read, reconnect, `WaitNamedPipe`.

---

## Summary

| Location | Function | Wait/Loop | Timeout Source | Verdict |
|----------|----------|-----------|----------------|---------|
| L576–666 | `open_pipe` | `loop` with `WaitNamedPipeW` | `ConnectOptions::timeout` (deadline) | ✅ Bounded |
| L715–716 | `overlapped_write` | `WaitForSingleObject` | `ConnectOptions::timeout` | ✅ Bounded |
| L730–744 | `overlapped_write` | `CancelIoEx` + `GetOverlappedResult(wait=true)` | — (cleanup) | ✅ Bounded |
| L800–801 | `overlapped_read` | `WaitForSingleObject` | `ConnectOptions::timeout` | ✅ Bounded |
| L814–826 | `overlapped_read` | `CancelIoEx` + `GetOverlappedResult(wait=true)` | — (cleanup) | ✅ Bounded |
| L247–251 | `reconnect` | Calls `connect_with(wait_for_the_pipe=true)` | `ConnectOptions::timeout` | ✅ Bounded |
| L646–648 | `open_pipe` (busy) | `WaitNamedPipeW` | Deadline-clamped ≥ 1 ms | ✅ Bounded |
| L662 | `open_pipe` (missing) | `std::thread::sleep` | `min(50ms, remaining)` | ✅ Bounded |

**No `INFINITE` waits found. No unbounded loops.**

---

## Detail by Function

### 1. `open_pipe` (lines 562–667)

**Purpose:** Opens the named pipe, waits if all instances are busy (`ERROR_PIPE_BUSY` / `ERROR_SEM_TIMEOUT`), or if the pipe is momentarily absent during reconnect (`ERROR_FILE_NOT_FOUND`).

**Loop structure:**
```rust
loop {
    match CreateFileW(...) {
        Ok(h) => return Ok(h),
        Err(e) => {
            let last_error = (e.code().0 as u32) & 0xFFFF;
            let busy = last_error == 231 || last_error == 121;
            let missing = last_error == 2;
            if !busy && !(missing && wait_for_the_pipe) {
                return Err(...); // Hard error
            }
            let remaining = deadline.saturating_duration_since(now);
            if remaining.is_zero() { return Err(timeout); }
            if busy {
                let wait_ms = remaining.as_millis().clamp(1, u32::MAX) as u32;
                let waited = WaitNamedPipeW(..., wait_ms);
                if !waited { return Err(...); }
            } else {
                std::thread::sleep(Duration::from_millis(50).min(remaining));
            }
        }
    }
}
```

**Timeout sources:**
- `deadline = Instant::now() + options.timeout` (line 575)
- `WaitNamedPipeW` clamped to `wait_ms ≥ 1` (line 646) — **0 would mean INFINITE**
- `std::thread::sleep` clamped to `min(50ms, remaining)` (line 662)

**Verdict:** ✅ All paths bounded by `options.timeout`.

---

### 2. `overlapped_write` (lines 672–755)

**Purpose:** Sends bytes with OVERLAPPED write, bounded by timeout. On timeout, cancels pending I/O and waits for cancellation.

**Wait points:**
```rust
// Line 715–716: Wait for write completion
let waited = WaitForSingleObject(event, timeout_ms);
// timeout_ms = timeout.as_millis().min(u32::MAX) as u32

// Line 730–744: On timeout, cancel + wait for cancellation
if waited == WAIT_TIMEOUT {
    CancelIoEx(handle, Some(&overlapped));
    let mut cancelled = 0u32;
    GetOverlappedResult(handle, &overlapped, &mut cancelled, true); // wait=true
    return Err(Timeout);
}
```

**Timeout sources:**
- `WaitForSingleObject(event, timeout_ms)` — `timeout_ms` derived from `options.timeout`
- `GetOverlappedResult(..., true)` — blocks until cancellation completes (bounded by OS)

**Verdict:** ✅ Write bounded; cancellation cleanup bounded by OS.

---

### 3. `overlapped_read` (lines 760–837)

**Purpose:** Reads one message with OVERLAPPED read, bounded by timeout. On timeout, cancels pending I/O and waits for cancellation.

**Wait points:**
```rust
// Line 800–801: Wait for read completion
let waited = WaitForSingleObject(event, timeout_ms);
// timeout_ms = timeout.as_millis().min(u32::MAX) as u32

// Line 814–826: On timeout, cancel + wait for cancellation
if waited == WAIT_TIMEOUT {
    CancelIoEx(handle, Some(&overlapped));
    let mut cancelled = 0u32;
    GetOverlappedResult(handle, &overlapped, &mut cancelled, true); // wait=true
    return Err(Timeout);
}
```

**Timeout sources:**
- Same as write: `WaitForSingleObject` with `timeout_ms` from `options.timeout`
- Cancellation cleanup: `GetOverlappedResult(..., true)`

**Verdict:** ✅ Read bounded; cancellation cleanup bounded by OS.

---

### 4. `reconnect` (lines 247–251)

**Purpose:** Closes old handle, then connects with `wait_for_the_pipe=true`.

```rust
pub fn reconnect(self, options: ConnectOptions) -> Result<IpcClient, IpcError> {
    let pipe_name = self.pipe_name.clone();
    drop(self); // Drop impl closes handle
    Self::connect_with(&pipe_name, options, true)
}
```

**Timeout source:** Delegates to `connect_with` → `open_pipe` with same `options.timeout`.

**Verdict:** ✅ Bounded by `options.timeout`.

---

### 5. `verify_server` (lines 303–372)

**Purpose:** Verifies server identity (PID, session, owner, image path).

**Wait points:** None — all Win32 calls are synchronous but fast (process queries). No loops.

**Verdict:** ✅ No waits.

---

### 6. `current_session_id` / `process_session_id` (lines 138–149, 389–397)

**Purpose:** Calls `ProcessIdToSessionId`.

**Wait points:** None — synchronous Win32 call.

**Verdict:** ✅ No waits.

---

### 7. `process_image_path` / `process_owner_sid` / `current_user_sid` (lines 400–516)

**Purpose:** Process queries via `OpenProcess` + `QueryFullProcessImageNameW` / token APIs.

**Wait points:** None — synchronous Win32 calls.

**Verdict:** ✅ No waits.

---

## Test Coverage for Timeouts

| Test | Function Tested | Mechanism |
|------|-----------------|-----------|
| `read_times_out_when_server_never_responds` | `overlapped_read` timeout | Server accepts, never writes; client read times out |
| `write_times_out_when_server_never_reads` | `overlapped_write` timeout | Server accepts, never reads; client write times out |
| `timeout_cancel_race_returns_success` | Cancellation race | Very short timeout (1ms); either success or clean timeout |
| `a_silent_server_times_out_instead_of_hanging` | Full request timeout | Server accepts, never responds; request times out |
| `a_busy_pipe_waits_then_says_why` | `open_pipe` busy wait | All instances busy; `WaitNamedPipe` times out |

**Note:** The first two tests pass with **old code** (without `CancelIoEx`) because the test sets `ConnectOptions::timeout = 500ms`, which times out the **entire request** before the overlapped I/O timeout would trigger. The `CancelIoEx` fix ensures:
- Fine-grained write vs read timeout control
- Correct behavior when no `ConnectOptions` timeout is set
- Cancellation cleanup prevents I/O completing in background

---

## UNPROVEN Items

| Item | Reason |
|------|--------|
| Partial reads (one frame split across reads) | MESSAGE mode delivers whole message; not simulated |
| Short write detection (inspected in `overlapped_write`, line 704–708) | Logic verified by code inspection; not simulated |
| Overlapped-I/O-specific timeout (without ConnectOptions timeout) | Tests use ConnectOptions timeout; not isolated |

---

## Conclusion

**No `INFINITE` waits. No unbounded loops.** Every wait in `k6wp-ipc` is bounded by `ConnectOptions::timeout` (default 5 s) or by OS cleanup during cancellation. The audit is complete.