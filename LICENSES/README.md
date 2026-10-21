# The third-party licences in this repository

Every bundled third-party licence lives here, with the file name the
component uses. The repository's own licence is the root `LICENSE`.

| file | component | licence |
|---|---|---|
| `ffmpeg.txt` | ffmpeg / ffprobe (the vendored binaries) | LGPL-2.1-or-later (with the GPL-2.0-or-later option for the parts that need it) |
| `libmpv.txt` | libmpv (mpv) | GPL-2.0-or-later (or LGPL-2.1-or-later, at the user's option) |
| `nlohmann-json.txt` | nlohmann/json (the C++ JSON) | MIT |
| `Qt.txt` | Qt (the C++ Studio, until Phase 2.7 removes it) | LGPL-3.0-only / GPL-2.0-or-later / GPL-3.0-only (the Qt licence) |
| `slint-GPL-3.0-only.txt` | Slint (the Rust Studio) | GPL-3.0-only |
| `slint-royalty-free-2.0.md` | Slint (the Rust Studio) | LicenseRef-Slint-Royalty-free-2.0 |
| `slint-software-3.0.md` | Slint (the Rust Studio) | LicenseRef-Slint-Software-3.0 (the paid / source-available plan) |

## The Slint licence decision (E7) — OPEN, the owner's call

**Slint 1.18.1 is TRIPLE-LICENSED:**

```
license = "GPL-3.0-only OR LicenseRef-Slint-Royalty-free-2.0 OR LicenseRef-Slint-Software-3.0"
```

So the earlier statement in the refactor notes — *"Slint is GPL-3.0-or-later,
so the combined work is GPL-3.0-or-later"* — is only true **if the GPL
option is chosen**. It is not forced:

1. **GPL-3.0-only** — the combined work (the Studio with Slint) becomes
   **GPL-3.0-only**. The repository is GPL-2.0-or-later, so the whole
   distribution would have to move to GPL-3.0 (or the Studio would have to
   be kept as a separate, GPL-3.0-only process with an IPC boundary).
2. **LicenseRef-Slint-Royalty-free-2.0** — a **royalty-free** licence that
   allows distributing the application under other terms, with attribution
   and three conditions: the licence must not be used for EMBEDDED systems,
   the software must not be distributed standalone (it has to be part of an
   application), and the application must not expose the Slint APIs.
   **This is the option that keeps the repository on GPL-2.0-or-later.**
3. **LicenseRef-Slint-Software-3.0** — the paid plan (the source-available
   licence of the paid tier); it also caps the distribution to the versions
   obtained under the paid plan.

**The decision is the owner's and it is not resolved here.** Until it is:

- **a public release is HELD** (the licence of the shipped Studio binary is
  not decided);
- the development continues — the choice affects the LICENSES/ files that
  ship, not the code.

**What is needed for the decision** (a legal review, not a code change):
the three licence texts in this directory, the product's distribution model
(a per-user desktop app, no embedded systems, no Slint API exposure — so
option 2 looks applicable), and the decision on whether the repository
stays GPL-2.0-or-later.

The Rust dependency policy (`rust/deny.toml`) allows all three Slint
licences, so the choice does not block the build.
