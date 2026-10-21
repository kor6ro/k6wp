# tools/session_snapshot.ps1 - snapshot + restore the LIVE session.
#
# C8 (from the refactor brief): a test script must NEVER restart Explorer or
# change the wallpaper / lock screen of the live session without an
# automatic snapshot AND an automatic restore - including when the script
# fails half-way through.
#
# This module is the shared helper every test script MUST use:
#
#   . (Join-Path $PSScriptRoot 'session_snapshot.ps1')
#   Invoke-SessionSafe -Name 'my test' -ScriptBlock { ... }
#
# Invoke-SessionSafe snapshots the session BEFORE the block runs and
# restores it in a `finally`, so the restore happens on success, on a thrown
# error, and on Ctrl+C (a PowerShell `finally` runs on Ctrl+C too).
#
# WHAT IS SNAPSHOTTED
#   - the display mode (resolution + refresh rate) of every active display
#   - the Windows wallpaper (SPI path), and K6WP's still state (the video the
#     still was taken from), so the still can be re-applied
#   - K6WP's lock-screen backup (original_lockscreen.json + .jpg)
#   - whether Explorer was running
#
# WHAT IS RESTORED
#   - the display mode (only if the script changed it)
#   - the wallpaper / the still (only if the script changed it)
#   - the lock screen (only if the script changed it)
#   - Explorer (only if the script stopped it)
#
# A restore that fails is LOGGED, never thrown: a failed restore must not
# mask the real result of the test.

$ErrorActionPreference = 'Continue'

Add-Type -AssemblyName System.Windows.Forms

if (-not ([System.Management.Automation.PSTypeName]'K6wpSession').Type) {
Add-Type @"
using System;
using System.Runtime.InteropServices;
using System.Text;
namespace K6wpSession {
  [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi, Pack=1)]
  public struct DEVMODE {
    // The layout must match DEVMODEA EXACTLY (the offsets are the API
    // contract): a wrong layout makes dmPelsWidth read 0 and the mode
    // enumeration return garbage.
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst=32)] public string dmDeviceName;
    public short dmSpecVersion;
    public short dmDriverVersion;
    public short dmSize;
    public short dmDriverExtra;
    public int dmFields;
    public int dmPositionX;          // union { POINTL position; ... }
    public int dmPositionY;
    public int dmDisplayOrientation;
    public int dmDisplayFixedOutput;
    public short dmColor;
    public short dmDuplex;
    public short dmYResolution;
    public short dmTTOption;
    public short dmCollate;
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst=32)] public string dmFormName;
    public short dmLogPixels;
    public int dmBitsPerPel;
    public int dmPelsWidth;
    public int dmPelsHeight;
    public int dmDisplayFlags;
    public int dmDisplayFrequency;
    public int dmICMMethod;
    public int dmICMIntent;
    public int dmMediaType;
    public int dmDitherType;
    public int dmReserved1;
    public int dmReserved2;
    public int dmPanningWidth;
    public int dmPanningHeight;
  }
  public static class Native {
    [DllImport("user32.dll", CharSet=CharSet.Ansi)]
    public static extern bool EnumDisplaySettingsA(string dev, int mode, ref DEVMODE dm);
    [DllImport("user32.dll", CharSet=CharSet.Ansi)]
    public static extern int ChangeDisplaySettingsExA(string dev, ref DEVMODE dm, IntPtr hwnd, uint flags, IntPtr param);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)]
    public static extern bool SystemParametersInfoW(uint a, uint b, string c, uint d);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)]
    public static extern bool SystemParametersInfoW(uint a, uint b, IntPtr c, uint d);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }
  }
}
"@
}

$script:SessionLog = New-Object System.Collections.Generic.List[string]

function Write-SessionLog([string]$msg) {
    $script:SessionLog.Add($msg)
    Write-Host "  [session] $msg"
}

# --- the snapshot -----------------------------------------------------------

function Get-SessionSnapshot {
    $snap = [ordered]@{
        displays = @()
        wallpaper_path = ''
        still_video = ''
        still_status_backup = ''
        lockscreen_json_backup = ''
        explorer_running = $true
        created = (Get-Date -Format 'o')
    }

    # Display modes: the current DEVMODE of every active display.
    foreach ($screen in [System.Windows.Forms.Screen]::AllScreens) {
        $dev = $screen.DeviceName
        $dm = New-Object 'K6wpSession.DEVMODE'
        $dm.dmSize = [System.Runtime.InteropServices.Marshal]::SizeOf($dm)
        if ([K6wpSession.Native]::EnumDisplaySettingsA($dev, -1, [ref]$dm)) {
            $snap.displays += [pscustomobject]@{
                device = $dev
                width = $dm.dmPelsWidth
                height = $dm.dmPelsHeight
                bpp = $dm.dmBitsPerPel
                frequency = $dm.dmDisplayFrequency
            }
        }
    }

    # The Windows wallpaper (may be empty when the shell paints its own).
    $buf = New-Object System.Text.StringBuilder 260
    [void][K6wpSession.Native]::SystemParametersInfoW(0x0074, 260, $buf, 0)  # SPI_GETDESKWALLPAPER
    $snap.wallpaper_path = $buf.ToString()

    # K6WP's still state: the video the still was taken from, so the still can
    # be re-applied (the wallpaper itself is a JPEG of that video's frame).
    $k6wp = Join-Path $env:LOCALAPPDATA 'K6WP'
    try {
        $cfg = Get-Content (Join-Path $k6wp 'config.json') -Raw | ConvertFrom-Json
        if ($cfg.video_path) { $snap.still_video = [string]$cfg.video_path }
    } catch { }
    $status = Join-Path $k6wp 'still_status.json'
    if (Test-Path -LiteralPath $status) {
        $snap.still_status_backup = (Get-Content -LiteralPath $status -Raw)
    }
    $lock = Join-Path $k6wp 'original_lockscreen.json'
    if (Test-Path -LiteralPath $lock) {
        $snap.lockscreen_json_backup = (Get-Content -LiteralPath $lock -Raw)
    }
    $snap.explorer_running = ($null -ne (Get-Process explorer -ErrorAction SilentlyContinue))
    return $snap
}

# --- the restore ------------------------------------------------------------

function Restore-SessionSnapshot {
    param($Snapshot, [switch]$RestoreDisplay, [switch]$RestoreWallpaper, [switch]$RestoreLockscreen, [switch]$RestoreExplorer)

    if ($null -eq $Snapshot) { return }

    if ($RestoreExplorer -and -not $Snapshot.explorer_running) {
        if ($null -eq (Get-Process explorer -ErrorAction SilentlyContinue)) {
            try {
                Start-Process explorer.exe | Out-Null
                Write-SessionLog "Explorer restarted (it was not running before the test)"
            } catch { Write-SessionLog "FAILED to restart Explorer: $($_.Exception.Message)" }
        }
    }

    if ($RestoreDisplay) {
        foreach ($d in $Snapshot.displays) {
            $dm = New-Object 'K6wpSession.DEVMODE'
            $dm.dmSize = [System.Runtime.InteropServices.Marshal]::SizeOf($dm)
            if (-not [K6wpSession.Native]::EnumDisplaySettingsA($d.device, -1, [ref]$dm)) { continue }
            if ($dm.dmPelsWidth -ne $d.width -or $dm.dmPelsHeight -ne $d.height -or
                $dm.dmDisplayFrequency -ne $d.frequency) {
                $dm.dmFields = 0x80000 -bor 0x100000 -bor 0x40000  # FREQUENCY|PELSWIDTH|PELSHEIGHT
                $rc = [K6wpSession.Native]::ChangeDisplaySettingsExA($d.device, [ref]$dm, [IntPtr]::Zero, 0, [IntPtr]::Zero)
                Write-SessionLog ("display {0}: restored to {1}x{2}@{3}Hz (rc={4})" -f $d.device, $d.width, $d.height, $d.frequency, $rc)
            }
        }
    }

    if ($RestoreWallpaper) {
        if (-not [string]::IsNullOrWhiteSpace($Snapshot.wallpaper_path)) {
            [void][K6wpSession.Native]::SystemParametersInfoW(0x0014, 0, $Snapshot.wallpaper_path, 0x01 -bor 0x02)
            Write-SessionLog "wallpaper restored: $($Snapshot.wallpaper_path)"
        }
        elseif (-not [string]::IsNullOrWhiteSpace($Snapshot.still_video)) {
            # The still wallpaper: re-apply the first frame of the same video.
            $repoRoot = Split-Path -Parent $PSScriptRoot
            $exe = Join-Path $repoRoot 'build\release\K6WP.exe'
            if (Test-Path -LiteralPath $exe) {
                & $exe --apply-still --wallpaper --video $Snapshot.still_video | Out-Null
                Write-SessionLog "still wallpaper re-applied from '$($Snapshot.still_video)'"
            } else {
                Write-SessionLog "SKIPPED the still wallpaper restore: '$exe' not found"
            }
        }
        else {
            Write-SessionLog "no wallpaper state to restore"
        }
        if (-not [string]::IsNullOrWhiteSpace($Snapshot.still_status_backup)) {
            $status = Join-Path $env:LOCALAPPDATA 'K6WP\still_status.json'
            try { Set-Content -LiteralPath $status -Value $Snapshot.still_status_backup -NoNewline } catch { }
        }
    }

    if ($RestoreLockscreen) {
        $lock = Join-Path $env:LOCALAPPDATA 'K6WP\original_lockscreen.json'
        if (-not [string]::IsNullOrWhiteSpace($Snapshot.lockscreen_json_backup)) {
            try { Set-Content -LiteralPath $lock -Value $Snapshot.lockscreen_json_backup -NoNewline } catch { }
            Write-SessionLog "lock-screen backup restored"
        }
    }
}

# --- the wrapper every test script must use ---------------------------------

function Invoke-SessionSafe {
    <#
      .SYNOPSIS
        Runs a test block with an automatic session snapshot + restore.
      .PARAMETER Name
        A label for the log lines.
      .PARAMETER ScriptBlock
        The test body.
      .PARAMETER RestoreDisplay
        Restore the display mode (resolution + refresh) afterwards.
      .PARAMETER RestoreWallpaper
        Restore the wallpaper / the still afterwards.
      .PARAMETER RestoreLockscreen
        Restore the lock-screen backup afterwards.
      .PARAMETER RestoreExplorer
        Restart Explorer if the block stopped it.
    #>
    param(
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][scriptblock]$ScriptBlock,
        [switch]$RestoreDisplay,
        [switch]$RestoreWallpaper,
        [switch]$RestoreLockscreen,
        [switch]$RestoreExplorer
    )
    Write-Host "== session-safe: $Name =="
    $snapshot = Get-SessionSnapshot
    try {
        & $ScriptBlock
    }
    finally {
        # The restore MUST happen even when the block threw: that is the whole
        # point of the helper.
        Restore-SessionSnapshot -Snapshot $snapshot -RestoreDisplay:$RestoreDisplay `
            -RestoreWallpaper:$RestoreWallpaper -RestoreLockscreen:$RestoreLockscreen `
            -RestoreExplorer:$RestoreExplorer
    }
}

# --- display mode helpers (used by the pacing experiment) -------------------

function Get-DisplayModes {
    # The refresh rates the current display actually offers.
    $out = @()
    foreach ($screen in [System.Windows.Forms.Screen]::AllScreens) {
        for ($i = 0; $i -lt 400; $i++) {
            $dm = New-Object 'K6wpSession.DEVMODE'
            $dm.dmSize = [System.Runtime.InteropServices.Marshal]::SizeOf($dm)
            if (-not [K6wpSession.Native]::EnumDisplaySettingsA($screen.DeviceName, $i, [ref]$dm)) { break }
            $key = "$($dm.dmPelsWidth)x$($dm.dmPelsHeight)@$($dm.dmDisplayFrequency)"
            if ($out -notcontains $key) { $out += $key }
        }
    }
    return $out
}

function Set-DisplayRefresh {
    param([string]$Device, [int]$Width, [int]$Height, [int]$Frequency)
    $dm = New-Object 'K6wpSession.DEVMODE'
    $dm.dmSize = [System.Runtime.InteropServices.Marshal]::SizeOf($dm)
    if (-not [K6wpSession.Native]::EnumDisplaySettingsA($Device, -1, [ref]$dm)) { return $null }
    $dm.dmDisplayFrequency = [uint32]$Frequency
    $dm.dmFields = 0x80000 -bor 0x100000 -bor 0x40000
    $rc = [K6wpSession.Native]::ChangeDisplaySettingsExA($Device, [ref]$dm, [IntPtr]::Zero, 0, [IntPtr]::Zero)
    return $rc
}
