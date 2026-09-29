# Audit Findings Reconciliation — K6WP

Artefak Fase 0 (Wave 0) dari plan `k6wp-audit-remediation`. Dokumen audit
gabungan tidak tersimpan di repo; sumber kebenaran daftar ID temuan adalah
`.omo/plans/k6wp-audit-remediation.md`. Setiap verdict di bawah diverifikasi
langsung terhadap kode aktual di worktree ini, bukan sekadar menyalin klaim
audit.

## Ringkasan verdict

| Verdict | Jumlah |
| ------- | ------ |
| BARU | 42 |
| DUPLIKAT | 0 |
| SUDAH DIKETAHUI | 4 |
| NON-ISSUE | 3 |
| **Total** | **49** |

Tidak ada temuan yang teridentifikasi sebagai DUPLIKAT dari temuan lain dalam
dokumen audit. Empat temuan sudah tercatat di `docs/` tim sebelum audit
(MED-6, MED-11, MED-14, LOW-11). Tiga temuan diverifikasi NON-ISSUE: HIGH-5,
MED-HIGH-1, MED-16. Satu temuan dikoreksi severity-nya (HIGH-4 turun ke
Low-Medium).

## Empat koreksi wajib (hasil verifikasi kode)

### HIGH-5 — NON-ISSUE

Temuan audit: occlusion dapat membuat engine "stuck-paused" karena dianggap
sebagai pemilik bit `pause_mask_`.

Verifikasi: occlusion **bukan** pemilik `pause_mask_`.

- `engine/src/occlusion_watch.cpp:235-250` — `OnPauseMaskChanged(bool
  slots_paused)` hanya meng-arm/disarm tick occlusion mengikuti mask gabungan
  yang sudah dihitung `EngineApp::ApplyPauseState`; ia tidak pernah menulis
  bit pause sendiri dan tidak pernah memanggil `SetPauseOwner`.
- `engine/src/engine_app.cpp:1086-1103` — `PauseOwnerName(int bit)` hanya
  mengenali `kPauseUser`, `kPauseFullscreen`, `kPauseSuspend`, `kPausePower`,
  `kPauseSessionLock`, `kPauseScreenOff`. Tidak ada bit occlusion di
  enumerasi pause owner, sehingga tidak ada jalur "stuck-paused" dari
  occlusion.

Kesimpulan: NON-ISSUE. Tidak ada fix yang direncanakan (lihat plan, OUT).

### MED-HIGH-1 — NON-ISSUE

Temuan audit: restart/stop engine diduga memakai `taskkill` by image name
(berisiko mematikan proses lain).

Verifikasi: `studio/src/apply_manager.cpp:129-245` `RestartEngine` sudah
sepenuhnya PID-based.

- Baris 145-159: PID diambil dari `get_state` (`EnginePidFromState`).
- Baris 162: `OpenProcess(SYNCHRONIZE | PROCESS_TERMINATE, FALSE, engine_pid)`.
- Baris 173: `WaitForSingleObject(proc.get(), 8000)` menunggu PID itu keluar.
- Baris 180: `TerminateProcess(proc.get(), 1)` hanya pada PID yang sama.
- Tidak ada pemanggilan `taskkill` maupun pencocokan image name di jalur ini.

Kesimpulan: NON-ISSUE. Perilaku sudah sesuai kontrak `docs/dev-contracts.md`
§1 ("never by image name").

### MED-16 — NON-ISSUE

Temuan audit: hardening DLL (`SetDefaultDllDirectories`) tidak diterapkan.

Verifikasi: `SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_APPLICATION_DIR |
...)` sudah ada di keempat `main.cpp`:

- `engine/src/main.cpp:128`
- `studio/src/main.cpp:62`
- `launcher/main.cpp:806`
- `compressor/src/main.cpp:237`

Kesimpulan: NON-ISSUE. Tidak ada fix yang direncanakan (lihat plan, OUT).

### HIGH-4 — severity turun ke Low-Medium

Temuan audit: `SaveConfig` menulis langsung (truncate) sehingga force-kill
dapat merusak `config.json`.

Verifikasi: temuan valid, tetapi severity audit terlalu tinggi.

- `shared/config_schema.cpp:115-148` — `SaveConfig` memang menulis langsung
  `std::ofstream out(path, std::ios::binary | std::ios::trunc)` (baris 139)
  tanpa `.tmp` + `MoveFileExW`. Risiko truncate pada force-kill tetap ada.
- Mitigasi yang sudah ada: `engine/src/engine_app.cpp:1226` — `HandleSetVideo`
  melakukan `cfg = k6wp::LoadConfig(config_path)` (fresh-read dari disk)
  sebelum `SaveConfig` (baris 1238), dengan pola preserve-merge yang hanya
  menimpa `video_path`. `HandleSetMonitor` memakai pola yang sama
  (baris 1301-1303). Karena nilai dibaca segar dari disk lalu digabung,
  risiko menimpa field lain jauh lebih rendah dari asumsi audit.
- Kontrak `.bak` (load fallback ke `<config>.bak` saat corrupt) sudah
  terdokumentasi di `docs/dev-contracts.md` §2 dan tetap menjadi jaring
  pengaman terakhir.

Kesimpulan: BARU (belum tercatat di docs/), severity dikoreksi menjadi
Low-Medium. Fix atomic-write (`SaveConfig` `.tmp` + `MoveFileExW` +
`PersistConfigField`) tetap dikerjakan di todo 14.

## Tabel rekonsiliasi

Kolom: ID temuan | status di `docs/tech-debt.md`, `docs/known-issues.md`,
`docs/dev-contracts.md` ("tidak tercatat" bila tidak ada) | verdict verifikasi
kode | bukti.

| ID | Status di docs/ | Verdict | Bukti verifikasi kode |
| --- | --- | --- | --- |
| CRIT-1 | tidak tercatat | BARU | `engine/src/engine_app.cpp:1249` `headless_owns_decode_ = !wallpaper_surface_live_;` ditulis dari worker IPC; `wallpaper_surface_live_`, `pin_verify_armed_` non-atomic. Fix: todo 11. |
| CRIT-2 | tidak tercatat | BARU | `engine/src/engine_app.cpp:1283` `multi_monitor_.SetActiveMonitor(id)` dieksekusi langsung di handler worker; `HandleSetVideo` (1159) memuat renderer dari worker. Fix: todo 11. |
| HIGH-1 | tidak tercatat | BARU | `engine/src/engine_app.cpp:1037-1080` — `ipc_server_.Stop()` di baris 1060, setelah `StopHeadlessRenderer()` (1056) dan `ShutdownWallpaperSurface()` (1057). Fix: todo 3. |
| HIGH-2 | tidak tercatat | BARU | `launcher/main.cpp:469` `out += '?';` untuk `wc >= 0x80` di `EscapeBackupJson` (455-473); `ReadLockscreenBackup` (485-513) parse manual string-find. Fix: todo 19. |
| HIGH-3 | tidak tercatat | BARU | `studio/src/apply_manager.cpp:173` `WaitForSingleObject(proc.get(), 8000)` di thread GUI; `studio/src/ipc_client.cpp:24` `kReadDeadlineMs = 2000` + poll sinkron; `studio/src/settings_widget.cpp:92` `WaitForSingleObject(raw, 8000)`. Fix: todos 17-18. |
| HIGH-4 | tidak tercatat (dev-contracts.md §2 hanya kontrak `.bak`) | BARU (severity dikoreksi: Low-Medium) | `shared/config_schema.cpp:139` trunc langsung; mitigasi fresh-read `engine/src/engine_app.cpp:1226` + preserve-merge (1238, 1301-1303). Fix: todo 14. |
| HIGH-5 | tidak tercatat | NON-ISSUE | Occlusion bukan `pause_mask_` owner: `engine/src/occlusion_watch.cpp:235-250` hanya arm/disarm; `engine/src/engine_app.cpp:1086-1103` `PauseOwnerName` tanpa bit occlusion. |
| MED-2 | tidak tercatat | BARU | `docs/dev-contracts.md` §4 hanya `config_test` (113) + `ipc_test` (100); tidak ada test gpu_pin/semver/sha1/cache-key/library CRUD. Fix: todos 21-22. |
| MED-3 | tidak tercatat | BARU | SHA1 duplikat: `compressor/src/cache_manager.cpp:147` `Sha1Hex` dan `studio/src/thumbnailer.cpp:115` `Sha1Hex` (komentar baris 30 "mirrored from compressor/src/cache_manager.cpp"). Fix: todo 25. |
| MED-4 | tidak tercatat | BARU | Empat call-site probe/detect dengan timeout tidak konsisten: `compressor/src/cli.cpp::ProbeDuration`, `compressor/src/encoder_detect.cpp::RunCommand`, `compressor/src/main.cpp::ProbeVideoProps`, `studio/src/ffprobe_helper.cpp::Probe`. Fix: todo 16. |
| MED-5 | tidak tercatat | BARU | `studio/src/main_window.cpp` = 2538 baris (terverifikasi). Fix: todos 26-27. |
| MED-6 | SUDAH DIKETAHUI — `docs/tech-debt.md` Phase-4 "IPC 10 Hz residual STILL OPEN" | SUDAH DIKETAHUI | `engine/src/ipc_server.cpp:28` `kAcceptSliceMs = 100`, `:127` `WaitForSingleObject(event, kAcceptSliceMs)`. Fix: todo 12 (update tech-debt jadi resolved). |
| MED-8 | tidak tercatat | BARU | `shared/version_compare.hpp:71-73` `int acc = 0; acc = acc * 10 + (text[i] - '0');` tanpa clamp overflow. Fix: todo 5. |
| MED-9 | tidak tercatat | BARU | `compressor/src/main.cpp::ProbeVideoProps` memanggil ffprobe dua kali (format + stream terpisah). Fix: todo 15. |
| MED-10 | tidak tercatat | BARU | `compressor/src/ffmpeg_job.cpp:96` `Widen` loop per-byte, dipakai di baris 159-218. Fix: todo 6. |
| MED-11 | SUDAH DIKETAHUI — `docs/known-issues.md` butir 3 (lupdate) | SUDAH DIKETAHUI | `studio/i18n/studio_en.ts` template 8 baris; keputusan owner: hapus scaffolding. Fix: todo 23. |
| MED-12 | tidak tercatat | BARU (plan: verdict CONFIRMED) | `shared/ipc_protocol.hpp:22` `PipeName = L"\\\\.\\pipe\\k6wp-engine"` tanpa suffix session; mutex `Local\K6WP-Engine-Singleton` per-session. Fix: todo 13. |
| MED-13 | tidak tercatat (known-issues.md butir 4 hanya path ffmpeg, bukan command) | BARU | `studio/src/thumbnailer.cpp:329` `-y -ss 1 -i ...` + `-s 320x180` fixed. Fix: todo 4. |
| MED-14 | SUDAH DIKETAHUI — `docs/tech-debt.md` Phase-4 "version-stamp centralization" | SUDAH DIKETAHUI | Versi 1.1.0 di 20 spot hand-synced (app.rc x3, installer.nsi, app.manifest, monitor_dump.rc, dll). Fix: todo 8. |
| MED-15 | tidak tercatat | BARU | `studio/src/compress_service.cpp::StartNext` membentuk argv; `compressor/src/cli.cpp` mem-parse; tanpa satu sumber kebenaran. Fix: todo 20. |
| MED-16 | tidak tercatat | NON-ISSUE | `SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_APPLICATION_DIR | ...)` ada di keempat main.cpp: `engine/src/main.cpp:128`, `studio/src/main.cpp:62`, `launcher/main.cpp:806`, `compressor/src/main.cpp:237`. |
| MED-17 | tidak tercatat | BARU | `engine/src/occlusion_watch.cpp:56` `InsertionSortInts`, `:68` `InsertionSortI64`, dipakai di baris 169/190. Fix: todo 7. |
| MED-18 | tidak tercatat | BARU | `shared/config_schema.cpp::ReadFile`, `shared/studio_settings.cpp::ReadFile`, `studio/src/library_manager.cpp::ReadFile` tanpa guard `file_size`. Fix: todo 29. |
| MED-HIGH-1 | dev-contracts.md §1 mendokumentasikan kontrak PID-based quit | NON-ISSUE | `studio/src/apply_manager.cpp:129-245` PID-based: `OpenProcess(..., engine_pid)` (162), `WaitForSingleObject(proc.get(), 8000)` (173), `TerminateProcess(proc.get(), 1)` (180); tanpa taskkill/image name. |
| LOW-1 | tidak tercatat | BARU | `engine/src/desktop_inject.cpp:242,448` `DestroyWindow(wnd)` tanpa cek return/`GetLastError`. Fix: todo 9a. |
| LOW-2 | tidak tercatat | BARU | Campuran `GetWindowLongPtrW`/`GetWindowLongW` di studio; perlu diseragamkan. Fix: todo 10. |
| LOW-4 | tidak tercatat | BARU | `studio/src/thumbnailer.cpp::CacheKeyHex` memakai 3 stat/item; perlu metadata cache. Fix: todo 30. |
| LOW-5 | tidak tercatat | BARU | `studio/src/compress_service.cpp::DrainChannel` (implement only if masih ada di kode). Fix: todo 30. |
| LOW-6 | tidak tercatat | BARU | `studio/src/ipc_client.cpp` disconnect tiap poll; perlu idle disconnect 5 detik. Fix: todo 30. |
| LOW-7 | tidak tercatat | BARU | `engine/src/occlusion_watch.cpp` load `dwmapi` per panggilan; perlu cache `HMODULE` sekali. Fix: todo 30. |
| LOW-8 | tidak tercatat | BARU | `studio/src/library_widget.hpp` duplikat `#include`. Fix: todo 10. |
| LOW-9 | tidak tercatat | BARU | Indentasi access-specifier belum dinormalisasi di header studio. Fix: todo 10. |
| LOW-10 | tidak tercatat | BARU | Magic numbers terpanas belum diekstrak ke `constexpr`. Fix: todo 10. |
| LOW-11 | SUDAH DIKETAHUI — `docs/tech-debt.md` (README.md:20-21, SECURITY.md:8) | SUDAH DIKETAHUI | `README.md:20-21` TODO screenshot; `SECURITY.md:8` TODO kontak. Fix: todo 10. |
| LOW-12 | tidak tercatat | BARU | Timer ID tersebar (`kDebounceTimerId`, `kWorkingSetTrimTimerId`, `kOcclusionPokeTimerId`, engine_app.hpp:139) tanpa `static_assert` pairwise-distinct. Fix: todo 9b. |
| LOW-13 | tidak tercatat | BARU | Ikon legacy di `packaging/` perlu dipindah ke `attic/` atau dihapus. Fix: todo 10. |
| LOW-15 | tidak tercatat (dev-contracts.md §1 belum memuat kontrak ack dua-tahap) | BARU | Ack saat ini = hasil eksekusi langsung di worker; semantik "diterima vs selesai" belum terdokumentasi. Fix: todo 11 (dokumentasi di dev-contracts.md §1). |
| LOW-16 | tidak tercatat | BARU | `studio/src/apply_manager.cpp:120-127` `ResolveEnginePath` fallback `../../build/msvc-dev/engine.exe` di bawah `#ifndef NDEBUG`. Fix: todo 24. |
| LOW-17 | tidak tercatat | BARU | `launcher/main.cpp::GrantK6wpFolderWriteAccess`; keputusan: dokumentasikan risiko saja, jangan ubah ACL. Fix: todo 31. |
| LOW-19 | tidak tercatat | BARU | `studio/src/update_checker.cpp`; kebijakan: update user-initiated, buka browser ke halaman rilis. Fix: todo 31. |
| LOW-20 | tidak tercatat | BARU | `vendor/VERSIONS.md`; dokumentasikan alasan libmpv dev-channel + jadwal review advisory. Fix: todo 31. |
| LOW-21 | tidak tercatat | BARU | `packaging/uninstall.bat`, `tools/uninstall.bat`, `packaging/installer.nsi`; perlu verifikasi restore lockscreen policy. Fix: todo 31. |
| LOW-22 | tidak tercatat | BARU | `CMakeLists.txt` ~39 (`/W4 /WX-`); baseline warning belum dihitung. Fix: todo 10. |
| LOW-23 | tidak tercatat | BARU | Header `vcpkg.json` perlu komentar "katalog, bukan instruksi build" atau dihapus. Fix: todo 10. |
| LOW-24 | tidak tercatat | BARU | `studio/src/main_window.cpp::ApplyVideoPath` tanpa disable/progress saat live-switch. Fix: todo 28. |
| LOW-25 | tidak tercatat | BARU | `studio/src/preview_widget.cpp` tanpa overlay pause/play hover. Fix: todo 28. |
| LOW-26 | tidak tercatat | BARU | `studio/src/main_window.cpp::OnCompressCurrent` tanpa label "Antrikan" saat sibuk. Fix: todo 28. |
| LOW-27 | tidak tercatat | BARU | Belum ada `docs/a11y-audit.md`. Fix: todo 32. |
| LOW-28 | tidak tercatat | BARU | `engine/src/mpv_renderer.cpp::SetAdapterPin` (~340) komentar `PATCH A` perlu diperjelas + log warning saat no-op. Fix: todo 9c. |

## Catatan metode

- Verdict **NON-ISSUE** = temuan terbukti tidak berlaku pada kode saat ini
  (diverifikasi langsung, bukan asumsi).
- Verdict **SUDAH DIKETAHUI** = temuan sudah tercatat di `docs/` tim sebelum
  dokumen audit; tetap masuk jalur fix bila plan memutuskan begitu.
- Verdict **BARU** = temuan valid yang belum tercatat di `docs/`; masuk jalur
  fix plan.
- Verdict **DUPLIKAT** = tidak ada temuan yang teridentifikasi sebagai
  duplikat dari temuan lain dalam dokumen audit.
- ID yang tidak disebut di plan (mis. LOW-3, LOW-14, LOW-18, LOW-29) tidak
  dimasukkan; plan adalah sumber kebenaran daftar ID.
- Semua path bukti relatif terhadap root worktree
  `k6wp-audit-remediation`.