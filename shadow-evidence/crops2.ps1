Add-Type -AssemblyName System.Drawing
$src = [System.Drawing.Image]::FromFile("C:\Users\axioo\Desktop\k6wp\shadow-evidence\popup-1-initial.png")
function Crop([int]$x, [int]$y, [int]$cw, [int]$ch, [int]$scale, [string]$name) {
  $bmp = New-Object System.Drawing.Bitmap ($cw * $scale), ($ch * $scale)
  $g = [System.Drawing.Graphics]::FromImage($bmp)
  $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::NearestNeighbor
  $destRect = New-Object System.Drawing.Rectangle 0, 0, ($cw * $scale), ($ch * $scale)
  $srcRect = New-Object System.Drawing.Rectangle $x, $y, $cw, $ch
  $g.DrawImage($src, $destRect, $srcRect, [System.Drawing.GraphicsUnit]::Pixel)
  $bmp.Save("C:\Users\axioo\Desktop\k6wp\shadow-evidence\crop-$name.png", [System.Drawing.Imaging.ImageFormat]::Png)
  $g.Dispose(); $bmp.Dispose()
  Write-Output "crop $name"
}
Crop 8 32 62 150 4 "sidebar"
Crop 40 40 640 100 2 "cards"
Crop 890 130 1010 60 2 "aktifkan-row"
Crop 890 180 1010 45 3 "actionbar"
$src.Dispose()
Write-Output "done"
