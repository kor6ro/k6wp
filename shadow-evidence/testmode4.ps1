# shadow-evidence/testmode4.ps1 — CDS_TEST 1280x720@60 dengan device \\.\DISPLAY1
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

function T([string]$tag, [string]$dev, [D+M]$mm) {
  $mm.dmSize = [System.Runtime.InteropServices.Marshal]::SizeOf($mm)
  $rc = [D]::ChangeDisplaySettingsExW($dev, [ref]$mm, [IntPtr]::Zero, [D]::CDS_TEST, [IntPtr]::Zero)
  $err = [System.Runtime.InteropServices.Marshal]::GetLastWin32Error()
  Write-Output ("{0} dev='{1}' -> rc={2} err={3}" -f $tag, $dev, $rc, $err)
}

$cur = New-Object D+M
$cur.dmSize = [System.Runtime.InteropServices.Marshal]::SizeOf($cur)
$rc = [D]::EnumDisplaySettingsExW('\\.\DISPLAY1', -1, [ref]$cur, 0)
Write-Output ("enum rc={0} {1}x{2}@{3}Hz fields=0x{4:X}" -f $rc, $cur.dmPelsWidth, $cur.dmPelsHeight, $cur.dmDisplayFrequency, $cur.dmFields)

$full = $cur
T "asli-full" "\\.\DISPLAY1" $full

$min = $cur
$min.dmFields = [D]::DM_PELSWIDTH -bor [D]::DM_PELSHEIGHT -bor [D]::DM_DISPLAYFREQUENCY -bor [D]::DM_BITSPERPEL
$min.dmPelsWidth = 1280; $min.dmPelsHeight = 720; $min.dmDisplayFrequency = 60; $min.dmBitsPerPel = 32
T "1280x720@60 minimal" "\\.\DISPLAY1" $min

$min2 = $cur
$min2.dmFields = [D]::DM_PELSWIDTH -bor [D]::DM_PELSHEIGHT -bor [D]::DM_BITSPERPEL
$min2.dmPelsWidth = 1280; $min2.dmPelsHeight = 720; $min2.dmBitsPerPel = 32
T "1280x720 (no freq flag)" "\\.\DISPLAY1" $min2
