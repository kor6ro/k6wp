# K6WP two-monitor run-book

**Audience:** you are helping test K6WP on a Windows PC with two monitors.
You do **not** need to be a developer. Copy-paste the commands exactly as
written. When you are done, send back everything listed under **PASTE THESE
BACK** at the bottom.

**Why this run-book exists:** the wallpaper engine may cover only one screen
when Windows is set to *Extend*. We need evidence from a real two-monitor
machine (your PC) to see exactly what the engine sees and where it places the
picture. The commands below collect that evidence automatically.

**Related documents (for the people maintaining K6WP, not for you):**
- Manual test checklist item 8 (multi-monitor cover): `docs/manual-test.md`
- Known limitations, §2 secondary-monitor capture: `packaging/known-limitations.md`

---

## Before you start (prerequisites)

1. **A K6WP engine build with logging turned on.**
   Whoever gave you the build must have compiled it with the CMake option
   `-DK6WP_VERBOSE=ON`. If you are not sure, ask them; without it the log
   lines we need may be missing.

2. **The build folder on this PC.**
   You need the folder that contains `engine.exe` and `monitor_dump.exe`.
   On a developer machine that is usually something like
   `...\k6wp\build\<folder>\` (for example `build\msvc-dev\` or
   `build\task-30\`). The evidence script looks for `monitor_dump.exe`
   automatically under `build\`.

3. **The repo folder** (the project folder that contains `tools\` and
   `docs\`). All commands below assume you open PowerShell **inside that
   folder**.

4. **Two monitors attached and working** in Windows (Extend or Duplicate —
   you will switch between them during the test).

---

## Step 1 — Set Windows to Extend

1. Press **Windows + P** (or right-click the desktop → **Display settings**).
2. Choose **Extend** — not Duplicate, not PC screen only.
3. Confirm the two screens show different content (each has its own desktop).

---

## Step 2 — Start the engine

1. Start `engine.exe` from your build folder (double-click it, or from a
   terminal: `& 'C:\path\to\build\...\engine.exe'`).
2. Wait about 10 seconds. You should see the wallpaper on the desktop.
   Leave the engine running.

---

## Step 3 — Run the evidence script (one command)

In PowerShell, from the repo folder, run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools\run_2monitor_evidence.ps1
```

The script prints a line like:

```text
bundle_dir: C:\...\k6wp\.omo\evidence\2monitor-20261005-123456
```

**Write that folder path down.** Everything you need to send back is inside
that folder. The script never fails the run — if something is missing it
writes a warning into `summary.txt` and still produces the rest.

Optional: if your `monitor_dump.exe` is not under `build\`, pass its path:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools\run_2monitor_evidence.ps1 `
  -MonitorDumpExe 'C:\path\to\monitor_dump.exe'
```

Optional: if your `engine.log` is not in the default place, pass it:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools\run_2monitor_evidence.ps1 `
  -EngineLog 'C:\path\to\engine.log'
```

---

## Step 4 — Switch to Duplicate and repeat

1. Press **Windows + P** and choose **Duplicate** (both screens show the
   same image).
2. If the engine stopped, start it again and wait ~10 seconds.
3. Run the script **again** (same command as Step 3).
4. Write down the **second** `bundle_dir` path.

---

## Step 5 — Note any portrait monitor

If either monitor is rotated (taller than wide — portrait), write that down
in your reply, for example:

```text
The right-hand monitor is portrait (vertical).
```

If both are normal landscape, write "both monitors landscape".

---

## Extra: capture the log lines yourself (optional)

If you want to double-check the placement log lines by hand, run:

```powershell
Get-Content "$env:LOCALAPPDATA\K6WP\engine.log" | Select-String -Pattern 'placement:'
```

To copy the whole log to a file you can send:

```powershell
Copy-Item "$env:LOCALAPPDATA\K6WP\engine.log" .\my-engine.log
```

If your engine writes its log somewhere else, replace the path with yours.

---

## PASTE THESE BACK

Send **all** of the following in your reply. The first three files are
produced automatically by the evidence script (in each `bundle_dir`
folder). The screenshot is taken by hand.

| # | What to send | Where to find it |
|---|---|---|
| 1 | **Extended `monitor_dump` JSON** — the file `monitor_dump.json` from the **Extend** run (and from the **Duplicate** run if you did it) | Inside the bundle folder printed as `bundle_dir` |
| 2 | **The `placement:` log lines** — the file `placement.log` from each run | Inside the bundle folder |
| 3 | **The injected-window rect census** — the file `injected_windows.json` from each run | Inside the bundle folder |
| 4 | **One screenshot per display mode** — a screenshot taken while the wallpaper is showing, once in **Extend** and once in **Duplicate** | Take with **Win + Shift + S**, save as a file, attach it |
| 5 | **The rig's Windows build** — run this and paste the output (the script also saves it as `windows_build.txt` in the bundle) | `winver` dialog, or PowerShell: `[System.Environment]::OSVersion.Version` |
| 6 | **The display mode for each run** — say which bundle was Extend and which was Duplicate | From your notes in Steps 1 and 4 |
| 7 | **Portrait note** — if any monitor is portrait, say which one (Step 5) | From your notes |

Also paste the contents of `summary.txt` from each bundle folder if anything
in it says `WARNING` — those warnings tell us what the script could not
collect.

**Thank you.** Once we have these files we can see whether the engine covered
every monitor, at what coordinates, and with what Windows build and display
mode — which is exactly what the fix needs.
