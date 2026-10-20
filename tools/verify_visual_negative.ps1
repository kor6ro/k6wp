# tools/verify_visual_negative.ps1 - prove the visual parity gate FAILS when
# the wallpaper is not playing. A gate that never fails is not a gate.
#
# Captures the desktop with NO engine running (the plain Windows wallpaper)
# and compares it against the engine-playing baseline. The mean_luma /
# stddev must differ enough to trip the gate, and the motion diff must be
# ~0 (a static desktop image does not move).
$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
. (Join-Path $here 'bench_lib.ps1')

$baseline = Join-Path $here '..\docs\bench\visual\baseline'
$negative = Join-Path $here '..\build\visual-negative'

$busy = @(Get-Process engine -ErrorAction SilentlyContinue)
if ($busy.Count -gt 0) { throw "an engine.exe is running (pid $($busy.Id -join ',')); stop it first" }

Remove-Item -LiteralPath $negative -Recurse -Force -ErrorAction SilentlyContinue
& (Join-Path $here 'bench_visual.ps1') -Capture $negative -Force | Out-Null

# The compare mode ends in Write-Error on a parity failure. That is the
# EXPECTED outcome here, so the error must not abort this script.
$prevPref = $ErrorActionPreference
$ErrorActionPreference = 'Continue'
$out = & (Join-Path $here 'bench_visual.ps1') -Compare $baseline $negative 2>&1
$rc = $LASTEXITCODE
$ErrorActionPreference = $prevPref
$out | Select-Object -Last 30
if ($rc -eq 0) {
    Write-Error "verify_visual_negative: FAILED - the parity gate passed a NON-playing desktop (the gate is useless)"
    exit 1
}
Write-Host "verify_visual_negative: OK - the parity gate correctly REJECTED the non-playing desktop (exit $rc)"
exit 0
