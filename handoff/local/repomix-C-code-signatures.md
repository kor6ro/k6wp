# C-code-signatures — Compressed signatures (≤ 120,000 tokens)
Compressed mode (`--compress`): only class/function/interface signatures; comments removed; empty lines removed.
Files covered: rust/**/src, engine/**, shared/**, studio/src/**, compressor/**, launcher/**, tools/*.ps1, CMakeLists.txt, Cargo.toml, packaging/*.ps1 / .nsi.
Excluded patterns applied: tests/corpus/**, vendor/** (except VERSIONS.md), thirdparty/**, build/**, rust/target/**, .git/**, docs/img/**, LICENSES/**, shadow-evidence/**, video/**, brand/**, **/Cargo.lock, tests/golden/*.json (names+sizes), *.log, .env*, handoff/**.
Full signatures are in source; this package is a curated index for rapid review.
Key files by size/token contribution (estimates): rust/crates/k6wp-core/src/*.rs (largest), engine/src/*.cpp, shared/*.cpp, studio/src/*.cpp.
Use `Select-String` or `rg` to locate specific signatures in repo; do not open >300 line files whole.
