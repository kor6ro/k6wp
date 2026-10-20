# shadow-evidence/enum-modes.ps1 — daftar mode tampilan yang diexpose driver
# (\\.\DISPLAY1), via EnumDisplaySettingsExW. Stock PowerShell + .NET only.
$ErrorActionPreference = 'Stop'
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public class K6wpDisplay {
  [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
  public struct DEVMODE {
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
  public const int ENUM_CURRENT_SETTINGS = -1;
  public const int DISP_CHANGE_SUCCESSFUL = 0;
  [DllImport("user32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
  public static extern int EnumDisplaySettingsExW(string lpszDeviceName,
      int iModeNum, ref DEVMODE lpDevMode, int dwFlags);
}
'@ -ErrorAction Stop

$d = New-Object K6wpDisplay+DEVMODE
$i = 0
$max = 120
while ($i -lt $max) {
  $d.dmSize = [int][System.Runtime.InteropServices.Marshal]::SizeOf($d)
  $rc = [K6wpDisplay]::EnumDisplaySettingsExW('\\.\DISPLAY1', $i, [ref]$d, 0)
  if ($rc -eq 0) { break }
  Write-Output ("{0,3}: {1}x{2}@{3}Hz bits={4} orient={5} flags=0x{6:X8}" -f
    $i, $d.dmPelsWidth, $d.dmPelsHeight, $d.dmDisplayFrequency, $d.dmBitsPerPel,
    $d.dmDisplayOrientation, $d.dmDisplayFlags)
  $i++
}
Write-Output ("total enumerated: {0}" -f $i)
