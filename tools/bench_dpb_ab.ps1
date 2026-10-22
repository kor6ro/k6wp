# tools/bench_dpb_ab.ps1 - task 1.4 (D3): the DPB A/B.
#
# The A/B: the SAME video, twice.
#   A: the source with a deep reference list (the 16-ref 4K corpus).
#   B: the same video after the compressor re-encoded it with the pinned
#      DPB (ref=2).
# The metric is the C4 pair (private bytes + GPU shared memory) plus the
# playback rate and the frame counters - a run whose rate is outside
# 0.97-1.03 is invalid.
#
# The two arms are measured ALTERNATELY inside ONE session (the GPU
# numbers drift up to 1.5x between sessions), so the comparison is A vs B,
# never A vs a stored number.
[CmdletBinding()]
param(
    [string]$EngineExe = '',
    [string]$FileA = '',
    [string]$FileB = '',
    [int]$Runs = 3,
    [int]$SettleSeconds = 6,
    [int]$SampleSeconds = 8,
    [string]$OutJson = ''
)
$ErrorActionPreference = 'Stop'
$script:Here = Split-Path -Parent $MyInvocation.MyCommand.Path
. (Join-Path $script:Here 'bench_lib.ps1')
# C8: the A/B replaces the wallpaper (the engine plays another file), so the
# live session is snapshotted and restored - and the user's engine is
# restarted in the same `finally` (the A/B stops it to start its own).
. (Join-Path $script:Here 'session_snapshot.ps1')

if ([string]::IsNullOrWhiteSpace($EngineExe)) { $EngineExe = Join-Path $script:Here '..\build\release\engine.exe' }
if ([string]::IsNullOrWhiteSpace($FileA)) { $FileA = Join-Path $script:Here '..\tests\corpus\highdpb_4k.mp4' }
if ([string]::IsNullOrWhiteSpace($FileB)) { $FileB = Join-Path $script:Here '..\build\dpb-4k.mp4' }
if ([string]::IsNullOrWhiteSpace($OutJson)) { $OutJson = Join-Path $script:Here '..\docs\bench\dpb-ab.json' }
$engine = [System.IO.Path]::GetFullPath($EngineExe)
foreach ($f in @($FileA, $FileB)) { if (-not (Test-Path -LiteralPath $f)) { throw "missing: $f" } }

function Measure-Arm([string]$File, [string]$Tag) {
    # The singleton mutex is released ASYNCHRONOUSLY when the previous
    # engine dies, so a start can lose the race and exit immediately.
    for ($i = 0; $i -lt 40; $i++) {
        if (-not (Get-Process engine -ErrorAction SilentlyContinue)) { break }
        Start-Sleep -Milliseconds 250
    }
    Start-Sleep -Milliseconds 800
    $dataDir = Join-Path $env:TEMP ("k6wp-dpb-" + [Guid]::NewGuid().ToString('N').Substring(0, 8))
    New-Item -ItemType Directory -Force -Path (Join-Path $dataDir 'K6WP') | Out-Null
    $csv = Join-Path $dataDir 'counters.csv'
    $old = $env:LOCALAPPDATA
    try {
        $env:LOCALAPPDATA = $dataDir
        $p = Start-Process -FilePath $engine -PassThru -WindowStyle Hidden -ArgumentList @(
            "--video=$File", "--dump-mpv-counters=$csv", '--dump-mpv-counters-count=30',
            '--dump-mpv-counters-interval-ms=1000', '--exit-after-ms=90000')
    }
    finally { $env:LOCALAPPDATA = $old }
    if (-not (Wait-K6wpEngine -TimeoutMs 25000)) {
        try { if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force } } catch { }
        throw "the engine did not answer the pipe ($Tag)"
    }
    $saved = Get-DesktopWindows
    try {
        Start-Sleep -Seconds $SettleSeconds
        $tp = Start-TypeperfSampler -ProcessId $p.Id -Seconds ($SampleSeconds + 10) -ProcessName 'engine'
        $series = @(Get-ProcessSeries -ProcessId $p.Id -Seconds $SampleSeconds -IntervalMs 1000)
        $gpu = @(Read-TypeperfCsv -Path $tp.Csv)
        try { if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue } } catch { }
        $rows = @(Import-Csv -LiteralPath $csv -ErrorAction SilentlyContinue)
        # The playback rate + the frame counters, from the mpv counters.
        $media = 0.0; $prev = $null; $frames = 0.0; $prevF = $null
        foreach ($r in $rows) {
            $t = $r.'playback-time'
            if ($t -and $t -ne 'na') { $tv = [double]$t; if ($null -ne $prev) { $d = $tv - $prev; if ($d -lt 0) { $media += $tv } else { $media += $d } }; $prev = $tv }
            $f = $r.'estimated-frame-number'
            if ($f -and $f -ne 'na') { $fv = [double]$f; if ($null -ne $prevF) { $d = $fv - $prevF; if ($d -lt 0) { $frames += $fv } else { $frames += $d } }; $prevF = $fv }
        }
        $win = 0.0
        if ($rows.Count -ge 2) {
            $win = ([double]$rows[$rows.Count - 1].elapsed_ms - [double]$rows[0].elapsed_ms) / 1000.0
        }
        $gpuM = @($gpu | ForEach-Object { $_.gpu_shared_mb } | Where-Object { $null -ne $_ })
        return [pscustomobject]@{
            tag = $Tag
            file = (Split-Path -Leaf $File)
            private_mb = (Get-Stats @($series | ForEach-Object { $_.private_mb })).median
            gpu_shared_mb = (Get-Stats $gpuM).median
            gpu_total_pct = (Get-Stats @($gpu | ForEach-Object { $_.gpu_total_pct } | Where-Object { $null -ne $_ })).median
            cpu_pct = (Get-Stats @($series | ForEach-Object { $_.cpu_pct })).median
            max_thread_cpu_pct = (Get-Stats @($series | ForEach-Object { $_.max_thread_cpu_pct })).median
            playback_rate = $(if ($win -gt 0) { [math]::Round($media / $win, 4) } else { $null })
            delivered_fps = $(if ($win -gt 0) { [math]::Round($frames / $win, 2) } else { $null })
            vo_delayed = $(if ($rows.Count -ge 2 -and $rows[$rows.Count - 1].'vo-delayed-frame-count' -ne 'na') { [double]$rows[$rows.Count - 1].'vo-delayed-frame-count' - [double]$rows[0].'vo-delayed-frame-count' } else { $null })
        }
    }
    finally { Restore-DesktopWindows -Saved $saved }
}

$busy = @(Get-Process engine -ErrorAction SilentlyContinue)
if ($busy.Count -gt 0) { throw "an engine.exe is already running" }
$results = @()
for ($i = 1; $i -le $Runs; $i++) {
    foreach ($arm in @('A', 'B')) {
        $file = if ($arm -eq 'A') { $FileA } else { $FileB }
        $r = Measure-Arm -File $file -Tag "$arm"
        $results += $r
        Write-Host ("  run {0} arm {1} ({2}): private={3} MB shared={4} MB rate={5} fps={6} delayed={7}" -f `
                $i, $arm, $r.file, $r.private_mb, $r.gpu_shared_mb, $r.playback_rate, $r.delivered_fps, $r.vo_delayed)
    }
}
$a = @($results | Where-Object { $_.tag -eq 'A' })
$b = @($results | Where-Object { $_.tag -eq 'B' })
$report = [ordered]@{
    tool = 'bench_dpb_ab'
    phase = 1
    date = (Get-Date -Format 'yyyy-MM-ddTHH:mm:ssK')
    label = 'live-proven (one machine)'
    method = 'ABAB alternating inside one session; C4 metrics (private bytes + GPU shared); the playback rate and the frame counters travel with every number'
    file_a = $FileA
    file_b = $FileB
    runs = $Runs
    a = [ordered]@{
          private_mb = (Get-Stats @($a | ForEach-Object { $_.private_mb })).median
          private_band_mb = (Get-Stats @($a | ForEach-Object { $_.private_mb })).band
          gpu_shared_band_mb = (Get-Stats @($a | ForEach-Object { $_.gpu_shared_mb })).band
          n = @($a).Count
        gpu_shared_mb = (Get-Stats @($a | ForEach-Object { $_.gpu_shared_mb })).median
        playback_rate = (Get-Stats @($a | ForEach-Object { $_.playback_rate })).median
    }
    b = [ordered]@{
          private_mb = (Get-Stats @($b | ForEach-Object { $_.private_mb })).median
          private_band_mb = (Get-Stats @($b | ForEach-Object { $_.private_mb })).band
          gpu_shared_band_mb = (Get-Stats @($b | ForEach-Object { $_.gpu_shared_mb })).band
          n = @($b).Count
        gpu_shared_mb = (Get-Stats @($b | ForEach-Object { $_.gpu_shared_mb })).median
        playback_rate = (Get-Stats @($b | ForEach-Object { $_.playback_rate })).median
    }
    results = $results
}
$json = $report | ConvertTo-Json -Depth 8
$outFull = [System.IO.Path]::GetFullPath($OutJson)
$outDir = [System.IO.Path]::GetDirectoryName($outFull)
if (-not (Test-Path -LiteralPath $outDir)) { New-Item -ItemType Directory -Force -Path $outDir | Out-Null }
$json | Set-Content -LiteralPath $outFull -Encoding UTF8
Write-Host ""
Write-Host ("A (16 refs)  : private {0} MB, GPU shared {1} MB, rate {2}" -f $report.a.private_mb, $report.a.gpu_shared_mb, $report.a.playback_rate)
Write-Host ("B (pinned 2) : private {0} MB, GPU shared {1} MB, rate {2}" -f $report.b.private_mb, $report.b.gpu_shared_mb, $report.b.playback_rate)
Write-Host "bench_dpb_ab: wrote $outFull"