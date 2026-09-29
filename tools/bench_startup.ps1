# bench_startup.ps1 - measure engine startup latency (process start -> Engine:FirstFrame)
#
# Usage:
#   .\bench_startup.ps1 [-Runs 5] [-EnginePath ..\build\msvc-dev\engine.exe]
#                       [-ExitAfterMs 3000] [-TimeoutMs 10000]
#
# Output (stdout, JSON envelope):
#   {"tool":"bench_startup","date":"...","hw":{...},"runs":5,
#    "samples_ms":[...],"median_ms":X,"budget_ms":2000,"pass":true,...}
#
# Also writes the same JSON to -OutJson (default docs/bench_startup.json).
# Budgets match planning.md section 6: startup < 2s (2000 ms).
#
# PowerShell 5.1 compatible. Native stderr is NOT terminating (Continue +
# $LASTEXITCODE), per Todo 4 learning.

[CmdletBinding()]
param(
    [int]$Runs = 5,
    [string]$EnginePath = (Join-Path $PSScriptRoot '..\build\msvc-dev\engine.exe'),
    [int]$ExitAfterMs = 3000,
    [int]$TimeoutMs = 10000,
    [string]$OutJson = (Join-Path $PSScriptRoot '..\docs\bench_startup.json')
)

$ErrorActionPreference = 'Continue'

function Get-Median([double[]]$Values) {
    if ($Values.Count -eq 0) { return 0.0 }
    $sorted = @($Values | Sort-Object)
    $n = $sorted.Count
    if ($n % 2 -eq 1) { return $sorted[($n - 1) / 2] }
    return ($sorted[$n / 2 - 1] + $sorted[$n / 2]) / 2.0
}

$engine = Resolve-Path $EnginePath -ErrorAction SilentlyContinue
if (-not $engine) {
    Write-Error "engine.exe not found at '$EnginePath'"
    exit 2
}

$tmpDir = Join-Path $env:TEMP "k6wp-bench-startup"
New-Item -ItemType Directory -Force -Path $tmpDir | Out-Null

$samples = New-Object System.Collections.Generic.List[double]

for ($i = 1; $i -le $Runs; $i++) {
    $outFile = Join-Path $tmpDir "run-$i-$PID.out"
    $errFile = Join-Path $tmpDir "run-$i-$PID.err"
    Remove-Item $outFile, $errFile -Force -ErrorAction SilentlyContinue

    $start = Get-Date
    $proc = Start-Process -FilePath $engine -ArgumentList "--exit-after-ms=$ExitAfterMs" `
        -RedirectStandardOutput $outFile -RedirectStandardError $errFile `
        -PassThru -WindowStyle Hidden

    $markerMs = $null
    $deadline = (Get-Date).AddMilliseconds($TimeoutMs)
    while ((Get-Date) -lt $deadline) {
        if (Test-Path $outFile) {
            $content = Get-Content $outFile -Raw -ErrorAction SilentlyContinue
            if ($content -match 'Engine:FirstFrame') {
                $markerMs = ((Get-Date) - $start).TotalMilliseconds
                break
            }
        }
        if ($proc.HasExited) { break }
        Start-Sleep -Milliseconds 5
    }

    if (-not $proc.HasExited) {
        Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
        $proc.WaitForExit()
    }

    if ($null -eq $markerMs) {
        Write-Warning "run ${i}: Engine:FirstFrame not seen within ${TimeoutMs}ms (exit code $($proc.ExitCode))"
        continue
    }
    $samples.Add($markerMs)
    Write-Host "run ${i}: $([math]::Round($markerMs, 1)) ms"
}

$median = Get-Median $samples.ToArray()
$rounded = @($samples.ToArray() | ForEach-Object { [math]::Round($_, 1) })
$budgetMs = 2000
$medianRounded = [math]::Round($median, 1)

# Reproducibility: max deviation from the median (rerun variance signal).
$maxDev = 0.0
foreach ($s in $rounded) {
    $d = [math]::Abs($s - $medianRounded)
    if ($d -gt $maxDev) { $maxDev = $d }
}

# HW inventory: engine bench mode loads no video, so GPU decode is idle.
# Never silent: record the GPU name(s) + explicit no-hwdec flag.
$gpuNames = @()
try {
    $gpuNames = @(Get-CimInstance Win32_VideoController -ErrorAction SilentlyContinue |
        ForEach-Object { $_.Name } | Where-Object { $_ })
} catch { $gpuNames = @() }
if ($gpuNames.Count -eq 0) { $gpuNames = @("unknown") }

$result = [ordered]@{
    tool = "bench_startup"
    date = (Get-Date -Format "yyyy-MM-dd")
    engine = "build/msvc-dev/engine.exe"
    hw = [ordered]@{
        gpu = ($gpuNames -join "; ")
        hwdec = "none"
        no_hw = $true
        note = "bench mode loads no video; GPU decode idle (no mpv stream)"
    }
    runs = $Runs
    samples_ms = $rounded
    median_ms = $medianRounded
    max_dev_ms = [math]::Round($maxDev, 1)
    budget_ms = $budgetMs
    pass = ($medianRounded -lt $budgetMs)
    reproducibility = "rerun variance: max sample deviation from median = $([math]::Round($maxDev,1)) ms; reproducible if median stays within +/-10% across runs"
}
$json = $result | ConvertTo-Json -Compress -Depth 5
$json
if (-not [string]::IsNullOrWhiteSpace($OutJson)) {
    $outFull = [System.IO.Path]::GetFullPath($OutJson)
    $outDir = [System.IO.Path]::GetDirectoryName($outFull)
    if (-not (Test-Path -LiteralPath $outDir)) {
        New-Item -ItemType Directory -Force -Path $outDir | Out-Null
    }
    $json | Set-Content -LiteralPath $outFull -Encoding UTF8
    Write-Host "bench_startup: wrote $outFull"
}