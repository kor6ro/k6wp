# shadow-evidence/test-disable2.ps1 — coba Disable-PnpDevice dengan ErrorAction Stop
$ErrorActionPreference = 'Continue'

$vd = Get-PnpDevice -FriendlyName '*Virtual Display*' -ErrorAction SilentlyContinue
if (-not $vd) { Write-output "VDD not found"; exit 1 }
Write-output ("Before: {0} Status={1}" -f $vd.Name, $vd.Status)

try {
  $vd | Disable-PnpDevice -Confirm:$false -ErrorAction Stop
  Write-output "Disable-PnpDevice: no exception thrown"
} catch {
  Write-output ("Disable-PnpDevice EXCEPTION: {0}" -f $_.Exception.Message)
}

Start-Sleep -Seconds 2
$vd2 = Get-PnpDevice -FriendlyName '*Virtual Display*' -ErrorAction SilentlyContinue
if ($vd2) {
  Write-output ("After: {0} Status={1}" -f $vd2.Name, $vd2.Status)
} else {
  Write-output "After: device not found"
}
