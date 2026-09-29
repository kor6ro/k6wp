# compressor - ffmpeg compress CLI (exe).
add_executable(compressor src/main.cpp src/cli.cpp src/encoder_detect.cpp src/ffmpeg_job.cpp src/cache_manager.cpp src/video_probe.cpp app.rc)

target_include_directories(compressor PRIVATE "${CMAKE_SOURCE_DIR}/packaging")
target_link_libraries(compressor PRIVATE k6wp_shared)

# probe_json_test (MED-9): unit tests for the single-JSON probe parser
# (order-independence, corrupt-input safety) + a --probe <video> QA mode
# that runs the real single-spawn ProbeVideoProps.
if(BUILD_TESTING)
add_executable(probe_json_test tests/probe_json_test.cpp src/video_probe.cpp)
target_link_libraries(probe_json_test PRIVATE k6wp_shared)
target_include_directories(probe_json_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
add_test(NAME probe_json_test COMMAND probe_json_test)

# compress_argv_contract_test (MED-15): golden Studio<->compressor argv
# contract — builds argv from shared/compress_args.hpp (the same builder
# StartNext uses) and replays it against the real compressor.exe --dry-run
# parser. Registered HERE (compressor scope, not root tests/ scope) so
# parallel tasks adding test targets do not conflict on one CMakeLists.
# src/cli.cpp is compiled in directly (the MED-9 probe_json_test pattern) so
# the suite can unit-test the pure PathsReferToSameFile guard; every argv it
# builds is still replayed against the real binary via --dry-run.
add_executable(compress_argv_contract_test tests/compress_argv_contract_test.cpp src/cli.cpp)
target_link_libraries(compress_argv_contract_test PRIVATE k6wp_shared)
add_dependencies(compress_argv_contract_test compressor)
add_test(NAME compress_argv_contract COMMAND compress_argv_contract_test)
endif()