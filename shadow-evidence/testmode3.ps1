# shadow-evidence/testmode3.ps1 — uji CDS_TEST dengan struct 244 byte (mirip set-display)
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
  public const int DM_PELSWIDTH = 0x00080000; public const int DM_PELSHEIGHT = 0x00100000;
  public const int DM_DISPLAYFREQUENCY = 0x00400000; public const int DM_BITSPERPEL = 0x00040000;
  public const int DM_DISPLAYORIENTATION = 0x00800000;
  [DllImport("user32.dll", CharSet = CharSet.Unicode)]
  public static extern int EnumDisplaySettingsExW(string dv, int mode, ref M m, int fl);
  [DllImport("user32.dll", CharSet = CharSet.Unicode)]
  public static extern int ChangeDisplaySettingsExW(string dv, ref M m, IntPtr h, int fl, IntPtr lp);
}
'@

$m = New-Object D+M
$m.dmSize = [System.Runtime.InteropServices.Marshal]::SizeOf($m)
$rcEnum = [D]::EnumDisplaySettingsExW($null, -1, [ref]$m, 0)
$err1 = [System.Runtime.InteropServices.Marshal]::GetLastWin32Error()
Write-Output ("enum rc={0} err={1} size={2} {3}x{4}@{5}Hz fields=0x{6:X} dev='{7}'" -f
  $rcEnum, $err1, $m.dmSize, $m.dmPelsWidth, $m.dmPelsHeight, $m.dmDisplayFrequency, $m.dmFields, $m.dmDeviceName)
if ($rcEnum -eq 0) {
  $m.dmSize = [System.Runtime.InteropServices.Marshal]::SizeOf($m)
  $rcEnum2 = [D]::EnumDisplaySettingsExW("\\.\DISPLAY1", -1, [ref]$m, 0)
  $err2 = [System.Runtime.InteropServices.Marshal]::GetLastWin32Error()
  Write-Output ("enum-DISPLAY1 rc={0} err={1} size={2}" -f $rcEnum2, $err2, $m.dmSize)
}

if ($rcEnum -ne 0) {
  $c = $m
  function T([string]$tag, [string]$dev, [D+M]$mm) {
    $mm.dmSize = [System.Runtime.InteropServices.Marshal]::SizeOf($mm)
    $rc = [D]::ChangeDisplaySettingsExW($dev, [ref]$mm, [IntPtr]::Zero, [D]::CDS_TEST, [IntPtr]::Zero)
    $err = [System.Runtime.InteropServices.Marshal]::GetLastWin32Error()
    Write-Output ("{0} dev='{1}' -> rc={2} err={3}" -f $tag, $dev, $rc, $err)
  }
  T "asli" $null $c
  T "asli" "\\.\DISPLAY1" $c
  $c2 = $c; $c2.dmPelsWidth = 1280; $c2.dmPelsHeight = 720; $c2.dmDisplayFrequency = 60;
  $c2.dmFields = [D]::DM_PELSWIDTH -bor [D]::DM_PELSHEIGHT -bor [D]::DM_DISPLAYFREQUENCY -bor [D]::DM_BITSPERPEL
  T "1280x720@60 minimal" $null $c2
  $c3 = $c; $c3.dmPelsWidth = 1280; $c3.dmPelsHeight = 720; $c3.dmDisplayFrequency = 60;
  T "1280x720@60 full" $null $c3
}
