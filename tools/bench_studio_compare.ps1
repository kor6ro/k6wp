# tools/bench_studio_compare.ps1 - E1: the Studio comparison, measured in
# IDENTICAL conditions.
#
# The rules this script enforces (they are the point of E1):
#
#   * ONE library index file (200 items) for both Studios - the same
#     K6WP_LIBRARY_JSON override, so the state is the same, not "similar";
#   * the same screen, the same isolated %LOCALAPPDATA%, the same env;
#   * ONE startup definition: the process start until the window has
#     PAINTED (the capture is not a flat colour) - "the first frame", not
#     "the window handle exists";
#   * the C4 metrics: private bytes + GPU shared memory. The working set is
#     recorded but never decides;
#   * the CPU while the window is static (the user is not touching it).
#
# Output: docs/bench/studio-compare-<slice>.json
[CmdletBinding()]
param(
    [string]$QtStudio = '',
    [string]$SlintStudio = '',
    [int]$Items = 200,
    [string]$Slice = '2.0',
    [string]$OutJson = '',
    [int]$StaticSeconds = 5
)
$ErrorActionPreference = 'Stop'
$script:Here = Split-Path -Parent $MyInvocation.MyCommand.Path
. (Join-Path $script:Here 'bench_lib.ps1')
if ([string]::IsNullOrWhiteSpace($QtStudio)) { $QtStudio = Join-Path $script:Here '..\build\release\studio.exe' }
if ([string]::IsNullOrWhiteSpace($SlintStudio)) { $SlintStudio = Join-Path $script:Here '..\rust\target\x86_64-pc-windows-msvc\debug\k6wp-studio.exe' }
if ([string]::IsNullOrWhiteSpace($OutJson)) { $OutJson = Join-Path $script:Here "..\docs\bench\studio-compare-$Slice.json" }
foreach ($e in @($QtStudio, $SlintStudio)) { if (-not (Test-Path -LiteralPath $e)) { throw "missing: $e" } }

Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;using System.Runtime.InteropServices;using System.Text;
public class StudioCap {
  [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr hdc, uint flags);
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
  [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }
}
"@

# --- the shared library index (200 items) ---------------------------------
$work = Join-Path $env:TEMP ("k6wp-studio-cmp-" + [Guid]::NewGuid().ToString('N').Substring(0, 8))
New-Item -ItemType Directory -Force -Path (Join-Path $work 'K6WP') | Out-Null
$library = Join-Path $work 'library.json'
$entries = @()
for ($i = 0; $i -lt $Items; $i++) {
    $entries += [ordered]@{
        src = "C:/videos/source-$i.mp4"
        dst = "C:/videos/copy-$i.mp4"
        res = "1920x1080"; fps = 30; crf = 23; encoder = "libx264"
        mtime = 1700000000; size = 1048576 + $i; duration = 30.0
        codec = "h264"; width = 1920; height = 1080
        thumb = "C:/videos/copy-$i.jpg"; broken = $false
    }
}
@{ version = 1; entries = $entries } | ConvertTo-Json -Depth 4 -Compress | Set-Content -LiteralPath $library -Encoding UTF8
"the shared library index: $library ($Items items, $((Get-Item $library).Length) bytes)"

function Get-CaptureLuma([IntPtr]$h, [string]$Path) {
    $r = New-Object StudioCap+RECT
    [void][StudioCap]::GetWindowRect($h, [ref]$r)
    $w = $r.Right - $r.Left; $ht = $r.Bottom - $r.Top
    if ($w -le 0 -or $ht -le 0) { return $null }
    $bmp = New-Object System.Drawing.Bitmap $w, $ht
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $hdc = $g.GetHdc()
    try { [void][StudioCap]::PrintWindow($h, $hdc, 2) } finally { $g.ReleaseHdc($hdc); $g.Dispose() }
    $bmp.Save($Path, [System.Drawing.Imaging.ImageFormat]::Png)
    # The mean + the spread: a window that has NOT painted is a flat colour
    # (spread ~0), a painted one has content (spread > 0).
    $sum = 0.0; $sum2 = 0.0; $n = 0
    for ($y = 0; $y -lt $bmp.Height; $y += 4) {
        for ($x = 0; $x -lt $bmp.Width; $x += 4) {
            $c = $bmp.GetPixel($x, $y)
            $l = 0.2126 * $c.R + 0.7152 * $c.G + 0.0722 * $c.B
            $sum += $l; $sum2 += $l * $l; $n++
        }
    }
    $bmp.Dispose()
    $mean = if ($n -gt 0) { $sum / $n } else { 0.0 }
    $spread = if ($n -gt 0) { [math]::Sqrt([math]::Max(0, ($sum2 / $n) - ($mean * $mean))) } else { 0.0 }
    return [pscustomobject]@{ mean = [math]::Round($mean, 2); spread = [math]::Round($spread, 2) }
}

function Measure-Studio([string]$Exe, [string]$Tag) {
    $old = $env:LOCALAPPDATA
    $oldLib = $env:K6WP_LIBRARY_JSON
    try {
        $env:LOCALAPPDATA = (Join-Path $work 'K6WP')
        $env:K6WP_LIBRARY_JSON = $library
        $sw = [Diagnostics.Stopwatch]::StartNew()
        $p = Start-Process -FilePath $Exe -PassThru
        # The first frame: the window handle exists AND the window has
        # painted (the capture is not a flat colour).
        $hwnd = [IntPtr]::Zero
        $firstFrameMs = $null
        $tmp = Join-Path $env:TEMP ("k6wp-studio-cmp-" + [Guid]::NewGuid().ToString('N').Substring(0, 6) + ".png")
        while ($sw.ElapsedMilliseconds -lt 30000) {
            $p.Refresh()
            if ($p.HasExited) { break }
            if ($p.MainWindowHandle -ne 0) {
                $hwnd = $p.MainWindowHandle
                $cap = Get-CaptureLuma $hwnd $tmp
                if ($cap -and $cap.spread -gt 0.5) { $firstFrameMs = $sw.ElapsedMilliseconds; break }
            }
            Start-Sleep -Milliseconds 25
        }
        Remove-Item $tmp -Force -ErrorAction SilentlyContinue
        # The static state: the private bytes, the GPU shared and the CPU
        # after the first frame (the user is not touching the window).
        $tp = Start-TypeperfSampler -ProcessId $p.Id -Seconds ($StaticSeconds + 10) -ProcessName 'studio'
        $series = @(Get-ProcessSeries -ProcessId $p.Id -Seconds $StaticSeconds -IntervalMs 1000)
        $gpu = @(Read-TypeperfCsv -Path $tp.Csv)
        try { if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue } } catch { }
        $gpuM = @($gpu | ForEach-Object { $_.gpu_shared_mb } | Where-Object { $null -ne $_ })
        $cpu = @($series | ForEach-Object { $_.cpu_pct } | Where-Object { $null -ne $_ })
        return [ordered]@{
            tag = $Tag
            exe = (Split-Path -Leaf $Exe)
            first_frame_ms = $firstFrameMs
            private_mb = (Get-Stats @($series | ForEach-Object { $_.private_mb })).median
            ws_mb = (Get-Stats @($series | ForEach-Object { $_.ws_mb })).median
            gpu_shared_mb = $(if ($gpuM.Count -gt 0) { (Get-Stats $gpuM).median } else { $null })
            cpu_pct_static = (Get-Stats $cpu).median
            max_thread_cpu_pct = (Get-Stats @($series | ForEach-Object { $_.max_thread_cpu_pct })).median
            threads = ($series | Select-Object -Last 1).threads
        }
    }
    finally {
        $env:LOCALAPPDATA = $old
        if ($null -eq $oldLib) { Remove-Item Env:\K6WP_LIBRARY_JSON -ErrorAction SilentlyContinue }
        else { $env:K6WP_LIBRARY_JSON = $oldLib }
    }
}

$qt = Measure-Studio -Exe $QtStudio -Tag 'qt'
$slint = Measure-Studio -Exe $SlintStudio -Tag 'slint'
# The same order again (the second arm) - the first run of a process can
# pay a cold-start cost (the font cache, the shader cache).
$qt2 = Measure-Studio -Exe $QtStudio -Tag 'qt'
$slint2 = Measure-Studio -Exe $SlintStudio -Tag 'slint'

$report = [ordered]@{
    tool = 'bench_studio_compare'
    phase = 2
    slice = $Slice
    date = (Get-Date -Format 'yyyy-MM-ddTHH:mm:ssK')
    label = 'live-proven (one machine)'
    method = 'ONE library index (200 items) for both Studios, the same screen, an isolated %LOCALAPPDATA%, and ONE startup definition: the process start until the window has PAINTED (the capture is not a flat colour). The C4 metrics: private bytes + GPU shared; the working set is recorded but never decides'
    library_items = $Items
    library_path = $library
    startup_definition = 'process start -> the window handle exists AND a PrintWindow capture is not a flat colour (the first painted frame)'
    qt = [ordered]@{
        first_frame_ms = @($qt.first_frame_ms, $qt2.first_frame_ms) | Where-Object { $null -ne $_ } | Measure-Object -Average | Select-Object -ExpandProperty Average
        private_mb = (Get-Stats @($qt.private_mb, $qt2.private_mb)).median
        ws_mb = (Get-Stats @($qt.ws_mb, $qt2.ws_mb)).median
        gpu_shared_mb = (Get-Stats @($qt.gpu_shared_mb, $qt2.gpu_shared_mb) | Where-Object { $null -ne $_ }).median
        cpu_pct_static = (Get-Stats @($qt.cpu_pct_static, $qt2.cpu_pct_static)).median
        threads = $qt.threads
    }
    slint = [ordered]@{
        first_frame_ms = @($slint.first_frame_ms, $slint2.first_frame_ms) | Where-Object { $null -ne $_ } | Measure-Object -Average | Select-Object -ExpandProperty Average
        private_mb = (Get-Stats @($slint.private_mb, $slint2.private_mb)).median
        ws_mb = (Get-Stats @($slint.ws_mb, $slint2.ws_mb)).median
        gpu_shared_mb = (Get-Stats @($slint.gpu_shared_mb, $slint2.gpu_shared_mb) | Where-Object { $null -ne $_ }).median
        cpu_pct_static = (Get-Stats @($slint.cpu_pct_static, $slint2.cpu_pct_static)).median
        threads = $slint.threads
    }
    runs = @($qt, $slint, $qt2, $slint2)
}
$json = $report | ConvertTo-Json -Depth 8
$outFull = [System.IO.Path]::GetFullPath($OutJson)
$outDir = [System.IO.Path]::GetDirectoryName($outFull)
if (-not (Test-Path -LiteralPath $outDir)) { New-Item -ItemType Directory -Force -Path $outDir | Out-Null }
$json | Set-Content -LiteralPath $outFull -Encoding UTF8
Write-Host ""
Write-Host ("Qt Studio    : first frame {0} ms, private {1} MB, GPU shared {2} MB, CPU {3} %, threads {4}" -f `
        [math]::Round($report.qt.first_frame_ms), $report.qt.private_mb, $report.qt.gpu_shared_mb, $report.qt.cpu_pct_static, $report.qt.threads)
Write-Host ("Slint Studio : first frame {0} ms, private {1} MB, GPU shared {2} MB, CPU {3} %, threads {4}" -f `
        [math]::Round($report.slint.first_frame_ms), $report.slint.private_mb, $report.slint.gpu_shared_mb, $report.slint.cpu_pct_static, $report.slint.threads)
Write-Host "bench_studio_compare: wrote $outFull"
