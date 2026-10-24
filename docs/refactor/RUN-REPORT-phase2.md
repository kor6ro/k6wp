# RUN REPORT phase2

## T0-T16 status
| Task | Status | Commit | Artifact |
|---|---|---|---|
| T0 | partial | - | docs/refactor/run-phase2/t0-raw.txt |
| T1 | BLOCKED | - | no UIA rig; baseline not measured |
| T2 | partial | - | tools/lint_ui_thread.ps1, design-tokens.slint, docs/ux-spec.md |
| T3 | partial | - | tests/golden/studio_logic.json (skeleton) |
| T4 | BLOCKED | - | needs thumbnailer measurement + corpus |
| T5 | BLOCKED | - | needs render-backend measurement |
| T6 | BLOCKED | - | needs import dialog impl |
| T7 | BLOCKED | - | needs live engine + IPC wiring |
| T8 | BLOCKED | - | needs playlist UI |
| T9 | BLOCKED | - | needs engine additive fields (M3) |
| T10 | BLOCKED | - | needs settings/onboarding impl |
| T11 | BLOCKED | - | needs i18n/a11y impl |
| T12 | partial | - | docs/refactor/parity-checklist.md |
| T13 | partial | - | docs/refactor/gate2-phase2-summary.md (NO-GO) |
| T14 | not started | - | Qt not removed (NO-GO) |
| T15 | done | - | docs/refactor/license-decision.md |
| T16 | not started | - | - |

## Gate raw outputs
- Rust fmt: clean
- Rust clippy: not run fully (disk); core+ipc build clean
- Rust tests: 106 total; 104 pass; 2 fail (corpus-dependent: the_dpb_byte_budget_is_pinned, the_savings_... need corpus files not present)
- cargo deny/audit: not run (disk)
- ctest: BLOCKED - missing nlohmann/json.hpp dependency (never committed); cannot configure build
- lint_ui_thread self-test: PASS

## UX numbers
Not measured (T1/T12 blocked by rig). Targets <=3 steps install, <=2 clicks Library->set: UNPROVEN.

## Studio metrics
Not comparable (shell only, no thumbnails). E1 numbers are Qt Studio's, not finished Slint's.

## Bundle
Not changed (Qt not removed). Baseline 148.13 MB stands. Target <=115 MB deferred to Phase 3.

## UNPROVEN
second monitor, mixed DPI, HDR, sleep->wake, live engine IPC end-to-end, second-monitor rig, corpus-dependent tests, ctest (missing dep).

## BLOCKED
C++ build blocked by missing thirdparty/json.hpp (nlohmann). T4-T11 implementation not feasible in this session.

## Trade-offs
None taken this run beyond documenting.

## New findings
- nlohmann/json.hpp missing from repo; C++ side cannot build from this checkout. Owner must restore dependency or vendor it.
- Corpus only has 2 of many files; sps_golden tests corpus-dependent.

## Owner decisions pending
(a) Slint license (T15 doc), (b) go/no-go Qt removal (NO-GO now), (c) git history rewrite (not done), (d) readiness for Phase 3 (not yet).
