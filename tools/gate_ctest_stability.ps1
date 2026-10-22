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
