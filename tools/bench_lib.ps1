# tools/bench_lib.ps1 - shared helpers for the Phase 0 measurement harness.
#
# The harness is intentionally dependency-free (stock PowerShell 5.1 + .NET)
# and never ships. It measures the REAL engine/Studio processes; it does not
# instrument them. All numbers must be written to docs/bench/*.json.
#
# Evidence labels used by the reports:
#   live-proven  = observed on this machine by this harness
#   test-proven  = asserted by ctest / Rust tests
#   code-quoted  = read from source, not observed
#   unmeasured   = no number exists

$ErrorActionPreference = 'Stop'
$script:BenchRoot = Split-Path -Parent $MyInvocation.MyCommand.Path

if (-not ([System.Management.Automation.PSTypeName]'K6wpBench.Native').Type) {
Add-Type @"
using System;
using System.Runtime.InteropServices;
using System.Text;
namespace K6wpBench {
  public static class Native {
    [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr hWnd, int nCmdShow);
    [DllImport("user32.dll")] public static extern bool GetWindowPlacement(IntPtr hWnd, ref WINDOWPLACEMENT p);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetClassName(IntPtr hWnd, System.Text.StringBuilder buf, int n);
    [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr hWnd);
    [DllImport("user32.dll")] public static extern bool EnumWindows(EnumWindowsProc cb, IntPtr p);
    [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint pid);
    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }
    [StructLayout(LayoutKind.Sequential)] public struct POINT { public int X, Y; }
    [StructLayout(LayoutKind.Sequential)] public struct WINDOWPLACEMENT {
      public int length, flags, showCmd;
      public POINT minPosition, maxPosition;
      public RECT normalPosition;
    }
  }
}
"@
}

function Get-K6wpSessionId {
    return (Get-Process -Id $PID).SessionId
}

function Get-K6wpPipeName {
    return ("k6wp-engine-{0}" -f (Get-K6wpSessionId))
}

function Send-K6wpIpc {
    param(
        [Parameter(Mandatory=$true)][string]$Cmd,
        [hashtable]$Payload = @{},
        [int]$TimeoutMs = 3000
    )
    # Byte-level client: the server replies with one NDJSON line per datagram.
    # A StreamReader/ReadLineAsync helper masked the real failure behind a
    # "Cannot access a closed pipe" Dispose exception, so framing is done
    # explicitly here and every read is bounded by the deadline.
    $name = Get-K6wpPipeName
    $pipe = $null
    $ms = $null
    try {
        $pipe = New-Object System.IO.Pipes.NamedPipeClientStream('.', $name, [System.IO.Pipes.PipeDirection]::InOut, [System.IO.Pipes.PipeOptions]::Asynchronous)
        $pipe.Connect($TimeoutMs)
        $msg = @{ version = 1; cmd = $Cmd; payload = $Payload } | ConvertTo-Json -Compress -Depth 8
        $bytes = [System.Text.Encoding]::UTF8.GetBytes($msg + "`n")
        $pipe.Write($bytes, 0, $bytes.Length)
        $pipe.Flush()
        $deadline = [DateTime]::UtcNow.AddMilliseconds($TimeoutMs)
        $buf = New-Object byte[] 8192
        $ms = New-Object System.IO.MemoryStream
        while ($true) {
            $remain = [int]($deadline - [DateTime]::UtcNow).TotalMilliseconds
            if ($remain -le 0) { throw "IPC ack timeout after ${TimeoutMs}ms (cmd=$Cmd)" }
            # NamedPipeClientStream.ReadTimeout is unsupported on .NET Framework,
            # so bound the overlapped read with an explicit wait handle.
            $ar = $pipe.BeginRead($buf, 0, $buf.Length, $null, $null)
            if (-not $ar.AsyncWaitHandle.WaitOne($remain)) {
                throw "IPC ack timeout after ${TimeoutMs}ms (cmd=$Cmd)"
            }
            $n = $pipe.EndRead($ar)
            if ($n -le 0) { throw "IPC pipe closed before ack (cmd=$Cmd)" }
            $ms.Write($buf, 0, $n)
            if ($buf[$n - 1] -eq 10) { break }
        }
        $line = [System.Text.Encoding]::UTF8.GetString($ms.ToArray()).TrimEnd("`r", "`n")
        if ([string]::IsNullOrWhiteSpace($line)) { throw "empty IPC ack (cmd=$Cmd)" }
        return ($line | ConvertFrom-Json)
    }
    finally {
        if ($ms) { try { $ms.Dispose() } catch { } }
        if ($pipe) { try { $pipe.Dispose() } catch { } }
    }
}

function Wait-K6wpEngine {
    param([int]$TimeoutMs = 10000)
    $deadline = [DateTime]::UtcNow.AddMilliseconds($TimeoutMs)
    while ([DateTime]::UtcNow -lt $deadline) {
        try {
            $ack = Send-K6wpIpc -Cmd 'get_state' -TimeoutMs 800
            if ($ack -and $ack.ok) { return $true }
        } catch { }
        Start-Sleep -Milliseconds 100
    }
    return $false
}

function Start-K6wpEngine {
    param(
        [Parameter(Mandatory=$true)][string]$EngineExe,
        [string]$Video = '',
        [string]$DataDir = '',
        [int]$RunMs = 20000,
        [int]$ConfigMs = 0
    )
    if ([string]::IsNullOrWhiteSpace($DataDir)) {
        $DataDir = Join-Path $env:TEMP ("k6wp-bench-" + [Guid]::NewGuid().ToString('N'))
    }
    $dataRoot = Join-Path $DataDir 'K6WP'
    New-Item -ItemType Directory -Force -Path $dataRoot | Out-Null
    $args = @("--exit-after-ms=$RunMs")
    if (-not [string]::IsNullOrWhiteSpace($Video)) {
        $args = @("--video=$Video") + $args
    }
    $oldLocal = $env:LOCALAPPDATA
    try {
        $env:LOCALAPPDATA = $DataDir
        $proc = Start-Process -FilePath $EngineExe -ArgumentList $args -PassThru -WindowStyle Hidden
    }
    finally {
        $env:LOCALAPPDATA = $oldLocal
    }
    if (-not (Wait-K6wpEngine -TimeoutMs 15000)) {
        try { if (-not $proc.HasExited) { Stop-Process -Id $proc.Id -Force } } catch { }
        throw "engine did not answer the IPC pipe within 15s (video=$Video)"
    }
    return [pscustomobject]@{ Process = $proc; DataDir = $DataDir; LogPath = (Join-Path $dataRoot 'engine.log') }
}

function Stop-K6wpEngine {
    param($Run, [int]$GraceMs = 12000)
    if ($null -eq $Run -or $null -eq $Run.Process) { return }
    $p = $Run.Process
    try { $p.Refresh() } catch { return }
    if ($p.HasExited) { return }
    try { [void](Send-K6wpIpc -Cmd 'quit' -TimeoutMs 2000) } catch { }
    try { [void]$p.WaitForExit($GraceMs) } catch { }
    try {
        $p.Refresh()
        if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue }
    } catch { }
}

function Get-Median {
    param([double[]]$Values)
    if ($null -eq $Values -or $Values.Count -eq 0) { return $null }
    $sorted = @($Values | Sort-Object)
    $n = $sorted.Count
    if ($n % 2 -eq 1) { return [double]$sorted[($n - 1) / 2] }
    return ([double]$sorted[$n / 2 - 1] + [double]$sorted[$n / 2]) / 2.0
}

function Get-Stats {
    param([double[]]$Values)
    $clean = @($Values | Where-Object { $null -ne $_ -and -not [double]::IsNaN($_) })
    if ($clean.Count -eq 0) {
        return [pscustomobject]@{ n = 0; median = $null; mean = $null; min = $null; max = $null }
    }
    $m = Get-Median $clean
    return [pscustomobject]@{
        n = $clean.Count
        median = [math]::Round($m, 2)
        mean = [math]::Round((($clean | Measure-Object -Average).Average), 2)
        min = [math]::Round((($clean | Measure-Object -Minimum).Minimum), 2)
        max = [math]::Round((($clean | Measure-Object -Maximum).Maximum), 2)
    }
}

function Get-ProcessSeries {
    param(
        [Parameter(Mandatory=$true)][int]$ProcessId,
        [int]$Seconds = 10,
        [int]$IntervalMs = 1000
    )
    $cores = [Environment]::ProcessorCount
    $out = New-Object System.Collections.Generic.List[object]
    $p = Get-Process -Id $ProcessId -ErrorAction Stop
    $prevCpu = $p.CPU
    if ($null -eq $prevCpu) { $prevCpu = 0.0 }
    $prevT = [DateTime]::UtcNow
    $start = [DateTime]::UtcNow
    # Per-thread CPU: the PROCESS aggregate hides a single spinning thread
    # (that is exactly how a 55 %-of-a-core busy-wait stayed invisible), so
    # the hottest thread is tracked separately (C2).
    $threadPrev = @{}
    foreach ($t in $p.Threads) { $threadPrev[[string]$t.Id] = $t.TotalProcessorTime.TotalMilliseconds }
    while (([DateTime]::UtcNow - $start).TotalSeconds -lt $Seconds) {
        Start-Sleep -Milliseconds $IntervalMs
        try {
            $p.Refresh()
            if ($p.HasExited) { break }
            $cpu = $p.CPU
            if ($null -eq $cpu) { $cpu = $prevCpu }
            $now = [DateTime]::UtcNow
            $dt = ($now - $prevT).TotalSeconds
            $pct = 0.0
            if ($dt -gt 0) {
                $pct = (($cpu - $prevCpu) / $dt / $cores) * 100.0
                if ($pct -lt 0) { $pct = 0.0 }
            }
            # The hottest single thread over this interval, as a percentage
            # of ONE core (not of the machine).
            $hotPct = 0.0
            $hotTid = 0
            foreach ($t in $p.Threads) {
                $k = [string]$t.Id
                $nowMs = $t.TotalProcessorTime.TotalMilliseconds
                if ($threadPrev.ContainsKey($k)) {
                    $d = $nowMs - $threadPrev[$k]
                    if ($dt -gt 0) {
                        $pctCore = ($d / 1000.0 / $dt) * 100.0
                        if ($pctCore -gt $hotPct) { $hotPct = $pctCore; $hotTid = $t.Id }
                    }
                }
                $threadPrev[$k] = $nowMs
            }
            $out.Add([pscustomobject]@{
                t_ms = [int](([DateTime]::UtcNow - $start).TotalMilliseconds)
                private_mb = [math]::Round($p.PrivateMemorySize64 / 1MB, 2)
                ws_mb = [math]::Round($p.WorkingSet64 / 1MB, 2)
                cpu_pct = [math]::Round($pct, 3)
                max_thread_cpu_pct = [math]::Round($hotPct, 2)
                hot_tid = $hotTid
                threads = $p.Threads.Count
                handles = $p.HandleCount
            })
            $prevCpu = $cpu
            $prevT = $now
        } catch { break }
    }
    return $out.ToArray()
}

function Get-TypeperfCounterPaths {
    param([int]$ProcessId, [string]$ProcessName = 'engine')
    $paths = New-Object System.Collections.Generic.List[string]
    try {
        $gpu = (Get-Counter -ListSet 'GPU Engine' -ErrorAction Stop).PathsWithInstances
        foreach ($p in $gpu) {
            if ($p -match ("pid_" + $ProcessId + "_") -and $p -match 'Utilization Percentage$') {
                $paths.Add($p)
            }
        }
    } catch { }
    try {
        $mem = (Get-Counter -ListSet 'GPU Process Memory' -ErrorAction Stop).PathsWithInstances
        foreach ($p in $mem) {
            if ($p -match ("pid_" + $ProcessId + "_") -and $p -match '(Dedicated|Shared) Usage$') {
                $paths.Add($p)
            }
        }
    } catch { }
    if (-not [string]::IsNullOrWhiteSpace($ProcessName)) {
        $paths.Add(("\Thread({0}*)\Context Switches/sec" -f $ProcessName))
    }
    return $paths.ToArray()
}

function Start-TypeperfSampler {
    param(
        [int]$ProcessId,
        [int]$Seconds = 10,
        [string]$ProcessName = 'engine'
    )
    $paths = Get-TypeperfCounterPaths -ProcessId $ProcessId -ProcessName $ProcessName
    if ($paths.Count -eq 0) { throw "no GPU/thread counters for pid $ProcessId" }
    $csv = Join-Path $env:TEMP ("k6wp-typeperf-" + [Guid]::NewGuid().ToString('N') + ".csv")
    $stdout = Join-Path $env:TEMP ("k6wp-typeperf-" + [Guid]::NewGuid().ToString('N') + ".out")
    $stderr = Join-Path $env:TEMP ("k6wp-typeperf-" + [Guid]::NewGuid().ToString('N') + ".err")
    # Start-Process joins -ArgumentList with spaces and does NOT add quotes.
    # Counter paths contain spaces ("GPU Engine"), so quote every path.
    $quoted = @($paths | ForEach-Object { '"' + $_ + '"' })
    $args = @($quoted) + @('-si', '1', '-sc', "$Seconds", '-f', 'CSV', '-o', ('"' + $csv + '"'), '-y')
    $proc = Start-Process -FilePath 'typeperf.exe' -ArgumentList $args -PassThru -WindowStyle Hidden `
        -RedirectStandardOutput $stdout -RedirectStandardError $stderr
    return [pscustomobject]@{
        Process = $proc
        Csv = $csv
        Stdout = $stdout
        Stderr = $stderr
        Paths = $paths
    }
}

function ConvertTo-NullableDouble {
    param($Value)
    if ($null -eq $Value) { return $null }
    $s = [string]$Value
    if ([string]::IsNullOrWhiteSpace($s)) { return $null }
    $d = 0.0
    $ok = [double]::TryParse($s, [Globalization.NumberStyles]::Float, [Globalization.CultureInfo]::InvariantCulture, [ref]$d)
    if (-not $ok) { return $null }
    if ($d -lt 0) { return $null }
    return $d
}

function Read-TypeperfCsv {
    param([string]$Path)
    $rows = @()
    if (-not (Test-Path -LiteralPath $Path)) { return $rows }
    try { $data = Import-Csv -LiteralPath $Path } catch { return $rows }
    foreach ($row in $data) {
        $props = @($row.PSObject.Properties)
        if ($props.Count -eq 0) { continue }
        $stamp = [string]$props[0].Value
        if ([string]::IsNullOrWhiteSpace($stamp)) { continue }
        $gpuTotal = 0.0; $gpu3d = 0.0; $gpuDecode = 0.0; $gpuCopy = 0.0
        $shared = 0.0; $dedicated = 0.0; $wake = 0.0
        $hasGpu = $false; $hasWake = $false
        foreach ($prop in $props) {
            $name = $prop.Name
            $val = ConvertTo-NullableDouble $prop.Value
            if ($null -eq $val) { continue }
            if ($name -match 'Utilization Percentage$') {
                $hasGpu = $true
                $gpuTotal += $val
                # Column names are full PDH paths:
                # \\host\GPU Engine(pid_.._engtype_3D)\Utilization Percentage
                if ($name -match 'engtype_3D\)') { $gpu3d += $val }
                elseif ($name -match 'engtype_VideoDecode\)') { $gpuDecode += $val }
                elseif ($name -match 'engtype_Copy\)') { $gpuCopy += $val }
            }
            elseif ($name -match 'Shared Usage$') { $shared += $val }
            elseif ($name -match 'Dedicated Usage$') { $dedicated += $val }
            elseif ($name -match 'Context Switches/sec$') {
                $hasWake = $true
                $wake += $val
            }
        }
        $rows += [pscustomobject]@{
            timestamp = $stamp
            gpu_total_pct = $(if ($hasGpu) { [math]::Round($gpuTotal, 2) } else { $null })
            gpu_3d_pct = $(if ($hasGpu) { [math]::Round($gpu3d, 2) } else { $null })
            gpu_decode_pct = $(if ($hasGpu) { [math]::Round($gpuDecode, 2) } else { $null })
            gpu_copy_pct = $(if ($hasGpu) { [math]::Round($gpuCopy, 2) } else { $null })
            gpu_shared_mb = [math]::Round($shared / 1MB, 2)
            gpu_dedicated_mb = [math]::Round($dedicated / 1MB, 2)
            wakeups_per_sec = $(if ($hasWake) { [math]::Round($wake, 2) } else { $null })
        }
    }
    return $rows
}

function Get-DesktopWindows {
    # Save + hide every visible top-level app window so the injected
    # wallpaper is not occluded. Shell surfaces (taskbar/Progman/WorkerW) are
    # never touched.
    #
    # The windows are enumerated with EnumWindows, NOT through
    # Get-Process.MainWindowHandle: a process can own several top-level
    # windows and the "main" one is not necessarily the one on screen (a
    # File Explorer window owned by a helper process, an Electron app with
    # one window per process, ...). Missing a covering window makes the
    # engine pause (occlusion/fullscreen) and the capture records the plain
    # desktop instead of the wallpaper.
    $saved = @()
    $skip = @('Shell_TrayWnd', 'Progman', 'WorkerW', 'K6WP.DesktopInject.1', 'Windows.UI.Core.CoreWindow')
    # An always-on-top agent-cursor overlay covers the WHOLE screen. It is
    # not an app window: minimizing it does nothing, but leaving it up makes
    # the engine's fullscreen watcher pause the decode AND the occlusion
    # watcher report 100 % coverage, so every measurement reads a paused
    # engine and the capture records the plain desktop. It is hidden for the
    # measurement and restored afterwards.
    $hideClasses = @('Cua.AgentCursorOverlay')
    $found = New-Object System.Collections.Generic.List[object]
    $cb = [K6wpBench.Native+EnumWindowsProc] {
        param($h, $p)
        if (-not [K6wpBench.Native]::IsWindowVisible($h)) { return $true }
        $sb = New-Object System.Text.StringBuilder 256
        [void][K6wpBench.Native]::GetClassName($h, $sb, 256)
        $cls = $sb.ToString()
        $procId = 0
        [void][K6wpBench.Native]::GetWindowThreadProcessId($h, [ref]$procId)
        $found.Add([pscustomobject]@{ Handle = $h; Class = $cls; Pid = $procId })
        return $true
    }
    [void][K6wpBench.Native]::EnumWindows($cb, [IntPtr]::Zero)
    foreach ($w in $found) {
        try {
            $h = $w.Handle
            $cls = $w.Class
            if ($skip -contains $cls) { continue }
            $wp = New-Object K6wpBench.Native+WINDOWPLACEMENT
            $wp.length = [Runtime.InteropServices.Marshal]::SizeOf($wp)
            if (-not [K6wpBench.Native]::GetWindowPlacement($h, [ref]$wp)) { continue }
            if ($wp.showCmd -eq 6) { continue }  # already minimized
            $name = ''
            try { $name = (Get-Process -Id $w.Pid -ErrorAction Stop).ProcessName } catch { $name = "pid$($w.Pid)" }
            $saved += [pscustomobject]@{ Handle = $h; ShowCmd = $wp.showCmd; Name = $name; Class = $cls }
            if ($hideClasses -contains $cls) {
                [void][K6wpBench.Native]::ShowWindow($h, 0)   # SW_HIDE
            } else {
                [void][K6wpBench.Native]::ShowWindow($h, 6)   # SW_MINIMIZE
            }
        } catch { }
    }
    return $saved
}

function Restore-DesktopWindows {
    param([object[]]$Saved)
    if ($null -eq $Saved) { return }
    foreach ($s in $Saved) {
        try {
            $cmd = [int]$s.ShowCmd
            if ($cmd -eq 0) { $cmd = 1 }
            [void][K6wpBench.Native]::ShowWindow([IntPtr]$s.Handle, $cmd)
        } catch { }
    }
}

function Get-HwInventory {
    $os = Get-CimInstance Win32_OperatingSystem
    $cpu = Get-CimInstance Win32_Processor | Select-Object -First 1
    $gpus = @(Get-CimInstance Win32_VideoController | ForEach-Object { $_.Name })
    $power = ''
    try { $power = (powercfg /getactivescheme) -join ' ' } catch { }
    $monitors = @()
    $dump = Join-Path $script:BenchRoot '..\build\msvc-dev\monitor_dump.exe'
    if (Test-Path -LiteralPath $dump) {
        try {
            $raw = & $dump 2>$null | Out-String
            $parsed = $raw | ConvertFrom-Json
            $monitors = @($parsed.monitors)
        } catch { }
    }
    return [pscustomobject]@{
        os = ("{0} {1} build {2}" -f $os.Caption, $os.Version, $os.BuildNumber)
        cpu = $cpu.Name
        cores = $cpu.NumberOfCores
        logical = $cpu.NumberOfLogicalProcessors
        ram_kib = $os.TotalVisibleMemorySize
        gpus = $gpus
        power_plan = $power.Trim()
        monitors = $monitors
        date = (Get-Date -Format 'yyyy-MM-ddTHH:mm:ssK')
    }
}

function Get-ScreenBitmap {
    Add-Type -AssemblyName System.Drawing, System.Windows.Forms
    $bounds = [System.Windows.Forms.SystemInformation]::VirtualScreen
    $bmp = New-Object System.Drawing.Bitmap $bounds.Width, $bounds.Height
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    try {
        $g.CopyFromScreen($bounds.Left, $bounds.Top, 0, 0, $bmp.Size)
    }
    finally { $g.Dispose() }
    return [pscustomobject]@{ Bitmap = $bmp; Bounds = $bounds }
}

function Save-ScreenPng {
    param([string]$Path, [int]$CropWidth = 0, [int]$CropHeight = 0)
    $shot = Get-ScreenBitmap
    $bmp = $shot.Bitmap
    try {
        if ($CropWidth -gt 0 -and $CropHeight -gt 0) {
            $x = [int](($bmp.Width - $CropWidth) / 2)
            $y = [int](($bmp.Height - $CropHeight) / 2)
            $crop = New-Object System.Drawing.Bitmap $CropWidth, $CropHeight
            $g = [System.Drawing.Graphics]::FromImage($crop)
            try { $g.DrawImage($bmp, (New-Object System.Drawing.Rectangle 0,0,$CropWidth,$CropHeight), (New-Object System.Drawing.Rectangle $x,$y,$CropWidth,$CropHeight), [System.Drawing.GraphicsUnit]::Pixel) }
            finally { $g.Dispose() }
            $bmp.Dispose()
            $bmp = $crop
        }
        $dir = [System.IO.Path]::GetDirectoryName([System.IO.Path]::GetFullPath($Path))
        if (-not (Test-Path -LiteralPath $dir)) { New-Item -ItemType Directory -Force -Path $dir | Out-Null }
        $bmp.Save([System.IO.Path]::GetFullPath($Path), [System.Drawing.Imaging.ImageFormat]::Png)
    }
    finally { $bmp.Dispose() }
}

function Get-RegionSample {
    param([int]$Width = 240, [int]$Height = 160)
    Add-Type -AssemblyName System.Drawing, System.Windows.Forms
    $bounds = [System.Windows.Forms.SystemInformation]::VirtualScreen
    $x = $bounds.Left + [int](($bounds.Width - $Width) / 2)
    $y = $bounds.Top + [int](($bounds.Height - $Height) / 2)
    $bmp = New-Object System.Drawing.Bitmap $Width, $Height
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    try { $g.CopyFromScreen($x, $y, 0, 0, $bmp.Size) } finally { $g.Dispose() }
    return $bmp
}

function Get-BitmapDiff {
    param($A, $B, [int]$Step = 4)
    $w = [Math]::Min($A.Width, $B.Width)
    $h = [Math]::Min($A.Height, $B.Height)
    $sum = 0.0
    $n = 0
    for ($y = 0; $y -lt $h; $y += $Step) {
        for ($x = 0; $x -lt $w; $x += $Step) {
            $pa = $A.GetPixel($x, $y)
            $pb = $B.GetPixel($x, $y)
            $sum += ([Math]::Abs($pa.R - $pb.R) + [Math]::Abs($pa.G - $pb.G) + [Math]::Abs($pa.B - $pb.B)) / 3.0
            $n++
        }
    }
    if ($n -eq 0) { return 0.0 }
    return $sum / $n
}

function Get-PeakWorkingSetMb {
    param([int]$ProcessId)
    try {
        $p = Get-Process -Id $ProcessId -ErrorAction Stop
        return [math]::Round($p.PeakWorkingSet64 / 1MB, 2)
    } catch { return $null }
}
