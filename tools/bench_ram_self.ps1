# tools/bench_ram_self.ps1 — DEV-ONLY self-measurement harness (never shipped).
# Runs engine.exe headless on a video, samples its RAM/handles/threads while
# it plays, then reports settled avg/peak. Lets the agent A/B memory tweaks
# without Task Manager screenshots.
#
# Usage:
#   powershell -ExecutionPolicy Bypass -File tools\bench_ram_self.ps1 `
#     -EngineExe build\msvc-dev\engine.exe `
#     -Video C:\path\to\video.mp4 -RunMs 60000 -OutJson out.json
#
# Requirements: no other engine.exe running (singleton mutex). The engine
# self-exits via --exit-after-ms; this script only observes.

param(
  [string]$EngineExe = "build\msvc-dev\engine.exe",
  [string]$Video = "",
  [int]$RunMs = 60000,
  [int]$SampleMs = 1000,
  [int]$SettleMs = 15000,
  [string]$OutJson = ""
)

$ErrorActionPreference = "Stop"

if ($Video -eq "" -or -not (Test-Path -LiteralPath $Video)) {
  throw "Video not found: $Video"
}
if (-not (Test-Path -LiteralPath $EngineExe)) {
  throw "Engine exe not found: $EngineExe"
}
$busy = Get-Process engine -ErrorAction SilentlyContinue
if ($busy) {
  throw "Another engine.exe is running (pid $($busy.Id)) - exit it first (singleton mutex)."
}

$exe = (Resolve-Path -LiteralPath $EngineExe).Path
$vid = (Resolve-Path -LiteralPath $Video).Path
Write-Host "bench_ram_self: exe=$exe"
Write-Host "bench_ram_self: video=$vid run_ms=$RunMs"

$logPath = Join-Path $env:LOCALAPPDATA "K6WP\engine.log"
$logBefore = 0
try {
  $logBefore = @(Get-Content -LiteralPath $logPath -ErrorAction Stop).Count
} catch { }

$proc = Start-Process -FilePath $exe -ArgumentList @(
  "--video", $vid, "--exit-after-ms", "$RunMs"
) -PassThru -WindowStyle Hidden

$samples = @()
$sw = [System.Diagnostics.Stopwatch]::StartNew()
$deadline = [DateTime]::UtcNow.AddMilliseconds($RunMs + 15000)
while (-not $proc.HasExited) {
  if ([DateTime]::UtcNow -gt $deadline) {
    Write-Host "bench_ram_self: deadline hit, killing pid $($proc.Id)"
    Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
    break
  }
  try {
    $p = Get-Process -Id $proc.Id -ErrorAction Stop
    $samples += [pscustomobject]@{
      t_ms          = $sw.ElapsedMilliseconds
      workingset_mb = [math]::Round($p.WorkingSet64 / 1MB, 1)
      private_mb    = [math]::Round($p.PrivateMemorySize64 / 1MB, 1)
      handles       = $p.HandleCount
      threads       = $p.Threads.Count
    }
  } catch {
    break
  }
  Start-Sleep -Milliseconds $SampleMs
}
try { $proc.WaitForExit(5000) | Out-Null } catch { }

if ($samples.Count -eq 0) { throw "No samples collected (engine exited immediately?)" }

$settled = @($samples | Where-Object { $_.t_ms -ge $SettleMs -and $_.t_ms -le $RunMs })
if ($settled.Count -eq 0) { $settled = $samples }

$result = [ordered]@{
  exe            = $exe
  video          = $vid
  run_ms         = $RunMs
  samples        = $samples.Count
  settled        = $settled.Count
  ws_avg_mb      = [math]::Round(($settled | Measure-Object workingset_mb -Average).Average, 1)
  ws_peak_mb     = ($settled | Measure-Object workingset_mb -Maximum).Maximum
  priv_avg_mb    = [math]::Round(($settled | Measure-Object private_mb -Average).Average, 1)
  priv_peak_mb   = ($settled | Measure-Object private_mb -Maximum).Maximum
  threads_last   = $settled[-1].threads
  threads_peak   = ($settled | Measure-Object threads -Maximum).Maximum
  handles_last   = $settled[-1].handles
}

# hwdec proof scoped to THIS run: only log lines appended after start.
try {
  $newLines = @(Get-Content -LiteralPath $logPath -ErrorAction Stop |
    Select-Object -Skip $logBefore)
  $hw = $newLines | Select-String "hwdec-current" | Select-Object -Last 1
  if ($hw) { $result["hwdec"] = $hw.Line.Trim() }
} catch { }

$result | ConvertTo-Json -Depth 3 | Write-Host
if ($OutJson -ne "") {
  $result | ConvertTo-Json -Depth 3 | Set-Content -LiteralPath $OutJson -Encoding UTF8
  Write-Host "bench_ram_self: wrote $OutJson"
}
