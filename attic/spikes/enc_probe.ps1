# spikes/enc_probe.ps1
# Spike (throwaway, NOT shipped): probe HW encoders with a 1-frame test encode.
# Picks the first encoder that passes (order: NVENC -> QSV -> AMF -> libx264).
# Usage: powershell -File spikes/enc_probe.ps1
# Output: JSON {picked, results:[{encoder,ok,error}]} to stdout. Exit 0 if >=1 encoder works.

$ErrorActionPreference = "Continue"
$ffmpeg = Join-Path $PSScriptRoot "..\vendor\ffmpeg\ffmpeg.exe"
$outDir = Join-Path $PSScriptRoot "..\build\spikes"
New-Item -ItemType Directory -Path $outDir -Force | Out-Null

$encoders = @("h264_nvenc", "h264_qsv", "h264_amf", "libx264")
$results = @()
$picked = $null

foreach ($enc in $encoders) {
    $out = Join-Path $outDir "enc_probe_$enc.mp4"
    if (Test-Path $out) { Remove-Item $out -Force }
    $err = & $ffmpeg -hide_banner -loglevel error -f lavfi -i "testsrc2=size=1920x1080:rate=30" -frames:v 1 -c:v $enc -y $out 2>&1
    $code = $LASTEXITCODE
    $ok = ($code -eq 0) -and (Test-Path $out) -and ((Get-Item $out).Length -gt 0)
    $errText = ($err | Out-String).Trim()
    if ($ok) {
        $results += @{ encoder = $enc; ok = $true; error = "" }
        if (-not $picked) { $picked = $enc }
        Write-Output "OK: $enc"
    } else {
        $results += @{ encoder = $enc; ok = $false; error = $errText }
        Write-Output "FAIL: $enc"
    }
}

$json = @{
    picked = $picked
    results = $results
} | ConvertTo-Json -Depth 4
Write-Output $json

if ($picked) { exit 0 } else { exit 1 }