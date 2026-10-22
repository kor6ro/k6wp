# tools/lint_env_overrides_selftest.ps1 - the self-test of the environment
# override lint (task L5).
#
# WHY
#   A lint that cannot fire is worse than no lint: it reports "OK" while the
#   defect it exists to catch is back. This builds a scratch directory with
#   known-bad and known-good sources and asserts the lint's verdict on each.
#
# WHAT IT ASSERTS
#   1. an UNGATED C++ read of a K6WP_* name      -> the lint FAILS
#   2. an ungated Rust read                        -> the lint FAILS
#   3. a C++ read behind `#if K6WP_ALLOW_ENV_OVERRIDES` -> the lint PASSES
#   4. a C++ read behind `#ifdef _DEBUG`          -> the lint PASSES
#   5. a Rust read behind `#[cfg(debug_assertions)]`     -> the lint PASSES
#   6. a read of a NON-K6WP name (LOCALAPPDATA)    -> the lint PASSES
#   7. a documented override (K6WP_CACHE_DIR)      -> the lint PASSES and
#      NAMES it (the classification stays visible)
#
[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$lint = Join-Path $root 'tools\lint_env_overrides.ps1'
$scratch = Join-Path $env:TEMP ("k6wp_env_lint_" + [Guid]::NewGuid().ToString('N').Substring(0, 8))

function New-ScratchFile([string]$name, [string]$content) {
    $path = Join-Path $scratch $name
    [IO.File]::WriteAllText($path, $content, (New-Object Text.UTF8Encoding($false)))
}

function Invoke-Lint([string]$path) {
    $old = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try {
        $output = & powershell -NoProfile -ExecutionPolicy Bypass -File $lint -ScanPath $path 2>&1
        $code = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $old
    }
    return [pscustomobject]@{ Code = $code; Output = ($output | Out-String) }
}

$checks = 0
$failures = 0
function Check([bool]$ok, [string]$what) {
    $script:checks++
    if ($ok) {
        Write-Host "  [OK] $what"
    } else {
        $script:failures++
        Write-Host "  [FAIL] $what"
    }
}

New-Item -ItemType Directory -Force -Path $scratch | Out-Null
try {
    # 1. an ungated C++ read -> the lint must fail
    New-ScratchFile 'ungated_cpp.cpp' @'
#include <cstdlib>
int Mode() {
  const char* mode = std::getenv("K6WP_ENGINE_PIPE");
  return mode == nullptr ? 0 : 1;
}
'@
    $r = Invoke-Lint (Join-Path $scratch 'ungated_cpp.cpp')
    Check ($r.Code -ne 0) 'an ungated C++ environment read fails the lint'

    # 2. an ungated Rust read -> the lint must fail
    New-ScratchFile 'ungated_rs.rs' @'
pub fn pipe() -> Option<String> {
    std::env::var("K6WP_ENGINE_MUTEX").ok()
}
'@
    $r = Invoke-Lint (Join-Path $scratch 'ungated_rs.rs')
    Check ($r.Code -ne 0) 'an ungated Rust environment read fails the lint'

    # 3. a C++ read behind the release gate -> the lint must pass
    New-ScratchFile 'gated_cpp.cpp' @'
#include "build_flags.hpp"
int Mode() {
#if K6WP_ALLOW_ENV_OVERRIDES
  const char* mode = std::getenv("K6WP_ENGINE_PIPE");
  return mode == nullptr ? 0 : 1;
#else
  return 0;
#endif
}
'@
    $r = Invoke-Lint (Join-Path $scratch 'gated_cpp.cpp')
    Check ($r.Code -eq 0) 'a C++ read behind K6WP_ALLOW_ENV_OVERRIDES passes the lint'

    # 4. a C++ read behind #ifdef _DEBUG -> the lint must pass
    New-ScratchFile 'debug_cpp.cpp' @'
int Mode() {
#ifdef _DEBUG
  const char* mode = std::getenv("K6WP_DEV");
  return mode == nullptr ? 0 : 1;
#else
  return 0;
#endif
}
'@
    $r = Invoke-Lint (Join-Path $scratch 'debug_cpp.cpp')
    Check ($r.Code -eq 0) 'a C++ read behind #ifdef _DEBUG passes the lint'

    # 5. a Rust read behind #[cfg(debug_assertions)] -> the lint must pass
    New-ScratchFile 'gated_rs.rs' @'
pub fn pipe() -> Option<String> {
    #[cfg(debug_assertions)]
    {
        if let Ok(name) = std::env::var("K6WP_ENGINE_MUTEX") {
            return Some(name);
        }
    }
    None
}
'@
    $r = Invoke-Lint (Join-Path $scratch 'gated_rs.rs')
    Check ($r.Code -eq 0) 'a Rust read behind #[cfg(debug_assertions)] passes the lint'

    # 6. a non-K6WP name is not an override -> the lint must pass
    New-ScratchFile 'ordinary.cpp' @'
int Mode() {
  const char* local = std::getenv("LOCALAPPDATA");
  return local == nullptr ? 0 : 1;
}
'@
    $r = Invoke-Lint (Join-Path $scratch 'ordinary.cpp')
    Check ($r.Code -eq 0) 'a read of an ordinary variable is not an override'

    # 7. a documented override passes and is NAMED
    New-ScratchFile 'documented.cpp' @'
int Cache() {
  const char* dir = std::getenv("K6WP_CACHE_DIR");
  return dir == nullptr ? 0 : 1;
}
'@
    $r = Invoke-Lint (Join-Path $scratch 'documented.cpp')
    Check ($r.Code -eq 0) 'a documented override passes the lint'
    Check ($r.Output -match 'K6WP_CACHE_DIR') 'the documented override is named in the output'
} finally {
    Remove-Item -LiteralPath $scratch -Recurse -Force -ErrorAction SilentlyContinue
}

Write-Host "lint_env_overrides_selftest: $checks checks, $failures failure(s)"
if ($failures -gt 0) { exit 1 }
exit 0
