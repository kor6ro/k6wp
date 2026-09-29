# Spike Results — Todo 3: WorkerW Strategi-A/B + fallback Progman

**Date**: 2026-09-16
**Machine**: Windows 11 Pro, build 26200 (24H2-era), 1920x1080 primary, single monitor
**Probe**: `attic/spikes/workerw_probe.cpp` (throwaway, NOT shipped; not in CMake build)
**Commit**: `k6wp-3: spike workerw A-B fallback (fase-0)`

## Verdict

**Injection mechanics VALIDATED on Win11 24H2; visible rendering requires DX presents (not GDI).**
The probe attaches a layered child window to Progman behind the icons (z-order verified, hit-testable via `WindowFromPoint`), but GDI-painted content is NOT composited to the screen because Progman is `WS_EX_NOREDIRECTIONBITMAP` (no GDI content at all). Per Microsoft's official guidance, the app must present frames to the layered window via DX (DXGI/D3D11 blt presents) — which is exactly what the mpv D3D11 renderer does in Todo 10.

The plan's QA-fail scenario is REAL on this machine: with **"Animate controls..." OFF**, `0x052C` spawns **nothing**. The probe fails gracefully, tries the Progman fallback, and logs clearly (QA-fail criterion met).

## Machine facts (build 26200)

| Item | Value |
|---|---|
| OS | Windows 10.0 (build 26200) — Win11 24H2-era |
| Client animations ("Animate controls...") | **OFF** |
| Progman HWND | `0x000000000001016C` (stable across runs), visible=YES |
| SHELLDLL_DefView HWND | `0x0000000000010170`, visible=YES, child of Progman |
| DefView inside Progman | **YES** (24H2 layout) → Strategy A (classic WorkerW-with-DefView) is N/A |
| DefView child | `SysListView32` (desktop icons list view) |
| Top-level WorkerW count | 13 before AND after `0x052C` (spawned=0) |
| Progman WorkerW children | 1 before AND after `0x052C` (spawned=0); HWND unstable across runs (0x2504FE→0x3304DE→0x3A07BC→...) |
| All 13 top-level WorkerWs | attach OK but `visible=NO` (shell-internal, unusable) |

## Strategy results

### Strategy A — classic WorkerW-with-DefView
**N/A.** DefView is a direct child of Progman (24H2 layout), not of a WorkerW. No top-level WorkerW contains DefView.

### Strategy B — 0x052C spawn + empty WorkerW
**FAILS on this machine.** With client animations OFF, `SendMessageTimeoutW(progman, 0x052C)` returns OK (result=0, error=0) but spawns **0** WorkerW (top-level 13→13, Progman-child 1→1). This is the plan's QA-fail scenario. The 13 existing top-level WorkerWs are all invisible shell-internal windows.

### Fallback — SetParent to Progman
**Injection works; GDI content not composited.**

Verified sequence (the winning recipe for Todo 9):
1. Create window as top-level `WS_POPUP` with `WS_EX_LAYERED | WS_EX_NOACTIVATE` (cross-process `CreateWindowEx` with a foreign parent is denied with error 5; create top-level first, then `SetParent`).
2. `SetParent(wnd, progman)` → OK (error=0).
3. **Re-apply `WS_EX_LAYERED` after SetParent** (`SetWindowLongPtr GWL_EXSTYLE`) + `SetLayeredWindowAttributes(wnd, 0, 255, LWA_ALPHA)` — SetParent strips the layered style; without re-applying, DWM never composites the child.
4. `SetWindowPos(wnd, defView, ...)` → z-order becomes `[DefView, test-window, WorkerW-child]` — directly below the icons, above the wallpaper layer.
5. `ShowWindow(SW_SHOW)`.

Evidence:
- `WindowFromPoint(center)` returns the test window (`K6WPProbeWnd`) when no other window occludes the desktop → the window IS on screen and hit-testable.
- `PrintWindow` returns RGB(192,192,192) → the window HAS gray content.
- Screen capture shows the wallpaper (RGB 2,10,109), NOT gray → GDI content is not composited.
- Sanity check: a plain top-level window (same class, same GDI brush) renders fine → GDI works for top-level windows; the desktop child path is the difference.

**Root cause**: Progman is created with `WS_EX_NOREDIRECTIONBITMAP` (Microsoft: "there is no GDI content for that window at all"). GDI-painted children of Progman are not redirected/composited. The official guidance: create a `WS_EX_LAYERED` child with `SetLayeredWindowAttributes(bAlpha=0xFF)` and **do DX blt presents** to it.

### Progman WorkerW child (native wallpaper layer)
Attach to the empty WorkerW child of Progman: SetParent OK, visible=YES, but same GDI-not-composited result. The WorkerW child is below DefView (behind icons) — correct position, but GDI content still not composited.

### DefView attach
Attach to DefView directly, pushed to bottom of its z-order (below `SysListView32` icons): SetParent OK, but GDI content not composited either.

## Occlusion caveat (important for QA)

A maximized window (e.g., Chrome at `(-8,-8)-(1928,1088)`) covering the desktop **hides the injected window** — expected, since the injected window is behind all top-level windows. Screenshot verification of the wallpaper MUST be done with other windows minimized/closed. This also means `WindowFromPoint` is only a valid on-screen check when the desktop is unobstructed.

## Recommendation for Todo 9 (engine/src/desktop_inject)

1. **Primary path on 24H2**: layered-child injection into Progman (recipe above). Do NOT rely on `0x052C` spawning a WorkerW — it spawns nothing when animations are OFF.
2. **Rendering**: the injected window must be presented via DX (D3D11 swapchain / DXGI blt presents), NOT GDI. mpv's `wid` + D3D11 hwdec renderer (Todo 10) targets the window handle directly — this is the intended path.
3. **Keep Strategy A/B code paths** for Win10 22H2 (classic WorkerW layout) — not testable on this machine.
4. **Z-order maintenance**: theme changes / wallpaper transitions can recreate the WorkerW child; re-apply `SetWindowPos(wnd, defView)` on a timer or shell hook (per the 24H2 guide).
5. **Detect 24H2**: check Progman for `WS_EX_NOREDIRECTIONBITMAP` (Microsoft's suggested signal) to choose the layered-child path vs the classic WorkerW path.

## Probe CLI (for reference)

```
workerw_probe.exe [--wallpaper-mode=auto|workerw|progman] [--hold-ms=N]
                  [--probe-all-workerws] [--target=progman-child|defview|top-level]
                  [--insert-below-defview] [--child-style] [--layered]
```

Exit 0 = at least one attach OK. All modes exit 0 on this machine (attach succeeds; visibility is a separate concern).

## Evidence files

- Probe source: `attic/spikes/workerw_probe.cpp`
- Screenshots (temp): `probe_layered4_screenshot.png` (layered Progman attach, desktop clear), `sanity_screenshot.png` (top-level GDI window renders), `baseline_screenshot.png`
- Probe logs (temp): `probe_final_{auto,workerw,progman}.txt`

---

# Spike Results — Todo 4: mpv hwdec + Qt embed + encoder 1-frame test

**Date**: 2026-09-16
**Machine**: same as Todo 3 (Win11 build 26200, 1920x1080, NVIDIA + Intel iGPU)
**Spikes**: `attic/spikes/mpv_hwdec.cpp`, `attic/spikes/qt_embed.cpp`, `attic/spikes/enc_probe.ps1` (throwaway, NOT shipped)
**Commit**: `k6wp-4: spike mpv-qt-encoder (fase-0)`

## Verdict

**All three spikes PASS.** libmpv renders 1080p H.264 into a window with `hwdec=d3d11va` active (log `Using hardware decoding (d3d11va)`, property `hwdec-current=d3d11va`). Software fallback (`hwdec=no`) still renders with **1.69× higher CPU** (842M vs 498M cycles/sec). Qt 6.8.3 `QWidget` + `WA_NativeWindow` + `winId→wid` embeds mpv in a QThread with no flicker (2 captures both show video content). Encoder probe picks `h264_nvenc` first; QSV also works; AMF fails gracefully (no AMD GPU); libx264 is the final fallback.

## mpv hwdec spike (`mpv_hwdec.cpp`)

- **Setup**: top-level Win32 window 1280x720 → `mpv_set_option_string("wid", <hwnd>)`, `hwdec=d3d11va`, `loop-file=inf`, `audio=no`, `vo=gpu`, `keep-open=yes`.
- **Result (hwdec=d3d11va)**: `[mpv-log] Using hardware decoding (d3d11va).`, `hwdec-current = d3d11va`, playback_started=1, exit 0.
- **Result (hwdec=no)**: `hwdec-current = no`, playback_started=1, exit 0 — software fallback works. CPU: **842,012,735 vs 498,352,373 cycles/sec (1.69×)** — QA-fail criterion "CPU lebih tinggi tercatat" met.
- **CPU measurement**: `QueryProcessCycleTime` delta over the hold window (not Task Manager).

## Qt embed spike (`qt_embed.cpp`)

- **Setup**: `QWidget` with `Qt::WA_NativeWindow`, `w.show()` + `w.raise()` + `w.activateWindow()`, `WId wid = w.winId()` → passed to mpv as `wid`. mpv handle created/used **inside a QThread** (`MpvWorker : QThread`, `run()` does create/init/loadfile/event-loop). No `Q_OBJECT`/moc needed (plain QThread subclass + `std::atomic` flags + `QTimer` polling).
- **Result**: `[qt] playback started`, both screen-region BMP captures show testsrc2 color bars (red 255,24,0 / green 0,216,0 / yellow 255,240,0; 1124/1170 unique sampled colors) → **video renders, no flicker** (2 captures 1s apart both show content, not blank).
- **Capture method**: D3D content is NOT readable via GDI BitBlt from the window DC — capture the **screen region** (`GetWindowRect` + BitBlt from screen DC) after DWM composition. Same lesson as Todo 3.
- **Compile flags required by Qt 6.8.3 on MSVC**: `/Zc:__cplusplus` AND `/permissive-` (both mandatory, else `qcompilerdetection.h` errors).
- **Runtime**: needs `C:\Qt\6.8.3\msvc2022_64\bin` on PATH + `QT_QPA_PLATFORM_PLUGIN_PATH=C:\Qt\6.8.3\msvc2022_64\plugins\platforms` (qwindows.dll).

## Encoder probe (`enc_probe.ps1`)

- **Method**: `ffmpeg -f lavfi -i testsrc2=size=1920x1080:rate=30 -frames:v 1 -c:v <enc> -y out.mp4` per encoder, order NVENC→QSV→AMF→libx264, pick first OK.
- **Result**: `picked = h264_nvenc`; h264_qsv OK; h264_amf FAIL (`DLL amfrt64.dll failed to open` — no AMD GPU, expected); libx264 OK. Exit 0.
- **Gotcha**: `$ErrorActionPreference = "Stop"` turns native stderr into a terminating `NativeCommandError` in PowerShell 5.1 — use `"Continue"` + check `$LASTEXITCODE`.

## Recommendation for Todo 10 (engine/src/mpv_renderer) and Todo 15 (encoder_detect)

1. **hwdec chain `d3d11va→dxva2→no`** works as planned; `hwdec-current` property is the reliable signal (log line also confirms).
2. **`wid` embedding works with `vo=gpu`** on a plain top-level window AND a Qt native window — no special VO needed.
3. **Qt embed pattern**: `WA_NativeWindow` + `winId()` + mpv in QThread (no moc) — validated for Todo 23 preview_widget.
4. **Encoder detect**: NVENC→QSV→AMF→libx264 order validated on this machine; AMF failure is graceful (falls through). `PickEncoder()` should log the chosen encoder + failures.
5. **CPU measurement for bench**: `QueryProcessCycleTime` works and distinguishes hwdec vs software (1.69× delta) — use in `tools/bench_cpu_mem.ps1` (Todo 12).

## Evidence files

- Spike sources: `attic/spikes/mpv_hwdec.cpp`, `attic/spikes/qt_embed.cpp`, `attic/spikes/enc_probe.ps1`
- Test video: `build/spikes/test_1080p.mp4` (testsrc2 1080p30, 10s, libx264, 7.1MB)
- Captures (temp): `qt_embed_cap1.bmp`, `qt_embed_cap2.bmp` (both show video content)
- Probe outputs (temp): `probe_final_{auto,workerw,progman}.txt` (Todo 3)

---

# Spike Results — P3.1: shared render (single decode, multi-window fan-out)

**Date**: 2026-09-22
**Machine**: Win11, 1 fisik monitor (QA = 2 window di monitor yang sama),
12th Gen Intel i7-12650H (hybrid 10C/16T), RAM 16GB,
Intel UHD + NVIDIA RTX 2050 + MS Basic Render (DXGI count=3, HYBRID class),
video `build/spikes/test_1080p.mp4` (1920x1080 H264 30fps).
**Spike**: `spikes/render_shared.cpp` (standalone, NOT shipped, NOT in CMake;
engine/ tidak disentuh — `git status` hanya `?? spikes/render_shared.cpp`).
*(2026-09-24: archived to `attic/spikes/render_shared.cpp` — same convention
as the Todo 3–4 probes; paths below refer to the original location.)*
**Build**:
```
vcvars64.bat && cl.exe /EHsc /std:c++17 /W4 spikes\render_shared.cpp
  /I vendor\libmpv\include vendor\libmpv\lib\mpv.lib
  opengl32.lib gdi32.lib user32.lib psapi.lib dxgi.lib
  /Fe:build\spikes\render_shared.exe
```
(THIRD-PARTY note: `LoadCursorW(..., MAKEINTRESOURCEW(32512))` — numeric
literal, bukan `IDC_ARROW`, untuk menghindari warning C4302 double-wrap.)

## Verdict

**ARSITEKTUR SHARING TERBUKTI; CPU-vs-hwdec GAGAL di jalur GL — OPT-B
direkomendasikan untuk Sesi B.**
`wglShareLists` + FBO→texture + `glFenceSync`/`glClientWaitSync` + 1 present
thread per window bekerja: 1 decode memberi makan 2 window di semua 4
fit-mode (contain/cover/stretch/center), thread peak 46 vs 75 (-29),
RAM WS -41MB vs hwdec-dual / -188MB vs software-dual. Tetapi di Windows
desktop-GL, `vo=libmpv` + `hwdec=d3d11va` jatuh ke `hwdec-current=no`
(software, sesuai peringatan docs render_gl.h: Windows butuh ANGLE untuk
hwdec-direct), sehingga CPU shared (+8% vs 2×hwdec-dual) lebih tinggi.
Dengan jalur decode setara (software-vs-software) shared menang CPU -11%.
Sesi B TIDAK BOLEH mengasumsikan CPU menang otomatis di jalur GL —
pilih OPT-B (shared D3D11 texture + blit via interop, hwdec tetap aktif)
atau terima trade CPU demi RAM/thread.

## Angka (semua run: affinity proc_mask=0xffff sys_mask=0xffff,
priority_class=32 NORMAL — kondisi sama, 8 s hold, win 640x360, tex 1920x1080)

| Konfigurasi | decode | vo | hwdec | rendered | presA/B | threads start/peak | cycles/s | WS / priv | brightness |
|---|---|---|---|---|---|---|---|---|---|
| shared cover | 1 | libmpv | no (SW) | 235 | 235/235 | 28/46 | 3,062,057,554 | 331/292MB (start 123/92) | 129.1 |
| dual (N-mpv baseline) | 2 | gpu | d3d11va+d3d11va | n/a | n/a | 24/75 | 2,822,640,302 | 372/382MB (start 39/40) | n/a |
| dual `--hwdec=no` (adil) | 2 | gpu | no+no | n/a | n/a | 24/74 | 3,457,914,895 | 519/531MB (start 39/40) | n/a |

Delta:
- shared vs 2×hwdec-dual: threads -29, WS -41MB, priv -90MB, CPU +8.5%
(+239M cycles/s) — hwdec mismatch, bukan arsitektur.
- shared vs 2×SW-dual (jalur setara): CPU -11.5% (-396M), WS -188MB (-36%),
priv -239MB, threads -28. Penghematan fan-out murni.
- Fit 3 s runs (shared): contain 90/85, cover 88/86, stretch 84/87,
center 86/88 presented, semua PASS, brightness 126.9-127.0 (non-black).

## Apa yang terbukti / tidak

- TERBUKTI: 1 `mpv_render_context` (OPENGL, advctl=1, get_proc =
wglGetProcAddress→opengl32) render ke master-FBO; 2 consumer ctx (pixel
format index SAMA =5) share texture; FBO tidak di-share (consumer hanya
sample); fence per-frame; ctx current di tepat 1 thread;
`wglSwapIntervalEXT(1)`; quad UV/vertex per fit-mode.
- TERBUKTI: teardown order (join present → render_context_free tanpa
concurrent render → terminate) exit 0 di semua run.
- TIDAK TERBUKTI (untested — 1-monitor rig): komposisi DWM per-monitor,
beda refresh rate / resolusi / DPI, span-auto virtual-screen, Explorer
restart / cabut-pasang monitor di mode shared. Sesi B wajib QA di rig
2-fisik sebelum klaim.
- Span-auto (P3.4): UNTESTABLE di rig ini — catat untested, jangan klaim.

## Caveat untuk Sesi B

1. **hwdec jatuh ke `no` di jalur GL** (`[prop] hwdec-current=no` meski
req d3d11va). CPU +8% vs hwdec-dual adalah buktinya. OPT-B (D3D11 shared
texture) satu-satunya jalan yang menjaga hwdec + fan-out. Jika Sesi B
tetap GL: scope pin-GPU (P3.6) ke mode per-monitor saja + tech-debt.
2. **Heuristik P3.6 salah di rig ini**: prompt "integrated =
Shared>Dedicated" mengklasifikasikan RTX 2050 (dedicated 3962MB <
shared 8035MB) sebagai integrated — sama seperti Intel UHD dan Basic
Render. Sesi B butuh heuristik lebih baik (VendorId 0x10DE discrete /
0x8086 integrated, atau substring Description), jangan pakai
perbandingan memori mentah.
3. **RAM spike ≠ RAM engine**: spike WS 331MB (tanpa lean profile P1.1:
tanpa demuxer bound, dumb-mode, dll). Target engine ≤70MB Sesi B butuh
lean options + texture clamp + present-thread ramping, bukan angka spike.
4. **Thread peak metodologi**: Toolhelp snapshot, start diukur pasca
mpv+GL alloc, peak selama hold. Dual start WS 39MB vs shared 123MB
karena shared mengukur pasca texture/FBO+render-ctx alloc — bandingkan
peak-to-peak, bukan start.
5. **P/E scheduling**: semua run affinity+priority identik (default
0xffff/NORMAL). Sesi B bench wajib kunci kondisi yang sama, atau delta
hybrid menipu.
6. Per-window presented selisih 1-5 frame antar window (vsync masing2) —
normal, bukan frame-drop decode (rendered penuh 29fps).

## Evidence / repro

- `build\spikes\render_shared.exe build\spikes\test_1080p.mp4 --mode=shared --hold-ms=8000 --fit=cover --verify`
- `... --mode=dual --hold-ms=8000` (hwdec) dan `... --mode=dual --hwdec=no --hold-ms=8000` (adil)
- Fit loop: `--fit=contain|cover|stretch|center --hold-ms=3000 --verify`
- Full logs di atas (console). Binary di `build/spikes/` (gitignored).

---

# Spike Results — P3.1-A2: OPT-B (native D3D11 render API?) + P3.6 heuristic

**Date**: 2026-09-22
**Spike**: `spikes/render_shared_d3d11.cpp` (PROBE + P3.6 verification ONLY;
fan-out sengaja TIDAK di-code — lihat verdict).
*(2026-09-24: archived to `attic/spikes/render_shared_d3d11.cpp` — original
path kept below for the record.)*
**Otorisasi**: Sesi A2, bukan Sesi B. engine/ untouched
(`git status` hanya `?? spikes/render_shared_d3d11.cpp` + docs).
Spike GL Sesi A (6bfb673) dipertahankan sebagai fallback knowledge.

## Verdict

**B1 (native D3D11 render API) TIDAK TERSEDIA di vendored libmpv.
hwdec-current TIDAK PERNAH TERCAPAI — gate utama gagal, fan-out tidak
di-code (sesuai disiplin: baris hwdec dicek pertama, masih `no`/N/A →
jangan lanjut). Keputusan B2 vs alternatif menunggu otorisasi.**

Double evidence:
1. Empiris: `mpv_render_context_create(api="d3d11")` → rc=**-19
`MPV_ERROR_NOT_IMPLEMENTED`** (`operation not implemented`). Per
render.h docs, -19 = "unknown API type OR support not built in" —
bedakan dari INVALID_PARAMETER (backend ada, param kurang). SEH guard
tidak terpicu (tidak ada AV): string `"d3d11"` ditolak bersih di level
dispatch backend.
2. Statis: headers = client API v2.5 (hanya `"opengl"`/`"sw"`, tanpa
`render_d3d11.h`); DLL `libmpv-2.dll` = v0.41.0-1023 tapi **runtime
`mpv_client_api_version()` = v131077 = 0x20005 = v2.5** (konsisten tanpa
D3D11 API); binary-strings scan: tidak ada string backend render-d3d11
(mpv render backends tidak diekspor sebagai simbol — hanya generic
`mpv_render_context_*`, terkonfirmasi via dumpbin).

## P3.6 heuristic — VERIFIED PASS (satu-satunya gate A2 yang lolos)

| Langkah | Hasil |
|---|---|
| `IDXGIFactory6::EnumAdapterByGpuPreference(0, MINIMUM_POWER)` | `Intel(R) UHD Graphics` vend=0x8086 ded=128MB shared=8035MB via=`GpuPreference(MINIMUM_POWER)` |
| `HIGH_PERFORMANCE[0]` (record) | NVIDIA-class adapter (discrete, dilewati — benar) |
| `D3D11CreateDevice(iGPU, BGRA\|VIDEO, 11_1→10_0)` | OK feature_level=0xb100 (11.1) |
| Heuristik prompt lama (`Shared>Dedicated`=integrated) | TERBUKTI SALAH di rig ini (RTX 2050 ikut integrated) — dibuang |
| Desain P3.6 shared-mode | Pemilihan adapter LANGSUNG saat `D3D11CreateDevice` (pola probe ini), BUKAN via property mpv `d3d11-adapter` (property hanya untuk jalur vo=gpu per-monitor) |

VendorId-fallback (0x10DE selalu discrete; 0x8086 integrated kecuali
"Arc"; 0x1002 threshold 512MB; skip `DXGI_ADAPTER_FLAG_SOFTWARE`) ada di
file sebagai kode, tidak tereksekusi di rig ini (primary path sukses).

## B2 (ANGLE EGL) — analisis deploy, dilaporkan SEBELUM coding (sesuai spec)

Status di rig: **tidak ada ANGLE di mana pun**.
- Qt 6.8.3 bin: hanya `opengl32sw.dll` (Mesa software GL), tanpa
`libEGL.dll`/`libGLESv2.dll` (Qt6 di Windows pakai native D3D11/GL).
- System32: tidak ada EGL. `vendor/libmpv/bin/`: hanya `libmpv-2.dll`.
- Build mpv sendiri: feature `egl-angle`, `egl-angle-win32`,
`gl-dxinterop`, `d3d11-egl` ADA (binary strings) — mpv siap pakai ANGLE,
tapi DLL-nya tidak dibundel.

Implikasi deploy B2:
1. **Sumber DLL**: (a) Chrome/Edge install dir — versi ikut browser,
moving target, SHA-pin rapuh; (b) build ANGLE dari source Chromium
(depot_tools, jam-an, pin via chromium tag — reproducible tapi berat);
(c) prebuilt pihak ketiga (trust + pin manual).
2. **Pin SHA256**: repo TIDAK PUNYA mekanisme pin (grep `packaging/*.ps1`
nol hit). Butuh infra baru: `vendor/angle/` + hashes + langkah verify di
build/packaging + update policy (ANGLE security updates).
3. **Ukuran**: +~2 DLL belasan MB ke installer/portable ZIP.
4. **Teknis (sketsa, belum dibuktikan)**: EGL display via
`eglGetPlatformDisplayEXT(EGL_PLATFORM_ANGLE + EGL_D3D11_DEVICE_ANGLE =
ID3D11Device iGPU kita)` → hwdec + render di iGPU (P3.6 selesai di level
device); share via `eglCreateContext(share_context)` — pola sama seperti
spike GL; present via `eglCreateWindowSurface` + `eglSwapInterval(1)` per
window. Estimasi 2-3 hari spike (detail ABI EGL/ANGLE) + risiko baru
(surface-init failure = fallback per-monitor tetap wajib).

## Opsi keputusan (menunggu otorisasi, JANGAN Sesi B dulu)

- **O1 — B2 ANGLE spike**: setujui sumber DLL + infra SHA-pin; otorisasi
spike `render_shared_angle.cpp` (EGL share + hwdec gate yang sama).
- **O2 — Upgrade vendored libmpv** ke build yang memuat backend render
D3D11 (bila ada upstream/build yang menyalakannya): vendor swap +
re-validasi total (hwdec chain, vo=gpu-next, semua --simulate-*).
Risiko terbesar, utred dulu eksistensinya sebelum sentuh vendor/.
- **O3 — Descope shared-decode**: Phase 3 fan-out dibatalkan; target
RAM/CPU dikejar via jalur per-monitor (lean profile + affinity E-core +
pin iGPU di vo=gpu, yang TERBUKTI hwdec aktif). Paling murah, jujur
terhadap bukti A+A2.

## Repro

- `build\spikes\render_shared_d3d11.exe --probe build\spikes\test_1080p.mp4`
→ exit 1, `[a2-probe] create(api-only) rc=-19 (operation not implemented)`.

---

# P3L.0 — vendor D3D11 render API investigation (read-only, 2026-09-22)

**Verdict: blocked on upstream/builder.** NON-GATING for P3L.1–P3L.4
(plan addendum C): investigation closed timeboxed, execution below did
not wait on it.

- Upstream `MPV_RENDER_API_TYPE_D3D11` is **not merged** — open PR
  mpv-player/mpv#17764 (kasper93, commit `87316d3`, milestone v0.42.0):
  adds `include/mpv/render_d3d11.h`, API 2.5→2.6,
  `video/out/d3d11/libmpv_d3d11.c`, gated on the existing `-Dd3d11`
  meson feature (`HAVE_D3D11`, no new option). Master headers/API stay
  v2.5 (`opengl`/`sw` only; `render_d3d11.h` → 404), latest stable
  v0.41.0 — consistent with our `-19 NOT_IMPLEMENTED` probe against
  vendored shinchiro `20260903`.
- Newest trusted builds track unpatched master: shinchiro `20260921`
  (`mpv-dev-x86_64-20260921-git-e76a35ec95.7z`) and zhongfly
  `2026-09-21-e76a35ec95` (mpv commit `e76a35e`) — **no public mpv-dev
  package contains the backend** (zhongfly ships no libmpv at all).
- Vendor-swap plan stays parked for 1.2.0: unblocks when #17764 lands
  and shinchiro publishes a post-merge nightly, or K6WP self-builds
  master+`87316d3` with `-Dlibmpv=true -Dd3d11=enabled` (precedent:
  open-ani/mediamp#37; note the in-PR ResizeBuffers texture-lifetime
  caveat).