param(
  [string]$Exe = "build\msvc-dev\studio.exe",
  [string]$Prefix = "C:\Users\axioo\Desktop\k6wp\shadow-evidence\popup"
)
Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;
using System.Runtime.InteropServices;
public class W32 {
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
  [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr h, IntPtr after, int x, int y, int cx, int cy, uint f);
  [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int cmd);
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [DllImport("user32.dll")] public static extern bool SetCursorPos(int x, int y);
  [DllImport("user32.dll")] public static extern void mouse_event(uint flags, int dx, int dy, uint data, IntPtr extra);
  [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr hdc, uint flags);
  [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }
}
"@
Get-Process studio, K6WP, engine -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Seconds 1
$p = Start-Process -FilePath $Exe -PassThru
$h = [IntPtr]::Zero
for ($i = 0; $i -lt 30 -and $h -eq [IntPtr]::Zero; $i++) {
  Start-Sleep -Seconds 1
  $p.Refresh()
  if ($p.MainWindowHandle -ne 0) { $h = $p.MainWindowHandle }
}
if ($h -eq [IntPtr]::Zero) {
  Write-Output "NO WINDOW"
  if (!$p.HasExited) { Stop-Process -Id $p.Id -Force }
  exit 1
}
[W32]::ShowWindow($h, 3) | Out-Null
[W32]::SetWindowPos($h, [IntPtr](-1), 0, 0, 0, 0, 0x0001 -bor 0x0002) | Out-Null
[W32]::SetForegroundWindow($h) | Out-Null
Start-Sleep -Seconds 2
$r = New-Object W32+RECT
[W32]::GetWindowRect($h, [ref]$r) | Out-Null
$w = $r.Right - $r.Left; $hh = $r.Bottom - $r.Top
Write-Output "rect=$($r.Left),$($r.Top) size=${w}x${hh}"
if ($w -le 0 -or $hh -le 0) {
  Write-Output "BAD RECT"
  if (!$p.HasExited) { Stop-Process -Id $p.Id -Force }
  exit 1
}

function Shot([string]$name) {
  $bmp = New-Object System.Drawing.Bitmap $w, $hh
  $g = [System.Drawing.Graphics]::FromImage($bmp)
  $hdc = $g.GetHdc()
  [W32]::PrintWindow($h, $hdc, 2) | Out-Null
  $g.ReleaseHdc($hdc)
  $bmp.Save("$Prefix-$name.png", [System.Drawing.Imaging.ImageFormat]::Png)
  $g.Dispose(); $bmp.Dispose()
  Write-Output "shot $name"
}
function Click([int]$ix, [int]$iy) {
  $sx = $r.Left + $ix; $sy = $r.Top + $iy
  [W32]::SetCursorPos($sx, $sy) | Out-Null
  Start-Sleep -Milliseconds 400
  [W32]::mouse_event(0x0002, 0, 0, 0, [IntPtr]::Zero)
  Start-Sleep -Milliseconds 80
  [W32]::mouse_event(0x0004, 0, 0, 0, [IntPtr]::Zero)
  Start-Sleep -Milliseconds 1200
  Write-Output "clicked img($ix,$iy)"
}
function CropFrom([string]$file, [int]$x, [int]$y, [int]$cw, [int]$ch, [int]$scale, [string]$name) {
  $src = [System.Drawing.Image]::FromFile($file)
  $bmp = New-Object System.Drawing.Bitmap ($cw * $scale), ($ch * $scale)
  $g = [System.Drawing.Graphics]::FromImage($bmp)
  $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::NearestNeighbor
  $rect = New-Object System.Drawing.Rectangle 0, 0, ($cw * $scale), ($ch * $scale)
  $srcR = New-Object System.Drawing.Rectangle $x, $y, $cw, $ch
  $g.DrawImage($src, $rect, $srcR, [System.Drawing.GraphicsUnit]::Pixel)
  $bmp.Save("$Prefix-zoom-$name.png", [System.Drawing.Imaging.ImageFormat]::Png)
  $g.Dispose(); $bmp.Dispose(); $src.Dispose()
  Write-Output "zoom $name"
}

Shot "1-initial"

# Open the "Pasang ke" popup (left column, new layout position)
Click 190 554
Shot "2-assignpopup"
# Close it: click an empty area outside the popup
Click 700 645
Start-Sleep -Milliseconds 600

# Open the "Layar:" ComboBox dropdown (right rail, new position)
Click 1747 508
Shot "3-combopopup"
Click 700 700
Start-Sleep -Milliseconds 600

# crops from 1-initial (new layout coordinates)
CropFrom "$Prefix-1-initial.png" 1575 170 355 60 3 "buttons"
CropFrom "$Prefix-1-initial.png" 1575 135 355 55 3 "statusrow"
CropFrom "$Prefix-1-initial.png" 1580 478 340 52 3 "combo"
CropFrom "$Prefix-1-initial.png" 40 530 560 70 2 "pasangke-area"
# crop of the assign popup itself
CropFrom "$Prefix-2-assignpopup.png" 60 520 620 300 2 "assignpopup"

if (!$p.HasExited) { Stop-Process -Id $p.Id -Force }
Get-Process K6WP, engine -ErrorAction SilentlyContinue | Stop-Process -Force
Write-Output "done"
