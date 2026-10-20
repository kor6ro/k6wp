Add-Type -AssemblyName System.Drawing
$img = [System.Drawing.Bitmap]::FromFile("C:\Users\axioo\Desktop\k6wp\shadow-evidence\rail-diag.png")
$W = $img.Width; $H = $img.Height
Write-Output "image ${W}x${H}"

function FindBar([string]$name, [int]$tr, [int]$tg, [int]$tb) {
  $minX = 99999; $maxX = -1; $minY = 99999; $maxY = -1; $count = 0
  for ($y = 0; $y -lt $H; $y++) {
    for ($x = 0; $x -lt $W; $x++) {
      $c = $img.GetPixel($x, $y)
      $dr = [Math]::Abs($c.R - $tr); $dg = [Math]::Abs($c.G - $tg); $db = [Math]::Abs($c.B - $tb)
      if ($dr -lt 40 -and $dg -lt 40 -and $db -lt 40) {
        if ($x -lt $minX) { $minX = $x }; if ($x -gt $maxX) { $maxX = $x }
        if ($y -lt $minY) { $minY = $y }; if ($y -gt $maxY) { $maxY = $y }
        $count++
      }
    }
  }
  if ($count -gt 0) {
    Write-Output ("$name : x={0}..{1} (w={2}) y={3}..{4} (h={5}) pixels={6}" -f $minX, $maxX, ($maxX - $minX + 1), $minY, $maxY, ($maxY - $minY + 1), $count)
  } else {
    Write-Output "$name : NOT FOUND"
  }
}

FindBar "GREEN(left)" 0 255 0
FindBar "MAGENTA(rail)" 255 0 204
$img.Dispose()
Write-Output "done"
