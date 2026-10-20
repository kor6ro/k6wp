param([string]$Exe = "build\msvc-dev\studio.exe", [string]$Out = "shadow-evidence\studio-shot.png")
Add-Type -AssemblyName System.Windows.Forms, System.Drawing
Add-Type @"
using System;
using System.Runtime.InteropServices;
public class Win32Rect {
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hWnd, out RECT rect);
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hWnd);
  [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr hWnd, int nCmdShow);
  [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }
}
"@
$p = Start-Process -FilePath $Exe -PassThru
Start-Sleep -Seconds 7
$p.Refresh()
$h = $p.MainWindowHandle
Write-Output "pid=$($p.Id) handle=$h exited=$($p.HasExited)"
if ($h -ne [IntPtr]::Zero) {
  [Win32Rect]::ShowWindow($h, 9) | Out-Null
  [Win32Rect]::SetForegroundWindow($h) | Out-Null
  Start-Sleep -Seconds 2
  $r = New-Object Win32Rect+RECT
  [Win32Rect]::GetWindowRect($h, [ref]$r) | Out-Null
  $w = $r.Right - $r.Left
  $hh = $r.Bottom - $r.Top
  Write-Output "rect=$($r.Left),$($r.Top),$($r.Right),$($r.Bottom) size=${w}x${hh}"
  if ($w -gt 0 -and $hh -gt 0) {
    $bmp = New-Object System.Drawing.Bitmap $w, $hh
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.CopyFromScreen($r.Left, $r.Top, 0, 0, (New-Object System.Drawing.Size $w, $hh))
    $bmp.Save($Out, [System.Drawing.Imaging.ImageFormat]::Png)
    $g.Dispose(); $bmp.Dispose()
    Write-Output "saved $Out"
  }
}
if (!$p.HasExited) { Stop-Process -Id $p.Id -Force }
