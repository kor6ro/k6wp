# Baseline kompetitor — K6WP vs Lively Wallpaper vs Wallpaper Engine (Todo 44, fase-6)

Date: 2026-09-16. Status: **K6WP side terisi (cited, bukan remeasure); kolom kompetitor N/A
(plan's QA-fail path — tidak terinstal, tidak menginstal, tidak memfabrikasi angka).**

## 1. HW box (tempat angka K6WP diukur)

| Item | Nilai (diukur 2026-09-16) |
|---|---|
| CPU | 12th Gen Intel Core i7-12650H |
| RAM | 15.7 GB |
| OS | Microsoft Windows 11 Home, build **26200** (v10.0.26200, 24H2-era) |
| GPU dGPU | NVIDIA GeForce RTX 2050, VRAM ~4095 MB (NVENC works — Todo 4/15) |
| GPU iGPU | Intel UHD Graphics, VRAM ~2048 MB (QSV works — Todo 15) |
| GPU AMD | **tidak ada** (h264_amf probe fails gracefully — Todo 15) |
| Monitor | 1× 1920×1080 primary |
| Animasi Windows | OFF ("Animate controls..." OFF — Todo 3: `0x052C` spawn nothing) |
| Power | AC laptop |

> Semua angka K6WP di bawah diukur di box ini. Perbandingan valid hanya di HW identik.

## 2. Methodology (agar future laptop run bisa mengisi sel N/A)

- **Korpus sama**: `tests/corpus/` — `anime.mp4` (32.4 MB, 1920×1080 H.264 30fps),
  `gaming.mp4` (423.02 MB), `slideshow.mp4` (0.01 MB, 1920×1080 H.264).
  Kompetitor harus memutar **clip yang sama** (rekomendasi: `anime.mp4` 1080p30).
- **Sampler sama**: `tools/bench_cpu_mem.ps1` — P/Invoke `QueryProcessCycleTime` +
  `GetProcessMemoryInfo`, sampling tiap 500 ms (`-IntervalMs 500`), idle ≥1 menit
  (gate penuh: 5 menit seperti `docs/bench_fase1.json`, 593 sampel). CPU% =
  cycle-delta/QPF/delta-sec×100; RAM = `PrivateUsage`/1MiB.
- **Startup sama**: `tools/bench_startup.ps1` — wall-clock process-start → marker
  `Engine:FirstFrame`, N runs, median (`-Runs 5` untuk gate).
- **Kondisi idle**: tutup aplikasi lain, AC power, 1×1080p, tunggu ≥1 menit stabil
  sebelum sampling. Catat proses yang disample (nama exe + PID).
- **GPU**: tidak ada sampler GPU yang diratifikasi di repo (Todo 43 mencatat
  `nvidia-smi dmon` / Task Manager sebagai kandidat manual). Kolom GPU = N/A
  sampai metode diratifikasi.
- **Kompetitor tidak diinstal di box ini** (bukti §4) → sel kompetitor = N/A.
  JANGAN install demi todo ini (MUST NOT DO); isi pada run mendatang di mesin
  yang sudah terinstal kompetitor, dengan korpus + sampler yang sama.

## 3. K6WP numbers (cite dari gate JSONs — tidak remeasure)

| Metric | K6WP (sumber) | Budget (planning §6) | Verdict |
|---|---|---|---|
| Startup median | **111.8 ms** (5 runs: 131.0/110.9/105.9/116.6/111.8) — `docs/bench_fase1.json`; rerun `docs/bench_startup.json` median **169.4 ms** (2 runs) | <2000 ms | PASS |
| CPU idle | **37.61%** avg, 593 sampel / 5 mnt — `docs/bench_fase1.json`; 1-mnt `docs/bench_cpu_mem.json` avg **63.48%** | <2% | **FAIL (artifact)** — skeleton idle (PeekMessage+Sleep(10), no video, no mpv instance). Bukan render load. Gap ditutup Todo 10 (LoadLoop event-driven) + idle tuning. Render-load number pending video path. |
| RAM engine | avg **15.82 MB**, peak 15.93 MB — `docs/bench_fase1.json`; 1-mnt avg **19.58 MB**, peak 19.77 MB — `docs/bench_cpu_mem.json` | <80 MB | PASS |
| RAM studio | **51.4 MB** (<150 MB) — `docs/bench_fase3.json` gate note; Todo 20: 47–48 MB WorkingSet | <150 MB (studio) | PASS |
| GPU 1080p | **tidak terukur** — `docs/bench_fase1.json`: "no video wired to CLI yet" | <5% | N/A pending (real 1080p H.264 via Todo 10 + hwdec=d3d11va, cf. spike Todo 4) |
| Compress HW (nvenc) | anime→1080p **5.06 s** (15.27 MB), gaming **10.11 s** (17.95 MB), slideshow **3.03 s** — `docs/bench_compress.json` (720p30 crf23); e2e 480×270 **3.85 s** — `docs/bench_fase3.json` | <30 s | PASS |
| IPC ack latency | set_video 0.31–0.59 ms, pause/resume/set_monitor/get_state 0.16–0.23 ms — `docs/bench_fase4.json` (15 cmds + 3 error paths, PID konstan) | <100 ms | PASS |
| Compressor matrix | 6/6 (3 valid nvenc/x264 + 3 invalid exit-2) — `docs/bench_fase2.json` | exit-contract | PASS |
| Autostart/fullscreen/library/migration | PASS — `docs/bench_fase5.json` (registry bersih pre/post, 7/7 predicate harness, manual_pending: positive fullscreen + resume-latency) | — | PASS (parsial headless) |

Setiap angka di atas traceable ke file JSON yang dikutip. Tidak ada angka baru diukur di Todo 44.

## 4. Competitor detection (prosedur eksak + hasil)

Tanggal cek: 2026-09-16. Perintah (PowerShell 5.1, satu invocasi):

1. `Get-ItemProperty HKCU:\...\Uninstall\* | Where DisplayName -match 'Lively|Wallpaper'` → **kosong**
2. `Get-ItemProperty HKLM:\...\Uninstall\*, HKLM:\...\WOW6432Node\...\Uninstall\* | Where -match` → **kosong**
3. `Get-AppxPackage *Lively*` → **kosong**
4. Start-menu `Programs` (user + ProgramData) recurse `-match 'Lively|Wallpaper'` → **kosong**
5. Common paths — semua `Test-Path = False`:
   `%LOCALAPPDATA%\Lively Wallpaper`, `%ProgramFiles%\Lively Wallpaper`,
   `%ProgramFiles(x86)%\Lively Wallpaper`, `%ProgramFiles%\Wallpaper Engine`,
   `%ProgramFiles(x86)%\Wallpaper Engine`,
   `%ProgramFiles(x86)%\Steam\steamapps\common\wallpaper_engine`
6. `Get-Process | Where ProcessName -match 'Lively|wallpaper32|wallpaper64'` → **kosong**

**Hasil: Lively Wallpaper TIDAK terinstal; Wallpaper Engine (+ Steam host-nya) TIDAK
terinstal di box ini.** Sesuai plan QA-fail: catat N/A + alasan, JANGAN install,
JANGAN fabrikasi angka.

## 5. Competitor table (HW identik, korpus `anime.mp4` 1080p30, sampler §2)

| Metric | K6WP (box §1) | Lively Wallpaper | Wallpaper Engine |
|---|---|---|---|
| Startup (FirstFrame/median) | 111.8 ms (fase1) / 169.4 ms (rerun) | **N/A** — tidak terinstal (§4); isi via §2 | **N/A** — tidak terinstal (§4); isi via §2 |
| CPU idle 1080p video (%) | N/A pending render path (skeleton 37.61% artifact) | **N/A** — tidak terinstal | **N/A** — tidak terinstal |
| RAM idle (MB) | 15.82 avg / 15.93 peak | **N/A** — tidak terinstal | **N/A** — tidak terinstal |
| GPU 1080p (%) | N/A pending (kedua belah pihak) | **N/A** — tidak terinstal + sampler GPU belum ratifikasi | **N/A** — tidak terinstal + sampler GPU belum ratifikasi |
| Compress 1080p→720p (s) | 5.06 (nvenc, anime) | n/a (bukan fitur kompetitor) | n/a (bukan fitur kompetitor) |

## 6. Verdict

- **K6WP terukur**: startup/RAM/compress/IPC PASS dengan margin besar; CPU-idle
  FAIL adalah artifact skeleton yang terdokumentasi (bukan render load) dan
  menunggu Todo 10 + re-bench; GPU kedua belah pihak belum terukur.
- **Kompetitor**: 0 dari 2 terinstal → per plan, tabel N/A + metodologi tercatat
  (§2) sehingga run mendatang di mesin ber-kompetitor dapat mengisi sel tanpa
  mengulang desain eksperimen. **Tidak ada angka kompetitor yang difabrikasi.**
- **Acceptance Todo 44**: tabel terisi (K6WP cited + kompetitor N/A beralasan) +
  metodologi tercatat (bukan angka asal) → **PASS via QA-fail path yang sah**.
