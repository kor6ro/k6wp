cmake_minimum_required(VERSION 3.25)

project(k6wp VERSION 1.3.0 LANGUAGES CXX)

# Version used across the project (engine / studio / compressor).
set(K6WP_VERSION "1.3.0")

# --- Single-source version (todo 8 / MED-14) ---------------------------------
# K6WP_VERSION is canonical. configure_file() stamps it into
# build/<cfg>/generated/version.h (C/C++/RC consumers), packaging/version.nsh
# (NSIS installer) and engine/app.manifest (in-place, XML). Every consumer
# references these generated files — no hand-synced version literals anywhere.
string(REPLACE "." ";" K6WP_VERSION_PARTS "${K6WP_VERSION}")
list(GET K6WP_VERSION_PARTS 0 K6WP_VERSION_MAJOR)
list(GET K6WP_VERSION_PARTS 1 K6WP_VERSION_MINOR)
list(GET K6WP_VERSION_PARTS 2 K6WP_VERSION_PATCH)
set(K6WP_VERSION_4 "${K6WP_VERSION}.0")

set(K6WP_GENERATED_DIR "${CMAKE_BINARY_DIR}/generated")
configure_file(version.h.in "${K6WP_GENERATED_DIR}/version.h" @ONLY)
configure_file(version.nsh.in "${CMAKE_SOURCE_DIR}/packaging/version.nsh" @ONLY)
configure_file(engine/app.manifest.in "${CMAKE_SOURCE_DIR}/engine/app.manifest" @ONLY)
# Every target (incl. rc.exe via the INCLUDE env var) can #include "version.h".
include_directories("${K6WP_GENERATED_DIR}")

# --- Build options -----------------------------------------------------------
# Distribution default: QA/test targets OFF so a plain release configure
# builds ONLY the 4 main exes (+ shared lib). Developers use the msvc-dev
# preset (BUILD_TESTING=ON) to get config_test/ipc_test + ctest.
option(BUILD_TESTING "Build test targets (config_test, ipc_test) + enable ctest" OFF)
# QA helper tools (monitor_dump). Defaults to BUILD_TESTING, so
# -DBUILD_TESTING=ON also brings monitor_dump; override independently with
# -DK6WP_BUILD_TOOLS=ON/OFF when needed.
option(K6WP_BUILD_TOOLS "Build QA helper tools (monitor_dump)" ${BUILD_TESTING})

# Must precede add_subdirectory: enables testing at the root so add_test()
# calls in shared/CMakeLists.txt register with ctest. No-op when
# BUILD_TESTING=OFF (distribution default).
include(CTest)

# --- Task 23: production logging verbosity (T24: preserve this block) ---------
# Default OFF: all Log()/diagnostic sinks write to their file sink only
# (engine.log, or the Studio helper's return value); no stdout/stderr mirrors.
# -DK6WP_VERBOSE=ON restores the debug console mirrors for local diagnosis.
# POSITION MATTERS: this must stay BEFORE add_subdirectory() below —
# add_compile_definitions() only applies to targets defined after it.
option(K6WP_VERBOSE "Mirror Log()/diagnostic lines to stdout/stderr (default OFF: file-only production logging)" OFF)
if(K6WP_VERBOSE)
  add_compile_definitions(K6WP_VERBOSE=1)
endif()

# --- Global C++ settings -----------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
  add_compile_options(/W4 /WX- /permissive-)
  # P4.3: Release-only size/speed flags (generator expressions — Debug /
  # msvc-dev builds are untouched). /GL needs /LTCG at link time, so both
  # are set together; /Gy+/Gw feed /OPT:REF+/OPT:ICF dead-stripping.
  add_compile_options(
    "$<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Release>>:/GL>"
    "$<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Release>>:/Gy>"
    "$<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Release>>:/Gw>"
  )
  add_link_options(
    "$<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Release>>:/LTCG>"
    "$<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Release>>:/OPT:REF>"
    "$<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Release>>:/OPT:ICF>"
  )
endif()

# --- Output layout -----------------------------------------------------------
# Multi-config generators (Visual Studio) place binaries in a per-config
# subdir (build/msvc-dev/Debug/). Flatten so the stub exes land directly in
# build/msvc-dev/ as the plan's acceptance criteria require.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
foreach(_k6wp_cfg IN ITEMS DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${_k6wp_cfg} "${CMAKE_BINARY_DIR}")
endforeach()
unset(_k6wp_cfg)

# --- Subprojects -------------------------------------------------------------
# NOTE (Task 23, T24: preserve): the K6WP_VERBOSE option block lives in
# "Build options" above on purpose — option() + add_compile_definitions()
# must precede these add_subdirectory() calls or the definition never
# reaches the engine/studio/compressor targets (verified: appended-at-end
# produced a silent no-op build).
add_subdirectory(shared)      # static lib k6wp_shared (config/ipc/monitor contracts)
add_subdirectory(engine)      # resident wallpaper engine (exe)
add_subdirectory(compressor)  # ffmpeg compress CLI (exe)
add_subdirectory(studio)      # Qt6 Widgets studio UI (exe)
add_subdirectory(launcher)    # Qt-free Win32 dispatcher K6WP.exe (Todo 7)