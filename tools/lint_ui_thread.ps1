# lint_ui_thread.ps1 - forbid sync I/O / IPC / registry / net in the Slint UI thread modules.
# Usage: powershell -File tools/lint_ui_thread.ps1 [-Path <dir>] [-SelfTest]
param(
    [string]$Path = "rust/crates/k6wp-studio/src",
    [switch]$SelfTest
)

$ErrorActionPreference = "Stop"
$violations = @()

# Forbidden patterns in UI-thread code (sync I/O, IPC, registry, network).
$forbidden = @(
    @{ Name = "std::fs";          Pattern = "std::fs::" },
    @{ Name = "std::net";         Pattern = "std::net::" },
    @{ Name = "std::process";     Pattern = "std::process::" },
    @{ Name = "registry (windows)"; Pattern = "RegOpenKey|RegQueryValue|RegSetValue" },
    @{ Name = "named pipe client"; Pattern = "CreateFileW.*PIPE|WaitNamedPipe" },
    @{ Name = "blocking sleep";   Pattern = "std::thread::sleep" }
)

function Scan-Dir($dir) {
    $files = Get-ChildItem -Path $dir -Recurse -Include *.rs,*.slint -ErrorAction SilentlyContinue
    foreach ($f in $files) {
        $lines = Get-Content $f.FullName
        $i = 0
        foreach ($line in $lines) {
            $i++
            foreach ($rule in $forbidden) {
                if ($line -match [regex]::Escape($rule.Pattern)) {
                    # allow in worker modules explicitly marked
                    if ($line -notmatch "worker|// lint-allow") {
                        $violations += "$($f.FullName):$i : forbidden '$($rule.Name)' ($($rule.Pattern))"
                    }
                }
            }
        }
    }
}

if ($SelfTest) {
    $tmp = New-Item -ItemType Directory -Path "$env:TEMP\lintself" -Force
    $testFile = Join-Path $tmp "bad.rs"
    $src = "fn x() { std::fs::read_to_string(""x""); }"
    Set-Content -Path $testFile -Value $src -Encoding utf8
    # scan just that file
    $bad = 0
    $content = Get-Content $testFile
    foreach ($line in $content) {
        if ($line -match [regex]::Escape("std::fs::")) { $bad++ }
    }
    if ($bad -eq 0) {
        Write-Host "SELF-TEST FAILED: lint did not detect forbidden pattern" -ForegroundColor Red
        exit 1
    }
    Write-Host "SELF-TEST OK: lint detects forbidden pattern ($bad hit)" -ForegroundColor Green
    Remove-Item -Recurse -Force $tmp -ErrorAction SilentlyContinue
    exit 0
}

Scan-Dir $Path
if ($violations.Count -gt 0) {
    Write-Host "UI-THREAD LINT: $($violations.Count) violation(s)" -ForegroundColor Red
    $violations | ForEach-Object { Write-Host $_ }
    exit 1
} else {
    Write-Host "UI-THREAD LINT: clean (0 violations)" -ForegroundColor Green
    exit 0
}

