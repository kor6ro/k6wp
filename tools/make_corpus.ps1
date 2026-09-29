<# .SYNOPSIS
  Generate the k6wp-18 benchmark corpus (3 clips, lavfi only, no downloads).
.DESCRIPTION
  Plan target was 1 min 4K per clip (anime/gaming/slideshow). DEVIATION (documented):
  this box generates 30s 1080p (1920x1080) instead. Rationale: 3x 1-min 4K
  lavfi encodes via libx264 are very slow on this dev box and would produce
  multi-GB files that must NOT be committed; 30s 1080p keeps generation to
  minutes, files to tens of MB, while still exercising the compressor bench
  (Todo 18 QA-fail-with-note path explicitly allows recording the gap).
  Pass -Width/-Height/-DurationSec to restore 4K/60s on a faster GPU box.
.EXAMPLE
  powershell -NoProfile -ExecutionPolicy Bypass -File tools\make_corpus.ps1
  powershell -NoProfile -ExecutionPolicy Bypass -File tools\make_corpus.ps1 -DurationSec 60 -Width 3840 -Height 2160
#>
param(
  [string]$OutDir = "",
  [int]$DurationSec = 30,
  [int]$Width = 1920,
  [int]$Height = 1080,
  [int]$Fps = 30
)

$ErrorActionPreference = "Continue"

if ([string]::IsNullOrWhiteSpace($OutDir)) {
  $OutDir = Join-Path $PSScriptRoot "..\tests\corpus"
}
$OutDir = [System.IO.Path]::GetFullPath($OutDir)
$Ffmpeg = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot "..\vendor\ffmpeg\ffmpeg.exe"))
$Ffprobe = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot "..\vendor\ffmpeg\ffprobe.exe"))

if (-not (Test-Path -LiteralPath $Ffmpeg)) {
  Write-Error "make_corpus: ffmpeg not found at $Ffmpeg"
  exit 1
}
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

$Size = "{0}x{1}" -f $Width, $Height
$AnimeDur = $DurationSec
$GamingDur = $DurationSec
$SlideEach = [int][Math]::Ceiling($DurationSec / 3)
$SlideTotal = $SlideEach * 3

function Invoke-Ffmpeg {
  param([string[]]$FfmpegArgs, [string]$Label)
  Write-Host "make_corpus: $Label ..."
  $p = Start-Process -FilePath $Ffmpeg -ArgumentList $FfmpegArgs -NoNewWindow -Wait -PassThru
  if ($p.ExitCode -ne 0) {
    Write-Error ("make_corpus: {0} failed with exit {1}" -f $Label, $p.ExitCode)
    return $false
  }
  return $true
}

$ok = $true

# 1. anime-like: testsrc2 fast motion, full fps (high temporal complexity).
$anime = Join-Path $OutDir "anime.mp4"
$okAnime = Invoke-Ffmpeg -Label ("anime (" + $Size + " " + $AnimeDur + "s testsrc2)") -FfmpegArgs @(
  "-y",
  "-f", "lavfi", "-i", ("testsrc2=size=" + $Size + ":rate=" + $Fps + ":duration=" + $AnimeDur),
  "-c:v", "libx264", "-preset", "veryfast", "-crf", "18",
  "-pix_fmt", "yuv420p", "-r", "$Fps",
  "-movflags", "+faststart",
  $anime
)
if (-not $okAnime) { $ok = $false }

# 2. gaming-like: smptehdbars + temporal noise (hard detail + grain).
$gaming = Join-Path $OutDir "gaming.mp4"
$okGaming = Invoke-Ffmpeg -Label ("gaming (" + $Size + " " + $GamingDur + "s smptehdbars+noise)") -FfmpegArgs @(
  "-y",
  "-f", "lavfi", "-i", ("smptehdbars=size=" + $Size + ":rate=" + $Fps + ":duration=" + $GamingDur),
  "-vf", "noise=alls=20:allf=t+u",
  "-c:v", "libx264", "-preset", "veryfast", "-crf", "18",
  "-pix_fmt", "yuv420p", "-r", "$Fps",
  "-movflags", "+faststart",
  $gaming
)
if (-not $okGaming) { $ok = $false }

# 3. slideshow-like: 3 solid-color slides concatenated at low fps (low temporal complexity).
$slideshow = Join-Path $OutDir "slideshow.mp4"
$slideRate = 5
$okSlide = Invoke-Ffmpeg -Label ("slideshow (" + $Size + " 3x" + $SlideEach + "s color slides @" + $slideRate + "fps)") -FfmpegArgs @(
  "-y",
  "-f", "lavfi", "-i", ("color=c=0xC0392B:size=" + $Size + ":rate=" + $slideRate + ":duration=" + $SlideEach),
  "-f", "lavfi", "-i", ("color=c=0x2980B9:size=" + $Size + ":rate=" + $slideRate + ":duration=" + $SlideEach),
  "-f", "lavfi", "-i", ("color=c=0x27AE60:size=" + $Size + ":rate=" + $slideRate + ":duration=" + $SlideEach),
  "-filter_complex", "[0:v][1:v][2:v]concat=n=3:v=1:a=0[out]",
  "-map", "[out]",
  "-c:v", "libx264", "-preset", "veryfast", "-crf", "18",
  "-pix_fmt", "yuv420p", "-r", "$slideRate",
  "-movflags", "+faststart",
  $slideshow
)
if (-not $okSlide) { $ok = $false }

# Verify with ffprobe (duration + size report, non-fatal if ffprobe missing).
if (Test-Path -LiteralPath $Ffprobe) {
  foreach ($f in @($anime, $gaming, $slideshow)) {
    if (Test-Path -LiteralPath $f) {
      $p = Start-Process -FilePath $Ffprobe -ArgumentList @("-v", "error", "-show_entries", "format=duration,size", "-of", "default=noprint_wrappers=1", $f) -NoNewWindow -Wait -PassThru
      if ($p.ExitCode -ne 0) {
        Write-Error ("make_corpus: ffprobe failed for {0} (exit {1})" -f $f, $p.ExitCode)
        $ok = $false
      }
      $mb = ((Get-Item -LiteralPath $f).Length / 1MB)
      Write-Host ("make_corpus: {0} = {1:N2} MB" -f ([System.IO.Path]::GetFileName($f)), $mb)
    } else {
      Write-Error ("make_corpus: missing output {0}" -f $f)
      $ok = $false
    }
  }
} else {
  Write-Host "make_corpus: ffprobe not found, skipping verification."
}

if (-not $ok) { exit 1 }
Write-Host ("make_corpus: done. corpus dir: {0} (deviation: {1}s {2}, not 60s 4K - see header)" -f $OutDir, $DurationSec, $Size)
exit 0
