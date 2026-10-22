# tools/bench_h3_bitdepth.ps1 - task H3: the bit depth and the codec, MEASURED.
#
# WHAT IT MEASURES
#   The extra-RAM estimate (the import dialog) says a 10-bit file costs
#   twice the frame pool of an 8-bit one. This measures it: the same
#   encoder settings, the same resolution, the same duration - only the bit
#   depth differs - and the two arms run ALTERNATELY inside one session
#   (the GPU numbers drift between sessions).
#
#   The metric is the C4 pair: the private bytes + the GPU shared memory.
#   The playback rate and the frame counters travel with every number: a
#   run whose rate is outside 0.97-1.03 is invalid.
#
# THE LIVE SESSION
#   The A/B replaces the wallpaper, so the session is snapshotted and
#   restored with tools/session_snapshot.ps1 (the wallpaper, the still, the
#   lock screen, the display mode). The user's engine is stopped for the
#   measurement and restarted with ITS OWN command line in the `finally`.
#
[CmdletBinding()]
param(
    [string]$EngineExe = '',
    [string]$CorpusDir = '',
    [int]$Runs = 3,
    [string]$OutJson = ''
)
$ErrorActionPreference = 'Stop'
$script:Here = Split-Path -Parent $MyInvocation.MyCommand.Path
. (Join-Path $script:Here 'bench_lib.ps1')
. (Join-Path $script:Here 'session_snapshot.ps1')

if ([string]::IsNullOrWhiteSpace($EngineExe)) { $EngineExe = Join-Path $script:Here '..\build\release\engine.exe' }
if ([string]::IsNullOrWhiteSpace($CorpusDir)) { $CorpusDir = Join-Path $script:Here '..\tests\corpus' }
if ([string]::IsNullOrWhiteSpace($OutJson)) { $OutJson = Join-Path $script:Here '..\docs\bench\h3-bitdepth.json' }
$engine = [IO.Path]::GetFullPath($EngineExe)
$corpus = [IO.Path]::GetFullPath($CorpusDir)

# The pairs: the same encoder settings, the same size - only the bit depth
# differs. The third file is the unknown-codec case (the parser reads no
# SPS for it, so the estimate is UNKNOWN).
$pairs = @(
    [pscustomobject]@{ name = 'h264_8bit_vs_10bit'
                       a = (Join-Path $corpus 'h264_8bit_1080p.mp4')
                       b = (Join-Path $corpus 'hi10_1080p.mp4')
                       note = 'H.264, 4 refs, 1920x1088 coded: the 10-bit frame pool is twice the 8-bit one' },
    [pscustomobject]@{ name = 'hevc_8bit_vs_10bit'
                       a = (Join-Path $corpus 'hevc8_1080p.mp4')
                       b = (Join-Path $corpus 'hevc10_1080p.mp4')
                       note = 'HEVC, 6 refs, 1920x1088 coded: the 10-bit frame pool is twice the 8-bit one' },
    [pscustomobject]@{ name = 'mpeg2_unknown_codec'
                       a = (Join-Path $corpus 'mpeg2_1080p.m2v')
                       b = ''
                       note = 'MPEG-2: the parser reads no SPS, so the estimate is UNKNOWN (the policy)' }
)
foreach ($p in $pairs) {
    foreach ($f in @($p.a, $p.b)) {
        if ($f -and -not (Test-Path -LiteralPath $f)) { throw "missing corpus file: $f" }
    }
}

# The live engine's command line, so it can be restarted exactly as it was.
$live = @(Get-CimInstance Win32_Process -Filter "Name='engine.exe'" -ErrorAction SilentlyContinue)
$liveCommandLine = if ($live.Count -gt 0) { $live[0].CommandLine } else { '' }

function Restart-LiveEngine {
    if ([string]::IsNullOrWhiteSpace($liveCommandLine)) { return }
    # The command line is "<exe> <args...>"; the exe path is quoted or not.
    $exe = $liveCommandLine
    $args = ''
    if ($exe -match '^"([^"]+)"\s*(.*)$') {
        $exe = $Matches[1]
        $args = $Matches[2]
    } elseif ($exe -match '^(\S+)\s+(.*)$') {
        $exe = $Matches[1]
        $args = $Matches[2]
    }
    if (-not (Test-Path -LiteralPath $exe)) {
        Write-Host "  [warn] the live engine's binary is gone ($exe); the session is restored without it"
        return
    }
    $argList = @()
    if ($args) { $argList = @($args -split '\s+') }
    try {
        if ($argList.Count -gt 0) {
            Start-Process -FilePath $exe -ArgumentList $argList -WindowStyle Hidden | Out-Null
        } else {
            Start-Process -FilePath $exe -WindowStyle Hidden | Out-Null
        }
        Write-Host "  [restore] the live engine restarted ($exe)"
    } catch {
        Write-Host "  [warn] the live engine could not be restarted: $_"
    }
}

$report = [ordered]@{
    tool = 'bench_h3_bitdepth'
    task = 'H3'
    date = (Get-Date -Format 'yyyy-MM-ddTHH:mm:ssK')
    label = 'live-proven (one machine)'
    method = 'ABAB alternating inside one session; the C4 metrics (private bytes + GPU shared); the playback rate and the frame counters travel with every number; the session is snapshotted and restored'
    engine = $engine
    runs = $Runs
    pairs = @()
}

try {
    Invoke-SessionSafe -Name 'H3 the bit-depth A/B' -ScriptBlock {
    # The A/B refuses to run beside a running engine.
    foreach ($proc in @(Get-Process engine -ErrorAction SilentlyContinue)) {
        try { Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue } catch { }
    }
    Start-Sleep -Seconds 2

    foreach ($pair in $pairs) {
        $bName = if ($pair.b) { Split-Path -Leaf $pair.b } else { '(none)' }
        Write-Host ("  {0}: A={1} B={2}" -f $pair.name, (Split-Path -Leaf $pair.a), $bName)
        # The A/B refuses to run beside a running engine: the previous
        # pair's engine may still be shutting down.
        for ($i = 0; $i -lt 40; $i++) {
            if (-not (Get-Process engine -ErrorAction SilentlyContinue)) { break }
            Start-Sleep -Milliseconds 250
        }
        foreach ($proc in @(Get-Process engine -ErrorAction SilentlyContinue)) {
            try { Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue } catch { }
        }
        Start-Sleep -Milliseconds 800
        $abOut = Join-Path $env:TEMP ("k6wp-h3-" + [Guid]::NewGuid().ToString('N').Substring(0, 8) + '.json')
        if ($pair.b) {
            & (Join-Path $script:Here 'bench_dpb_ab.ps1') -EngineExe $engine -FileA $pair.a -FileB $pair.b `
                -Runs $Runs -OutJson $abOut | ForEach-Object { Write-Host "    $_" }
        } else {
            # The unknown-codec case: one arm against itself is meaningless,
            # so it is measured once (the numbers are the reference, not a
            # comparison).
            & (Join-Path $script:Here 'bench_dpb_ab.ps1') -EngineExe $engine -FileA $pair.a -FileB $pair.a `
                -Runs 1 -OutJson $abOut | ForEach-Object { Write-Host "    $_" }
        }
        $ab = Get-Content -LiteralPath $abOut -Raw | ConvertFrom-Json
        # The rate decides whether a run counts: outside 0.97-1.03 the two
        # arms did not do the same amount of work (the project's rule).
        $aRate = [double]$ab.a.playback_rate
        $bRate = [double]$ab.b.playback_rate
        $aValid = ($aRate -ge 0.97 -and $aRate -le 1.03)
        $bValid = ($bRate -ge 0.97 -and $bRate -le 1.03)
        $report.pairs += [ordered]@{
            name = $pair.name
            note = $pair.note
            a = (Split-Path -Leaf $pair.a)
            b = $bName
            a_private_mb = $ab.a.private_mb
            a_private_band_mb = $ab.a.private_band_mb
            a_gpu_shared_mb = $ab.a.gpu_shared_mb
            a_gpu_shared_band_mb = $ab.a.gpu_shared_band_mb
            a_playback_rate = $aRate
            a_rate_valid = $aValid
            b_private_mb = $ab.b.private_mb
            b_private_band_mb = $ab.b.private_band_mb
            b_gpu_shared_mb = $ab.b.gpu_shared_mb
            b_gpu_shared_band_mb = $ab.b.gpu_shared_band_mb
            b_playback_rate = $bRate
            b_rate_valid = $bValid
            comparable = ($aValid -and $bValid)
        }
        Remove-Item -LiteralPath $abOut -Force -ErrorAction SilentlyContinue
    }
    } | Out-Null
} finally {
    # The user's engine comes back, exactly as it was - even when a pair
    # failed (the live session must never be left without its wallpaper).
    Restart-LiveEngine
}

$json = $report | ConvertTo-Json -Depth 6
[IO.File]::WriteAllText([System.IO.Path]::GetFullPath($OutJson), $json, (New-Object Text.UTF8Encoding($false)))
Write-Host "bench_h3_bitdepth: the report is in $OutJson"
exit 0
