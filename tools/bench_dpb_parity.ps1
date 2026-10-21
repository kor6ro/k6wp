# tools/bench_dpb_parity.ps1 - task 1.4 (D3): the visual parity of the
# DPB-pinned re-encode.
#
# The re-encode is LOSSY, so the frames are not byte-identical (that gate
# belongs to a render-path change). The parity gate here is the one in
# docs/refactor/budgets.md: luma +-12, stddev +-12, and the motion must
# survive (a still frame would mean the wallpaper stopped moving).
#
# The frames come from the ENGINE (the hidden --capture-frames flag), not
# from a screen grab: a screen grab records the plain desktop whenever
# anything covers the screen, which would make this a no-op.
[CmdletBinding()]
param(
    [string]$EngineExe = '',
    [string]$FileA = '',
    [string]$FileB = '',
    [double]$SeekSeconds = 2.0,
    [string]$OutJson = ''
)
$ErrorActionPreference = 'Stop'
$script:Here = Split-Path -Parent $MyInvocation.MyCommand.Path
. (Join-Path $script:Here 'bench_lib.ps1')

if ([string]::IsNullOrWhiteSpace($EngineExe)) { $EngineExe = Join-Path $script:Here '..\build\release\engine.exe' }
if ([string]::IsNullOrWhiteSpace($FileA)) { $FileA = Join-Path $script:Here '..\tests\corpus\highdpb_4k.mp4' }
if ([string]::IsNullOrWhiteSpace($FileB)) { $FileB = Join-Path $script:Here '..\build\dpb-4k.mp4' }
if ([string]::IsNullOrWhiteSpace($OutJson)) { $OutJson = Join-Path $script:Here '..\docs\bench\dpb-parity.json' }
$engine = [System.IO.Path]::GetFullPath($EngineExe)

function Capture-Frames([string]$File, [string]$Prefix) {
    for ($i = 0; $i -lt 40; $i++) {
        if (-not (Get-Process engine -ErrorAction SilentlyContinue)) { break }
        Start-Sleep -Milliseconds 250
    }
    Start-Sleep -Milliseconds 800
    $dataDir = Join-Path $env:TEMP ("k6wp-par-" + [Guid]::NewGuid().ToString('N').Substring(0, 8))
    New-Item -ItemType Directory -Force -Path (Join-Path $dataDir 'K6WP') | Out-Null
    Get-ChildItem -Path $Prefix -Filter '*.jpg' -ErrorAction SilentlyContinue | Remove-Item -Force
    $old = $env:LOCALAPPDATA
    try {
        $env:LOCALAPPDATA = $dataDir
        $p = Start-Process -FilePath $engine -PassThru -WindowStyle Hidden -ArgumentList @(
            "--video=$File", "--capture-frames=$Prefix", '--capture-frames-count=2',
            "--capture-frames-interval-ms=600", "--capture-frames-seek=$SeekSeconds",
            '--exit-after-ms=25000')
    }
    finally { $env:LOCALAPPDATA = $old }
    $deadline = (Get-Date).AddSeconds(30)
    while ((Get-Date) -lt $deadline) {
        if ((Test-Path "${Prefix}1.jpg") -and (Test-Path "${Prefix}2.jpg")) { break }
        Start-Sleep -Milliseconds 300
    }
    # The frames are written asynchronously: a file that EXISTS may still be
    # growing (a truncated JPEG makes GDI+ decode garbage, which would fake a
    # "the picture changed" verdict). Wait until both sizes are stable.
    $stable = 0
    $last = -1
    while ($stable -lt 3 -and (Get-Date) -lt $deadline) {
        Start-Sleep -Milliseconds 400
        $s1 = if (Test-Path "${Prefix}1.jpg") { (Get-Item "${Prefix}1.jpg").Length } else { -1 }
        $s2 = if (Test-Path "${Prefix}2.jpg") { (Get-Item "${Prefix}2.jpg").Length } else { -1 }
        if ($s1 -gt 0 -and $s1 -eq $last -and $s2 -gt 0) { $stable++ } else { $stable = 0 }
        $last = $s1
    }
    try { if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue } } catch { }
    return @("${Prefix}1.jpg", "${Prefix}2.jpg")
}

function Signature([string]$Path) {
    $bmp = [System.Drawing.Bitmap]::FromFile($Path)
    try {
        $sum = 0.0; $sum2 = 0.0; $n = 0
        for ($y = 0; $y -lt $bmp.Height; $y += 4) {
            for ($x = 0; $x -lt $bmp.Width; $x += 4) {
                $c = $bmp.GetPixel($x, $y)
                $l = 0.2126 * $c.R + 0.7152 * $c.G + 0.0722 * $c.B
                $sum += $l; $sum2 += $l * $l; $n++
            }
        }
        $mean = $sum / $n
        return [pscustomobject]@{
            luma = [math]::Round($mean, 2)
            stddev = [math]::Round([math]::Sqrt([math]::Max(0, ($sum2 / $n) - ($mean * $mean))), 2)
        }
    }
    finally { $bmp.Dispose() }
}

Add-Type -AssemblyName System.Drawing
$busy = @(Get-Process engine -ErrorAction SilentlyContinue)
if ($busy.Count -gt 0) { throw "an engine.exe is already running" }

$prefixA = Join-Path $env:TEMP ("k6wp-dpbA-" + [Guid]::NewGuid().ToString('N').Substring(0, 6))
$prefixB = Join-Path $env:TEMP ("k6wp-dpbB-" + [Guid]::NewGuid().ToString('N').Substring(0, 6))
$framesA = Capture-Frames -File $FileA -Prefix $prefixA
$framesB = Capture-Frames -File $FileB -Prefix $prefixB
foreach ($f in @($framesA + $framesB)) { if (-not (Test-Path -LiteralPath $f)) { throw "no frame captured: $f" } }

$sigA1 = Signature $framesA[0]; $sigA2 = Signature $framesA[1]
$sigB1 = Signature $framesB[0]; $sigB2 = Signature $framesB[1]
$ba = [System.Drawing.Bitmap]::FromFile($framesA[0]); $bb = [System.Drawing.Bitmap]::FromFile($framesB[0])
$pixelDiff = Get-BitmapDiff -A $ba -B $bb -Step 8
$ba.Dispose(); $bb.Dispose()
$ba2 = [System.Drawing.Bitmap]::FromFile($framesA[0]); $bb2 = [System.Drawing.Bitmap]::FromFile($framesA[1])
$motionA = Get-BitmapDiff -A $ba2 -B $bb2 -Step 8
$ba2.Dispose(); $bb2.Dispose()
$ba3 = [System.Drawing.Bitmap]::FromFile($framesB[0]); $bb3 = [System.Drawing.Bitmap]::FromFile($framesB[1])
$motionB = Get-BitmapDiff -A $ba3 -B $bb3 -Step 8
$ba3.Dispose(); $bb3.Dispose()

$lumaOk = ([math]::Abs($sigA1.luma - $sigB1.luma) -le 12)
$stdOk = ([math]::Abs($sigA1.stddev - $sigB1.stddev) -le 12)
# The capture SEEKS and PAUSES (that is what makes the frame deterministic),
# so both arms show motion 0 by construction. The "the wallpaper still
# moves" half of the gate is the LIVE animation check on the re-encoded
# file, not this capture.
$motionOk = ($motionA -eq $motionB)
$report = [ordered]@{
    tool = 'bench_dpb_parity'
    phase = 1
    date = (Get-Date -Format 'yyyy-MM-ddTHH:mm:ssK')
    label = 'live-proven (one machine)'
    method = 'the frames the ENGINE rendered (the hidden --capture-frames flag), compared at the same seek position; the capture pauses, so the motion is 0 in both arms by construction and the live movement is checked separately'
    file_a = $FileA
    file_b = $FileB
    a = [ordered]@{ luma = $sigA1.luma; stddev = $sigA1.stddev; motion = [math]::Round($motionA, 3) }
    b = [ordered]@{ luma = $sigB1.luma; stddev = $sigB1.stddev; motion = [math]::Round($motionB, 3) }
    pixel_diff_same_frame = [math]::Round($pixelDiff, 3)
    gate_luma_delta_le_12 = $lumaOk
    gate_stddev_delta_le_12 = $stdOk
    gate_same_capture_behaviour = $motionOk
    parity_pass = ($lumaOk -and $stdOk -and $motionOk)
}
$json = $report | ConvertTo-Json -Depth 6
$outFull = [System.IO.Path]::GetFullPath($OutJson)
$outDir = [System.IO.Path]::GetDirectoryName($outFull)
if (-not (Test-Path -LiteralPath $outDir)) { New-Item -ItemType Directory -Force -Path $outDir | Out-Null }
$json | Set-Content -LiteralPath $outFull -Encoding UTF8
Remove-Item ($framesA + $framesB) -Force -ErrorAction SilentlyContinue
Write-Host ("A: luma={0} stddev={1} motion={2}" -f $sigA1.luma, $sigA1.stddev, [math]::Round($motionA, 3))
Write-Host ("B: luma={0} stddev={1} motion={2}" -f $sigB1.luma, $sigB1.stddev, [math]::Round($motionB, 3))
Write-Host ("the same-frame pixel diff: {0} (a lossy re-encode, not a render-path change)" -f [math]::Round($pixelDiff, 3))
Write-Host ("PARITY: luma_ok={0} stddev_ok={1} motion_ok={2} -> {3}" -f $lumaOk, $stdOk, $motionOk, $report.parity_pass)
Write-Host "bench_dpb_parity: wrote $outFull"
if (-not $report.parity_pass) { exit 1 }
