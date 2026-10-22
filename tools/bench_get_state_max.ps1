# tools/bench_get_state_max.ps1 - L1: the `get_state` max, measured against a
# PRIVATE engine instance so the live wallpaper session is never touched.
#
# WHY A PRIVATE INSTANCE
#   The K1 stress tool (tools/stress_ipc.ps1) stops the engine that is running
#   and starts the one it was given. That is correct for a dedicated bench
#   machine, but it takes the user's wallpaper down. This tool does the
#   opposite: it starts a SECOND engine beside the live one, with a unique
#   singleton mutex and pipe (the debug-only overrides), on a config that has
#   no video and a monitor id no display matches. The engine therefore owns
#   ZERO wallpaper slots and never paints a pixel.
#
# WHAT IT MEASURES
#   The same Rust stress client K1 used (4 clients by default) hammers
#   `get_state` for N requests and records every latency. The report carries
#   the p50/p95/p99/max and, with `-WithTrace`, the accumulator's per-label
#   n/mean/max from `ipc_trace.hpp` (debug builds only).
#
#   The MEANS are the honest per-field numbers: the accumulator adds a clock
#   pair, a map lookup and a mutex per scope, and the engine is a Debug build,
#   so every mean is an UPPER BOUND on the release cost.
#
#   The trace-on MAX is usable since L2: the accumulator logs its summary only
#   when the OUTERMOST scope exits (a thread-local depth counter), after its
#   own duration was taken - no measured block encloses the flush. Before L2
#   the flush landed inside the scope every 200 samples and the max was fake
#   (9-15 ms); the L1 gate therefore ran with the trace OFF, and it still does.
#   The trace-off run remains the latency gate.
#
#   A max is ONE sample: under a live session it can be an OS scheduler stall
#   (L2 saw 31-62 ms maxima with a p99 of 0-1 ms). Quote the p99 next to any
#   max, and the sample series (`-SamplesOut`) when a max has to be explained.
#
# THE EVIDENCE RULE
#   One invocation = one run. The L1 gate needs n >= 5 runs per binary, with
#   the same parameters; the caller loops and keeps every JSON.
#
# USAGE
#   powershell -File tools/bench_get_state_max.ps1 -EngineExe build\msvc-dev\engine.exe -Label before-1 -OutJson docs\bench\l1\before-1.json
#
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$EngineExe,
    [Parameter(Mandatory = $true)][string]$Label,
    [Parameter(Mandatory = $true)][string]$OutJson,
    [string]$SamplesOut = '',
    [int]$Threads = 4,
    [int]$Requests = 2000,
    [int]$ReadyTimeoutSec = 20,
    # OFF by default: the trace adds a clock pair, a map lookup and a mutex
    # per scope, so the means are upper bounds - and the client latency is the
    # gate, not the trace. Use `-WithTrace` for the per-label means (L2) and
    # for the per-field maxima (usable since the L2 depth gate: the summary
    # logs only at the outermost scope, so no measured block encloses the
    # flush). NOTE the name: a [switch] parameter types its variable, so it
    # must NOT be called `-Trace` if the script also has a `$trace` accumulator
    # (case-insensitive variables + a type constraint = "Cannot convert
    # OrderedDictionary to SwitchParameter").
    [switch]$WithTrace
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$EngineExe = (Resolve-Path -LiteralPath $EngineExe).Path
$client = Join-Path $root 'rust\target\release\examples\stress_client.exe'
if (-not (Test-Path -LiteralPath $client)) {
    throw "the stress client is not built: $client (cargo build --release --example stress_client -p k6wp-ipc)"
}

$runId = [Guid]::NewGuid().ToString('N').Substring(0, 8)
$mutexName = "Local\K6WP-Engine-L1-$runId"
$pipeName = "\\.\pipe\k6wp-engine-l1-$runId"
$workDir = Join-Path $env:TEMP "k6wp_l1_$runId"
New-Item -ItemType Directory -Force -Path $workDir | Out-Null
$configPath = Join-Path $workDir 'config.json'
$clientJson = Join-Path $workDir 'client.json'
$probeJson = Join-Path $workDir 'probe.json'
$logPath = Join-Path $env:LOCALAPPDATA 'K6WP\engine.log'

# A valid v6 config with NO video and a monitor id that matches no display:
# the engine serves IPC, owns zero slots, and paints nothing.
$config = [ordered]@{
    version                = 6
    video_path             = ''
    fit_mode               = 'cover'
    speed                  = 1.0
    monitor_id             = 99
    crf                    = 23
    resolution_w           = 1920
    resolution_h           = 1080
    fps_cap                = 30
    battery_saver          = $false
    battery_mode           = 'cap24'
    cpu_affinity           = 'auto'
    gpu_adapter            = 'auto'
    reconcile_mode         = 'legacy'
    pause_unload_after_sec = 30
}
# NO BOM: the config parser rejects a leading BOM (the H4 rule).
$jsonConfig = $config | ConvertTo-Json
[IO.File]::WriteAllText($configPath, $jsonConfig, (New-Object Text.UTF8Encoding($false)))

$logStart = 0
if (Test-Path -LiteralPath $logPath) { $logStart = (Get-Item -LiteralPath $logPath).Length }

function Read-LogDelta {
    if (-not (Test-Path -LiteralPath $logPath)) { return '' }
    $stream = [IO.File]::Open($logPath, 'Open', 'Read', 'ReadWrite')
    try {
        $null = $stream.Seek($logStart, 'Begin')
        $reader = New-Object IO.StreamReader($stream)
        return $reader.ReadToEnd()
    } finally {
        $stream.Dispose()
    }
}

function Invoke-StressClient {
    param([string[]]$ClientArgs)
    # The client reports on STDERR; PS 5.1 with EAP=Stop turns a native
    # command's stderr into a TERMINATING error, so the call runs under
    # EAP=Continue and the exit code is read explicitly.
    $old = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try {
        $lines = & $client @ClientArgs 2>&1
        $code = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $old
    }
    return [pscustomobject]@{ Code = $code; Output = @($lines) }
}

$engineProcess = $null
try {
    $env:K6WP_ENGINE_MUTEX = $mutexName
    $env:K6WP_ENGINE_PIPE = $pipeName
    if ($WithTrace) { $env:K6WP_IPC_TRACE = '1' }
    try {
        $engineProcess = Start-Process -FilePath $EngineExe `
            -ArgumentList @('--config', "`"$configPath`"") `
            -WindowStyle Hidden -PassThru
    } finally {
        Remove-Item Env:\K6WP_ENGINE_MUTEX -ErrorAction SilentlyContinue
        Remove-Item Env:\K6WP_ENGINE_PIPE -ErrorAction SilentlyContinue
        Remove-Item Env:\K6WP_IPC_TRACE -ErrorAction SilentlyContinue
    }

    # Readiness: a probe request, not a sleep. A connect failure shows up as
    # failures=1 in the probe report (the client itself exits 0).
    $deadline = (Get-Date).AddSeconds($ReadyTimeoutSec)
    $ready = $false
    while (-not $ready -and (Get-Date) -lt $deadline) {
        Start-Sleep -Milliseconds 300
        if ($engineProcess.HasExited) {
            throw ("the private engine exited early (code {0}) - is the binary a DEBUG build? " +
                   "the K6WP_ENGINE_* overrides are compiled out of release builds" -f $engineProcess.ExitCode)
        }
        Remove-Item -LiteralPath $probeJson -Force -ErrorAction SilentlyContinue
        $null = Invoke-StressClient @('--threads', '1', '--requests', '1',
            '--pipe', $pipeName, '--json', $probeJson)
        if (Test-Path -LiteralPath $probeJson) {
            try {
                $probe = Get-Content -LiteralPath $probeJson -Raw | ConvertFrom-Json
                if ($probe.requests -ge 1 -and $probe.failures -eq 0) { $ready = $true }
            } catch { }
        }
    }
    if (-not $ready) { throw "the private engine did not answer on $pipeName within $ReadyTimeoutSec s" }

    # The measured run: the SAME client K1 used, with the same parameters on
    # both sides of the A/B. No churn: the spike lives in `get_state` itself.
    $clientArgs = @(
        '--threads', "$Threads",
        '--requests', "$Requests",
        '--pipe', $pipeName,
        '--json', $clientJson
    )
    if ($SamplesOut) { $clientArgs += @('--samples', $SamplesOut) }
    $run = Invoke-StressClient $clientArgs
    $run.Output | ForEach-Object { Write-Host "  [l1] $_" }
    if ($run.Code -ne 0) { throw "the stress client failed (exit $($run.Code))" }
    $clientReport = Get-Content -LiteralPath $clientJson -Raw | ConvertFrom-Json

    # The trace summary: the LAST line per label in the delta (the summary is
    # logged every 200 samples per label, so the last one covers the run).
    # Every label is kept: L2 measures the per-field scopes (state.*), so the
    # report must not whitelist a fixed set.
    $trace = [ordered]@{}
    $delta = Read-LogDelta
    foreach ($line in ($delta -split "`r?`n")) {
        if ($line -match 'ipc-trace: (.+?): n=(\d+) the mean=(-?\d+) us the max=(\d+) us') {
            $trace[$Matches[1]] = [ordered]@{
                n       = [int]$Matches[2]
                mean_us = [int]$Matches[3]
                max_us  = [int]$Matches[4]
            }
        }
    }

    $report = [ordered]@{
        tool         = 'bench_get_state_max'
        task         = 'L1/L2'
        label        = $Label
        date         = (Get-Date).ToString('o')
        engine       = $EngineExe
        private      = $true
        mutex        = $mutexName
        pipe         = $pipeName
        threads      = $Threads
        requests     = $clientReport.requests
        p50_ms       = $clientReport.p50_ms
        p95_ms       = $clientReport.p95_ms
        p99_ms       = $clientReport.p99_ms
        max_ms       = $clientReport.max_ms
        failures     = $clientReport.failures
        wall_seconds = $clientReport.wall_seconds
        trace        = $trace
    }
    $outDir = Split-Path -Parent ([IO.Path]::GetFullPath($OutJson))
    if (-not (Test-Path -LiteralPath $outDir)) { New-Item -ItemType Directory -Force -Path $outDir | Out-Null }
    $report | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $OutJson -Encoding UTF8
    Write-Host ("  [l1] {0}: {1} requests, the p50 {2} ms, the p99 {3} ms, the max {4} ms" -f `
            $Label, $report.requests, $report.p50_ms, $report.p99_ms, $report.max_ms)
    if ($trace.Count -gt 0) {
        Write-Host "  [l2] the trace (n / mean / max, us):"
        foreach ($name in ($trace.Keys | Sort-Object)) {
            $t = $trace[$name]
            Write-Host ("       {0,-18} {1,6}  {2,8}  {3,8}" -f $name, $t.n, $t.mean_us, $t.max_us)
        }
    }
    else {
        Write-Host "  [l2] no trace labels (run with -WithTrace on a debug engine)"
    }
} finally {
    if ($engineProcess -and -not $engineProcess.HasExited) {
        Stop-Process -Id $engineProcess.Id -Force -ErrorAction SilentlyContinue
        $engineProcess.WaitForExit(5000) | Out-Null
    }
    Remove-Item -LiteralPath $workDir -Recurse -Force -ErrorAction SilentlyContinue
}
