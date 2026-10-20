# tools/look_desktop2.ps1 - start the engine with extra args, screenshot.
[CmdletBinding()]
param(
    [string]$EngineExe = '',
    [string]$OutPng = '',
    [string[]]$ExtraArgs = @(),
    [int]$WaitSec = 10
)
$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
. (Join-Path $here 'bench_lib.ps1')
if ([string]::IsNullOrWhiteSpace($EngineExe)) { $EngineExe = Join-Path $here '..\build\release\engine.exe' }
if ([string]::IsNullOrWhiteSpace($OutPng)) { $OutPng = Join-Path $here '..\build\desktop-look2.png' }
$engine = [System.IO.Path]::GetFullPath($EngineExe)
$video = Join-Path $here '..\tests\corpus\anime.mp4'
$dataDir = Join-Path $env:TEMP ("k6wp-look2-" + [Guid]::NewGuid().ToString('N').Substring(0, 8))
New-Item -ItemType Directory -Force -Path (Join-Path $dataDir 'K6WP') | Out-Null
$busy = @(Get-Process engine -ErrorAction SilentlyContinue)
if ($busy.Count -gt 0) { throw "engine already running" }
$old = $env:LOCALAPPDATA
$argList = @("--video=$video", '--exit-after-ms=60000') + $ExtraArgs
try {
    $env:LOCALAPPDATA = $dataDir
    $p = Start-Process -FilePath $engine -ArgumentList $argList -PassThru -WindowStyle Hidden
}
finally { $env:LOCALAPPDATA = $old }
if (-not (Wait-K6wpEngine -TimeoutMs 15000)) { throw "engine did not answer the pipe" }
Start-Sleep -Seconds $WaitSec
$saved = Get-DesktopWindows
try {
    Start-Sleep -Seconds 3
    $st = Send-K6wpIpc -Cmd 'get_state' -TimeoutMs 3000
    Write-Host ("paused={0}" -f $st.state.paused)
    Save-ScreenPng -Path $OutPng
    Write-Host "screenshot -> $OutPng"
}
finally {
    Restore-DesktopWindows -Saved $saved
    try { if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue } } catch { }
}
Get-Content (Join-Path $dataDir 'K6WP\engine.log') | Select-String '24H2 path ->|wallpaper-mode|occlusion: slot|pause: owner' | ForEach-Object { $_.Line } | Select-Object -First 10
