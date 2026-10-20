Add-Type -AssemblyName System.Drawing
$img = [System.Drawing.Bitmap]::FromFile("C:\Users\axioo\Desktop\k6wp\shadow-evidence\popup-1-initial.png")
function ColorKey($c) { "{0},{1},{2}" -f $c.R, $c.G, $c.B }

function ScanLine([int]$y, [int]$x0 = 0, [int]$x1 = 1935) {
  Write-Output "=== y=$y ==="
  $prev = ""
  $runs = 0
  for ($x = $x0; $x -le $x1; $x++) {
    $k = ColorKey $img.GetPixel($x, $y)
    if ($k -ne $prev) {
      Write-Output ("x={0}: {1}" -f $x, $k)
      $prev = $k
      $runs++
      if ($runs -gt 45) { Write-Output "  ...(truncated)"; break }
    }
  }
}

# empty zone: preview area (left col) vs rail pane background
ScanLine 300
# combo row
ScanLine 480
# gallery area
ScanLine 950
$img.Dispose()
Write-Output "done"
