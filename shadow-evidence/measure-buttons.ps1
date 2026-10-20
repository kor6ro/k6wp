Add-Type -AssemblyName System.Drawing
$img = [System.Drawing.Bitmap]::FromFile("C:\Users\axioo\Desktop\k6wp\shadow-evidence\popup-1-initial.png")

function ColorKey($c) { "{0},{1},{2}" -f $c.R, $c.G, $c.B }

# 1) find button edges along the row center y=188
Write-Output "=== horizontal transitions y=188 (button row) ==="
$prev = ""
for ($x = 895; $x -le 1905; $x++) {
  $k = ColorKey $img.GetPixel($x, 188)
  if ($k -ne $prev) { Write-Output ("x={0}: {1}" -f $x, $k); $prev = $k }
}

# 2) vertical transitions of first button at x=1000
Write-Output "=== vertical transitions x=1000 (first button) ==="
$prev = ""
for ($y = 160; $y -le 215; $y++) {
  $k = ColorKey $img.GetPixel(1000, $y)
  if ($k -ne $prev) { Write-Output ("y={0}: {1}" -f $y, $k); $prev = $k }
}

$img.Dispose()
Write-Output "done"
