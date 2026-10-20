# tools/bench_frames.ps1 - engine-side visual parity (render-path) check.
#
# Compares the frames the engine RENDERS (not what the desktop composite
# shows). The engine writes them with the hidden --capture-frames QA flag,
# which uses mpv's own screenshot of the decoded frame.
#
# Why not a screen capture: an always-on-top overlay, a paused engine or a
# covered desktop all make a screen grab record the plain desktop, which
# silently turns a visual-parity gate into a no-op. The engine-side frame
# is immune to all of that.
#
# The corpus clip starts at a deterministic frame (frame 0), so the two
# builds must produce the SAME pixels: any difference is a real change in
# the render path (scaler, colour, decode).
#
# Usage:
#   powershell -NoProfile -ExecutionPolicy Bypass -File tools\bench_frames.ps1 `
#     -BaselineExe build\ab\engine-baseline.exe -CandidateExe build\ab\engine-candidate.exe

[CmdletBinding()]
param(
    [string]$BaselineExe = '',
    [string]$CandidateExe = '',
    [string]$CorpusDir = '',
    [string]$OutDir = '',
    [double]$Tolerance = 3.0
)
$ErrorActionPreference = 'Stop'
$script:Here = Split-Path -Parent $MyInvocation.MyCommand.Path
. (Join-Path $script:Here 'bench_lib.ps1')

if ([string]::IsNullOrWhiteSpace($BaselineExe)) { $BaselineExe = Join-Path $script:Here '..\build\ab\engine-baseline.exe' }
if ([string]::IsNullOrWhiteSpace($CandidateExe)) { $CandidateExe = Join-Path $script:Here '..\build\ab\engine-candidate.exe' }
if ([string]::IsNullOrWhiteSpace($CorpusDir)) { $CorpusDir = Join-Path $script:Here '..\tests\corpus' }
if ([string]::IsNullOrWhiteSpace($OutDir)) { $OutDir = Join-Path $script:Here '..\docs\bench\frames' }
$video = Join-Path $CorpusDir 'anime.mp4'
if (-not (Test-Path -LiteralPath $video)) { throw "missing corpus: $video" }

Add-Type -AssemblyName System.Drawing

function Get-Signature([string]$Path) {
    $bmp = [System.Drawing.Bitmap]::FromFile($Path)
    try {
        $sum = 0.0; $sum2 = 0.0; $rs = 0.0; $gs = 0.0; $bs = 0.0; $n = 0
        for ($y = 0; $y -lt $bmp.Height; $y += 4) {
            for ($x = 0; $x -lt $bmp.Width; $x += 4) {
                $c = $bmp.GetPixel($x, $y)
                $l = 0.2126 * $c.R + 0.7152 * $c.G + 0.0722 * $c.B
                $sum += $l; $sum2 += $l * $l
                $rs += $c.R; $gs += $c.G; $bs += $c.B; $n++
            }
        }
        $mean = $sum / $n
        return [pscustomobject]@{
            w = $bmp.Width; h = $bmp.Height
            mean_luma = [math]::Round($mean, 2)
            stddev = [math]::Round([math]::Sqrt([math]::Max(0, ($sum2 / $n) - ($mean * $mean))), 2)
            mean_r = [math]::Round($rs / $n, 2)
            mean_g = [math]::Round($gs / $n, 2)
            mean_b = [math]::Round($bs / $n, 2)
        }
    }
    finally { $bmp.Dispose() }
}

function Invoke-Capture([string]$Exe, [string]$Tag) {
    $exe = [System.IO.Path]::GetFullPath($Exe)
    if (-not (Test-Path -LiteralPath $exe)) { throw "engine not found: $exe" }
    $dir = Join-Path $OutDir $Tag
    Remove-Item -LiteralPath $dir -Recurse -Force -ErrorAction SilentlyContinue
    New-Item -ItemType Directory -Force -Path $dir | Out-Null
    $prefix = Join-Path $dir 'frame'
    $busy = @(Get-Process engine -ErrorAction SilentlyContinue)
    if ($busy.Count -gt 0) { throw "an engine.exe is already running (pid $($busy.Id -join ','))" }
    $dataDir = Join-Path $env:TEMP ("k6wp-frames-" + [Guid]::NewGuid().ToString('N').Substring(0, 8))
    New-Item -ItemType Directory -Force -Path (Join-Path $dataDir 'K6WP') | Out-Null
    $old = $env:LOCALAPPDATA
    try {
        $env:LOCALAPPDATA = $dataDir
        $p = Start-Process -FilePath $exe -PassThru -WindowStyle Hidden -ArgumentList @(
            "--video=$video", "--capture-frames=$prefix", '--capture-frames-count=3',
            '--capture-frames-interval-ms=700', '--capture-frames-seek=5',
            '--exit-after-ms=15000')
    }
    finally { $env:LOCALAPPDATA = $old }
    if (-not (Wait-K6wpEngine -TimeoutMs 15000)) {
        try { if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force } } catch { }
        throw "${Tag}: engine did not answer the IPC pipe"
    }
    # Wait for the frames to appear (the mpv VO writes them asynchronously).
    $files = @()
    $deadline = (Get-Date).AddSeconds(20)
    while ((Get-Date) -lt $deadline) {
        $files = @(Get-ChildItem -LiteralPath $dir -Filter 'frame*.jpg' -ErrorAction SilentlyContinue |
            Sort-Object Name)
        if ($files.Count -ge 3) { break }
        Start-Sleep -Milliseconds 300
    }
    try { if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue } } catch { }
    if ($files.Count -eq 0) { throw "${Tag}: no frame was written" }
    return $files
}

Write-Host "bench_frames: baseline  = $BaselineExe"
Write-Host "bench_frames: candidate = $CandidateExe"

$baseFiles = Invoke-Capture -Exe $BaselineExe -Tag 'baseline'
$candFiles = Invoke-Capture -Exe $CandidateExe -Tag 'candidate'
Write-Host ("bench_frames: captured {0} baseline frame(s), {1} candidate frame(s)" -f $baseFiles.Count, $candFiles.Count)

$rows = @()
$n = [math]::Min($baseFiles.Count, $candFiles.Count)
for ($i = 0; $i -lt $n; $i++) {
    $a = Get-Signature $baseFiles[$i].FullName
    $b = Get-Signature $candFiles[$i].FullName
    $ba = [System.Drawing.Bitmap]::FromFile($baseFiles[$i].FullName)
    $bb = [System.Drawing.Bitmap]::FromFile($candFiles[$i].FullName)
    try { $diff = Get-BitmapDiff -A $ba -B $bb -Step 4 } finally { $ba.Dispose(); $bb.Dispose() }
    $rows += [pscustomobject]@{
        frame = $i + 1
        size = "$($a.w)x$($a.h)"
        baseline_luma = $a.mean_luma
        candidate_luma = $b.mean_luma
        luma_delta = [math]::Round([math]::Abs($a.mean_luma - $b.mean_luma), 3)
        baseline_rgb = "$($a.mean_r),$($a.mean_g),$($a.mean_b)"
        candidate_rgb = "$($b.mean_r),$($b.mean_g),$($b.mean_b)"
        pixel_diff = [math]::Round($diff, 3)
        pass = ($diff -le $Tolerance)
    }
}
$rows | Format-Table -AutoSize | Out-String | Write-Host

$worst = ($rows | Measure-Object -Property pixel_diff -Maximum).Maximum
$allPass = @($rows | Where-Object { -not $_.pass }).Count -eq 0
Write-Host ("worst pixel diff: {0} (tolerance {1}) -> {2}" -f $worst, $Tolerance, $(if ($allPass) { 'PASS' } else { 'FAIL' }))

$report = [ordered]@{
    tool = 'bench_frames'
    phase = 1
    date = (Get-Date -Format 'yyyy-MM-ddTHH:mm:ssK')
    label = 'live-proven'
    method = 'engine-side rendered frames (hidden --capture-frames QA flag, mpv decoded-frame screenshot); immune to the desktop composite'
    corpus = $video
    tolerance = $Tolerance
    frames = $rows
    worst_pixel_diff = $worst
    pass = $allPass
}
$json = $report | ConvertTo-Json -Depth 8
$outFull = [System.IO.Path]::GetFullPath((Join-Path $OutDir 'frames.json'))
$json | Set-Content -LiteralPath $outFull -Encoding UTF8
Write-Host "bench_frames: wrote $outFull"
if (-not $allPass) { exit 1 }
exit 0
