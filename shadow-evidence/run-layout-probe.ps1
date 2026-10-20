param(
  [string]$QmlExe = "C:\Qt\6.8.3\msvc2022_64\bin\qml.exe",
  [string]$QmlFile = "C:\Users\axioo\Desktop\k6wp\shadow-evidence\layout-probe.qml",
  [string]$Out = "C:\Users\axioo\Desktop\k6wp\shadow-evidence"
)
Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;
using System.Runtime.InteropServices;
public class W35 {
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
  [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr h, IntPtr after, int x, int y, int cx, int cy, uint f);
  [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr hdc, int flags);
  [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc p, IntPtr l);
  [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
  [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr h);
  public delegate bool EnumProc(IntPtr h, IntPtr l);
  [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }
}
"@
$p = Start-Process -FilePath $QmlExe -ArgumentList $QmlFile -PassThru

$hwnd = [IntPtr]::Zero
for ($i = 0; $i -lt 30 -and $hwnd -eq [IntPtr]::Zero; $i++) {
  Start-Sleep -Seconds 1
  $p.Refresh()
  if ($p.MainWindowHandle -ne 0) { $hwnd = $p.MainWindowHandle; break }
  # fallback: enumerate windows of this pid
  $targetPid = $p.Id
  $script:found = [IntPtr]::Zero
  $cb = [W35+EnumProc] {
    param($h, $l)
    $wpid = 0
    [W35]::GetWindowThreadProcessId($h, [ref]$wpid) | Out-Null
    if ($wpid -eq $targetPid -and [W35]::IsWindowVisible($h)) { $script:found = $h; return $false }
    return $true
  }
  [W35]::EnumWindows($cb, [IntPtr]::Zero) | Out-Null
  if ($script:found -ne [IntPtr]::Zero) { $hwnd = $script:found; break }
}
if ($hwnd -eq [IntPtr]::Zero) { Write-Output "NO WINDOW FOUND"; if (!$p.HasExited) { Stop-Process -Id $p.Id -Force }; exit 1 }
Write-Output "hwnd=$hwnd after polling"

[W35]::SetWindowPos($hwnd, [IntPtr](-1), 100, 100, 0, 0, 0x0001 -bor 0x0002 -bor 0x0040) | Out-Null
Start-Sleep -Seconds 1
$r = New-Object W35+RECT
[W35]::GetWindowRect($hwnd, [ref]$r) | Out-Null
$w = $r.Right - $r.Left; $hh = $r.Bottom - $r.Top
Write-Output "rect=$($r.Left),$($r.Top) size=${w}x${hh}"
if ($w -le 0) { Write-Output "BAD RECT"; if (!$p.HasExited) { Stop-Process -Id $p.Id -Force }; exit 1 }

$bmp = New-Object System.Drawing.Bitmap $w, $hh
$g = [System.Drawing.Graphics]::FromImage($bmp)
$hdc = $g.GetHdc()
[W35]::PrintWindow($hwnd, $hdc, 2) | Out-Null
$g.ReleaseHdc($hdc)
$bmp.Save("$Out\layout-probe.png", [System.Drawing.Imaging.ImageFormat]::Png)
$g.Dispose(); $bmp.Dispose()
Write-Output "saved layout-probe.png"

if (!$p.HasExited) { Stop-Process -Id $p.Id -Force }
Write-Output "done"
