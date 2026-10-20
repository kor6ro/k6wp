param(
  [string]$Exe = "build\msvc-dev\studio.exe",
  [string]$Prefix = "C:\Users\axioo\Desktop\k6wp\shadow-evidence\combo"
)
Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;
using System.Runtime.InteropServices;
public class W33 {
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
  [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr h, IntPtr after, int x, int y, int cx, int cy, uint f);
  [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int cmd);
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [DllImport("user32.dll")] public static extern bool SetCursorPos(int x, int y);
  [DllImport("user32.dll")] public static extern void mouse_event(uint flags, int dx, int dy, uint data, IntPtr extra);
  [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr hdc, int flags);
  [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }
}
"@
$p = Start-Process -FilePath $Exe -PassThru
Start-Sleep -Seconds 8
$p.Refresh()
$h = $p.MainWindowHandle
[W33]::ShowWindow($h, 3) | Out-Null
[W33]::SetWindowPos($h, [IntPtr](-1), 0, 0, 0, 0, 0x0001 -bor 0x0002) | Out-Null
[W33]::SetForegroundWindow($h) | Out-Null
Start-Sleep -Seconds 2
$r = New-Object W33+RECT
[W33]::GetWindowRect($h, [ref]$r) | Out-Null
$w = $r.Right - $r.Left; $hh = $r.Bottom - $r.Top

function Shot([string]$name) {
  $bmp = New-Object System.Drawing.Bitmap $w, $hh
  $g = [System.Drawing.Graphics]::FromImage($bmp)
  $hdc = $g.GetHdc()
  [W33]::PrintWindow($h, $hdc, 2) | Out-Null
  $g.ReleaseHdc($hdc)
  $bmp.Save("$Prefix-$name.png", [System.Drawing.Imaging.ImageFormat]::Png)
  $g.Dispose(); $bmp.Dispose()
  Write-Output "shot $name"
}
function Click([int]$ix, [int]$iy, [int]$settleMs = 400) {
  $sx = $r.Left + $ix; $sy = $r.Top + $iy
  [W33]::SetCursorPos($sx, $sy) | Out-Null
  Start-Sleep -Milliseconds $settleMs
  [W33]::mouse_event(0x0002, 0, 0, 0, [IntPtr]::Zero)
  Start-Sleep -Milliseconds 90
  [W33]::mouse_event(0x0004, 0, 0, 0, [IntPtr]::Zero)
  Start-Sleep -Milliseconds 250
  Write-Output "clicked img($ix,$iy)"
}

# single, clean ComboBox click
Click 1060 478 500
Start-Sleep -Milliseconds 400
Shot "a-after400ms"
Start-Sleep -Milliseconds 1500
Shot "b-after2s"

if (!$p.HasExited) { Stop-Process -Id $p.Id -Force }
Write-Output "done"
