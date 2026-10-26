# COST-RULES.md — Aturan hemat (≤ 1 halaman) untuk semua run berikut

Berlaku sejak 2026-10-26. Setiap agent yang menjalankan run K6WP wajib membaca `HANDOFF-DIGEST.md` dan `COST-RULES.md` ini sebelum mulai.

---

## 8 Aturan hemat
1. **Awal run hanya baca**: `HANDOFF-DIGEST.md`, baris run di `STATE.md` (bagian open commitments), `NEXT TASK`. Jangan membaca ulang dokumen lain kecuali tugas menyebutnya.
2. **Cari dengan `Select-String` / `rg`; baca dengan rentang baris** (`Get-Content -First`/`-Last`). Jangan buka file > 300 baris utuh. Keluaran perintah ke file; hanya `tail -n 20` ke konteks chat.
3. **Tes relevan saja** (`cargo test -p <crate> <filter>`, `ctest -R <nama>`). **Gerbang penuh sekali di akhir run**, bukan tiap langkah.
4. **Satu deliverable per run; tanpa side quest**. Ukur ulang hanya klaim yang menjadi isi deliverable; skrip ukur yang sudah ada (`tools/bench_*.ps1`) dipakai ulang.
5. **Laporan ≤ 15 baris**, hanya delta; jangan menyalin laporan sebelumnya (`docs/refactor/RUN-*.md`). Label status: `Selesai` hanya bila semua artefak §7 (`STATS.md`, `HANDOFF-DIGEST.md`, `COST-RULES.md`, `REVIEWER-PROMPT.md`, `RUN-HANDOFF.md`) ada.
6. **Jangan ulang verifikasi pemulihan/pengamanan** yang sudah tercatat selesai (`ledger.md`: I4 = 5/5 binary bersih; G6 = 0 failure; J2 = 10/10 hijau; L5 = lint + scan + self-test hijau).
7. **Pekerjaan mekanis** (ekstraksi golden, skrip, port logika murni) cocok model murah; desain, ADR, diagnosis kegagalan = model terkuat. (Usulan; tergantung alat.)
8. **Berhenti di batas langkah bila konteks hampir habis** dan tulis `NEXT TASK`; jangan menyelesaikan dengan ringkasan kosong. Tulis satu baris "biaya run ≈ …" jika alat menampilkan; jika tidak, "tidak terlihat".

---

## Prompt run ramping (≤ 12 baris — tempel tiap run, ganti prompt standar ROADMAP.md)
```
Run ini: [deliverable]. Baca dulu HANDOFF-DIGEST.md §1-3, STATE.md NEXT TASK, dokumen tugas yang disebut. Tanpa side quest. Laporan ≤ 15 baris, hanya delta. Verifikasi: tes relevan selama; gerbang penuh di akhir. Tidak ulang verifikasi yang sudah tercatat selesai (I4/G6/J2/L5). Biaya ≈ … (atau "tidak terlihat"). NEXT TASK tetap A-13a (ADR thumbnailer) — jangan mulai di run ini.
```

---
*Ditulis 2026-10-26. Tidak mengubah kode produk (Rust/C++/Slint/QML). Tidak menjalankan pengukuran baru.*
