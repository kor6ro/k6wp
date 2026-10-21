# tools/extract_sps_truth.ps1 - the E3 ground truth.
#
# ffmpeg's `trace_headers` bitstream filter is the reference parser: it
# prints the SPS fields it decoded. This script runs it over the corpus and
# writes the expected DPB values into tests/golden/sps-dpb.json - the
# fixture the Rust parser is checked against.
#
# The corpus is NOT committed (it is gitignored), so the fixture records the
# FILE NAME and the expected values; tools/build_corpus.ps1 regenerates the
# corpus (and its SHA-256s are recorded next to the fixture).
#
# Usage:
#   powershell -NoProfile -ExecutionPolicy Bypass -File tools\extract_sps_truth.ps1
$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
$corpus = Join-Path $here '..\tests\corpus'
$ffmpeg = Join-Path $here '..\vendor\ffmpeg\ffmpeg.exe'
$out = Join-Path $here '..\tests\golden\sps-dpb.json'
if (-not (Test-Path -LiteralPath $ffmpeg)) { throw "ffmpeg not found: $ffmpeg" }

function Get-TraceFields([string]$File) {
    # ffmpeg writes its log to stderr; that is the DATA here, not an error.
    $old = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    $text = & $ffmpeg -hide_banner -v verbose -i $File -c copy -bsf:v trace_headers -f null - 2>&1 | Out-String
    $ErrorActionPreference = $old
    $refs = @()
    $vui = @()
    $hevc = @()
    $mbs = @()
    foreach ($line in ($text -split "`n")) {
        if ($line -match 'max_num_ref_frames\s+\S+\s*=\s*(\d+)') { $refs += [int]$Matches[1] }
        elseif ($line -match 'max_dec_frame_buffering\s+\S+\s*=\s*(\d+)') { $vui += [int]$Matches[1] }
        elseif ($line -match 'sps_max_dec_pic_buffering_minus1(?:\[\d+\])?\s+\S+\s*=\s*(\d+)') { $hevc += [int]$Matches[1] }
        elseif ($line -match 'frame_mbs_only_flag\s+\S+\s*=\s*(\d+)') { $mbs += [int]$Matches[1] }
    }
    $result = [ordered]@{
        max_num_ref_frames = if ($refs.Count -gt 0) { ($refs | Measure-Object -Maximum).Maximum } else { $null }
        max_dec_frame_buffering = if ($vui.Count -gt 0) { ($vui | Measure-Object -Maximum).Maximum } else { $null }
        sps_max_dec_pic_buffering = if ($hevc.Count -gt 0) { (($hevc | Measure-Object -Maximum).Maximum) + 1 } else { $null }
        interlaced = if ($mbs.Count -gt 0) { (($mbs | Measure-Object -Minimum).Minimum -eq 0) } else { $null }
        sps_count = $refs.Count + $hevc.Count
    }
    return $result
}

$files = Get-ChildItem -LiteralPath $corpus -File |
    Where-Object { $_.Extension -in @('.mp4', '.h264', '.mkv', '.webm') } |
    Where-Object { $_.Length -gt 0 } |
    Sort-Object Name
if ($files.Count -eq 0) { throw "the corpus is empty - run tools\build_corpus.ps1 first" }

$cases = @()
foreach ($f in $files) {
    Write-Host ("probing {0} ..." -f $f.Name)
    $truth = Get-TraceFields $f.FullName
    $sha = (Get-FileHash -LiteralPath $f.FullName -Algorithm SHA256).Hash.ToLower()
    $cases += [ordered]@{
        file = $f.Name
        sha256 = $sha
        bytes = $f.Length
        expected = $truth
    }
    Write-Host ("   refs={0} vui_dpb={1} hevc_dpb={2} interlaced={3}" -f `
            $truth.max_num_ref_frames, $truth.max_dec_frame_buffering, $truth.sps_max_dec_pic_buffering, $truth.interlaced)
}

$report = [ordered]@{
    tool = 'extract_sps_truth'
    date = (Get-Date -Format 'yyyy-MM-ddTHH:mm:ssK')
    label = 'the ground truth: ffmpeg trace_headers (its own bitstream parser)'
    note = 'the corpus is gitignored; tools\build_corpus.ps1 regenerates it. The sha256 pins the exact file the values were taken from.'
    cases = $cases
}
$json = $report | ConvertTo-Json -Depth 8
# No BOM: serde_json (and every JSON parser) rejects a byte-order mark.
$utf8 = New-Object System.Text.UTF8Encoding($false)
[System.IO.File]::WriteAllText([System.IO.Path]::GetFullPath($out), $json, $utf8)
Write-Host ""
Write-Host ("wrote {0} ({1} cases)" -f $out, $cases.Count)
