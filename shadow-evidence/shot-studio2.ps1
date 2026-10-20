param([string]$Exe = "build\msvc-dev\studio.exe", [string]$Out = "shadow-evidence\studio-shot.png")
Add-Type -AssemblyName System.Windows.Forms, System.Drawing
Add-Type @"
using System;
using System.Runtime.InteropServices;
public class Win32Cap {
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hWnd, out RECT rect);
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hWnd);
  [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr hWnd, int nCmdShow);
  [DllImport("user32.dll")] public static extern void keybd_event(byte bVk, byte bScan, int dwFlags, int dwExtraInfo);
  [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr hWnd, IntPtr hdcBlt, int nFlags);
  [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }
}
"@
$p = Start-Process -FilePath $Exe -PassThru
Start-Sleep -Seconds 7
$p.Refresh()
$h = $p.MainWindowHandle
Write-Output "pid=$($p.Id) handle=$h exited=$($p.HasExited)"
if ($h -eq [IntPtr]::Zero) { if (!$p.HasExited) { Stop-Process -Id $p.Id -Force }; exit 1 }

[Win32Cap]::ShowWindow($h, 9) | Out-Null
[Win32Cap]::keybd_event(0x12, 0, 0, 0)
[Win32Cap]::SetForegroundWindow($h) | Out-Null
[Win32Cap]::keybd_event(0x12, 0, 2, 0)
Start-Sleep -Seconds 2

$r = New-Object Win32Cap+RECT
[Win32Cap]::GetWindowRect($h, [ref]$r) | Out-Null
$w = $r.Right - $r.Left
$hh = $r.Bottom - $r.Top
Write-Output "rect=$($r.Left),$($r.Top),$($r.Right),$($r.Bottom) size=${w}x${hh}"

if ($w -gt 0 -and $hh -gt 0) {
  # Try PrintWindow (works even when obscured, incl. DWM-composited windows)
  $bmp = New-Object System.Drawing.Bitmap $w, $hh
  $g = [System.Drawing.Graphics]::FromImage($bmp)
  $hdc = $g.GetHdc()
  $ok = [Win32Cap]::PrintWindow($h, $hdc, 2)
  $g.ReleaseHdc($hdc)
  Write-Output "PrintWindow=$ok"
  $bmp.Save($Out, [System.Drawing.Imaging.ImageFormat]::Png)
  $g.Dispose(); $bmp.Dispose()
  Write-Output "saved(PrintWindow) $Out"
}
if (!$p.HasExited) { Stop-Process -Id $p.Id -Force }
