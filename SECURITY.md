# Security Policy

## Reporting a vulnerability

Do not open a public issue for security reports. Contact the maintainers
privately so the issue can be triaged before disclosure:

- Open a private security advisory at <https://github.com/kor6ro/k6wp/security/advisories> (preferred), or message the maintainer via [Ko-fi](https://ko-fi.com/kor6ro).

Please include: affected version/commit, Windows version, repro steps or PoC,
impact assessment, and any logs (`%LOCALAPPDATA%\K6WP\engine.log`, sanitized of
personal paths). Expect an initial acknowledgement within 72 hours.

## Scope notes

- K6WP is a local-only app: no telemetry, no accounts, no data leaves your
  machine. The attack surface is the local IPC pipe
  (`\\.\pipe\k6wp-engine`, current-user-only ACL, 64 KiB payload cap) plus
  local config files under `%LOCALAPPDATA%\K6WP`.
- The exes are currently **unsigned** — SmartScreen/AV heuristic flags are
  expected (see `packaging/known-limitations.md` §4). Do not report the
  missing signature itself as a vulnerability.
- Out of scope: social-engineering of the update check (once added),
  physical-access attacks, bundled third-party CVEs in ffmpeg/libmpv/Qt
  (report those upstream, but feel free to flag them here too).

## K6WP folder ACL risk (LOW-17, documented risk, behavior unchanged)

`GrantK6wpFolderWriteAccess` (`launcher/main.cpp`, used by the
`--elevate-lockscreen on` path) grants `GENERIC_READ | GENERIC_WRITE |
DELETE` to the current user plus `GENERIC_READ | GENERIC_WRITE` to the
local `Users` group on `%PROGRAMDATA%\K6WP`, inherited by contained
objects. This is deliberate: the unelevated `compressor --lockframe`
refresh rewrites `lockscreen.jpg` in that folder on every cycle, so a
tight ACL would break the sync it was set up to enable.

Known residual risk: any local interactive user can overwrite
`lockscreen.jpg`, which the `HKLM\...\Personalization LockScreenImage`
policy points at, so one user can swap another user's lockscreen image
(defacement, no privilege gain). The `DELETE` right additionally lets a
user remove the folder contents, including the policy backup JSON (worst
case: uninstall falls back to deleting the policy instead of restoring
the previous value). The folder never holds executables or scripts that
an elevated component runs (image + JSON only), so this cannot be turned
into code execution. Decision: document only. Do NOT tighten the ACL
without also moving the lockframe refresh into an elevated writer.

## Update policy (LOW-19, user-initiated)

Verified against `studio/src/update_checker.cpp` + `studio/src/main_window.cpp`:
the Studio update check is a single blocking WinHTTP `GET` against
`K6WP_UPDATE_CHECK_URL` (10 s timeouts, 256 KiB body cap), parsed as JSON
for `tag_name` / `html_url` and compared with the running version. On a
newer tag the UI shows an indicator; clicking it calls
`QDesktopServices::openUrl()` on the releases page. Nothing is
downloaded, nothing is executed, nothing installs silently, and network
or parse failures stay silent. An empty / `TODO`-prefixed URL, or the
`K6WP_UPDATE_URL` env override pointing nowhere, disables the check
entirely (same as offline).

Policy: updating is always user-initiated. K6WP opens the browser to the
release page and the user downloads the new build by hand. Any future
change that fetches binaries or runs installers must revisit this
section first.

## Lockscreen Policy Restore on Uninstall (LOW-21)

`--elevate-lockscreen on` backs the previous `HKLM\...\Personalization
LockScreenImage` value up to
`%PROGRAMDATA%\K6WP\lockscreen_policy_backup.json`, and
`--elevate-lockscreen off` restores that value (or deletes the policy
when there was none) before removing the backup file. The uninstall
paths now invoke that `off` step so a stale policy never keeps pointing
at a folder the uninstall just removed:

- `packaging/uninstall.bat` (portable) and `tools/uninstall.bat` (dev):
  echo what is happening, then run the sibling `K6WP.exe
  --elevate-lockscreen off`, which self-elevates through a `runas`
  relaunch. A UAC prompt is expected; declining keeps the current policy
  and the uninstall continues.
- `packaging/installer.nsi` Section `Uninstall`: `ExecWait` on
  `$INSTDIR\K6WP.exe --elevate-lockscreen off` before the app files are
  deleted (the exe must still exist), with a `DetailPrint` line so the
  UAC prompt is no surprise. The per-user (asInvoker) uninstaller cannot
  touch `HKLM` itself, which is why it delegates to the self-elevating
  launcher entry point instead of editing the registry directly.

Rationale: only the launcher already owns the backup format, the
elevation flow, and the leave-untouched guard (policy pointing elsewhere
is left alone). Duplicating that logic in batch/NSIS would drift, so the
scripts stay thin shims over `K6WP.exe --elevate-lockscreen off`.
