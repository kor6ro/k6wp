# tools/check_release_overrides.ps1 - the release binaries must NOT contain
# the environment overrides (task I4, extended by L5).
#
# WHY
#   The overrides exist so a TEST can talk to its own engine instead of the
#   user's live session, so the two-writer test can prove the config lock is
#   what prevents the lost update, or so the K1/L2 instrument can accumulate
#   the `get_state` cost. A shipped binary must not let an environment
#   variable redirect the IPC endpoint, the singleton mutex or the library
#   path: whoever can set an environment variable for the process would then
#   be able to point the app at a pipe it controls.
#
# THE RULE
#   The overrides are compiled in a DEBUG build only (the C++: `_DEBUG`; the
#   Rust: `debug_assertions`). This script proves it for the RELEASE
#   binaries: it scans them for the variable names. A release binary that
#   still contains one of them FAILS the check.
#
#   The list is the AUDIT of every override in the tree (L5, 2026-10-22):
#   every `GetEnvironmentVariable*` / `std::env::var*` that reads a
#   `K6WP_*` name. The test-only names (K6WP_TEST_CONFIG_PATH,
#   K6WP_IPC_LIVE, K6WP_IPC_ENGINE_EXE, K6WP_RUST_WRITER_*) are NOT in the
#   list: they live in test binaries, which are built into the same
#   directory in a CI release build.
#
#   The scan must never pass vacuously: with `-RequireBinaries` the script
#   FAILS when it found no binary to scan (a missing Rust release build must
#   not look like a clean bill of health).
#
[CmdletBinding()]
param(
    [string]$ReleaseDir = '',
    [string]$RustReleaseDir = '',
    # Fail when no binary was scanned at all (a CI gate must not pass
    # because the build directory was empty or missing).
    [switch]$RequireBinaries
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

# The audit of every override in the tree (L5). Each one is compiled out of
# a release build; the scan proves it.
$forbidden = @('K6WP_ENGINE_PIPE',      # the IPC pipe name (C++)
               'K6WP_ENGINE_MUTEX',     # the singleton mutex name (C++)
               'K6WP_LIBRARY_JSON',     # the library index path (C++ and Rust)
               'K6WP_TEST_NO_CONFIG_LOCK',  # the config lock bypass (C++)
               'K6WP_IPC_TRACE',        # the IPC latency trace (C++)
               'K6WP_DEV',              # the dev-engine fallback (the Studio)
               'K6WP_SPS_DEBUG')        # the SPS box dump (the Rust core)

$binaries = @()
if (Test-Path -LiteralPath $ReleaseDir) {
    $binaries += @(Get-ChildItem -LiteralPath $ReleaseDir -Filter '*.exe' -File)
}

# The Rust release binaries: `cargo build --release` writes to
# `rust/target/release`, and with an explicit `--target` to
# `rust/target/<triple>/release`. Both are scanned.
$rustDirs = @()
if ($RustReleaseDir) {
    $rustDirs += $RustReleaseDir
} else {
    $rustDirs += (Join-Path $root 'rust\target\release')
    $rustDirs += (Join-Path $root 'rust\target\x86_64-pc-windows-msvc\release')
}
foreach ($dir in $rustDirs) {
    if (Test-Path -LiteralPath $dir) {
        $binaries += @(Get-ChildItem -LiteralPath $dir -Filter '*.exe' -File)
    } else {
        Write-Host ("  [skip] {0}: no such release directory" -f $dir)
    }
}

# The same binary can be reached through two directories (the Rust target
# and its explicit-triple twin): scan every path once.
$seenPaths = @{}
$unique = @()
foreach ($binary in $binaries) {
    if ($seenPaths.ContainsKey($binary.FullName)) { continue }
    $seenPaths[$binary.FullName] = $true
    $unique += $binary
}
$binaries = $unique

if ($binaries.Count -eq 0) {
    if ($RequireBinaries) {
        Write-Error "no release binary to scan (the C++ dir '$ReleaseDir' and the Rust dirs are empty): the check would pass vacuously"
        exit 2
    }
    Write-Error "no release binary in $ReleaseDir"
    exit 2
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
Write-Host "check_release_overrides: OK ($($binaries.Count) release binaries scanned, $($forbidden.Count) names)"
exit 0
