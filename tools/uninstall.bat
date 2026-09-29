@echo off
rem K6WP uninstall helper (Todo 36): removes the HKCU Run autostart entry
rem plus, on confirmation, the per-user data under %LOCALAPPDATA%\K6WP.
rem HKCU needs no admin rights.
setlocal

rem Stop the resident processes BEFORE touching the data folder. Without this,
rem a still-running engine rewrites config.json immediately after the rmdir
rem below, so the folder reappears and the "data deleted" message is a lie.
rem Mirrors packaging\uninstall.bat, and uses K6WP.exe --stop rather than
rem `taskkill /IM`: an image name is not unique on a developer or IT machine,
rem so a name-based kill takes down other people's engine.exe / studio.exe.
rem --stop sends the engine an IPC quit (graceful, so its WM_CLOSE path runs
rem and restores the OS wallpaper), then escalates to a per-PID kill, and only
rem for images next to THIS K6WP.exe.
set "K6WPEXE=%~dp0K6WP.exe"
if not exist "%K6WPEXE%" set "K6WPEXE=%~dp0..\build\release\K6WP.exe"
echo Stopping K6WP processes (IPC quit first, then per-PID)...
if exist "%K6WPEXE%" goto stop_run
echo K6WP.exe not found - cannot stop processes safely.
echo Close engine.exe and Studio by hand, then run this script again.
goto stop_done
:stop_run
"%K6WPEXE%" --stop
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

echo Restoring lockscreen policy backup (if lockscreen sync was ever turned on)...
echo This needs administrator rights - a UAC prompt will appear. Declining keeps the current lockscreen policy.
if exist "%K6WPEXE%" goto lock_run
echo K6WP.exe not found - skipping lockscreen restore.
echo To restore manually, run: K6WP.exe --elevate-lockscreen off (as administrator).
goto lock_done
:lock_run
rem Exit codes from K6WP.exe: 0 restored, 5 UAC declined, 4 wait budget
rem expired, other = the elevated helper failed. `if errorlevel N` is read at
rem run time, so these tests see the helper's real code.
"%K6WPEXE%" --elevate-lockscreen off
if errorlevel 5 goto lock_declined
if errorlevel 1 goto lock_failed
goto lock_done
:lock_declined
echo.
echo WARNING: the administrator prompt was declined, so the lockscreen policy was NOT restored.
echo It may still point at %%PROGRAMDATA%%\K6WP\lockscreen.jpg in
echo HKLM\SOFTWARE\Policies\Microsoft\Windows\Personalization\LockScreenImage.
echo That file is not part of this uninstall, so your lockscreen stays on that image.
echo To restore manually, run: K6WP.exe --elevate-lockscreen off (as administrator).
goto lock_done
:lock_failed
echo.
echo WARNING: K6WP could not restore the lockscreen policy (the elevated helper failed or ran out of time).
echo It may still point at %%PROGRAMDATA%%\K6WP\lockscreen.jpg in
echo HKLM\SOFTWARE\Policies\Microsoft\Windows\Personalization\LockScreenImage.
echo That file is not part of this uninstall, so your lockscreen stays on that image.
echo To restore manually, run: K6WP.exe --elevate-lockscreen off (as administrator).
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
endlocal
