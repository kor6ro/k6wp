# tools/bench_pacing.ps1 - C1: prove a pacing change costs no QUALITY.
#
# WHAT IT MEASURES
#   For each (engine build, fps cap, display refresh) it runs the real engine
#   on the corpus clip and records:
#     - the live mpv frame-quality counters (drops, mistimed frames, delayed
#       frames, frames consumed, media position)
#     - the per-OS-thread CPU of the engine (the aggregate hides a spinning
#       thread, which is exactly how the desync busy-wait stayed hidden)
#     - the GPU utilization per engine type
#     - private bytes + GPU shared memory (the honest RAM numbers)
#
# WHY ABAB
#   The GPU numbers drift up to 1.5x between sessions, so the two builds are
#   measured ALTERNATELY inside one session and compared with each other -
#   never against a stored absolute.
#
# Usage:
#   powershell -NoProfile -ExecutionPolicy Bypass -File tools\bench_pacing.ps1 `
#     -BaselineExe build\ab\engine-baseline.exe -CandidateExe build\ab\engine-candidate.exe `
#     -FpsCaps 24,30 -Seconds 60 -OutJson docs\bench\pacing.json

[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$BaselineExe,
    [Parameter(Mandatory=$true)][string]$CandidateExe,
    [string]$CorpusDir = '',
    [string[]]$FpsCaps = @('24', '30'),
    [int]$Seconds = 60,
    [int]$RefreshHz = 0,
    [string]$OutJson = '',
    [int]$Repetitions = 2
)
$ErrorActionPreference = 'Stop'
$script:Here = Split-Path -Parent $MyInvocation.MyCommand.Path
. (Join-Path $script:Here 'bench_lib.ps1')
. (Join-Path $script:Here 'session_snapshot.ps1')

if ([string]::IsNullOrWhiteSpace($CorpusDir)) { $CorpusDir = Join-Path $script:Here '..\tests\corpus' }
if ([string]::IsNullOrWhiteSpace($OutJson)) { $OutJson = Join-Path $script:Here '..\docs\bench\pacing.json' }
# -FpsCaps 24,30 arrives as ONE string on the command line; split it.
$FpsCaps = @($FpsCaps | ForEach-Object { ([string]$_) -split ',' } | ForEach-Object { $_.Trim() } | Where-Object { $_ } | ForEach-Object { [int]$_ })
if ($FpsCaps.Count -eq 0) { throw "no fps caps given" }
foreach ($c in $FpsCaps) {
    # Cast BEFORE comparing: with a string on the left, PowerShell compares
    # lexicographically ("30" -gt "240" is TRUE).
    $ci = [int]$c
    if ($ci -lt 1 -or $ci -gt 240) { throw "an fps cap of $ci is not a frame rate" }
}
$BaselineExe = [System.IO.Path]::GetFullPath($BaselineExe)
$CandidateExe = [System.IO.Path]::GetFullPath($CandidateExe)
$video = Join-Path $CorpusDir 'anime.mp4'
if (-not (Test-Path -LiteralPath $video)) { throw "missing corpus: $video" }
foreach ($p in @($BaselineExe, $CandidateExe)) {
    if (-not (Test-Path -LiteralPath $p)) { throw "engine not found: $p" }
}

# --- per-thread CPU sampler (C2) -------------------------------------------
# The process aggregate hides a single spinning thread, so the sampler takes
# the CPU delta of EVERY engine thread and reports the hottest one.
function Get-ThreadCpuDeltas {
    param([int]$ProcessId, [double]$WindowSec)
    $p = Get-Process -Id $ProcessId -ErrorAction Stop
    $before = @{}
    foreach ($t in $p.Threads) { $before[[string]$t.Id] = $t.TotalProcessorTime.TotalMilliseconds }
    Start-Sleep -Milliseconds ([int]($WindowSec * 1000))
    $p.Refresh()
    $out = @()
    foreach ($t in $p.Threads) {
        $k = [string]$t.Id
        if (-not $before.ContainsKey($k)) { continue }
        $deltaMs = $t.TotalProcessorTime.TotalMilliseconds - $before[$k]
        # % of ONE core over the window.
        $out += [pscustomobject]@{
            tid = $t.Id
            cpu_pct_of_core = [math]::Round(($deltaMs / 1000.0 / $WindowSec) * 100, 2)
            state = [string]$t.ThreadState
        }
    }
    return $out
}

function Read-CountersCsv([string]$Path) {
    if (-not (Test-Path -LiteralPath $Path)) { return @() }
    return @(Import-Csv -LiteralPath $Path)
}

function Convert-Counters([object[]]$Rows) {
    # The counters are cumulative, so the interesting value is the DELTA
    # between the first and the last sample.
    if ($Rows.Count -lt 2) { return $null }
    $first = $Rows[0]
    $last = $Rows[$Rows.Count - 1]
    $out = [ordered]@{ samples = $Rows.Count }
    foreach ($col in $first.PSObject.Properties.Name) {
        if ($col -in @('sample', 'elapsed_ms')) { continue }
        $a = $first.$col
        $b = $last.$col
        if ($a -eq 'na' -or $b -eq 'na' -or $a -eq '' -or $b -eq '') { $out[$col] = $null; continue }
        $na = 0.0
        $nb = 0.0
        if ([double]::TryParse([string]$a, [ref]$na) -and [double]::TryParse([string]$b, [ref]$nb)) {
            $out[$col] = $nb - $na
        } else {
            # A non-numeric counter (the "vf" filter chain is a string): keep
            # the last value as text, it is still evidence.
            $out[$col] = [string]$b
        }
    }
    # estimated-frame-number RESETS on every loop (the corpus clip is 30 s
    # and the measurement window is 60 s), so its first-to-last delta is
    # meaningless. The number of frames CONSUMED is the sum of the positive
    # per-sample increments, plus the frame index at each reset - that is
    # the metric C1 needs ("the same number of frames decoded in both
    # modes").
    $frames = 0.0
    $prev = $null
    $resets = 0
    foreach ($row in $Rows) {
        $v = $row.'estimated-frame-number'
        if ($null -eq $v -or $v -eq 'na' -or $v -eq '') { continue }
        $v = [double]$v
        if ($null -eq $prev) { $prev = $v; continue }
        $d = $v - $prev
        if ($d -lt 0) {
            # A loop: the counter went back to the start of the file.
            $resets++
            $frames += $v      # the frames of the finished part
        } else {
            $frames += $d
        }
        $prev = $v
    }
    $out['frames_consumed'] = [math]::Round($frames, 0)
    $out['loops'] = $resets

    # The media position advance over the window, with the loop resets
    # accumulated. Dividing it by the wall-clock window gives the PLAYBACK
    # RATE: 1.0 means the wallpaper runs at real time, 0.75 means it runs a
    # quarter slow (the video is stretched, which is a QUALITY defect the
    # CPU/GPU numbers alone would never show).
    $media = 0.0
    $prevMedia = $null
    foreach ($row in $Rows) {
        $v = $row.'playback-time'
        if ($null -eq $v -or $v -eq 'na' -or $v -eq '') { continue }
        $v = [double]$v
        if ($null -eq $prevMedia) { $prevMedia = $v; continue }
        $d = $v - $prevMedia
        if ($d -lt 0) { $media += $v } else { $media += $d }
        $prevMedia = $v
    }
    $windowSec = 0.0
    if ($Rows.Count -ge 2) {
        $e0 = 0.0; $e1 = 0.0
        [void][double]::TryParse([string]$Rows[0].elapsed_ms, [ref]$e0)
        [void][double]::TryParse([string]$Rows[$Rows.Count - 1].elapsed_ms, [ref]$e1)
        $windowSec = ($e1 - $e0) / 1000.0
    }
    $out['media_seconds_advanced'] = [math]::Round($media, 3)
    $out['window_seconds'] = [math]::Round($windowSec, 3)
    $out['playback_rate'] = $(if ($windowSec -gt 0) { [math]::Round($media / $windowSec, 4) } else { $null })
    $out['decoded_fps'] = $(if ($windowSec -gt 0) { [math]::Round($frames / $windowSec, 2) } else { $null })
    return $out
}

function Invoke-PacingRun {
    param([string]$Exe, [string]$Tag, [int]$FpsCap)
    $dataDir = Join-Path $env:TEMP ("k6wp-pace-" + [Guid]::NewGuid().ToString('N').Substring(0, 8))
    New-Item -ItemType Directory -Force -Path (Join-Path $dataDir 'K6WP') | Out-Null
    $csv = Join-Path $dataDir 'counters.csv'
    $busy = @(Get-Process engine -ErrorAction SilentlyContinue)
    if ($busy.Count -gt 0) {
        # A previous run's engine may still be releasing the singleton mutex.
        foreach ($e in $busy) { try { Stop-Process -Id $e.Id -Force } catch { } }
        $deadline = (Get-Date).AddSeconds(10)
        while ((Get-Date) -lt $deadline -and (Get-Process engine -ErrorAction SilentlyContinue)) {
            Start-Sleep -Milliseconds 200
        }
        if (Get-Process engine -ErrorAction SilentlyContinue) {
            throw "an engine.exe is still running; cannot start a clean run"
        }
        Start-Sleep -Milliseconds 500
    }
    $old = $env:LOCALAPPDATA
    $started = $false
    $tail = ''
    for ($attempt = 1; $attempt -le 4 -and -not $started; $attempt++) {
        $env:LOCALAPPDATA = $dataDir
        try {
            $p = Start-Process -FilePath $Exe -PassThru -WindowStyle Hidden -ArgumentList @(
                "--video=$video", "--fps-cap=$FpsCap", "--dump-mpv-counters=$csv",
                "--dump-mpv-counters-count=$Seconds", "--dump-mpv-counters-interval-ms=1000",
                "--exit-after-ms=$($Seconds * 1000 + 20000)")
        }
        finally { $env:LOCALAPPDATA = $old }
        # The singleton mutex is released ASYNCHRONOUSLY when the previous
        # engine dies, so a start can lose the race and exit with "another
        # instance is already running". Retry a few times.
        if (Wait-K6wpEngine -TimeoutMs 20000) { $started = $true; break }
        $logPath = Join-Path $dataDir 'K6WP\engine.log'
        if (Test-Path -LiteralPath $logPath) {
            $tail = (@(Get-Content -LiteralPath $logPath -Tail 4) -join ' | ')
        }
        try { if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force } } catch { }
        Start-Sleep -Seconds 2
    }
    if (-not $started) {
        throw "${Tag}: engine did not answer the pipe. log: $tail"
    }
    # The engine pauses the decode when a window covers the desktop. Every
    # covering window must be minimized and the engine must be PLAYING
    # before the measurement, or the counters read zero by definition.
    $savedWindows = Get-DesktopWindows
    Start-Sleep -Seconds 5
    $resumed = $false
    $deadline = (Get-Date).AddSeconds(20)
    while ((Get-Date) -lt $deadline) {
        $st = $null
        try { $st = Send-K6wpIpc -Cmd 'get_state' -TimeoutMs 2000 } catch { }
        if ($st -and $st.ok -and -not $st.state.paused) { $resumed = $true; break }
        Start-Sleep -Milliseconds 300
    }
    if (-not $resumed) { Write-Host "    WARNING: the engine reports paused - the counters may read zero" }

    # The GPU counters and the per-thread CPU are sampled in the SAME window.
    $tp = Start-TypeperfSampler -ProcessId $p.Id -Seconds $Seconds -ProcessName 'engine'
    $threads = Get-ThreadCpuDeltas -ProcessId $p.Id -WindowSec $Seconds
    try { [void]$tp.Process.WaitForExit(($Seconds + 15) * 1000) } catch { }
    $gpuRows = @(Read-TypeperfCsv -Path $tp.Csv)
    $counterRows = @(Read-CountersCsv -Path $csv)
    $p.Refresh()
    $priv = [math]::Round($p.PrivateMemorySize64 / 1MB, 2)
    $gpuMem = @($gpuRows | ForEach-Object { $_.gpu_shared_mb } | Where-Object { $null -ne $_ })
    try { if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue } } catch { }

    $hot = @($threads | Sort-Object cpu_pct_of_core -Descending)
    $result = [pscustomobject]@{
        tag = $Tag
        fps_cap = $FpsCap
        max_thread_cpu_pct = $(if ($hot.Count -gt 0) { $hot[0].cpu_pct_of_core } else { $null })
        max_thread_tid = $(if ($hot.Count -gt 0) { $hot[0].tid } else { $null })
        max_thread_state = $(if ($hot.Count -gt 0) { $hot[0].state } else { $null })
        thread_count = $threads.Count
        private_mb = $priv
        gpu_shared_mb = (Get-Stats $gpuMem).median
        gpu_total_pct = (Get-Stats @($gpuRows | ForEach-Object { $_.gpu_total_pct } | Where-Object { $null -ne $_ })).median
        gpu_decode_pct = (Get-Stats @($gpuRows | ForEach-Object { $_.gpu_decode_pct } | Where-Object { $null -ne $_ })).median
        gpu_3d_pct = (Get-Stats @($gpuRows | ForEach-Object { $_.gpu_3d_pct } | Where-Object { $null -ne $_ })).median
        counters = (Convert-Counters $counterRows)
        counters_raw = $counterRows
        vf_chain = $(if ($counterRows.Count -gt 0) { $counterRows[$counterRows.Count - 1].vf } else { '' })
    }
    # The windows stay minimized for the whole experiment; they are restored
    # once, at the end (inside the session-safe finally).
    return $result
}

# --- the experiment ---------------------------------------------------------
$results = New-Object System.Collections.Generic.List[object]
$savedWindows = @()

# C8: the display mode and the wallpaper/lock screen are snapshotted and
# restored automatically, even when a run fails half-way. The covering
# windows are minimized for the measurement and restored in the finally.
Invoke-SessionSafe -Name 'pacing experiment' -RestoreDisplay -RestoreWallpaper -RestoreLockscreen -RestoreExplorer -ScriptBlock {
    try {
        if ($RefreshHz -gt 0) {
            $screen = [System.Windows.Forms.Screen]::PrimaryScreen
            $rc = Set-DisplayRefresh -Device $screen.DeviceName -Width $screen.Bounds.Width -Height $screen.Bounds.Height -Frequency $RefreshHz
            Write-Host "  display refresh -> ${RefreshHz}Hz (rc=$rc)"
            Start-Sleep -Seconds 3
        }
        $script:savedWindows = Get-DesktopWindows
        foreach ($cap in $FpsCaps) {
            for ($rep = 1; $rep -le $Repetitions; $rep++) {
                # ABAB: the two builds alternate inside ONE session.
                foreach ($arm in @('A', 'B')) {
                    $exe = if ($arm -eq 'A') { $BaselineExe } else { $CandidateExe }
                    $name = if ($arm -eq 'A') { 'desync' } else { 'vsync' }
                    Write-Host "  run cap=$cap rep=$rep arm=$arm ($name)"
                    $r = Invoke-PacingRun -Exe $exe -Tag "$name-cap$cap-rep$rep" -FpsCap $cap
                    $results.Add($r)
                    Write-Host ("    max thread CPU {0}% (tid {1}, {2}), private {3} MB, gpu shared {4} MB, decoded {5} fps, rate {6}, drops {7}" -f `
                        $r.max_thread_cpu_pct, $r.max_thread_tid, $r.max_thread_state, $r.private_mb, $r.gpu_shared_mb,
                        $(if ($r.counters -and $null -ne $r.counters['decoded_fps']) { $r.counters['decoded_fps'] } else { 'n/a' }),
                        $(if ($r.counters -and $null -ne $r.counters['playback_rate']) { $r.counters['playback_rate'] } else { 'n/a' }),
                        $(if ($r.counters -and $null -ne $r.counters['frame-drop-count']) { $r.counters['frame-drop-count'] } else { 'n/a' }))
                }
            }
        }
    }
    finally {
        Restore-DesktopWindows -Saved $script:savedWindows
    }
}

function Get-MedianOf {
    param($List, [string]$Prop)
    $vals = @($List | ForEach-Object { $_.$Prop } | Where-Object { $null -ne $_ })
    if ($vals.Count -eq 0) { return $null }
    return (Get-Stats $vals).median
}

# --- the comparison ---------------------------------------------------------
$rows = @()
foreach ($cap in $FpsCaps) {
    $a = @($results | Where-Object { $_.fps_cap -eq $cap -and $_.tag -like 'desync*' })
    $b = @($results | Where-Object { $_.fps_cap -eq $cap -and $_.tag -like 'vsync*' })
    if ($a.Count -eq 0 -or $b.Count -eq 0) { continue }
    foreach ($prop in @('max_thread_cpu_pct', 'private_mb', 'gpu_shared_mb', 'gpu_total_pct', 'gpu_decode_pct', 'gpu_3d_pct')) {
        $rows += [pscustomobject]@{
            fps_cap = $cap
            metric = $prop
            desync = Get-MedianOf $a $prop
            vsync = Get-MedianOf $b $prop
        }
    }
    foreach ($counter in @('frames_consumed', 'decoded_fps', 'loops', 'playback_rate',
                           'media_seconds_advanced', 'frame-drop-count', 'decoder-frame-drop-count',
                           'mistimed-frame-count', 'vo-delayed-frame-count')) {
        $av = @($a | ForEach-Object { $_.counters[$counter] } | Where-Object { $null -ne $_ })
        $bv = @($b | ForEach-Object { $_.counters[$counter] } | Where-Object { $null -ne $_ })
        if ($av.Count -eq 0 -or $bv.Count -eq 0) { continue }
        $rows += [pscustomobject]@{
            fps_cap = $cap
            metric = $counter
            desync = (Get-Stats $av).median
            vsync = (Get-Stats $bv).median
        }
    }
}

$report = [ordered]@{
    tool = 'bench_pacing'
    phase = 1
    date = (Get-Date -Format 'yyyy-MM-ddTHH:mm:ssK')
    label = 'live-proven (one machine)'
    baseline_exe = $BaselineExe
    candidate_exe = $CandidateExe
    corpus = $video
    seconds_per_run = $Seconds
    repetitions = $Repetitions
    refresh_hz = $(if ($RefreshHz -gt 0) { $RefreshHz } else { 'display default' })
    method = 'ABAB alternating inside one session; live mpv counters + per-OS-thread CPU + GPU counters + private/GPU-shared memory'
    runs = $results
    comparison = $rows
}
$json = $report | ConvertTo-Json -Depth 12
$outFull = [System.IO.Path]::GetFullPath($OutJson)
$outDir = [System.IO.Path]::GetDirectoryName($outFull)
if (-not (Test-Path -LiteralPath $outDir)) { New-Item -ItemType Directory -Force -Path $outDir | Out-Null }
$json | Set-Content -LiteralPath $outFull -Encoding UTF8

Write-Host ""
Write-Host ("=" * 100)
Write-Host "  pacing comparison (desync vs vsync)"
Write-Host ("=" * 100)
$rows | Format-Table -AutoSize | Out-String | Write-Host
Write-Host "bench_pacing: wrote $outFull"
