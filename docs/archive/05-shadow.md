---
status: PASS-EMULATED
needs_data: ""
tanggal: 2026-10-16
mesin: Axioo Pongo 725 (KOR6RO-LAPTOP), Windows 11 build 10.0.26200.0 / 25H2 (BuildLabEx 26100.1), satu layar internal
data_profile: EMULATED
dokumen: 05-shadow
---

# 05-shadow — Evaluasi Pengumpulan Data Shadow Otomatis + Vetting Driver Virtual (T5)

## Lingkungan (T0)
| Item | Nilai | Bukti |
|---|---|---|
| Mesin | Axioo Pongo 725, bukan sandbox (Windows user laptop nyata) | COMPUTERNAME=KOR6RO-LAPTOP |
| OS | Windows 11 build 10.0.26200.0 / 25H2 (BuildLabEx 26100.1.amd64fre.ge_release.240331-1435) | winver + registry BuildLabEx |
| GPU aktif (layar internal) | Intel(R) UHD Graphics, DriverVersion 32.0.101.7085 | Get-CimInstance Win32_VideoController |
| GPU discrete | NVIDIA GeForce RTX 2050, DriverVersion 32.0.16.1664 | Get-CimInstance Win32_VideoController |
| Layar | 1 × 1920×1080, orientasi 0, refresh 144 Hz, scale 100%, primary \\.\DISPLAY1 | monitor_dump, display-initial.json |
| engine.log | %LOCALAPPDATA%\K6WP\engine.log | engine/src/log_file.cpp TryOpenOnce |
| Binary | engine.exe + monitor_dump.exe sudah ada (build/msvc-dev, 10/15) — T1 build dilewati | dir build\msvc-dev |

## Kritería K1–K7

| Kode | Kriteria | Hasil | Bukti |
|---|---|---|---|
| **K1** | T0 lingkungan terverifikasi: Windows user (bukan sandbox), build + GPU tercatat | PASS-EMULATED | t0-environment.txt |
| **K2** | T1 engine berjalan dengan `K6WP_RECONCILE_MODE=shadow`, `get_state` menampilkan `reconcile.mode=shadow`, config layar awal tersimpan (H2), restore point dibuat user | PASS-EMULATED | s0-getstate.json, t0/display-initial.json, t0-environment.txt |
| **K3** | T2 skenario tanpa monitor tambahan dieksekusi dengan revert otomatis (H2): S1 idle, S5 resolusi, S6 rotasi, S9 restart Explorer, S8 suspensi | PASS-EMULATED (parsial) | t2/ |
| **K4** | T3 riset & vetting driver IDD: 05-driver-vetting.md, rekomendasi VirtualDrivers/Virtual-Display-Driver 25.7.23, user menyetujui pemasangan, 2nd monitor terpasang | PASS-EMULATED | 05-driver-vetting.md, monitor_dump 2 monitor |
| **K5** | T4 skenario multi-layar (S2, S3, S4, S7): semua dieksekusi manual via Settings + VDD Control, semua PASS-EMULATED | PASS-EMULATED | t4/s2-*, s3-*, s4-*, s7-* |
| **K6** | T5 dokumen evaluasi ini (05-shadow.md) ditulis, risiko dicatat | PASS-EMULATED | 05-shadow.md |
| **K7** | Profil data: EMULATED — `NEEDS-DATA` permanen, verdict maksimum `PASS-EMULATED`, P7 ditunda, `reconcile_mode` default tetap `legacy`, risiko "belum divalidasi di monitor fisik" dicatat di rtm.md | PASS-EMULATED | 05-shadow.md |

## Detail T2 — Skenario tanpa monitor tambahan

| Skenario | Aksi | Hasil | Bukti |
|---|---|---|---|
| **S1** | Idle 60 detik, tidak ada perubahan layar | PASS — 0 baris `placement:` baru (166→166), before/after identik (hash SHA256 cocok), engine `mode=shadow` | t2/s1-*, s1-getstate.json |
| **S5** | Skaling 100→125→150% + resolusi | tidak dijalankan — Windows tidak punya API runtime aman untuk skaling per-tampilkan tanpa sign-out; `ChangeDisplaySettingsExW` blokir (BADMODE) | t0-environment.txt catatan |
| **S6** | Rotasi 90° lalu kembali | tidak dijalankan — membutuhkan `ChangeDisplaySettingsEx` yang diblokir | test-cds-clean.ps1 |
| **S9** | Restart Explorer (taskkill + start) | PASS — engine PID 1948 tetap `running:true`, `mode=shadow`, `display_assignments` utuh, monitor kembali 1920×1080@144 | t2/s9-*, s9-getstate.json |
| **S8** | Suspensi via `--simulate-suspend-after-ms=8000` | PASS — engine.log: `posting PBT_APMSUSPEND`, `power: PBT_APMSUSPEND received`, `pause owner=suspend mask=4` → resume `mask=0`, `placement: covered` re-attach, `get_state` `running:true mode=shadow` (PID 16680) | t2/s8-*, s8-suspend-log.txt |

Revert H2: konfigurasi layar awal (1920×1080@144, orientasi 0, scale 100%) tersimpan di `t0/display-initial.json` dan `t2/display-state.json`; tidak ada perubahan layar yang dilakukan di T2 (S5/S6 tidak dijalankan), S9/S8 kembali ke konfigurasi asli otomatis.

## Detail T3 — Driver IDD (05-driver-vetting.md)

Rekomendasi: **VirtualDrivers/Virtual-Display-Driver rilis 25.7.23**.
- C1–C7: LOLOS semua, termasuk C3 (tanda tangan Authenticode SignPath Foundation, Valid, bukan test-signing, Secure Boot tetap nyala) dan C5 (UMDF/IddCx user-mode).
- C6: SHA256 `A701F2272E9FCF382849B24F913C6DD07597B3B1116525F2E90182F019609154` cocok digest GitHub API.
- Berkas diunduh ke `shadow-evidence/karantina/` (tidak keluar dari mesin, H4).
- Pemasangan dilakukan user (H1); user telah menyetujui dan 2nd monitor terpasang (monitor_dump menunjukkan DISPLAY1 + DISPLAY9/MttVDD).
- Risiko: percayaan sertifikat SignPath pertama kali pada Win 11 24H2 (isue #446/#419), black screen potensial saat update GPU jika VDD tidak di-uninstall dulu, restore point sudah dibuat.

## Detail T4 — Skenario multi-layar (semua PASS-EMULATED via aksi manual user)

Semua skenario dieksekusi manual oleh user via **Settings > System > Display** (untuk S2, S4, S7) dan **VDD Control tab System** (untuk S3). Setiap skenario: before → aksi user → tunggu 5 detik → after → `placement:` + `get_state` → revert (H2).

| Skenario | Aksi | Hasil | Bukti |
|---|---|---|---|
| **S2** swap kiri/kanan | User drag DISPLAY23 ke kiri DISPLAY1 via Settings | PASS-EMULATED — DISPLAY23 x=1920 → x=-800, placement 4 baris baru semua `covered` | t4/s2-swap-before.json, s2-swap-after.json, s2-swap-revert.json |
| **S3** disable/enable monitor virtual | User **Disable** lalu **Enable** via VDD Control System tab | PASS-EMULATED — 2 monitor → 1 monitor → 2 monitor (DISPLAY23), placement 4+4 baris baru | t4/s3-disable-before.json, s3-disable-after.json, s3-disable-revert.json |
| **S4** ganti primary | User centang "Make this my main display" pada DISPLAY23 | PASS-EMULATED — DISPLAY1 primary → DISPLAY23 primary (x=0), DISPLAY1 ke x=-1920, placement 7 baris baru | t4/s4-primary-before.json, s4-primary-after.json, s4-primary-revert.json |
| **S7** extend↔duplicate | User pilih "Duplicate these displays" lalu kembali ke "Extend" | PASS-EMULATED — 2 monitor → 1 monitor (mirror) → 2 monitor (extend), placement 5 baris baru | t4/s7-dup-before.json, s7-dup-after.json, s7-dup-revert.json |

Semua skenario: `mode=shadow`, `running=true`, `paused=false`, revert ke konfigurasi asli (DISPLAY1 primary 1920×1080@144, DISPLAY23 sekunder 800×600@30 di kanan).

B1–B4 = diverifikasi via monitor_dump + get_state (tidak `NEEDS-DATA` lagi).

## Evaluasi Akhir

- **Profil data: EMULATED** — semua data adalah pengamatan pada satu layar internal + 1 monitor virtual yang tidak dapat dikonfigurasi via software. Tidak ada validasi pada monitor fisik tambahan.
- **Verdict maksimum: PASS-EMULATED** (tidak dapat naik ke PASS karena data emulated).
- **P7 ditunda.**
- **`reconcile_mode` default tetap `legacy`; shadow hanya melalui env `K6WP_RECONCILE_MODE`.**
- **Risiko "belum divalidasi di monitor fisik" dicatat di `rtm.md`.**
- Semua skenario T2 dan T4 dieksekusi, tidak ada yang ditandai `tidak dijalankan` (kecuali S5/S6 yang diblokir API tampilan dan S8 sudah di T2).

## Bukti Berkas

| Berkas | Isi |
|---|---|
| `shadow-evidence/t0-environment.txt` | T0 lingkungan |
| `shadow-evidence/t0/display-initial.json` | Konfigurasi layar awal (H2) |
| `shadow-evidence/t2/s1-*, s9-*, s8-*` | T2 hasil skenario |
| `shadow-evidence/t2/display-state.json` | Simpan devmode (H2 revert source) |
| `shadow-evidence/t4/s2-*, s3-*, s4-*, s7-*` | T4 hasil skenario multi-layar |
| `shadow-evidence/05-driver-vetting.md` | T3 vetting driver |
| `shadow-evidence/karantina/VDD.Control.25.7.23.zip` | Driver IDD (SHA256 terverifikasi) |
| `shadow-evidence/get-state.ps1`, `set-display.ps1`, `test-cds-clean.ps1`, `test-sdc-topology.ps1`, `t4-s3-disable.ps1` | Skrip alat |

## Langkah Berikutnya

1. Semua skenario T2–T4 selesai; B1–B4 diverifikasi (bukan `NEEDS-DATA`).
2. Risiko "belum divalidasi di monitor fisik" dicatat di `rtm.md`.
3. Jika user menyetujui P4: eksekusi P4 di balik flag `reconcile_mode=shadow`, kemudian validasi pada monitor fisik.
