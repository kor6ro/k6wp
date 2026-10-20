# launcher - Qt-free Win32 dispatcher producing K6WP.exe (Todo 7).
#
# Owner decision F1: dispatcher (not unified binary) keeps the resident engine
# free of Qt import cost (RAM budget < 80MB). This target must NEVER link Qt6:
# prove with `dumpbin /dependents K6WP.exe` (no Qt6*.dll).
#
# K6WP.exe also carries the one-shot `--apply-still` helper (still_apply.cpp):
# the only place that touches WinRT. It runs as a short-lived child, never as
# part of the resident engine.
#
# Todo 11 (done): /SUBSYSTEM:WINDOWS via WIN32_EXECUTABLE + wWinMain entry
# point, no console. main.cpp stays console-clean either way.
add_executable(launcher main.cpp cli.cpp process_paths.cpp process_stop.cpp lockscreen_backup.cpp lockscreen_policy.cpp lockscreen_sync.cpp still_apply.cpp app.rc)

# Todo 11: GUI subsystem — no console window. Entry point is wWinMain in
# main.cpp (CommandLineToArgvW keeps the Run(argc, argv) contract).
set_target_properties(launcher PROPERTIES WIN32_EXECUTABLE TRUE)

# packaging/ is on the RC include path so app.rc can reference k6wp-on.ico by
# bare name (same pattern as engine/app.rc).

# Binary contract (Todo 8/10 build on it): ./K6WP.exe next to
# engine.exe / studio.exe in the flattened output dir.
set_target_properties(launcher PROPERTIES OUTPUT_NAME "K6WP")

# C++/WinRT (still_apply.cpp) uses the C++20 coroutine machinery; the C++17
# /await path is deprecated in the VS 18 STL and fails with STL1011. C++20 is
# required for the helper (and only the helper) - the engine/studio stay 17.
target_compile_features(launcher PRIVATE cxx_std_20)

target_include_directories(launcher PRIVATE "${CMAKE_SOURCE_DIR}/packaging")
target_include_directories(launcher PRIVATE "${CMAKE_SOURCE_DIR}/shared")

# Plain Win32 (user32: mutex/window focus/wallpaper; shell32:
# CommandLineToArgvW + ShellExecuteW runas; advapi32: registry for the legacy
# policy cleanup).
#
# k6wp_shared: the still helper reuses the Studio settings parser, the original
# wallpaper/lock-screen stores and the status JSON (shared/still.cpp). It pulls
# no Qt (it is a static lib over nlohmann/json + Win32).
#
# windowsapp.lib + ole32: the C++/WinRT LockScreen API (header-only, Windows
# SDK only - no new third-party dependency). They are linked into this helper
# process ONLY; tests/engine_no_winrt_test.cpp proves engine.exe stays clean.
target_link_libraries(launcher PRIVATE k6wp_shared user32 shell32 advapi32
                                     windowsapp ole32 uuid)

# lockscreen_backup_test (HIGH-2 round-trip, merged into CTest by MED-2
# slice B): the test #includes the REAL main.cpp (anonymous-namespace
# escape/decode reachable; wWinMain is compiled but unused — this target
# stays console-subsystem so the test's own main() is the entry point and
# the launcher build above is untouched). Same Win32 libs as the launcher.
if(BUILD_TESTING)
add_executable(lockscreen_backup_test ../tests/lockscreen_backup_test.cpp lockscreen_backup.cpp process_paths.cpp process_stop.cpp cli.cpp lockscreen_policy.cpp lockscreen_sync.cpp still_apply.cpp)
target_link_libraries(lockscreen_backup_test PRIVATE k6wp_shared user32 shell32 advapi32 windowsapp ole32 uuid)
target_compile_features(lockscreen_backup_test PRIVATE cxx_std_20)
target_include_directories(lockscreen_backup_test PRIVATE "${CMAKE_SOURCE_DIR}/shared")
# The test spells CJK/accent paths as literal UTF-8 wide strings; without
# /utf-8 MSVC decodes the BOM-less source per the system ANSI code page and
# those literals (but not the \U escapes) come out wrong. Test-target-only
# flag — the launcher product build above is untouched.
target_compile_options(lockscreen_backup_test PRIVATE /utf-8)
add_test(NAME lockscreen_backup_test COMMAND lockscreen_backup_test)

# launcher_stop_test (1.2.0 uninstall fixes): locks the two pure decision
# layers behind the uninstall paths — which processes the uninstaller may stop
# (directory-scoped, never image-name) and how the bounded elevated-worker wait
# maps to an exit code + user message. Same include-the-real-.cpp pattern as
# lockscreen_backup_test above (anonymous-namespace escape, console subsystem).
if(BUILD_TESTING)
  add_executable(launcher_stop_test ../tests/launcher_stop_test.cpp lockscreen_backup.cpp process_paths.cpp process_stop.cpp cli.cpp lockscreen_policy.cpp lockscreen_sync.cpp still_apply.cpp)
  target_link_libraries(launcher_stop_test PRIVATE k6wp_shared user32 shell32 advapi32 windowsapp ole32 uuid)
  target_compile_features(launcher_stop_test PRIVATE cxx_std_20)
  target_include_directories(launcher_stop_test PRIVATE "${CMAKE_SOURCE_DIR}/shared")
  target_compile_options(launcher_stop_test PRIVATE /utf-8)
  add_test(NAME launcher_stop_test COMMAND launcher_stop_test)
endif()
endif()
