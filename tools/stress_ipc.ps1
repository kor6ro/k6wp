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
