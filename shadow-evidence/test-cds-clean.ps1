# shadow-evidence/test-cds-clean.ps1 — uji ChangeDisplaySettingsExW dengan struct benar
$ErrorActionPreference = 'Stop'
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public class D {
  [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
  public struct M {
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 32)] public string dmDeviceName;
    public short dmSpecVersion; public short dmDriverVersion; public short dmSize;
    public short dmDriverExtra; public int dmFields; public int dmPositionX;
    public int dmPositionY; public int dmDisplayOrientation;
    public int dmDisplayFixedOutput; public short dmColor; public short dmDuplex;
    public short dmYResolution; public short dmTTOption; public short dmCollate;
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 32)] public string dmFormName;
    public short dmLogPixels; public int dmBitsPerPel; public int dmPelsWidth;
    public int dmPelsHeight; public int dmDisplayFlags; public int dmDisplayFrequency;
    public int dmICMMethod; public int dmICMIntent; public int dmMediaType;
    public int dmDitherType; public int dmReserved1; public int dmReserved2;
    public int dmPelsWidth2; public int dmPelsHeight2;
    public int dmReserved3; public int dmReserved4; public int dmReserved5;
    public int dmReserved6; public int dmReserved7; public int dmReserved8;
  }
  public const int CDS_TEST = 2;
  public const int CDS_UPDATEREGISTRY = 1;
  public const int CDS_SET_PRIMARY = 0x00000010;
  public const int DISP_CHANGE_SUCCESSFUL = 0;
  [DllImport("user32.dll", CharSet = CharSet.Unicode)]
  public static extern int EnumDisplaySettingsExW(string dv, int mode, ref M m, int fl);
  [DllImport("user32.dll", CharSet = CharSet.Unicode)]
  public static extern int ChangeDisplaySettingsExW(string dv, ref M m, IntPtr h, int fl, IntPtr lp);
}
'@

function Test-Dev([string]$dev, [int]$flags, [string]$tag) {
  $m = New-Object D+M
  $m.dmSize = [System.Runtime.InteropServices.Marshal]::SizeOf($m)
  $rc = [D]::EnumDisplaySettingsExW($dev, -1, [ref]$m, 0)
  if ($rc -eq 0) { Write-Output ("{0}: ENUM_FAIL dev={1}" -f $tag, $dev); return }
  $m.dmSize = [System.Runtime.InteropServices.Marshal]::SizeOf($m)
  $rc2 = [D]::ChangeDisplaySettingsExW($dev, [ref]$m, [IntPtr]::Zero, $flags, [IntPtr]::Zero)
  $err = [System.Runtime.InteropServices.Marshal]::GetLastWin32Error()
  Write-Output ("{0}: dev={1} {2}x{3}@{4}Hz fields=0x{5:X} flags=0x{6:X} -> rc={7} err={8}" -f
    $tag, $dev, $m.dmPelsWidth, $m.dmPelsHeight, $m.dmDisplayFrequency, $m.dmFields, $flags, $rc2, $err)
}

Test-Dev "\\.\DISPLAY1" 2 "NOOP-D1"
Test-Dev "\\.\DISPLAY9" 2 "NOOP-D9"
Test-Dev "\\.\DISPLAY9" 18 "SET-PRIMARY-D9"
