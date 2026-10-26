# measure_studio_library.ps1 - the Blok A-5 measurement for the Rust Studio's
# Library page: how long until the first painted frame, what it costs in RAM
# (private bytes + GPU shared), how much CPU it burns while idle, and whether
# the UI thread ever stalls while the grid is scrolled end to end.
#
# Usage:
#   powershell -File tools/measure_studio_library.ps1 -Exe <path> -Out docs/bench/studio-library-200.json
#
# Method (also written into the JSON, so a reader does not have to trust the
# prose):
#   * first content: poll PrintWindow(PW_RENDERFULLCONTENT) every 25 ms and
#     stop at the first frame with more than one colour. Resolution is 25 ms;
#     the number is an upper bound of "pixels were on screen".
#   * LIBREADY: the app prints "LIBREADY <ms>" on stderr with --ui-watch when
#     the library model is filled. That is our own code's budget, separated
#     from the compositor's.
#   * private bytes: Process.PrivateMemorySize64. GPU shared: the
#     "\GPU Process Memory(pid_N)\Shared Usage" performance counter.
#   * UI stalls: --ui-watch makes the app post to its own event loop every
#     8 ms and print "UIBLOCK <ms>" for any answer slower than 16 ms. The
#     count over a full scroll is the stall metric; the watchdog is compiled
#     into the binary but off unless the flag is passed.
#   * idle CPU: a separate run WITHOUT --ui-watch (the watchdog itself wakes
#     the loop), sampled over 5 s after 4 s of settling.
param(
    [Parameter(Mandatory = $true)][string]$ExePath,
    [Parameter(Mandatory = $true)][string]$OutPath,
    [int]$Runs = 5,
    [int]$IdleSeconds = 5,
    [int]$ScrollNotches = 400
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$Exe = (Resolve-Path $ExePath).Path
$Out = $OutPath
if (-not (Test-Path (Split-Path -Parent $Out))) { New-Item -ItemType Directory -Path (Split-Path -Parent $Out) -Force | Out-Null }

# Compile the C# helper once, then invoke its static methods.
$cs = @'
using System;
using System.Drawing;
using System.Runtime.InteropServices;

public static class BenchCap {
    [StructLayout(LayoutKind.Sequential)]
    public struct RECT { public int Left, Top, Right, Bottom; }

    [DllImport("user32.dll")] public static extern bool SetProcessDpiAwarenessContext(IntPtr value);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hWnd, out RECT rect);
    [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr hWnd, out RECT rect);
    [DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr hWnd, ref POINT point);
    [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr hWnd, IntPtr hdc, uint flags);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hWnd);
    [DllImport("user32.dll")] public static extern bool SetCursorPos(int x, int y);
    [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr hWnd, uint msg, IntPtr wParam, IntPtr lParam);

    [StructLayout(LayoutKind.Sequential)]
    public struct POINT { public int X, Y; }

    public const uint PW_RENDERFULLCONTENT = 2;
    public const uint WM_MOUSEWHEEL = 0x020A;
    public static readonly IntPtr PER_MONITOR_AWARE_V2 = new IntPtr(-4);

    /// The client rect in screen pixels.
    public static RECT ClientBounds(IntPtr hWnd) {
        RECT r;
        GetClientRect(hWnd, out r);
        POINT origin = new POINT { X = 0, Y = 0 };
        ClientToScreen(hWnd, ref origin);
        return new RECT { Left = origin.X, Top = origin.Y, Right = origin.X + r.Right, Bottom = origin.Y + r.Bottom };
    }

    /// True when the window paints more than one colour. Sampling every 24 px
    /// keeps this cheap enough to run inside a 25 ms poll.
    public static bool HasContent(IntPtr hWnd, out int width, out int height) {
        RECT r = ClientBounds(hWnd);
        width = r.Right - r.Left;
        height = r.Bottom - r.Top;
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

    public static void ScrollDown(IntPtr hWnd, int notches, int delayMs) {
        RECT r = ClientBounds(hWnd);
        int x = (r.Left + r.Right) / 2;
        int y = (r.Top + r.Bottom) / 2;
        SetCursorPos(x, y);
        System.Threading.Thread.Sleep(80);
        IntPtr point = new IntPtr((y << 16) | (x & 0xFFFF));
        for (int i = 0; i < notches; i++) {
            PostMessage(hWnd, WM_MOUSEWHEEL, new IntPtr((-120) << 16), point);
            if (delayMs > 0) System.Threading.Thread.Sleep(delayMs);
        }
    }
}
'@

$csPath = [System.IO.Path]::GetTempFileName()
$csPath = $csPath -replace '\.tmp$', '.cs'
Set-Content $csPath $cs -Encoding utf8
Add-Type -Path $csPath -ReferencedAssemblies "System.Drawing.dll" -ErrorAction Stop
Remove-Item $csPath -Force

[void][BenchCap]::SetProcessDpiAwarenessContext([BenchCap]::PER_MONITOR_AWARE_V2)

function Get-GpuMemoryMb([int]$ProcessId) {
    # The counter instances are named `pid_<n>_luid_..._phys_...`, one per
    # adapter/engine, so a bare `pid_<n>` path never matches. Enumerate the
    # wildcard set, keep this pid's instances, and SUM them: a process can hold
    # memory on more than one adapter.
    $result = @{ shared = $null; dedicated = $null; available = $false; instances = 0; reason = $null }
    $pattern = "^pid_$ProcessId(_|$)"
    try {
        $samples = (Get-Counter "\GPU Process Memory(*)\Shared Usage" -ErrorAction Stop).CounterSamples
        $mine = @($samples | Where-Object { $_.InstanceName -match $pattern -and $_.Status -eq 0 })
        if ($mine.Count -gt 0) {
            $result.shared = [math]::Round((($mine | Measure-Object -Property CookedValue -Sum).Sum) / 1MB, 1)
            $result.instances = $mine.Count
            $result.available = $true
        } else {
            $result.reason = "no GPU Process Memory instance for pid $ProcessId"
        }
    } catch {
        $result.reason = $_.Exception.Message
    }
    try {
        $samples = (Get-Counter "\GPU Process Memory(*)\Dedicated Usage" -ErrorAction Stop).CounterSamples
        $mine = @($samples | Where-Object { $_.InstanceName -match $pattern -and $_.Status -eq 0 })
        if ($mine.Count -gt 0) {
            $result.dedicated = [math]::Round((($mine | Measure-Object -Property CookedValue -Sum).Sum) / 1MB, 1)
            $result.available = $true
        }
    } catch { }
    return $result
}

function Stop-Studio {
    Get-Process -Name "k6wp-studio" -ErrorAction SilentlyContinue | Stop-Process -Force
    Start-Sleep -Milliseconds 250
}

function Start-Studio([string[]]$ExtraArgs) {
    $err = Join-Path $env:TEMP ("studio-bench-" + [guid]::NewGuid().ToString("N") + ".err")
    $arguments = @()
    if ($ExtraArgs -and $ExtraArgs.Count -gt 0) { $arguments += $ExtraArgs }
    if ($arguments.Count -gt 0) {
        $proc = Start-Process -FilePath $Exe -ArgumentList $arguments -PassThru -RedirectStandardError $err
    } else {
        $proc = Start-Process -FilePath $Exe -PassThru -RedirectStandardError $err
    }
    return [pscustomobject]@{ Process = $proc; ErrorFile = $err; Started = [datetime]::Now }
}

function Wait-Content($proc, [int]$TimeoutMs = 15000) {
    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    while ($sw.ElapsedMilliseconds -lt $TimeoutMs) {
        $proc.Refresh()
        $hwnd = $proc.MainWindowHandle
        if ($hwnd -ne [IntPtr]::Zero) {
            $w = 0; $h = 0
            if ([BenchCap]::HasContent($hwnd, [ref]$w, [ref]$h)) {
                return [pscustomobject]@{ Ms = $sw.ElapsedMilliseconds; Width = $w; Height = $h }
            }
        }
        Start-Sleep -Milliseconds 25
    }
    return $null
}

function Read-LibReady([string]$ErrorFile) {
    $text = Get-Content $ErrorFile -Raw -ErrorAction SilentlyContinue
    if ($text -match "LIBREADY ([0-9.]+)") { return [double]$Matches[1] }
    return $null
}

function Read-Stalls([string]$ErrorFile) {
    $text = Get-Content $ErrorFile -Raw -ErrorAction SilentlyContinue
    if (-not $text) { return @{ Count = 0; Max = 0.0 } }
    $values = @()
    foreach ($m in [regex]::Matches($text, "UIBLOCK ([0-9.]+)")) { $values += [double]$m.Groups[1].Value }
    $max = 0.0
    if ($values.Count -gt 0) { $max = ($values | Measure-Object -Maximum).Maximum }
    return @{ Count = $values.Count; Max = [math]::Round($max, 1) }
}

# The library the run will show, read from the JSON the Studio reads, so the
# numbers can be tied to an exact input.
$libraryPath = Join-Path $env:LOCALAPPDATA "K6WP\library.json"
$libraryItems = 0
$libraryMissing = 0
if (Test-Path $libraryPath) {
    $lib = Get-Content $libraryPath -Raw | ConvertFrom-Json
    $entries = @($lib.entries)
    $libraryItems = $entries.Count
    $libraryMissing = ($entries | Where-Object { $_.broken }).Count
}

$exeInfo = Get-Item $Exe
$exeHash = (Get-FileHash $Exe -Algorithm SHA256).Hash.ToLower()

Write-Host "measuring $Exe ($([math]::Round($exeInfo.Length / 1MB, 2)) MB) over $Runs runs"
Write-Host "library: $libraryPath ($libraryItems items, $libraryMissing unavailable)"

$runResults = @()
for ($i = 1; $i -le $Runs; $i++) {
    Stop-Studio
    $started = Start-Studio @("--ui-watch")
    $content = Wait-Content $started.Process
    if (-not $content) {
        Stop-Studio
        throw "run ${i}: the window never painted content"
    }
    $firstFrame = $content.Ms
    $libReady = Read-LibReady $started.ErrorFile
    Start-Sleep -Milliseconds 600
    $started.Process.Refresh()
    $gpuFirst = Get-GpuMemoryMb $started.Process.Id
    $first = @{
        private_mb = [math]::Round($started.Process.PrivateMemorySize64 / 1MB, 1)
        working_set_mb = [math]::Round($started.Process.WorkingSet64 / 1MB, 1)
        gpu_shared_mb = $gpuFirst.shared
        gpu_dedicated_mb = $gpuFirst.dedicated
        gpu_instances = $gpuFirst.instances
        gpu_reason = $gpuFirst.reason
        window = "$($content.Width)x$($content.Height)"
    }

    [BenchCap]::ScrollDown($started.Process.MainWindowHandle, $ScrollNotches, 8)
    Start-Sleep -Milliseconds 2000
    $started.Process.Refresh()
    $gpuAfter = Get-GpuMemoryMb $started.Process.Id
    $after = @{
        private_mb = [math]::Round($started.Process.PrivateMemorySize64 / 1MB, 1)
        working_set_mb = [math]::Round($started.Process.WorkingSet64 / 1MB, 1)
        gpu_shared_mb = $gpuAfter.shared
        gpu_dedicated_mb = $gpuAfter.dedicated
    }
    $stalls = Read-Stalls $started.ErrorFile
    # Let the watchdog see one more idle second: an idle Studio must not stall.
    Start-Sleep -Milliseconds 1000
    $idleStalls = Read-Stalls $started.ErrorFile
    Stop-Studio

    $runResults += [pscustomobject]@{
        run = $i
        first_frame_ms = $firstFrame
        lib_ready_ms = $libReady
        private_mb_first = $first.private_mb
        working_set_mb_first = $first.working_set_mb
        gpu_shared_mb_first = $first.gpu_shared_mb
        gpu_dedicated_mb_first = $first.gpu_dedicated_mb
        gpu_instances_first = $first.gpu_instances
        gpu_reason_first = $first.gpu_reason
        private_mb_after_scroll = $after.private_mb
        working_set_mb_after_scroll = $after.working_set_mb
        gpu_shared_mb_after_scroll = $after.gpu_shared_mb
        gpu_dedicated_mb_after_scroll = $after.gpu_dedicated_mb
        uiblock_count = $stalls.Count
        uiblock_max_ms = $stalls.Max
        uiblock_count_idle = $idleStalls.Count
        window = $first.window
    }
    Write-Host ("run {0}: first frame {1} ms, lib ready {2} ms, private {3} MB -> {4} MB, gpu shared {5} -> {6} MB, gpu dedicated {7} -> {8} MB, stalls {9}" -f `
            $i, $firstFrame, $libReady, $first.private_mb, $after.private_mb, $first.gpu_shared_mb, $after.gpu_shared_mb, $first.gpu_dedicated_mb, $after.gpu_dedicated_mb, $stalls.Count)
}

# Idle CPU: without the watchdog, because the watchdog is what wakes the loop.
Stop-Studio
$idle = Start-Studio @()
$null = Wait-Content $idle.Process
Start-Sleep -Seconds 4
$idle.Process.Refresh()
$cpuStart = $idle.Process.TotalProcessorTime
$wall = [System.Diagnostics.Stopwatch]::StartNew()
Start-Sleep -Seconds $IdleSeconds
$idle.Process.Refresh()
$cpuUsed = ($idle.Process.TotalProcessorTime - $cpuStart).TotalMilliseconds
$wallMs = $wall.ElapsedMilliseconds
$idleCpu = [math]::Round(100.0 * $cpuUsed / $wallMs, 2)
$idlePrivate = [math]::Round($idle.Process.PrivateMemorySize64 / 1MB, 1)
$idleGpu = Get-GpuMemoryMb $idle.Process.Id
$idleStalls = Read-Stalls $idle.ErrorFile
Stop-Studio

function Summarize($values) {
    $clean = @($values | Where-Object { $_ -ne $null })
    if ($clean.Count -eq 0) { return $null }
    $sorted = $clean | Sort-Object
    $median = if ($sorted.Count % 2 -eq 1) {
        $sorted[[int][math]::Floor($sorted.Count / 2)]
    } else {
        ($sorted[$sorted.Count / 2 - 1] + $sorted[$sorted.Count / 2]) / 2
    }
    return [pscustomobject]@{
        median = [math]::Round($median, 1)
        min = ($sorted | Measure-Object -Minimum).Minimum
        max = ($sorted | Measure-Object -Maximum).Maximum
        n = $clean.Count
    }
}

$summary = [ordered]@{
    first_frame_ms = Summarize $runResults.first_frame_ms
    lib_ready_ms = Summarize $runResults.lib_ready_ms
    private_mb_first = Summarize $runResults.private_mb_first
    private_mb_after_scroll = Summarize $runResults.private_mb_after_scroll
    gpu_shared_mb_first = Summarize $runResults.gpu_shared_mb_first
    gpu_shared_mb_after_scroll = Summarize $runResults.gpu_shared_mb_after_scroll
    gpu_dedicated_mb_first = Summarize $runResults.gpu_dedicated_mb_first
    gpu_dedicated_mb_after_scroll = Summarize $runResults.gpu_dedicated_mb_after_scroll
    uiblock_count_scroll = Summarize $runResults.uiblock_count
    uiblock_count_idle = Summarize $runResults.uiblock_count_idle
}

$os = Get-CimInstance Win32_OperatingSystem
$cpu = Get-CimInstance Win32_Processor | Select-Object -First 1
$dpi = (Get-CimInstance Win32_DesktopMonitor | Select-Object -First 1)
$doc = [ordered]@{
    schema = "k6wp.bench.studio-library/1"
    created = (Get-Date).ToString("yyyy-MM-ddTHH:mm:ssK")
    binary = [ordered]@{
        path = $Exe
        bytes = $exeInfo.Length
        sha256 = $exeHash
    }
    input = [ordered]@{
        library = $libraryPath
        items = $libraryItems
        unavailable = $libraryMissing
        grid_columns = 4
        card_width_px = 208
        row_height_px = 173
    }
    host = [ordered]@{
        os = $os.Caption
        build = $os.BuildNumber
        cpu = $cpu.Name.Trim()
        ram_gb = [math]::Round((Get-CimInstance Win32_ComputerSystem).TotalPhysicalMemory / 1GB, 1)
        session_dpi = 96
    }
    method = [ordered]@{
        first_frame = "poll PrintWindow(PW_RENDERFULLCONTENT) every 25 ms; first frame with >1 colour in the client area; resolution 25 ms"
        lib_ready = "app-side: stderr 'LIBREADY <ms>' under --ui-watch, printed when the library model is filled"
        private_bytes = "Process.PrivateMemorySize64 after the first frame and after an end-to-end scroll"
        gpu_shared = "Get-Counter '\GPU Process Memory(*)\Shared Usage' and '...\Dedicated Usage'; instances matching '^pid_<pid>_' are summed (one per adapter/engine); a missing instance is reported as gpu_reason, never silently 0"
        uiblock = "app-side watchdog: post to own event loop every 8 ms, print 'UIBLOCK <ms>' when an answer takes >16 ms; counted over the scroll and over 1 s of idle"
        idle_cpu = "run without --ui-watch: TotalProcessorTime delta over $IdleSeconds s, percent of one core"
        scroll = "$ScrollNotches wheel notches, 8 ms apart, pointer over the client area centre"
        runs = $Runs
    }
    runs = $runs
    summary = $summary
    idle = [ordered]@{
        cpu_pct_of_one_core = $idleCpu
        private_mb = $idlePrivate
        gpu_shared_mb = $idleGpu.shared
        gpu_dedicated_mb = $idleGpu.dedicated
        uiblock_count = $idleStalls.Count
        seconds = $IdleSeconds
    }
}

$json = $doc | ConvertTo-Json -Depth 6
$utf8 = New-Object System.Text.UTF8Encoding($false)
[System.IO.File]::WriteAllText($Out, $json, $utf8)
Write-Host "wrote $Out"
$summary.GetEnumerator() | ForEach-Object {
    $s = $_.Value
    if ($s) { Write-Host ("  {0}: median {1} (min {2}, max {3}, n {4})" -f $_.Key, $s.median, $s.min, $s.max, $s.n) }
}
Write-Host ("  idle cpu: {0}% of one core, private {1} MB, gpu shared {2}, dedicated {3}, uiblock {4}" -f $idleCpu, $idlePrivate, $idleGpu.shared, $idleGpu.dedicated, $idleStalls.Count)


