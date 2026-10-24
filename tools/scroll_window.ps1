# scroll_window.ps1 - scroll a window with the wheel, for measuring and for
# capturing the far end of a long list.
#
# Usage: powershell -File tools/scroll_window.ps1 -ProcessId 1234 -Notches 20 [-Delay 30]
#
# The wheel is delivered as WM_MOUSEWHEEL after the pointer has been moved over
# the window's centre: Slint routes the wheel to the element under the pointer,
# so a message with the pointer elsewhere scrolls nothing.
param(
    [Parameter(Mandatory = $true)][int]$ProcessId,
    [Parameter(Mandatory = $true)][int]$Notches,
    [int]$Delay = 30
)

$ErrorActionPreference = "Stop"

Add-Type @"
using System;
using System.Runtime.InteropServices;

public static class WinScroll {
    [StructLayout(LayoutKind.Sequential)]
    public struct RECT { public int Left, Top, Right, Bottom; }

    [DllImport("user32.dll")] public static extern bool SetProcessDpiAwarenessContext(IntPtr value);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hWnd, out RECT rect);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hWnd);
    [DllImport("user32.dll")] public static extern bool SetCursorPos(int x, int y);
    [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr hWnd, uint msg, IntPtr wParam, IntPtr lParam);

    public static readonly IntPtr PER_MONITOR_AWARE_V2 = new IntPtr(-4);
    public const uint WM_MOUSEWHEEL = 0x020A;

    /// Packs the screen coordinates the way WM_MOUSEWHEEL expects them.
    public static IntPtr Point(int x, int y) {
        return new IntPtr((y << 16) | (x & 0xFFFF));
    }

    /// One wheel notch: 120 units, positive = away from the user = up.
    public static IntPtr Delta(int notches) {
        return new IntPtr((notches * 120) << 16);
    }
}
"@

[void][WinScroll]::SetProcessDpiAwarenessContext([WinScroll]::PER_MONITOR_AWARE_V2)

$proc = Get-Process -Id $ProcessId
$hwnd = $proc.MainWindowHandle
if ($hwnd -eq [IntPtr]::Zero) { throw "process $ProcessId has no main window yet" }
[void][WinScroll]::SetForegroundWindow($hwnd)
Start-Sleep -Milliseconds 250

$r = New-Object WinScroll+RECT
[void][WinScroll]::GetWindowRect($hwnd, [ref]$r)
$x = [int](($r.Left + $r.Right) / 2)
$y = [int](($r.Top + $r.Bottom) / 2)
[void][WinScroll]::SetCursorPos($x, $y)
Start-Sleep -Milliseconds 120

$step = if ($Notches -ge 0) { 1 } else { -1 }
for ($i = 0; $i -lt [math]::Abs($Notches); $i++) {
    [void][WinScroll]::PostMessage($hwnd, [WinScroll]::WM_MOUSEWHEEL, [WinScroll]::Delta(-$step), [WinScroll]::Point($x, $y))
    Start-Sleep -Milliseconds $Delay
}
Write-Host "sent $Notches notches at $x,$y"
