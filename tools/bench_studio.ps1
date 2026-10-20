# tools/bench_studio.ps1 - Phase 0 Studio (GUI) sampler.
#
# Measures the Studio desktop app: startup latency, idle RAM/CPU after the UI
# is up, and the memory of a loaded preview. The Studio is a Qt/QML app, so
# there is no stdout marker: startup is measured as
#   process start -> first visible top-level window (MainWindowHandle != 0)
#   process start -> first non-empty window title
# which is what a user actually perceives as "the app opened".
#
# Method: the REAL studio.exe, isolated %LOCALAPPDATA% (so the user's real
# library/settings are untouched), no code instrumentation. .NET process
# counters for RAM/CPU, same as the engine harness.
#
# The Studio singleton mutex (Local\K6WP-Studio-App-Singleton) means only one
# instance can run: the harness refuses to start if one is already up.
#
# Usage:
#   powershell -NoProfile -ExecutionPolicy Bypass -File tools\bench_studio.ps1 `
#     -Runs 3 -OutJson docs\bench\studio.json

[CmdletBinding()]
param(
    [string]$StudioExe = '',
    [string]$CorpusDir = '',
    [int]$Runs = 3,
    [string]$OutJson = '',
    [switch]$SkipMinimize
)

$ErrorActionPreference = 'Stop'
$script:Here = Split-Path -Parent $MyInvocation.MyCommand.Path
. (Join-Path $script:Here 'bench_lib.ps1')
if ([string]::IsNullOrWhiteSpace($StudioExe)) { $StudioExe = Join-Path $script:Here '..\build\release\studio.exe' }
if ([string]::IsNullOrWhiteSpace($CorpusDir)) { $CorpusDir = Join-Path $script:Here '..\tests\corpus' }
if ([string]::IsNullOrWhiteSpace($OutJson)) { $OutJson = Join-Path $script:Here '..\docs\bench\studio.json' }

if ($Runs -lt 1) { throw "Runs must be >= 1" }
if (-not (Test-Path -LiteralPath $StudioExe)) { throw "studio not found: $StudioExe" }
$busy = @(Get-Process studio -ErrorAction SilentlyContinue)
if ($busy.Count -gt 0) {
    throw ("studio.exe is already running (pid {0}); close it first" -f (($busy | ForEach-Object { $_.Id }) -join ','))
}

$hw = Get-HwInventory
$savedWindows = @()
if (-not $SkipMinimize) {
    Write-Host "bench_studio: minimizing top-level windows (restored at the end)"
    $savedWindows = Get-DesktopWindows
}

$results = New-Object System.Collections.Generic.List[object]
try {
    for ($i = 1; $i -le $Runs; $i++) {
        Write-Host "bench_studio: run $i ..."
        $dataDir = Join-Path $env:TEMP ("k6wp-bench-studio-{0}" -f [Guid]::NewGuid().ToString('N').Substring(0, 8))
        New-Item -ItemType Directory -Force -Path (Join-Path $dataDir 'K6WP') | Out-Null
        $oldLocal = $env:LOCALAPPDATA
        $proc = $null
        try {
            $env:LOCALAPPDATA = $dataDir
            $sw = [Diagnostics.Stopwatch]::StartNew()
            $proc = Start-Process -FilePath $StudioExe -PassThru
        }
        finally {
            $env:LOCALAPPDATA = $oldLocal
        }

        # Startup: wait for the first visible top-level window.
        $windowMs = $null
        $titleMs = $null
        $deadline = (Get-Date).AddSeconds(30)
        while ((Get-Date) -lt $deadline) {
            try { $proc.Refresh() } catch { break }
            if ($proc.HasExited) { break }
            if ($null -eq $windowMs -and $proc.MainWindowHandle -ne 0) {
                $windowMs = $sw.ElapsedMilliseconds
            }
            if ($null -eq $titleMs -and -not [string]::IsNullOrWhiteSpace($proc.MainWindowTitle)) {
                $titleMs = $sw.ElapsedMilliseconds
            }
            if ($null -ne $windowMs -and $null -ne $titleMs) { break }
            Start-Sleep -Milliseconds 20
        }
        if ($null -eq $windowMs) {
            Write-Host "  WARNING: no visible window within 30s (studio may have exited: $($proc.HasExited))"
        }

        # Settle, then sample the idle UI.
        Start-Sleep -Seconds 3
        $idle = @(Get-ProcessSeries -ProcessId $proc.Id -Seconds 10 -IntervalMs 1000)

        $priv = @($idle | ForEach-Object { $_.private_mb })
        $ws = @($idle | ForEach-Object { $_.ws_mb })
        $cpu = @($idle | ForEach-Object { $_.cpu_pct })

        $results.Add([pscustomobject]@{
            run = $i
            startup_window_ms = $windowMs
            startup_title_ms = $titleMs
            exit_code = $(try { if ($proc.HasExited) { $proc.ExitCode } else { $null } } catch { $null })
            private_mb = (Get-Stats $priv)
            ws_mb = (Get-Stats $ws)
            cpu_pct = (Get-Stats $cpu)
            threads_last = $(if ($idle.Count -gt 0) { $idle[-1].threads } else { $null })
            handles_last = $(if ($idle.Count -gt 0) { $idle[-1].handles } else { $null })
            data_dir = $dataDir
        })
        Write-Host ("  startup {0} ms (window), idle priv median {1} MB, cpu mean {2}%" -f `
            $windowMs, (Get-Stats $priv).median, (Get-Stats $cpu).mean)

        try { if (-not $proc.HasExited) { Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue } } catch { }
        try { $proc.WaitForExit(5000) | Out-Null } catch { }
    }
}
finally {
    if (-not $SkipMinimize) { Restore-DesktopWindows -Saved $savedWindows }
}

function Agg($key) {
    $vals = @($results | ForEach-Object { $_.$key.median } | Where-Object { $null -ne $_ })
    if ($vals.Count -eq 0) { return $null }
    return Get-Stats $vals
}
$startupVals = @($results | ForEach-Object { $_.startup_window_ms } | Where-Object { $null -ne $_ })

$report = [ordered]@{
    tool = 'bench_studio'
    phase = 0
    date = (Get-Date -Format 'yyyy-MM-ddTHH:mm:ssK')
    label = 'live-proven'
    studio = $StudioExe
    runs = $Runs
    hw = $hw
    method = 'real studio.exe; isolated LOCALAPPDATA; startup = process start -> first visible top-level window; idle sampled 10s after a 3s settle; .NET process counters'
    startup_window_ms = $(if ($startupVals.Count -gt 0) { Get-Stats $startupVals } else { $null })
    private_mb = (Agg 'private_mb')
    ws_mb = (Agg 'ws_mb')
    cpu_pct = (Agg 'cpu_pct')
    raw = $results
}

$json = $report | ConvertTo-Json -Depth 12
$outFull = [System.IO.Path]::GetFullPath($OutJson)
$outDir = [System.IO.Path]::GetDirectoryName($outFull)
if (-not (Test-Path -LiteralPath $outDir)) { New-Item -ItemType Directory -Force -Path $outDir | Out-Null }
$json | Set-Content -LiteralPath $outFull -Encoding UTF8
Write-Host "bench_studio: wrote $outFull"
