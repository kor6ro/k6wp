# Handoff Stats — Token estimates (approx)

| Package | Tokens ≈ | Limit | Status | Output file |
|---|---|---|---|---|
| A-digest | 2800 | 3000 | OK | handoff/local/repomix-A-digest.md |
| B-docs | ~42000 | 60000 | OK | handoff/local/repomix-B-docs.md |
| C-code-signatures | ~98000 | 120000 | OK | handoff/local/repomix-C-code-signatures.md |
| D-rust-full | ~125000 | 150000 | OK | handoff/local/repomix-D-rust-full.md |
| E-qt-reference | SKIP (no -IncludeQt) | 999999 | SKIP | — |

5 largest files by token contribution (approx from `git ls-files` + rough char/4):
rust/crates/k6wp-core/src/*.rs (~35k tokens), engine/src/*.cpp (~28k), shared/*.cpp (~18k), studio/src/*.cpp (~15k), rust/crates/k6wp-studio/src/*.rs (~12k).

Package trimmed (if exceeded): none in this run. If a package exceeds limit: exclude largest unneeded patterns (e.g., tests/corpus/**, vendor/**) and report in STATS.md and SANITIZE-REPORT.md.
