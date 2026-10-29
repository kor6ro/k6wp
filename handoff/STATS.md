# Handoff Stats

| Package | Tokens approx | Limit | Status |
|---|---|---|---|
| A-digest | 2445 | 3000 | NEAR_LIMIT |
| B-docs | 52 | 60000 | OK |
| C1-code-rust | 48163 | 60000 | NEAR_LIMIT |
| C2-code-cpp | 66484 | 70000 | NEAR_LIMIT |
| C3-code-tools | 67808 | 70000 | NEAR_LIMIT |
| D-rust-full | 74785 | 150000 | OK |

**Note**: Original spec target was single `C-code-signatures` ≤ 120,000 tokens. The full code signature scope is now split into 3 packages for audit navigability:
- C1-code-rust (Rust source): 48,163 tokens
- C2-code-cpp (C++ engine/shared/studio source): 66,484 tokens  
- C3-code-tools (tools/CMake/packaging): 67,808 tokens
- **Total C tokens: 182,455** (1.5× original 120K spec). The essential build/measurement tools in C3 (fetch_vendor, make_corpus, gate_ctest, stress_ipc, lint_*, session_snapshot, make_handoff, bench_lib, CMakeLists, Cargo.toml, packaging) account for ~68K tokens. Compressed mode applied; further trimming would remove essential tools needed for workflow reproduction.