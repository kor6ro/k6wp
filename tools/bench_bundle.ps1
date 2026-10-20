# tools/bench_bundle.ps1 - Phase 0 bundle size inventory.
#
# Answers: "how big is what we ship, and what is the largest lever?"
# Measures the RELEASE build tree (build\release) plus the shipped ZIP(s) in
# dist\. Produces a per-file breakdown grouped by role so the Phase 1/2/3
# bundle-reduction targets can be quantified against a real baseline.
#
# No code instrumentation: this is a filesystem census of the artifacts that
# packaging/installer.nsi and packaging/make_zip.ps1 actually stage.
#
# Usage:
#   powershell -NoProfile -ExecutionPolicy Bypass -File tools\bench_bundle.ps1 `
#     -OutJson docs\bench\bundle.json

[CmdletBinding()]
param(
    [string]$BuildDir = '',
    [string]$VendorDir = '',
    [string]$DistDir = '',
    [string]$OutJson = ''
)

$ErrorActionPreference = 'Stop'
$script:Here = Split-Path -Parent $MyInvocation.MyCommand.Path
if ([string]::IsNullOrWhiteSpace($BuildDir)) { $BuildDir = Join-Path $script:Here '..\build\release' }
if ([string]::IsNullOrWhiteSpace($VendorDir)) { $VendorDir = Join-Path $script:Here '..\vendor' }
if ([string]::IsNullOrWhiteSpace($DistDir)) { $DistDir = Join-Path $script:Here '..\dist' }
if ([string]::IsNullOrWhiteSpace($OutJson)) { $OutJson = Join-Path $script:Here '..\docs\bench\bundle.json' }

$BuildDir = [System.IO.Path]::GetFullPath($BuildDir)
$VendorDir = [System.IO.Path]::GetFullPath($VendorDir)
$DistDir = [System.IO.Path]::GetFullPath($DistDir)
if (-not (Test-Path -LiteralPath $BuildDir)) { throw "build dir not found: $BuildDir" }

# The shipping set is the top level of the release tree plus the Qt/QML
# subdirectories the installer stages (see packaging/installer.nsi).
$stagedDirs = @('platforms', 'imageformats', 'styles', 'iconengines', 'qml', 'networkinformation', 'tls')

# packaging/installer.nsi stages libmpv-2.dll and the VC runtime from
# vendor\, NOT from the build tree, so the census must include them or the
# shipping size is understated by ~116 MB.
$vendorFiles = @(
    @{ path = (Join-Path $VendorDir 'ffmpeg\ffmpeg.exe');   group = 'ffmpeg'; rel = 'ffmpeg.exe' },
    @{ path = (Join-Path $VendorDir 'ffmpeg\ffprobe.exe');  group = 'ffmpeg'; rel = 'ffprobe.exe' },
    @{ path = (Join-Path $VendorDir 'libmpv\bin\libmpv-2.dll'); group = 'mpv'; rel = 'libmpv-2.dll' },
    @{ path = (Join-Path $VendorDir 'vc_runtime\vcruntime140.dll');   group = 'msvc-runtime'; rel = 'vcruntime140.dll' },
    @{ path = (Join-Path $VendorDir 'vc_runtime\vcruntime140_1.dll'); group = 'msvc-runtime'; rel = 'vcruntime140_1.dll' },
    @{ path = (Join-Path $VendorDir 'vc_runtime\msvcp140.dll');       group = 'msvc-runtime'; rel = 'msvcp140.dll' }
)

function Get-GroupName([string]$rel) {
    $top = ($rel -split '[\\/]')[0]
    switch ($true) {
        ($top -eq 'libmpv-2.dll') { return 'mpv' }
        ($top -like 'Qt6*') { return 'qt' }
        ($top -eq 'ffmpeg.exe' -or $top -eq 'ffprobe.exe') { return 'ffmpeg' }
        ($top -eq 'dxcompiler.dll' -or $top -eq 'dxil.dll') { return 'shader-cache' }
        ($top -in @('engine.exe', 'studio.exe', 'K6WP.exe', 'compressor.exe')) { return 'k6wp-exes' }
        ($top -like 'vcruntime*' -or $top -like 'msvcp*') { return 'msvc-runtime' }
        ($stagedDirs -contains $top) { return "qt-payload/$top" }
        ($top -eq 'monitor_dump.exe') { return 'qa-tools' }
        ($top -eq '') { return 'root' }
        default { return 'other' }
    }
}

$files = @()
$rootFiles = @(Get-ChildItem -LiteralPath $BuildDir -File -ErrorAction SilentlyContinue)
foreach ($f in $rootFiles) {
    # The build tree also carries a windeployqt copy of libmpv/ffmpeg-era DLLs;
    # the installer takes libmpv from vendor\, so the build copy is not staged.
    if ($f.Name -eq 'libmpv-2.dll') { continue }
    $files += [pscustomobject]@{
        rel = $f.Name
        bytes = $f.Length
        group = (Get-GroupName $f.Name)
        source = 'build'
    }
}
foreach ($d in $stagedDirs) {
    $dp = Join-Path $BuildDir $d
    if (-not (Test-Path -LiteralPath $dp)) { continue }
    foreach ($f in @(Get-ChildItem -LiteralPath $dp -Recurse -File -ErrorAction SilentlyContinue)) {
        $rel = $d + '/' + $f.FullName.Substring($dp.Length).TrimStart('\', '/')
        $files += [pscustomobject]@{
            rel = ($rel -replace '\\', '/')
            bytes = $f.Length
            group = (Get-GroupName $rel)
            source = 'build'
        }
    }
}
$missingVendor = @()
foreach ($v in $vendorFiles) {
    if (Test-Path -LiteralPath $v.path) {
        $fi = Get-Item -LiteralPath $v.path
        $files += [pscustomobject]@{
            rel = $v.rel
            bytes = $fi.Length
            group = $v.group
            source = 'vendor'
        }
    }
    else {
        $missingVendor += $v.rel
    }
}

$totalBytes = ($files | Measure-Object -Property bytes -Sum).Sum
$groups = @{}
foreach ($g in ($files | Group-Object group)) {
    $gb = ($g.Group | Measure-Object -Property bytes -Sum).Sum
    $groups[$g.Name] = [ordered]@{
        files = $g.Count
        bytes = $gb
        mb = [math]::Round($gb / 1MB, 2)
        pct = [math]::Round(($gb / $totalBytes) * 100, 1)
    }
}

$top = @($files | Sort-Object bytes -Descending | Select-Object -First 25 | ForEach-Object {
    [ordered]@{ rel = $_.rel; bytes = $_.bytes; mb = [math]::Round($_.bytes / 1MB, 2); group = $_.group; source = $_.source }
})

$zips = @()
if (Test-Path -LiteralPath $DistDir) {
    $zips = @(Get-ChildItem -LiteralPath $DistDir -Filter '*.zip' -File -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTime -Descending | ForEach-Object {
            [ordered]@{
                name = $_.Name
                bytes = $_.Length
                mb = [math]::Round($_.Length / 1MB, 2)
                modified = $_.LastWriteTime.ToString('yyyy-MM-dd')
            }
        })
}

$report = [ordered]@{
    tool = 'bench_bundle'
    phase = 0
    date = (Get-Date -Format 'yyyy-MM-ddTHH:mm:ssK')
    label = 'live-proven'
    method = 'filesystem census of the release build tree (the set packaging/installer.nsi stages) + shipped zips in dist/'
    build_dir = $BuildDir
    total_files = $files.Count
    total_bytes = $totalBytes
    total_mb = [math]::Round($totalBytes / 1MB, 2)
    groups = $groups
    largest_files = $top
    shipped_zips = $zips
    missing_vendor = $missingVendor
    notes = @(
        'mpv = libmpv-2.dll; qt = Qt6*.dll; qml/qt-payload = the QML module tree; ffmpeg = ffmpeg.exe+ffprobe.exe.',
        'The staging set mirrors packaging/installer.nsi: libmpv + VC runtime come from vendor\, the Qt payload from the release build tree.',
        'This is the pre-refactor baseline. Phase 1.3 (lean libmpv), Phase 2.7 (Qt removal) and Phase 3 target these groups.',
        'Sizes are on-disk bytes; the ZIP column is the compressed shipping size (dist/ may lag the current build).'
    )
}

$json = $report | ConvertTo-Json -Depth 8
$outFull = [System.IO.Path]::GetFullPath($OutJson)
$outDir = [System.IO.Path]::GetDirectoryName($outFull)
if (-not (Test-Path -LiteralPath $outDir)) { New-Item -ItemType Directory -Force -Path $outDir | Out-Null }
$json | Set-Content -LiteralPath $outFull -Encoding UTF8
Write-Host ("bench_bundle: {0} files, {1} MB -> {2}" -f $files.Count, [math]::Round($totalBytes / 1MB, 2), $outFull)
foreach ($k in ($groups.Keys | Sort-Object)) {
    Write-Host ("  {0,-22} {1,10} MB  ({2}%)" -f $k, $groups[$k].mb, $groups[$k].pct)
}
