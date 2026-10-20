# tools/bench_wallpaper_frames.ps1 - capture the WALLPAPER the user sees.
#
# Grabs the injected wallpaper window (class K6WP.DesktopInject.1) with
# PrintWindow. Unlike a desktop screen grab, this captures what the engine
# actually PRESENTED into the wallpaper surface, so it is immune to an
# always-on-top overlay covering the desktop.
#
# Two questions this answers:
#   1. ANIMATION: the frames must DIFFER from each other (the wallpaper
#      moves). A frozen sequence means the VO stopped presenting.
#   2. PARITY: the frames of two builds must show the same picture.
#
# Usage:
#   powershell -NoProfile -ExecutionPolicy Bypass -File tools\bench_wallpaper_frames.ps1 `
#     -EngineExe build\ab\engine-candidate.exe -OutDir build\ab\wpframes -Frames 3

[CmdletBinding()]
param(
    [string]$EngineExe = '',
    [string]$OutDir = '',
    [int]$Frames = 3,
    [int]$IntervalMs = 1500,
    [int]$SettleSec = 6
)
$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
. (Join-Path $here 'bench_lib.ps1')

if ([string]::IsNullOrWhiteSpace($EngineExe)) { $EngineExe = Join-Path $here '..\build\release\engine.exe' }
if ([string]::IsNullOrWhiteSpace($OutDir)) { $OutDir = Join-Path $here '..\build\wpframes' }
$engine = [System.IO.Path]::GetFullPath($EngineExe)
$video = Join-Path $here '..\tests\corpus\anime.mp4'
$dataDir = Join-Path $env:TEMP ("k6wp-wp-" + [Guid]::NewGuid().ToString('N').Substring(0, 8))
New-Item -ItemType Directory -Force -Path (Join-Path $dataDir 'K6WP') | Out-Null
$outDir = [System.IO.Path]::GetFullPath($OutDir)
Remove-Item -LiteralPath $outDir -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $outDir | Out-Null

Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;using System.Runtime.InteropServices;using System.Text;
public class WPF {
  [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc cb, IntPtr p);
  [DllImport("user32.dll")] public static extern bool EnumChildWindows(IntPtr parent, EnumProc cb, IntPtr p);
  public delegate bool EnumProc(IntPtr h, IntPtr p);
  [DllImport("user32.dll")] public static extern int GetClassName(IntPtr h, StringBuilder s, int n);
  [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr hdc, uint flags);
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
  [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left,Top,Right,Bottom; }
}
"@
# The injected surface is a CHILD of the shell's WorkerW (or of Progman), so
# it is not a top-level window. The enumeration lives at SCRIPT scope: a
# scriptblock-to-delegate conversion inside a function loses the callback's
# captured scope and the delegate comes back null.
$script:injected = New-Object System.Collections.Generic.List[object]
$childCb = [WPF+EnumProc]{
    param($h, $p)
    $sb = New-Object System.Text.StringBuilder 256
    [void][WPF]::GetClassName($h, $sb, 256)
    if ($sb.ToString() -eq 'K6WP.DesktopInject.1') { $script:injected.Add($h) }
    return $true
}
$topCb = [WPF+EnumProc]{
    param($h, $p)
    [void][WPF]::EnumChildWindows($h, $childCb, [IntPtr]::Zero)
    return $true
}
function Get-InjectedWindow {
    $script:injected.Clear()
    [void][WPF]::EnumWindows($topCb, [IntPtr]::Zero)
    if ($script:injected.Count -eq 0) { return [IntPtr]::Zero }
    return $script:injected[0]
}

function Save-Injected([IntPtr]$h, [string]$Path) {
    $r = New-Object WPF+RECT
    [void][WPF]::GetWindowRect($h, [ref]$r)
    $w = $r.Right - $r.Left; $ht = $r.Bottom - $r.Top
    if ($w -le 0 -or $ht -le 0) { throw "the injected window has no area" }
    $bmp = New-Object System.Drawing.Bitmap $w, $ht
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $hdc = $g.GetHdc()
    try { $ok = [WPF]::PrintWindow($h, $hdc, 2) }
    finally { $g.ReleaseHdc($hdc); $g.Dispose() }
    if (-not $ok) { $bmp.Dispose(); throw "PrintWindow failed" }
    $bmp.Save($Path, [System.Drawing.Imaging.ImageFormat]::Png)
    $bmp.Dispose()
}

$busy = @(Get-Process engine -ErrorAction SilentlyContinue)
if ($busy.Count -gt 0) { throw "engine already running (pid $($busy.Id -join ','))" }
$old = $env:LOCALAPPDATA
try {
    $env:LOCALAPPDATA = $dataDir
    $p = Start-Process -FilePath $engine -ArgumentList @("--video=$video", '--exit-after-ms=120000') -PassThru -WindowStyle Hidden
}
finally { $env:LOCALAPPDATA = $old }
if (-not (Wait-K6wpEngine -TimeoutMs 15000)) { throw "engine did not answer the pipe" }

$saved = @()
try {
    # Minimize the covering windows so the engine is not occlusion-paused
    # (a paused engine holds a still frame - the wallpaper would not move).
    $saved = Get-DesktopWindows
    Start-Sleep -Seconds $SettleSec
    $h = Get-InjectedWindow
    if ($h -eq [IntPtr]::Zero) { throw "no injected wallpaper window found" }
    Write-Host ("bench_wallpaper_frames: injected window 0x{0:X}" -f $h.ToInt64())

    $paths = @()
    for ($i = 1; $i -le $Frames; $i++) {
        $path = Join-Path $outDir ("frame{0}.png" -f $i)
        Save-Injected -h $h -Path $path
        $paths += $path
        Write-Host ("  captured frame {0} -> {1}" -f $i, (Split-Path -Leaf $path))
        if ($i -lt $Frames) { Start-Sleep -Milliseconds $IntervalMs }
    }
}
finally {
    Restore-DesktopWindows -Saved $saved
    try { if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue } } catch { }
}

# The frames must differ from each other: a still sequence means the VO
# stopped presenting and the wallpaper is frozen.
$bmps = @($paths | ForEach-Object { [System.Drawing.Bitmap]::FromFile($_) })
$diffs = @()
for ($i = 1; $i -lt $bmps.Count; $i++) {
    $d = Get-BitmapDiff -A $bmps[$i - 1] -B $bmps[$i] -Step 6
    $diffs += [math]::Round($d, 3)
}
foreach ($b in $bmps) { $b.Dispose() }
Write-Host ""
Write-Host ("frame-to-frame diffs: {0}" -f ($diffs -join ', '))
$animating = @($diffs | Where-Object { $_ -gt 0.5 }).Count -ge 1
if (-not $animating) {
    Write-Error "bench_wallpaper_frames: the wallpaper did NOT animate (all frames identical) - the VO stopped presenting"
    exit 1
}
Write-Host "the wallpaper animates (the frames differ)"
exit 0
