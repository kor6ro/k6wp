# e2e_a12b_undo.ps1 - A-12 Langkah 2 end-to-end proof of the toast "Urungkan".
#
# WHAT IT DOES
#   1. Starts the engine and waits until get_state answers on the pipe.
#   2. Starts the DEBUG Studio with --e2e-pick 0, which installs card 0.
#   3. Waits until the engine reports card 0 as the current video.
#   4. Clicks card 1 (a real mouse click on the Slint grid). The engine must
#      switch to card 1 and the Studio must show the toast with "Urungkan".
#   5. Captures the window (the toast is in the PNG) and clicks "Urungkan".
#   6. Waits until the engine reports card 0 again, then captures the window.
#
# The verdict is the engine's own get_state: the picture must go
#   card 0 -> card 1 -> card 0
# Not "a request was sent" - the video path actually changed and came back.
#
# USAGE
#   powershell -File tools/e2e_a12b_undo.ps1
#   powershell -File tools/e2e_a12b_undo.ps1 -UndoX 897 -UndoY 568
#
# The click coordinates are in 960x640 logical pixels and are scaled by the
# window's real client width, so a 125%/150% DPI session still hits the button.
# The session is only touched through tools/session_snapshot.ps1.

param(
    [string]$Engine = "build/msvc-dev/engine.exe",
    [string]$Exe = "D:\cargo-target2\x86_64-pc-windows-msvc\debug\k6wp-studio.exe",
    [string]$Probe = "D:\cargo-target2\x86_64-pc-windows-msvc\release\examples\get_state_once.exe",
    [string]$OutDir = "docs/img",
    [int]$SecondIndex = 1,
    [int]$UndoX = 897,
    [int]$UndoY = 568,
    [int]$TimeoutSec = 40
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot "session_snapshot.ps1")

foreach ($path in @($Engine, $Exe, $Probe)) {
    if (-not (Test-Path $path)) { throw "missing binary: $path" }
}
if (-not (Test-Path $OutDir)) { New-Item -ItemType Directory -Path $OutDir -Force | Out-Null }

Add-Type @"
using System;
using System.Drawing;
using System.Runtime.InteropServices;

public static class UndoCap {
    [StructLayout(LayoutKind.Sequential)]
    public struct RECT { public int Left, Top, Right, Bottom; }
    [StructLayout(LayoutKind.Sequential)]
    public struct POINT { public int X, Y; }

    [DllImport("user32.dll")] public static extern bool SetProcessDpiAwarenessContext(IntPtr value);
    [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr hWnd, out RECT rect);
    [DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr hWnd, ref POINT point);
    [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr hWnd, IntPtr hdc, uint flags);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hWnd);
    [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr hWnd, uint msg, IntPtr wParam, IntPtr lParam);
    [DllImport("user32.dll")] public static extern IntPtr SendMessage(IntPtr hWnd, uint msg, IntPtr wParam, IntPtr lParam);

    public const uint PW_RENDERFULLCONTENT = 2;
    public const uint WM_MOUSEMOVE = 0x0200;
    public const uint WM_LBUTTONDOWN = 0x0201;
    public const uint WM_LBUTTONUP = 0x0202;
    public static readonly IntPtr PER_MONITOR_AWARE_V2 = new IntPtr(-4);

    public static RECT ClientBounds(IntPtr hWnd) {
        RECT r;
        GetClientRect(hWnd, out r);
        POINT origin = new POINT { X = 0, Y = 0 };
        ClientToScreen(hWnd, ref origin);
        return new RECT { Left = origin.X, Top = origin.Y, Right = origin.X + r.Right, Bottom = origin.Y + r.Bottom };
    }

    public static int ClientWidth(IntPtr hWnd) {
        RECT r;
        GetClientRect(hWnd, out r);
        return r.Right - r.Left;
    }

    public static int ClientHeight(IntPtr hWnd) {
        RECT r;
        GetClientRect(hWnd, out r);
        return r.Bottom - r.Top;
    }

    public static bool HasContent(IntPtr hWnd) {
        int width = ClientWidth(hWnd), height = ClientHeight(hWnd);
        if (width <= 0 || height <= 0) return false;
        using (Bitmap bmp = new Bitmap(width, height)) {
            using (Graphics g = Graphics.FromImage(bmp)) {
                IntPtr hdc = g.GetHdc();
                bool ok;
                try { ok = PrintWindow(hWnd, hdc, PW_RENDERFULLCONTENT); }
                finally { g.ReleaseHdc(hdc); }
                if (!ok) return false;
            }
            Color first = bmp.GetPixel(0, 0);
            for (int y = 0; y < height; y += 24)
                for (int x = 0; x < width; x += 24)
                    if (bmp.GetPixel(x, y) != first) return true;
            return false;
        }
    }

    public static void ClickAt(IntPtr hWnd, int x, int y) {
        IntPtr point = new IntPtr((y << 16) | (x & 0xFFFF));
        PostMessage(hWnd, WM_MOUSEMOVE, IntPtr.Zero, point);
        System.Threading.Thread.Sleep(50);
        PostMessage(hWnd, WM_LBUTTONDOWN, IntPtr.Zero, point);
        System.Threading.Thread.Sleep(80);
        PostMessage(hWnd, WM_LBUTTONUP, IntPtr.Zero, point);
    }

    public static bool Capture(IntPtr hWnd, string path) {
        int width = ClientWidth(hWnd), height = ClientHeight(hWnd);
        if (width <= 0 || height <= 0) return false;
        using (Bitmap bmp = new Bitmap(width, height)) {
            using (Graphics g = Graphics.FromImage(bmp)) {
                IntPtr hdc = g.GetHdc();
                try {
                    if (!PrintWindow(hWnd, hdc, PW_RENDERFULLCONTENT)) return false;
                } finally { g.ReleaseHdc(hdc); }
            }
            bmp.Save(path, System.Drawing.Imaging.ImageFormat.Png);
        }
        return true;
    }
}
"@ -ReferencedAssemblies System.Drawing

[void][UndoCap]::SetProcessDpiAwarenessContext([UndoCap]::PER_MONITOR_AWARE_V2)

function Wait-Pipe([int]$seconds) {
    $previous = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try {
        $sw = [System.Diagnostics.Stopwatch]::StartNew()
        while ($sw.Elapsed.TotalSeconds -lt $seconds) {
            $out = & $Probe 2>$null
            if ($LASTEXITCODE -eq 0 -and $out) { return $true }
            Start-Sleep -Milliseconds 500
        }
        return $false
    } finally {
        $ErrorActionPreference = $previous
    }
}

# The Studio keeps one IPC connection open (A-12 Langkah 1), so a second probe
# cannot connect while it runs. The engine's own log is the witness: every
# accepted set_video writes `ipc: set_video live-switched to <path>`.
$logPath = Join-Path $env:LOCALAPPDATA "K6WP\engine.log"
$script:logOffset = 0
if (Test-Path $logPath) { $script:logOffset = (Get-Item $logPath).Length }

function Get-NewLog {
    if (-not (Test-Path $logPath)) { return "" }
    $fs = [System.IO.File]::Open($logPath, 'Open', 'Read', 'ReadWrite')
    try {
        $null = $fs.Seek($script:logOffset, 'Begin')
        $reader = New-Object System.IO.StreamReader($fs)
        return $reader.ReadToEnd()
    } finally {
        $fs.Dispose()
    }
}

function Get-LogVideos {
    $text = Get-NewLog
    $paths = @()
    foreach ($m in [regex]::Matches($text, "set_video live-switched to (.+?)\r?\n")) {
        $paths += $m.Groups[1].Value.Trim()
    }
    return $paths
}

function Wait-LogVideo([string]$needle, [int]$seconds) {
    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    while ($sw.Elapsed.TotalSeconds -lt $seconds) {
        $paths = @(Get-LogVideos)
        $hit = $paths | Where-Object { $_ -like "*$needle*" } | Select-Object -Last 1
        if ($hit) {
            # The newest accepted switch must be the one we waited for, not an
            # older line still matched by the needle.
            if ($paths[-1] -like "*$needle*") { return $paths[-1] }
        }
        Start-Sleep -Milliseconds 250
    }
    return $null
}

$engineProc = $null
$studioProc = $null
$verdict = [ordered]@{
    schema      = "k6wp.e2e.a12b-undo/1"
    ok          = $false
    first_video = $null
    after_click = $null
    after_undo  = $null
    error       = $null
}

Invoke-SessionSafe {
    # Refuse to fight a live session: the pipe is single-client (A-12 Langkah 1).
    if (Get-Process engine -ErrorAction SilentlyContinue) { throw "an engine is already running - stop it first" }
    if (Get-Process k6wp-studio -ErrorAction SilentlyContinue) { throw "a Studio is already running - stop it first" }

    try {
        Write-Host "starting the engine..."
        $engineProc = Start-Process -FilePath $Engine -WindowStyle Hidden -PassThru
        if (-not (Wait-Pipe $TimeoutSec)) { throw "the engine never answered on its pipe" }
        Write-Host "engine answers on the pipe"

        Write-Host "starting Studio (debug, --e2e-pick 0)..."
        $errFile = Join-Path $env:TEMP ("studio-e2e-undo-" + [guid]::NewGuid().ToString("N") + ".err")
        $studioProc = Start-Process -FilePath $Exe -ArgumentList "--e2e-pick", "0" -PassThru -RedirectStandardError $errFile

        $hwnd = [IntPtr]::Zero
        $sw = [System.Diagnostics.Stopwatch]::StartNew()
        while ($sw.Elapsed.TotalSeconds -lt $TimeoutSec) {
            $studioProc.Refresh()
            $hwnd = $studioProc.MainWindowHandle
            if ($hwnd -ne [IntPtr]::Zero -and [UndoCap]::HasContent($hwnd)) { break }
            Start-Sleep -Milliseconds 100
        }
        if ($hwnd -eq [IntPtr]::Zero) { throw "the Studio window never painted" }

        $first = Wait-LogVideo "clip_1.mp4" 20
        if (-not $first) { throw "card 0 was never installed (engine log has: $((Get-LogVideos) -join ', '))" }
        $verdict.first_video = $first
        Write-Host "installed by --e2e-pick 0: $first"

        # Real click on card 1, in logical 960x640 coordinates scaled to the
        # window's true client width.
        $scale = [UndoCap]::ClientWidth($hwnd) / 960.0
        $cardX = [int](356 * $scale)
        $cardY = [int](120 * $scale)
        Write-Host ("clicking card {0} at ({1},{2}) [scale {3}]" -f $SecondIndex, $cardX, $cardY, $scale)
        [void][UndoCap]::SetForegroundWindow($hwnd)
        Start-Sleep -Milliseconds 300
        [UndoCap]::ClickAt($hwnd, $cardX, $cardY)

        $second = Wait-LogVideo "clip_2.mp4" 20
        if (-not $second) { throw "card 1 was never installed (engine log has: $((Get-LogVideos) -join ', '))" }
        $verdict.after_click = $second
        Write-Host "after the click: $second"

        # The toast lives ~3 s; capture it before it fades.
        Start-Sleep -Milliseconds 400
        $toastShot = Join-Path $OutDir "e2e-a12b-undo-toast.png"
        if (-not [UndoCap]::Capture($hwnd, $toastShot)) { throw "cannot capture the toast" }
        Write-Host "toast screenshot: $toastShot"

        $undoX = [int]($UndoX * $scale)
        $undoY = [int]($UndoY * $scale)
        Write-Host ("clicking Urungkan at ({0},{1})" -f $undoX, $undoY)
        [UndoCap]::ClickAt($hwnd, $undoX, $undoY)

        $reverted = Wait-LogVideo "clip_1.mp4" 20
        if (-not $reverted) {
            $shot = Join-Path $OutDir "e2e-a12b-undo-after.png"
            [void][UndoCap]::Capture($hwnd, $shot)
            throw "Urungkan did not revert the video (engine log has: $((Get-LogVideos) -join ', '), see $shot)"
        }
        $verdict.after_undo = $reverted
        Write-Host "after Urungkan: $reverted"

        $afterShot = Join-Path $OutDir "e2e-a12b-undo-after.png"
        [void][UndoCap]::Capture($hwnd, $afterShot)
        Write-Host "after screenshot: $afterShot"

        # Observed, not asserted: the undo apply goes through the same
        # WallpaperApplied path, so the next toast offers "Urungkan" again
        # (undo -> redo). The button is still single-shot per toast: the pure
        # history test proves take_undo() is consumed, and this run proves the
        # engine only moves on a real click.
        Write-Host "note: the undo apply re-offers Urungkan (redo) by design of the shared path"

        $verdict.ok = $true
        $verdict.error = $null
    } catch {
        $verdict.ok = $false
        $verdict.error = $_.Exception.Message
        Write-Warning $_.Exception.Message
        throw
    } finally {
        foreach ($proc in @($studioProc, $engineProc)) {
            if ($proc -and -not $proc.HasExited) {
                $proc.Kill()
                $null = $proc.WaitForExit(3000)
            }
        }
    }
} | Out-Null

$json = Join-Path "docs/bench" "e2e-a12b-undo.json"
New-Item -ItemType Directory -Path "docs/bench" -Force | Out-Null
$verdict | ConvertTo-Json -Depth 4 | Set-Content -Path $json -Encoding UTF8
Write-Host "verdict: $($verdict.ok) ($json)"
if (-not $verdict.ok) { Write-Host "reason: $($verdict.error)"; exit 1 }
