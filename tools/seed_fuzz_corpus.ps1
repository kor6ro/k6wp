# tools/seed_fuzz_corpus.ps1 - the seed corpus for cargo-fuzz (task I2).
#
# WHY SYNTHETIC SEEDS
#   The real corpus (tests/corpus) holds the user's videos and is NOT in
#   git, so the CI cannot use it. The seeds here are SYNTHETIC: a small
#   MP4-shaped byte string (ftyp + moov > trak > ... > avc1 > avcC) plus
#   the hostile shapes the parser must survive (a huge box size, a huge
#   table count, a truncated box). They are committed, deterministic and
#   contain no user data.
#
#   If the real corpus is present, its HEADERS are added as extra seeds
#   (the local run is then broader than the CI run - and the local run is
#   the one that finds the interesting samples).
#
# USAGE
#   powershell -File tools/seed_fuzz_corpus.ps1
#
[CmdletBinding()]
param(
    [switch]$WithRealCorpus
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$fuzz = Join-Path $root 'rust\fuzz\corpus'

function New-Dir([string]$path) {
    New-Item -ItemType Directory -Force -Path $path | Out-Null
}

function Write-Bytes([string]$path, [byte[]]$bytes) {
    [IO.File]::WriteAllBytes($path, $bytes)
}

# --- a synthetic MP4 (the sizes are computed by the helper below) ---
# The synthetic structure is written with the sizes computed explicitly
# (the helper above builds the payloads; the sizes are set here).
function Build-SyntheticMp4 {
    param([byte[]]$SpsNal)
    $chunks = New-Object System.Collections.Generic.List[byte[]]

    function Add-Box([string]$Kind, [byte[]]$Body) {
        $box = New-Object System.IO.MemoryStream
        $w = New-Object System.IO.BinaryWriter($box)
        $size = 8 + $Body.Length
        $w.Write([byte[]]@([byte]($size -shr 24), [byte]($size -shr 16), [byte]($size -shr 8), [byte]$size))
        $w.Write([Text.Encoding]::ASCII.GetBytes($Kind))
        $w.Write($Body)
        $w.Flush()
        $result = $box.ToArray()
        $w.Dispose()
        return $result
    }

    $ftyp = Add-Box 'ftyp' ([Text.Encoding]::ASCII.GetBytes('isom' + [char]0 + [char]0 + [char]2 + [char]0 + 'isomiso2avc1mp41'))
    $chunks.Add($ftyp)

    $avcCBody = New-Object System.IO.MemoryStream
    $aw = New-Object System.IO.BinaryWriter($avcCBody)
    $aw.Write([byte]1); $aw.Write([byte]0x42); $aw.Write([byte]0xC0); $aw.Write([byte]0x1F)
    $aw.Write([byte]0xFF); $aw.Write([byte]0xE1)
    $aw.Write([byte](($SpsNal.Length - 1) -shr 8)); $aw.Write([byte](($SpsNal.Length - 1) -band 0xFF))
    $aw.Write($SpsNal)
    $aw.Write([byte]1); $aw.Write([byte]0)
    $aw.Flush()
    $avcC = Add-Box 'avcC' $avcCBody.ToArray()
    $aw.Dispose()

    $avc1 = Add-Box 'avc1' $avcC
    $stsdBody = [byte[]]@(0, 0, 0, 0, 0, 0, 0, 1) + $avc1
    $stsd = Add-Box 'stsd' $stsdBody
    $stbl = Add-Box 'stbl' $stsd
    $minf = Add-Box 'minf' $stbl
    $mdia = Add-Box 'mdia' $minf
    $trak = Add-Box 'trak' $mdia
    $moov = Add-Box 'moov' $trak
    $chunks.Add($moov)

    $all = New-Object System.IO.MemoryStream
    foreach ($c in $chunks) { $all.Write($c, 0, $c.Length) }
    $all.Flush()
    $bytes = $all.ToArray()
    $all.Dispose()
    return $bytes
}

# The seeds ------------------------------------------------------------------
$spsNal = [byte[]]@(0x67, 0x42, 0xC0, 0x1F, 0xDA, 0x01, 0x40, 0x16, 0xEC, 0x04, 0x59, 0x9B, 0x20, 0x2D, 0xF9, 0x0A, 0x1E, 0x00, 0x00, 0x03, 0x00, 0x01, 0x00, 0x00, 0x03, 0x00, 0x32, 0x0F, 0x16, 0x2E, 0x48)
$mp4 = Build-SyntheticMp4 -SpsNal $spsNal

foreach ($target in @('sps_parser', 'box_parser', 'library_codec', 'ipc_codec')) {
    New-Dir (Join-Path $fuzz $target)
}

Write-Bytes (Join-Path $fuzz 'sps_parser\synthetic_mp4') $mp4
Write-Bytes (Join-Path $fuzz 'sps_parser\synthetic_mp4_truncated') ($mp4[0..([Math]::Min(200, $mp4.Length - 1))])
$huge = $mp4[0..7] + [byte[]]@(0xFF, 0xFF, 0xFF, 0xFF) + $mp4[8..($mp4.Length - 1)]
Write-Bytes (Join-Path $fuzz 'sps_parser\huge_mdat_size') $huge
Write-Bytes (Join-Path $fuzz 'box_parser\synthetic_mp4') $mp4
Write-Bytes (Join-Path $fuzz 'box_parser\synthetic_mp4_truncated') ($mp4[0..([Math]::Min(120, $mp4.Length - 1))])
Write-Bytes (Join-Path $fuzz 'box_parser\all_ones') ([byte[]]@(0xFF) * 512)

$libraryJson = @'
{"version":1,"entries":[{"path":"C:/videos/a.mp4","title":"a","duration_sec":10.5,"width":1920,"height":1080}]}
'@
Write-Bytes (Join-Path $fuzz 'library_codec\valid') ([Text.Encoding]::UTF8.GetBytes($libraryJson))
Write-Bytes (Join-Path $fuzz 'library_codec\broken') ([Text.Encoding]::UTF8.GetBytes('{"version":1,"entries":[{"path":'))

$ipcFrame = '{"version":1,"cmd":"get_state","payload":{}}' + "`n"
Write-Bytes (Join-Path $fuzz 'ipc_codec\get_state') ([Text.Encoding]::UTF8.GetBytes($ipcFrame))
Write-Bytes (Join-Path $fuzz 'ipc_codec\set_video') ([Text.Encoding]::UTF8.GetBytes('{"version":1,"cmd":"set_video","payload":{"path":"C:/v/a.mp4"}}' + "`n"))
Write-Bytes (Join-Path $fuzz 'ipc_codec\garbage') ([Text.Encoding]::UTF8.GetBytes('not json at all'))

# The real corpus headers (the local run is broader than the CI run).
if ($WithRealCorpus -and (Test-Path (Join-Path $root 'tests\corpus'))) {
    Get-ChildItem (Join-Path $root 'tests\corpus') -File | ForEach-Object {
        $head = New-Object byte[] 65536
        $stream = [IO.File]::OpenRead($_.FullName)
        $read = $stream.Read($head, 0, $head.Length)
        $stream.Dispose()
        Write-Bytes (Join-Path $fuzz "sps_parser\corpus_$($_.BaseName)") ($head[0..($read - 1)])
        Write-Bytes (Join-Path $fuzz "box_parser\corpus_$($_.BaseName)") ($head[0..($read - 1)])
    }
    Write-Host "  [fuzz] the real corpus headers were added as seeds"
}

$count = (Get-ChildItem $fuzz -Recurse -File).Count
Write-Host "  [fuzz] the seed corpus: $count files in $fuzz"
