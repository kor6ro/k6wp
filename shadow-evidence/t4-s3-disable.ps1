# shadow-evidence/t4-s3-disable.ps1 — S3: disable/enable virtual monitor (tiruan, bukan cabut kabel)
# Uses DisplaySwitch.exe /internal (internal only, effectively disables external/VDD monitor)
# then /extend to restore. Revert is built in.
$ErrorActionPreference = 'Stop'

Write-Output "=== S3: disable/enable monitor virtual (tiruan) ==="

# Before
build\msvc-dev\monitor_dump.exe > shadow-evidence\t4\s3-before.json
powershell -NoProfile -File shadow-evidence\get-state.ps1 > shadow-evidence\t4\s3-before-getstate.json
$NbBefore=(Select-String -Path $env:LOCALAPPDATA\K6WP\engine.log -Pattern 'placement:').Count
Write-Output ("before: placement lines = {0}" -f $NbBefore)
type shadow-evidence\t4\s3-before.json

# Action: disable external/VDD monitor (internal only)
Write-Output "Action: DisplaySwitch /internal"
powershell -NoProfile -Command "Start-Process 'cmd.exe' -ArgumentList '/c echo off & DisplaySwitch.exe /internal' -Wait"
Start-Sleep -Seconds 5

# After (disabled)
build\msvc-dev\monitor_dump.exe > shadow-evidence\t4\s3-disabled.json
powershell -NoProfile -File shadow-evidence\get-state.ps1 > shadow-evidence\t4\s3-disabled-getstate.json
Write-Output "=== AFTER DISABLE ==="
type shadow-evidence\t4\s3-disabled.json
$NbDisabled=(Select-String -Path $env:LOCALAPPDATA\K6WP\engine.log -Pattern 'placement:').Count
Write-Output ("new placement lines since before: {0}" -f ($NbDisabled - $NbBefore))
Select-String -Path $env:LOCALAPPDATA\K6WP\engine.log -Pattern 'placement:' | Select-Object -Skip $NbBefore | Select-Object -Last 5

# Revert: restore extend
Write-Output "Revert: DisplaySwitch /extend"
powershell -NoProfile -Command "Start-Process 'cmd.exe' -ArgumentList '/c echo off & DisplaySwitch.exe /extend' -Wait"
Start-Sleep -Seconds 3

# After revert
build\msvc-dev\monitor_dump.exe > shadow-evidence\t4\s3-reverted.json
powershell -NoProfile -File shadow-evidence\get-state.ps1 > shadow-evidence\t4\s3-reverted-getstate.json
Write-Output "=== AFTER REVERT ==="
type shadow-evidence\t4\s3-reverted.json
