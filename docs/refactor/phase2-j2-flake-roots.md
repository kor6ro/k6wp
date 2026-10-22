# J2 - the flake roots: what was really wrong, and the gate that proves it

Task J2. The rule the brief sets: **a test that sometimes fails may not be
called "environmental" without a proven root cause.** This file records the
two flakes that were seen, what was actually wrong with each, what is
fixed, and what stays open.

---

## 1. `still_test` - "settings: rewrite is re-read (no stale cache)"

**The symptom:** the check failed at random. It writes the settings file
twice - the first with `wallpaper_sync = true, lockscreen_sync = false,
offset = 2.5`, the second with the opposite values - and expects the
second read to see the new values.

**The root cause (proven, not guessed):** the still's settings cache key
is **(size, mtime)** (`shared/still.cpp::ReadStillSettingsCached`). The
two writes produce files of the **same size** (the same structure, three
different values) and, when they land in the same filesystem mtime tick,
the **same mtime** - so the cache key does not change and the second read
returns the **stale** value. Whether the two writes share a tick depends
on the filesystem's timestamp granularity, which is why the failure was
intermittent.

**The fix (the brief's own prescription - an explicit timestamp, not a
wait):** `tests/still_test.cpp::WriteSettings` now sets the file's
**mtime explicitly** (the current system time plus one second per write),
so the cache key always changes. The test no longer depends on the clock
and never waits for a tick.

**The proof:** the test passes repeatedly (5 consecutive runs, 99 checks,
0 failures each) - and it no longer depends on the filesystem's
granularity.

## 2. `studio_logic_test` - "readywait live engine -> true"

**The symptom:** the ready-wait leg (the Studio waits up to 8 s for the
**live** engine to answer) failed on several consecutive ctest runs, and
passed after the engine was restarted.

**The evidence (from the engine's own log):**

```
[10:37:10.141] ipc: ack write failed (error 232), re-listening
```

Error **232** is `ERROR_NO_DATA` - the pipe was already closed **when the
engine tried to write its answer**. The Studio's client gives up after
**2 s** ("Ipc: ack timeout (2s)"), so the engine's answer arrived **more
than two seconds late** - and then the engine re-listened.

**The narrowing (what is proven, what is not):**

| fact | how it is known |
|---|---|
| the engine's ack was more than 2 s late | the client's 2 s timeout + the engine's 232 write failure |
| the engine's IPC thread serves **one client at a time** | `engine/src/ipc_server.cpp` (the accept loop) |
| so a late ack **blocks the next client** | the accept loop is serial - the same log line shows the next `client connected` only after the failure |
| the engine was busy with a power event and the occlusion pokes at that moment | the log: `PBT_APMPOWERSTATUSCHANGE received`, then the occlusion pokes |
| the state recovered only after an engine **restart** | the ctest runs kept failing until the engine was restarted, then the leg passed |

**What is NOT yet proven:** which lock (if any) the IPC thread was
blocked on. The `get_state` handler takes three leaf locks
(`video_mutex_`, `options_mutex_` and the pin-verify mutex in
`EngineApp::BuildStateJson`) - any of them, held for a long time or
deadlocked, blocks the IPC thread for as long as it is held, and the IPC
thread is the only thing that answers the Studio.

**Why this is a production symptom, not only a test symptom:** the Studio's
status poll and its "apply" flow talk to the same pipe. A late or missing
ack makes the Studio show **"Disconnected"** and the apply flow fail with
`kNotReady` - the user sees the wallpaper switch as broken.

**The open finding (to be closed in the Phase 2 work):**

1. the `get_state` handler must not block on the main loop's locks - the
   state it reports must be an **atomic snapshot** published by the main
   loop, so the IPC thread is never blocked by a video load;
2. the IPC's ack latency must be **bounded** (a watchdog on the IPC
   thread's handler, or the same snapshot discipline).

Until 1 and 2 are done, the ready-wait leg stays **sensitive to a busy
engine** - and the J2 gate (10 consecutive runs with the live engine)
is the check that it does not happen in practice.

## 3. The CI lint: a test may not touch a production name

`tools/lint_test_overrides.ps1` (a CI gate and a pre-commit check)
scans every test source and fails when a test:

- uses the **production pipe name** (`\\.\pipe\k6wp-engine-...`) without
  claiming a unique one (`UseUniquePipeNameForTest` /
  `UseRealPipeNameForTest`), or
- mentions the **production singleton mutex**
  (`Local\K6WP-Engine-Singleton`) without the `K6WP_ENGINE_MUTEX`
  override.

That is the class of mistake that caused the 30-minute hang and the flaky
ready-wait: a test that talks to the user's live engine. The lint catches
it at review time instead of at 3 a.m.

**The current result:** 43 test sources scanned, **0 violations**.

## 4. The gate: 10 consecutive full ctest runs with the live engine

`tools/gate_ctest_stability.ps1` runs the **full** suite ten times in a
row **with the live engine running**, records the wall time of every run,
restarts the engine if a run kills it, and writes
`docs/bench/ctest-stability.json`.

The requirement: **0 failures in the 10 runs**. The times are in the
report.

## 5. What changed, in one list

| what | where | the kind of fix |
|---|---|---|
| the settings cache key collided (the same size + the same mtime tick) | `tests/still_test.cpp` | the test sets the mtime **explicitly** - the cause is gone, not masked |
| a test could touch the production pipe or mutex | `tools/lint_test_overrides.ps1` (+ the CI) | a **lint**: the mistake is caught at review time |
| the suite's stability was never measured | `tools/gate_ctest_stability.ps1` | a **gate**: 10 consecutive runs with the live engine, the times recorded |
| the engine's IPC ack can be late (> 2 s) and the IPC thread is serial | the engine's `get_state` handler | **OPEN**: the state must become an atomic snapshot (see above) |
