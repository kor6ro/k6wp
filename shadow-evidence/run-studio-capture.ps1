$env:QT_ASSUME_STDERR_HAS_CONSOLE = "1"
$env:QML_IMPORT_TRACE = "0"
$env:QT_LOGGING_RULES = "qt.qml.*=true"
$p = Start-Process -FilePath "build\msvc-dev\studio.exe" -RedirectStandardError "shadow-evidence\studio-err2.txt" -RedirectStandardOutput "shadow-evidence\studio-out2.txt" -PassThru
Start-Sleep -Seconds 6
if (!$p.HasExited) { Stop-Process -Id $p.Id -Force; Write-Output "killed" } else { Write-Output "exited code $($p.ExitCode)" }
Write-Output "--- stderr ---"
Get-Content "shadow-evidence\studio-err2.txt"
Write-Output "--- stdout ---"
Get-Content "shadow-evidence\studio-out2.txt"
