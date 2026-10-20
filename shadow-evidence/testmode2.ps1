# shadow-evidence/testmode2.ps1 — isolasi CDS_TEST: device NULL vs \\.\DISPLAY1,
# dan field lengkap vs minimal.
$ErrorActionPreference = 'Stop'
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public class D {
  [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
  public struct M {
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 32)] public string s;
    public short a, b, c, d; public int e, f, g, h, i;
    public short j, k, l, m, n;
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 32)] public string t;
    public short o; public int p, q, r, s2, u, v;
    public int w, x, y, z, aa, bb, cc, dd, ee, ff, gg, hh;
  }
  public const int CDS_TEST = 2;
  public const int DM_PELSWIDTH = 0x00080000;
  public const int DM_PELSHEIGHT = 0x00100000;
  public const int DM_DISPLAYFREQUENCY = 0x00400000;
  public const int DM_BITSPERPEL = 0x00040000;
  [DllImport("user32.dll", CharSet = CharSet.Unicode)]
  public static extern int EnumDisplaySettingsExW(string dv, int mode, ref M m, int fl);
  [DllImport("user32.dll", CharSet = CharSet.Unicode)]
  public static extern int ChangeDisplaySettingsExW(string dv, ref M m, IntPtr h, int fl, IntPtr lp);
}
'@

function Test-Dev([string]$dev, [D+M]$m, [string]$tag) {
  $m.e = [System.Runtime.InteropServices.Marshal]::SizeOf($m)
  $rc = [D]::ChangeDisplaySettingsExW($dev, [ref]$m, [IntPtr]::Zero, [D]::CDS_TEST, [IntPtr]::Zero)
  Write-Output ("{0} dev='{1}' -> TEST rc={2}" -f $tag, $dev, $rc)
}

$cur = New-Object D+M
$cur.e = [System.Runtime.InteropServices.Marshal]::SizeOf($cur)
$null = [D]::EnumDisplaySettingsExW([System.String]::Empty, -1, [ref]$cur, 0)
Write-Output ("current: {0}x{1}@{2}Hz fields=0x{3:X} size={4}" -f $cur.q, $cur.r, $cur.u, $cur.f, $cur.e)

# 1. exact stored, NULL device
$m1 = $cur   # struct copy (nilai)
Test-Dev $null $m1 "exact-NULL"
Test-Dev "\\.\DISPLAY1" $m1 "exact-DISPLAY1"

# 2. minimal fields (only PELS+FREQ+BITS), values = current
$m2 = $cur
$m2.f = [D]::DM_PELSWIDTH -bor [D]::DM_PELSHEIGHT -bor [D]::DM_DISPLAYFREQUENCY -bor [D]::DM_BITSPERPEL
Test-Dev $null $m2 "minimal-NULL"
Test-Dev "\\.\DISPLAY1" $m2 "minimal-DISPLAY1"

# 3. minimal fields + change to 1280x720@60
$m3 = $cur
$m2 = $cur
$m3.f = [D]::DM_PELSWIDTH -bor [D]::DM_PELSHEIGHT -bor [D]::DM_DISPLAYFREQUENCY -bor [D]::DM_BITSPERPEL
$m3.q = 1280; $m3.r = 720; $m3.u = 60
Test-Dev $null $m3 "minimal-1280x720@60-NULL"
