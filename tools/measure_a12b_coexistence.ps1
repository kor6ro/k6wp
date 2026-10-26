# measure_a12b_coexistence.ps1 - A-12 Langkah 1: does a Studio holding the pipe
# starve a second client (CLI/tray)?
#
# Control: engine only + stress_pause_resume (1 thread, then 4 threads).
# Test:    the same stress while the Studio is open and holding the connection.
#
# The Studio is the release build by default (it uses the real library.json).
# The session (wallpaper/display/lock screen) is snapshotted and restored.
#
# Usage:
#   powershell -File tools/measure_a12b_coexistence.ps1 `
#       -Studio <k6wp-studio.exe> -Stress <stress_pause_resume.exe> `
#       -Engine build/msvc-dev/engine.exe
param(
    [Parameter(Mandatory = $true)][string]$Studio,
    [Parameter(Mandatory = $true)][string]$Stress,
    [string]$Engine = "build/msvc-dev/engine.exe",
    [string]$OutDir = "docs/bench",
    [int]$Threads1 = 1,
    [int]$Requests1 = 300,
    [int]$Threads4 = 4,
    [int]$Requests4 = 150
)

$ErrorActionPreference = "Stop"
if (-not (Test-Path $OutDir)) { New-Item -ItemType Directory -Path $OutDir -Force | Out-Null }
. (Join-Path (Split-Path -Parent $MyInvocation.MyCommand.Path) "session_snapshot.ps1")

function Wait-Engine([int]$Seconds = 15) {
    $deadline = (Get-Date).AddSeconds($Seconds)
    $previous = $ErrorActionPreference
    $ErrorActionPreference = "Continue"   # native stderr must not terminate
    try {
        while ((Get-Date) -lt $deadline) {
            & $Stress --threads 1 --requests 1 --interval-min 1 --interval-max 1 2>$null | Out-Null
            if ($LASTEXITCODE -eq 0) { return $true }
            Start-Sleep -Milliseconds 500
        }
        return $false
    } finally {
        $ErrorActionPreference = $previous
    }
}

function Invoke-Stress([string]$Label, [int]$Threads, [int]$Requests, [string]$Path) {
    Write-Host "stress: $Label ($Threads thread(s), $Requests each)"
    # The stress client reports on stderr; Windows PowerShell 5.1 turns native
    # stderr into a terminating error under ErrorActionPreference=Stop.
    $previous = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    try {
        & $Stress --threads $Threads --requests $Requests --json $Path 2>$null | Out-Null
        $code = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $previous
    }
    if ($code -ne 0) { throw "stress $Label failed with $code" }
}

Invoke-SessionSafe {
    $startedEngine = $null
    if (-not (Get-Process engine -ErrorAction SilentlyContinue)) {
        $startedEngine = Start-Process -FilePath $Engine -PassThru
        Write-Host "engine started: pid $($startedEngine.Id)"
    }
    if (-not (Wait-Engine)) { throw "the engine never answered on its pipe" }
    Start-Sleep -Milliseconds 800

    Invoke-Stress "control-1t" $Threads1 $Requests1 (Join-Path $OutDir "a12b-control-1t.json")
    Invoke-Stress "control-4t" $Threads4 $Requests4 (Join-Path $OutDir "a12b-control-4t.json")

    $studioProc = Start-Process -FilePath $Studio -PassThru
    Write-Host "studio started: pid $($studioProc.Id); waiting for its first poll"
    Start-Sleep -Seconds 5
    if ($studioProc.HasExited) { throw "the Studio exited before the test" }

    Invoke-Stress "studio-1t" $Threads1 $Requests1 (Join-Path $OutDir "a12b-studio-1t.json")
    Invoke-Stress "studio-4t" $Threads4 $Requests4 (Join-Path $OutDir "a12b-studio-4t.json")

    # One more single request with stderr kept: the exact reason a second
    # client cannot get in while the Studio holds the connection.
    $errPath = Join-Path $OutDir "a12b-studio-error.txt"
    $previous = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    try {
        & $Stress --threads 1 --requests 2 --interval-min 1 --interval-max 1 2> $errPath | Out-Null
    } finally {
        $ErrorActionPreference = $previous
    }
    Stop-Process -Id $studioProc.Id -Force -ErrorAction SilentlyContinue
    if ($startedEngine) { Stop-Process -Id $startedEngine.Id -Force -ErrorAction SilentlyContinue }
}

$summary = [ordered]@{
    schema    = "k6wp.bench.ipc-coexistence/1"
    created   = (Get-Date).ToString("o")
    engine    = $Engine
    studio    = $Studio
    stress    = $Stress
    control   = [ordered]@{}
    studio_up = [ordered]@{}
}
foreach ($pair in @(
    @{ side = "control"; file = "a12b-control-1t.json" },
    @{ side = "control"; file = "a12b-control-4t.json" },
    @{ side = "studio_up"; file = "a12b-studio-1t.json" },
    @{ side = "studio_up"; file = "a12b-studio-4t.json" }
)) {
    $path = Join-Path $OutDir $pair.file
    if (Test-Path $path) {
        $summary[$pair.side][$pair.file] = Get-Content $path -Raw | ConvertFrom-Json
    }
}
$errPath = Join-Path $OutDir "a12b-studio-error.txt"
if (Test-Path $errPath) {
    $summary.studio_up["error_first_line"] =
        ((Get-Content $errPath | Select-Object -First 1) -replace '^stress_pause_resume\.exe : ', '')
}
$summary | ConvertTo-Json -Depth 5 -Compress | Set-Content -Encoding utf8 (Join-Path $OutDir "studio-ipc-coexistence.json")
Write-Host "wrote $(Join-Path $OutDir 'studio-ipc-coexistence.json')"
