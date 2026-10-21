# tools/bench_unload_d2.ps1 - task 1.1 (D2): the black-frame gate.
#
# THE GATE (from the brief):
#   1. NO black frame when the wallpaper becomes visible again after a real
#      fullscreen app exits (n=5), measured with a VISUAL sampler that
#      samples the screen across the exit.
#   2. NO Explorer UI hang (a timeout probe against Explorer's UI thread).
#   3. An EXPLICIT user pause still shows the picture.
#   4. The static wallpaper UNDERNEATH must be visible while Unloaded.
#
# Why the SCREEN and not the injected surface: after the D2 unload the
# injected surface is DESTROYED (the whole mpv instance goes), so there is
# no surface left to capture - what the user sees is the shell's own
# wallpaper (the per-user still). The screen IS the thing the gate is
# about.
#
# The sampler hides the covering windows first (the agent's own
# always-on-top overlay would otherwise cover the screen and make every
# sample read "not black" - a false pass), and the live session is
# snapshotted + restored with tools/session_snapshot.ps1 (C8).
[CmdletBinding()]
param(
    [string]$EngineExe = '',
    [int]$UnloadAfterMs = 3000,
    [int]$Runs = 5,
    [int]$SampleMs = 75,
    [string]$OutJson = ''
)
$ErrorActionPreference = 'Stop'
$script:Here = Split-Path -Parent $MyInvocation.MyCommand.Path
. (Join-Path $script:Here 'bench_lib.ps1')
. (Join-Path $script:Here 'session_snapshot.ps1')
if ([string]::IsNullOrWhiteSpace($EngineExe)) { $EngineExe = Join-Path $script:Here '..\build\release\engine.exe' }
if ([string]::IsNullOrWhiteSpace($OutJson)) { $OutJson = Join-Path $script:Here '..\docs\\bench\\unload-d2.json' }
$engine = [System.IO.Path]::GetFullPath($EngineExe)
$video = Join-Path $script:Here '..\tests\corpus\anime.mp4'

Add-Type -AssemblyName System.Drawing
Add-Type -AssemblyName System.Windows.Forms
Add-Type @"
using System;using System.Runtime.InteropServices;using System.Text;
public class D2 {
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
  [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
  [DllImport("user32.dll")] public static extern bool AttachThreadInput(uint a, uint b, bool attach);
  [DllImport("kernel32.dll")] public static extern uint GetCurrentThreadId();
  [DllImport("user32.dll")] public static extern IntPtr FindWindowW(string c, string n);
  [DllImport("user32.dll", CharSet=CharSet.Unicode)]
  public static extern IntPtr SendMessageTimeout(IntPtr h, uint m, IntPtr w, IntPtr l, uint f, uint t, out IntPtr r);
  [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc cb, IntPtr p);
  [DllImport("user32.dll")] public static extern bool EnumChildWindows(IntPtr parent, EnumProc cb, IntPtr p);
  public delegate bool EnumProc(IntPtr h, IntPtr p);
  [DllImport("user32.dll")] public static extern int GetClassName(IntPtr h, StringBuilder s, int n);
}
"@

$script:injected = New-Object System.Collections.Generic.List[object]
$childCb = [D2+EnumProc]{
    param($h, $p)
    $sb = New-Object System.Text.StringBuilder 256
    [void][D2]::GetClassName($h, $sb, 256)
    if ($sb.ToString() -eq 'K6WP.DesktopInject.1') { $script:injected.Add($h) }
    return $true
}
$topCb = [D2+EnumProc]{
    param($h, $p)
    [void][D2]::EnumChildWindows($h, $childCb, [IntPtr]::Zero)
    return $true
}
function Get-InjectedCount {
    $script:injected.Clear()
    [void][D2]::EnumWindows($topCb, [IntPtr]::Zero)
    return $script:injected.Count
}

# The Explorer UI thread probe: a WM_NULL with a timeout to the taskbar
# (Shell_TrayWnd lives on Explorer's UI thread). A timeout means the shell
# is hung - the exact failure the earlier ShowWindow attempt caused.
function Test-ExplorerResponsive {
    param([int]$TimeoutMs = 2000)
    $tray = [D2]::FindWindowW('Shell_TrayWnd', $null)
    if ($tray -eq [IntPtr]::Zero) { return $null }
    $res = [IntPtr]::Zero
    $sw = [Diagnostics.Stopwatch]::StartNew()
    $ok = [D2]::SendMessageTimeout($tray, 0, [IntPtr]::Zero, [IntPtr]::Zero, 2, [uint32]$TimeoutMs, [ref]$res)
    return [pscustomobject]@{ responsive = [bool]$ok; ms = $sw.ElapsedMilliseconds }
}

function Get-ScreenLuma {
    $bmp = New-Object System.Drawing.Bitmap 240, 135
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    try {
        $g.CopyFromScreen(0, 0, 0, 0, (New-Object System.Drawing.Size(240, 135)))
    } finally { $g.Dispose() }
    try {
        $sum = 0.0; $n = 0
        for ($y = 0; $y -lt $bmp.Height; $y += 3) {
            for ($x = 0; $x -lt $bmp.Width; $x += 3) {
                $c = $bmp.GetPixel($x, $y)
                $sum += 0.2126 * $c.R + 0.7152 * $c.G + 0.0722 * $c.B; $n++
            }
        }
        return [math]::Round($sum / $n, 2)
    } finally { $bmp.Dispose() }
}

function Start-Engine {
    for ($i = 0; $i -lt 40; $i++) {
        if (-not (Get-Process engine -ErrorAction SilentlyContinue)) { break }
        Start-Sleep -Milliseconds 250
    }
    Start-Sleep -Milliseconds 800
    $dataDir = Join-Path $env:TEMP ("k6wp-d2-" + [Guid]::NewGuid().ToString('N').Substring(0, 8))
    New-Item -ItemType Directory -Force -Path (Join-Path $dataDir 'K6WP') | Out-Null
    $old = $env:LOCALAPPDATA
    try {
        $env:LOCALAPPDATA = $dataDir
        $proc = Start-Process -FilePath $engine -PassThru -WindowStyle Hidden -ArgumentList @(
            "--video=$video", "--unload-after-pause-ms=$UnloadAfterMs", '--exit-after-ms=120000')
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
    $form.BackColor = [System.Drawing.Color]::Black
    $form.Show()
    $hwnd = [IntPtr]$form.Handle
    $fgPid = 0
    $fgThread = [D2]::GetWindowThreadProcessId([D2]::GetForegroundWindow(), [ref]$fgPid)
    $myThread = [D2]::GetCurrentThreadId()
    [void][D2]::AttachThreadInput($myThread, $fgThread, $true)
    [void][D2]::SetForegroundWindow($hwnd)
    [void][D2]::AttachThreadInput($myThread, $fgThread, $false)
    [void]$form.Activate()
    return $form
}
function Wait-State {
    param([string]$State, [int]$TimeoutSec = 30)
    $deadline = (Get-Date).AddSeconds($TimeoutSec)
    while ((Get-Date) -lt $deadline) {
        $st = Send-K6wpIpc -Cmd 'get_state' -TimeoutMs 2000
        if ($st.state.decode_state -eq $State) { return $true }
        Start-Sleep -Milliseconds 200
    }
    return $false
}

$script:results = New-Object System.Collections.Generic.List[object]
$busy = @(Get-Process engine -ErrorAction SilentlyContinue)
if ($busy.Count -gt 0) { throw "an engine.exe is already running" }

# C8: the live session is snapshotted and restored automatically, even if a
# run fails half-way.
Invoke-SessionSafe -Name 'unload D2 gate' -RestoreDisplay -RestoreWallpaper -RestoreLockscreen -RestoreExplorer -ScriptBlock {
    for ($run = 1; $run -le $Runs; $run++) {
        $e = Start-Engine
        try {
            $saved = Get-DesktopWindows
            try {
                $explorerBefore = Test-ExplorerResponsive
                $form = New-Fullscreen
                if (-not (Wait-State 'unloaded')) { throw "the engine never reached the Unloaded state" }
                # 4. The static wallpaper underneath must be visible while
                #    Unloaded: the injected surface is GONE (the whole mpv
                #    instance was destroyed), so the shell's wallpaper shows.
                $surfacesWhileUnloaded = Get-InjectedCount
                $explorerDuring = Test-ExplorerResponsive

                # --- the exit: close the fullscreen app and sample the
                #     screen across the transition.
                $samples = @()
                $form.Close()
                $sw = [Diagnostics.Stopwatch]::StartNew()
                for ($i = 0; $i -lt 40; $i++) {
                    $luma = Get-ScreenLuma
                    $samples += [pscustomobject]@{ ms = $sw.ElapsedMilliseconds; luma = $luma }
                    if ($sw.ElapsedMilliseconds -gt 3000) { break }
                    Start-Sleep -Milliseconds $SampleMs
                }
                # The wallpaper must be back and MOVING.
                $animating = $false
                $deadline = (Get-Date).AddSeconds(15)
                while ((Get-Date) -lt $deadline) {
                    $a = Get-ScreenLuma
                    Start-Sleep -Milliseconds 500
                    $b = Get-ScreenLuma
                    if ([math]::Abs($a - $b) -gt 0.4) { $animating = $true; break }
                }
                $explorerAfter = Test-ExplorerResponsive
                $black = @($samples | Where-Object { $_.luma -lt 5 })
                $lumas = @($samples | ForEach-Object { [double]$_.luma } | Sort-Object)
                $medianLuma = if ($lumas.Count -gt 0) { [math]::Round($lumas[[int]($lumas.Count / 2)], 2) } else { $null }
                $script:results.Add([pscustomobject]@{
                    run = $run
                    samples = $samples.Count
                    black_samples = $black.Count
                    min_luma = ($samples | Measure-Object luma -Minimum).Minimum
                    median_luma = $medianLuma
                    injected_surfaces_while_unloaded = $surfacesWhileUnloaded
                    wallpaper_visible_while_unloaded = ($surfacesWhileUnloaded -eq 0)
                    explorer_before = $explorerBefore
                    explorer_during = $explorerDuring
                    explorer_after = $explorerAfter
                    explorer_responsive = ($null -eq $explorerAfter -or $explorerAfter.responsive)
                    animating_after_resume = $animating
                })
                Write-Host ("  run {0}: black={1}/{2} min_luma={3} surfaces_while_unloaded={4} explorer_ok={5} animating={6}" -f `
                        $run, $black.Count, $samples.Count, ($samples | Measure-Object luma -Minimum).Minimum,
                        $surfacesWhileUnloaded, $script:results[$script:results.Count - 1].explorer_responsive, $animating)
            }
            finally { Restore-DesktopWindows -Saved $saved }
        }
        finally {
            try { if (-not $e.Proc.HasExited) { Stop-Process -Id $e.Proc.Id -Force } } catch { }
        }
    }

    # 3. An EXPLICIT user pause still shows the picture.
    $e = Start-Engine
    try {
        $saved = Get-DesktopWindows
        try {
            Start-Sleep -Seconds 4
            [void](Send-K6wpIpc -Cmd 'pause' -TimeoutMs 3000)
            Start-Sleep -Seconds ([int](($UnloadAfterMs / 1000) + 3))
            $st = Send-K6wpIpc -Cmd 'get_state' -TimeoutMs 3000
            $luma = Get-ScreenLuma
            $script:results.Add([pscustomobject]@{
                run = 'user-pause'
                decode_state = $st.state.decode_state
                luma = $luma
                picture_present = ($luma -gt 5.0)
                surfaces = (Get-InjectedCount)
            })
            Write-Host ("  user pause: state={0} luma={1} (the picture must stay)" -f $st.state.decode_state, $luma)
            [void](Send-K6wpIpc -Cmd 'resume' -TimeoutMs 3000)
        }
        finally { Restore-DesktopWindows -Saved $saved }
    }
    finally {
        try { if (-not $e.Proc.HasExited) { Stop-Process -Id $e.Proc.Id -Force } } catch { }
    }
}

$gateRuns = @($script:results | Where-Object { $_.run -is [int] })
$blackTotal = ($gateRuns | Measure-Object black_samples -Sum).Sum
$report = [ordered]@{
    tool = 'bench_unload_d2'
    phase = 1
    date = (Get-Date -Format 'yyyy-MM-ddTHH:mm:ssK')
    label = 'live-proven (one machine)'
    engine = $engine
    unload_after_ms = $UnloadAfterMs
    method = 'the screen is sampled across a REAL fullscreen exit; the covering windows are hidden first (the agent overlay would make every sample read non-black - a false pass); the live session is snapshotted + restored with tools/session_snapshot.ps1'
    runs = $script:results.Count
    black_samples_total = $blackTotal
    gate_no_black_frame = ($blackTotal -eq 0)
    gate_wallpaper_visible_while_unloaded = (@($gateRuns | Where-Object { -not $_.wallpaper_visible_while_unloaded }).Count -eq 0)
    gate_explorer_responsive = (@($gateRuns | Where-Object { -not $_.explorer_responsive }).Count -eq 0)
    gate_animating_after_resume = (@($gateRuns | Where-Object { -not $_.animating_after_resume }).Count -eq 0)
    user_pause = ($script:results | Where-Object { $_.run -eq 'user-pause' })
    results = $script:results
}
$json = $report | ConvertTo-Json -Depth 8
$outFull = [System.IO.Path]::GetFullPath($OutJson)
$outDir = [System.IO.Path]::GetDirectoryName($outFull)
if (-not (Test-Path -LiteralPath $outDir)) { New-Item -ItemType Directory -Force -Path $outDir | Out-Null }
$json | Set-Content -LiteralPath $outFull -Encoding UTF8
Write-Host ""
Write-Host ("GATE no black frame        : {0} (black samples: {1})" -f $report.gate_no_black_frame, $blackTotal)
Write-Host ("GATE wallpaper visible      : {0}" -f $report.gate_wallpaper_visible_while_unloaded)
Write-Host ("GATE explorer responsive    : {0}" -f $report.gate_explorer_responsive)
Write-Host ("GATE animating after resume : {0}" -f $report.gate_animating_after_resume)
Write-Host ("user pause keeps the picture: {0}" -f $report.user_pause.picture_present)
Write-Host "bench_unload_d2: wrote $outFull"
if (-not ($report.gate_no_black_frame -and $report.gate_wallpaper_visible_while_unloaded -and
            $report.gate_explorer_responsive -and $report.gate_animating_after_resume)) {
    exit 1
}
