# tools/g5_diff_baseline.ps1 - task I1: the G5 golden fixtures against the
# BASELINE build.
#
# WHAT IT PROVES
#   The G5 fixture (tests/golden/engine_logic.json) freezes the engine's pure
#   decisions. A fixture is only worth something if it describes the SAME
#   behaviour as the build it is compared against. This script builds the
#   harness (tests/engine_logic_dump.cpp + the case table) inside a worktree
#   at the `baseline-pre-refactor` tag and compares the two outputs case by
#   case:
#
#     - every module whose behaviour was NOT intentionally changed must be
#       IDENTICAL (the same inputs -> the same outputs);
#     - the modules that WERE intentionally changed (the task 1.1 unload
#       state machine, and the `hides_wallpaper` field that G5 extracted out
#       of EngineApp) are EXCLUDED and reported as such.
#
# WHY A WORKTREE
#   The baseline is a build of an OLDER tree. The harness is copied in (the
#   case table is the SAME file, so the two cannot drift), the missing
#   symbols come from tests/g5_baseline_shim.hpp, and the tree is patched
#   only inside the worktree (the repo is never touched).
#
# THE OUTPUT
#   docs/bench/g5-differential.json - the report (the counts, the excluded
#   modules, every difference).
#
[CmdletBinding()]
param(
    [string]$Worktree = '',
    [string]$Tag = 'baseline-pre-refactor'
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
if (-not $Worktree) {
    $Worktree = Join-Path $root 'build\wt-baseline'
}
$report = Join-Path $root 'docs\bench\g5-differential.json'
$baselineOut = Join-Path $root 'build\g5-baseline-output.json'
$currentFixture = Join-Path $root 'tests\golden\engine_logic.json'

# --- the worktree -----------------------------------------------------------
if (Test-Path (Join-Path $Worktree '.git')) {
    Write-Host "  [g5] the worktree exists: $Worktree"
} else {
    Write-Host "  [g5] creating the worktree at $Tag"
    git -C $root worktree add $Worktree $Tag
    if ($LASTEXITCODE -ne 0) { throw "the worktree could not be created" }
}
# The vendored dependencies (they are not in git).
if (-not (Test-Path (Join-Path $Worktree 'vendor'))) {
    robocopy (Join-Path $root 'vendor') (Join-Path $Worktree 'vendor') /E /NFL /NDL /NJH /NJS /NP | Out-Null
}
if (-not (Test-Path (Join-Path $Worktree 'shared\thirdparty'))) {
    robocopy (Join-Path $root 'shared\thirdparty') (Join-Path $Worktree 'shared\thirdparty') /E /NFL /NDL /NJH /NJS /NP | Out-Null
}

# --- copy the harness (the SAME case table) ---------------------------------
foreach ($file in @('engine_logic_cases.hpp', 'engine_logic_dump.cpp', 'g5_baseline_shim.hpp')) {
    Copy-Item (Join-Path $root "tests\$file") (Join-Path $Worktree "tests\$file") -Force
}

# The baseline has no unload_state.hpp: the shim provides the missing symbols.
$casesFile = Join-Path $Worktree 'tests\engine_logic_cases.hpp'
$cases = Get-Content $casesFile -Raw
if ($cases -notmatch 'g5_baseline_shim') {
    $cases = $cases.Replace('#include "unload_state.hpp"', '#include "g5_baseline_shim.hpp"')
    Set-Content -LiteralPath $casesFile -Value $cases -NoNewline
    Write-Host '  [g5] the case table points at the baseline shim'
}

# --- the build target (only inside the worktree) ----------------------------
$cmakeFile = Join-Path $Worktree 'engine\CMakeLists.txt'
$cmake = Get-Content $cmakeFile -Raw
if ($cmake -notmatch 'engine_logic_dump') {
    $target = @"

  # engine_logic_dump (task I1): the G5 harness, built against the BASELINE
  # so the differential test can compare the two behaviours. This target
  # exists only in this worktree (the repo's tree has the same target with
  # the real unload_state.hpp).
  add_executable(engine_logic_dump ../tests/engine_logic_dump.cpp
    src/desktop_placement.cpp
    src/retry_schedule.cpp
    src/crash_restart_guard.cpp
    src/topology_diff.cpp
    src/reconcile_policy.cpp
    src/pin_verify_schedule.cpp
  )
  target_link_libraries(engine_logic_dump PRIVATE user32 k6wp_shared)
  target_include_directories(engine_logic_dump PRIVATE
    `${CMAKE_CURRENT_SOURCE_DIR}/src
    `${CMAKE_SOURCE_DIR}/shared
    `${CMAKE_SOURCE_DIR}/tests
  )
"@
    $anchor = 'add_executable(engine_no_winrt_test'
    if (-not $cmake.Contains($anchor)) {
        # Older trees: anchor on the last target instead.
        $anchor = 'add_executable(engine_no_winrt_test'
    }
    if (-not $cmake.Contains($anchor)) { throw 'the CMake anchor was not found' }
    $cmake = $cmake.Replace($anchor, $target + "`r`n" + $anchor)
    Set-Content -LiteralPath $cmakeFile -Value $cmake -NoNewline
    Write-Host '  [g5] the worktree CMake got the engine_logic_dump target'
}

# --- build + run ------------------------------------------------------------
Push-Location $Worktree
try {
    cmake --preset msvc-dev 2>&1 | Out-Null
    if ($LASTEXITCODE -ne 0) { throw 'the baseline configure failed' }
    cmake --build --preset msvc-dev --target engine_logic_dump 2>&1 |
        Select-String 'error C|error LNK' | Select-Object -First 5
    if ($LASTEXITCODE -ne 0) { throw 'the baseline build failed' }
    # The dump writes its progress to stderr: with $ErrorActionPreference =
    # 'Stop' a native command that touches stderr raises a terminating error
    # in Windows PowerShell, so the process is started with its own
    # redirection instead of `2>$null`.
    $dumpExe = Join-Path $Worktree 'build\msvc-dev\engine_logic_dump.exe'
    $dumpErr = Join-Path $root 'build\g5-baseline-dump.err'
    $proc = Start-Process -FilePath $dumpExe -ArgumentList $baselineOut -Wait -PassThru -NoNewWindow -RedirectStandardError $dumpErr
    if ($proc.ExitCode -ne 0) { throw 'the baseline dump failed' }
} finally {
    Pop-Location
}

# --- the comparison ---------------------------------------------------------
$current = Get-Content $currentFixture -Raw -Encoding UTF8 | ConvertFrom-Json
$baseline = Get-Content $baselineOut -Raw -Encoding UTF8 | ConvertFrom-Json

# The modules that are NOT compared (and why).
$excluded = @{
    'unload_state' = 'the task 1.1 unload state machine: the baseline has no counterpart'
    'workerw_host' = 'task C7: the WorkerW host rule was inline in the baseline, not a pure function'
}
# The fields that are not compared, per module.
$excludedFields = @{
    'pause_controller' = @('hides_wallpaper')
}

$differences = @()
$compared = 0
$skipped = 0
foreach ($case in $current.cases) {
    $module = $case.module
    $baseCase = $baseline.cases | Where-Object { $_.module -eq $module -and $_.name -eq $case.name }
    if ($excluded.ContainsKey($module)) {
        $skipped++
        continue
    }
    if (-not $baseCase) {
        $differences += [pscustomobject]@{ module = $module; name = $case.name; field = '(the case itself)'; current = 'present'; baseline = 'missing' }
        continue
    }
    $skipFields = @()
    if ($excludedFields.ContainsKey($module)) { $skipFields = $excludedFields[$module] }
    foreach ($prop in $case.output.PSObject.Properties) {
        if ($skipFields -contains $prop.Name) { continue }
        $compared++
        $baseValue = $baseCase.output.($prop.Name)
        $currentValue = $prop.Value
        if ("$baseValue" -ne "$currentValue") {
            $differences += [pscustomobject]@{
                module = $module
                name = $case.name
                field = $prop.Name
                current = "$currentValue"
                baseline = "$baseValue"
            }
        }
    }
}

$reportData = [pscustomobject]@{
    tag = $Tag
    baseline_commit = (git -C $Worktree rev-parse HEAD).Trim()
    compared_values = $compared
    skipped_cases = $skipped
    excluded_modules = $excluded
    excluded_fields = $excludedFields
    difference_count = $differences.Count
    differences = $differences
    verdict = if ($differences.Count -eq 0) { 'IDENTICAL' } else { 'DRIFT' }
}
$reportData | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $report -Encoding UTF8

Write-Host ("  [g5] the baseline: {0}" -f $reportData.baseline_commit.Substring(0, 8))
Write-Host ("  [g5] compared values: {0}, skipped cases: {1}" -f $compared, $skipped)
if ($differences.Count -eq 0) {
    Write-Host '  [g5] VERDICT: IDENTICAL - the fixture describes the baseline behaviour'
} else {
    Write-Host ("  [g5] VERDICT: DRIFT - {0} difference(s)" -f $differences.Count)
    $differences | Select-Object -First 10 | ForEach-Object { Write-Host ("    {0}/{1} {2}: the current={3} the baseline={4}" -f $_.module, $_.name, $_.field, $_.current, $_.baseline) }
}
Write-Host ("  [g5] the report: {0}" -f $report)

if ($differences.Count -ne 0) { exit 1 }
exit 0
