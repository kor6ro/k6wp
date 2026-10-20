# shadow-evidence/test-primary.ps1 — coba ubah primary display via ChangeDisplaySettingsExW
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
    public int ee, ff, gg;
  }
  public const int CDS_TEST = 2; public const int CDS_UPDATEREGISTRY = 1;
  public const int CDS_SET_PRIMARY = 0x00000010; public const int CDS_RESET = 0x40000000;
  public const int DM_PELSWIDTH = 0x00080000; public const int DM_PELSHEIGHT = 0x00100000;
  public const int DM_DISPLAYFREQUENCY = 0x00400000; public const int DM_BITSPERPEL = 0x00040000;
  public const int DM_POSITION = 0x00000020; public const int DM_DISPLAYORIENTATION = 0x00800000;
  public const int DISP_CHANGE_SUCCESSFUL = 0; public const int DISP_CHANGE_BADMODE = -2;
  [DllImport("user32.dll", CharSet = CharSet.Unicode)]
  public static extern int EnumDisplaySettingsExW(string dv, int mode, ref M m, int fl);
  [DllImport("user32.dll", CharSet = CharSet.Unicode)]
  public static extern int ChangeDisplaySettingsExW(string dv, ref M m, IntPtr h, int fl, IntPtr lp);
}
'@

function Get-Dev([string]$dev) {
  $m = New-Object D+M
  $m.c = [System.Runtime.InteropServices.Marshal]::SizeOf($m)
  $rc = [D]::EnumDisplaySettingsExW($dev, -1, [ref]$m, 0)
  if ($rc -eq 0) { Write-Output ("ENUM_FAIL dev={0} err={1}" -f $dev, [System.Runtime.InteropServices.Marshal]::GetLastWin32Error()); return $null }
  return $m
}

# 1. Enum both
$d1 = Get-Dev "\\.\DISPLAY1"
$d9 = Get-Dev "\\.\DISPLAY9"
if ($null -eq $d1 -or $null -eq $d9) { Write-Output "ENUM_GAGAL"; exit 1 }
Write-Output ("DISPLAY1: {0}x{1}@{2}Hz fields=0x{3:X} pos=({4},{5})" -f $d1.q,$d1.r,$d1.u,$d1.f,$d1.g,$d1.h)
Write-Output ("DISPLAY9: {0}x{1}@{2}Hz fields=0x{3:X} pos=({4},{5})" -f $d9.q,$d9.r,$d9.u,$d9.f,$d9.g,$d9.h)

# 2. Try CDS_TEST no-op on DISPLAY1
$m1 = $d1
$m1.c = [System.Runtime.InteropServices.Marshal]::SizeOf($m1)
$rc = [D]::ChangeDisplaySettingsExW("\\.\DISPLAY1", [ref]$m1, [IntPtr]::Zero, [D]::CDS_TEST, [IntPtr]::Zero)
Write-Output ("NOOP-DISPLAY1 CDS_TEST -> rc={0} err={1}" -f $rc, [System.Runtime.InteropServices.Marshal]::GetLastWin32Error())

# 3. Try CDS_SET_PRIMARY on DISPLAY9 (test only)
$m9 = $d9
$m9.c = [System.Runtime.InteropServices.Marshal]::SizeOf($m9)
$rc = [D]::ChangeDisplaySettingsExW("\\.\DISPLAY9", [ref]$m9, [IntPtr]::Zero, [D]::CDS_SET_PRIMARY -bor [D]::CDS_TEST, [IntPtr]::Zero)
Write-Output ("SET-PRIMARY-DISPLAY9 CDS_TEST -> rc={0} err={1}" -f $rc, [System.Runtime.InteropServices.Marshal]::GetLastWin32Error())

# 4. Try CDS_RESET on DISPLAY1 (test only)
$m1b = $d1
$m1b.c = [System.Runtime.InteropServices.Marshal]::SizeOf($m1b)
$rc = [D]::ChangeDisplaySettingsExW("\\.\DISPLAY1", [ref]$m1b, [IntPtr]::Zero, [D]::CDS_RESET -bor [D]::CDS_TEST, [IntPtr]::Zero)
Write-Output ("RESET-DISPLAY1 CDS_TEST -> rc={0} err={1}" -f $rc, [System.Runtime.InteropServices.Marshal]::GetLastWin32Error())
