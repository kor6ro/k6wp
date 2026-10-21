# Phase 2 — the Studio in Rust/Slint (no Qt)

Date: 2026-10-21
Branch: `refactor/rust`
Status: **slice 2.0 done** (the shell), the feature slices follow.

## Why this phase exists (and what it is NOT)

The measurement says the Qt removal is **not** a bundle-size play: the Qt
group is 19.78 MB of a 148.13 MB ZIP (13.4 %) — dropping it entirely cannot
reach the old "≥ 30 % ZIP" target (`docs/refactor/phase1-3-bundle.md`).

It IS the right change for the **Studio process**: 337.53 MB of private
bytes and 1290 ms of startup. Those are the Phase 2 gates:

| metric | baseline (Qt Studio) | **gate** | the Slint shell (2.0) |
|---|---|---|---|
| Studio private bytes | 337.53 MB | **≤ 150 MB** | **2.4 – 8.8 MB** (debug build) |
| Studio startup (to the first window) | 1290 ms | **≤ 700 ms** | **422 ms** (median of 5) |
| threads | 55 | — | 4–5 |

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
