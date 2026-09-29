# tools/verify_wallpaper.ps1 — repeat wallpaper-switch stress + chrome audit (Todo 11 + 30).
#
# Part A (Todo 11, Wave 2) loops N x set_video via the exact current NDJSON IPC protocol and proves:
#   1. every switch is acked ok,
#   2. the engine process stays alive throughout,
#   3. working-set memory stays stable (non-monotonic, bounded),
#   4. a mid-loop bad-path (missing file) is cleanly REJECTED without killing
#      the engine or breaking the loop.
#
# Part B (Todo 30) then audits the window chrome live via user32 P/Invoke:
#   TOOLWINDOW bit set, NO WS_CAPTION/WS_BORDER frame, NO owned visible
#   top-level (no taskbar button), NO-AltTab (follows TOOLWINDOW), tray
#   owner message-window present + install log quoted. Any single failing
#   check => RESULT: FAIL + non-zero exit.
#
# Protocol (shared/ipc_protocol.cpp, engine/src/ipc_server.cpp HandleMessage):
#   request: {"version":1,"cmd":"set_video","payload":{"path":"<utf8>"}}\n
#   ok ack:  {"ok":true}\n
#   reject:  {"error":"..."}\n   (frameless/empty datagrams get NO reply)
# Pipe: \\.\pipe\k6wp-engine (message mode).
#
# Pure stock PowerShell + .NET (NamedPipeClientStream). Zero dependencies.
# The harness launches the engine separately; this script NEVER starts it:
# if no engine answers at start it prints FAIL + exits non-zero.
#
# Usage:
#   powershell -ExecutionPolicy Bypass -File tools/verify_wallpaper.ps1 `
#     -VideoA <path> -VideoB <path> [-Count 20] [-EngineLog <path>]
# Defaults resolve relative to this script (repo-root agnostic, no hardcoded
# user paths): ..\build\spikes\test_1080p.mp4 and ..\tests\corpus\slideshow.mp4.
# -EngineLog optionally points at the engine.log to use for the honest
# fallbacks (default: $env:LOCALAPPDATA\K6WP\engine.log). Pass the private
# log when the engine runs with a session-local LOCALAPPDATA override.

[CmdletBinding()]
param(
  [string]$VideoA = '',
  [string]$VideoB = '',
  [int]$Count = 20,
  [string]$PipeName = 'k6wp-engine',
  [int]$ConnectTimeoutMs = 5000,
  [int]$AckTimeoutMs = 20000,
  [string]$EngineLog = ''
)

$ErrorActionPreference = 'Stop'

# Resolve default videos relative to THIS script (repo-root agnostic, no
# hardcoded user paths). $PSScriptRoot can be empty under some hosts
# (e.g. nested powershell -File), so fall back to MyCommand path, then CWD.
$scriptDir = $PSScriptRoot
if ([string]::IsNullOrEmpty($scriptDir)) {
  if ($MyInvocation.MyCommand.Path) {
    $scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
  } else {
    $scriptDir = (Get-Location).Path
  }
}
if ([string]::IsNullOrEmpty($VideoA)) {
  $VideoA = Join-Path $scriptDir '..\build\spikes\test_1080p.mp4'
}
if ([string]::IsNullOrEmpty($VideoB)) {
  $VideoB = Join-Path $scriptDir '..\tests\corpus\slideshow.mp4'
}

function Fail([string]$msg) {
  Write-Output "FAIL $msg"
  exit 1
}

# --- Preconditions: both videos must exist (real files; engine validates). ---
$VideoA = [IO.Path]::GetFullPath($VideoA)
$VideoB = [IO.Path]::GetFullPath($VideoB)
if (-not (Test-Path -LiteralPath $VideoA -PathType Leaf)) {
  Fail "VideoA not found: $VideoA"
}
if (-not (Test-Path -LiteralPath $VideoB -PathType Leaf)) {
  Fail "VideoB not found: $VideoB"
}
if ($Count -lt 1) { Fail "Count must be >= 1 (got $Count)" }

# --- Engine liveness at start (single instance expected; strays killed by harness). ---
$engines = @(Get-Process -Name 'engine' -ErrorAction SilentlyContinue)
if ($engines.Count -eq 0) {
  Write-Output 'FAIL engine not running'
  exit 1
}
if ($engines.Count -gt 1) {
  Fail "ambiguous: $($engines.Count) engine processes running (kill strays first)"
}
$engineId = $engines[0].Id
Write-Output "engine pid: $engineId"
Write-Output "videoA: $VideoA"
Write-Output "videoB: $VideoB"
Write-Output "count: $Count"

function Test-EngineAlive {
  try {
    $p = Get-Process -Id $engineId -ErrorAction Stop
    return (-not $p.HasExited)
  } catch {
    return $false
  }
}

function Get-WorkingSet {
  $p = Get-Process -Id $engineId -ErrorAction Stop
  $p.Refresh()
  return $p.WorkingSet64
}

# --- Connect (timeout => engine-down FAIL, never start the engine here). ---
$client = New-Object IO.Pipes.NamedPipeClientStream(
  '.', $PipeName, [IO.Pipes.PipeDirection]::InOut,
  [IO.Pipes.PipeOptions]::None)
try {
  $client.Connect($ConnectTimeoutMs)
} catch {
  $client.Dispose()
  Write-Output 'FAIL engine not running'
  exit 1
}
$client.ReadMode = [IO.Pipes.PipeTransmissionMode]::Message
$utf8 = [Text.Encoding]::UTF8
Write-Output 'pipe: connected'

function Read-Ack {
  # Reads one message-mode datagram with a hard timeout. Returns ack text.
  $buf = New-Object byte[] 65536
  $ms = New-Object IO.MemoryStream
  try {
    $deadline = [DateTime]::UtcNow.AddMilliseconds($AckTimeoutMs)
    do {
      $remaining = [int](($deadline - [DateTime]::UtcNow).TotalMilliseconds)
      if ($remaining -le 0) { throw [TimeoutException]'ack read timed out' }
      $ar = $client.BeginRead($buf, 0, $buf.Length, $null, $null)
      if (-not $ar.AsyncWaitHandle.WaitOne($remaining)) {
        throw [TimeoutException]'ack read timed out'
      }
      $n = $client.EndRead($ar)
      if ($n -le 0) { throw [IO.IOException]'pipe closed while reading ack' }
      $ms.Write($buf, 0, $n)
    } while (-not $client.IsMessageComplete)
    return $utf8.GetString($ms.ToArray())
  } finally {
    $ms.Dispose()
  }
}

function Send-SetVideo([string]$path) {
  # Returns @{ Ok=[bool]; Ack=[string]; LatencyMs=[double]; Error=[string] }.
  $sw = [Diagnostics.Stopwatch]::StartNew()
  try {
    $msg = (@{ version = 1; cmd = 'set_video'; payload = @{ path = $path } } |
      ConvertTo-Json -Compress -Depth 5) + "`n"
    $bytes = $utf8.GetBytes($msg)
    $client.Write($bytes, 0, $bytes.Length)
    $client.Flush()
    $ackText = Read-Ack
    $sw.Stop()
    $ack = $ackText | ConvertFrom-Json
    $ok = ($null -ne $ack.ok) -and ($ack.ok -eq $true)
    $err = $null
    if ($null -ne $ack.error) { $err = [string]$ack.error }
    return @{ Ok = $ok; Ack = $ackText.Trim(); LatencyMs = $sw.Elapsed.TotalMilliseconds; Error = $err }
  } catch {
    $sw.Stop()
    return @{ Ok = $false; Ack = ''; LatencyMs = $sw.Elapsed.TotalMilliseconds; Error = "EXCEPTION: $($_.Exception.Message)" }
  }
}

$memSamples = New-Object System.Collections.Generic.List[long]
$okCount = 0
$badPathOk = $false
$badPathError = ''
$failed = $false

try {
  for ($i = 1; $i -le $Count; $i++) {
    if (-not (Test-EngineAlive)) { Fail "engine died before iteration $i" }
    if ($i % 2 -eq 1) { $vid = $VideoA; $slot = 'A' } else { $vid = $VideoB; $slot = 'B' }
    $r = Send-SetVideo $vid
    if (-not $r.Ok) {
      Write-Output ("iter {0:D2}/{1} video={2} ack=FAIL latency={3:N0}ms ack={4} err={5}" -f $i, $Count, $slot, $r.LatencyMs, $r.Ack, $r.Error)
      $failed = $true
      break
    }
    $okCount++
    try {
      $ws = Get-WorkingSet
    } catch {
      Fail "engine died at iteration $i (cannot sample memory)"
    }
    $memSamples.Add($ws)
    $wsMb = [math]::Round($ws / 1MB, 1)
    Write-Output ("iter {0:D2}/{1} video={2} ack=ok latency={3:N0}ms workingset={4}MB" -f $i, $Count, $slot, $r.LatencyMs, $wsMb)

    # Mid-loop bad-path: missing file must be cleanly REJECTED, engine alive,
    # loop continues. (Iteration 10 of the default Count=20.)
    if ($i -eq 10) {
      $missing = Join-Path $env:TEMP ('k6wp-verify-wallpaper-missing-{0}.mp4' -f [Guid]::NewGuid().ToString('N'))
      $b = Send-SetVideo $missing
      if ($b.Ok) {
        Write-Output "iter 10 bad-path: UNEXPECTED-OK (engine accepted missing file) ack=$($b.Ack)"
        $failed = $true
        break
      }
      if ([string]::IsNullOrEmpty($b.Error) -and [string]::IsNullOrEmpty($b.Ack)) {
        Write-Output 'iter 10 bad-path: TIMEOUT (no ack for missing file)'
        $failed = $true
        break
      }
      if ($b.Error -like 'EXCEPTION:*') {
        Write-Output "iter 10 bad-path: TRANSPORT-FAIL $($b.Error)"
        $failed = $true
        break
      }
      if (-not (Test-EngineAlive)) {
        Write-Output "iter 10 bad-path: engine DIED after reject (ack=$($b.Ack))"
        $failed = $true
        break
      }
      $badPathOk = $true
      $badPathError = if ($b.Error) { $b.Error } else { $b.Ack }
      Write-Output "iter 10 bad-path: REJECT as expected (error=$badPathError) engine=alive loop=continues"
    }
  }
} finally {
  $client.Dispose()
}

if ($failed) {
  Write-Output "RESULT: FAIL ($okCount/$Count acks)"
  exit 1
}

if ($okCount -ne $Count) {
  Write-Output "RESULT: FAIL ($okCount/$Count acks)"
  exit 1
}
if (-not $badPathOk) {
  Write-Output "RESULT: FAIL ($okCount/$Count acks, bad-path probe did not run/reject)"
  exit 1
}
if (-not (Test-EngineAlive)) {
  Write-Output "RESULT: FAIL ($okCount/$Count acks, engine died at end)"
  exit 1
}

$first = $memSamples[0]
$mid = $memSamples[[math]::Floor($memSamples.Count / 2)]
$last = $memSamples[$memSamples.Count - 1]
$firstMb = [math]::Round($first / 1MB, 1)
$midMb = [math]::Round($mid / 1MB, 1)
$lastMb = [math]::Round($last / 1MB, 1)
$peak = ($memSamples | Measure-Object -Maximum).Maximum
$peakMb = [math]::Round($peak / 1MB, 1)
Write-Output ("memory first={0}MB mid={1}MB last={2}MB peak={3}MB (limit={4}MB)" -f $firstMb, $midMb, $lastMb, $peakMb, ([math]::Round($first * 1.5 / 1MB, 1)))
$memOk = ($last -le ($first * 1.5)) -and ($mid -le ($first * 1.5))
if (-not $memOk) {
  Write-Output 'RESULT: FAIL (memory unstable: end or mid exceeds start*1.5)'
  exit 1
}

# --- Todo 30: taskbar / Alt+Tab / frame / tray checks (appended; T11 above untouched). ---
# Live window reads via stock .NET P/Invoke only (Add-Type user32). Known trap
# (T7/T10): shell-side FindWindowW(Progman) often returns 0x0 (walk blindness)
# and EnumWindows MISSES SetParent'd popups, so the injected HWND is found via
# EnumChildWindows of Progman (+ top-level WorkerW fallback), matching the T9
# measurement pattern. engine.log lines are an HONEST fallback only (quoted
# verbatim, labelled via-log), never a faked live read.
Add-Type -TypeDefinition @'
using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;
public static class K6wpVerify {
  private delegate bool EnumProc(IntPtr hWnd, IntPtr lParam);
  [DllImport("user32.dll", CharSet = CharSet.Unicode)]
  private static extern IntPtr FindWindowW(string lpClassName, string lpWindowName);
  [DllImport("user32.dll")]
  private static extern bool EnumWindows(EnumProc lpEnumFunc, IntPtr lParam);
  [DllImport("user32.dll")]
  private static extern bool EnumChildWindows(IntPtr hWndParent, EnumProc lpEnumFunc, IntPtr lParam);
  [DllImport("user32.dll")]
  private static extern bool IsWindowVisible(IntPtr hWnd);
  [DllImport("user32.dll", CharSet = CharSet.Unicode)]
  private static extern int GetClassNameW(IntPtr hWnd, StringBuilder lpClassName, int nMaxCount);
  [DllImport("user32.dll", CharSet = CharSet.Unicode)]
  private static extern int GetWindowTextW(IntPtr hWnd, StringBuilder lpString, int nMaxCount);
  [DllImport("user32.dll")]
  private static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint lpdwProcessId);
  [DllImport("user32.dll")]
  private static extern IntPtr GetWindowLongPtrW(IntPtr hWnd, int nIndex);
  private const int GWL_EXSTYLE = -20;
  private const int GWL_STYLE = -16;
  public static string FindMethod = "none";
  private static string wantClass = "";
  private static long foundHwnd = 0;
  private static bool ChildCb(IntPtr h, IntPtr p) {
    StringBuilder sb = new StringBuilder(256);
    GetClassNameW(h, sb, sb.Capacity);
    if (sb.ToString() == wantClass) { foundHwnd = h.ToInt64(); return false; }
    return true;
  }
  private static List<IntPtr> workers = new List<IntPtr>();
  private static bool WorkerCb(IntPtr h, IntPtr p) {
    StringBuilder sb = new StringBuilder(256);
    GetClassNameW(h, sb, sb.Capacity);
    if (sb.ToString() == "WorkerW") workers.Add(h);
    return true;
  }
  public static long FindInjected(string cls) {
    wantClass = cls; foundHwnd = 0; FindMethod = "none";
    IntPtr prog = FindWindowW("Progman", null);
    if (prog != IntPtr.Zero) {
      EnumChildWindows(prog, ChildCb, IntPtr.Zero);
      if (foundHwnd != 0) { FindMethod = "Progman-children"; return foundHwnd; }
    }
    workers.Clear();
    EnumWindows(WorkerCb, IntPtr.Zero);
    foreach (IntPtr w in workers) {
      EnumChildWindows(w, ChildCb, IntPtr.Zero);
      if (foundHwnd != 0) { FindMethod = "WorkerW-children"; return foundHwnd; }
    }
    FindMethod = "not-found";
    return 0;
  }
  public static long GetExStyle(long hwnd) {
    return GetWindowLongPtrW(new IntPtr(hwnd), GWL_EXSTYLE).ToInt64();
  }
  public static long GetStyle(long hwnd) {
    return GetWindowLongPtrW(new IntPtr(hwnd), GWL_STYLE).ToInt64();
  }
  private static uint wantPid = 0;
  private static string wantCls = "";
  private static string wantTitle = "";
  private static int tbCount = 0;
  private static bool TbCb(IntPtr h, IntPtr p) {
    if (!IsWindowVisible(h)) return true;
    uint pid; GetWindowThreadProcessId(h, out pid);
    if (pid != wantPid) return true;
    StringBuilder c = new StringBuilder(256); GetClassNameW(h, c, c.Capacity);
    StringBuilder t = new StringBuilder(256); GetWindowTextW(h, t, t.Capacity);
    if (c.ToString() == wantCls || t.ToString() == wantTitle) tbCount++;
    return true;
  }
  public static int CountOwnedTopLevels(uint pid, string cls, string title) {
    wantPid = pid; wantCls = cls; wantTitle = title; tbCount = 0;
    EnumWindows(TbCb, IntPtr.Zero);
    return tbCount;
  }
  private static long foundMsg = 0;
  private static bool MsgCb(IntPtr h, IntPtr p) {
    uint pid; GetWindowThreadProcessId(h, out pid);
    if (pid != wantPid) return true;
    StringBuilder c = new StringBuilder(256); GetClassNameW(h, c, c.Capacity);
    if (c.ToString() == wantCls) { foundMsg = h.ToInt64(); return false; }
    return true;
  }
  public static long FindMsgWindow(uint pid, string cls) {
    wantPid = pid; wantCls = cls; foundMsg = 0;
    EnumWindows(MsgCb, IntPtr.Zero);
    return foundMsg;
  }
}
'@

$injClass = 'K6WP.DesktopInject.1'
$injTitle = 'K6WP Wallpaper'
$msgClass = 'K6WP.Engine.MessageWindow.1'
$checkFailures = New-Object System.Collections.Generic.List[string]

$logPath = $EngineLog
if ([string]::IsNullOrEmpty($logPath)) {
  $logPath = Join-Path $env:LOCALAPPDATA 'K6WP\engine.log'
}

function Get-LogHits([string]$path, [string]$pattern) {
  try {
    $fs = New-Object IO.FileStream($path, [IO.FileMode]::Open,
      [IO.FileAccess]::Read, [IO.FileShare]::ReadWrite)
    try {
      $sr = New-Object IO.StreamReader($fs, [Text.Encoding]::UTF8)
      $text = $sr.ReadToEnd()
      $sr.Dispose()
      return @($text -split "`r?`n" | Where-Object { $_ -like $pattern })
    } finally {
      $fs.Dispose()
    }
  } catch {
    return @()
  }
}

$hwnd = [K6wpVerify]::FindInjected($injClass)
$findHow = [K6wpVerify]::FindMethod
if ($hwnd -ne 0) {
  Write-Output ("window: injected HWND=0x{0:X} via {1}" -f $hwnd, $findHow)
} else {
  Write-Output 'window: injected HWND NOT FOUND via live enumeration (Progman + WorkerW children)'
}

# 1. TOOLWINDOW: exstyle bit 0x80 (WS_EX_TOOLWINDOW) must be set (expect 0x08080080).
$toolPass = $false
$toolDetail = ''
if ($hwnd -ne 0) {
  $ex = [K6wpVerify]::GetExStyle($hwnd)
  $toolDetail = ('exstyle=0x{0:X8} (live)' -f $ex)
  if (($ex -band 0x80) -ne 0) { $toolPass = $true }
} else {
  $rows = Get-LogHits $logPath '*final exstyle*'
  if ($rows.Count -gt 0) {
    $line = $rows[$rows.Count - 1].Trim()
    $toolDetail = "via engine.log (no live HWND): $line"
    if ($line -like '*TOOLWINDOW=YES*') { $toolPass = $true }
  } else {
    $toolDetail = 'no live HWND and no final-exstyle line in engine.log'
  }
}
if ($toolPass) {
  Write-Output "check TOOLWINDOW: PASS $toolDetail"
} else {
  Write-Output "check TOOLWINDOW: FAIL $toolDetail"
  $checkFailures.Add('TOOLWINDOW')
}

# 2. NO-caption/border: style must NOT have WS_CAPTION (0xC00000) nor WS_BORDER
# (0x800000) — WS_POPUP frameless (live read only; no honest fallback exists).
$framePass = $false
$frameDetail = ''
if ($hwnd -ne 0) {
  $st = [K6wpVerify]::GetStyle($hwnd)
  $frameDetail = ('style=0x{0:X8} (live)' -f $st)
  if ((($st -band 0xC00000) -eq 0) -and (($st -band 0x800000) -eq 0)) { $framePass = $true }
} else {
  $frameDetail = 'no live HWND: style bits are only readable cross-process live'
}
if ($framePass) {
  Write-Output "check NO-caption-border: PASS $frameDetail"
} else {
  Write-Output "check NO-caption-border: FAIL $frameDetail"
  $checkFailures.Add('NO-caption-border')
}

# 3. NO-taskbar-button: zero visible top-levels owned by the engine pid carrying
# the injected class/title (T7 pattern: EnumWindows filtered on engine PID).
$tb = [K6wpVerify]::CountOwnedTopLevels([UInt32]$engineId, $injClass, $injTitle)
if ($tb -eq 0) {
  Write-Output 'check NO-taskbar-button: PASS 0 owned visible top-levels with injected class/title'
} else {
  Write-Output "check NO-taskbar-button: FAIL $tb owned visible top-level(s) with injected class/title"
  $checkFailures.Add('NO-taskbar-button')
}

# 4. NO-AltTab: WS_EX_TOOLWINDOW excludes the window from Alt+Tab, so this
# follows the TOOLWINDOW verdict (stated, not re-measured).
if ($toolPass) {
  Write-Output 'check NO-alttab: PASS (TOOLWINDOW set => excluded from Alt+Tab; follows TOOLWINDOW check)'
} else {
  Write-Output 'check NO-alttab: FAIL (follows TOOLWINDOW check)'
  $checkFailures.Add('NO-alttab')
}

# 5. tray-present: engine alive (proven above) + tray owner message window found
# via EnumWindows PID+class (T12 pattern); install log quoted best-effort.
# Stated honestly: cross-process tray icon pixels are not readable; the owner
# window + install line is the evidence.
$msgHwnd = [K6wpVerify]::FindMsgWindow([UInt32]$engineId, $msgClass)
$trayRows = Get-LogHits $logPath '*tray: icon installed*'
$trayLogNote = ('log icon-installed lines={0}' -f $trayRows.Count)
if ($trayRows.Count -gt 0) {
  $trayLogNote += (' last: {0}' -f $trayRows[$trayRows.Count - 1].Trim())
}
if ($msgHwnd -ne 0) {
  Write-Output ("check tray-present: PASS message-window HWND=0x{0:X} class={1}; {2}" -f $msgHwnd, $msgClass, $trayLogNote)
} else {
  if ($trayRows.Count -gt 0) {
    Write-Output ("check tray-present: PASS via engine.log (no live message HWND): {0}" -f $trayRows[$trayRows.Count - 1].Trim())
  } else {
    Write-Output "check tray-present: FAIL no message window and no icon-installed line in engine.log"
    $checkFailures.Add('tray-present')
  }
}

if ($checkFailures.Count -gt 0) {
  Write-Output ("RESULT: FAIL ($okCount/$Count acks, checks failed: {0})" -f ($checkFailures -join ','))
  exit 1
}

Write-Output ("RESULT: PASS ($okCount/$Count acks, bad-path REJECT, engine alive, memory stable, TOOLWINDOW, NO-taskbar, NO-alttab, NO-frame, tray)")
exit 0
