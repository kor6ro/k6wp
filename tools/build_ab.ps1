# tools/build_ab.ps1 - build a baseline/candidate pair of engine binaries.
#
# WHY THIS EXISTS
#   Copy-Item preserves the SOURCE file's LastWriteTime. Restoring a source
#   file from a .bak therefore leaves it OLDER than its object file, and
#   MSBuild silently skips the recompile: the "new" build is the OLD binary.
#   That produced two identical engine binaries and a VOID A/B comparison
#   (the frame parity check "passed" with pixel diff 0.000 because it
#   compared a build with itself).
#
#   This script always TOUCHES the sources after editing them and VERIFIES
#   that the two binaries really differ, so a void comparison can never
#   happen again.
#
# Usage:
#   powershell -NoProfile -ExecutionPolicy Bypass -File tools\build_ab.ps1 `
#     -BaselineVideoSync display-desync -CandidateVideoSync display-vsync
#
# Output: build\ab\engine-baseline.exe and build\ab\engine-candidate.exe
# (plus libmpv-2.dll, which the engine loads from its own directory).

[CmdletBinding()]
param(
    [ValidateSet('display-desync', 'display-vsync')][string]$BaselineVideoSync = 'display-desync',
    [ValidateSet('display-desync', 'display-vsync')][string]$CandidateVideoSync = 'display-vsync',
    [string]$Source = '',
    [string]$OutDir = ''
)
$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
$repo = Split-Path -Parent $here
if ([string]::IsNullOrWhiteSpace($Source)) { $Source = Join-Path $repo 'engine\src\mpv_renderer.cpp' }
if ([string]::IsNullOrWhiteSpace($OutDir)) { $OutDir = Join-Path $repo 'build\ab' }
$OutDir = [System.IO.Path]::GetFullPath($OutDir)
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

$original = Get-Content -LiteralPath $Source -Raw

function Build-Variant {
    param([string]$VideoSync, [string]$Target)
    $patched = $original -replace 'set_opt\("video-sync", "display-(desync|vsync)"\);', "set_opt(`"video-sync`", `"$VideoSync`");"
    if ($patched -eq $original -and $original -notmatch [regex]::Escape("`"$VideoSync`")")) {
        throw "could not patch the video-sync option to '$VideoSync' in $Source"
    }
    Set-Content -LiteralPath $Source -Value $patched -NoNewline
    # TOUCH: Copy-Item/Set-Content can leave an mtime older than the object
    # file, and MSBuild then skips the recompile.
    (Get-Item -LiteralPath $Source).LastWriteTime = Get-Date
    $out = & cmake --build --preset release --target engine 2>&1
    if ($LASTEXITCODE -ne 0) {
        $out | Select-Object -Last 20 | Write-Host
        throw "the release build failed"
    }
    $exe = Join-Path $repo 'build\release\engine.exe'
    Copy-Item -LiteralPath $exe -Destination $Target -Force
    (Get-Item -LiteralPath $Target).LastWriteTime = Get-Date
    Write-Host ("  built {0} ({1})" -f (Split-Path -Leaf $Target), $VideoSync)
}

try {
    Write-Host "build_ab: baseline  = $BaselineVideoSync"
    Build-Variant -VideoSync $BaselineVideoSync -Target (Join-Path $OutDir 'engine-baseline.exe')
    Write-Host "build_ab: candidate = $CandidateVideoSync"
    Build-Variant -VideoSync $CandidateVideoSync -Target (Join-Path $OutDir 'engine-candidate.exe')
}
finally {
    Set-Content -LiteralPath $Source -Value $original -NoNewline
    (Get-Item -LiteralPath $Source).LastWriteTime = Get-Date
}

# The engine loads libmpv-2.dll from its own directory.
Copy-Item -LiteralPath (Join-Path $repo 'build\release\libmpv-2.dll') -Destination (Join-Path $OutDir 'libmpv-2.dll') -Force

# VERIFY: the two binaries MUST differ, otherwise the A/B would compare a
# build with itself.
$h1 = (Get-FileHash (Join-Path $OutDir 'engine-baseline.exe') -Algorithm SHA256).Hash
$h2 = (Get-FileHash (Join-Path $OutDir 'engine-candidate.exe') -Algorithm SHA256).Hash
if ($h1 -eq $h2) {
    throw "the baseline and candidate binaries are IDENTICAL - the A/B would be void (rebuild failed to recompile?)"
}
Write-Host ("build_ab: OK, the two binaries differ ({0} vs {1})" -f $h1.Substring(0, 12), $h2.Substring(0, 12))
