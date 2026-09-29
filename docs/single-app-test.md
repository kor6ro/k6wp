# K6WP Single-App - Dokumen Uji Manual + Reboot + Bug/Limitasi

Ringkasan: K6WP adalah satu software final. Cara pakai: jalankan K6WP.exe,
jendela Studio terbuka, pilih video, klik Apply, wallpaper desktop langsung
berganti tanpa restart engine. Engine + tray tetap jalan walau Studio
ditutup. Semua langkah di bawah memakai artefak rilis
dist/K6WP-portable-1.0.0.zip (245.3 MB, bukti: dist/make_zip-1.0.0.log
baris 106-107). Setiap langkah punya format: Perintah -> Observasi yang
diharapkan -> Aturan PASS/FAIL. Label [manual-only] artinya langkah itu
belum punya log bukti otomatis dan harus dicek manusia.

## 1. Tabel file diubah/ditambah per todo (1-12) + alasan satu baris

| Todo | File | Kenapa (satu baris) |
|------|------|---------------------|
| 1 | studio/src/main_window.hpp/.cpp | Workspace live-apply: tampil path, tombol Browse/Import/Apply, X=quit |
| 1 | studio/src/apply_manager.cpp | IPC-first set_video, fallback restart bila engine mati |
| 2 | studio/src/main_window.hpp/.cpp | Label status engine (poll get_state) + tombol Pause/Resume |
| 3 | studio/src/main_window.hpp/.cpp | Panel library: list, select-to-apply, hapus, empty-state |
| 3 | studio/src/library_manager.cpp | Remove() hapus metadata + file + thumbnail (deviasi D1) |
| 4 | studio/src/main_window.hpp/.cpp | Tombol Compress + label progres (async, non-blocking) |
| 4 | studio/src/compress_bridge.* | Bridge ke compressor.exe (CLI tidak diubah) |
| 5 | engine/src/engine_app.cpp | HandleSetVideo + OnTrayQuickSwitch persist video_path ke config.json |
| 5 | shared/config_schema.* | Reuse SaveConfig/DefaultConfigPath (%LOCALAPPDATA%/K6WP) |
| 6 | attic/tests/app_live_apply.py | Skrip QA: 5x set_video, PID stabil, persist, bad-path, reconnect (archived) |
| 7 | launcher/main.cpp | K6WP.exe dispatcher Qt-free: modes, mutex, probe-sebelum-spawn |
| 7 | launcher/CMakeLists.txt | Target launcher, OUTPUT_NAME K6WP, link user32+shell32 saja |
| 7 | CMakeLists.txt (root) | add_subdirectory launcher |
| 8 | launcher/main.cpp | DETACHED_PROCESS spawn, exit codes 0/2/3, --help, --minimized fwd |
| 9 | packaging/app.ico + app_paused.ico | Placeholder worker art (16/32/48/256 px, glyph K6) |
| 9 | packaging/app-icon-README.md | Cara ganti icon tanpa ubah kode |
| 9 | packaging/make_app_icon.py | Generator .ico via Pillow, usage: python packaging/make_app_icon.py |
| 9 | engine/app.rc + engine/resource.h | IDI_APPICON 101 / IDI_APPICON_PAUSED 102 + VERSIONINFO K6WP 1.0.0 |
| 9 | engine/CMakeLists.txt | Tambah app.rc + packaging di RC include path |
| 9 | engine/src/tray.cpp/.hpp | LoadIconW resource 101/102 + fallback stock; menu Show/Pause/Next/Exit |
| 9 | engine/src/engine_app.cpp | Slot get_current/on_next, OpenStudio fokus-jika-ada, boot-seed autoplay |
| 10 | shared/autostart.hpp | Komentar AutostartCommand diluruskan ke K6WP.exe --engine --silent |
| 10 | shared/autostart.cpp | Run value -> "K6WP.exe" --engine --silent; fallback engine.exe --minimized |
| 11 | engine/CMakeLists.txt | WIN32_EXECUTABLE TRUE (subsystem Windows GUI) |
| 11 | studio/CMakeLists.txt | WIN32_EXECUTABLE TRUE (subsystem Windows GUI) |
| 11 | launcher/CMakeLists.txt | WIN32_EXECUTABLE TRUE (subsystem Windows GUI) |
| 11 | engine/src/log_file.hpp/.cpp | Log mirror ke %LOCALAPPDATA%/K6WP/engine.log + OutputDebugString fallback |
| 11 | studio/app.rc + launcher/app.rc | VERSIONINFO minimal (ProductName K6WP, 1.0.0) + app.ico |
| 12 | packaging/make_zip.ps1 | Stage K6WP.exe + assert Qt-free + assert version + assert <250MB |
| 12 | dist/K6WP-portable-1.0.0.zip | Artefak rilis final (245.3 MB) |

## 2. Cara build (perintah persis)

Jalankan dari root repo (`<repo-root>`):

1. Set Qt:
   $env:QT_ROOT = "C:\Qt\6.8.3\msvc2022_64"
2. Configure:
   cmake --preset msvc-dev
3. Build semua target:
   cmake --build --preset msvc-dev
4. Unit test config:
   build\msvc-dev\config_test.exe
   (exit 0 = semua lolos)
5. Unit test IPC:
   build\msvc-dev\ipc_test.exe
   (exit 0 = semua lolos)
6. Packaging rilis:
   powershell -ExecutionPolicy Bypass -File packaging\make_zip.ps1
   (hasil: dist\K6WP-portable-1.0.0.zip + dist\make_zip-1.0.0.log)

## 3. Uji manual bernomor (14 kriteria acceptance)

Prasyarat: ekstrak ZIP rilis ke folder kosong, tutup semua engine.exe /
studio.exe / K6WP.exe (cek via tasklist). Siapkan 2 file video valid
(sebut A dan B). Semua perintah di bawah dijalankan di folder hasil ekstrak.

Langkah 1 - K6WP.exe launch (default).
Perintah: klik ganda K6WP.exe (atau: K6WP.exe).
Observasi: satu proses engine.exe naik, satu proses studio.exe naik.
Aturan: PASS jika tasklist menunjukkan tepat 1 engine.exe dan 1 studio.exe;
FAIL jika engine.exe berjumlah 0 atau 2+.

Langkah 2 - Studio terbuka.
Perintah: lihat jendela Studio (dari Langkah 1, tanpa argumen tambahan).
Observasi: jendela Studio tampil dengan workspace (path video, tombol
Browse/Import/Apply, panel library, log view). [manual-only]
Aturan: PASS jika jendela terlihat dan responsif; FAIL jika tidak muncul
atau freeze >5 detik.

Langkah 3 - Pilih video.
Perintah: di Studio klik Browse/Import, pilih file A (atau klik entri di
panel library).
Observasi: path A tampil di display workspace; tidak ada dialog error.
[manual-only]
Aturan: PASS jika path A tampil; FAIL jika path kosong atau error.

Langkah 4 - Apply -> wallpaper langsung berganti (tanpa restart engine).
Perintah: catat PID engine via tasklist /FI "IMAGENAME eq engine.exe",
lalu klik Apply; bandingkan PID sesudah Apply.
Observasi: wallpaper desktop berganti ke A; PID engine SAMA sebelum/sesudah
(get_state video==A).
Aturan: PASS jika wallpaper berganti dan PID identik; FAIL jika PID berubah
(engine restart) atau wallpaper tetap.

Langkah 5 - Cek config.json.
Perintah: buka %LOCALAPPDATA%\K6WP\config.json (notepad).
Observasi: field video_path sama persis dengan path A yang di-Apply.
Aturan: PASS jika video_path==A; FAIL jika masih path lama atau file hilang.

Langkah 6 - Tray muncul.
Perintah: lihat notification area; buka %LOCALAPPDATA%\K6WP\engine.log.
Observasi: ikon K6WP terlihat dengan tooltip "K6WP Engine"; log berisi
baris "tray: using embedded app icon (resource 101, paused=0)" dan
"tray: icon installed (callback=WM_APP+20)". [manual-only untuk ikon visual;
baris log terbukti di build/app_live_apply.log baris 7-8]
Aturan: PASS jika ikon terlihat DAN dua baris log ada; FAIL jika ikon absen
atau log memuat "using stock icon".

Langkah 7 - Tray Pause/Resume.
Perintah: klik kanan ikon tray -> Pause; lalu klik kanan -> Resume.
Observasi: setelah Pause tooltip menjadi "K6WP Engine - Paused" dan gerakan
wallpaper berhenti; setelah Resume tooltip kembali dan wallpaper jalan lagi.
[manual-only untuk visual; round-trip IPC terbukti di build/qa_todo234.log
baris 4-6]
Aturan: PASS jika tooltip berubah dan gerakan berhenti/jalan sesuai; FAIL
jika tidak ada perubahan atau engine crash.

Langkah 8 - Buka Studio dari tray (reopen-from-tray).
Perintah: tutup jendela Studio (X); lalu klik kanan tray -> Show Studio.
Observasi: jendela Studio muncul lagi (atau fokus jika sudah ada); jumlah
engine.exe tetap 1.
Aturan: PASS jika Studio tampil dan engine tetap 1; FAIL jika Studio tidak
muncul atau engine menjadi 2.

Langkah 9 - Tanpa console hitam.
Perintah: jalankan K6WP.exe / engine.exe / studio.exe; hitung jendela
conhost baru; cek dumpbin /headers (subsystem) dan engine.log.
Observasi: tidak ada jendela console; dumpbin menulis
"subsystem (Windows GUI)" untuk ketiganya; engine.log tetap tumbuh.
[manual-only untuk hitung conhost; dumpbin+log terbukti di
build/qa_todo11.log baris 2-4 dan 12-16]
Aturan: PASS jika nol console baru dan 3 subsystem GUI; FAIL jika console
muncul atau subsystem masih Console.

Langkah 10 - Restart Studio tanpa duplikat engine.
Perintah: dari kondisi 1 engine + 1 studio, tutup Studio (X), buka lagi via
K6WP.exe; cek tasklist tiap tahap.
Observasi: selama Studio tutup, engine.exe tetap 1; setelah buka lagi,
tetap 1 engine.exe + 1 studio.exe.
Aturan: PASS jika hitungan 1/1 di semua tahap; FAIL jika engine 0 atau 2+.

Langkah 11 - Ganti video berulang tanpa crash.
Perintah: ulangi pilih-Apply antara A dan B sebanyak 10x; pantau PID dan
get_state tiap ganti.
Observasi: tiap ack <<500ms, PID stabil, video mengikuti terakhir,
config.json cocok tiap ganti; tidak ada crash/hang.
Aturan: PASS jika 10/10 ack ok + PID sama; FAIL jika ada ack error/crash.
(Catatan bukti: skrip otomatis baru membuktikan 5x; bukti:
build/app_live_apply.log baris 13-22 + RESULT: PASS baris 31. Sisa 5x
berikutnya [manual-only].)

Langkah 12 - Start-with-Windows ON.
Perintah: di Studio centang Start with Windows; lalu cek:
reg query "HKCU\Software\Microsoft\Windows\CurrentVersion\Run" /v K6WP
Observasi: value persis: "<...>\K6WP.exe" --engine --silent.
Simulasi logon: jalankan command dari value itu secara manual.
Observasi lanjutan: 1 engine naik, 0 studio, IPC get_state ok, tray naik.
(Terbukti di build/qa_todo10.log baris 4 dan 6-9.)
Aturan: PASS jika reg value cocok DAN simulasi menghasilkan engine=1,
studio=0; FAIL jika value salah atau studio ikut terbuka.

Langkah 13 - Start-with-Windows OFF.
Perintah: di Studio hilangkan centang; lalu cek reg query yang sama.
Observasi: value K6WP tidak ada ("value-gone"). (Terbukti di
build/qa_todo10.log baris 10.)
Aturan: PASS jika reg query melaporkan value tidak ditemukan; FAIL jika
value masih ada.

Langkah 14 - Sanity build + packaging.
Perintah: cmake --build --preset msvc-dev (exit 0);
build\msvc-dev\config_test.exe (exit 0); build\msvc-dev\ipc_test.exe
(exit 0); powershell -ExecutionPolicy Bypass -File packaging\make_zip.ps1
(exit 0).
Observasi: ZIP berisi K6WP.exe + engine/studio/compressor + Qt deploy +
ffmpeg + libmpv; log memuat "Import check OK", "Qt-free assert OK",
"Version assert OK", "Size assert OK: 245.3 MB < 250 MB".
(Terbukti di dist/make_zip-1.0.0.log baris 70-75 dan 106-107.) [manual-only
bila dijalankan ulang oleh pembaca; klaim angka berasal dari log rilis]
Aturan: PASS jika keempat perintah exit 0 dan empat baris assert ada;
FAIL jika salah satu exit non-nol atau assert hilang.

## 4. Uji reboot autostart (ON vs OFF)

### 4a. SIMULASI (tanpa reboot fisik - sudah terbukti di build/qa_todo10.log)

1. Nyalakan toggle ON (Langkah 12), salin persis Run value.
2. Jalankan command value itu manual dari cmd (mensimulasikan logon).
3. Cek 4 kondisi: (a) tepat 1 engine.exe di tasklist; (b) 0 studio.exe;
   (c) get_state menjawab ok; (d) tray terpasang (log "tray: icon installed").
   Bukti: build/qa_todo10.log baris 6 (T2 autologon-cmd PASS: exit=0,
   engine=1, studio=0, ipc-up) dan baris 9 (T2t tray PASS).
4. Cek wallpaper pipeline: kirim set_video A, pastikan get_state video==A.
   Bukti: build/qa_todo10.log baris 8 (T2w PASS).
5. Matikan toggle OFF, pastikan value hilang (build/qa_todo10.log baris 10).
6. Boot-autoplay dari config (tanpa --video) terbukti terpisah:
   build/qa_todo5.log skenario 2 PASS + build/app_live_apply.log baris 9
   ("engine: boot autoplay ... starting renderer").

### 4b. FISIK (wajib dilakukan pengguna - [manual-only])

Kasus ON:
1. Toggle ON, pastikan reg value benar (Langkah 12).
2. Restart PC fisik, login sebagai user yang sama.
3. Checklist: (a) wallpaper terakhir tampil otomatis; (b) ikon tray K6WP ada;
   (c) tasklist menunjukkan tepat 1 engine.exe; (d) TIDAK ada jendela Studio
   terbuka; (e) tidak ada prompt UAC saat logon (HKCU tidak butuh admin).
4. PASS jika semua (a)-(e) benar; FAIL jika salah satu gagal, catat yang mana.

Kasus OFF:
1. Toggle OFF, pastikan reg value hilang (Langkah 13).
2. Restart PC fisik, login.
3. Checklist: tasklist TIDAK memuat engine.exe/studio.exe/K6WP.exe; tidak ada
   ikon tray; tidak ada prompt apapun.
4. PASS jika sistem bersih; FAIL jika ada proses K6WP yang jalan.

## 5. Bug/limitasi jujur

B1. D1 - Studio Delete menghapus metadata + FILE (bukan metadata saja).
Dialog konfirmasi menulis eksplisit "Remove this entry (metadata and
file)?" plus path file. Ini desain yang diterima (bukan bug), tapi pengguna
harus tahu file videonya ikut terhapus. (Sumber: Read
studio/src/main_window.cpp baris 312-315 + Read
studio/src/library_manager.cpp baris 220-226.)

B2. Engine GetConfig dibaca di thread IPC tanpa lock (jinak).
ConfigWatcher::GetConfig() hanya mengembalikan referensi tanpa mutex
(engine/src/config_watch.cpp baris 110-112) dan dipanggil dari path
HandleSetVideo yang berjalan di thread server IPC (engine/src/engine_app.cpp
baris 517). Ini pola yang sama dengan power-reader pre-existing
(ReadSystemPower(config_watcher.GetConfig()) di engine/src/power.hpp
baris 56). Praktis aman karena penulis config (TryReload/persist) dan
pembaca berjalan berurutan di alur IPC, tapi pembaca kode harus tahu tidak
ada lock di sini. Berbeda dengan BuildStateJson yang mengunci video_mutex_
(engine/src/engine_app.cpp baris 605-608).

B3. Ikon .ico placeholder adalah worker art, bukan final.
File packaging/app.ico + app_paused.ico dibuat via
packaging/make_app_icon.py (glyph "K6", ukuran 16/32/48/256). Ganti artwork
cukup dengan menimpa dua file .ico itu (nama sama) lalu rebuild engine;
TIDAK perlu ubah kode. (Sumber: Grep packaging/make_app_icon.py SIZES +
Read .omo/notepads/k6wp-single-app/tray.md + packaging/app-icon-README.md.)

B4. Tray Next/Show-Studio butuh Explorer/tray asli.
QA headless hanya membuktikan via baris log ("tray: icon installed",
"tray: menu ... selected", "Next Wallpaper ignored (MRU empty)").
Klik visual, tooltip, dan fokus-jendela Show-Studio hanya bisa dibuktikan di
mesin dengan desktop aktif. Bukti keterbatasan: build/qa_todo5.log baris 3-4
("BLOCKED: ... no display/tray interaction possible in this environment").
Next dengan MRU kosong diabaikan + log tanpa crash (engine/src/tray.cpp
baris 248). (Sumber: Grep engine/src/tray.cpp + Read build/qa_todo5.log.)

B5. Mutex single-instance dipegang selama Studio berjalan.
Nama mutex Local\K6WP-Studio-Singleton (per-session, per-user). Launcher
pertama menahan mutex sambil menunggu proses studio; instance kedua hanya
memfokuskan jendela lalu exit 0 tanpa spawn. Artinya satu user satu Studio;
ini by-design, bukan bug. (Sumber: Grep launcher/main.cpp baris 42 +
Read .omo/notepads/k6wp-single-app/launcher.md.)

B6. Catatan historis: build/qa_todo1.log stage4 pernah FAIL
(video==B tidak pulih setelah restart path, RESULT: FAIL baris 36 dan 47).
Sudah diperbaiki oleh boot-seed autoplay di Init (LoadLoop path config bila
valid); verifikasi ulang PASS di build/qa_todo5.log skenario 2 dan
build/app_live_apply.log baris 9. Disebut di sini agar riwayat bukti jujur.

B7. build/qa_todo7.log tidak terbaca (biner) di sesi penulisan; klaim
launcher dirujuk ke build/qa_todo78.log (ASCII, T0-T8 PASS) sebagai gantinya.
Jangan kutip qa_todo7.log sebagai bukti sampai ditulis ulang ASCII.

B8. get_state tidak punya field "paused".
BuildStateJson hanya berisi running, pid, wallpaper_mode, video, config
(engine/src/engine_app.cpp baris 609-616). Status pause hanya terlihat via
tooltip tray ("K6WP Engine - Paused") dan berhentinya gerakan wallpaper.
Otomasi yang butuh flag pause harus baca tooltip/log, bukan get_state.

## 6. Catatan uninstall

Jalankan tools/uninstall.bat (ikut ter-stage di ZIP, bukti
dist/make_zip-1.0.0.log baris 68). Skrip menghapus Run value K6WP
(reg delete HKCU...\Run /v K6WP) lalu menawarkan hapus data
%LOCALAPPDATA%\K6WP (jawab y/N). HKCU tidak butuh admin/UAC.
(Sumber: Read tools/uninstall.bat.)

## 7. Sumber (metode verifikasi per bagian)

- Ringkasan + angka ZIP: Read dist/make_zip-1.0.0.log (baris 51-58,
  70-75, 93, 106-108).
- Tabel file: Grep file-per-todo dari .omo/plans/k6wp-single-app.md +
  Read .omo/notepads/k6wp-single-app/{packaging,tray,autostart,launcher,
  engine}.md + Grep simbol kunci (mutex, AutostartCommand, Remove,
  GetConfig, tray menu) untuk memastikan path/symbol benar-benar ada.
- Cara build: Grep QT_ROOT + cmake --build --preset msvc-dev di
  packaging/make_zip.ps1 (baris 57-58); Grep preset msvc-dev di
  CMakePresets.json; Grep config_test/ipc_test di shared/CMakeLists.txt;
  usage header packaging/make_zip.ps1 baris 12.
- Langkah 1-5, 11: Read build/qa_todo1.log, build/qa_todo234.log,
  build/app_live_apply.log; Read engine/src/engine_app.cpp baris 505-531
  (persist) + 603-618 (state fields); D1: Read studio/src/main_window.cpp
  baris 298-325 + studio/src/library_manager.cpp baris 213-230.
- Langkah 6-8: Grep engine/src/tray.cpp (menu + tooltip + TaskbarCreated) +
  Read build/qa_todo9.log + build/app_live_apply.log baris 7-8.
- Langkah 9: Read build/qa_todo11.log baris 2-4, 12, 20, 22.
- Langkah 10: Read build/qa_todo78.log baris 8-10 + Grep mutex
  launcher/main.cpp baris 42.
- Langkah 12-13 + reboot simulasi: Read build/qa_todo10.log (12 baris) +
  Grep shared/autostart.cpp/.hpp (Run value format).
- Langkah 14: Read dist/make_zip-1.0.0.log baris 69-75.
- Bug/limitasi: Read/Grep sesuai kutipan tiap item B1-B8 di atas.
- Uninstall: Read tools/uninstall.bat (28 baris).
- Yang TIDAK diverifikasi otomatis dan ditandai [manual-only]: jendela
  Studio visual, tooltip visual, hitung conhost manual, reboot fisik,
  5x switching tambahan di luar skrip, dan eksekusi ulang build oleh pembaca.
