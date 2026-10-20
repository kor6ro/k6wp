Add-Type -AssemblyName System.Drawing
$img = [System.Drawing.Bitmap]::FromFile("C:\Users\axioo\Desktop\k6wp\shadow-evidence\popup-1-initial.png")
$border = @(58, 64, 78)
$W = $img.Width

function IsBorder($c) {
  return ([math]::Abs($c.R - 58) -le 6) -and ([math]::Abs($c.G - 64) -le 6) -and ([math]::Abs($c.B - 78) -le 6)
}

# find the 4 button x-edges along y=203 (interior row)
$y = 203
$edges = @()
for ($x = 900; $x -le 1910; $x++) {
  $c = $img.GetPixel($x, $y)
  if (IsBorder $c) { $edges += $x }
}
Write-Output ("border x positions: " + ($edges -join ", "))

# group edges into button spans
$buttons = @()
$i = 0
while ($i -lt $edges.Count) {
  $left = $edges[$i]
  # find matching right edge of same button: next edge at least 50px away
  $j = $i + 1
  while ($j -lt $edges.Count -and ($edges[$j] - $left) -lt 50) { $j++ }
  if ($j -lt $edges.Count) {
    $buttons += , @($left, $edges[$j])
    $i = $j + 1
  } else { break }
}
if ($buttons.Count -eq 0) { Write-Output "no button pairs found"; $img.Dispose(); exit }

# find vertical extent (top/bottom borders) at the middle of the first button
$mx = [math]::Floor(($buttons[0][0] + $buttons[0][1]) / 2)
$top = $null; $bottom = $null
for ($yy = 150; $yy -le 260; $yy++) {
  $c = $img.GetPixel($mx, $yy)
  if (IsBorder $c) {
    if ($null -eq $top) { $top = $yy; $bottom = $yy } else { $bottom = $yy }
  }
}
Write-Output "button vertical: top=$top bottom=$bottom"

foreach ($b in $buttons) {
  $x0 = $b[0] + 2; $x1 = $b[1] - 2
  $minX = 99999; $maxX = -1; $minY = 99999; $maxY = -1
  for ($xx = $x0; $xx -le $x1; $xx++) {
    for ($yy = $top + 2; $yy -le $bottom - 2; $yy++) {
      $c = $img.GetPixel($xx, $yy)
      if ($c.R -gt 90 -or $c.G -gt 90 -or $c.B -gt 90) {
        if ($xx -lt $minX) { $minX = $xx }
        if ($xx -gt $maxX) { $maxX = $xx }
        if ($yy -lt $minY) { $minY = $yy }
        if ($yy -gt $maxY) { $maxY = $yy }
      }
    }
  }
  $bx = ($x0 + $x1) / 2.0; $by = ($top + $bottom) / 2.0
  $cx = ($minX + $maxX) / 2.0; $cy = ($minY + $maxY) / 2.0
  Write-Output ("button x={0}..{1} (center {2}) content x={3}..{4} (center {5}) => dx={6}" -f $x0, $x1, $bx, $minX, $maxX, $cx, ($cx - $bx))
  Write-Output ("         y={0}..{1} (center {2}) content y={3}..{4} (center {5}) => dy={6}" -f ($top + 2), ($bottom - 2), $by, $minY, $maxY, $cy, ($cy - $by))
}
$img.Dispose()
Write-Output "done"
