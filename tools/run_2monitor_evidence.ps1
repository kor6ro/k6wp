# tools/run_2monitor_evidence.ps1 — one-command 2-monitor evidence bundle (row 30).
#
# Runs on the third-party rig headless. ONE command collects:
#   1. monitor_dump.exe JSON (extended shape: monitors[] + virtual_screen)
#   2. An injected-window census via EnumWindows + EnumChildWindows matching
#      class K6WP.DesktopInject.1, saving each child's GetWindowRect
#   3. The `placement:` lines from engine.log
#   4. The rig's Windows build (OSVersion)
# Everything lands in ONE timestamped directory under .omo/evidence/.
#
# Contract (row 30 acceptance):
#   - Exits 0 ALWAYS — a missing engine.log, a missing monitor_dump.exe, or a
#     dead engine degrades to a written warning + still exit 0, so the operator
#     still gets the rest of the bundle.
#   - Census technique (notepads/k6wp-final-wallpaper-app/learnings.md:94-98):
#     Find Progman/WorkerW by EnumWindows + class match (FindWindowW returns
#     0x0 from QA scripts — walk blindness), then EnumChildWindows by class
#     K6WP.DesktopInject.1 to count injected windows (== monitor count).
#   - GetClassNameW trap (learnings.md:171-175): MUST declare
#     CharSet = CharSet.Unicode or every class name reads as its first letter
#     and class matching silently fails.
#   - Does NOT automate screenshots — a human takes those (see the run-book).
#   - Does NOT start the engine — the operator starts it first (run-book step).
#   - No network, no telemetry, no third-party dependency (stock PS + .NET).
#
# Usage (from the repo root, or anywhere — paths resolve relative to this script):
#   powershell -NoProfile -File tools/run_2monitor_evidence.ps1
#   powershell -NoProfile -File tools/run_2monitor_evidence.ps1 `
#     -MonitorDumpExe <path> [-EngineLog <path>] [-OutRoot <path>]
#
# Output directory layout:
#   .omo/evidence/2monitor-<yyyyMMdd-HHmmss>/
#     monitor_dump.json      raw monitor_dump.exe stdout
#     placement.log          Select-String -Pattern 'placement:' extract
#     injected_windows.json  census_count + per-window GetWindowRect
#     windows_build.txt      [System.Environment]::OSVersion.Version
#     summary.txt            counts + any degraded-mode warnings

[CmdletBinding()]
param(
  # Optional explicit path to monitor_dump.exe. When empty the script searches
  # build/task-30 then any build/*/monitor_dump.exe (most recent first).
  [string]$MonitorDumpExe = '',
  # Optional explicit path to engine.log. Default: %LOCALAPPDATA%\K6WP\engine.log
  [string]$EngineLog = '',
  # Optional override for the bundle root. Default: <repo>/.omo/evidence
  [string]$OutRoot = ''
)

# NEVER fail the operator. Individual steps catch and warn; the script always
# exits 0 so a degraded bundle is still a usable bundle.
$ErrorActionPreference = 'Continue'

# --- Resolve repo root relative to THIS script (repo-root agnostic) ---
$scriptDir = $PSScriptRoot
if ([string]::IsNullOrEmpty($scriptDir)) {
  if ($MyInvocation.MyCommand.Path) {
    $scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
  } else {
    $scriptDir = (Get-Location).Path
  }
}
$repoRoot = Split-Path -Parent $scriptDir

if ([string]::IsNullOrEmpty($OutRoot)) {
  $OutRoot = Join-Path $repoRoot '.omo\evidence'
}

$warnings = New-Object System.Collections.Generic.List[string]

# --- Create the timestamped bundle directory (always) ---
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$bundleDir = Join-Path $OutRoot ("2monitor-{0}" -f $stamp)
try {
  New-Item -ItemType Directory -Path $bundleDir -Force | Out-Null
} catch {
  # Last-resort: fall back to %TEMP% so the script still exits 0 with a path.
  $bundleDir = Join-Path $env:TEMP ("k6wp-2monitor-{0}" -f $stamp)
  New-Item -ItemType Directory -Path $bundleDir -Force | Out-Null
  $warnings.Add("could not create $OutRoot; bundle written to $bundleDir instead")
}

# ---------------------------------------------------------------------------
# 1. monitor_dump.exe -> monitor_dump.json
# ---------------------------------------------------------------------------
$dumpExe = $MonitorDumpExe
if ([string]::IsNullOrEmpty($dumpExe)) {
  # Prefer the per-task binary dir, then any build/*/monitor_dump.exe.
  $candidates = @()
  $taskDirDump = Join-Path $repoRoot 'build\task-30\monitor_dump.exe'
  if (Test-Path -LiteralPath $taskDirDump) { $candidates += $taskDirDump }
  $buildRoot = Join-Path $repoRoot 'build'
  if (Test-Path -LiteralPath $buildRoot) {
    $any = Get-ChildItem -Path $buildRoot -Filter 'monitor_dump.exe' -Recurse -ErrorAction SilentlyContinue |
      Sort-Object LastWriteTime -Descending
    foreach ($a in $any) { if ($candidates -notcontains $a.FullName) { $candidates += $a.FullName } }
  }
  if ($candidates.Count -gt 0) { $dumpExe = $candidates[0] }
}

$monitorCount = -1
$dumpJsonPath = Join-Path $bundleDir 'monitor_dump.json'
if ([string]::IsNullOrEmpty($dumpExe) -or -not (Test-Path -LiteralPath $dumpExe)) {
  $warnings.Add("monitor_dump.exe not found (pass -MonitorDumpExe <path>); monitor_dump.json omitted")
} else {
  try {
    $dumpOut = & $dumpExe 2>&1
    $dumpText = ($dumpOut | Out-String).Trim()
    # Write raw stdout; never re-encode (PS 5.1 JSON-in-string trap).
    Set-Content -LiteralPath $dumpJsonPath -Value $dumpText -Encoding UTF8
    # Parse .monitors[] (row 2 wrapper shape: {"monitors":[...],"virtual_screen":{...}})
    try {
      $parsed = $dumpText | ConvertFrom-Json
      if ($null -ne $parsed -and $null -ne $parsed.monitors) {
        $monitorCount = @($parsed.monitors).Count
      } else {
        $warnings.Add("monitor_dump.json has no .monitors[] array (unexpected shape)")
      }
    } catch {
      $warnings.Add("monitor_dump.json could not be parsed as JSON: $($_.Exception.Message)")
    }
  } catch {
    $warnings.Add("monitor_dump.exe failed to run: $($_.Exception.Message)")
  }
}

# ---------------------------------------------------------------------------
# 2. engine.log placement: lines -> placement.log
# ---------------------------------------------------------------------------
$logPath = $EngineLog
if ([string]::IsNullOrEmpty($logPath)) {
  $logPath = Join-Path $env:LOCALAPPDATA 'K6WP\engine.log'
}
$placementPath = Join-Path $bundleDir 'placement.log'
$placementCount = 0
if (-not (Test-Path -LiteralPath $logPath)) {
  $warnings.Add("engine.log not found at $logPath (start the engine first, or pass -EngineLog <path>)")
} else {
  try {
    # Exact run-book commands:
    #   Get-Content <engine.log> | Select-String -Pattern 'placement:'
    $hits = @(Get-Content -LiteralPath $logPath -ErrorAction Stop |
      Select-String -Pattern 'placement:')
    $placementCount = $hits.Count
    if ($placementCount -gt 0) {
      $hits | ForEach-Object { $_.Line } | Set-Content -LiteralPath $placementPath -Encoding UTF8
    } else {
      Set-Content -LiteralPath $placementPath -Value '(no placement: lines found)' -Encoding UTF8
      $warnings.Add("engine.log exists but contains no placement: lines (build the engine with -DK6WP_VERBOSE=ON)")
    }
  } catch {
    $warnings.Add("could not read engine.log: $($_.Exception.Message)")
  }
}

# ---------------------------------------------------------------------------
# 3. Injected-window census (EnumWindows + EnumChildWindows + GetWindowRect)
#    GetClassNameW MUST be CharSet.Unicode (learnings.md:171-175 trap).
# ---------------------------------------------------------------------------
$censusPath = Join-Path $bundleDir 'injected_windows.json'
$censusCount = 0
$censusWindows = @()

$censusSource = @'
using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;

public static class K6wpCensus {
  [StructLayout(LayoutKind.Sequential)]
  public struct RECT {
    public int Left;
    public int Top;
    public int Right;
    public int Bottom;
  }

  private delegate bool EnumProc(IntPtr hWnd, IntPtr lParam);

  // NOTE: GetClassNameW must be CharSet.Unicode — without it every class name
  // marshals as ANSI and reads as its FIRST letter ("Progman" -> "P",
  // "K6WP.DesktopInject.1" -> "K"), so class matching silently fails.
  [DllImport("user32.dll", CharSet = CharSet.Unicode)]
  private static extern int GetClassNameW(IntPtr hWnd, StringBuilder lpClassName, int nMaxCount);

  [DllImport("user32.dll")]
  private static extern bool EnumWindows(EnumProc lpEnumFunc, IntPtr lParam);

  [DllImport("user32.dll")]
  private static extern bool EnumChildWindows(IntPtr hWndParent, EnumProc lpEnumFunc, IntPtr lParam);

  [DllImport("user32.dll")]
  private static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);

  [DllImport("user32.dll")]
  private static extern bool IsWindow(IntPtr hWnd);

  // Top-level shells we search under: Progman and WorkerW (24H2 hosts the
  // injected surface under WorkerW; classic/earlier layouts use Progman).
  private static readonly List<long> shells = new List<long>();
  private static readonly List<long> injected = new List<long>();
  private const string InjectClass = "K6WP.DesktopInject.1";

  private static string GetClass(IntPtr h) {
    StringBuilder sb = new StringBuilder(256);
    GetClassNameW(h, sb, sb.Capacity);
    return sb.ToString();
  }

  private static bool ShellCb(IntPtr h, IntPtr p) {
    string c = GetClass(h);
    if (c == "Progman" || c == "WorkerW") shells.Add(h.ToInt64());
    return true; // keep enumerating
  }

  private static bool ChildCb(IntPtr h, IntPtr p) {
    if (GetClass(h) == InjectClass) injected.Add(h.ToInt64());
    return true; // count ALL injected windows, not just the first
  }

  // Returns "hwnd|left|top|right|bottom" lines for every injected window.
  public static string[] Census() {
    shells.Clear();
    injected.Clear();

    // Walk-blindness trap: FindWindowW(Progman) often returns 0x0 from QA
    // scripts. EnumWindows + class match is the reliable path.
    EnumWindows(ShellCb, IntPtr.Zero);

    foreach (long shell in shells) {
      EnumChildWindows(new IntPtr(shell), ChildCb, IntPtr.Zero);
    }

    // De-duplicate (a window can appear under both shells in odd layouts).
    HashSet<long> seen = new HashSet<long>();
    List<string> rows = new List<string>();
    foreach (long hw in injected) {
      if (!seen.Add(hw)) continue;
      IntPtr h = new IntPtr(hw);
      if (!IsWindow(h)) continue;
      RECT r;
      if (!GetWindowRect(h, out r)) continue;
      rows.Add(string.Format("{0}|{1}|{2}|{3}|{4}", hw, r.Left, r.Top, r.Right, r.Bottom));
    }
    return rows.ToArray();
  }
}
'@

try {
  if (-not ('K6wpCensus' -as [type])) {
    Add-Type -TypeDefinition $censusSource -ErrorAction Stop
  }
  $rows = [K6wpCensus]::Census()
  $censusCount = $rows.Count
  $censusWindows = @()
  foreach ($row in $rows) {
    $parts = $row -split '\|'
    if ($parts.Count -eq 5) {
      $hwnd = [Convert]::ToInt64($parts[0])
      $left = [int]$parts[1]; $top = [int]$parts[2]
      $right = [int]$parts[3]; $bottom = [int]$parts[4]
      $censusWindows += [pscustomobject]@{
        hwnd    = ('0x{0:X}' -f $hwnd)
        class   = 'K6WP.DesktopInject.1'
        left    = $left
        top     = $top
        right   = $right
        bottom  = $bottom
        width   = ($right - $left)
        height  = ($bottom - $top)
      }
    }
  }
} catch {
  $warnings.Add("census P/Invoke failed: $($_.Exception.Message)")
}

# Degraded-mode contract (row 30): census_count: 0 + the exact warning line.
if ($censusCount -eq 0) {
  $warnings.Add('engine not running or no injected windows found')
}

$censusObj = [pscustomobject]@{
  census_count  = $censusCount
  monitor_count = $monitorCount
  class         = 'K6WP.DesktopInject.1'
  technique     = 'EnumWindows(Progman/WorkerW) + EnumChildWindows + GetWindowRect; GetClassNameW CharSet=Unicode'
  windows       = $censusWindows
}
# ConvertTo-Json on pre-built objects avoids the PS 5.1 backslash-escaped-quote trap.
$censusObj | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $censusPath -Encoding UTF8

# ---------------------------------------------------------------------------
# 4. Windows build -> windows_build.txt
# ---------------------------------------------------------------------------
$buildPath = Join-Path $bundleDir 'windows_build.txt'
try {
  $osv = [System.Environment]::OSVersion
  $buildLines = @(
    ('OSVersion.Version: {0}' -f $osv.Version),
    ('OSVersion.VersionString: {0}' -f $osv.VersionString),
    ('OSVersion.Platform: {0}' -f $osv.Platform),
    ('64BitOperatingSystem: {0}' -f [System.Environment]::Is64BitOperatingSystem),
    '',
    'Equivalent winver check (paste the dialog if asked):',
    '  winver',
    'Equivalent PowerShell check:',
    '  [System.Environment]::OSVersion.Version'
  )
  $buildLines | Set-Content -LiteralPath $buildPath -Encoding UTF8
} catch {
  $warnings.Add("could not read OSVersion: $($_.Exception.Message)")
}

# ---------------------------------------------------------------------------
# 5. summary.txt — counts + warnings (always written)
# ---------------------------------------------------------------------------
$summaryPath = Join-Path $bundleDir 'summary.txt'
$engineAlive = $false
try {
  $engines = @(Get-Process -Name 'engine' -ErrorAction SilentlyContinue)
  $engineAlive = ($engines.Count -gt 0)
} catch { $engineAlive = $false }

$summaryLines = @(
  ('bundle_dir: {0}' -f $bundleDir),
  ('generated: {0}' -f (Get-Date -Format 'o')),
  ('monitor_dump_exe: {0}' -f $(if ($dumpExe) { $dumpExe } else { '(not found)' })),
  ('engine_log: {0}' -f $logPath),
  ('monitor_count: {0}' -f $monitorCount),
  ('census_count: {0}' -f $censusCount),
  ('placement_line_count: {0}' -f $placementCount),
  ('engine_process_alive: {0}' -f $engineAlive),
  ('census_equals_monitors: {0}' -f $(if ($monitorCount -ge 0 -and $censusCount -eq $monitorCount) { 'yes' } else { 'no-or-unknown' })),
  '',
  'warnings:'
)
if ($warnings.Count -eq 0) {
  $summaryLines += '  (none)'
} else {
  foreach ($w in $warnings) { $summaryLines += ('  - {0}' -f $w) }
}
$summaryLines | Set-Content -LiteralPath $summaryPath -Encoding UTF8

# ---------------------------------------------------------------------------
# Console report + exit 0 (always)
# ---------------------------------------------------------------------------
Write-Output ('bundle_dir: {0}' -f $bundleDir)
Write-Output ('monitor_count: {0}' -f $monitorCount)
Write-Output ('census_count: {0}' -f $censusCount)
Write-Output ('placement_line_count: {0}' -f $placementCount)
Write-Output ('engine_process_alive: {0}' -f $engineAlive)
foreach ($w in $warnings) {
  Write-Output ('WARNING: {0}' -f $w)
}
Write-Output ('files: monitor_dump.json, placement.log, injected_windows.json, windows_build.txt, summary.txt')
Write-Output ('screenshots are NOT automated - take one per display mode by hand (see docs/runbook-2monitor.md)')
exit 0
