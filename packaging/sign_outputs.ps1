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
