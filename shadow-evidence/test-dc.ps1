# shadow-evidence/test-dc.ps1 — test SetDisplayConfig wrapper
. ./shadow-evidence/display-config.ps1
$cfg = Get-DisplayConfig
Write-Output ("paths={0} modes={1}" -f $cfg.Np, $cfg.Nm)
for ($i = 0; $i -lt $cfg.Np; $i++) {
  $p = $cfg.Paths[$i]
  $t = $p.targetInfo
  $s = $p.sourceInfo
  $smi = Get-SourceModeIdx $s.modeInfoIdx
  Write-Output ("path {0}: active={1} targetId={2} srcModeIdx={3} flags=0x{4:X} pos=({5},{6})" -f
    $i, (($p.flags -band [DC]::PATH_ACTIVE) -ne 0), $t.id, $smi, $p.flags,
    $cfg.Modes[$smi].modeInfo.sourceMode.position.x, $cfg.Modes[$smi].modeInfo.sourceMode.position.y)
}
