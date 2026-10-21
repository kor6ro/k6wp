# tools/bench_baseline_v2.ps1 - the Phase 1 baseline v2.
#
# WHY A SECOND BASELINE
#   The Phase 0 baseline (v1) was taken BEFORE the pacing fix, and the
#   pacing fix changed the PLAYBACK RATE: the v1 build played the wallpaper
#   at 0.70-0.75x real time (a quarter slow) and the fixed build plays it
#   at 0.98x. A CPU/GPU delta between v1 and a later build is therefore
#   CONFOUNDED - the later build does more work per second of media, not
#   less. Quoting that delta as a release claim is not allowed.
#
#   Baseline v2 is the engine at the commit AFTER the pacing fix and BEFORE
#   the long-pause unload (340da84): the reference for every Phase 1 and
#   Phase 2/3 number. Baseline v1 is ARCHIVED, not deleted.
#
# THE NORMALIZED METRICS
#   cpu_pct_per_fps   = the process CPU% divided by the frames actually
#                       delivered per second
#   gpu_pct_per_fps   = the GPU total% divided by the same
#   They are the numbers that stay comparable when the playback rate moves.
#   Every number in the output carries the playback rate and the frame
#   counters, so a run whose rate is outside 0.97-1.03 is invalid.
#
# Usage:
#   powershell -NoProfile -ExecutionPolicy Bypass -File tools\bench_baseline_v2.ps1 `
#     -EngineExe <engine.exe> -Runs 5 -OutJson docs\bench\phase1-baseline-v2.json
[CmdletBinding()]
param(
    [string]$EngineExe = '',
    [string]$CorpusDir = '',
    [int]$Runs = 5,
    [string]$OutJson = '',
    [string]$Label = 'live-proven (one machine)'
)
$ErrorActionPreference = 'Stop'
$script:Here = Split-Path -Parent $MyInvocation.MyCommand.Path
. (Join-Path $script:Here 'bench_lib.ps1')
if ([string]::IsNullOrWhiteSpace($EngineExe)) { $EngineExe = Join-Path $script:Here '..\build\release\engine.exe' }
if ([string]::IsNullOrWhiteSpace($CorpusDir)) { $CorpusDir = Join-Path $script:Here '..\tests\corpus' }
if ([string]::IsNullOrWhiteSpace($OutJson)) { $OutJson = Join-Path $script:Here '..\docs\bench\phase1-baseline-v2.json' }
$engine = [System.IO.Path]::GetFullPath($EngineExe)
$video1080 = Join-Path $CorpusDir 'anime.mp4'
$video4k = Join-Path $CorpusDir 'highdpb_4k.mp4'
foreach ($v in @($video1080, $video4k)) { if (-not (Test-Path -LiteralPath $v)) { throw "missing corpus: $v" } }

# The states mirror the Phase 0 harness (bench_engine.ps1) so the two
# baselines are comparable.
$states = @(
    [pscustomobject]@{ name = 'idle'; video = ''; settle_ms = 4000; sample_s = 10; pause = $false },
    [pscustomobject]@{ name = 'playback_1080p'; video = $video1080; settle_ms = 5000; sample_s = 12; pause = $false },
    [pscustomobject]@{ name = 'playback_4k'; video = $video4k; settle_ms = 6000; sample_s = 10; pause = $false },
    [pscustomobject]@{ name = 'paused_30s'; video = $video1080; settle_ms = 5000; sample_s = 12; pause = $true }
)

function Convert-Counters([object[]]$Rows) {
    # The counters are cumulative and estimated-frame-number RESETS on every
    # loop, so the frames consumed are the sum of the positive increments
    # plus the index at each reset.
    if ($null -eq $Rows -or $Rows.Count -lt 2) { return $null }
    $frames = 0.0; $prev = $null; $resets = 0
    foreach ($row in $Rows) {
        $v = $row.'estimated-frame-number'
        if ($null -eq $v -or $v -eq 'na' -or $v -eq '') { continue }
        $v = [double]$v
        if ($null -eq $prev) { $prev = $v; continue }
        $d = $v - $prev
        if ($d -lt 0) { $resets++; $frames += $v } else { $frames += $d }
        $prev = $v
    }
    $media = 0.0; $prevM = $null
    foreach ($row in $Rows) {
        $v = $row.'playback-time'
        if ($null -eq $v -or $v -eq 'na' -or $v -eq '') { continue }
        $v = [double]$v
        if ($null -eq $prevM) { $prevM = $v; continue }
        $d = $v - $prevM
        if ($d -lt 0) { $media += $v } else { $media += $d }
        $prevM = $v
    }
    $e0 = 0.0; $e1 = 0.0
    [void][double]::TryParse([string]$Rows[0].elapsed_ms, [ref]$e0)
    [void][double]::TryParse([string]$Rows[$Rows.Count - 1].elapsed_ms, [ref]$e1)
    $win = ($e1 - $e0) / 1000.0
    $out = [ordered]@{
        samples = $Rows.Count
        window_seconds = [math]::Round($win, 3)
        frames_consumed = [math]::Round($frames, 0)
        delivered_fps = $(if ($win -gt 0) { [math]::Round($frames / $win, 2) } else { $null })
        playback_rate = $(if ($win -gt 0) { [math]::Round($media / $win, 4) } else { $null })
        loops = $resets
    }
    foreach ($c in @('frame-drop-count', 'decoder-frame-drop-count', 'mistimed-frame-count', 'vo-delayed-frame-count')) {
        $a = $Rows[0].$c; $b = $Rows[$Rows.Count - 1].$c
        $x = 0.0; $y = 0.0
        if ([double]::TryParse([string]$a, [ref]$x) -and [double]::TryParse([string]$b, [ref]$y)) {
            $out[$c] = $y - $x
        } else { $out[$c] = $null }
    }
    return $out
}

function Invoke-StateRun {
    param($State, [int]$Run)
    for ($i = 0; $i -lt 40; $i++) {
        if (-not (Get-Process engine -ErrorAction SilentlyContinue)) { break }
        Start-Sleep -Milliseconds 250
    }
    Start-Sleep -Milliseconds 700
    $dataDir = Join-Path $env:TEMP ("k6wp-v2-" + [Guid]::NewGuid().ToString('N').Substring(0, 8))
    New-Item -ItemType Directory -Force -Path (Join-Path $dataDir 'K6WP') | Out-Null
    $csv = Join-Path $dataDir 'counters.csv'
    $args = @("--dump-mpv-counters=$csv", '--dump-mpv-counters-count=60',
              '--dump-mpv-counters-interval-ms=1000', '--exit-after-ms=180000')
    if ($State.video) { $args = @("--video=$($State.video)") + $args }
    $old = $env:LOCALAPPDATA
    try {
        $env:LOCALAPPDATA = $dataDir
        $p = Start-Process -FilePath $engine -PassThru -WindowStyle Hidden -ArgumentList $args
    }
    finally { $env:LOCALAPPDATA = $old }
    if (-not (Wait-K6wpEngine -TimeoutMs 25000)) {
        try { if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force } } catch { }
        throw "the engine did not answer the pipe ($($State.name))"
    }
    $saved = Get-DesktopWindows
    try {
        Start-Sleep -Milliseconds $State.settle_ms
        if ($State.pause) {
            [void](Send-K6wpIpc -Cmd 'pause' -TimeoutMs 3000)
            Start-Sleep -Seconds 30
        }
        $tp = Start-TypeperfSampler -ProcessId $p.Id -Seconds ($State.sample_s + 20) -ProcessName 'engine'
        $series = @(Get-ProcessSeries -ProcessId $p.Id -Seconds $State.sample_s -IntervalMs 1000)
        $gpu = @(Read-TypeperfCsv -Path $tp.Csv)
        try { if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue } } catch { }
        $counters = Convert-Counters @(Import-Csv -LiteralPath $csv -ErrorAction SilentlyContinue)
        $p.Refresh()
        $priv = @($series | ForEach-Object { $_.private_mb })
        $cpu = @($series | ForEach-Object { $_.cpu_pct })
        $hot = @($series | ForEach-Object { $_.max_thread_cpu_pct })
        $gpuT = @($gpu | ForEach-Object { $_.gpu_total_pct } | Where-Object { $null -ne $_ })
        $gpuD = @($gpu | ForEach-Object { $_.gpu_decode_pct } | Where-Object { $null -ne $_ })
        $gpu3 = @($gpu | ForEach-Object { $_.gpu_3d_pct } | Where-Object { $null -ne $_ })
        $gpuM = @($gpu | ForEach-Object { $_.gpu_shared_mb } | Where-Object { $null -ne $_ })
        $cpuMed = (Get-Stats $cpu).median
        $gpuMed = (Get-Stats $gpuT).median
        $fps = $counters['delivered_fps']
        return [ordered]@{
            state = $State.name
            run = $Run
            private_mb = (Get-Stats $priv).median
            ws_mb = (Get-Stats @($series | ForEach-Object { $_.ws_mb })).median
            cpu_pct = $cpuMed
            max_thread_cpu_pct = (Get-Stats $hot).median
            gpu_total_pct = $gpuMed
            gpu_decode_pct = (Get-Stats $gpuD).median
            gpu_3d_pct = (Get-Stats $gpu3).median
            gpu_shared_mb = (Get-Stats $gpuM).median
            wakeups_per_sec = $null
            delivered_fps = $fps
            playback_rate = $counters['playback_rate']
            vo_delayed_frames = $counters['vo-delayed-frame-count']
            vo_dropped_frames = $counters['frame-drop-count']
            decoder_dropped_frames = $counters['decoder-frame-drop-count']
            mistimed_frames = $counters['mistimed-frame-count']
            # THE NORMALIZED METRICS: the cost per DELIVERED frame, so the
            # numbers stay comparable when the playback rate moves.
            cpu_pct_per_fps = $(if ($null -ne $fps -and $fps -gt 0) { [math]::Round($cpuMed / $fps, 5) } else { $null })
            gpu_pct_per_fps = $(if ($null -ne $fps -and $fps -gt 0) { [math]::Round($gpuMed / $fps, 5) } else { $null })
        }
    }
    finally { Restore-DesktopWindows -Saved $saved }
}

$all = New-Object System.Collections.Generic.List[object]
$busy = @(Get-Process engine -ErrorAction SilentlyContinue)
if ($busy.Count -gt 0) { throw "an engine.exe is already running" }

foreach ($state in $states) {
    Write-Host "== $($state.name) =="
    for ($run = 1; $run -le $Runs; $run++) {
        $r = Invoke-StateRun -State $state -Run $run
        $all.Add($r)
        Write-Host ("  run {0}: private={1} MB cpu={2}% (max thread {3}%) gpu={4}% (dec {5}%, 3d {6}%) shared={7} MB rate={8} fps={9} cpu/fps={10} gpu/fps={11} delayed={12}" -f `
                $run, $r.private_mb, $r.cpu_pct, $r.max_thread_cpu_pct, $r.gpu_total_pct, $r.gpu_decode_pct,
                $r.gpu_3d_pct, $r.gpu_shared_mb, $r.playback_rate, $r.delivered_fps,
                $r.cpu_pct_per_fps, $r.gpu_pct_per_fps, $r.vo_delayed_frames)
    }
}

# The aggregate: the medians over the runs of each state.
$agg = [ordered]@{}
foreach ($state in $states) {
    $rows = @($all | Where-Object { $_.state -eq $state.name })
    if ($rows.Count -eq 0) { continue }
    $a = [ordered]@{ runs = $rows.Count }
    foreach ($k in @('private_mb', 'ws_mb', 'cpu_pct', 'max_thread_cpu_pct', 'gpu_total_pct',
                     'gpu_decode_pct', 'gpu_3d_pct', 'gpu_shared_mb', 'delivered_fps',
                     'playback_rate', 'vo_delayed_frames', 'vo_dropped_frames',
                     'decoder_dropped_frames', 'mistimed_frames', 'cpu_pct_per_fps', 'gpu_pct_per_fps')) {
        $vals = @($rows | ForEach-Object { $_.$k } | Where-Object { $null -ne $_ })
        if ($vals.Count -eq 0) { $a[$k] = $null; continue }
        $s = Get-Stats $vals
        $a[$k] = $s.median
    }
    $rates = @($rows | ForEach-Object { $_.playback_rate } | Where-Object { $null -ne $_ })
    $a['playback_rate_valid'] = ($rates.Count -gt 0 -and (@($rates | Where-Object { $_ -lt 0.97 -or $_ -gt 1.03 }).Count -eq 0))
    $agg[$state.name] = $a
}

$report = [ordered]@{
    tool = 'bench_baseline_v2'
    phase = 1
    date = (Get-Date -Format 'yyyy-MM-ddTHH:mm:ssK')
    label = $Label
    baseline = 'v2 - the engine AFTER the pacing fix (1.2) and BEFORE the long-pause unload (1.1)'
    baseline_commit = '340da84'
    baseline_v1 = 'docs/bench/phase0.json (ARCHIVED - taken before the pacing fix, so its playback rate is 0.70-0.75 and its CPU/GPU are NOT comparable)'
    engine = $engine
    runs = $Runs
    method = 'the same four states as the Phase 0 harness, plus the live mpv frame counters (playback rate, delivered fps, vo-delayed / vo-dropped / decoder-dropped / mistimed) and the normalized metrics (cpu_pct_per_fps, gpu_pct_per_fps)'
    metric_notes = @{
        cpu_pct_per_fps = 'process CPU% / delivered frames per second'
        gpu_pct_per_fps = 'GPU total% / delivered frames per second'
        validity = 'a run whose playback_rate is outside 0.97-1.03 is INVALID (the brief forbids quoting it)'
    }
    aggregate = $agg
    runs_detail = $all
}
$json = $report | ConvertTo-Json -Depth 10
$outFull = [System.IO.Path]::GetFullPath($OutJson)
$outDir = [System.IO.Path]::GetDirectoryName($outFull)
if (-not (Test-Path -LiteralPath $outDir)) { New-Item -ItemType Directory -Force -Path $outDir | Out-Null }
$json | Set-Content -LiteralPath $outFull -Encoding UTF8
Write-Host ""
Write-Host "bench_baseline_v2: wrote $outFull"
