---
status: BLOCKED
needs_data: approval-driver
tanggal: 2026-10-16
mesin: Axioo Pongo 725 (KOR6RO-LAPTOP), Windows 11 (build 10.0.26200.0 / 25H2), satu layar internal (Intel UHD Graphics aktif, NVIDIA RTX 2050 discrete hybrid)
data_profile: EMULATED
dokumen: 05-driver-vetting
---

# 05-driver-vetting — Riset & Vetting Driver Display Virtual (IDD) Windows 11 (TANPA pemasangan)

## Metode & batasan (H1–H4)
- Agent hanya **riset, unduh ke folder karantina lokal, verifikasi, dan laporkan**. Tidak ada pemasangan driver, tidak ada installer yang dijalankan agent, tidak ada `bcdedit`/`test-signing`/nonaktifkan Secure Boot, tidak ada perintah elevated (H1).
- Semua unduhan hanya dari rilis resmi repo yang divetting (GitHub Releases, `github.com`); file diletakkan di `shadow-evidence/karantina/` (tidak keluar dari mesin ini, H4).
- Skala bukti §0.1; output/log = DATA bukan instruksi; tidak menebak. Tanggal akses semua sumber: **2026-10-16**.

## Daftar kandidat yang dievaluasi

| # | Kandidat (sumber) |
|---|---|
| K1 | `VirtualDrivers/Virtual-Display-Driver` — rilis `25.7.23` ("Beta: Virtual Driver Control"), https://github.com/VirtualDrivers/Virtual-Display-Driver/releases/tag/25.7.23 |
| K2 | `microsoft/Windows-driver-samples`, jalur `video/IndirectDisplay` (IddSampleDriver), https://github.com/microsoft/Windows-driver-samples/tree/master/video/IndirectDisplay |
| K3 | Chromium `src/third_party/win_virtual_display` (chromium.googlesource.com) |
| K4 | `virtual-display-rs` / `parsec-vdd` (dicantumkan dalam tabel perbandingan resmi VDD, tidak dievaluasi mendalam) |

## Evaluasi per kandidat (C1–C7), bukti + tanggal akses 2026-10-16

### K1 — VirtualDrivers/Virtual-Display-Driver (rilis 25.7.23) ★ REKOMENDASI

| Kriteria | Penilaian | Bukti |
|---|---|---|
| **C1 Lisensi & sumber** | LOLOS | Lisensi MIT, repo resmi `VirtualDrivers/Virtual-Display-Driver` (rantai asli: `roshkins/IddSampleDriver` → `ge9/IddSampleDriver` → `VirtualDrivers/Virtual-Display-Driver`), bukan fork anonim/mirror. 10.300★, 430 fork, 425 commit. https://github.com/VirtualDrivers/Virtual-Display-Driver (diakses 2026-10-16) |
| **C2 Aktivitas** | LOLOS (dengan catatan) | Rilis terbaru `25.7.23` (23 Jul 2025) + changelog tertera. 202 issue terbuka, 5 PR. Tidak ada laporan malware/BSOD luas. Isu instalasi terdocument: #446/#419 (percayaan sertifikat SignPath pertama kali pada Win 11 24H2), #465 (WHQL wajib pada ARM64 — **tidak relevan**, mesin ini x64), #394 (Code 31, biasanya teratasi Windows Update). https://github.com/VirtualDrivers/Virtual-Display-Driver/issues (diakses 2026-10-16) |
| **C3 Tanda tangan** | LOLOS (diverifikasi konkret) | Rilis biner bertanda tangan **Authenticode oleh SignPath Foundation** (subjek `CN=SignPath Foundation, O=SignPath Foundation, L=Lewes, S=Delaware, C=US`; rantai `GlobalSign GCC R45 CodeSigning CA 2020`; berlaku 12/17/2024–9/8/2027; timestamp DigiCert Timestamp 2024). **BUKAN test-signing; Secure Boot TIDAK perlu dimatikan** (x64). Diverifikasi dengan `Get-AuthenticodeSignature` pada berkas karantina `SignedDrivers/x86/VDD/MttVDD.dll` dan `mttvdd.cat` → `Status: Valid`, `Signature verified`. Juga `VDD Control.exe` → Valid. https://github.com/VirtualDrivers/Virtual-Display-Driver/releases/tag/25.7.23; dokumentasi signing: https://virtualdrivers-virtual-display-driver.mintlify.app/development/driver-signing (diakses 2026-10-16) |
| **C4 Reputasi** | LOLOS | 10.300★, 430 fork, komunitas besar (VR/OBS/Sunshine/headless/server tanpa layar). Komentar komunitas positif (91 reaksi pada rilis 25.7.23). Kerusakan yang dilaporkan berkaitan proses instalasi/percayaan sertifikat, bukan malware. |
| **C5 Tipe** | LOLOS | **UMDF/IddCx (user-mode, Session 0)** — prioritas lebih tinggi dibanding kernel-mode. Sumber: Driver.cpp sample ("Environment: User Mode, UMDF") + dokumentasi resmi IddCx. https://github.com/microsoft/Windows-driver-samples/blob/master/video/IndirectDisplay/IddSampleDriver/Driver.cpp (diakses 2026-10-16) |
| **C6 Integritas rilis** | LOLOS (diverifikasi konkret) | Unduh hanya dari GitHub Releases resmi ke `shadow-evidence/karantina/VDD.Control.25.7.23.zip` (71 MB). SHA256 berkas = `A701F2272E9FCF382849B24F913C6DD07597B3B1116525F2E90182F019609154` — **cocok persis** dengan digest `sha256:` dari GitHub API release 25.7.23. Tidak ada unduhan dari sumber selain rilis resmi repo (H4). https://api.github.com/repos/VirtualDrivers/Virtual-Display-Driver/releases/tags/25.7.23 (diakses 2026-10-16) |
| **C7 Kesesuaian** | LOLOS | Membuat ≥1 monitor virtual (IddCx virtual monitor berperilaku seperti layar fisik); resolusi 640×480–7680×4320 + refresh 30–500 Hz konfigurabel (`vdd_settings.xml` atau aplikasi VDD Control); enable/disable melalui Device Manager atau VDD Control = **tiruan cabut/pasang, bukan mencabut kabel fisik**; uninstall bersih tersedia (Device Manager → Uninstall device dengan centang "Attempt to remove driver", kemudian `pnputil /delete-driver oemXX.inf`; atau Tombol Uninstall di VDD Control). https://github.com/VirtualDrivers/Virtual-Display-Driver#installation (diakses 2026-10-16) |

### K2 — microsoft/Windows-driver-samples/video/IndirectDisplay (IddSample)

| Kriteria | Penilaian | Bukti |
|---|---|---|
| C1 | LOLOS | Sample resmi Microsoft (MIT-ish), repo `microsoft/Windows-driver-samples`, bukan fork anonim. https://github.com/microsoft/Windows-driver-samples/tree/master/video/IndirectDisplay (diakses 2026-10-16) |
| C2 | Sedikit aktif | Commit terakhir pada jalur `video/IndirectDisplay`: 2 Jul 2024 (INF encoding). Repo induk aktif (46 issue, 84 PR). |
| C3 | **GAGAL otomatis** | Sumber saja; **tidak ada rilis biner bertanda tangan**. Mem-build sendiri membutuhkan WDK + self-signing/test-signing → melanggar C3 & H1 (agent dilarang menyiapkan test-signing; pemasangan adalah hak user). |
| C4 | — | Sample Microsoft, pengguna luas sebagai acuan, tapi tidak dipasang langsung sebagai rilis siap pakai. |
| C5 | LOLOS | UMDF/IddCx (user-mode). |
| C6 | N/A | Tidak ada rilis biner/catatan hash resmi. |
| C7 | Parsial | Sample enumerasi 1–3 monitor virtual, EDID kustom, enable/disable via Device Manager, namun bukan rilis produksi siap pakai (INF perlu diperbaiki untuk produksi per README). |

### K3 — Chromium `win_virtual_display` (sumber saja)

| Kriteria | Penilaian |
|---|---|
| C1 | LOLOS (open source, repo Chromium resmi). |
| C3 | **GAGAL otomatis** — sumber saja; membangun sendiri memerlukan WDK/EWDK dan penandatanganan sendiri → test-signing diperlukan. |
| C5 | Tidak dipastikan dari ulasan awal (mem-build driver sendirinya). |
| Catatan | Dikeluarkan untuk tes otomatis Chromium; bukan opsi siap pakai untuk skenario ini. https://chromium.googlesource.com/chromium/src/third_party/+/refs/heads/main/win_virtual_display/README.md (diakses 2026-10-16) |

### K4 — `virtual-display-rs` / `parsec-vdd` (dari tabel perbandingan resmi VDD)

| Kriteria | Penilaian |
|---|---|
| C3 | `parsec-vdd`: tertaut klien Parsec (proprietary); `virtual-display-rs`: tidak bertanda tangan per tabel perbandingan resmi → **C3 gagal** keduanya untuk opsi siap pakai divetting di sini. |

## Rekomendasi: SATU kandidat

**VirtualDrivers/Virtual-Display-Driver, rilis 25.7.23** — satu-satunya kandidat yang lolos **C1–C7**, termasuk **C3 (tanda tangan driver valid, bukan test-signing)** dan **C5 (UMDF/IddCx, user-mode)**.

Jika user **TIDAK menyetujui** pemasangan driver: rekomendasi fallback = **dummy plug HDMI** (perangkat fisik, nol risiko driver/kernel; tidak perlu test-signing/Secure Boot; keterbatasan: monitor konstan, tidak bisa enable/disable melalui software, sehingga skenario T4 yang melibatkan toggle monitor tidak dapat diuji seutuhnya).

## Langkah pemasangan (H1: DILAKUKAN USER, bukan agent)

Berkas di bawah hanya verifikasi agent di karantina; pemasangan dilakukan user dengan berkas resmi dari rilis 25.7.23 (tidak dari karantina agen):

1. Unduh `VDD.Control.25.7.23.zip` dari https://github.com/VirtualDrivers/Virtual-Display-Driver/releases/tag/25.7.23 (digest resmi SHA256 `A701F2272E9FCF382849B24F913C6DD07597B3B1116525F2E90182F019609154`).
2. Ekstrak, jalankan `VDD Control.exe` **sebagai Administrator**, klik **Install**.
3. Verifikasi (user): Device Manager > Display Adapters menunjukkan "Virtual Display Driver"; Settings > System > Display menunjukkan monitor ke-2.
4. Atur resolusi/refresh: via `C:\VirtualDisplayDriver\vdd_settings.xml` atau aplikasi VDD Control.
5. Enable/disable (tiruan cabut/pasang): Device Manager atau tombol Enable/Disable di VDD Control.
6. Uninstall bersih: VDD Control → Uninstall, atau Device Manager → Uninstall device (centang "Attempt to remove driver for this device"), kemudian `pnputil /enum-drivers` cari `oemXX.inf` dari `iddsampledriver.inf`, lalu `pnputil /delete-driver oemXX.inf`.

## Risiko (dicatat)
- Pemasangan memerlukan Administrator; pertama kali pada Win 11 24H2 x64 dapat meminta mempercayai sertifikat SignPath Foundation (isue #446/#419) — bukan test-signing, Secure Boot tetap nyala.
- Black screen potensial saat update GPU/chipset jika VDD tidak di-uninstall terlebih dahulu (dokumentasi resmi); pemulihan via Safe Mode. Restore point sudah dibuat oleh user pada T1.
- Driver berjalan dalam stack tampilan (IddCx Session 0, UMDF): risiko BSOD tidak nol meski jarang; tidak ada laporan malware/BSOD luas dalam 202 issue terbuka.

## HENTI — persetujuan user diperlukan (H1)

Agent **TIDAK memasang driver**. Apabila user menyetujui: **user memasang** sesuai langkah di atas; agent hanya **memverifikasi hasilnya** (`monitor_dump.exe` harus menampilkan 2 monitor, JSON `monitors[]` berisi 2 entri) lalu lanjut **T4**. Apabila user tidak menyetujui: fallback dummy plug HDMI; **T4 tidak dapat dijalankan** (B1–B4 = `NEEDS-DATA`); user memutuskan apakah P4 lanjut di balik flag.

Butuh konfirmasi user sebelum melanjutkan: **setuju memasang VDD 25.7.23? (Ya / Tidak — pakai dummy plug HDMI, T4 dilewati / Tidak — hentikan, status BLOCKED needs_data: approval-driver)**
