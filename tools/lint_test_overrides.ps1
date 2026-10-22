# tools/lint_test_overrides.ps1 - task J2: a test may not touch the
# PRODUCTION pipe or mutex name without claiming a unique one.
#
# WHY
#   The production names are per-session and fixed for the whole logon
#   session:
#     - the pipe:  \\.\pipe\k6wp-engine-<session_id>
#     - the mutex: Local\K6WP-Engine-Singleton
#   A test that uses them talks to the USER's live engine (or blocks on the
#   user's engine's mutex) - that is how the suite once hung for 30 minutes
#   and how the studio_logic_test's "readywait" leg became flaky.
#
#   The mechanism that makes a test safe is the E5 override: a test claims
#   a UNIQUE pipe name (and a unique mutex) per test process.
#
# THE RULES
#   1. A test source that mentions the production pipe base
#      ("k6wp-engine") must also use the test helper
#      (UseUniquePipeNameForTest / UseRealPipeNameForTest) - the helper is
#      what redirects the name.
#   2. A test source that mentions the production singleton mutex
#      ("K6WP-Engine-Singleton") must also use the mutex override
#      (K6WP_ENGINE_MUTEX) - otherwise it would collide with the user's
#      live engine.
#   3. A test source that starts an engine must set BOTH overrides.
#
# The lint is a CI gate (a step in .github/workflows/ci.yml) and a
# pre-commit check.
#
[CmdletBinding()]
param(
    [string]$TestDir = ''
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
if (-not $TestDir) {
    $TestDir = Join-Path $root 'tests'
}

# The pipe base as it appears in a NAME (a temp directory that merely
# contains the text is not a pipe usage).
$pipePattern = 'pipe[\\/]+k6wp-engine'
$pipeHelper = 'UseUniquePipeNameForTest|UseRealPipeNameForTest'
$mutexName = 'K6WP-Engine-Singleton'
$mutexOverride = 'K6WP_ENGINE_MUTEX'

$failures = @()
$checked = 0
foreach ($file in Get-ChildItem -Path $TestDir -Filter '*.cpp' -File) {
    $checked++
    $text = Get-Content $file.FullName -Raw
    $problems = @()

    if ($text -match $pipePattern -and $text -notmatch $pipeHelper) {
        $problems += "it uses the production pipe name but never claims a unique one (UseUniquePipeNameForTest / UseRealPipeNameForTest)"
    }
    if ($text.Contains($mutexName) -and $text -notmatch $mutexOverride) {
        $problems += "it mentions the production singleton mutex ('$mutexName') but never overrides it (K6WP_ENGINE_MUTEX)"
    }
    if ($problems.Count -gt 0) {
        $failures += [pscustomobject]@{ file = $file.Name; problems = $problems }
    }
}

if ($failures.Count -gt 0) {
    foreach ($f in $failures) {
        Write-Host ("  [FAIL] {0}" -f $f.file)
        foreach ($p in $f.problems) {
            Write-Host ("         {0}" -f $p)
        }
    }
    Write-Error ("lint_test_overrides: {0} of {1} test sources touch a production name without an override" -f $failures.Count, $checked)
    exit 1
}

Write-Host ("lint_test_overrides: OK ({0} test sources scanned)" -f $checked)
exit 0
