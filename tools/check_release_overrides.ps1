# tools/check_release_overrides.ps1 - the release binaries must NOT contain
# the environment overrides (task I4).
#
# WHY
#   The overrides (K6WP_ENGINE_PIPE, K6WP_ENGINE_MUTEX, K6WP_LIBRARY_JSON,
#   K6WP_TEST_NO_CONFIG_LOCK) exist so a TEST can talk to its own engine
#   instead of the user's live session, or so the two-writer test can prove
#   the config lock is what prevents the lost update. A shipped binary must
#   not let an environment variable redirect the IPC endpoint, the singleton
#   mutex or the library path: whoever can set an environment variable for
#   the process would then be able to point the app at a pipe it controls.
#
# THE RULE
#   The overrides are compiled in a DEBUG build only (the C++: `_DEBUG`; the
#   Rust: `debug_assertions`). This script proves it for the RELEASE
#   binaries: it scans them for the three variable names. A release binary
#   that still contains one of them FAILS the check.
#
[CmdletBinding()]
param(
    [string]$ReleaseDir = '',
    [string]$RustReleaseDir = ''
)

function Contains-Bytes([byte[]]$haystack, [byte[]]$needle) {
    if ($needle.Length -eq 0 -or $haystack.Length -lt $needle.Length) {
        return $false
    }
    for ($i = 0; $i -le $haystack.Length - $needle.Length; $i++) {
        $match = $true
        for ($j = 0; $j -lt $needle.Length; $j++) {
            if ($haystack[$i + $j] -ne $needle[$j]) {
                $match = $false
                break
            }
        }
        if ($match) {
            return $true
        }
    }
    return $false
}

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
if (-not $ReleaseDir) {
    $ReleaseDir = Join-Path $root 'build\release'
}
if (-not (Test-Path $ReleaseDir)) {
    Write-Error "the release directory is missing: $ReleaseDir (build it first)"
    exit 2
}

$forbidden = @('K6WP_ENGINE_PIPE', 'K6WP_ENGINE_MUTEX', 'K6WP_LIBRARY_JSON',
               'K6WP_TEST_NO_CONFIG_LOCK')
if (-not $RustReleaseDir) {
    $RustReleaseDir = Join-Path $root 'rust\target\release'
}
$binaries = @(Get-ChildItem -Path $ReleaseDir -Filter '*.exe' -File)
if (Test-Path $RustReleaseDir) {
    # The Rust binaries (the Studio) carry the same rule: the override is
    # compiled out of a release build (`debug_assertions`).
    $binaries += @(Get-ChildItem -Path $RustReleaseDir -Filter '*.exe' -File)
}
if ($binaries.Count -eq 0) {
    Write-Error "no release binary in $ReleaseDir"
    exit 2
}

# The UTF-16LE form (the Windows wide strings) and the ASCII form.
$patterns = @()
foreach ($name in $forbidden) {
    $patterns += [Text.Encoding]::Unicode.GetBytes($name)
    $patterns += [Text.Encoding]::ASCII.GetBytes($name)
}

$failures = 0
foreach ($binary in $binaries) {
    $bytes = [IO.File]::ReadAllBytes($binary.FullName)
    $found = @()
    foreach ($name in $forbidden) {
        $wide = [Text.Encoding]::Unicode.GetBytes($name)
        $ascii = [Text.Encoding]::ASCII.GetBytes($name)
        if (Contains-Bytes $bytes $wide) { $found += "$name (UTF-16)" }
        elseif (Contains-Bytes $bytes $ascii) { $found += "$name (ASCII)" }
    }
    if ($found.Count -gt 0) {
        $failures++
        Write-Host ("  [FAIL] {0}: the release binary still contains {1}" -f $binary.Name, ($found -join ', '))
    } else {
        Write-Host ("  [OK] {0}: no environment override in the release binary" -f $binary.Name)
    }
}

if ($failures -gt 0) {
    Write-Error "$failures release binary/binaries still contain an environment override"
    exit 1
}
Write-Host "check_release_overrides: OK ($($binaries.Count) release binaries scanned)"
exit 0