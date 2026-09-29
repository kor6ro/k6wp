# bench_cpu_mem.ps1 - sample engine CPU% (QueryProcessCycleTime) + private MB
# (GetProcessMemoryInfo) over a time window.
#
# Usage:
#   .\bench_cpu_mem.ps1 [-Minutes 5] [-EnginePath ..\build\msvc-dev\engine.exe]
#                       [-IntervalMs 500]
#
# Output (stdout, JSON envelope):
#   {"tool":"bench_cpu_mem","date":"...","hw":{...},"minutes":5,"samples":N,
#    "avg_cpu_percent":X,"avg_private_mb":Y,"peak_private_mb":Z,
#    "budget":{...},"pass":true/false,...}
#
# Also writes the same JSON to -OutJson (default docs/bench_cpu_mem.json).
# Budgets match planning.md section 6: CPU < 2% (hwdec active), RAM < 80 MB.
#
# CPU% is computed from QueryProcessCycleTime deltas divided by the QPC
# frequency (Todo 4 learning: PULONG64, not ULARGE_INTEGER*). Private MB is
# PROCESS_MEMORY_COUNTERS.PrivateUsage / 1MiB.
#
# PowerShell 5.1 compatible.

[CmdletBinding()]
param(
    [int]$Minutes = 5,
    [string]$EnginePath = (Join-Path $PSScriptRoot '..\build\msvc-dev\engine.exe'),
    [int]$IntervalMs = 500,
    [string]$OutJson = (Join-Path $PSScriptRoot '..\docs\bench_cpu_mem.json')
)

$ErrorActionPreference = 'Continue'

$engine = Resolve-Path $EnginePath -ErrorAction SilentlyContinue
if (-not $engine) {
    Write-Error "engine.exe not found at '$EnginePath'"
    exit 2
}

# --- P/Invoke: QueryProcessCycleTime + GetProcessMemoryInfo + QPF -----------
$nativeSource = @'
using System;
using System.Runtime.InteropServices;

public static class K6wpBenchNative {
    [StructLayout(LayoutKind.Sequential)]
    public struct PROCESS_MEMORY_COUNTERS {
        public uint cb;
        public uint PageFaultCount;
        public UIntPtr PeakWorkingSetSize;
        public UIntPtr WorkingSetSize;
        public UIntPtr QuotaPeakPagedPoolUsage;
        public UIntPtr QuotaPagedPoolUsage;
        public UIntPtr QuotaPeakNonPagedPoolUsage;
        public UIntPtr QuotaNonPagedPoolUsage;
        public UIntPtr PagefileUsage;
        public UIntPtr PeakPagefileUsage;
        public UIntPtr PrivateUsage;
    }

    [DllImport("kernel32.dll", SetLastError = true)]
    public static extern bool QueryProcessCycleTime(IntPtr hProcess, out ulong cycleTime);

    [DllImport("kernel32.dll")]
    public static extern bool QueryPerformanceFrequency(out long lpFrequency);

    [DllImport("psapi.dll", SetLastError = true)]
    public static extern bool GetProcessMemoryInfo(
        IntPtr hProcess, ref PROCESS_MEMORY_COUNTERS ppsmemCounters, uint cb);
}
'@
Add-Type -TypeDefinition $nativeSource

# --- Launch engine (no --exit-after-ms -> runs until killed) ----------------
$tmpDir = Join-Path $env:TEMP "k6wp-bench-cpu"
New-Item -ItemType Directory -Force -Path $tmpDir | Out-Null
$outFile = Join-Path $tmpDir "run-$PID.out"
$errFile = Join-Path $tmpDir "run-$PID.err"

$proc = Start-Process -FilePath $engine `
    -RedirectStandardOutput $outFile -RedirectStandardError $errFile `
    -PassThru -WindowStyle Hidden

try {
    $qpf = 0L
    [void][K6wpBenchNative]::QueryPerformanceFrequency([ref]$qpf)
    if ($qpf -le 0) {
        Write-Error "QueryPerformanceFrequency failed"
        exit 2
    }

    $cpuSamples = New-Object System.Collections.Generic.List[double]
    $memSamples = New-Object System.Collections.Generic.List[double]
    $peakPrivateMb = 0.0

    $prevCycles = [uint64]0
    $prevTime = Get-Date
    $deadline = (Get-Date).AddMinutes($Minutes)
    $first = $true

    while ((Get-Date) -lt $deadline) {
        if ($proc.HasExited) {
            Write-Warning "engine exited early (code $($proc.ExitCode))"
            break
        }

        $cycles = [uint64]0
        if (-not [K6wpBenchNative]::QueryProcessCycleTime($proc.Handle, [ref]$cycles)) {
            Write-Warning "QueryProcessCycleTime failed (error $([Runtime.InteropServices.Marshal]::GetLastWin32Error()))"
            break
        }

        $counters = New-Object K6wpBenchNative+PROCESS_MEMORY_COUNTERS
        $counters.cb = [System.Runtime.InteropServices.Marshal]::SizeOf($counters)
        if (-not [K6wpBenchNative]::GetProcessMemoryInfo($proc.Handle, [ref]$counters, $counters.cb)) {
            Write-Warning "GetProcessMemoryInfo failed (error $([Runtime.InteropServices.Marshal]::GetLastWin32Error()))"
            break
        }

        $now = Get-Date
        if (-not $first) {
            $deltaCycles = $cycles - $prevCycles
            $deltaSec = ($now - $prevTime).TotalSeconds
            if ($deltaSec -gt 0) {
                $cpuPercent = ($deltaCycles / $qpf) / $deltaSec * 100.0
                $cpuSamples.Add($cpuPercent)
            }
        }
        $first = $false

        $privateMb = $counters.PrivateUsage.ToUInt64() / 1MB
        $memSamples.Add($privateMb)
        if ($privateMb -gt $peakPrivateMb) { $peakPrivateMb = $privateMb }

        $prevCycles = $cycles
        $prevTime = $now
        Start-Sleep -Milliseconds $IntervalMs
    }
}
finally {
    if (-not $proc.HasExited) {
        Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
        $proc.WaitForExit()
    }
}

$avgCpu = 0.0
if ($cpuSamples.Count -gt 0) { $avgCpu = ($cpuSamples | Measure-Object -Average).Average }
$avgMem = 0.0
if ($memSamples.Count -gt 0) { $avgMem = ($memSamples | Measure-Object -Average).Average }

# Budgets from planning.md section 6 (engine idle, video playing).
$cpuBudget = 2.0
$ramBudgetMb = 80.0
$avgCpuR = [math]::Round($avgCpu, 2)
$avgMemR = [math]::Round($avgMem, 2)
$peakMemR = [math]::Round($peakPrivateMb, 2)
$cpuPass = ($avgCpuR -lt $cpuBudget)
$ramPass = ($peakMemR -lt $ramBudgetMb)

# HW inventory: bench mode runs the engine without a video, so no hwdec
# stream is active. Never silent: explicit no-hwdec flag.
$gpuNames = @()
try {
    $gpuNames = @(Get-CimInstance Win32_VideoController -ErrorAction SilentlyContinue |
        ForEach-Object { $_.Name } | Where-Object { $_ })
} catch { $gpuNames = @() }
if ($gpuNames.Count -eq 0) { $gpuNames = @("unknown") }

$shortNote = ""
if ($Minutes -lt 5) {
    $shortNote = "SHORT RUN: $Minutes min < 5-min full gate window; full 5-min evidence lives in docs/bench_fase1.json."
}

$result = [ordered]@{
    tool = "bench_cpu_mem"
    date = (Get-Date -Format "yyyy-MM-dd")
    engine = "build/msvc-dev/engine.exe"
    hw = [ordered]@{
        gpu = ($gpuNames -join "; ")
        hwdec = "none"
        no_hw = $true
        note = "bench mode runs engine without a video (no mpv instance); 2% CPU budget assumes hwdec playback"
    }
    minutes = $Minutes
    samples = $memSamples.Count
    avg_cpu_percent = $avgCpuR
    avg_private_mb = $avgMemR
    peak_private_mb = $peakMemR
    budget = [ordered]@{
        cpu_idle_pct = $cpuBudget
        ram_engine_mb = $ramBudgetMb
    }
    verdict = [ordered]@{
        cpu_pass = $cpuPass
        ram_pass = $ramPass
    }
    pass = ($cpuPass -and $ramPass)
    reproducibility = "QueryProcessCycleTime deltas / QPF, ${IntervalMs}ms interval. Rerun +-10% on avg_cpu_percent under identical load; close other apps before comparing. $shortNote"
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
    Write-Host "bench_cpu_mem: wrote $outFull"
}