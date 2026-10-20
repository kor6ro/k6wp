# tools/run_phase0_bench.ps1 - run the whole Phase 0 measurement suite.
#
# One command that produces every Phase 0 evidence file:
#   docs/bench/phase0.json        engine RAM/CPU/GPU/wakeups, n>=5 per state
#   docs/bench/phase0.raw.json    the per-run detail
#   docs/bench/studio.json        Studio startup + idle RAM/CPU
#   docs/bench/bundle.json        install-size census
#   docs/bench/resume.json        suspend/resume latency
#   docs/bench/visual/baseline/   visual parity baseline (PNGs + signature)
#
# The production engine MUST be stopped first (singleton mutex): the harness
# measures the REAL engine, so a resident production engine would either
# block the run or pollute the numbers.
#
# Usage:
#   powershell -NoProfile -ExecutionPolicy Bypass -File tools\run_phase0_bench.ps1 `
#     [-Runs 5] [-SkipVisual]
#
# Exit code 0 = every sampler produced its file; non-zero = a sampler failed.

[CmdletBinding()]
param(
    [int]$Runs = 5,
    [string]$EngineExe = '',
    [string]$OutDir = '',
    [switch]$SkipVisual,
    [switch]$SkipResume
)

$ErrorActionPreference = 'Stop'
$script:Here = Split-Path -Parent $MyInvocation.MyCommand.Path
. (Join-Path $script:Here 'bench_lib.ps1')
if ([string]::IsNullOrWhiteSpace($EngineExe)) { $EngineExe = Join-Path $script:Here '..\build\release\engine.exe' }
if ([string]::IsNullOrWhiteSpace($OutDir)) { $OutDir = Join-Path $script:Here '..\docs\bench' }
$OutDir = [System.IO.Path]::GetFullPath($OutDir)
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

# --- preflight -------------------------------------------------------------
if (-not (Test-Path -LiteralPath $EngineExe)) { throw "engine not found: $EngineExe" }
$busy = @(Get-Process engine -ErrorAction SilentlyContinue)
if ($busy.Count -gt 0) {
    throw ("an engine.exe is already running (pid {0}). Stop it first: K6WP.exe --stop" -f (($busy | ForEach-Object { $_.Id }) -join ','))
}
$studioBusy = @(Get-Process studio -ErrorAction SilentlyContinue)
if ($studioBusy.Count -gt 0) {
    throw ("studio.exe is already running (pid {0}). Close it first." -f (($studioBusy | ForEach-Object { $_.Id }) -join ','))
}

$steps = @()
$failed = @()

function Invoke-Step {
    param([string]$Name, [scriptblock]$Action)
    Write-Host ""
    Write-Host ("=" * 72)
    Write-Host "  $Name"
    Write-Host ("=" * 72)
    $sw = [Diagnostics.Stopwatch]::StartNew()
    try {
        & $Action
        $script:steps += [pscustomobject]@{ step = $Name; status = 'ok'; seconds = [math]::Round($sw.Elapsed.TotalSeconds, 1) }
        Write-Host ("  -> {0} OK ({1}s)" -f $Name, [math]::Round($sw.Elapsed.TotalSeconds, 1))
    }
    catch {
        $script:steps += [pscustomobject]@{ step = $Name; status = 'FAILED'; seconds = [math]::Round($sw.Elapsed.TotalSeconds, 1); error = $_.Exception.Message }
        $script:failed += $Name
        Write-Host ("  -> {0} FAILED: {1}" -f $Name, $_.Exception.Message)
    }
}

Invoke-Step "engine (RAM/CPU/GPU/wakeups, n>=$Runs)" {
    & (Join-Path $script:Here 'bench_engine.ps1') -Runs $Runs -EngineExe $EngineExe -OutJson (Join-Path $OutDir 'phase0.json')
    if (-not (Test-Path -LiteralPath (Join-Path $OutDir 'phase0.json'))) { throw "bench_engine wrote no output" }
}

Invoke-Step 'studio (startup + idle RAM/CPU)' {
    & (Join-Path $script:Here 'bench_studio.ps1') -Runs 3 -OutJson (Join-Path $OutDir 'studio.json')
    if (-not (Test-Path -LiteralPath (Join-Path $OutDir 'studio.json'))) { throw "bench_studio wrote no output" }
}

Invoke-Step 'bundle (install-size census)' {
    & (Join-Path $script:Here 'bench_bundle.ps1') -OutJson (Join-Path $OutDir 'bundle.json')
    if (-not (Test-Path -LiteralPath (Join-Path $OutDir 'bundle.json'))) { throw "bench_bundle wrote no output" }
}

if (-not $SkipResume) {
    Invoke-Step 'resume (suspend/resume latency)' {
        & (Join-Path $script:Here 'bench_resume.ps1') -Runs 3 -OutJson (Join-Path $OutDir 'resume.json')
        if (-not (Test-Path -LiteralPath (Join-Path $OutDir 'resume.json'))) { throw "bench_resume wrote no output" }
    }
}

if (-not $SkipVisual) {
    Invoke-Step 'visual (parity baseline capture)' {
        & (Join-Path $script:Here 'capture_visual_baseline.ps1')
        $sig = Join-Path $OutDir 'visual\baseline\signature.json'
        if (-not (Test-Path -LiteralPath $sig)) { throw "visual capture wrote no signature" }
    }
}

Write-Host ""
Write-Host ("=" * 72)
Write-Host "  Phase 0 summary"
Write-Host ("=" * 72)
$steps | Format-Table -AutoSize | Out-String | Write-Host
if ($failed.Count -gt 0) {
    Write-Host ("FAILED: {0}" -f ($failed -join ', '))
    exit 1
}
Write-Host "All Phase 0 samplers produced their evidence files."
exit 0
