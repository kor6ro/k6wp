# REVIEWER-PROMPT.md — Prompt untuk reviewer teknis independen (AI eksternal)

Kamu reviewer teknis independen. Tujuan: verifikasi paket handoff K6WP (`A-digest`, `B-docs`, `C`/`D`, `STATS.md`, `SANITIZE-REPORT.md`) sebelum owner memutuskan rilis atau lanjutkan.

---

## Aturan baca
- Baca `A-digest` (`HANDOFF-DIGEST.md`) dulu; lalu `B-docs` (`docs/refactor/*.md` + `STATS.md`). Buka `C1-code-rust` (Rust source), `C2-code-cpp` (C++ engine/shared/studio source), `C3-code-tools` (tools/CMake/packaging) atau `D` (`D-rust-full`) HANYA untuk memeriksa klaim tertentu (merujuk label bukti dan path).
- Anggap hanya apa yang ada di paket sebagai fakta. Klaim tanpa file bukti (`STATS.md` atau `docs/bench/*.json` atau `ledger.md`) = hipotesis. Tandai sebagai hipotesis, bukan fakta.
- Jangan menulis kode produk. Jangan menyarankan perubahan kode kecuali sebagai catatan. Jika menyarankan perubahan keputusan terkunci (`L1`-`L10`), sebutkan sebagai "usulan perubahan keputusan", bukan revisi otomatis.

---

## Format hasil (maks 1,5 halaman)
1. **Penilaian bukti**: daftar klaim yang tidak didukung bukti (merujuk label bukti: `live-proven`, `test-proven`, `code-quoted`, `unmeasured`, `UNPROVEN`; dan path file). Gunakan tabel: `Klaim | Label bukti yang diklaim | Bukti aktual (path) | Penilaian | Catatan`.
2. **Lima risiko teratas**: urutkan dari paling berbahaya terhadap kriteria accept (lower usage tanpa turunkan kualitas; UX; rapi; just better; Studio full Rust paritas; rilis). Untuk tiap risiko: `Risiko | Bukti di paket | Dampak terhadap kriteria accept | Saran mitigasi (tanpa kode)`.
3. **Rencana ulang untuk anggaran sisa ≈ US$10**: tiga jalur:
   - **Jalur minimum** (`Studio full Rust paritas + rilis`): deliverable per run (berdasarkan `HANDOFF-DIGEST.md` §7), estimasi jumlah run (9-13), risiko utama, apakah membutuhkan `F3` (lisensi Slint).
   - **Alternatif 1** (`rilis engine baru dengan Studio Qt dulu`): apa yang dilepas (`Studio full Rust`), dampak terhadap kriteria accept, berapa run lebih cepat.
   - **Hibrida** (`rilis engine + lanjut Rust paralel`): pembagian tim/agent, risiko paralel (WIP belum commit, `calm-otter` vs `proud-garden`).
4. **Pemotongan lingkup yang disarankan** (berdasarkan `HANDOFF-DIGEST.md` §7): misalnya i18n id, matriks DPI penuh, dialog sekunder, ADR sekunder, 4K 6-ref A/B, HEVC 8/10-bit ABAB. Untuk tiap: `Item | Dampak terhadap kriteria accept | Apakah aman dipotong untuk rilis`.
5. **Satu hal pertama yang akan dilakukan**: satu tindakan konkret (bukan daftar panjang). Contoh: "Verifikasi `SANITIZE-REPORT.md` menunjukkan 0 temuan rahasia sebelum `share/` dikirim"; atau "Periksa apakah `STATS.md` token per paket konsisten dengan batas"; atau "Konfirmasi `NEXT TASK` (A-13a ADR thumbnailer) masih valid sebelum owner menyetujui rencana".

---

## Larangan
- Jangan menulis kode produk (Rust/C++/Slint/QML). Jangan menyarankan perubahan kode kecuali sebagai catatan.
- Jangan menyarankan perubahan keputusan terkunci (`L1`-`L10`) tanpa menyebutnya sebagai "usulan perubahan keputusan".
- Jangan membuat klaim baru (misalnya "angka ini salah") tanpa merujuk file bukti yang ada di paket (`STATS.md`, `HANDOFF-DIGEST.md`, `docs/bench/*.json`).
- Jangan membaca file > 300 baris utuh kecuali yang diperlukan untuk verifikasi klaim tertentu; gunakan `Select-String` atau `-First`/`-Last`.

---
*Ditulis 2026-10-26. Paket yang akan direview: `handoff/local/` (apa adanya) dan `handoff/share/` (sanitasi). Jika `SANITIZE-REPORT.md` menunjukkan temuan yang tidak yakin, `share/` mungkin tidak boleh dibuat — laporkan.*
