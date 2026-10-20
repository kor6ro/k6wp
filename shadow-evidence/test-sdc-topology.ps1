# shadow-evidence/test-sdc-topology.ps1 — coba SetDisplayConfig dengan topology flags saja
# (tidak perlu memodifikasi path/mode array)
$ErrorActionPreference = 'Stop'

Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public class SDC {
  [DllImport("user32.dll", ExactSpelling = true, CharSet = CharSet.Unicode)]
  public static extern int SetDisplayConfig(
    uint numPathArrayElements, IntPtr pathArray,
    uint numModeInfoArrayElements, IntPtr modeInfoArray,
    uint flags);
  public const uint SDC_APPLY = 0x80;
  public const uint SDC_ALLOW_CHANGES = 0x400;
  public const uint SDC_TOPOLOGY_INTERNAL = 0x1;
  public const uint SDC_TOPOLOGY_CLONE = 0x2;
  public const uint SDC_TOPOLOGY_EXTEND = 0x4;
}
'@

function Try-Topology([uint32]$topo, [string]$name) {
  $rc = [SDC]::SetDisplayConfig(0, [IntPtr]::Zero, 0, [IntPtr]::Zero,
    $topo -bor [SDC]::SDC_APPLY -bor [SDC]::SDC_ALLOW_CHANGES)
  $err = [System.Runtime.InteropServices.Marshal]::GetLastWin32Error()
  Write-Output ("{0}: rc={1} err={2}" -f $name, $rc, $err)
}

Try-Topology 2 "CLONE (duplicate)"
Start-Sleep -Seconds 3
Try-Topology 4 "EXTEND"
