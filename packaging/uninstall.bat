@echo off
rem K6WP uninstall helper - portable-ZIP copy (Task 28).
rem Dev copy lives at tools\uninstall.bat (Todo 36); alignment noted in evidence.
rem Overlap note (Task 27): dist\K6WP-Setup.exe (packaging\installer.nsi)
rem Section "Uninstall" covers full removal (verified artifact, do not touch):
rem K6WP.exe --stop, Run value, per-user wallpaper/lock-screen restore from
rem %LOCALAPPDATA%\K6WP\original_*, legacy HKLM policy cleanup, app files,
rem shortcuts, HKCU uninstall key, keep/remove data offer. This .bat is the
rem portable-ZIP helper with the same keep/remove data semantics, aligned as
rem far as batch can.
rem Process stop (K6WP.exe --stop, not taskkill /IM): an image name is not
rem unique on a developer or IT machine, so `taskkill /IM engine.exe` used to
rem kill EVERY engine.exe on the box, including another app's. --stop asks the
rem engine to quit over its own IPC pipe first (so its graceful shutdown runs
rem and restores the OS wallpaper), then escalates to a per-PID kill, and only
rem for images sitting next to THIS K6WP.exe. A colliding engine.exe or
rem studio.exe in another folder is never touched.
rem Graceful-first rationale: the EngineApp message window handles WM_CLOSE by
rem calling RequestShutdown() (engine/src/engine_app.cpp), which posts WM_QUIT
rem so Run() breaks out and calls Shutdown(): ShutdownWallpaperSurface() plus
rem RequestStillRestore() (detached K6WP.exe --apply-still --restore, which
rem reads the persistent original_wallpaper.json / original_lockscreen.*
rem stores), plus tray icon Remove() and a forced kill skips all of that.
rem LIVE-TEST NOTE (Task 28, still valid): with only its hidden top-level
rem message window present, build\release\engine.exe did not honour the
rem WM_CLOSE that `taskkill /IM` sends, so the forced fallback did the
rem termination and no in-engine wallpaper restore ran that round. --stop posts
rem WM_CLOSE to the process's real window (taskkill matches a window by TITLE,
rem which a hidden "K6WP Engine" window does not have) and keeps the forced
rem per-PID fallback for anything that survives the grace period, so the
rem guaranteed restore for installed copies is still the NSIS
rem registry-backup path, which batch cannot do without new dependencies.
rem No pixel-level wallpaper verification is claimed here (headless sessions
rem cannot verify pixels); transcripts prove process stop + Run-key absence +
rem exit codes only.
rem The former `ping -n 4` inter-task wait now lives inside K6WP.exe as a real
rem WaitForSingleObject on each process handle (ping, not timeout, was the
rem house rule there because timeout aborts when stdin is redirected, e.g.
rem `echo N | uninstall.bat`; the wait is no longer in this script).
rem Fallback when K6WP.exe is gone (deleted folder / partial extract): we do NOT
rem fall back to a machine-wide kill - that is the bug, not the fix. The script
rem says which windows to close by hand instead.
rem HKCU needs no admin rights.
setlocal

echo Stopping K6WP processes (IPC quit first, then per-PID)...
if exist "%~dp0K6WP.exe" goto stop_run
echo K6WP.exe not found next to this script - cannot stop processes safely.
echo Close engine.exe and Studio by hand, then run this script again.
goto stop_done
:stop_run
"%~dp0K6WP.exe" --stop
if errorlevel 1 goto stop_failed
echo K6WP processes stopped (not-running is fine - already clean).
goto stop_done
:stop_failed
echo WARNING: some K6WP processes could not be stopped.
echo Close engine.exe and Studio by hand, then run this script again.
:stop_done

echo K6WP uninstall: removing autostart entry...
reg delete "HKCU\Software\Microsoft\Windows\CurrentVersion\Run" /v K6WP /f
if %ERRORLEVEL% EQU 0 (
  echo Autostart entry removed.
) else (
  echo Autostart entry was not present - already clean.
)

echo Restoring your pre-K6WP desktop wallpaper and lock-screen image (per-user, no admin)...
if exist "%~dp0K6WP.exe" goto still_run
echo K6WP.exe not found next to this script - skipping the per-user restore.
echo To restore manually, run: K6WP.exe --apply-still --restore
goto still_done
:still_run
rem No UAC: the per-user still helper restores the persistent
rem original_wallpaper.json / original_lockscreen.* stores. A missing store is
rem a successful no-op, so this is safe to run on a machine that never used
rem the feature.
"%~dp0K6WP.exe" --apply-still --restore
if errorlevel 1 echo WARNING: the wallpaper/lock-screen restore did not complete. Run K6WP.exe --apply-still --restore again before deleting K6WP.
:still_done

echo Removing a leftover legacy lockscreen policy (only if an old K6WP left one)...
echo This needs administrator rights - a UAC prompt will appear. Declining keeps the legacy policy.
if exist "%~dp0K6WP.exe" goto lock_run
echo K6WP.exe not found next to this script - skipping the legacy cleanup.
echo To clean up manually, run: K6WP.exe --elevate-lockscreen off (as administrator).
goto lock_done
:lock_run
rem Exit codes from K6WP.exe: 0 cleaned, 5 UAC declined, 4 wait budget
rem expired, other = the elevated helper failed. `if errorlevel N` is read at
rem run time, so these tests see the helper's real code.
"%~dp0K6WP.exe" --elevate-lockscreen off
if errorlevel 5 goto lock_declined
if errorlevel 1 goto lock_failed
goto lock_done
:lock_declined
echo.
echo WARNING: the administrator prompt was declined, so a leftover legacy lockscreen policy was NOT removed.
echo It may still point at %%PROGRAMDATA%%\K6WP\lockscreen.jpg in
echo HKLM\SOFTWARE\Policies\Microsoft\Windows\Personalization\LockScreenImage.
echo To clean it up manually, run: K6WP.exe --elevate-lockscreen off (as administrator).
goto lock_done
:lock_failed
echo.
echo WARNING: K6WP could not remove the legacy lockscreen policy ^(the elevated helper failed or ran out of time^).
echo It may still point at %%PROGRAMDATA%%\K6WP\lockscreen.jpg in
echo HKLM\SOFTWARE\Policies\Microsoft\Windows\Personalization\LockScreenImage.
echo To clean it up manually, run: K6WP.exe --elevate-lockscreen off (as administrator).
:lock_done

set /p CLEAN="Also delete K6WP cache and library data in %%LOCALAPPDATA%%\K6WP? [y/N] "
if /i "%CLEAN%"=="y" (
  if exist "%LOCALAPPDATA%\K6WP" (
    rmdir /s /q "%LOCALAPPDATA%\K6WP"
    echo K6WP data deleted.
  ) else (
    echo No K6WP data folder found - already clean.
  )
) else (
  echo Keeping K6WP data.
)

echo Done.
echo To finish, delete this portable folder itself.
endlocal
