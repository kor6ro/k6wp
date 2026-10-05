# studio - Qt6 Widgets UI (exe). Qt is REQUIRED; it is installed via
# aqtinstall (see QT_ROOT env var). Wave 3: main window shell (Todo 20).
# Fail fast with a clear diagnostic when Qt is absent instead of raw
# find_package spew: install Qt 6 and point QT_ROOT / CMAKE_PREFIX_PATH at it
# (the msvc-dev/release presets do this via $env{QT_ROOT} automatically).
# P4.2: Network dropped — the update check is WinHTTP now (winhttp.lib).
find_package(Qt6 QUIET COMPONENTS Widgets Concurrent Quick Qml QuickControls2 QuickWidgets)
if(NOT Qt6_FOUND)
  message(FATAL_ERROR "Qt6 Widgets/Quick/Qml/QuickControls2/QuickWidgets/Concurrent not found. Install Qt 6 (aqtinstall, INCLUDING the qtdeclarative archive that carries the QML/Quick runtime) and set QT_ROOT to the Qt prefix (e.g. C:/Qt/6.8.3/msvc2022_64), then reconfigure with -DCMAKE_PREFIX_PATH=\"%QT_ROOT%\".")
endif()
find_package(Qt6 REQUIRED COMPONENTS Widgets Concurrent Quick Qml QuickControls2 QuickWidgets)

qt_standard_project_setup()

qt_add_executable(studio
  src/main.cpp
  src/preview_widget.cpp
  src/preview_widget.hpp
  src/apply_manager.cpp
  src/apply_manager.hpp
  src/compress_service.cpp
  src/compress_service.hpp
  src/probe_async.cpp
  src/probe_async.hpp
  src/compress_errors.cpp
  src/compress_errors.hpp
  src/user_errors.cpp
  src/user_errors.hpp
  src/ffprobe_helper.cpp
  src/ffprobe_helper.hpp
  app.rc
  # Todo 11: app icon (IDI_APPICON) + VERSIONINFO (ProductName K6WP).
  # packaging/ is on the RC include path so app.rc can reference k6wp-on.ico
  # by bare name (same pattern as engine/app.rc).
  )

# The window icon is embedded here rather than through a .qrc, because
# qt_add_qml_module() turns AUTORCC OFF for its target (it manages resources
# itself), so a plain icons.qrc listed as a source is silently never compiled -
# QIcon(":/icons/k6wp-on.ico") then returns a null icon and the window ends up
# with no icon at all. qt_add_resources embeds it unconditionally and keeps the
# same :/icons/ path the C++ call sites use.
qt_add_resources(studio "k6wp_icons"
  PREFIX "/icons"
  BASE "${CMAKE_SOURCE_DIR}/packaging"
  FILES "${CMAKE_SOURCE_DIR}/packaging/k6wp-on.ico"
)

# Todo 11: GUI subsystem — no console window. qt_add_executable links
# Qt's qtmain entry point, so int main() in src/main.cpp stays unchanged.
set_target_properties(studio PROPERTIES WIN32_EXECUTABLE TRUE)
target_include_directories(studio PRIVATE "${CMAKE_SOURCE_DIR}/packaging")
# QtAddQmlModule's generated studio_qmltyperegistrations.cpp pulls the
# QML_ELEMENT header in with `#include <studio_bridge.hpp>` (angle brackets,
# resolved from the build tree), so studio/src must be on the include path -
# the same reason every test target below already adds it.
target_include_directories(studio PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

# Phase 1 of the Widgets -> QML migration: the shell is a QMainWindow that
# hosts a QQuickWidget, so the target needs Quick/Qml/QuickControls2 as well
# as Widgets. Qt6::QuickWidgets is what provides <QQuickWidget> (it is a
# separate module from Qt6::Quick in Qt 6). Qt6::Widgets stays linked ON
# PURPOSE -- the mpv preview is a QWidget (PreviewWidget embeds libmpv through
# winId(), a HWND, not a QQuickItem). The previous spike proved the vendored
# libmpv has no D3D11 render backend and that the OpenGL path drops to
# hwdec-current=no, so the preview CANNOT become a QQuickItem yet; it is a
# child widget of the window.
target_link_libraries(studio PRIVATE k6wp_shared Qt6::Widgets Qt6::Quick Qt6::Qml Qt6::QuickControls2 Qt6::QuickWidgets Qt6::Concurrent winhttp shell32)

# QML module for the shell's Main.qml. URI K6WP makes `import K6WP` resolve
# from inside the binary (the file is compiled into a resource, so a deployed
# studio.exe needs no loose .qml files to start).
#
# RESOURCE_PREFIX is pinned to "/qt/qml" instead of relying on the QTP0001
# policy state: the module's resource dir is then always /qt/qml/K6WP, so
# QmlShell's qrc: URL is stable across policy changes.
#
# The source file lives in the qml/ SUBDIRECTORY (qml/Main.qml). The resource
# system would therefore place it at /qt/qml/K6WP/qml/Main.qml, while the
# generated qmldir (which sits at /qt/qml/K6WP/qmldir) declares the type as
# "Main 1.0 Main.qml" -- i.e. the file would sit OUTSIDE the directory its own
# qmldir describes and `import K6WP` would fail to resolve the type. Pinning
# QT_RESOURCE_ALIAS to the bare file name moves the resource to
# /qt/qml/K6WP/Main.qml, which is what the qmldir expects. Verified against the
# generated .qrc/qmldir in build/msvc-dev/studio/.qt/rcc/ (see qml_shell.cpp,
# which loads exactly that URL and asserts QQuickWidget::status()).
set_source_files_properties(qml/Main.qml PROPERTIES QT_RESOURCE_ALIAS Main.qml)
# Row 20: same alias treatment as Main.qml - the source lives in qml/ but the
# generated qmldir declares the bare file name, so the resource must land at
# /qt/qml/K6WP/DisplayCanvas.qml next to it.
set_source_files_properties(qml/DisplayCanvas.qml PROPERTIES QT_RESOURCE_ALIAS DisplayCanvas.qml)
qt_add_qml_module(studio
  URI K6WP
  VERSION 1.0
  RESOURCE_PREFIX "/qt/qml"
  QML_FILES
    qml/Main.qml
    qml/DisplayCanvas.qml
)

# About dialog version string (canonical K6WP_VERSION lives in the root
# CMakeLists.txt).
target_compile_definitions(studio PRIVATE "K6WP_VERSION_STR=\"${K6WP_VERSION}\"")

# MED-5 part 2 commit 1: CompressController wraps the single queued
# CompressService (owns the service + request ledger + tab request building).
target_sources(studio PRIVATE
  src/compress_controller.cpp
  src/compress_controller.hpp
)

# MED-5 part 1: EngineStatusController (the pure engine-status decision
# function). first_run_wizard.cpp stays because studio_logic_test compiles it.
target_sources(studio PRIVATE
  src/first_run_wizard.cpp
  src/first_run_wizard.hpp
  src/engine_status_controller.cpp
  src/engine_status_controller.hpp
)

# Studio IPC client (Todo 29, Wave 4): WaitNamedPipe+CreateFile retry + 4 perintah.
target_sources(studio PRIVATE
  src/ipc_client.cpp
  src/ipc_client.hpp
)

# The QML-facing backends. StudioBridge owns the IpcClient/ApplyManager and
# re-exposes live engine status; QmlShell is the QMainWindow that hosts the
# QQuickWidget and the native PreviewWidget. The old widget tree (main_window,
# settings/compressor/wallpaper tabs, library_widget, import_dialog,
# compress_status_widget) has been deleted now that QML serves every tab;
# first_run_wizard.cpp survives only because studio_logic_test compiles it.
target_sources(studio PRIVATE
  src/studio_bridge.cpp
  src/studio_bridge.hpp
  src/qml_shell.cpp
  src/qml_shell.hpp
  # Phase 3: CompressBridge, the Kompresor tab's backend. Owns a
  # CompressController BY VALUE (CompressController is not a QObject) and
  # republishes the compress queue to QML as primitives only.
  src/compress_bridge.cpp
  src/compress_bridge.hpp
  # Phase 4: SettingsBridge, the Pengaturan tab's backend. Owns StudioSettings
  # (a plain struct) plus the engine playback config, and persists both.
  src/settings_bridge.cpp
  src/settings_bridge.hpp
  src/cache_dir_policy.cpp
  src/cache_dir_policy.hpp
  # UI-language store (studio_ui.ini). Standalone on purpose: main.cpp reads the
  # language before any QML exists, so it must not depend on the bridge.
  src/ui_language.cpp
  src/ui_language.hpp
  # Phase 5: LibraryGridModel, the library grid's QAbstractListModel over
  # LibraryManager. Contains the throwing LibraryManager API so a LibraryError
  # can never unwind into the QML engine.
  src/library_grid_model.cpp
  src/library_grid_model.hpp
  src/library_format.cpp
  src/library_format.hpp
  # Phase 6: PlaylistBridge, the Wallpaper tab's playlist backend. Owns
  # playlist.json (shared/playlist.hpp); the engine reads the same file.
  src/playlist_bridge.cpp
  src/playlist_bridge.hpp
)

# Optional "new version available" check (Studio-only, WinHTTP worker).
# Never downloads/installs; silent on network failure or when disabled.
target_sources(studio PRIVATE
  src/update_checker.cpp
  src/update_checker.hpp
)

# Studio library metadata index (Todo 38, fase-5): library.json CRUD +
# persist + delete + LRU cache-key link.
target_sources(studio PRIVATE
  src/library_manager.cpp
  src/library_manager.hpp
  src/library_json_codec.cpp
  src/library_json_codec.hpp
)

# Studio thumbnailer (Todo 41, fase-6): central mtime-keyed thumbs cache
# (%LOCALAPPDATA%/K6WP/thumbs, K6WP_THUMBS_DIR test override), fast ffmpeg
# input seek (-ss 1 before -i). Single CreateProcessW site.
target_sources(studio PRIVATE
  src/thumbnailer.cpp
  src/thumbnailer.hpp
)

# studio_async_test (HIGH-3 slice B): offscreen Qt test proving the
# blocking IPC/process waits run on workers while the event loop pumps —
# cancel flag, queued error delivery, ack-deadline intact, exactly-1x retry.
# Compiles the REAL ipc_client.cpp + apply_manager.cpp (AUTOMOC from
# qt_standard_project_setup covers ApplyManager's Q_OBJECT). Skips the fake
# pipe-server sections when a live engine answers the session pipe.
if(BUILD_TESTING)
add_executable(studio_async_test
  ../tests/studio_async_test.cpp
  src/ipc_client.cpp
  src/apply_manager.cpp
  src/engine_status_controller.cpp  # Apply -> WaitForEngineReady (free fn)
)
target_link_libraries(studio_async_test PRIVATE k6wp_shared Qt6::Widgets Qt6::Concurrent)
target_include_directories(studio_async_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_options(studio_async_test PRIVATE /Zc:__cplusplus /permissive-)
add_test(NAME studio_async_test COMMAND studio_async_test)
set_tests_properties(studio_async_test PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endif()

# display_canvas_test (row 20): the offscreen QQmlComponent harness for
# qml/DisplayCanvas.qml. QGuiApplication, NOT studio_logic_test's
# QCoreApplication: instantiating QtQuick items needs a QPA platform (that is
# why studio_logic_test's environment at :237-239 deliberately omits the
# offscreen plugin), so this suite carries the same
# QT_QPA_PLATFORM=offscreen env property as studio_async_test.
#
# The QML file is loaded from SOURCE through the K6WP_DISPLAY_CANVAS_QML
# compile definition: the compiled copy is a resource of the studio target,
# not of this test executable, and the component takes its data through its
# own properties, so no Studio singleton or IPC engine is involved.
if(BUILD_TESTING)
add_executable(display_canvas_test
  ../tests/display_canvas_test.cpp
)
target_link_libraries(display_canvas_test PRIVATE k6wp_shared
  Qt6::Gui Qt6::Qml Qt6::Quick Qt6::QuickControls2)
target_compile_definitions(display_canvas_test PRIVATE
  "K6WP_DISPLAY_CANVAS_QML=\"${CMAKE_CURRENT_SOURCE_DIR}/qml/DisplayCanvas.qml\"")
target_compile_options(display_canvas_test PRIVATE /Zc:__cplusplus /permissive-)
# Offscreen QPA deployment: studio's windeployqt POST_BUILD deploys the
# app-local runtime into the same flat directory, which makes Qt resolve its
# prefix to the build dir and drop the Qt-install fallback for platform
# plugins. The deployed platforms/ dir then only holds qwindowsd.dll, so
# QT_QPA_PLATFORM=offscreen fails ("Could not find the Qt platform plugin
# offscreen") and Qt's fatal handler blocks in a modal dialog. Copying the
# offscreen plugin beside the deployed windows one puts it where the same
# loader looks.
if(TARGET Qt6::QOffscreenIntegrationPlugin)
  add_custom_command(TARGET display_canvas_test POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E make_directory
            "$<TARGET_FILE_DIR:display_canvas_test>/platforms"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "$<TARGET_FILE:Qt6::QOffscreenIntegrationPlugin>"
            "$<TARGET_FILE_DIR:display_canvas_test>/platforms/$<TARGET_FILE_NAME:Qt6::QOffscreenIntegrationPlugin>"
    COMMENT "Copying the offscreen QPA plugin next to display_canvas_test.exe"
    VERBATIM
  )
else()
  message(WARNING "Qt6::QOffscreenIntegrationPlugin not found; display_canvas_test needs it for QT_QPA_PLATFORM=offscreen.")
endif()
add_test(NAME display_canvas_test COMMAND display_canvas_test)
set_tests_properties(display_canvas_test PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endif()

# thumbnailer_test (MED-13): unit tests for the thumbnailer pure helpers
# (safe seek, aspect-preserving scale+pad command, cache-key versioning).
# The test #includes thumbnailer.cpp directly (occlusion_test-style single
# TU) so the anonymous-namespace helpers are reachable; only ffprobe_helper
# is compiled separately. K6WP_VERBOSE=1 so the QA failure path logs a
# clear error to stderr.
if(BUILD_TESTING)
add_executable(thumbnailer_test
  ../tests/thumbnailer_test.cpp
  src/ffprobe_helper.cpp
)
target_link_libraries(thumbnailer_test PRIVATE k6wp_shared)
target_include_directories(thumbnailer_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(thumbnailer_test PRIVATE K6WP_VERBOSE=1)
add_test(NAME thumbnailer_test COMMAND thumbnailer_test)

# library_crud_test (MED-2 slice A): LibraryManager metadata-index CRUD over
# a tmp dir (add/upsert/sorted-list/persist/reload/broken-entry/remove/
# corrupt-input/clear). The test #includes the REAL src/library_manager.cpp
# (thumbnailer_test-style single TU); only ffprobe_helper is compiled
# separately. Entries carry caller metadata so no ffprobe ever spawns; all
# removes are metadata-only so QFile::moveToTrash is never invoked (Qt link
# still required). Qt compile flags mirror studio_async_test.
add_executable(library_crud_test
  ../tests/library_crud_test.cpp
  src/ffprobe_helper.cpp
  src/library_json_codec.cpp
)
target_link_libraries(library_crud_test PRIVATE k6wp_shared Qt6::Widgets Qt6::Concurrent)
target_include_directories(library_crud_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_options(library_crud_test PRIVATE /Zc:__cplusplus /permissive-)
add_test(NAME library_crud_test COMMAND library_crud_test)

# studio_logic_test (MED-2 slice B): Studio non-GUI logic without the full
# main_window (QCoreApplication, never QApplication — the offscreen QPA
# plugin is absent and its load failure hangs init). Compiles the REAL
# src/ipc_client.cpp + src/apply_manager.cpp (AUTOMOC covers Q_OBJECT).
# Covers unwired-manager refusals, WriteConfig round-trip, ResolveEnginePath,
# no-server kNotRunning, and SyncMonitor dedup over an in-process fake pipe
# server on the real session pipe name (skips server sections when a live
# engine answers), plus the import-path threading regressions
# (library_grid_model.cpp) which need the model, the thumbnailer and an event
# loop. QmlShell (the QQuickWidget + libmpv window) is NOT linked: the test
# supplies SetActiveQmlShell/ActiveQmlShell itself, so only the
# QMainWindow-declaring header is needed, which Qt6::Widgets already provides.
add_executable(studio_logic_test
  ../tests/studio_logic_test.cpp
  src/ipc_client.cpp
  src/apply_manager.cpp
  src/first_run_wizard.cpp
  src/engine_status_controller.cpp
  src/compress_service.cpp
  src/compress_controller.cpp
  # CompressBridge is here for its Finished contract: a job that ends without
  # an output must say so, because resultChanged() never fires for it.
  src/compress_bridge.cpp
  src/compress_errors.cpp
  src/library_manager.cpp
  src/library_json_codec.cpp
  src/library_grid_model.cpp
  src/library_format.cpp
  src/thumbnailer.cpp
  src/probe_async.cpp
  src/ffprobe_helper.cpp
  # SettingsBridge (sections 18-19): reload() must not swallow a corrupt-settings
  # error, and clearCache() must refuse a cache_dir that is not a K6WP-managed
  # cache. ui_language.cpp comes along because the bridge's constructor reads
  # studio_ui.ini. Qt6::Qml supplies the QML_ELEMENT registration macros the
  # bridge's class declaration uses; shell32 supplies the known-folder lookup
  # the clearCache guard asks the shell for.
  src/settings_bridge.cpp
  src/cache_dir_policy.cpp
  src/ui_language.cpp
  # user_errors.cpp (1.2.1, section 20): the engine / IPC / preview error
  # mapping that StudioBridge used to show raw. Included so the mapping itself
  # is unit-testable, not just its call sites.
  src/user_errors.cpp
  # Row 19: the Display model bridge. studio_bridge.cpp + update_checker.cpp
  # (the ctor's optional update check) are compiled here so the display
  # surface (displays / displayCapability / duplicateModeNotice + the
  # assign/clear invokables) is unit-testable in this suite; winhttp is what
  # update_checker.cpp links against in the studio target too.
  src/studio_bridge.cpp
  src/update_checker.cpp
)
target_link_libraries(studio_logic_test PRIVATE k6wp_shared Qt6::Widgets Qt6::Concurrent Qt6::Qml shell32 winhttp)
target_include_directories(studio_logic_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
# studio_bridge.hpp stamps the About-dialog version the same way the studio
# target does (K6WP_VERSION_STR; generated/version.h carries the same value).
target_compile_definitions(studio_logic_test PRIVATE "K6WP_VERSION_STR=\"${K6WP_VERSION}\"")
target_compile_options(studio_logic_test PRIVATE /Zc:__cplusplus /permissive-)
add_test(NAME studio_logic_test COMMAND studio_logic_test)
# MED-5 part 2: the live compress-queue leg needs compressor.exe next to the
# test binary (CompressService::ResolveCompressorPath primary) + ffmpeg via
# env (K6WP_FFMPEG/K6WP_FFPROBE, same keys the compressor reads).
add_dependencies(studio_logic_test compressor)
add_custom_command(TARGET studio_logic_test POST_BUILD
  COMMAND "${CMAKE_COMMAND}" -E copy_if_different
          "$<TARGET_FILE:compressor>"
          "$<TARGET_FILE_DIR:studio_logic_test>/compressor.exe"
  COMMENT "Copying compressor.exe next to studio_logic_test.exe (queue test)"
  VERBATIM
)
set_tests_properties(studio_logic_test PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;K6WP_FFMPEG=${CMAKE_SOURCE_DIR}/vendor/ffmpeg/ffmpeg.exe;K6WP_FFPROBE=${CMAKE_SOURCE_DIR}/vendor/ffmpeg/ffprobe.exe")

# fake_pipe_test (MED-2 slice B): Qt-free IpcClient round-trips over an
# in-process fake named-pipe server (no Qt, no event loop — plain console
# main). Covers the pipe-name helpers, scripted ack shapes (ok / error /
# malformed), and the no-server kNotRunning path (`--no-server` runs only
# the server-down section for QA-failure evidence). Skips server sections
# when a live engine answers the session pipe.
add_executable(fake_pipe_test
  ../tests/fake_pipe_test.cpp
  src/ipc_client.cpp
)
target_link_libraries(fake_pipe_test PRIVATE k6wp_shared)
target_include_directories(fake_pipe_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
add_test(NAME fake_pipe_test COMMAND fake_pipe_test)

# playlist_bridge_test: PlaylistBridge CRUD/persist round-trip over a scratch
# playlist.json (K6WP_PLAYLIST_JSON override). QCoreApplication; the native
# picker is never invoked (addPaths drives the additions). Qt6::Qml supplies the
# QML_ELEMENT registration macros; Qt6::Widgets the QFileDialog symbol.
add_executable(playlist_bridge_test
  ../tests/playlist_bridge_test.cpp
  src/playlist_bridge.cpp
)
target_link_libraries(playlist_bridge_test PRIVATE k6wp_shared Qt6::Widgets Qt6::Qml)
target_include_directories(playlist_bridge_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_options(playlist_bridge_test PRIVATE /Zc:__cplusplus /permissive-)
add_test(NAME playlist_bridge_test COMMAND playlist_bridge_test)
endif()

# Vendored libmpv (Todo 2): headers + MSVC import lib. libmpv-2.dll is copied
# next to the exe by the POST_BUILD step below (preview widget, Todo 23).
target_include_directories(studio PRIVATE "${CMAKE_SOURCE_DIR}/vendor/libmpv/include")
target_link_libraries(studio PRIVATE "${CMAKE_SOURCE_DIR}/vendor/libmpv/lib/mpv.lib")

add_custom_command(TARGET studio POST_BUILD
  COMMAND "${CMAKE_COMMAND}" -E copy_if_different
          "${CMAKE_SOURCE_DIR}/vendor/libmpv/bin/libmpv-2.dll"
          "$<TARGET_FILE_DIR:studio>/libmpv-2.dll"
  COMMENT "Copying libmpv-2.dll next to studio.exe"
  VERBATIM
)

# Qt 6.8.3 on MSVC requires a conforming __cplusplus and permissive- mode;
# without either, qcompilerdetection.h fails (C1189/C2338). Validated in
# spike Todo 4.
target_compile_options(studio PRIVATE /Zc:__cplusplus /permissive-)

# Deployment script (windeployqt) so studio.exe runs from a copied folder.
qt_generate_deploy_app_script(
  TARGET studio
  OUTPUT_SCRIPT deploy_script
  NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})

# POST_BUILD windeployqt: copies Qt6Core/Widgets/Gui DLLs + platforms/qwindows.dll
# next to the exe so studio.exe runs from build/msvc-dev/ WITHOUT QT_ROOT on PATH.
# Derive Qt prefix from Qt6Widgets_DIR (always set by find_package COMPONENTS).
get_filename_component(_qt_prefix "${Qt6Widgets_DIR}/../../.." ABSOLUTE)

# --- UI language (Qt Linguist) ------------------------------------------------
# MED-11 (owner decision: hapus) removed this scaffolding: the UI was
# Indonesian-only, every string was hardcoded, and the lupdate/lrelease/
# translations targets plus the studio/i18n/*.ts baseline were deleted as
# unused. The owner has since reversed that decision - the UI language is
# selectable at runtime again - so the chain below is back. What MED-11 got
# right still holds and shapes the design: the C++/QML sources ARE the
# Indonesian translation, so "id" is the source language, the default, and
# needs neither a .qm nor a QTranslator. That is why there is deliberately no
# studio_id.ts: translating Indonesian into Indonesian only adds a file that
# can drift from its own source. Only a non-source language loads a catalogue.
#
# Targets:
#   lupdate      re-extract studio/i18n/studio_en.ts (the file Linguist edits)
#   lrelease     compile that .ts to build/i18n/studio_en.qm
#   translations both, lupdate first
# Only `lrelease` gates the studio target, so a plain build compiles the
# catalogue but never rewrites the committed .ts. After changing UI strings run
# `cmake --build --preset msvc-dev --target translations` to refresh it.
#
# find_program, never a hardcoded path: the tools live in whichever Qt prefix
# find_package just picked (_qt_prefix, derived above), with QT_ROOT/bin as a
# second hint and PATH as the last resort.
find_program(K6WP_LUPDATE NAMES lupdate
  HINTS "${_qt_prefix}/bin" "$ENV{QT_ROOT}/bin")
find_program(K6WP_LRELEASE NAMES lrelease
  HINTS "${_qt_prefix}/bin" "$ENV{QT_ROOT}/bin")

set(K6WP_TS_EN "${CMAKE_CURRENT_SOURCE_DIR}/i18n/studio_en.ts")
# lrelease writes into the studio target's own build subtree; the POST_BUILD
# step below is what puts the catalogue where the exe can find it. Keeping the
# two apart matters because the runtime output dir is flattened to
# ${CMAKE_BINARY_DIR} (see the root CMakeLists), so $<TARGET_FILE_DIR:studio>
# is the top-level build dir - writing the .qm straight there would make the
# copy step a no-op on itself.
set(K6WP_QM_EN "${CMAKE_CURRENT_BINARY_DIR}/i18n/studio_en.qm")
set(K6WP_I18N_STAMP "${CMAKE_CURRENT_BINARY_DIR}/i18n/lupdate.stamp")

# lupdate needs the QML sources, not just the C++: qsTr() in qml/Main.qml and
# qml/DisplayCanvas.qml is where the Studio UI text lives, and qt_add_qml_module
# compiles those files into a qrc, so no on-disk .qml would be found by a
# build-tree scan. Passing each file explicitly is safe for the lookup that
# matters - lupdate takes the QML context from the file name alone
# (QQmlTranslation::contextFromQmlFilename), so "qml/Main.qml" yields the same
# "Main" context the QML engine asks for at runtime from /qt/qml/K6WP/Main.qml,
# and "qml/DisplayCanvas.qml" yields the matching "DisplayCanvas" context. The
# Display panel lives in the second file (row 25), so both must be listed.
#
# Argument order is load-bearing in lupdate 6.8: sources must come BEFORE -ts.
# The reverse order aborts with "File '...' has no recognized extension".
file(GLOB K6WP_TR_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp")
set(K6WP_TR_QML_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/qml/Main.qml"
  "${CMAKE_CURRENT_SOURCE_DIR}/qml/DisplayCanvas.qml")

# Both tools degrade to a clear no-op target instead of failing the configure:
# a Studio that cannot translate is still a working Studio (main.cpp falls back
# to the source language when no .qm is found), so a broken/missing Linguist
# must not block a release build.
if(K6WP_LUPDATE)
  add_custom_command(OUTPUT "${K6WP_I18N_STAMP}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/i18n"
    # lupdate will not create the output directory itself, and git cannot
    # track an empty one - so the first extraction on a fresh clone needs it.
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_CURRENT_SOURCE_DIR}/i18n"
    COMMAND "${K6WP_LUPDATE}" -locations relative
            -source-language id -target-language en
            ${K6WP_TR_SOURCES} ${K6WP_TR_QML_SOURCES}
            -ts "${K6WP_TS_EN}"
    COMMAND "${CMAKE_COMMAND}" -E touch "${K6WP_I18N_STAMP}"
    DEPENDS ${K6WP_TR_SOURCES} ${K6WP_TR_QML_SOURCES}
            "${K6WP_LUPDATE}"
    COMMENT "lupdate: extracting translatable strings into i18n/studio_en.ts"
    VERBATIM
  )
  add_custom_target(lupdate DEPENDS "${K6WP_I18N_STAMP}")
else()
  message(WARNING "lupdate not found (searched ${_qt_prefix}/bin, $ENV{QT_ROOT}/bin, PATH). The English .ts will NOT be refreshed; translate from the committed studio/i18n/studio_en.ts.")
  add_custom_target(lupdate
    COMMAND "${CMAKE_COMMAND}" -E echo "translations: lupdate not found - studio_en.ts was NOT refreshed."
    VERBATIM
  )
endif()

if(K6WP_LRELEASE)
  # DEPENDS on the lupdate stamp, not just the .ts: that is what fixes
  # lupdate-before-lrelease under a parallel generator. Two phony targets
  # ordered by add_dependencies() are not ordered.
  add_custom_command(OUTPUT "${K6WP_QM_EN}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/i18n"
    COMMAND "${K6WP_LRELEASE}" "${K6WP_TS_EN}" -qm "${K6WP_QM_EN}"
    DEPENDS "${K6WP_TS_EN}" "${K6WP_I18N_STAMP}" "${K6WP_LRELEASE}"
    COMMENT "lrelease: compiling i18n/studio_en.qm"
    VERBATIM
  )
  add_custom_target(lrelease DEPENDS "${K6WP_QM_EN}")
else()
  message(WARNING "lrelease not found (searched ${_qt_prefix}/bin, $ENV{QT_ROOT}/bin, PATH). No .qm will be built; Studio falls back to the Indonesian source strings.")
  add_custom_target(lrelease
    COMMAND "${CMAKE_COMMAND}" -E echo "translations: lrelease not found - no .qm was built."
    VERBATIM
  )
endif()

add_custom_target(translations DEPENDS lupdate lrelease)

if(K6WP_LRELEASE)
  # The catalogue has to exist before studio.exe is deployed beside it, and
  # windeployqt --no-translations is left alone: that flag excludes Qt's OWN
  # translation catalogues, which we do not want to ship. Ours is copied here.
  add_dependencies(studio lrelease)
  add_custom_command(TARGET studio POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E make_directory "$<TARGET_FILE_DIR:studio>/i18n"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${K6WP_QM_EN}"
            "$<TARGET_FILE_DIR:studio>/i18n/studio_en.qm"
    COMMENT "Copying studio_en.qm next to studio.exe (UI language)"
    VERBATIM
  )
endif()

# POST_BUILD windeployqt (see _qt_prefix derivation above).
# P4.2: --exclude-plugins qtuiotouchplugin — the generic touch-input plugin
# is the sole remaining Qt6Network consumer (windeployqt: "Adding Qt6Network
# for qtuiotouchplugind.dll"), and Studio's update check is WinHTTP now, so
# Network/TLS/bearer plugins must not ship. Desktop Widgets needs no TuioTouch.
# Phase 1: --qmldir <exe_dir>/qml makes windeployqt copy the QML runtime
# (Qt6Quick/Qml/QuickControls2/QuickWidgets DLLs, the QtQuick/* import trees and
# the scene-graph adaptation plugins, including the Material style plugin) next
# to studio.exe. Without it a copied studio.exe folder loads no QML at all.
# --no-translations is kept.
#
# Two prerequisites, both learned the hard way:
#   1. the directory must exist BEFORE windeployqt runs - --qmldir is handed
#      straight to qmlimportscanner, which fails with "No such file or
#      directory" (non-zero exit -> the whole POST_BUILD step fails);
#   2. it must contain a .qml file, because that is what qmlimportscanner scans
#      to discover which import trees to deploy. Our Main.qml is compiled into
#      a qrc, so there is nothing loose to scan - hence the explicit copy. The
#      copy is a scan input only; at runtime the exe loads the qrc copy, and
#      the deployed tree supplies the QtQuick/* imports it pulls in.
add_custom_command(TARGET studio POST_BUILD
  COMMAND "${CMAKE_COMMAND}" -E make_directory "$<TARGET_FILE_DIR:studio>/qml"
  COMMAND "${CMAKE_COMMAND}" -E copy_if_different
          "${CMAKE_CURRENT_SOURCE_DIR}/qml/Main.qml"
          "$<TARGET_FILE_DIR:studio>/qml/Main.qml"
  COMMAND "${_qt_prefix}/bin/windeployqt.exe"
          --no-translations
          --no-system-d3d-compiler
          --no-opengl-sw
          --exclude-plugins qtuiotouchplugin
          --qmldir "$<TARGET_FILE_DIR:studio>/qml"
          "$<TARGET_FILE:studio>"
  COMMENT "Deploying Qt runtime DLLs next to studio.exe (windeployqt)"
  VERBATIM
)
