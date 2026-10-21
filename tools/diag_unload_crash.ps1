# tools/diag_unload_crash.ps1 - reproduce the post-unload crash and capture
# the exit code + the last log lines.
$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
. (Join-Path $here 'bench_lib.ps1')

$engine = Join-Path $here '..\build\release\engine.exe'
$video = Join-Path $here '..\tests\corpus\anime.mp4'
$dataDir = Join-Path $env:TEMP ("k6wp-crash-" + [Guid]::NewGuid().ToString('N').Substring(0, 8))
New-Item -ItemType Directory -Force -Path (Join-Path $dataDir 'K6WP') | Out-Null
$old = $env:LOCALAPPDATA
try {
    $env:LOCALAPPDATA = $dataDir
    $p = Start-Process -FilePath $engine -PassThru -WindowStyle Hidden -ArgumentList @(
        "--video=$video", "--unload-after-pause-ms=4000", '--exit-after-ms=60000')
}
finally { $env:LOCALAPPDATA = $old }
if (-not (Wait-K6wpEngine -TimeoutMs 20000)) { throw "engine did not answer the pipe" }
$saved = Get-DesktopWindows
try {
    Start-Sleep -Seconds 4
    [void](Send-K6wpIpc -Cmd 'pause' -TimeoutMs 3000)
    Write-Host "paused; waiting for the unload..."
    for ($i = 0; $i -lt 30; $i++) {
        Start-Sleep -Milliseconds 500
        if ($p.HasExited) { break }
        $st = Send-K6wpIpc -Cmd 'get_state' -TimeoutMs 1500 -ErrorAction SilentlyContinue
        if ($st -and $st.state.decode_state -eq 'unloaded') { Write-Host "unloaded"; break }
    }
    Start-Sleep -Seconds 2
    if ($p.HasExited) {
        Write-Host ("ENGINE EXITED: code={0}" -f $p.ExitCode)
    } else {
        Write-Host "engine still alive"
        try { [void](Send-K6wpIpc -Cmd 'resume' -TimeoutMs 3000) } catch { Write-Host "resume failed: $($_.Exception.Message)" }
        Start-Sleep -Seconds 3
        Write-Host ("after resume: hasExited={0}" -f $p.HasExited)
    }
}
finally {
    Restore-DesktopWindows -Saved $saved
    try { if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue } } catch { }
}
Write-Host "--- engine log tail ---"
Get-Content (Join-Path $dataDir 'K6WP\engine.log') -Tail 20
Write-Host "--- crash dumps ---"
Get-ChildItem (Join-Path $dataDir 'K6WP') -Recurse -Filter '*.dmp' -ErrorAction SilentlyContinue | Select-Object Name,Length
