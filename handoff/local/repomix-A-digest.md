# HANDOFF-DIGEST.md — K6WP refactor (≤ 3000 tokens)

Generated: 2026-10-26. Read first: `docs/refactor/STATE.md`, `ledger.md`, `budgets.md`. Official folder: `proud-garden` (empty/non-repo at this session); active workspace: `calm-otter`. No measurements in this run.

---

## 1. Apa ini
K6WP — engine C++ (reference) + Studio Slint (separate process, no mpv, no HWND-embed). Refactor tujuan: lower RAM/startup tanpa turunkan kualitas wallpaper; rapi; just better. Kriteria accept: Studio full Rust paritas; engine tetap C++ untuk rilis ini; lower usage; UX; rapi; just better (`docs/refactor/STATE.md` §1).

---

## 2. Keputusan terkunci (10 baris)
| # | Keputusan | Bukti |
|---|---|---|
| L1 | DPB = byte budget (`effective_refs × w × h × 1.5 > 64 MB`), bukan count; level dilaporkan, bukan cap | `phase1-4-sps-dpb.md` |
| L2 | `num_ref_frames` dari Rust SPS parser (9/9 vs ffmpeg `trace_headers`) | `k6wp-core/src/sps.rs` |
| L3 | Config v6 (`pause_unload_after_sec` default 30 s, 0 = never); newer file best-effort read; unknown keys preserved saat rewrite | `phase1-4c-config-policy.md` |
| L4 | Unload = destroy whole mpv instance; hanya owner yang membuat invisible (fullscreen/screen-off/lock/suspend); user pause = keep picture | `phase1-1-unload.md` |
| L5 | Studio = separate process (no mpv, no HWND-embed) | `phase2-studio-slint.md` |
| L6 | OS min = Windows 10 22H2 (build 19041); engine/launcher/Studio exit 6 jika lebih tua | `C6` |
| L7 | Golden fixtures = kontrak; valid hanya jika terbukti vs `baseline-pre-refactor` atau perubahan dicatat sengaja | `g5-coverage.md` |
| L8 | Env overrides (`K6WP_ENGINE_PIPE`, `K6WP_ENGINE_MUTEX`, `K6WP_LIBRARY_JSON`) dikompilasi keluar dari release binary dan di-scan | `phase2-l5-override-proof.md` |
| L9 | Rilis publik TERTAHAN sampai owner putuskan lisensi Slint (`license-decision.md`, F3) | `LICENSES/` |
| L10 | ZIP tetap 148.13 MB; target 115 MB ditunda Phase 3; `ffprobe.exe` hanya dihapus setelah J6 | `G7`, `budget.md` |

---

## 3. Status per fase/run (tabel: artefak yang ada di repo)
| Phase | Deliverable utama | Status | Bukti | Path |
|---|---|---|---|---|
| 0 | Baseline + kontrak | Selesai | `live-proven` / `test-proven` | `budget.md`, `ledger.md` |
| 1.1 + D2 | Unload setelah pause panjang | Selesai (Gate 1 PASS) | 0 black frames (n=5, 170 sampel); resume median ~798 ms | `phase1-1-unload.md`, `docs/bench/unload-d2.json` |
| 1.2 + C1 | Pacing fix (`display-vsync`) | Selesai | Rate 0.98-1.00; 0 late frames; pixel diff 0.000 | `phase1-2-vsync.md`, `docs/bench/ab-vsync.json` |
| 1.3 + D4 | ffprobe census; removal otorisasi | Selesai (pengukuran) | 0 disagreement (5 field); parser 9/9; ZIP unchanged | `phase1-3-bundle.md` |
| 1.4 + E3 | DPB parser + corpus | Selesai | Byte budget (L1); 9/9 match; 101 golden | `phase1-4-sps-dpb.md` |
| 1.5 | Span + old-OS branches removed | Selesai | -345 LOC net; ctest 49/49 | `ledger.md` |
| 2.2 | `k6wp-core` Rust contracts | Selesai | 101 shared golden; replay C++ 92/92 | `rust/crates/k6wp-core/` |
| 2.3 | Slint shell (FemtoVG default) | In progress | FemtoVG measured (39.56 MB, 1027 ms); software + Skia UNPROVEN | `adr-slint-backend.md` |
| 2.4 | UX spec + design system | Open | `ux-spec.md` ada; flow script belum | — |
| 2.5 | Library pages (F1 virtual grid + F2 lazy thumbnailer) | Open | — | — |
| 2.6 + G6 | Rust IPC client (`k6wp-ipc`) | Selesai | Server verified (pid, session, owner SID, image path) sebelum command | `phase2-k1-ipc-latency.md` |
| 2.7 | Drop Qt (setelah 100% parity + J4 Studio) | Open | — | — |
| E1 | Studio comparison (Qt vs Slint 2.0) | Selesai (re-measured) | Qt 500.67 MB / 3576 ms; Slint 2.0 39.56 MB / 1027 ms | `docs/bench/studio-compare-2.0.json` |
| E2 | Slint renderer choice | Accepted (provisional) | Default FemtoVG; software/Skia UNPROVEN | `adr-slint-backend.md` |
| E5 | Test bounded + pipe unique | Selesai | 300 s timeout; unique pipe per test; 49/49 ctest with live engine | `phase2-e5.md` |
| E6 | Gate 1 doc complete | Selesai | Resume latency median 798 ms; 0 black frames; 5 evidence rules | `docs/refactor/gate1-phase1-summary.md` |
| E7 | Lisensi Slint disiapkan | Selesai (keputusan = owner) | Triple-license (GPL-3.0-or-later OR Slint-Royalty-free-2.0 OR Software-3.0); rilis tertahan | `license-decision.md` |
| G1 | Byte-budget threshold | Selesai (owner-directed trade-off) | Held-back RAM terukur (n=5, medians + band); threshold 64 MB tetap | `h1-ab-*.json` |
| G2 | Parser hardened | Selesai | Caps, checked arithmetic, proptest, mutation + 4 regression vectors | `docs/refactor/g2-*` |
| G3 | Ctest notes reconciled | Selesai | 49/49 dengan live engine (51 s) | `budget.md` |
| G4 | Schema v6 + old-engine compatibility | Selesai | Best-effort read newer file; unknown keys preserved; downgrade: file byte-identical, .bak sama | `docs/refactor/phase1-4c-config-policy.md` |
| G5 | Engine pure logic frozen | Selesai | 91 cases / 9 modules; C++ + Rust replay 92/92 | `tests/golden/engine_logic.json`, `g5-coverage.md` |
| G6 + 2.6 | Rust IPC client verified | Selesai | Client reconnect fix (releases old handle first); phantom failure fixed; 5/5 clean 20k requests | `phase2-l4-ipc-client-retry.md` |
| G7 | ZIP target jujur | Selesai | 148.13 MB; 115 MB target DEFER Phase 3 | `docs/bench/bundle-zip.json` |
| H1 | G1 verdict jujur | Selesai | Trade-off dicatat; data tidak dukung perubahan threshold | `docs/refactor/phase1-4d-h1-threshold-verdict.md` |
| I1 | G5 fixtures vs baseline | Selesai | 213 nilai dibanding, 0 bedanya, 24 dilewati & dideklarasi; provenance map | `g5-coverage.md` |
| I2 | SPS parser seek | Selesai | 512 MB non-faststart = 14.628 bytes; container policy eksplisit (Matroska/WebM/AVI/MPEG-TS fallback); cargo-fuzz 4 target compile | `docs/refactor/I2.md` |
| I3 | Config round trip | Selesai | 6 fixture case; unknown keys preserved; atomic publish; downgrade policy | `docs/refactor/I3.md` |
| I4 | Overrides compiled out | Selesai | 1 violation (`K6WP_SPS_DEBUG`) diperbaiki; lint (272 file, 21 baca, 0 error) + binary scan (6 binary, 0 temuan) + self-test (8 cek) | `phase2-l5-override-proof.md` |
| J1 | Seeded random differential | Open (before 3.7) | — | — |
| J2 | Two flakes rooted | Selesai (satu temuan tetap open) | `still_test` root cause proven (mtime collision); lint 43 sumber, 0 pelanggaran; 10x ctest hijau (50-59 s); open: `studio_logic_test` readywait engine > 2 s (client reconnect phantom) | `docs/refactor/phase2-j2-flake-roots.md` |
| J3 | Config writers under one lock | Selesai | C++ + Rust; 366 cek lulus; negative control 0/5 tanpa kunci vs 5/5 | `docs/refactor/J3.md` |
| J4 | Studio goldens / 12 TO FIXTURE rows | Open (before 2.7 / 3.7) | — | — |
| J5 | Fuzz run or honest record | Open (before release) | ASAN runtime belum terpasang; target compile | — |
| J6 | SPS probe budget before ffprobe removal | Open (before removal) | ffprobe spawn 30 ms; swap belum dibenarkan; ZIP tetap 148.13 MB | — |
| K1 | IPC slow ack rooted | Selesai | Toolhelp snapshot (~75 ms) di-cache (p50 1 ms, p99 3-4 ms); L1 hapus field; 10 menit stress: 604 s, p99 3 ms, max 152 ms, 0 "ack write failed" | `docs/bench/ipc-stress-10min.json`, `docs/bench/l1/` |
| L1 | `get_state` max rooted + removed | Selesai | Field (tidak ada konsumen) + snapshot dihapus; max median 56 -> 7 ms (n=5), 0/20.000 >= 20 ms; slow samples 981 ms apart = 1 s TTL | `docs/bench/l1/` |
| L2 | Poll-path cost audit | Selesai | Setiap field tidak > 1 ms; `exists` stat (21.053 ms SMB mati) dihapus; audit tidak temukan I/O poll-path lain | `docs/refactor/ipc-poll-cost.md`, `docs/bench/l2/` |
| L3 | Engine CPU + wakeups dengan Studio polling | Parallel dengan 2.4 | — | — |
| L4 | Rust client reconnect + bounded retry + real bug fixed | Selesai | `WaitNamedPipe` bounded; reconnect release handle dulu; phantom failure (0 bytes saat `ReadFile` sinkron) diperbaiki; 5/5 bersih 20.000 req; ~0.25% baseline (3/1200) dicatat | `docs/bench/l4/` |
| L5 | Overrides compiled out + CI lint + binary scan | Selesai | 1 pelanggaran (`K6WP_SPS_DEBUG`) diperbaiki; lint + scan di CI; `rust-release` job baru; 4 override tetap dengan alasan | `phase2-l5-override-proof.md` |
| M7 | Rust IPC deterministic tests | Selesai | 14 test lulus (9 original + 5 baru) | `rust/crates/k6wp-ipc/src/client.rs` |
| N2 | J3 config lock evidence | Selesai | 366 cek lulus; tests 77/78/79; UI thread tidak menunggu kunci | `docs/refactor/config_test.cpp` |
| N3 | Corpus hygiene | Selesai | 17 file gitignored (~1.1 GB); golden 16 case (+anime_4k_60s, highdpb_4k_6ref); open: 4K 6-ref A/B n≥5, HEVC 8/10-bit ABAB | `docs/refactor/N3.md` |
| A-12c | Lokasi kerja resmi | Selesai (folder resmi `proud-garden` kosong/non-repo; active workspace `calm-otter`) | `docs/refactor/STATE.md` §0 |
| A-8..A-11 | WIP belum commit | Partial | A-8/A-9/A-10/A-11 summary tercatat; beberapa belum selesai (`Urungkan` toast, GPU sampler, A/B RAM, ADR thumbnailer) | `STATE.md`, `RUN-A8/9/10/11.md` |

**ROADMAP.md**: UNPROVEN — file tidak ada di repo.

---

## 4. Angka terukur (hanya angka dengan file bukti)
| Metrik | Baseline v2 | Sekarang | n | Label | File |
|---|---|---|---|---|---|
| Private bytes, paused 30 s | 162.35 MB | 60.1 MB | 5 | `live-proven` | `budget.md`, `docs/bench/unload.json` |
| GPU shared, paused 30 s | 151.57 MB | 0 MB | 5 | `live-proven` | `budget.md` |
| CPU playback 1080p | 4.73 % | 1.06 % | 5 | `live-proven` | `docs/bench/ab-vsync.json` |
| GPU 3D playback 1080p | 12.47 % | 9.39 % | 5 | `live-proven` | `docs/bench/ab-vsync.json` |
| Playback rate (desync -> vsync) | 0.70-0.75 | 0.98-1.00 | 5 | `live-proven` | `docs/bench/ab-vsync.json`, `phase1-2-vsync.md` |
| Rendered-frame parity | 184-197 late frames | 0 | 5 | `live-proven` | `docs/bench/ab-vsync.json` |
| Private bytes 4K/16-ref | 387.23 MB | 228.2 MB (↓41%) | 5 | `live-proven` | `docs/bench/dpb-ab.json` |
| GPU shared 4K/16-ref | 468.08 MB | 432.5 MB (↓7.5%) | 5 | `live-proven` | `docs/bench/dpb-ab.json` |
| Studio Qt vs Slint 2.0 (private / startup) | 500.67 MB / 3576 ms | 39.56 MB / 1027 ms | 1 (isolated) | `live-proven` | `docs/bench/studio-compare-2.0.json` |
| IPC ack p99 (idle engine) | 84 ms (baseline) | 3-4 ms (K1 cache) -> L1 hapus field (max 4-14 ms, p99 0-1 ms) | 4000/req, n=3-5 | `live-proven` / `test-proven` | `docs/bench/ipc-stress-10min.json`, `docs/bench/l1/`, `docs/bench/l2/` |
| IPC per-field cost (`get_state`) | — | Semua < 1 ms | 4000/req, n=3 | `test-proven` | `docs/bench/l2/`, `docs/bench/l4/` |
| ZIP size (ffprobe pending) | 148.13 MB | 148.13 MB (target 115 MB DEFER) | — | `code-quoted` | `docs/bench/bundle-zip.json` |
| Golden fixtures (C++ + Rust replay) | 92 (C++) | 92 (Rust replay sama) | — | `test-proven` | `tests/golden/shared_behavior.json`, `tests/golden/engine_logic.json` |
| Studio test total | — | 127 (core 93, ipc 13, studio 21) | — | `test-proven` | `docs/refactor/ledger.md` (A-10) |
| Ctest (live engine) | — | 49/49 (51-75 s) | 10 run berturut | `test-proven` | `docs/bench/ctest-stability.json` |
| Config lock (C++ + Rust interleaved) | — | 366 cek lulus | — | `test-proven` | `docs/refactor/config_test.cpp` (tests 77-79) |

**Perhatian sebanding**: Studio Qt (500.67 MB) vs Slint 2.0 (39.56 MB) adalah kondisi IDENTIK (library index 200 item, `K6WP_LIBRARY_JSON` override sama, isolated `%LOCALAPPDATA%`, startup = proses start sampai window PAINTED). Studio Slint final TIDAK sebanding sampai F1/F2 (virtual grid + thumbnailer) selesai — angka sekarang adalah shell 2.0 saja. Software/Skia renderer UNPROVEN (tidak diukur). `docs/bench/studio-compare-2.0-empty.json` (shell kosong) tersedia untuk perbandingan.

---

## 5. Klaim yang ditarik / UNPROVEN
- M2 "0 vs 3" (file `studio-ipc-policy.json` tidak ada saat klaim dibuat) — ditarik; data parsial (idle=0, scroll=2). Label: `UNPROVEN` (`docs/bench/studio-ipc-policy.json` dibuat setelah koreksi).
- UIBLOCK = watchdog noise — data parsial (idle=0, scroll=2, watchdog belum diuji tanpa noise). Label: `UNPROVEN`.
- GPU shared Studio Slint — UNPROVEN (iGPU host; tidak terukur secara terpisah).
- `studio_logic_test` readywait engine > 2 s — open (client reconnect phantom, bukan engine lock). `docs/bench/l4/` menunjukkan 5/5 bersih 20.000 req setelah perbaikan client.
- Software + Skia renderer — UNPROVEN (tidak diukur). `adr-slint-backend.md` menyatakan kondisi untuk buka kembali sebelum cut-over (2.7).
- Q1a (timeout overlapped-I/O-specific): passes dengan kode lama juga (deadline ConnectOptions menangkapnya) — label `UNPROVEN`.
- Q1b (race determinism timeout): race tidak deterministik — label `UNPROVEN`.
- Q1d (nextest CI gate): nextest belum terpasang di CI — label `UNPROVEN`; langkah install ditambahkan (`rust/Cargo.toml` + workflow).
- 5 baris `LIVE TEST` (`g5-coverage.md`): fullscreen/occlusion classification, device-lost watch, watchdog, launcher lock-screen policy — belum diuji langsung Win32.
- 12 baris `TO FIXTURE` (`g5-coverage.md`): belum dibekukan sebelum 3.7.
- `ffprobe.exe` removal: syarat J6 belum terpenuhi (budget ZIP tetap 148.13 MB).
- ZIP target 115 MB: DEFER Phase 3 (tidak "hampir").
- Roadmap.md: UNPROVEN — file tidak ada di repo.

---

## 6. Peta repo + perintah
- Build C++: `cmake --preset msvc-dev; cmake --build --preset msvc-dev; ctest --preset msvc-dev` (50/49 test, 54-75 s).
- Rust: `cd rust; cargo build/test/clippy --workspace --target x86_64-pc-windows-msvc`.
- Proof: `powershell -File tools/g5_diff_baseline.ps1` (differential); `tools/check_release_overrides.ps1` (5 binary bersih); `tools/drive_engine_ipc.ps1` (live IPC); `tools/bench_unload_d2.ps1` (black-frame gate); `tools/bench_dpb_ab.ps1` (DPB A/B).
- Gate: `tools/gate_ctest_stability.ps1` (10x berturut-turut hijau).
- Struktur repo utama: `rust/` (core, ipc, studio), `engine/` (C++ reference), `shared/` (C++ contracts), `studio/src/` (C++ Qt), `studio/qml/` (QML), `compressor/`, `launcher/`, `tests/`, `docs/bench/` (JSON bukti pengukuran), `docs/refactor/` (ADR, gate, ledger, STATE), `tools/` (skrip pengukuran + verifikasi).
- `repomix.config.json` dan `tools/make_handoff.ps1` dibuat di run ini; `handoff/` (local + share) dibuat oleh skrip.

---

## 7. Sisa pekerjaan (A-13a tetap NEXT)
Dari `ROADMAP.md`: tidak ada (file tidak ada). Dari `STATE.md` open commitments:
- 2.4 (UX + design system) -> 2.5 (pages) -> J1 (seeded random) / J5 (fuzz) / J6 (ffprobe) / J4 (golden Studio + 12 TO FIXTURE) per deadline -> F3 (lisensi) -> 2.7 (drop Qt) -> Gate 2 -> Phase 3 -> E4 (lean ffmpeg/libmpv, opsional, deferred).
- A-13a (ADR thumbnailer) adalah NEXT TASK saat ini; belum dimulai di run ini (`docs/refactor/adr-thumbnailer.md` belum ada).
- Kecepatan aktual: sejak A-5, deliverable selesai per run (A-5: config/hygiene; A-6: DPB parser; A-7: Studio 2.0 shell + E2; A-8: test aman-paralel; A-9: test count + E2e; A-10: klaim ditarik + M2; A-11: M2 data + `studio-ipc-policy.json`; A-12: lokasi resmi + STATE update). Rata-rata: ~1 deliverable utama per run; 7 run (A-5..A-11) menyelesaikan 1 fase utama + beberapa sub-tugas.
- Estimasi jujur sisa run: 2.4 (1 run) + 2.5 (2-3 run: F1 grid + F2 thumbnailer + import + kompresi) + J4 (2-3 run: golden Studio + 12 TO FIXTURE) + J1/J5/J6 (1-2 run paralel) + F3 (1 run, keputusan owner) + 2.7 (1 run) + Gate 2 (1 run) = **9-13 run** untuk Studio full Rust paritas + rilis. Tanpa F3 (lisensi belum putus) rilis tetap tertahan.
- Kandidat pemotongan lingkup (usulan untuk owner): i18n id (tidak disebut di kriteria accept); matriks DPI penuh (hanya dasar di STATE); dialog sekunder (tidak disebut); ADR sekunder (bisa ditunda setelah rilis); hevc 8/10-bit ABAB (belum terukur, bisa ditunda); 4K 6-ref A/B (open, bisa ditunda jika tidak berdampak parity). Dampak terhadap kriteria accept: pemotongan ini TIDAK menurunkan "lower usage tanpa menurunkan kualitas wallpaper" atau "UX"; hanya mengurangi ruang lingkup tambahan yang belum terkunci.

---

## 8. Risiko dan jebakan
- Agent berhenti dini / ringkasan kosong: selalu tulis `NEXT TASK` di `STATE.md`; jangan menyelesaikan dengan ringkasan kosong.
- Laporan menyalin laporan lama: verifikasi file bukti (`STATS.md`, `docs/bench/*.json`) sebelum klaim; setiap angka harus punya file.
- Klaim tanpa file bukti: semua tabel di §4 menyertakan path file; klaim tanpa file = `UNPROVEN`.
- Hitungan test tidak konsisten: `ctest` berubah antara run (live engine menambah ~15 s); selalu sebut `n` dan kondisi (engine up/down).
- Lingkungan: folder resmi `proud-garden` kosong/non-repo; workspace aktif `calm-otter` (`branch` tidak eksplisit tapi `git status` bersih); `C:\Users\axioo\Desktop\k6wp` TIDAK kanonik.
- WIP belum commit: A-8..A-11 commit di `calm-otter` (`9c96f8f`, `4791b80`, `d11e19a`); A-12 lanjut di `calm-otter` (folder `proud-garden` kosong).
- Cadangan bundle: `tests/corpus/` gitignored; regenerasi dengan `tools/make_corpus.ps1` jika hilang.
- Konteks: `HANDOFF-DIGEST.md` adalah satu-satunya bagian yang ditulis LLM; sisanya (skrip, konfigurasi, ringkasan angka, aturan biaya) dari dokumen yang sudah ada.
- Perilaku agent yang berulang: jangan ulang verifikasi pemulihan/pengamanan yang sudah tercatat selesai (I4: 5/5 binary bersih; G6: 0 failure; J2: 10/10 hijau; L5: lint + scan + self-test hijau).

---

## 9. Keputusan menunggu owner
- **Lisensi Slint (F3)**: triple-license memungkinkan repo tetap GPL-2.0-or-later (tidak otomatis GPL-3.0-or-later). Keputusan = owner (`docs/refactor/license-decision.md`).
- **Riwayat git**: A-5/A-6/A-7 commit di `calm-otter` (`refactor/rust`); `proud-garden` kosong. Perlu konfirmasi apakah `proud-garden` akan di-populasi atau `calm-otter` tetap kanonik.
- **Uji manual mesin nyata**: semua angka `live-proven` dari satu mesin (host ini); `UNPROVEN` tetap sampai diuji di mesin lain.
- **Item UNPROVEN yang diterima**: Q1d (nextest), Q1a/Q1b (timeout determinism), `g5-coverage.md` 5 LIVE TEST + 12 TO FIXTURE, `studio_logic_test` readywait (>2 s), software/Skia renderer. Owner harus menyetujui mana yang diterima sebagai cukup untuk rilis.
- **Opsi strategi rilis**: (a) rilis engine baru dengan Studio Qt dulu (tidak perlu F3, tapi tidak memenuhi "Studio full Rust"); (b) tunggu F3 + J4 + Gate 2 (sesuai kriteria accept); (c) hibrida: rilis engine + Studio Qt (Qt tetap), lanjut Rust di cabang paralel. Dampak: (a) cepat tapi tidak capai target; (b) lengkap tapi lambat; (c) kompromi.

---

## 10. Pertanyaan untuk reviewer eksternal
1. Apakah `STATS.md` (token per paket) konsisten dengan batas (A ≤3k, B ≤60k, C ≤120k, D ≤150k)? Apakah pemotongan (jika ada) dilaporkan jujur?
2. Apakah klaim tanpa file bukti (`UNPROVEN`) ditandai jelas, bukan disamarkan sebagai `live-proven`?
3. Apakah rencana anggaran ~US$10 sisa realistis (9-13 run, 1 deliverable/run, tanpa side quest)? Kandidat pemotongan mana yang paling aman?
4. Apakah `SANITIZE-REPORT.md` menunjukkan 0 temuan rahasia (email, token, path pengguna)? Jika ada yang tidak yakin, apakah `share/` tidak dibuat?
5. Apakah `REVIEWER-PROMPT.md` cukup untuk memandu AI lain tanpa membaca dokumen besar lain?

---
*Digabung dari STATE.md, ledger.md, budgets.md, ux-spec.md, docs/bench/*.json (hanya file ≤20 KB dibaca isi; yang lebih besar hanya nama+ukuran: `pacing-144hz.json`, `pacing-60hz.json`, `ab-vsync.json` > 20 KB). ROADMAP.md: UNPROVEN (tidak ada). Tidak ada pengukuran baru dijalankan.*
