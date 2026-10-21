# tools/bench_unload.ps1 - task 1.1: does the unload actually free the RAM?
#
# The engine is started with --unload-after-pause-ms, paused over IPC, and
# the memory is sampled before and after the unload. The C4 metrics are the
# honest ones: PRIVATE BYTES + GPU SHARED MEMORY. The working set is recorded
# but never decides a verdict (the engine trims its own working set).
#
# The pause is driven over the REAL IPC command, so the production path
# (pause -> mask -> deadline -> unload) is what is measured.
[CmdletBinding()]
param(
    [string]$EngineExe = '',
    [int]$UnloadAfterMs = 5000,
    [int]$SampleSeconds = 12,
    [string]$OutJson = '',
    # "ipc" = the explicit user pause (the wallpaper stays ON SCREEN, so the
    #         engine deliberately keeps the decode pipeline: the control).
    # "fullscreen" = a real borderless fullscreen window, so the engine's
    #         fullscreen watcher pauses the decode AND nobody can see the
    #         wallpaper: the case the unload exists for.
    [ValidateSet('ipc', 'fullscreen')][string]$Mode = 'fullscreen'
)
$ErrorActionPreference = 'Stop'
$script:Here = Split-Path -Parent $MyInvocation.MyCommand.Path
. (Join-Path $script:Here 'bench_lib.ps1')
if ([string]::IsNullOrWhiteSpace($EngineExe)) { $EngineExe = Join-Path $script:Here '..\build\release\engine.exe' }
if ([string]::IsNullOrWhiteSpace($OutJson)) { $OutJson = Join-Path $script:Here '..\docs\bench\unload.json' }
$engine = [System.IO.Path]::GetFullPath($EngineExe)
$video = Join-Path $script:Here '..\tests\corpus\anime.mp4'

$busy = @(Get-Process engine -ErrorAction SilentlyContinue)
if ($busy.Count -gt 0) { throw "an engine.exe is already running" }

$dataDir = Join-Path $env:TEMP ("k6wp-unload-" + [Guid]::NewGuid().ToString('N').Substring(0, 8))
New-Item -ItemType Directory -Force -Path (Join-Path $dataDir 'K6WP') | Out-Null
$old = $env:LOCALAPPDATA
try {
    $env:LOCALAPPDATA = $dataDir
    $p = Start-Process -FilePath $engine -PassThru -WindowStyle Hidden -ArgumentList @(
        "--video=$video", "--unload-after-pause-ms=$UnloadAfterMs", '--exit-after-ms=120000')
}
finally { $env:LOCALAPPDATA = $old }
if (-not (Wait-K6wpEngine -TimeoutMs 20000)) { throw "engine did not answer the pipe" }

# Minimize the covering windows so the engine is not occlusion-paused.
$saved = Get-DesktopWindows
try {
    Start-Sleep -Seconds 5
    $tp = Start-TypeperfSampler -ProcessId $p.Id -Seconds 60 -ProcessName 'engine'
    $series = New-Object System.Collections.Generic.List[object]
    $gpuRows = New-Object System.Collections.Generic.List[object]

    # The real IPC pause: the production path (pause -> mask -> deadline ->
    # unload) is what is measured, not a copy of it.
    $ack = $null
    if ($Mode -eq 'ipc') {
        $ack = Send-K6wpIpc -Cmd 'pause' -TimeoutMs 3000
        Write-Host ("bench_unload: pause ack ok={0}" -f $ack.ok)
    } else {
        # A real borderless fullscreen window: the engine's fullscreen
        # watcher detects it exactly as it would a fullscreen game.
        Add-Type -AssemblyName System.Windows.Forms
        Add-Type @"
using System;using System.Runtime.InteropServices;
public class Fg {
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int c);
  [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
  [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr h, IntPtr a, int x, int y, int cx, int cy, uint f);
  [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
  [DllImport("user32.dll")] public static extern bool AttachThreadInput(uint a, uint b, bool attach);
  [DllImport("kernel32.dll")] public static extern uint GetCurrentThreadId();
}
"@
        $form = New-Object System.Windows.Forms.Form
        $form.FormBorderStyle = 'None'
        $form.StartPosition = 'Manual'
        $form.Location = [System.Drawing.Point]::new(0, 0)
        $form.Size = [System.Drawing.Size]::new(
            [System.Windows.Forms.SystemInformation]::VirtualScreen.Width,
            [System.Windows.Forms.SystemInformation]::VirtualScreen.Height)
        $form.TopMost = $true
        $form.ShowInTaskbar = $false
        $form.Show()
        # The engine's fullscreen watcher only looks at the FOREGROUND window,
        # so the form must actually take the foreground.
        $hwnd = [IntPtr]$form.Handle
        $fgPid = 0
        $fgThread = [Fg]::GetWindowThreadProcessId([Fg]::GetForegroundWindow(), [ref]$fgPid)
        $myThread = [Fg]::GetCurrentThreadId()
        [void][Fg]::AttachThreadInput($myThread, $fgThread, $true)
        [void][Fg]::SetForegroundWindow($hwnd)
        [void][Fg]::AttachThreadInput($myThread, $fgThread, $false)
        [void]$form.Activate()
        Write-Host ("bench_unload: fullscreen window up (fg={0}), waiting for the engine to pause" -f `
            [Fg]::GetForegroundWindow())
        for ($i = 0; $i -lt 20; $i++) {
            Start-Sleep -Milliseconds 300
            $st = Send-K6wpIpc -Cmd 'get_state' -TimeoutMs 2000
            if ($st.state.paused) { break }
        }
    }

    # Sample through the pause, the unload and the resume.
    $phase = 'playing'
    for ($i = 0; $i -lt 30; $i++) {
        $series.Add((Get-ProcessSeries -ProcessId $p.Id -Seconds 1 -IntervalMs 500)[0])
        $gpuRows.AddRange(@(Read-TypeperfCsv -Path $tp.Csv | Select-Object -Last 1))
        $st = Send-K6wpIpc -Cmd 'get_state' -TimeoutMs 2000
        $ds = $st.state.decode_state
        if ($ds -ne $phase) {
            $phase = $ds
            Write-Host ("  -> decode_state = {0} (t={1}s)" -f $phase, $i)
        }
        if ($phase -eq 'unloaded' -and $i -gt 6) {
            Write-Host "  -> resuming"
            if ($Mode -eq 'ipc') {
                [void](Send-K6wpIpc -Cmd 'resume' -TimeoutMs 3000)
            } else {
                # Leaving fullscreen is what clears the fullscreen bit; the
                # engine then reloads the video by itself.
                $form.Close()
            }
        }
        if ($phase -eq 'active' -and $i -gt 10) { break }
    }
    if ($Mode -eq 'fullscreen' -and $null -ne $form) { try { $form.Close() } catch { } }
    try { [void]$tp.Process.WaitForExit(10000) } catch { }
    $gpuAll = @(Read-TypeperfCsv -Path $tp.Csv)

    $logPath = Join-Path $dataDir 'K6WP\engine.log'
    $log = @(Get-Content -LiteralPath $logPath -ErrorAction SilentlyContinue)

    $priv = @($series | ForEach-Object { $_.private_mb })
    $gpuShared = @($gpuAll | ForEach-Object { $_.gpu_shared_mb } | Where-Object { $null -ne $_ })
    $result = [ordered]@{
        tool = 'bench_unload'
        phase = 1
        date = (Get-Date -Format 'yyyy-MM-ddTHH:mm:ssK')
        label = 'live-proven (one machine)'
        engine = $engine
        unload_after_ms = $UnloadAfterMs
        method = 'the real IPC pause -> mask -> deadline -> unload path; private bytes + GPU shared (C4), the working set is recorded but never decides'
        private_mb = (Get-Stats $priv)
        gpu_shared_mb = (Get-Stats $gpuShared)
        gpu_total_pct = (Get-Stats @($gpuAll | ForEach-Object { $_.gpu_total_pct } | Where-Object { $null -ne $_ })).median
        series = $series
        gpu_series = $gpuAll
        log_tail = @($log | Select-String 'pause: owner|decode: ' | ForEach-Object { $_.Line })
    }

    Write-Host ""
    Write-Host "--- engine log (pause / unload / reload) ---"
    $result.log_tail | ForEach-Object { Write-Host "  $_" }
    Write-Host ""
    Write-Host "--- per-sample RAM (private is the C4 metric) ---"
    $i = 0
    foreach ($s in $series) {
        $i++
        Write-Host ("  t={0,6} ms  private={1,7} MB  ws={2,6} MB  threads={3}" -f $s.t_ms, $s.private_mb, $s.ws_mb, $s.threads)
    }
    $json = $result | ConvertTo-Json -Depth 8
    $outFull = [System.IO.Path]::GetFullPath($OutJson)
    $outDir = [System.IO.Path]::GetDirectoryName($outFull)
    if (-not (Test-Path -LiteralPath $outDir)) { New-Item -ItemType Directory -Force -Path $outDir | Out-Null }
    $json | Set-Content -LiteralPath $outFull -Encoding UTF8
    Write-Host "bench_unload: wrote $outFull"
}
finally {
    Restore-DesktopWindows -Saved $saved
    try { if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue } } catch { }
}
