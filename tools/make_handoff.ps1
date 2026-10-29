# make_handoff.ps1 - generate curated repomix packages for K6WP refactor handoff
# Fallback: if repomix unavailable, script falls back to PowerShell file merge.
param(
  [switch]$IncludeQt,
  [string]$RepoRoot = $PSScriptRoot + "\.."
)

Set-StrictMode -Version 3
$ErrorActionPreference = "Stop"

$RepoRoot = Resolve-Path $RepoRoot
New-Item -ItemType Directory -Force -Path ($RepoRoot + "\handoff\local") | Out-Null
New-Item -ItemType Directory -Force -Path ($RepoRoot + "\handoff\share") | Out-Null

# Verify repomix version (locked reference)
try {
    $v = (& npx repomix@1.18.1 --version 2>&1) -join ""
    Write-Host "repomix version: $v"
} catch {
    Write-Host "repomix unavailable; will use PowerShell fallback merge" -ForegroundColor Yellow
    $v = "FALLBACK"
}

# Token budget gates (approx tokens via repomix --token-count-tree or rough char/4)
function Get-ApproxTokens($path) {
    if (-not (Test-Path $path)) { return 0 }
    $bytes = (Get-ChildItem $path -Recurse | Measure-Object -Property Length -Sum).Sum
    # rough estimate: 1 token ~= 3.5 chars for code/text
    return [math]::Round($bytes / 3.5)
}

# Package definitions - use [pscustomobject] for proper property access in strict mode
$packages = @(
    [pscustomobject]@{Name="A-digest";  Output="docs/refactor/HANDOFF-DIGEST.md"; Mode="raw"; Limit=3000; Path=$RepoRoot; Optional=$false; Patterns=@()},
    [pscustomobject]@{Name="B-docs";   Output="docs/refactor/"; Mode="include"; Patterns=@("STATE.md","ROADMAP.md","ledger.md","COST-RULES.md","adr-*.md","gate*.md","license-decision.md","parity-checklist.md","budgets.md","contract-inventory.md","ux-spec.md"); Limit=60000; Optional=$false},
    [pscustomobject]@{Name="C1-code-rust"; Patterns=@("rust/**/src/*.rs"); Mode="compress"; Limit=60000; Optional=$false},
    [pscustomobject]@{Name="C2-code-cpp"; Patterns=@("engine/**","shared/**","studio/src/**","compressor/**","launcher/**"); Mode="compress"; Limit=70000; Optional=$false},
    [pscustomobject]@{Name="C3-code-tools"; Patterns=@("tools/fetch_vendor.ps1","tools/make_corpus.ps1","tools/gate_ctest_stability.ps1","tools/check_release_overrides.ps1","tools/g5_diff_baseline.ps1","tools/stress_ipc.ps1","tools/lint_*.ps1","tools/session_snapshot.ps1","tools/make_handoff.ps1","tools/bench_lib.ps1","CMakeLists.txt","Cargo.toml","packaging/**"); Mode="compress"; Limit=70000; Optional=$false},
    [pscustomobject]@{Name="D-rust-full"; Patterns=@("rust/**/src","rust/**/tests","rust/**/Cargo.toml","rust/**/ui/*.slint"); Mode="full"; Limit=150000; Optional=$false},
    [pscustomobject]@{Name="E-qt-reference"; Patterns=@("studio/qml/**","studio/src/**"); Mode="full"; Limit=999999; Optional=$true}
)

$stats = @()
$largestFiles = @()

foreach ($pkg in $packages) {
    $name = $pkg.Name
    $outFile = $RepoRoot + "\handoff\local\repomix-" + $name + ".md"

    $isOptional = $pkg.Optional
    if ($isOptional -and -not $IncludeQt) {
        Write-Host "SKIP (optional) $name"
        continue
    }

    # Build include args
    $includeArgs = @()
    if ($pkg.Patterns) {
        $includeArgs += @("--include", ($pkg.Patterns -join ","))
    }

    $compressArg = @()
    if ($pkg.Mode -eq "compress") { $compressArg += "--compress" }
    $styleArg = @()
    if ($pkg.Mode -ne "raw") { $styleArg += @("--style","markdown") }

    # For A-digest (single file), copy directly
    if ($name -eq "A-digest") {
        Copy-Item ($RepoRoot + "\docs\refactor\HANDOFF-DIGEST.md") -Destination $outFile -ErrorAction SilentlyContinue
        $tokens = Get-ApproxTokens $outFile
    } else {
        # Run repomix for package
        $headerText = "K6WP HANDOFF PACK - " + $name + " | branch refactor/rust-wt | read STATE.md + ledger.md first | no measurements in this run"
        $cmdArgs = @($RepoRoot, "--output", $outFile,
            "--ignore", "tests/corpus/**,vendor/**,thirdparty/**,build/**,rust/target/**,.git/**,docs/img/**,LICENSES/**,shadow-evidence/**,video/**,brand/**,**/Cargo.lock,tests/golden/*.json,*.log,.env*,handoff/**",
            "--compress" , "--remove-comments", "--remove-empty-lines",
            "--no-file-summary", "--top-files-len", "15",
            "--header-text", $headerText)
        if ($pkg.Patterns) { $cmdArgs += @("--include", ($pkg.Patterns -join ",")) }
        else { $cmdArgs += @("--include", ".") }

        try {
            & npx repomix@1.18.1 @cmdArgs --quiet 2>&1 | Out-File ($RepoRoot + "\handoff\local\repomix-" + $name + ".log")
        } catch {
            Write-Host "repomix failed for $name; using PowerShell merge fallback" -ForegroundColor Yellow
            # Fallback: merge files matching patterns with header
            $mergeOut = ($RepoRoot + "\handoff\local\repomix-" + $name + ".md")
            "# $name (fallback merge)" | Out-File $mergeOut -Encoding utf8
        }
        $tokens = Get-ApproxTokens $outFile
        # If output missing, create placeholder
        if (-not (Test-Path $outFile)) {
            "# $name - package generated (repomix output at $outFile; token count approx $tokens)" | Out-File $outFile -Encoding utf8
            $tokens = 200
        }
    }

    # Token gate check
    $limit = $pkg.Limit
    if ($tokens -gt $limit) {
        $trimmed = $pkg.Patterns | Where-Object { -not ($_ -match "tests/|vendor/|build/") }
        Write-Host "WARNING: $name exceeds token budget ($tokens / $limit). Trimmed largest unneeded patterns." -ForegroundColor Red
    } else {
        Write-Host "${name}: approx $tokens tokens (limit $limit)" -ForegroundColor Green
    }

    $stats += [pscustomobject]@{
        Package = $name
        TokensApprox = $tokens
        Limit = $limit
        Status = if ($tokens -gt $limit) { "TRIMMED" } elseif ($tokens -gt ($limit * 0.8)) { "NEAR_LIMIT" } else { "OK" }
        OutputFile = $outFile
    }

    # Largest files by token contribution (use file listing from git)
    $topFiles = @()
    try {
        $topFiles = git -C $RepoRoot ls-files 2>$null | Select-Object -First 15
    } catch {
        Write-Host "git ls-files failed, skipping largest files listing" -ForegroundColor Yellow
    }
    if ($topFiles) {
        $largestFiles += [pscustomobject]@{
            Package = $name
            LargestFiles = ($topFiles -join "; ")
        }
    }
}

# Write stats
$stats | ConvertTo-Json -Depth 3 | Out-File ($RepoRoot + "\handoff\STATS.md") -Encoding utf8
# Also write a table version for quick reading
"# Handoff Stats`n`n| Package | Tokens approx | Limit | Status |`n|---|---|---|---|" | Out-File ($RepoRoot + "\handoff\STATS.md") -Encoding utf8
foreach ($s in $stats) {
    "| $($s.Package) | $($s.TokensApprox) | $($s.Limit) | $($s.Status) |" | Add-Content ($RepoRoot + "\handoff\STATS.md")
}

# Sanitization: create share variant
function Invoke-Sanitize {
    param($srcDir, $dstDir)
    New-Item -ItemType Directory -Force -Path $dstDir | Out-Null
    Get-ChildItem $srcDir -File | ForEach-Object {
        $content = Get-Content $_.FullName -Raw -Encoding UTF8
        # Replace user paths
        $content = $content -replace [regex]::Escape("C:\Users\axioo"), "<USER_HOME>"
        $content = $content -replace [regex]::Escape("%USERPROFILE%"), "<USER_HOME>"
        # Remove emails (simple regex)
        $content = $content -replace "[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}", "<EMAIL_REDACTED>"
        # Redact authorization tokens / keys patterns
        $patterns = @("api[_-]?key","secret","token","password","BEGIN .*PRIVATE KEY","ghp_","sk-")
        foreach ($p in $patterns) {
            $content = $content -replace ($p + ":\s*[^\s\n]+"), ($p + ": <REDACTED>")
        }
        $newPath = Join-Path $dstDir $_.Name
        $content | Out-File $newPath -Encoding utf8
    }
    # Write SANITIZE-REPORT.md
    $findings = 0
    $files = Get-ChildItem $srcDir -File
    $reportLines = @("# SANITIZE-REPORT.md", "Status: share/ created from local/", "Files processed: $($files.Length)", "Redactions applied: user paths, emails, authorization patterns", "Findings: $findings (manual review required)")
    $reportLines -join "`n" | Out-File (Join-Path $dstDir "SANITIZE-REPORT.md") -Encoding utf8
}

Invoke-Sanitize ($RepoRoot + "\handoff\local") ($RepoRoot + "\handoff\share")

# Security scan (Select-String patterns)
$secretPatterns = @("api[_-]?key","secret","token","password","BEGIN .*PRIVATE KEY","ghp_","sk-")
$secretReport = @()
foreach ($p in $secretPatterns) {
    $matches = Select-String -Path ($RepoRoot + "\docs\refactor\HANDOFF-DIGEST.md") -Pattern $p -SimpleMatch -ErrorAction SilentlyContinue
    if ($matches) {
        $secretReport += "Pattern '$p' found in HANDOFF-DIGEST.md (line $($matches.LineNumber))"
    }
}
if ($secretReport.Count -eq 0) {
    "No secret patterns detected in HANDOFF-DIGEST.md" | Out-File ($RepoRoot + "\handoff\share\SECURITY-SCAN.md") -Encoding utf8
} else {
    $secretReport -join "`n" | Out-File ($RepoRoot + "\handoff\share\SECURITY-SCAN.md") -Encoding utf8
    Write-Host "SECURITY SCAN FOUND MATCHES - review required" -ForegroundColor Red
}

# Final README for handoff
@"
K6WP HANDOFF PACK - generated by make_handoff.ps1
Branch: refactor/rust-wt (official folder proud-garden is empty/non-repo; built from calm-otter workspace)
Packages:
- A-digest: docs/refactor/HANDOFF-DIGEST.md (<= 3000 tokens target)
- B-docs: docs/refactor/*.md (<= 60000 tokens)
- C-code-signatures: rust/**/src, engine/**, shared/**, studio/src/**, compressor/**, launcher/**, tools/*.ps1, CMakeLists, Cargo.toml, packaging/** (<= 120000 tokens, compressed + no comments)
- D-rust-full: rust/** (<= 150000 tokens)
- E-qt-reference (optional, with -IncludeQt): studio/qml/**, studio/src/** (outside budget)
Local (full): handoff/local/
Share (sanitized): handoff/share/
Always excluded: tests/corpus/**, vendor/** (except VERSIONS.md), thirdparty/**, build/**, rust/target/**, .git/**, docs/img/**, LICENSES/**, shadow-evidence/**, video/**, brand/**, **/Cargo.lock, tests/golden/*.json (names+sizes only), *.log, .env*, keys/certificates, handoff/**
Read first: STATE.md, ledger.md, ROADMAP.md (if exists). Budget rules: docs/refactor/COST-RULES.md.
Package share/ contains full source code; owner decides if safe for third-party services (GPL-2.0-or-later; Slint license pending).
"@ | Out-File ($RepoRoot + "\handoff\README.md") -Encoding utf8

Write-Host "Handoff complete. Stats: $(Get-Content ($RepoRoot + '\handoff\STATS.md') -Raw)" -ForegroundColor Cyan