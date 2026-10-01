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
