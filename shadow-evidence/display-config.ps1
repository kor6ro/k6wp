# shadow-evidence/display-config.ps1 — SetDisplayConfig wrapper for multi-monitor ops
# Implements: Get-DisplayConfig, Set-PrimaryDisplay, Swap-DisplayPosition,
#             Disable-Display, Enable-Display, Apply-DisplayConfig
# Uses user32.dll GetDisplayConfigBufferSizes / QueryDisplayConfig / SetDisplayConfig
# Based on MartinGC94/DisplayConfig structs.
$ErrorActionPreference = 'Stop'

Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;

public class DC {
  [StructLayout(LayoutKind.Sequential)]
  public struct LUID { public uint LowPart; public int HighPart; }
  [StructLayout(LayoutKind.Sequential)]
  public struct POINTL { public int x; public int y; }
  [StructLayout(LayoutKind.Sequential)]
  public struct DISPLAYCONFIG_SOURCE_MODE {
    public uint width; public uint height; public uint pixelFormat; public POINTL position;
  }
  [StructLayout(LayoutKind.Explicit)]
  public struct ModeInfoUnion {
    [FieldOffset(0)] public DISPLAYCONFIG_SOURCE_MODE sourceMode;
    [FieldOffset(60)] public byte _pad;
  }
  [StructLayout(LayoutKind.Sequential)]
  public struct DISPLAYCONFIG_MODE_INFO {
    public uint infoType;
    public uint id;
    public LUID adapterId;
    public ModeInfoUnion modeInfo;
  }
  [StructLayout(LayoutKind.Sequential)]
  public struct DISPLAYCONFIG_PATH_SOURCE_INFO {
    public LUID adapterId;
    public uint id;
    public uint modeInfoIdx;
    public uint statusFlags;
  }
  [StructLayout(LayoutKind.Sequential)]
  public struct DISPLAYCONFIG_PATH_TARGET_INFO {
    public LUID adapterId;
    public uint id;
    public uint modeInfoIdx;
    public uint outputTechnology;
    public uint rotation;
    public uint scaling;
    public uint refreshRateNum; public uint refreshRateDen;
    public uint scanlineOrdering;
    [MarshalAs(UnmanagedType.U1)] public bool targetAvailable;
    public byte _pad1; public byte _pad2; public byte _pad3;
    public uint statusFlags;
  }
  [StructLayout(LayoutKind.Sequential)]
  public struct DISPLAYCONFIG_PATH_INFO {
    public DISPLAYCONFIG_PATH_SOURCE_INFO sourceInfo;
    public DISPLAYCONFIG_PATH_TARGET_INFO targetInfo;
    public uint flags;
  }
  public const uint QDC_ALL_PATHS = 0x1;
  public const uint SDC_APPLY = 0x80;
  public const uint PATH_ACTIVE = 0x1;
  public const uint MODE_IDX_INVALID = 0xFFFFFFFF;
  public const uint SOURCE_MODE_IDX_INVALID = 0xFFFF;
  public const uint MODE_INFO_TYPE_SOURCE = 1;
  public const uint MODE_INFO_TYPE_TARGET = 2;
  [DllImport("user32.dll", ExactSpelling = true, CharSet = CharSet.Unicode)]
  public static extern int GetDisplayConfigBufferSizes(uint flags, out uint numPathArrayElements, out uint numModeInfoArrayElements);
  [DllImport("user32.dll", ExactSpelling = true, CharSet = CharSet.Unicode)]
  public static extern int QueryDisplayConfig(
    uint flags, ref uint numPathArrayElements,
    [In, Out] DISPLAYCONFIG_PATH_INFO[] pathArray,
    ref uint numModeInfoArrayElements,
    [In, Out] DISPLAYCONFIG_MODE_INFO[] modeInfoArray,
    IntPtr currentTopologyId);
  [DllImport("user32.dll", ExactSpelling = true, CharSet = CharSet.Unicode)]
  public static extern int SetDisplayConfig(
    uint numPathArrayElements, [In] DISPLAYCONFIG_PATH_INFO[] pathArray,
    uint numModeInfoArrayElements, [In] DISPLAYCONFIG_MODE_INFO[] modeInfoArray,
    uint flags);
}
'@ -ErrorAction Stop

function Get-SourceModeIdx([uint32]$modeInfoIdx) { return ($modeInfoIdx -band 0xFFFF0000) -shr 16 }
function Set-SourceModeIdx([ref][uint32]$modeInfoIdx, [uint32]$v) { $modeInfoIdx.Value = ($modeInfoIdx.Value -band 0x0000FFFF) -bor ($v -shl 16) }
function Get-CloneGroup([uint32]$modeInfoIdx) { return $modeInfoIdx -band 0x0000FFFF }

function Get-DisplayConfig {
  $np = [uint32]0; $nm = [uint32]0
  $rc = [DC]::GetDisplayConfigBufferSizes([DC]::QDC_ALL_PATHS, [ref]$np, [ref]$nm)
  if ($rc -ne 0) { throw "GetDisplayConfigBufferSizes failed rc=$rc" }
  $paths = New-Object 'DC+DISPLAYCONFIG_PATH_INFO[]'($np)
  $modes = New-Object 'DC+DISPLAYCONFIG_MODE_INFO[]'($nm)
  $rc = [DC]::QueryDisplayConfig([DC]::QDC_ALL_PATHS, [ref]$np, $paths, [ref]$nm, $modes, [IntPtr]::Zero)
  if ($rc -ne 0) { throw "QueryDisplayConfig failed rc=$rc" }
  @{ Paths = $paths; Modes = $modes; Np = $np; Nm = $nm }
}

function Apply-DisplayConfig($paths, $modes) {
  $rc = [DC]::SetDisplayConfig($paths.Length, $paths, $modes.Length, $modes, [DC]::SDC_APPLY)
  if ($rc -ne 0) { throw "SetDisplayConfig failed rc=$rc" }
  Write-Output "SetDisplayConfig applied OK"
}

function Find-PathByDeviceName($config, $deviceName) {
  $m = [regex]::Match($deviceName, 'DISPLAY(\d+)')
  if (-not $m.Success) { throw "Cannot parse device name: $deviceName" }
  $targetId = [uint32]$m.Groups[1].Value
  for ($i = 0; $i -lt $config.Paths.Length; $i++) {
    $t = $config.Paths[$i].targetInfo
    if ($t.id -eq $targetId -and $t.targetAvailable) { return $i }
  }
  return -1
}

function Set-PrimaryDisplay($config, $deviceName) {
  $idx = Find-PathByDeviceName $config $deviceName
  if ($idx -lt 0) { throw "Device $deviceName not found" }
  $path = $config.Paths[$idx]
  $smi = Get-SourceModeIdx $path.sourceInfo.modeInfoIdx
  if ($smi -eq [DC]::SOURCE_MODE_IDX_INVALID) { throw "No source mode for $deviceName" }
  $sm = $config.Modes[$smi]
  if ($sm.modeInfo.sourceMode.position.x -eq 0 -and $sm.modeInfo.sourceMode.position.y -eq 0) {
    Write-Output "$deviceName already primary"; return $config
  }
  $oldX = $sm.modeInfo.sourceMode.position.x
  $oldY = $sm.modeInfo.sourceMode.position.y
  for ($m = 0; $m -lt $config.Modes.Length; $m++) {
    $mi = $config.Modes[$m]
    if ($mi.infoType -ne [DC]::MODE_INFO_TYPE_SOURCE) { continue }
    $pos = $mi.modeInfo.sourceMode.position
    if ($m -eq $smi) { $pos.x = 0; $pos.y = 0 }
    else { $pos.x -= $oldX; $pos.y -= $oldY }
    $mi.modeInfo.sourceMode.position = $pos
    $config.Modes[$m] = $mi
  }
  Write-Output ("Primary set to {0} (was at {1},{2})" -f $deviceName, $oldX, $oldY)
  return $config
}

function Swap-DisplayPosition($config, $deviceName1, $deviceName2) {
  $i1 = Find-PathByDeviceName $config $deviceName1
  $i2 = Find-PathByDeviceName $config $deviceName2
  if ($i1 -lt 0 -or $i2 -lt 0) { throw "One of $deviceName1, $deviceName2 not found" }
  $smi1 = Get-SourceModeIdx $config.Paths[$i1].sourceInfo.modeInfoIdx
  $smi2 = Get-SourceModeIdx $config.Paths[$i2].sourceInfo.modeInfoIdx
  if ($smi1 -eq [DC]::SOURCE_MODE_IDX_INVALID -or $smi2 -eq [DC]::SOURCE_MODE_IDX_INVALID) {
    throw "Missing source mode"
  }
  $pos1 = $config.Modes[$smi1].modeInfo.sourceMode.position
  $pos2 = $config.Modes[$smi2].modeInfo.sourceMode.position
  $config.Modes[$smi1].modeInfo.sourceMode.position = $pos2
  $config.Modes[$smi2].modeInfo.sourceMode.position = $pos1
  Write-Output ("Swapped: {0}({1},{2}) <-> {1}({3},{4})" -f $deviceName1, $pos1.x, $pos1.y, $deviceName2, $pos2.x, $pos2.y)
  return $config
}

function Disable-Display($config, $deviceName) {
  $idx = Find-PathByDeviceName $config $deviceName
  if ($idx -lt 0) { throw "Device $deviceName not found" }
  $path = $config.Paths[$idx]
  if (($path.flags -band [DC]::PATH_ACTIVE) -eq 0) {
    Write-Output "$deviceName already inactive"; return $config
  }
  $path.flags = $path.flags -band (-bnot [DC]::PATH_ACTIVE)
  $config.Paths[$idx] = $path
  Set-SourceModeIdx([ref]$config.Paths[$idx].sourceInfo.modeInfoIdx, [DC]::SOURCE_MODE_IDX_INVALID)
  $config.Paths[$idx].targetInfo.modeInfoIdx = [DC]::MODE_IDX_INVALID
  Write-Output "Disabled $deviceName"
  return $config
}

function Enable-Display($config, $deviceName) {
  $idx = Find-PathByDeviceName $config $deviceName
  if ($idx -lt 0) { throw "Device $deviceName not found" }
  $path = $config.Paths[$idx]
  if (($path.flags -band [DC]::PATH_ACTIVE) -ne 0) {
    Write-Output "$deviceName already active"; return $config
  }
  $path.flags = $path.flags -bor [DC]::PATH_ACTIVE
  $config.Paths[$idx] = $path
  Write-Output "Enabled $deviceName"
  return $config
}
