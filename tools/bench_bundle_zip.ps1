# tools/bench_bundle_zip.ps1 - the compressed (shipping) size per group.
#
# The INSTALL size and the ZIP size are different levers: the install is
# dominated by binaries that do not compress, and the ZIP is what the user
# downloads. This measures the per-group COMPRESSED contribution from the
# actual shipped ZIP, so the bundle target is set against the number the
# user actually feels.
$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
. (Join-Path $here 'bench_lib.ps1')

$zipPath = (Get-ChildItem (Join-Path $here '..\dist') -Filter '*.zip' -File |
    Sort-Object LastWriteTime -Descending | Select-Object -First 1).FullName
if (-not $zipPath) { throw "no ZIP found in dist\" }
Write-Host "zip: $zipPath"

Add-Type -AssemblyName System.IO.Compression.FileSystem
$zip = [System.IO.Compression.ZipFile]::OpenRead($zipPath)
try {
    $rows = @()
    foreach ($e in $zip.Entries) {
        if ($e.Length -eq 0) { continue }
        $name = $e.FullName
        $group = 'other'
        if ($name -match '(^|/)ffmpeg\.exe$' -or $name -match '(^|/)ffprobe\.exe$') { $group = 'ffmpeg+ffprobe' }
        elseif ($name -match '(^|/)libmpv-2\.dll$') { $group = 'mpv' }
        elseif ($name -match '(^|/)Qt6.*\.dll$') { $group = 'qt-dlls' }
        elseif ($name -match '(^|/)qml/') { $group = 'qml' }
        elseif ($name -match '(^|/)dxcompiler\.dll$' -or $name -match '(^|/)dxil\.dll$') { $group = 'shader-cache' }
        elseif ($name -match '(^|/)(engine|studio|K6WP|compressor)\.exe$') { $group = 'k6wp-exes' }
        elseif ($name -match '(^|/)vcruntime' -or $name -match '(^|/)msvcp') { $group = 'msvc-runtime' }
        $rows += [pscustomobject]@{
            name = $name
            uncompressed = $e.Length
            compressed = $e.CompressedLength
            group = $group
        }
    }
    $totalU = ($rows | Measure-Object uncompressed -Sum).Sum
    $totalC = ($rows | Measure-Object compressed -Sum).Sum
    Write-Host ""
    Write-Host ("ZIP: {0} entries, {1} MB uncompressed, {2} MB compressed ({3}% ratio)" -f `
        $rows.Count, [math]::Round($totalU / 1MB, 2), [math]::Round($totalC / 1MB, 2),
        [math]::Round(($totalC / $totalU) * 100, 1))
    Write-Host ""
    Write-Host ("{0,-20} {1,10} {2,10} {3,8}" -f 'group', 'uncomp MB', 'zip MB', 'zip %')
    $agg = $rows | Group-Object group | ForEach-Object {
        [pscustomobject]@{
            group = $_.Name
            uncomp = [math]::Round((($_.Group | Measure-Object uncompressed -Sum).Sum) / 1MB, 2)
            zip = [math]::Round((($_.Group | Measure-Object compressed -Sum).Sum) / 1MB, 2)
            pct = [math]::Round((($_.Group | Measure-Object compressed -Sum).Sum) / $totalC * 100, 1)
        }
    } | Sort-Object zip -Descending
    $agg | Format-Table -AutoSize
    $out = [ordered]@{
        tool = 'bench_bundle_zip'
        date = (Get-Date -Format 'yyyy-MM-ddTHH:mm:ssK')
        label = 'live-proven (one machine)'
        zip = (Split-Path -Leaf $zipPath)
        entries = $rows.Count
        uncompressed_mb = [math]::Round($totalU / 1MB, 2)
        compressed_mb = [math]::Round($totalC / 1MB, 2)
        groups = $agg
        largest = @($rows | Sort-Object compressed -Descending | Select-Object -First 12 name,
            @{n='uncompressed_mb';e={[math]::Round($_.uncompressed/1MB,2)}},
            @{n='zip_mb';e={[math]::Round($_.compressed/1MB,2)}})
    }
    $json = $out | ConvertTo-Json -Depth 6
    $p = Join-Path $here '..\docs\bench\bundle-zip.json'
    $json | Set-Content -LiteralPath ([System.IO.Path]::GetFullPath($p)) -Encoding UTF8
    Write-Host "wrote $p"
}
finally { $zip.Dispose() }
