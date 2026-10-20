$srcDir = "C:\Users\axioo\Desktop\k6wp\studio\qml"
$stageDirs = @(
  "C:\Users\axioo\Desktop\k6wp\build\msvc-dev\studio\K6WP",
  "C:\Users\axioo\Desktop\k6wp\build\release\studio\K6WP"
)
foreach ($sd in $stageDirs) {
  Write-Output "=== STAGE: $sd ==="
  if (-not (Test-Path $sd)) { Write-Output "  (missing)"; continue }
  $srcFiles = Get-ChildItem "$srcDir\*.qml"
  foreach ($f in $srcFiles) {
    $staged = Join-Path $sd $f.Name
    if (-not (Test-Path $staged)) { Write-Output ("  MISSING  : {0}" -f $f.Name); continue }
    $h1 = (Get-FileHash $f.FullName -Algorithm MD5).Hash
    $h2 = (Get-FileHash $staged -Algorithm MD5).Hash
    if ($h1 -ne $h2) {
      $t = (Get-Item $staged).LastWriteTime
      Write-Output ("  DIFFER  : {0}   (staged mtime {1})" -f $f.Name, $t)
    }
  }
  Write-Output "  (only DIFFER/MISSING rows listed)"
}
Write-Output "done"
