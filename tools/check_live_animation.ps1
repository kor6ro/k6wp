# tools/check_live_animation.ps1 - is the LIVE wallpaper actually moving?
#
# The user's still wallpaper (the video's first frame, applied via
# SPI_SETDESKWALLPAPER) looks IDENTICAL to the live video's first frame, so
# a desktop screenshot cannot tell them apart. This grabs the INJECTED
# wallpaper window itself (the engine's own surface) twice and compares: a
# moving surface means the live wallpaper is animating.
$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
. (Join-Path $here 'bench_lib.ps1')
Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;using System.Runtime.InteropServices;using System.Text;
public class WPA {
  [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc cb, IntPtr p);
  [DllImport("user32.dll")] public static extern bool EnumChildWindows(IntPtr parent, EnumProc cb, IntPtr p);
  public delegate bool EnumProc(IntPtr h, IntPtr p);
  [DllImport("user32.dll")] public static extern int GetClassName(IntPtr h, StringBuilder s, int n);
  [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr hdc, uint flags);
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
  [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left,Top,Right,Bottom; }
}
"@
$script:injected = New-Object System.Collections.Generic.List[object]
$childCb = [WPA+EnumProc]{
    param($h, $p)
    $sb = New-Object System.Text.StringBuilder 256
    [void][WPA]::GetClassName($h, $sb, 256)
    if ($sb.ToString() -eq 'K6WP.DesktopInject.1') { $script:injected.Add($h) }
    return $true
}
$topCb = [WPA+EnumProc]{
    param($h, $p)
    [void][WPA]::EnumChildWindows($h, $childCb, [IntPtr]::Zero)
    return $true
}
function Get-Injected {
    $script:injected.Clear()
    [void][WPA]::EnumWindows($topCb, [IntPtr]::Zero)
    if ($script:injected.Count -eq 0) { return [IntPtr]::Zero }
    return $script:injected[0]
}
function Save-Injected([IntPtr]$h, [string]$Path) {
    $r = New-Object WPA+RECT
    [void][WPA]::GetWindowRect($h, [ref]$r)
    $bmp = New-Object System.Drawing.Bitmap ($r.Right - $r.Left), ($r.Bottom - $r.Top)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $hdc = $g.GetHdc()
    try { [void][WPA]::PrintWindow($h, $hdc, 2) } finally { $g.ReleaseHdc($hdc); $g.Dispose() }
    $bmp.Save($Path, [System.Drawing.Imaging.ImageFormat]::Png)
    $bmp.Dispose()
}

$h = Get-Injected
if ($h -eq [IntPtr]::Zero) { throw "no injected wallpaper window found (is the engine running?)" }
$a = Join-Path $env:TEMP ("k6wp-anim-a-" + [Guid]::NewGuid().ToString('N').Substring(0, 6) + ".png")
$b = Join-Path $env:TEMP ("k6wp-anim-b-" + [Guid]::NewGuid().ToString('N').Substring(0, 6) + ".png")
Save-Injected $h $a
Start-Sleep -Milliseconds 1200
Save-Injected $h $b
$ba = [System.Drawing.Bitmap]::FromFile($a)
$bb = [System.Drawing.Bitmap]::FromFile($b)
try { $diff = Get-BitmapDiff -A $ba -B $bb -Step 6 } finally { $ba.Dispose(); $bb.Dispose() }
Remove-Item $a, $b -Force -ErrorAction SilentlyContinue
Write-Host ("injected-wallpaper frame-to-frame diff: {0}" -f [math]::Round($diff, 3))
if ($diff -le 0.5) {
    Write-Error "the live wallpaper is NOT animating (the injected surface does not change)"
    exit 1
}
Write-Host "the live wallpaper IS animating"
exit 0
