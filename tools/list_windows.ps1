# tools/list_windows.ps1 - enumerate visible top-level windows with their rects.
# Used to find what covers the desktop (the engine's occlusion/fullscreen
# watchers pause the decode when a window covers the monitor).
Add-Type @"
using System;using System.Runtime.InteropServices;using System.Text;
public class WEnum {
  [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc cb, IntPtr p);
  public delegate bool EnumProc(IntPtr h, IntPtr p);
  [DllImport("user32.dll")] public static extern int GetWindowText(IntPtr h, StringBuilder s, int n);
  [DllImport("user32.dll")] public static extern int GetClassName(IntPtr h, StringBuilder s, int n);
  [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr h);
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
  [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
  [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
  [DllImport("user32.dll")] public static extern int GetSystemMetrics(int i);
  [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left,Top,Right,Bottom; }
}
"@
$rows = New-Object System.Collections.Generic.List[object]
$cb = [WEnum+EnumProc]{
  param($h, $p)
  if (-not [WEnum]::IsWindowVisible($h)) { return $true }
  $sb = New-Object System.Text.StringBuilder 512
  [void][WEnum]::GetWindowText($h, $sb, 512)
  $title = $sb.ToString()
  $cb2 = New-Object System.Text.StringBuilder 256
  [void][WEnum]::GetClassName($h, $cb2, 256)
  $cls = $cb2.ToString()
  $r = New-Object WEnum+RECT
  [void][WEnum]::GetWindowRect($h, [ref]$r)
  $pid2 = 0
  [void][WEnum]::GetWindowThreadProcessId($h, [ref]$pid2)
  $w = $r.Right - $r.Left; $ht = $r.Bottom - $r.Top
  $proc = try { (Get-Process -Id $pid2 -ErrorAction Stop).ProcessName } catch { '?' }
  $rows.Add([pscustomobject]@{
    hwnd = ('0x{0:X}' -f $h.ToInt64())
    pid = $pid2
    proc = $proc
    class = $cls
    rect = "$($r.Left),$($r.Top) ${w}x${ht}"
    fg = ($h -eq [WEnum]::GetForegroundWindow())
    title = $(if ($title.Length -gt 60) { $title.Substring(0,60) } else { $title })
  })
  return $true
}
[void][WEnum]::EnumWindows($cb, [IntPtr]::Zero)
$sw = [WEnum]::GetSystemMetrics(0); $sh = [WEnum]::GetSystemMetrics(1)
"screen: ${sw}x${sh}"
$rows | Where-Object { $_.rect -notmatch '^0,0 0x0$' } | Sort-Object fg -Descending | Format-Table -AutoSize
