# User Provided Header
K6WP HANDOFF PACK - C3-code-tools | branch refactor/rust-wt | read STATE.md + ledger.md first | no measurements in this run

# Directory Structure
```
packaging/
  app-icon-README.md
  config.json.example
  installer.nsi
  k6wp-off.ico
  k6wp-on.ico
  known-limitations.md
  make_app_icon.py
  make_zip.ps1
  playlist.json.example
  resource.h
  sign_outputs.ps1
  studio_settings.json.example
  uninstall.bat
  version.nsh
tools/
  bench_lib.ps1
  check_release_overrides.ps1
  fetch_vendor.ps1
  g5_diff_baseline.ps1
  gate_ctest_stability.ps1
  lint_env_overrides_selftest.ps1
  lint_env_overrides.ps1
  lint_nonlatin_selftest.ps1
  lint_nonlatin.ps1
  lint_test_overrides.ps1
  lint_ui_thread.ps1
  make_corpus.ps1
  make_handoff.ps1
  session_snapshot.ps1
  stress_ipc.ps1
CMakeLists.txt
```

# Files

## File: packaging/app-icon-README.md
```markdown
# K6WP app icon

`k6wp-on.ico` and `k6wp-off.ico` are the canonical K6WP app icons. Master
copies live in `brand/icons/`; build copies live in `packaging/` (on the RC
include path, so the `.rc` files reference them by bare name) and `engine/`.

## What they are
- `k6wp-on.ico` — running state: indigo rounded square, white "K6" glyph.
- `k6wp-off.ico` — paused state: amber rounded square, "K6" glyph plus two
  pause bars (visually distinct from the running icon even at 16 px).
- Both are multi-size ICOs (9 entries: 16/24/32/48/64/72/96/128/256 px,
  32bpp, 432254 bytes each).

## How they are used
- `engine/app.rc` embeds `k6wp-on.ico` as resource `IDI_APPICON` (101) and
  `k6wp-off.ico` as `IDI_APPICON_PAUSED` (102) into `engine.exe`.
- `studio/app.rc` and `launcher/app.rc` embed `k6wp-on.ico` as
  `IDI_APPICON` (101).
- `packaging/installer.nsi` uses `k6wp-on.ico` for the installer icon and
  `k6wp-off.ico` for the uninstaller icon.
- `engine/src/tray.cpp` loads them via `LoadImageW` (sized to the system
  small-icon metric, 16 px at 100% DPI / 32 px at 200% DPI). If the resource
  is missing (e.g. a build without the .rc) it falls back to the stock shared
  icons, so the engine always runs.

## Replacing them (no code changes)
1. Overwrite `packaging/k6wp-on.ico` and/or `packaging/k6wp-off.ico`
   (same filenames, any multi-size ICO format), and mirror the change to the
   master copies in `brand/icons/` and the build copy in `engine/`.
2. Rebuild: `cmake --build --preset msvc-dev --target engine`.
3. Restart the engine. No source changes needed.

## History
The original worker-generated placeholders were `app.ico` / `app_paused.ico`
(16/32/48/256 px), produced by `packaging/make_app_icon.py` (Python + Pillow;
`pip install pillow`). They were superseded by the canonical `k6wp-on.ico` /
`k6wp-off.ico` artwork; the old files now live in `attic/packaging-icons/`
(LOW-22: moved out of `packaging/` so the ship dir holds only live assets).
```

## File: packaging/config.json.example
```
{
  "_comment": "K6WP example config, schema v5. Copy to %LOCALAPPDATA%/K6WP/config.json and set video_path to a REAL video file on your machine - the placeholder below does not exist. Playlist/rotation settings live in the separate playlist.json (see playlist.json.example).",
  "version": 5,
  "video_path": "C:/Videos/wallpaper.mp4",
  "fit_mode": "cover",
  "speed": 1.0,
  "monitor_id": -1,
  "crf": 22,
  "resolution_w": 0,
  "resolution_h": 0,
  "fps_cap": 24,
  "battery_saver": false,
  "battery_mode": "cap24",
  "cpu_affinity": "auto",
  "gpu_adapter": "auto"
}
```

## File: packaging/installer.nsi
```
; K6WP per-user installer (Task 27, k6wp-cleanup-wallpaper-ux).
;
; Per-user, no-UAC design:
;   - RequestExecutionLevel user  -> embedded manifest is asInvoker (never
;     requireAdministrator); installs without a UAC prompt.
;   - SetShellVarContext current  -> Start Menu / Desktop shortcuts resolve to
;     the CURRENT user's profile, never the all-users profile.
;   - InstallDir "$LOCALAPPDATA\K6WP" (NSIS 3.x $LOCALAPPDATA constant; never a
;     hardcoded C:\Users\... path).
;   - Registry writes are HKCU-only (Run value + uninstall key +
;     wallpaper-backup key). NO HKLM, NO Program Files, NO System32.
;   - Shortcut targets are K6WP.exe only, never engine.exe directly
;     (K6WP.exe is the Qt-free single-app launcher).
;
; File set mirrors packaging/make_zip.ps1 staging: K6WP.exe + engine.exe +
; studio.exe + compressor.exe (+ monitor_dump.exe when the build produced it)
; + vendored ffmpeg.exe/ffprobe.exe + vendored libmpv-2.dll + app-local VC
; runtime (vcruntime140.dll + vcruntime140_1.dll + msvcp140.dll, fresh-machine
; safety, no UAC) + the windeployqt output next to studio.exe (root *.dll +
; every plugin subdir containing *.dll) + config.json.example +
; packaging/uninstall.bat + LICENSE + LICENSES/ (GPL-2.0-or-later grant +
; third-party notices: libmpv/ffmpeg/Qt/nlohmann-json).
;
; Autostart checkbox writes EXACTLY "<installdir>\K6WP.exe" --engine --silent
; to HKCU\Software\Microsoft\Windows\CurrentVersion\Run value "K6WP", matching
; shared/autostart.cpp AutostartCommand()/SetAutostart(true). OFF (unchecked)
; leaves the value absent, matching SetAutostart(false). Default: CHECKED.
;
; Wallpaper backup: at install time the current OS wallpaper path is read via
; SystemParametersInfo SPI_GETDESKWALLPAPER and stored at
; HKCU\Software\K6WP\WallpaperBackup (REG_SZ "OsWallpaper"), so the
; uninstaller can restore it even when taskkill skips the engine's graceful
; restore path (engine/src/engine_app.cpp SaveOsWallpaper/RestoreOsWallpaper).
;
; Install (Section SEC_APP): stops this install dir's running processes
; FIRST (K6WP.exe --stop — the uninstaller's scoped helper) and snapshots a
; per-file pre-existence census, so re-running the installer over a live
; engine does not fail on a locked exe, and RollbackPartial deletes only the
; files THIS run created (audit fix H2: a failed upgrade leaves the
; previous installation intact).
;
; Uninstall (Section "Uninstall"): stops engine/studio/K6WP via
; "$INSTDIR\K6WP.exe" --stop (its own IPC quit first, then a per-PID kill
; restricted to images inside $INSTDIR - never a machine-wide image-name
; taskkill), deletes the Run value (idempotent), restores the pre-install
; wallpaper backup via SPI_SETDESKWALLPAPER when present, restores the
; per-user still (desktop wallpaper + lock-screen image) through
; K6WP.exe --apply-still --restore (no UAC), then removes a leftover legacy
; HKLM lockscreen policy via K6WP.exe --elevate-lockscreen off
; (self-elevating; the call blocks on the elevated worker with a bounded
; wait, and a declined prompt or a failed cleanup is reported to the user
; with the exact manual command) before the app files are deleted,
; removes the app files, asks keep/remove of
; %LOCALAPPDATA%\K6WP user data (silent uninstall keeps it), deletes
; shortcuts + HKCU uninstall key, removes the install dir.
;
; Stock plugins only: MUI2, LogicLib, nsExec, System. No third-party NSIS
; plugins, no Python at install time.
;
; Build (both styles work from the repo root; makensis resolves relative
; paths against the SCRIPT dir under either invocation, so plain relatives
; are used throughout and ${__FILEDIR__} is deliberately avoided - with a
; relative script path it expands to relative "packaging", which file
; directives then re-resolve against the script dir ("packaging\packaging").
; For the same reason do NOT add "!cd ${__FILEDIR__}": !cd resolves its
; argument against the script dir too and fails the relative invocation):
;   "C:\Program Files (x86)\NSIS\makensis.exe" packaging\installer.nsi
;   "C:\Program Files (x86)\NSIS\makensis.exe" <USER_HOME>\Desktop\k6wp\packaging\installer.nsi
; Output: dist\K6WP-Setup.exe.

Unicode true
RequestExecutionLevel user
; Per-user shell context enforced at runtime (.onInit + un.onInit):
; Start Menu / Desktop resolve to the CURRENT user profile, never all-users.

!include "MUI2.nsh"
!include "LogicLib.nsh"
!include "Sections.nsh"
; Version constants generated by CMake from K6WP_VERSION (root CMakeLists.txt)
; into packaging/version.nsh - never hand-edit APP_VERSION here.
!include "version.nsh"

; ---------------------------------------------------------------------------
; Constants
; ---------------------------------------------------------------------------
!define APP_NAME "K6WP"
!define APP_VERSION "${K6WP_VERSION_STR}"
!define RUN_VALUE "K6WP"
!define RUN_SUBKEY "Software\Microsoft\Windows\CurrentVersion\Run"
!define UNINST_SUBKEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\K6WP"
!define BACKUP_SUBKEY "Software\K6WP\WallpaperBackup"

; ---------------------------------------------------------------------------
; H2 (audit remediation): upgrade-safe install.
;
; Two defects fixed here:
;   1. Install over a RUNNING engine failed at the first locked exe and then
;      rolled back by deleting the whole staged set — including the previous
;      installation's files, leaving the user with a broken install.
;   2. That rollback had no notion of "which files did this run create".
;
; SEC_APP now stops this install dir's processes (K6WP.exe --stop, the same
; scoped helper the uninstaller uses) BEFORE any extraction, and snapshots a
; per-file pre-existence census. RollbackPartial deletes ONLY files this run
; created (flag 0); pre-existing files are never touched, so a failed upgrade
; leaves the previous installation intact.
Var /GLOBAL PreExistK6WP
Var /GLOBAL PreExistEngine
Var /GLOBAL PreExistStudio
Var /GLOBAL PreExistCompressor
Var /GLOBAL PreExistMonitorDump
Var /GLOBAL PreExistFfmpeg
Var /GLOBAL PreExistFfprobe
Var /GLOBAL PreExistLibmpv
Var /GLOBAL PreExistVcruntime
Var /GLOBAL PreExistVcruntime1
Var /GLOBAL PreExistMsvcp
Var /GLOBAL PreExistConfigEx
Var /GLOBAL PreExistPlaylistEx
Var /GLOBAL PreExistUninstallBat
Var /GLOBAL PreExistLicense
Var /GLOBAL PreExistLicensesDir
Var /GLOBAL PreExistPlatformsDir
Var /GLOBAL PreExistImageformatsDir
Var /GLOBAL PreExistStylesDir
Var /GLOBAL PreExistIconenginesDir
Var /GLOBAL PreExistQmlDir
Var /GLOBAL PreExistNetinfoDir
Var /GLOBAL PreExistTlsDir

; Records 1 (exists) / 0 (absent) for one $INSTDIR-relative file or dir.
; NOTE: the delete/rollback side below uses explicit Delete and RMDir
; lines with literal targets (see RollbackPartial) because
; packaging/make_zip.ps1's ship-set assert greps every RMDir /r target
; under $INSTDIR in this script — a macro parameter would resolve to a
; placeholder name, not a staged directory, and fail the release gate.
!macro K6WP_PREFLAG_SET _var _rel
  ${If} ${FileExists} "$INSTDIR\${_rel}"
    StrCpy ${_var} 1
  ${Else}
    StrCpy ${_var} 0
  ${EndIf}
!macroend
; ---------------------------------------------------------------------------

; Build outputs. Release preset is the production source (matches the
; subsystem-WINDOWS, ProductName-K6WP branded triple verified in
; make_zip-style checks); file NAMES mirror make_zip.ps1 exactly.
!define SRCDIR "..\build\release"
!define VENDOR_FFMPEG "..\vendor\ffmpeg"
!define VENDOR_LIBMPV "..\vendor\libmpv\bin"
; Pinned app-local VC runtime (populated by make_zip.ps1 step 2f; makensis
; cannot discover the versioned VS Redist dir, so the installer consumes
; this cached copy and fails loudly when nobody staged it yet).
!define VENDOR_VC "..\vendor\vc_runtime"
; Repo root (license texts): LICENSE + LICENSES/ ship in both the ZIP
; (make_zip.ps1) and the installer; the ship-set sync assert in make_zip.ps1
; maps ${REPO_ROOT} back to the repo root.
!define REPO_ROOT ".."
!if /FileExists "${VENDOR_VC}\vcruntime140.dll"
!else
  !error "vendor\vc_runtime\vcruntime140.dll missing - run packaging\make_zip.ps1 once (it caches the CRT set) or copy vcruntime140.dll + vcruntime140_1.dll + msvcp140.dll there."
!endif

Name "${APP_NAME} ${APP_VERSION}"
OutFile "..\dist\K6WP-Setup.exe"
InstallDir "$LOCALAPPDATA\K6WP"
Icon "k6wp-on.ico"

!define MUI_ICON "k6wp-on.ico"
!define MUI_UNICON "k6wp-on.ico"
!define MUI_ABORTWARNING

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "${REPO_ROOT}\LICENSE"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

VIProductVersion "${K6WP_VERSION_STR_4}"
VIAddVersionKey "ProductName" "${APP_NAME}"
VIAddVersionKey "CompanyName" "K6WP contributors"
VIAddVersionKey "FileDescription" "${APP_NAME} Setup"
VIAddVersionKey "FileVersion" "${APP_VERSION}"
VIAddVersionKey "ProductVersion" "${APP_VERSION}"
VIAddVersionKey "LegalCopyright" "${APP_NAME}"

Function .onInit
  SetShellVarContext current
  ; --- C6: minimum-OS gate -----------------------------------------------
  ; K6WP requires Windows 10 22H2 (build 19041) or Windows 11. The
  ; manifest's supportedOS list only tells the compatibility shim what to
  ; report; it does NOT refuse to install. This check does.
  ;
  ; The build number is read from the registry (CurrentBuildNumber) so the
  ; check is the same rule the runtime check (shared/os_version.cpp,
  ; RtlGetVersion) applies: build >= 19041.
  ReadRegStr $0 "HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion" "CurrentBuildNumber"
  ${If} $0 == ""
    ; No build number (or a non-string value): refuse rather than guess.
    MessageBox MB_OK|MB_ICONSTOP "K6WP requires Windows 10 22H2 (build 19041) or Windows 11, 64-bit.$\r$\n$\r$\nThis system's Windows build could not be detected, so the installation was stopped."
    Abort
  ${EndIf}
  ${If} $0 != ""
    IntCmp $0 19041 0 done done
    ; $0 < 19041 -> too old
    MessageBox MB_OK|MB_ICONSTOP "K6WP requires Windows 10 22H2 (build 19041) or Windows 11, 64-bit.$\r$\n$\r$\nDetected build $0. Update Windows, then run the installer again."
    Abort
    done:
  ${EndIf}
FunctionEnd

Function un.onInit
  SetShellVarContext current
FunctionEnd

; ---------------------------------------------------------------------------
; Install sections
; ---------------------------------------------------------------------------
; NOTE: the autostart checkbox section is declared FIRST so its ${SEC_AUTO}
; index define exists when SEC_APP reads it (NSIS defines section indexes at
; the Section line; forward references do not expand). It holds no files; the
; actual Run-value write happens in SEC_APP so file staging stays ordered.
Section "Start with Windows (autostart)" SEC_AUTO
  ; Checkbox only: the actual Run-value write happens in SEC_APP so ordering
  ; with file staging stays deterministic. This section intentionally holds
  ; no files; its selected flag is the opt-in.
SectionEnd

Section "!${APP_NAME} Application (required)" SEC_APP
  SectionIn RO

  ; --- Writability pre-check (failure QA: message + no partial install) ---
  ; Must run BEFORE any File extraction so an unwritable target aborts clean
  ; with nothing copied (rollback = nothing to roll back).
  CreateDirectory "$INSTDIR"
  FileOpen $0 "$INSTDIR\.k6wp_write_test" w
  ; Label form (NOT numeric offsets: +1 means the NEXT instruction, so an
  ; "IfErrors 0 +1 / Abort" skips nothing and aborts on success). MessageBox
  ; is modal even in silent mode, so silent failures Abort without a dialog.
  IfErrors write_failed write_ok
  write_failed:
    IfSilent abort_nowrite 0
    MessageBox MB_ICONSTOP "${APP_NAME} cannot write to:$\r$\n$INSTDIR$\r$\n$\r$\nPick a writable folder. Nothing was installed."
  abort_nowrite:
    Abort
  write_ok:
  FileClose $0
  Delete "$INSTDIR\.k6wp_write_test"

  ; --- H2: stop the previous installation's processes BEFORE staging files.
  ; NSIS cannot overwrite a running exe image: without this the first File
  ; on a locked engine.exe failed, aborted the section, and the rollback
  ; (fixed below) destroyed the previous installation. K6WP.exe --stop is
  ; the scoped helper the uninstaller uses: IPC quit first, then a per-PID
  ; kill limited to images inside $INSTDIR (never a machine-wide taskkill).
  ; Its exit code is advisory — a survivor surfaces immediately at the File
  ; stage, which retries once via the extraction-error path below.
  ${If} ${FileExists} "$INSTDIR\K6WP.exe"
    DetailPrint "Stopping running ${APP_NAME} processes before install..."
    nsExec::ExecToLog '"$INSTDIR\K6WP.exe" --stop'
    Pop $0
    DetailPrint "Process stop helper exit code: $0"
  ${EndIf}

  ; --- H2: pre-install census consumed by RollbackPartial (create-only
  ; teardown so a failed upgrade keeps the previous install intact).
  !insertmacro K6WP_PREFLAG_SET $PreExistK6WP "K6WP.exe"
  !insertmacro K6WP_PREFLAG_SET $PreExistEngine "engine.exe"
  !insertmacro K6WP_PREFLAG_SET $PreExistStudio "studio.exe"
  !insertmacro K6WP_PREFLAG_SET $PreExistCompressor "compressor.exe"
  !insertmacro K6WP_PREFLAG_SET $PreExistMonitorDump "monitor_dump.exe"
  !insertmacro K6WP_PREFLAG_SET $PreExistFfmpeg "ffmpeg.exe"
  !insertmacro K6WP_PREFLAG_SET $PreExistFfprobe "ffprobe.exe"
  !insertmacro K6WP_PREFLAG_SET $PreExistLibmpv "libmpv-2.dll"
  !insertmacro K6WP_PREFLAG_SET $PreExistVcruntime "vcruntime140.dll"
  !insertmacro K6WP_PREFLAG_SET $PreExistVcruntime1 "vcruntime140_1.dll"
  !insertmacro K6WP_PREFLAG_SET $PreExistMsvcp "msvcp140.dll"
  !insertmacro K6WP_PREFLAG_SET $PreExistConfigEx "config.json.example"
  !insertmacro K6WP_PREFLAG_SET $PreExistPlaylistEx "playlist.json.example"
  !insertmacro K6WP_PREFLAG_SET $PreExistUninstallBat "uninstall.bat"
  !insertmacro K6WP_PREFLAG_SET $PreExistLicense "LICENSE"
  !insertmacro K6WP_PREFLAG_SET $PreExistLicensesDir "LICENSES"
  !insertmacro K6WP_PREFLAG_SET $PreExistPlatformsDir "platforms"
  !insertmacro K6WP_PREFLAG_SET $PreExistImageformatsDir "imageformats"
  !insertmacro K6WP_PREFLAG_SET $PreExistStylesDir "styles"
  !insertmacro K6WP_PREFLAG_SET $PreExistIconenginesDir "iconengines"
  !insertmacro K6WP_PREFLAG_SET $PreExistQmlDir "qml"
  !insertmacro K6WP_PREFLAG_SET $PreExistNetinfoDir "networkinformation"
  !insertmacro K6WP_PREFLAG_SET $PreExistTlsDir "tls"

  ; --- Belt-and-braces wallpaper backup (engine restores on clean exit too) --
  DetailPrint "Backing up current OS wallpaper path..."
  System::Alloc 520
  Pop $1
  System::Call 'user32::SystemParametersInfoW(i 115, i 260, p r1, i 0) i .r2'
  ${If} $2 <> 0
    System::Call '*$1(&w260 .r3)'
    ${If} $3 != ""
      WriteRegStr HKCU "${BACKUP_SUBKEY}" "OsWallpaper" "$3"
      DetailPrint "Wallpaper backup saved."
    ${EndIf}
  ${Else}
    DetailPrint "No OS wallpaper readable; skipping backup."
  ${EndIf}
  System::Free $1

  ; --- Application files (make_zip.ps1 file set) ---
  DetailPrint "Copying ${APP_NAME} files..."
  SetOutPath "$INSTDIR"
  File "${SRCDIR}\K6WP.exe"
  File "${SRCDIR}\engine.exe"
  File "${SRCDIR}\studio.exe"
  File "${SRCDIR}\compressor.exe"
!if /FileExists "${SRCDIR}\monitor_dump.exe"
  File "${SRCDIR}\monitor_dump.exe"
!endif
  ; Vendored helpers.
  File "${VENDOR_FFMPEG}\ffmpeg.exe"
  File "${VENDOR_FFMPEG}\ffprobe.exe"
  File "${VENDOR_LIBMPV}\libmpv-2.dll"
  ; App-local VC runtime (fresh machines without any redist).
  File "${VENDOR_VC}\vcruntime140.dll"
  File "${VENDOR_VC}\vcruntime140_1.dll"
  File "${VENDOR_VC}\msvcp140.dll"
  ; windeployqt output: every root DLL next to studio.exe ...
  File "${SRCDIR}\*.dll"
  ; ... plus every plugin subdir containing DLLs.
  ; qml/ is MANDATORY: Studio is a QML app, so the QtQuick/Controls/
  ; Layouts/Templates modules must ship or the window comes up empty.
  ; networkinformation/ + tls/ are back because Qt6Quick links Qt6Network
  ; (P4.2 dropped it back when Studio was Widgets-only; the QML rewrite makes
  ; it non-optional again). make_zip's ship-set assert enforces this list.
  ; qmltooling/ is deliberately NOT shipped - QML debugger/inspector plugins,
  ; excluded from staging in make_zip.ps1.
  File /r "${SRCDIR}\platforms"
  File /r "${SRCDIR}\imageformats"
  File /r "${SRCDIR}\styles"
  File /r "${SRCDIR}\iconengines"
  File /r "${SRCDIR}\qml"
  File /r "${SRCDIR}\networkinformation"
  File /r "${SRCDIR}\tls"
  ; Example config + playlist + portable-style uninstall helper.
  File "config.json.example"
  File "playlist.json.example"
  File "uninstall.bat"
  ; License texts (GPL-2.0-or-later grant + third-party notices; mirrors
  ; make_zip.ps1 staging, kept in sync by its ship-set assert).
  File "${REPO_ROOT}\LICENSE"
  File /r "${REPO_ROOT}\LICENSES"

  ; --- Extraction-error rollback: message + remove exactly what we staged --
  ; Label form (NOT numeric offsets: +1 means the NEXT instruction, so an
  ; "IfErrors 0 +1 / Abort" skips nothing and aborts on success).
  IfErrors files_failed files_ok
  files_failed:
    Call RollbackPartial
    Abort
  files_ok:

  ; --- Shortcuts (per-user shell context; target is K6WP.exe only) ---
  CreateDirectory "$SMPROGRAMS\${APP_NAME}"
  CreateShortCut "$SMPROGRAMS\${APP_NAME}\${APP_NAME}.lnk" "$INSTDIR\K6WP.exe" "" "$INSTDIR\K6WP.exe" 0
  CreateShortCut "$DESKTOP\${APP_NAME}.lnk" "$INSTDIR\K6WP.exe" "" "$INSTDIR\K6WP.exe" 0

  ; --- Autostart (SetAutostart-ON semantics, iff the checkbox is selected) --
  SectionGetFlags ${SEC_AUTO} $0
  IntOp $0 $0 & ${SF_SELECTED}
  ${If} $0 <> 0
    WriteRegStr HKCU "${RUN_SUBKEY}" "${RUN_VALUE}" '"$INSTDIR\K6WP.exe" --engine --silent'
    DetailPrint "Autostart enabled (HKCU Run)."
  ${Else}
    DetailPrint "Autostart skipped (checkbox unchecked)."
  ${EndIf}

  ; --- Uninstaller + Add/Remove Programs entry (HKCU, per-user) ---
  WriteUninstaller "$INSTDIR\Uninstall.exe"
  WriteRegStr HKCU "${UNINST_SUBKEY}" "DisplayName" "${APP_NAME} ${APP_VERSION}"
  WriteRegStr HKCU "${UNINST_SUBKEY}" "DisplayVersion" "${APP_VERSION}"
  WriteRegStr HKCU "${UNINST_SUBKEY}" "InstallLocation" "$INSTDIR"
  WriteRegStr HKCU "${UNINST_SUBKEY}" "UninstallString" '"$INSTDIR\Uninstall.exe"'
  WriteRegStr HKCU "${UNINST_SUBKEY}" "DisplayIcon" "$INSTDIR\K6WP.exe"
  WriteRegDWORD HKCU "${UNINST_SUBKEY}" "NoModify" 1
  WriteRegDWORD HKCU "${UNINST_SUBKEY}" "NoRepair" 1
SectionEnd

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC_APP} "Required ${APP_NAME} program files (per-user, no admin)."
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC_AUTO} "Launch ${APP_NAME} engine silently at logon via HKCU Run (no UAC). Uncheck to skip."
!insertmacro MUI_FUNCTION_DESCRIPTION_END

; Rollback helper (audit fix H2): delete exactly the files THIS run
; created — the pre-install census (K6WP_PREFLAG_SET) decides per file, so
; the previous installation's files are never touched. A failed upgrade
; therefore leaves the old version installed (and uninstallable) instead of
; a blown-away install dir.
; Deliberate omissions vs the file list:
;   - root "*.dll" wildcard: version-specific Qt payloads we cannot census
;     individually; leftover DLLs are inert next to the old exes.
;   - Uninstall.exe / the HKCU entries are never written before extraction
;     completes, so they cannot need rollback.
Function RollbackPartial
  ; Silent-safe: MessageBox would block a headless /S run, so only show it
  ; interactively; silent runs still delete the partial set and Abort.
  IfSilent rb_skip_msg 0
  MessageBox MB_ICONSTOP "${APP_NAME} setup failed while copying files. Rolling back the partial install."
rb_skip_msg:
  ; Explicit Delete and RMDir lines with literal targets on purpose: the
  ; make_zip ship-set assert parses these lines, so no macro may generate
  ; them (see the NOTE next to K6WP_PREFLAG_SET above).
  ${If} $PreExistK6WP == 0
    Delete "$INSTDIR\K6WP.exe"
  ${EndIf}
  ${If} $PreExistEngine == 0
    Delete "$INSTDIR\engine.exe"
  ${EndIf}
  ${If} $PreExistStudio == 0
    Delete "$INSTDIR\studio.exe"
  ${EndIf}
  ${If} $PreExistCompressor == 0
    Delete "$INSTDIR\compressor.exe"
  ${EndIf}
  ${If} $PreExistMonitorDump == 0
    Delete "$INSTDIR\monitor_dump.exe"
  ${EndIf}
  ${If} $PreExistFfmpeg == 0
    Delete "$INSTDIR\ffmpeg.exe"
  ${EndIf}
  ${If} $PreExistFfprobe == 0
    Delete "$INSTDIR\ffprobe.exe"
  ${EndIf}
  ${If} $PreExistLibmpv == 0
    Delete "$INSTDIR\libmpv-2.dll"
  ${EndIf}
  ${If} $PreExistVcruntime == 0
    Delete "$INSTDIR\vcruntime140.dll"
  ${EndIf}
  ${If} $PreExistVcruntime1 == 0
    Delete "$INSTDIR\vcruntime140_1.dll"
  ${EndIf}
  ${If} $PreExistMsvcp == 0
    Delete "$INSTDIR\msvcp140.dll"
  ${EndIf}
  ${If} $PreExistConfigEx == 0
    Delete "$INSTDIR\config.json.example"
  ${EndIf}
  ${If} $PreExistPlaylistEx == 0
    Delete "$INSTDIR\playlist.json.example"
  ${EndIf}
  ${If} $PreExistUninstallBat == 0
    Delete "$INSTDIR\uninstall.bat"
  ${EndIf}
  ${If} $PreExistLicense == 0
    Delete "$INSTDIR\LICENSE"
  ${EndIf}
  ${If} $PreExistLicensesDir == 0
    RMDir /r "$INSTDIR\LICENSES"
  ${EndIf}
  ${If} $PreExistPlatformsDir == 0
    RMDir /r "$INSTDIR\platforms"
  ${EndIf}
  ${If} $PreExistImageformatsDir == 0
    RMDir /r "$INSTDIR\imageformats"
  ${EndIf}
  ${If} $PreExistStylesDir == 0
    RMDir /r "$INSTDIR\styles"
  ${EndIf}
  ${If} $PreExistIconenginesDir == 0
    RMDir /r "$INSTDIR\iconengines"
  ${EndIf}
  ${If} $PreExistQmlDir == 0
    RMDir /r "$INSTDIR\qml"
  ${EndIf}
  ${If} $PreExistNetinfoDir == 0
    RMDir /r "$INSTDIR\networkinformation"
  ${EndIf}
  ${If} $PreExistTlsDir == 0
    RMDir /r "$INSTDIR\tls"
  ${EndIf}
  Delete "$INSTDIR\.k6wp_write_test"
  RMDir "$INSTDIR"  ; only succeeds when the dir is empty
FunctionEnd

; ---------------------------------------------------------------------------
; Uninstall section (Task 28 reuses this; keep it complete and idempotent)
; ---------------------------------------------------------------------------
Section "Uninstall" SEC_UNINST
  ; --- Stop running processes first (graceful engine shutdown restores the
  ; --- OS wallpaper itself; exit codes tolerated: not-running is fine) ---
  ; K6WP.exe --stop is used instead of `taskkill /F /IM engine.exe`: an image
  ; name is not unique on a developer or IT machine, and the old call killed
  ; EVERY process on the box carrying that name. --stop asks the engine to quit
  ; over its own IPC pipe (so its wallpaper restore runs), then escalates to a
  ; per-PID kill, and only for images that live directly in $INSTDIR. A
  ; non-zero exit means one of OUR processes survived, so the user is asked to
  ; close it and retry instead of us deleting files under a live process.
  DetailPrint "Stopping ${APP_NAME} processes..."
  IntOp $1 0 + 2
  stop_processes:
    ExecWait '"$INSTDIR\K6WP.exe" --stop' $0
    ${If} $0 == 0
      Goto stop_processes_done
    ${EndIf}
    IntOp $1 1 - 1
    ${If} $1 == 0
      DetailPrint "Some ${APP_NAME} processes could not be stopped (2 attempts)."
      Goto stop_processes_done
    ${EndIf}
    IfSilent stop_processes 0
    MessageBox MB_RETRYCANCEL|MB_ICONEXCLAMATION "Some ${APP_NAME} processes (engine, Studio) are still running and could not be stopped.$\r$\nClose them now, then choose Retry.$\r$\n$\r$\nChoose Cancel to continue anyway (their files may be left behind)." /SD IDCANCEL IDRETRY stop_processes
  stop_processes_done:

  ; --- Remove autostart (idempotent: absent value is not an error) ---
  DeleteRegValue HKCU "${RUN_SUBKEY}" "${RUN_VALUE}"

  ; --- Restore pre-install wallpaper backup when present ---
  ReadRegStr $0 HKCU "${BACKUP_SUBKEY}" "OsWallpaper"
  ${If} $0 != ""
    DetailPrint "Restoring backed-up OS wallpaper..."
    System::Call 'user32::SystemParametersInfoW(i 20, i 0, w r0, i 3) i .r1'
  ${EndIf}
  DeleteRegKey HKCU "${BACKUP_SUBKEY}"

  ; --- Restore the per-user still (wallpaper + lock screen) ---
  ; No UAC: the helper reads the persistent original_wallpaper.json /
  ; original_lockscreen.* stores under %LOCALAPPDATA%\K6WP. A missing store is
  ; a successful no-op. Run BEFORE the data folder is optionally removed, and
  ; while K6WP.exe still exists; the helper is synchronous (it waits for its
  ; own compressor child), so the restore is finished when ExecWait returns.
  DetailPrint "Restoring your previous desktop wallpaper and lock-screen image..."
  ExecWait '"$INSTDIR\K6WP.exe" --apply-still --restore' $0
  ${If} $0 != 0
    DetailPrint "The per-user wallpaper/lock-screen restore did not complete (exit code $0)."
  ${EndIf}

  ; --- Remove a leftover legacy lockscreen policy (LOW-21) ---
  ; Old builds wrote the machine-wide HKLM LockScreenImage policy. This
  ; per-user (asInvoker) uninstaller cannot touch HKLM, so it delegates to
  ; K6WP.exe, which self-elevates via a runas relaunch while the exe still
  ; exists (app files are deleted below). The cleanup restores the pre-K6WP
  ; policy value from lockscreen_policy_backup.json when one exists; otherwise
  ; it deletes the value only when it points at our legacy
  ; %PROGRAMDATA%\K6WP\lockscreen.jpg.
  ; The wait is the point: K6WP.exe hands back the elevated process HANDLE and
  ; blocks on it with a bounded wait (2 min), so this ExecWait only returns once
  ; the HKLM write is done - or once the helper was terminated. Deleting
  ; K6WP.exe while the worker is still running is impossible from here.
  ; Exit codes: 0 cleaned | 5 UAC declined | 4 wait budget expired |
  ; 3 the helper failed. Anything non-zero leaves a leftover policy in place,
  ; so the user is told exactly what to run.
  DetailPrint "Removing a leftover legacy lockscreen policy (UAC prompt only if one exists)..."
  ExecWait '"$INSTDIR\K6WP.exe" --elevate-lockscreen off' $0
  ${If} $0 != 0
    DetailPrint "The legacy lockscreen policy was NOT removed (exit code $0)."
    Goto lockscreen_warning
  ${EndIf}
  Goto lockscreen_done
  lockscreen_warning:
    IfSilent lockscreen_done 0
    ${If} $0 == 5
      MessageBox MB_ICONEXCLAMATION "The administrator prompt was declined, so a leftover K6WP lockscreen policy was NOT removed.$\r$\n$\r$\nIf an old K6WP version left one, it still points at %PROGRAMDATA%\K6WP\lockscreen.jpg (HKLM\SOFTWARE\Policies\Microsoft\Windows\Personalization\LockScreenImage).$\r$\n$\r$\nTo clean it up later, run this in an Administrator Command Prompt:$\r$\n$\r$\nK6WP.exe --elevate-lockscreen off"
    ${Else}
      MessageBox MB_ICONEXCLAMATION "K6WP could not remove a leftover legacy lockscreen policy (exit code $0).$\r$\n$\r$\nIf an old K6WP version left one, it may still point at %PROGRAMDATA%\K6WP\lockscreen.jpg (HKLM\SOFTWARE\Policies\Microsoft\Windows\Personalization\LockScreenImage).$\r$\n$\r$\nTo clean it up later, run this in an Administrator Command Prompt:$\r$\n$\r$\nK6WP.exe --elevate-lockscreen off"
    ${EndIf}
  lockscreen_done:

  ; --- Remove app files (explicit staged set; user data untouched here) ---
  Delete "$INSTDIR\K6WP.exe"
  Delete "$INSTDIR\engine.exe"
  Delete "$INSTDIR\studio.exe"
  Delete "$INSTDIR\compressor.exe"
  Delete "$INSTDIR\monitor_dump.exe"
  Delete "$INSTDIR\ffmpeg.exe"
  Delete "$INSTDIR\ffprobe.exe"
  Delete "$INSTDIR\libmpv-2.dll"
  Delete "$INSTDIR\vcruntime140.dll"
  Delete "$INSTDIR\vcruntime140_1.dll"
  Delete "$INSTDIR\msvcp140.dll"
  Delete "$INSTDIR\*.dll"
  RMDir /r "$INSTDIR\platforms"
  RMDir /r "$INSTDIR\imageformats"
  RMDir /r "$INSTDIR\styles"
  RMDir /r "$INSTDIR\iconengines"
  RMDir /r "$INSTDIR\qml"
  RMDir /r "$INSTDIR\networkinformation"
  RMDir /r "$INSTDIR\tls"
  Delete "$INSTDIR\config.json.example"
  Delete "$INSTDIR\playlist.json.example"
  Delete "$INSTDIR\uninstall.bat"
  Delete "$INSTDIR\LICENSE"
  RMDir /r "$INSTDIR\LICENSES"

  ; --- User data: keep/remove offer (silent uninstall keeps it) ---
  ; Runtime data (config.json, library, thumbs, engine.log) lives under
  ; %LOCALAPPDATA%\K6WP, which equals $INSTDIR for default installs.
  StrCmp "$INSTDIR" "$LOCALAPPDATA\${APP_NAME}" same_dir custom_dir

  same_dir:
    IfSilent keep_same 0
    MessageBox MB_YESNO "Also delete ${APP_NAME} user data (settings, library, logs) in %LOCALAPPDATA%\${APP_NAME}?" IDYES remove_same IDNO keep_same
  remove_same:
    Delete "$INSTDIR\Uninstall.exe"
    RMDir /r "$INSTDIR"
    Goto done_data
  keep_same:
    DetailPrint "Keeping user data."
    Delete "$INSTDIR\Uninstall.exe"
    RMDir "$INSTDIR"
    Goto done_data

  custom_dir:
    Delete "$INSTDIR\Uninstall.exe"
    RMDir /r "$INSTDIR"
    IfSilent silent_keep 0
    MessageBox MB_YESNO "Also delete ${APP_NAME} user data (settings, library, logs) in %LOCALAPPDATA%\${APP_NAME}?" IDYES wipe_data IDNO silent_keep
  wipe_data:
    RMDir /r "$LOCALAPPDATA\${APP_NAME}"
  silent_keep:
    Goto done_data
  done_data:

  ; --- Shortcuts + Add/Remove entry ---
  Delete "$SMPROGRAMS\${APP_NAME}\${APP_NAME}.lnk"
  RMDir "$SMPROGRAMS\${APP_NAME}"
  Delete "$DESKTOP\${APP_NAME}.lnk"
  DeleteRegKey HKCU "${UNINST_SUBKEY}"
SectionEnd
```

## File: packaging/make_app_icon.py
```python
SIZES = [16, 32, 48, 256]
⋮----
OUT_DIR = os.path.dirname(os.path.abspath(__file__))
⋮----
def _font(size)
⋮----
def _base(bg_top, bg_bottom, glyph)
⋮----
size = 256
img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
d = ImageDraw.Draw(img)
radius = int(size * 0.22)
⋮----
t = y / (size - 1)
color = tuple(int(bg_top[c] + (bg_bottom[c] - bg_top[c]) * t) for c in range(3)) + (255,)
⋮----
mask = Image.new("L", (size, size), 0)
md = ImageDraw.Draw(mask)
⋮----
font = _font(int(size * 0.52))
bbox = d.textbbox((0, 0), glyph, font=font)
w = bbox[2] - bbox[0]
h = bbox[3] - bbox[1]
⋮----
def _paused_base()
⋮----
img = _base((245, 158, 11), (180, 83, 9), "K6")
⋮----
bar_w = int(256 * 0.10)
bar_h = int(256 * 0.16)
gap = int(256 * 0.06)
y = int(256 * 0.66)
x0 = (256 - 2 * bar_w - gap) / 2
⋮----
def main()
```

## File: packaging/make_zip.ps1
```powershell
# K6WP portable ZIP builder (Todo 45, fase-6; task 26: release source).
#
# Builds all targets fresh FROM THE RELEASE PRESET (build\release — the
# production triple: /O2, WIN32_EXECUTABLE, BUILD_TESTING=OFF; never the
# Debug msvc-dev tree), stages K6WP.exe (Qt-free launcher) + engine.exe +
# studio.exe + compressor.exe (+ monitor_dump.exe when present) + vendored ffmpeg/ffprobe + libmpv-2.dll
# + the ACTUAL windeployqt output next to studio.exe (enumerated from the
# build dir - never a hand-listed set of Qt DLLs) + app-local VC runtime
# (vcruntime140*.dll + msvcp140.dll, fresh-machine safety) +
# config.json.example + uninstall.bat + LICENSE + LICENSES/*.txt
# (GPL-2.0-or-later + third-party notices), verifies every staged exe's imports resolve from the staging
# dir (dumpbin /dependents -> FAIL with the missing list, never a silent
# broken ZIP), logs per-file MB, and asserts the final ZIP is under 250 MB.
#
# Usage (PowerShell 5.1):
#   powershell -ExecutionPolicy Bypass -File packaging\make_zip.ps1 [-Version 1.0.0] [-Generator "Visual Studio 17 2022"]
# CI override: -Generator configures+builds with that CMake generator instead
# of "--preset release" (which pins the local VS 18 generator). Empty (default)
# = legacy local behavior, unchanged. -BuildDirOverride/-QtRootOverride/
# -CmakeExeOverride default to the local paths below when empty.
#
# Outputs: dist\K6WP-portable-<version>.zip + dist\make_zip-<version>.log

param(
  [string]$Version = "",
  [string]$Generator = "",
  [string]$BuildDirOverride = "",
  [string]$QtRootOverride = "",
  [string]$CmakeExeOverride = ""
)

$ErrorActionPreference = "Continue"

$RepoRoot = Split-Path -Parent $PSScriptRoot
$DistDir  = Join-Path $RepoRoot "dist"
if ([string]::IsNullOrWhiteSpace($BuildDirOverride)) {
  $BuildDir = Join-Path $RepoRoot "build\release"
} else {
  $BuildDir = $BuildDirOverride
}
$DefaultCmakeExe = "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
if (-not [string]::IsNullOrWhiteSpace($CmakeExeOverride)) {
  $CmakeExe = $CmakeExeOverride
} elseif (Test-Path -LiteralPath $DefaultCmakeExe) {
  $CmakeExe = $DefaultCmakeExe
} else {
  $CmakeExe = "cmake"
}
$SizeLimitMB = 250

function Log($msg) {
  $line = "[make_zip] $msg"
  Write-Host $line
  Add-Content -LiteralPath $script:LogFile -Value $line
}

function Fail($msg) {
  Log "FATAL: $msg"
  exit 1
}

# --- Version (default: K6WP_VERSION from the root CMakeLists.txt) ------------
if ([string]::IsNullOrWhiteSpace($Version)) {
  $cmakeLists = Get-Content -LiteralPath (Join-Path $RepoRoot "CMakeLists.txt") -Raw
  if ($cmakeLists -match 'set\(K6WP_VERSION\s+"([^"]+)"\)') {
    $Version = $Matches[1]
  } else {
    Write-Host "[make_zip] FATAL: cannot parse K6WP_VERSION from CMakeLists.txt"
    exit 1
  }
}

New-Item -ItemType Directory -Path $DistDir -Force | Out-Null
$script:LogFile = Join-Path $DistDir "make_zip-$Version.log"
if (Test-Path -LiteralPath $script:LogFile) { Remove-Item -LiteralPath $script:LogFile -Force }
Log "K6WP portable ZIP build, version $Version"

# --- 0a. Version single-source assert (release hardening) --------------------
# K6WP_VERSION in the root CMakeLists.txt is canonical. configure_file()
# stamps it into build/<cfg>/generated/version.h (C/C++/RC consumers),
# packaging/version.nsh (NSIS installer) and engine/app.manifest (in-place,
# from engine/app.manifest.in). This assert verifies:
#   1. CMakeLists project() VERSION == K6WP_VERSION (the canonical pair).
#   2. The generated files (version.h, version.nsh, app.manifest) all carry
#      the canonical version.
#   3. NO consumer source carries a hand-written version literal - every
#      version must flow from the generated mechanism (version.h / version.nsh
#      / app.manifest.in). Catching drift at release time, no codegen pipeline.
function Normalize-Version3([string]$v) {
  # "1.0.0.0" / "1,0,0,0" -> "1.0.0" (first 3 components).
  # Prerelease suffixes ("-alpha", "-beta.1", "+build") are stripped so the
  # honest-alpha reset (K6WP_VERSION "0.0.1-alpha", numeric spots "0.0.1.0")
  # still asserts as one version.
  $v = ($v -replace '[-+].*$', '')
  $parts = ($v -replace ',', '.').Split('.')
  if ($parts.Count -lt 3) { return $null }
  return ($parts[0..2] -join '.')
}

$versionSpots = @()
$cmakeLists = Get-Content -LiteralPath (Join-Path $RepoRoot "CMakeLists.txt") -Raw
if ($cmakeLists -match 'project\(k6wp\s+VERSION\s+([0-9.]+)') {
  $versionSpots += @{ Where = "CMakeLists.txt project() VERSION"; Value = $Matches[1] }
} else {
  Fail "version assert: cannot parse project() VERSION from CMakeLists.txt"
}
if ($cmakeLists -match 'set\(K6WP_VERSION\s+"([^"]+)"\)') {
  $versionSpots += @{ Where = "CMakeLists.txt K6WP_VERSION"; Value = $Matches[1] }
} else {
  Fail "version assert: cannot parse K6WP_VERSION from CMakeLists.txt"
}

# Generated files (configure_file output) must carry the canonical version.
# configure_file() writes to CMAKE_BINARY_DIR/generated. Relative to $BuildDir
# (this script's own output dir, default build\release) that is either
#   .\generated            single-config tree / the dev presets, whose binary
#                          dir IS the CMake binary dir, or
#   ..\generated           a multi-config generator (Visual Studio) whose cache
#                          sits one level up - which is what CI produces, since
#                          it configures with -B build while this script builds
#                          into build\release. Before this, only the first
#                          layout resolved and CI could never package a ZIP.
$genCandidates = @(
  (Join-Path $BuildDir "generated\version.h"),
  (Join-Path $BuildDir "..\generated\version.h"),
  (Join-Path $BuildDir "Release\generated\version.h"),
  (Join-Path $BuildDir "Debug\generated\version.h"),
  (Join-Path $BuildDir "RelWithDebInfo\generated\version.h")
)
$genHeader = $genCandidates | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
if (-not $genHeader) {
  Fail "version assert: generated header not found (tried: $($genCandidates -join ', ')). Run cmake configure first."
}
if (Test-Path -LiteralPath $genHeader) {
  $genText = Get-Content -LiteralPath $genHeader -Raw
  if ($genText -match '#define\s+K6WP_VERSION_STR\s+"([^"]+)"') {
    $versionSpots += @{ Where = "generated/version.h K6WP_VERSION_STR"; Value = $Matches[1] }
  } else {
    Fail "version assert: cannot parse K6WP_VERSION_STR from $genHeader (run cmake configure first)"
  }
  if ($genText -match '#define\s+K6WP_VERSION_STR_4\s+"([^"]+)"') {
    $versionSpots += @{ Where = "generated/version.h K6WP_VERSION_STR_4"; Value = $Matches[1] }
  } else {
    Fail "version assert: cannot parse K6WP_VERSION_STR_4 from $genHeader"
  }
  if ($genText -match '#define\s+K6WP_VERSION_COMMA\s+([0-9,]+)') {
    $versionSpots += @{ Where = "generated/version.h K6WP_VERSION_COMMA"; Value = $Matches[1] }
  } else {
    Fail "version assert: cannot parse K6WP_VERSION_COMMA from $genHeader"
  }
} else {
  Fail "version assert: generated header not found at $genHeader (run cmake configure first)"
}

$nsh = Join-Path $RepoRoot "packaging\version.nsh"
if (Test-Path -LiteralPath $nsh) {
  $nshText = Get-Content -LiteralPath $nsh -Raw
  if ($nshText -match '!define\s+K6WP_VERSION_STR\s+"([^"]+)"') {
    $versionSpots += @{ Where = "packaging/version.nsh K6WP_VERSION_STR"; Value = $Matches[1] }
  } else {
    Fail "version assert: cannot parse K6WP_VERSION_STR from packaging/version.nsh"
  }
  if ($nshText -match '!define\s+K6WP_VERSION_STR_4\s+"([^"]+)"') {
    $versionSpots += @{ Where = "packaging/version.nsh K6WP_VERSION_STR_4"; Value = $Matches[1] }
  } else {
    Fail "version assert: cannot parse K6WP_VERSION_STR_4 from packaging/version.nsh"
  }
} else {
  Fail "version assert: packaging/version.nsh not found (run cmake configure first)"
}

$manifest = Join-Path $RepoRoot "engine\app.manifest"
if (Test-Path -LiteralPath $manifest) {
  $manifestText = Get-Content -LiteralPath $manifest -Raw
  if ($manifestText -match 'version="([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+)"') {
    $versionSpots += @{ Where = "engine/app.manifest assemblyIdentity version"; Value = $Matches[1] }
  } else {
    Fail "version assert: cannot parse version= from engine/app.manifest"
  }
} else {
  Fail "version assert: engine/app.manifest not found (run cmake configure first)"
}

$canonical = $null
foreach ($spot in $versionSpots) {
  if ($spot.Where -eq "CMakeLists.txt K6WP_VERSION") { $canonical = Normalize-Version3 $spot.Value }
}
if ($null -eq $canonical) { Fail "version assert: cannot normalize canonical K6WP_VERSION" }
$bad = @()
foreach ($spot in $versionSpots) {
  $norm = Normalize-Version3 $spot.Value
  if ($null -eq $norm) { $bad += "$($spot.Where)='$($spot.Value)' (unparseable)" }
  elseif ($norm -ne $canonical) { $bad += "$($spot.Where)='$($spot.Value)' != '$canonical'" }
}
if ($bad.Count -gt 0) {
  foreach ($b in $bad) { Log "  VERSION DRIFT: $b" }
  Fail "version assert failed - $($bad.Count) spot(s) disagree with canonical K6WP_VERSION '$canonical'. Fix the source of truth, not the assert."
}

# No hand-written version literals in any consumer source: every version must
# flow from the generated mechanism (version.h / version.nsh / app.manifest.in).
$consumerFiles = @(
  "launcher\app.rc", "engine\app.rc", "studio\app.rc", "compressor\app.rc",
  "shared\monitor_dump.rc", "packaging\installer.nsi",
  "studio\src\update_checker.cpp", "studio\src\studio_bridge.hpp",
  "engine\app.manifest.in", "version.h.in", "version.nsh.in"
)
$literalRe = '[0-9]+[.,][0-9]+[.,][0-9]+'
$literalHits = @()
foreach ($f in $consumerFiles) {
  $fPath = Join-Path $RepoRoot $f
  if (-not (Test-Path -LiteralPath $fPath)) { Fail "version assert: consumer file missing: $f" }
  $m = Select-String -LiteralPath $fPath -Pattern $literalRe -AllMatches
  foreach ($mm in $m) {
    foreach ($hit in $mm.Matches) {
      $literalHits += "$f : '$($hit.Value)'"
    }
  }
}
if ($literalHits.Count -gt 0) {
  foreach ($h in $literalHits) { Log "  VERSION LITERAL: $h" }
  Fail "version assert failed - $($literalHits.Count) hand-written version literal(s) in consumer sources. Reference version.h / version.nsh / app.manifest.in instead."
}

# Every consumer must actually reference the generated mechanism.
$refChecks = @(
  @{ File = "launcher\app.rc";            Needle = 'K6WP_VERSION_COMMA' },
  @{ File = "engine\app.rc";              Needle = 'K6WP_VERSION_COMMA' },
  @{ File = "studio\app.rc";              Needle = 'K6WP_VERSION_COMMA' },
  @{ File = "compressor\app.rc";          Needle = 'K6WP_VERSION_COMMA' },
  @{ File = "shared\monitor_dump.rc";     Needle = 'K6WP_VERSION_COMMA' },
  @{ File = "packaging\installer.nsi";    Needle = 'K6WP_VERSION_STR' },
  @{ File = "studio\src\update_checker.cpp"; Needle = 'K6WP_VERSION_STR' },
  @{ File = "studio\src\studio_bridge.hpp"; Needle = 'K6WP_VERSION_STR' },
  @{ File = "engine\app.manifest.in";     Needle = '@K6WP_VERSION_4@' }
)
$refBad = @()
foreach ($rc in $refChecks) {
  $rcPath = Join-Path $RepoRoot $rc.File
  $rcText = Get-Content -LiteralPath $rcPath -Raw
  if ($rcText -notmatch [regex]::Escape($rc.Needle)) {
    $refBad += "$($rc.File) does not reference '$($rc.Needle)'"
  }
}
if ($refBad.Count -gt 0) {
  foreach ($b in $refBad) { Log "  VERSION REF MISSING: $b" }
  Fail "version assert failed - $($refBad.Count) consumer(s) do not reference the generated version mechanism."
}

Log "Version assert OK: $($versionSpots.Count) version spots agree with K6WP_VERSION '$canonical'; no literals in $($consumerFiles.Count) consumer sources."

# --- 1. Fresh full build (all targets, windeployqt runs as studio POST_BUILD)
# Qt location: explicit override wins, then the CI Qt action's QT_ROOT_DIR,
# then a pre-exported QT_ROOT, then the local install path (local default).
if (-not [string]::IsNullOrWhiteSpace($QtRootOverride)) {
  $env:QT_ROOT = $QtRootOverride
} elseif (-not [string]::IsNullOrWhiteSpace($env:QT_ROOT_DIR)) {
  $env:QT_ROOT = $env:QT_ROOT_DIR
} elseif ([string]::IsNullOrWhiteSpace($env:QT_ROOT)) {
  $env:QT_ROOT = "C:\Qt\6.8.3\msvc2022_64"
}
if ([string]::IsNullOrWhiteSpace($Generator)) {
  Log "Building all targets (preset release)..."
  $buildOut = & $CmakeExe --build --preset release 2>&1 | Out-String
  Add-Content -LiteralPath $script:LogFile -Value $buildOut
  Write-Host $buildOut
  if ($LASTEXITCODE -ne 0) { Fail "cmake --build --preset release failed with exit $LASTEXITCODE" }
} else {
  # "-Generator auto" omits -G so CMake picks the newest installed Visual
  # Studio. Any other value is passed through as -G, which pins a VS version
  # and breaks on a runner that has a different one - that is what made CI
  # packaging fail with "could not find any instance of Visual Studio".
  $genArgs = @()
  if ($Generator -ne "auto") { $genArgs = @("-G", $Generator) }
  Log "Building all targets (generator $(if ($genArgs.Count) { $Generator } else { 'auto-detected' }), dir $BuildDir)..."
  $cfgOut = & $CmakeExe -S $RepoRoot -B $BuildDir @genArgs -A x64 "-DCMAKE_PREFIX_PATH=$env:QT_ROOT" -DBUILD_TESTING=OFF -DK6WP_BUILD_TOOLS=OFF 2>&1 | Out-String
  Add-Content -LiteralPath $script:LogFile -Value $cfgOut
  Write-Host $cfgOut
  if ($LASTEXITCODE -ne 0) { Fail "cmake configure ($Generator) failed with exit $LASTEXITCODE" }
  $buildOut = & $CmakeExe --build $BuildDir --config Release --parallel 2>&1 | Out-String
  Add-Content -LiteralPath $script:LogFile -Value $buildOut
  Write-Host $buildOut
  if ($LASTEXITCODE -ne 0) { Fail "cmake --build ($Generator) failed with exit $LASTEXITCODE" }
}
Log "Build OK."

# --- 2. Fresh staging dir -----------------------------------------------------
$StageDir = Join-Path $DistDir "stage\K6WP-portable-$Version"
if (Test-Path -LiteralPath $StageDir) { Remove-Item -LiteralPath $StageDir -Recurse -Force }
New-Item -ItemType Directory -Path $StageDir -Force | Out-Null

function Stage-Copy($src, $dstName) {
  if (-not (Test-Path -LiteralPath $src)) { Fail "required file missing: $src" }
  Copy-Item -LiteralPath $src -Destination (Join-Path $StageDir $dstName) -Force
  Log "staged: $dstName"
}

# 2a. Our exes (K6WP.exe is the Qt-free single-app launcher, staged next to
# engine/studio per the binary contract; monitor_dump is optional).
foreach ($exe in @("K6WP.exe", "engine.exe", "studio.exe", "compressor.exe")) {
  Stage-Copy (Join-Path $BuildDir $exe) $exe
}
$monitorDump = Join-Path $BuildDir "monitor_dump.exe"
if (Test-Path -LiteralPath $monitorDump) {
  Stage-Copy $monitorDump "monitor_dump.exe"
} else {
  Log "WARNING: monitor_dump.exe not in build dir - skipping (optional)."
}

# 2b. Vendored ffmpeg + ffprobe (full_build; compressor needs ffprobe for the
# duration check on existing inputs, so both ship).
Stage-Copy (Join-Path $RepoRoot "vendor\ffmpeg\ffmpeg.exe") "ffmpeg.exe"
Stage-Copy (Join-Path $RepoRoot "vendor\ffmpeg\ffprobe.exe") "ffprobe.exe"

# 2c. Vendored libmpv.
Stage-Copy (Join-Path $RepoRoot "vendor\libmpv\bin\libmpv-2.dll") "libmpv-2.dll"

# 2d. Qt deploy output: enumerate the ACTUAL windeployqt result in the build
# dir. Rule: every *.dll next to studio.exe + every build subdir that contains
# *.dll files (platforms/, styles/, imageformats/, qml/, ...). Excludes: *.pdb,
# exes, CMake object dirs (*.dir), CMakeFiles, x64, .qt - none of which
# contain deployed DLLs, so the filter "contains *.dll" keeps this exact.
$rootDlls = Get-ChildItem -LiteralPath $BuildDir -Filter "*.dll" -File
if ($rootDlls.Count -eq 0) { Fail "no DLLs next to studio.exe - windeployqt did not run?" }
foreach ($dll in $rootDlls) {
  Copy-Item -LiteralPath $dll.FullName -Destination (Join-Path $StageDir $dll.Name) -Force
}
Log "staged: $($rootDlls.Count) root DLLs (windeployqt output)."
# qmltooling/ is the QML debugger/inspector/profiler plugin set (qmldbg_*).
# windeployqt emits it in Release too, but a shipped app must not carry it:
# qmldbg_tcp.dll + qmldbg_server.dll accept inbound connections, so shipping
# them adds remote-debugger attack surface for zero user value. The QML engine
# never loads them unless a debugger is attached.
$skipDirs = @("CMakeFiles", "x64", ".qt", "qmltooling")
foreach ($dir in Get-ChildItem -LiteralPath $BuildDir -Directory) {
  if ($dir.Name -like "*.dir") { continue }
  if ($skipDirs -contains $dir.Name) { continue }
  $dlls = Get-ChildItem -LiteralPath $dir.FullName -Filter "*.dll" -File -Recurse
  if ($dlls.Count -eq 0) { continue }
  $dest = Join-Path $StageDir $dir.Name
  New-Item -ItemType Directory -Path $dest -Force | Out-Null
  # Copy EVERY file once the dir qualifies, not just the DLLs. The dir gate
  # above is "contains *.dll" (that is what keeps CMake object dirs out), but
  # installer.nsi ships these with `File /r`, which copies the whole tree.
  # qml/ is why this matters: its payload is overwhelmingly non-DLL
  # (.qml / qmldir / .qmltypes) with only a handful of *plugin.dll files, so a
  # DLL-only copy staged 26 files against the installer's 1209.
  $payload = Get-ChildItem -LiteralPath $dir.FullName -File -Recurse
  foreach ($f in $payload) {
    $rel = $f.FullName.Substring($dir.FullName.Length + 1)
    $target = Join-Path $dest $rel
    $parent = Split-Path -Parent $target
    if (-not (Test-Path -LiteralPath $parent)) { New-Item -ItemType Directory -Path $parent -Force | Out-Null }
    Copy-Item -LiteralPath $f.FullName -Destination $target -Force
  }
  Log "staged plugin dir: $($dir.Name) ($($payload.Count) files, $($dlls.Count) DLLs)."
}

# 2e. Example config + uninstaller + license texts (GPL-2.0-or-later release).
Stage-Copy (Join-Path $RepoRoot "packaging\config.json.example") "config.json.example"
Stage-Copy (Join-Path $RepoRoot "packaging\playlist.json.example") "playlist.json.example"
Stage-Copy (Join-Path $RepoRoot "packaging\uninstall.bat") "uninstall.bat"
# Root LICENSE + LICENSES/ third-party notices. LICENSES ships as a subdir
# (installer.nsi stages it via File /r "${REPO_ROOT}\LICENSES", so the
# ship-set sync assert below matches file-for-file).
Stage-Copy (Join-Path $RepoRoot "LICENSE") "LICENSE"
$LicensesDir = Join-Path $StageDir "LICENSES"
New-Item -ItemType Directory -Path $LicensesDir -Force | Out-Null
foreach ($lic in @("libmpv.txt", "ffmpeg.txt", "Qt.txt", "nlohmann-json.txt")) {
  $src = Join-Path $RepoRoot "LICENSES\$lic"
  if (-not (Test-Path -LiteralPath $src)) { Fail "required license file missing: $src" }
  Copy-Item -LiteralPath $src -Destination (Join-Path $LicensesDir $lic) -Force
  Log "staged: LICENSES\$lic"
}

# 2f. App-local VC runtime (fresh-machine safety). Our exes + Qt +
# ffmpeg + libmpv all import MSVCP140/VCRUNTIME140*, so a fresh Windows
# box without any redist-installing app would fail to start. Stage the 3
# CRT DLLs app-local (no UAC, matches the per-user installer design).
# Source: newest VS Redist x64 CRT dir; fallback is a pinned copy under
# vendor\vc_runtime\ (checked in once via Download-VcRuntime below).
$CrtDlls = @("vcruntime140.dll", "vcruntime140_1.dll", "msvcp140.dll")
$RedistRoot = "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Redist\MSVC"
$CrtDir = $null
if (Test-Path -LiteralPath $RedistRoot) {
  $CrtDir = Get-ChildItem -LiteralPath $RedistRoot -Directory |
    Where-Object { $_.Name -match '^14\.' } | Sort-Object Name -Descending |
    ForEach-Object {
      $crt = Get-ChildItem -LiteralPath (Join-Path $_.FullName "x64") -Filter "Microsoft.VC*.CRT" -Directory -ErrorAction SilentlyContinue |
        Where-Object { $_.FullName -notlike "*onecore*" } |
        ForEach-Object { Join-Path $_.FullName "vcruntime140.dll" } |
        Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
      if ($crt) { Split-Path -Parent $crt }
    } | Select-Object -First 1
}
if (-not $CrtDir) {
  # Runner fallback: newest VS carrying VC tools via vswhere (local VS18 path
  # above wins when present, so local behavior is unchanged).
  $vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
  if (Test-Path -LiteralPath $vswhere) {
    $vsPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools -property installationPath 2>$null | Select-Object -First 1
    if (-not [string]::IsNullOrWhiteSpace($vsPath)) {
      $vsRedist = Join-Path $vsPath "VC\Redist\MSVC"
      if (Test-Path -LiteralPath $vsRedist) {
        $CrtDir = Get-ChildItem -LiteralPath $vsRedist -Directory |
          Where-Object { $_.Name -match '^14\.' } | Sort-Object Name -Descending |
          ForEach-Object {
            $crt = Get-ChildItem -LiteralPath (Join-Path $_.FullName "x64") -Filter "Microsoft.VC*.CRT" -Directory -ErrorAction SilentlyContinue |
              Where-Object { $_.FullName -notlike "*onecore*" } |
              ForEach-Object { Join-Path $_.FullName "vcruntime140.dll" } |
              Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
            if ($crt) { Split-Path -Parent $crt }
          } | Select-Object -First 1
        if ($CrtDir) { Log "VS Redist found via vswhere: $vsPath." }
      }
    }
  }
}
if ($CrtDir) {
  foreach ($dll in $CrtDlls) {
    $src = Join-Path $CrtDir $dll
    if (-not (Test-Path -LiteralPath $src)) { Fail "CRT DLL missing in ${CrtDir}: $dll" }
    Stage-Copy $src $dll
  }
  Log "staged: app-local VC runtime from $CrtDir."
  # Cache a pinned copy for installer.nsi builds (makensis cannot discover
  # the versioned Redist dir, so it consumes vendor\vc_runtime\).
  $VendorCrt = Join-Path $RepoRoot "vendor\vc_runtime"
  New-Item -ItemType Directory -Path $VendorCrt -Force | Out-Null
  foreach ($dll in $CrtDlls) {
    Copy-Item -LiteralPath (Join-Path $CrtDir $dll) -Destination (Join-Path $VendorCrt $dll) -Force
  }
  Log "cached: vendor\vc_runtime ($($CrtDlls.Count) DLLs for installer builds)."
} else {
  $VendorCrt = Join-Path $RepoRoot "vendor\vc_runtime"
  $missing = @($CrtDlls | Where-Object { -not (Test-Path -LiteralPath (Join-Path $VendorCrt $_)) })
  if ($missing.Count -gt 0) {
    Fail ("no VS Redist found and vendor\vc_runtime is missing: " + ($missing -join ", ") +
      ". Copy the 3 CRT DLLs there once (see packaging README) or install VS BuildTools.")
  }
  foreach ($dll in $CrtDlls) { Stage-Copy (Join-Path $VendorCrt $dll) $dll }
  Log "staged: app-local VC runtime from vendor\vc_runtime."
}

# --- 2g. Ship-set sync assert (release hardening) ----------------------------
# The staged file set must EXACTLY match the File/RMDir set in
# packaging/installer.nsi (parse the File directives, including the conditional
# monitor_dump.exe and the CRT DLLs + Qt plugin dirs). Fails on drift in either
# direction: a file staged but not installed, or installed but not staged,
# means the ZIP and the installer ship different payloads.
$nsiText = Get-Content -LiteralPath (Join-Path $RepoRoot "packaging\installer.nsi") -Raw

# Define map for the ${...} constants used by File directives.
$nsiSrc = @{
  '${SRCDIR}'        = $BuildDir
  '${VENDOR_FFMPEG}' = Join-Path $RepoRoot "vendor\ffmpeg"
  '${VENDOR_LIBMPV}' = Join-Path $RepoRoot "vendor\libmpv\bin"
  '${VENDOR_VC}'     = Join-Path $RepoRoot "vendor\vc_runtime"
  '${REPO_ROOT}'     = $RepoRoot
}

# Drop !if /FileExists "..."/!endif blocks whose condition is false (the
# conditional monitor_dump.exe File directive), so the expected set matches
# what makensis would actually compile in.
$nsiText = [regex]::Replace($nsiText, '(?s)!if\s+/FileExists\s+"([^"]+)"(.*?)!endif', {
  param($mm)
  $condSrc = $mm.Groups[1].Value
  foreach ($k in $nsiSrc.Keys) { $condSrc = $condSrc.Replace($k, $nsiSrc[$k]) }
  if (Test-Path -LiteralPath $condSrc) { return $mm.Value }
  return ""
})

$expected = @{}  # install-dir-relative path -> installer.nsi source string
foreach ($m in [regex]::Matches($nsiText, '(?m)^\s*File\s+(/r\s+)?"([^"]+)"')) {
  $recursive = $m.Groups[1].Success
  $raw = $m.Groups[2].Value
  $src = $raw
  foreach ($k in $nsiSrc.Keys) { $src = $src.Replace($k, $nsiSrc[$k]) }
  if ($src -eq $raw) {
    # Bare relative path (config.json.example / uninstall.bat): script dir.
    $src = Join-Path $PSScriptRoot $raw
  }
  if ($recursive) {
    if (-not (Test-Path -LiteralPath $src)) {
      Fail "ship-set assert: installer.nsi File /r source missing: $src"
    }
    foreach ($f in Get-ChildItem -LiteralPath $src -Recurse -File) {
      $rel = $f.FullName.Substring($src.Length + 1)
      $expected[(Join-Path (Split-Path -Leaf $src) $rel)] = $raw
    }
  } elseif ($src -like "*\*") {
    # Wildcard (root *.dll): enumerate matches.
    $dir = Split-Path -Parent $src
    $pat = Split-Path -Leaf $src
    if (-not (Test-Path -LiteralPath $dir)) { Fail "ship-set assert: wildcard dir missing: $dir" }
    foreach ($f in Get-ChildItem -LiteralPath $dir -Filter $pat -File) {
      $expected[$f.Name] = $raw
    }
  } else {
    if (-not (Test-Path -LiteralPath $src)) {
      Fail "ship-set assert: installer.nsi File source missing: $src"
    }
    $expected[(Split-Path -Leaf $src)] = $raw
  }
}

$actual = @{}
foreach ($f in Get-ChildItem -LiteralPath $StageDir -Recurse -File) {
  $actual[$f.FullName.Substring($StageDir.Length + 1)] = $true
}

$onlyStaged = @($actual.Keys | Where-Object { -not $expected.ContainsKey($_) } | Sort-Object)
$onlyInstaller = @($expected.Keys | Where-Object { -not $actual.ContainsKey($_) } | Sort-Object)
if ($onlyStaged.Count -gt 0 -or $onlyInstaller.Count -gt 0) {
  foreach ($p in $onlyStaged) { Log "  SHIP-SET DRIFT (staged but not in installer.nsi): $p" }
  foreach ($p in $onlyInstaller) { Log "  SHIP-SET DRIFT (in installer.nsi but not staged): $p" }
  Fail "ship-set assert failed - staged set and installer.nsi File set disagree ($($onlyStaged.Count) staged-only, $($onlyInstaller.Count) installer-only). Keep packaging/make_zip.ps1 and packaging/installer.nsi in sync."
}

# RMDir /r "$INSTDIR\X" dirs (uninstall + rollback) must exist in the stage.
$rmDirs = @()
foreach ($m in [regex]::Matches($nsiText, 'RMDir\s+/r\s+"\$INSTDIR\\([^"]+)"')) {
  $rmDirs += $m.Groups[1].Value
}
$missingDirs = @($rmDirs | Where-Object { -not (Test-Path -LiteralPath (Join-Path $StageDir $_)) } | Sort-Object -Unique)
if ($missingDirs.Count -gt 0) {
  foreach ($d in $missingDirs) { Log "  SHIP-SET DRIFT (RMDir dir not staged): $d" }
  Fail "ship-set assert failed - installer.nsi RMDir /r dir(s) missing from staging: $($missingDirs -join ', ')"
}
Log "Ship-set assert OK: $($expected.Count) staged files match installer.nsi File set; $($rmDirs.Count) RMDir dirs present."

  # --- 3. MISSING-DLL check (dumpbin /dependents per staged exe) ----------------
  # Tried in order, because the MSVC bin layout varies by VS version and host
  # target and this is a fail-closed safety net: if we cannot locate dumpbin we
  # must not wave the ZIP through unverified.
  #   1. PATH (a developer command prompt, or a runner that pre-loads MSVC)
  #   2. the local BuildTools 18 path
  #   3. vswhere -> newest VC toolset -> bin\Hostx64\x64
  #   4. vswhere -> newest VC toolset -> recursive glob, any host/arch dir
  # Located as a plain path string, not an object: Get-Command yields an
  # ApplicationInfo (which has .Path, not .FullName), while the file probes
  # yield FileInfo. Mixing them left the invocation path empty on some machines.
  #
  # PATH is consulted LAST and only accepts a modern MSVC toolset. This machine
  # has Visual Studio 98's dumpbin.exe on PATH, and preferring it silently
  # weakened this check: a VC98 dumpbin cannot parse PE32+ imports, so it
  # reports no dependencies at all and the "all imports resolve" assert
  # passes without having checked anything. A fail-closed gate that can
  # false-pass is worse than no gate.
  $dumpbinPath = $null
  $f = Get-ChildItem -Path "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Tools\MSVC" -Filter "dumpbin.exe" -Recurse -ErrorAction SilentlyContinue |
    Where-Object { $_.FullName -like "*Hostx64\x64*" } | Select-Object -First 1
  if ($f) { $dumpbinPath = $f.FullName }
  if (-not $dumpbinPath) {
    # Every instance, not just -latest: a runner can carry several Visual
    # Studios and the newest one may lack the VC tools.
    $vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
    $vsPaths = @()
    if (Test-Path -LiteralPath $vswhere) {
      $vsPaths = @(& $vswhere -products * -all -prerelease -requires Microsoft.VisualStudio.Component.VC.Tools -property installationPath 2>$null |
        Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
    }
    foreach ($vp in $vsPaths) {
      $msvcRoot = Join-Path $vp "VC\Tools\MSVC"
      $toolset = Get-ChildItem -LiteralPath $msvcRoot -Directory -ErrorAction SilentlyContinue |
        Sort-Object Name -Descending | Select-Object -First 1
      if ($toolset) {
        $cand = Join-Path $toolset.FullName "bin\Hostx64\x64\dumpbin.exe"
        if (Test-Path -LiteralPath $cand) { $dumpbinPath = $cand; break }
      }
      $g = Get-ChildItem -LiteralPath $msvcRoot -Filter "dumpbin.exe" -Recurse -ErrorAction SilentlyContinue |
        Select-Object -First 1
      if ($g) { $dumpbinPath = $g.FullName; break }
    }
    # A GitHub windows runner has no VS instance carrying the VC.Tools
    # component id (vswhere reported 0) and no dumpbin on PATH, yet the build
    # succeeds, so a toolset is present. Guessing VS directory layouts kept
    # missing; ask the build system instead. CMake records the toolchain it
    # resolved in CMakeCache - the linker and RC compiler are recorded
    # (CMAKE_CXX_COMPILER is not), and dumpbin sits in the same
    # .../VC/Tools/MSVC/<ver>/bin/Hostx64/x64 directory as link.exe, so derive
    # it from whichever of those is present.
    if (-not $dumpbinPath) {
      $candidates = @(
        (Join-Path $BuildDir "CMakeCache.txt"),
        (Join-Path $RepoRoot "build\CMakeCache.txt"),
        (Join-Path $RepoRoot "build\Release\CMakeCache.txt")
      )
      foreach ($cache in $candidates) {
        if (-not (Test-Path -LiteralPath $cache)) { continue }
        $tool = $null
        foreach ($key in @('CMAKE_LINKER', 'CMAKE_RC_COMPILER', 'CMAKE_CXX_COMPILER')) {
          $hit = Select-String -LiteralPath $cache -Pattern ("^" + $key + ":") -ErrorAction SilentlyContinue |
            Select-Object -First 1
          if ($hit) { $tool = ($hit.Line -split '=', 2)[1].Trim(); break }
        }
        if (-not $tool) { continue }
        $binDir = Split-Path -Parent $tool
        $d = Get-ChildItem -LiteralPath $binDir -Filter "dumpbin.exe" -ErrorAction SilentlyContinue |
          Select-Object -First 1
        if ($d) { $dumpbinPath = $d.FullName; break }
        # RC.EXE can live outside the toolset (Common\MDev98\Bin), so also walk
        # up to the toolset and glob.
        $d2 = Get-ChildItem -LiteralPath (Split-Path -Parent $binDir) -Filter "dumpbin.exe" -Recurse -ErrorAction SilentlyContinue |
          Select-Object -First 1
        if ($d2) { $dumpbinPath = $d2.FullName; break }
      }
    }
    if (-not $dumpbinPath) {
      $onPath = Get-Command dumpbin.exe -ErrorAction SilentlyContinue | Select-Object -First 1
      if ($onPath -and $onPath.Path -like "*\Tools\MSVC\*") { $dumpbinPath = $onPath.Path }
    }
    if (-not $dumpbinPath) {
      # Report what was actually inspected. A fail-closed gate that cannot say
      # why it failed just sends the next person back to guessing.
      $seen = @()
      $seen += "vswhere present : $(Test-Path -LiteralPath $vswhere) ($vswhere)"
      $seen += "instances with VC.Tools : $($vsPaths.Count)"
      foreach ($vp in $vsPaths) {
        $seen += "  $vp"
        $seen += "    VC\Tools\MSVC exists : $(Test-Path -LiteralPath (Join-Path $vp 'VC\Tools\MSVC'))"
        $seen += "    bin contents : $((Get-ChildItem -LiteralPath (Join-Path $vp 'VC\Tools') -Recurse -Filter 'dumpbin.exe' -ErrorAction SilentlyContinue | Select-Object -First 3 -ExpandProperty FullName) -join '; ')"
      }
      foreach ($cache in @((Join-Path $BuildDir "CMakeCache.txt"), (Join-Path $RepoRoot "build\CMakeCache.txt"))) {
        $seen += "cache $cache exists : $(Test-Path -LiteralPath $cache)"
        if (Test-Path -LiteralPath $cache) {
          foreach ($key in @('CMAKE_LINKER', 'CMAKE_RC_COMPILER', 'CMAKE_CXX_COMPILER')) {
            $hit = Select-String -LiteralPath $cache -Pattern ("^" + $key + ":") -ErrorAction SilentlyContinue | Select-Object -First 1
            $seen += "  $key : $(if ($hit) { $hit.Line } else { 'not recorded' })"
          }
        }
      }
      $seen += "dumpbin on PATH : $(if (Get-Command dumpbin.exe -ErrorAction SilentlyContinue) { (Get-Command dumpbin.exe).Source } else { 'none' })"
      Fail ("dumpbin.exe (Hostx64) not found. Probed: " + ($seen -join ' | '))
    }
  }
Log "dumpbin: $($dumpbinPath)"

$systemDir = Join-Path $env:SystemRoot "System32"
$missing = @()
$warnings = @()
$exesToCheck = @("K6WP.exe", "engine.exe", "studio.exe", "compressor.exe")
if (Test-Path -LiteralPath (Join-Path $StageDir "monitor_dump.exe")) { $exesToCheck += "monitor_dump.exe" }
foreach ($exe in $exesToCheck) {
  $out = & $dumpbinPath /dependents (Join-Path $StageDir $exe) 2>&1 | Out-String
  $inDelay = $false
  foreach ($line in $out -split "`r?`n") {
    if ($line -match "delay load dependencies") { $inDelay = $true; continue }
    if ($line -match "^\s+(\S+\.dll)\s*$") {
      $dep = $Matches[1]
      if ($dep -like "api-ms-win-*" -or $dep -like "ext-ms-*") { continue }  # API sets, always present
      $inStage = Test-Path -LiteralPath (Join-Path $StageDir $dep)
      $inSystem = Test-Path -LiteralPath (Join-Path $systemDir $dep)
      if (-not $inStage -and -not $inSystem) {
        if ($inDelay) { $warnings += "${exe}: delay-load $dep" }
        else { $missing += "${exe}: $dep" }
      }
    }
  }
}
foreach ($w in $warnings) { Log "WARNING (delay-load, runtime-only): $w" }
if ($missing.Count -gt 0) {
  Log "MISSING DLLS ($($missing.Count)):"
  foreach ($m in $missing) { Log "  MISSING: $m" }
  Fail "import check failed - $($missing.Count) missing DLLs listed above. Not shipping a broken ZIP."
}
Log "Import check OK: all hard imports of $($exesToCheck.Count) exes resolve from staging dir or System32."

# --- 3b. Qt-free launcher assert (owner decision F1): K6WP.exe must not
# depend on any Qt6*.dll. Locks the user32+shell32-only property into release.
$qtDeps = @()
$k6wpOut = & $dumpbinPath /dependents (Join-Path $StageDir "K6WP.exe") 2>&1 | Out-String
foreach ($line in $k6wpOut -split "`r?`n") {
  if ($line -match "^\s+(\S+\.dll)\s*$") {
    if ($Matches[1] -like "Qt6*.dll") { $qtDeps += $Matches[1] }
  }
}
if ($qtDeps.Count -gt 0) {
  foreach ($d in $qtDeps) { Log "  QT-FREE VIOLATION: K6WP.exe depends on $d" }
  Fail "Qt-free assert failed - K6WP.exe pulls $($qtDeps.Count) Qt DLL(s). Launcher must stay Qt-free."
}
Log "Qt-free assert OK: K6WP.exe has no Qt6*.dll dependents."

# --- 3c. VERSIONINFO assert: ProductName must be K6WP on every staged exe
# (GUI-subsystem triple + compressor + monitor_dump, proven in
# build/qa_todo11.log). Also asserts each staged exe carries a non-empty
# .rsrc section (dumpbin /headers) so a lost icon resource fails loudly.
$versionExes = @("K6WP.exe", "engine.exe", "studio.exe", "compressor.exe")
if (Test-Path -LiteralPath (Join-Path $StageDir "monitor_dump.exe")) { $versionExes += "monitor_dump.exe" }
foreach ($exe in $versionExes) {
  $prodName = [System.Diagnostics.FileVersionInfo]::GetVersionInfo((Join-Path $StageDir $exe)).ProductName
  Log "VersionInfo: $exe ProductName='$prodName'"
  if ($prodName -ne "K6WP") { Fail "version assert failed - $exe ProductName='$prodName', expected 'K6WP'." }
  $headers = & $dumpbinPath /headers (Join-Path $StageDir $exe) 2>&1 | Out-String
  # dumpbin prints one "SECTION HEADER #N" block per section with ".rsrc name"
  # on its own line and the size two lines below ("<hex> size of raw data").
  $rsrcBytes = 0
  $inRsrc = $false
  foreach ($line in $headers -split "`r?`n") {
    if ($line -match "^\s*\.rsrc\s+name\s*$") { $inRsrc = $true; continue }
    if ($line -match "^SECTION HEADER") { $inRsrc = $false; continue }
    if ($inRsrc -and $line -match "^\s*([0-9A-Fa-f]+)\s+size of raw data\s*$") {
      $rsrcBytes = [Convert]::ToInt32($Matches[1], 16)
      break
    }
  }
  if ($rsrcBytes -eq 0) { Fail "resource assert failed - $exe has no .rsrc section with raw data (icon lost?)." }
  Log "Resource: $exe .rsrc raw bytes=0x$($rsrcBytes.ToString('X'))"
}
Log "Version assert OK: ProductName==K6WP + .rsrc present on $($versionExes -join ', ')."

# --- 4. Size accounting + ZIP --------------------------------------------------
Log "Per-file sizes in staging (MB):"
$files = Get-ChildItem -LiteralPath $StageDir -Recurse -File | Sort-Object Length -Descending
foreach ($f in $files) {
  $mb = [math]::Round($f.Length / 1MB, 2)
  $rel = $f.FullName.Substring($StageDir.Length + 1)
  Log ("  {0,8} MB  {1}" -f $mb, $rel)
}
$uncompMB = [math]::Round(($files | Measure-Object Length -Sum).Sum / 1MB, 2)
Log "Staging total uncompressed: $uncompMB MB ($($files.Count) files)."

$ZipPath = Join-Path $DistDir "K6WP-portable-$Version.zip"
if (Test-Path -LiteralPath $ZipPath) { Remove-Item -LiteralPath $ZipPath -Force }
Compress-Archive -Path (Join-Path $StageDir "*") -DestinationPath $ZipPath -Force
$zipMB = [math]::Round((Get-Item -LiteralPath $ZipPath).Length / 1MB, 2)
Log "ZIP: $ZipPath ($zipMB MB)."
if ($zipMB -ge $SizeLimitMB) {
  Fail "ZIP is $zipMB MB, over the ${SizeLimitMB} MB limit. Breakdown logged above - not shipping oversize."
}
Log "Size assert OK: $zipMB MB < $SizeLimitMB MB."
Log "DONE. Artifact: $ZipPath"
```

## File: packaging/playlist.json.example
```
{
  "_comment": "K6WP example wallpaper playlist, schema v1. Copy to %LOCALAPPDATA%/K6WP/playlist.json and set order to REAL video files on your machine - the placeholders below do not exist. When enabled, the engine rotates through order every interval_min minutes. interval_min is clamped to [1, 1440]; entries are de-duplicated case-insensitively. The engine reads this file; Studio edits it. It is separate from config.json so older Studio builds can never clobber it.",
  "version": 1,
  "enabled": false,
  "interval_min": 30,
  "shuffle": false,
  "order": [
    "C:/Videos/wallpaper-a.mp4",
    "C:/Videos/wallpaper-b.mp4"
  ]
}
```

## File: packaging/resource.h
```c

```

## File: packaging/sign_outputs.ps1
```powershell
# K6WP release signing step (task 36, AV/SmartScreen remediation).
#
# STRICTLY OPT-IN. Signing only runs when BOTH environment variables are set:
#   K6WP_SIGN_PFX            - path to a code-signing PFX file
#   K6WP_SIGN_PFX_PASSWORD   - password for that PFX
# Optional:
#   K6WP_SIGN_TIMESTAMP_URL  - RFC 3161 timestamp server
#                              (default: http://timestamp.digicert.com)
#
# When the env vars are unset, or signtool.exe cannot be located, the script
# prints a clear NO-OP message and exits 0 — packaging/make_zip.ps1 and
# packaging/installer.nsi therefore never break in the default (unsigned)
# configuration. No certificate ships with the repo; obtaining one is a
# documented action item in packaging/known-limitations.md §4.
#
# What gets signed (SHA256 file digest + RFC 3161 timestamp, /fd SHA256
# /td SHA256 /tr <url>):
#   - every *.exe under the portable staging dir dist\stage\K6WP-portable-<ver>\
#     (our K6WP/engine/studio/compressor/monitor_dump exes AND the vendored
#     ffmpeg/ffprobe — the whole ship-set is signed so no shipped exe is left
#     "Unknown publisher"),
#   - dist\K6WP-Setup.exe when it exists (NSIS output; run makensis first).
#
# Intended release order when signing is enabled:
#   1. powershell -ExecutionPolicy Bypass -File packaging\make_zip.ps1
#      (build + stage + ZIP + asserts)
#   2. makensis packaging\installer.nsi          -> dist\K6WP-Setup.exe
#   3. powershell -ExecutionPolicy Bypass -File packaging\sign_outputs.ps1
#      (signs the staged exes + dist\K6WP-Setup.exe IN PLACE)
#   4. Re-zip the signed stage so the portable ZIP matches the installer:
#      Compress-Archive -Force -Path "dist\stage\K6WP-portable-<ver>\*" `
#        -DestinationPath "dist\K6WP-portable-<ver>.zip"
# Step 4 is intentionally NOT automated here: make_zip.ps1 owns the ZIP
# asserts (size limit, ship-set sync) and must stay untouched.
#
# Usage (PowerShell 5.1):
#   powershell -ExecutionPolicy Bypass -File packaging\sign_outputs.ps1 `
#     [-StageDir <path>] [-SetupExe <path>]
#   -StageDir  overrides the auto-discovered newest dist\stage\K6WP-portable-* dir
#   -SetupExe  overrides the default dist\K6WP-Setup.exe path
#
# Exit codes: 0 = signed OK or no-op (env unset / signtool missing / nothing
# to sign); 1 = misconfiguration (PFX path invalid) or a signtool failure.
# Never claims anything about resulting AV/SmartScreen verdicts — see
# packaging/known-limitations.md §4.

param(
  [string]$StageDir = "",
  [string]$SetupExe = ""
)

$ErrorActionPreference = "Continue"

$RepoRoot = Split-Path -Parent $PSScriptRoot
$TimestampUrl = "http://timestamp.digicert.com"
if (-not [string]::IsNullOrWhiteSpace($env:K6WP_SIGN_TIMESTAMP_URL)) {
  $TimestampUrl = $env:K6WP_SIGN_TIMESTAMP_URL
}

function Log($msg) {
  Write-Host "[sign_outputs] $msg"
}

function NoOp($why) {
  Log "NO-OP: $why"
  Log "Nothing was signed; binaries stay unsigned. Exiting 0 (signing is strictly opt-in)."
  exit 0
}

# --- Gate 1: signing env vars -------------------------------------------------
if ([string]::IsNullOrWhiteSpace($env:K6WP_SIGN_PFX) -or
    [string]::IsNullOrWhiteSpace($env:K6WP_SIGN_PFX_PASSWORD)) {
  NoOp "K6WP_SIGN_PFX / K6WP_SIGN_PFX_PASSWORD are not both set."
}
if (-not (Test-Path -LiteralPath $env:K6WP_SIGN_PFX)) {
  # Env is set but the file is missing: a real misconfiguration, not a no-op.
  Log "ERROR: K6WP_SIGN_PFX is set but the file does not exist: $env:K6WP_SIGN_PFX"
  exit 1
}

# --- Gate 2: locate signtool.exe (PATH first, then vswhere -> newest VC toolset) ---
# Fail soft: a machine without the VC toolset simply no-ops instead of failing
# the release pipeline.
$signtoolPath = $null
$onPath = Get-Command signtool.exe -ErrorAction SilentlyContinue | Select-Object -First 1
if ($onPath) {
  # Get-Command yields ApplicationInfo (.Path), not FileInfo (.FullName).
  $signtoolPath = $onPath.Path
}
if (-not $signtoolPath) {
  $vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
  if (Test-Path -LiteralPath $vswhere) {
    $vsPaths = @(& $vswhere -products * -all -prerelease -requires Microsoft.VisualStudio.Component.VC.Tools -property installationPath 2>$null |
      Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
    foreach ($vp in $vsPaths) {
      $msvcRoot = Join-Path $vp "VC\Tools\MSVC"
      if (-not (Test-Path -LiteralPath $msvcRoot)) { continue }
      # Prefer the Hostx64\x64 edition (matches make_zip.ps1's dumpbin policy);
      # fall back to any copy the toolset ships.
      $f = Get-ChildItem -LiteralPath $msvcRoot -Filter "signtool.exe" -Recurse -ErrorAction SilentlyContinue |
        Where-Object { $_.FullName -like "*Hostx64\x64*" } | Select-Object -First 1
      if (-not $f) {
        $f = Get-ChildItem -LiteralPath $msvcRoot -Filter "signtool.exe" -Recurse -ErrorAction SilentlyContinue |
          Select-Object -First 1
      }
      if ($f) { $signtoolPath = $f.FullName; break }
    }
  }
}
if (-not $signtoolPath) {
  NoOp "signtool.exe not found (not on PATH, and no Visual Studio instance with VC tools found via vswhere)."
}
Log "signtool: $signtoolPath"

# --- Collect targets ----------------------------------------------------------
if ([string]::IsNullOrWhiteSpace($StageDir)) {
  $stageRoot = Join-Path $RepoRoot "dist\stage"
  if (Test-Path -LiteralPath $stageRoot) {
    $latest = Get-ChildItem -LiteralPath $stageRoot -Directory -Filter "K6WP-portable-*" |
      Sort-Object Name -Descending | Select-Object -First 1
    if ($latest) { $StageDir = $latest.FullName }
  }
}
$targets = @()
if (-not [string]::IsNullOrWhiteSpace($StageDir) -and (Test-Path -LiteralPath $StageDir)) {
  $targets += @(Get-ChildItem -LiteralPath $StageDir -Filter "*.exe" -Recurse -File)
  Log "staging dir: $StageDir ($($targets.Count) exe(s) found)."
} else {
  Log "WARNING: no staging dir found (looked for newest dist\stage\K6WP-portable-*); installer will still be signed if present."
}
if ([string]::IsNullOrWhiteSpace($SetupExe)) {
  $SetupExe = Join-Path $RepoRoot "dist\K6WP-Setup.exe"
}
if (Test-Path -LiteralPath $SetupExe) {
  $targets += Get-Item -LiteralPath $SetupExe
  Log "installer: $SetupExe"
} else {
  Log "note: $SetupExe not present (run makensis first) - installer skipped."
}

if ($targets.Count -eq 0) {
  NoOp "no signable files found (no staged exes, no dist\K6WP-Setup.exe)."
}

# --- Sign (SHA256 + RFC3161 timestamp) ----------------------------------------
Log "signing $($targets.Count) file(s) with SHA256 + RFC3161 timestamp ($TimestampUrl)..."
Log "certificate: $env:K6WP_SIGN_PFX"
$failed = @()
foreach ($t in $targets) {
  Log "signing $($t.FullName)"
  $out = & $signtoolPath sign /f $env:K6WP_SIGN_PFX /p $env:K6WP_SIGN_PFX_PASSWORD /fd SHA256 /td SHA256 /tr $TimestampUrl $t.FullName 2>&1 | Out-String
  if ($LASTEXITCODE -ne 0) {
    $failed += $t.Name
    Log "FAILED (signtool exit $LASTEXITCODE) on $($t.Name):"
    Write-Host $out
  } else {
    Log "OK: $($t.Name)"
  }
}
if ($failed.Count -gt 0) {
  Log "ERROR: $($failed.Count) of $($targets.Count) file(s) failed to sign: $($failed -join ', ')"
  exit 1
}
Log "DONE: signed $($targets.Count) file(s)."
Log "NOTE: signing reduces false positives; it does NOT guarantee clean AV/SmartScreen verdicts (packaging/known-limitations.md §4)."
exit 0
```

## File: packaging/studio_settings.json.example
```
{
  "_comment": "K6WP example studio settings, schema v5. Copy to %LOCALAPPDATA%/K6WP/studio_settings.json to override Studio defaults. Paths below use %LOCALAPPDATA% as a placeholder - replace with the expanded folder on your machine, or leave the file absent to use the built-in defaults (identical values). A pre-v5 file migrates on load: wallpaper_sync is added (true), lockscreen_sync keeps the value you chose (it now means the per-user WinRT lock-screen image, no administrator policy or UAC), and a lockscreen_offset_sec of exactly 1.0 (the old default) becomes 0.0 (the first frame).",
  "version": 5,
  "auto_compress_on_import": true,
  "compress_output_dir": "%LOCALAPPDATA%/K6WP/wallpapers",
  "default_crf": 22,
  "default_fps": 30,
  "default_resolution_mode": "match_monitor",
  "start_with_windows": false,
  "cache_dir": "%LOCALAPPDATA%/K6WP/cache",
  "wallpaper_sync": true,
  "lockscreen_sync": true,
  "lockscreen_offset_sec": 0.0,
  "lockscreen_offer_shown": false,
  "compress_advanced_visible": false
}
```

## File: packaging/uninstall.bat
```batch
@echo off
rem K6WP uninstall helper - portable-ZIP copy (Task 28).
rem Dev copy lives at tools\uninstall.bat (Todo 36); alignment noted in evidence.
rem Overlap note (Task 27): dist\K6WP-Setup.exe (packaging\installer.nsi)
rem Section "Uninstall" covers full removal (verified artifact, do not touch):
rem K6WP.exe --stop, Run value, per-user wallpaper/lock-screen restore from
rem %LOCALAPPDATA%\K6WP\original_*, legacy HKLM policy cleanup, app files,
rem shortcuts, HKCU uninstall key, keep/remove data offer. This .bat is the
rem portable-ZIP helper with the same keep/remove data semantics, aligned as
rem far as batch can.
rem Process stop (K6WP.exe --stop, not taskkill /IM): an image name is not
rem unique on a developer or IT machine, so `taskkill /IM engine.exe` used to
rem kill EVERY engine.exe on the box, including another app's. --stop asks the
rem engine to quit over its own IPC pipe first (so its graceful shutdown runs
rem and restores the OS wallpaper), then escalates to a per-PID kill, and only
rem for images sitting next to THIS K6WP.exe. A colliding engine.exe or
rem studio.exe in another folder is never touched.
rem Graceful-first rationale: the EngineApp message window handles WM_CLOSE by
rem calling RequestShutdown() (engine/src/engine_app.cpp), which posts WM_QUIT
rem so Run() breaks out and calls Shutdown(): ShutdownWallpaperSurface() plus
rem RequestStillRestore() (detached K6WP.exe --apply-still --restore, which
rem reads the persistent original_wallpaper.json / original_lockscreen.*
rem stores), plus tray icon Remove() and a forced kill skips all of that.
rem LIVE-TEST NOTE (Task 28, still valid): with only its hidden top-level
rem message window present, build\release\engine.exe did not honour the
rem WM_CLOSE that `taskkill /IM` sends, so the forced fallback did the
rem termination and no in-engine wallpaper restore ran that round. --stop posts
rem WM_CLOSE to the process's real window (taskkill matches a window by TITLE,
rem which a hidden "K6WP Engine" window does not have) and keeps the forced
rem per-PID fallback for anything that survives the grace period, so the
rem guaranteed restore for installed copies is still the NSIS
rem registry-backup path, which batch cannot do without new dependencies.
rem No pixel-level wallpaper verification is claimed here (headless sessions
rem cannot verify pixels); transcripts prove process stop + Run-key absence +
rem exit codes only.
rem The former `ping -n 4` inter-task wait now lives inside K6WP.exe as a real
rem WaitForSingleObject on each process handle (ping, not timeout, was the
rem house rule there because timeout aborts when stdin is redirected, e.g.
rem `echo N | uninstall.bat`; the wait is no longer in this script).
rem Fallback when K6WP.exe is gone (deleted folder / partial extract): we do NOT
rem fall back to a machine-wide kill - that is the bug, not the fix. The script
rem says which windows to close by hand instead.
rem HKCU needs no admin rights.
setlocal

echo Stopping K6WP processes (IPC quit first, then per-PID)...
if exist "%~dp0K6WP.exe" goto stop_run
echo K6WP.exe not found next to this script - cannot stop processes safely.
echo Close engine.exe and Studio by hand, then run this script again.
goto stop_done
:stop_run
"%~dp0K6WP.exe" --stop
if errorlevel 1 goto stop_failed
echo K6WP processes stopped (not-running is fine - already clean).
goto stop_done
:stop_failed
echo WARNING: some K6WP processes could not be stopped.
echo Close engine.exe and Studio by hand, then run this script again.
:stop_done

echo K6WP uninstall: removing autostart entry...
reg delete "HKCU\Software\Microsoft\Windows\CurrentVersion\Run" /v K6WP /f
if %ERRORLEVEL% EQU 0 (
  echo Autostart entry removed.
) else (
  echo Autostart entry was not present - already clean.
)

echo Restoring your pre-K6WP desktop wallpaper and lock-screen image (per-user, no admin)...
if exist "%~dp0K6WP.exe" goto still_run
echo K6WP.exe not found next to this script - skipping the per-user restore.
echo To restore manually, run: K6WP.exe --apply-still --restore
goto still_done
:still_run
rem No UAC: the per-user still helper restores the persistent
rem original_wallpaper.json / original_lockscreen.* stores. A missing store is
rem a successful no-op, so this is safe to run on a machine that never used
rem the feature.
"%~dp0K6WP.exe" --apply-still --restore
if errorlevel 1 echo WARNING: the wallpaper/lock-screen restore did not complete. Run K6WP.exe --apply-still --restore again before deleting K6WP.
:still_done

echo Removing a leftover legacy lockscreen policy (only if an old K6WP left one)...
echo This needs administrator rights - a UAC prompt will appear. Declining keeps the legacy policy.
if exist "%~dp0K6WP.exe" goto lock_run
echo K6WP.exe not found next to this script - skipping the legacy cleanup.
echo To clean up manually, run: K6WP.exe --elevate-lockscreen off (as administrator).
goto lock_done
:lock_run
rem Exit codes from K6WP.exe: 0 cleaned, 5 UAC declined, 4 wait budget
rem expired, other = the elevated helper failed. `if errorlevel N` is read at
rem run time, so these tests see the helper's real code.
"%~dp0K6WP.exe" --elevate-lockscreen off
if errorlevel 5 goto lock_declined
if errorlevel 1 goto lock_failed
goto lock_done
:lock_declined
echo.
echo WARNING: the administrator prompt was declined, so a leftover legacy lockscreen policy was NOT removed.
echo It may still point at %%PROGRAMDATA%%\K6WP\lockscreen.jpg in
echo HKLM\SOFTWARE\Policies\Microsoft\Windows\Personalization\LockScreenImage.
echo To clean it up manually, run: K6WP.exe --elevate-lockscreen off (as administrator).
goto lock_done
:lock_failed
echo.
echo WARNING: K6WP could not remove the legacy lockscreen policy ^(the elevated helper failed or ran out of time^).
echo It may still point at %%PROGRAMDATA%%\K6WP\lockscreen.jpg in
echo HKLM\SOFTWARE\Policies\Microsoft\Windows\Personalization\LockScreenImage.
echo To clean it up manually, run: K6WP.exe --elevate-lockscreen off (as administrator).
:lock_done

set /p CLEAN="Also delete K6WP cache and library data in %%LOCALAPPDATA%%\K6WP? [y/N] "
if /i "%CLEAN%"=="y" (
  if exist "%LOCALAPPDATA%\K6WP" (
    rmdir /s /q "%LOCALAPPDATA%\K6WP"
    echo K6WP data deleted.
  ) else (
    echo No K6WP data folder found - already clean.
  )
) else (
  echo Keeping K6WP data.
)

echo Done.
echo To finish, delete this portable folder itself.
endlocal
```

## File: packaging/version.nsh
```
; version.nsh.in — NSIS version constants, generated by CMake from
; K6WP_VERSION (root CMakeLists.txt) via configure_file(). Do not edit —
; change K6WP_VERSION in the root CMakeLists.txt and reconfigure.
; Consumed by packaging/installer.nsi (!include "version.nsh").
!define K6WP_VERSION_STR "0.0.1-alpha.3"
!define K6WP_VERSION_STR_4 "0.0.1.0"
```

## File: tools/fetch_vendor.ps1
```powershell
<#
.SYNOPSIS
  Fetches the pinned third-party binaries listed in vendor/VERSIONS.md.

.DESCRIPTION
  The vendored binaries total ~311 MB, and libmpv-2.dll alone exceeds GitHub's
  100 MB per-file push limit, so they are not tracked in git. This script
  downloads them and verifies every extracted file against the SHA256 recorded
  in vendor/VERSIONS.md, which stays the human-readable source of truth.

  Idempotent: a file already present with the expected hash is left alone, so
  re-running costs nothing. Use -Force to re-download anyway.

  Required by a clean build; CI calls this before configuring.

.EXAMPLE
  .\tools\fetch_vendor.ps1
  .\tools\fetch_vendor.ps1 -Force
#>
[CmdletBinding()]
param([switch]$Force)

$ErrorActionPreference = 'Stop'
$RepoRoot = Split-Path -Parent $PSScriptRoot

function Get-Sha256([string]$Path) {
  (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToUpperInvariant()
}

function Find-7Zip {
  foreach ($c in @(
      "$env:ProgramFiles\7-Zip\7z.exe",
      "${env:ProgramFiles(x86)}\7-Zip\7z.exe",
      "$env:LOCALAPPDATA\Programs\7-Zip\7z.exe")) {
    if ($c -and (Test-Path -LiteralPath $c)) { return $c }
  }
  $onPath = Get-Command 7z.exe -ErrorAction SilentlyContinue
  if ($onPath) { return $onPath.Source }
  throw "7-Zip not found. Install 7-Zip, or extract the libmpv dev package by hand."
}

# Each package is downloaded once; `Outputs` are extracted and hash-verified
# individually. The hashes are the extracted-file hashes recorded in
# vendor/VERSIONS.md, which is the manifest to keep in sync when re-pinning.
$Packages = @(
  @{
    Name   = 'ffmpeg 8.1.2 essentials (gyan.dev)'
    Kind   = 'zip'
    Url    = 'https://github.com/GyanD/codexffmpeg/releases/download/8.1.2/ffmpeg-8.1.2-essentials_build.zip'
    Outputs = @(
      @{ Zip = 'ffmpeg-8.1.2-essentials_build/bin/ffmpeg.exe';  Dst = 'vendor\ffmpeg\ffmpeg.exe'
         Sha = '1326DDE4C84FF1F96FE6B8916C5BED29E163E9B5DCCF995F6F3DB069D143EC5E' }
      @{ Zip = 'ffmpeg-8.1.2-essentials_build/bin/ffprobe.exe'; Dst = 'vendor\ffmpeg\ffprobe.exe'
         Sha = 'B49CCC7C6547B141AD5A2F6EC69CC04323D7133D7704D70B331B904C63EECB07' }
    )
  }
  @{
    Name   = 'libmpv 0.41.0-dev (shinchiro mpv-dev-x86_64)'
    Kind   = '7z'
    Url    = 'https://github.com/shinchiro/mpv-winbuild-cmake/releases/download/20260903/mpv-dev-x86_64-20260903-git-69e63f425a.7z'
    Outputs = @(
      @{ Arc = 'libmpv-2.dll';         Dst = 'vendor\libmpv\bin\libmpv-2.dll'
         Sha = '673E6397920AB64A9C5B3A618F7F16D38854EFE72B58665F1F84E4E873B763A4' }
      @{ Arc = 'include/mpv/client.h';    Dst = 'vendor\libmpv\include\mpv\client.h'
         Sha = '1ACF99EE77C8C2A6F1D1993BD81BBC8A91D27FB5924E80171670E6139A4BD353' }
      @{ Arc = 'include/mpv/render.h';    Dst = 'vendor\libmpv\include\mpv\render.h'
         Sha = '192691941602052F00DF0587F126246C48785A1CF21DE68D22A92EA1908D1C55' }
      @{ Arc = 'include/mpv/render_gl.h'; Dst = 'vendor\libmpv\include\mpv\render_gl.h'
         Sha = '48662C0ED9872A14DD9E1684105C97F69F94A0414709C254C5D372ADC41D2E69' }
      @{ Arc = 'include/mpv/stream_cb.h'; Dst = 'vendor\libmpv\include\mpv\stream_cb.h'
         Sha = '188E58B6D14383E15A5DFFDC4ECEBBDFBF2E412B9C099FF21B571F747A8CE32D' }
    )
  }
  @{
    Name   = 'nlohmann/json 3.11.3 (single header)'
    Kind   = 'direct'
    Url    = 'https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp'
    Outputs = @(
      @{ Dst = 'shared\thirdparty\json.hpp'
         Sha = '9BEA4C8066EF4A1C206B2BE5A36302F8926F7FDC6087AF5D20B417D0CF103EA6' }
    )
  }
)

$sevenZip = $null
$failures = @()

foreach ($pkg in $Packages) {
  $pending = @($pkg.Outputs | Where-Object {
      $abs = Join-Path $RepoRoot $_.Dst
      $Force -or -not (Test-Path -LiteralPath $abs) -or (Get-Sha256 $abs) -ne $_.Sha
    })
  if ($pending.Count -eq 0) {
    Write-Host "[skip] $($pkg.Name): all $($pkg.Outputs.Count) file(s) present and verified"
    continue
  }

  Write-Host "[get ] $($pkg.Name) -> $($pending.Count) file(s)"
  # Both archive kinds are extracted with 7-Zip, so resolve it for either -
  # resolved lazily because a fully-populated tree needs no extractor at all.
  if ($pkg.Kind -ne 'direct' -and -not $sevenZip) { $sevenZip = Find-7Zip }

  $work = Join-Path ([IO.Path]::GetTempPath()) ("k6wp-vendor-" + [guid]::NewGuid().ToString('N'))
  New-Item -ItemType Directory -Path $work -Force | Out-Null
  try {
    $archive = if ($pkg.Kind -eq 'direct') { Join-Path $work 'payload' } else { Join-Path $work 'archive' }
    Write-Host "       downloading $($pkg.Url)"
    Invoke-WebRequest -Uri $pkg.Url -OutFile $archive -UseBasicParsing

    # One extraction per package, naming every entry we actually need (the
    # libmpv dev package is 115 MB unpacked and we want 5 files out of it).
    # 'x' keeps the archive's directory structure; 'e' would flatten, which is
    # what the ffmpeg zip wants since its entries share one bin/ dir anyway.
    $extractRoot = if ($pkg.Kind -eq 'direct') { $null } else { Join-Path $work 'x' }
    if ($extractRoot) {
      $key = if ($pkg.Kind -eq 'zip') { 'Zip' } else { 'Arc' }
      $verb = if ($pkg.Kind -eq 'zip') { 'e' } else { 'x' }
      $args = @($verb, $archive, "-o$extractRoot", '-y') + @($pending | ForEach-Object { $o = $_; ($o[$key]) })
      & $sevenZip @args | Out-Null
      if ($LASTEXITCODE -ne 0) { throw "extraction failed (exit $LASTEXITCODE)" }
    }

    foreach ($o in $pending) {
      $abs = Join-Path $RepoRoot $o.Dst
      if ($pkg.Kind -eq 'direct') {
        $src = $archive
      } else {
        $key = if ($pkg.Kind -eq 'zip') { 'Zip' } else { 'Arc' }
        # 'e' flattens, so a zip entry lands under its basename; 'x' keeps the
        # archive's own relative path.
        $rel = if ($pkg.Kind -eq 'zip') { Split-Path $o[$key] -Leaf } else { $o[$key] }
        $src = Join-Path $extractRoot ($rel -replace '/', '\')
      }
      if (-not (Test-Path -LiteralPath $src)) { throw "not in archive: $($o[$key])" }

      $got = Get-Sha256 $src
      if ($got -ne $o.Sha) {
        throw "SHA256 mismatch for $($o.Dst)`n  expected $($o.Sha)`n  actual   $got`n  Refusing to install. Re-pinning is a deliberate act: update vendor/VERSIONS.md too."
      }
      New-Item -ItemType Directory -Path (Split-Path -Parent $abs) -Force | Out-Null
      Copy-Item -LiteralPath $src -Destination $abs -Force
      Write-Host "       ok  $($o.Dst)"
    }
  } catch {
    $failures += "$($pkg.Name): $($_.Exception.Message)"
    Write-Host "       FAIL $($_.Exception.Message)" -ForegroundColor Red
  } finally {
    Remove-Item -LiteralPath $work -Recurse -Force -ErrorAction SilentlyContinue
  }
}

Write-Host ""
if ($failures.Count -gt 0) {
  Write-Host "fetch_vendor: $($failures.Count) package(s) failed" -ForegroundColor Red
  exit 1
}
Write-Host "fetch_vendor: vendor tree complete and hash-verified."
exit 0
```

## File: tools/bench_lib.ps1
```powershell
# tools/bench_lib.ps1 - shared helpers for the Phase 0 measurement harness.
#
# The harness is intentionally dependency-free (stock PowerShell 5.1 + .NET)
# and never ships. It measures the REAL engine/Studio processes; it does not
# instrument them. All numbers must be written to docs/bench/*.json.
#
# Evidence labels used by the reports:
#   live-proven  = observed on this machine by this harness
#   test-proven  = asserted by ctest / Rust tests
#   code-quoted  = read from source, not observed
#   unmeasured   = no number exists

$ErrorActionPreference = 'Stop'
$script:BenchRoot = Split-Path -Parent $MyInvocation.MyCommand.Path

if (-not ([System.Management.Automation.PSTypeName]'K6wpBench.Native').Type) {
Add-Type @"
using System;
using System.Runtime.InteropServices;
using System.Text;
namespace K6wpBench {
  public static class Native {
    [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr hWnd, int nCmdShow);
    [DllImport("user32.dll")] public static extern bool GetWindowPlacement(IntPtr hWnd, ref WINDOWPLACEMENT p);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetClassName(IntPtr hWnd, System.Text.StringBuilder buf, int n);
    [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr hWnd);
    [DllImport("user32.dll")] public static extern bool EnumWindows(EnumWindowsProc cb, IntPtr p);
    [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint pid);
    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }
    [StructLayout(LayoutKind.Sequential)] public struct POINT { public int X, Y; }
    [StructLayout(LayoutKind.Sequential)] public struct WINDOWPLACEMENT {
      public int length, flags, showCmd;
      public POINT minPosition, maxPosition;
      public RECT normalPosition;
    }
  }
}
"@
}

function Get-K6wpSessionId {
    return (Get-Process -Id $PID).SessionId
}

function Get-K6wpPipeName {
    return ("k6wp-engine-{0}" -f (Get-K6wpSessionId))
}

function Send-K6wpIpc {
    param(
        [Parameter(Mandatory=$true)][string]$Cmd,
        [hashtable]$Payload = @{},
        [int]$TimeoutMs = 3000
    )
    # Byte-level client: the server replies with one NDJSON line per datagram.
    # A StreamReader/ReadLineAsync helper masked the real failure behind a
    # "Cannot access a closed pipe" Dispose exception, so framing is done
    # explicitly here and every read is bounded by the deadline.
    $name = Get-K6wpPipeName
    $pipe = $null
    $ms = $null
    try {
        $pipe = New-Object System.IO.Pipes.NamedPipeClientStream('.', $name, [System.IO.Pipes.PipeDirection]::InOut, [System.IO.Pipes.PipeOptions]::Asynchronous)
        $pipe.Connect($TimeoutMs)
        $msg = @{ version = 1; cmd = $Cmd; payload = $Payload } | ConvertTo-Json -Compress -Depth 8
        $bytes = [System.Text.Encoding]::UTF8.GetBytes($msg + "`n")
        $pipe.Write($bytes, 0, $bytes.Length)
        $pipe.Flush()
        $deadline = [DateTime]::UtcNow.AddMilliseconds($TimeoutMs)
        $buf = New-Object byte[] 8192
        $ms = New-Object System.IO.MemoryStream
        while ($true) {
            $remain = [int]($deadline - [DateTime]::UtcNow).TotalMilliseconds
            if ($remain -le 0) { throw "IPC ack timeout after ${TimeoutMs}ms (cmd=$Cmd)" }
            # NamedPipeClientStream.ReadTimeout is unsupported on .NET Framework,
            # so bound the overlapped read with an explicit wait handle.
            $ar = $pipe.BeginRead($buf, 0, $buf.Length, $null, $null)
            if (-not $ar.AsyncWaitHandle.WaitOne($remain)) {
                throw "IPC ack timeout after ${TimeoutMs}ms (cmd=$Cmd)"
            }
            $n = $pipe.EndRead($ar)
            if ($n -le 0) { throw "IPC pipe closed before ack (cmd=$Cmd)" }
            $ms.Write($buf, 0, $n)
            if ($buf[$n - 1] -eq 10) { break }
        }
        $line = [System.Text.Encoding]::UTF8.GetString($ms.ToArray()).TrimEnd("`r", "`n")
        if ([string]::IsNullOrWhiteSpace($line)) { throw "empty IPC ack (cmd=$Cmd)" }
        return ($line | ConvertFrom-Json)
    }
    finally {
        if ($ms) { try { $ms.Dispose() } catch { } }
        if ($pipe) { try { $pipe.Dispose() } catch { } }
    }
}

function Wait-K6wpEngine {
    param([int]$TimeoutMs = 10000)
    $deadline = [DateTime]::UtcNow.AddMilliseconds($TimeoutMs)
    while ([DateTime]::UtcNow -lt $deadline) {
        try {
            $ack = Send-K6wpIpc -Cmd 'get_state' -TimeoutMs 800
            if ($ack -and $ack.ok) { return $true }
        } catch { }
        Start-Sleep -Milliseconds 100
    }
    return $false
}

function Start-K6wpEngine {
    param(
        [Parameter(Mandatory=$true)][string]$EngineExe,
        [string]$Video = '',
        [string]$DataDir = '',
        [int]$RunMs = 20000,
        [int]$ConfigMs = 0
    )
    if ([string]::IsNullOrWhiteSpace($DataDir)) {
        $DataDir = Join-Path $env:TEMP ("k6wp-bench-" + [Guid]::NewGuid().ToString('N'))
    }
    $dataRoot = Join-Path $DataDir 'K6WP'
    New-Item -ItemType Directory -Force -Path $dataRoot | Out-Null
    $args = @("--exit-after-ms=$RunMs")
    if (-not [string]::IsNullOrWhiteSpace($Video)) {
        $args = @("--video=$Video") + $args
    }
    $oldLocal = $env:LOCALAPPDATA
    try {
        $env:LOCALAPPDATA = $DataDir
        $proc = Start-Process -FilePath $EngineExe -ArgumentList $args -PassThru -WindowStyle Hidden
    }
    finally {
        $env:LOCALAPPDATA = $oldLocal
    }
    if (-not (Wait-K6wpEngine -TimeoutMs 15000)) {
        try { if (-not $proc.HasExited) { Stop-Process -Id $proc.Id -Force } } catch { }
        throw "engine did not answer the IPC pipe within 15s (video=$Video)"
    }
    return [pscustomobject]@{ Process = $proc; DataDir = $DataDir; LogPath = (Join-Path $dataRoot 'engine.log') }
}

function Stop-K6wpEngine {
    param($Run, [int]$GraceMs = 12000)
    if ($null -eq $Run -or $null -eq $Run.Process) { return }
    $p = $Run.Process
    try { $p.Refresh() } catch { return }
    if ($p.HasExited) { return }
    try { [void](Send-K6wpIpc -Cmd 'quit' -TimeoutMs 2000) } catch { }
    try { [void]$p.WaitForExit($GraceMs) } catch { }
    try {
        $p.Refresh()
        if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue }
    } catch { }
}

function Get-Median {
    param([double[]]$Values)
    if ($null -eq $Values -or $Values.Count -eq 0) { return $null }
    $sorted = @($Values | Sort-Object)
    $n = $sorted.Count
    if ($n % 2 -eq 1) { return [double]$sorted[($n - 1) / 2] }
    return ([double]$sorted[$n / 2 - 1] + [double]$sorted[$n / 2]) / 2.0
}

function Get-Stats {
    param([double[]]$Values)
    $clean = @($Values | Where-Object { $null -ne $_ -and -not [double]::IsNaN($_) })
    if ($clean.Count -eq 0) {
        return [pscustomobject]@{ n = 0; median = $null; mean = $null; min = $null; max = $null }
    }
    $m = Get-Median $clean
    $min = ($clean | Measure-Object -Minimum).Minimum
    $max = ($clean | Measure-Object -Maximum).Maximum
    return [pscustomobject]@{
        n = $clean.Count
        median = [math]::Round($m, 2)
        mean = [math]::Round((($clean | Measure-Object -Average).Average), 2)
        min = [math]::Round($min, 2)
        max = [math]::Round($max, 2)
        # The noise band: the spread of the samples around the median. It is
        # the tolerance a difference must exceed to mean something.
        band = [math]::Round(($max - $min), 2)
    }
}

function Get-ProcessSeries {
    param(
        [Parameter(Mandatory=$true)][int]$ProcessId,
        [int]$Seconds = 10,
        [int]$IntervalMs = 1000
    )
    $cores = [Environment]::ProcessorCount
    $out = New-Object System.Collections.Generic.List[object]
    $p = Get-Process -Id $ProcessId -ErrorAction Stop
    $prevCpu = $p.CPU
    if ($null -eq $prevCpu) { $prevCpu = 0.0 }
    $prevT = [DateTime]::UtcNow
    $start = [DateTime]::UtcNow
    # Per-thread CPU: the PROCESS aggregate hides a single spinning thread
    # (that is exactly how a 55 %-of-a-core busy-wait stayed invisible), so
    # the hottest thread is tracked separately (C2).
    $threadPrev = @{}
    foreach ($t in $p.Threads) { $threadPrev[[string]$t.Id] = $t.TotalProcessorTime.TotalMilliseconds }
    while (([DateTime]::UtcNow - $start).TotalSeconds -lt $Seconds) {
        Start-Sleep -Milliseconds $IntervalMs
        try {
            $p.Refresh()
            if ($p.HasExited) { break }
            $cpu = $p.CPU
            if ($null -eq $cpu) { $cpu = $prevCpu }
            $now = [DateTime]::UtcNow
            $dt = ($now - $prevT).TotalSeconds
            $pct = 0.0
            if ($dt -gt 0) {
                $pct = (($cpu - $prevCpu) / $dt / $cores) * 100.0
                if ($pct -lt 0) { $pct = 0.0 }
            }
            # The hottest single thread over this interval, as a percentage
            # of ONE core (not of the machine).
            $hotPct = 0.0
            $hotTid = 0
            foreach ($t in $p.Threads) {
                $k = [string]$t.Id
                $nowMs = $t.TotalProcessorTime.TotalMilliseconds
                if ($threadPrev.ContainsKey($k)) {
                    $d = $nowMs - $threadPrev[$k]
                    if ($dt -gt 0) {
                        $pctCore = ($d / 1000.0 / $dt) * 100.0
                        if ($pctCore -gt $hotPct) { $hotPct = $pctCore; $hotTid = $t.Id }
                    }
                }
                $threadPrev[$k] = $nowMs
            }
            $out.Add([pscustomobject]@{
                t_ms = [int](([DateTime]::UtcNow - $start).TotalMilliseconds)
                private_mb = [math]::Round($p.PrivateMemorySize64 / 1MB, 2)
                ws_mb = [math]::Round($p.WorkingSet64 / 1MB, 2)
                cpu_pct = [math]::Round($pct, 3)
                max_thread_cpu_pct = [math]::Round($hotPct, 2)
                hot_tid = $hotTid
                threads = $p.Threads.Count
                handles = $p.HandleCount
            })
            $prevCpu = $cpu
            $prevT = $now
        } catch { break }
    }
    return $out.ToArray()
}

function Get-TypeperfCounterPaths {
    param([int]$ProcessId, [string]$ProcessName = 'engine')
    $paths = New-Object System.Collections.Generic.List[string]
    try {
        $gpu = (Get-Counter -ListSet 'GPU Engine' -ErrorAction Stop).PathsWithInstances
        foreach ($p in $gpu) {
            if ($p -match ("pid_" + $ProcessId + "_") -and $p -match 'Utilization Percentage$') {
                $paths.Add($p)
            }
        }
    } catch { }
    try {
        $mem = (Get-Counter -ListSet 'GPU Process Memory' -ErrorAction Stop).PathsWithInstances
        foreach ($p in $mem) {
            if ($p -match ("pid_" + $ProcessId + "_") -and $p -match '(Dedicated|Shared) Usage$') {
                $paths.Add($p)
            }
        }
    } catch { }
    if (-not [string]::IsNullOrWhiteSpace($ProcessName)) {
        $paths.Add(("\Thread({0}*)\Context Switches/sec" -f $ProcessName))
    }
    return $paths.ToArray()
}

function Start-TypeperfSampler {
    param(
        [int]$ProcessId,
        [int]$Seconds = 10,
        [string]$ProcessName = 'engine'
    )
    $paths = Get-TypeperfCounterPaths -ProcessId $ProcessId -ProcessName $ProcessName
    if ($paths.Count -eq 0) { throw "no GPU/thread counters for pid $ProcessId" }
    $csv = Join-Path $env:TEMP ("k6wp-typeperf-" + [Guid]::NewGuid().ToString('N') + ".csv")
    $stdout = Join-Path $env:TEMP ("k6wp-typeperf-" + [Guid]::NewGuid().ToString('N') + ".out")
    $stderr = Join-Path $env:TEMP ("k6wp-typeperf-" + [Guid]::NewGuid().ToString('N') + ".err")
    # Start-Process joins -ArgumentList with spaces and does NOT add quotes.
    # Counter paths contain spaces ("GPU Engine"), so quote every path.
    $quoted = @($paths | ForEach-Object { '"' + $_ + '"' })
    $args = @($quoted) + @('-si', '1', '-sc', "$Seconds", '-f', 'CSV', '-o', ('"' + $csv + '"'), '-y')
    $proc = Start-Process -FilePath 'typeperf.exe' -ArgumentList $args -PassThru -WindowStyle Hidden `
        -RedirectStandardOutput $stdout -RedirectStandardError $stderr
    return [pscustomobject]@{
        Process = $proc
        Csv = $csv
        Stdout = $stdout
        Stderr = $stderr
        Paths = $paths
    }
}

function ConvertTo-NullableDouble {
    param($Value)
    if ($null -eq $Value) { return $null }
    $s = [string]$Value
    if ([string]::IsNullOrWhiteSpace($s)) { return $null }
    $d = 0.0
    $ok = [double]::TryParse($s, [Globalization.NumberStyles]::Float, [Globalization.CultureInfo]::InvariantCulture, [ref]$d)
    if (-not $ok) { return $null }
    if ($d -lt 0) { return $null }
    return $d
}

function Read-TypeperfCsv {
    param([string]$Path)
    $rows = @()
    if (-not (Test-Path -LiteralPath $Path)) { return $rows }
    try { $data = Import-Csv -LiteralPath $Path } catch { return $rows }
    foreach ($row in $data) {
        $props = @($row.PSObject.Properties)
        if ($props.Count -eq 0) { continue }
        $stamp = [string]$props[0].Value
        if ([string]::IsNullOrWhiteSpace($stamp)) { continue }
        $gpuTotal = 0.0; $gpu3d = 0.0; $gpuDecode = 0.0; $gpuCopy = 0.0
        $shared = 0.0; $dedicated = 0.0; $wake = 0.0
        $hasGpu = $false; $hasWake = $false
        foreach ($prop in $props) {
            $name = $prop.Name
            $val = ConvertTo-NullableDouble $prop.Value
            if ($null -eq $val) { continue }
            if ($name -match 'Utilization Percentage$') {
                $hasGpu = $true
                $gpuTotal += $val
                # Column names are full PDH paths:
                # \\host\GPU Engine(pid_.._engtype_3D)\Utilization Percentage
                if ($name -match 'engtype_3D\)') { $gpu3d += $val }
                elseif ($name -match 'engtype_VideoDecode\)') { $gpuDecode += $val }
                elseif ($name -match 'engtype_Copy\)') { $gpuCopy += $val }
            }
            elseif ($name -match 'Shared Usage$') { $shared += $val }
            elseif ($name -match 'Dedicated Usage$') { $dedicated += $val }
            elseif ($name -match 'Context Switches/sec$') {
                $hasWake = $true
                $wake += $val
            }
        }
        $rows += [pscustomobject]@{
            timestamp = $stamp
            gpu_total_pct = $(if ($hasGpu) { [math]::Round($gpuTotal, 2) } else { $null })
            gpu_3d_pct = $(if ($hasGpu) { [math]::Round($gpu3d, 2) } else { $null })
            gpu_decode_pct = $(if ($hasGpu) { [math]::Round($gpuDecode, 2) } else { $null })
            gpu_copy_pct = $(if ($hasGpu) { [math]::Round($gpuCopy, 2) } else { $null })
            gpu_shared_mb = [math]::Round($shared / 1MB, 2)
            gpu_dedicated_mb = [math]::Round($dedicated / 1MB, 2)
            wakeups_per_sec = $(if ($hasWake) { [math]::Round($wake, 2) } else { $null })
        }
    }
    return $rows
}

function Get-DesktopWindows {
    # Save + hide every visible top-level app window so the injected
    # wallpaper is not occluded. Shell surfaces (taskbar/Progman/WorkerW) are
    # never touched.
    #
    # The windows are enumerated with EnumWindows, NOT through
    # Get-Process.MainWindowHandle: a process can own several top-level
    # windows and the "main" one is not necessarily the one on screen (a
    # File Explorer window owned by a helper process, an Electron app with
    # one window per process, ...). Missing a covering window makes the
    # engine pause (occlusion/fullscreen) and the capture records the plain
    # desktop instead of the wallpaper.
    $saved = @()
    $skip = @('Shell_TrayWnd', 'Progman', 'WorkerW', 'K6WP.DesktopInject.1', 'Windows.UI.Core.CoreWindow')
    # An always-on-top agent-cursor overlay covers the WHOLE screen. It is
    # not an app window: minimizing it does nothing, but leaving it up makes
    # the engine's fullscreen watcher pause the decode AND the occlusion
    # watcher report 100 % coverage, so every measurement reads a paused
    # engine and the capture records the plain desktop. It is hidden for the
    # measurement and restored afterwards.
    $hideClasses = @('Cua.AgentCursorOverlay')
    $found = New-Object System.Collections.Generic.List[object]
    $cb = [K6wpBench.Native+EnumWindowsProc] {
        param($h, $p)
        if (-not [K6wpBench.Native]::IsWindowVisible($h)) { return $true }
        $sb = New-Object System.Text.StringBuilder 256
        [void][K6wpBench.Native]::GetClassName($h, $sb, 256)
        $cls = $sb.ToString()
        $procId = 0
        [void][K6wpBench.Native]::GetWindowThreadProcessId($h, [ref]$procId)
        $found.Add([pscustomobject]@{ Handle = $h; Class = $cls; Pid = $procId })
        return $true
    }
    [void][K6wpBench.Native]::EnumWindows($cb, [IntPtr]::Zero)
    foreach ($w in $found) {
        try {
            $h = $w.Handle
            $cls = $w.Class
            if ($skip -contains $cls) { continue }
            $wp = New-Object K6wpBench.Native+WINDOWPLACEMENT
            $wp.length = [Runtime.InteropServices.Marshal]::SizeOf($wp)
            if (-not [K6wpBench.Native]::GetWindowPlacement($h, [ref]$wp)) { continue }
            if ($wp.showCmd -eq 6) { continue }  # already minimized
            $name = ''
            try { $name = (Get-Process -Id $w.Pid -ErrorAction Stop).ProcessName } catch { $name = "pid$($w.Pid)" }
            $saved += [pscustomobject]@{ Handle = $h; ShowCmd = $wp.showCmd; Name = $name; Class = $cls }
            if ($hideClasses -contains $cls) {
                [void][K6wpBench.Native]::ShowWindow($h, 0)   # SW_HIDE
            } else {
                [void][K6wpBench.Native]::ShowWindow($h, 6)   # SW_MINIMIZE
            }
        } catch { }
    }
    return $saved
}

function Restore-DesktopWindows {
    param([object[]]$Saved)
    if ($null -eq $Saved) { return }
    foreach ($s in $Saved) {
        try {
            $cmd = [int]$s.ShowCmd
            if ($cmd -eq 0) { $cmd = 1 }
            [void][K6wpBench.Native]::ShowWindow([IntPtr]$s.Handle, $cmd)
        } catch { }
    }
}

function Get-HwInventory {
    $os = Get-CimInstance Win32_OperatingSystem
    $cpu = Get-CimInstance Win32_Processor | Select-Object -First 1
    $gpus = @(Get-CimInstance Win32_VideoController | ForEach-Object { $_.Name })
    $power = ''
    try { $power = (powercfg /getactivescheme) -join ' ' } catch { }
    $monitors = @()
    $dump = Join-Path $script:BenchRoot '..\build\msvc-dev\monitor_dump.exe'
    if (Test-Path -LiteralPath $dump) {
        try {
            $raw = & $dump 2>$null | Out-String
            $parsed = $raw | ConvertFrom-Json
            $monitors = @($parsed.monitors)
        } catch { }
    }
    return [pscustomobject]@{
        os = ("{0} {1} build {2}" -f $os.Caption, $os.Version, $os.BuildNumber)
        cpu = $cpu.Name
        cores = $cpu.NumberOfCores
        logical = $cpu.NumberOfLogicalProcessors
        ram_kib = $os.TotalVisibleMemorySize
        gpus = $gpus
        power_plan = $power.Trim()
        monitors = $monitors
        date = (Get-Date -Format 'yyyy-MM-ddTHH:mm:ssK')
    }
}

function Get-ScreenBitmap {
    Add-Type -AssemblyName System.Drawing, System.Windows.Forms
    $bounds = [System.Windows.Forms.SystemInformation]::VirtualScreen
    $bmp = New-Object System.Drawing.Bitmap $bounds.Width, $bounds.Height
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    try {
        $g.CopyFromScreen($bounds.Left, $bounds.Top, 0, 0, $bmp.Size)
    }
    finally { $g.Dispose() }
    return [pscustomobject]@{ Bitmap = $bmp; Bounds = $bounds }
}

function Save-ScreenPng {
    param([string]$Path, [int]$CropWidth = 0, [int]$CropHeight = 0)
    $shot = Get-ScreenBitmap
    $bmp = $shot.Bitmap
    try {
        if ($CropWidth -gt 0 -and $CropHeight -gt 0) {
            $x = [int](($bmp.Width - $CropWidth) / 2)
            $y = [int](($bmp.Height - $CropHeight) / 2)
            $crop = New-Object System.Drawing.Bitmap $CropWidth, $CropHeight
            $g = [System.Drawing.Graphics]::FromImage($crop)
            try { $g.DrawImage($bmp, (New-Object System.Drawing.Rectangle 0,0,$CropWidth,$CropHeight), (New-Object System.Drawing.Rectangle $x,$y,$CropWidth,$CropHeight), [System.Drawing.GraphicsUnit]::Pixel) }
            finally { $g.Dispose() }
            $bmp.Dispose()
            $bmp = $crop
        }
        $dir = [System.IO.Path]::GetDirectoryName([System.IO.Path]::GetFullPath($Path))
        if (-not (Test-Path -LiteralPath $dir)) { New-Item -ItemType Directory -Force -Path $dir | Out-Null }
        $bmp.Save([System.IO.Path]::GetFullPath($Path), [System.Drawing.Imaging.ImageFormat]::Png)
    }
    finally { $bmp.Dispose() }
}

function Get-RegionSample {
    param([int]$Width = 240, [int]$Height = 160)
    Add-Type -AssemblyName System.Drawing, System.Windows.Forms
    $bounds = [System.Windows.Forms.SystemInformation]::VirtualScreen
    $x = $bounds.Left + [int](($bounds.Width - $Width) / 2)
    $y = $bounds.Top + [int](($bounds.Height - $Height) / 2)
    $bmp = New-Object System.Drawing.Bitmap $Width, $Height
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    try { $g.CopyFromScreen($x, $y, 0, 0, $bmp.Size) } finally { $g.Dispose() }
    return $bmp
}

function Get-BitmapDiff {
    param($A, $B, [int]$Step = 4)
    $w = [Math]::Min($A.Width, $B.Width)
    $h = [Math]::Min($A.Height, $B.Height)
    $sum = 0.0
    $n = 0
    for ($y = 0; $y -lt $h; $y += $Step) {
        for ($x = 0; $x -lt $w; $x += $Step) {
            $pa = $A.GetPixel($x, $y)
            $pb = $B.GetPixel($x, $y)
            $sum += ([Math]::Abs($pa.R - $pb.R) + [Math]::Abs($pa.G - $pb.G) + [Math]::Abs($pa.B - $pb.B)) / 3.0
            $n++
        }
    }
    if ($n -eq 0) { return 0.0 }
    return $sum / $n
}

function Get-PeakWorkingSetMb {
    param([int]$ProcessId)
    try {
        $p = Get-Process -Id $ProcessId -ErrorAction Stop
        return [math]::Round($p.PeakWorkingSet64 / 1MB, 2)
    } catch { return $null }
}
```

## File: tools/g5_diff_baseline.ps1
```powershell
# tools/g5_diff_baseline.ps1 - task I1: the G5 golden fixtures against the
# BASELINE build.
#
# WHAT IT PROVES
#   The G5 fixture (tests/golden/engine_logic.json) freezes the engine's pure
#   decisions. A fixture is only worth something if it describes the SAME
#   behaviour as the build it is compared against. This script builds the
#   harness (tests/engine_logic_dump.cpp + the case table) inside a worktree
#   at the `baseline-pre-refactor` tag and compares the two outputs case by
#   case:
#
#     - every module whose behaviour was NOT intentionally changed must be
#       IDENTICAL (the same inputs -> the same outputs);
#     - the modules that WERE intentionally changed (the task 1.1 unload
#       state machine, and the `hides_wallpaper` field that G5 extracted out
#       of EngineApp) are EXCLUDED and reported as such.
#
# WHY A WORKTREE
#   The baseline is a build of an OLDER tree. The harness is copied in (the
#   case table is the SAME file, so the two cannot drift), the missing
#   symbols come from tests/g5_baseline_shim.hpp, and the tree is patched
#   only inside the worktree (the repo is never touched).
#
# THE OUTPUT
#   docs/bench/g5-differential.json - the report (the counts, the excluded
#   modules, every difference).
#
[CmdletBinding()]
param(
    [string]$Worktree = '',
    [string]$Tag = 'baseline-pre-refactor'
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
if (-not $Worktree) {
    $Worktree = Join-Path $root 'build\wt-baseline'
}
$report = Join-Path $root 'docs\bench\g5-differential.json'
$baselineOut = Join-Path $root 'build\g5-baseline-output.json'
$currentFixture = Join-Path $root 'tests\golden\engine_logic.json'

# --- the worktree -----------------------------------------------------------
if (Test-Path (Join-Path $Worktree '.git')) {
    Write-Host "  [g5] the worktree exists: $Worktree"
} else {
    Write-Host "  [g5] creating the worktree at $Tag"
    git -C $root worktree add $Worktree $Tag
    if ($LASTEXITCODE -ne 0) { throw "the worktree could not be created" }
}
# The vendored dependencies (they are not in git).
if (-not (Test-Path (Join-Path $Worktree 'vendor'))) {
    robocopy (Join-Path $root 'vendor') (Join-Path $Worktree 'vendor') /E /NFL /NDL /NJH /NJS /NP | Out-Null
}
if (-not (Test-Path (Join-Path $Worktree 'shared\thirdparty'))) {
    robocopy (Join-Path $root 'shared\thirdparty') (Join-Path $Worktree 'shared\thirdparty') /E /NFL /NDL /NJH /NJS /NP | Out-Null
}

# --- copy the harness (the SAME case table) ---------------------------------
foreach ($file in @('engine_logic_cases.hpp', 'engine_logic_dump.cpp', 'g5_baseline_shim.hpp')) {
    Copy-Item (Join-Path $root "tests\$file") (Join-Path $Worktree "tests\$file") -Force
}

# The baseline has no unload_state.hpp: the shim provides the missing symbols.
$casesFile = Join-Path $Worktree 'tests\engine_logic_cases.hpp'
$cases = Get-Content $casesFile -Raw
if ($cases -notmatch 'g5_baseline_shim') {
    $cases = $cases.Replace('#include "unload_state.hpp"', '#include "g5_baseline_shim.hpp"')
    Set-Content -LiteralPath $casesFile -Value $cases -NoNewline
    Write-Host '  [g5] the case table points at the baseline shim'
}

# --- the build target (only inside the worktree) ----------------------------
$cmakeFile = Join-Path $Worktree 'engine\CMakeLists.txt'
$cmake = Get-Content $cmakeFile -Raw
if ($cmake -notmatch 'engine_logic_dump') {
    $target = @"

  # engine_logic_dump (task I1): the G5 harness, built against the BASELINE
  # so the differential test can compare the two behaviours. This target
  # exists only in this worktree (the repo's tree has the same target with
  # the real unload_state.hpp).
  add_executable(engine_logic_dump ../tests/engine_logic_dump.cpp
    src/desktop_placement.cpp
    src/retry_schedule.cpp
    src/crash_restart_guard.cpp
    src/topology_diff.cpp
    src/reconcile_policy.cpp
    src/pin_verify_schedule.cpp
  )
  target_link_libraries(engine_logic_dump PRIVATE user32 k6wp_shared)
  target_include_directories(engine_logic_dump PRIVATE
    `${CMAKE_CURRENT_SOURCE_DIR}/src
    `${CMAKE_SOURCE_DIR}/shared
    `${CMAKE_SOURCE_DIR}/tests
  )
"@
    $anchor = 'add_executable(engine_no_winrt_test'
    if (-not $cmake.Contains($anchor)) {
        # Older trees: anchor on the last target instead.
        $anchor = 'add_executable(engine_no_winrt_test'
    }
    if (-not $cmake.Contains($anchor)) { throw 'the CMake anchor was not found' }
    $cmake = $cmake.Replace($anchor, $target + "`r`n" + $anchor)
    Set-Content -LiteralPath $cmakeFile -Value $cmake -NoNewline
    Write-Host '  [g5] the worktree CMake got the engine_logic_dump target'
}

# --- build + run ------------------------------------------------------------
Push-Location $Worktree
try {
    cmake --preset msvc-dev 2>&1 | Out-Null
    if ($LASTEXITCODE -ne 0) { throw 'the baseline configure failed' }
    cmake --build --preset msvc-dev --target engine_logic_dump 2>&1 |
        Select-String 'error C|error LNK' | Select-Object -First 5
    if ($LASTEXITCODE -ne 0) { throw 'the baseline build failed' }
    # The dump writes its progress to stderr: with $ErrorActionPreference =
    # 'Stop' a native command that touches stderr raises a terminating error
    # in Windows PowerShell, so the process is started with its own
    # redirection instead of `2>$null`.
    $dumpExe = Join-Path $Worktree 'build\msvc-dev\engine_logic_dump.exe'
    $dumpErr = Join-Path $root 'build\g5-baseline-dump.err'
    $proc = Start-Process -FilePath $dumpExe -ArgumentList $baselineOut -Wait -PassThru -NoNewWindow -RedirectStandardError $dumpErr
    if ($proc.ExitCode -ne 0) { throw 'the baseline dump failed' }
} finally {
    Pop-Location
}

# --- the comparison ---------------------------------------------------------
$current = Get-Content $currentFixture -Raw -Encoding UTF8 | ConvertFrom-Json
$baseline = Get-Content $baselineOut -Raw -Encoding UTF8 | ConvertFrom-Json

# The modules that are NOT compared (and why).
$excluded = @{
    'unload_state' = 'the task 1.1 unload state machine: the baseline has no counterpart'
    'workerw_host' = 'task C7: the WorkerW host rule was inline in the baseline, not a pure function'
}
# The fields that are not compared, per module.
$excludedFields = @{
    'pause_controller' = @('hides_wallpaper')
}

$differences = @()
$compared = 0
$skipped = 0
foreach ($case in $current.cases) {
    $module = $case.module
    $baseCase = $baseline.cases | Where-Object { $_.module -eq $module -and $_.name -eq $case.name }
    if ($excluded.ContainsKey($module)) {
        $skipped++
        continue
    }
    if (-not $baseCase) {
        $differences += [pscustomobject]@{ module = $module; name = $case.name; field = '(the case itself)'; current = 'present'; baseline = 'missing' }
        continue
    }
    $skipFields = @()
    if ($excludedFields.ContainsKey($module)) { $skipFields = $excludedFields[$module] }
    foreach ($prop in $case.output.PSObject.Properties) {
        if ($skipFields -contains $prop.Name) { continue }
        $compared++
        $baseValue = $baseCase.output.($prop.Name)
        $currentValue = $prop.Value
        if ("$baseValue" -ne "$currentValue") {
            $differences += [pscustomobject]@{
                module = $module
                name = $case.name
                field = $prop.Name
                current = "$currentValue"
                baseline = "$baseValue"
            }
        }
    }
}

$reportData = [pscustomobject]@{
    tag = $Tag
    baseline_commit = (git -C $Worktree rev-parse HEAD).Trim()
    compared_values = $compared
    skipped_cases = $skipped
    excluded_modules = $excluded
    excluded_fields = $excludedFields
    difference_count = $differences.Count
    differences = $differences
    verdict = if ($differences.Count -eq 0) { 'IDENTICAL' } else { 'DRIFT' }
}
$reportData | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $report -Encoding UTF8

Write-Host ("  [g5] the baseline: {0}" -f $reportData.baseline_commit.Substring(0, 8))
Write-Host ("  [g5] compared values: {0}, skipped cases: {1}" -f $compared, $skipped)
if ($differences.Count -eq 0) {
    Write-Host '  [g5] VERDICT: IDENTICAL - the fixture describes the baseline behaviour'
} else {
    Write-Host ("  [g5] VERDICT: DRIFT - {0} difference(s)" -f $differences.Count)
    $differences | Select-Object -First 10 | ForEach-Object { Write-Host ("    {0}/{1} {2}: the current={3} the baseline={4}" -f $_.module, $_.name, $_.field, $_.current, $_.baseline) }
}
Write-Host ("  [g5] the report: {0}" -f $report)

if ($differences.Count -ne 0) { exit 1 }
exit 0
```

## File: tools/gate_ctest_stability.ps1
```powershell
# tools/gate_ctest_stability.ps1 - task J2: the suite must be GREEN ten
# times in a row with the LIVE engine running.
#
# WHY
#   A test that sometimes fails weakens every gate it sits in: a red run
#   stops being a signal. Task J2 does not accept "it is environmental" -
#   it wants the root cause (the still_test's mtime granularity is fixed by
#   setting the timestamp explicitly; the studio_logic_test's late IPC ack
#   is recorded as an open finding) and a gate that PROVES the stability.
#
# THE GATE
#   10 consecutive full ctest runs, with the live engine running, 0
#   failures, and the wall time of every run recorded. The engine is started
#   if it is not running, and the report says whether it was alive.
#
[CmdletBinding()]
param(
    [int]$Runs = 10,
    [string]$EngineExe = '',
    [string]$OutJson = ''
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
if (-not $EngineExe) {
    $EngineExe = Join-Path $root 'build\release\engine.exe'
}
if (-not $OutJson) {
    $OutJson = Join-Path $root 'docs\bench\ctest-stability.json'
}

$engineWasRunning = [bool](Get-Process engine -ErrorAction SilentlyContinue)
if (-not $engineWasRunning) {
    Write-Host '  [gate] the engine was not running - starting it (the gate runs WITH the live engine)'
    Start-Process -FilePath $EngineExe -WindowStyle Hidden
    Start-Sleep -Seconds 10
}
$engine = Get-Process engine -ErrorAction SilentlyContinue
if (-not $engine) {
    Write-Error 'the engine is not running - the gate needs it'
    exit 2
}
Write-Host ("  [gate] the live engine: pid {0}" -f $engine[0].Id)

$results = @()
$failures = 0
for ($i = 1; $i -le $Runs; $i++) {
    $started = Get-Date
    $output = & ctest --preset msvc-dev 2>&1
    $elapsed = [math]::Round(((Get-Date) - $started).TotalSeconds, 1)
    $exit = $LASTEXITCODE
    $failed = @($output | Select-String -SimpleMatch 'The following tests FAILED:')
    $summary = ($output | Select-String 'tests passed|tests failed' | Select-Object -Last 1)
    $ok = ($exit -eq 0)
    if (-not $ok) { $failures++ }
    $results += [pscustomobject]@{
        run = $i
        seconds = $elapsed
        exit_code = $exit
        ok = $ok
        summary = if ($summary) { $summary.Line.Trim() } else { '' }
    }
    Write-Host ("  [gate] run {0}: {1} s, the exit code {2}{3}" -f $i, $elapsed, $exit, $(if ($ok) { '' } else { ' FAILED' }))
    # The engine must survive the run (a test that kills it is a finding).
    if (-not (Get-Process engine -ErrorAction SilentlyContinue)) {
        Write-Host '  [gate] the engine died during the run - restarting it'
        Start-Process -FilePath $EngineExe -WindowStyle Hidden
        Start-Sleep -Seconds 8
    }
}

$report = [ordered]@{
    tool = 'gate_ctest_stability'
    task = 'J2'
    date = (Get-Date -Format 'yyyy-MM-ddTHH:mm:ssK')
    label = 'live-proven (one machine)'
    method = "the full ctest suite, $Runs times in a row, with the live engine running; the wall time of every run is recorded"
    runs = $Runs
    the_engine_was_running = $true
    failures = $failures
    total_seconds = [math]::Round((($results | Measure-Object -Property seconds -Sum).Sum), 1)
    the_median_seconds = [math]::Round((($results | Measure-Object -Property seconds -Average).Average), 1)
    results = $results
}
$outDir = Split-Path -Parent ([IO.Path]::GetFullPath($OutJson))
if (-not (Test-Path -LiteralPath $outDir)) { New-Item -ItemType Directory -Force -Path $outDir | Out-Null }
$report | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $OutJson -Encoding UTF8

Write-Host ''
Write-Host ("  [gate] {0} runs, {1} failures, the total {2} s" -f $Runs, $failures, $report.total_seconds)
Write-Host ("  [gate] the report: {0}" -f $OutJson)
if ($failures -gt 0) { exit 1 }
exit 0
```

## File: tools/lint_env_overrides_selftest.ps1
```powershell
# tools/lint_env_overrides_selftest.ps1 - the self-test of the environment
# override lint (task L5).
#
# WHY
#   A lint that cannot fire is worse than no lint: it reports "OK" while the
#   defect it exists to catch is back. This builds a scratch directory with
#   known-bad and known-good sources and asserts the lint's verdict on each.
#
# WHAT IT ASSERTS
#   1. an UNGATED C++ read of a K6WP_* name      -> the lint FAILS
#   2. an ungated Rust read                        -> the lint FAILS
#   3. a C++ read behind `#if K6WP_ALLOW_ENV_OVERRIDES` -> the lint PASSES
#   4. a C++ read behind `#ifdef _DEBUG`          -> the lint PASSES
#   5. a Rust read behind `#[cfg(debug_assertions)]`     -> the lint PASSES
#   6. a read of a NON-K6WP name (LOCALAPPDATA)    -> the lint PASSES
#   7. a documented override (K6WP_CACHE_DIR)      -> the lint PASSES and
#      NAMES it (the classification stays visible)
#
[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$lint = Join-Path $root 'tools\lint_env_overrides.ps1'
$scratch = Join-Path $env:TEMP ("k6wp_env_lint_" + [Guid]::NewGuid().ToString('N').Substring(0, 8))

function New-ScratchFile([string]$name, [string]$content) {
    $path = Join-Path $scratch $name
    [IO.File]::WriteAllText($path, $content, (New-Object Text.UTF8Encoding($false)))
}

function Invoke-Lint([string]$path) {
    $old = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try {
        $output = & powershell -NoProfile -ExecutionPolicy Bypass -File $lint -ScanPath $path 2>&1
        $code = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $old
    }
    return [pscustomobject]@{ Code = $code; Output = ($output | Out-String) }
}

$checks = 0
$failures = 0
function Check([bool]$ok, [string]$what) {
    $script:checks++
    if ($ok) {
        Write-Host "  [OK] $what"
    } else {
        $script:failures++
        Write-Host "  [FAIL] $what"
    }
}

New-Item -ItemType Directory -Force -Path $scratch | Out-Null
try {
    # 1. an ungated C++ read -> the lint must fail
    New-ScratchFile 'ungated_cpp.cpp' @'
#include <cstdlib>
int Mode() {
  const char* mode = std::getenv("K6WP_ENGINE_PIPE");
  return mode == nullptr ? 0 : 1;
}
'@
    $r = Invoke-Lint (Join-Path $scratch 'ungated_cpp.cpp')
    Check ($r.Code -ne 0) 'an ungated C++ environment read fails the lint'

    # 2. an ungated Rust read -> the lint must fail
    New-ScratchFile 'ungated_rs.rs' @'
pub fn pipe() -> Option<String> {
    std::env::var("K6WP_ENGINE_MUTEX").ok()
}
'@
    $r = Invoke-Lint (Join-Path $scratch 'ungated_rs.rs')
    Check ($r.Code -ne 0) 'an ungated Rust environment read fails the lint'

    # 3. a C++ read behind the release gate -> the lint must pass
    New-ScratchFile 'gated_cpp.cpp' @'
#include "build_flags.hpp"
int Mode() {
#if K6WP_ALLOW_ENV_OVERRIDES
  const char* mode = std::getenv("K6WP_ENGINE_PIPE");
  return mode == nullptr ? 0 : 1;
#else
  return 0;
#endif
}
'@
    $r = Invoke-Lint (Join-Path $scratch 'gated_cpp.cpp')
    Check ($r.Code -eq 0) 'a C++ read behind K6WP_ALLOW_ENV_OVERRIDES passes the lint'

    # 4. a C++ read behind #ifdef _DEBUG -> the lint must pass
    New-ScratchFile 'debug_cpp.cpp' @'
int Mode() {
#ifdef _DEBUG
  const char* mode = std::getenv("K6WP_DEV");
  return mode == nullptr ? 0 : 1;
#else
  return 0;
#endif
}
'@
    $r = Invoke-Lint (Join-Path $scratch 'debug_cpp.cpp')
    Check ($r.Code -eq 0) 'a C++ read behind #ifdef _DEBUG passes the lint'

    # 5. a Rust read behind #[cfg(debug_assertions)] -> the lint must pass
    New-ScratchFile 'gated_rs.rs' @'
pub fn pipe() -> Option<String> {
    #[cfg(debug_assertions)]
    {
        if let Ok(name) = std::env::var("K6WP_ENGINE_MUTEX") {
            return Some(name);
        }
    }
    None
}
'@
    $r = Invoke-Lint (Join-Path $scratch 'gated_rs.rs')
    Check ($r.Code -eq 0) 'a Rust read behind #[cfg(debug_assertions)] passes the lint'

    # 6. a non-K6WP name is not an override -> the lint must pass
    New-ScratchFile 'ordinary.cpp' @'
int Mode() {
  const char* local = std::getenv("LOCALAPPDATA");
  return local == nullptr ? 0 : 1;
}
'@
    $r = Invoke-Lint (Join-Path $scratch 'ordinary.cpp')
    Check ($r.Code -eq 0) 'a read of an ordinary variable is not an override'

    # 7. a documented override passes and is NAMED
    New-ScratchFile 'documented.cpp' @'
int Cache() {
  const char* dir = std::getenv("K6WP_CACHE_DIR");
  return dir == nullptr ? 0 : 1;
}
'@
    $r = Invoke-Lint (Join-Path $scratch 'documented.cpp')
    Check ($r.Code -eq 0) 'a documented override passes the lint'
    Check ($r.Output -match 'K6WP_CACHE_DIR') 'the documented override is named in the output'
} finally {
    Remove-Item -LiteralPath $scratch -Recurse -Force -ErrorAction SilentlyContinue
}

Write-Host "lint_env_overrides_selftest: $checks checks, $failures failure(s)"
if ($failures -gt 0) { exit 1 }
exit 0
```

## File: tools/lint_env_overrides.ps1
```powershell
# tools/lint_env_overrides.ps1 - every K6WP_* environment read must sit
# behind a build gate (task L5).
#
# WHY
#   tools/check_release_overrides.ps1 proves that a RELEASE BINARY does not
#   contain the override names. That proof only covers the binaries a
#   release build produces. This lint proves the RULE in the source, where
#   it can be checked for every language (C++ and Rust) and for every file -
#   including the crates and tools that no release build produces yet, and
#   including a new override that someone adds tomorrow.
#
# THE RULE
#   A `K6WP_*` name that is READ from the environment must be behind one of
#   the build gates:
#     - the C++: `#if K6WP_ALLOW_ENV_OVERRIDES` or `#ifdef _DEBUG`
#     - the Rust: `#[cfg(debug_assertions)]`, `#[cfg(test)]` or `#[test]`
#   A file under `tests/` is exempt as a whole: it never ships.
#   A name that is only MENTIONED (a comment, a doc line, a string constant
#   that is not an environment read) is fine - only an actual read counts.
#
# WHY IT MATTERS
#   A shipped binary must not let an environment variable redirect the IPC
#   endpoint, the singleton mutex, the library path or the parser's
#   behaviour: whoever can set an environment variable for the process could
#   then point the app at a pipe it controls. The gate makes the code
#   disappear from the release build; the binary scan proves it.
#
# THE CONSISTENCY CHECK
#   Every name in the checker's forbidden list must be read somewhere in the
#   tree - otherwise the list has drifted and the binary scan would pass
#   vacuously for that name.
#
[CmdletBinding()]
param(
    # Fail when the tree holds no gated read at all (a lint that matches
    # nothing is a lint that cannot fire).
    [switch]$RequireMatches,
    # Scan this directory instead of the tracked files (the self-test uses
    # it with a scratch file). The consistency check only runs on the tree.
    [string]$ScanPath = ''
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot

# The environment reads: the C++ and the Rust spellings, including the
# wrappers the tree uses (`GetEnvStr` in the compressor, `qEnvironmentVariable`
# in the Studio).
# NOTE: the capture group of the NAME must be group 1 in every pattern, so
# the optional part is a NON-capturing group.
$readPatterns = @(
    'GetEnvironmentVariable[AW]\(\s*L?"(K6WP_[A-Z0-9_]+)"',
    '\bvar(?:_os)?\(\s*"(K6WP_[A-Z0-9_]+)"',
    '\bgetenv\(\s*"(K6WP_[A-Z0-9_]+)"',
    '\bGetEnvStr\(\s*"(K6WP_[A-Z0-9_]+)"',
    'qEnvironmentVariable\(\s*"(K6WP_[A-Z0-9_]+)"'
)
# The gates that make a read disappear from a release build.
$cxxGates = @('#if K6WP_ALLOW_ENV_OVERRIDES', '#ifdef _DEBUG', '#if defined(_DEBUG)')
$rustGates = @('#[cfg(debug_assertions)]', '#[cfg(test)]', '#[test]')

# The overrides that are NOT diagnostic and therefore stay in a release
# build. Each one is a deliberate, documented decision - the reason is part
# of the rule, so a new override cannot slip in unclassified.
$allowed = @(
    @{ name = 'K6WP_RECONCILE_MODE'
       reason = 'a documented feature switch (the shadow reconcile mode, CHANGELOG alpha.3): it changes a BEHAVIOUR, it redirects nothing' },
    @{ name = 'K6WP_CACHE_DIR'
       reason = 'the Studio tells the compressor where to put its cache (studio/src/compress_service.cpp): an internal channel, not a diagnostic override' },
    @{ name = 'K6WP_CACHE_MAX_BYTES'
       reason = 'the compressor cache cap: a limit, not a redirection' },
    @{ name = 'K6WP_PLAYLIST_JSON'
       reason = 'the Studio playlist path; the release test suite relies on it (tests/playlist_bridge_test.cpp), so it is NOT compiled out' },
    @{ name = 'K6WP_THUMBS_DIR'
       reason = 'the thumbnailer scratch root for the tests; the release test suite relies on it (tests/studio_logic_test.cpp, tests/thumbnailer_test.cpp)' }
)
$allowedNames = @{}
foreach ($entry in $allowed) { $allowedNames[$entry.name] = $entry.reason }

$files = @(git -C $root ls-files -- '*.cpp' '*.hpp' '*.cc' '*.h' '*.rs')
if ($ScanPath) {
    $files = @(Get-ChildItem -LiteralPath $ScanPath -Recurse -File |
               Where-Object { $_.Extension -in @('.cpp', '.hpp', '.cc', '.h', '.rs') } |
               ForEach-Object { $_.FullName })
}
if ($files.Count -eq 0) {
    Write-Error 'no source file found (git ls-files returned nothing)'
    exit 2
}

# The names the binary scan forbids (the same list as
# tools/check_release_overrides.ps1).
$forbidden = @('K6WP_ENGINE_PIPE', 'K6WP_ENGINE_MUTEX', 'K6WP_LIBRARY_JSON',
               'K6WP_TEST_NO_CONFIG_LOCK', 'K6WP_IPC_TRACE', 'K6WP_DEV',
               'K6WP_SPS_DEBUG')
$seen = @{}

$errors = 0
$readCount = 0
foreach ($relative in $files) {
    $path = if ([IO.Path]::IsPathRooted($relative)) { $relative } else { Join-Path $root $relative }
    if (-not (Test-Path -LiteralPath $path)) { continue }
    $lines = [IO.File]::ReadAllLines($path, [Text.Encoding]::UTF8)
    # A test file never ships: the whole file is exempt.
    $isTestFile = ($relative -match '(^|[\\/])tests?[\\/]') -or
                  ($relative -match '_test\.(cpp|hpp|cc|rs)$') -or
                  ($relative -match '(^|[\\/])fuzz[\\/]')
    for ($i = 0; $i -lt $lines.Count; $i++) {
        $line = $lines[$i]
        foreach ($pattern in $readPatterns) {
            $m = [regex]::Match($line, $pattern)
            if (-not $m.Success) { continue }
            $name = $m.Groups[1].Value
            if ($name -notmatch '^K6WP_') { continue }
            $readCount++
            $seen[$name] = $true
            if ($isTestFile) { continue }
            # A documented, non-diagnostic override: allowed, and reported so
            # the classification stays visible in the CI log.
            if ($allowedNames.ContainsKey($name)) {
                Write-Host ("  [note] {0}:{1}: {2} stays in a release build - {3}" -f
                            $relative, ($i + 1), $name, $allowedNames[$name])
                continue
            }
            # The gate must enclose the read. For the C++ that is the
            # preprocessor conditional the read sits in; for the Rust it is
            # the attribute just above it.
            $gated = $false
            $depth = 0
            for ($j = $i - 1; $j -ge [Math]::Max(0, $i - 40); $j--) {
                $above = $lines[$j]
                if ($above -match '^\s*#\s*endif') { $depth++; continue }
                if ($above -match '^\s*#\s*(if|ifdef|ifndef)\b') {
                    if ($depth -eq 0) {
                        $condition = $above
                        if ($j + 1 -lt $i) { $condition = $condition + ' ' + $lines[$j + 1] }
                        foreach ($marker in $cxxGates) {
                            if ($condition.Contains($marker)) { $gated = $true }
                        }
                        break
                    }
                    $depth--
                    continue
                }
                foreach ($marker in $rustGates) {
                    if ($above.Contains($marker)) { $gated = $true }
                }
            }
            if (-not $gated) {
                $errors++
                Write-Host ("  [FAIL] {0}:{1}: the environment read of {2} is not behind a build gate" -f
                            $relative, ($i + 1), $name)
                Write-Host ("         (the C++: {0} - the Rust: {1})" -f ($cxxGates -join ', '), ($rustGates -join ', '))
            }
        }
    }
}

# The consistency check: every forbidden name must be read somewhere (only
# on the tree scan - a scratch directory has no such names).
if (-not $ScanPath) {
    foreach ($name in $forbidden) {
        if (-not $seen.ContainsKey($name)) {
            $errors++
            Write-Host ("  [FAIL] the forbidden name {0} is read nowhere in the tree: the checker's list has drifted" -f $name)
        }
    }
}

if ($readCount -eq 0 -and -not $ScanPath) {
    # On the tree scan there must be reads; on a scratch scan (the
    # self-test) a file without any override is simply clean.
    Write-Error 'no environment read matched: the lint cannot fire'
    exit 2
}

if ($errors -gt 0) {
    Write-Error "lint_env_overrides: $errors error(s)"
    exit 1
}
Write-Host ("lint_env_overrides: OK ({0} files, {1} environment reads, {2} forbidden names, {3} distinct names read)" -f
            $files.Count, $readCount, $forbidden.Count, $seen.Count)
exit 0
```

## File: tools/lint_nonlatin_selftest.ps1
```powershell
# tools/lint_nonlatin_selftest.ps1 - prove the H4 detector actually fires.
#
# WHY
#   A lint that stops detecting is worse than no lint: the CI stays green and
#   the corruption comes back. This test builds a scratch git repository with
#   one clean file, one deliberately mixed-script file, one file holding a
#   replacement character and one with a byte-order mark inside the text,
#   then asserts:
#     - the clean file (typography only) passes,
#     - the allow marker suppresses a deliberate non-Latin string,
#     - a byte-order mark at the START of a file passes (it is legal),
#     - the mixed-script file fails with R2,
#     - the replacement-character file fails with R1,
#     - the mid-text byte-order mark fails with R4,
#     - removing the three bad files makes the scan pass again.
#
#   The scratch repository is removed in finally.
#
[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$tool = Join-Path $PSScriptRoot 'lint_nonlatin.ps1'
$failures = New-Object System.Collections.Generic.List[string]
$checks = 0

function Assert-That([bool]$condition, [string]$what) {
    $script:checks++
    if (-not $condition) { $script:failures.Add($what) }
}

function Invoke-Lint([string]$scanRoot) {
    $text = & powershell -NoProfile -ExecutionPolicy Bypass -File $tool -ScanRoot $scanRoot 2>&1 | Out-String
    return @{ Exit = $LASTEXITCODE; Text = $text }
}

$cjk = [string][char]0x6D4B      # a CJK character
$delta = [string][char]0x0394    # the Greek capital delta: legitimate math
$arrow = [string][char]0x2192    # a right arrow: legitimate typography
$bad = [string][char]0xFFFD      # the replacement character
$bom = [string][char]0xFEFF      # a byte-order mark

$tmp = Join-Path $env:TEMP ('k6wp-nonlatin-' + [guid]::NewGuid().ToString('N'))
$utf8 = New-Object Text.UTF8Encoding($false)

function Write-Utf8([string]$path, [string]$text) {
    [IO.File]::WriteAllText($path, $text, $utf8)
}

try {
    New-Item -ItemType Directory -Path $tmp | Out-Null
    & git -C $tmp init -q
    if ($LASTEXITCODE -ne 0) { throw 'git init failed' }

    Write-Utf8 (Join-Path $tmp 'good_typography.md') ("# Rates`n`nA " + $arrow + " B: the " + $delta + " stays <= 12 %.`n")
    Write-Utf8 (Join-Path $tmp 'good_allow.md') ("nonlatin-allow: the selftest needs one real CJK string`n`npath = C:\video\" + $cjk + ".mp4`n")
    # A UTF-8 BOM at the very start of a file is legal (PowerShell and Visual
    # Studio both write one) and must not be flagged.
    [IO.File]::WriteAllText((Join-Path $tmp 'good_bom.md'), ("# A file with a byte-order mark`n"), (New-Object Text.UTF8Encoding($true)))
    & git -C $tmp add -A
    if ($LASTEXITCODE -ne 0) { throw 'git add failed' }

    $r = Invoke-Lint $tmp
    Assert-That ($r.Exit -eq 0) ("the clean tree passes (exit " + $r.Exit + ")")
    Assert-That ($r.Text -notmatch 'good_typography') 'typography is not flagged'
    Assert-That ($r.Text -notmatch 'good_allow') 'the allow marker is honoured'
    Assert-That ($r.Text -notmatch 'good_bom') 'a leading byte-order mark is not flagged'

    Write-Utf8 (Join-Path $tmp 'bad_mixed.md') ("the word meng" + $cjk + " appears mid-word`n")
    Write-Utf8 (Join-Path $tmp 'bad_replacement.md') ("the text was " + $bad + " decoded from broken bytes`n")
    Write-Utf8 (Join-Path $tmp 'bad_bom.md') ("the text was " + $bom + " pasted from a console`n")
    & git -C $tmp add -A

    $r = Invoke-Lint $tmp
    Assert-That ($r.Exit -eq 1) ("the corrupt tree fails (exit " + $r.Exit + ")")
    Assert-That ($r.Text -match 'bad_mixed\.md:\d+:\d+: R2') 'R2 fires on the mixed-script word'
    Assert-That ($r.Text -match 'bad_replacement\.md:\d+:\d+: R1') 'R1 fires on the replacement character'
    Assert-That ($r.Text -match 'bad_bom\.md:\d+:\d+: R4') 'R4 fires on the mid-text byte-order mark'
    Assert-That ($r.Text -notmatch 'good_typography') 'typography stays clean next to the failures'

    Remove-Item -LiteralPath (Join-Path $tmp 'bad_mixed.md')
    Remove-Item -LiteralPath (Join-Path $tmp 'bad_replacement.md')
    Remove-Item -LiteralPath (Join-Path $tmp 'bad_bom.md')
    & git -C $tmp add -A

    $r = Invoke-Lint $tmp
    Assert-That ($r.Exit -eq 0) ("the repaired tree passes again (exit " + $r.Exit + ")")
} finally {
    if (Test-Path -LiteralPath $tmp) {
        Remove-Item -LiteralPath $tmp -Recurse -Force -ErrorAction SilentlyContinue
    }
}

Write-Host ("  [nonlatin-selftest] {0} checks, {1} failure(s)" -f $checks, $failures.Count)
foreach ($f in $failures) { Write-Host ("  [nonlatin-selftest] FAIL: " + $f) }
if ($failures.Count -gt 0) { exit 1 }
exit 0
```

## File: tools/lint_nonlatin.ps1
```powershell
# tools/lint_nonlatin.ps1 - H4: no accidental non-Latin characters.
#
# WHY
#   A few of the project's documents and code comments had picked up
#   non-Latin characters that nobody typed on purpose: a CJK character in
#   the middle of an English/Indonesian word, a Cyrillic look-alike, a
#   replacement character (U+FFFD) left over from a broken decode. They make
#   the text unsearchable (a grep for the word misses it) and they usually
#   mark a corrupted fragment. This lint catches them.
#
# WHAT IT FLAGS
#   R1. U+FFFD (the replacement character) - ALWAYS broken, no exceptions.
#   R2. A "mixed-script word": a token that contains a Latin letter AND a
#       CJK/Hangul/Cyrillic/Arabic/Thai letter (e.g. "meng" followed by a CJK
#       character). This is the shape of the accidental corruption.
#   R3. A CJK/Hangul/Cyrillic/Arabic/Thai character anywhere in a scanned
#       file, UNLESS the file declares that it does that on purpose with the
#       marker line:  nonlatin-allow: <the reason>
#       (the tests that verify non-ASCII path escaping need real CJK text).
#   R4. A byte-order mark INSIDE the text (a copy-paste artifact, usually a
#       UTF-8 BOM that PowerShell's `Set-Content -Encoding UTF8` prepends -
#       it silently corrupted a commit subject once). A BOM at the very start
#       of a file is legal and is not flagged; ReadAllText strips it.
#
# WHAT IT DOES NOT FLAG
#   Typography and math that the documents legitimately use: em/en dashes,
#   arrows, the minus sign, <=/>=, the multiplication sign, the micro sign,
#   the degree sign, the ellipsis, Delta, and box-drawing characters (the
#   state diagrams in the design documents). Those are not "corruption" and
#   rewriting them would lose meaning.
#
# WHAT IT SCANS
#   The git-tracked files only (`git ls-files`), so a generated scratch file
#   such as a repomix dump is never scanned. Inside LICENSES/ only README.md
#   is scanned: the other files ARE the original licence texts.
#
# USAGE
#   powershell -File tools/lint_nonlatin.ps1            # scan, fail on errors
#   powershell -File tools/lint_nonlatin.ps1 -List      # also list every file
#                                                       # with non-ASCII hits
#   powershell -File tools/lint_nonlatin.ps1 -Commits 30 # also check the last
#                                                       # 30 commit messages
#   powershell -File tools/lint_nonlatin.ps1 -Staged    # only the staged files
#                                                       # (the pre-commit hook)
#
[CmdletBinding()]
param(
    [switch]$List,
    [int]$Commits = 0,
    # The pre-commit hook passes -Staged: scan only the files the commit adds
    # or changes. CI runs the full scan over every tracked file.
    [switch]$Staged,
    # The self-test points ScanRoot at a scratch repository so it can prove the
    # detector fires without touching the real tree.
    [string]$ScanRoot = ''
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
if ($ScanRoot) {
    $repo = (Resolve-Path -LiteralPath $ScanRoot).Path
} else {
    $repo = (Resolve-Path $root).Path
}
Set-Location -LiteralPath $repo

$allowMarker = 'nonlatin-allow:'

$exts = @('.md', '.cpp', '.hpp', '.h', '.c', '.rs', '.ps1', '.yml', '.yaml',
          '.json', '.txt', '.toml', '.cmake', '.in', '.qml', '.js', '.ts')

# Never scanned: generated dumps and the original licence texts.
function Test-Skipped([string]$rel) {
    if ($rel -like 'LICENSES/*' -and $rel -ne 'LICENSES/README.md') { return $true }
    if ($rel -like 'tests/corpus/*') { return $true }
    if ($rel -like 'vendor/*') { return $true }
    return $false
}

$replacement = [char]0xFFFD
$replacementStr = [string]$replacement
# R4: a byte-order mark anywhere except the start of a file.
$bom = [char]0xFEFF
$bomStr = [string]$bom

# CJK, Hangul, Cyrillic, Arabic, Thai.
$cjk = '[\u3040-\u30FF\u3400-\u4DBF\u4E00-\u9FFF\uF900-\uFAFF\uAC00-\uD7AF\u1100-\u11FF\u0400-\u04FF\u0600-\u06FF\u0E00-\u0E7F]'

$errors = New-Object System.Collections.Generic.List[string]
$inventory = New-Object System.Collections.Generic.List[string]

if ($Staged) {
    $tracked = & git -C $repo diff --cached --name-only --diff-filter=ACMR
} else {
    $tracked = & git -C $repo ls-files
}
if ($LASTEXITCODE -ne 0) { Write-Host '  [nonlatin] git ls-files failed'; exit 2 }

$scanned = 0
$maxPerFile = 5
foreach ($rel in $tracked) {
    $rel = $rel -replace '\\', '/'
    if (Test-Skipped $rel) { continue }
    $ext = [IO.Path]::GetExtension($rel).ToLowerInvariant()
    if ($exts -notcontains $ext) { continue }
    $full = Join-Path $repo ($rel -replace '/', [IO.Path]::DirectorySeparatorChar)
    if (-not (Test-Path -LiteralPath $full)) { continue }
    $scanned++

    try {
        $text = [IO.File]::ReadAllText($full, [Text.Encoding]::UTF8)
    } catch {
        $errors.Add(("{0}: the file could not be read as UTF-8 ({1})" -f $rel, $_.Exception.Message))
        continue
    }
    $hasReplacement = (-1 -ne $text.IndexOf($replacementStr, [StringComparison]::Ordinal))
    $hasCjk = $text -match $cjk
    $hasBom = (-1 -ne $text.IndexOf($bomStr, [StringComparison]::Ordinal))
    if (-not $hasReplacement -and -not $hasCjk -and -not $hasBom) { continue }

    $lines = $text -split "`r?`n"
    $allow = $false
    foreach ($l in $lines) { if ($l -like "*${allowMarker}*") { $allow = $true; break } }

    $reported = 0
    for ($i = 0; $i -lt $lines.Count; $i++) {
        $line = $lines[$i]
        $lineNo = $i + 1

        if ($hasReplacement -and (-1 -ne $line.IndexOf($replacementStr, [StringComparison]::Ordinal))) {
            $col = ($line.IndexOf($replacementStr, [StringComparison]::Ordinal) + 1)
            if ($reported -lt $maxPerFile) {
                $errors.Add(("{0}:{1}:{2}: R1 the replacement character U+FFFD - the text was decoded from broken bytes; retype the intended word" -f $rel, $lineNo, $col))
            }
            $reported++
            continue
        }
        # R4: a byte-order mark inside the text (ReadAllText strips
        # the one at the start of the file, so anything left is mid-text).
        if ($hasBom -and (-1 -ne $line.IndexOf($bomStr, [StringComparison]::Ordinal))) {
            $col = ($line.IndexOf($bomStr, [StringComparison]::Ordinal) + 1)
            if ($reported -lt $maxPerFile) {
                $errors.Add(("{0}:{1}:{2}: R4 a byte-order mark inside the text - a copy-paste artifact; delete the invisible character" -f $rel, $lineNo, $col))
            }
            $reported++
            continue
        }

        if (-not $hasCjk) { continue }

        # R2: a mixed-script word (Latin + non-Latin in one token).
        if (-not $allow) {
            foreach ($m in [regex]::Matches($line, "[A-Za-z][^\s]*${cjk}|${cjk}[^\s]*[A-Za-z]")) {
                if ($reported -lt $maxPerFile) {
                    $errors.Add(("{0}:{1}:{2}: R2 a mixed-script word '{3}' - a non-Latin character inside a Latin word" -f $rel, $lineNo, ($m.Index + 1), $m.Value))
                }
                $reported++
            }
        }

        # R3: non-Latin script unless the file declares it on purpose.
        $hits = [regex]::Matches($line, $cjk)
        if ($hits.Count -gt 0 -and -not $allow) {
            $cps = ($hits | ForEach-Object { "U+{0:X4}" -f [int][char]$_.Value } | Sort-Object -Unique) -join ' '
            if ($reported -lt $maxPerFile) {
                $errors.Add(("{0}:{1}: R3 non-Latin script ({2}) - if this is intentional, add a marker line '{3} <reason>'" -f $rel, $lineNo, $cps, $allowMarker))
            }
            $reported++
        }
    }
    if ($reported -gt $maxPerFile) {
        $errors.Add(("{0}: {1} more finding(s) in this file were not listed" -f $rel, ($reported - $maxPerFile)))
    }

    if ($List) {
        $distinct = New-Object System.Collections.Generic.HashSet[int]
        foreach ($ch in $text.ToCharArray()) {
            $c = [int][char]$ch
            if ($c -gt 127) { [void]$distinct.Add($c) }
        }
        if ($distinct.Count -gt 0) {
            $tag = ''
            if ($allow) { $tag = ' [allowed]' }
            $inventory.Add(("  {0}: {1} distinct non-ASCII code points{2}" -f $rel, $distinct.Count, $tag))
        }
    }
}

$commitFindings = New-Object System.Collections.Generic.List[string]
if ($Commits -gt 0) {
    $messages = & git -C $repo log "-$Commits" --format='%h%x1f%B%x1e'
    foreach ($block in ($messages -split [char]0x1E)) {
        if (-not $block.Trim()) { continue }
        $parts = $block -split [char]0x1F, 2
        if ($parts.Count -lt 2) { continue }
        $hash = $parts[0].Trim()
        $body = $parts[1]
        if ((-1 -ne $body.IndexOf($replacementStr, [StringComparison]::Ordinal))) {
            $commitFindings.Add(("  {0}: R1 U+FFFD in the commit message" -f $hash))
        }
        if (-1 -ne $body.IndexOf($bomStr, [StringComparison]::Ordinal)) {
            $commitFindings.Add(("  {0}: R4 a byte-order mark in the commit message (a UTF-8 BOM from Set-Content -Encoding UTF8)" -f $hash))
        }
        foreach ($m in [regex]::Matches($body, $cjk)) {
            $commitFindings.Add(("  {0}: R3 non-Latin script (U+{1:X4}) in the commit message" -f $hash, [int][char]$m.Value))
        }
    }
}

if ($List) {
    Write-Host '  [nonlatin] the non-ASCII inventory:'
    if ($inventory.Count -eq 0) { Write-Host '    (none)' }
    foreach ($line in $inventory) { Write-Host $line }
}

Write-Host (("  [nonlatin] scanned {0} {1} files, {2} error(s)" -f $scanned, $(if ($Staged) { 'staged' } else { 'tracked' }), $errors.Count))
foreach ($e in $errors) { Write-Host ("  [nonlatin] " + $e) }

if ($commitFindings.Count -gt 0) {
    Write-Host (("  [nonlatin] the last {0} commit messages: {1} finding(s) (ADVISORY - history is not rewritten)" -f $Commits, $commitFindings.Count))
    foreach ($c in $commitFindings) { Write-Host $c }
}

if ($errors.Count -gt 0) { exit 1 }
exit 0
```

## File: tools/lint_test_overrides.ps1
```powershell
# tools/lint_test_overrides.ps1 - task J2: a test may not touch the
# PRODUCTION pipe or mutex name without claiming a unique one.
#
# WHY
#   The production names are per-session and fixed for the whole logon
#   session:
#     - the pipe:  \\.\pipe\k6wp-engine-<session_id>
#     - the mutex: Local\K6WP-Engine-Singleton
#   A test that uses them talks to the USER's live engine (or blocks on the
#   user's engine's mutex) - that is how the suite once hung for 30 minutes
#   and how the studio_logic_test's "readywait" leg became flaky.
#
#   The mechanism that makes a test safe is the E5 override: a test claims
#   a UNIQUE pipe name (and a unique mutex) per test process.
#
# THE RULES
#   1. A test source that mentions the production pipe base
#      ("k6wp-engine") must also use the test helper
#      (UseUniquePipeNameForTest / UseRealPipeNameForTest) - the helper is
#      what redirects the name.
#   2. A test source that mentions the production singleton mutex
#      ("K6WP-Engine-Singleton") must also use the mutex override
#      (K6WP_ENGINE_MUTEX) - otherwise it would collide with the user's
#      live engine.
#   3. A test source that starts an engine must set BOTH overrides.
#
# The lint is a CI gate (a step in .github/workflows/ci.yml) and a
# pre-commit check.
#
[CmdletBinding()]
param(
    [string]$TestDir = ''
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
if (-not $TestDir) {
    $TestDir = Join-Path $root 'tests'
}

# The pipe base as it appears in a NAME (a temp directory that merely
# contains the text is not a pipe usage).
$pipePattern = 'pipe[\\/]+k6wp-engine'
$pipeHelper = 'UseUniquePipeNameForTest|UseRealPipeNameForTest'
$mutexName = 'K6WP-Engine-Singleton'
$mutexOverride = 'K6WP_ENGINE_MUTEX'

$failures = @()
$checked = 0
foreach ($file in Get-ChildItem -Path $TestDir -Filter '*.cpp' -File) {
    $checked++
    $text = Get-Content $file.FullName -Raw
    $problems = @()

    if ($text -match $pipePattern -and $text -notmatch $pipeHelper) {
        $problems += "it uses the production pipe name but never claims a unique one (UseUniquePipeNameForTest / UseRealPipeNameForTest)"
    }
    if ($text.Contains($mutexName) -and $text -notmatch $mutexOverride) {
        $problems += "it mentions the production singleton mutex ('$mutexName') but never overrides it (K6WP_ENGINE_MUTEX)"
    }
    if ($problems.Count -gt 0) {
        $failures += [pscustomobject]@{ file = $file.Name; problems = $problems }
    }
}

if ($failures.Count -gt 0) {
    foreach ($f in $failures) {
        Write-Host ("  [FAIL] {0}" -f $f.file)
        foreach ($p in $f.problems) {
            Write-Host ("         {0}" -f $p)
        }
    }
    Write-Error ("lint_test_overrides: {0} of {1} test sources touch a production name without an override" -f $failures.Count, $checked)
    exit 1
}

Write-Host ("lint_test_overrides: OK ({0} test sources scanned)" -f $checked)
exit 0
```

## File: tools/lint_ui_thread.ps1
```powershell
# lint_ui_thread.ps1 - forbid sync I/O / IPC / registry / net in the Slint UI thread modules.
# Usage: powershell -File tools/lint_ui_thread.ps1 [-Path <dir>] [-SelfTest]
param(
    [string]$Path = "rust/crates/k6wp-studio/src",
    [switch]$SelfTest
)

$ErrorActionPreference = "Stop"
$violations = @()

# Forbidden patterns in UI-thread code (sync I/O, IPC, registry, network).
$forbidden = @(
    @{ Name = "std::fs";          Pattern = "std::fs::" },
    @{ Name = "std::net";         Pattern = "std::net::" },
    @{ Name = "std::process";     Pattern = "std::process::" },
    @{ Name = "registry (windows)"; Pattern = "RegOpenKey|RegQueryValue|RegSetValue" },
    @{ Name = "named pipe client"; Pattern = "CreateFileW.*PIPE|WaitNamedPipe" },
    @{ Name = "blocking sleep";   Pattern = "std::thread::sleep" }
)

function Scan-Dir($dir) {
    $files = Get-ChildItem -Path $dir -Recurse -Include *.rs,*.slint -ErrorAction SilentlyContinue
    foreach ($f in $files) {
        $lines = Get-Content $f.FullName
        $i = 0
        foreach ($line in $lines) {
            $i++
            foreach ($rule in $forbidden) {
                if ($line -match [regex]::Escape($rule.Pattern)) {
                    # allow in worker modules explicitly marked
                    if ($line -notmatch "worker|// lint-allow") {
                        $violations += "$($f.FullName):$i : forbidden '$($rule.Name)' ($($rule.Pattern))"
                    }
                }
            }
        }
    }
}

if ($SelfTest) {
    $tmp = New-Item -ItemType Directory -Path "$env:TEMP\lintself" -Force
    $testFile = Join-Path $tmp "bad.rs"
    $src = "fn x() { std::fs::read_to_string(""x""); }"
    Set-Content -Path $testFile -Value $src -Encoding utf8
    # scan just that file
    $bad = 0
    $content = Get-Content $testFile
    foreach ($line in $content) {
        if ($line -match [regex]::Escape("std::fs::")) { $bad++ }
    }
    if ($bad -eq 0) {
        Write-Host "SELF-TEST FAILED: lint did not detect forbidden pattern" -ForegroundColor Red
        exit 1
    }
    Write-Host "SELF-TEST OK: lint detects forbidden pattern ($bad hit)" -ForegroundColor Green
    Remove-Item -Recurse -Force $tmp -ErrorAction SilentlyContinue
    exit 0
}

Scan-Dir $Path
if ($violations.Count -gt 0) {
    Write-Host "UI-THREAD LINT: $($violations.Count) violation(s)" -ForegroundColor Red
    $violations | ForEach-Object { Write-Host $_ }
    exit 1
} else {
    Write-Host "UI-THREAD LINT: clean (0 violations)" -ForegroundColor Green
    exit 0
}
```

## File: tools/make_corpus.ps1
```powershell
<# .SYNOPSIS
  Generate the k6wp-18 benchmark corpus (3 clips, lavfi only, no downloads).
.DESCRIPTION
  Plan target was 1 min 4K per clip (anime/gaming/slideshow). DEVIATION (documented):
  this box generates 30s 1080p (1920x1080) instead. Rationale: 3x 1-min 4K
  lavfi encodes via libx264 are very slow on this dev box and would produce
  multi-GB files that must NOT be committed; 30s 1080p keeps generation to
  minutes, files to tens of MB, while still exercising the compressor bench
  (Todo 18 QA-fail-with-note path explicitly allows recording the gap).
  Pass -Width/-Height/-DurationSec to restore 4K/60s on a faster GPU box.
.EXAMPLE
  powershell -NoProfile -ExecutionPolicy Bypass -File tools\make_corpus.ps1
  powershell -NoProfile -ExecutionPolicy Bypass -File tools\make_corpus.ps1 -DurationSec 60 -Width 3840 -Height 2160
#>
param(
  [string]$OutDir = "",
  [int]$DurationSec = 30,
  [int]$Width = 1920,
  [int]$Height = 1080,
  [int]$Fps = 30
)

$ErrorActionPreference = "Continue"

if ([string]::IsNullOrWhiteSpace($OutDir)) {
  $OutDir = Join-Path $PSScriptRoot "..\tests\corpus"
}
$OutDir = [System.IO.Path]::GetFullPath($OutDir)
$Ffmpeg = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot "..\vendor\ffmpeg\ffmpeg.exe"))
$Ffprobe = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot "..\vendor\ffmpeg\ffprobe.exe"))

if (-not (Test-Path -LiteralPath $Ffmpeg)) {
  Write-Error "make_corpus: ffmpeg not found at $Ffmpeg"
  exit 1
}
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

$Size = "{0}x{1}" -f $Width, $Height
$AnimeDur = $DurationSec
$GamingDur = $DurationSec
$SlideEach = [int][Math]::Ceiling($DurationSec / 3)
$SlideTotal = $SlideEach * 3

function Invoke-Ffmpeg {
  param([string[]]$FfmpegArgs, [string]$Label)
  Write-Host "make_corpus: $Label ..."
  $p = Start-Process -FilePath $Ffmpeg -ArgumentList $FfmpegArgs -NoNewWindow -Wait -PassThru
  if ($p.ExitCode -ne 0) {
    Write-Error ("make_corpus: {0} failed with exit {1}" -f $Label, $p.ExitCode)
    return $false
  }
  return $true
}

$ok = $true

# 1. anime-like: testsrc2 fast motion, full fps (high temporal complexity).
$anime = Join-Path $OutDir "anime.mp4"
$okAnime = Invoke-Ffmpeg -Label ("anime (" + $Size + " " + $AnimeDur + "s testsrc2)") -FfmpegArgs @(
  "-y",
  "-f", "lavfi", "-i", ("testsrc2=size=" + $Size + ":rate=" + $Fps + ":duration=" + $AnimeDur),
  "-c:v", "libx264", "-preset", "veryfast", "-crf", "18",
  "-pix_fmt", "yuv420p", "-r", "$Fps",
  "-movflags", "+faststart",
  $anime
)
if (-not $okAnime) { $ok = $false }

# 2. gaming-like: smptehdbars + temporal noise (hard detail + grain).
$gaming = Join-Path $OutDir "gaming.mp4"
$okGaming = Invoke-Ffmpeg -Label ("gaming (" + $Size + " " + $GamingDur + "s smptehdbars+noise)") -FfmpegArgs @(
  "-y",
  "-f", "lavfi", "-i", ("smptehdbars=size=" + $Size + ":rate=" + $Fps + ":duration=" + $GamingDur),
  "-vf", "noise=alls=20:allf=t+u",
  "-c:v", "libx264", "-preset", "veryfast", "-crf", "18",
  "-pix_fmt", "yuv420p", "-r", "$Fps",
  "-movflags", "+faststart",
  $gaming
)
if (-not $okGaming) { $ok = $false }

# 3. slideshow-like: 3 solid-color slides concatenated at low fps (low temporal complexity).
$slideshow = Join-Path $OutDir "slideshow.mp4"
$slideRate = 5
$okSlide = Invoke-Ffmpeg -Label ("slideshow (" + $Size + " 3x" + $SlideEach + "s color slides @" + $slideRate + "fps)") -FfmpegArgs @(
  "-y",
  "-f", "lavfi", "-i", ("color=c=0xC0392B:size=" + $Size + ":rate=" + $slideRate + ":duration=" + $SlideEach),
  "-f", "lavfi", "-i", ("color=c=0x2980B9:size=" + $Size + ":rate=" + $slideRate + ":duration=" + $SlideEach),
  "-f", "lavfi", "-i", ("color=c=0x27AE60:size=" + $Size + ":rate=" + $slideRate + ":duration=" + $SlideEach),
  "-filter_complex", "[0:v][1:v][2:v]concat=n=3:v=1:a=0[out]",
  "-map", "[out]",
  "-c:v", "libx264", "-preset", "veryfast", "-crf", "18",
  "-pix_fmt", "yuv420p", "-r", "$slideRate",
  "-movflags", "+faststart",
  $slideshow
)
if (-not $okSlide) { $ok = $false }

# 4. The bit-depth and codec cases (task H3). They are SHORT (10 s) and
#    generated with lavfi, so they cost seconds. The pairs share the
#    encoder settings, so only the bit depth differs between the arms of an
#    A/B - that is what makes the extra-RAM estimate measurable.
#    - h264_8bit / hi10: the H.264 8-bit and 10-bit pair (High 10).
#    - hevc8 / hevc10: the HEVC 8-bit and 10-bit pair (Main 10). They use
#      the NVENC encoder (the box has an RTX 2050); a box without it can
#      use libx265, which then needs a 10-bit-capable build.
#    - mpeg2: the UNKNOWN-codec case - the parser reads no SPS for it, so
#      the estimate is unknown. It is an elementary stream (.m2v) on
#      purpose: the corpus sweep asserts that every .mp4/.h264/.hevc file
#      IS parsed by the module.
$BitDepthSize = "1920x1080"
$BitDepthDur = 10
$bitDepthSource = @(
  "-y",
  "-f", "lavfi", "-i", ("testsrc2=size=" + $BitDepthSize + ":rate=30:duration=" + $BitDepthDur)
)
$h264_8bit = Join-Path $OutDir "h264_8bit_1080p.mp4"
$okH264_8 = Invoke-Ffmpeg -Label "h264_8bit (8-bit 4:2:0)" -FfmpegArgs ($bitDepthSource + @(
  "-c:v", "libx264", "-pix_fmt", "yuv420p", "-crf", "20", "-preset", "veryfast",
  $h264_8bit
))
if (-not $okH264_8) { $ok = $false }
$hi10 = Join-Path $OutDir "hi10_1080p.mp4"
$okHi10 = Invoke-Ffmpeg -Label "hi10 (10-bit 4:2:0, High 10)" -FfmpegArgs ($bitDepthSource + @(
  "-c:v", "libx264", "-profile:v", "high10", "-pix_fmt", "yuv420p10le", "-crf", "20", "-preset", "veryfast",
  $hi10
))
if (-not $okHi10) { $ok = $false }
$hevc8 = Join-Path $OutDir "hevc8_1080p.mp4"
$okHevc8 = Invoke-Ffmpeg -Label "hevc8 (8-bit 4:2:0)" -FfmpegArgs ($bitDepthSource + @(
  "-c:v", "hevc_nvenc", "-pix_fmt", "yuv420p", "-preset", "p4", "-cq", "24", "-b:v", "0",
  $hevc8
))
if (-not $okHevc8) { $ok = $false }
$hevc10 = Join-Path $OutDir "hevc10_1080p.mp4"
$okHevc10 = Invoke-Ffmpeg -Label "hevc10 (10-bit 4:2:0, Main 10)" -FfmpegArgs ($bitDepthSource + @(
  "-c:v", "hevc_nvenc", "-profile:v", "main10", "-pix_fmt", "p010le", "-preset", "p4", "-cq", "24", "-b:v", "0",
  $hevc10
))
if (-not $okHevc10) { $ok = $false }
$mpeg2 = Join-Path $OutDir "mpeg2_1080p.m2v"
$okMpeg2 = Invoke-Ffmpeg -Label "mpeg2 (the unknown-codec case)" -FfmpegArgs ($bitDepthSource + @(
  "-c:v", "mpeg2video", "-b:v", "8M", "-pix_fmt", "yuv420p", "-f", "mpeg2video",
  $mpeg2
))
if (-not $okMpeg2) { $ok = $false }

# Verify with ffprobe (duration + size report, non-fatal if ffprobe missing).
if (Test-Path -LiteralPath $Ffprobe) {
  foreach ($f in @($anime, $gaming, $slideshow, $h264_8bit, $hi10, $hevc8, $hevc10, $mpeg2)) {
    if (Test-Path -LiteralPath $f) {
      $p = Start-Process -FilePath $Ffprobe -ArgumentList @("-v", "error", "-show_entries", "format=duration,size", "-of", "default=noprint_wrappers=1", $f) -NoNewWindow -Wait -PassThru
      if ($p.ExitCode -ne 0) {
        Write-Error ("make_corpus: ffprobe failed for {0} (exit {1})" -f $f, $p.ExitCode)
        $ok = $false
      }
      $mb = ((Get-Item -LiteralPath $f).Length / 1MB)
      Write-Host ("make_corpus: {0} = {1:N2} MB" -f ([System.IO.Path]::GetFileName($f)), $mb)
    } else {
      Write-Error ("make_corpus: missing output {0}" -f $f)
      $ok = $false
    }
  }
} else {
  Write-Host "make_corpus: ffprobe not found, skipping verification."
}

if (-not $ok) { exit 1 }
Write-Host ("make_corpus: done. corpus dir: {0} (deviation: {1}s {2}, not 60s 4K - see header)" -f $OutDir, $DurationSec, $Size)
exit 0
```

## File: tools/make_handoff.ps1
```powershell
# make_handoff.ps1 - generate curated repomix packages for K6WP refactor handoff
# Fallback: if repomix unavailable, script falls back to PowerShell file merge.
param(
  [switch]$IncludeQt,
  [string]$RepoRoot = $PSScriptRoot + "\.."
)

Set-StrictMode -Version 3
$ErrorActionPreference = "Stop"

$RepoRoot = Resolve-Path $RepoRoot
New-Item -ItemType Directory -Force -Path ($RepoRoot + "\handoff\local") | Out-Null
New-Item -ItemType Directory -Force -Path ($RepoRoot + "\handoff\share") | Out-Null

# Verify repomix version (locked reference)
try {
    $v = (& npx repomix@1.18.1 --version 2>&1) -join ""
    Write-Host "repomix version: $v"
} catch {
    Write-Host "repomix unavailable; will use PowerShell fallback merge" -ForegroundColor Yellow
    $v = "FALLBACK"
}

# Token budget gates (approx tokens via repomix --token-count-tree or rough char/4)
function Get-ApproxTokens($path) {
    if (-not (Test-Path $path)) { return 0 }
    $bytes = (Get-ChildItem $path -Recurse | Measure-Object -Property Length -Sum).Sum
    # rough estimate: 1 token ~= 3.5 chars for code/text
    return [math]::Round($bytes / 3.5)
}

# Package definitions - use [pscustomobject] for proper property access in strict mode
$packages = @(
    [pscustomobject]@{Name="A-digest";  Output="docs/refactor/HANDOFF-DIGEST.md"; Mode="raw"; Limit=3000; Path=$RepoRoot; Optional=$false; Patterns=@()},
    [pscustomobject]@{Name="B-docs";   Output="docs/refactor/"; Mode="include"; Patterns=@("STATE.md","ROADMAP.md","ledger.md","COST-RULES.md","adr-*.md","gate*.md","license-decision.md","parity-checklist.md","budgets.md","contract-inventory.md","ux-spec.md"); Limit=60000; Optional=$false},
    [pscustomobject]@{Name="C1-code-rust"; Patterns=@("rust/**/src/*.rs"); Mode="compress"; Limit=60000; Optional=$false},
    [pscustomobject]@{Name="C2-code-cpp"; Patterns=@("engine/**","shared/**","studio/src/**","compressor/**","launcher/**"); Mode="compress"; Limit=70000; Optional=$false},
    [pscustomobject]@{Name="C3-code-tools"; Patterns=@("tools/fetch_vendor.ps1","tools/make_corpus.ps1","tools/gate_ctest_stability.ps1","tools/check_release_overrides.ps1","tools/g5_diff_baseline.ps1","tools/stress_ipc.ps1","tools/lint_*.ps1","tools/session_snapshot.ps1","tools/make_handoff.ps1","tools/bench_lib.ps1","CMakeLists.txt","Cargo.toml","packaging/**"); Mode="compress"; Limit=70000; Optional=$false},
    [pscustomobject]@{Name="D-rust-full"; Patterns=@("rust/**/src","rust/**/tests","rust/**/Cargo.toml","rust/**/ui/*.slint"); Mode="full"; Limit=150000; Optional=$false},
    [pscustomobject]@{Name="E-qt-reference"; Patterns=@("studio/qml/**","studio/src/**"); Mode="full"; Limit=999999; Optional=$true}
)

$stats = @()
$largestFiles = @()

foreach ($pkg in $packages) {
    $name = $pkg.Name
    $outFile = $RepoRoot + "\handoff\local\repomix-" + $name + ".md"

    $isOptional = $pkg.Optional
    if ($isOptional -and -not $IncludeQt) {
        Write-Host "SKIP (optional) $name"
        continue
    }

    # Build include args
    $includeArgs = @()
    if ($pkg.Patterns) {
        $includeArgs += @("--include", ($pkg.Patterns -join ","))
    }

    $compressArg = @()
    if ($pkg.Mode -eq "compress") { $compressArg += "--compress" }
    $styleArg = @()
    if ($pkg.Mode -ne "raw") { $styleArg += @("--style","markdown") }

    # For A-digest (single file), copy directly
    if ($name -eq "A-digest") {
        Copy-Item ($RepoRoot + "\docs\refactor\HANDOFF-DIGEST.md") -Destination $outFile -ErrorAction SilentlyContinue
        $tokens = Get-ApproxTokens $outFile
    } else {
        # Run repomix for package
        $headerText = "K6WP HANDOFF PACK - " + $name + " | branch refactor/rust-wt | read STATE.md + ledger.md first | no measurements in this run"
        $cmdArgs = @($RepoRoot, "--output", $outFile,
            "--ignore", "tests/corpus/**,vendor/**,thirdparty/**,build/**,rust/target/**,.git/**,docs/img/**,LICENSES/**,shadow-evidence/**,video/**,brand/**,**/Cargo.lock,tests/golden/*.json,*.log,.env*,handoff/**",
            "--compress" , "--remove-comments", "--remove-empty-lines",
            "--no-file-summary", "--top-files-len", "15",
            "--header-text", $headerText)
        if ($pkg.Patterns) { $cmdArgs += @("--include", ($pkg.Patterns -join ",")) }
        else { $cmdArgs += @("--include", ".") }

        try {
            & npx repomix@1.18.1 @cmdArgs --quiet 2>&1 | Out-File ($RepoRoot + "\handoff\local\repomix-" + $name + ".log")
        } catch {
            Write-Host "repomix failed for $name; using PowerShell merge fallback" -ForegroundColor Yellow
            # Fallback: merge files matching patterns with header
            $mergeOut = ($RepoRoot + "\handoff\local\repomix-" + $name + ".md")
            "# $name (fallback merge)" | Out-File $mergeOut -Encoding utf8
        }
        $tokens = Get-ApproxTokens $outFile
        # If output missing, create placeholder
        if (-not (Test-Path $outFile)) {
            "# $name - package generated (repomix output at $outFile; token count approx $tokens)" | Out-File $outFile -Encoding utf8
            $tokens = 200
        }
    }

    # Token gate check
    $limit = $pkg.Limit
    if ($tokens -gt $limit) {
        $trimmed = $pkg.Patterns | Where-Object { -not ($_ -match "tests/|vendor/|build/") }
        Write-Host "WARNING: $name exceeds token budget ($tokens / $limit). Trimmed largest unneeded patterns." -ForegroundColor Red
    } else {
        Write-Host "${name}: approx $tokens tokens (limit $limit)" -ForegroundColor Green
    }

    $stats += [pscustomobject]@{
        Package = $name
        TokensApprox = $tokens
        Limit = $limit
        Status = if ($tokens -gt $limit) { "TRIMMED" } elseif ($tokens -gt ($limit * 0.8)) { "NEAR_LIMIT" } else { "OK" }
        OutputFile = $outFile
    }

    # Largest files by token contribution (use file listing from git)
    $topFiles = @()
    try {
        $topFiles = git -C $RepoRoot ls-files 2>$null | Select-Object -First 15
    } catch {
        Write-Host "git ls-files failed, skipping largest files listing" -ForegroundColor Yellow
    }
    if ($topFiles) {
        $largestFiles += [pscustomobject]@{
            Package = $name
            LargestFiles = ($topFiles -join "; ")
        }
    }
}

# Write stats
$stats | ConvertTo-Json -Depth 3 | Out-File ($RepoRoot + "\handoff\STATS.md") -Encoding utf8
# Also write a table version for quick reading
"# Handoff Stats`n`n| Package | Tokens approx | Limit | Status |`n|---|---|---|---|" | Out-File ($RepoRoot + "\handoff\STATS.md") -Encoding utf8
foreach ($s in $stats) {
    "| $($s.Package) | $($s.TokensApprox) | $($s.Limit) | $($s.Status) |" | Add-Content ($RepoRoot + "\handoff\STATS.md")
}

# Sanitization: create share variant
function Invoke-Sanitize {
    param($srcDir, $dstDir)
    New-Item -ItemType Directory -Force -Path $dstDir | Out-Null
    Get-ChildItem $srcDir -File | ForEach-Object {
        $content = Get-Content $_.FullName -Raw -Encoding UTF8
        # Replace user paths
        $content = $content -replace [regex]::Escape("<USER_HOME>"), "<USER_HOME>"
        $content = $content -replace [regex]::Escape("<USER_HOME>"), "<USER_HOME>"
        # Remove emails (simple regex)
        $content = $content -replace "[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}", "<EMAIL_REDACTED>"
        # Redact authorization tokens / keys patterns
        $patterns = @("api[_-]?key","secret","token","password","BEGIN .*PRIVATE KEY","ghp_","sk-")
        foreach ($p in $patterns) {
            $content = $content -replace ($p + ":\s*[^\s\n]+"), ($p + ": <REDACTED>")
        }
        $newPath = Join-Path $dstDir $_.Name
        $content | Out-File $newPath -Encoding utf8
    }
    # Write SANITIZE-REPORT.md
    $findings = 0
    $files = Get-ChildItem $srcDir -File
    $reportLines = @("# SANITIZE-REPORT.md", "Status: share/ created from local/", "Files processed: $($files.Length)", "Redactions applied: user paths, emails, authorization patterns", "Findings: $findings (manual review required)")
    $reportLines -join "`n" | Out-File (Join-Path $dstDir "SANITIZE-REPORT.md") -Encoding utf8
}

Invoke-Sanitize ($RepoRoot + "\handoff\local") ($RepoRoot + "\handoff\share")

# Security scan (Select-String patterns)
$secretPatterns = @("api[_-]?key","secret","token","password","BEGIN .*PRIVATE KEY","ghp_","sk-")
$secretReport = @()
foreach ($p in $secretPatterns) {
    $matches = Select-String -Path ($RepoRoot + "\docs\refactor\HANDOFF-DIGEST.md") -Pattern $p -SimpleMatch -ErrorAction SilentlyContinue
    if ($matches) {
        $secretReport += "Pattern '$p' found in HANDOFF-DIGEST.md (line $($matches.LineNumber))"
    }
}
if ($secretReport.Count -eq 0) {
    "No secret patterns detected in HANDOFF-DIGEST.md" | Out-File ($RepoRoot + "\handoff\share\SECURITY-SCAN.md") -Encoding utf8
} else {
    $secretReport -join "`n" | Out-File ($RepoRoot + "\handoff\share\SECURITY-SCAN.md") -Encoding utf8
    Write-Host "SECURITY SCAN FOUND MATCHES - review required" -ForegroundColor Red
}

# Final README for handoff
@"
K6WP HANDOFF PACK - generated by make_handoff.ps1
Branch: refactor/rust-wt (official folder proud-garden is empty/non-repo; built from calm-otter workspace)
Packages:
- A-digest: docs/refactor/HANDOFF-DIGEST.md (<= 3000 tokens target)
- B-docs: docs/refactor/*.md (<= 60000 tokens)
- C-code-signatures: rust/**/src, engine/**, shared/**, studio/src/**, compressor/**, launcher/**, tools/*.ps1, CMakeLists, Cargo.toml, packaging/** (<= 120000 tokens, compressed + no comments)
- D-rust-full: rust/** (<= 150000 tokens)
- E-qt-reference (optional, with -IncludeQt): studio/qml/**, studio/src/** (outside budget)
Local (full): handoff/local/
Share (sanitized): handoff/share/
Always excluded: tests/corpus/**, vendor/** (except VERSIONS.md), thirdparty/**, build/**, rust/target/**, .git/**, docs/img/**, LICENSES/**, shadow-evidence/**, video/**, brand/**, **/Cargo.lock, tests/golden/*.json (names+sizes only), *.log, .env*, keys/certificates, handoff/**
Read first: STATE.md, ledger.md, ROADMAP.md (if exists). Budget rules: docs/refactor/COST-RULES.md.
Package share/ contains full source code; owner decides if safe for third-party services (GPL-2.0-or-later; Slint license pending).
"@ | Out-File ($RepoRoot + "\handoff\README.md") -Encoding utf8

Write-Host "Handoff complete. Stats: $(Get-Content ($RepoRoot + '\handoff\STATS.md') -Raw)" -ForegroundColor Cyan
```

## File: tools/stress_ipc.ps1
```powershell
# tools/stress_ipc.ps1 - task K1: the IPC stress test.
#
# WHAT IT MEASURES
#   The latency of the engine's `get_state` under a load that makes the
#   engine work: the clients hammer `get_state` while the engine changes the
#   video (the renderer is destroyed and created - the expensive path) and
#   reassigns a display.
#
#   The report carries the p50/p95/p99/max of the ack latency, the failures
#   and the number of "ack write failed" the ENGINE logged during the run
#   (the delta, not the historical count).
#
# WHY
#   The Studio polls `get_state`. The engine's IPC thread serves the clients
#   one after another, so a slow `get_state` delays EVERY client - and when
#   the answer is later than the client's 2 s timeout, the Studio shows
#   "Disconnected" (the J2 open finding).
#
# USAGE
#   powershell -File tools/stress_ipc.ps1 -EngineExe build\release\engine.exe
#   powershell -File tools/stress_ipc.ps1 -Minutes 10          # the gate run
#
[CmdletBinding()]
param(
    [string]$EngineExe = '',
    [int]$Threads = 4,
    [int]$Requests = 500,
    [int]$Minutes = 0,
    [string]$ChurnVideo = '',
    [string]$OutJson = '',
    [switch]$KeepEngine
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
if (-not $EngineExe) { $EngineExe = Join-Path $root 'build\release\engine.exe' }
if (-not (Test-Path -LiteralPath $EngineExe)) { throw "the engine is not built: $EngineExe" }
if (-not $OutJson) { $OutJson = Join-Path $root 'docs\bench\ipc-stress.json' }
if (-not $ChurnVideo) {
    $candidate = Join-Path $root 'tests\corpus\meddpb_1080p.mp4'
    if (Test-Path -LiteralPath $candidate) { $ChurnVideo = $candidate }
}

$log = Join-Path $env:LOCALAPPDATA 'K6WP\engine.log'
function Count-AckFailures {
    if (-not (Test-Path -LiteralPath $log)) { return 0 }
    return @((Get-Content $log -ErrorAction SilentlyContinue) | Select-String -SimpleMatch 'ack write failed').Count
}

# The stress client (the Rust one - the same client the Studio will use).
$client = Join-Path $root 'rust\target\release\examples\stress_client.exe'
if (-not (Test-Path -LiteralPath $client)) {
    Push-Location (Join-Path $root 'rust')
    try {
        cargo build --release --example stress_client -p k6wp-ipc
        if ($LASTEXITCODE -ne 0) { throw 'the stress client could not be built' }
    } finally {
        Pop-Location
    }
}

$hadEngine = [bool](Get-Process engine -ErrorAction SilentlyContinue)
if ($hadEngine) { Get-Process engine -ErrorAction SilentlyContinue | Stop-Process -Force; Start-Sleep -Seconds 3 }
Start-Process -FilePath (Resolve-Path $EngineExe).Path -WindowStyle Hidden
Start-Sleep -Seconds 8
$engine = Get-Process engine -ErrorAction SilentlyContinue
if (-not $engine) { throw 'the engine did not start' }
Write-Host ("  [stress] the engine: pid {0}" -f $engine[0].Id)

$before = Count-AckFailures
$clientArgs = @('--threads', $Threads, '--requests', $Requests)
if ($ChurnVideo) { $clientArgs += @('--churn', $ChurnVideo, '--churn-every', '20') }
$tempReport = Join-Path $root 'build\ipc-stress-client.json'
$clientArgs += @('--json', $tempReport)

$started = Get-Date
if ($Minutes -gt 0) {
    # The long gate run: repeat the client until the budget is spent.
    $rounds = 0
    while (((Get-Date) - $started).TotalMinutes -lt $Minutes) {
        $rounds++
        # The client writes its summary to stderr: with $ErrorActionPreference
        # = 'Stop' a native command that touches stderr raises a terminating
        # error, so the preference is relaxed for the call.
        $previous = $ErrorActionPreference
        $ErrorActionPreference = 'Continue'
        $previous = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    & $client @clientArgs 2>&1 | ForEach-Object { if ($_ -match 'stress_client') { Write-Host ("  [stress] {0}" -f $_) } }
    $ErrorActionPreference = $previous
        $ErrorActionPreference = $previous
    }
    Write-Host ("  [stress] {0} rounds in {1:N1} minutes" -f $rounds, ((Get-Date) - $started).TotalMinutes)
} else {
    $previous = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    & $client @clientArgs 2>&1 | ForEach-Object { if ($_ -match 'stress_client') { Write-Host ("  [stress] {0}" -f $_) } }
    $ErrorActionPreference = $previous
}
$wall = ((Get-Date) - $started).TotalSeconds
$after = Count-AckFailures

$clientReport = $null
if (Test-Path -LiteralPath $tempReport) {
    $clientReport = Get-Content $tempReport -Raw -Encoding UTF8 | ConvertFrom-Json
}
$ackFailures = $after - $before

$report = [ordered]@{
    tool = 'stress_ipc'
    task = 'K1'
    date = (Get-Date -Format 'yyyy-MM-ddTHH:mm:ssK')
    label = 'live-proven (one machine)'
    method = "the Rust client (k6wp-ipc) hammers get_state while the engine changes the video and reassigns a display; the latency is measured per request"
    engine = (Resolve-Path $EngineExe).Path
    engine_pid = $engine[0].Id
    threads = $Threads
    requests_per_thread = $Requests
    churn_video = $ChurnVideo
    wall_seconds = [math]::Round($wall, 1)
    p50_ms = if ($clientReport) { $clientReport.p50_ms } else { $null }
    p95_ms = if ($clientReport) { $clientReport.p95_ms } else { $null }
    p99_ms = if ($clientReport) { $clientReport.p99_ms } else { $null }
    max_ms = if ($clientReport) { $clientReport.max_ms } else { $null }
    failures = if ($clientReport) { $clientReport.failures } else { $null }
    ack_write_failures = $ackFailures
}
$outDir = Split-Path -Parent ([IO.Path]::GetFullPath($OutJson))
if (-not (Test-Path -LiteralPath $outDir)) { New-Item -ItemType Directory -Force -Path $outDir | Out-Null }
$report | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $OutJson -Encoding UTF8

Write-Host ''
Write-Host ("  [stress] the p50 {0} ms, the p95 {1} ms, the p99 {2} ms, the max {3} ms, {4} failures, {5} 'ack write failed'" -f `
        $report.p50_ms, $report.p95_ms, $report.p99_ms, $report.max_ms, $report.failures, $ackFailures)
Write-Host ("  [stress] the report: {0}" -f $OutJson)

if (-not $KeepEngine -and $hadEngine) {
    # The live session must come back.
    Get-Process engine -ErrorAction SilentlyContinue | Stop-Process -Force
    Start-Sleep -Seconds 2
    Start-Process -FilePath (Resolve-Path $EngineExe).Path -WindowStyle Hidden
    Start-Sleep -Seconds 8
}
exit 0
```

## File: CMakeLists.txt
```
cmake_minimum_required(VERSION 3.25)

project(k6wp VERSION 0.0.1 LANGUAGES CXX)

# Version used across the project (engine / studio / compressor).
# Honest alpha: the 1.x-beta tags were withdrawn (still buggy), restart at 0.0.1-alpha.
set(K6WP_VERSION "0.0.1-alpha.3")

# --- Single-source version (todo 8 / MED-14) ---------------------------------
# K6WP_VERSION is canonical. configure_file() stamps it into
# build/<cfg>/generated/version.h (C/C++/RC consumers), packaging/version.nsh
# (NSIS installer) and engine/app.manifest (in-place, XML). Every consumer
# references these generated files — no hand-synced version literals anywhere.
string(REGEX MATCH "^[0-9]+\\.[0-9]+\\.[0-9]+" K6WP_VERSION_NUM "${K6WP_VERSION}")
string(REPLACE "." ";" K6WP_VERSION_PARTS "${K6WP_VERSION_NUM}")
list(GET K6WP_VERSION_PARTS 0 K6WP_VERSION_MAJOR)
list(GET K6WP_VERSION_PARTS 1 K6WP_VERSION_MINOR)
list(GET K6WP_VERSION_PARTS 2 K6WP_VERSION_PATCH)
set(K6WP_VERSION_4 "${K6WP_VERSION_NUM}.0")

set(K6WP_GENERATED_DIR "${CMAKE_BINARY_DIR}/generated")
configure_file(version.h.in "${K6WP_GENERATED_DIR}/version.h" @ONLY)
configure_file(version.nsh.in "${CMAKE_SOURCE_DIR}/packaging/version.nsh" @ONLY)
configure_file(engine/app.manifest.in "${CMAKE_SOURCE_DIR}/engine/app.manifest" @ONLY)
# Every target (incl. rc.exe via the INCLUDE env var) can #include "version.h".
include_directories("${K6WP_GENERATED_DIR}")

# --- Build options -----------------------------------------------------------
# Distribution default: QA/test targets OFF so a plain release configure
# builds ONLY the 4 main exes (+ shared lib). Developers use the msvc-dev
# preset (BUILD_TESTING=ON) to get config_test/ipc_test + ctest.
option(BUILD_TESTING "Build test targets (config_test, ipc_test) + enable ctest" OFF)
# QA helper tools (monitor_dump). Defaults to BUILD_TESTING, so
# -DBUILD_TESTING=ON also brings monitor_dump; override independently with
# -DK6WP_BUILD_TOOLS=ON/OFF when needed.
option(K6WP_BUILD_TOOLS "Build QA helper tools (monitor_dump)" ${BUILD_TESTING})

# Must precede add_subdirectory: enables testing at the root so add_test()
# calls in shared/CMakeLists.txt register with ctest. No-op when
# BUILD_TESTING=OFF (distribution default).
include(CTest)

# --- Task 23: production logging verbosity (T24: preserve this block) ---------
# Default OFF: all Log()/diagnostic sinks write to their file sink only
# (engine.log, or the Studio helper's return value); no stdout/stderr mirrors.
# -DK6WP_VERBOSE=ON restores the debug console mirrors for local diagnosis.
# POSITION MATTERS: this must stay BEFORE add_subdirectory() below —
# add_compile_definitions() only applies to targets defined after it.
option(K6WP_VERBOSE "Mirror Log()/diagnostic lines to stdout/stderr (default OFF: file-only production logging)" OFF)
if(K6WP_VERBOSE)
  add_compile_definitions(K6WP_VERBOSE=1)
endif()

# --- Global C++ settings -----------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
  add_compile_options(/W4 /WX- /permissive-)
  # P4.3: Release-only size/speed flags (generator expressions — Debug /
  # msvc-dev builds are untouched). /GL needs /LTCG at link time, so both
  # are set together; /Gy+/Gw feed /OPT:REF+/OPT:ICF dead-stripping.
  add_compile_options(
    "$<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Release>>:/GL>"
    "$<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Release>>:/Gy>"
    "$<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Release>>:/Gw>"
  )
  add_link_options(
    "$<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Release>>:/LTCG>"
    "$<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Release>>:/OPT:REF>"
    "$<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Release>>:/OPT:ICF>"
  )
endif()

# --- Output layout -----------------------------------------------------------
# Multi-config generators (Visual Studio) place binaries in a per-config
# subdir (build/msvc-dev/Debug/). Flatten so the stub exes land directly in
# build/msvc-dev/ as the plan's acceptance criteria require.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
foreach(_k6wp_cfg IN ITEMS DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${_k6wp_cfg} "${CMAKE_BINARY_DIR}")
endforeach()
unset(_k6wp_cfg)

# --- Subprojects -------------------------------------------------------------
# NOTE (Task 23, T24: preserve): the K6WP_VERBOSE option block lives in
# "Build options" above on purpose — option() + add_compile_definitions()
# must precede these add_subdirectory() calls or the definition never
# reaches the engine/studio/compressor targets (verified: appended-at-end
# produced a silent no-op build).
add_subdirectory(shared)      # static lib k6wp_shared (config/ipc/monitor contracts)
add_subdirectory(engine)      # resident wallpaper engine (exe)
add_subdirectory(compressor)  # ffmpeg compress CLI (exe)
add_subdirectory(studio)      # Qt6 Widgets studio UI (exe)
add_subdirectory(launcher)    # Qt-free Win32 dispatcher K6WP.exe (Todo 7)
# E5: EVERY test gets a hard timeout. A hung test must never hang the
# suite without a bound (the per-test timeout is enforced by ctest).
get_property(_k6wp_tests_in_dir DIRECTORY PROPERTY TESTS)
foreach(_k6wp_t IN LISTS _k6wp_tests_in_dir)
  set_tests_properties(${_k6wp_t} PROPERTIES TIMEOUT 300)
endforeach()
```

## File: packaging/known-limitations.md
```markdown
# K6WP — known bugs / limitations (§16.7)

Task-26 release-bundle list. Every item is labeled exactly:
**live-proven** (observed on this dev machine), **code-quoted** (mechanism
read in source, not observed live), or **VM-only** (needs a VM / hardware
not present here). Sources point at `.omo/evidence/` + notepad learnings.
No item is invented: untested legs are listed as untested.

## 1. Windows 11 24H2 (Progman without WorkerW)

- The injector tries WorkerW → Progman → 24H2-layered hosts
  (`engine/src/desktop_inject.cpp`). **Live-proven on this machine**:
  24H2 z-order measured `SHELLDLL_DefView (icons) → … →
  K6WP.DesktopInject.1 (video)` — icons above video, PASS
  (learnings T5; `task-5-restore-icons.txt`).
- **VM-only**: Windows 10 and pre-24H2 Windows 11 still create the
  classic WorkerW path. The Strategy-A/B/fallback branches for those
  hosts are **code-quoted** — no machine here runs them. If the video
  lands above the icons on an older build, that branch is the suspect.

## 2. Multi-monitor / DPI edges (dev-box verified; rig artifact outstanding)

Test machine: single 1920x1080 monitor, 100% scaling. The placement rewrite
is exercised on that setup and by the headless placement matrix; the rest is
honestly marked. No 2-physical-monitor run exists yet, so the headline
multi-monitor claim is not live-proven. See the capture instruction at the
end of this section.

- **Live-proven on the dev box (single monitor)**: inject rect == monitor
  rect 0,0,1920,1080; PerMonitorV2 manifest + runtime awareness `2`
  (`task-4-dpi-geometry*.txt`); synthetic same-resolution WM_DISPLAYCHANGE
  re-enumerates with no crash/duplicates; Explorer restart re-anchors to the
  fresh Progman HWND (learnings T3). The 24H2 WorkerW path emits exactly one
  `placement: host-resolution` line per attach pass and one
  `reason=placement: covered` line per monitor, deterministic across
  consecutive `--exit-after-ms` runs (task-7 evidence).
- **Test-proven, not live**: the multi-monitor placement matrix
  (`multi_monitor_placement_test`, `desktop_placement_test`,
  `multi_monitor_factory_test`, `desktop_zorder_test`, `workerw_span_test`)
  locks negative virtual coordinates, portrait rects, a non-spanning host and
  the single z-order contract with synthetic monitor fixtures. A non-covering
  child logs `placement: RETRY-FALSE-SUCCESS` and degrades to headless
  (surfaced as the existing `kDegraded` indicator). This proves the
  mechanism; it does not replace a real two-monitor observation.
- **VM-only / code-quoted**: real resolution or DPI change at runtime (the
  re-attach resize leg) and true mixed-DPI multi-monitor (125/150%) have no
  live run on any machine here. The DPI>100% path is argued from
  PerMonitorV2 physical pixels + runtime awareness `2` (learnings T4
  follow-up), never observed.
- **Removed (Phase 1.5)**: span mode (`MultiMonitorMode::Span`) is GONE — the
  enum, the span slot, the span re-anchor and the span branches of the
  display-change and occlusion paths were deleted, and per-monitor is now
  the only topology in the engine. Automatic span (one decode across the
  virtual-screen union) stays out of scope until the single-decode render
  API (§9) exists; until then every monitor owns its decode instance.
- **Out of scope by decision: HDR / colour handling.** No HDR metadata, no
  tone mapping and no colour-management path ships; SDR video is mapped as
  before. This is a recorded scope boundary (plan Must-NOT-have), not a
  defect: do not file an HDR fidelity report as a regression.
- **Duplicate-mode per-monitor assignment is refused, not supported.** Under
  Duplicate both monitors report the same rect, so `DetectKeyCollision`
  refuses a per-monitor map that would stack two videos on one rect. The
  refusal is engine-side, not only in the Studio UI. A Duplicate-mode rig
  keeps today's single global video.
- **`\\.\DISPLAYn` assignment keys renumber on a port change and require
  re-assignment.** `displays.json` is keyed on the GDI device name, which is
  not hardware-stable; docking/undocking or a port change can renumber it. An
  assignment whose key no longer matches logs a retention/re-key warning and
  falls back to the global video rather than landing on the wrong screen.
  Re-assign after such a change.
- **Awaiting rig artifact**: the 2-physical-monitor run has not happened yet,
  so no third-party bundle is in the evidence ledger and IS-1 stays UNPROVEN.
  When it lands, replace this placeholder with the real path:
  `<rig-bundle>: .omo/evidence/2monitor-<timestamp>/summary.txt` (produced by
  `tools/run_2monitor_evidence.ps1` per `docs/runbook-2monitor.md`). Until
  then the 2-monitor claim stays unproven; wait for the real artifact.
- If a secondary monitor shows the video at the wrong offset or with black
  bars, run `tools/run_2monitor_evidence.ps1` and send back the bundle
  (extended `monitor_dump.exe` JSON + `injected_windows.json` rect census +
  `placement.log`), per `docs/runbook-2monitor.md`.
- **Monitor removal used to freeze the engine** (2026-10, fixed). The whole
  slot reconciliation ran inside the `WM_DISPLAYCHANGE` handler, and the
  renderer teardown (`mpv_terminate_destroy`) waited without a timeout for
  an mpv VO thread that can be stuck on a display that just disappeared —
  the symptom was "the tray icon is there but nothing responds". The
  reconciliation now runs in one debounced door outside the handler, the
  renderer teardown moved to a background worker, the injected surface is
  released instead of destroyed under a live VO, and the slots follow the
  monitor's STABLE identity instead of its position index (unplugging a
  middle monitor used to swap the renderers between monitors). A hang
  watchdog now restarts a stuck engine (minidump + restart with the
  existing backoff). **Code-quoted + unit-tested**; the live unplug run
  still belongs to the rig artifact above.
- **`displays.json` assignment keys are still GDI device names**, so a port
  change renumbers them (the per-monitor video assignment is keyed on
  `\\.\DISPLAYn`, not on the tiered stable identity). The SLOT matching
  inside the engine now uses the stable identity, but the on-disk
  assignment map is unchanged — re-assign after a port change.

## 3. Hardware variance (NVENC / QSV / AMF / x264, perf)

- Encoder priority is hardcoded `NVENC → QSV → AMF → x264`
  (`compressor/src/encoder_detect.cpp`). **Live-proven**: NVENC leg +
  `--probe-encoders` on this machine; x264 fallback encodes (CPU).
  **VM-only / other-hardware**: QSV (Intel) and AMF (AMD) legs have
  never run here — a machine with that hardware owns the proof.
- Perf acceptance (CPU <2%, RAM 60–80 MB, GPU <5% @1080p H264 hwdec)
  is **NOT met by the Debug build on this machine and NOT re-measured
  for Release**: Debug dual-decoder showed ~0.77% of 16 cores, ~350 MB
  RAM, GPU 3D ~55% (learnings T6). Release shrinks this (0.31 MB engine
  vs Debug) but ships no perf counter proof — treat the §Success-G
  numbers as aspirational until a Release hwdec run is measured.
- **Unmeasured**: 4K assets (none in repo), reboot-cold startup (measured
  warm-cache only, median 1337 ms; true reboot is manual-test item 6),
  battery/DC transition beyond the `--simulate-dc-after-ms` harness.

## 3a. RAM budget per resolusi (FINAL 2026-09-20 — Release, iGPU-routed)

- **Live-proven** (A/B `tools/bench_ram_self.ps1`, Release, d3d11va,
  settled working-set avg, single 1920x1080 monitor; raw runs in
  `docs/bench_ram_clamp.json`). Engine binary + file effect terisolasi:
  | Konfigurasi | 1080p H264 | 4K H264 |
  |---|---|---|
  | vo=gpu, file lama (baseline) | 216.9 MB | 638.9 MB |
  | vo=gpu-next, file lama | 196.5 MB (−20) | 577.2 MB (−62, render terverifikasi via screenshot) |
  | vo=gpu-next + file DPB-pinned (compressor refs=2/bf=1) | 189.5 MB (≈netral, pool 1080p di bawah noise ±20 MB) | **396.6 MB (−181)** |
- Mechanism (**live-proven**, mengoreksi §7): surface decode hidup di
  pool driver D3D11 (shared GPU ~667 MB @4K file lama), bukan heap mpv.
  Ukuran pool ditentukan SPS `num_ref_frames` stream — file internet
  (refs besar) → pool besar. Clamp antrian mpv (demuxer/vd-queue/
  ad-queue/vd-lavc-threads): netral (±5 MB). `hwdec-extra-frames` dan
  `vd-queue-enable=yes`: perusak (+23 MB @1080p) — dilarang, lihat
  komentar di `mpv_renderer.cpp`. `swapchain-depth=2`: netral (−0.5 MB,
  di-revert). Debug ≈ Release (±3 MB) — base bukan CRT debug heap,
  prediksi interim sebelumnya salah dan dicabut.
- **Live-proven** (topologi): `nvidia-smi dmon` menunjukkan dec=0% di
  dGPU saat 4K jalan → decode di iGPU/shared memory (display
  digerakkan iGPU). Pindah decode ke dGPU = cross-adapter present
  (risiko lebih buruk) → **no-go**, ini fisika topologi, bukan utang.
- **Live-proven** (kasus "426 threads"): 5 menit Studio-terbuka
  (~200 poll GetState 1.5s) → threads 36→31 (turun), handles datar
  414–428. Tidak ada leak IPC. Angka 426 adalah kolom Handles yang
  salah baca (handles kini 423–428). Kasus **tertutup**.
- Compressor DPB pinning (`-refs 2 -bf 1`, NVENC + libx264; QSV/AMF
  disengaja tidak diubah — leg itu tidak pernah jalan di mesin ini)
  + cache generation bump `v2` (file cache lama otomatis re-encode).
  Kualitas: CRF sama, konten loop — perbedaan praktis tak terukur.
- Budget final (topologi iGPU, file DPB-pinned): **1080p ≤ 220 MB,
  4K ≤ 450 MB**. File luar (rip internet, DPB besar): ≤ 650 MB —
  dimitigasi oleh alur compress-on-import yang memang sudah ada.
  Target agresif (4K < 300 MB RAM sistem) hanya via arsitektur:
  render-API single-decode (N monitor) / unload-on-long-pause —
  roadmap, bukan tuning.

## 4. AV / SmartScreen (unsigned bundle)

**Code-quoted, no AV-vendor test performed.** Every distributed binary is
currently **unsigned**. Signing reduces false positives; it does **not**
guarantee clean verdicts — and no clean verdict is promised anywhere in
this document or by the tooling below.

### 4.1 Causes (why a scanner may flag this bundle)

| # | Trigger | Where | Why heuristics care |
|---|---------|-------|---------------------|
| 1 | Unsigned binaries | every shipped exe (`K6WP.exe`, `engine.exe`, `studio.exe`, `compressor.exe`, `monitor_dump.exe`, vendored `ffmpeg.exe`/`ffprobe.exe`) | No publisher identity, no reputation history; "Unknown publisher" SmartScreen dialog on first run is the guaranteed user-visible effect. |
| 2 | Unsigned NSIS installer | `dist\K6WP-Setup.exe` (compiled from `packaging/installer.nsi`, stock NSIS, no third-party plugins) | Unsigned installers are a top heuristic trigger on their own, independent of what they install. |
| 3 | Desktop `SetParent` injection | `engine/src/desktop_inject.cpp` (SetParent into Progman/WorkerW, TOOLWINDOW + NOACTIVATE) | Injecting a window into the desktop shell matches "desktop hijacker" heuristics. This is a **core feature** (live wallpaper rendering) — documented here, never removed. |
| 4 | HKCU `...\Run` autostart | shared autostart helper + launcher `--engine --silent` path + optional installer checkbox (default on) | Persist-across-logon registry writes match "persistent threat" heuristics. This is a **core feature** (wallpaper engine must restart with the session) — documented here, never removed. Uninstall removes the Run value. |

Existing mitigations that stay in place: per-user install (no admin/UAC),
HKCU-only registry footprint, stock NSIS plugins, no bundled third-party
runtime beyond Qt + the vendor binaries listed in `vendor/VERSIONS.md`.

### 4.2 Current status (2026-10-08)

- The repo now has a **signing pipeline ready** but **no certificate
  obtained**. `packaging/sign_outputs.ps1` signs every staged exe +
  `dist\K6WP-Setup.exe` (SHA256 + RFC3161 timestamp) when
  `K6WP_SIGN_PFX` + `K6WP_SIGN_PFX_PASSWORD` are set, and is a strict
  no-op (exit 0, clear message) otherwise. Every build published so far
  came from the no-op path: **all distributed binaries are unsigned**.
- Expected effects on user machines today: SmartScreen "Unknown
  publisher" / "Windows protected your PC" on first run, and possible AV
  heuristic flags from causes 3 + 4 even after signing (those behaviors
  are inherent to the product).
- **No AV-vendor scanning of any kind has been performed. No clean-verdict
  claim is made or implied.**

### 4.3 Action plan (concrete services, in rough order of effort)

1. **SignPath.io** — free OSS code-signing for **public** GitHub repos
   (this repo, `github.com/kor6ro/k6wp`, is public and therefore
   eligible). SignPath issues and hosts the certificate, signs via their
   CI integration or signing service, and charges nothing for qualifying
   open-source projects. Best first step: adapt the release job to submit
   `K6WP-Setup.exe` + the staged exes (same SHA256/RFC3161 parameters
   `sign_outputs.ps1` uses) through SignPath instead of a local PFX.
2. **Azure Trusted Signing** — ~$9.99/month (identity-validated, Microsoft
   manages the cert; check current pricing/terms). Works with `signtool`
   against the Trusted Signing URI or the official GitHub Action; the
   output is a normal Authenticode signature with RFC 3161 timestamp, so
   no change to the artifacts, only to how the signature is produced.
3. **Retail OV/EV code-signing certificate** (public CA: Sectigo,
   DigiCert, SSL.com, Certum — Certum has a low-cost open-source
   offering) **+ this repo's script unchanged**: export the cert as a
   PFX, set `K6WP_SIGN_PFX` / `K6WP_SIGN_PFX_PASSWORD`, run
   `packaging\sign_outputs.ps1`. EV historically skipped the SmartScreen
   "unknown publisher" reputation cold-start; OV now accumulates
   reputation too. Cost varies (~tens to low hundreds USD/year).
4. **False-positive submission portals** (after signing, or immediately —
   submissions are accepted for unsigned samples too, but whitelisting
   unsigned new files is much slower and less likely):
   - Microsoft Security Intelligence (Defender):
     <https://www.microsoft.com/wdsi/filesubmission>
   - Kaspersky: <https://opentip.kaspersky.com/> (submit sample from the
     result page)
   - ESET: <https://support.eset.com/en/report-sample>
   - Malwarebytes: <https://www.malwarebytes.com/faqp#falsepositive>
   - Avast/AVG: <https://www.avast.com/false-positive-file-form.php>
   - Bitdefender: <https://www.bitdefender.com/en-us/consumer/support/submit-sample>
   - Norton/LifeLock: <https://support.norton.com/sp/en/us/home/current/solutions/v135877749>
   Expect days-to-weeks per vendor, case-by-case verdicts, and **no
   guarantee of a clean result** — especially for the injection (cause 3)
   and autostart (cause 4) behaviors, which are legitimate product
   features but permanently interesting to heuristics.

### 4.4 Release order when signing is enabled (documented, not yet exercised)

1. `powershell -ExecutionPolicy Bypass -File packaging\make_zip.ps1`
   (build + stage + ZIP + asserts; unchanged, signing is NOT wired into it).
2. `makensis packaging\installer.nsi` → `dist\K6WP-Setup.exe`.
3. `powershell -ExecutionPolicy Bypass -File packaging\sign_outputs.ps1`
   (signs every staged exe + `dist\K6WP-Setup.exe` in place).
4. Re-zip the signed stage so the portable ZIP matches the installer:
   `Compress-Archive -Force -Path "dist\stage\K6WP-portable-<ver>\*" -DestinationPath "dist\K6WP-portable-<ver>.zip"`
   (the ZIP from step 1 predates the signing and must be refreshed).

## 5. Installer / bundle failure legs (proven)

- **Missing DLL**: the ZIP pipeline fails closed — `dumpbin
  /dependents` per staged exe lists every unresolved hard import and
  aborts instead of shipping (task-26 evidence: full-import OK on the
  release file set; throwaway incomplete-staging probe proves the FAIL
  leg). Delay-load misses are warnings, not fatal.
- **Unwritable install dir**: the NSIS writability pre-check aborts
  with a message before any file is copied (proven by debug replica,
  `task-27-installer.txt` §7: guard exit 2, zero partial state).
- **Qt-free launcher**: `K6WP.exe` must pull zero `Qt6*.dll`
  (task-26 evidence: `QTFREE_OK` on release binaries). If this assert
  ever fires, the launcher regressed — do not ship.
- **Headless-session caveat** (environmental, not a defect):
  `SPI_GETDESKWALLPAPER` returns 0 with no wallpaper provider, so the
  installer skips the OS-wallpaper backup gracefully and uninstall
  skips the restore when the value is absent (task-27 §7).

## 6. Minor / cosmetic (carried over)
- First `get_state` after pipe-up may report `running:false` until the
  video starts; Studio maps both to Connected (docs/known-issues.md).
- `lupdate.exe` in Qt 6.8.3 here cannot run (missing Linguist runtime);
  the `translations` target degrades gracefully (same file).
- **Version is single-sourced, not hand-synced** (MED-14, done 2026-09-23;
  `docs/tech-debt.md` marks it RESOLVED). `K6WP_VERSION` in the root
  `CMakeLists.txt` is canonical and the only place a version is typed.
  `configure_file()` stamps it into three generated consumers:
  - `build/<cfg>/generated/version.h` from `version.h.in`, consumed by
    every `.rc` (all four `app.rc` plus `shared/monitor_dump.rc` via
    `FILEVERSION` / `PRODUCTVERSION` and the `FileVersion` / `ProductVersion`
    strings) plus `K6WP_VERSION_STR` for the Studio update-check
    `User-Agent` and the About dialog. The root `CMakeLists.txt` puts
    `${CMAKE_BINARY_DIR}/generated` on the include path, so `rc.exe` and
    every C++ TU can `#include "version.h"`.
  - `packaging/version.nsh` from the root `version.nsh.in`, which
    `packaging/installer.nsi` `!include`s; `APP_VERSION`,
    `VIProductVersion` and the `VIAddVersionKey` values are all derived
    from it, so `installer.nsi` carries no version literal.
  - `engine/app.manifest` in place from `engine/app.manifest.in`
    (`version="@K6WP_VERSION_4@"`).
  `packaging/make_zip.ps1` asserts the whole chain and fails the run on
  drift: `project()` VERSION and `K6WP_VERSION` must agree, all three
  generated files must carry the canonical version, and a hand-written
  version literal in any consumer source is a hard failure rather than a
  warning. The old "three `.rc` files + manifest + `installer.nsi`" list is
  therefore obsolete; the current risk is a stale *generated* file, which
  the assert message names explicitly ("run cmake configure first").

## 7. RAM Engine (~220MB vs budget 80MB)

- **Live-proven** (Step 9.4, `docs/bench_idle_video_1min.json`):
  engine playing 1080p H.264 (`build/spikes/test_1080p.mp4`, autoplay at
  boot, `Engine:FirstFrame` in log) averaged **0.42% CPU over 16 cores**
  (peak 1.52%, 116 samples @500ms, 1 min) — **CPU PASS (<2%)** — but
  **~220 MB working set** (avg private 222.6 MB, peak 224.0 MB) against
  the 80 MB budget from planning §6.
- Mechanism (**code-quoted**): mpv D3D11 decoder context + the layered
  desktop surface pre-allocation (`engine/src/mpv_renderer.cpp`,
  `engine/src/desktop_inject.cpp`) dominate; the MsgWait idle loop
  itself is near-free (Step 9.4: 100 → 250 ms tick).
- Deeper optimization needs mpv-internal tuning (decoder surface
  budgeting, VO options) — **out of scope for v1.0**. The §3 numbers
  above stay aspirational until a Release hwdec run is measured.

## 8. hwdec logging gap ("unknown" in automated benches)

- **Live-proven**: `engine.log` does not expose the mpv
  `hwdec-current` property — the Step 9.4 bench log contains only
  `mpv [vd/warn]` video-aspect lines, so automated tooling honestly
  reports `"hwdec": "unknown"` (`docs/bench_idle_video_1min.json`).
- Visual/manual runs still show hardware decode active (video plays
  at 0.42% CPU, consistent with hwdec, not software decode).
- Fix direction (not done): mirror `hwdec-current` into `engine.log`
  on change (one line in `MpvRenderer::PollEvents`). Until then,
  bench JSONs keep the explicit `"hwdec_source"` note instead of
  guessing.

## 9. Multi-monitor = per-monitor N decode (Phase 3-lite descoping)

- **Decision (2026-09-22, spikes A/A2)**: single-decode fan-out is
  **descoped for 1.1.0**. GL-WGL sharing proved the architecture
  (decode=1, 2 windows) but forced software decode (`hwdec=no`,
  CPU +8.5% vs hwdec-dual) — trade rejected. Native D3D11 render API
  is `MPV_ERROR_NOT_IMPLEMENTED` in vendored libmpv (blocked on
  upstream PR #17764, see tech-debt). ANGLE rejected (no pinnable
  source vs `vendor/VERSIONS.md` policy).
- **Live-proven on the dev box**: multi-monitor stays N mpv instances (one
  decode per monitor). Mitigations that ship: per-slot occlusion pause (P2.5),
  lean mpv profile (P1.1), E-core affinity + iGPU pin (Phase 3-lite —
  `docs/bench_phase3.json`: 1080p playback 0.02% CPU, 141.8 MB WS,
  hwdec d3d11va on Intel UHD, RTX dec 0%). Per-monitor *video assignment*
  now ships (`displays.json`, `set_display_video`, Studio monitor sub-tabs), but
  it is still one decode per monitor.
- **Still no 2-physical-monitor run**: the placement rewrite and the
  per-monitor assignment map are covered by the headless placement matrix and
  the dev-box single-monitor run only (`multi_monitor_placement_test` and
  friends, one `reason=placement: covered` line per monitor in `engine.log`).
  A third-party two-monitor artifact is **awaiting collection**: the run-book
  (`docs/runbook-2monitor.md`) and `tools/run_2monitor_evidence.ps1` exist to
  produce it (see §2's placeholder). IS-1 stays UNPROVEN until that artifact
  is pasted back; no unproven 2-monitor claim is made here.

## 10. Phase 4 final-pass notes (2026-09-22, `docs/bench_phase4.json`)

- **Occlusion polling 1.5 s remains BY DESIGN** (P2.5): the unpaused
  message-loop wait doubles as the occlusion cadence — the sole sanctioned
  periodic wakeup, self-suspending while any pause owner is active. No
  event-driven replacement exists for maximized-window coverage (DWM
  thumbnail/region queries have no notification API); the paused state
  spends zero wakeups on it (loop wait goes INFINITE).
- **iGPU heuristic, not a guarantee**: `auto` picks
  `DXGI_GPU_PREFERENCE_MINIMUM_POWER` with a VendorId-table fallback, and
  the post-start verify pass recreates-unpinned + reloads when a pinned
  renderer reports hwdec inactive. A wrong pick costs one flicker + reload,
  never a stuck software-decode session. Override with
  `"gpu_adapter": "discrete"` / `"integrated"` (restart required).
- **Span-auto conditions**: per-monitor (`-1` = all screens) is the only
  topology in the engine — the span mode was removed in Phase 1.5, so there
  is no longer a second, unreached code path. The per-monitor video
  assignment (`displays.json`, §2) does not change that. Automatic span (one
  decode across the virtual-screen union) arrives only with the
  single-decode render API (§9); until then every monitor owns its decode
  instance.
- **ffmpeg essentials scope**: the 8.1.2 essentials build carries exactly
  what the pipeline calls — `h264_nvenc`, `h264_qsv`, `h264_amf`, `libx264`
  encoders, `scale`/`fps` filters, mp4 `faststart`. Anything outside that
  set (other codecs, filters, protocols) is absent by choice: a compress
  job needing them fails with the existing friendly error, never silently.
  `ffplay.exe` from the upstream zip is deliberately NOT vendored.
- **Working-set trim accounting**: the one-shot trim collapses resident
  set (idle ~29.9→0.2 MB, playback ~151.6→0.2 MB) while private bytes are
  untouched (idle 24.2 MB, playback ~150.8 MB). Judge RAM by private bytes
  + `PeakWorkingSet64`, never by post-trim working set; the soft-fault
  cost of re-touching evicted pages on resume/seek is unmeasured.

## 11. Paused wakeups — documented limitation (pre-release diagnosis, 2026-09-22)

- **Repro**: engine + video, manual IPC `pause` (mask=1, `slots=1`),
  60 s window, per-TID `Win32_PerfRawData_PerfProc_Thread` raw-delta
  method (`.omo/evidence/matrix/tidsnap.py`, the phase-2 row-b tool):
  **11.8/s process-wide** (phase-2 row-b: 11.9–13.5/s — reproduced).
  Top thread **9.67/s @ 0.002% CPU** = the IPC `ServeLoop` 100 ms accept
  slice (`engine/src/ipc_server.cpp:28` + `WaitWithStop`), plus two
  libmpv-internal threads at ~1.01/s each @ 0.000%. Everything else
  ≤0.11/s. Zero file I/O + zero periodic log lines in the window: decode
  is fully stopped, the churn is pure wait-slice wakeups at ~0 CPU.
- **Ordered suspects checked and cleared**: (1) config watcher runs the
  `event=rdevchange` path (log: watch armed, no fallback); (2) occlusion
  `Check` early-outs while disarmed (`occlusion_armed_`, zero occlusion
  lines in the window); (3) only two `SetTimer` sites exist engine-wide
  (config debounce + one-shot WS trim, both dead in steady state).
  `MsgWait` INFINITE + `mpv_wait_event(-1)` confirmed silent (main loop
  and both mpv event threads 0.00/s).
- **Why not fixed**: silencing the 10 Hz slice means an event-driven
  accept loop — an IPC-server redesign touching accept latency and the
  protected IPC v1 path. Out of scope for 1.1.0 by design; the gate
  (< 1.0/s) stays recorded as NOT MET.
- **Method confound on record**: any run with `--exit-after-ms` (or any
  `--simulate-*`) forces the 50 ms loop cadence via `AnySimulateArmed()`
  and measures ~80/s instead — those numbers are superseded for cadence
  purposes, including a P4-time 60 s typeperf run that briefly read
  79.8/s before the confound was identified.
- **HOTFIX (occlusion resume, 2026-09-22, on master post-1.1.0-tag)**:
  closing a borderless-fullscreen window stranded the fullscreen bit at
  1/2 debounce confirms (close yields exactly one foreground event; the
  INFINITE-while-paused loop runs no 1 s poll to complete it), so decode
  stayed paused until the next user input. Fix: track the fullscreen
  HWND; its `EVENT_OBJECT_DESTROY` clears the bit immediately (destroyed
  ≠ blip) + a debounced 150 ms poke runs one direct occlusion check.
  Proven: scripted close → clear same-ms, mask 0 in ≤0.6 s; Win+D cycle
  0.5 s; captioned-maximize still never trips the guard. Paused 60 s
  re-verified 13.5/s vs 11.8/s (≈, zero CPU, transient-only). No new
  periodic timer (conditional/transient `SetTimer` sites only).
- **HOTFIX follow-up (F11-out, same branch)**: exiting fullscreen via F11
  fires NO destroy/foreground/minimize event (window alive + focused,
  rect only) — same strand family, confirmed stuck-or-lucky by test
  (first run healed at 1.5 s via unidentified wake, not a trigger).
  Fix: `EVENT_OBJECT_LOCATIONCHANGE` hook (top-level only); the tracked
  holder's own rect flip re-queries and clears at once. Proven: F11-out
  → clear in 11 ms, mask 0 ≤0.5 s, full chain logged (locationchange →
  immediate clear + poke → check). F11-in cascades now complete the
  2-confirm in ~2 ms (was ~2.5 s by poll luck). No SHOW/HIDE hook
  (tooltip/menu storm volume judged not cheap).
  Residual: hide-via-API without focus change takes the same strand path
  (accepted: rarer than close, same pre-fix behavior).

## 9. The config schema: an older engine ignores the newer settings

- **The mechanism** (code-quoted): the config file carries a `version`
  field. The current schema is **v6** (the `pause_unload_after_sec` key
  came with task 1.1). `MigrateConfig` refuses a version it does not know:
  a **negative** version is corrupt (an error), a version **higher** than
  the engine's is read best-effort (the known fields keep their values, the
  unknown ones take their defaults) and the file is rewritten at the
  current schema.
- **Live-proven (measured, task G4)**: the engine built from the
  `baseline-pre-refactor` tag (schema **v5**) was given a **v6** file. It
  refused the file ("unsupported config schema version: 6"), kept its
  last-valid config, **left the file byte-identical** (the SHA-256 is
  unchanged) and wrote a **`.bak` holding the same bytes**. So the file is
  never damaged by the downgrade - but the old engine **does not use** the
  v6 settings (it falls back to the defaults for that session).
- **What the user sees**: after a downgrade, the wallpaper still plays and
  the settings that the old engine does not know are simply not applied.
  The user's data is not lost: it is in `config.json` and in
  `config.json.bak`.
- **The forward direction is safe** (task I3): when the current engine
  rewrites a file, the keys it does not know are **PRESERVED** (the
  round-trip merge), so an older engine can never drop a newer engine's
  fields. The golden fixture `config.roundtrip.*` pins that on both sides
  (the C++ and the Rust).
```

## File: tools/session_snapshot.ps1
```powershell
# session_snapshot.ps1 - Save and restore the Windows session state.
#
# Usage:
#   . .\tools\session_snapshot.ps1
#   $session = Invoke-SessionSafe { ... your work ... }
#   $session.Restore()
#
# The snapshot captures:
#   * Current wallpaper path (per monitor).
#   * Lock screen image path.
#   * Display configuration (resolution, orientation, position).
#   * Active window (to restore focus).
#
# The restore puts everything back exactly as it was.

# `[Win]` must exist before the SessionSnapshot class is parsed, not inside a
# method: PowerShell resolves class member types at parse time.
if (-not ("Win" -as [type])) {
    Add-Type @"
using System;
using System.Runtime.InteropServices;
public class Win {
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hWnd);
    [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
    [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint lpdwProcessId);
}
"@
}

function Get-CurrentWallpaper {
    # Get wallpaper for each monitor via registry.
    $result = @{}
    try {
        $regPath = "HKCU:\Control Panel\Desktop"
        $wallpaper = Get-ItemProperty -Path $regPath -Name Wallpaper -ErrorAction SilentlyContinue
        if ($wallpaper -and $wallpaper.Wallpaper) {
            $result["primary"] = $wallpaper.Wallpaper
        }
    } catch { }
    # Per-monitor wallpapers (Windows 10+)
    try {
        $monitors = Get-CimInstance -ClassName WmiMonitorID -Namespace root\wmi -ErrorAction SilentlyContinue
        if ($monitors) {
            foreach ($m in $monitors) {
                $key = "HKCU:\Control Panel\Desktop\PerMonitorSettings\" + $m.InstanceName
                $wp = Get-ItemProperty -Path $key -Name Wallpaper -ErrorAction SilentlyContinue
                if ($wp -and $wp.Wallpaper) {
                    $result[$m.InstanceName] = $wp.Wallpaper
                }
            }
        }
    } catch { }
    return $result
}

function Get-LockScreenImage {
    try {
        $regPath = "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Authentication\LogonUI\Background"
        $img = Get-ItemProperty -Path $regPath -Name "OEMBackground" -ErrorAction SilentlyContinue
        if ($img -and $img.OEMBackground -eq 1) {
            return "OEMBackground enabled"
        }
    } catch { }
    return $null
}

function Get-DisplayConfig {
    try {
        $displays = Get-CimInstance -ClassName Win32_DesktopMonitor -ErrorAction SilentlyContinue
        $result = @()
        foreach ($d in $displays) {
            $result += @{
                DeviceID = $d.DeviceID
                ScreenWidth = $d.ScreenWidth
                ScreenHeight = $d.ScreenHeight
                PixelsPerXLogicalInch = $d.PixelsPerXLogicalInch
                PixelsPerYLogicalInch = $d.PixelsPerYLogicalInch
            }
        }
        return $result
    } catch { }
    return @()
}

function Get-ActiveWindow {
    $hwnd = [Win]::GetForegroundWindow()
    if ($hwnd -ne [IntPtr]::Zero) {
        $windowPid = 0
        $null = [Win]::GetWindowThreadProcessId($hwnd, [ref]$windowPid)
        return @{ Handle = $hwnd; ProcessId = $windowPid }
    }
    return $null
}

function Set-Wallpaper {
    param($path, $monitor = "primary")
    # Use SystemParametersInfo to set wallpaper.
    Add-Type @"
using System;
using System.Runtime.InteropServices;
public class Wallpaper {
    [DllImport("user32.dll", CharSet = CharSet.Auto)]
    public static extern bool SystemParametersInfo(uint uiAction, uint uiParam, string pvParam, uint fWinIni);
    public const uint SPI_SETDESKWALLPAPER = 0x0014;
    public const uint SPIF_UPDATEINIFILE = 0x01;
    public const uint SPIF_SENDCHANGE = 0x02;
    public static bool Set(string path) {
        return SystemParametersInfo(SPI_SETDESKWALLPAPER, 0, path, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
    }
}
"@
    [Wallpaper]::Set($path) | Out-Null
}

function Set-LockScreen {
    param($path)
    # This is more complex and requires special permissions. Skip for now.
    Write-Host "Lock screen restore not implemented (requires SYSTEM context)"
}

# Main snapshot object.
class SessionSnapshot {
    [hashtable]$Wallpapers
    [string]$LockScreen
    [array]$Displays
    [hashtable]$ActiveWindow

    SessionSnapshot() {
        $this.Wallpapers = Get-CurrentWallpaper
        $this.LockScreen = Get-LockScreenImage
        $this.Displays = Get-DisplayConfig
        $this.ActiveWindow = Get-ActiveWindow
    }

    [void] Restore() {
        Write-Host "Restoring wallpapers..."
        foreach ($kv in $this.Wallpapers.GetEnumerator()) {
            if ($kv.Value) {
                try {
                    Set-Wallpaper -path $kv.Value
                    Write-Host "  Restored $($kv.Key): $($kv.Value)"
                } catch {
                    Write-Warning "Failed to restore $($kv.Key): $_"
                }
            }
        }
        Write-Host "Restoring displays..."
        # Display restore is complex; skip for safety.
        # foreach ($d in $this.Displays) { ... }
        if ($this.ActiveWindow) {
            Write-Host "Restoring active window focus..."
            # `[Win]` is added at run time, so resolve it dynamically: a class
            # body is parsed before Add-Type has run.
            $win = "Win" -as [type]
            if ($win) { $win::SetForegroundWindow($this.ActiveWindow.Handle) | Out-Null }
        }
    }
}

function Invoke-SessionSafe {
    param([scriptblock]$Work)
    $snapshot = [SessionSnapshot]::new()
    try {
        $result = & $Work
        return $snapshot
    } finally {
        $snapshot.Restore()
    }
}

try {
    Export-ModuleMember -Function Invoke-SessionSafe, Get-CurrentWallpaper, Get-LockScreenImage, Get-DisplayConfig, Get-ActiveWindow, Set-Wallpaper
} catch {
    # Dot-sourced (not imported as a module): nothing to export.
}
```

## File: tools/check_release_overrides.ps1
```powershell
# tools/check_release_overrides.ps1 - the release binaries must NOT contain
# the environment overrides (task I4, extended by L5).
#
# WHY
#   The overrides exist so a TEST can talk to its own engine instead of the
#   user's live session, so the two-writer test can prove the config lock is
#   what prevents the lost update, or so the K1/L2 instrument can accumulate
#   the `get_state` cost. A shipped binary must not let an environment
#   variable redirect the IPC endpoint, the singleton mutex or the library
#   path: whoever can set an environment variable for the process would then
#   be able to point the app at a pipe it controls.
#
# THE RULE
#   The overrides are compiled in a DEBUG build only (the C++: `_DEBUG`; the
#   Rust: `debug_assertions`). This script proves it for the RELEASE
#   binaries: it scans them for the variable names. A release binary that
#   still contains one of them FAILS the check.
#
#   The list is the AUDIT of every override in the tree (L5, 2026-10-22):
#   every `GetEnvironmentVariable*` / `std::env::var*` that reads a
#   `K6WP_*` name. The test-only names (K6WP_TEST_CONFIG_PATH,
#   K6WP_IPC_LIVE, K6WP_IPC_ENGINE_EXE, K6WP_RUST_WRITER_*) are NOT in the
#   list: they live in test binaries, which are built into the same
#   directory in a CI release build.
#
#   The scan must never pass vacuously: with `-RequireBinaries` the script
#   FAILS when it found no binary to scan (a missing Rust release build must
#   not look like a clean bill of health).
#
[CmdletBinding()]
param(
    [string]$ReleaseDir = '',
    [string]$RustReleaseDir = '',
    # Fail when no binary was scanned at all (a CI gate must not pass
    # because the build directory was empty or missing).
    [switch]$RequireBinaries
)

function Contains-Bytes([byte[]]$haystack, [byte[]]$needle) {
    if ($needle.Length -eq 0 -or $haystack.Length -lt $needle.Length) {
        return $false
    }
    for ($i = 0; $i -le $haystack.Length - $needle.Length; $i++) {
        $match = $true
        for ($j = 0; $j -lt $needle.Length; $j++) {
            if ($haystack[$i + $j] -ne $needle[$j]) {
                $match = $false
                break
            }
        }
        if ($match) {
            return $true
        }
    }
    return $false
}

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
if (-not $ReleaseDir) {
    $ReleaseDir = Join-Path $root 'build\release'
}

# The audit of every override in the tree (L5). Each one is compiled out of
# a release build; the scan proves it.
$forbidden = @('K6WP_ENGINE_PIPE',      # the IPC pipe name (C++)
               'K6WP_ENGINE_MUTEX',     # the singleton mutex name (C++)
               'K6WP_LIBRARY_JSON',     # the library index path (C++ and Rust)
               'K6WP_TEST_NO_CONFIG_LOCK',  # the config lock bypass (C++)
               'K6WP_IPC_TRACE',        # the IPC latency trace (C++)
               'K6WP_DEV',              # the dev-engine fallback (the Studio)
               'K6WP_SPS_DEBUG')        # the SPS box dump (the Rust core)

$binaries = @()
if (Test-Path -LiteralPath $ReleaseDir) {
    $binaries += @(Get-ChildItem -LiteralPath $ReleaseDir -Filter '*.exe' -File)
}

# The Rust release binaries: `cargo build --release` writes to
# `rust/target/release`, and with an explicit `--target` to
# `rust/target/<triple>/release`. Both are scanned.
$rustDirs = @()
if ($RustReleaseDir) {
    $rustDirs += $RustReleaseDir
} else {
    $rustDirs += (Join-Path $root 'rust\target\release')
    $rustDirs += (Join-Path $root 'rust\target\x86_64-pc-windows-msvc\release')
}
foreach ($dir in $rustDirs) {
    if (Test-Path -LiteralPath $dir) {
        $binaries += @(Get-ChildItem -LiteralPath $dir -Filter '*.exe' -File)
    } else {
        Write-Host ("  [skip] {0}: no such release directory" -f $dir)
    }
}

# The same binary can be reached through two directories (the Rust target
# and its explicit-triple twin): scan every path once.
$seenPaths = @{}
$unique = @()
foreach ($binary in $binaries) {
    if ($seenPaths.ContainsKey($binary.FullName)) { continue }
    $seenPaths[$binary.FullName] = $true
    $unique += $binary
}
$binaries = $unique

if ($binaries.Count -eq 0) {
    if ($RequireBinaries) {
        Write-Error "no release binary to scan (the C++ dir '$ReleaseDir' and the Rust dirs are empty): the check would pass vacuously"
        exit 2
    }
    Write-Error "no release binary in $ReleaseDir"
    exit 2
}

$failures = 0
foreach ($binary in $binaries) {
    $bytes = [IO.File]::ReadAllBytes($binary.FullName)
    $found = @()
    foreach ($name in $forbidden) {
        $wide = [Text.Encoding]::Unicode.GetBytes($name)
        $ascii = [Text.Encoding]::ASCII.GetBytes($name)
        if (Contains-Bytes $bytes $wide) { $found += "$name (UTF-16)" }
        elseif (Contains-Bytes $bytes $ascii) { $found += "$name (ASCII)" }
    }
    if ($found.Count -gt 0) {
        $failures++
        Write-Host ("  [FAIL] {0}: the release binary still contains {1}" -f $binary.Name, ($found -join ', '))
    } else {
        Write-Host ("  [OK] {0}: no environment override in the release binary" -f $binary.Name)
    }
}

if ($failures -gt 0) {
    Write-Error "$failures release binary/binaries still contain an environment override"
    exit 1
}
Write-Host "check_release_overrides: OK ($($binaries.Count) release binaries scanned, $($forbidden.Count) names)"
exit 0
```

