# tools/install_hooks.ps1 - point git at the repository's shared hooks.
#
# WHY
#   .git/hooks is not tracked, so a hook that lives there exists on one
#   machine only. The tracked directory is .githooks/ and this script sets
#   core.hooksPath to it. Run it once after cloning (CONTRIBUTING.md says so).
#
# WHAT IT INSTALLS
#   pre-commit  -> tools/lint_nonlatin.ps1 -Staged  (task H4)
#
# The script is safe to re-run and does nothing if the path is already right.
#
[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
Set-Location -LiteralPath $root

if (-not (Test-Path -LiteralPath '.githooks/pre-commit')) {
    Write-Error '.githooks/pre-commit is missing'
    exit 1
}

# Git for Windows runs the hook through sh; a CRLF line ending there yields
# "$'\r': command not found" and the hook silently stops guarding.
$hook = Get-Content -LiteralPath '.githooks/pre-commit' -Raw
if ($hook -match "`r`n") {
    $hook = $hook -replace "`r`n", "`n"
    [IO.File]::WriteAllText((Resolve-Path '.githooks/pre-commit'), $hook, (New-Object Text.UTF8Encoding($false)))
    Write-Host '  [hooks] normalized .githooks/pre-commit to LF'
}

$current = & git config --get core.hooksPath
if ($current -eq '.githooks') {
    Write-Host '  [hooks] core.hooksPath is already .githooks'
    exit 0
}

& git config core.hooksPath '.githooks'
if ($LASTEXITCODE -ne 0) { Write-Error 'git config core.hooksPath failed'; exit 1 }
Write-Host '  [hooks] core.hooksPath = .githooks (pre-commit lint installed)'
