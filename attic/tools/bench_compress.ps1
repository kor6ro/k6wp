<# .SYNOPSIS
  Benchmark compressor.exe over the k6wp-18 corpus; emits JSON array to stdout.
.DESCRIPTION
  For each *.mp4 in -CorpusDir, runs:
    compressor.exe --in <clip> --out <OutDir>/<base>_bench.mp4 --res 1280x720 --fps 30 --crf 23 --encoder auto
  Times the run with Measure-Command, parses the final {"ok":...} stdout line
  for the encoder name, stats the output file size in MB, and emits one JSON
   object per file: {file, encoder, seconds, size_mb}. Prints the JSON
   envelope to stdout (progress goes to stderr via Write-Host -> console).
   Also writes the same JSON to -OutJson (default docs/bench_compress.json).
   Target: HW <30s/file (planning.md section 6: 15-30s for 1-min 4K->1080p
   on a hardware encoder). If missed, record the gap honestly - never fake
   numbers. Without a hardware encoder the run is flagged no_hw:true.
   Cache-masking rule: the compressor LRU cache (Todo 17) is keyed on
   (in,res,fps,crf,encoder), NOT on --out, so re-running the same matrix
   returns cache_hit:true copies. For genuine encode timings point
   $env:K6WP_CACHE_DIR at a fresh empty dir (or vary res/crf/encoder);
   every row records its cache_hit flag as proof.
.EXAMPLE
  powershell -NoProfile -ExecutionPolicy Bypass -File tools\bench_compress.ps1
#>
param(
  [string]$CorpusDir = "",
  [string]$OutDir = "",
  [string]$Res = "1280x720",
  [int]$Fps = 30,
  [int]$Crf = 23,
  [string]$OutJson = ""
)

$ErrorActionPreference = "Continue"

if ([string]::IsNullOrWhiteSpace($CorpusDir)) {
  $CorpusDir = Join-Path $PSScriptRoot "..\tests\corpus"
}
if ([string]::IsNullOrWhiteSpace($OutDir)) {
  $OutDir = Join-Path $env:TEMP "k6wp_bench"
}
$CorpusDir = [System.IO.Path]::GetFullPath($CorpusDir)
$OutDir = [System.IO.Path]::GetFullPath($OutDir)
$Compressor = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot "..\build\msvc-dev\compressor.exe"))

if (-not (Test-Path -LiteralPath $Compressor)) {
  Write-Error "bench_compress: compressor not found at $Compressor (build --target compressor first)"
  exit 1
}
if (-not (Test-Path -LiteralPath $CorpusDir)) {
  Write-Error "bench_compress: corpus dir not found: $CorpusDir (run tools\make_corpus.ps1 first)"
  exit 1
}
if ([string]::IsNullOrWhiteSpace($OutJson)) {
  $OutJson = Join-Path $PSScriptRoot "..\docs\bench_compress.json"
}
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

$files = Get-ChildItem -LiteralPath $CorpusDir -Filter "*.mp4" | Sort-Object Name
if ($files.Count -eq 0) {
  Write-Error "bench_compress: no *.mp4 in $CorpusDir"
  exit 1
}

$results = @()
$failed = $false

foreach ($f in $files) {
  $base = [System.IO.Path]::GetFileNameWithoutExtension($f.Name)
  $outFile = Join-Path $OutDir ($base + "_bench.mp4")
  [void](Write-Host ("bench_compress: {0} -> {1} ..." -f $f.Name, $outFile))

  $stdoutFile = Join-Path $OutDir ($base + "_stdout.txt")
  $stderrFile = Join-Path $OutDir ($base + "_stderr.txt")
  $sw = [System.Diagnostics.Stopwatch]::StartNew()
  # Start-Process native redirect (no cmd /c: a leading quote makes cmd.exe
  # strip quotes per its parsing rule - Todo 14 learning; no 2> operator so
  # PS5.1 emits no NativeCommandError records).
  $p = Start-Process -FilePath $Compressor -ArgumentList @("--in", $f.FullName, "--out", $outFile, "--res", $Res, "--fps", "$Fps", "--crf", "$Crf", "--encoder", "auto") -NoNewWindow -Wait -PassThru -RedirectStandardOutput $stdoutFile -RedirectStandardError $stderrFile
  $sw.Stop()
  $seconds = [Math]::Round($sw.Elapsed.TotalSeconds, 2)
  $exitCode = $p.ExitCode

  $encoder = "unknown"
  $cacheHit = $false
  $sizeMb = 0.0
  if ($exitCode -ne 0) {
    Write-Error ("bench_compress: {0} failed with exit {1} ({2}s)" -f $f.Name, $exitCode, $seconds)
    $failed = $true
  } else {
    if (Test-Path -LiteralPath $stdoutFile) {
      # @() wrap: single-line files come back as scalar String (char-indexed
      # without the wrap - Todo 43 learning), not String[].
      $lines = @(Get-Content -LiteralPath $stdoutFile)
      for ($i = $lines.Count - 1; $i -ge 0; $i--) {
        $line = [string]$lines[$i]
        $line = $line.Trim()
        if ([string]::IsNullOrEmpty($line)) { continue }
        if ($line.StartsWith('{"ok"') -or $line.StartsWith('{ "ok"') -or ($line -match '"ok"\s*:\s*true')) {
          try {
            $j = $line | ConvertFrom-Json
            if ($j.encoder) { $encoder = [string]$j.encoder }
            if ($j.cache_hit -eq $true) { $cacheHit = $true }
          } catch {
            $m = [regex]::Match($line, '"encoder"\s*:\s*"([^"]+)"')
            if ($m.Success) { $encoder = $m.Groups[1].Value }
            if ($line -match '"cache_hit"\s*:\s*true') { $cacheHit = $true }
          }
          break
        }
      }
    }
    if (Test-Path -LiteralPath $outFile) {
      $sizeMb = [Math]::Round(((Get-Item -LiteralPath $outFile).Length / 1MB), 2)
    } else {
      Write-Error ("bench_compress: output missing for {0}" -f $f.Name)
      $failed = $true
    }
  }

  [void](Write-Host ("bench_compress: {0} encoder={1} seconds={2} size_mb={3} cache_hit={4}" -f $f.Name, $encoder, $seconds, $sizeMb, $cacheHit))
  $results += New-Object PSObject -Property @{
    file = $f.Name
    encoder = $encoder
    seconds = $seconds
    size_mb = $sizeMb
    cache_hit = $cacheHit
  }
}

$budgetSec = 30.0
$allPass = $true
foreach ($r in $results) {
  if ($r.seconds -ge $budgetSec) { $allPass = $false }
}
if ($results.Count -eq 0) { $allPass = $false }

# HW flag: any non-x264 encoder means a hardware encoder did the work.
# All-x264 (or unknown) = no hardware encoder on this box -> no_hw:true.
$hwUsed = @($results | Where-Object { $_.encoder -ne "libx264" -and $_.encoder -ne "unknown" }).Count -gt 0
$hwLabel = "none"
if ($hwUsed) {
  $hwLabel = (($results | Where-Object { $_.encoder -ne "libx264" -and $_.encoder -ne "unknown" } | Select-Object -ExpandProperty encoder | Sort-Object -Unique) -join "; ")
}
$gpuNames = @()
try {
  $gpuNames = @(Get-CimInstance Win32_VideoController -ErrorAction SilentlyContinue |
    ForEach-Object { $_.Name } | Where-Object { $_ })
} catch { $gpuNames = @() }
if ($gpuNames.Count -eq 0) { $gpuNames = @("unknown") }

$rows = @($results | Select-Object file, encoder, seconds, size_mb, cache_hit)
$envelope = [ordered]@{
  tool = "bench_compress"
  date = (Get-Date -Format "yyyy-MM-dd")
  hw = [ordered]@{
    gpu = ($gpuNames -join "; ")
    hwdec = $hwLabel
    no_hw = (-not $hwUsed)
  }
  corpus = $CorpusDir
  res = $Res
  fps = $Fps
  crf = $Crf
  results = $rows
  budget_sec = $budgetSec
  pass = ($allPass -and (-not $failed))
  reproducibility = "Fresh K6WP_CACHE_DIR per run (cache key is in/res/fps/crf/encoder, not --out). Rerun +-10% on seconds under identical load."
}
$json = $envelope | ConvertTo-Json -Depth 5 -Compress
Write-Output $json
if (-not [string]::IsNullOrWhiteSpace($OutJson)) {
  $outFull = [System.IO.Path]::GetFullPath($OutJson)
  $outDirJson = [System.IO.Path]::GetDirectoryName($outFull)
  if (-not (Test-Path -LiteralPath $outDirJson)) {
    New-Item -ItemType Directory -Force -Path $outDirJson | Out-Null
  }
  $json | Set-Content -LiteralPath $outFull -Encoding UTF8
  [void](Write-Host ("bench_compress: wrote {0}" -f $outFull))
}

if ($failed) { exit 1 }
exit 0
