# Known issues

Five honest limitations, each with where to look. None blocks the
manual checklist in docs/manual-test.md.

1. First `get_state` after pipe-up reports `running:false` until the
   video actually starts; later polls report `running:true`. Studio maps
   both to Connected (only the paused flag splits the label), so this is
   cosmetic. Pre-existing engine behavior, out of scope.
   (task-24-status-states.txt, Honest notes)
2. A synthetic tray double-click (posted message, no real mouse) opens
   Studio but cannot steal foreground: Windows foreground-lock blocks
   `SetForegroundWindow` without genuine input, so the foreground stayed
   on the harness window in the probe. Singleton behavior (one Studio,
   same HWND) passes; a real mouse double-click transfers foreground
   normally. (task-13-studio-singleton.txt; learnings.md T13 FG note)
3. `lupdate.exe` in this Qt 6.8.3 install cannot run (missing Linguist
   runtime, exit 0xC0000135); `lrelease.exe` works. Toolchain note only, and
   the build already accommodates it: `studio/CMakeLists.txt` resolves both
   tools with `find_program` and, when one is missing, warns at configure time
   and registers a no-op target that prints what it skipped instead of
   failing. The Studio binary is therefore buildable either way, it just
   ships no catalogue. Note that MED-11's removal of the i18n scaffolding is
   itself **reversed** as of 1.2.0: `studio/i18n/studio_en.ts` exists again
   (211 messages, none unfinished) and the `lupdate` / `lrelease` /
   `translations` targets are live. Only `lrelease` gates the `studio`
   target, so a plain build compiles `build/i18n/studio_en.qm` and copies it
   next to `studio.exe` without rewriting the committed `.ts`; after changing
   UI strings run `cmake --build --preset msvc-dev --target translations`.
   If a healthy Qt Linguist install is available, re-extracting the catalogue
   is worth doing: `.ts` `<location>` entries are only as trustworthy as the
   last `lupdate` run.
   (task-26-i18n.txt; MED-11 removal, reversed by the 1.2.0 i18n restore)
4. The library thumbnailer resolves ffmpeg relative to the running exe,
   so Studio must run from the build output layout (ffmpeg next to the
   exe). Running a studio binary copied elsewhere alone logs ffmpeg
   failures and yields no thumbnails. (task-23-library-thumbnails.txt
   Phase 1 harness note; learnings.md T21 probe trap)
5. Cold-start <2 s is measured, not reboot-cold: median 1337 ms over 5
   same-session runs (warm file cache) using pipe-ack plus 700 ms settle
   as the readiness proxy, because shell-side polling cannot see the
   injected window cross-process. No 4K asset exists in the repo, so the
   4K path is unmeasured; by construction a slow video keeps the engine
   alive on the idle surface rather than crashing. The true reboot
   measurement is a manual step (docs/manual-test.md item 6).
   (task-18-coldstart.txt)
