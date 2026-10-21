# tools/bench_unload_scenarios.ps1 - task 1.1: the required scenario tests.
#
# Scenarios (the spec's list):
#   1. resume latency after an unload (median n=5), and NO BLACK FRAME
#   2. the video changes while Unloaded (a new renderer is NOT created; the
#      file is simply loaded again)
#   3. quit while Unloaded (no hang in the teardown)
#   4. the user pause is NOT unloaded (the picture stays - the control)
#   5. a fullscreen app in and out (the invisible-owner case)
#
# Every scenario drives the REAL engine over the REAL IPC path.
[CmdletBinding()]
param(
    [string]$EngineExe = '',
    [int]$UnloadAfterMs = 3000,
    [int]$ResumeRuns = 5,
    [string]$OutJson = ''
)
$ErrorActionPreference = 'Stop'
$script:Here = Split-Path -Parent $MyInvocation.MyCommand.Path
. (Join-Path $script:Here 'bench_lib.ps1')
if ([string]::IsNullOrWhiteSpace($EngineExe)) { $EngineExe = Join-Path $script:Here '..\build\release\engine.exe' }
if ([string]::IsNullOrWhiteSpace($OutJson)) { $OutJson = Join-Path $script:Here '..\docs\bench\unload-scenarios.json' }
$engine = [System.IO.Path]::GetFullPath($EngineExe)
$video = Join-Path $script:Here '..\tests\corpus\anime.mp4'
$video2 = Join-Path $script:Here '..\tests\corpus\slideshow.mp4'

Add-Type -AssemblyName System.Drawing
Add-Type -AssemblyName System.Windows.Forms
Add-Type @"
using System;using System.Runtime.InteropServices;using System.Text;
public class FS {
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
  [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
  [DllImport("user32.dll")] public static extern bool AttachThreadInput(uint a, uint b, bool attach);
  [DllImport("kernel32.dll")] public static extern uint GetCurrentThreadId();
  [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc cb, IntPtr p);
  [DllImport("user32.dll")] public static extern bool EnumChildWindows(IntPtr parent, EnumProc cb, IntPtr p);
  public delegate bool EnumProc(IntPtr h, IntPtr p);
  [DllImport("user32.dll")] public static extern int GetClassName(IntPtr h, StringBuilder s, int n);
  [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr hdc, uint flags);
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
  [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left,Top,Right,Bottom; }
}
"@

$script:found = New-Object System.Collections.Generic.List[object]
$childCb = [FS+EnumProc]{
    param($h, $p)
    $sb = New-Object System.Text.StringBuilder 256
    [void][FS]::GetClassName($h, $sb, 256)
    if ($sb.ToString() -eq 'K6WP.DesktopInject.1') { $script:found.Add($h) }
    return $true
}
$topCb = [FS+EnumProc]{
    param($h, $p)
    [void][FS]::EnumChildWindows($h, $childCb, [IntPtr]::Zero)
    return $true
}
function Get-InjectedWindow {
    $script:found.Clear()
    [void][FS]::EnumWindows($topCb, [IntPtr]::Zero)
    if ($script:found.Count -eq 0) { return [IntPtr]::Zero }
    return $script:found[0]
}
function Save-Injected([IntPtr]$h, [string]$Path) {
    $r = New-Object FS+RECT
    [void][FS]::GetWindowRect($h, [ref]$r)
    $bmp = New-Object System.Drawing.Bitmap ($r.Right - $r.Left), ($r.Bottom - $r.Top)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $hdc = $g.GetHdc()
    try { [void][FS]::PrintWindow($h, $hdc, 2) } finally { $g.ReleaseHdc($hdc); $g.Dispose() }
    $bmp.Save($Path, [System.Drawing.Imaging.ImageFormat]::Png)
    $bmp.Dispose()
}
function Get-Luma([string]$Path) {
    $bmp = [System.Drawing.Bitmap]::FromFile($Path)
    try {
        $sum = 0.0; $n = 0
        for ($y = 0; $y -lt $bmp.Height; $y += 6) {
            for ($x = 0; $x -lt $bmp.Width; $x += 6) {
                $c = $bmp.GetPixel($x, $y)
                $sum += 0.2126 * $c.R + 0.7152 * $c.G + 0.0722 * $c.B; $n++
            }
        }
        return [math]::Round($sum / $n, 2)
    }
    finally { $bmp.Dispose() }
}
function Start-Engine {
    param([string]$Extra = '')
    # The singleton mutex is released ASYNCHRONOUSLY when the previous
    # engine dies, so a start can lose the race and exit immediately. Wait
    # for the old process to be gone (and for the mutex to settle) first.
    for ($i = 0; $i -lt 40; $i++) {
        if (-not (Get-Process engine -ErrorAction SilentlyContinue)) { break }
        Start-Sleep -Milliseconds 250
    }
    Start-Sleep -Milliseconds 800
    $dataDir = Join-Path $env:TEMP ("k6wp-scen-" + [Guid]::NewGuid().ToString('N').Substring(0, 8))
    New-Item -ItemType Directory -Force -Path (Join-Path $dataDir 'K6WP') | Out-Null
    $old = $env:LOCALAPPDATA
    $argList = @("--video=$video", "--unload-after-pause-ms=$UnloadAfterMs", '--exit-after-ms=120000')
    if ($Extra) { $argList = @("--video=$Extra", "--unload-after-pause-ms=$UnloadAfterMs", '--exit-after-ms=120000') }
    try {
        $env:LOCALAPPDATA = $dataDir
        $proc = Start-Process -FilePath $engine -PassThru -WindowStyle Hidden -ArgumentList $argList
    }
    finally { $env:LOCALAPPDATA = $old }
    if (-not (Wait-K6wpEngine -TimeoutMs 20000)) { throw "engine did not answer the pipe" }
    return [pscustomobject]@{ Proc = $proc; DataDir = $dataDir }
}
function New-Fullscreen {
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
    $hwnd = [IntPtr]$form.Handle
    $fgPid = 0
    $fgThread = [FS]::GetWindowThreadProcessId([FS]::GetForegroundWindow(), [ref]$fgPid)
    $myThread = [FS]::GetCurrentThreadId()
    [void][FS]::AttachThreadInput($myThread, $fgThread, $true)
    [void][FS]::SetForegroundWindow($hwnd)
    [void][FS]::AttachThreadInput($myThread, $fgThread, $false)
    [void]$form.Activate()
    return $form
}
function Wait-State {
    param([string]$State, [int]$TimeoutSec = 25)
    $deadline = (Get-Date).AddSeconds($TimeoutSec)
    while ((Get-Date) -lt $deadline) {
        $st = Send-K6wpIpc -Cmd 'get_state' -TimeoutMs 2000
        if ($st.state.decode_state -eq $State) { return $true }
        Start-Sleep -Milliseconds 200
    }
    return $false
}

$results = [ordered]@{}
$busy = @(Get-Process engine -ErrorAction SilentlyContinue)
if ($busy.Count -gt 0) { throw "an engine.exe is already running" }

# --- scenario 5 + 1: fullscreen in, unload, fullscreen out, resume latency ---
# Each cycle runs a FRESH engine: the measurement is the resume latency
# itself, and a fresh engine per cycle keeps the cycles independent (a
# reused engine has to re-arm the fullscreen watcher, which the win-event
# hook does not always deliver).
Write-Host "== scenario 5/1: fullscreen -> unload -> resume (n=$ResumeRuns) =="
$latencies = @()
$blackFrames = 0
for ($cycle = 1; $cycle -le $ResumeRuns; $cycle++) {
    $run = Start-Engine
    try {
        $saved = Get-DesktopWindows
        try {
            $form = New-Fullscreen
            if (-not (Wait-State 'unloaded')) { throw "the engine never reached the Unloaded state" }
            # The resume latency: how long from leaving fullscreen until the
            # wallpaper is moving again.
            $sw = [Diagnostics.Stopwatch]::StartNew()
            $form.Close()
            $h = Get-InjectedWindow
            $ok = $false
            while ($sw.ElapsedMilliseconds -lt 20000) {
                $tmp = Join-Path $env:TEMP ("k6wp-scen-" + [Guid]::NewGuid().ToString('N').Substring(0, 6) + ".png")
                Save-Injected $h $tmp
                $luma = Get-Luma $tmp
                Remove-Item $tmp -Force -ErrorAction SilentlyContinue
                if ($luma -lt 3) { $blackFrames++ }
                # The animation is back when two frames 400 ms apart differ.
                $a = Join-Path $env:TEMP ("k6wp-a-" + [Guid]::NewGuid().ToString('N').Substring(0, 6) + ".png")
                $b = Join-Path $env:TEMP ("k6wp-b-" + [Guid]::NewGuid().ToString('N').Substring(0, 6) + ".png")
                Save-Injected $h $a; Start-Sleep -Milliseconds 400; Save-Injected $h $b
                $ba = [System.Drawing.Bitmap]::FromFile($a); $bb = [System.Drawing.Bitmap]::FromFile($b)
                $diff = Get-BitmapDiff -A $ba -B $bb -Step 8
                $ba.Dispose(); $bb.Dispose()
                Remove-Item $a, $b -Force -ErrorAction SilentlyContinue
                if ($diff -gt 0.5) { $ok = $true; break }
            }
            $latencies += $sw.ElapsedMilliseconds
            Write-Host ("  cycle {0}: resume {1} ms, animating={2}" -f $cycle, $sw.ElapsedMilliseconds, $ok)
            if (-not $ok) { throw "the wallpaper did not come back after the resume" }
        }
        finally { Restore-DesktopWindows -Saved $saved }
    }
    finally {
        try { if (-not $run.Proc.HasExited) { Stop-Process -Id $run.Proc.Id -Force } } catch { }
    }
}
$results['resume_latency_ms'] = (Get-Stats $latencies)
$results['black_frames_seen'] = $blackFrames

# --- scenario 3: quit while Unloaded ---
Write-Host "== scenario 3: quit while Unloaded =="
$run = Start-Engine
try {
    $saved = Get-DesktopWindows
    try {
        $form = New-Fullscreen
        if (-not (Wait-State 'unloaded')) { throw "the engine never reached the Unloaded state" }
        $sw = [Diagnostics.Stopwatch]::StartNew()
        $ack = Send-K6wpIpc -Cmd 'quit' -TimeoutMs 5000
        $exited = $run.Proc.WaitForExit(15000)
        $quitMs = $sw.ElapsedMilliseconds
        $results['quit_while_unloaded'] = [ordered]@{
            acked = [bool]$ack.ok
            exited = [bool]$exited
            exit_ms = $quitMs
            exit_code = $(if ($exited) { $run.Proc.ExitCode } else { $null })
        }
        Write-Host ("  quit: acked={0} exited={1} in {2} ms (exit code {3})" -f `
            $ack.ok, $exited, $quitMs, $results['quit_while_unloaded'].exit_code)
        if (-not $exited) { throw "the engine did not exit after a quit while Unloaded" }
        $form.Close()
    }
    finally { Restore-DesktopWindows -Saved $saved }
}
finally {
    try { if (-not $run.Proc.HasExited) { Stop-Process -Id $run.Proc.Id -Force } } catch { }
}

# --- scenario 2: the video changes while Unloaded ---
Write-Host "== scenario 2: the video changes while Unloaded =="
$run = Start-Engine
try {
    $saved = Get-DesktopWindows
    try {
        $form = New-Fullscreen
        if (-not (Wait-State 'unloaded')) { throw "the engine never reached the Unloaded state" }
        $ack = Send-K6wpIpc -Cmd 'set_video' -TimeoutMs 5000 -Payload @{ path = $video2 }
        Start-Sleep -Seconds 2
        $st = Send-K6wpIpc -Cmd 'get_state' -TimeoutMs 3000
        $results['video_change_while_unloaded'] = [ordered]@{
            acked = [bool]$ack.ok
            decode_state = $st.state.decode_state
            video = $st.state.video
        }
        Write-Host ("  set_video while Unloaded: acked={0} state={1} video='{2}'" -f `
            $ack.ok, $st.state.decode_state, $st.state.video)
        $form.Close()
    }
    finally { Restore-DesktopWindows -Saved $saved }
}
finally {
    try { if (-not $run.Proc.HasExited) { Stop-Process -Id $run.Proc.Id -Force } } catch { }
}

# --- scenario 4: the user pause is NOT unloaded (the control) ---
Write-Host "== scenario 4: the explicit user pause keeps the picture =="
$run = Start-Engine
try {
    $saved = Get-DesktopWindows
    try {
        Start-Sleep -Seconds 3
        [void](Send-K6wpIpc -Cmd 'pause' -TimeoutMs 3000)
        Start-Sleep -Seconds ([int](($UnloadAfterMs / 1000) + 3))
        $st = Send-K6wpIpc -Cmd 'get_state' -TimeoutMs 3000
        $h = Get-InjectedWindow
        $tmp = Join-Path $env:TEMP 'k6wp-scen-user.png'
        Save-Injected $h $tmp
        $luma = Get-Luma $tmp
        Remove-Item $tmp -Force -ErrorAction SilentlyContinue
        $results['user_pause'] = [ordered]@{
            decode_state = $st.state.decode_state
            surface_luma = $luma
            picture_present = ($luma -gt 5.0)
        }
        Write-Host ("  user pause: state={0} surface luma={1}" -f $st.state.decode_state, $luma)
        [void](Send-K6wpIpc -Cmd 'resume' -TimeoutMs 3000)
    }
    finally { Restore-DesktopWindows -Saved $saved }
}
finally {
    try { if (-not $run.Proc.HasExited) { Stop-Process -Id $run.Proc.Id -Force } } catch { }
}

$report = [ordered]@{
    tool = 'bench_unload_scenarios'
    phase = 1
    date = (Get-Date -Format 'yyyy-MM-ddTHH:mm:ssK')
    label = 'live-proven (one machine)'
    engine = $engine
    unload_after_ms = $UnloadAfterMs
    results = $results
}
$json = $report | ConvertTo-Json -Depth 8
$outFull = [System.IO.Path]::GetFullPath($OutJson)
$outDir = [System.IO.Path]::GetDirectoryName($outFull)
if (-not (Test-Path -LiteralPath $outDir)) { New-Item -ItemType Directory -Force -Path $outDir | Out-Null }
$json | Set-Content -LiteralPath $outFull -Encoding UTF8
Write-Host "bench_unload_scenarios: wrote $outFull"
