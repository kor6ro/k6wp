# tools/bench_unload_visual.ps1 - task 1.1: the Unloaded state must not
# black out the wallpaper, and the resume must not flash a black frame.
#
# The check is done on the INJECTED wallpaper window itself (PrintWindow on
# the K6WP.DesktopInject.1 surface), not on a desktop screenshot: a desktop
# grab records the plain desktop whenever anything covers the screen, which
# would silently turn this into a no-op.
#
# Phases:
#   1. playing   - the wallpaper is animating
#   2. paused    - the file is still loaded
#   3. unloaded  - the file was released; the LAST FRAME must still be there
#   4. resumed   - the animation must be back
[CmdletBinding()]
param(
    [string]$EngineExe = '',
    [int]$UnloadAfterMs = 5000,
    [string]$OutJson = ''
)
$ErrorActionPreference = 'Stop'
$script:Here = Split-Path -Parent $MyInvocation.MyCommand.Path
. (Join-Path $script:Here 'bench_lib.ps1')
if ([string]::IsNullOrWhiteSpace($EngineExe)) { $EngineExe = Join-Path $script:Here '..\build\release\engine.exe' }
if ([string]::IsNullOrWhiteSpace($OutJson)) { $OutJson = Join-Path $script:Here '..\docs\bench\unload-visual.json' }
$engine = [System.IO.Path]::GetFullPath($EngineExe)
$video = Join-Path $script:Here '..\tests\corpus\anime.mp4'

Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;using System.Runtime.InteropServices;using System.Text;
public class WPV {
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
$childCb = [WPV+EnumProc]{
    param($h, $p)
    $sb = New-Object System.Text.StringBuilder 256
    [void][WPV]::GetClassName($h, $sb, 256)
    if ($sb.ToString() -eq 'K6WP.DesktopInject.1') { $script:found.Add($h) }
    return $true
}
$topCb = [WPV+EnumProc]{
    param($h, $p)
    [void][WPV]::EnumChildWindows($h, $childCb, [IntPtr]::Zero)
    return $true
}
function Get-InjectedWindow {
    $script:found.Clear()
    [void][WPV]::EnumWindows($topCb, [IntPtr]::Zero)
    if ($script:found.Count -eq 0) { return [IntPtr]::Zero }
    return $script:found[0]
}
function Save-Injected([IntPtr]$h, [string]$Path) {
    $r = New-Object WPV+RECT
    [void][WPV]::GetWindowRect($h, [ref]$r)
    $bmp = New-Object System.Drawing.Bitmap ($r.Right - $r.Left), ($r.Bottom - $r.Top)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $hdc = $g.GetHdc()
    try { [void][WPV]::PrintWindow($h, $hdc, 2) } finally { $g.ReleaseHdc($hdc); $g.Dispose() }
    $bmp.Save($Path, [System.Drawing.Imaging.ImageFormat]::Png)
    $bmp.Dispose()
}
function Get-Signature([string]$Path) {
    $bmp = [System.Drawing.Bitmap]::FromFile($Path)
    try {
        $sum = 0.0; $sum2 = 0.0; $n = 0
        for ($y = 0; $y -lt $bmp.Height; $y += 4) {
            for ($x = 0; $x -lt $bmp.Width; $x += 4) {
                $c = $bmp.GetPixel($x, $y)
                $l = 0.2126 * $c.R + 0.7152 * $c.G + 0.0722 * $c.B
                $sum += $l; $sum2 += $l * $l; $n++
            }
        }
        $mean = $sum / $n
        return [pscustomobject]@{
            mean_luma = [math]::Round($mean, 2)
            stddev = [math]::Round([math]::Sqrt([math]::Max(0, ($sum2 / $n) - ($mean * $mean))), 2)
        }
    }
    finally { $bmp.Dispose() }
}

$busy = @(Get-Process engine -ErrorAction SilentlyContinue)
if ($busy.Count -gt 0) { throw "an engine.exe is already running" }
$dataDir = Join-Path $env:TEMP ("k6wp-unloadv-" + [Guid]::NewGuid().ToString('N').Substring(0, 8))
New-Item -ItemType Directory -Force -Path (Join-Path $dataDir 'K6WP') | Out-Null
$old = $env:LOCALAPPDATA
try {
    $env:LOCALAPPDATA = $dataDir
    $p = Start-Process -FilePath $engine -PassThru -WindowStyle Hidden -ArgumentList @(
        "--video=$video", "--unload-after-pause-ms=$UnloadAfterMs", '--exit-after-ms=120000')
}
finally { $env:LOCALAPPDATA = $old }
if (-not (Wait-K6wpEngine -TimeoutMs 20000)) { throw "engine did not answer the pipe" }

$saved = Get-DesktopWindows
$shots = @{}
try {
    Start-Sleep -Seconds 5
    $h = Get-InjectedWindow
    if ($h -eq [IntPtr]::Zero) { throw "no injected wallpaper window found" }

    # Phase 1: playing (two frames 1.2 s apart - they must differ).
    $a = Join-Path $env:TEMP 'k6wp-unloadv-a.png'
    $b = Join-Path $env:TEMP 'k6wp-unloadv-b.png'
    Save-Injected $h $a
    Start-Sleep -Milliseconds 1200
    Save-Injected $h $b
    $ba = [System.Drawing.Bitmap]::FromFile($a); $bb = [System.Drawing.Bitmap]::FromFile($b)
    $playingDiff = Get-BitmapDiff -A $ba -B $bb -Step 6
    $ba.Dispose(); $bb.Dispose()
    $shots['playing1'] = $a; $shots['playing2'] = $b
    Write-Host ("phase 1 playing: frame-to-frame diff = {0}" -f [math]::Round($playingDiff, 3))

    # Phase 2: pause, then wait for the Unloaded state.
    [void](Send-K6wpIpc -Cmd 'pause' -TimeoutMs 3000)
    $deadline = (Get-Date).AddSeconds(20)
    $state = 'paused'
    while ((Get-Date) -lt $deadline) {
        $st = Send-K6wpIpc -Cmd 'get_state' -TimeoutMs 2000
        $state = $st.state.decode_state
        if ($state -eq 'unloaded') { break }
        Start-Sleep -Milliseconds 300
    }
    Write-Host ("phase 3 decode_state = {0}" -f $state)
    if ($state -ne 'unloaded') { throw "the engine never reached the Unloaded state" }

    # Phase 3: the unloaded surface must still show the picture (not black).
    $c = Join-Path $env:TEMP 'k6wp-unloadv-c.png'
    Save-Injected $h $c
    $shots['unloaded'] = $c
    $sigC = Get-Signature $c
    $sigA = Get-Signature $a
    Write-Host ("phase 3 unloaded: luma={0} stddev={1} (playing was luma={2} stddev={3})" -f `
        $sigC.mean_luma, $sigC.stddev, $sigA.mean_luma, $sigA.stddev)

    # Phase 4: resume, then the animation must be back.
    [void](Send-K6wpIpc -Cmd 'resume' -TimeoutMs 3000)
    Start-Sleep -Seconds 3
    $d = Join-Path $env:TEMP 'k6wp-unloadv-d.png'
    $e = Join-Path $env:TEMP 'k6wp-unloadv-e.png'
    Save-Injected $h $d
    Start-Sleep -Milliseconds 1200
    Save-Injected $h $e
    $shots['resumed1'] = $d; $shots['resumed2'] = $e
    $bd = [System.Drawing.Bitmap]::FromFile($d); $be = [System.Drawing.Bitmap]::FromFile($e)
    $resumedDiff = Get-BitmapDiff -A $bd -B $be -Step 6
    $bd.Dispose(); $be.Dispose()
    Write-Host ("phase 4 resumed: frame-to-frame diff = {0}" -f [math]::Round($resumedDiff, 3))

    $report = [ordered]@{
        tool = 'bench_unload_visual'
        phase = 1
        date = (Get-Date -Format 'yyyy-MM-ddTHH:mm:ssK')
        label = 'live-proven (one machine)'
        method = 'PrintWindow on the injected wallpaper surface (immune to the desktop composite)'
        playing_diff = [math]::Round($playingDiff, 3)
        unloaded_luma = $sigC.mean_luma
        unloaded_stddev = $sigC.stddev
        playing_luma = $sigA.mean_luma
        playing_stddev = $sigA.stddev
        resumed_diff = [math]::Round($resumedDiff, 3)
        animating_while_playing = ($playingDiff -gt 0.5)
        animating_after_resume = ($resumedDiff -gt 0.5)
        # The unloaded surface must still carry a picture: a black frame
        # would read as luma ~0 and stddev ~0.
        unloaded_shows_picture = ($sigC.mean_luma -gt 5.0 -and $sigC.stddev -gt 5.0)
    }
    $json = $report | ConvertTo-Json -Depth 6
    $outFull = [System.IO.Path]::GetFullPath($OutJson)
    $outDir = [System.IO.Path]::GetDirectoryName($outFull)
    if (-not (Test-Path -LiteralPath $outDir)) { New-Item -ItemType Directory -Force -Path $outDir | Out-Null }
    $json | Set-Content -LiteralPath $outFull -Encoding UTF8
    Write-Host "bench_unload_visual: wrote $outFull"
    $ok = $report.animating_while_playing -and $report.animating_after_resume -and $report.unloaded_shows_picture
    if (-not $ok) { Write-Error "the Unloaded state broke the wallpaper"; exit 1 }
    Write-Host "the Unloaded state keeps the picture and the resume restores the animation"
}
finally {
    Restore-DesktopWindows -Saved $saved
    try { if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue } } catch { }
}
