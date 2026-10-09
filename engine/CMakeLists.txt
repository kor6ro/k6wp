# engine - resident wallpaper engine (exe). Real logic lands in Wave 1.
add_executable(engine
  src/main.cpp
  src/crash_dump_prune.cpp
  src/engine_app.cpp
  src/cli_options.cpp
  src/cpu_affinity.cpp
  src/occlusion_poke_scheduler.cpp
  src/pin_verify_schedule.cpp
  src/playlist_controller.cpp
  src/test_simulator.cpp
  src/tray_actions.cpp
  src/log_file.cpp
  src/os_wallpaper.cpp
  src/lockscreen_glue.cpp
  src/working_set_trim.cpp
  src/desktop_inject.cpp
  src/desktop_placement.cpp
  src/bench_hook.cpp
  src/mpv_renderer.cpp
  src/config_watch.cpp
  src/ipc_server.cpp
  src/ipc_marshal.cpp  # CRIT-2: worker-side set_video/set_monitor validators
  src/ipc_command_marshal.cpp  # CRIT-2: worker->main command queue
  src/fullscreen_watch.cpp
  src/occlusion_watch.cpp
  src/power.cpp
  src/multi_monitor.cpp
  src/gpu_pin.cpp
  src/tray.cpp
  src/timer_ids.hpp  # central timer-ID registry (pairwise-distinct asserts)
  app.rc  # Todo 9: app icon (IDI_APPICON/IDI_APPICON_PAUSED) + VERSIONINFO
)

# Todo 11: GUI subsystem — no console window. Entry point is wWinMain in
# src/main.cpp (UTF-8 argv conversion keeps ParseCli/pipe QA unchanged).
set_target_properties(engine PROPERTIES WIN32_EXECUTABLE TRUE)

# engine_app.cpp uses user32 directly (RegisterClassExW/CreateWindowExW/
# PostMessageW/SetConsoleCtrlHandler); k6wp_shared's PRIVATE user32 also
# propagates, but list it explicitly for direct usage.
# shell32: CommandLineToArgvW in src/main.cpp.
# wtsapi32: WTSRegisterSessionNotification for session-lock pause.
# dxgi: IDXGIFactory6 adapter pick for the P3L.3 d3d11-adapter pin.
target_link_libraries(engine PRIVATE k6wp_shared user32 shell32 wtsapi32 dxgi psapi)

# Vendored libmpv (Todo 2): headers + MSVC import lib. libmpv-2.dll is copied
# next to the exe by the POST_BUILD step below (mpv renderer, Todo 10).
# Mirrors the studio/CMakeLists.txt pattern.
target_include_directories(engine PRIVATE "${CMAKE_SOURCE_DIR}/vendor/libmpv/include")
# packaging/ is on the RC include path so app.rc can reference k6wp-on.ico /
# k6wp-off.ico by bare name (CMake feeds target include dirs to rc.exe via
# the INCLUDE env var).
target_include_directories(engine PRIVATE "${CMAKE_SOURCE_DIR}/packaging")
target_link_libraries(engine PRIVATE "${CMAKE_SOURCE_DIR}/vendor/libmpv/lib/mpv.lib")

add_custom_command(TARGET engine POST_BUILD
  COMMAND "${CMAKE_COMMAND}" -E copy_if_different
          "${CMAKE_SOURCE_DIR}/vendor/libmpv/bin/libmpv-2.dll"
          "$<TARGET_FILE_DIR:engine>/libmpv-2.dll"
  COMMENT "Copying libmpv-2.dll next to engine.exe"
  VERBATIM
)

# Embed the DPI manifest (PerMonitorV2, Todo 11). /MANIFESTINPUT merges our
# settings into the default manifest; /MANIFEST:EMBED writes it into the exe.
target_link_options(engine PRIVATE
  "/MANIFEST:EMBED"
  "/MANIFESTINPUT:${CMAKE_CURRENT_SOURCE_DIR}/app.manifest"
)

if(BUILD_TESTING)
# occlusion_test - unit tests for occlusion_watch pure helpers (P2.5 Todo 12).
# Compiles src/occlusion_watch.cpp standalone: only the free helpers
# (ClipRect/UnionAreaClipped/ComputeCoverage/HysteresisDecide) plus
# OnPauseMaskChanged are referenced, so no MultiMonitor/mpv link deps.
# user32: PostMessageW in OnPauseMaskChanged. k6wp_shared: monitor_util.hpp
# include path (PUBLIC) for MonitorInfo.
add_executable(occlusion_test ../tests/occlusion_test.cpp src/occlusion_watch.cpp)
target_link_libraries(occlusion_test PRIVATE k6wp_shared user32)
target_include_directories(occlusion_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
add_test(NAME occlusion_test COMMAND occlusion_test)

# ipc_marshal_test - unit tests for the PostMessage-marshal payload
# validators (CRIT-2, audit-remediation todo 11). Compiles
# src/ipc_marshal.cpp standalone: windows.h-free (nlohmann/json +
# <filesystem> only), so no user32/mpv link deps.
add_executable(ipc_marshal_test ../tests/ipc_marshal_test.cpp src/ipc_marshal.cpp)
target_link_libraries(ipc_marshal_test PRIVATE k6wp_shared)
target_include_directories(ipc_marshal_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
add_test(NAME ipc_marshal_test COMMAND ipc_marshal_test)

# gpu_pin_test (MED-2 slice A): pure-logic coverage — gpu_pin VendorId table
# incl. the documented RTX 2050 case (2050 fixture in the test source),
# version_compare golden table incl. MED-8 overflow anchors, Sha1Hex "abc"
# anchor, compressor CacheKeyHex determinism. The test #includes the REAL
# engine/src/gpu_pin.cpp (anonymous-namespace heuristic reachable, no header
# change); cache_manager.cpp is a separate windows.h-free source. dxgi: the
# included gpu_pin.cpp references CreateDXGIFactory1.
add_executable(gpu_pin_test ../tests/gpu_pin_test.cpp ../compressor/src/cache_manager.cpp)
target_link_libraries(gpu_pin_test PRIVATE k6wp_shared dxgi)
target_include_directories(gpu_pin_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/compressor/src)
add_test(NAME gpu_pin_test COMMAND gpu_pin_test)

# tray_menu_test - HMENU ownership of the tray quick-switch submenu
# (1.2.0 leak fix). The test #includes the REAL engine/src/tray.cpp so it
# drives the production AppendQuickSwitchSubmenu (anonymous namespace, cf.
# gpu_pin_test) and asserts USER-object handle balance via GetGuiResources
# with an empty MRU - the first-run case that leaked one menu handle per tray
# right-click. TrackPopupMenu is not called, so the suite is headless.
# log_file.cpp is linked for AppendEngineLogLine, which tray.cpp's Log()
# references; user32 for menu/USER APIs, shell32 for Shell_NotifyIconW.
add_executable(tray_menu_test ../tests/tray_menu_test.cpp src/log_file.cpp)
target_link_libraries(tray_menu_test PRIVATE user32 shell32)
target_include_directories(tray_menu_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
add_test(NAME tray_menu_test COMMAND tray_menu_test)

# crash_dump_prune_test - cap + ordering of %LOCALAPPDATA%\K6WP\crashes
# (1.2.0 leak fix). Compiles src/crash_dump_prune.cpp standalone (windows.h +
# cwchar only, no k6wp_shared dependency) and drives it against a throwaway
# %TEMP% subdirectory with synthetic mtimes, so the real crash directory is
# never touched. Covers the cap, the name tie-break, the non-fatal locked-file
# path, foreign-file safety and the bounded work budget.
add_executable(crash_dump_prune_test ../tests/crash_dump_prune_test.cpp src/crash_dump_prune.cpp)
target_include_directories(crash_dump_prune_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
add_test(NAME crash_dump_prune_test COMMAND crash_dump_prune_test)

# engine_units_test (P0-3): pure-logic coverage for the units split out of
# engine_app.cpp - cli_options (argv), PauseController (owner bitmask),
# PendingCommandQueue (worker->main handoff) and TestSimulator (QA schedule).
# Row 16 adds src/config_watch.cpp + src/log_file.cpp (the REAL ConfigWatcher
# second-file mtime/size + reload-callback path, driven with no message
# window) and links k6wp_shared (LoadConfig) + user32 (the watcher TU's
# SetTimer/KillTimer references). The test TU stays windows.h-free; no
# engine_app / mpv link deps.
add_executable(engine_units_test
  ../tests/engine_units_test.cpp
  src/cli_options.cpp
  src/test_simulator.cpp
  src/config_watch.cpp
  src/log_file.cpp
  # H3 (1.3.0-beta.2): DecidePowerCapAction (battery static/cap24 split) is
  # pure and unit-tested here; power.cpp carries no mpv/engine_app deps.
  src/power.cpp
)
target_link_libraries(engine_units_test PRIVATE k6wp_shared user32)
target_include_directories(engine_units_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
add_test(NAME engine_units_test COMMAND engine_units_test)

# engine_state_test (P0-2): the extracted PinVerifySchedule deadline/counter and
# PlaylistController reload/snapshot/rotation, driven through fake hooks.
# Row 17: src/multi_monitor.cpp is compiled in so the boot-time assignment
# convergence checks drive the REAL MultiMonitor against link-level
# DesktopInjector / MpvRenderer / ListMonitors stubs in the test TU
# (multi_monitor_factory_test pattern; row 12 verified k6wp_shared +
# TU stubs coexist - displays_schema.obj does not pull monitor_util.obj).
# Still mpv-free: no mpv_renderer.cpp, no mpv.lib.
add_executable(engine_state_test
  ../tests/engine_state_test.cpp
  src/pin_verify_schedule.cpp
  src/playlist_controller.cpp
  src/occlusion_poke_scheduler.cpp
  src/ipc_command_marshal.cpp
  src/ipc_marshal.cpp
  src/multi_monitor.cpp
)
target_link_libraries(engine_state_test PRIVATE k6wp_shared user32)
target_include_directories(engine_state_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
add_test(NAME engine_state_test COMMAND engine_state_test)

# desktop_placement_test (multi-monitor extend setup, row 3): pure geometry
# helpers extracted from DesktopInjector::CreateAndAttach (the :261-265
# inline offset math). windows.h-free; no engine/mpv link deps.
add_executable(desktop_placement_test
  ../tests/desktop_placement_test.cpp
  src/desktop_placement.cpp
)
target_include_directories(desktop_placement_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
add_test(NAME desktop_placement_test COMMAND desktop_placement_test)

# multi_monitor_factory_test (multi-monitor extend setup, row 6): the
# injectable SlotFactory seam. Compiles src/multi_monitor.cpp standalone
# against link-level DesktopInjector / MpvRenderer / ListMonitors stubs
# defined in the test TU (tests/occlusion_test.cpp pattern), proving a
# headless MultiMonitor links WITHOUT desktop_inject.cpp, mpv_renderer.cpp
# or mpv.lib - target_link_libraries below must never grow an mpv entry.
# user32: GetSystemMetrics in MultiMonitor::GetSpanGeometry.
# Row 9: src/desktop_placement.cpp is linked so the false-success test can
# drive the REAL CoversMonitor/CoverageReason on screen-space rects; still
# no mpv, still no desktop_inject.cpp.
# Row 33: k6wp_shared added so multi_monitor.cpp's row-33 gates
# (DetectKeyCollisionForCommand / DropCollidingAssignments -> REAL
# DetectKeyCollision, shared/displays_schema) resolve at link — same reason
# placement_test links k6wp_shared (row 12 comment).
add_executable(multi_monitor_factory_test
  ../tests/multi_monitor_factory_test.cpp
  src/multi_monitor.cpp
  src/desktop_placement.cpp
)
target_link_libraries(multi_monitor_factory_test PRIVATE user32 k6wp_shared)
target_include_directories(multi_monitor_factory_test PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/shared
)
add_test(NAME multi_monitor_factory_test COMMAND multi_monitor_factory_test)

# workerw_span_test (multi-monitor extend setup, row 10): the strategy-B
# span gate. Pure containment k6wp::WorkerWSpansRect (desktop_placement,
# windows.h-free, synthetic rects) plus the thin HWND wrapper
# k6wp::WorkerWSpansVirtualScreen from desktop_inject.cpp (GetWindowRect +
# the four SM_*VIRTUALSCREEN metrics), incl. the nullptr -> false no-crash
# guard. user32: window/metric APIs in desktop_inject.cpp; gdi32:
# GetStockObject in its RegisterClass path.
add_executable(workerw_span_test
  ../tests/workerw_span_test.cpp
  src/desktop_inject.cpp
  src/desktop_placement.cpp
)
target_link_libraries(workerw_span_test PRIVATE user32 gdi32)
target_include_directories(workerw_span_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
add_test(NAME workerw_span_test COMMAND workerw_span_test)

# desktop_zorder_test (multi-monitor extend setup, row 8): the ONE z-order
# contract across injection branches. Drives the REAL
# k6wp::ResolveZOrderInsertAfter from desktop_inject.cpp (the GetParent
# sibling guard, k6wp linkage per the row-10 pattern; desktop_inject.hpp
# stays windows.h-free) over a hidden parent/child hierarchy: the DefView
# handle is returned ONLY when GetParent(DefView)==GetParent(target);
# anything else falls back to HWND_BOTTOM with reason
# "z-order: sibling-mismatch-fallback". user32/gdi32 mirror
# workerw_span_test (same desktop_inject.cpp link closure).
add_executable(desktop_zorder_test
  ../tests/desktop_zorder_test.cpp
  src/desktop_inject.cpp
  src/desktop_placement.cpp
)
target_link_libraries(desktop_zorder_test PRIVATE user32 gdi32)
target_include_directories(desktop_zorder_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
add_test(NAME desktop_zorder_test COMMAND desktop_zorder_test)

# multi_monitor_placement_test (multi-monitor extend setup, row 12): the
# placement matrix - 1/2/3 monitors, negative origins (left/above primary),
# portrait secondary, duplicate mode (shared physical rect) and mixed-DPI.
# Same link model as multi_monitor_factory_test (row 6): src/multi_monitor.cpp
# + src/desktop_placement.cpp against TU-level DesktopInjector / MpvRenderer /
# ListMonitors stubs, so the binary never links desktop_inject.cpp,
# mpv_renderer.cpp or mpv.lib - target_link_libraries below must never grow
# an mpv entry. k6wp_shared is linked ONLY for DetectKeyCollision (row 5,
# shared/displays_schema.hpp); the TU still owns ListMonitors /
# GetPrimaryMonitor so monitor_util.obj is never pulled. user32:
# GetSystemMetrics in MultiMonitor::GetSpanGeometry.
add_executable(multi_monitor_placement_test
  ../tests/multi_monitor_placement_test.cpp
  src/multi_monitor.cpp
  src/desktop_placement.cpp
)
target_link_libraries(multi_monitor_placement_test PRIVATE user32 k6wp_shared)
target_include_directories(multi_monitor_placement_test PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/shared
)
add_test(NAME multi_monitor_placement_test COMMAND multi_monitor_placement_test)

# display_assignment_test (multi-monitor extend setup, row 18): the full
# per-monitor assignment lifecycle over the fake-pipe pattern
# (tests/fake_pipe_test.cpp / studio_logic_test SyncMonitor dedup). Cases
# (a)-(f): assign->get_state map, clear->default revert, unknown-device
# reject, missing-path degraded coverage + reject, IS-7 collision refusal
# via the REAL DetectKeyCollision, old-engine ParseEngineState defaults.
# Link model (engine_state_test row-17 pattern): src/multi_monitor.cpp +
# src/ipc_marshal.cpp compiled in; DesktopInjector / MpvRenderer /
# ListMonitors stubs live in the test TU, so the binary never links
# desktop_inject.cpp, mpv_renderer.cpp or mpv.lib. studio/src/ipc_client.cpp
# is compiled in for the REAL IpcClient (Qt-free, fake_pipe_test pattern).
# k6wp_shared: displays_schema (Save/Load/DetectKeyCollision) +
# ipc_protocol (ParseEngineState). user32: GetSystemMetrics in
# multi_monitor.cpp.
add_executable(display_assignment_test
  ../tests/display_assignment_test.cpp
  ../studio/src/ipc_client.cpp
  src/ipc_marshal.cpp
  src/multi_monitor.cpp
)
target_link_libraries(display_assignment_test PRIVATE k6wp_shared user32)
target_include_directories(display_assignment_test PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/studio/src
)
add_test(NAME display_assignment_test COMMAND display_assignment_test)
endif()
