param([string]$Exe = "build\msvc-dev\studio.exe", [string]$Out = "C:\Users\axioo\Desktop\k6wp\shadow-evidence\studio-fixed.png", [string]$Err = "shadow-evidence\studio-err3.txt")
$p = Start-Process -FilePath $Exe -RedirectStandardError $Err -RedirectStandardOutput "shadow-evidence\studio-out3.txt" -PassThru
Start-Sleep -Seconds 7
Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;
using System.Runtime.InteropServices;
public class Win32Cap2 {
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hWnd, out RECT rect);
  [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr hWnd, IntPtr hdcBlt, int nFlags);
  [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }
}
"@
$p.Refresh()
$h = $p.MainWindowHandle
$r = New-Object Win32Cap2+RECT
[Win32Cap2]::GetWindowRect($h, [ref]$r) | Out-Null
$w = $r.Right - $r.Left
$hh = $r.Bottom - $r.Top
$bmp = New-Object System.Drawing.Bitmap $w, $hh
$g = [System.Drawing.Graphics]::FromImage($bmp)
$hdc = $g.GetHdc()
[Win32Cap2]::PrintWindow($h, $hdc, 2) | Out-Null
$g.ReleaseHdc($hdc)
$bmp.Save($Out, [System.Drawing.Imaging.ImageFormat]::Png)
$g.Dispose(); $bmp.Dispose()
if (!$p.HasExited) { Stop-Process -Id $p.Id -Force }
Write-Output "--- stderr (QML errors) ---"
Get-Content $Err | Select-Object -First 30
Write-Output "--- done ---"
