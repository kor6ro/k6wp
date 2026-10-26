# B-docs — Curated docs package (≤ 60,000 tokens)
Includes (raw, no HTML comments stripped beyond existing state):
- STATE.md (full — read first)
- ledger.md (full — one row per task)
- budgets.md (full — protected metrics + gates)
- COST-RULES.md (this package's rules)
- adr-*.md (ADR files: slint-backend, thumbnailer if exists)
- gate*.md (gate summaries)
- license-decision.md
- parity-checklist.md
- contract-inventory.md
- ux-spec.md (first 30 lines shown; full in repo)
- RUN-A8/9/10/11.md, RUN-HANDOFF.md (run reports)
Excluded from package content (names + sizes only): docs/img/*.png, docs/bench/*.json > 20 KB (listed below).

JSON files included fully (≤ 20 KB): budget, g5-coverage, phase0, resume, studio, unload, bundle, dpb-ab, ctest-stability, ipc-stress-10min, studio-compare-2.0, studio-library-200.
JSON files > 20 KB (name + size only): pacing-144hz.json (~237 KB), pacing-60hz.json (~237 KB), ab-vsync.json (~39 KB).

Complete file list of docs/refactor included in this package is available via `git ls-files docs/refactor/`.
