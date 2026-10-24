# capture_window.ps1 - save one window to a PNG, without stray overlapping pixels.
#
# Usage: powershell -File tools/capture_window.ps1 -ProcessId 1234 -Out D:\shot.png [-Client]
#
# Two details the naive CopyFromScreen version got wrong:
#   * the host process is DPI-unaware, so GetWindowRect returned virtualized
#     pixels and the crop was offset by the scale factor (the neighbouring
#     window's text bled into the top and bottom of the image);
#   * a DWM-composed window is better asked to render itself.
# So: opt into per-monitor DPI awareness, ask the window to paint itself with
# PrintWindow(PW_RENDERFULLCONTENT), and fall back to a screen grab only when the
# window refuses (some GPU renderers answer with an empty bitmap).
param(
    [Parameter(Mandatory = $true)][int]$ProcessId,
    [Parameter(Mandatory = $true)][string]$Out,
    [switch]$Client
)

$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Drawing

Add-Type @"
using System;
using System.Drawing;
using System.Runtime.InteropServices;

public static class WinCap {
    [StructLayout(LayoutKind.Sequential)]
    public struct RECT { public int Left, Top, Right, Bottom; }

    [DllImport("user32.dll")] public static extern bool SetProcessDpiAwarenessContext(IntPtr value);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hWnd, out RECT rect);
    [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr hWnd, out RECT rect);
    [DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr hWnd, ref POINT point);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hWnd);
    [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr hWnd, IntPtr hdc, uint flags);

    [StructLayout(LayoutKind.Sequential)]
    public struct POINT { public int X, Y; }

    public const uint PW_RENDERFULLCONTENT = 2;
    public static readonly IntPtr PER_MONITOR_AWARE_V2 = new IntPtr(-4);

    /// The window rect, or the client rect in screen coordinates.
    public static RECT Bounds(IntPtr hWnd, bool client) {
        RECT r;
        if (!client) {
            GetWindowRect(hWnd, out r);
        } else {
            GetClientRect(hWnd, out r);
            POINT origin = new POINT { X = r.Left, Y = r.Top };
            ClientToScreen(hWnd, ref origin);
            r.Right += origin.X;
            r.Bottom += origin.Y;
            r.Left = origin.X;
            r.Top = origin.Y;
        }
        return r;
    }

}
"@

# DPI awareness first: every rect below is physical pixels.
[void][WinCap]::SetProcessDpiAwarenessContext([WinCap]::PER_MONITOR_AWARE_V2)

$proc = Get-Process -Id $ProcessId
$hwnd = $proc.MainWindowHandle
if ($hwnd -eq [IntPtr]::Zero) {
    throw "process $ProcessId has no main window yet"
}
[void][WinCap]::SetForegroundWindow($hwnd)
Start-Sleep -Milliseconds 600

$r = [WinCap]::Bounds($hwnd, [bool]$Client)
$w = $r.Right - $r.Left
$h = $r.Bottom - $r.Top
if ($w -le 0 -or $h -le 0) {
    throw "the window has an empty rect (${w}x${h})"
}

$bmp = New-Object System.Drawing.Bitmap $w, $h
$graphics = [System.Drawing.Graphics]::FromImage($bmp)

# A window that answers PrintWindow with a blank surface (some GPU renderers
# do) must not be shipped as an empty screenshot.
function Test-Content($image) {
    $first = $image.GetPixel(0, 0)
    for ($y = 0; $y -lt $image.Height; $y += 20) {
        for ($x = 0; $x -lt $image.Width; $x += 20) {
            if ($image.GetPixel($x, $y) -ne $first) { return $true }
        }
    }
    return $false
}

$hdc = $graphics.GetHdc()
$printed = $false
try {
    $printed = [WinCap]::PrintWindow($hwnd, $hdc, [WinCap]::PW_RENDERFULLCONTENT)
} finally {
    $graphics.ReleaseHdc($hdc)
}

if (-not $printed -or -not (Test-Content $bmp)) {
    Write-Host "PrintWindow gave no content; falling back to a screen grab"
    $graphics.CopyFromScreen($r.Left, $r.Top, 0, 0, (New-Object System.Drawing.Size $w, $h))
}

$bmp.Save($Out, [System.Drawing.Imaging.ImageFormat]::Png)
$graphics.Dispose()
$bmp.Dispose()
Write-Host "saved $Out ${w}x${h} (printwindow=$printed client=$([bool]$Client))"
