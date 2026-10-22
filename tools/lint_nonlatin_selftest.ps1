# tools/lint_nonlatin_selftest.ps1 - prove the H4 detector actually fires.
#
# WHY
#   A lint that stops detecting is worse than no lint: the CI stays green and
#   the corruption comes back. This test builds a scratch git repository with
#   one clean file, one deliberately mixed-script file, one file holding a
#   replacement character and one with a byte-order mark inside the text,
#   then asserts:
#     - the clean file (typography only) passes,
#     - the allow marker suppresses a deliberate non-Latin string,
#     - a byte-order mark at the START of a file passes (it is legal),
#     - the mixed-script file fails with R2,
#     - the replacement-character file fails with R1,
#     - the mid-text byte-order mark fails with R4,
#     - removing the three bad files makes the scan pass again.
#
#   The scratch repository is removed in finally.
#
[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$tool = Join-Path $PSScriptRoot 'lint_nonlatin.ps1'
$failures = New-Object System.Collections.Generic.List[string]
$checks = 0

function Assert-That([bool]$condition, [string]$what) {
    $script:checks++
    if (-not $condition) { $script:failures.Add($what) }
}

function Invoke-Lint([string]$scanRoot) {
    $text = & powershell -NoProfile -ExecutionPolicy Bypass -File $tool -ScanRoot $scanRoot 2>&1 | Out-String
    return @{ Exit = $LASTEXITCODE; Text = $text }
}

$cjk = [string][char]0x6D4B      # a CJK character
$delta = [string][char]0x0394    # the Greek capital delta: legitimate math
$arrow = [string][char]0x2192    # a right arrow: legitimate typography
$bad = [string][char]0xFFFD      # the replacement character
$bom = [string][char]0xFEFF      # a byte-order mark

$tmp = Join-Path $env:TEMP ('k6wp-nonlatin-' + [guid]::NewGuid().ToString('N'))
$utf8 = New-Object Text.UTF8Encoding($false)

function Write-Utf8([string]$path, [string]$text) {
    [IO.File]::WriteAllText($path, $text, $utf8)
}

try {
    New-Item -ItemType Directory -Path $tmp | Out-Null
    & git -C $tmp init -q
    if ($LASTEXITCODE -ne 0) { throw 'git init failed' }

    Write-Utf8 (Join-Path $tmp 'good_typography.md') ("# Rates`n`nA " + $arrow + " B: the " + $delta + " stays <= 12 %.`n")
    Write-Utf8 (Join-Path $tmp 'good_allow.md') ("nonlatin-allow: the selftest needs one real CJK string`n`npath = C:\video\" + $cjk + ".mp4`n")
    # A UTF-8 BOM at the very start of a file is legal (PowerShell and Visual
    # Studio both write one) and must not be flagged.
    [IO.File]::WriteAllText((Join-Path $tmp 'good_bom.md'), ("# A file with a byte-order mark`n"), (New-Object Text.UTF8Encoding($true)))
    & git -C $tmp add -A
    if ($LASTEXITCODE -ne 0) { throw 'git add failed' }

    $r = Invoke-Lint $tmp
    Assert-That ($r.Exit -eq 0) ("the clean tree passes (exit " + $r.Exit + ")")
    Assert-That ($r.Text -notmatch 'good_typography') 'typography is not flagged'
    Assert-That ($r.Text -notmatch 'good_allow') 'the allow marker is honoured'
    Assert-That ($r.Text -notmatch 'good_bom') 'a leading byte-order mark is not flagged'

    Write-Utf8 (Join-Path $tmp 'bad_mixed.md') ("the word meng" + $cjk + " appears mid-word`n")
    Write-Utf8 (Join-Path $tmp 'bad_replacement.md') ("the text was " + $bad + " decoded from broken bytes`n")
    Write-Utf8 (Join-Path $tmp 'bad_bom.md') ("the text was " + $bom + " pasted from a console`n")
    & git -C $tmp add -A

    $r = Invoke-Lint $tmp
    Assert-That ($r.Exit -eq 1) ("the corrupt tree fails (exit " + $r.Exit + ")")
    Assert-That ($r.Text -match 'bad_mixed\.md:\d+:\d+: R2') 'R2 fires on the mixed-script word'
    Assert-That ($r.Text -match 'bad_replacement\.md:\d+:\d+: R1') 'R1 fires on the replacement character'
    Assert-That ($r.Text -match 'bad_bom\.md:\d+:\d+: R4') 'R4 fires on the mid-text byte-order mark'
    Assert-That ($r.Text -notmatch 'good_typography') 'typography stays clean next to the failures'

    Remove-Item -LiteralPath (Join-Path $tmp 'bad_mixed.md')
    Remove-Item -LiteralPath (Join-Path $tmp 'bad_replacement.md')
    Remove-Item -LiteralPath (Join-Path $tmp 'bad_bom.md')
    & git -C $tmp add -A

    $r = Invoke-Lint $tmp
    Assert-That ($r.Exit -eq 0) ("the repaired tree passes again (exit " + $r.Exit + ")")
} finally {
    if (Test-Path -LiteralPath $tmp) {
        Remove-Item -LiteralPath $tmp -Recurse -Force -ErrorAction SilentlyContinue
    }
}

Write-Host ("  [nonlatin-selftest] {0} checks, {1} failure(s)" -f $checks, $failures.Count)
foreach ($f in $failures) { Write-Host ("  [nonlatin-selftest] FAIL: " + $f) }
if ($failures.Count -gt 0) { exit 1 }
exit 0
