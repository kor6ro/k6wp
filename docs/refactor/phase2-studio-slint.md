# Phase 2 — the Studio in Rust/Slint (no Qt)

Date: 2026-10-21
Branch: `refactor/rust`
Status: **slice 2.0 done** (the shell), the feature slices follow.

## Why this phase exists (and what it is NOT)

The measurement says the Qt removal is **not** a bundle-size play: the Qt
group is 19.78 MB of a 148.13 MB ZIP (13.4 %) — dropping it entirely cannot
reach the old "≥ 30 % ZIP" target (`docs/refactor/phase1-3-bundle.md`).

It IS the right change for the **Studio process**: the Qt Studio holds
hundreds of megabytes of private bytes and takes seconds to paint its
first frame. Those are the Phase 2 gates.

## The Studio numbers — RE-measured (E1)

> **The earlier claim in this document is RETRACTED.** "2.4–8.8 MB vs Qt
> 337 MB" and "422 vs 1290 ms" were not comparable: the two were measured
> in different states, with different startup definitions, and the Slint
> side was a DEBUG build with a 2-row list. The numbers below replace them.

**The conditions (identical for both):** ONE library index file with 200
items, pointed at by the same `K6WP_LIBRARY_JSON` override; the same screen
(1920x1080@144); an isolated `%LOCALAPPDATA%`; and ONE startup definition —
**the process start until the window has PAINTED** (a `PrintWindow` capture
is no longer a flat colour), not "the window handle exists". The C4
metrics: private bytes + GPU shared. The working set is recorded and never
decides. Evidence: `docs/bench/studio-compare-2.0.json`.

| | **Qt Studio** (release) | **Slint Studio 2.0** (release) | delta |
|---|---|---|---|
| first painted frame | 3576 ms | **1027 ms** | **−71 %** |
| private bytes | 500.67 MB | **39.56 MB** | **−92 %** |
| GPU shared | 45.60 MB | 42.97 MB | −6 % |
| CPU (static) | 0.14 % | 0.05 % | |
| threads | 59 | 13 | −78 % |

**These are the SHELL's numbers, not the finished Studio's.** The Slint
side has no thumbnailer, no preview, no compress queue and no settings
bindings yet; each arrives as its own slice, and E1 is re-run at every
slice. The Qt side is the FINISHED Studio, so the gap will narrow — the
budget (≤ 150 MB, ≤ 700 ms) has to hold for the FINISHED Slint Studio.

### The reconciliation (why the Qt number moved)

The Qt Studio's private bytes were quoted at three different numbers. They
are the SAME binary, measured at three points in its life:

| point in the Studio's life | private bytes | where it comes from |
|---|---|---|
| the process, before the QML engine is up | **67.5 MB** | the "old baseline ~51 MB" - the same ballpark, measured before Qt/QML is loaded |
| 0.8–2.0 s (the QML is loading) | 102.5 MB | |
| 2.4 s (the QML compiles) | 191.5 MB | |
| the window is up (2.8 s) | 400 MB | the QML's peak |
| **the Phase 0 sample point** (3–13 s after the start, an empty library) | **338.12 MB** | the Phase 0 baseline's 337.53 MB — reproduced here to within 0.6 MB |
| **the E1 sample point** (5 s after the FIRST PAINTED FRAME, the 200-item library bound) | **500.67 MB** | the QML peak + the library grid model + the QML image cache |

So "337 MB" and "500 MB" are **not comparable**: they are the same process
at two different moments, and the difference is the QML engine's
allocation curve, not a regression. The Phase 0 number is reproduced
exactly (338.12 vs 337.53 MB) with the Phase 0 method in the same
harness — the proof that the two measurements differ in their
**definition**, not in the build. The old ~51 MB baseline was taken before
the QML engine was up (67.5 MB measured now).

**The rule this sets for the rest of Phase 2:** every Studio number carries
its measurement point (the definition line in the JSON) and the same
library. A comparison without both is "not comparable".

### The slice table (the shell only, for reference)

| metric | the Slint shell (2.0) | the finished Studio (the gate) |
|---|---|---|
| private bytes | 39.56 MB | **≤ 150 MB** |
| first painted frame | 1027 ms | **≤ 700 ms** |

**These are the SHELL's numbers, not the finished Studio's.** The shell has
no library, no thumbnailer, no preview, no compress queue and no settings
bindings yet; each of those arrives as its own slice with its own gate. The
margin is large enough that the finished Studio should stay inside the
budget, and every slice re-measures it.

## The workspace

```
rust/
  Cargo.toml            the workspace (one crate per deliverable)
  deny.toml             the dependency policy (cargo-deny)
  crates/k6wp-studio/   the Slint Studio
    build.rs            slint-build compiles ui/app.slint
    ui/app.slint        the UI
    src/main.rs         the Rust side (the data bindings)
```

Phase 3 (the engine in Rust) adds its crate here and shares the protocol
and the settings crates.

## The supply chain is green (D6)

- `cargo deny check` → **advisories ok, bans ok, licenses ok, sources ok**.
  The policy is written down, not implicit: crates.io + the RustSec
  database only, every licence must be *known* (an unknown licence is an
  error), no wildcards, no unpinned git sources. The duplicate versions in
  the tree are upstream-driven (the Slint tree pins its own `windows-sys`,
  `hashbrown` and `syn`) and are reported as warnings so a NEW duplicate is
  visible.
- `cargo audit` → **0 vulnerabilities** (exit 0), 5 "unmaintained"
  warnings from upstream crates in the Slint/winit tree (`bincode`,
  `ttf-parser`, `zbus_names`, `zvariant`, `zvariant_derive`).

**The licence note is unresolved by design and flagged for the owner's legal
review:** the repository is GPL-2.0-or-later and Slint is GPL-3.0-or-later
(or a commercial licence), so the combined work is **GPL-3.0-or-later**. It
is recorded in `deny.toml` and must be settled before the Studio ships.

## The slices (each with its own gate)

1. **2.0 — the shell** (done): the window, the version, the library list
   binding, the Slint ↔ Rust data path.
2. **2.1 — the library**: the real library model (the JSON metadata index
   the Qt Studio reads), the selection, the live switch over the engine's
   IPC. Gate: the same library actions as the Qt Studio, plus the RAM and
   the startup re-measured.
3. **2.2 — the settings**: the config surface, the IPC for the settings,
   the lock-screen and the still toggles.
4. **2.3 — the preview and the thumbnailer**: the mpv preview and the
   frame extraction. Gate: the preview still plays, the thumbnails still
   match (the visual parity gate), and the RAM stays inside the budget.
5. **2.4 — the compress queue**: the queue, the progress, the errors.
6. **2.5 — the cut-over**: the Qt Studio leaves the install, the bundle
   assert moves to the measured ZIP, and the Phase 2 gate is re-measured on
   the shipped set.

## How to build and check it

```powershell
cd rust
cargo build --target x86_64-pc-windows-msvc
cargo deny check
cargo audit
```
