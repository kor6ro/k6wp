# gen_library_200.ps1 - generate a 200-entry synthetic library.json + thumbnails.
# No user data. Output dir is outside the repo (default D:\k6wp-test\library-200).
param(
    [string]$OutDir = "D:\k6wp-test\library-200",
    [int]$Count = 200
)
$ErrorActionPreference = "Stop"
$thumbDir = Join-Path $OutDir "thumbs"
$mediaDir = Join-Path $OutDir "media"
New-Item -ItemType Directory -Path $thumbDir -Force | Out-Null
New-Item -ItemType Directory -Path $mediaDir -Force | Out-Null

Add-Type -AssemblyName System.Drawing
$entries = @()
for ($i = 0; $i -lt $Count; $i++) {
    $n = $i + 1
    $isMissing = ($i % 17 -eq 0 -and $i -ne 0)  # ~12 entries
    $isUnc = ($i -ge $Count - 4)                 # last 4 are UNC
    if ($isUnc) {
        $src = "\\192.0.2.1\x\a$n.mp4"
        $dst = "\\192.0.2.1\x\a$n.mp4"
    } elseif ($isMissing) {
        $src = Join-Path $mediaDir "missing_$n.mp4"
        $dst = $src
    } else {
        $src = Join-Path $mediaDir "clip_$n.mp4"
        $dst = $src
        # create a tiny placeholder file so existence check succeeds
        if (-not (Test-Path $src)) { Set-Content -Path $src -Value "synthetic" -Encoding ascii }
    }
    # thumbnail
    $thumb = Join-Path $thumbDir "thumb_$n.jpg"
    $bmp = New-Object System.Drawing.Bitmap 160, 90
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $r = ($i * 37) % 256; $gr = ($i * 73) % 256; $b = ($i * 113) % 256; $g.Clear([System.Drawing.Color]::FromArgb($r, $gr, $b))
    $font = New-Object System.Drawing.Font "Segoe UI", 14
    $g.DrawString("$n", $font, [System.Drawing.Brushes]::White, 8, 8)
    $bmp.Save($thumb, [System.Drawing.Imaging.ImageFormat]::Jpeg)
    $g.Dispose(); $bmp.Dispose()

    $entries += [ordered]@{
        src = $src
        dst = $dst
        res = "1920x1080"
        fps = 30
        crf = 23
        encoder = "libx264"
        mtime = 0
        size = 123456
        duration = 12.5
        codec = "h264"
        width = 1920
        height = 1080
        thumb = $thumb
        broken = $isMissing -or $isUnc
    }
}
$lib = [ordered]@{ version = 1; entries = $entries }
$json = $lib | ConvertTo-Json -Depth 5
# No BOM: the file is read by serde_json (Rust) and nlohmann (C++), and a BOM
# is a difference between the two writers waiting to happen.
$utf8 = New-Object System.Text.UTF8Encoding($false)
[System.IO.File]::WriteAllText((Join-Path $OutDir "library.json"), $json, $utf8)
Write-Host "wrote $Count entries to $OutDir"
$missingCount = @($entries | Where-Object { $_.src -like '*missing_*' }).Count
$uncCount = @($entries | Where-Object { $_.src -like '*192.0.2.1*' }).Count
$thumbCount = @(Get-ChildItem -Path $thumbDir -File).Count
Write-Host "missing: $missingCount  unc: $uncCount  thumbs: $thumbCount"

