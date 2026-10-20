# shadow-evidence/set-display.ps1 — atur/kembalikan tampilan via ChangeDisplaySettingsExW
# Hanya layar primary (\.\DISPLAY1, NULL device). Tidak mengubah skaling (skaling
# per-tampilkan Windows tidak punya API runtime aman tanpa sign-out).
# Aksi:
#   save      - tangkap devmode sekarang ke $Store (JSON)
#   apply     - muat $Store, override width/height/freq/orientasi, CDS_TEST lalu
#               CDS_UPDATEREGISTRY. Cetak hasil tes + apply.
#   revert    - muat $Store, kembalikan devmode asli persis, CDS_TEST lalu apply.
#   check     - bandingkan devmode sekarang dengan $Store (cetak SAME/DIFF).
# Semua aksi cetak baris 'RESULT:<value>' agar caller dapat parse.
# Stock PowerShell + .NET only. Hanya baca/tulis file lokal (shadow-evidence).
[CmdletBinding()]
param(
  [Parameter(Mandatory=$true)][string]$Action,   # save|apply|revert|check
  [int]$Width = 0,
  [int]$Height = 0,
  [int]$Frequency = 0,
  [int]$Orientation = 0,
  [string]$Store = ''
)

if ([string]::IsNullOrEmpty($PSScriptRoot)) {
  $PSScriptRoot = Split-Path -Parent $PSCommandPath
}
if ([string]::IsNullOrEmpty($Store)) {
  $Store = "$PSScriptRoot\t2\display-state.json"
}

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
  public const int CDS_TEST = 0x00000002;
  public const int CDS_UPDATEREGISTRY = 0x00000001;
  public const int DISP_CHANGE_SUCCESSFUL = 0;
  public const int DM_PELSWIDTH = 0x00080000;
  public const int DM_PELSHEIGHT = 0x00100000;
  public const int DM_DISPLAYFREQUENCY = 0x00400000;
  public const int DM_DISPLAYORIENTATION = 0x00800000;
  public const int DM_BITSPERPEL = 0x00040000;
  [DllImport("user32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
  public static extern int EnumDisplaySettingsExW(string lpszDeviceName,
      int iModeNum, ref DEVMODE lpDevMode, int dwFlags);
  [DllImport("user32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
  public static extern int ChangeDisplaySettingsExW(string lpszDeviceName,
      ref DEVMODE lpDevMode, IntPtr hwnd, int dwflags, IntPtr lparam);
}
'@

function To-JsonFromDevmode([K6wpDisplay+DEVMODE]$d) {
  @{
    dmSize = $d.dmSize; dmFields = $d.dmFields;
    dmPositionX = $d.dmPositionX; dmPositionY = $d.dmPositionY;
    dmDisplayOrientation = $d.dmDisplayOrientation;
    dmDisplayFixedOutput = $d.dmDisplayFixedOutput;
    dmColor = $d.dmColor; dmDuplex = $d.dmDuplex; dmYResolution = $d.dmYResolution;
    dmTTOption = $d.dmTTOption; dmCollate = $d.dmCollate;
    dmDeviceName = $d.dmDeviceName; dmFormName = $d.dmFormName;
    dmLogPixels = $d.dmLogPixels; dmBitsPerPel = $d.dmBitsPerPel;
    dmPelsWidth = $d.dmPelsWidth; dmPelsHeight = $d.dmPelsHeight;
    dmDisplayFlags = $d.dmDisplayFlags; dmDisplayFrequency = $d.dmDisplayFrequency;
    dmICMMethod = $d.dmICMMethod; dmICMIntent = $d.dmICMIntent;
    dmMediaType = $d.dmMediaType; dmDitherType = $d.dmDitherType;
    dmPelsWidth2 = $d.dmPelsWidth2; dmPelsHeight2 = $d.dmPelsHeight2;
  } | ConvertTo-Json -Depth 5
}

function Devmode-From-Json([string]$path) {
  $j = Get-Content -LiteralPath $Path -Raw | ConvertFrom-Json
  $d = New-Object K6wpDisplay+DEVMODE
  $d.dmSize = [int][System.Runtime.InteropServices.Marshal]::SizeOf($d)
  # Set via reflection-field assignment below in caller; here fill known props.
  $fields = [K6wpDisplay+DEVMODE].GetFields()
  foreach ($f in $fields) {
    $name = $f.Name
    if ($j.PSObject.Properties.Name -contains $name) {
      $val = $j.$name
      if ($f.FieldType -eq [int16]) { $f.SetValue($d, [int16]$val) }
      elseif ($f.FieldType -eq [int]) { $f.SetValue($d, [int]$val) }
      elseif ($f.FieldType -eq [string]) { $f.SetValue($d, [string]$val) }
    }
  }
  $d.dmSize = [System.Runtime.InteropServices.Marshal]::SizeOf($d)
  return $d
}

switch ($Action) {
  'save' {
    $cur = New-Object K6wpDisplay+DEVMODE
    $cur.dmSize = [int][System.Runtime.InteropServices.Marshal]::SizeOf($cur)
    $rc = [K6wpDisplay]::EnumDisplaySettingsExW('\\.\DISPLAY1', [K6wpDisplay]::ENUM_CURRENT_SETTINGS, [ref]$cur, 0)
    if ($rc -eq 0) {
      $cur = New-Object K6wpDisplay+DEVMODE
      $cur.dmSize = [int][System.Runtime.InteropServices.Marshal]::SizeOf($cur)
      $rc = [K6wpDisplay]::EnumDisplaySettingsExW('\\.\DISPLAY1', [K6wpDisplay]::ENUM_CURRENT_SETTINGS, [ref]$cur, 0)
    }
    if ($rc -eq 0) { Write-Output ("RESULT:ENUM_FAIL err={0}" -f [System.Runtime.InteropServices.Marshal]::GetLastWin32Error()); exit 1 }
    $json = To-JsonFromDevmode $cur
    $dir = Split-Path -Parent $Store
    if (-not [string]::IsNullOrEmpty($dir) -and -not (Test-Path -LiteralPath $dir)) {
      New-Item -ItemType Directory -Path $dir -Force | Out-Null
    }
    Set-Content -LiteralPath $Store -Value $json -Encoding UTF8
    Write-Output ("RESULT:SAVED {0}x{1}@{2}Hz orient={3} fields=0x{4:X}" -f
      $cur.dmPelsWidth, $cur.dmPelsHeight, $cur.dmDisplayFrequency,
      $cur.dmDisplayOrientation, $cur.dmFields)
  }
  'apply' {
    if (-not (Test-Path -LiteralPath $Store)) { Write-Output 'RESULT:STORE_MISSING'; exit 1 }
    $d = Devmode-From-Json $Store
    if ($Width -gt 0) { $d.dmPelsWidth = $Width; $d.dmFields = $d.dmFields -bor [K6wpDisplay]::DM_PELSWIDTH }
    if ($Height -gt 0) { $d.dmPelsHeight = $Height; $d.dmFields = $d.dmFields -bor [K6wpDisplay]::DM_PELSHEIGHT }
    if ($Frequency -gt 0) { $d.dmDisplayFrequency = $Frequency; $d.dmFields = $d.dmFields -bor [K6wpDisplay]::DM_DISPLAYFREQUENCY }
    if ($Orientation -ge 0) {
      $d.dmDisplayOrientation = $Orientation
      $d.dmFields = $d.dmFields -bor [K6wpDisplay]::DM_DISPLAYORIENTATION
    }
    $d.dmSize = [int][System.Runtime.InteropServices.Marshal]::SizeOf($d)
    $rcTest = [K6wpDisplay]::ChangeDisplaySettingsExW('\\.\DISPLAY1', [ref]$d, [IntPtr]::Zero,
        [K6wpDisplay]::CDS_TEST, [IntPtr]::Zero)
    if ($rcTest -ne [K6wpDisplay]::DISP_CHANGE_SUCCESSFUL) {
      Write-Output ("RESULT:TEST_FAIL rc={0}" -f $rcTest); exit 1
    }
    $rc = [K6wpDisplay]::ChangeDisplaySettingsExW('\\.\DISPLAY1', [ref]$d, [IntPtr]::Zero,
        [K6wpDisplay]::CDS_UPDATEREGISTRY, [IntPtr]::Zero)
    Write-Output ("RESULT:APPLY_OK rc={0} {1}x{2}@{3}Hz orient={4}" -f
      $rc, $d.dmPelsWidth, $d.dmPelsHeight, $d.dmDisplayFrequency, $d.dmDisplayOrientation)
  }
  'revert' {
    if (-not (Test-Path -LiteralPath $Store)) { Write-Output 'RESULT:STORE_MISSING'; exit 1 }
    $d = Devmode-From-Json $Store
    # Kembalikan precis seperti disimpan (fields asli, tanpa override param).
    $d.dmFields = [int]((Get-Content -LiteralPath $Store -Raw | ConvertFrom-Json).dmFields)
    $d.dmSize = [int][System.Runtime.InteropServices.Marshal]::SizeOf($d)
    $rcTest = [K6wpDisplay]::ChangeDisplaySettingsExW('\\.\DISPLAY1', [ref]$d, [IntPtr]::Zero,
        [K6wpDisplay]::CDS_TEST, [IntPtr]::Zero)
    if ($rcTest -ne [K6wpDisplay]::DISP_CHANGE_SUCCESSFUL) {
      Write-Output ("RESULT:TEST_FAIL rc={0}" -f $rcTest); exit 1
    }
    $rc = [K6wpDisplay]::ChangeDisplaySettingsExW('\\.\DISPLAY1', [ref]$d, [IntPtr]::Zero,
        [K6wpDisplay]::CDS_UPDATEREGISTRY, [IntPtr]::Zero)
    Write-Output ("RESULT:REVERT_OK rc={0} {1}x{2}@{3}Hz orient={4}" -f
      $rc, $d.dmPelsWidth, $d.dmPelsHeight, $d.dmDisplayFrequency, $d.dmDisplayOrientation)
  }
  'check' {
    if (-not (Test-Path -LiteralPath $Store)) { Write-Output 'RESULT:STORE_MISSING'; exit 1 }
    $cur = New-Object K6wpDisplay+DEVMODE
    $cur.dmSize = [int][System.Runtime.InteropServices.Marshal]::SizeOf($cur)
    $rc = [K6wpDisplay]::EnumDisplaySettingsExW('\\.\DISPLAY1', [K6wpDisplay]::ENUM_CURRENT_SETTINGS, [ref]$cur, 0)
    if ($rc -eq 0) {
      $cur = New-Object K6wpDisplay+DEVMODE
      $cur.dmSize = [int][System.Runtime.InteropServices.Marshal]::SizeOf($cur)
      $rc = [K6wpDisplay]::EnumDisplaySettingsExW('\\.\DISPLAY1', [K6wpDisplay]::ENUM_CURRENT_SETTINGS, [ref]$cur, 0)
    }
    if ($rc -eq 0) { Write-Output ("RESULT:ENUM_FAIL err={0}" -f [System.Runtime.InteropServices.Marshal]::GetLastWin32Error()); exit 1 }
    $j = Get-Content -LiteralPath $Store -Raw | ConvertFrom-Json
    $same = ($cur.dmPelsWidth -eq [int]$j.dmPelsWidth) -and
            ($cur.dmPelsHeight -eq [int]$j.dmPelsHeight) -and
            ($cur.dmDisplayFrequency -eq [int]$j.dmDisplayFrequency) -and
            ($cur.dmDisplayOrientation -eq [int]$j.dmDisplayOrientation)
    if ($same) { Write-Output 'RESULT:SAME' } else {
      Write-Output ("RESULT:DIFF now={0}x{1}@{2}Hz orient={3} saved={4}x{5}@{6}Hz orient={7}" -f
        $cur.dmPelsWidth, $cur.dmPelsHeight, $cur.dmDisplayFrequency, $cur.dmDisplayOrientation,
        $j.dmPelsWidth, $j.dmPelsHeight, $j.dmDisplayFrequency, $j.dmDisplayOrientation)
    }
  }
  default { Write-Output 'RESULT:BAD_ACTION'; exit 1 }
}
