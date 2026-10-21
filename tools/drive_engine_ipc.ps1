# tools/drive_engine_ipc.ps1 - the G6 live drive: the Rust IPC client against
# the REAL engine.
#
# WHAT IT DOES
#   It runs the Rust live test (k6wp-ipc's live_engine) against the running
#   engine, which drives the WHOLE command set: get_state, set_video, pause,
#   resume, set_display_video, an unknown command, a payload with an unknown
#   key, a reconnect and the quit.
#
# THE RULES (C8)
#   The live session is only touched through tools/session_snapshot.ps1: the
#   display mode, the wallpaper, the still and the lock screen are snapshotted
#   BEFORE the drive and restored in the `finally` - including when the drive
#   fails half-way. The engine is restarted in the same `finally` (the quit
#   at the end of the drive stops it).
#
[CmdletBinding()]
param(
    [string]$EngineExe = '',
    [switch]$KeepEngineStopped
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'session_snapshot.ps1')

if (-not $EngineExe) {
    $EngineExe = Join-Path $root 'build\release\engine.exe'
}
if (-not (Test-Path $EngineExe)) {
    throw "the engine is not built: $EngineExe"
}
$EngineExe = (Resolve-Path $EngineExe).Path

$env:K6WP_IPC_LIVE = '1'
$env:K6WP_IPC_ENGINE_EXE = $EngineExe

try {
    Invoke-SessionSafe -Name 'the G6 IPC drive' `
        -RestoreDisplay -RestoreWallpaper -RestoreLockscreen `
        -ScriptBlock {
        # The engine must be running before the drive (the user may have it
        # already; if not, start it).
        $engine = Get-Process engine -ErrorAction SilentlyContinue
        if (-not $engine) {
            Write-Host '  [ipc] the engine is not running - starting it'
            Start-Process -FilePath $EngineExe -WindowStyle Hidden
            Start-Sleep -Seconds 8
        }

        Write-Host '  [ipc] driving the engine through the Rust client'
        Push-Location (Join-Path $root 'rust')
        try {
            cargo test -p k6wp-ipc --test live_engine -- --nocapture
            if ($LASTEXITCODE -ne 0) {
                throw "the live IPC drive FAILED (the exit code was $LASTEXITCODE)"
            }
        } finally {
            Pop-Location
        }
    }
} finally {
    # The drive quits the engine: the live session must come back.
    if (-not $KeepEngineStopped) {
        $engine = Get-Process engine -ErrorAction SilentlyContinue
        if (-not $engine) {
            Write-Host '  [ipc] restarting the engine (the drive quit it)'
            Start-Process -FilePath $EngineExe -WindowStyle Hidden
            Start-Sleep -Seconds 8
        }
        $engine = Get-Process engine -ErrorAction SilentlyContinue
        if ($engine) {
            $mb = [math]::Round($engine[0].PrivateMemorySize64 / 1MB, 1)
            Write-Host "  [ipc] the engine is back (pid $($engine[0].Id), $mb MB private)"
        } else {
            Write-Warning '  [ipc] the engine did NOT come back - start it by hand'
        }
    }
    Remove-Item Env:\K6WP_IPC_LIVE -ErrorAction SilentlyContinue
    Remove-Item Env:\K6WP_IPC_ENGINE_EXE -ErrorAction SilentlyContinue
}

Write-Host 'drive_engine_ipc: done'
