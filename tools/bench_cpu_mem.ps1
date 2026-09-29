# tools/bench_cpu_mem.ps1 — generic CPU/RAM sampler for a running process.
#
# Samples TotalProcessorTime deltas (=> % of all cores) + WorkingSet/Private
# bytes at a fixed interval, then writes a JSON summary. Stock PowerShell +
# .NET only, zero dependencies. Used by Step 9.4 to record engine idle-video
# numbers honestly (no pass/fail gate here — numbers go to known-limitations).
#
# Usage:
#   powershell -ExecutionPolicy Bypass -File tools/bench_cpu_mem.ps1 `
#     -ProcessId <pid> -Minutes 1 -IntervalMs 500 -OutJson <path>
#   (or -ProcessName <name> to attach to a single live process by name.)

[CmdletBinding()]
param(
  [int]$ProcessId = 0,
  [string]$ProcessName = '',
  [double]$Minutes = 1,
  [int]$IntervalMs = 500,
  [string]$OutJson = ''
)

$ErrorActionPreference = 'Stop'

if ($ProcessId -le 0 -and [string]::IsNullOrEmpty($ProcessName)) {
  Write-Output 'FAIL: pass -ProcessId or -ProcessName'
  exit 1
}
if ($Minutes -le 0) { Write-Output 'FAIL: Minutes must be > 0'; exit 1 }
if ($IntervalMs -lt 100) { Write-Output 'FAIL: IntervalMs must be >= 100'; exit 1 }

function Get-Target {
  if ($ProcessId -gt 0) {
    return Get-Process -Id $ProcessId -ErrorAction Stop
  }
  $ps = @(Get-Process -Name $ProcessName -ErrorAction SilentlyContinue)
  if ($ps.Count -eq 0) { throw "no process named '$ProcessName'" }
  if ($ps.Count -gt 1) { throw "$($ps.Count) processes named '$ProcessName' (pass -ProcessId)" }
  return $ps[0]
}

$cores = [Environment]::ProcessorCount
$deadline = [DateTime]::UtcNow.AddMinutes($Minutes)
$cpuSamples = New-Object System.Collections.Generic.List[double]
$wsSamples = New-Object System.Collections.Generic.List[long]
$privSamples = New-Object System.Collections.Generic.List[long]

$p = Get-Target
$prevCpu = $p.CPU  # TotalProcessorTime in seconds (null until sampled once)
$prevT = [DateTime]::UtcNow
if ($null -eq $prevCpu) { $prevCpu = 0.0 }

while ([DateTime]::UtcNow -lt $deadline) {
  Start-Sleep -Milliseconds $IntervalMs
  try {
    $p = Get-Target
    $p.Refresh()
  } catch {
    Write-Output 'FAIL: target process exited during sampling'
    exit 1
  }
  $now = [DateTime]::UtcNow
  $curCpu = $p.CPU
  if ($null -eq $curCpu) { $curCpu = $prevCpu }
  $dt = ($now - $prevT).TotalSeconds
  if ($dt -gt 0) {
    $pct = (($curCpu - $prevCpu) / $dt / $cores) * 100.0
    if ($pct -lt 0) { $pct = 0 }
    $cpuSamples.Add($pct)
  }
  $wsSamples.Add($p.WorkingSet64)
  try { $privSamples.Add($p.PrivateMemorySize64) } catch { $privSamples.Add([long]0) }
  $prevCpu = $curCpu
  $prevT = $now
}

function Avg([System.Collections.Generic.List[double]]$xs) {
  if ($xs.Count -eq 0) { return 0.0 }
  ($xs | Measure-Object -Average).Average
}
function Max-Long([System.Collections.Generic.List[long]]$xs) {
  if ($xs.Count -eq 0) { return 0 }
  ($xs | Measure-Object -Maximum).Maximum
}

$summary = [ordered]@{
  tool = 'bench_cpu_mem'
  date = (Get-Date -Format 'yyyy-MM-dd')
  samples = $cpuSamples.Count
  interval_ms = $IntervalMs
  minutes = $Minutes
  cores = $cores
  avg_cpu_percent = [math]::Round((Avg $cpuSamples), 2)
  peak_cpu_percent = [math]::Round((($cpuSamples | Measure-Object -Maximum).Maximum), 2)
  avg_workingset_mb = [math]::Round(((($wsSamples | Measure-Object -Average).Average) / 1MB), 2)
  peak_workingset_mb = [math]::Round(((Max-Long $wsSamples) / 1MB), 2)
  avg_private_mb = [math]::Round(((($privSamples | Measure-Object -Average).Average) / 1MB), 2)
  peak_private_mb = [math]::Round(((Max-Long $privSamples) / 1MB), 2)
  reproducibility = "TotalProcessorTime deltas / $cores cores, ${IntervalMs}ms interval. Rerun +-10% on avg_cpu_percent under identical load; close other apps before comparing."
}

$json = ($summary | ConvertTo-Json -Compress)
Write-Output $json
if (-not [string]::IsNullOrEmpty($OutJson)) {
  Set-Content -LiteralPath $OutJson -Value $json -Encoding utf8
}
