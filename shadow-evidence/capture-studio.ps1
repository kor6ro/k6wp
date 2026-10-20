param(
  [string]$Exe = "build\msvc-dev\studio.exe",
  [string]$OutPng = "C:\Users\axioo\Desktop\k6wp\shadow-evidence\rail-diag.png",
  [int]$WaitSec = 8
)
Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;
using System.Runtime.InteropServices;
public class W36 {
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
  [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr h, IntPtr after, int x, int y, int cx, int cy, uint f);
  [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr hdc, uint flags);
  [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }
}
"@
$env:QT_ASSUME_STDERR_HAS_CONSOLE = "1"
$p = Start-Process -FilePath $Exe -PassThru
$hwnd = [IntPtr]::Zero
for ($i = 0; $i -lt 25 -and $hwnd -eq [IntPtr]::Zero; $i++) {
  Start-Sleep -Seconds 1
  $p.Refresh()
  if ($p.MainWindowHandle -ne 0) { $hwnd = $p.MainWindowHandle }
}
if ($hwnd -eq [IntPtr]::Zero) { Write-Output "NO WINDOW"; if (!$p.HasExited) { Stop-Process -Id $p.Id -Force }; exit 1 }
[W36]::SetWindowPos($hwnd, [IntPtr](-1), -8, -8, 0, 0, 0x0001 -bor 0x0002 -bor 0x0040) | Out-Null
Start-Sleep -Seconds ($WaitSec)
$r = New-Object W36+RECT
[W36]::GetWindowRect($hwnd, [ref]$r) | Out-Null
$w = $r.Right - $r.Left; $hh = $r.Bottom - $r.Top
Write-Output "rect=$($r.Left),$($r.Top) size=${w}x${hh}"
if ($w -le 0) { if (!$p.HasExited) { Stop-Process -Id $p.Id -Force }; exit 1 }
$bmp = New-Object System.Drawing.Bitmap $w, $hh
$g = [System.Drawing.Graphics]::FromImage($bmp)
$hdc = $g.GetHdc()
[W36]::PrintWindow($hwnd, $hdc, 2) | Out-Null
$g.ReleaseHdc($hdc)
$bmp.Save($OutPng, [System.Drawing.Imaging.ImageFormat]::Png)
$g.Dispose(); $bmp.Dispose()
Write-Output "saved $OutPng"
if (!$p.HasExited) { Stop-Process -Id $p.Id -Force }
Get-Process K6WP, engine -ErrorAction SilentlyContinue | Stop-Process -Force
Write-Output "done"
