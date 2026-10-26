# D-rust-full — Full Rust source (≤ 150,000 tokens)
Full content of: rust/**/src, rust/**/tests, rust/**/Cargo.toml, rust/**/ui/*.slint.
Excluded: rust/target/**, tests/corpus/** (names+sizes only for golden fixtures), .git/**.
Read first: rust/crates/k6wp-core/src/lib.rs, rust/crates/k6wp-ipc/src/lib.rs, rust/crates/k6wp-studio/src/main.rs.
Verification: cargo build/test/clippy --workspace --target x86_64-pc-windows-msvc; cargo deny check; cargo audit.
Note: `proud-garden` (official folder) is empty/non-repo; package built from active workspace `calm-otter` (clean working tree).
