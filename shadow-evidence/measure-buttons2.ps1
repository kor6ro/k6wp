Add-Type -AssemblyName System.Drawing
$img = [System.Drawing.Bitmap]::FromFile("C:\Users\axioo\Desktop\k6wp\shadow-evidence\popup-1-initial.png")
function ColorKey($c) { "{0},{1},{2}" -f $c.R, $c.G, $c.B }

Write-Output "=== vertical x=1000, y=140..250 ==="
$prev = ""
for ($y = 140; $y -le 250; $y++) {
  $k = ColorKey $img.GetPixel(1000, $y)
  if ($k -ne $prev) { Write-Output ("y={0}: {1}" -f $y, $k); $prev = $k }
}

Write-Output "=== horizontal y=210, x=890..1910 (border scan) ==="
$prev = ""
$runs = 0
for ($x = 890; $x -le 1910; $x++) {
  $k = ColorKey $img.GetPixel($x, 210)
  if ($k -ne $prev) { Write-Output ("x={0}: {1}" -f $x, $k); $prev = $k; $runs++ ; if ($runs -gt 60) { Write-Output "..." ; break } }
}
$img.Dispose()
Write-Output "done"
