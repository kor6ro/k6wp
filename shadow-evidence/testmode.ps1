# shadow-evidence/testmode.ps1 — coba CDS_TEST pada beberapa mode, cetak hasil
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
    public int w, x, y, z, aa, bb, cc, dd;
  }
  public const int CDS_TEST = 2;
  public const int DM_PELSWIDTH = 0x00080000;
  public const int DM_PELSHEIGHT = 0x00100000;
  public const int DM_DISPLAYFREQUENCY = 0x00400000;
  public const int DM_BITSPERPEL = 0x00040000;
  public const int DM_DISPLAYORIENTATION = 0x00800000;
  [DllImport("user32.dll", CharSet = CharSet.Unicode)]
  public static extern int EnumDisplaySettingsExW(string dv, int mode, ref M m, int fl);
  [DllImport("user32.dll", CharSet = CharSet.Unicode)]
  public static extern int ChangeDisplaySettingsExW(string dv, ref M m, IntPtr h, int fl, IntPtr lp);
}
'@

function Test-One([int]$w, [int]$h, [int]$freq, [switch]$KeepFields) {
  $m = New-Object D+M
  $m.e = [System.Runtime.InteropServices.Marshal]::SizeOf($m)
  $null = [D]::EnumDisplaySettingsExW([System.String]::Empty, -1, [ref]$m, 0)
  if ($KeepFields) {
    $m.q = $w; $m.r = $h; $m.u = $freq;
    $m.e = $m.f = $m.f -bor [D]::DM_PELSWIDTH -bor [D]::DM_PELSHEIGHT -bor [D]::DM_DISPLAYFREQUENCY;
  } else {
    $m.p = 32; $m.q = $w; $m.r = $h; $m.u = $freq;
    $m.e = [D]::DM_PELSWIDTH -bor [D]::DM_PELSHEIGHT -bor [D]::DM_DISPLAYFREQUENCY -bor [D]::DM_BITSPERPEL;
  }
  $m.e = [System.Runtime.InteropServices.Marshal]::SizeOf($m)
  $rc = [D]::ChangeDisplaySettingsExW([System.String]::Empty, [ref]$m, [IntPtr]::Zero, [D]::CDS_TEST, [IntPtr]::Zero)
  Write-Output ("{0}x{1}@{2}Hz keepFields={3} -> TEST rc={4}" -f $w, $h, $freq, $KeepFields, $rc)
}

Test-One 1280 720 60
Test-One 1280 720 60 -KeepFields
Test-One 1366 768 60
Test-One 1366 768 60 -KeepFields
Test-One 1920 1080 60
Test-One 1920 1080 60 -KeepFields
Test-One 640 480 60
Test-One 640 480 60 -KeepFields
