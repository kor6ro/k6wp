# tools/capture_visual_baseline.ps1 - drives the visual parity capture.
#
# Starts the real engine on the corpus clip, lets it play, captures the
# desktop region, then stops the engine. The engine MUST be running for the
# capture to show the wallpaper (otherwise the capture is just the plain
# desktop and the baseline would be meaningless).
[CmdletBinding()]
param(
    [string]$EngineExe = '',
    [string]$OutDir = ''
)
$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
. (Join-Path $here 'bench_lib.ps1')

if ([string]::IsNullOrWhiteSpace($EngineExe)) { $EngineExe = Join-Path $here '..\build\release\engine.exe' }
if ([string]::IsNullOrWhiteSpace($OutDir)) { $OutDir = Join-Path $here '..\docs\bench\visual\baseline' }
$engine = [System.IO.Path]::GetFullPath($EngineExe)
$video = Join-Path $here '..\tests\corpus\anime.mp4'
$outDir = [System.IO.Path]::GetFullPath($OutDir)
$dataDir = Join-Path $env:TEMP ("k6wp-visual-" + [Guid]::NewGuid().ToString('N').Substring(0, 8))
New-Item -ItemType Directory -Force -Path (Join-Path $dataDir 'K6WP') | Out-Null

$busy = @(Get-Process engine -ErrorAction SilentlyContinue)
if ($busy.Count -gt 0) { throw "an engine.exe is already running (pid $($busy.Id -join ','))" }

$old = $env:LOCALAPPDATA
try {
    $env:LOCALAPPDATA = $dataDir
    $p = Start-Process -FilePath $engine -ArgumentList @("--video=$video", '--exit-after-ms=120000') -PassThru -WindowStyle Hidden
}
finally { $env:LOCALAPPDATA = $old }

if (-not (Wait-K6wpEngine -TimeoutMs 15000)) {
    try { if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force } } catch { }
    throw "engine did not answer the IPC pipe"
}
Write-Host "capture_visual_baseline: engine pid=$($p.Id) ($engine), playing $video"
Start-Sleep -Seconds 4

# The wallpaper is only on screen while the engine is PLAYING:
#   - the occlusion / fullscreen watchers pause the decode, and a paused
#     engine shows the plain desktop instead of the video;
#   - the occlusion pause is PER SLOT, so get_state.paused (the global mask)
#     does NOT report it - the engine's log does.
# A capture taken during that pause silently records the plain desktop (the
# "negative" capture), which would make the parity gate compare two copies
# of the desktop background.
#
# So: hide the covering windows, then WAIT until the engine itself reports
# that it resumed, and only then capture.
$saved = Get-DesktopWindows
try {
    $logPath = Join-Path $dataDir 'K6WP\engine.log'
    $resumed = $false
    $deadline = (Get-Date).AddSeconds(30)
    while ((Get-Date) -lt $deadline) {
        if (Test-Path -LiteralPath $logPath) {
            $lines = @(Get-Content -LiteralPath $logPath -ErrorAction SilentlyContinue)
            $paused = @($lines | Select-String 'occlusion: slot \d+ coverage=100').Count
            $res = @($lines | Select-String 'occlusion: slot \d+ coverage=0\.0% resuming').Count
            if ($res -gt 0 -and $res -ge $paused) { $resumed = $true; break }
        }
        Start-Sleep -Milliseconds 400
    }
    if (-not $resumed) {
        throw "the engine never resumed playback (occlusion/fullscreen) - the capture would record the desktop, not the wallpaper"
    }
    Write-Host "capture_visual_baseline: engine resumed playback; settling 3s before the capture"
    Start-Sleep -Seconds 3
    & (Join-Path $here 'bench_visual.ps1') -Capture $outDir -Force
}
finally {
    Restore-DesktopWindows -Saved $saved
    try { [void](Send-K6wpIpc -Cmd 'quit' -TimeoutMs 2000) } catch { }
    try { if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue } } catch { }
}
Write-Host "capture_visual_baseline: done -> $outDir"
