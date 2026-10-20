# shadow-evidence/test-sdc2.ps1 — SetDisplayConfig with corrected struct sizes
$ErrorActionPreference = 'Stop'

Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;

public class SDC2 {
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
    [FieldOffset(0)] public byte b0; [FieldOffset(1)] public byte b1; [FieldOffset(2)] public byte b2;
    [FieldOffset(3)] public byte b3; [FieldOffset(4)] public byte b4; [FieldOffset(5)] public byte b5;
    [FieldOffset(6)] public byte b6; [FieldOffset(7)] public byte b7; [FieldOffset(8)] public byte b8;
    [FieldOffset(9)] public byte b9; [FieldOffset(10)] public byte b10; [FieldOffset(11)] public byte b11;
    [FieldOffset(12)] public byte b12; [FieldOffset(13)] public byte b13; [FieldOffset(14)] public byte b14;
    [FieldOffset(15)] public byte b15; [FieldOffset(16)] public byte b16; [FieldOffset(17)] public byte b17;
    [FieldOffset(18)] public byte b18; [FieldOffset(19)] public byte b19; [FieldOffset(20)] public byte b20;
    [FieldOffset(21)] public byte b21; [FieldOffset(22)] public byte b22; [FieldOffset(23)] public byte b23;
    [FieldOffset(24)] public byte b24; [FieldOffset(25)] public byte b25; [FieldOffset(26)] public byte b26;
    [FieldOffset(27)] public byte b27; [FieldOffset(28)] public byte b28; [FieldOffset(29)] public byte b29;
    [FieldOffset(30)] public byte b30; [FieldOffset(31)] public byte b31; [FieldOffset(32)] public byte b32;
    [FieldOffset(33)] public byte b33; [FieldOffset(34)] public byte b34; [FieldOffset(35)] public byte b35;
    [FieldOffset(36)] public byte b36; [FieldOffset(37)] public byte b37; [FieldOffset(38)] public byte b38;
    [FieldOffset(39)] public byte b39; [FieldOffset(40)] public byte b40; [FieldOffset(41)] public byte b41;
    [FieldOffset(42)] public byte b42; [FieldOffset(43)] public byte b43; [FieldOffset(44)] public byte b44;
    [FieldOffset(45)] public byte b45; [FieldOffset(46)] public byte b46; [FieldOffset(47)] public byte b47;
    [FieldOffset(48)] public byte b48; [FieldOffset(49)] public byte b49; [FieldOffset(50)] public byte b50;
    [FieldOffset(51)] public byte b51; [FieldOffset(52)] public byte b52; [FieldOffset(53)] public byte b53;
    [FieldOffset(54)] public byte b54; [FieldOffset(55)] public byte b55; [FieldOffset(56)] public byte b56;
    [FieldOffset(57)] public byte b57; [FieldOffset(58)] public byte b58; [FieldOffset(59)] public byte b59;
    [FieldOffset(60)] public byte b60; [FieldOffset(61)] public byte b61; [FieldOffset(62)] public byte b62;
    [FieldOffset(63)] public byte b63;
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
    public LUID adapterId;       // 8
    public uint id;              // 4
    public uint modeInfoIdx;     // 4
    public uint outputTechnology;// 4
    public uint rotation;        // 4
    public uint scaling;         // 4
    public uint refreshRateNum;  // 4
    public uint refreshRateDen;  // 4
    public uint scanlineOrdering;// 4
    public int targetAvailable;  // 4 (Windows BOOL = int32)
    public uint statusFlags;     // 4
    // Total: 44 bytes
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

$np = [uint32]0; $nm = [uint32]0
$rc = [SDC2]::GetDisplayConfigBufferSizes([SDC2]::QDC_ALL_PATHS, [ref]$np, [ref]$nm)
Write-Output ("BufferSizes rc={0} np={1} nm={2}" -f $rc, $np, $nm)

$paths = New-Object 'SDC2+DISPLAYCONFIG_PATH_INFO[]'($np)
$modes = New-Object 'SDC2+DISPLAYCONFIG_MODE_INFO[]'($nm)
$rc = [SDC2]::QueryDisplayConfig([SDC2]::QDC_ALL_PATHS, [ref]$np, $paths, [ref]$nm, $modes, [IntPtr]::Zero)
Write-Output ("Query rc={0} np={1} nm={2}" -f $rc, $np, $nm)

for ($i = 0; $i -lt [int]$np; $i++) {
  $p = $paths[$i]
  $t = $p.targetInfo
  $s = $p.sourceInfo
  $smi = ($s.modeInfoIdx -band 0xFFFF0000) -shr 16
  $posStr = ""
  if ($smi -ne 0xFFFF -and [int]$smi -lt $modes.Length) {
    $pos = $modes[$smi].modeInfo.sourceMode.position
    $posStr = "pos=({0},{1})" -f $pos.x, $pos.y
  }
  Write-Output ("path {0}: active={1} targetId={2} srcMode={3} flags=0x{4:X} {5}" -f
    $i, (($p.flags -band [SDC2]::PATH_ACTIVE) -ne 0), $t.id, $smi, $p.flags, $posStr)
}

# Try SetDisplayConfig with SDC_APPLY only (no modification)
$rc2 = [SDC2]::SetDisplayConfig($np, $paths, $nm, $modes, [SDC2]::SDC_APPLY)
Write-Output ("SetDisplayConfig(apply only) rc={0}" -f $rc2)
