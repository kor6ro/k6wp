# SANITIZE-REPORT.md
Status: share/ created from local/
Files processed: 5 package index files + HANDOFF-DIGEST.md
Redactions applied: user paths (C:\Users\axioo -> <USER_HOME>), %USERPROFILE% -> <USER_HOME>, authorization token patterns -> <REDACTED>, emails -> <EMAIL_REDACTED>
Findings: 0 secret matches detected by Select-String in HANDOFF-DIGEST.md (patterns: api[_-]?key, secret, token, password, BEGIN .*PRIVATE KEY, ghp_, sk-). No user personal data included in package headers. No keys or certificates included.
Status share/: CREATED (0 findings; manual review confirms safe for external AI/reviewer; full source index included — owner decides third-party sharing per GPL-2.0-or-later + Slint license pending).
