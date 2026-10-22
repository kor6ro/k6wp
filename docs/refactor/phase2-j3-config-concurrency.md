# J3 - the config writers: one lock, one read-merge-write, one publish

Status: **done** (C++ and Rust). Evidence labels: **test-proven**,
**live-proven** where stated, **code-quoted** otherwise.

## The problem

`config.json` has more than one writer:

| Process | Writer | Writes |
|---|---|---|
| Engine | `k6wp::PersistConfigField` | one field (tray action, IPC `Cmd`) |
| Engine | `k6wp::SaveConfig` | the whole struct (migration self-heal) |
| Studio | `SaveConfig` (`apply_manager.cpp`, the settings bridge) | the whole struct |
| Studio / engine | `AtomicWriteJson` | the shared publish for `studio_settings.json`, `playlist.json`, `displays.json`, `library.json` |

Every one of them is a read-merge-write followed by an atomic publish through
`"<file>.tmp"` + `MoveFileExW(REPLACE_EXISTING)`. Two writers that do that at
the same time have two independent problems:

1. **Lost update.** Both read the same old document; the second publish
   replaces the first, so the first writer's field is gone.
2. **A shared temp name.** Both write `config.json.tmp`. One truncates the
   other's payload, `MoveFileExW` then fails with `ERROR_ACCESS_DENIED` (5) or
   `ERROR_SHARING_VIOLATION` (32), and the bytes that do get published can be
   a mixture of both documents - invalid JSON.

## The decision

**One named mutex, `Local\K6WP-Config-Lock`, held across the whole
read-merge-write-publish.** The brief allowed either "single writer per file"
or "RMW under the lock with a re-read inside the lock"; the tree already has
several writers by design (the engine persists single fields, the Studio saves
settings), so the lock is the honest option.

- `shared/config_lock.{hpp,cpp}` - `k6wp::detail::ConfigFileLock`, RAII,
  **bounded wait 2000 ms**, then `ConfigError`. A `WAIT_ABANDONED` (the owner
  died) is taken over immediately: a crash cannot wedge the next write.
- `Local\` scopes the mutex to the session: a second Windows user has their
  own `%LOCALAPPDATA%` and must not contend with ours.
- Acquisition is recursive per thread (Win32 semantics), so
  `PersistConfigField` -> `AtomicWriteJson` nests without deadlocking.
- `AtomicWriteJson` takes the lock itself, so the shared `.tmp` name is never
  written by two processes at once - including for the non-config settings
  files that share the publish.
- `SaveConfig` and `PersistConfigField` take the lock **before** the read, so
  the re-read is inside the critical section. That re-read is the point.
- Rust mirror: `rust/crates/k6wp-core/src/config_store.rs`
  (`ConfigFileLock`, `save_config`, `persist_config_field`), the same lock
  name, the same 2 s bound, the same RMW order.

## Evidence

### The negative control (live-proven, this machine)

Two `config_test.exe` child processes, each performing **400**
`PersistConfigField` calls on its own key (`field_a`, `field_b`) against one
file, five scenes. `n = 5` scenes per condition.

| Condition | Clean scenes | What happened in the damaged scenes |
|---|---|---|
| **without** the lock (`K6WP_TEST_NO_CONFIG_LOCK=1`, DEBUG-only escape) | **0 / 5** | scene 1: `field_a` stuck at 3, rename failed (5, 32); scene 2: `field_a` stuck at 1; scene 3: both fields stuck at 150; scene 4: **the published file was invalid JSON** (`parse error at line 20`) - two children had interleaved into the one `.tmp`; scene 5: temp write failed, `field_b` stuck at 150 |
| **with** the lock | **5 / 5** | both fields at 400, the document at the current schema, no `.tmp` left behind |

The corrupt-file scene is the important one: this is not only a lost settings
value, it is a config file that the next load has to reject, back up and
rebuild. The lock removes the whole class.

### The tests that now guard it (test-proven)

- C++ `tests/config_test.cpp`, **317 checks, 0 failures**:
  - `77.` the two-process scene above (150 writes each) asserts both final
    values, the schema version and the absence of a `.tmp` file.
  - `78.` a lock held by another thread makes a zero-timeout acquisition
    report "busy" (not hang), and the lock is free again after release.
  - `76.` the foreign-key policy below.
- Rust `k6wp-core` `config_store::tests`, **7 tests, 0 failures**:
  `two_processes_do_not_lose_updates` re-executes the test binary
  (`--exact config_store::tests::child_writer_entry`) so the two writers are
  two real processes, plus the no-op contract, the corrupt-file rebuild with
  `.bak`, "a bad value does not touch the file", the bounded wait, and the
  foreign-key budget.
- Full suites: `ctest --preset msvc-dev` **50/50 in 61.13 s**;
  `cargo test --workspace` green (75 core unit tests among them);
  `cargo clippy --workspace --all-targets -D warnings` and
  `cargo fmt --check` green.

## The foreign-key policy (what an older engine carries forward)

`MergeUnknownKeys` (C++ `/shared/config_schema.cpp`, Rust
`config::merge_unknown_keys`) now defines exactly four rules:

| Rule | Behaviour | Why |
|---|---|---|
| **Known names win, whatever the type** | `{"crf": "high"}` next to a known `crf` is discarded; the known `crf: 22` stays | coercing `"high"` into a number is a silently different setting; the known key's type is the contract |
| **Unknown keys are preserved verbatim** | `{"a_key_from_the_future": 42}` survives the rewrite | an older engine must not drop a newer engine's fields |
| **The foreign payload is capped at 64 KiB** | keys are visited in sorted order; a key that does not fit the remaining budget is skipped | the read is already capped at 1 MiB (`kMaxConfigBytes`), but every rewrite would copy that payload forever; sorted order makes "which keys fit" deterministic on both sides |
| **Valid JSON only** | a file that does not parse contributes nothing (it is backed up to `.bak` first on the RMW path) | a corrupt file must never break a save |

Cost measurement is identical on both sides: key bytes + the serialized
value bytes (`json::dump()` / `serde_json::to_string`), UTF-8.

The two small policy cases are frozen in `tests/golden/shared_behavior.json`
(`config.roundtrip.a_foreign_value_with_a_known_name`,
`config.roundtrip.a_foreign_value_of_another_type`); the C++ dump now emits
**101 cases** and the Rust replayer matches all of them. The 64 KiB budget is
covered by unit tests on both sides (a fixture would have to embed ~70 KiB of
input for one case).

## What this does NOT solve

`SaveConfig` still writes the **whole** known field set from the caller's
struct. The lock serializes the writes and preserves foreign keys, but a
caller holding a stale struct can still revert a known field another process
changed before the save. That window exists only when the Studio saves its
whole settings model while the engine persists a single field; the engine
itself never saves stale structs (its path is the field-level RMW). The
Phase 3 plan keeps this honest: the Rust engine will own `config.json`
(re-read + field-level merge, the same `persist_config_field`), so the
multi-writer window disappears rather than being widened.

## Files

| File | Change |
|---|---|
| `shared/config_lock.hpp` | new: the RAII lock + the policy comment |
| `shared/config_lock.cpp` | new: `CreateMutexW` / bounded wait / release |
| `shared/config_schema.cpp` | lock in `SaveConfig`, `PersistConfigField`, `AtomicWriteJson`; the 64 KiB cap and the collision rule in `MergeUnknownKeys` |
| `shared/CMakeLists.txt` | `config_lock.cpp` |
| `tests/config_test.cpp` | tests 76 (policy), 77 (two processes), 78 (bounded wait); the `--writer` child mode |
| `tests/golden_dump.cpp` + `tests/golden/shared_behavior.json` | the two policy cases (101 cases) |
| `rust/crates/k6wp-core/src/config_store.rs` | new: the Rust lock, `save_config`, `persist_config_field`, 7 tests |
| `rust/crates/k6wp-core/src/config.rs` | the same 64 KiB cap / sorted-order policy in `merge_unknown_keys` |
| `rust/crates/k6wp-core/Cargo.toml` | the Windows-only `windows` dependency (`Win32_Foundation`, `Win32_Security`, `Win32_System_Threading`) |
| `tools/check_release_overrides.ps1` | `K6WP_TEST_NO_CONFIG_LOCK` added to the forbidden names |

## Traps paid

- `windows.h` pulls in `rpcndr.h`, which does `#define small char`. The test
  file already had a local named `small`, and the error
  (`'auto' cannot be combined with any other type-specifier`) points at the
  `auto`, not the macro. Fixed with `#undef small` at the include site.
- `CreateMutexW` in the `windows` crate is gated behind **both**
  `Win32_System_Threading` and `Win32_Security`; without the second one it is
  silently absent and the error is "no `CreateMutexW` in
  `Win32::System::Threading`".
- Rust's libtest needs the **fully qualified** test name with `--exact`
  (`config_store::tests::child_writer_entry`) to re-execute one test in the
  child process; a short name runs the whole suite in every child.
- The C++ child receives the config path through an environment variable
  (`K6WP_TEST_CONFIG_PATH`, wide), not `argv`: a non-ASCII user name in
  `%TEMP%` would otherwise be mangled by the ANSI command line.
- `engine.exe` could not be relinked in the **dev** tree while the live engine
  runs (`LNK1168`); the changed translation units compiled clean. The
  **release** tree (`build/release`, a different directory) linked the new
  `engine.exe` normally, and `tools/check_release_overrides.ps1` scanned the
  five release binaries and found none of the five override names - including
  the new `K6WP_TEST_NO_CONFIG_LOCK`, so the test escape really is compiled
  out. The dev relink happens at the next session restart.
