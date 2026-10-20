param(
  [string]$Png = "C:\Users\axioo\Desktop\k6wp\shadow-evidence\popup-1-initial.png",
  [int]$Y0 = 196,
  [int]$Y1 = 214,
  [int]$X0 = 900,
  [int]$X1 = 1900,
  [int]$Gap = 14
)
Add-Type -AssemblyName System.Drawing
$img = [System.Drawing.Bitmap]::FromFile($Png)
$W = $img.Width; $H = $img.Height

# cluster text pixels (bright) along the band
$cols = @{}
for ($y = $Y0; $y -le $Y1 -and $y -lt $H; $y++) {
  for ($x = $X0; $x -le $X1 -and $x -lt $W; $x++) {
    $c = $img.GetPixel($x, $y)
    if ($c.R -gt 150 -and $c.G -gt 150) { $cols[$x] = $true }
  }
}
$xs = $cols.Keys | Sort-Object
$clusters = @()
$curMin = -1; $curMax = -1
foreach ($x in $xs) {
  if ($curMin -lt 0) { $curMin = $x; $curMax = $x; continue }
  if ($x - $curMax -le $Gap) { $curMax = $x } else { $clusters += ,@($curMin, $curMax); $curMin = $x; $curMax = $x }
}
if ($curMin -ge 0) { $clusters += ,@($curMin, $curMax) }
foreach ($cl in $clusters) {
  Write-Output ("cluster x={0}..{1} w={2}" -f $cl[0], $cl[1], ($cl[1] - $cl[0] + 1))
}
$img.Dispose()
