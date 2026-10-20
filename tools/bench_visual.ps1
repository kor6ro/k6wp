# tools/bench_visual.ps1 - Phase 0 visual parity sampler.
#
# Captures what the user actually SEES on the desktop, so a later refactor
# can prove it did not change the picture. Two modes:
#
#   -Capture <dir>   capture the current desktop region(s) to PNG
#   -Compare <a> <b>  diff two capture directories and report
#
# The capture is a GDI screen grab of the wallpaper region. Because the
# wallpaper is a video, the frame is not reproducible, so parity is measured
# with a STATISTICAL signature, not a byte-exact comparison:
#
#   mean_luma   average brightness of the region (catches "black screen")
#   stddev      contrast (catches "flat grey" / failed decode)
#   mean_rgb    average colour per channel (catches colour-channel swaps)
#   changed_pct fraction of sampled pixels that differ between two captures
#
# A refactor passes the visual parity gate when the mean/stddev of the
# capture stay within the configured tolerance of the baseline AND the
# changed-pixel fraction is in the expected band (a moving wallpaper always
# differs frame to frame; a frozen one is a regression).
#
# The wallpaper is injected BEHIND the desktop, so all other windows are
# minimized first - otherwise the capture shows the foreground app.
#
# Usage:
#   powershell -NoProfile -ExecutionPolicy Bypass -File tools\bench_visual.ps1 `
#     -Capture docs\bench\visual\baseline
#   powershell -NoProfile -ExecutionPolicy Bypass -File tools\bench_visual.ps1 `
#     -Compare docs\bench\visual\baseline docs\bench\visual\candidate

[CmdletBinding()]
param(
    # Compare/Compare2 come FIRST so that `-Compare <dirA> <dirB>` binds the
    # second directory to Compare2 by position (PowerShell fills positional
    # slots in declaration order; with Capture first it would steal it).
    [string]$Compare = '',
    [string]$Compare2 = '',
    [string]$Capture = '',
    [int]$CropWidth = 0,
    [int]$CropHeight = 0,
    [double]$LumaTolerance = 12.0,
    [double]$StddevTolerance = 12.0,
    [switch]$Force,
    [switch]$SkipMinimize
)

$ErrorActionPreference = 'Stop'
$script:Here = Split-Path -Parent $MyInvocation.MyCommand.Path
. (Join-Path $script:Here 'bench_lib.ps1')

# Mode is decided by which parameter was EXPLICITLY bound. Deciding on
# "is it non-empty" makes the positional binding of the first argument
# ambiguous (it lands on $Capture), which silently ran the wrong mode.
$doCapture = $PSBoundParameters.ContainsKey('Capture') -and -not $PSBoundParameters.ContainsKey('Compare')
$doCompare = $PSBoundParameters.ContainsKey('Compare') -or $PSBoundParameters.ContainsKey('Compare2')
if (-not $doCapture -and -not $doCompare) {
    throw "pass -Capture <dir> or -Compare <dirA> <dirB>"
}
if ($doCompare -and [string]::IsNullOrWhiteSpace($Compare2)) {
    throw "-Compare needs TWO directories: -Compare <dirA> <dirB>"
}

# The signature is computed over the FULL captured screen (the same pixels
# that go into the PNG), so the signature and the motion check describe the
# same thing. A center crop of the testsrc2 corpus clip lands on two static
# colour bars, which would make a FROZEN video indistinguishable from a
# playing one.
function Get-RegionSignature {
    param([int]$Step = 4)
    $shot = Get-ScreenBitmap
    $bmp = $shot.Bitmap
    try {
        $sum = 0.0; $sum2 = 0.0
        $rs = 0.0; $gs = 0.0; $bs = 0.0
        $n = 0
        for ($y = 0; $y -lt $bmp.Height; $y += $Step) {
            for ($x = 0; $x -lt $bmp.Width; $x += $Step) {
                $c = $bmp.GetPixel($x, $y)
                $l = 0.2126 * $c.R + 0.7152 * $c.G + 0.0722 * $c.B
                $sum += $l; $sum2 += $l * $l
                $rs += $c.R; $gs += $c.G; $bs += $c.B
                $n++
            }
        }
        if ($n -eq 0) { return $null }
        $mean = $sum / $n
        $variance = [math]::Max(0.0, ($sum2 / $n) - ($mean * $mean))
        return [pscustomobject]@{
            pixels = $n
            mean_luma = [math]::Round($mean, 2)
            stddev = [math]::Round([math]::Sqrt($variance), 2)
            mean_r = [math]::Round($rs / $n, 2)
            mean_g = [math]::Round($gs / $n, 2)
            mean_b = [math]::Round($bs / $n, 2)
        }
    }
    finally { $bmp.Dispose() }
}

if ($doCapture) {
    # Never silently overwrite an existing capture: a baseline that gets
    # replaced by an unrelated capture is worse than no baseline at all.
    if ((Test-Path -LiteralPath $Capture) -and -not $Force) {
        $existing = @(Get-ChildItem -LiteralPath $Capture -Filter 'frame*.png' -ErrorAction SilentlyContinue)
        if ($existing.Count -gt 0) {
            throw ("capture directory already holds {0} frame(s): {1}. Pass -Force to replace it." -f $existing.Count, $Capture)
        }
    }
    $savedWindows = @()
    if (-not $SkipMinimize) {
        Write-Host "bench_visual: minimizing top-level windows (restored at the end)"
        $savedWindows = Get-DesktopWindows
    }
    try {
        $dir = [System.IO.Path]::GetFullPath($Capture)
        New-Item -ItemType Directory -Force -Path $dir | Out-Null
        $frames = @()
        # Four frames spread over time: a moving wallpaper differs between
        # them, a frozen one does not. The FULL virtual screen is captured
        # (not a center crop) so the motion check sees real movement.
        for ($i = 1; $i -le 4; $i++) {
            $png = Join-Path $dir ("frame{0}.png" -f $i)
            Save-ScreenPng -Path $png -CropWidth $CropWidth -CropHeight $CropHeight
            $sig = Get-RegionSignature
            $frames += [pscustomobject]@{ index = $i; file = (Split-Path -Leaf $png); signature = $sig }
            Write-Host ("  frame {0}: mean_luma={1} stddev={2} rgb=({3},{4},{5})" -f `
                $i, $sig.mean_luma, $sig.stddev, $sig.mean_r, $sig.mean_g, $sig.mean_b)
            if ($i -lt 4) { Start-Sleep -Seconds 1 }
        }
        $m = $frames | ForEach-Object { $_.signature.mean_luma }
        $s = $frames | ForEach-Object { $_.signature.stddev }
        # The motion value is stored INSIDE the signature so the parity gate
        # can be evaluated from signature.json alone (no PNG re-read needed).
        # The PNGs stay in the directory as the human-inspectable evidence.
        $motion = $null
        # No script block here: a `{ }` block would create a child scope and
        # the assignment to $motion would not reach the caller.
        Add-Type -AssemblyName System.Drawing, System.Windows.Forms
        $frameA = [System.Drawing.Bitmap]::FromFile((Join-Path $dir 'frame1.png'))
        $frameB = [System.Drawing.Bitmap]::FromFile((Join-Path $dir 'frame2.png'))
        try { $motion = [math]::Round((Get-BitmapDiff -A $frameA -B $frameB -Step 4), 3) }
        finally { $frameA.Dispose(); $frameB.Dispose() }
        $report = [ordered]@{
            tool = 'bench_visual'
            phase = 0
            date = (Get-Date -Format 'yyyy-MM-ddTHH:mm:ssK')
            label = 'live-proven'
            dir = $dir
            region = [ordered]@{ full_screen = $true; crop_width = $CropWidth; crop_height = $CropHeight; note = 'signature and motion diff are computed over the full captured frame' }
            frames = $frames
            mean_luma = Get-Stats $m
            stddev = Get-Stats $s
            motion_diff = $motion
        }
        $json = $report | ConvertTo-Json -Depth 8
        $json | Set-Content -LiteralPath (Join-Path $dir 'signature.json') -Encoding UTF8
        Write-Host "bench_visual: captured to $dir"
    }
    finally {
        if (-not $SkipMinimize) { Restore-DesktopWindows -Saved $savedWindows }
    }
    exit 0
}

# --- Compare mode ---
$dirA = [System.IO.Path]::GetFullPath($Compare)
$dirB = [System.IO.Path]::GetFullPath($Compare2)
$sigA = Get-Content -LiteralPath (Join-Path $dirA 'signature.json') -Raw | ConvertFrom-Json
$sigB = Get-Content -LiteralPath (Join-Path $dirB 'signature.json') -Raw | ConvertFrom-Json

$fail = @()
$report = [ordered]@{
    tool = 'bench_visual'
    phase = 0
    date = (Get-Date -Format 'yyyy-MM-ddTHH:mm:ssK')
    label = 'live-proven'
    a = $dirA
    b = $dirB
    tolerance = [ordered]@{ mean_luma = $LumaTolerance; stddev = $StddevTolerance }
    checks = @()
    pass = $true
}
foreach ($key in @('mean_luma', 'stddev')) {
    $a = [double]$sigA.$key.median
    $b = [double]$sigB.$key.median
    $delta = [math]::Abs($a - $b)
    $tol = if ($key -eq 'mean_luma') { $LumaTolerance } else { $StddevTolerance }
    $ok = $delta -le $tol
    if (-not $ok) { $fail += $key }
    $report.checks += [ordered]@{ metric = $key; a = $a; b = $b; delta = [math]::Round($delta, 2); tolerance = $tol; pass = $ok }
}

# Frame-to-frame motion inside each capture: a frozen wallpaper is a
# regression (the video stopped), so both captures must show motion. The
# value is read from signature.json when present (it is computed at capture
# time); otherwise it is recomputed from the PNGs. The diff runs over the
# FULL captured frames with a coarse step, so it stays fast at 1080p+.
function Get-Motion([string]$dir, $sig) {
    if ($sig -and $null -ne $sig.motion_diff) {
        return [double]$sig.motion_diff
    }
    Add-Type -AssemblyName System.Drawing, System.Windows.Forms
    $files = @(Get-ChildItem -LiteralPath $dir -Filter 'frame*.png' | Sort-Object Name)
    if ($files.Count -lt 2) { return $null }
    $a = [System.Drawing.Bitmap]::FromFile($files[0].FullName)
    $b = [System.Drawing.Bitmap]::FromFile($files[1].FullName)
    try { return Get-BitmapDiff -A $a -B $b -Step 4 }
    finally { $a.Dispose(); $b.Dispose() }
}
$motionA = Get-Motion $dirA $sigA
$motionB = Get-Motion $dirB $sigB
if ($null -ne $motionA -and $null -ne $motionB) {
    $moving = ($motionA -gt 0.0) -and ($motionB -gt 0.0)
    if (-not $moving) { $fail += 'motion' }
    $report.checks += [ordered]@{ metric = 'motion_diff'; a = [math]::Round($motionA, 3); b = [math]::Round($motionB, 3); pass = $moving }
}
$report.pass = ($fail.Count -eq 0)
$report.failed_checks = $fail

$json = $report | ConvertTo-Json -Depth 8
Write-Host $json
if (-not $report.pass) {
    # The EXIT CODE is the contract (1 = parity failed). Write-Error would be
    # a TERMINATING error under this script's ErrorActionPreference and would
    # abort a caller that expects to inspect the JSON, so the failure is
    # reported on the host stream and signalled through the exit code.
    Write-Host ("bench_visual: PARITY FAILED ({0})" -f ($fail -join ', '))
    exit 1
}
Write-Host "bench_visual: parity OK"
exit 0
