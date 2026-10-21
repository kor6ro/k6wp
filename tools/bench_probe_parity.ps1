# tools/bench_probe_parity.ps1 - can the 97 MB ffprobe be replaced by libmpv?
#
# ffprobe.exe is 96.98 MB of the 389 MB install (about 35 MB of the 148 MB
# ZIP, 24 % on its own). The product only asks it for FIVE fields:
#   format.duration, the first video stream's codec_name / width / height /
#   r_frame_rate, and whether an audio stream exists.
#
# libmpv-2.dll is ALREADY shipped, so probing through it costs zero new
# shipping bytes. This script compares the two answers field by field over
# a corpus, so the swap can be decided on evidence instead of hope.
#
# Usage:
#   powershell -NoProfile -ExecutionPolicy Bypass -File tools\bench_probe_parity.ps1

[CmdletBinding()]
param(
    [string]$EngineExe = '',
    [string]$CorpusDir = '',
    [string]$OutJson = '',
    [int[]]$ExtraFiles = @()
)
$ErrorActionPreference = 'Stop'
$script:Here = Split-Path -Parent $MyInvocation.MyCommand.Path
. (Join-Path $script:Here 'bench_lib.ps1')

if ([string]::IsNullOrWhiteSpace($EngineExe)) { $EngineExe = Join-Path $script:Here '..\build\release\engine.exe' }
if ([string]::IsNullOrWhiteSpace($CorpusDir)) { $CorpusDir = Join-Path $script:Here '..\tests\corpus' }
if ([string]::IsNullOrWhiteSpace($OutJson)) { $OutJson = Join-Path $script:Here '..\docs\bench\probe-parity.json' }
$engine = [System.IO.Path]::GetFullPath($EngineExe)
$ffprobe = (Join-Path $script:Here '..\vendor\ffmpeg\ffprobe.exe')
if (-not (Test-Path -LiteralPath $ffprobe)) { throw "ffprobe not found at $ffprobe" }

$files = @(Get-ChildItem -LiteralPath $CorpusDir -Filter '*.mp4' -File | ForEach-Object { $_.FullName })
$files += $ExtraFiles
if ($files.Count -eq 0) { throw "no corpus files" }

function Get-FfprobeProps([string]$Path) {
    $out = & $ffprobe -v error -show_entries format=duration -show_entries `
        stream=codec_name,codec_type,width,height,r_frame_rate -of json $Path 2>$null | Out-String
    $root = $out | ConvertFrom-Json
    $props = [ordered]@{
        duration = 0.0; codec = ''; width = 0; height = 0; fps = 0.0; has_audio = $false; found_video = $false
    }
    if ($root.format -and $root.format.duration) { $props.duration = [double]$root.format.duration }
    foreach ($s in $root.streams) {
        if ($s.codec_type -eq 'audio') { $props.has_audio = $true; continue }
        if ($s.codec_type -eq 'video' -and -not $props.found_video) {
            $props.found_video = $true
            $props.codec = [string]$s.codec_name
            $props.width = [int]$s.width
            $props.height = [int]$s.height
            if ($s.r_frame_rate -and $s.r_frame_rate -like '*/*') {
                $parts = $s.r_frame_rate -split '/'
                $n = [double]$parts[0]; $d = [double]$parts[1]
                if ($d -ne 0) { $props.fps = $n / $d }
            } elseif ($s.r_frame_rate) { $props.fps = [double]$s.r_frame_rate }
        }
    }
    return $props
}

function Get-MpvProps([string]$Path, [string]$Engine) {
    $json = Join-Path $env:TEMP ("k6wp-props-" + [Guid]::NewGuid().ToString('N').Substring(0, 8) + ".json")
    $busy = @(Get-Process engine -ErrorAction SilentlyContinue)
    if ($busy.Count -gt 0) {
        # The singleton mutex is released asynchronously when the previous
        # engine dies, so a start can lose the race.
        foreach ($e in $busy) { try { Stop-Process -Id $e.Id -Force } catch { } }
        for ($i = 0; $i -lt 40; $i++) {
            if (-not (Get-Process engine -ErrorAction SilentlyContinue)) { break }
            Start-Sleep -Milliseconds 250
        }
        Start-Sleep -Milliseconds 800
    }
    $dataDir = Join-Path $env:TEMP ("k6wp-probe-" + [Guid]::NewGuid().ToString('N').Substring(0, 8))
    New-Item -ItemType Directory -Force -Path (Join-Path $dataDir 'K6WP') | Out-Null
    $old = $env:LOCALAPPDATA
    try {
        $env:LOCALAPPDATA = $dataDir
        $p = Start-Process -FilePath $Engine -PassThru -WindowStyle Hidden -ArgumentList @(
            "--video=$Path", "--dump-media-props=$json", '--exit-after-ms=12000')
    }
    finally { $env:LOCALAPPDATA = $old }
    $deadline = (Get-Date).AddSeconds(20)
    while (-not (Test-Path -LiteralPath $json) -and (Get-Date) -lt $deadline) {
        Start-Sleep -Milliseconds 200
    }
    try { if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue } } catch { }
    if (-not (Test-Path -LiteralPath $json)) { throw "the engine wrote no media props for $Path" }
    $m = Get-Content -LiteralPath $json -Raw | ConvertFrom-Json
    Remove-Item -LiteralPath $json -Force -ErrorAction SilentlyContinue
    $props = [ordered]@{
        duration = 0.0; codec = ''; width = 0; height = 0; fps = 0.0; has_audio = $false; found_video = $false
    }
    if ($m.has_duration) { $props.duration = [double]$m.duration }
    if ($m.has_codec) { $props.codec = [string]$m.codec }
    if ($null -ne $m.width) { $props.width = [int]$m.width }
    if ($null -ne $m.height) { $props.height = [int]$m.height }
    if ($m.has_fps -and $m.fps) {
        # mpv's container-fps is a rational string ("30/1").
        $f = [string]$m.fps
        if ($f -like '*/*') {
            $parts = $f -split '/'
            $n = [double]$parts[0]; $d = [double]$parts[1]
            if ($d -ne 0) { $props.fps = $n / $d }
        } else { $props.fps = [double]$f }
    } elseif ($m.has_estimated_fps -and $m.estimated_fps) {
        $props.fps = [double]$m.estimated_fps
    }
    if ($m.has_audio) { $props.has_audio = $true }
    if ($m.has_video) { $props.found_video = $true }
    return $props
}

$rows = @()
foreach ($f in $files) {
    Write-Host ("probing {0}" -f (Split-Path -Leaf $f))
    $a = Get-FfprobeProps $f
    $b = Get-MpvProps $f $engine
    $rows += [ordered]@{
        file = (Split-Path -Leaf $f)
        ffprobe = $a
        mpv = $b
        duration_delta = [math]::Round([math]::Abs($a.duration - $b.duration), 4)
        duration_ok = ([math]::Abs($a.duration - $b.duration) -lt 0.05)
        codec_ok = ($a.codec -eq $b.codec)
        size_ok = ($a.width -eq $b.width -and $a.height -eq $b.height)
        fps_ok = ([math]::Abs($a.fps - $b.fps) -lt 0.01)
        audio_ok = ($a.has_audio -eq $b.has_audio)
    }
}
$fail = @($rows | Where-Object { -not ($_.duration_ok -and $_.codec_ok -and $_.size_ok -and $_.fps_ok -and $_.audio_ok) })
Write-Host ""
Write-Host ("{0,-28} {1,-8} {2,-8} {3,-8} {4,-8} {5,-8} {6,-8} {7}" -f 'file', 'dur', 'codec', 'size', 'fps', 'audio', 'video', 'ffprobe->mpv')
foreach ($r in $rows) {
    Write-Host ("{0,-28} {1,-8} {2,-8} {3,-8} {4,-8} {5,-8} {6,-8} {7}" -f `
        $r.file, $r.duration_ok, $r.codec_ok, $r.size_ok, $r.fps_ok, $r.audio_ok, `
        ($r.ffprobe.found_video -eq $r.mpv.found_video), ("$($r.ffprobe.duration)/$($r.mpv.duration) $($r.ffprobe.fps)/$($r.mpv.fps)"))
}
Write-Host ""
Write-Host ("files compared: {0}, fields disagreeing: {1}" -f $rows.Count, $fail.Count)
$report = [ordered]@{
    tool = 'bench_probe_parity'
    date = (Get-Date -Format 'yyyy-MM-ddTHH:mm:ssK')
    label = 'live-proven (one machine)'
    method = 'the same five fields from ffprobe and from the live mpv instance, per file'
    files = $rows.Count
    disagreements = $fail.Count
    rows = $rows
}
$json = $report | ConvertTo-Json -Depth 8
$outFull = [System.IO.Path]::GetFullPath($OutJson)
$outDir = [System.IO.Path]::GetDirectoryName($outFull)
if (-not (Test-Path -LiteralPath $outDir)) { New-Item -ItemType Directory -Force -Path $outDir | Out-Null }
$json | Set-Content -LiteralPath $outFull -Encoding UTF8
Write-Host "bench_probe_parity: wrote $outFull"
if ($fail.Count -gt 0) { exit 1 }
