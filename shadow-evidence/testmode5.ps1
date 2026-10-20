# shadow-evidence/testmode5.ps1 — isolasi CDS: ChangeDisplaySettingsW vs Ex, minimal no-op
$ErrorActionPreference = 'Stop'
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public class D {
  [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
  public struct M {
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 32)] public string dmDeviceName;
    public short a, b, c, d; public int e, f, g, h, i;
    public short j, k, l, m, n;
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 32)] public string t;
    public short o; public int p, q, r, s2, u, v;
    public int w, x, y, z, aa, bb, cc, dd;
    public int ee, ff, gg, hh;
  }
  public const int CDS_TEST = 2;
  public const int DM_PELSWIDTH = 0x00080000; public const int DM_PELSHEIGHT = 0x00100000;
  public const int DM_DISPLAYFREQUENCY = 0x00400000; public const int DM_BITSPERPEL = 0x00040000;
  [DllImport("user32.dll", CharSet = CharSet.Unicode)]
  public static extern int EnumDisplaySettingsW(string dv, int mode, ref M m);
  [DllImport("user32.dll", CharSet = CharSet.Unicode)]
  public static extern int ChangeDisplaySettingsW(ref M m, int fl);
  [DllImport("user32.dll", CharSet = CharSet.Unicode)]
  public static extern int ChangeDisplaySettingsExW(string dv, ref M m, IntPtr h, int fl, IntPtr lp);
}
'@

function T([string]$tag, [string]$dev, [D+M]$mm, [switch]$UseW) {
  $mm.c = [System.Runtime.InteropServices.Marshal]::SizeOf($mm)
  $rc = if ($UseW) { [D]::ChangeDisplaySettingsW([ref]$mm, [D]::CDS_TEST) } else {
    [D]::ChangeDisplaySettingsExW($dev, [ref]$mm, [IntPtr]::Zero, [D]::CDS_TEST, [IntPtr]::Zero) }
  $err = [System.Runtime.InteropServices.Marshal]::GetLastWin32Error()
  Write-Output ("{0} dev='{1}' -> rc={2} err={3}" -f $tag, $dev, $rc, $err)
}

$cur = New-Object D+M
$cur.c = [System.Runtime.InteropServices.Marshal]::SizeOf($cur)
$rc = [D]::EnumDisplaySettingsW($null, -1, [ref]$cur)
Write-Output ("enum-W rc={0} {1}x{2}@{3}Hz fields=0x{4:X}" -f $rc, $cur.q, $cur.r, $cur.u, $cur.e)

$full = $cur
T "asli-full-Ex" "\\.\DISPLAY1" $full
T "asli-full-W" $null $full -UseW

$min = $cur
$min.e = [D]::DM_PELSWIDTH -bor [D]::DM_PELSHEIGHT
T "minimal no-op 1920x1080 Ex" "\\.\DISPLAY1" $min
T "minimal no-op 1920x1080 W" $null $min -UseW
