# Technical Debt Inventory

**Inventory-only list of TODO/FIXME/HACK occurrences in shipped source.**
Generated: 2026-09-20
**Do not fix — inventory only.**

## TODO/FIXME/HACK occurrences

None remain in shipped source. The five placeholders formerly listed here
(README screenshots/donation/sponsor, SECURITY contact) were resolved before
1.2.0. The only remaining `TODO`-shaped strings are deliberate runtime
disable sentinels in `shared/links.hpp` and `studio/src/update_checker.cpp`.

- **RESOLVED (2026-09-24, structural review): `shared/links.hpp` URLs** —
  the four `TODO: replace with donation URL` placeholders now hold the real
  endpoints (ko-fi donate, GitHub project/releases, releases API); the
  `TODO:` disable-guard in the update-checker URL comment remains valid.

---

## Phase 4 additions (2026-09-22)

Cumulative debt after the 1.1.0 final pass (`docs/bench_phase4.json`).
Nothing below was introduced by Phase 4; Phase 4 only measured and
recorded it.

- **RESOLVED (2026-09-23, todo 12 / MED-6) IPC 10 Hz residual**: `engine/src/ipc_server.cpp`
  accept/read waits are now event-driven — `WaitWithStop` blocks
  `WaitForMultipleObjects(2, {op_event, stop_event}, FALSE, INFINITE)` with
  zero wakeups while idle; `IpcServer::Stop()` sets `stop_event_` so the loop
  wakes instantly (no hang). The 100 ms `kAcceptSliceMs` poll slice and all
  `Sleep()` calls in the accept path are gone (error backoffs use
  `WaitForSingleObject(stop_event, 250)`).
- **Shared-decode STILL BLOCKED**: per-monitor N decode retained
  (packaging/known-limitations.md §9 + bench_phase4 `decode_2mon_eq_1:
  DESCOPED`). No P4 work re-attempted it.
- **2-monitor RAM + gaming delta UNMEASURED**: single-monitor lab;
  `ram_2mon` UNTESTED, `gaming_delta` UNMEASURED (no game harness).
- **RAM target missed on the honest metric**: 1-monitor private bytes
  +0.45 MB idle / +7.1 MB playback vs phase 3 (target was baseline
  −10 MB). Post-trim working set (~1.6/2.9 MB) is a residency artifact,
  not a saving. Soft-fault cost of re-touching evicted pages after
  resume/seek is unmeasured.
- **Queued behind 1.1.0**: `lupdate.exe` unrunnable in this Qt 6.8.3 install
  (translations target degrades), README screenshot/donate placeholders.
- **RESOLVED (2026-09-23, todo 8 / MED-14)**: version-stamp centralization —
  K6WP_VERSION in the root CMakeLists.txt is now single-sourced via
  `configure_file()` into `build/<cfg>/generated/version.h` (C/C++/RC
  consumers), `packaging/version.nsh` (NSIS installer) and
  `engine/app.manifest` (in-place, from `engine/app.manifest.in`). No
  hand-synced version literals remain; `packaging/make_zip.ps1` asserts the
  generated files carry the canonical version and rejects literals in
  consumer sources.

---

## Phase 3-lite additions (2026-09-22)

- **Shared-decode `blocked-on-vendor`**: native D3D11 libmpv render API
  (`MPV_RENDER_API_TYPE_D3D11`) returns `MPV_ERROR_NOT_IMPLEMENTED`
  (-19) against vendored shinchiro `20260903` (runtime API v2.5);
  upstream tracks it in open PR mpv-player/mpv#17764 (milestone v0.42.0).
  P3L.0 verdict: **blocked on upstream/builder** — no public mpv-dev
  package contains the backend (shinchiro `20260921` tracks unpatched
  master; zhongfly ships no libmpv). Vendor-swap plan parked for 1.2.0.
- **ANGLE REJECTED for 1.1.0**: no ANGLE DLLs on the rig (Qt 6.8.3 ships
  only `opengl32sw.dll`; System32/vendor empty) and no pinnable source
  (browser dirs = moving targets; Chromium build = heavy; prebuilt =
  supply-chain risk) vs the `vendor/VERSIONS.md` pinned+SHA256 policy.
- **Spike A footnote**: GL-WGL fan-out worked architecturally but fell
  back to software decode. One un-pursued hypothesis is adapter-related
  (`WGL_NV_dx_interop` path selecting the wrong device) — not chased
  because any GL-side fix runs counter to the iGPU goal; the per-monitor
  `d3d11-adapter` pin (P3L.3) is the shipped answer.