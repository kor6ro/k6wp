# shadow-evidence/test-disable.ps1 — coba Disable-PnpDevice / Enable-PnpDevice untuk VDD
$ErrorActionPreference = 'Continue'

$vd = Get-PnpDevice -FriendlyName '*Virtual Display*' -ErrorAction SilentlyContinue
if (-not $vd) { Write-Output "VDD device not found"; exit 1 }
Write-Output ("Before: {0} Status={1}" -f $vd.Name, $vd.Status)

# Try disable
$vd | Disable-PnpDevice -Confirm:$false -ErrorAction SilentlyContinue
Start-Sleep -Seconds 3
$vd2 = Get-PnpDevice -FriendlyName '*Virtual Display*' -ErrorAction SilentlyContinue
if ($vd2) {
  Write-Output ("After disable: {0} Status={1}" -f $vd2.Name, $vd2.Status)
} else {
  Write-Output "After disable: device not found (possibly removed/disabled)"
}

# Check monitor_dump after disable
& 'build\msvc-dev\monitor_dump.exe' 2>&1

# Re-enable
$vd3 = Get-PnpDevice -FriendlyName '*Virtual Display*' -ErrorAction SilentlyContinue
if ($vd3) {
  $vd3 | Enable-PnpDevice -Confirm:$false -ErrorAction SilentlyContinue
  Start-Sleep -Seconds 3
  & 'build\msvc-dev\monitor_dump.exe' 2>&1
}
