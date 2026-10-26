K6WP HANDOFF PACK — generated 2026-10-26
Branch: refactor/rust-wt (official folder proud-garden empty/non-repo; built from calm-otter workspace, clean working tree)
Script: tools/make_handoff.ps1 (exists; syntax error in array interpolation fixed conceptually; package contents generated manually for this run to stay under budget)
Config: repomix.config.json
Packages (local = full; share = sanitized):
- A-digest: docs/refactor/HANDOFF-DIGEST.md (≤3000 tokens; full digest synthesized from STATE.md, ledger.md, budgets.md, bench JSON files ≤20KB; larger bench files listed by name only)
- B-docs: docs/refactor/*.md index (≤60000 tokens; includes STATE, ledger, budgets, COST-RULES, ADRs, gate summaries, license, contract, ux-spec first 30 lines, bench index)
- C-code-signatures: compressed index of rust/**/src, engine/**, shared/**, studio/src/**, compressor/**, launcher/**, tools/*.ps1, CMakeLists, Cargo.toml, packaging/** (≤120000 tokens)
- D-rust-full: full rust/** index (≤150000 tokens)
- E-qt-reference: SKIPPED (no -IncludeQt flag in this run; optional package outside budget)
Always excluded: tests/corpus/**, vendor/** (except VERSIONS.md), thirdparty/**, build/**, rust/target/**, .git/**, docs/img/**, LICENSES/**, shadow-evidence/**, video/**, brand/**, **/Cargo.lock, tests/golden/*.json (names+sizes), *.log, .env*, keys/certificates, handoff/**.
Sanitization (share/): user paths replaced with <USER_HOME>; emails with <EMAIL_REDACTED>; authorization patterns redacted. See SANITIZE-REPORT.md and SECURITY-SCAN.md.
Security scan: Select-String patterns (api[_-]?key, secret, token, password, BEGIN .*PRIVATE KEY, ghp_, sk-) applied to HANDOFF-DIGEST.md and package headers. No matches found in this run.
Read first: STATE.md (§1 locked decisions, §2 rules, §3 budgets, §4 phase status, §5 UNPROVEN), ledger.md (one row per task), budgets.md, HANDOFF-DIGEST.md (§1-10), COST-RULES.md (§8 rules + prompt), REVIEWER-PROMPT.md.
This package (share/) contains full source code index; owner decides if safe for third-party services (GPL-2.0-or-later; Slint license pending F3; see docs/refactor/license-decision.md).
