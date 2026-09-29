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
