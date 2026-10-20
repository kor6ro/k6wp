# tools/diag_wakeups3.ps1 - attribute the wakeups to engine threads.
#
# The Phase 0 bench measures "\Thread(engine*)\Context Switches/sec" (an
# aggregate over the engine's threads) and reads ~1273/s during 1080p
# playback. This tool breaks that down per thread instance and maps each
# instance onto the live OS thread (id, state, wait reason, CPU time), so
# the spinner can be NAMED instead of guessed.
[CmdletBinding()]
param(
    [string]$EngineExe = '',
    [string]$Video = ''
)
$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
. (Join-Path $here 'bench_lib.ps1')

if ([string]::IsNullOrWhiteSpace($EngineExe)) { $EngineExe = Join-Path $here '..\build\release\engine.exe' }
if ([string]::IsNullOrWhiteSpace($Video)) { $Video = Join-Path $here '..\tests\corpus\anime.mp4' }
$engine = [System.IO.Path]::GetFullPath($EngineExe)
$video = [System.IO.Path]::GetFullPath($Video)
$data = Join-Path $env:TEMP ("k6wp-wake3-" + [Guid]::NewGuid().ToString('N').Substring(0, 8))
New-Item -ItemType Directory -Force -Path (Join-Path $data 'K6WP') | Out-Null

$busy = @(Get-Process engine -ErrorAction SilentlyContinue)
if ($busy.Count -gt 0) { throw "engine already running (pid $($busy.Id -join ','))" }

$old = $env:LOCALAPPDATA
try {
    $env:LOCALAPPDATA = $data
    $p = Start-Process -FilePath $engine -ArgumentList @("--video=$video", '--exit-after-ms=90000') -PassThru -WindowStyle Hidden
}
finally { $env:LOCALAPPDATA = $old }
if (-not (Wait-K6wpEngine -TimeoutMs 15000)) {
    try { if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force } } catch { }
    throw "engine did not answer the pipe"
}
Start-Sleep -Seconds 6

# The occlusion watcher pauses playback when a window covers the desktop, so
# the engine MUST be playing for this measurement to mean anything. Every
# other Phase 0 sampler minimizes the top-level windows for the same reason.
$saved = Get-DesktopWindows
try {
    # Confirm the engine is actually PLAYING (a paused engine reads ~36/s).
    $st = Send-K6wpIpc -Cmd 'get_state' -TimeoutMs 3000
    Write-Host ("engine state: paused={0} video='{1}'" -f $st.state.paused, $st.state.video)
    if ($st.state.paused) { Write-Host "WARNING: engine is PAUSED - the wakeup numbers will be the paused floor, not the playback figure" }

    $paths = @((Get-Counter -ListSet 'Thread' -ErrorAction Stop).PathsWithInstances |
        Where-Object { $_ -like '\Thread(engine/*)\Context Switches/sec' })
    Write-Host ("engine thread instances: {0}" -f $paths.Count)

    # Per-OS-thread CPU delta over the sampling window identifies the
    # spinner by name. The Thread perf object instance number is NOT the OS
    # thread id, so CPU time is the reliable attribution signal.
    $p.Refresh()
    $cpuBefore = @{}
    foreach ($t in $p.Threads) { $cpuBefore[[string]$t.Id] = $t.TotalProcessorTime.TotalMilliseconds }

    $rows = @()
    for ($sample = 1; $sample -le 3; $sample++) {
        $samples = @()
        try { $samples = @((Get-Counter $paths -ErrorAction Stop).CounterSamples) }
        catch { $samples = @((Get-Counter $paths -ErrorAction SilentlyContinue).CounterSamples) }
        foreach ($s in $samples) {
            $tid = ($s.Path -replace '^.*\\Thread\(engine/', '') -replace '\)\\Context Switches/sec$', ''
            $rows += [pscustomobject]@{ sample = $sample; tid = $tid; cs = [math]::Round($s.CookedValue, 1) }
        }
        Start-Sleep -Milliseconds 700
    }

    $p.Refresh()
    $cpuAfter = @{}
    foreach ($t in $p.Threads) { $cpuAfter[[string]$t.Id] = $t.TotalProcessorTime.TotalMilliseconds }

    Write-Host ""
    Write-Host "--- context switches/sec per Thread-object instance (playback) ---"
    $rows | Group-Object tid | ForEach-Object {
        $vals = $_.Group | ForEach-Object { $_.cs }
        [pscustomobject]@{ inst = $_.Name; cs = (Get-Stats $vals).median; n = $vals.Count }
    } | Sort-Object cs -Descending | Select-Object -First 10 | Format-Table -AutoSize | Out-String | Write-Host
    $total = ($rows | Group-Object sample | ForEach-Object { [math]::Round((($_.Group | Measure-Object cs -Sum).Sum), 1) })
    Write-Host ("total per sample: " + ($total -join ', '))

    Write-Host ""
    Write-Host "--- engine OS threads by CPU delta over the sample window ---"
    $window = 2.1
    $deltas = @()
    foreach ($t in $p.Threads) {
        $k = [string]$t.Id
        if ($cpuBefore.ContainsKey($k)) {
            $d = $cpuAfter[$k] - $cpuBefore[$k]
            $deltas += [pscustomobject]@{
                tid = $t.Id
                cpu_ms = [math]::Round($d, 0)
                cpu_pct_of_1core = [math]::Round(($d / 1000.0 / $window) * 100, 1)
                state = [string]$t.ThreadState
                wait = [string]$t.WaitReason
            }
        }
    }
    $deltas | Sort-Object cpu_ms -Descending | Select-Object -First 10 | Format-Table -AutoSize | Out-String | Write-Host
}
finally {
    Restore-DesktopWindows -Saved $saved
    try { if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue } } catch { }
}
