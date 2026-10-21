# tools/bench_engine.ps1 - Phase 0 engine sampler (n>=5 per state).
#
# States: idle, playback_1080p, playback_4k, paused_30s.
# Measures: private bytes, working set, CPU %, GPU utilization per engine type,
# GPU dedicated/shared memory, thread context-switch rate (wakeups/s).
#
# Method: real engine.exe, isolated %LOCALAPPDATA%, windows minimized so the
# injected wallpaper is not occluded, typeperf CSV for GPU + wakeups, .NET
# process counters for RAM/CPU. No code instrumentation is used.
#
# Usage:
#   powershell -NoProfile -ExecutionPolicy Bypass -File tools\bench_engine.ps1 `
#     -Runs 5 -OutJson docs\bench\phase0.json
#
# Requires: no other engine.exe (singleton mutex), vendor corpus in tests/corpus.

[CmdletBinding()]
param(
    [string]$EngineExe = '',
    [string]$CorpusDir = '',
    [int]$Runs = 5,
    [string]$OutJson = '',
    # NOT "States": PowerShell variables are case-insensitive, so a
    # $States parameter would be the SAME variable as the local $states
    # array below, and assigning the array would clobber the filter.
    [string[]]$OnlyStates = @(),
    [switch]$SkipMinimize
)

$ErrorActionPreference = 'Stop'
$script:Here = Split-Path -Parent $MyInvocation.MyCommand.Path
. (Join-Path $script:Here 'bench_lib.ps1')
if ([string]::IsNullOrWhiteSpace($EngineExe)) { $EngineExe = Join-Path $script:Here '..\build\release\engine.exe' }
if ([string]::IsNullOrWhiteSpace($CorpusDir)) { $CorpusDir = Join-Path $script:Here '..\tests\corpus' }
if ([string]::IsNullOrWhiteSpace($OutJson)) { $OutJson = Join-Path $script:Here '..\docs\bench\phase0.json' }

if ($Runs -lt 1) { throw "Runs must be >= 1" }
if (-not (Test-Path -LiteralPath $EngineExe)) { throw "engine not found: $EngineExe" }
$busy = @(Get-Process engine -ErrorAction SilentlyContinue)
if ($busy.Count -gt 0) {
    throw ("another engine.exe is running (pid {0}); stop it first (K6WP.exe --stop)" -f (($busy | ForEach-Object { $_.Id }) -join ','))
}

$video1080 = Join-Path $CorpusDir 'anime.mp4'
$video4k = Join-Path $CorpusDir 'highdpb_4k.mp4'
if (-not (Test-Path -LiteralPath $video1080)) { throw "missing corpus: $video1080 (run tools/make_corpus.ps1)" }
if (-not (Test-Path -LiteralPath $video4k)) { throw "missing corpus: $video4k (generate the high-DPB 4K clip, see docs/refactor/baseline-tests.md)" }

$states = @(
    [pscustomobject]@{ name = 'idle'; video = ''; settle_ms = 4000; sample_s = 8; pause_after_ms = 0; run_ms = 17000; note = 'no video, no slots' },
    [pscustomobject]@{ name = 'playback_1080p'; video = $video1080; settle_ms = 5000; sample_s = 12; pause_after_ms = 0; run_ms = 22000; note = 'anime.mp4 1080p30 H.264, 1 slot' },
    [pscustomobject]@{ name = 'playback_4k'; video = $video4k; settle_ms = 6000; sample_s = 10; pause_after_ms = 0; run_ms = 21000; note = 'highdpb_4k.mp4 4K30 H.264, 16 refs, 1 slot' },
    [pscustomobject]@{ name = 'paused_30s'; video = $video1080; settle_ms = 5000; sample_s = 12; pause_after_ms = 5000; run_ms = 56000; note = 'play 5s, pause, hold 30s, then sample' }
)
# -OnlyStates filters the sweep (used by the A/B experiments, which only need
# the playback state). An unknown name is an error, not a silent no-op.
if ($OnlyStates.Count -gt 0) {
    # The value may arrive as ONE comma-joined string (a nested call through
    # & operator flattens a string[] into a single string), so split it.
    $wanted = @($OnlyStates | ForEach-Object { ([string]$_) -split ',' } | ForEach-Object { $_.Trim() } | Where-Object { $_ })
    $known = @($states | ForEach-Object { $_.name })
    foreach ($s in $wanted) {
        if ($known -notcontains $s) { throw "unknown state '$s' (known: $($known -join ', '))" }
    }
    $states = @($states | Where-Object { $wanted -contains $_.name })
}

$hw = Get-HwInventory
$savedWindows = @()
if (-not $SkipMinimize) {
    Write-Host "bench_engine: minimizing top-level windows (restored at the end)"
    $savedWindows = Get-DesktopWindows
}
$results = New-Object System.Collections.Generic.List[object]
$logsDir = Join-Path $script:Here '..\build\bench-logs'
New-Item -ItemType Directory -Force -Path $logsDir | Out-Null

try {
    foreach ($state in $states) {
        for ($i = 1; $i -le $Runs; $i++) {
            $tag = "{0}-run{1}" -f $state.name, $i
            Write-Host "bench_engine: $tag ..."
            $run = $null
            $tp = $null
            $series = @()
            $gpuRows = @()
            $occlusionPause = $false
            $hwdec = ''
            try {
                $dataDir = Join-Path $env:TEMP ("k6wp-bench-{0}-{1}" -f $tag, [Guid]::NewGuid().ToString('N').Substring(0,8))
                $run = Start-K6wpEngine -EngineExe $EngineExe -Video $state.video -DataDir $dataDir -RunMs $state.run_ms
                Start-Sleep -Milliseconds $state.settle_ms

                if ($state.pause_after_ms -gt 0) {
                    Start-Sleep -Milliseconds $state.pause_after_ms
                    $ack = Send-K6wpIpc -Cmd 'pause'
                    if (-not $ack.ok) { throw "pause rejected: $($ack | ConvertTo-Json -Compress)" }
                    Start-Sleep -Seconds 30
                    $st = Send-K6wpIpc -Cmd 'get_state'
                    if (-not $st.ok -or -not $st.state.paused) { throw "engine did not report paused after 30s" }
                }

                $tp = Start-TypeperfSampler -ProcessId $run.Process.Id -Seconds $state.sample_s -ProcessName 'engine'
                $series = @(Get-ProcessSeries -ProcessId $run.Process.Id -Seconds $state.sample_s -IntervalMs 1000)
                try { [void]$tp.Process.WaitForExit(($state.sample_s + 8) * 1000) } catch { }
                $gpuRows = @(Read-TypeperfCsv -Path $tp.Csv)

                try {
                    $log = Get-Content -LiteralPath $run.LogPath -ErrorAction Stop
                    $hwLine = @($log | Select-String 'hwdec-current=' | Select-Object -Last 1)
                    if ($hwLine.Count -gt 0) { $hwdec = $hwLine[0].Line.Trim() }
                    $occlusionPause = @($log | Select-String 'occlusion:.*pausing').Count -gt 0
                } catch { }

                try { Copy-Item -LiteralPath $run.LogPath -Destination (Join-Path $logsDir ($tag + '.log')) -Force } catch { }

                $priv = @($series | ForEach-Object { $_.private_mb })
                $ws = @($series | ForEach-Object { $_.ws_mb })
                $cpu = @($series | ForEach-Object { $_.cpu_pct })
                $hotThread = @($series | ForEach-Object { $_.max_thread_cpu_pct })
                $gpuTotal = @($gpuRows | ForEach-Object { $_.gpu_total_pct } | Where-Object { $null -ne $_ })
                $gpuDecode = @($gpuRows | ForEach-Object { $_.gpu_decode_pct } | Where-Object { $null -ne $_ })
                $gpu3d = @($gpuRows | ForEach-Object { $_.gpu_3d_pct } | Where-Object { $null -ne $_ })
                $gpuShared = @($gpuRows | ForEach-Object { $_.gpu_shared_mb } | Where-Object { $null -ne $_ })
                $wake = @($gpuRows | ForEach-Object { $_.wakeups_per_sec } | Where-Object { $null -ne $_ })

                $result = [pscustomobject]@{
                    state = $state.name
                    run = $i
                    video = $state.video
                    note = $state.note
                    sample_seconds = $state.sample_s
                    samples = $series.Count
                    private_mb = (Get-Stats $priv)
                    ws_mb = (Get-Stats $ws)
                    cpu_pct = (Get-Stats $cpu)
                    # The hottest single thread, as a percentage of ONE core.
                    # The process aggregate hides a spinning thread (C2).
                    max_thread_cpu_pct = (Get-Stats $hotThread)
                    gpu_total_pct = (Get-Stats $gpuTotal)
                    gpu_decode_pct = (Get-Stats $gpuDecode)
                    gpu_3d_pct = (Get-Stats $gpu3d)
                    gpu_shared_mb = (Get-Stats $gpuShared)
                    wakeups_per_sec = (Get-Stats $wake)
                    threads_last = $(if ($series.Count -gt 0) { $series[-1].threads } else { $null })
                    handles_last = $(if ($series.Count -gt 0) { $series[-1].handles } else { $null })
                    hwdec = $hwdec
                    occlusion_pause_seen = $occlusionPause
                    data_dir = $run.DataDir
                }
                $results.Add($result)
                Write-Host ("  priv median {0} MB, cpu mean {1}%, gpu {2}%, wake {3}/s, hwdec={4}" -f `
                    $result.private_mb.median, $result.cpu_pct.mean, $result.gpu_total_pct.median, $result.wakeups_per_sec.median, $result.hwdec)
            }
            finally {
                if ($tp -and $tp.Process -and -not $tp.Process.HasExited) { try { Stop-Process -Id $tp.Process.Id -Force } catch { } }
                if ($run) { Stop-K6wpEngine -Run $run }
            }
        }
    }
}
finally {
    if (-not $SkipMinimize) { Restore-DesktopWindows -Saved $savedWindows }
}

# Aggregate per state: median of the per-run medians, min/max as the noise band.
$aggregate = [ordered]@{}
foreach ($state in $states) {
    $rows = @($results | Where-Object { $_.state -eq $state.name })
    if ($rows.Count -eq 0) { continue }
    $metric = @{}
    foreach ($key in @('private_mb', 'ws_mb', 'cpu_pct', 'max_thread_cpu_pct', 'gpu_total_pct', 'gpu_decode_pct', 'gpu_3d_pct', 'gpu_shared_mb', 'wakeups_per_sec')) {
        $vals = @($rows | ForEach-Object { $_.$key.median } | Where-Object { $null -ne $_ })
        $metric[$key] = Get-Stats $vals
    }
    $aggregate[$state.name] = [ordered]@{
        runs = $rows.Count
        note = $state.note
        video = $state.video
        private_mb = $metric.private_mb
        ws_mb = $metric.ws_mb
        cpu_pct = $metric.cpu_pct
        max_thread_cpu_pct = $metric.max_thread_cpu_pct
        gpu_total_pct = $metric.gpu_total_pct
        gpu_decode_pct = $metric.gpu_decode_pct
        gpu_3d_pct = $metric.gpu_3d_pct
        gpu_shared_mb = $metric.gpu_shared_mb
        wakeups_per_sec = $metric.wakeups_per_sec
        occlusion_pause_seen = @($rows | Where-Object { $_.occlusion_pause_seen }).Count
        hwdec = @($rows | ForEach-Object { $_.hwdec } | Where-Object { $_ } | Select-Object -Last 1)
    }
}

$report = [ordered]@{
    tool = 'bench_engine'
    phase = 0
    date = (Get-Date -Format 'yyyy-MM-dd')
    label = 'live-proven'
    engine = $EngineExe
    runs_per_state = $Runs
    hw = $hw
    method = 'real engine.exe; isolated LOCALAPPDATA; windows minimized; typeperf GPU/wakeup CSV; .NET process counters; n>=5; settle per state'
    states = $aggregate
}

# The summary file carries the per-state aggregate; the raw file carries the
# per-run detail. Writing the SAME document twice would just double the
# repository size and hide which file to look at.
$rawReport = [ordered]@{
    tool = 'bench_engine'
    phase = 0
    date = (Get-Date -Format 'yyyy-MM-dd')
    label = 'live-proven'
    engine = $EngineExe
    runs_per_state = $Runs
    hw = $hw
    method = $report.method
    states = $aggregate
    raw = $results
}

$json = $report | ConvertTo-Json -Depth 12
$rawJson = $rawReport | ConvertTo-Json -Depth 12
$outFull = [System.IO.Path]::GetFullPath($OutJson)
$outDir = [System.IO.Path]::GetDirectoryName($outFull)
if (-not (Test-Path -LiteralPath $outDir)) { New-Item -ItemType Directory -Force -Path $outDir | Out-Null }
$json | Set-Content -LiteralPath $outFull -Encoding UTF8
$rawJson | Set-Content -LiteralPath ([System.IO.Path]::ChangeExtension($outFull, '.raw.json')) -Encoding UTF8
Write-Host "bench_engine: wrote $outFull"
Write-Host "bench_engine: wrote $([System.IO.Path]::ChangeExtension($outFull, '.raw.json'))"
