# tools/capture_visual_baseline.ps1 - drives the visual parity capture.
#
# Starts the real engine on the corpus clip, lets it play, captures the
# desktop region, then stops the engine. The engine MUST be running for the
# capture to show the wallpaper (otherwise the capture is just the plain
# desktop and the baseline would be meaningless).
$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
. (Join-Path $here 'bench_lib.ps1')

$engine = Join-Path $here '..\build\release\engine.exe'
$video = Join-Path $here '..\tests\corpus\anime.mp4'
$outDir = Join-Path $here '..\docs\bench\visual\baseline'
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
Write-Host "capture_visual_baseline: engine pid=$($p.Id), playing $video"
Start-Sleep -Seconds 6

try {
    & (Join-Path $here 'bench_visual.ps1') -Capture $outDir -Force
}
finally {
    try { [void](Send-K6wpIpc -Cmd 'quit' -TimeoutMs 2000) } catch { }
    try { if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue } } catch { }
}
Write-Host "capture_visual_baseline: done -> $outDir"
