# tools/bench_resume.ps1 - Phase 0 resume / suspend latency sampler.
#
# Measures how long the engine takes to come back to a VISIBLE, MOVING
# wallpaper after a suspend/resume cycle.
#
# Two independent signals, because neither alone is trustworthy:
#
#  1. CONTROL-PATH latency (exact, code-driven):
#     the engine's own log timestamps. `--simulate-suspend-after-ms=N` posts
#     PBT_APMSUSPEND then PBT_APMRESUMEAUTOMATIC 2 s later, both through the
#     real Win32 power-broadcast path (engine_app.cpp HandlePowerBroadcast).
#     The log lines "power: PBT_APMSUSPEND received" and
#     "power: PBT_APMRESUMEAUTOMATIC received" are timestamped to the
#     millisecond, so the gap between them is the suspend->resume control
#     latency with no estimation.
#
#  2. VISUAL latency (proxy, honest about its limits):
#     after resume, sample the desktop region until it CHANGES relative to the
#     pre-suspend frame, i.e. until the wallpaper is actually animating again.
#     This is a lower bound on "the user sees motion again". It depends on the
#     screen-capture path (GDI CopyFromScreen) and the video's own content,
#     so it is reported as a proxy, never as the engine's internal latency.
#
# Method: real engine.exe, isolated %LOCALAPPDATA%, windows minimized so the
# injected wallpaper is not occluded. No code instrumentation.
#
# Usage:
#   powershell -NoProfile -ExecutionPolicy Bypass -File tools\bench_resume.ps1 `
#     -Runs 3 -OutJson docs\bench\resume.json

[CmdletBinding()]
param(
    [string]$EngineExe = '',
    [string]$CorpusDir = '',
    [int]$Runs = 3,
    [string]$OutJson = '',
    [switch]$SkipMinimize
)

$ErrorActionPreference = 'Stop'
$script:Here = Split-Path -Parent $MyInvocation.MyCommand.Path
. (Join-Path $script:Here 'bench_lib.ps1')
if ([string]::IsNullOrWhiteSpace($EngineExe)) { $EngineExe = Join-Path $script:Here '..\build\release\engine.exe' }
if ([string]::IsNullOrWhiteSpace($CorpusDir)) { $CorpusDir = Join-Path $script:Here '..\tests\corpus' }
if ([string]::IsNullOrWhiteSpace($OutJson)) { $OutJson = Join-Path $script:Here '..\docs\bench\resume.json' }

if ($Runs -lt 1) { throw "Runs must be >= 1" }
if (-not (Test-Path -LiteralPath $EngineExe)) { throw "engine not found: $EngineExe" }
$busy = @(Get-Process engine -ErrorAction SilentlyContinue)
if ($busy.Count -gt 0) {
    throw ("another engine.exe is running (pid {0}); stop it first" -f (($busy | ForEach-Object { $_.Id }) -join ','))
}

$video = Join-Path $CorpusDir 'anime.mp4'
if (-not (Test-Path -LiteralPath $video)) { throw "missing corpus: $video" }

$hw = Get-HwInventory
$savedWindows = @()
if (-not $SkipMinimize) {
    Write-Host "bench_resume: minimizing top-level windows (restored at the end)"
    $savedWindows = Get-DesktopWindows
}

$results = New-Object System.Collections.Generic.List[object]
try {
    for ($i = 1; $i -le $Runs; $i++) {
        Write-Host "bench_resume: run $i ..."
        $dataDir = Join-Path $env:TEMP ("k6wp-bench-resume-{0}" -f [Guid]::NewGuid().ToString('N').Substring(0, 8))
        New-Item -ItemType Directory -Force -Path (Join-Path $dataDir 'K6WP') | Out-Null
        $oldLocal = $env:LOCALAPPDATA
        $proc = $null
        try {
            $env:LOCALAPPDATA = $dataDir
            $proc = Start-Process -FilePath $EngineExe -PassThru -WindowStyle Hidden `
                -ArgumentList @("--video=$video", '--simulate-suspend-after-ms=6000', '--exit-after-ms=45000')
        }
        finally {
            $env:LOCALAPPDATA = $oldLocal
        }
        if (-not (Wait-K6wpEngine -TimeoutMs 15000)) {
            try { if (-not $proc.HasExited) { Stop-Process -Id $proc.Id -Force } } catch { }
            throw "engine did not answer the IPC pipe within 15s"
        }
        $logPath = Join-Path $dataDir 'K6WP\engine.log'

        # Let it play and settle before the simulated suspend fires.
        Start-Sleep -Milliseconds 4000

        # The suspend fires at 6000 ms and the resume 2 s later. The engine
        # logs both power markers with millisecond timestamps, so the control
        # latency is exact.
        #
        # The VISUAL reference frame must be captured while the wallpaper is
        # FROZEN (i.e. after the suspend marker, once the renderer has
        # actually paused). Capturing it before the suspend would make the
        # post-resume diff meaningless: the wallpper had already moved on
        # between the reference and the freeze.
        $suspendAt = $null
        $resumeAt = $null
        $visualMs = $null
        $frozen = $null
        $deadline = (Get-Date).AddSeconds(40)
        while ((Get-Date) -lt $deadline) {
            if (Test-Path -LiteralPath $logPath) {
                $lines = @(Get-Content -LiteralPath $logPath -ErrorAction SilentlyContinue)
                $sus = @($lines | Select-String 'PBT_APMSUSPEND received' | Select-Object -Last 1)
                $res = @($lines | Select-String 'PBT_APMRESUMEAUTOMATIC received' | Select-Object -Last 1)
                if ($null -eq $suspendAt -and $sus.Count -gt 0) {
                    $suspendAt = $sus[0].Line
                    # Give the renderer a moment to freeze, then take the
                    # reference frame of the frozen wallpaper.
                    Start-Sleep -Milliseconds 400
                    $frozen = Get-RegionSample -Width 240 -Height 160
                }
                if ($null -eq $resumeAt -and $res.Count -gt 0) {
                    $resumeAt = $res[0].Line
                    # Resume just happened: start the visual-change clock.
                    $sw = [Diagnostics.Stopwatch]::StartNew()
                    while ($sw.ElapsedMilliseconds -lt 10000) {
                        $now = Get-RegionSample -Width 240 -Height 160
                        $diff = Get-BitmapDiff -A $frozen -B $now
                        if ($diff -gt 0.5) { $visualMs = $sw.ElapsedMilliseconds; break }
                        Start-Sleep -Milliseconds 25
                    }
                    break
                }
            }
            Start-Sleep -Milliseconds 10
        }

        # Control-path latency straight out of the engine's own timestamps.
        $controlMs = $null
        if ($suspendAt -and $resumeAt) {
            $ts = { param($line) if ($line -match '^\[(\d{2}):(\d{2}):(\d{2})\.(\d{3})\]') {
                        ([int]$Matches[1] * 3600000) + ([int]$Matches[2] * 60000) + ([int]$Matches[3] * 1000) + [int]$Matches[4]
                    } else { $null } }
            $a = & $ts $suspendAt
            $b = & $ts $resumeAt
            if ($null -ne $a -and $null -ne $b) { $controlMs = $b - $a }
        }

        # Did the engine actually resume playback (state check)?
        $stateAfter = $null
        try { $stateAfter = Send-K6wpIpc -Cmd 'get_state' -TimeoutMs 2000 } catch { }

        $results.Add([pscustomobject]@{
            run = $i
            control_path_ms = $controlMs
            visual_change_ms = $visualMs
            resume_log_line = $resumeAt
            paused_after_resume = $(if ($stateAfter -and $stateAfter.ok) { [bool]$stateAfter.state.paused } else { $null })
            video_after_resume = $(if ($stateAfter -and $stateAfter.ok) { $stateAfter.state.video } else { $null })
            data_dir = $dataDir
        })
        Write-Host ("  control-path {0} ms, visual-change {1} ms, paused_after_resume={2}" -f `
            $controlMs, $visualMs, $results[$results.Count - 1].paused_after_resume)

        try { if (-not $proc.HasExited) { Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue } } catch { }
        try { $proc.WaitForExit(5000) | Out-Null } catch { }
    }
}
finally {
    if (-not $SkipMinimize) { Restore-DesktopWindows -Saved $savedWindows }
}

$control = @($results | ForEach-Object { $_.control_path_ms } | Where-Object { $null -ne $_ })
$visual = @($results | ForEach-Object { $_.visual_change_ms } | Where-Object { $null -ne $_ })

$report = [ordered]@{
    tool = 'bench_resume'
    phase = 0
    date = (Get-Date -Format 'yyyy-MM-ddTHH:mm:ssK')
    label = 'live-proven'
    engine = $EngineExe
    video = $video
    runs = $Runs
    hw = $hw
    method = 'real engine.exe; --simulate-suspend-after-ms posts the real PBT_APMSUSPEND/PBT_APMRESUMEAUTOMATIC path; control latency = the engine log timestamps; visual latency = GDI screen-sample diff until the desktop changes again'
    control_path_ms = $(if ($control.Count -gt 0) { Get-Stats $control } else { $null })
    visual_change_ms = $(if ($visual.Count -gt 0) { Get-Stats $visual } else { $null })
    caveats = @(
        'control_path_ms is the engine log timestamp delta between the suspend and resume markers. The engine posts the resume 2000 ms after the suspend, so ~2000 ms is the EXPECTED control value here, not the engine reacting to a real sleep.',
        'visual_change_ms is a PROXY lower bound: it depends on the GDI screen-capture path and on the video content changing. A still frame would read as "no change".',
        'Neither number is the duration of a real machine sleep; a real suspend/resume needs the hardware path and cannot be simulated faithfully in-process.'
    )
    raw = $results
}

$json = $report | ConvertTo-Json -Depth 12
$outFull = [System.IO.Path]::GetFullPath($OutJson)
$outDir = [System.IO.Path]::GetDirectoryName($outFull)
if (-not (Test-Path -LiteralPath $outDir)) { New-Item -ItemType Directory -Force -Path $outDir | Out-Null }
$json | Set-Content -LiteralPath $outFull -Encoding UTF8
Write-Host "bench_resume: wrote $outFull"
