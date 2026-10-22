# tools/lint_env_overrides.ps1 - every K6WP_* environment read must sit
# behind a build gate (task L5).
#
# WHY
#   tools/check_release_overrides.ps1 proves that a RELEASE BINARY does not
#   contain the override names. That proof only covers the binaries a
#   release build produces. This lint proves the RULE in the source, where
#   it can be checked for every language (C++ and Rust) and for every file -
#   including the crates and tools that no release build produces yet, and
#   including a new override that someone adds tomorrow.
#
# THE RULE
#   A `K6WP_*` name that is READ from the environment must be behind one of
#   the build gates:
#     - the C++: `#if K6WP_ALLOW_ENV_OVERRIDES` or `#ifdef _DEBUG`
#     - the Rust: `#[cfg(debug_assertions)]`, `#[cfg(test)]` or `#[test]`
#   A file under `tests/` is exempt as a whole: it never ships.
#   A name that is only MENTIONED (a comment, a doc line, a string constant
#   that is not an environment read) is fine - only an actual read counts.
#
# WHY IT MATTERS
#   A shipped binary must not let an environment variable redirect the IPC
#   endpoint, the singleton mutex, the library path or the parser's
#   behaviour: whoever can set an environment variable for the process could
#   then point the app at a pipe it controls. The gate makes the code
#   disappear from the release build; the binary scan proves it.
#
# THE CONSISTENCY CHECK
#   Every name in the checker's forbidden list must be read somewhere in the
#   tree - otherwise the list has drifted and the binary scan would pass
#   vacuously for that name.
#
[CmdletBinding()]
param(
    # Fail when the tree holds no gated read at all (a lint that matches
    # nothing is a lint that cannot fire).
    [switch]$RequireMatches,
    # Scan this directory instead of the tracked files (the self-test uses
    # it with a scratch file). The consistency check only runs on the tree.
    [string]$ScanPath = ''
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot

# The environment reads: the C++ and the Rust spellings, including the
# wrappers the tree uses (`GetEnvStr` in the compressor, `qEnvironmentVariable`
# in the Studio).
# NOTE: the capture group of the NAME must be group 1 in every pattern, so
# the optional part is a NON-capturing group.
$readPatterns = @(
    'GetEnvironmentVariable[AW]\(\s*L?"(K6WP_[A-Z0-9_]+)"',
    '\bvar(?:_os)?\(\s*"(K6WP_[A-Z0-9_]+)"',
    '\bgetenv\(\s*"(K6WP_[A-Z0-9_]+)"',
    '\bGetEnvStr\(\s*"(K6WP_[A-Z0-9_]+)"',
    'qEnvironmentVariable\(\s*"(K6WP_[A-Z0-9_]+)"'
)
# The gates that make a read disappear from a release build.
$cxxGates = @('#if K6WP_ALLOW_ENV_OVERRIDES', '#ifdef _DEBUG', '#if defined(_DEBUG)')
$rustGates = @('#[cfg(debug_assertions)]', '#[cfg(test)]', '#[test]')

# The overrides that are NOT diagnostic and therefore stay in a release
# build. Each one is a deliberate, documented decision - the reason is part
# of the rule, so a new override cannot slip in unclassified.
$allowed = @(
    @{ name = 'K6WP_RECONCILE_MODE'
       reason = 'a documented feature switch (the shadow reconcile mode, CHANGELOG alpha.3): it changes a BEHAVIOUR, it redirects nothing' },
    @{ name = 'K6WP_CACHE_DIR'
       reason = 'the Studio tells the compressor where to put its cache (studio/src/compress_service.cpp): an internal channel, not a diagnostic override' },
    @{ name = 'K6WP_CACHE_MAX_BYTES'
       reason = 'the compressor cache cap: a limit, not a redirection' },
    @{ name = 'K6WP_PLAYLIST_JSON'
       reason = 'the Studio playlist path; the release test suite relies on it (tests/playlist_bridge_test.cpp), so it is NOT compiled out' },
    @{ name = 'K6WP_THUMBS_DIR'
       reason = 'the thumbnailer scratch root for the tests; the release test suite relies on it (tests/studio_logic_test.cpp, tests/thumbnailer_test.cpp)' }
)
$allowedNames = @{}
foreach ($entry in $allowed) { $allowedNames[$entry.name] = $entry.reason }

$files = @(git -C $root ls-files -- '*.cpp' '*.hpp' '*.cc' '*.h' '*.rs')
if ($ScanPath) {
    $files = @(Get-ChildItem -LiteralPath $ScanPath -Recurse -File |
               Where-Object { $_.Extension -in @('.cpp', '.hpp', '.cc', '.h', '.rs') } |
               ForEach-Object { $_.FullName })
}
if ($files.Count -eq 0) {
    Write-Error 'no source file found (git ls-files returned nothing)'
    exit 2
}

# The names the binary scan forbids (the same list as
# tools/check_release_overrides.ps1).
$forbidden = @('K6WP_ENGINE_PIPE', 'K6WP_ENGINE_MUTEX', 'K6WP_LIBRARY_JSON',
               'K6WP_TEST_NO_CONFIG_LOCK', 'K6WP_IPC_TRACE', 'K6WP_DEV',
               'K6WP_SPS_DEBUG')
$seen = @{}

$errors = 0
$readCount = 0
foreach ($relative in $files) {
    $path = if ([IO.Path]::IsPathRooted($relative)) { $relative } else { Join-Path $root $relative }
    if (-not (Test-Path -LiteralPath $path)) { continue }
    $lines = [IO.File]::ReadAllLines($path, [Text.Encoding]::UTF8)
    # A test file never ships: the whole file is exempt.
    $isTestFile = ($relative -match '(^|[\\/])tests?[\\/]') -or
                  ($relative -match '_test\.(cpp|hpp|cc|rs)$') -or
                  ($relative -match '(^|[\\/])fuzz[\\/]')
    for ($i = 0; $i -lt $lines.Count; $i++) {
        $line = $lines[$i]
        foreach ($pattern in $readPatterns) {
            $m = [regex]::Match($line, $pattern)
            if (-not $m.Success) { continue }
            $name = $m.Groups[1].Value
            if ($name -notmatch '^K6WP_') { continue }
            $readCount++
            $seen[$name] = $true
            if ($isTestFile) { continue }
            # A documented, non-diagnostic override: allowed, and reported so
            # the classification stays visible in the CI log.
            if ($allowedNames.ContainsKey($name)) {
                Write-Host ("  [note] {0}:{1}: {2} stays in a release build - {3}" -f
                            $relative, ($i + 1), $name, $allowedNames[$name])
                continue
            }
            # The gate must enclose the read. For the C++ that is the
            # preprocessor conditional the read sits in; for the Rust it is
            # the attribute just above it.
            $gated = $false
            $depth = 0
            for ($j = $i - 1; $j -ge [Math]::Max(0, $i - 40); $j--) {
                $above = $lines[$j]
                if ($above -match '^\s*#\s*endif') { $depth++; continue }
                if ($above -match '^\s*#\s*(if|ifdef|ifndef)\b') {
                    if ($depth -eq 0) {
                        $condition = $above
                        if ($j + 1 -lt $i) { $condition = $condition + ' ' + $lines[$j + 1] }
                        foreach ($marker in $cxxGates) {
                            if ($condition.Contains($marker)) { $gated = $true }
                        }
                        break
                    }
                    $depth--
                    continue
                }
                foreach ($marker in $rustGates) {
                    if ($above.Contains($marker)) { $gated = $true }
                }
            }
            if (-not $gated) {
                $errors++
                Write-Host ("  [FAIL] {0}:{1}: the environment read of {2} is not behind a build gate" -f
                            $relative, ($i + 1), $name)
                Write-Host ("         (the C++: {0} - the Rust: {1})" -f ($cxxGates -join ', '), ($rustGates -join ', '))
            }
        }
    }
}

# The consistency check: every forbidden name must be read somewhere (only
# on the tree scan - a scratch directory has no such names).
if (-not $ScanPath) {
    foreach ($name in $forbidden) {
        if (-not $seen.ContainsKey($name)) {
            $errors++
            Write-Host ("  [FAIL] the forbidden name {0} is read nowhere in the tree: the checker's list has drifted" -f $name)
        }
    }
}

if ($readCount -eq 0 -and -not $ScanPath) {
    # On the tree scan there must be reads; on a scratch scan (the
    # self-test) a file without any override is simply clean.
    Write-Error 'no environment read matched: the lint cannot fire'
    exit 2
}

if ($errors -gt 0) {
    Write-Error "lint_env_overrides: $errors error(s)"
    exit 1
}
Write-Host ("lint_env_overrides: OK ({0} files, {1} environment reads, {2} forbidden names, {3} distinct names read)" -f
            $files.Count, $readCount, $forbidden.Count, $seen.Count)
exit 0
