# tools/diag_gpu.ps1 - who is using the GPU right now?
#
# The Phase 0 baseline recorded 22.6% GPU for 1080p playback; a later
# re-measurement of the SAME binary read 35%. The engine's own counters
# cannot explain that, so the machine's GPU load is sampled per process to
# see whether another consumer appeared.
$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
. (Join-Path $here 'bench_lib.ps1')

$paths = @((Get-Counter -ListSet 'GPU Engine' -ErrorAction Stop).PathsWithInstances |
    Where-Object { $_ -like '*Utilization Percentage*' })
Write-Host ("GPU Engine instances: {0}" -f $paths.Count)

$rows = @()
for ($i = 1; $i -le 3; $i++) {
    $samples = @()
    try { $samples = @((Get-Counter $paths -ErrorAction Stop).CounterSamples) }
    catch { $samples = @((Get-Counter $paths -ErrorAction SilentlyContinue).CounterSamples) }
    foreach ($s in $samples) {
        if ($s.CookedValue -le 0.1) { continue }
        if ($s.Path -notmatch 'pid_(\d+)_') { continue }
        $pid2 = [int]$Matches[1]
        $engtype = ''
        if ($s.Path -match 'engtype_([A-Za-z]+)') { $engtype = $Matches[1] }
        $rows += [pscustomobject]@{
            sample = $i
            pid = $pid2
            proc = $(try { (Get-Process -Id $pid2 -ErrorAction Stop).ProcessName } catch { 'gone' })
            engtype = $engtype
            pct = [math]::Round($s.CookedValue, 2)
        }
    }
    Start-Sleep -Milliseconds 700
}

Write-Host ""
Write-Host "--- GPU utilization by process (top 15) ---"
$rows | Group-Object pid, proc | ForEach-Object {
    $vals = $_.Group | ForEach-Object { $_.pct }
    [pscustomobject]@{
        pid = $_.Group[0].pid
        proc = $_.Group[0].proc
        total_pct = [math]::Round((Get-Stats $vals).median, 2)
        engines = (($_.Group | Group-Object engtype | ForEach-Object { "$($_.Name):$([math]::Round((($_.Group | Measure-Object pct -Sum).Sum),1))" }) -join ' ')
    }
} | Sort-Object total_pct -Descending | Select-Object -First 15 | Format-Table -AutoSize | Out-String | Write-Host

Write-Host "--- display / refresh state ---"
try {
    Get-CimInstance -Namespace root\wmi -ClassName WmiMonitorListedSupportedSourceModes -ErrorAction Stop |
        Select-Object -First 3 @{n='mode';e={$_.ModeInformation}} | Format-Table -AutoSize | Out-String | Write-Host
} catch { }
try {
    $d = Get-CimInstance Win32_VideoController | Select-Object Name, CurrentHorizontalResolution, CurrentVerticalResolution, CurrentRefreshRate, VideoModeDescription
    $d | Format-Table -AutoSize | Out-String | Write-Host
} catch { }
