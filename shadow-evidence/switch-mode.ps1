# shadow-evidence/switch-mode.ps1 — put DISPLAY1 into a DIFFERENT mode so
# Windows sends a real WM_DISPLAYCHANGE to every window.
# Usage: powershell -File shadow-evidence\switch-mode.ps1 [apply|revert]
# Prints RESULT:APPLIED / RESULT:NO_ALT / RESULT:FAILED.
param([string]$Action = 'apply')
$ErrorActionPreference = 'Stop'

Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public class SW {
  [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
  public struct DEVMODE {
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 32)] public string dmDeviceName;
    public short dmSpecVersion, dmDriverVersion, dmSize, dmDriverExtra;
    public int dmFields;
    public int dmPositionX, dmPositionY, dmDisplayOrientation, dmDisplayFixedOutput;
    public short dmColor, dmDuplex, dmYResolution, dmTTOption, dmCollate;
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 32)] public string dmFormName;
    public short dmLogPixels;
    public int dmBitsPerPel, dmPelsWidth, dmPelsHeight, dmDisplayFlags, dmDisplayFrequency;
    public int dmICMMethod, dmICMIntent, dmMediaType, dmDitherType, dmReserved1, dmReserved2;
    public int dmPelsWidth2, dmPelsHeight2, dmDisplayOrientation2, dmDisplayFixedOutput2;
  }
  [DllImport("user32.dll", CharSet = CharSet.Unicode)]
  public static extern int EnumDisplaySettingsExW(string dev, int mode, ref DEVMODE dm, int flags);
  [DllImport("user32.dll", CharSet = CharSet.Unicode)]
  public static extern int ChangeDisplaySettingsExW(string dev, ref DEVMODE dm, IntPtr h, uint flags, IntPtr l);
}
'@

$dev = '\\.\DISPLAY1'
$cur = New-Object SW+DEVMODE
$cur.dmSize = [System.Runtime.InteropServices.Marshal]::SizeOf($cur)
$rc = [SW]::EnumDisplaySettingsExW($dev, -1, [ref]$cur, 0)
if ($rc -eq 0) { Write-Output 'RESULT:ENUM_FAIL'; exit 1 }
$curW = $cur.dmPelsWidth; $curH = $cur.dmPelsHeight; $curF = $cur.dmDisplayFrequency
Write-Output ("current: {0}x{1}@{2}" -f $curW, $curH, $curF)

if ($Action -eq 'revert') {
    $m = New-Object SW+DEVMODE
    $m.dmSize = [System.Runtime.InteropServices.Marshal]::SizeOf($m)
    $rc = [SW]::EnumDisplaySettingsExW($dev, -1, [ref]$m, 0)
    $m.dmFields = 0x00000000 -bor 0x00010000 -bor 0x00020000 -bor 0x00040000 -bor 0x00080000 -bor 0x00100000
    $m.dmPelsWidth = $curW; $m.dmPelsHeight = $curH; $m.dmDisplayFrequency = $curF
    $m.dmSize = [System.Runtime.InteropServices.Marshal]::SizeOf($m)
    $rc = [SW]::ChangeDisplaySettingsExW($dev, [ref]$m, [IntPtr]::Zero, 0x00000001, [IntPtr]::Zero)
    Write-Output ("RESULT:REVERT rc={0}" -f $rc)
    exit 0
}

# Find an alternative mode (same size is fine - a different refresh rate still
# sends WM_DISPLAYCHANGE).
$alt = $null
for ($i = 0; $i -lt 400; $i++) {
    $m = New-Object SW+DEVMODE
    $m.dmSize = [System.Runtime.InteropServices.Marshal]::SizeOf($m)
    if ([SW]::EnumDisplaySettingsExW($dev, $i, [ref]$m, 0) -eq 0) { break }
    if ($m.dmPelsWidth -ne $curW -or $m.dmPelsHeight -ne $curH -or $m.dmDisplayFrequency -ne $curF) {
        $alt = $m; break
    }
}
if (-not $alt) { Write-Output 'RESULT:NO_ALT'; exit 1 }
$alt.dmFields = 0x00000000 -bor 0x00010000 -bor 0x00020000 -bor 0x00040000 -bor 0x00080000 -bor 0x00100000
$alt.dmSize = [System.Runtime.InteropServices.Marshal]::SizeOf($m)
$rc = [SW]::ChangeDisplaySettingsExW($dev, [ref]$alt, [IntPtr]::Zero, 0x00000001, [IntPtr]::Zero)
Write-Output ("RESULT:APPLIED rc={0} {1}x{2}@{3}" -f $rc, $alt.dmPelsWidth, $alt.dmPelsHeight, $alt.dmDisplayFrequency)
