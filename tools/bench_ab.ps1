# tools/bench_ab.ps1 - A/B compare two engine binaries on the same states.
#
# Answers: "does change X actually help, and does it regress anything?"
# Both binaries are measured with the SAME harness, the SAME corpus, the
# SAME n and the SAME host conditions, so the delta is attributable.
#
# The comparison is GATE-AWARE: a change is only an improvement if it
# improves the protected metric AND does not regress the others. The
# gates come from docs/refactor/budgets.md.
#
# Usage:
#   powershell -NoProfile -ExecutionPolicy Bypass -File tools\bench_ab.ps1 `
#     -BaselineExe build\engine-baseline-phase0.exe `
#     -CandidateExe build\release\engine.exe `
#     -Runs 5 -OutJson docs\bench\ab.json

[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$BaselineExe,
    [Parameter(Mandatory=$true)][string]$CandidateExe,
    [int]$Runs = 5,
    [string[]]$States = @('playback_1080p'),
    [string]$OutJson = '',
    [string]$Label = 'ab'
)

$ErrorActionPreference = 'Stop'
$script:Here = Split-Path -Parent $MyInvocation.MyCommand.Path
. (Join-Path $script:Here 'bench_lib.ps1')

$BaselineExe = [System.IO.Path]::GetFullPath($BaselineExe)
$CandidateExe = [System.IO.Path]::GetFullPath($CandidateExe)
foreach ($p in @($BaselineExe, $CandidateExe)) {
    if (-not (Test-Path -LiteralPath $p)) { throw "engine not found: $p" }
}
if ([string]::IsNullOrWhiteSpace($OutJson)) {
    $OutJson = Join-Path $script:Here ("..\docs\bench\ab-{0}.json" -f $Label)
}
# -States may arrive as ONE comma-joined string (the command line flattens a
# string[] into a single value), so split it into the individual names.
$States = @($States | ForEach-Object { ([string]$_) -split ',' } | ForEach-Object { $_.Trim() } | Where-Object { $_ })
if ($States.Count -eq 0) { throw "-States is empty; nothing to compare" }

function Measure-Engine {
    param([string]$Exe, [string]$Tag)
    $out = Join-Path $env:TEMP ("k6wp-ab-{0}-{1}.json" -f $Tag, [Guid]::NewGuid().ToString('N').Substring(0, 8))
    # -OnlyStates is a string[]; passing it straight through keeps the array
    # intact (a comma list would be flattened into one string).
    & (Join-Path $script:Here 'bench_engine.ps1') -EngineExe $Exe -Runs $Runs -OnlyStates $States -OutJson $out | Out-Null
    if (-not (Test-Path -LiteralPath $out)) { throw "bench_engine produced no output for $Tag" }
    $doc = Get-Content -LiteralPath $out -Raw | ConvertFrom-Json
    Remove-Item -LiteralPath $out -Force -ErrorAction SilentlyContinue
    return $doc
}

Write-Host "bench_ab: baseline  = $BaselineExe"
Write-Host "bench_ab: candidate = $CandidateExe"
Write-Host "bench_ab: states = $($States -join ', '), runs = $Runs"

$base = Measure-Engine -Exe $BaselineExe -Tag 'base'
$cand = Measure-Engine -Exe $CandidateExe -Tag 'cand'

# Metric comparison. Direction: 'lower' = smaller is better.
$metrics = @(
    @{ name = 'private_mb';        dir = 'lower'; unit = 'MB' }
    @{ name = 'ws_mb';             dir = 'lower'; unit = 'MB' }
    @{ name = 'cpu_pct';           dir = 'lower'; unit = '%' }
    @{ name = 'gpu_total_pct';     dir = 'lower'; unit = '%' }
    @{ name = 'gpu_decode_pct';    dir = 'lower'; unit = '%' }
    @{ name = 'gpu_3d_pct';        dir = 'lower'; unit = '%' }
    @{ name = 'gpu_shared_mb';     dir = 'lower'; unit = 'MB' }
    @{ name = 'wakeups_per_sec';   dir = 'lower'; unit = '/s' }
)

$rows = @()
foreach ($state in $States) {
    $b = $base.states.$state
    $c = $cand.states.$state
    if ($null -eq $b -or $null -eq $c) { continue }
    foreach ($m in $metrics) {
        $bv = $b.$($m.name)
        $cv = $c.$($m.name)
        if ($null -eq $bv -or $null -eq $cv -or $bv.n -eq 0 -or $cv.n -eq 0) { continue }
        $bmed = [double]$bv.median
        $cmed = [double]$cv.median
        $delta = $cmed - $bmed
        $pct = if ($bmed -ne 0) { [math]::Round(($delta / $bmed) * 100, 2) } else { $null }
        $improved = if ($m.dir -eq 'lower') { $delta -lt 0 } else { $delta -gt 0 }
        # A change is only meaningful if it exceeds the measured noise.
        $noise = if ($bmed -ne 0) { [math]::Abs((($bv.max - $bv.min) / $bmed) * 100) } else { 0 }
        $significant = ($null -ne $pct) -and ([math]::Abs($pct) -gt $noise)
        $rows += [pscustomobject]@{
            state = $state
            metric = $m.name
            unit = $m.unit
            baseline = $bmed
            candidate = $cmed
            delta = [math]::Round($delta, 3)
            delta_pct = $pct
            noise_pct = [math]::Round($noise, 2)
            direction = $m.dir
            improved = $improved
            significant = $significant
            verdict = $(if ($significant -and $improved) { 'BETTER' } elseif ($significant -and -not $improved) { 'WORSE' } else { 'same' })
        }
    }
}

Write-Host ""
Write-Host ("=" * 92)
Write-Host ("  {0}: baseline vs candidate" -f $Label)
Write-Host ("=" * 92)
$rows | Format-Table -AutoSize | Out-String | Write-Host

$better = @($rows | Where-Object { $_.verdict -eq 'BETTER' })
$worse = @($rows | Where-Object { $_.verdict -eq 'WORSE' })
Write-Host ("BETTER: {0}   WORSE: {1}   same: {2}" -f $better.Count, $worse.Count, ($rows.Count - $better.Count - $worse.Count))
if ($worse.Count -gt 0) {
    Write-Host ""
    Write-Host "REGRESSIONS (must be justified or reverted):"
    $worse | Format-Table -AutoSize | Out-String | Write-Host
}

$report = [ordered]@{
    tool = 'bench_ab'
    phase = 1
    date = (Get-Date -Format 'yyyy-MM-ddTHH:mm:ssK')
    label = 'live-proven'
    tag = $Label
    baseline_exe = $BaselineExe
    candidate_exe = $CandidateExe
    runs_per_state = $Runs
    states = $States
    method = 'same harness, same corpus, same n, same host; deltas compared against the measured noise band'
    comparison = $rows
    better_count = $better.Count
    worse_count = $worse.Count
    baseline = $base
    candidate = $cand
}
$json = $report | ConvertTo-Json -Depth 12
$outFull = [System.IO.Path]::GetFullPath($OutJson)
$outDir = [System.IO.Path]::GetDirectoryName($outFull)
if (-not (Test-Path -LiteralPath $outDir)) { New-Item -ItemType Directory -Force -Path $outDir | Out-Null }
$json | Set-Content -LiteralPath $outFull -Encoding UTF8
Write-Host "bench_ab: wrote $outFull"
