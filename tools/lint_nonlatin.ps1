# tools/lint_nonlatin.ps1 - H4: no accidental non-Latin characters.
#
# WHY
#   A few of the project's documents and code comments had picked up
#   non-Latin characters that nobody typed on purpose: a CJK character in
#   the middle of an English/Indonesian word, a Cyrillic look-alike, a
#   replacement character (U+FFFD) left over from a broken decode. They make
#   the text unsearchable (a grep for the word misses it) and they usually
#   mark a corrupted fragment. This lint catches them.
#
# WHAT IT FLAGS
#   R1. U+FFFD (the replacement character) - ALWAYS broken, no exceptions.
#   R2. A "mixed-script word": a token that contains a Latin letter AND a
#       CJK/Hangul/Cyrillic/Arabic/Thai letter (e.g. "meng" followed by a CJK
#       character). This is the shape of the accidental corruption.
#   R3. A CJK/Hangul/Cyrillic/Arabic/Thai character anywhere in a scanned
#       file, UNLESS the file declares that it does that on purpose with the
#       marker line:  nonlatin-allow: <the reason>
#       (the tests that verify non-ASCII path escaping need real CJK text).
#   R4. A byte-order mark INSIDE the text (a copy-paste artifact, usually a
#       UTF-8 BOM that PowerShell's `Set-Content -Encoding UTF8` prepends -
#       it silently corrupted a commit subject once). A BOM at the very start
#       of a file is legal and is not flagged; ReadAllText strips it.
#
# WHAT IT DOES NOT FLAG
#   Typography and math that the documents legitimately use: em/en dashes,
#   arrows, the minus sign, <=/>=, the multiplication sign, the micro sign,
#   the degree sign, the ellipsis, Delta, and box-drawing characters (the
#   state diagrams in the design documents). Those are not "corruption" and
#   rewriting them would lose meaning.
#
# WHAT IT SCANS
#   The git-tracked files only (`git ls-files`), so a generated scratch file
#   such as a repomix dump is never scanned. Inside LICENSES/ only README.md
#   is scanned: the other files ARE the original licence texts.
#
# USAGE
#   powershell -File tools/lint_nonlatin.ps1            # scan, fail on errors
#   powershell -File tools/lint_nonlatin.ps1 -List      # also list every file
#                                                       # with non-ASCII hits
#   powershell -File tools/lint_nonlatin.ps1 -Commits 30 # also check the last
#                                                       # 30 commit messages
#   powershell -File tools/lint_nonlatin.ps1 -Staged    # only the staged files
#                                                       # (the pre-commit hook)
#
[CmdletBinding()]
param(
    [switch]$List,
    [int]$Commits = 0,
    # The pre-commit hook passes -Staged: scan only the files the commit adds
    # or changes. CI runs the full scan over every tracked file.
    [switch]$Staged,
    # The self-test points ScanRoot at a scratch repository so it can prove the
    # detector fires without touching the real tree.
    [string]$ScanRoot = ''
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
if ($ScanRoot) {
    $repo = (Resolve-Path -LiteralPath $ScanRoot).Path
} else {
    $repo = (Resolve-Path $root).Path
}
Set-Location -LiteralPath $repo

$allowMarker = 'nonlatin-allow:'

$exts = @('.md', '.cpp', '.hpp', '.h', '.c', '.rs', '.ps1', '.yml', '.yaml',
          '.json', '.txt', '.toml', '.cmake', '.in', '.qml', '.js', '.ts')

# Never scanned: generated dumps and the original licence texts.
function Test-Skipped([string]$rel) {
    if ($rel -like 'LICENSES/*' -and $rel -ne 'LICENSES/README.md') { return $true }
    if ($rel -like 'tests/corpus/*') { return $true }
    if ($rel -like 'vendor/*') { return $true }
    return $false
}

$replacement = [char]0xFFFD
$replacementStr = [string]$replacement
# R4: a byte-order mark anywhere except the start of a file.
$bom = [char]0xFEFF
$bomStr = [string]$bom

# CJK, Hangul, Cyrillic, Arabic, Thai.
$cjk = '[\u3040-\u30FF\u3400-\u4DBF\u4E00-\u9FFF\uF900-\uFAFF\uAC00-\uD7AF\u1100-\u11FF\u0400-\u04FF\u0600-\u06FF\u0E00-\u0E7F]'

$errors = New-Object System.Collections.Generic.List[string]
$inventory = New-Object System.Collections.Generic.List[string]

if ($Staged) {
    $tracked = & git -C $repo diff --cached --name-only --diff-filter=ACMR
} else {
    $tracked = & git -C $repo ls-files
}
if ($LASTEXITCODE -ne 0) { Write-Host '  [nonlatin] git ls-files failed'; exit 2 }

$scanned = 0
$maxPerFile = 5
foreach ($rel in $tracked) {
    $rel = $rel -replace '\\', '/'
    if (Test-Skipped $rel) { continue }
    $ext = [IO.Path]::GetExtension($rel).ToLowerInvariant()
    if ($exts -notcontains $ext) { continue }
    $full = Join-Path $repo ($rel -replace '/', [IO.Path]::DirectorySeparatorChar)
    if (-not (Test-Path -LiteralPath $full)) { continue }
    $scanned++

    try {
        $text = [IO.File]::ReadAllText($full, [Text.Encoding]::UTF8)
    } catch {
        $errors.Add(("{0}: the file could not be read as UTF-8 ({1})" -f $rel, $_.Exception.Message))
        continue
    }
    $hasReplacement = (-1 -ne $text.IndexOf($replacementStr, [StringComparison]::Ordinal))
    $hasCjk = $text -match $cjk
    $hasBom = (-1 -ne $text.IndexOf($bomStr, [StringComparison]::Ordinal))
    if (-not $hasReplacement -and -not $hasCjk -and -not $hasBom) { continue }

    $lines = $text -split "`r?`n"
    $allow = $false
    foreach ($l in $lines) { if ($l -like "*${allowMarker}*") { $allow = $true; break } }

    $reported = 0
    for ($i = 0; $i -lt $lines.Count; $i++) {
        $line = $lines[$i]
        $lineNo = $i + 1

        if ($hasReplacement -and (-1 -ne $line.IndexOf($replacementStr, [StringComparison]::Ordinal))) {
            $col = ($line.IndexOf($replacementStr, [StringComparison]::Ordinal) + 1)
            if ($reported -lt $maxPerFile) {
                $errors.Add(("{0}:{1}:{2}: R1 the replacement character U+FFFD - the text was decoded from broken bytes; retype the intended word" -f $rel, $lineNo, $col))
            }
            $reported++
            continue
        }
        # R4: a byte-order mark inside the text (ReadAllText strips
        # the one at the start of the file, so anything left is mid-text).
        if ($hasBom -and (-1 -ne $line.IndexOf($bomStr, [StringComparison]::Ordinal))) {
            $col = ($line.IndexOf($bomStr, [StringComparison]::Ordinal) + 1)
            if ($reported -lt $maxPerFile) {
                $errors.Add(("{0}:{1}:{2}: R4 a byte-order mark inside the text - a copy-paste artifact; delete the invisible character" -f $rel, $lineNo, $col))
            }
            $reported++
            continue
        }

        if (-not $hasCjk) { continue }

        # R2: a mixed-script word (Latin + non-Latin in one token).
        if (-not $allow) {
            foreach ($m in [regex]::Matches($line, "[A-Za-z][^\s]*${cjk}|${cjk}[^\s]*[A-Za-z]")) {
                if ($reported -lt $maxPerFile) {
                    $errors.Add(("{0}:{1}:{2}: R2 a mixed-script word '{3}' - a non-Latin character inside a Latin word" -f $rel, $lineNo, ($m.Index + 1), $m.Value))
                }
                $reported++
            }
        }

        # R3: non-Latin script unless the file declares it on purpose.
        $hits = [regex]::Matches($line, $cjk)
        if ($hits.Count -gt 0 -and -not $allow) {
            $cps = ($hits | ForEach-Object { "U+{0:X4}" -f [int][char]$_.Value } | Sort-Object -Unique) -join ' '
            if ($reported -lt $maxPerFile) {
                $errors.Add(("{0}:{1}: R3 non-Latin script ({2}) - if this is intentional, add a marker line '{3} <reason>'" -f $rel, $lineNo, $cps, $allowMarker))
            }
            $reported++
        }
    }
    if ($reported -gt $maxPerFile) {
        $errors.Add(("{0}: {1} more finding(s) in this file were not listed" -f $rel, ($reported - $maxPerFile)))
    }

    if ($List) {
        $distinct = New-Object System.Collections.Generic.HashSet[int]
        foreach ($ch in $text.ToCharArray()) {
            $c = [int][char]$ch
            if ($c -gt 127) { [void]$distinct.Add($c) }
        }
        if ($distinct.Count -gt 0) {
            $tag = ''
            if ($allow) { $tag = ' [allowed]' }
            $inventory.Add(("  {0}: {1} distinct non-ASCII code points{2}" -f $rel, $distinct.Count, $tag))
        }
    }
}

$commitFindings = New-Object System.Collections.Generic.List[string]
if ($Commits -gt 0) {
    $messages = & git -C $repo log "-$Commits" --format='%h%x1f%B%x1e'
    foreach ($block in ($messages -split [char]0x1E)) {
        if (-not $block.Trim()) { continue }
        $parts = $block -split [char]0x1F, 2
        if ($parts.Count -lt 2) { continue }
        $hash = $parts[0].Trim()
        $body = $parts[1]
        if ((-1 -ne $body.IndexOf($replacementStr, [StringComparison]::Ordinal))) {
            $commitFindings.Add(("  {0}: R1 U+FFFD in the commit message" -f $hash))
        }
        if (-1 -ne $body.IndexOf($bomStr, [StringComparison]::Ordinal)) {
            $commitFindings.Add(("  {0}: R4 a byte-order mark in the commit message (a UTF-8 BOM from Set-Content -Encoding UTF8)" -f $hash))
        }
        foreach ($m in [regex]::Matches($body, $cjk)) {
            $commitFindings.Add(("  {0}: R3 non-Latin script (U+{1:X4}) in the commit message" -f $hash, [int][char]$m.Value))
        }
    }
}

if ($List) {
    Write-Host '  [nonlatin] the non-ASCII inventory:'
    if ($inventory.Count -eq 0) { Write-Host '    (none)' }
    foreach ($line in $inventory) { Write-Host $line }
}

Write-Host (("  [nonlatin] scanned {0} {1} files, {2} error(s)" -f $scanned, $(if ($Staged) { 'staged' } else { 'tracked' }), $errors.Count))
foreach ($e in $errors) { Write-Host ("  [nonlatin] " + $e) }

if ($commitFindings.Count -gt 0) {
    Write-Host (("  [nonlatin] the last {0} commit messages: {1} finding(s) (ADVISORY - history is not rewritten)" -f $Commits, $commitFindings.Count))
    foreach ($c in $commitFindings) { Write-Host $c }
}

if ($errors.Count -gt 0) { exit 1 }
exit 0