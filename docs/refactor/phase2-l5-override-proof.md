# L5 — the diagnostic overrides are compiled out of the release build

**Task.** Prove that `K6WP_IPC_TRACE` and every other diagnostic override is
compiled out of a release build (or cost-free and leak-free), audit **all**
of them, and wire the proof into CI.

**Verdict: DONE, with one real defect found and fixed.** The audit of every
`K6WP_*` environment read in the tree found **one** diagnostic override that
was **not** compiled out of the release build: `K6WP_SPS_DEBUG` (the SPS
parser's per-box stderr dump). It is now behind `#[cfg(debug_assertions)]`.
The proof is now **two-layered and in CI**: a **source lint** (the rule: every
`K6WP_*` read must sit behind a build gate) and a **binary scan** (the
artifact: a release binary must not contain the name) — with a self-test that
proves the lint still fires.

---

## 1. The audit (every `K6WP_*` environment read in the tree)

The lint found **21 reads** of **18 distinct names** over **272 tracked
source files**. The classification (the reason is part of the rule, in
`tools/lint_env_overrides.ps1`):

| name | site | gate | class |
| --- | --- | --- | --- |
| `K6WP_ENGINE_PIPE` | `shared/ipc_protocol.cpp:181` | `K6WP_ALLOW_ENV_OVERRIDES` | compiled out |
| `K6WP_ENGINE_MUTEX` | `engine/src/engine_app.cpp:146` | `K6WP_ALLOW_ENV_OVERRIDES` | compiled out |
| `K6WP_LIBRARY_JSON` | `studio/src/library_manager.cpp:99` | `K6WP_ALLOW_ENV_OVERRIDES` | compiled out |
| `K6WP_LIBRARY_JSON` | `studio/src/thumbnailer.cpp:152` | `K6WP_ALLOW_ENV_OVERRIDES` | compiled out |
| `K6WP_LIBRARY_JSON` | `rust/crates/k6wp-studio/src/main.rs:30` | `#[cfg(debug_assertions)]` | compiled out |
| `K6WP_TEST_NO_CONFIG_LOCK` | `shared/config_lock.cpp:21` | `K6WP_ALLOW_ENV_OVERRIDES` | compiled out |
| `K6WP_IPC_TRACE` | `engine/src/ipc_trace.hpp:63` | `K6WP_ALLOW_ENV_OVERRIDES` | compiled out |
| `K6WP_DEV` | `studio/src/apply_manager.cpp:143` | `#ifdef _DEBUG` | compiled out |
| `K6WP_SPS_DEBUG` | `rust/crates/k6wp-core/src/sps.rs:845` | **`#[cfg(debug_assertions)]` (L5 added it)** | compiled out |
| `K6WP_RECONCILE_MODE` | `engine/src/engine_app.cpp:259` | none | **stays** - a documented feature switch (the shadow reconcile mode, CHANGELOG alpha.3); it changes a behaviour, it redirects nothing |
| `K6WP_CACHE_DIR` | `compressor/src/cache_manager.cpp:29` | none | **stays** - the Studio tells the compressor where to put its cache (`studio/src/compress_service.cpp:213`): an internal channel, not a diagnostic |
| `K6WP_CACHE_MAX_BYTES` | `compressor/src/cache_manager.cpp:47` | none | **stays** - the compressor cache cap: a limit, not a redirection |
| `K6WP_PLAYLIST_JSON` | `studio/src/playlist_bridge.cpp:19` | none | **stays** - the release test suite relies on it (`tests/playlist_bridge_test.cpp`) |
| `K6WP_THUMBS_DIR` | `studio/src/thumbnailer.cpp:270` | none | **stays** - the thumbnailer scratch root; the release test suite relies on it (`tests/studio_logic_test.cpp`, `tests/thumbnailer_test.cpp`) |

The test-only names (`K6WP_TEST_CONFIG_PATH`, `K6WP_IPC_LIVE`,
`K6WP_IPC_ENGINE_EXE`, `K6WP_RUST_WRITER_*`) live in test files or in
`#[test]` functions: they are never in a shipped binary, and they are not in
the forbidden list (the CI release build puts the **test** binaries in the
same directory, so they must not be forbidden there).

### The one real defect

`K6WP_SPS_DEBUG` printed **one stderr line per box** of the SPS parser and
was **not** gated: it was live in every build, including the release ones
that the Phase 3 engine will ship. It is now behind
`#[cfg(debug_assertions)]`, and its name is in the forbidden list, so the CI
scan catches a regression.

### Why the four "stays" are not changed here

Three of them are **shipped behaviour** of the released alpha (the
`K6WP_RECONCILE_MODE` switch is in the alpha.3 CHANGELOG; the playlist and
the thumbnail root are read by the Studio) and one is an **internal channel**
between the Studio and the compressor. Compiling them out now would change
the shipped behaviour of the alpha **and** break the release-mode test
suite, which sets `K6WP_LIBRARY_JSON`, `K6WP_PLAYLIST_JSON` and
`K6WP_THUMBS_DIR`. That is an owner decision, so they are **classified and
recorded** (in the lint's allow-list, with the reason) instead of being
silently changed. The open question is written down in §6.

---

## 2. The two layers of the proof

### Layer 1 — the source lint (`tools/lint_env_overrides.ps1`)

The **rule**: a `K6WP_*` name that is READ from the environment must sit
behind a build gate (`#if K6WP_ALLOW_ENV_OVERRIDES`, `#ifdef _DEBUG`,
`#[cfg(debug_assertions)]`, `#[cfg(test)]`, `#[test]`) or be on the
documented allow-list.

The lint reads the **tracked** files only, matches the C++ and the Rust
spellings (including the tree's own wrappers: `GetEnvStr`,
`qEnvironmentVariable`), walks back over the enclosing preprocessor
conditional (so a gate from an unrelated block above does not count), and
prints the **allow-listed** names with their reason on every run - so the
classification stays visible in the CI log.

It also has a **consistency check**: every name in the checker's forbidden
list must be read somewhere in the tree, or the list has drifted and the
binary scan would pass vacuously for that name.

**Result on the tree: 272 files, 21 reads, 18 distinct names, 0 errors.**

### Layer 2 — the binary scan (`tools/check_release_overrides.ps1`)

The **artifact**: a release binary must not contain the name. The list is
now the audit's list (7 names). The script scans the C++ release directory
**and** the Rust release directories (`rust/target/release` and
`rust/target/<triple>/release`).

**`-RequireBinaries` closes the vacuous pass**: without it, a missing or
empty build directory would look like a clean bill of health. The CI step
uses it.

**Result locally: 6 release binaries (engine, studio, compressor, K6WP and
the Rust `k6wp-studio.exe`), 0 findings.**

**The detector fires**: the same script against a **debug** binary
(`build/l1/engine.exe`) **fails** with the names it finds. A scan that
cannot fail is not a scan.

---

## 3. The self-test (a lint that stops detecting is worse than no lint)

`tools/lint_env_overrides_selftest.ps1` builds a scratch directory with
known-good and known-bad sources and asserts the verdict on each:

| case | expected |
| --- | --- |
| an ungated C++ read (`std::getenv("K6WP_ENGINE_PIPE")`) | the lint FAILS |
| an ungated Rust read (`std::env::var("K6WP_ENGINE_MUTEX")`) | the lint FAILS |
| a C++ read behind `#if K6WP_ALLOW_ENV_OVERRIDES` | the lint passes |
| a C++ read behind `#ifdef _DEBUG` | the lint passes |
| a Rust read behind `#[cfg(debug_assertions)]` | the lint passes |
| a read of an ordinary variable (`LOCALAPPDATA`) | the lint passes |
| a documented override (`K6WP_CACHE_DIR`) | the lint passes **and names it** |

**8 checks, 0 failures.** The self-test also caught a bug in the lint
itself (the "cannot fire" guard rejected a scratch file that legitimately has
no override) - the guard now applies to the tree scan only.

---

## 4. The CI wiring

| step | job | what it proves |
| --- | --- | --- |
| `The environment override lint` | `build-test` | the rule, on every tracked file |
| `The environment override lint self-test` | `build-test` | the lint still fires |
| `The release binaries hold no environment override` | `build-test` | the artifact (the C++ release binaries that are about to be packaged), now with `-RequireBinaries` |
| `The Rust release build + the override scan` | **`rust-release` (new)** | the Rust crates in a release build, scanned the same way |

The **Rust workspace was not in CI at all** before L5. The new job builds
`k6wp-core` and `k6wp-ipc` in release (with the examples) and scans the
result with `-RequireBinaries`. The Slint Studio is deliberately not built
there: the GUI build is slow and the Studio is not shipped yet; its
override is behind `#[cfg(debug_assertions)]` (proved by the source lint)
and its release binary was scanned locally (clean).

---

## 5. The cost-free/leak-free part

`K6WP_IPC_TRACE` obeys the same gate as the pipe and the mutex names
(`shared/build_flags.hpp`: `_DEBUG` only; the Rust: `debug_assertions`), so
in a release build `IpcTraceEnabled()` is a **constant false**: the compiler
removes the timers, the labels and the log calls. The proof is the byte
scan: the release engine contains **neither** the name `K6WP_IPC_TRACE` nor
the `ipc-trace` format string - the code is **gone**, not merely
unreachable. A removed branch cannot leak a handle, a thread or a byte, so
the "cost-free and leak-free" question does not arise.

The same holds for the other gated overrides: they are **not present in the
binary**, not present-but-false.

---

## 6. The open questions (the owner's, recorded not hidden)

1. **Should `K6WP_RECONCILE_MODE`, `K6WP_PLAYLIST_JSON` and
   `K6WP_THUMBS_DIR` be compiled out of the release build too?** They are
   path/behaviour redirections of the shipped alpha, and the release-mode
   test suite depends on two of them. Compiling them out means the Studio's
   roots must become **explicit** (a parameter instead of an environment
   variable) - a real change to the Studio's API, not a one-line gate. Until
   then they stay, classified, in the lint's allow-list.
2. **The release-mode test suite is not hermetic**: `K6WP_LIBRARY_JSON` is
   compiled out in a Release build, so the tests that set it fall back to
   the **real** `%LOCALAPPDATA%\K6WP` paths and write there. The Debug suite
   (the one this repository runs, `ctest --preset msvc-dev`) is unaffected.
   The fix belongs to item 1: the tests must pass their roots explicitly.
3. The Rust **Studio** release binary joins the CI scan when the Studio is
   packaged (2.7); until then the source lint is its guarantee.

---

## 7. Evidence index

| item | file |
| --- | --- |
| the source lint | `tools/lint_env_overrides.ps1` |
| the self-test | `tools/lint_env_overrides_selftest.ps1` |
| the binary scan | `tools/check_release_overrides.ps1` |
| the fix | `rust/crates/k6wp-core/src/sps.rs` (`K6WP_SPS_DEBUG`) |
| the CI | `.github/workflows/ci.yml` (3 steps + the new `rust-release` job) |
| the contract | `docs/refactor/contract-inventory.md` (§ the env overrides) |

**Labels:** the audit = **code-quoted** (the lint's own output: 272 files,
21 reads, 18 names); the release-binary scan = **test-proven** (6 binaries,
0 findings; the detector fires on a debug binary); the self-test =
**test-proven** (8 checks); the "cost-free/leak-free" claim =
**code-quoted** (the gate + the byte scan); the open questions =
**code-quoted** (the shipped behaviour and the tests that rely on it).
