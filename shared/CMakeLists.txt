# k6wp_shared - static library with the cross-process contracts:
# config schema, IPC protocol, monitor utils.
add_library(k6wp_shared STATIC
  config_schema.cpp
  studio_settings.cpp
  playlist.cpp
  displays_schema.cpp
  ipc_protocol.cpp
  monitor_util.cpp
  autostart.cpp
  still.cpp
  legacy_lockscreen_policy.cpp
  ffmpeg_path.cpp
  proc_util.cpp
  sha1.cpp
  compress_args.cpp
  settings_io.cpp
)

target_include_directories(k6wp_shared PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_features(k6wp_shared PUBLIC cxx_std_17)

# monitor_util uses EnumDisplayMonitors/GetMonitorInfoW/EnumDisplaySettingsExW
# (user32). PRIVATE is fine: static-lib link deps propagate to consumers, and
# the header exposes no Win32 types.
target_link_libraries(k6wp_shared PRIVATE user32)

# ipc_protocol uses Win32 ACL APIs (InitializeAcl/AddAccessAllowedAce/
# GetTokenInformation) from advapi32. PRIVATE is fine: static-lib link deps
# propagate to consumers.
target_link_libraries(k6wp_shared PRIVATE advapi32)

# monitor_util uses GetScaleFactorForMonitor (Shcore.dll, a Windows system DLL,
# not a third-party library). PRIVATE is fine for the same reason.
target_link_libraries(k6wp_shared PRIVATE Shcore)

# monitor_dump - prints the monitor list as JSON (QA tool for Todo 7).
# Hidden from distribution builds; enable with -DBUILD_TESTING=ON
# (msvc-dev preset default) or -DK6WP_BUILD_TOOLS=ON.
if(K6WP_BUILD_TOOLS OR BUILD_TESTING)
add_executable(monitor_dump monitor_dump.cpp monitor_dump.rc)
target_link_libraries(monitor_dump PRIVATE k6wp_shared)
# packaging/ is on the RC include path so monitor_dump.rc can reference
# k6wp-on.ico by bare name and #include "resource.h" (same pattern as
# engine/launcher/compressor app.rc files).
target_include_directories(monitor_dump PRIVATE "${CMAKE_SOURCE_DIR}/packaging")
endif()

if(BUILD_TESTING)
# golden_dump - Phase 0 task 0.6: freeze the pure shared/ behaviour as a
# golden JSON document a Rust port can replay. NOT a test binary: it writes
# (or checks) tests/golden/shared_behavior.json.
add_executable(golden_dump ../tests/golden_dump.cpp)
target_link_libraries(golden_dump PRIVATE k6wp_shared)
target_include_directories(golden_dump PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# config_test - mini unit tests for the config schema (Todo 5). argv[1]
# carries config_schema.json so the schema-doc sync guard can run.
add_executable(config_test ../tests/config_test.cpp)
target_link_libraries(config_test PRIVATE k6wp_shared)
target_include_directories(config_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME config_test COMMAND config_test
         "${CMAKE_CURRENT_SOURCE_DIR}/config_schema.json")

# ipc_test - mini unit tests for the NDJSON IPC protocol (Todo 6).
add_executable(ipc_test ../tests/ipc_test.cpp)
target_link_libraries(ipc_test PRIVATE k6wp_shared)
target_include_directories(ipc_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME ipc_test COMMAND ipc_test)

# sha1_test - SHA1 vector tests (Todo 25, MED-3).
add_executable(sha1_test ../tests/sha1_test.cpp)
target_link_libraries(sha1_test PRIVATE k6wp_shared)
target_include_directories(sha1_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME sha1_test COMMAND sha1_test)

# proc_util_test - RunCaptured helper tests (Todo 16, MED-4).
add_executable(proc_util_test ../tests/proc_util_test.cpp)
target_link_libraries(proc_util_test PRIVATE k6wp_shared)
target_include_directories(proc_util_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME proc_util_test COMMAND proc_util_test)

# playlist_test - wallpaper playlist + rotation contract (io, validation,
# migration, .bak, SelectNextIndex / PlaylistIndexForPath pure helpers).
add_executable(playlist_test ../tests/playlist_test.cpp)
target_link_libraries(playlist_test PRIVATE k6wp_shared)
target_include_directories(playlist_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME playlist_test COMMAND playlist_test)

# displays_schema_test - per-monitor assignment store (displays.json io,
# validation, migration, .bak contract, DetectKeyCollision IS-7 rules).
add_executable(displays_schema_test ../tests/displays_schema_test.cpp)
target_link_libraries(displays_schema_test PRIVATE k6wp_shared)
target_include_directories(displays_schema_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME displays_schema_test COMMAND displays_schema_test)

# monitor_util_test - MonitorInfo orientation/refresh/scale plus the
# IsPortrait/ScalePercent pure helpers and the scale fallback seam.
add_executable(monitor_util_test ../tests/monitor_util_test.cpp)
target_link_libraries(monitor_util_test PRIVATE k6wp_shared)
target_include_directories(monitor_util_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME monitor_util_test COMMAND monitor_util_test)

# stable_key_test - P2a tiered monitor identity (AssignedIdentity /
# AssignStableKeys pure seam): tier precedence ccd->edid->gdi, uniqueness,
# and the degraded/ambiguous fallback. Headless (no CCD/registry).
add_executable(stable_key_test ../tests/stable_key_test.cpp)
target_link_libraries(stable_key_test PRIVATE k6wp_shared)
target_include_directories(stable_key_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME stable_key_test COMMAND stable_key_test)

# still_test: the per-user still contract (shared/still.cpp) - paths and the
# J1 IsStillPath predicate, the J8 refresh policy, the "WxH" parser, the
# original wallpaper / lock-screen / status JSON stores, the helper spawn
# seam, and the legacy HKLM policy cleanup classification + seam
# (shared/legacy_lockscreen_policy.cpp).
add_executable(still_test ../tests/still_test.cpp)
target_link_libraries(still_test PRIVATE k6wp_shared)
target_include_directories(still_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME still_test COMMAND still_test)

# golden_test - CI gate for the frozen shared/ behaviour. Regenerates the
# golden document and fails when it drifts from the committed copy, so a
# behaviour change in shared/ can never land unnoticed (the Rust port
# replays the SAME file in Phase 2/3).
add_test(NAME golden_test COMMAND golden_dump --check
         "${CMAKE_SOURCE_DIR}/tests/golden/shared_behavior.json")
endif()