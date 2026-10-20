Add-Type -AssemblyName System.Drawing
$src = [System.Drawing.Image]::FromFile("C:\Users\axioo\Desktop\k6wp\shadow-evidence\popup-2-assignpopup.png")

# 3x zoom of the assign popup area
$cw = 300; $ch = 200; $scale = 3
$bmp = New-Object System.Drawing.Bitmap ($cw * $scale), ($ch * $scale)
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::NearestNeighbor
$destRect = New-Object System.Drawing.Rectangle 0, 0, ($cw * $scale), ($ch * $scale)
$srcRect = New-Object System.Drawing.Rectangle 55, 570, $cw, $ch
$g.DrawImage($src, $destRect, $srcRect, [System.Drawing.GraphicsUnit]::Pixel)
$bmp.Save("C:\Users\axioo\Desktop\k6wp\shadow-evidence\zoom-assignpopup.png", [System.Drawing.Imaging.ImageFormat]::Png)
$g.Dispose(); $bmp.Dispose()

# Pixel sampling: scan a horizontal line through the "Semua layar" radio label (~y=600) and print unique colors with counts
function SampleLine($img, $y, $x0, $x1) {
  $counts = @{}
  for ($x = $x0; $x -le $x1; $x++) {
    $c = $img.GetPixel($x, $y)
    $key = "{0},{1},{2}" -f $c.R, $c.G, $c.B
    if ($counts.ContainsKey($key)) { $counts[$key]++ } else { $counts[$key] = 1 }
  }
  $counts.GetEnumerator() | Sort-Object Value -Descending | Select-Object -First 6 | ForEach-Object { "y=$y : $($_.Key) x$($_.Value)" }
}
Write-Output "=== line through title 'Pasang ke' (y=596) ==="
SampleLine $src 596 70 145
Write-Output "=== line through 'Semua layar' label (y=607) ==="
SampleLine $src 607 108 200
Write-Output "=== line through 'Layar 0 (utama)' (y=666) ==="
SampleLine $src 666 108 230
Write-Output "=== line through 'Layar 1' (y=717) ==="
SampleLine $src 717 108 180
$src.Dispose()
Write-Output "done"
