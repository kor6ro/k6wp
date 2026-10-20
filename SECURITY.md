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

## K6WP folder ACL risk (LOW-17, resolved by removal)

The old elevated mechanism granted `GENERIC_READ | GENERIC_WRITE | DELETE`
to the current user plus `GENERIC_READ | GENERIC_WRITE` to the local
`Users` group on `%PROGRAMDATA%\K6WP`
(`GrantK6wpFolderWriteAccess`), because the unelevated `compressor
--lockframe` refresh rewrote `lockscreen.jpg` there on every cycle — a file
the machine-wide `HKLM\...\Personalization LockScreenImage` policy pointed
at. That residual risk (one local user defacing another user's lock screen)
no longer exists: the feature is per-user now.

The still pipeline writes only under `%LOCALAPPDATA%\K6WP\stills` (which
already inherits the per-user profile ACL), and the lock-screen image is
applied through the per-user WinRT `LockScreen::SetImageFileAsync` API.
There is no cross-user write path and no HKLM write in the normal flow.
`GrantK6wpFolderWriteAccess` and the `--elevate-lockscreen on` path were
deleted; `--elevate-lockscreen off` remains as a cleanup-only shim that
removes a leftover legacy policy/ACL/`lockscreen.jpg` (see LOW-21 below).

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

## Legacy lockscreen policy cleanup (LOW-21)

K6WP no longer WRITES the machine-wide
`HKLM\SOFTWARE\Policies\Microsoft\Windows\Personalization\LockScreenImage`
policy. Builds before the per-user still change could have left that value
pointing at `%PROGRAMDATA%\K6WP\lockscreen.jpg`, plus the permissive
`%PROGRAMDATA%\K6WP` ACL and a `lockscreen_policy_backup.json`. The
cleanup-only entry point removes exactly that residue:

`K6WP.exe --elevate-lockscreen off` (self-elevates through a `runas`
relaunch) deletes the policy value ONLY when it points at our legacy path,
deletes `%PROGRAMDATA%\K6WP` when it contains nothing but our legacy files,
and removes the backup file. A policy pointing elsewhere is left untouched.
The Studio detects the residue read-only (no UAC) and offers a single
"Bersihkan" button; declining the UAC prompt leaves everything as-is. The
`on` argument is rejected (`kExitBadFlag`); there is no way to re-create
the policy.

The uninstall paths invoke the same `off` step so a stale policy never
keeps pointing at a folder the uninstall just removed:

- `packaging/uninstall.bat` (portable) and `tools/uninstall.bat` (dev):
  echo what is happening, then run the sibling `K6WP.exe
  --elevate-lockscreen off`. A UAC prompt is expected; declining keeps
  the current policy and the uninstall continues.
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
