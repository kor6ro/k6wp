# shadow-evidence/get-state.ps1 — query the engine's get_state over the IPC pipe.
# Usage: powershell -File shadow-evidence\get-state.ps1 [timeoutMs]
# Prints the state JSON on one line, or "NO_ANSWER" when the engine did not
# answer within the timeout (the hang signal).
param([int]$TimeoutMs = 3000)
$ErrorActionPreference = 'Stop'

$sid = (Get-Process -Id $PID).SessionId
$pipe = "k6wp-engine-$sid"
$client = New-Object System.IO.Pipes.NamedPipeClientStream('.', $pipe, [System.IO.Pipes.PipeDirection]::InOut)
try {
    $client.Connect($TimeoutMs)
} catch {
    Write-Output "NO_ANSWER (connect failed: $($_.Exception.Message))"
    exit 1
}
$writer = New-Object System.IO.StreamWriter($client)
$writer.AutoFlush = $true
$reader = New-Object System.IO.StreamReader($client)
$writer.WriteLine('{"version":1,"cmd":"get_state","payload":{}}')

$task = $reader.ReadLineAsync()
if (-not $task.Wait($TimeoutMs)) {
    Write-Output "NO_ANSWER (timeout ${TimeoutMs}ms)"
    $client.Dispose()
    exit 1
}
$line = $task.Result
if ([string]::IsNullOrWhiteSpace($line)) { Write-Output "NO_ANSWER (empty)"; exit 1 }
Write-Output $line
$client.Dispose()
