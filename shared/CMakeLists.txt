# k6wp_shared - static library with the cross-process contracts:
# config schema, IPC protocol, monitor utils.
add_library(k6wp_shared STATIC
  config_schema.cpp
  studio_settings.cpp
  ipc_protocol.cpp
  monitor_util.cpp
  autostart.cpp
  lockscreen.cpp
  ffmpeg_path.cpp
  proc_util.cpp
  sha1.cpp
  compress_args.cpp
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
endif()