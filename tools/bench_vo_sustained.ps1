# tools/bench_vo_sustained.ps1 - prove the VO keeps PRESENTING frames.
#
# The screen-level animation check is not usable in an agent session: the
# agent's own always-on-top cursor overlay covers the whole screen, and the
# engine correctly pauses the wallpaper when the desktop is covered. So the
# animation is proven from the GPU side instead: if the VO had stalled
# (e.g. a vblank wait that never completes on a window without a
# redirection surface), the decode and the 3D work would stop after the
# first frame. Sustained per-second GPU work proves the VO keeps
# producing frames for the whole playback window.
[CmdletBinding()]
param(
    [string]$EngineExe = '',
    [int]$Seconds = 15
)
$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
. (Join-Path $here 'bench_lib.ps1')

if ([string]::IsNullOrWhiteSpace($EngineExe)) { $EngineExe = Join-Path $here '..\build\release\engine.exe' }
$engine = [System.IO.Path]::GetFullPath($EngineExe)
$video = Join-Path $here '..\tests\corpus\anime.mp4'
$dataDir = Join-Path $env:TEMP ("k6wp-vo-" + [Guid]::NewGuid().ToString('N').Substring(0, 8))
New-Item -ItemType Directory -Force -Path (Join-Path $dataDir 'K6WP') | Out-Null

$busy = @(Get-Process engine -ErrorAction SilentlyContinue)
if ($busy.Count -gt 0) { throw "engine already running (pid $($busy.Id -join ','))" }

$old = $env:LOCALAPPDATA
try {
    $env:LOCALAPPDATA = $dataDir
    # The engine must outlive the sampling window (it self-exits on
    # --exit-after-ms), otherwise the counters vanish mid-run and the
    # measurement silently reads "no work" instead of "the engine is gone".
    $p = Start-Process -FilePath $engine -ArgumentList @("--video=$video", "--exit-after-ms=$($Seconds * 1000 + 90000)") -PassThru -WindowStyle Hidden
}
finally { $env:LOCALAPPDATA = $old }
if (-not (Wait-K6wpEngine -TimeoutMs 15000)) { throw "engine did not answer the pipe" }
Start-Sleep -Seconds 3

# The engine pauses the decode when a window covers the desktop (occlusion
# watcher). Every covering window must be minimized BEFORE the measurement,
# or the engine is paused and the GPU work is zero by definition.
$saved = Get-DesktopWindows
$logPath = Join-Path $dataDir 'K6WP\engine.log'
$resumed = $false
$deadline = (Get-Date).AddSeconds(30)
$lastRetry = (Get-Date).AddSeconds(-10)
while ((Get-Date) -lt $deadline) {
    if (Test-Path -LiteralPath $logPath) {
        $lines = @(Get-Content -LiteralPath $logPath -ErrorAction SilentlyContinue)
        $paused = @($lines | Select-String 'occlusion: slot \d+ coverage=100').Count
        $res = @($lines | Select-String 'occlusion: slot \d+ coverage=0\.0% resuming').Count
        if ($res -gt 0 -and $res -ge $paused) { $resumed = $true; break }
    }
    # The engine clears its fullscreen bit on a FOREGROUND-CHANGE event, and
    # while that bit is set the message loop waits INFINITE (no periodic
    # re-check). Minimizing again produces the event that unblocks it.
    if (((Get-Date) - $lastRetry).TotalSeconds -ge 4) {
        $lastRetry = Get-Date
        [void](Get-DesktopWindows)
    }
    Start-Sleep -Milliseconds 300
}
if (-not $resumed) { throw "the engine never resumed playback (occlusion) - the GPU work would read zero by definition" }
Write-Host "bench_vo_sustained: engine resumed playback"
Start-Sleep -Seconds 2

$tp = Start-TypeperfSampler -ProcessId $p.Id -Seconds $Seconds -ProcessName 'engine'
try { [void]$tp.Process.WaitForExit(($Seconds + 10) * 1000) } catch { }
$rows = @(Read-TypeperfCsv -Path $tp.Csv)
try { if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue } } catch { }
Restore-DesktopWindows -Saved $saved

Write-Host ""
Write-Host "--- per-second GPU work (engine pid $($p.Id)) ---"
Write-Host ("{0,-8} {1,12} {2,12} {3,12}" -f 'second', 'gpu_total%', 'gpu_3d%', 'gpu_decode%')
$first = $true
$i = 0
foreach ($r in $rows) {
    $i++
    $tot = if ($null -ne $r.gpu_total_pct) { $r.gpu_total_pct } else { 'n/a' }
    $d3 = if ($null -ne $r.gpu_3d_pct) { $r.gpu_3d_pct } else { 'n/a' }
    $dec = if ($null -ne $r.gpu_decode_pct) { $r.gpu_decode_pct } else { 'n/a' }
    Write-Host ("{0,-8} {1,12} {2,12} {3,12}" -f $i, $tot, $d3, $dec)
}
$live = @($rows | Where-Object { $null -ne $_.gpu_3d_pct -and $_.gpu_3d_pct -gt 0.5 })
$decLive = @($rows | Where-Object { $null -ne $_.gpu_decode_pct -and $_.gpu_decode_pct -gt 0.5 })
Write-Host ""
Write-Host ("samples with 3D work  > 0.5%: {0}/{1}" -f $live.Count, $rows.Count)
Write-Host ("samples with decode   > 0.5%: {0}/{1}" -f $decLive.Count, $rows.Count)
if ($live.Count -lt ($rows.Count - 1) -or $decLive.Count -lt ($rows.Count - 1)) {
    Write-Error "the VO did NOT keep presenting: the GPU work is not sustained"
    exit 1
}
Write-Host "the VO kept rendering for the whole window (sustained GPU work)"
exit 0
