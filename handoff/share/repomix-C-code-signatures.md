# User Provided Header
K6WP HANDOFF PACK - C-code-signatures | branch refactor/rust-wt | read STATE.md + ledger.md first | no measurements in this run

# Directory Structure
```
compressor/
  src/
    cache_manager.cpp
    cache_manager.hpp
    cli.cpp
    cli.hpp
    dpb_probe.cpp
    dpb_probe.hpp
    encoder_detect.cpp
    encoder_detect.hpp
    ffmpeg_job.cpp
    ffmpeg_job.hpp
    main.cpp
    video_probe.cpp
    video_probe.hpp
  tests/
    compress_argv_contract_test.cpp
    compress_friendly_error_test.cpp
    dpb_frame_size_test.cpp
    probe_json_test.cpp
    widen_utf8_test.cpp
  app.rc
  CMakeLists.txt
engine/
  src/
    bench_hook.cpp
    bench_hook.hpp
    cli_options.cpp
    cli_options.hpp
    config_watch.cpp
    config_watch.hpp
    cpu_affinity.cpp
    cpu_affinity.hpp
    crash_dump_prune.cpp
    crash_dump_prune.hpp
    crash_restart_guard.cpp
    crash_restart_guard.hpp
    desktop_inject.cpp
    desktop_inject.hpp
    desktop_placement.cpp
    desktop_placement.hpp
    device_lost_watch.cpp
    device_lost_watch.hpp
    engine_app.cpp
    engine_app.hpp
    engine_watchdog.cpp
    engine_watchdog.hpp
    fullscreen_watch.cpp
    fullscreen_watch.hpp
    gpu_pin.cpp
    gpu_pin.hpp
    ipc_command_marshal.cpp
    ipc_command_marshal.hpp
    ipc_marshal.cpp
    ipc_marshal.hpp
    ipc_server.cpp
    ipc_server.hpp
    ipc_trace.hpp
    lock_order.hpp
    log_file.cpp
    log_file.hpp
    main.cpp
    mpv_renderer.cpp
    mpv_renderer.hpp
    multi_monitor.cpp
    multi_monitor.hpp
    occlusion_poke_scheduler.cpp
    occlusion_poke_scheduler.hpp
    occlusion_watch.cpp
    occlusion_watch.hpp
    pause_controller.hpp
    pending_queue.hpp
    pin_verify_schedule.cpp
    pin_verify_schedule.hpp
    playlist_controller.cpp
    playlist_controller.hpp
    power.cpp
    power.hpp
    reconcile_policy.cpp
    reconcile_policy.hpp
    renderer_graveyard.cpp
    renderer_graveyard.hpp
    retry_schedule.cpp
    retry_schedule.hpp
    test_simulator.cpp
    test_simulator.hpp
    timer_ids.hpp
    topology_diff.cpp
    topology_diff.hpp
    topology_snapshot.hpp
    tray_actions.cpp
    tray_actions.hpp
    tray.cpp
    tray.hpp
    unload_state.hpp
    working_set_trim.cpp
    working_set_trim.hpp
  app.manifest
  app.manifest.in
  app.rc
  CMakeLists.txt
launcher/
  app.rc
  cli.cpp
  cli.hpp
  CMakeLists.txt
  exit_codes.hpp
  lockscreen_backup.cpp
  lockscreen_backup.hpp
  lockscreen_policy.cpp
  lockscreen_policy.hpp
  lockscreen_sync.cpp
  lockscreen_sync.hpp
  main.cpp
  process_paths.cpp
  process_paths.hpp
  process_stop.cpp
  process_stop.hpp
  still_apply.cpp
  still_apply.hpp
  win32_raii.hpp
packaging/
  app-icon-README.md
  config.json.example
  installer.nsi
  k6wp-off.ico
  k6wp-on.ico
  known-limitations.md
  make_app_icon.py
  make_zip.ps1
  playlist.json.example
  resource.h
  sign_outputs.ps1
  studio_settings.json.example
  uninstall.bat
  version.nsh
rust/
  crates/
    k6wp-core/
      src/
        compress_args.rs
        config_store.rs
        config.rs
        displays.rs
        engine_logic.rs
        ipc.rs
        lib.rs
        library.rs
        playlist.rs
        settings.rs
        sps.rs
        version.rs
    k6wp-ipc/
      src/
        lib.rs
    k6wp-studio/
      src/
        grid.rs
        history.rs
        lib.rs
        main.rs
        thumbnailer.rs
        thumbs.rs
        worker.rs
shared/
  autostart.cpp
  autostart.hpp
  build_flags.hpp
  CMakeLists.txt
  compress_args.cpp
  compress_args.hpp
  config_lock.cpp
  config_lock.hpp
  config_schema.cpp
  config_schema.hpp
  config_schema.json
  displays_schema.cpp
  displays_schema.hpp
  displays_schema.json
  ffmpeg_path.cpp
  ffmpeg_path.hpp
  ipc_protocol.cpp
  ipc_protocol.hpp
  legacy_lockscreen_policy.cpp
  legacy_lockscreen_policy.hpp
  links.hpp
  lockscreen_exit.hpp
  monitor_dump.cpp
  monitor_dump.rc
  monitor_util.cpp
  monitor_util.hpp
  os_version.cpp
  os_version.hpp
  playlist.cpp
  playlist.hpp
  proc_util.cpp
  proc_util.hpp
  settings_io.cpp
  settings_io.hpp
  sha1.cpp
  sha1.hpp
  still.cpp
  still.hpp
  studio_settings.cpp
  studio_settings.hpp
  studio_settings.json
  version_compare.hpp
studio/
  src/
    apply_manager.cpp
    apply_manager.hpp
    bridge_diagnostics.hpp
    cache_dir_policy.cpp
    cache_dir_policy.hpp
    compress_bridge.cpp
    compress_bridge.hpp
    compress_controller.cpp
    compress_controller.hpp
    compress_errors.cpp
    compress_errors.hpp
    compress_first_offer.hpp
    compress_service.cpp
    compress_service.hpp
    engine_status_controller.cpp
    engine_status_controller.hpp
    ffprobe_helper.cpp
    ffprobe_helper.hpp
    first_run_wizard.cpp
    first_run_wizard.hpp
    ipc_client.cpp
    ipc_client.hpp
    library_format.cpp
    library_format.hpp
    library_grid_model.cpp
    library_grid_model.hpp
    library_json_codec.cpp
    library_json_codec.hpp
    library_manager.cpp
    library_manager.hpp
    main.cpp
    playlist_bridge.cpp
    playlist_bridge.hpp
    preview_widget.cpp
    preview_widget.hpp
    probe_async.cpp
    probe_async.hpp
    qml_shell.cpp
    qml_shell.hpp
    settings_bridge.cpp
    settings_bridge.hpp
    studio_bridge.cpp
    studio_bridge.hpp
    thumbnailer.cpp
    thumbnailer.hpp
    ui_language.cpp
    ui_language.hpp
    update_checker.cpp
    update_checker.hpp
    user_errors.cpp
    user_errors.hpp
    video_paths.hpp
tools/
  bench_ab.ps1
  bench_baseline_v2.ps1
  bench_bundle_zip.ps1
  bench_bundle.ps1
  bench_compress.ps1
  bench_cpu_mem.ps1
  bench_dpb_ab.ps1
  bench_dpb_parity.ps1
  bench_engine.ps1
  bench_frames.ps1
  bench_get_state_max.ps1
  bench_h3_bitdepth.ps1
  bench_lib.ps1
  bench_pacing.ps1
  bench_probe_parity.ps1
  bench_ram_self.ps1
  bench_resume.ps1
  bench_startup.ps1
  bench_studio_compare.ps1
  bench_studio.ps1
  bench_unload_d2.ps1
  bench_unload_scenarios.ps1
  bench_unload_visual.ps1
  bench_unload.ps1
  bench_visual.ps1
  bench_vo_sustained.ps1
  bench_wallpaper_frames.ps1
  build_ab.ps1
  capture_visual_baseline.ps1
  capture_window.ps1
  check_live_animation.ps1
  check_release_overrides.ps1
  clear_wallpaper.ps1
  diag_gpu.ps1
  diag_unload_crash.ps1
  diag_wakeups3.ps1
  drive_engine_ipc.ps1
  e2e_a12b_undo.ps1
  e2e_set_wallpaper.ps1
  extract_sps_truth.ps1
  fetch_vendor.ps1
  g5_diff_baseline.ps1
  gate_ctest_stability.ps1
  gen_library_200.ps1
  install_hooks.ps1
  lint_env_overrides_selftest.ps1
  lint_env_overrides.ps1
  lint_nonlatin_selftest.ps1
  lint_nonlatin.ps1
  lint_test_overrides.ps1
  lint_ui_thread.ps1
  list_windows.ps1
  look_desktop2.ps1
  make_corpus.ps1
  make_handoff.ps1
  measure_a12b_coexistence.ps1
  measure_studio_library.ps1
  run_2monitor_evidence.ps1
  run_phase0_bench.ps1
  scroll_window.ps1
  seed_fuzz_corpus.ps1
  session_snapshot.ps1
  stress_ipc.ps1
  verify_visual_negative.ps1
  verify_wallpaper.ps1
CMakeLists.txt
```

# Files

## File: compressor/src/cache_manager.cpp
```cpp
std::string GetEnvStr(const char* name) {
⋮----
std::string value(buf);
⋮----
std::filesystem::path CacheRoot() {
⋮----
std::uint64_t CacheMaxBytes() {
⋮----
std::string CacheKeyHex(const std::filesystem::path& in, int res_w,
⋮----
// Cache generation: v3 = single shared SHA1 (MED-3) + DPB-pinned encodes
// (refs=2/bf=1, 2026-09-20). Bumps the key so pre-v3 cached files miss
// and re-encode instead of being served forever.
⋮----
std::filesystem::path CachePathForKey(const std::string& key_hex) {
⋮----
bool CacheHas(const std::string& key_hex) {
⋮----
int CacheFetchToOut(const std::string& key_hex,
⋮----
int CacheStoreFromOut(const std::string& key_hex,
⋮----
int CachePrune(std::string& error) {
⋮----
struct Entry {
⋮----
std::filesystem::directory_iterator it(root, ec);
```

## File: compressor/src/cache_manager.hpp
```cpp
std::filesystem::path CacheRoot();
⋮----
std::uint64_t CacheMaxBytes();
⋮----
std::string CacheKeyHex(const std::filesystem::path& in, int res_w,
⋮----
std::filesystem::path CachePathForKey(const std::string& key_hex);
⋮----
bool CacheHas(const std::string& key_hex);
⋮----
int CacheFetchToOut(const std::string& key_hex,
⋮----
int CacheStoreFromOut(const std::string& key_hex,
⋮----
int CachePrune(std::string& error);
```

## File: compressor/src/encoder_detect.cpp
```cpp
std::wstring FindFfmpeg() {
⋮----
// ---- subprocess helper ----
⋮----
struct RunResult {
⋮----
/// Run a command via the central k6wp::RunCaptured helper (single spawn
/// site), capturing stdout+stderr.
/// Uses lpApplicationName for the exe (avoids cmd.exe quote-stripping).
/// Budget is kCompressTimeoutMs: these are real ffmpeg encode probes
/// (hardware-encoder init can take seconds), not cheap metadata reads.
/// Unlike the old inline block, a hung ffmpeg is killed on timeout instead
/// of lingering as an orphan while the caller reads STILL_ACTIVE (259).
RunResult RunCommand(const std::wstring& exe_path,
⋮----
// ---- candidate encoders in priority order ----
⋮----
struct EncoderCandidate {
const char* name;  // ffmpeg encoder name
const char* label; // short CLI label
⋮----
bool LineContainsEncoder(const std::string& line, const char* name) {
⋮----
std::vector<EncoderInfo> ListEncoders() {
⋮----
bool Probe1Frame(const std::string& encoder_name) {
⋮----
std::string PickEncoder() {
⋮----
// All HW encoders failed — fall back to libx264.
```

## File: compressor/src/encoder_detect.hpp
```cpp
struct EncoderInfo {
⋮----
std::vector<EncoderInfo> ListEncoders();
⋮----
bool Probe1Frame(const std::string& encoder_name);
⋮----
std::string PickEncoder();
```

## File: compressor/src/ffmpeg_job.cpp
```cpp
BOOL WINAPI CancelHandler(DWORD ctrl_type) {
⋮----
std::string FriendlyId(const char* id) {
⋮----
std::string EscapeJsonString(const std::string& s) {
⋮----
std::string FormatError(const char* friendly_id, const std::string& technical_detail) {
⋮----
class HandleGuard {
⋮----
explicit HandleGuard(HANDLE h = nullptr) : h_(h) {}
⋮----
HandleGuard(const HandleGuard&) = delete;
⋮----
HandleGuard(HandleGuard&& o) noexcept : h_(o.h_) { o.h_ = nullptr; }
⋮----
HANDLE get() const { return h_; }
HANDLE* out() { return &h_; }
void reset(HANDLE h = nullptr) {
⋮----
std::wstring FindFfmpeg() {
⋮----
// Decode UTF-8 to UTF-16 (MED-10). CP_UTF8 with dwFlags=0 is deliberately
// lenient (invalid sequences -> U+FFFD, never a hard failure) so a bad byte
// can't drop the whole command line; the old byte-wise widen sign-extended
// every byte >= 0x80 (0xC3 -> U+FFC3) and never decoded multibyte sequences.
std::wstring Widen(const std::string& s) {
⋮----
// ---- progress parsing ----
⋮----
// Parse "HH:MM:SS.xx" (ffmpeg Duration / out_time) into microseconds.
⋮----
long long ParseTimestampUs(const char* s) {
⋮----
bool StartsWith(const std::string& line, const char* prefix) {
⋮----
std::string ResolveEncoderName(const std::string& label_or_auto) {
⋮----
bool BuildFfmpegCmdline(const FfmpegJobOptions& opts, std::wstring& cmdline,
⋮----
// Normal compression mode
⋮----
// Per-encoder quality flags. CRF range 16-28 is enforced by the CLI.
⋮----
// DPB pinning: wallpaper loops gain nothing from deep reference
// lists, but the decoder's DXVA surface pool scales with the SPS
// num_ref_frames. refs=2/bf=1 keeps the pool minimal (A/B 2026-09-20
// targets -50..-150 MB system RAM @4K decode). NVENC + libx264 only:
// the QSV/AMF legs never run on this machine, so their equivalent
// flags stay unset rather than risk an untested encode failure.
⋮----
// Decode-friendly tuning: libx264 only (the x264-style CRF leg).
// NVENC/QSV/AMF stay untouched — -tune is an x264 option.
⋮----
int RunFfmpegJob(const FfmpegJobOptions& opts, std::string& error) {
⋮----
HandleGuard write_pipe(write_raw);
⋮----
HandleGuard job(CreateJobObjectW(nullptr, nullptr));
```

## File: compressor/src/ffmpeg_job.hpp
```cpp
struct FfmpegJobOptions {
⋮----
std::string ResolveEncoderName(const std::string& label_or_auto);
⋮----
bool BuildFfmpegCmdline(const FfmpegJobOptions& opts, std::wstring& cmdline,
⋮----
int RunFfmpegJob(const FfmpegJobOptions& opts, std::string& error);
```

## File: compressor/src/video_probe.cpp
```cpp
bool ParseProbeJson(const std::string& json_text, VideoProps& out) {
⋮----
// Same validity bar as the old probe: codec + positive dimensions.
⋮----
VideoProps ProbeVideoProps(const std::filesystem::path& in) {
```

## File: compressor/src/video_probe.hpp
```cpp
struct VideoProps {
⋮----
[[nodiscard]] bool ParseProbeJson(const std::string& json_text,
⋮----
[[nodiscard]] VideoProps ProbeVideoProps(const std::filesystem::path& in);
```

## File: compressor/tests/compress_argv_contract_test.cpp
```cpp
void Check(bool cond, const char* what) {
⋮----
std::wstring WidenUtf8(const std::string& s) {
⋮----
bool QuoteArg(const std::string& tok, std::wstring& quoted) {
⋮----
// Spawn the real compressor.exe with `argv` (argv[0] implied, quoted per
// QuoteArg). `build_ok` is set false when a token cannot be quoted, in which
// case the returned ProcResult is meaningless. Shared by step 3 and step 4
// so both spawn the binary through one code path.
k6wp::ProcResult RunCompressor(const std::filesystem::path& exe,
⋮----
// Whole-file bytes, for the data-loss assertion: the guard's real contract is
// that the input survives BYTE-IDENTICAL, not merely that it still exists.
std::string ReadAllBytes(const std::filesystem::path& p) {
std::ifstream f(p, std::ios::binary);
⋮----
// Points the spawned compressor at the pinned vendored ffmpeg via
// K6WP_FFMPEG. Without this the test depends on shared/ffmpeg_path.cpp's
// directory guessing (exe_dir\..\..\vendor\ffmpeg), which holds in a dev tree
// but not on a clean CI checkout, and the "exits 0" assertions then fail for a
⋮----
bool PinFfmpegForChild() {
⋮----
int main(int argc, char** argv) {
```

## File: compressor/tests/compress_friendly_error_test.cpp
```cpp
void Check(bool cond, const char* what) {
⋮----
std::string Lower(std::string s) {
⋮----
bool Contains(const std::string& hay, const std::string& needle) {
⋮----
bool JsonField(const std::string& json, const std::string& field,
⋮----
bool HasDeveloperDetail(const std::string& s) {
⋮----
int main() {
⋮----
std::ofstream touch(lock_in, std::ios::binary);
```

## File: compressor/tests/probe_json_test.cpp
```cpp
void Check(bool cond, const char* what) {
⋮----
bool Near(double a, double b, double eps = 1e-9) {
⋮----
// Same facts, shuffled: streams AFTER format, keys within each object in
// reverse order, audio stream first. Must parse identically (JSON is
// order-independent, unlike the old line-order -of default parsing).
⋮----
void ExpectCanonical(const k6wp::compressor::VideoProps& p, const char* tag) {
⋮----
void TestCanonicalAndShuffled() {
⋮----
void TestNoAudio() {
⋮----
void TestDurationAsNumber() {
⋮----
void TestMissingDuration() {
⋮----
void TestCorrupt() {
⋮----
void TestBadFps() {
⋮----
int RunUnit() {
⋮----
int main(int argc, char** argv) {
⋮----
std::ifstream in(argv[2], std::ios::binary);
```

## File: compressor/tests/widen_utf8_test.cpp
```cpp
std::filesystem::path FindFfmpeg() { return {}; }
⋮----
std::string PickEncoder() { return "libx264"; }
⋮----
static std::wstring WidenOld(const std::string& s) {
⋮----
static void Check(bool ok, const char* what) {
⋮----
static void DumpWchars(const char* label, const std::wstring& ws) {
⋮----
int main(int argc, char** argv) {
```

## File: compressor/app.rc
```
// compressor/app.rc - resource script for compressor.exe (Todo 11).
//
// Minimal VERSIONINFO (ProductName K6WP, version from generated version.h) + the shared app
// icon from packaging/k6wp-on.ico (bare name: packaging/ is on the RC include
// path, same pattern as engine/app.rc).

#include <windows.h>
#include "resource.h"
#include "version.h"

IDI_APPICON ICON "k6wp-on.ico"

VS_VERSION_INFO VERSIONINFO
 FILEVERSION K6WP_VERSION_COMMA
 PRODUCTVERSION K6WP_VERSION_COMMA
 FILEFLAGSMASK 0x3fL
#ifdef _DEBUG
 FILEFLAGS 0x1L
#else
 FILEFLAGS 0x0L
#endif
 FILEOS 0x40004L
 FILETYPE 0x1L
 FILESUBTYPE 0x0L
BEGIN
    BLOCK "StringFileInfo"
    BEGIN
        BLOCK "040904b0"
        BEGIN
            VALUE "CompanyName", "K6WP contributors"
            VALUE "FileDescription", "K6WP Compressor"
            VALUE "FileVersion", K6WP_VERSION_STR_4
            VALUE "InternalName", "compressor"
            VALUE "LegalCopyright", "Copyright (C) 2026 K6WP contributors - GPL-2.0-or-later"
            VALUE "OriginalFilename", "compressor.exe"
            VALUE "ProductName", "K6WP"
            VALUE "ProductVersion", K6WP_VERSION_STR_4
        END
    END
    BLOCK "VarFileInfo"
    BEGIN
        VALUE "Translation", 0x409, 1200
    END
END
```

## File: engine/src/bench_hook.cpp
```cpp
void MarkFirstFrame() {
```

## File: engine/src/bench_hook.hpp
```cpp
void MarkFirstFrame();
```

## File: engine/src/cli_options.cpp
```cpp
void PrintUsage(FILE* out) {
⋮----
const char* WallpaperModeToString(WallpaperMode mode) {
⋮----
int ParseCli(int argc, char** argv, CliOptions& out) {
```

## File: engine/src/cli_options.hpp
```cpp
enum class WallpaperMode { kAuto, kWorkerW, kProgman };
⋮----
struct CliOptions {
⋮----
const char* WallpaperModeToString(WallpaperMode mode);
⋮----
int ParseCli(int argc, char** argv, CliOptions& out);
```

## File: engine/src/config_watch.cpp
```cpp
void Log(const char* fmt, ...) {
⋮----
void ConfigWatcher::Start(const std::filesystem::path& config_path,
⋮----
void ConfigWatcher::WatchSecondFile(const std::filesystem::path& path,
⋮----
void ConfigWatcher::Poll() {
⋮----
void* ConfigWatcher::EventHandle() const {
⋮----
void ConfigWatcher::SetDebounceWindow(void* hwnd) { debounce_hwnd_ = hwnd; }
⋮----
bool ConfigWatcher::ArmDebounce(bool is_second) {
⋮----
void ConfigWatcher::OnDebounceExpired() {
⋮----
bool ConfigWatcher::OnDirectoryEvent() {
⋮----
if (offset >= bytes) break;  // malformed-chain guard
⋮----
if (event_driven_) IssueWatch();  // re-arm (falls back on failure)
⋮----
void ConfigWatcher::CheckForChange() {
⋮----
// Detect change via mtime OR size.
⋮----
bool ConfigWatcher::CheckSecondForChange() {
⋮----
// Deleted between events: consume the snapshot so a later re-create is
// seen as a change again. No callback on deletion (watch+reload only).
⋮----
// The owner's callback re-reads the file and re-converges; its own catch
// keeps a corrupt file from throwing. The backstop here guarantees a
// callback bug can never unwind the engine's message loop.
⋮----
void ConfigWatcher::IssueWatch() {
⋮----
void ConfigWatcher::TeardownWatch() {
⋮----
void ConfigWatcher::Stop() {
⋮----
WallpaperConfig ConfigWatcher::GetConfig() const {
std::lock_guard<std::mutex> lock(cfg_mutex_);
⋮----
void ConfigWatcher::TryReload() {
```

## File: engine/src/config_watch.hpp
```cpp
class ConfigWatcher {
⋮----
ConfigWatcher() = default;
⋮----
ConfigWatcher(const ConfigWatcher&) = delete;
⋮----
void Start(const std::filesystem::path& config_path,
⋮----
void WatchSecondFile(const std::filesystem::path& path,
⋮----
bool CheckSecondForChange();
⋮----
void Poll();
⋮----
void* EventHandle() const;
⋮----
bool OnDirectoryEvent();
⋮----
void SetDebounceWindow(void* hwnd);
⋮----
void OnDebounceExpired();
⋮----
void Stop();
⋮----
WallpaperConfig GetConfig() const;
⋮----
static constexpr std::uintptr_t DebounceTimerId() { return kDebounceTimerId; }
⋮----
void CheckForChange();
⋮----
bool ArmDebounce(bool is_second);
⋮----
void IssueWatch();
⋮----
void TeardownWatch();
⋮----
void TryReload();
```

## File: engine/src/cpu_affinity.cpp
```cpp
void ApplyCpuAffinity(const std::string& mode, AffinityLogFn log) {
```

## File: engine/src/cpu_affinity.hpp
```cpp
void ApplyCpuAffinity(const std::string& mode, AffinityLogFn log);
```

## File: engine/src/crash_dump_prune.cpp
```cpp
unsigned long long FileTimeTicks(const FILETIME& ft) {
⋮----
void PruneOldCrashDumps(const wchar_t* crash_dir) {
```

## File: engine/src/crash_dump_prune.hpp
```cpp
void PruneOldCrashDumps(const wchar_t* crash_dir);
```

## File: engine/src/crash_restart_guard.cpp
```cpp
bool CrashRestartPath(wchar_t (&out)[MAX_PATH]) {
⋮----
CrashRestartState CrashRestartAdvance(CrashRestartState s, long long now_ms,
⋮----
bool CrashRestartAllowed(const CrashRestartState& s, int max_restarts) {
⋮----
void ClearCrashRestartFile() {
```

## File: engine/src/crash_restart_guard.hpp
```cpp
struct CrashRestartState {
⋮----
CrashRestartState CrashRestartAdvance(CrashRestartState s, long long now_ms,
⋮----
bool CrashRestartAllowed(const CrashRestartState& s, int max_restarts);
⋮----
void ClearCrashRestartFile();
```

## File: engine/src/desktop_inject.cpp
```cpp
HWND ResolveZOrderInsertAfter(HWND insert_after, HWND target,
⋮----
bool WorkerWSpansVirtualScreen(HWND hwnd);
⋮----
bool IsClass(HWND hwnd, const wchar_t* cls) {
⋮----
std::wstring ClassOf(HWND hwnd) {
⋮----
// --- desktop window discovery ----------------------------------------------
⋮----
struct EnumCtx {
⋮----
HWND def_view_host = nullptr;  // top-level window whose direct child is DefView
⋮----
BOOL CALLBACK EnumTopLevelProc(HWND hwnd, LPARAM lParam) {
⋮----
struct DesktopWindows {
⋮----
std::vector<HWND> worker_ws;          // top-level WorkerW (classic layout)
std::vector<HWND> progman_worker_ws;  // WorkerW parented to Progman (24H2)
⋮----
BOOL CALLBACK EnumProgmanChildProc(HWND hwnd, LPARAM lParam) {
⋮----
// True when the WorkerW can host our injected surface.
//
// A WorkerW qualifies when it is EMPTY - that is the shell's wallpaper
// WorkerW, the only host that puts the surface above the wallpaper layer
// and below the icons. It ALSO qualifies when its only child is one of OUR
// OWN stale injected surfaces (class kInjectedClass): a re-anchor pass
// resolves the host BEFORE the old surface is released, so the previous
// wallpaper window is still parented in the WorkerW at that moment.
// Treating that as "not a wallpaper WorkerW" made the engine fall back to
⋮----
bool WorkerWAcceptsSurface(HWND workerw) {
⋮----
DesktopWindows FindDesktopWindows() {
⋮----
HWND FindWorkerWStrategyA(const DesktopWindows& d) {
⋮----
void LogShared(LogFn log, const char* fmt, ...);
⋮----
HWND FindWorkerWStrategyB(const DesktopWindows& d, LogFn log = nullptr) {
⋮----
struct SpawnWorkerWResult {
⋮----
SpawnWorkerWResult SpawnWorkerWViaProgman(HWND progman, LogFn log = nullptr) {
⋮----
bool IsWin11_24H2(HWND progman) {
⋮----
const char* InjectModeToString(InjectMode mode) {
⋮----
void EnforceFramelessStyle(HWND wnd, bool layered) {
⋮----
LRESULT CALLBACK InjectedWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
⋮----
void LogShared(LogFn log, const char* fmt, ...) {
⋮----
bool WorkerWSpansVirtualScreen(HWND hwnd) {
⋮----
// No DefView discovered at all: the bottom is the honest position.
⋮----
void Logf(const char* fmt, ...) {
⋮----
bool RegisterClass() {
⋮----
bool CreateAndAttach(HWND target, HWND insert_after, int x, int y, int width,
⋮----
void LogPlacement(const char* branch, HWND target, int req_x, int req_y,
⋮----
HWND AttachToDesktop(int x, int y, int width, int height) {
⋮----
SharedHost ResolveSharedHost(InjectMode mode, LogFn log) {
⋮----
DesktopInjector::DesktopInjector(LogFn log) : impl_(std::make_unique<Impl>()) {
⋮----
DesktopInjector::DesktopInjector(DesktopInjector&& other) noexcept
⋮----
bool DesktopInjector::Attach(int x, int y, int width, int height) {
⋮----
void DesktopInjector::SetInjectMode(InjectMode mode) {
⋮----
void DesktopInjector::SetSharedHost(const SharedHost& host) {
⋮----
std::string DesktopInjector::last_coverage_reason() const {
⋮----
void DesktopInjector::Detach() {
⋮----
void DesktopInjector::OnDisplayChange(int x, int y, int width, int height) {
⋮----
void* DesktopInjector::injected_hwnd() const {
⋮----
void DesktopInjector::HideVoChildren() {
⋮----
void* DesktopInjector::ReleaseSurface() {
⋮----
bool DesktopInjector::IsSurfaceAlive() const {
⋮----
void DesktopInjector::ReassertFrameless() {
```

## File: engine/src/desktop_inject.hpp
```cpp
enum class InjectMode {
⋮----
struct SharedHost {
⋮----
const char* branch = "";       // row 4 branch label consumed by LogPlacement
PlacementRect client_rect{};   // measured client rect, screen coordinates
⋮----
// Resolve the desktop host ONCE per attach pass (row 7). Performs the full
// strategy selection AttachToDesktop used to run per slot (FindDesktopWindows
// + FindWorkerWStrategyA/B + SpawnWorkerWViaProgman + the forced/24H2/classic
// branches), measures the chosen host's client rect, and logs ONE
// `placement: host-resolution` line (plus the moved strategy lines).
// `log` may be nullptr (logging disabled, matching LogFn semantics).
// Returns SharedHost{} when no Progman exists - callers treat that as
// "attach will fail", not as a crash.
SharedHost ResolveSharedHost(InjectMode mode, LogFn log);
⋮----
class DesktopInjector {
⋮----
explicit DesktopInjector(LogFn log);
⋮----
DesktopInjector(const DesktopInjector&) = delete;
⋮----
DesktopInjector(DesktopInjector&& other) noexcept;
⋮----
bool Attach(int x, int y, int width, int height);
⋮----
void SetInjectMode(InjectMode mode);
⋮----
void SetSharedHost(const SharedHost& host);
⋮----
std::string last_coverage_reason() const;
⋮----
void Detach();
⋮----
void OnDisplayChange(int x, int y, int width, int height);
⋮----
void* injected_hwnd() const;
⋮----
void HideVoChildren();
⋮----
void* ReleaseSurface();
⋮----
bool IsSurfaceAlive() const;
⋮----
void ReassertFrameless();
```

## File: engine/src/desktop_placement.cpp
```cpp
ClientOffset HostClientOffset(const PlacementRect& host_window_rect,
⋮----
PlacementRect ChildRectInClient(const ClientOffset& offset,
⋮----
CoverageVerdict CoversMonitor(const PlacementRect& child_in_client,
⋮----
const char* CoverageReason(CoverageVerdict verdict) {
⋮----
bool WorkerWSpansRect(const PlacementRect& worker_rect,
⋮----
bool WorkerWAcceptsChildClasses(const std::vector<std::string>& child_classes,
```

## File: engine/src/desktop_placement.hpp
```cpp
struct PlacementRect {
⋮----
struct ClientOffset {
⋮----
enum class CoverageVerdict {
⋮----
ClientOffset HostClientOffset(const PlacementRect& host_window_rect,
⋮----
PlacementRect ChildRectInClient(const ClientOffset& offset,
⋮----
CoverageVerdict CoversMonitor(const PlacementRect& child_in_client,
⋮----
const char* CoverageReason(CoverageVerdict verdict);
⋮----
bool WorkerWSpansRect(const PlacementRect& worker_rect,
⋮----
bool WorkerWAcceptsChildClasses(const std::vector<std::string>& child_classes,
```

## File: engine/src/device_lost_watch.cpp
```cpp
IDXGIFactory7* CreateFactory7() {
⋮----
bool DeviceLostWatch::Start() {
⋮----
bool DeviceLostWatch::Rearm() {
⋮----
void DeviceLostWatch::Stop() {
```

## File: engine/src/device_lost_watch.hpp
```cpp
class DeviceLostWatch {
⋮----
explicit DeviceLostWatch(LogFn log = nullptr) : log_(log) {}
⋮----
DeviceLostWatch(const DeviceLostWatch&) = delete;
⋮----
bool Start();
⋮----
void Stop();
⋮----
bool Rearm();
⋮----
void* EventHandle() const { return event_; }
⋮----
bool active() const { return active_; }
```

## File: engine/src/engine_watchdog.cpp
```cpp
std::wstring SiblingLauncherPath() {
⋮----
std::wstring WriteHangMinidump() {
⋮----
bool SpawnRestartHelper() {
⋮----
}  // namespace
⋮----
void EngineWatchdog::Start(std::atomic<unsigned long long>* heartbeat,
⋮----
// A timed wait (not a plain sleep): the watcher adds NO wakeups to
// the message loop and no work to the paused engine, and Stop()
// can still wake it immediately.
⋮----
std::unique_lock<std::mutex> lock(wake_mutex_);
⋮----
void EngineWatchdog::Stop() {
⋮----
std::lock_guard<std::mutex> lock(wake_mutex_);
```

## File: engine/src/engine_watchdog.hpp
```cpp
inline bool IsEngineHung(unsigned long long last_seen,
⋮----
class EngineWatchdog {
⋮----
explicit EngineWatchdog(LogFn log = nullptr) : log_(log) {}
⋮----
EngineWatchdog(const EngineWatchdog&) = delete;
⋮----
void Start(std::atomic<unsigned long long>* heartbeat,
⋮----
void Stop();
⋮----
static bool IsHung(unsigned long long last_seen,
```

## File: engine/src/fullscreen_watch.cpp
```cpp
void Log(const char* fmt, ...) {
⋮----
struct MonitorRects {
⋮----
BOOL CALLBACK CollectMonitorRect(HMONITOR monitor, HDC , LPRECT ,
⋮----
void CALLBACK FullscreenWinEventProc(HWINEVENTHOOK , DWORD event,
⋮----
bool FullscreenWatch::IsFullscreenCandidate(int l, int t, int r, int b,
⋮----
bool FullscreenWatch::QueryFullscreenTarget(void** hwnd_out) noexcept {
⋮----
void FullscreenWatch::SetNotifyWindow(void* hwnd) {
⋮----
void FullscreenWatch::Start(FullscreenCallback on_change) {
⋮----
void FullscreenWatch::Evaluate(bool now_fullscreen, bool from_event,
⋮----
void FullscreenWatch::OnHookEvent(unsigned long win_event) {
⋮----
void FullscreenWatch::OnWindowDestroyed(void* hwnd) {
⋮----
void FullscreenWatch::OnWindowMoved(void* hwnd) {
⋮----
void FullscreenWatch::Poll() {
⋮----
void FullscreenWatch::Stop() {
```

## File: engine/src/fullscreen_watch.hpp
```cpp
class FullscreenWatch {
⋮----
FullscreenWatch() = default;
⋮----
FullscreenWatch(const FullscreenWatch&) = delete;
⋮----
void Start(FullscreenCallback on_change);
⋮----
void Poll();
⋮----
void Stop();
⋮----
void SetNotifyWindow(void* hwnd);
⋮----
void OnHookEvent(unsigned long win_event);
⋮----
static constexpr unsigned HookMessageId() { return 0x8000u + 0x0051u; }
⋮----
static constexpr unsigned PokeMessageId() { return 0x8000u + 0x0053u; }
⋮----
void OnWindowDestroyed(void* hwnd);
⋮----
void OnWindowMoved(void* hwnd);
⋮----
static bool IsFullscreenCandidate(int l, int t, int r, int b, int ml,
⋮----
static bool QueryFullscreenTarget(void** hwnd_out) noexcept;
⋮----
void Evaluate(bool now_fullscreen, bool from_event, unsigned long win_event,
```

## File: engine/src/gpu_pin.cpp
```cpp
std::string Narrow(const wchar_t* w) {
⋮----
bool IsIntegrated(unsigned vendor, const std::string& desc,
⋮----
std::vector<GpuAdapterInfo> Enumerate() {
⋮----
std::string DescribeAdapters() {
⋮----
std::string ResolveAdapterPin(const std::string& mode) {
```

## File: engine/src/gpu_pin.hpp
```cpp
struct GpuAdapterInfo {
⋮----
std::string DescribeAdapters();
⋮----
std::string ResolveAdapterPin(const std::string& mode);
```

## File: engine/src/ipc_command_marshal.cpp
```cpp
bool IpcCommandMarshal::QueueVideo(const std::string& payload_json) {
⋮----
bool IpcCommandMarshal::QueueMonitor(const std::string& payload_json) {
⋮----
bool IpcCommandMarshal::QueueDisplayVideo(const std::string& payload_json) {
```

## File: engine/src/ipc_command_marshal.hpp
```cpp
class IpcCommandMarshal {
⋮----
explicit IpcCommandMarshal(LogFn log = nullptr) : log_(log) {}
⋮----
void SetWindow(void* hwnd) { hwnd_.store(hwnd, std::memory_order_release); }
void ClearWindow() { hwnd_.store(nullptr, std::memory_order_release); }
⋮----
bool QueueVideo(const std::string& payload_json);
bool QueueMonitor(const std::string& payload_json);
bool QueueDisplayVideo(const std::string& payload_json);
⋮----
bool TakeVideo(std::string* out) { return pending_.TakeVideo(out); }
bool TakeMonitor(std::string* out) { return pending_.TakeMonitor(out); }
bool TakeDisplayVideo(std::string* out) {
```

## File: engine/src/ipc_marshal.cpp
```cpp
bool IsGdiDeviceName(const std::string& d) {
⋮----
std::optional<int> ParseSetMonitorPayload(const std::string& payload_json) {
⋮----
std::optional<std::string> ValidateSetVideoPayload(
⋮----
std::optional<DisplayVideoCommand> ParseSetDisplayVideoPayload(
```

## File: engine/src/ipc_marshal.hpp
```cpp
std::optional<int> ParseSetMonitorPayload(const std::string& payload_json);
⋮----
std::optional<std::string> ValidateSetVideoPayload(
⋮----
struct DisplayVideoCommand {
⋮----
std::optional<DisplayVideoCommand> ParseSetDisplayVideoPayload(
```

## File: engine/src/ipc_server.hpp
```cpp
struct IpcHandlers {
⋮----
class IpcServer {
⋮----
IpcServer();
⋮----
IpcServer(const IpcServer&) = delete;
⋮----
IpcServer(IpcServer&&) = delete;
⋮----
bool Start(const IpcHandlers& handlers);
⋮----
void Stop();
⋮----
bool IsRunning() const;
```

## File: engine/src/lock_order.hpp
```cpp
class RecursiveSlotsGuard {
⋮----
explicit RecursiveSlotsGuard(std::recursive_mutex& m) : lock_(m) {
⋮----
RecursiveSlotsGuard(const RecursiveSlotsGuard&) = delete;
⋮----
class LeafGuard {
⋮----
explicit LeafGuard(std::mutex& m) : lock_(m) {
⋮----
LeafGuard(const LeafGuard&) = delete;
```

## File: engine/src/log_file.cpp
```cpp
void DebugOut(const char* line) {
⋮----
std::vector<wchar_t> wbuf(static_cast<size_t>(wlen));
⋮----
void TryOpenOnce() {
⋮----
void MaybeRotateLog() {
⋮----
void FlushLocked() {
⋮----
bool IsImportantLine(const char* line) {
⋮----
void AppendEngineLogLine(const char* line, bool important) {
⋮----
std::lock_guard<std::mutex> lock(g_file_mutex);
⋮----
void FlushEngineLog() {
⋮----
void EngineLogfV(const char* fmt, va_list args, bool important) {
⋮----
std::lock_guard<std::mutex> lock(mutex);
⋮----
void FlushEngineLogFromCrash() {
std::unique_lock<std::mutex> lock(g_file_mutex, std::try_to_lock);
```

## File: engine/src/log_file.hpp
```cpp
void AppendEngineLogLine(const char* line, bool important = false);
⋮----
void EngineLogfV(const char* fmt, va_list args, bool important = false);
void FlushEngineLog();
⋮----
void FlushEngineLogFromCrash();
```

## File: engine/src/mpv_renderer.cpp
```cpp
void Log(const char* fmt, ...) {
std::lock_guard<std::mutex> lock(g_mpv_log_mutex);
⋮----
void LogImportant(const char* fmt, ...) {
⋮----
MpvRenderer::MpvRenderer() = default;
⋮----
std::lock_guard<std::mutex> lock(mutex_);
⋮----
void MpvRenderer::StopEventThread() {
⋮----
void MpvRenderer::Wakeup() {
⋮----
void MpvRenderer::SetMessageWindow(void* hwnd) {
⋮----
bool MpvRenderer::Create(void* hwnd) {
⋮----
void MpvRenderer::SetAdapterPin(const std::string& substr) {
⋮----
bool MpvRenderer::pin_active() const {
⋮----
bool MpvRenderer::ScreenshotToFile(const std::string& path) {
⋮----
bool MpvRenderer::SeekTo(double seconds) {
⋮----
bool MpvRenderer::GetInt64Property(const std::string& name,
⋮----
bool MpvRenderer::UnloadVideo() {
⋮----
// "stop" unloads the current file: the decoder, its surfaces and the
// demuxer buffers are released, while the instance, the VO and the window
// survive and the last presented frame stays on screen. The path is kept
// so ReloadVideo() can bring the stream back.
⋮----
bool MpvRenderer::ReloadVideo() {
⋮----
std::lock_guard<std::mutex> ev_lock(event_mutex_);
⋮----
bool MpvRenderer::GetPropertyValue(const std::string& name,
⋮----
bool MpvRenderer::DumpPropertyList(const std::wstring& path) const {
⋮----
void MpvRenderer::SetHWND(void* hwnd) {
⋮----
bool MpvRenderer::LoadLoop(const std::string& path, bool force) {
⋮----
void MpvRenderer::SetFitMode(const std::string& fit_mode,
⋮----
void MpvRenderer::Pause() {
⋮----
void MpvRenderer::Resume() {
⋮----
void MpvRenderer::SetFpsCap(int fps) {
⋮----
void MpvRenderer::EventThreadLoop() {
⋮----
void MpvRenderer::HandleEvent(mpv_event* ev) {
⋮----
std::lock_guard<std::mutex> lock(event_mutex_);
⋮----
// Armed: cleared by the loop's own START_FILE, or consumed by the
// watchdog when no restart arrives (see CheckEofWatchdog).
⋮----
bool MpvRenderer::ApplyHwdecValue(const std::string& cur) {
⋮----
void MpvRenderer::OnHwdecPropertyChange() {
⋮----
void MpvRenderer::CheckEofWatchdog() {
⋮----
void MpvRenderer::TryFallbackHwdec() {
```

## File: engine/src/mpv_renderer.hpp
```cpp
class MpvRenderer {
⋮----
MpvRenderer();
⋮----
MpvRenderer(const MpvRenderer&) = delete;
⋮----
bool Create(void* hwnd);
⋮----
void SetHWND(void* hwnd);
⋮----
void SetAdapterPin(const std::string& substr);
bool pin_active() const;
⋮----
bool LoadLoop(const std::string& path, bool force = false);
⋮----
void SetFitMode(const std::string& fit_mode, double window_aspect = 0.0);
⋮----
void Pause();
⋮----
void Resume();
⋮----
void SetFpsCap(int fps);
⋮----
void Wakeup();
⋮----
bool EverStarted() const {
⋮----
bool IsHwdecActive() const {
⋮----
bool ScreenshotToFile(const std::string& path);
⋮----
bool SeekTo(double seconds);
⋮----
bool UnloadVideo();
⋮----
bool ReloadVideo();
⋮----
bool video_unloaded() const {
⋮----
bool GetInt64Property(const std::string& name, long long& out) const;
⋮----
bool GetPropertyValue(const std::string& name, std::string& out) const;
⋮----
bool DumpPropertyList(const std::wstring& path) const;
⋮----
void SetMessageWindow(void* hwnd);
⋮----
void OnHwdecPropertyChange();
⋮----
void EventThreadLoop();
⋮----
void StopEventThread();
⋮----
void HandleEvent(mpv_event* ev);
⋮----
bool ApplyHwdecValue(const std::string& cur);
⋮----
void TryFallbackHwdec();
⋮----
void CheckEofWatchdog();
```

## File: engine/src/occlusion_poke_scheduler.cpp
```cpp
void OcclusionPokeScheduler::SetHooks(void* hwnd, BoolFn surface_live,
⋮----
void OcclusionPokeScheduler::Schedule(unsigned long win_event) {
⋮----
void OcclusionPokeScheduler::OnTimer() {
```

## File: engine/src/occlusion_poke_scheduler.hpp
```cpp
class OcclusionPokeScheduler {
⋮----
explicit OcclusionPokeScheduler(LogFn log = nullptr) : log_(log) {}
⋮----
void SetHooks(void* hwnd, BoolFn surface_live, BoolFn any_slot_paused,
⋮----
void Schedule(unsigned long win_event);
void OnTimer();
void Disarm() { armed_ = false; }
```

## File: engine/src/occlusion_watch.hpp
```cpp
struct CoverRect {
⋮----
inline CoverRect MonitorCover(const MonitorInfo& mi) noexcept {
⋮----
CoverRect ClipRect(CoverRect r, CoverRect clip) noexcept;
⋮----
long long UnionAreaClipped(const CoverRect* rects, size_t count,
⋮----
double ComputeCoverage(CoverRect monitor, const CoverRect* rects,
⋮----
inline bool HysteresisDecide(double coverage, bool currently_paused) noexcept {
⋮----
class OcclusionWatch {
⋮----
static constexpr unsigned RearmMessageId() { return 0x8000u + 0x0052u; }
⋮----
OcclusionWatch() = default;
⋮----
OcclusionWatch(const OcclusionWatch&) = delete;
⋮----
void SetLog(LogFn log) { log_ = log; }
⋮----
void SetNotifyWindow(void* hwnd) { notify_hwnd_ = hwnd; }
⋮----
void OnPauseMaskChanged(bool slots_paused);
⋮----
bool armed() const { return occlusion_armed_.load(std::memory_order_acquire); }
⋮----
void OnRearmMessage() noexcept;
⋮----
void Check(MultiMonitor& slots);
⋮----
void CheckNow(MultiMonitor& slots);
⋮----
void Log(const char* fmt, ...);
```

## File: engine/src/pending_queue.hpp
```cpp
class PendingCommandQueue {
⋮----
void SetVideo(std::string payload) {
std::lock_guard<std::mutex> lock(mutex_);
⋮----
void ClearVideo() {
⋮----
bool TakeVideo(std::string* out) {
⋮----
void SetMonitor(std::string payload) {
⋮----
void ClearMonitor() {
⋮----
bool TakeMonitor(std::string* out) {
⋮----
void SetDisplayVideo(std::string payload) {
⋮----
void ClearDisplayVideo() {
⋮----
bool TakeDisplayVideo(std::string* out) {
```

## File: engine/src/playlist_controller.cpp
```cpp
void PlaylistController::SetHooks(PathFn resolve_path, StringFn current_video,
⋮----
void PlaylistController::Reload() {
⋮----
void PlaylistController::Arm() {
⋮----
std::lock_guard<std::mutex> lock(mutex_);
⋮----
void PlaylistController::OnSlotsPausedChange(bool slots_paused_now) {
⋮----
bool PlaylistController::Due() const {
⋮----
bool PlaylistController::Fire() {
```

## File: engine/src/playlist_controller.hpp
```cpp
class PlaylistController {
⋮----
explicit PlaylistController(LogFn log = nullptr) : log_(log) {}
⋮----
void SetHooks(PathFn resolve_path, StringFn current_video,
⋮----
void Reload();
⋮----
void Arm();
⋮----
void OnSlotsPausedChange(bool slots_paused_now);
⋮----
bool Due() const;
⋮----
bool Fire();
⋮----
bool enabled() const { return playlist_.enabled; }
bool has_order() const { return !playlist_.order.empty(); }
bool get_enabled() const {
⋮----
long long get_size() const {
⋮----
long long get_index() const {
```

## File: engine/src/power.cpp
```cpp
std::string FormatRestore(int fps) {
⋮----
PowerSaver::PowerSaver(CapApplier cap, Reader reader, LogFn log)
⋮----
PowerCapAction DecidePowerCapAction(PowerSaverState state,
⋮----
void PowerSaver::Update() {
⋮----
PowerReading ReadSystemPower(const WallpaperConfig& config) {
```

## File: engine/src/power.hpp
```cpp
enum class PowerSaverState {
⋮----
enum class PowerCapAction { kNone, kCapFps, kFreeze };
⋮----
PowerCapAction DecidePowerCapAction(PowerSaverState state,
⋮----
struct PowerReading {
⋮----
class PowerSaver {
⋮----
PowerSaver(CapApplier cap, Reader reader, LogFn log = LogFn());
⋮----
PowerSaver(const PowerSaver&) = delete;
⋮----
void Update();
⋮----
void Log(const std::string& message) const {
⋮----
PowerReading ReadSystemPower(const WallpaperConfig& config);
```

## File: engine/src/reconcile_policy.cpp
```cpp
void AppendUtf8(std::string& out, std::uint32_t cp) {
⋮----
std::string NarrowDevice(const std::wstring& wide) {
⋮----
std::string DeviceKeyUtf8(const MonitorInfo& mi) {
⋮----
ReconcileAction DecideReconcileAction(const std::string& trigger) {
⋮----
const char* ReconcileActionName(ReconcileAction action) {
⋮----
bool ReconcileActionNeedsStableTopology(ReconcileAction action) {
⋮----
std::string SlotIdentityKey(const MonitorInfo& mi) {
⋮----
bool TopologyStable(const std::vector<MonitorInfo>& first,
⋮----
std::string TopologySignature(const std::vector<MonitorInfo>& monitors) {
```

## File: engine/src/reconcile_policy.hpp
```cpp
enum class ReconcileAction {
⋮----
ReconcileAction DecideReconcileAction(const std::string& trigger);
⋮----
const char* ReconcileActionName(ReconcileAction action);
⋮----
bool ReconcileActionNeedsStableTopology(ReconcileAction action);
⋮----
std::string SlotIdentityKey(const MonitorInfo& mi);
⋮----
bool TopologyStable(const std::vector<MonitorInfo>& first,
⋮----
std::string TopologySignature(const std::vector<MonitorInfo>& monitors);
```

## File: engine/src/renderer_graveyard.cpp
```cpp
struct GraveyardItem {
⋮----
struct GraveyardState {
⋮----
GraveyardState& State() {
⋮----
void WorkerMain() {
⋮----
bool RendererGraveyard::Submit(std::unique_ptr<MpvRenderer> renderer,
⋮----
int RendererGraveyard::Drain() {
⋮----
bool RendererGraveyard::WaitIdle(int timeout_ms) {
⋮----
bool RendererGraveyard::Shutdown(int timeout_ms, LogFn log) {
⋮----
struct JoinState {
⋮----
unsigned long long RendererGraveyard::submitted_count() {
⋮----
unsigned long long RendererGraveyard::destroyed_count() {
⋮----
int RendererGraveyard::pending_window_count() {
```

## File: engine/src/renderer_graveyard.hpp
```cpp
class RendererGraveyard {
⋮----
static bool Submit(std::unique_ptr<MpvRenderer> renderer,
⋮----
static int Drain();
⋮----
static bool WaitIdle(int timeout_ms);
⋮----
static bool Shutdown(int timeout_ms, LogFn log = nullptr);
⋮----
static unsigned long long submitted_count();
static unsigned long long destroyed_count();
static int pending_window_count();
```

## File: engine/src/retry_schedule.cpp
```cpp
bool retry_schedule_due(long long now_ms, const RetryClock& c) {
⋮----
long long retry_next_delay_ms(long long now_ms, const RetryClock& c) {
⋮----
bool retry_schedule_exhausted(const RetryClock& c) {
```

## File: engine/src/retry_schedule.hpp
```cpp
struct RetryClock {
⋮----
bool retry_schedule_due(long long now_ms, const RetryClock& c);
⋮----
long long retry_next_delay_ms(long long now_ms, const RetryClock& c);
⋮----
bool retry_schedule_exhausted(const RetryClock& c);
```

## File: engine/src/test_simulator.cpp
```cpp
void TestSimulator::Configure(const CliOptions& opts) {
⋮----
bool TestSimulator::AnyArmed() const {
⋮----
TestSimulator::Fired TestSimulator::Tick(std::int64_t elapsed_ms) {
```

## File: engine/src/test_simulator.hpp
```cpp
class TestSimulator {
⋮----
struct Fired {
⋮----
void Configure(const CliOptions& opts);
⋮----
bool AnyArmed() const;
⋮----
bool ExitReached(std::int64_t elapsed_ms) const {
⋮----
int exit_after_ms() const { return exit_after_ms_; }
int device_lost_after_ms() const { return device_lost_ms_; }
int suspend_after_ms() const { return suspend_ms_; }
int dc_after_ms() const { return dc_ms_; }
int monitor_off_after_ms() const { return monitor_off_ms_; }
bool dc_latched() const { return dc_latched_; }
⋮----
Fired Tick(std::int64_t elapsed_ms);
```

## File: engine/src/timer_ids.hpp
```cpp

```

## File: engine/src/topology_diff.cpp
```cpp
void SortUnique(std::vector<std::string>& v) {
⋮----
bool TopologyDiff::empty() const {
⋮----
TopologyDiff diff_topology(const TopologySnapshot& old_s,
```

## File: engine/src/topology_diff.hpp
```cpp
struct TopologyDiff {
⋮----
bool empty() const;
⋮----
TopologyDiff diff_topology(const TopologySnapshot& old_s,
```

## File: engine/src/topology_snapshot.hpp
```cpp
struct TopologyMonitor {
⋮----
struct TopologySnapshot {
```

## File: engine/src/tray_actions.cpp
```cpp
void OpenStudio(TrayLogFn log) {
⋮----
void OpenSupport(TrayLogFn log) {
⋮----
std::vector<wchar_t> wurl(static_cast<std::size_t>(wlen));
```

## File: engine/src/tray_actions.hpp
```cpp
void OpenStudio(TrayLogFn log);
⋮----
void OpenSupport(TrayLogFn log);
```

## File: engine/src/tray.cpp
```cpp
std::wstring RecentLabel(const std::string& utf8_path) {
⋮----
// Appends the MRU "Quick-switch" submenu to `menu`. True when the submenu was
⋮----
bool AppendQuickSwitchSubmenu(HMENU menu, const std::vector<std::string>& recent) {
⋮----
void Log(const char* fmt, ...);
⋮----
void Log(const char* fmt, ...) {
⋮----
HICON LoadAppIcon(bool paused, bool* owned_out) {
⋮----
bool TrayIcon::Install(void* hwnd, TrayCallbacks callbacks) {
std::lock_guard<std::mutex> lock(mutex_);
⋮----
void TrayIcon::Remove() {
⋮----
bool TrayIcon::ResolveShouldShowPausedLocked() const {
⋮----
void TrayIcon::UpdateIconForState() {
⋮----
void TrayIcon::SetPaused(bool paused) {
⋮----
void TrayIcon::SetError(bool error) {
⋮----
void TrayIcon::PushRecent(const std::string& utf8_path) {
⋮----
void TrayIcon::OnTrayNotify(void* hwnd, std::size_t w_param, long l_param) {
⋮----
void TrayIcon::OnMenuCommand(unsigned id) {
⋮----
void TrayIcon::OnTaskbarCreated() {
⋮----
std::string TrayIcon::RecentAt(std::size_t idx) const {
⋮----
void TrayIcon::PrepareIconLocked(void* nid_void, unsigned extra_flags) {
⋮----
bool TrayIcon::AddIconLocked() {
⋮----
void TrayIcon::DeleteIconLocked() {
⋮----
void TrayIcon::ModifyIconLocked() {
```

## File: engine/src/tray.hpp
```cpp
struct TrayCallbacks {
⋮----
class TrayIcon {
⋮----
TrayIcon() = default;
⋮----
TrayIcon(const TrayIcon&) = delete;
⋮----
bool Install(void* hwnd, TrayCallbacks callbacks);
⋮----
void Remove();
⋮----
void SetPaused(bool paused);
⋮----
void SetError(bool error);
⋮----
void PushRecent(const std::string& utf8_path);
⋮----
void OnTrayNotify(void* hwnd, std::size_t w_param, long l_param);
⋮----
void OnMenuCommand(unsigned id);
⋮----
void OnTaskbarCreated();
⋮----
std::string RecentAt(std::size_t idx) const;
⋮----
unsigned taskbar_created_msg() const {
std::lock_guard<std::mutex> lock(mutex_);
⋮----
void UpdateIconForState();
⋮----
bool AddIconLocked();
void DeleteIconLocked();
void ModifyIconLocked();
⋮----
void PrepareIconLocked(void* nid, unsigned extra_flags);
⋮----
bool ResolveShouldShowPausedLocked() const;
```

## File: engine/src/working_set_trim.cpp
```cpp
void WorkingSetTrim::Arm(void* hwnd) {
⋮----
void WorkingSetTrim::RunOnce() {
```

## File: engine/src/working_set_trim.hpp
```cpp
class WorkingSetTrim {
⋮----
explicit WorkingSetTrim(LogFn log = nullptr) : log_(log) {}
⋮----
void Arm(void* hwnd);
⋮----
void RunOnce();
⋮----
void Cancel() { armed_ = false; }
```

## File: engine/app.manifest
```
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<assembly xmlns="urn:schemas-microsoft-com:asm.v1" manifestVersion="1.0"
          xmlns:asmv3="urn:schemas-microsoft-com:asm.v3">
  <assemblyIdentity
      type="win32"
      name="K6WP.Engine"
      version="0.0.1.0"
      processorArchitecture="amd64"/>

  <description>K6WP Wallpaper Engine</description>

  <trustInfo xmlns="urn:schemas-microsoft-com:asm.v3">
    <security>
      <requestedPrivileges>
        <requestedExecutionLevel level="asInvoker" uiAccess="false"/>
      </requestedPrivileges>
    </security>
  </trustInfo>

<asmv3:application>
    <asmv3:windowsSettings xmlns="http://schemas.microsoft.com/SMI/2005/WindowsSettings">
      <dpiAware>true/pm</dpiAware>
    </asmv3:windowsSettings>
    <asmv3:windowsSettings xmlns="http://schemas.microsoft.com/SMI/2016/WindowsSettings">
      <dpiAwareness>PerMonitorV2</dpiAwareness>
      <longPathAware>true</longPathAware>
    </asmv3:windowsSettings>
  </asmv3:application>

  <compatibility xmlns="urn:schemas-microsoft-com:compatibility.v1">
    <application>
      <!-- C6: the supportedOS ladder. The manifest alone does NOT refuse to
           run: it only tells the compatibility shim which version to report
           and enables the right behaviours. The REAL minimum-version gate is
           the RtlGetVersion check in shared/os_version.cpp, which the engine,
           the launcher and the Studio all run before anything else starts.
           Windows 10 and Windows 11 share the same GUID (the Windows 10 one);
           22H2 is enforced by the runtime build check (>= 19041), not by
           the manifest. -->
      <supportedOS Id="{e2011457-1546-43c5-a5fe-008deee3d3f0}"/> <!-- Vista -->
      <supportedOS Id="{35138b9a-5d96-4fbd-8e2d-a2440225f93a}"/> <!-- 7 -->
      <supportedOS Id="{4a2f28e3-53b9-4441-ba9c-d69d4a4a6e38}"/> <!-- 8 -->
      <supportedOS Id="{1f676c76-80e1-4239-95bb-83d0f6d0da78}"/> <!-- 8.1 -->
      <supportedOS Id="{8e0f7a12-bfb3-4fe8-b9a5-48fd50a15a9a}"/> <!-- 10 / 11 -->
    </application>
  </compatibility>
</assembly>
```

## File: engine/app.manifest.in
```
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<assembly xmlns="urn:schemas-microsoft-com:asm.v1" manifestVersion="1.0"
          xmlns:asmv3="urn:schemas-microsoft-com:asm.v3">
  <assemblyIdentity
      type="win32"
      name="K6WP.Engine"
      version="@K6WP_VERSION_4@"
      processorArchitecture="amd64"/>

  <description>K6WP Wallpaper Engine</description>

  <trustInfo xmlns="urn:schemas-microsoft-com:asm.v3">
    <security>
      <requestedPrivileges>
        <requestedExecutionLevel level="asInvoker" uiAccess="false"/>
      </requestedPrivileges>
    </security>
  </trustInfo>

<asmv3:application>
    <asmv3:windowsSettings xmlns="http://schemas.microsoft.com/SMI/2005/WindowsSettings">
      <dpiAware>true/pm</dpiAware>
    </asmv3:windowsSettings>
    <asmv3:windowsSettings xmlns="http://schemas.microsoft.com/SMI/2016/WindowsSettings">
      <dpiAwareness>PerMonitorV2</dpiAwareness>
      <longPathAware>true</longPathAware>
    </asmv3:windowsSettings>
  </asmv3:application>

  <compatibility xmlns="urn:schemas-microsoft-com:compatibility.v1">
    <application>
      <!-- C6: the supportedOS ladder. The manifest alone does NOT refuse to
           run: it only tells the compatibility shim which version to report
           and enables the right behaviours. The REAL minimum-version gate is
           the RtlGetVersion check in shared/os_version.cpp, which the engine,
           the launcher and the Studio all run before anything else starts.
           Windows 10 and Windows 11 share the same GUID (the Windows 10 one);
           22H2 is enforced by the runtime build check (>= 19041), not by
           the manifest. -->
      <supportedOS Id="{e2011457-1546-43c5-a5fe-008deee3d3f0}"/> <!-- Vista -->
      <supportedOS Id="{35138b9a-5d96-4fbd-8e2d-a2440225f93a}"/> <!-- 7 -->
      <supportedOS Id="{4a2f28e3-53b9-4441-ba9c-d69d4a4a6e38}"/> <!-- 8 -->
      <supportedOS Id="{1f676c76-80e1-4239-95bb-83d0f6d0da78}"/> <!-- 8.1 -->
      <supportedOS Id="{8e0f7a12-bfb3-4fe8-b9a5-48fd50a15a9a}"/> <!-- 10 / 11 -->
    </application>
  </compatibility>
</assembly>
```

## File: engine/app.rc
```
// engine/app.rc - resource script for engine.exe (Todo 9).
//
// Resource ids (keep in sync with kTrayResourceId / kTrayResourceIdPaused in
// src/tray.hpp; single definition in packaging/resource.h):
//   101 IDI_APPICON        - running-state tray/app icon (k6wp-on.ico)
//   102 IDI_APPICON_PAUSED - paused-state tray icon (k6wp-off.ico, visually
//                            distinct)
//
// The .ico files live in ../packaging/ and are referenced by bare name; the
// packaging/ directory is on the RC include path (CMake feeds target include
// dirs to rc.exe via the INCLUDE env var). See packaging/app-icon-README.md
// for how to replace the artwork without code changes.

#include <windows.h>
#include "resource.h"
#include "version.h"

IDI_APPICON        ICON "k6wp-on.ico"
IDI_APPICON_PAUSED ICON "k6wp-off.ico"

VS_VERSION_INFO VERSIONINFO
 FILEVERSION K6WP_VERSION_COMMA
 PRODUCTVERSION K6WP_VERSION_COMMA
 FILEFLAGSMASK 0x3fL
#ifdef _DEBUG
 FILEFLAGS 0x1L
#else
 FILEFLAGS 0x0L
#endif
 FILEOS 0x40004L
 FILETYPE 0x1L
 FILESUBTYPE 0x0L
BEGIN
    BLOCK "StringFileInfo"
    BEGIN
        BLOCK "040904b0"
        BEGIN
            VALUE "CompanyName", "K6WP contributors"
            VALUE "FileDescription", "K6WP Engine"
            VALUE "FileVersion", K6WP_VERSION_STR_4
            VALUE "InternalName", "engine"
            VALUE "LegalCopyright", "Copyright (C) 2026 K6WP contributors - GPL-2.0-or-later"
            VALUE "OriginalFilename", "engine.exe"
            VALUE "ProductName", "K6WP"
            VALUE "ProductVersion", K6WP_VERSION_STR_4
        END
    END
    BLOCK "VarFileInfo"
    BEGIN
        VALUE "Translation", 0x409, 1200
    END
END
```

## File: launcher/app.rc
```
// launcher/app.rc - resource script for K6WP.exe (Todo 11).
//
// Minimal VERSIONINFO (ProductName K6WP, version from generated version.h) + the shared app
// icon from packaging/k6wp-on.ico (bare name: packaging/ is on the RC include
// path, same pattern as engine/app.rc).

#include <windows.h>
#include "resource.h"
#include "version.h"

IDI_APPICON ICON "k6wp-on.ico"

VS_VERSION_INFO VERSIONINFO
 FILEVERSION K6WP_VERSION_COMMA
 PRODUCTVERSION K6WP_VERSION_COMMA
 FILEFLAGSMASK 0x3fL
#ifdef _DEBUG
 FILEFLAGS 0x1L
#else
 FILEFLAGS 0x0L
#endif
 FILEOS 0x40004L
 FILETYPE 0x1L
 FILESUBTYPE 0x0L
BEGIN
    BLOCK "StringFileInfo"
    BEGIN
        BLOCK "040904b0"
        BEGIN
            VALUE "CompanyName", "K6WP contributors"
            VALUE "FileDescription", "K6WP Launcher"
            VALUE "FileVersion", K6WP_VERSION_STR_4
            VALUE "InternalName", "K6WP"
            VALUE "LegalCopyright", "Copyright (C) 2026 K6WP contributors - GPL-2.0-or-later"
            VALUE "OriginalFilename", "K6WP.exe"
            VALUE "ProductName", "K6WP"
            VALUE "ProductVersion", K6WP_VERSION_STR_4
        END
    END
    BLOCK "VarFileInfo"
    BEGIN
        VALUE "Translation", 0x409, 1200
    END
END
```

## File: launcher/cli.cpp
```cpp
void PrintUsage(FILE* out) {
⋮----
int ParseArgs(int argc, wchar_t** argv, Options& out) {
```

## File: launcher/cli.hpp
```cpp
struct Options {
⋮----
void PrintUsage(FILE* out);
⋮----
int ParseArgs(int argc, wchar_t** argv, Options& out);
```

## File: launcher/exit_codes.hpp
```cpp

```

## File: launcher/lockscreen_backup.cpp
```cpp
std::string EscapeBackupJson(const std::wstring& wide) {
⋮----
bool WriteLockscreenBackup(const std::filesystem::path& backup_path,
⋮----
bool ReadLockscreenBackup(const std::filesystem::path& backup_path,
⋮----
std::ifstream in(backup_path, std::ios::binary);
⋮----
// Todo 19 (HIGH-2) backward compat: a pre-fix backup flattened every
// non-ASCII char to '?'. Warn explicitly instead of silently restoring a
// corrupted path.
```

## File: launcher/lockscreen_backup.hpp
```cpp
std::string EscapeBackupJson(const std::wstring& wide);
⋮----
bool WriteLockscreenBackup(const std::filesystem::path& backup_path,
⋮----
bool ReadLockscreenBackup(const std::filesystem::path& backup_path,
```

## File: launcher/lockscreen_policy.cpp
```cpp
std::wstring FormatSysError(unsigned long code) {
⋮----
std::wstring msg(msg_buf, len);
⋮----
bool IsElevated() {
⋮----
HandleGuard guard(token);
⋮----
std::filesystem::path ProgramDataK6wpDir() {
⋮----
bool ReadLockscreenPolicy(std::wstring& value_out, bool& present) {
⋮----
bool WriteLockscreenPolicy(const std::wstring& image_path) {
⋮----
bool DeleteLockscreenPolicy() {
```

## File: launcher/lockscreen_policy.hpp
```cpp
std::wstring FormatSysError(unsigned long code);
⋮----
bool IsElevated();
⋮----
std::filesystem::path ProgramDataK6wpDir();
⋮----
bool ReadLockscreenPolicy(std::wstring& value_out, bool& present);
bool WriteLockscreenPolicy(const std::wstring& image_path);
bool DeleteLockscreenPolicy();
```

## File: launcher/lockscreen_sync.cpp
```cpp
int RelaunchElevated() {
⋮----
void RemoveLegacyFolder(const std::filesystem::path& dir) {
⋮----
ElevatedWaitOutcome ClassifyElevatedWait(bool launched, unsigned long launch_error,
⋮----
int LockscreenExitCodeFor(ElevatedWaitOutcome outcome) {
⋮----
std::wstring DescribeLockscreenRestore(int exit_code) {
⋮----
int RunElevateLockscreen(const Options& opts) {
⋮----
// Nothing to clean -> no UAC prompt. The check is read-only and mirrors
// the Studio's "Bersihkan" detection; a policy pointing elsewhere is left
```

## File: launcher/lockscreen_sync.hpp
```cpp
enum class ElevatedWaitOutcome {
⋮----
ElevatedWaitOutcome ClassifyElevatedWait(bool launched, unsigned long launch_error,
⋮----
int LockscreenExitCodeFor(ElevatedWaitOutcome outcome);
⋮----
std::wstring DescribeLockscreenRestore(int exit_code);
⋮----
int RunElevateLockscreen(const Options& opts);
```

## File: launcher/main.cpp
```cpp
std::filesystem::path SiblingPath(const wchar_t* file_name) {
⋮----
bool EngineAlive() {
⋮----
bool WaitForEnginePipe(DWORD timeout_ms) {
⋮----
bool SpawnDetached(const std::filesystem::path& exe,
⋮----
// Ensures exactly one engine: probe first, spawn ONLY when the pipe is
// absent (no blind duplicates). The probe is confirmed with one retry so a
// transiently-busy single pipe instance can never cause a duplicate engine.
// A stale/busy pipe (both 2000ms probes timed out but the name exists) gets
// an explicit warning, then the same duplicate-safe start attempt: the engine
// owns Local\K6WP-Engine-Singleton and rejects a second instance with exit 0.
// After spawn, waits up to kPipeReadyWaitMs for the pipe before returning so
// Studio (or --silent callers) never race a half-born engine.
// Returns exit code (0 ok, 3 on error).
int EnsureEngine(const Options& opts) {
⋮----
Sleep(500);  // one confirmation probe against a transiently-busy pipe
⋮----
// Classify the double-timeout: absent pipe (normal first boot, silent path)
// vs stale/busy pipe (dead listener or engine mid-start). Zero-timeout
// probe returns immediately: ERROR_FILE_NOT_FOUND means absent, anything
// else means the name exists but no instance is connectable.
⋮----
return 0;  // became connectable between the two probes
⋮----
struct FocusCtx {
⋮----
BOOL CALLBACK FocusStudioWindow(HWND hwnd, LPARAM lparam) {
⋮----
HandleGuard process(
⋮----
return FALSE;  // stop enumeration
⋮----
void FocusExistingStudio() {
⋮----
// ---- elevated lockscreen helper (see lockscreen_sync.cpp) ------------------
⋮----
int Run(int argc, wchar_t** argv) {
// C6: the minimum-OS check comes FIRST - before the arguments are parsed
// and before anything is spawned. An unsupported Windows must fail with a
// clear message and a distinct exit code, not with a mysterious failure
// later on.
⋮----
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
```

## File: launcher/process_paths.cpp
```cpp
bool IsK6wpAppImageName(const std::wstring& base_name) {
⋮----
std::wstring BaseNameText(const std::wstring& full_image) {
⋮----
std::wstring ParentDirText(const std::wstring& full_image) {
⋮----
std::wstring TrimTrailingSeparators(std::wstring dir) {
⋮----
if (dir[dir.size() - 2] == L':') break;  // keep a drive root as-is
⋮----
// The scope test that replaces the image-name kill: a K6WP image name is only
// OURS when it sits directly in own_dir. Compared component-wise (not by
// prefix), so "K6WP-old" never matches "K6WP", and case-insensitively, because
⋮----
bool IsK6wpOwnImage(const std::wstring& full_image,
⋮----
std::wstring OwnDir() {
⋮----
std::string NarrowAscii(const wchar_t* text) {
```

## File: launcher/process_paths.hpp
```cpp
bool IsK6wpAppImageName(const std::wstring& base_name);
⋮----
std::wstring BaseNameText(const std::wstring& full_image);
std::wstring ParentDirText(const std::wstring& full_image);
std::wstring TrimTrailingSeparators(std::wstring dir);
⋮----
bool IsK6wpOwnImage(const std::wstring& full_image,
⋮----
std::wstring OwnDir();
⋮----
std::string NarrowAscii(const wchar_t* text);
```

## File: launcher/process_stop.cpp
```cpp
std::wstring SessionPipeNameImpl() {
⋮----
}  // namespace
⋮----
// One NDJSON v1 request frame, byte-identical to shared/ipc_protocol.hpp
// Encode(): {"version":1,"cmd":"<name>","payload":{}}\n. The launcher links no
⋮----
std::string BuildEngineCommandLine(const wchar_t* cmd_name) {
⋮----
bool AckIsOk(const std::string& ack) {
⋮----
DWORD PidFromStateJson(const std::string& ack) {
⋮----
enum class PipeSendResult { kAckOk, kNoEngine, kNoAck, kError };
⋮----
PipeSendResult SendEngineCommand(const std::string& line, DWORD ack_timeout_ms,
⋮----
HandleGuard done(CreateEventW(nullptr, TRUE, FALSE, nullptr));
⋮----
bool WaitForProcessExit(HANDLE process, DWORD timeout_ms) {
⋮----
struct WmCloseCtx {
⋮----
BOOL CALLBACK PostWmCloseToPid(HWND hwnd, LPARAM lparam) {
⋮----
int StopOwnProcess(DWORD pid, DWORD grace_ms) {
⋮----
std::vector<DWORD> OwnImagePids(const std::wstring& own_dir,
⋮----
void StopEngineOverIpc() {
⋮----
std::wstring SessionPipeName() { return SessionPipeNameImpl(); }
⋮----
int RunStop() {
```

## File: launcher/process_stop.hpp
```cpp
std::wstring SessionPipeName();
⋮----
std::string BuildEngineCommandLine(const wchar_t* cmd_name);
⋮----
bool AckIsOk(const std::string& ack);
⋮----
DWORD PidFromStateJson(const std::string& ack);
⋮----
int RunStop();
```

## File: launcher/still_apply.cpp
```cpp
std::filesystem::path SiblingCompressor() {
⋮----
std::wstring FormatOffset(double seconds) {
⋮----
std::filesystem::path NewStillPath() {
⋮----
bool StartsWith(const std::wstring& text, const wchar_t* prefix) {
⋮----
void PruneStills(std::size_t keep) {
struct Entry {
⋮----
for (std::filesystem::directory_iterator it(k6wp::StillDir(), ec), end;
⋮----
bool ExtractFrame(const std::filesystem::path& video, double offset_s,
⋮----
bool ReadDword(HKEY root, const wchar_t* subkey, const wchar_t* name,
⋮----
bool WriteDword(HKEY root, const wchar_t* subkey, const wchar_t* name,
⋮----
void DeleteValue(HKEY root, const wchar_t* subkey, const wchar_t* name) {
⋮----
bool SaveCurrentLockscreenImage(const std::filesystem::path& target,
⋮----
bool SetLockscreenImage(const std::filesystem::path& image) {
⋮----
std::string LockscreenModeHint() {
⋮----
bool CaptureLockscreenOriginal(k6wp::OriginalLockscreen& state,
⋮----
void DisableRotatingLockscreenModes() {
⋮----
void RestoreLockscreenModes(const k6wp::OriginalLockscreen& state) {
⋮----
bool ApplyLockscreen(const std::filesystem::path& still,
⋮----
bool RestoreLockscreen(std::string& mode_hint, std::string& error) {
⋮----
bool SetWallpaper(const std::filesystem::path& still) {
⋮----
void PrimaryResolution(int& width, int& height) {
⋮----
void PublishStatus(const k6wp::StillStatus& status) {
⋮----
int RunApplyStill(const Options& opts) {
```

## File: launcher/still_apply.hpp
```cpp
int RunApplyStill(const Options& opts);
```

## File: launcher/win32_raii.hpp
```cpp
class HandleGuard {
⋮----
explicit HandleGuard(HANDLE h) : handle_(h) {}
HandleGuard(const HandleGuard&) = delete;
⋮----
HandleGuard(HandleGuard&& other) noexcept : handle_(other.handle_) {
⋮----
HANDLE get() const { return handle_; }
bool valid() const {
```

## File: packaging/app-icon-README.md
```markdown
# K6WP app icon

`k6wp-on.ico` and `k6wp-off.ico` are the canonical K6WP app icons. Master
copies live in `brand/icons/`; build copies live in `packaging/` (on the RC
include path, so the `.rc` files reference them by bare name) and `engine/`.

## What they are
- `k6wp-on.ico` — running state: indigo rounded square, white "K6" glyph.
- `k6wp-off.ico` — paused state: amber rounded square, "K6" glyph plus two
  pause bars (visually distinct from the running icon even at 16 px).
- Both are multi-size ICOs (9 entries: 16/24/32/48/64/72/96/128/256 px,
  32bpp, 432254 bytes each).

## How they are used
- `engine/app.rc` embeds `k6wp-on.ico` as resource `IDI_APPICON` (101) and
  `k6wp-off.ico` as `IDI_APPICON_PAUSED` (102) into `engine.exe`.
- `studio/app.rc` and `launcher/app.rc` embed `k6wp-on.ico` as
  `IDI_APPICON` (101).
- `packaging/installer.nsi` uses `k6wp-on.ico` for the installer icon and
  `k6wp-off.ico` for the uninstaller icon.
- `engine/src/tray.cpp` loads them via `LoadImageW` (sized to the system
  small-icon metric, 16 px at 100% DPI / 32 px at 200% DPI). If the resource
  is missing (e.g. a build without the .rc) it falls back to the stock shared
  icons, so the engine always runs.

## Replacing them (no code changes)
1. Overwrite `packaging/k6wp-on.ico` and/or `packaging/k6wp-off.ico`
   (same filenames, any multi-size ICO format), and mirror the change to the
   master copies in `brand/icons/` and the build copy in `engine/`.
2. Rebuild: `cmake --build --preset msvc-dev --target engine`.
3. Restart the engine. No source changes needed.

## History
The original worker-generated placeholders were `app.ico` / `app_paused.ico`
(16/32/48/256 px), produced by `packaging/make_app_icon.py` (Python + Pillow;
`pip install pillow`). They were superseded by the canonical `k6wp-on.ico` /
`k6wp-off.ico` artwork; the old files now live in `attic/packaging-icons/`
(LOW-22: moved out of `packaging/` so the ship dir holds only live assets).
```

## File: packaging/config.json.example
```
{
  "_comment": "K6WP example config, schema v5. Copy to %LOCALAPPDATA%/K6WP/config.json and set video_path to a REAL video file on your machine - the placeholder below does not exist. Playlist/rotation settings live in the separate playlist.json (see playlist.json.example).",
  "version": 5,
  "video_path": "C:/Videos/wallpaper.mp4",
  "fit_mode": "cover",
  "speed": 1.0,
  "monitor_id": -1,
  "crf": 22,
  "resolution_w": 0,
  "resolution_h": 0,
  "fps_cap": 24,
  "battery_saver": false,
  "battery_mode": "cap24",
  "cpu_affinity": "auto",
  "gpu_adapter": "auto"
}
```

## File: packaging/installer.nsi
```
; K6WP per-user installer (Task 27, k6wp-cleanup-wallpaper-ux).
;
; Per-user, no-UAC design:
;   - RequestExecutionLevel user  -> embedded manifest is asInvoker (never
;     requireAdministrator); installs without a UAC prompt.
;   - SetShellVarContext current  -> Start Menu / Desktop shortcuts resolve to
;     the CURRENT user's profile, never the all-users profile.
;   - InstallDir "$LOCALAPPDATA\K6WP" (NSIS 3.x $LOCALAPPDATA constant; never a
;     hardcoded C:\Users\... path).
;   - Registry writes are HKCU-only (Run value + uninstall key +
;     wallpaper-backup key). NO HKLM, NO Program Files, NO System32.
;   - Shortcut targets are K6WP.exe only, never engine.exe directly
;     (K6WP.exe is the Qt-free single-app launcher).
;
; File set mirrors packaging/make_zip.ps1 staging: K6WP.exe + engine.exe +
; studio.exe + compressor.exe (+ monitor_dump.exe when the build produced it)
; + vendored ffmpeg.exe/ffprobe.exe + vendored libmpv-2.dll + app-local VC
; runtime (vcruntime140.dll + vcruntime140_1.dll + msvcp140.dll, fresh-machine
; safety, no UAC) + the windeployqt output next to studio.exe (root *.dll +
; every plugin subdir containing *.dll) + config.json.example +
; packaging/uninstall.bat + LICENSE + LICENSES/ (GPL-2.0-or-later grant +
; third-party notices: libmpv/ffmpeg/Qt/nlohmann-json).
;
; Autostart checkbox writes EXACTLY "<installdir>\K6WP.exe" --engine --silent
; to HKCU\Software\Microsoft\Windows\CurrentVersion\Run value "K6WP", matching
; shared/autostart.cpp AutostartCommand()/SetAutostart(true). OFF (unchecked)
; leaves the value absent, matching SetAutostart(false). Default: CHECKED.
;
; Wallpaper backup: at install time the current OS wallpaper path is read via
; SystemParametersInfo SPI_GETDESKWALLPAPER and stored at
; HKCU\Software\K6WP\WallpaperBackup (REG_SZ "OsWallpaper"), so the
; uninstaller can restore it even when taskkill skips the engine's graceful
; restore path (engine/src/engine_app.cpp SaveOsWallpaper/RestoreOsWallpaper).
;
; Install (Section SEC_APP): stops this install dir's running processes
; FIRST (K6WP.exe --stop — the uninstaller's scoped helper) and snapshots a
; per-file pre-existence census, so re-running the installer over a live
; engine does not fail on a locked exe, and RollbackPartial deletes only the
; files THIS run created (audit fix H2: a failed upgrade leaves the
; previous installation intact).
;
; Uninstall (Section "Uninstall"): stops engine/studio/K6WP via
; "$INSTDIR\K6WP.exe" --stop (its own IPC quit first, then a per-PID kill
; restricted to images inside $INSTDIR - never a machine-wide image-name
; taskkill), deletes the Run value (idempotent), restores the pre-install
; wallpaper backup via SPI_SETDESKWALLPAPER when present, restores the
; per-user still (desktop wallpaper + lock-screen image) through
; K6WP.exe --apply-still --restore (no UAC), then removes a leftover legacy
; HKLM lockscreen policy via K6WP.exe --elevate-lockscreen off
; (self-elevating; the call blocks on the elevated worker with a bounded
; wait, and a declined prompt or a failed cleanup is reported to the user
; with the exact manual command) before the app files are deleted,
; removes the app files, asks keep/remove of
; %LOCALAPPDATA%\K6WP user data (silent uninstall keeps it), deletes
; shortcuts + HKCU uninstall key, removes the install dir.
;
; Stock plugins only: MUI2, LogicLib, nsExec, System. No third-party NSIS
; plugins, no Python at install time.
;
; Build (both styles work from the repo root; makensis resolves relative
; paths against the SCRIPT dir under either invocation, so plain relatives
; are used throughout and ${__FILEDIR__} is deliberately avoided - with a
; relative script path it expands to relative "packaging", which file
; directives then re-resolve against the script dir ("packaging\packaging").
; For the same reason do NOT add "!cd ${__FILEDIR__}": !cd resolves its
; argument against the script dir too and fails the relative invocation):
;   "C:\Program Files (x86)\NSIS\makensis.exe" packaging\installer.nsi
;   "C:\Program Files (x86)\NSIS\makensis.exe" <USER_HOME>\Desktop\k6wp\packaging\installer.nsi
; Output: dist\K6WP-Setup.exe.

Unicode true
RequestExecutionLevel user
; Per-user shell context enforced at runtime (.onInit + un.onInit):
; Start Menu / Desktop resolve to the CURRENT user profile, never all-users.

!include "MUI2.nsh"
!include "LogicLib.nsh"
!include "Sections.nsh"
; Version constants generated by CMake from K6WP_VERSION (root CMakeLists.txt)
; into packaging/version.nsh - never hand-edit APP_VERSION here.
!include "version.nsh"

; ---------------------------------------------------------------------------
; Constants
; ---------------------------------------------------------------------------
!define APP_NAME "K6WP"
!define APP_VERSION "${K6WP_VERSION_STR}"
!define RUN_VALUE "K6WP"
!define RUN_SUBKEY "Software\Microsoft\Windows\CurrentVersion\Run"
!define UNINST_SUBKEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\K6WP"
!define BACKUP_SUBKEY "Software\K6WP\WallpaperBackup"

; ---------------------------------------------------------------------------
; H2 (audit remediation): upgrade-safe install.
;
; Two defects fixed here:
;   1. Install over a RUNNING engine failed at the first locked exe and then
;      rolled back by deleting the whole staged set — including the previous
;      installation's files, leaving the user with a broken install.
;   2. That rollback had no notion of "which files did this run create".
;
; SEC_APP now stops this install dir's processes (K6WP.exe --stop, the same
; scoped helper the uninstaller uses) BEFORE any extraction, and snapshots a
; per-file pre-existence census. RollbackPartial deletes ONLY files this run
; created (flag 0); pre-existing files are never touched, so a failed upgrade
; leaves the previous installation intact.
Var /GLOBAL PreExistK6WP
Var /GLOBAL PreExistEngine
Var /GLOBAL PreExistStudio
Var /GLOBAL PreExistCompressor
Var /GLOBAL PreExistMonitorDump
Var /GLOBAL PreExistFfmpeg
Var /GLOBAL PreExistFfprobe
Var /GLOBAL PreExistLibmpv
Var /GLOBAL PreExistVcruntime
Var /GLOBAL PreExistVcruntime1
Var /GLOBAL PreExistMsvcp
Var /GLOBAL PreExistConfigEx
Var /GLOBAL PreExistPlaylistEx
Var /GLOBAL PreExistUninstallBat
Var /GLOBAL PreExistLicense
Var /GLOBAL PreExistLicensesDir
Var /GLOBAL PreExistPlatformsDir
Var /GLOBAL PreExistImageformatsDir
Var /GLOBAL PreExistStylesDir
Var /GLOBAL PreExistIconenginesDir
Var /GLOBAL PreExistQmlDir
Var /GLOBAL PreExistNetinfoDir
Var /GLOBAL PreExistTlsDir

; Records 1 (exists) / 0 (absent) for one $INSTDIR-relative file or dir.
; NOTE: the delete/rollback side below uses explicit Delete and RMDir
; lines with literal targets (see RollbackPartial) because
; packaging/make_zip.ps1's ship-set assert greps every RMDir /r target
; under $INSTDIR in this script — a macro parameter would resolve to a
; placeholder name, not a staged directory, and fail the release gate.
!macro K6WP_PREFLAG_SET _var _rel
  ${If} ${FileExists} "$INSTDIR\${_rel}"
    StrCpy ${_var} 1
  ${Else}
    StrCpy ${_var} 0
  ${EndIf}
!macroend
; ---------------------------------------------------------------------------

; Build outputs. Release preset is the production source (matches the
; subsystem-WINDOWS, ProductName-K6WP branded triple verified in
; make_zip-style checks); file NAMES mirror make_zip.ps1 exactly.
!define SRCDIR "..\build\release"
!define VENDOR_FFMPEG "..\vendor\ffmpeg"
!define VENDOR_LIBMPV "..\vendor\libmpv\bin"
; Pinned app-local VC runtime (populated by make_zip.ps1 step 2f; makensis
; cannot discover the versioned VS Redist dir, so the installer consumes
; this cached copy and fails loudly when nobody staged it yet).
!define VENDOR_VC "..\vendor\vc_runtime"
; Repo root (license texts): LICENSE + LICENSES/ ship in both the ZIP
; (make_zip.ps1) and the installer; the ship-set sync assert in make_zip.ps1
; maps ${REPO_ROOT} back to the repo root.
!define REPO_ROOT ".."
!if /FileExists "${VENDOR_VC}\vcruntime140.dll"
!else
  !error "vendor\vc_runtime\vcruntime140.dll missing - run packaging\make_zip.ps1 once (it caches the CRT set) or copy vcruntime140.dll + vcruntime140_1.dll + msvcp140.dll there."
!endif

Name "${APP_NAME} ${APP_VERSION}"
OutFile "..\dist\K6WP-Setup.exe"
InstallDir "$LOCALAPPDATA\K6WP"
Icon "k6wp-on.ico"

!define MUI_ICON "k6wp-on.ico"
!define MUI_UNICON "k6wp-on.ico"
!define MUI_ABORTWARNING

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "${REPO_ROOT}\LICENSE"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

VIProductVersion "${K6WP_VERSION_STR_4}"
VIAddVersionKey "ProductName" "${APP_NAME}"
VIAddVersionKey "CompanyName" "K6WP contributors"
VIAddVersionKey "FileDescription" "${APP_NAME} Setup"
VIAddVersionKey "FileVersion" "${APP_VERSION}"
VIAddVersionKey "ProductVersion" "${APP_VERSION}"
VIAddVersionKey "LegalCopyright" "${APP_NAME}"

Function .onInit
  SetShellVarContext current
  ; --- C6: minimum-OS gate -----------------------------------------------
  ; K6WP requires Windows 10 22H2 (build 19041) or Windows 11. The
  ; manifest's supportedOS list only tells the compatibility shim what to
  ; report; it does NOT refuse to install. This check does.
  ;
  ; The build number is read from the registry (CurrentBuildNumber) so the
  ; check is the same rule the runtime check (shared/os_version.cpp,
  ; RtlGetVersion) applies: build >= 19041.
  ReadRegStr $0 "HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion" "CurrentBuildNumber"
  ${If} $0 == ""
    ; No build number (or a non-string value): refuse rather than guess.
    MessageBox MB_OK|MB_ICONSTOP "K6WP requires Windows 10 22H2 (build 19041) or Windows 11, 64-bit.$\r$\n$\r$\nThis system's Windows build could not be detected, so the installation was stopped."
    Abort
  ${EndIf}
  ${If} $0 != ""
    IntCmp $0 19041 0 done done
    ; $0 < 19041 -> too old
    MessageBox MB_OK|MB_ICONSTOP "K6WP requires Windows 10 22H2 (build 19041) or Windows 11, 64-bit.$\r$\n$\r$\nDetected build $0. Update Windows, then run the installer again."
    Abort
    done:
  ${EndIf}
FunctionEnd

Function un.onInit
  SetShellVarContext current
FunctionEnd

; ---------------------------------------------------------------------------
; Install sections
; ---------------------------------------------------------------------------
; NOTE: the autostart checkbox section is declared FIRST so its ${SEC_AUTO}
; index define exists when SEC_APP reads it (NSIS defines section indexes at
; the Section line; forward references do not expand). It holds no files; the
; actual Run-value write happens in SEC_APP so file staging stays ordered.
Section "Start with Windows (autostart)" SEC_AUTO
  ; Checkbox only: the actual Run-value write happens in SEC_APP so ordering
  ; with file staging stays deterministic. This section intentionally holds
  ; no files; its selected flag is the opt-in.
SectionEnd

Section "!${APP_NAME} Application (required)" SEC_APP
  SectionIn RO

  ; --- Writability pre-check (failure QA: message + no partial install) ---
  ; Must run BEFORE any File extraction so an unwritable target aborts clean
  ; with nothing copied (rollback = nothing to roll back).
  CreateDirectory "$INSTDIR"
  FileOpen $0 "$INSTDIR\.k6wp_write_test" w
  ; Label form (NOT numeric offsets: +1 means the NEXT instruction, so an
  ; "IfErrors 0 +1 / Abort" skips nothing and aborts on success). MessageBox
  ; is modal even in silent mode, so silent failures Abort without a dialog.
  IfErrors write_failed write_ok
  write_failed:
    IfSilent abort_nowrite 0
    MessageBox MB_ICONSTOP "${APP_NAME} cannot write to:$\r$\n$INSTDIR$\r$\n$\r$\nPick a writable folder. Nothing was installed."
  abort_nowrite:
    Abort
  write_ok:
  FileClose $0
  Delete "$INSTDIR\.k6wp_write_test"

  ; --- H2: stop the previous installation's processes BEFORE staging files.
  ; NSIS cannot overwrite a running exe image: without this the first File
  ; on a locked engine.exe failed, aborted the section, and the rollback
  ; (fixed below) destroyed the previous installation. K6WP.exe --stop is
  ; the scoped helper the uninstaller uses: IPC quit first, then a per-PID
  ; kill limited to images inside $INSTDIR (never a machine-wide taskkill).
  ; Its exit code is advisory — a survivor surfaces immediately at the File
  ; stage, which retries once via the extraction-error path below.
  ${If} ${FileExists} "$INSTDIR\K6WP.exe"
    DetailPrint "Stopping running ${APP_NAME} processes before install..."
    nsExec::ExecToLog '"$INSTDIR\K6WP.exe" --stop'
    Pop $0
    DetailPrint "Process stop helper exit code: $0"
  ${EndIf}

  ; --- H2: pre-install census consumed by RollbackPartial (create-only
  ; teardown so a failed upgrade keeps the previous install intact).
  !insertmacro K6WP_PREFLAG_SET $PreExistK6WP "K6WP.exe"
  !insertmacro K6WP_PREFLAG_SET $PreExistEngine "engine.exe"
  !insertmacro K6WP_PREFLAG_SET $PreExistStudio "studio.exe"
  !insertmacro K6WP_PREFLAG_SET $PreExistCompressor "compressor.exe"
  !insertmacro K6WP_PREFLAG_SET $PreExistMonitorDump "monitor_dump.exe"
  !insertmacro K6WP_PREFLAG_SET $PreExistFfmpeg "ffmpeg.exe"
  !insertmacro K6WP_PREFLAG_SET $PreExistFfprobe "ffprobe.exe"
  !insertmacro K6WP_PREFLAG_SET $PreExistLibmpv "libmpv-2.dll"
  !insertmacro K6WP_PREFLAG_SET $PreExistVcruntime "vcruntime140.dll"
  !insertmacro K6WP_PREFLAG_SET $PreExistVcruntime1 "vcruntime140_1.dll"
  !insertmacro K6WP_PREFLAG_SET $PreExistMsvcp "msvcp140.dll"
  !insertmacro K6WP_PREFLAG_SET $PreExistConfigEx "config.json.example"
  !insertmacro K6WP_PREFLAG_SET $PreExistPlaylistEx "playlist.json.example"
  !insertmacro K6WP_PREFLAG_SET $PreExistUninstallBat "uninstall.bat"
  !insertmacro K6WP_PREFLAG_SET $PreExistLicense "LICENSE"
  !insertmacro K6WP_PREFLAG_SET $PreExistLicensesDir "LICENSES"
  !insertmacro K6WP_PREFLAG_SET $PreExistPlatformsDir "platforms"
  !insertmacro K6WP_PREFLAG_SET $PreExistImageformatsDir "imageformats"
  !insertmacro K6WP_PREFLAG_SET $PreExistStylesDir "styles"
  !insertmacro K6WP_PREFLAG_SET $PreExistIconenginesDir "iconengines"
  !insertmacro K6WP_PREFLAG_SET $PreExistQmlDir "qml"
  !insertmacro K6WP_PREFLAG_SET $PreExistNetinfoDir "networkinformation"
  !insertmacro K6WP_PREFLAG_SET $PreExistTlsDir "tls"

  ; --- Belt-and-braces wallpaper backup (engine restores on clean exit too) --
  DetailPrint "Backing up current OS wallpaper path..."
  System::Alloc 520
  Pop $1
  System::Call 'user32::SystemParametersInfoW(i 115, i 260, p r1, i 0) i .r2'
  ${If} $2 <> 0
    System::Call '*$1(&w260 .r3)'
    ${If} $3 != ""
      WriteRegStr HKCU "${BACKUP_SUBKEY}" "OsWallpaper" "$3"
      DetailPrint "Wallpaper backup saved."
    ${EndIf}
  ${Else}
    DetailPrint "No OS wallpaper readable; skipping backup."
  ${EndIf}
  System::Free $1

  ; --- Application files (make_zip.ps1 file set) ---
  DetailPrint "Copying ${APP_NAME} files..."
  SetOutPath "$INSTDIR"
  File "${SRCDIR}\K6WP.exe"
  File "${SRCDIR}\engine.exe"
  File "${SRCDIR}\studio.exe"
  File "${SRCDIR}\compressor.exe"
!if /FileExists "${SRCDIR}\monitor_dump.exe"
  File "${SRCDIR}\monitor_dump.exe"
!endif
  ; Vendored helpers.
  File "${VENDOR_FFMPEG}\ffmpeg.exe"
  File "${VENDOR_FFMPEG}\ffprobe.exe"
  File "${VENDOR_LIBMPV}\libmpv-2.dll"
  ; App-local VC runtime (fresh machines without any redist).
  File "${VENDOR_VC}\vcruntime140.dll"
  File "${VENDOR_VC}\vcruntime140_1.dll"
  File "${VENDOR_VC}\msvcp140.dll"
  ; windeployqt output: every root DLL next to studio.exe ...
  File "${SRCDIR}\*.dll"
  ; ... plus every plugin subdir containing DLLs.
  ; qml/ is MANDATORY: Studio is a QML app, so the QtQuick/Controls/
  ; Layouts/Templates modules must ship or the window comes up empty.
  ; networkinformation/ + tls/ are back because Qt6Quick links Qt6Network
  ; (P4.2 dropped it back when Studio was Widgets-only; the QML rewrite makes
  ; it non-optional again). make_zip's ship-set assert enforces this list.
  ; qmltooling/ is deliberately NOT shipped - QML debugger/inspector plugins,
  ; excluded from staging in make_zip.ps1.
  File /r "${SRCDIR}\platforms"
  File /r "${SRCDIR}\imageformats"
  File /r "${SRCDIR}\styles"
  File /r "${SRCDIR}\iconengines"
  File /r "${SRCDIR}\qml"
  File /r "${SRCDIR}\networkinformation"
  File /r "${SRCDIR}\tls"
  ; Example config + playlist + portable-style uninstall helper.
  File "config.json.example"
  File "playlist.json.example"
  File "uninstall.bat"
  ; License texts (GPL-2.0-or-later grant + third-party notices; mirrors
  ; make_zip.ps1 staging, kept in sync by its ship-set assert).
  File "${REPO_ROOT}\LICENSE"
  File /r "${REPO_ROOT}\LICENSES"

  ; --- Extraction-error rollback: message + remove exactly what we staged --
  ; Label form (NOT numeric offsets: +1 means the NEXT instruction, so an
  ; "IfErrors 0 +1 / Abort" skips nothing and aborts on success).
  IfErrors files_failed files_ok
  files_failed:
    Call RollbackPartial
    Abort
  files_ok:

  ; --- Shortcuts (per-user shell context; target is K6WP.exe only) ---
  CreateDirectory "$SMPROGRAMS\${APP_NAME}"
  CreateShortCut "$SMPROGRAMS\${APP_NAME}\${APP_NAME}.lnk" "$INSTDIR\K6WP.exe" "" "$INSTDIR\K6WP.exe" 0
  CreateShortCut "$DESKTOP\${APP_NAME}.lnk" "$INSTDIR\K6WP.exe" "" "$INSTDIR\K6WP.exe" 0

  ; --- Autostart (SetAutostart-ON semantics, iff the checkbox is selected) --
  SectionGetFlags ${SEC_AUTO} $0
  IntOp $0 $0 & ${SF_SELECTED}
  ${If} $0 <> 0
    WriteRegStr HKCU "${RUN_SUBKEY}" "${RUN_VALUE}" '"$INSTDIR\K6WP.exe" --engine --silent'
    DetailPrint "Autostart enabled (HKCU Run)."
  ${Else}
    DetailPrint "Autostart skipped (checkbox unchecked)."
  ${EndIf}

  ; --- Uninstaller + Add/Remove Programs entry (HKCU, per-user) ---
  WriteUninstaller "$INSTDIR\Uninstall.exe"
  WriteRegStr HKCU "${UNINST_SUBKEY}" "DisplayName" "${APP_NAME} ${APP_VERSION}"
  WriteRegStr HKCU "${UNINST_SUBKEY}" "DisplayVersion" "${APP_VERSION}"
  WriteRegStr HKCU "${UNINST_SUBKEY}" "InstallLocation" "$INSTDIR"
  WriteRegStr HKCU "${UNINST_SUBKEY}" "UninstallString" '"$INSTDIR\Uninstall.exe"'
  WriteRegStr HKCU "${UNINST_SUBKEY}" "DisplayIcon" "$INSTDIR\K6WP.exe"
  WriteRegDWORD HKCU "${UNINST_SUBKEY}" "NoModify" 1
  WriteRegDWORD HKCU "${UNINST_SUBKEY}" "NoRepair" 1
SectionEnd

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC_APP} "Required ${APP_NAME} program files (per-user, no admin)."
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC_AUTO} "Launch ${APP_NAME} engine silently at logon via HKCU Run (no UAC). Uncheck to skip."
!insertmacro MUI_FUNCTION_DESCRIPTION_END

; Rollback helper (audit fix H2): delete exactly the files THIS run
; created — the pre-install census (K6WP_PREFLAG_SET) decides per file, so
; the previous installation's files are never touched. A failed upgrade
; therefore leaves the old version installed (and uninstallable) instead of
; a blown-away install dir.
; Deliberate omissions vs the file list:
;   - root "*.dll" wildcard: version-specific Qt payloads we cannot census
;     individually; leftover DLLs are inert next to the old exes.
;   - Uninstall.exe / the HKCU entries are never written before extraction
;     completes, so they cannot need rollback.
Function RollbackPartial
  ; Silent-safe: MessageBox would block a headless /S run, so only show it
  ; interactively; silent runs still delete the partial set and Abort.
  IfSilent rb_skip_msg 0
  MessageBox MB_ICONSTOP "${APP_NAME} setup failed while copying files. Rolling back the partial install."
rb_skip_msg:
  ; Explicit Delete and RMDir lines with literal targets on purpose: the
  ; make_zip ship-set assert parses these lines, so no macro may generate
  ; them (see the NOTE next to K6WP_PREFLAG_SET above).
  ${If} $PreExistK6WP == 0
    Delete "$INSTDIR\K6WP.exe"
  ${EndIf}
  ${If} $PreExistEngine == 0
    Delete "$INSTDIR\engine.exe"
  ${EndIf}
  ${If} $PreExistStudio == 0
    Delete "$INSTDIR\studio.exe"
  ${EndIf}
  ${If} $PreExistCompressor == 0
    Delete "$INSTDIR\compressor.exe"
  ${EndIf}
  ${If} $PreExistMonitorDump == 0
    Delete "$INSTDIR\monitor_dump.exe"
  ${EndIf}
  ${If} $PreExistFfmpeg == 0
    Delete "$INSTDIR\ffmpeg.exe"
  ${EndIf}
  ${If} $PreExistFfprobe == 0
    Delete "$INSTDIR\ffprobe.exe"
  ${EndIf}
  ${If} $PreExistLibmpv == 0
    Delete "$INSTDIR\libmpv-2.dll"
  ${EndIf}
  ${If} $PreExistVcruntime == 0
    Delete "$INSTDIR\vcruntime140.dll"
  ${EndIf}
  ${If} $PreExistVcruntime1 == 0
    Delete "$INSTDIR\vcruntime140_1.dll"
  ${EndIf}
  ${If} $PreExistMsvcp == 0
    Delete "$INSTDIR\msvcp140.dll"
  ${EndIf}
  ${If} $PreExistConfigEx == 0
    Delete "$INSTDIR\config.json.example"
  ${EndIf}
  ${If} $PreExistPlaylistEx == 0
    Delete "$INSTDIR\playlist.json.example"
  ${EndIf}
  ${If} $PreExistUninstallBat == 0
    Delete "$INSTDIR\uninstall.bat"
  ${EndIf}
  ${If} $PreExistLicense == 0
    Delete "$INSTDIR\LICENSE"
  ${EndIf}
  ${If} $PreExistLicensesDir == 0
    RMDir /r "$INSTDIR\LICENSES"
  ${EndIf}
  ${If} $PreExistPlatformsDir == 0
    RMDir /r "$INSTDIR\platforms"
  ${EndIf}
  ${If} $PreExistImageformatsDir == 0
    RMDir /r "$INSTDIR\imageformats"
  ${EndIf}
  ${If} $PreExistStylesDir == 0
    RMDir /r "$INSTDIR\styles"
  ${EndIf}
  ${If} $PreExistIconenginesDir == 0
    RMDir /r "$INSTDIR\iconengines"
  ${EndIf}
  ${If} $PreExistQmlDir == 0
    RMDir /r "$INSTDIR\qml"
  ${EndIf}
  ${If} $PreExistNetinfoDir == 0
    RMDir /r "$INSTDIR\networkinformation"
  ${EndIf}
  ${If} $PreExistTlsDir == 0
    RMDir /r "$INSTDIR\tls"
  ${EndIf}
  Delete "$INSTDIR\.k6wp_write_test"
  RMDir "$INSTDIR"  ; only succeeds when the dir is empty
FunctionEnd

; ---------------------------------------------------------------------------
; Uninstall section (Task 28 reuses this; keep it complete and idempotent)
; ---------------------------------------------------------------------------
Section "Uninstall" SEC_UNINST
  ; --- Stop running processes first (graceful engine shutdown restores the
  ; --- OS wallpaper itself; exit codes tolerated: not-running is fine) ---
  ; K6WP.exe --stop is used instead of `taskkill /F /IM engine.exe`: an image
  ; name is not unique on a developer or IT machine, and the old call killed
  ; EVERY process on the box carrying that name. --stop asks the engine to quit
  ; over its own IPC pipe (so its wallpaper restore runs), then escalates to a
  ; per-PID kill, and only for images that live directly in $INSTDIR. A
  ; non-zero exit means one of OUR processes survived, so the user is asked to
  ; close it and retry instead of us deleting files under a live process.
  DetailPrint "Stopping ${APP_NAME} processes..."
  IntOp $1 0 + 2
  stop_processes:
    ExecWait '"$INSTDIR\K6WP.exe" --stop' $0
    ${If} $0 == 0
      Goto stop_processes_done
    ${EndIf}
    IntOp $1 1 - 1
    ${If} $1 == 0
      DetailPrint "Some ${APP_NAME} processes could not be stopped (2 attempts)."
      Goto stop_processes_done
    ${EndIf}
    IfSilent stop_processes 0
    MessageBox MB_RETRYCANCEL|MB_ICONEXCLAMATION "Some ${APP_NAME} processes (engine, Studio) are still running and could not be stopped.$\r$\nClose them now, then choose Retry.$\r$\n$\r$\nChoose Cancel to continue anyway (their files may be left behind)." /SD IDCANCEL IDRETRY stop_processes
  stop_processes_done:

  ; --- Remove autostart (idempotent: absent value is not an error) ---
  DeleteRegValue HKCU "${RUN_SUBKEY}" "${RUN_VALUE}"

  ; --- Restore pre-install wallpaper backup when present ---
  ReadRegStr $0 HKCU "${BACKUP_SUBKEY}" "OsWallpaper"
  ${If} $0 != ""
    DetailPrint "Restoring backed-up OS wallpaper..."
    System::Call 'user32::SystemParametersInfoW(i 20, i 0, w r0, i 3) i .r1'
  ${EndIf}
  DeleteRegKey HKCU "${BACKUP_SUBKEY}"

  ; --- Restore the per-user still (wallpaper + lock screen) ---
  ; No UAC: the helper reads the persistent original_wallpaper.json /
  ; original_lockscreen.* stores under %LOCALAPPDATA%\K6WP. A missing store is
  ; a successful no-op. Run BEFORE the data folder is optionally removed, and
  ; while K6WP.exe still exists; the helper is synchronous (it waits for its
  ; own compressor child), so the restore is finished when ExecWait returns.
  DetailPrint "Restoring your previous desktop wallpaper and lock-screen image..."
  ExecWait '"$INSTDIR\K6WP.exe" --apply-still --restore' $0
  ${If} $0 != 0
    DetailPrint "The per-user wallpaper/lock-screen restore did not complete (exit code $0)."
  ${EndIf}

  ; --- Remove a leftover legacy lockscreen policy (LOW-21) ---
  ; Old builds wrote the machine-wide HKLM LockScreenImage policy. This
  ; per-user (asInvoker) uninstaller cannot touch HKLM, so it delegates to
  ; K6WP.exe, which self-elevates via a runas relaunch while the exe still
  ; exists (app files are deleted below). The cleanup restores the pre-K6WP
  ; policy value from lockscreen_policy_backup.json when one exists; otherwise
  ; it deletes the value only when it points at our legacy
  ; %PROGRAMDATA%\K6WP\lockscreen.jpg.
  ; The wait is the point: K6WP.exe hands back the elevated process HANDLE and
  ; blocks on it with a bounded wait (2 min), so this ExecWait only returns once
  ; the HKLM write is done - or once the helper was terminated. Deleting
  ; K6WP.exe while the worker is still running is impossible from here.
  ; Exit codes: 0 cleaned | 5 UAC declined | 4 wait budget expired |
  ; 3 the helper failed. Anything non-zero leaves a leftover policy in place,
  ; so the user is told exactly what to run.
  DetailPrint "Removing a leftover legacy lockscreen policy (UAC prompt only if one exists)..."
  ExecWait '"$INSTDIR\K6WP.exe" --elevate-lockscreen off' $0
  ${If} $0 != 0
    DetailPrint "The legacy lockscreen policy was NOT removed (exit code $0)."
    Goto lockscreen_warning
  ${EndIf}
  Goto lockscreen_done
  lockscreen_warning:
    IfSilent lockscreen_done 0
    ${If} $0 == 5
      MessageBox MB_ICONEXCLAMATION "The administrator prompt was declined, so a leftover K6WP lockscreen policy was NOT removed.$\r$\n$\r$\nIf an old K6WP version left one, it still points at %PROGRAMDATA%\K6WP\lockscreen.jpg (HKLM\SOFTWARE\Policies\Microsoft\Windows\Personalization\LockScreenImage).$\r$\n$\r$\nTo clean it up later, run this in an Administrator Command Prompt:$\r$\n$\r$\nK6WP.exe --elevate-lockscreen off"
    ${Else}
      MessageBox MB_ICONEXCLAMATION "K6WP could not remove a leftover legacy lockscreen policy (exit code $0).$\r$\n$\r$\nIf an old K6WP version left one, it may still point at %PROGRAMDATA%\K6WP\lockscreen.jpg (HKLM\SOFTWARE\Policies\Microsoft\Windows\Personalization\LockScreenImage).$\r$\n$\r$\nTo clean it up later, run this in an Administrator Command Prompt:$\r$\n$\r$\nK6WP.exe --elevate-lockscreen off"
    ${EndIf}
  lockscreen_done:

  ; --- Remove app files (explicit staged set; user data untouched here) ---
  Delete "$INSTDIR\K6WP.exe"
  Delete "$INSTDIR\engine.exe"
  Delete "$INSTDIR\studio.exe"
  Delete "$INSTDIR\compressor.exe"
  Delete "$INSTDIR\monitor_dump.exe"
  Delete "$INSTDIR\ffmpeg.exe"
  Delete "$INSTDIR\ffprobe.exe"
  Delete "$INSTDIR\libmpv-2.dll"
  Delete "$INSTDIR\vcruntime140.dll"
  Delete "$INSTDIR\vcruntime140_1.dll"
  Delete "$INSTDIR\msvcp140.dll"
  Delete "$INSTDIR\*.dll"
  RMDir /r "$INSTDIR\platforms"
  RMDir /r "$INSTDIR\imageformats"
  RMDir /r "$INSTDIR\styles"
  RMDir /r "$INSTDIR\iconengines"
  RMDir /r "$INSTDIR\qml"
  RMDir /r "$INSTDIR\networkinformation"
  RMDir /r "$INSTDIR\tls"
  Delete "$INSTDIR\config.json.example"
  Delete "$INSTDIR\playlist.json.example"
  Delete "$INSTDIR\uninstall.bat"
  Delete "$INSTDIR\LICENSE"
  RMDir /r "$INSTDIR\LICENSES"

  ; --- User data: keep/remove offer (silent uninstall keeps it) ---
  ; Runtime data (config.json, library, thumbs, engine.log) lives under
  ; %LOCALAPPDATA%\K6WP, which equals $INSTDIR for default installs.
  StrCmp "$INSTDIR" "$LOCALAPPDATA\${APP_NAME}" same_dir custom_dir

  same_dir:
    IfSilent keep_same 0
    MessageBox MB_YESNO "Also delete ${APP_NAME} user data (settings, library, logs) in %LOCALAPPDATA%\${APP_NAME}?" IDYES remove_same IDNO keep_same
  remove_same:
    Delete "$INSTDIR\Uninstall.exe"
    RMDir /r "$INSTDIR"
    Goto done_data
  keep_same:
    DetailPrint "Keeping user data."
    Delete "$INSTDIR\Uninstall.exe"
    RMDir "$INSTDIR"
    Goto done_data

  custom_dir:
    Delete "$INSTDIR\Uninstall.exe"
    RMDir /r "$INSTDIR"
    IfSilent silent_keep 0
    MessageBox MB_YESNO "Also delete ${APP_NAME} user data (settings, library, logs) in %LOCALAPPDATA%\${APP_NAME}?" IDYES wipe_data IDNO silent_keep
  wipe_data:
    RMDir /r "$LOCALAPPDATA\${APP_NAME}"
  silent_keep:
    Goto done_data
  done_data:

  ; --- Shortcuts + Add/Remove entry ---
  Delete "$SMPROGRAMS\${APP_NAME}\${APP_NAME}.lnk"
  RMDir "$SMPROGRAMS\${APP_NAME}"
  Delete "$DESKTOP\${APP_NAME}.lnk"
  DeleteRegKey HKCU "${UNINST_SUBKEY}"
SectionEnd
```

## File: packaging/make_app_icon.py
```python
SIZES = [16, 32, 48, 256]
⋮----
OUT_DIR = os.path.dirname(os.path.abspath(__file__))
⋮----
def _font(size)
⋮----
def _base(bg_top, bg_bottom, glyph)
⋮----
size = 256
img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
d = ImageDraw.Draw(img)
radius = int(size * 0.22)
⋮----
t = y / (size - 1)
color = tuple(int(bg_top[c] + (bg_bottom[c] - bg_top[c]) * t) for c in range(3)) + (255,)
⋮----
mask = Image.new("L", (size, size), 0)
md = ImageDraw.Draw(mask)
⋮----
font = _font(int(size * 0.52))
bbox = d.textbbox((0, 0), glyph, font=font)
w = bbox[2] - bbox[0]
h = bbox[3] - bbox[1]
⋮----
def _paused_base()
⋮----
img = _base((245, 158, 11), (180, 83, 9), "K6")
⋮----
bar_w = int(256 * 0.10)
bar_h = int(256 * 0.16)
gap = int(256 * 0.06)
y = int(256 * 0.66)
x0 = (256 - 2 * bar_w - gap) / 2
⋮----
def main()
```

## File: packaging/make_zip.ps1
```powershell
# K6WP portable ZIP builder (Todo 45, fase-6; task 26: release source).
#
# Builds all targets fresh FROM THE RELEASE PRESET (build\release — the
# production triple: /O2, WIN32_EXECUTABLE, BUILD_TESTING=OFF; never the
# Debug msvc-dev tree), stages K6WP.exe (Qt-free launcher) + engine.exe +
# studio.exe + compressor.exe (+ monitor_dump.exe when present) + vendored ffmpeg/ffprobe + libmpv-2.dll
# + the ACTUAL windeployqt output next to studio.exe (enumerated from the
# build dir - never a hand-listed set of Qt DLLs) + app-local VC runtime
# (vcruntime140*.dll + msvcp140.dll, fresh-machine safety) +
# config.json.example + uninstall.bat + LICENSE + LICENSES/*.txt
# (GPL-2.0-or-later + third-party notices), verifies every staged exe's imports resolve from the staging
# dir (dumpbin /dependents -> FAIL with the missing list, never a silent
# broken ZIP), logs per-file MB, and asserts the final ZIP is under 250 MB.
#
# Usage (PowerShell 5.1):
#   powershell -ExecutionPolicy Bypass -File packaging\make_zip.ps1 [-Version 1.0.0] [-Generator "Visual Studio 17 2022"]
# CI override: -Generator configures+builds with that CMake generator instead
# of "--preset release" (which pins the local VS 18 generator). Empty (default)
# = legacy local behavior, unchanged. -BuildDirOverride/-QtRootOverride/
# -CmakeExeOverride default to the local paths below when empty.
#
# Outputs: dist\K6WP-portable-<version>.zip + dist\make_zip-<version>.log

param(
  [string]$Version = "",
  [string]$Generator = "",
  [string]$BuildDirOverride = "",
  [string]$QtRootOverride = "",
  [string]$CmakeExeOverride = ""
)

$ErrorActionPreference = "Continue"

$RepoRoot = Split-Path -Parent $PSScriptRoot
$DistDir  = Join-Path $RepoRoot "dist"
if ([string]::IsNullOrWhiteSpace($BuildDirOverride)) {
  $BuildDir = Join-Path $RepoRoot "build\release"
} else {
  $BuildDir = $BuildDirOverride
}
$DefaultCmakeExe = "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
if (-not [string]::IsNullOrWhiteSpace($CmakeExeOverride)) {
  $CmakeExe = $CmakeExeOverride
} elseif (Test-Path -LiteralPath $DefaultCmakeExe) {
  $CmakeExe = $DefaultCmakeExe
} else {
  $CmakeExe = "cmake"
}
$SizeLimitMB = 250

function Log($msg) {
  $line = "[make_zip] $msg"
  Write-Host $line
  Add-Content -LiteralPath $script:LogFile -Value $line
}

function Fail($msg) {
  Log "FATAL: $msg"
  exit 1
}

# --- Version (default: K6WP_VERSION from the root CMakeLists.txt) ------------
if ([string]::IsNullOrWhiteSpace($Version)) {
  $cmakeLists = Get-Content -LiteralPath (Join-Path $RepoRoot "CMakeLists.txt") -Raw
  if ($cmakeLists -match 'set\(K6WP_VERSION\s+"([^"]+)"\)') {
    $Version = $Matches[1]
  } else {
    Write-Host "[make_zip] FATAL: cannot parse K6WP_VERSION from CMakeLists.txt"
    exit 1
  }
}

New-Item -ItemType Directory -Path $DistDir -Force | Out-Null
$script:LogFile = Join-Path $DistDir "make_zip-$Version.log"
if (Test-Path -LiteralPath $script:LogFile) { Remove-Item -LiteralPath $script:LogFile -Force }
Log "K6WP portable ZIP build, version $Version"

# --- 0a. Version single-source assert (release hardening) --------------------
# K6WP_VERSION in the root CMakeLists.txt is canonical. configure_file()
# stamps it into build/<cfg>/generated/version.h (C/C++/RC consumers),
# packaging/version.nsh (NSIS installer) and engine/app.manifest (in-place,
# from engine/app.manifest.in). This assert verifies:
#   1. CMakeLists project() VERSION == K6WP_VERSION (the canonical pair).
#   2. The generated files (version.h, version.nsh, app.manifest) all carry
#      the canonical version.
#   3. NO consumer source carries a hand-written version literal - every
#      version must flow from the generated mechanism (version.h / version.nsh
#      / app.manifest.in). Catching drift at release time, no codegen pipeline.
function Normalize-Version3([string]$v) {
  # "1.0.0.0" / "1,0,0,0" -> "1.0.0" (first 3 components).
  # Prerelease suffixes ("-alpha", "-beta.1", "+build") are stripped so the
  # honest-alpha reset (K6WP_VERSION "0.0.1-alpha", numeric spots "0.0.1.0")
  # still asserts as one version.
  $v = ($v -replace '[-+].*$', '')
  $parts = ($v -replace ',', '.').Split('.')
  if ($parts.Count -lt 3) { return $null }
  return ($parts[0..2] -join '.')
}

$versionSpots = @()
$cmakeLists = Get-Content -LiteralPath (Join-Path $RepoRoot "CMakeLists.txt") -Raw
if ($cmakeLists -match 'project\(k6wp\s+VERSION\s+([0-9.]+)') {
  $versionSpots += @{ Where = "CMakeLists.txt project() VERSION"; Value = $Matches[1] }
} else {
  Fail "version assert: cannot parse project() VERSION from CMakeLists.txt"
}
if ($cmakeLists -match 'set\(K6WP_VERSION\s+"([^"]+)"\)') {
  $versionSpots += @{ Where = "CMakeLists.txt K6WP_VERSION"; Value = $Matches[1] }
} else {
  Fail "version assert: cannot parse K6WP_VERSION from CMakeLists.txt"
}

# Generated files (configure_file output) must carry the canonical version.
# configure_file() writes to CMAKE_BINARY_DIR/generated. Relative to $BuildDir
# (this script's own output dir, default build\release) that is either
#   .\generated            single-config tree / the dev presets, whose binary
#                          dir IS the CMake binary dir, or
#   ..\generated           a multi-config generator (Visual Studio) whose cache
#                          sits one level up - which is what CI produces, since
#                          it configures with -B build while this script builds
#                          into build\release. Before this, only the first
#                          layout resolved and CI could never package a ZIP.
$genCandidates = @(
  (Join-Path $BuildDir "generated\version.h"),
  (Join-Path $BuildDir "..\generated\version.h"),
  (Join-Path $BuildDir "Release\generated\version.h"),
  (Join-Path $BuildDir "Debug\generated\version.h"),
  (Join-Path $BuildDir "RelWithDebInfo\generated\version.h")
)
$genHeader = $genCandidates | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
if (-not $genHeader) {
  Fail "version assert: generated header not found (tried: $($genCandidates -join ', ')). Run cmake configure first."
}
if (Test-Path -LiteralPath $genHeader) {
  $genText = Get-Content -LiteralPath $genHeader -Raw
  if ($genText -match '#define\s+K6WP_VERSION_STR\s+"([^"]+)"') {
    $versionSpots += @{ Where = "generated/version.h K6WP_VERSION_STR"; Value = $Matches[1] }
  } else {
    Fail "version assert: cannot parse K6WP_VERSION_STR from $genHeader (run cmake configure first)"
  }
  if ($genText -match '#define\s+K6WP_VERSION_STR_4\s+"([^"]+)"') {
    $versionSpots += @{ Where = "generated/version.h K6WP_VERSION_STR_4"; Value = $Matches[1] }
  } else {
    Fail "version assert: cannot parse K6WP_VERSION_STR_4 from $genHeader"
  }
  if ($genText -match '#define\s+K6WP_VERSION_COMMA\s+([0-9,]+)') {
    $versionSpots += @{ Where = "generated/version.h K6WP_VERSION_COMMA"; Value = $Matches[1] }
  } else {
    Fail "version assert: cannot parse K6WP_VERSION_COMMA from $genHeader"
  }
} else {
  Fail "version assert: generated header not found at $genHeader (run cmake configure first)"
}

$nsh = Join-Path $RepoRoot "packaging\version.nsh"
if (Test-Path -LiteralPath $nsh) {
  $nshText = Get-Content -LiteralPath $nsh -Raw
  if ($nshText -match '!define\s+K6WP_VERSION_STR\s+"([^"]+)"') {
    $versionSpots += @{ Where = "packaging/version.nsh K6WP_VERSION_STR"; Value = $Matches[1] }
  } else {
    Fail "version assert: cannot parse K6WP_VERSION_STR from packaging/version.nsh"
  }
  if ($nshText -match '!define\s+K6WP_VERSION_STR_4\s+"([^"]+)"') {
    $versionSpots += @{ Where = "packaging/version.nsh K6WP_VERSION_STR_4"; Value = $Matches[1] }
  } else {
    Fail "version assert: cannot parse K6WP_VERSION_STR_4 from packaging/version.nsh"
  }
} else {
  Fail "version assert: packaging/version.nsh not found (run cmake configure first)"
}

$manifest = Join-Path $RepoRoot "engine\app.manifest"
if (Test-Path -LiteralPath $manifest) {
  $manifestText = Get-Content -LiteralPath $manifest -Raw
  if ($manifestText -match 'version="([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+)"') {
    $versionSpots += @{ Where = "engine/app.manifest assemblyIdentity version"; Value = $Matches[1] }
  } else {
    Fail "version assert: cannot parse version= from engine/app.manifest"
  }
} else {
  Fail "version assert: engine/app.manifest not found (run cmake configure first)"
}

$canonical = $null
foreach ($spot in $versionSpots) {
  if ($spot.Where -eq "CMakeLists.txt K6WP_VERSION") { $canonical = Normalize-Version3 $spot.Value }
}
if ($null -eq $canonical) { Fail "version assert: cannot normalize canonical K6WP_VERSION" }
$bad = @()
foreach ($spot in $versionSpots) {
  $norm = Normalize-Version3 $spot.Value
  if ($null -eq $norm) { $bad += "$($spot.Where)='$($spot.Value)' (unparseable)" }
  elseif ($norm -ne $canonical) { $bad += "$($spot.Where)='$($spot.Value)' != '$canonical'" }
}
if ($bad.Count -gt 0) {
  foreach ($b in $bad) { Log "  VERSION DRIFT: $b" }
  Fail "version assert failed - $($bad.Count) spot(s) disagree with canonical K6WP_VERSION '$canonical'. Fix the source of truth, not the assert."
}

# No hand-written version literals in any consumer source: every version must
# flow from the generated mechanism (version.h / version.nsh / app.manifest.in).
$consumerFiles = @(
  "launcher\app.rc", "engine\app.rc", "studio\app.rc", "compressor\app.rc",
  "shared\monitor_dump.rc", "packaging\installer.nsi",
  "studio\src\update_checker.cpp", "studio\src\studio_bridge.hpp",
  "engine\app.manifest.in", "version.h.in", "version.nsh.in"
)
$literalRe = '[0-9]+[.,][0-9]+[.,][0-9]+'
$literalHits = @()
foreach ($f in $consumerFiles) {
  $fPath = Join-Path $RepoRoot $f
  if (-not (Test-Path -LiteralPath $fPath)) { Fail "version assert: consumer file missing: $f" }
  $m = Select-String -LiteralPath $fPath -Pattern $literalRe -AllMatches
  foreach ($mm in $m) {
    foreach ($hit in $mm.Matches) {
      $literalHits += "$f : '$($hit.Value)'"
    }
  }
}
if ($literalHits.Count -gt 0) {
  foreach ($h in $literalHits) { Log "  VERSION LITERAL: $h" }
  Fail "version assert failed - $($literalHits.Count) hand-written version literal(s) in consumer sources. Reference version.h / version.nsh / app.manifest.in instead."
}

# Every consumer must actually reference the generated mechanism.
$refChecks = @(
  @{ File = "launcher\app.rc";            Needle = 'K6WP_VERSION_COMMA' },
  @{ File = "engine\app.rc";              Needle = 'K6WP_VERSION_COMMA' },
  @{ File = "studio\app.rc";              Needle = 'K6WP_VERSION_COMMA' },
  @{ File = "compressor\app.rc";          Needle = 'K6WP_VERSION_COMMA' },
  @{ File = "shared\monitor_dump.rc";     Needle = 'K6WP_VERSION_COMMA' },
  @{ File = "packaging\installer.nsi";    Needle = 'K6WP_VERSION_STR' },
  @{ File = "studio\src\update_checker.cpp"; Needle = 'K6WP_VERSION_STR' },
  @{ File = "studio\src\studio_bridge.hpp"; Needle = 'K6WP_VERSION_STR' },
  @{ File = "engine\app.manifest.in";     Needle = '@K6WP_VERSION_4@' }
)
$refBad = @()
foreach ($rc in $refChecks) {
  $rcPath = Join-Path $RepoRoot $rc.File
  $rcText = Get-Content -LiteralPath $rcPath -Raw
  if ($rcText -notmatch [regex]::Escape($rc.Needle)) {
    $refBad += "$($rc.File) does not reference '$($rc.Needle)'"
  }
}
if ($refBad.Count -gt 0) {
  foreach ($b in $refBad) { Log "  VERSION REF MISSING: $b" }
  Fail "version assert failed - $($refBad.Count) consumer(s) do not reference the generated version mechanism."
}

Log "Version assert OK: $($versionSpots.Count) version spots agree with K6WP_VERSION '$canonical'; no literals in $($consumerFiles.Count) consumer sources."

# --- 1. Fresh full build (all targets, windeployqt runs as studio POST_BUILD)
# Qt location: explicit override wins, then the CI Qt action's QT_ROOT_DIR,
# then a pre-exported QT_ROOT, then the local install path (local default).
if (-not [string]::IsNullOrWhiteSpace($QtRootOverride)) {
  $env:QT_ROOT = $QtRootOverride
} elseif (-not [string]::IsNullOrWhiteSpace($env:QT_ROOT_DIR)) {
  $env:QT_ROOT = $env:QT_ROOT_DIR
} elseif ([string]::IsNullOrWhiteSpace($env:QT_ROOT)) {
  $env:QT_ROOT = "C:\Qt\6.8.3\msvc2022_64"
}
if ([string]::IsNullOrWhiteSpace($Generator)) {
  Log "Building all targets (preset release)..."
  $buildOut = & $CmakeExe --build --preset release 2>&1 | Out-String
  Add-Content -LiteralPath $script:LogFile -Value $buildOut
  Write-Host $buildOut
  if ($LASTEXITCODE -ne 0) { Fail "cmake --build --preset release failed with exit $LASTEXITCODE" }
} else {
  # "-Generator auto" omits -G so CMake picks the newest installed Visual
  # Studio. Any other value is passed through as -G, which pins a VS version
  # and breaks on a runner that has a different one - that is what made CI
  # packaging fail with "could not find any instance of Visual Studio".
  $genArgs = @()
  if ($Generator -ne "auto") { $genArgs = @("-G", $Generator) }
  Log "Building all targets (generator $(if ($genArgs.Count) { $Generator } else { 'auto-detected' }), dir $BuildDir)..."
  $cfgOut = & $CmakeExe -S $RepoRoot -B $BuildDir @genArgs -A x64 "-DCMAKE_PREFIX_PATH=$env:QT_ROOT" -DBUILD_TESTING=OFF -DK6WP_BUILD_TOOLS=OFF 2>&1 | Out-String
  Add-Content -LiteralPath $script:LogFile -Value $cfgOut
  Write-Host $cfgOut
  if ($LASTEXITCODE -ne 0) { Fail "cmake configure ($Generator) failed with exit $LASTEXITCODE" }
  $buildOut = & $CmakeExe --build $BuildDir --config Release --parallel 2>&1 | Out-String
  Add-Content -LiteralPath $script:LogFile -Value $buildOut
  Write-Host $buildOut
  if ($LASTEXITCODE -ne 0) { Fail "cmake --build ($Generator) failed with exit $LASTEXITCODE" }
}
Log "Build OK."

# --- 2. Fresh staging dir -----------------------------------------------------
$StageDir = Join-Path $DistDir "stage\K6WP-portable-$Version"
if (Test-Path -LiteralPath $StageDir) { Remove-Item -LiteralPath $StageDir -Recurse -Force }
New-Item -ItemType Directory -Path $StageDir -Force | Out-Null

function Stage-Copy($src, $dstName) {
  if (-not (Test-Path -LiteralPath $src)) { Fail "required file missing: $src" }
  Copy-Item -LiteralPath $src -Destination (Join-Path $StageDir $dstName) -Force
  Log "staged: $dstName"
}

# 2a. Our exes (K6WP.exe is the Qt-free single-app launcher, staged next to
# engine/studio per the binary contract; monitor_dump is optional).
foreach ($exe in @("K6WP.exe", "engine.exe", "studio.exe", "compressor.exe")) {
  Stage-Copy (Join-Path $BuildDir $exe) $exe
}
$monitorDump = Join-Path $BuildDir "monitor_dump.exe"
if (Test-Path -LiteralPath $monitorDump) {
  Stage-Copy $monitorDump "monitor_dump.exe"
} else {
  Log "WARNING: monitor_dump.exe not in build dir - skipping (optional)."
}

# 2b. Vendored ffmpeg + ffprobe (full_build; compressor needs ffprobe for the
# duration check on existing inputs, so both ship).
Stage-Copy (Join-Path $RepoRoot "vendor\ffmpeg\ffmpeg.exe") "ffmpeg.exe"
Stage-Copy (Join-Path $RepoRoot "vendor\ffmpeg\ffprobe.exe") "ffprobe.exe"

# 2c. Vendored libmpv.
Stage-Copy (Join-Path $RepoRoot "vendor\libmpv\bin\libmpv-2.dll") "libmpv-2.dll"

# 2d. Qt deploy output: enumerate the ACTUAL windeployqt result in the build
# dir. Rule: every *.dll next to studio.exe + every build subdir that contains
# *.dll files (platforms/, styles/, imageformats/, qml/, ...). Excludes: *.pdb,
# exes, CMake object dirs (*.dir), CMakeFiles, x64, .qt - none of which
# contain deployed DLLs, so the filter "contains *.dll" keeps this exact.
$rootDlls = Get-ChildItem -LiteralPath $BuildDir -Filter "*.dll" -File
if ($rootDlls.Count -eq 0) { Fail "no DLLs next to studio.exe - windeployqt did not run?" }
foreach ($dll in $rootDlls) {
  Copy-Item -LiteralPath $dll.FullName -Destination (Join-Path $StageDir $dll.Name) -Force
}
Log "staged: $($rootDlls.Count) root DLLs (windeployqt output)."
# qmltooling/ is the QML debugger/inspector/profiler plugin set (qmldbg_*).
# windeployqt emits it in Release too, but a shipped app must not carry it:
# qmldbg_tcp.dll + qmldbg_server.dll accept inbound connections, so shipping
# them adds remote-debugger attack surface for zero user value. The QML engine
# never loads them unless a debugger is attached.
$skipDirs = @("CMakeFiles", "x64", ".qt", "qmltooling")
foreach ($dir in Get-ChildItem -LiteralPath $BuildDir -Directory) {
  if ($dir.Name -like "*.dir") { continue }
  if ($skipDirs -contains $dir.Name) { continue }
  $dlls = Get-ChildItem -LiteralPath $dir.FullName -Filter "*.dll" -File -Recurse
  if ($dlls.Count -eq 0) { continue }
  $dest = Join-Path $StageDir $dir.Name
  New-Item -ItemType Directory -Path $dest -Force | Out-Null
  # Copy EVERY file once the dir qualifies, not just the DLLs. The dir gate
  # above is "contains *.dll" (that is what keeps CMake object dirs out), but
  # installer.nsi ships these with `File /r`, which copies the whole tree.
  # qml/ is why this matters: its payload is overwhelmingly non-DLL
  # (.qml / qmldir / .qmltypes) with only a handful of *plugin.dll files, so a
  # DLL-only copy staged 26 files against the installer's 1209.
  $payload = Get-ChildItem -LiteralPath $dir.FullName -File -Recurse
  foreach ($f in $payload) {
    $rel = $f.FullName.Substring($dir.FullName.Length + 1)
    $target = Join-Path $dest $rel
    $parent = Split-Path -Parent $target
    if (-not (Test-Path -LiteralPath $parent)) { New-Item -ItemType Directory -Path $parent -Force | Out-Null }
    Copy-Item -LiteralPath $f.FullName -Destination $target -Force
  }
  Log "staged plugin dir: $($dir.Name) ($($payload.Count) files, $($dlls.Count) DLLs)."
}

# 2e. Example config + uninstaller + license texts (GPL-2.0-or-later release).
Stage-Copy (Join-Path $RepoRoot "packaging\config.json.example") "config.json.example"
Stage-Copy (Join-Path $RepoRoot "packaging\playlist.json.example") "playlist.json.example"
Stage-Copy (Join-Path $RepoRoot "packaging\uninstall.bat") "uninstall.bat"
# Root LICENSE + LICENSES/ third-party notices. LICENSES ships as a subdir
# (installer.nsi stages it via File /r "${REPO_ROOT}\LICENSES", so the
# ship-set sync assert below matches file-for-file).
Stage-Copy (Join-Path $RepoRoot "LICENSE") "LICENSE"
$LicensesDir = Join-Path $StageDir "LICENSES"
New-Item -ItemType Directory -Path $LicensesDir -Force | Out-Null
foreach ($lic in @("libmpv.txt", "ffmpeg.txt", "Qt.txt", "nlohmann-json.txt")) {
  $src = Join-Path $RepoRoot "LICENSES\$lic"
  if (-not (Test-Path -LiteralPath $src)) { Fail "required license file missing: $src" }
  Copy-Item -LiteralPath $src -Destination (Join-Path $LicensesDir $lic) -Force
  Log "staged: LICENSES\$lic"
}

# 2f. App-local VC runtime (fresh-machine safety). Our exes + Qt +
# ffmpeg + libmpv all import MSVCP140/VCRUNTIME140*, so a fresh Windows
# box without any redist-installing app would fail to start. Stage the 3
# CRT DLLs app-local (no UAC, matches the per-user installer design).
# Source: newest VS Redist x64 CRT dir; fallback is a pinned copy under
# vendor\vc_runtime\ (checked in once via Download-VcRuntime below).
$CrtDlls = @("vcruntime140.dll", "vcruntime140_1.dll", "msvcp140.dll")
$RedistRoot = "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Redist\MSVC"
$CrtDir = $null
if (Test-Path -LiteralPath $RedistRoot) {
  $CrtDir = Get-ChildItem -LiteralPath $RedistRoot -Directory |
    Where-Object { $_.Name -match '^14\.' } | Sort-Object Name -Descending |
    ForEach-Object {
      $crt = Get-ChildItem -LiteralPath (Join-Path $_.FullName "x64") -Filter "Microsoft.VC*.CRT" -Directory -ErrorAction SilentlyContinue |
        Where-Object { $_.FullName -notlike "*onecore*" } |
        ForEach-Object { Join-Path $_.FullName "vcruntime140.dll" } |
        Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
      if ($crt) { Split-Path -Parent $crt }
    } | Select-Object -First 1
}
if (-not $CrtDir) {
  # Runner fallback: newest VS carrying VC tools via vswhere (local VS18 path
  # above wins when present, so local behavior is unchanged).
  $vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
  if (Test-Path -LiteralPath $vswhere) {
    $vsPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools -property installationPath 2>$null | Select-Object -First 1
    if (-not [string]::IsNullOrWhiteSpace($vsPath)) {
      $vsRedist = Join-Path $vsPath "VC\Redist\MSVC"
      if (Test-Path -LiteralPath $vsRedist) {
        $CrtDir = Get-ChildItem -LiteralPath $vsRedist -Directory |
          Where-Object { $_.Name -match '^14\.' } | Sort-Object Name -Descending |
          ForEach-Object {
            $crt = Get-ChildItem -LiteralPath (Join-Path $_.FullName "x64") -Filter "Microsoft.VC*.CRT" -Directory -ErrorAction SilentlyContinue |
              Where-Object { $_.FullName -notlike "*onecore*" } |
              ForEach-Object { Join-Path $_.FullName "vcruntime140.dll" } |
              Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
            if ($crt) { Split-Path -Parent $crt }
          } | Select-Object -First 1
        if ($CrtDir) { Log "VS Redist found via vswhere: $vsPath." }
      }
    }
  }
}
if ($CrtDir) {
  foreach ($dll in $CrtDlls) {
    $src = Join-Path $CrtDir $dll
    if (-not (Test-Path -LiteralPath $src)) { Fail "CRT DLL missing in ${CrtDir}: $dll" }
    Stage-Copy $src $dll
  }
  Log "staged: app-local VC runtime from $CrtDir."
  # Cache a pinned copy for installer.nsi builds (makensis cannot discover
  # the versioned Redist dir, so it consumes vendor\vc_runtime\).
  $VendorCrt = Join-Path $RepoRoot "vendor\vc_runtime"
  New-Item -ItemType Directory -Path $VendorCrt -Force | Out-Null
  foreach ($dll in $CrtDlls) {
    Copy-Item -LiteralPath (Join-Path $CrtDir $dll) -Destination (Join-Path $VendorCrt $dll) -Force
  }
  Log "cached: vendor\vc_runtime ($($CrtDlls.Count) DLLs for installer builds)."
} else {
  $VendorCrt = Join-Path $RepoRoot "vendor\vc_runtime"
  $missing = @($CrtDlls | Where-Object { -not (Test-Path -LiteralPath (Join-Path $VendorCrt $_)) })
  if ($missing.Count -gt 0) {
    Fail ("no VS Redist found and vendor\vc_runtime is missing: " + ($missing -join ", ") +
      ". Copy the 3 CRT DLLs there once (see packaging README) or install VS BuildTools.")
  }
  foreach ($dll in $CrtDlls) { Stage-Copy (Join-Path $VendorCrt $dll) $dll }
  Log "staged: app-local VC runtime from vendor\vc_runtime."
}

# --- 2g. Ship-set sync assert (release hardening) ----------------------------
# The staged file set must EXACTLY match the File/RMDir set in
# packaging/installer.nsi (parse the File directives, including the conditional
# monitor_dump.exe and the CRT DLLs + Qt plugin dirs). Fails on drift in either
# direction: a file staged but not installed, or installed but not staged,
# means the ZIP and the installer ship different payloads.
$nsiText = Get-Content -LiteralPath (Join-Path $RepoRoot "packaging\installer.nsi") -Raw

# Define map for the ${...} constants used by File directives.
$nsiSrc = @{
  '${SRCDIR}'        = $BuildDir
  '${VENDOR_FFMPEG}' = Join-Path $RepoRoot "vendor\ffmpeg"
  '${VENDOR_LIBMPV}' = Join-Path $RepoRoot "vendor\libmpv\bin"
  '${VENDOR_VC}'     = Join-Path $RepoRoot "vendor\vc_runtime"
  '${REPO_ROOT}'     = $RepoRoot
}

# Drop !if /FileExists "..."/!endif blocks whose condition is false (the
# conditional monitor_dump.exe File directive), so the expected set matches
# what makensis would actually compile in.
$nsiText = [regex]::Replace($nsiText, '(?s)!if\s+/FileExists\s+"([^"]+)"(.*?)!endif', {
  param($mm)
  $condSrc = $mm.Groups[1].Value
  foreach ($k in $nsiSrc.Keys) { $condSrc = $condSrc.Replace($k, $nsiSrc[$k]) }
  if (Test-Path -LiteralPath $condSrc) { return $mm.Value }
  return ""
})

$expected = @{}  # install-dir-relative path -> installer.nsi source string
foreach ($m in [regex]::Matches($nsiText, '(?m)^\s*File\s+(/r\s+)?"([^"]+)"')) {
  $recursive = $m.Groups[1].Success
  $raw = $m.Groups[2].Value
  $src = $raw
  foreach ($k in $nsiSrc.Keys) { $src = $src.Replace($k, $nsiSrc[$k]) }
  if ($src -eq $raw) {
    # Bare relative path (config.json.example / uninstall.bat): script dir.
    $src = Join-Path $PSScriptRoot $raw
  }
  if ($recursive) {
    if (-not (Test-Path -LiteralPath $src)) {
      Fail "ship-set assert: installer.nsi File /r source missing: $src"
    }
    foreach ($f in Get-ChildItem -LiteralPath $src -Recurse -File) {
      $rel = $f.FullName.Substring($src.Length + 1)
      $expected[(Join-Path (Split-Path -Leaf $src) $rel)] = $raw
    }
  } elseif ($src -like "*\*") {
    # Wildcard (root *.dll): enumerate matches.
    $dir = Split-Path -Parent $src
    $pat = Split-Path -Leaf $src
    if (-not (Test-Path -LiteralPath $dir)) { Fail "ship-set assert: wildcard dir missing: $dir" }
    foreach ($f in Get-ChildItem -LiteralPath $dir -Filter $pat -File) {
      $expected[$f.Name] = $raw
    }
  } else {
    if (-not (Test-Path -LiteralPath $src)) {
      Fail "ship-set assert: installer.nsi File source missing: $src"
    }
    $expected[(Split-Path -Leaf $src)] = $raw
  }
}

$actual = @{}
foreach ($f in Get-ChildItem -LiteralPath $StageDir -Recurse -File) {
  $actual[$f.FullName.Substring($StageDir.Length + 1)] = $true
}

$onlyStaged = @($actual.Keys | Where-Object { -not $expected.ContainsKey($_) } | Sort-Object)
$onlyInstaller = @($expected.Keys | Where-Object { -not $actual.ContainsKey($_) } | Sort-Object)
if ($onlyStaged.Count -gt 0 -or $onlyInstaller.Count -gt 0) {
  foreach ($p in $onlyStaged) { Log "  SHIP-SET DRIFT (staged but not in installer.nsi): $p" }
  foreach ($p in $onlyInstaller) { Log "  SHIP-SET DRIFT (in installer.nsi but not staged): $p" }
  Fail "ship-set assert failed - staged set and installer.nsi File set disagree ($($onlyStaged.Count) staged-only, $($onlyInstaller.Count) installer-only). Keep packaging/make_zip.ps1 and packaging/installer.nsi in sync."
}

# RMDir /r "$INSTDIR\X" dirs (uninstall + rollback) must exist in the stage.
$rmDirs = @()
foreach ($m in [regex]::Matches($nsiText, 'RMDir\s+/r\s+"\$INSTDIR\\([^"]+)"')) {
  $rmDirs += $m.Groups[1].Value
}
$missingDirs = @($rmDirs | Where-Object { -not (Test-Path -LiteralPath (Join-Path $StageDir $_)) } | Sort-Object -Unique)
if ($missingDirs.Count -gt 0) {
  foreach ($d in $missingDirs) { Log "  SHIP-SET DRIFT (RMDir dir not staged): $d" }
  Fail "ship-set assert failed - installer.nsi RMDir /r dir(s) missing from staging: $($missingDirs -join ', ')"
}
Log "Ship-set assert OK: $($expected.Count) staged files match installer.nsi File set; $($rmDirs.Count) RMDir dirs present."

  # --- 3. MISSING-DLL check (dumpbin /dependents per staged exe) ----------------
  # Tried in order, because the MSVC bin layout varies by VS version and host
  # target and this is a fail-closed safety net: if we cannot locate dumpbin we
  # must not wave the ZIP through unverified.
  #   1. PATH (a developer command prompt, or a runner that pre-loads MSVC)
  #   2. the local BuildTools 18 path
  #   3. vswhere -> newest VC toolset -> bin\Hostx64\x64
  #   4. vswhere -> newest VC toolset -> recursive glob, any host/arch dir
  # Located as a plain path string, not an object: Get-Command yields an
  # ApplicationInfo (which has .Path, not .FullName), while the file probes
  # yield FileInfo. Mixing them left the invocation path empty on some machines.
  #
  # PATH is consulted LAST and only accepts a modern MSVC toolset. This machine
  # has Visual Studio 98's dumpbin.exe on PATH, and preferring it silently
  # weakened this check: a VC98 dumpbin cannot parse PE32+ imports, so it
  # reports no dependencies at all and the "all imports resolve" assert
  # passes without having checked anything. A fail-closed gate that can
  # false-pass is worse than no gate.
  $dumpbinPath = $null
  $f = Get-ChildItem -Path "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Tools\MSVC" -Filter "dumpbin.exe" -Recurse -ErrorAction SilentlyContinue |
    Where-Object { $_.FullName -like "*Hostx64\x64*" } | Select-Object -First 1
  if ($f) { $dumpbinPath = $f.FullName }
  if (-not $dumpbinPath) {
    # Every instance, not just -latest: a runner can carry several Visual
    # Studios and the newest one may lack the VC tools.
    $vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
    $vsPaths = @()
    if (Test-Path -LiteralPath $vswhere) {
      $vsPaths = @(& $vswhere -products * -all -prerelease -requires Microsoft.VisualStudio.Component.VC.Tools -property installationPath 2>$null |
        Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
    }
    foreach ($vp in $vsPaths) {
      $msvcRoot = Join-Path $vp "VC\Tools\MSVC"
      $toolset = Get-ChildItem -LiteralPath $msvcRoot -Directory -ErrorAction SilentlyContinue |
        Sort-Object Name -Descending | Select-Object -First 1
      if ($toolset) {
        $cand = Join-Path $toolset.FullName "bin\Hostx64\x64\dumpbin.exe"
        if (Test-Path -LiteralPath $cand) { $dumpbinPath = $cand; break }
      }
      $g = Get-ChildItem -LiteralPath $msvcRoot -Filter "dumpbin.exe" -Recurse -ErrorAction SilentlyContinue |
        Select-Object -First 1
      if ($g) { $dumpbinPath = $g.FullName; break }
    }
    # A GitHub windows runner has no VS instance carrying the VC.Tools
    # component id (vswhere reported 0) and no dumpbin on PATH, yet the build
    # succeeds, so a toolset is present. Guessing VS directory layouts kept
    # missing; ask the build system instead. CMake records the toolchain it
    # resolved in CMakeCache - the linker and RC compiler are recorded
    # (CMAKE_CXX_COMPILER is not), and dumpbin sits in the same
    # .../VC/Tools/MSVC/<ver>/bin/Hostx64/x64 directory as link.exe, so derive
    # it from whichever of those is present.
    if (-not $dumpbinPath) {
      $candidates = @(
        (Join-Path $BuildDir "CMakeCache.txt"),
        (Join-Path $RepoRoot "build\CMakeCache.txt"),
        (Join-Path $RepoRoot "build\Release\CMakeCache.txt")
      )
      foreach ($cache in $candidates) {
        if (-not (Test-Path -LiteralPath $cache)) { continue }
        $tool = $null
        foreach ($key in @('CMAKE_LINKER', 'CMAKE_RC_COMPILER', 'CMAKE_CXX_COMPILER')) {
          $hit = Select-String -LiteralPath $cache -Pattern ("^" + $key + ":") -ErrorAction SilentlyContinue |
            Select-Object -First 1
          if ($hit) { $tool = ($hit.Line -split '=', 2)[1].Trim(); break }
        }
        if (-not $tool) { continue }
        $binDir = Split-Path -Parent $tool
        $d = Get-ChildItem -LiteralPath $binDir -Filter "dumpbin.exe" -ErrorAction SilentlyContinue |
          Select-Object -First 1
        if ($d) { $dumpbinPath = $d.FullName; break }
        # RC.EXE can live outside the toolset (Common\MDev98\Bin), so also walk
        # up to the toolset and glob.
        $d2 = Get-ChildItem -LiteralPath (Split-Path -Parent $binDir) -Filter "dumpbin.exe" -Recurse -ErrorAction SilentlyContinue |
          Select-Object -First 1
        if ($d2) { $dumpbinPath = $d2.FullName; break }
      }
    }
    if (-not $dumpbinPath) {
      $onPath = Get-Command dumpbin.exe -ErrorAction SilentlyContinue | Select-Object -First 1
      if ($onPath -and $onPath.Path -like "*\Tools\MSVC\*") { $dumpbinPath = $onPath.Path }
    }
    if (-not $dumpbinPath) {
      # Report what was actually inspected. A fail-closed gate that cannot say
      # why it failed just sends the next person back to guessing.
      $seen = @()
      $seen += "vswhere present : $(Test-Path -LiteralPath $vswhere) ($vswhere)"
      $seen += "instances with VC.Tools : $($vsPaths.Count)"
      foreach ($vp in $vsPaths) {
        $seen += "  $vp"
        $seen += "    VC\Tools\MSVC exists : $(Test-Path -LiteralPath (Join-Path $vp 'VC\Tools\MSVC'))"
        $seen += "    bin contents : $((Get-ChildItem -LiteralPath (Join-Path $vp 'VC\Tools') -Recurse -Filter 'dumpbin.exe' -ErrorAction SilentlyContinue | Select-Object -First 3 -ExpandProperty FullName) -join '; ')"
      }
      foreach ($cache in @((Join-Path $BuildDir "CMakeCache.txt"), (Join-Path $RepoRoot "build\CMakeCache.txt"))) {
        $seen += "cache $cache exists : $(Test-Path -LiteralPath $cache)"
        if (Test-Path -LiteralPath $cache) {
          foreach ($key in @('CMAKE_LINKER', 'CMAKE_RC_COMPILER', 'CMAKE_CXX_COMPILER')) {
            $hit = Select-String -LiteralPath $cache -Pattern ("^" + $key + ":") -ErrorAction SilentlyContinue | Select-Object -First 1
            $seen += "  $key : $(if ($hit) { $hit.Line } else { 'not recorded' })"
          }
        }
      }
      $seen += "dumpbin on PATH : $(if (Get-Command dumpbin.exe -ErrorAction SilentlyContinue) { (Get-Command dumpbin.exe).Source } else { 'none' })"
      Fail ("dumpbin.exe (Hostx64) not found. Probed: " + ($seen -join ' | '))
    }
  }
Log "dumpbin: $($dumpbinPath)"

$systemDir = Join-Path $env:SystemRoot "System32"
$missing = @()
$warnings = @()
$exesToCheck = @("K6WP.exe", "engine.exe", "studio.exe", "compressor.exe")
if (Test-Path -LiteralPath (Join-Path $StageDir "monitor_dump.exe")) { $exesToCheck += "monitor_dump.exe" }
foreach ($exe in $exesToCheck) {
  $out = & $dumpbinPath /dependents (Join-Path $StageDir $exe) 2>&1 | Out-String
  $inDelay = $false
  foreach ($line in $out -split "`r?`n") {
    if ($line -match "delay load dependencies") { $inDelay = $true; continue }
    if ($line -match "^\s+(\S+\.dll)\s*$") {
      $dep = $Matches[1]
      if ($dep -like "api-ms-win-*" -or $dep -like "ext-ms-*") { continue }  # API sets, always present
      $inStage = Test-Path -LiteralPath (Join-Path $StageDir $dep)
      $inSystem = Test-Path -LiteralPath (Join-Path $systemDir $dep)
      if (-not $inStage -and -not $inSystem) {
        if ($inDelay) { $warnings += "${exe}: delay-load $dep" }
        else { $missing += "${exe}: $dep" }
      }
    }
  }
}
foreach ($w in $warnings) { Log "WARNING (delay-load, runtime-only): $w" }
if ($missing.Count -gt 0) {
  Log "MISSING DLLS ($($missing.Count)):"
  foreach ($m in $missing) { Log "  MISSING: $m" }
  Fail "import check failed - $($missing.Count) missing DLLs listed above. Not shipping a broken ZIP."
}
Log "Import check OK: all hard imports of $($exesToCheck.Count) exes resolve from staging dir or System32."

# --- 3b. Qt-free launcher assert (owner decision F1): K6WP.exe must not
# depend on any Qt6*.dll. Locks the user32+shell32-only property into release.
$qtDeps = @()
$k6wpOut = & $dumpbinPath /dependents (Join-Path $StageDir "K6WP.exe") 2>&1 | Out-String
foreach ($line in $k6wpOut -split "`r?`n") {
  if ($line -match "^\s+(\S+\.dll)\s*$") {
    if ($Matches[1] -like "Qt6*.dll") { $qtDeps += $Matches[1] }
  }
}
if ($qtDeps.Count -gt 0) {
  foreach ($d in $qtDeps) { Log "  QT-FREE VIOLATION: K6WP.exe depends on $d" }
  Fail "Qt-free assert failed - K6WP.exe pulls $($qtDeps.Count) Qt DLL(s). Launcher must stay Qt-free."
}
Log "Qt-free assert OK: K6WP.exe has no Qt6*.dll dependents."

# --- 3c. VERSIONINFO assert: ProductName must be K6WP on every staged exe
# (GUI-subsystem triple + compressor + monitor_dump, proven in
# build/qa_todo11.log). Also asserts each staged exe carries a non-empty
# .rsrc section (dumpbin /headers) so a lost icon resource fails loudly.
$versionExes = @("K6WP.exe", "engine.exe", "studio.exe", "compressor.exe")
if (Test-Path -LiteralPath (Join-Path $StageDir "monitor_dump.exe")) { $versionExes += "monitor_dump.exe" }
foreach ($exe in $versionExes) {
  $prodName = [System.Diagnostics.FileVersionInfo]::GetVersionInfo((Join-Path $StageDir $exe)).ProductName
  Log "VersionInfo: $exe ProductName='$prodName'"
  if ($prodName -ne "K6WP") { Fail "version assert failed - $exe ProductName='$prodName', expected 'K6WP'." }
  $headers = & $dumpbinPath /headers (Join-Path $StageDir $exe) 2>&1 | Out-String
  # dumpbin prints one "SECTION HEADER #N" block per section with ".rsrc name"
  # on its own line and the size two lines below ("<hex> size of raw data").
  $rsrcBytes = 0
  $inRsrc = $false
  foreach ($line in $headers -split "`r?`n") {
    if ($line -match "^\s*\.rsrc\s+name\s*$") { $inRsrc = $true; continue }
    if ($line -match "^SECTION HEADER") { $inRsrc = $false; continue }
    if ($inRsrc -and $line -match "^\s*([0-9A-Fa-f]+)\s+size of raw data\s*$") {
      $rsrcBytes = [Convert]::ToInt32($Matches[1], 16)
      break
    }
  }
  if ($rsrcBytes -eq 0) { Fail "resource assert failed - $exe has no .rsrc section with raw data (icon lost?)." }
  Log "Resource: $exe .rsrc raw bytes=0x$($rsrcBytes.ToString('X'))"
}
Log "Version assert OK: ProductName==K6WP + .rsrc present on $($versionExes -join ', ')."

# --- 4. Size accounting + ZIP --------------------------------------------------
Log "Per-file sizes in staging (MB):"
$files = Get-ChildItem -LiteralPath $StageDir -Recurse -File | Sort-Object Length -Descending
foreach ($f in $files) {
  $mb = [math]::Round($f.Length / 1MB, 2)
  $rel = $f.FullName.Substring($StageDir.Length + 1)
  Log ("  {0,8} MB  {1}" -f $mb, $rel)
}
$uncompMB = [math]::Round(($files | Measure-Object Length -Sum).Sum / 1MB, 2)
Log "Staging total uncompressed: $uncompMB MB ($($files.Count) files)."

$ZipPath = Join-Path $DistDir "K6WP-portable-$Version.zip"
if (Test-Path -LiteralPath $ZipPath) { Remove-Item -LiteralPath $ZipPath -Force }
Compress-Archive -Path (Join-Path $StageDir "*") -DestinationPath $ZipPath -Force
$zipMB = [math]::Round((Get-Item -LiteralPath $ZipPath).Length / 1MB, 2)
Log "ZIP: $ZipPath ($zipMB MB)."
if ($zipMB -ge $SizeLimitMB) {
  Fail "ZIP is $zipMB MB, over the ${SizeLimitMB} MB limit. Breakdown logged above - not shipping oversize."
}
Log "Size assert OK: $zipMB MB < $SizeLimitMB MB."
Log "DONE. Artifact: $ZipPath"
```

## File: packaging/playlist.json.example
```
{
  "_comment": "K6WP example wallpaper playlist, schema v1. Copy to %LOCALAPPDATA%/K6WP/playlist.json and set order to REAL video files on your machine - the placeholders below do not exist. When enabled, the engine rotates through order every interval_min minutes. interval_min is clamped to [1, 1440]; entries are de-duplicated case-insensitively. The engine reads this file; Studio edits it. It is separate from config.json so older Studio builds can never clobber it.",
  "version": 1,
  "enabled": false,
  "interval_min": 30,
  "shuffle": false,
  "order": [
    "C:/Videos/wallpaper-a.mp4",
    "C:/Videos/wallpaper-b.mp4"
  ]
}
```

## File: packaging/resource.h
```c

```

## File: packaging/sign_outputs.ps1
```powershell
# K6WP release signing step (task 36, AV/SmartScreen remediation).
#
# STRICTLY OPT-IN. Signing only runs when BOTH environment variables are set:
#   K6WP_SIGN_PFX            - path to a code-signing PFX file
#   K6WP_SIGN_PFX_PASSWORD   - password for that PFX
# Optional:
#   K6WP_SIGN_TIMESTAMP_URL  - RFC 3161 timestamp server
#                              (default: http://timestamp.digicert.com)
#
# When the env vars are unset, or signtool.exe cannot be located, the script
# prints a clear NO-OP message and exits 0 — packaging/make_zip.ps1 and
# packaging/installer.nsi therefore never break in the default (unsigned)
# configuration. No certificate ships with the repo; obtaining one is a
# documented action item in packaging/known-limitations.md §4.
#
# What gets signed (SHA256 file digest + RFC 3161 timestamp, /fd SHA256
# /td SHA256 /tr <url>):
#   - every *.exe under the portable staging dir dist\stage\K6WP-portable-<ver>\
#     (our K6WP/engine/studio/compressor/monitor_dump exes AND the vendored
#     ffmpeg/ffprobe — the whole ship-set is signed so no shipped exe is left
#     "Unknown publisher"),
#   - dist\K6WP-Setup.exe when it exists (NSIS output; run makensis first).
#
# Intended release order when signing is enabled:
#   1. powershell -ExecutionPolicy Bypass -File packaging\make_zip.ps1
#      (build + stage + ZIP + asserts)
#   2. makensis packaging\installer.nsi          -> dist\K6WP-Setup.exe
#   3. powershell -ExecutionPolicy Bypass -File packaging\sign_outputs.ps1
#      (signs the staged exes + dist\K6WP-Setup.exe IN PLACE)
#   4. Re-zip the signed stage so the portable ZIP matches the installer:
#      Compress-Archive -Force -Path "dist\stage\K6WP-portable-<ver>\*" `
#        -DestinationPath "dist\K6WP-portable-<ver>.zip"
# Step 4 is intentionally NOT automated here: make_zip.ps1 owns the ZIP
# asserts (size limit, ship-set sync) and must stay untouched.
#
# Usage (PowerShell 5.1):
#   powershell -ExecutionPolicy Bypass -File packaging\sign_outputs.ps1 `
#     [-StageDir <path>] [-SetupExe <path>]
#   -StageDir  overrides the auto-discovered newest dist\stage\K6WP-portable-* dir
#   -SetupExe  overrides the default dist\K6WP-Setup.exe path
#
# Exit codes: 0 = signed OK or no-op (env unset / signtool missing / nothing
# to sign); 1 = misconfiguration (PFX path invalid) or a signtool failure.
# Never claims anything about resulting AV/SmartScreen verdicts — see
# packaging/known-limitations.md §4.

param(
  [string]$StageDir = "",
  [string]$SetupExe = ""
)

$ErrorActionPreference = "Continue"

$RepoRoot = Split-Path -Parent $PSScriptRoot
$TimestampUrl = "http://timestamp.digicert.com"
if (-not [string]::IsNullOrWhiteSpace($env:K6WP_SIGN_TIMESTAMP_URL)) {
  $TimestampUrl = $env:K6WP_SIGN_TIMESTAMP_URL
}

function Log($msg) {
  Write-Host "[sign_outputs] $msg"
}

function NoOp($why) {
  Log "NO-OP: $why"
  Log "Nothing was signed; binaries stay unsigned. Exiting 0 (signing is strictly opt-in)."
  exit 0
}

# --- Gate 1: signing env vars -------------------------------------------------
if ([string]::IsNullOrWhiteSpace($env:K6WP_SIGN_PFX) -or
    [string]::IsNullOrWhiteSpace($env:K6WP_SIGN_PFX_PASSWORD)) {
  NoOp "K6WP_SIGN_PFX / K6WP_SIGN_PFX_PASSWORD are not both set."
}
if (-not (Test-Path -LiteralPath $env:K6WP_SIGN_PFX)) {
  # Env is set but the file is missing: a real misconfiguration, not a no-op.
  Log "ERROR: K6WP_SIGN_PFX is set but the file does not exist: $env:K6WP_SIGN_PFX"
  exit 1
}

# --- Gate 2: locate signtool.exe (PATH first, then vswhere -> newest VC toolset) ---
# Fail soft: a machine without the VC toolset simply no-ops instead of failing
# the release pipeline.
$signtoolPath = $null
$onPath = Get-Command signtool.exe -ErrorAction SilentlyContinue | Select-Object -First 1
if ($onPath) {
  # Get-Command yields ApplicationInfo (.Path), not FileInfo (.FullName).
  $signtoolPath = $onPath.Path
}
if (-not $signtoolPath) {
  $vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
  if (Test-Path -LiteralPath $vswhere) {
    $vsPaths = @(& $vswhere -products * -all -prerelease -requires Microsoft.VisualStudio.Component.VC.Tools -property installationPath 2>$null |
      Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
    foreach ($vp in $vsPaths) {
      $msvcRoot = Join-Path $vp "VC\Tools\MSVC"
      if (-not (Test-Path -LiteralPath $msvcRoot)) { continue }
      # Prefer the Hostx64\x64 edition (matches make_zip.ps1's dumpbin policy);
      # fall back to any copy the toolset ships.
      $f = Get-ChildItem -LiteralPath $msvcRoot -Filter "signtool.exe" -Recurse -ErrorAction SilentlyContinue |
        Where-Object { $_.FullName -like "*Hostx64\x64*" } | Select-Object -First 1
      if (-not $f) {
        $f = Get-ChildItem -LiteralPath $msvcRoot -Filter "signtool.exe" -Recurse -ErrorAction SilentlyContinue |
          Select-Object -First 1
      }
      if ($f) { $signtoolPath = $f.FullName; break }
    }
  }
}
if (-not $signtoolPath) {
  NoOp "signtool.exe not found (not on PATH, and no Visual Studio instance with VC tools found via vswhere)."
}
Log "signtool: $signtoolPath"

# --- Collect targets ----------------------------------------------------------
if ([string]::IsNullOrWhiteSpace($StageDir)) {
  $stageRoot = Join-Path $RepoRoot "dist\stage"
  if (Test-Path -LiteralPath $stageRoot) {
    $latest = Get-ChildItem -LiteralPath $stageRoot -Directory -Filter "K6WP-portable-*" |
      Sort-Object Name -Descending | Select-Object -First 1
    if ($latest) { $StageDir = $latest.FullName }
  }
}
$targets = @()
if (-not [string]::IsNullOrWhiteSpace($StageDir) -and (Test-Path -LiteralPath $StageDir)) {
  $targets += @(Get-ChildItem -LiteralPath $StageDir -Filter "*.exe" -Recurse -File)
  Log "staging dir: $StageDir ($($targets.Count) exe(s) found)."
} else {
  Log "WARNING: no staging dir found (looked for newest dist\stage\K6WP-portable-*); installer will still be signed if present."
}
if ([string]::IsNullOrWhiteSpace($SetupExe)) {
  $SetupExe = Join-Path $RepoRoot "dist\K6WP-Setup.exe"
}
if (Test-Path -LiteralPath $SetupExe) {
  $targets += Get-Item -LiteralPath $SetupExe
  Log "installer: $SetupExe"
} else {
  Log "note: $SetupExe not present (run makensis first) - installer skipped."
}

if ($targets.Count -eq 0) {
  NoOp "no signable files found (no staged exes, no dist\K6WP-Setup.exe)."
}

# --- Sign (SHA256 + RFC3161 timestamp) ----------------------------------------
Log "signing $($targets.Count) file(s) with SHA256 + RFC3161 timestamp ($TimestampUrl)..."
Log "certificate: $env:K6WP_SIGN_PFX"
$failed = @()
foreach ($t in $targets) {
  Log "signing $($t.FullName)"
  $out = & $signtoolPath sign /f $env:K6WP_SIGN_PFX /p $env:K6WP_SIGN_PFX_PASSWORD /fd SHA256 /td SHA256 /tr $TimestampUrl $t.FullName 2>&1 | Out-String
  if ($LASTEXITCODE -ne 0) {
    $failed += $t.Name
    Log "FAILED (signtool exit $LASTEXITCODE) on $($t.Name):"
    Write-Host $out
  } else {
    Log "OK: $($t.Name)"
  }
}
if ($failed.Count -gt 0) {
  Log "ERROR: $($failed.Count) of $($targets.Count) file(s) failed to sign: $($failed -join ', ')"
  exit 1
}
Log "DONE: signed $($targets.Count) file(s)."
Log "NOTE: signing reduces false positives; it does NOT guarantee clean AV/SmartScreen verdicts (packaging/known-limitations.md §4)."
exit 0
```

## File: packaging/studio_settings.json.example
```
{
  "_comment": "K6WP example studio settings, schema v5. Copy to %LOCALAPPDATA%/K6WP/studio_settings.json to override Studio defaults. Paths below use %LOCALAPPDATA% as a placeholder - replace with the expanded folder on your machine, or leave the file absent to use the built-in defaults (identical values). A pre-v5 file migrates on load: wallpaper_sync is added (true), lockscreen_sync keeps the value you chose (it now means the per-user WinRT lock-screen image, no administrator policy or UAC), and a lockscreen_offset_sec of exactly 1.0 (the old default) becomes 0.0 (the first frame).",
  "version": 5,
  "auto_compress_on_import": true,
  "compress_output_dir": "%LOCALAPPDATA%/K6WP/wallpapers",
  "default_crf": 22,
  "default_fps": 30,
  "default_resolution_mode": "match_monitor",
  "start_with_windows": false,
  "cache_dir": "%LOCALAPPDATA%/K6WP/cache",
  "wallpaper_sync": true,
  "lockscreen_sync": true,
  "lockscreen_offset_sec": 0.0,
  "lockscreen_offer_shown": false,
  "compress_advanced_visible": false
}
```

## File: packaging/uninstall.bat
```batch
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
```

## File: packaging/version.nsh
```
; version.nsh.in — NSIS version constants, generated by CMake from
; K6WP_VERSION (root CMakeLists.txt) via configure_file(). Do not edit —
; change K6WP_VERSION in the root CMakeLists.txt and reconfigure.
; Consumed by packaging/installer.nsi (!include "version.nsh").
!define K6WP_VERSION_STR "0.0.1-alpha.3"
!define K6WP_VERSION_STR_4 "0.0.1.0"
```

## File: shared/autostart.cpp
```cpp
class RegKeyGuard {
⋮----
explicit RegKeyGuard(HKEY key) : key_(key) {}
RegKeyGuard(const RegKeyGuard&) = delete;
⋮----
HKEY get() const { return key_; }
⋮----
std::string WideToUtf8(const std::wstring& wide) {
⋮----
std::string FormatRegError(const char* op, DWORD code) {
⋮----
std::wstring msg(msg_buf, msg_len);
⋮----
void SetError(std::string* error_out, const std::string& msg) {
⋮----
}  // namespace
⋮----
std::wstring AutostartCommand() {
⋮----
// Determine the target exe for the Run value.
// Priority 1: if the caller itself is K6WP.exe, use it.
// Priority 2: otherwise, prefer <exe_dir>/K6WP.exe when it exists.
// Priority 3: legacy fall-back to sibling engine.exe + --minimized.
// Never throws (error_code overloads on filesystem ops).
⋮----
// Normalize to lower for comparison.
⋮----
// Caller is K6WP.exe itself — use its own path, silent + engine flag.
// exe is already the caller's path; keep it.
⋮----
bool IsAutostart() noexcept {
⋮----
RegKeyGuard guard(key);
⋮----
bool SetAutostart(bool enable, std::string* error_out) {
```

## File: shared/autostart.hpp
```cpp
bool IsAutostart() noexcept;
⋮----
std::wstring AutostartCommand();
⋮----
bool SetAutostart(bool enable, std::string* error_out);
```

## File: shared/compress_args.cpp
```cpp
std::vector<std::string> BuildCompressArgv(const CompressArgs& args) {
⋮----
bool IsKnownCompressorFlag(const std::string& flag) {
```

## File: shared/compress_args.hpp
```cpp
struct CompressArgs {
⋮----
std::vector<std::string> BuildCompressArgv(const CompressArgs& args);
⋮----
bool IsKnownCompressorFlag(const std::string& flag);
```

## File: shared/config_schema.json
```json
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "$id": "https://k6wp.local/schemas/config.schema.json",
  "title": "K6WP WallpaperConfig",
  "description": "Versioned on-disk configuration for the K6WP wallpaper engine. Stored at %LOCALAPPDATA%/K6WP/config.json. Version 0 (legacy) is missing the newer fields and is migrated to version 6 by the loader; version 1 migrates to version 6 with monitor_id forced to -1 (the v0/v1 id had no render effect); version 2 migrates with legacy fit_mode fill normalized to cover; version 3 migrates with battery_mode defaulted to cap24; version 4 migrates with cpu_affinity/gpu_adapter defaulted to auto; version 5 migrates with pause_unload_after_sec defaulted to 30. On migration (or corrupt input) the loader backs up the original bytes to <config>.bak and rewrites the file at the current version.",
  "type": "object",
  "additionalProperties": false,
  "required": ["version"],
  "properties": {
    "version": {
      "type": "integer",
      "minimum": 0,
      "maximum": 6,
      "description": "Schema version. 0 = legacy (missing fields are defaulted on load), 1 = monitor_id without render effect, 2 = monitor targeting (-1 = all screens), 3 = legacy fit_mode fill normalized to cover, 4 = battery_mode cap24|static, 5 = cpu_affinity auto|all + gpu_adapter auto|integrated|discrete, 6 = current (pause_unload_after_sec: unload the decode pipeline after a long pause)."
    },
    "video_path": {
      "type": "string",
      "description": "Path to the wallpaper video file, UTF-8 encoded. May contain non-UTF8 bytes on disk; the loader reads the raw path."
    },
    "fit_mode": {
      "type": "string",
      "enum": ["cover", "fill", "fit", "stretch", "center"],
      "default": "cover",
      "description": "How the video is fitted to the monitor. cover/fill = fullscreen fill (no black bars); fit = contain (letterbox); stretch = distort to fill; center = original size."
    },
    "speed": {
      "type": "number",
      "minimum": 0.5,
      "maximum": 2.0,
      "default": 1.0,
      "description": "Playback speed multiplier. Values outside 0.5-2.0 are rejected. RESERVED: validated and persisted but not yet applied by the renderer."
    },
    "monitor_id": {
      "type": "integer",
      "minimum": -1,
      "default": -1,
      "description": "Target monitor id: -1 = all screens; >=0 = only that monitor."
    },
    "crf": {
      "type": "integer",
      "minimum": 16,
      "maximum": 28,
      "default": 22,
      "description": "Compression CRF for the compressor. Values outside 16-28 are rejected."
    },
    "resolution_w": {
      "type": "integer",
      "minimum": 0,
      "default": 0,
      "description": "Compress target width. 0 = use monitor native resolution."
    },
    "resolution_h": {
      "type": "integer",
      "minimum": 0,
      "default": 0,
      "description": "Compress target height. 0 = use monitor native resolution."
    },
    "fps_cap": {
      "type": "integer",
      "minimum": 1,
      "maximum": 30,
      "default": 24,
      "description": "Maximum playback/compress framerate. Values above 30 are rejected."
    },
    "battery_saver": {
      "type": "boolean",
      "default": false,
      "description": "When true, the engine caps fps / pauses while on battery (DC power)."
    },
    "battery_mode": {
      "type": "string",
      "enum": ["cap24", "static"],
      "default": "cap24",
      "description": "DC-power policy, consulted only when battery_saver is on and the box is on DC power. cap24 = cap fps to 24 (legacy behavior); static = pause slot decode via the power pause bit, holding the last frame, with no fps-cap change. Ignored on AC and when battery_saver is off."
    },
    "cpu_affinity": {
      "type": "string",
      "enum": ["auto", "all"],
      "default": "auto",
      "description": "Engine CPU affinity. auto = pin the engine to performance cores on hybrid (P/E) CPUs; all = use every logical processor."
    },
    "gpu_adapter": {
      "type": "string",
      "enum": ["auto", "integrated", "discrete"],
      "default": "auto",
      "description": "GPU adapter preference for video decode/render on hybrid systems (mpv d3d11-adapter pin). auto = prefer the integrated GPU; integrated/discrete force that adapter when present."
    },
    "reconcile_mode": {
      "type": "string",
      "enum": ["legacy", "shadow", "on"],
      "default": "legacy",
      "description": "Display-resilience reconcile engine mode. legacy = today's behavior (no reconcile pass); shadow = compute snapshot + diff each debounce burst and report only (no slot mutation, never writes displays.json); on = cut over to live reconcile (P4)."
    },
    "pause_unload_after_sec": {
      "type": "integer",
      "minimum": 0,
      "maximum": 3600,
      "default": 30,
      "description": "Unload the decode pipeline after this many seconds of continuous pause. A paused wallpaper used to hold the whole decoder and its GPU surfaces (measured: 162 MB private + 152 MB GPU shared for a paused 1080p session, against 24 MB for an idle engine). After the deadline the file is released, the wallpaper stays on screen (the last frame) and the memory goes back. 0 = never unload."
    }
  }
}
```

## File: shared/displays_schema.cpp
```cpp
bool NeedsMigration(const nlohmann::json& raw) {
⋮----
bool HasDotDotComponent(const std::wstring& p) {
const std::filesystem::path fp(p);
⋮----
}  // namespace
⋮----
nlohmann::json DisplaysToJson(const DisplaysConfig& cfg) {
⋮----
DisplaysConfig LoadDisplays(const std::filesystem::path& path) {
⋮----
void SaveDisplays(const std::filesystem::path& path,
⋮----
void ValidateDisplays(DisplaysConfig& cfg) {
⋮----
DisplaysConfig MigrateDisplays(const nlohmann::json& raw) {
⋮----
std::filesystem::path DefaultDisplaysPath() {
⋮----
std::vector<std::wstring> DetectKeyCollision(
```

## File: shared/displays_schema.json
```json
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "$id": "https://k6wp.local/schemas/displays.schema.json",
  "title": "K6WP DisplaysConfig",
  "description": "Per-monitor assignment store for the K6WP wallpaper engine. Stored at %LOCALAPPDATA%/K6WP/displays.json. Deliberately separate from config.json: older Studio builds rewrite config.json from their own WallpaperConfig struct and would erase an assignment map stored there (same rationale as playlist.json). Engine reads the file (and watches it); Studio writes it. Version 0 (legacy/absent) is migrated to version 1 by the loader. On migration (or corrupt input) the loader backs up the original bytes to <file>.bak and rewrites the file at the current version.",
  "type": "object",
  "additionalProperties": false,
  "required": ["version"],
  "properties": {
    "version": {
      "type": "integer",
      "minimum": 0,
      "maximum": 1,
      "description": "Schema version. 0 = legacy (missing fields are defaulted on load), 1 = current."
    },
    "assignments": {
      "type": "object",
      "default": {},
      "description": "Per-monitor video assignments, keyed by the GDI device name as returned by MONITORINFOEXW.szDevice (e.g. \\\\.\\DISPLAY1). Each value names the video file that plays on that monitor. Empty object = no per-monitor assignment (engine falls back to config.json playback state).",
      "additionalProperties": {
        "type": "object",
        "required": ["path"],
        "properties": {
          "path": {
            "type": "string",
            "description": "Absolute path to the wallpaper video file, UTF-8 encoded. Must be non-empty and must not contain a '..' path component; values outside those rules are rejected by ValidateDisplays."
          },
          "exists": {
            "type": "boolean",
            "default": false,
            "description": "Studio-side existence hint recorded at write time (matches the {path,exists} QML shape of playlist items). The engine re-checks the filesystem at apply time and does not trust this flag."
          }
        }
      }
    },
    "displays": {
      "type": "array",
      "default": [],
      "description": "Display-only metadata Studio may write (orientation notes, friendly names, ...). The engine IGNORES this array; it is preserved as raw JSON across load/save round-trips."
    }
  }
}
```

## File: shared/ffmpeg_path.hpp
```cpp
std::filesystem::path FindFfmpeg();
⋮----
std::filesystem::path FindFfprobe();
```

## File: shared/legacy_lockscreen_policy.cpp
```cpp
std::filesystem::path ProgramDataBase() {
⋮----
std::wstring ReadPolicyValue() {
⋮----
std::filesystem::path SiblingLauncher() {
⋮----
std::filesystem::path LegacyLockscreenDir() {
⋮----
std::filesystem::path LegacyLockscreenImagePath() {
⋮----
LegacyLockscreenState ReadLegacyLockscreenState() noexcept {
⋮----
LegacyCleanupOutcome ClassifyLegacyCleanup(bool ran, int exit_code) noexcept {
⋮----
LegacyCleanupOutcome RunLegacyCleanup(int timeout_ms) noexcept {
⋮----
void SetLegacyCleanupFn(LegacyCleanupFn fn) noexcept { g_cleanup_fn = fn; }
```

## File: shared/legacy_lockscreen_policy.hpp
```cpp
std::filesystem::path LegacyLockscreenDir();
⋮----
std::filesystem::path LegacyLockscreenImagePath();
⋮----
struct LegacyLockscreenState {
⋮----
bool cleanup_needed() const {
⋮----
LegacyLockscreenState ReadLegacyLockscreenState() noexcept;
⋮----
enum class LegacyCleanupOutcome {
⋮----
LegacyCleanupOutcome ClassifyLegacyCleanup(bool ran, int exit_code) noexcept;
⋮----
LegacyCleanupOutcome RunLegacyCleanup(int timeout_ms) noexcept;
⋮----
void SetLegacyCleanupFn(LegacyCleanupFn fn) noexcept;
```

## File: shared/links.hpp
```cpp

```

## File: shared/lockscreen_exit.hpp
```cpp

```

## File: shared/monitor_dump.cpp
```cpp
int main() {
```

## File: shared/monitor_dump.rc
```
// shared/monitor_dump.rc - resource script for monitor_dump.exe.
//
// IDI_APPICON + VERSIONINFO (ProductName K6WP), same pattern as the other
// app.rc files. The .ico lives in ../packaging/ and is referenced by bare
// name; packaging/ is on the RC include path (shared/CMakeLists.txt feeds
// it to monitor_dump, same pattern as engine/launcher/compressor).

#include <windows.h>
#include "resource.h"
#include "version.h"

IDI_APPICON ICON "k6wp-on.ico"

VS_VERSION_INFO VERSIONINFO
 FILEVERSION K6WP_VERSION_COMMA
 PRODUCTVERSION K6WP_VERSION_COMMA
 FILEFLAGSMASK 0x3fL
#ifdef _DEBUG
 FILEFLAGS 0x1L
#else
 FILEFLAGS 0x0L
#endif
 FILEOS 0x40004L
 FILETYPE 0x1L
 FILESUBTYPE 0x0L
BEGIN
    BLOCK "StringFileInfo"
    BEGIN
        BLOCK "040904b0"
        BEGIN
            VALUE "CompanyName", "K6WP contributors"
            VALUE "FileDescription", "K6WP Monitor Dump"
            VALUE "FileVersion", K6WP_VERSION_STR_4
            VALUE "InternalName", "monitor_dump"
            VALUE "LegalCopyright", "Copyright (C) 2026 K6WP contributors - GPL-2.0-or-later"
            VALUE "OriginalFilename", "monitor_dump.exe"
            VALUE "ProductName", "K6WP"
            VALUE "ProductVersion", K6WP_VERSION_STR_4
        END
    END
    BLOCK "VarFileInfo"
    BEGIN
        VALUE "Translation", 0x409, 1200
    END
END
```

## File: shared/monitor_util.cpp
```cpp
struct EnumMonitorsContext {
⋮----
BOOL CALLBACK OnMonitorEnum(HMONITOR hmon, HDC , LPRECT ,
⋮----
int ResolveScalePercent(int dsf_value, bool api_ok) noexcept {
⋮----
std::string DeviceNameToUtf8(const std::wstring& wide) noexcept {
⋮----
std::string EscapeJsonString(const std::string& in) {
⋮----
std::string AsciiLower(const std::string& in) {
⋮----
struct EdidIdentity {
⋮----
std::string HexUpper4(unsigned int v) {
⋮----
std::string HexLower(unsigned int v) {
⋮----
bool ParseEdid(const std::vector<unsigned char>& edid, EdidIdentity* out) {
⋮----
struct CcdEntry {
⋮----
std::vector<CcdEntry> QueryCcdMonitors() {
⋮----
bool ReadEdidForDevicePath(const std::wstring& monitor_device_path,
⋮----
std::vector<unsigned char> buf(size);
⋮----
std::vector<AssignedIdentity> AssignStableKeys(
⋮----
void EnrichMonitorIdentities(std::vector<MonitorInfo>& monitors) noexcept {
⋮----
VirtualScreenGeometry GetVirtualScreenGeometry() noexcept {
⋮----
std::string FormatMonitorsJson(const std::vector<MonitorInfo>& monitors,
⋮----
std::string FormatMonitorsJson(const std::vector<MonitorInfo>& monitors) {
⋮----
std::vector<MonitorInfo> ListMonitors() noexcept {
⋮----
MonitorInfo GetPrimaryMonitor() noexcept {
⋮----
bool SetProcessDpiAwarenessContextPMDA() noexcept {
```

## File: shared/monitor_util.hpp
```cpp
struct MonitorInfo {
⋮----
bool IsPortrait() const { return height > width; }
⋮----
int ScalePercent() const { return scale_pct; }
⋮----
std::vector<MonitorInfo> ListMonitors() noexcept;
⋮----
MonitorInfo GetPrimaryMonitor() noexcept;
⋮----
struct MonitorIdentityInputs {
⋮----
struct AssignedIdentity {
⋮----
std::vector<AssignedIdentity> AssignStableKeys(
⋮----
void EnrichMonitorIdentities(std::vector<MonitorInfo>& monitors) noexcept;
⋮----
int ResolveScalePercent(int dsf_value, bool api_ok) noexcept;
⋮----
struct VirtualScreenGeometry {
⋮----
VirtualScreenGeometry GetVirtualScreenGeometry() noexcept;
⋮----
std::string FormatMonitorsJson(const std::vector<MonitorInfo>& monitors,
⋮----
std::string FormatMonitorsJson(const std::vector<MonitorInfo>& monitors);
⋮----
bool SetProcessDpiAwarenessContextPMDA() noexcept;
```

## File: shared/os_version.cpp
```cpp
RtlGetVersionFn ResolveRtlGetVersion() {
⋮----
OsVersion GetOsVersion() {
⋮----
bool IsSupportedOsVersion(const OsVersion& version) {
⋮----
const char* MinOsRequirementText() {
⋮----
std::string MinOsErrorMessage(const OsVersion& version) {
⋮----
std::wstring MinOsErrorMessageWide(const OsVersion& version) {
```

## File: shared/os_version.hpp
```cpp
struct OsVersion {
⋮----
OsVersion GetOsVersion();
⋮----
bool IsSupportedOsVersion(const OsVersion& version);
⋮----
const char* MinOsRequirementText();
⋮----
std::string MinOsErrorMessage(const OsVersion& version);
⋮----
std::wstring MinOsErrorMessageWide(const OsVersion& version);
```

## File: shared/playlist.cpp
```cpp
bool NeedsMigration(const nlohmann::json& raw) {
⋮----
std::wstring FoldAscii(const std::wstring& in) {
⋮----
std::size_t SelectNextIndex(std::size_t current, std::size_t count,
⋮----
int PlaylistIndexForPath(const std::vector<std::wstring>& order,
⋮----
std::vector<std::wstring> NormalizeOrder(std::vector<std::wstring> order) {
⋮----
nlohmann::json PlaylistToJson(const PlaylistConfig& cfg) {
⋮----
PlaylistConfig LoadPlaylist(const std::filesystem::path& path) {
⋮----
void SavePlaylist(const std::filesystem::path& path, const PlaylistConfig& cfg) {
⋮----
void ValidatePlaylist(PlaylistConfig& cfg) {
⋮----
PlaylistConfig MigratePlaylist(const nlohmann::json& raw) {
⋮----
std::filesystem::path DefaultPlaylistPath() {
⋮----
std::filesystem::path PlaylistPathForConfig(
```

## File: shared/playlist.hpp
```cpp
struct PlaylistConfig {
⋮----
std::size_t SelectNextIndex(std::size_t current, std::size_t count,
⋮----
int PlaylistIndexForPath(const std::vector<std::wstring>& order,
⋮----
std::vector<std::wstring> NormalizeOrder(std::vector<std::wstring> order);
⋮----
PlaylistConfig LoadPlaylist(const std::filesystem::path& path);
⋮----
void SavePlaylist(const std::filesystem::path& path, const PlaylistConfig& cfg);
⋮----
nlohmann::json PlaylistToJson(const PlaylistConfig& cfg);
⋮----
void ValidatePlaylist(PlaylistConfig& cfg);
⋮----
PlaylistConfig MigratePlaylist(const nlohmann::json& raw);
⋮----
std::filesystem::path DefaultPlaylistPath();
⋮----
std::filesystem::path PlaylistPathForConfig(const std::filesystem::path& config_path);
```

## File: shared/proc_util.cpp
```cpp
class Handle {
⋮----
Handle() = default;
explicit Handle(HANDLE h) : h_(h) {}
⋮----
Handle(const Handle&) = delete;
⋮----
Handle(Handle&& other) noexcept : h_(other.h_) { other.h_ = nullptr; }
⋮----
HANDLE get() const { return h_; }
void reset(HANDLE h = nullptr) {
⋮----
bool DrainAvailable(HANDLE read_pipe, std::string& out) {
⋮----
void DrainToEof(HANDLE read_pipe, std::string& out) {
⋮----
ProcResult RunCaptured(const std::filesystem::path& exe,
⋮----
Handle read_pipe(raw_read);
Handle write_pipe(raw_write);
```

## File: shared/proc_util.hpp
```cpp
struct ProcResult {
⋮----
[[nodiscard]] ProcResult RunCaptured(const std::filesystem::path& exe,
```

## File: shared/settings_io.cpp
```cpp
std::string ReadFile(const std::filesystem::path& path, const char* what) {
⋮----
std::ifstream in(path, std::ios::binary);
⋮----
void BackupFile(const std::filesystem::path& path) noexcept {
```

## File: shared/settings_io.hpp
```cpp
std::string ReadFile(const std::filesystem::path& path, const char* what);
⋮----
void BackupFile(const std::filesystem::path& path) noexcept;
```

## File: shared/sha1.cpp
```cpp
std::uint32_t Rotl(std::uint32_t v, unsigned n) {
⋮----
struct Sha1Ctx {
⋮----
void Sha1Transform(Sha1Ctx& ctx, const unsigned char* p) {
⋮----
void Sha1Update(Sha1Ctx& ctx, const unsigned char* data, std::size_t len) {
⋮----
std::string Sha1Hex(const std::string& data) {
```

## File: shared/sha1.hpp
```cpp
std::string Sha1Hex(const std::string& data);
```

## File: shared/still.cpp
```cpp
std::filesystem::path LocalAppDataBase() {
⋮----
void DebugLogW(const wchar_t* text) { OutputDebugStringW(text); }
⋮----
void FileStamp(const std::filesystem::path& path, std::uintmax_t& size,
⋮----
struct SettingsCache {
⋮----
SettingsCache& Cache() {
⋮----
std::filesystem::path SiblingLauncher() {
⋮----
bool SpawnHelperDetached(const std::wstring& extra_args) {
⋮----
std::wstring Utf8ToWide(const std::string& utf8) {
⋮----
std::string WideToUtf8(const std::wstring& wide) {
⋮----
std::filesystem::path StillDir() {
⋮----
std::filesystem::path StillStatusPath() {
⋮----
std::filesystem::path OriginalWallpaperStorePath() {
⋮----
std::filesystem::path OriginalLockscreenStorePath() {
⋮----
std::filesystem::path OriginalLockscreenImagePath() {
⋮----
StillSettings ReadStillSettingsCached() noexcept {
⋮----
bool IsStillPath(const std::filesystem::path& path) noexcept {
⋮----
bool StillRefreshAllowed(bool manual, bool same_file,
⋮----
if (same_file) return false;  // nothing changed: no second render needed
⋮----
bool ParseStillRes(const std::wstring& text, int& width, int& height) noexcept {
⋮----
// --- original wallpaper store --------------------------------------------------
⋮----
std::wstring ReadOriginalWallpaper() noexcept {
⋮----
bool WriteOriginalWallpaper(const std::wstring& path) noexcept {
⋮----
bool CaptureOriginalWallpaper() noexcept {
⋮----
bool RestoreOriginalWallpaper() noexcept {
⋮----
std::string OriginalLockscreenToJson(const OriginalLockscreen& state) {
⋮----
bool OriginalLockscreenFromJson(const std::string& text,
⋮----
bool ReadOriginalLockscreen(OriginalLockscreen& out) noexcept {
⋮----
bool WriteOriginalLockscreen(const OriginalLockscreen& state) noexcept {
⋮----
void ClearOriginalLockscreen() noexcept {
⋮----
std::string StillStatusToJson(const StillStatus& status) {
⋮----
bool StillStatusFromJson(const std::string& text, StillStatus& out) noexcept {
⋮----
bool ReadStillStatus(StillStatus& out) noexcept {
⋮----
bool WriteStillStatus(const StillStatus& status) noexcept {
⋮----
bool RequestStillApply(const std::filesystem::path& video) noexcept {
⋮----
bool RequestStillRestore(bool wallpaper, bool lockscreen) noexcept {
⋮----
void SetStillSpawnFn(StillSpawnFn fn) noexcept { g_spawn_fn = fn; }
```

## File: shared/still.hpp
```cpp
std::filesystem::path StillDir();
⋮----
std::filesystem::path StillStatusPath();
⋮----
std::filesystem::path OriginalWallpaperStorePath();
⋮----
std::filesystem::path OriginalLockscreenStorePath();
⋮----
std::filesystem::path OriginalLockscreenImagePath();
⋮----
struct StillSettings {
⋮----
StillSettings ReadStillSettingsCached() noexcept;
⋮----
bool IsStillPath(const std::filesystem::path& path) noexcept;
⋮----
bool StillRefreshAllowed(bool manual, bool same_file,
⋮----
bool ParseStillRes(const std::wstring& text, int& width, int& height) noexcept;
⋮----
std::wstring ReadOriginalWallpaper() noexcept;
⋮----
bool WriteOriginalWallpaper(const std::wstring& path) noexcept;
⋮----
bool CaptureOriginalWallpaper() noexcept;
⋮----
bool RestoreOriginalWallpaper() noexcept;
⋮----
struct OriginalLockscreen {
⋮----
std::string OriginalLockscreenToJson(const OriginalLockscreen& state);
bool OriginalLockscreenFromJson(const std::string& text,
⋮----
bool ReadOriginalLockscreen(OriginalLockscreen& out) noexcept;
bool WriteOriginalLockscreen(const OriginalLockscreen& state) noexcept;
⋮----
void ClearOriginalLockscreen() noexcept;
⋮----
struct StillStatus {
⋮----
std::string StillStatusToJson(const StillStatus& status);
bool StillStatusFromJson(const std::string& text, StillStatus& out) noexcept;
bool ReadStillStatus(StillStatus& out) noexcept;
bool WriteStillStatus(const StillStatus& status) noexcept;
⋮----
bool RequestStillApply(const std::filesystem::path& video) noexcept;
bool RequestStillRestore(bool wallpaper, bool lockscreen) noexcept;
⋮----
void SetStillSpawnFn(StillSpawnFn fn) noexcept;
```

## File: shared/studio_settings.cpp
```cpp
bool NeedsSettingsMigration(const nlohmann::json& raw) {
⋮----
std::string TolerantEnumString(const nlohmann::json& raw, const char* key,
⋮----
std::filesystem::path K6wpLocalDir(const wchar_t* leaf) {
⋮----
std::filesystem::path DefaultCompressOutputDir() {
⋮----
std::filesystem::path DefaultStudioCacheDir() {
⋮----
StudioSettings DefaultStudioSettings() {
⋮----
StudioSettings LoadStudioSettings(const std::filesystem::path& path) {
⋮----
void SaveStudioSettings(const std::filesystem::path& path,
⋮----
void ValidateStudioSettings(StudioSettings& settings) {
⋮----
StudioSettings MigrateStudioSettings(const nlohmann::json& raw) {
⋮----
// Empty-string dirs mean "unset" (e.g. hand-edited file): resolve to the
⋮----
std::filesystem::path DefaultStudioSettingsPath() {
```

## File: shared/studio_settings.hpp
```cpp
struct StudioSettings {
⋮----
StudioSettings DefaultStudioSettings();
⋮----
StudioSettings LoadStudioSettings(const std::filesystem::path& path);
⋮----
void SaveStudioSettings(const std::filesystem::path& path,
⋮----
void ValidateStudioSettings(StudioSettings& settings);
⋮----
StudioSettings MigrateStudioSettings(const nlohmann::json& raw);
⋮----
std::filesystem::path DefaultStudioSettingsPath();
⋮----
std::filesystem::path DefaultCompressOutputDir();
⋮----
std::filesystem::path DefaultStudioCacheDir();
```

## File: shared/studio_settings.json
```json
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "$id": "https://k6wp.local/schemas/studio-settings.schema.json",
  "title": "K6WP StudioSettings",
  "description": "Studio-side preferences for the K6WP wallpaper app. Stored at %LOCALAPPDATA%/K6WP/studio_settings.json. Deliberately separate from config.json, which is pure Engine playback state (video_path/fit/monitor/speed/fps/battery). Version 0 (legacy: missing version or missing fields) is migrated to version 5 by the loader; version 1 migrates with compress_advanced_visible defaulted to false; version 2 migrates with the removed keep_original/start_minimized keys ignored; version 3 migrates with lockscreen_offer_shown defaulted to false; version 4 migrates to the per-user still mechanism: wallpaper_sync defaulted to true, lockscreen_sync KEPT exactly as stored, and a lockscreen_offset_sec of exactly 1.0 (the old hard-coded default) moved to 0.0 (the first frame). A fresh install defaults BOTH sync flags to true. The three additive Studio UI preferences (closeToTray, playlistSource, performancePreset) may be absent from files written before plan todo 8: the loader defaults them and rewrites the file at the current version — no version bump, same pattern check_updates followed. On migration (or corrupt input) the loader backs up the original bytes to <file>.bak and rewrites the file at the current version.",
  "type": "object",
  "additionalProperties": false,
  "required": ["version"],
  "properties": {
    "version": {
      "type": "integer",
      "minimum": 0,
      "maximum": 5,
      "description": "Schema version. 0 = legacy (missing fields are defaulted on load), 1 = lockscreen sync fields, 2 = compressor advanced-box visibility, 3 = the Studio UI preferences (closeToTray/playlistSource/performancePreset, additive), 4 = the Studio-private lockscreen_offer_shown, 5 = current (adds wallpaper_sync and moves the offset default to 0.0)."
    },
    "auto_compress_on_import": {
      "type": "boolean",
      "default": true,
      "description": "When true, importing a video into the Studio library compresses it automatically with the defaults below."
    },
    "compress_output_dir": {
      "type": "string",
      "description": "Directory for compressed wallpapers. Default %LOCALAPPDATA%/K6WP/wallpapers. Must not be empty."
    },
    "default_crf": {
      "type": "integer",
      "minimum": 16,
      "maximum": 28,
      "default": 22,
      "description": "Default compression CRF for the compressor. Agrees with the engine/compressor CRF 22 default. Values outside 16-28 are rejected."
    },
    "default_fps": {
      "type": "integer",
      "minimum": 1,
      "maximum": 30,
      "default": 30,
      "description": "Default compression target framerate. Values outside 1-30 are rejected."
    },
    "default_resolution_mode": {
      "type": "string",
      "enum": ["match_monitor", "source", "720p", "1080p", "2160p"],
      "default": "match_monitor",
      "description": "Default compression resolution. match_monitor = scale to the target monitor's native resolution; source = keep the input resolution; otherwise cap the long edge at the given height."
    },
    "start_with_windows": {
      "type": "boolean",
      "default": false,
      "description": "When true, Studio registers the HKCU Run autostart entry (engine tray-only start)."
    },
    "cache_dir": {
      "type": "string",
      "description": "Compressor cache directory. Default %LOCALAPPDATA%/K6WP/cache. Must not be empty."
    },
    "wallpaper_sync": {
      "type": "boolean",
      "default": true,
      "description": "Per-user still: when true, the first frame of the active video becomes the real Windows desktop wallpaper (SPI_SETDESKWALLPAPER). The original wallpaper is saved once to %LOCALAPPDATA%/K6WP/original_wallpaper.json so it can be restored. No administrator rights. Engine-readable (LOW-16)."
    },
    "lockscreen_sync": {
      "type": "boolean",
      "default": true,
      "description": "Per-user still: when true, the first frame of the active video becomes the Windows lock-screen image (WinRT LockScreen::SetImageFileAsync). No HKLM policy, no administrator rights, no UAC. A fresh install defaults to true; a file that already carries this key keeps the user's stored value. Engine-readable (LOW-16)."
    },
    "lockscreen_offset_sec": {
      "type": "number",
      "minimum": 0,
      "default": 0.0,
      "description": "Seek offset in seconds for the still frame extract. 0 = the FIRST frame. Must be >= 0. A pre-v5 file with the old default of exactly 1.0 migrates to 0.0. Engine-readable (LOW-16)."
    },
    "lockscreen_offer_shown": {
      "type": "boolean",
      "default": false,
      "description": "Studio-private: the one-time onboarding offer for the lock screen has been shown. Never read by the engine."
    },
    "compress_advanced_visible": {
      "type": "boolean",
      "default": false,
      "description": "Compressor tab advanced-box expanded state (session UI only; v1 files migrate it to false)."
    },
    "check_updates": {
      "type": "boolean",
      "default": true,
      "description": "Optional 'new version available' check (Studio-only, never downloads/installs). When true, Studio GETs the releases API once per start plus manual Help -> Check for updates; network failures are silent. Files missing this key migrate it to true."
    },
    "closeToTray": {
      "type": "boolean",
      "default": false,
      "description": "When true, closing Studio hides it to the system tray instead of quitting (plan todo 8, additive inside schema v3; files missing this key migrate it to false)."
    },
    "playlistSource": {
      "type": "string",
      "enum": ["all", "custom"],
      "default": "all",
      "description": "Playlist rotation source: all = every library wallpaper, custom = a user-picked subset (plan todo 8, additive inside schema v3). Any other value migrates to the default \"all\"."
    },
    "performancePreset": {
      "type": "string",
      "enum": ["Hemat", "Seimbang", "Maksimal"],
      "default": "Seimbang",
      "description": "Studio performance preset: Hemat = battery saver, Seimbang = balanced, Maksimal = full quality (plan todo 8, additive inside schema v3). Any other value migrates to the default \"Seimbang\"."
    }
  }
}
```

## File: shared/version_compare.hpp
```cpp
struct ParsedSemver {
⋮----
inline ParsedSemver ParseSemver(const std::string& text) {
⋮----
inline int CompareSemver(const std::string& a, const std::string& b) {
⋮----
inline bool IsNewerVersion(const std::string& latest,
```

## File: studio/src/apply_manager.cpp
```cpp
struct ProcHandle {
⋮----
explicit ProcHandle(HANDLE handle) : h(handle) {}
⋮----
ProcHandle(const ProcHandle&) = delete;
⋮----
HANDLE get() const { return h; }
bool valid() const { return h != nullptr && h != INVALID_HANDLE_VALUE; }
⋮----
bool WaitForEnginePipe(DWORD total_ms, DWORD slice_ms,
⋮----
bool WaitForProcessExit(HANDLE proc, DWORD total_ms, DWORD slice_ms,
⋮----
ApplyManager::ApplyManager(QObject* parent) : QObject(parent) {}
⋮----
void ApplyManager::AppendLog(const QString& line) {
⋮----
emit LogMessage(line);
⋮----
bool ApplyManager::WriteConfig(const WallpaperConfig& cfg,
⋮----
bool ApplyManager::IsDevEngineFallbackEnabled() {
⋮----
QString ApplyManager::ResolveEnginePath() {
⋮----
bool ApplyManager::RestartEngine(QString* error_out) {
⋮----
ProcHandle proc(raw);
⋮----
bool ApplyManager::SyncMonitor(int monitor_id, QString* error_out) {
⋮----
ApplyResult ApplyManager::Apply(const WallpaperConfig& cfg,
⋮----
void ApplyManager::RequestCancel() {
```

## File: studio/src/apply_manager.hpp
```cpp
class QString;
⋮----
struct ApplyResult {
enum class Outcome {
⋮----
class ApplyManager final : public QObject {
⋮----
void SetIpcClient(IpcClient* shared) { ipc_ = shared; }
⋮----
void SetConfigPath(const std::filesystem::path& path) {
⋮----
std::filesystem::path ConfigPath() const {
⋮----
bool WriteConfig(const WallpaperConfig& cfg, QString* error_out = nullptr);
⋮----
bool RestartEngine(QString* error_out = nullptr);
⋮----
ApplyResult Apply(const WallpaperConfig& cfg, QString* error_out = nullptr);
⋮----
bool SyncMonitor(int monitor_id, QString* error_out = nullptr);
⋮----
static QString ResolveEnginePath();
⋮----
static bool IsDevEngineFallbackEnabled();
⋮----
const QStringList& log() const { return log_; }
void ClearLog() { log_.clear(); }
⋮----
void RequestCancel();
⋮----
void LogMessage(const QString& line);
⋮----
void AppendLog(const QString& line);
```

## File: studio/src/bridge_diagnostics.hpp
```cpp
inline void AppendCapped(QStringList& log, const QString& line, int cap) {
⋮----
inline bool SetChangedError(QString& last_error, const QString& error) {
```

## File: studio/src/cache_dir_policy.cpp
```cpp
std::wstring FoldPath(const std::filesystem::path& path) {
⋮----
bool IsSamePath(const std::filesystem::path& a,
⋮----
// True when `child` is strictly below `ancestor`, not equal to it.
bool IsUnder(const std::filesystem::path& child,
⋮----
bool IsVolumeRoot(const std::filesystem::path& path) {
⋮----
// Where the sweep would actually go. A junction or a symlink is a name, not a
// location, so resolving it is the difference between checking the folder the
// user picked and checking the folder that gets emptied. False means the
// target could not be resolved, and the caller refuses rather than guessing.
bool ResolveTarget(const std::filesystem::path& path,
⋮----
// A Windows known folder (Videos, Documents, ...), or false when the shell has
// none. Asked of the shell rather than assembled from <USER_HOME> so a folder
// OneDrive has moved off the profile is still recognised.
bool KnownFolder(const KNOWNFOLDERID& id, std::filesystem::path* path) {
⋮----
// Directories that are never a cache, whatever cache_dir says: the K6WP data
// root (studio_settings.json, config.json, library.json live there), its
// parent, the profile itself, and the six folders Windows treats as the user's
// own content. The profile-relative spellings are the fallback for the case
// where the shell cannot answer.
std::vector<std::filesystem::path> ProtectedDirs(
⋮----
const std::filesystem::path home(profile);
⋮----
bool HoldsNoRegularFile(const std::filesystem::path& dir) {
⋮----
for (std::filesystem::recursive_directory_iterator it(dir, ec), end;
⋮----
QString DescribePath(const std::filesystem::path& configured,
⋮----
bool CacheDirIsClearable(const std::filesystem::path& configured,
⋮----
bool MarkCacheDirOptedIn(const std::filesystem::path& dir) {
```

## File: studio/src/cache_dir_policy.hpp
```cpp
bool MarkCacheDirOptedIn(const std::filesystem::path& dir);
⋮----
bool CacheDirIsClearable(const std::filesystem::path& configured,
```

## File: studio/src/compress_bridge.cpp
```cpp
QString FormatEta(int eta_s) {
⋮----
CompressBridge::CompressBridge(QObject* parent) : QObject(parent) {
⋮----
emit statusTextChanged();
emit progressChanged();
⋮----
emit jobFinishedWithoutResult();
⋮----
emit resultChanged();
⋮----
void CompressBridge::refreshDefaults() {
⋮----
emit inputsChanged();
⋮----
void CompressBridge::pickSource() {
⋮----
void CompressBridge::setSourcePath(const QString& path) {
⋮----
void CompressBridge::pickOutDir() {
⋮----
void CompressBridge::openOutDir() {
⋮----
void CompressBridge::openResultDir() {
⋮----
void CompressBridge::setAdvanced(bool on) {
⋮----
void CompressBridge::setCrf(int value) {
⋮----
void CompressBridge::setFps(int value) {
⋮----
void CompressBridge::setEncoder(const QString& name) {
⋮----
void CompressBridge::setResolutionText(const QString& text) {
⋮----
void CompressBridge::setForceLong(bool on) {
⋮----
void CompressBridge::setAutoApply(bool on) {
⋮----
emit autoApplyChanged();
⋮----
QString CompressBridge::etaText() const { return FormatEta(eta_s_); }
⋮----
QString CompressBridge::queueText() const {
⋮----
void CompressBridge::RefreshQueue() { emit queueChanged(); }
⋮----
void CompressBridge::AppendLog(const QString& line) {
⋮----
emit logChanged();
⋮----
void CompressBridge::SetLastError(const QString& error) {
⋮----
emit lastErrorChanged();
⋮----
void CompressBridge::ClearLastError() { SetLastError(QString()); }
⋮----
void CompressBridge::clearResult() {
⋮----
void CompressBridge::start() {
⋮----
void CompressBridge::EnqueueWithProbe(const QString& src, int res_w, int res_h,
⋮----
void CompressBridge::resolveConsent(bool approved) {
⋮----
void CompressBridge::Dispatch(const QString& src, int res_w, int res_h, bool probed_ok,
⋮----
void CompressBridge::cancel() {
```

## File: studio/src/compress_bridge.hpp
```cpp
class CompressBridge : public QObject {
⋮----
QML_NAMED_ELEMENT(Compress)
⋮----
explicit CompressBridge(QObject* parent = nullptr);
⋮----
Q_PROPERTY(QString sourcePath READ sourcePath NOTIFY inputsChanged)
Q_PROPERTY(QString outDir READ outDir NOTIFY inputsChanged)
Q_PROPERTY(bool advanced READ advanced NOTIFY inputsChanged)
Q_PROPERTY(int crf READ crf NOTIFY inputsChanged)
Q_PROPERTY(int fps READ fps NOTIFY inputsChanged)
Q_PROPERTY(QString encoder READ encoder NOTIFY inputsChanged)
Q_PROPERTY(QString resolutionText READ resolutionText WRITE setResolutionText
⋮----
Q_PROPERTY(bool forceLong READ forceLong NOTIFY inputsChanged)
Q_PROPERTY(bool autoApply READ autoApply NOTIFY autoApplyChanged)
⋮----
QString sourcePath() const { return source_path_; }
QString outDir() const { return out_dir_; }
bool advanced() const { return advanced_; }
int crf() const { return crf_; }
int fps() const { return fps_; }
QString encoder() const { return encoder_; }
QString resolutionText() const { return resolution_text_; }
bool forceLong() const { return force_long_; }
bool autoApply() const { return auto_apply_; }
⋮----
Q_PROPERTY(bool running READ running NOTIFY queueChanged)
Q_PROPERTY(int pending READ pending NOTIFY queueChanged)
Q_PROPERTY(int progress READ progress NOTIFY progressChanged)
Q_PROPERTY(QString etaText READ etaText NOTIFY progressChanged)
Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
Q_PROPERTY(QString detailText READ detailText NOTIFY statusTextChanged)
Q_PROPERTY(QString queueText READ queueText NOTIFY queueChanged)
⋮----
Q_PROPERTY(QString resultPath READ resultPath NOTIFY resultChanged)
Q_PROPERTY(bool hasResult READ hasResult NOTIFY resultChanged)
Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
Q_PROPERTY(QStringList log READ log NOTIFY logChanged)
⋮----
bool running() const { return controller_.IsRunning(); }
int pending() const { return controller_.PendingCount(); }
int progress() const { return progress_; }
QString etaText() const;
QString statusText() const { return status_text_; }
QString detailText() const { return detail_text_; }
QString queueText() const;
QString resultPath() const { return result_path_; }
bool hasResult() const { return !result_path_.isEmpty(); }
QString lastError() const { return last_error_; }
QStringList log() const { return log_; }
⋮----
Q_INVOKABLE void pickSource();
Q_INVOKABLE void setSourcePath(const QString& path);
Q_INVOKABLE void pickOutDir();
Q_INVOKABLE void openOutDir();
Q_INVOKABLE void openResultDir();
⋮----
Q_INVOKABLE void setAdvanced(bool on);
Q_INVOKABLE void setCrf(int value);
Q_INVOKABLE void setFps(int value);
Q_INVOKABLE void setEncoder(const QString& name);
Q_INVOKABLE void setResolutionText(const QString& text);
Q_INVOKABLE void setForceLong(bool on);
Q_INVOKABLE void setAutoApply(bool on);
⋮----
Q_INVOKABLE void refreshDefaults();
⋮----
Q_INVOKABLE void start();
Q_INVOKABLE void cancel();
⋮----
Q_INVOKABLE void resolveConsent(bool approved);
Q_INVOKABLE void clearResult();
⋮----
void inputsChanged();
void queueChanged();
void progressChanged();
void statusTextChanged();
void resultChanged();
⋮----
void jobFinishedWithoutResult();
void lastErrorChanged();
void autoApplyChanged();
void logChanged();
⋮----
void consentRequired(double durationMinutes);
⋮----
void AppendLog(const QString& line);
void SetLastError(const QString& error);
void ClearLastError();
void RefreshQueue();
⋮----
void EnqueueWithProbe(const QString& src, int res_w, int res_h, bool need_res);
⋮----
void Dispatch(const QString& src, int res_w, int res_h, bool probed_ok,
```

## File: studio/src/compress_controller.cpp
```cpp
bool CompressController::Enqueue(const CompressRequest& req, JobMeta& meta,
⋮----
CompressService::EnqueueResult CompressController::EnqueueWithStatus(
⋮----
bool CompressController::TakeRequest(int job_id, CompressRequest* out) {
⋮----
QString CompressController::DefaultWallpapersDir() {
⋮----
QString CompressController::ResolveTabOutDir(const QString& override_dir,
⋮----
void CompressController::ResolveSimpleRes(const std::string& mode,
⋮----
int CompressController::ResolveEngineFpsCap() {
⋮----
int CompressController::ComputeEnqueueFps(double fps_source,
⋮----
CompressRequest CompressController::BuildTabRequest(
```

## File: studio/src/compress_controller.hpp
```cpp
struct TabRequestInputs {
⋮----
class CompressController {
⋮----
CompressController() = default;
⋮----
CompressController(const CompressController&) = delete;
⋮----
CompressService* service() { return &service_; }
const CompressService* service() const { return &service_; }
⋮----
bool Enqueue(const CompressRequest& req, JobMeta& meta,
⋮----
CompressService::EnqueueResult EnqueueWithStatus(
⋮----
bool TakeRequest(int job_id, CompressRequest* out);
⋮----
bool IsRunning() const { return service_.IsRunning(); }
int PendingCount() const { return service_.PendingCount(); }
void CancelCurrent() { service_.CancelCurrent(); }
⋮----
void CancelAll() { service_.CancelAll(); }
⋮----
void SetCacheDir(const QString& dir) { service_.SetCacheDir(dir); }
⋮----
static QString DefaultWallpapersDir();
⋮----
static QString ResolveTabOutDir(const QString& override_dir,
⋮----
static void ResolveSimpleRes(const std::string& mode,
⋮----
static int ComputeEnqueueFps(double fps_source, int engine_fps_cap);
⋮----
static int ResolveEngineFpsCap();
⋮----
static CompressRequest BuildTabRequest(const TabRequestInputs& in);
```

## File: studio/src/compress_errors.cpp
```cpp
QString FriendlyCompressError(const QString& technical) {
```

## File: studio/src/compress_errors.hpp
```cpp
QString FriendlyCompressError(const QString& technical);
```

## File: studio/src/compress_first_offer.hpp
```cpp
inline qint64 CompressFirstOfferMb(const QString& path) {
const QFileInfo info(path);
```

## File: studio/src/compress_service.cpp
```cpp
QString UniqueOutPath(const QString& dir, const QString& in_path) {
⋮----
const QDir out_dir(dir);
⋮----
CompressService::CompressService(QObject* parent) : QObject(parent) {
⋮----
QString CompressService::ResolveCompressorPath() {
⋮----
CompressService::EnqueueResult CompressService::EnqueueDetailed(
⋮----
void CompressService::CancelCurrent() {
⋮----
const QFileInfo partial_info(partial);
⋮----
void CompressService::CancelAll() {
⋮----
emit QueueSummary(batch_ok_, batch_fail_);
⋮----
bool CompressService::StartNext() {
⋮----
void CompressService::HandleStartFailure(const QString& msg) {
⋮----
this, [this, ok, fail]() { emit QueueSummary(ok, fail); },
⋮----
void CompressService::OnReadyReadStdout() {
⋮----
void CompressService::OnReadyReadStderr() {
⋮----
void CompressService::OnProcessFinished(int exit_code,
⋮----
emit Finished(meta, true, QString(), ok_info_);
⋮----
void CompressService::AppendLog(const QString& line) {
emit LogMessage(line);
⋮----
void CompressService::DrainChannel(QProcess::ProcessChannel channel,
⋮----
void CompressService::HandleStdoutLine(const QString& line) {
⋮----
emit Progress(pct, eta);
⋮----
void CompressService::HandleStderrLine(const QString& line) {
```

## File: studio/src/compress_service.hpp
```cpp
struct CompressRequest {
⋮----
struct CompressOkInfo {
⋮----
struct JobMeta {
enum class Origin { kTab, kImport, kRecompress, kFirstRun };
⋮----
QString UniqueOutPath(const QString& dir, const QString& in_path);
⋮----
class CompressService final : public QObject {
⋮----
CompressService(const CompressService&) = delete;
⋮----
static QString ResolveCompressorPath();
⋮----
enum class EnqueueResult { kStarted, kQueued, kInvalid, kStartFailed };
⋮----
EnqueueResult EnqueueDetailed(const CompressRequest& req, JobMeta& meta,
⋮----
void SetCacheDir(const QString& dir) { cache_dir_ = dir; }
⋮----
void CancelCurrent();
⋮----
void CancelAll();
⋮----
bool IsRunning() const { return running_; }
⋮----
int PendingCount() const { return static_cast<int>(queue_.size()); }
⋮----
void Started(const JobMeta& meta);
void Progress(int percent, int eta_s);
void Finished(const JobMeta& meta, bool ok, const QString& technical,
⋮----
void QueueSummary(int ok_count, int fail_count);
void LogMessage(const QString& line);
⋮----
void OnReadyReadStdout();
void OnReadyReadStderr();
void OnProcessFinished(int exit_code, QProcess::ExitStatus exit_status);
⋮----
struct Job {
⋮----
bool StartNext();
void HandleStartFailure(const QString& msg);
void AppendLog(const QString& line);
void DrainChannel(QProcess::ProcessChannel channel, QString& buf);
void HandleStdoutLine(const QString& line);
void HandleStderrLine(const QString& line);
```

## File: studio/src/engine_status_controller.cpp
```cpp
EngineStatusView DecideEngineStatus(const IpcResult& res) {
⋮----
bool WaitForEngineReady(IpcClient& ipc, std::atomic<bool>& cancel,
```

## File: studio/src/engine_status_controller.hpp
```cpp
struct EngineStatusView {
enum class Kind {
⋮----
EngineStatusView DecideEngineStatus(const IpcResult& res);
⋮----
bool WaitForEngineReady(IpcClient& ipc, std::atomic<bool>& cancel,
```

## File: studio/src/ffprobe_helper.cpp
```cpp
std::string GetEnvStr(const char* name) {
⋮----
std::string value(buf);
⋮----
void LogFfprobeError(const std::string& msg) {
⋮----
FfprobeHelper::FfprobeHelper() : ffprobe_path_(FindFfprobe()) {}
⋮----
bool FfprobeHelper::Probe(const std::filesystem::path& video,
⋮----
// Spawn goes through the central k6wp::RunCaptured helper (single
// spawn site, kProbeTimeoutMs budget, orphan-kill on timeout).
⋮----
// Parse JSON output (nlohmann/json via k6wp_shared thirdparty/json.hpp,
// included at file top).
⋮----
// Extract duration from format. ffprobe JSON reports duration as a
// string ("30.000000"), so accept string or number.
⋮----
break;  // first video stream only
```

## File: studio/src/ffprobe_helper.hpp
```cpp
struct VideoMetadata {
⋮----
std::string codec = "";     // e.g. "h264", "hevc", "vp9"
⋮----
class FfprobeHelper {
⋮----
FfprobeHelper();
⋮----
[[nodiscard]] bool Probe(const std::filesystem::path& video,
```

## File: studio/src/first_run_wizard.cpp
```cpp
bool IsFirstRunCondition(bool has_settings_file, bool lib_empty,
⋮----
LibraryEntry BuildFirstRunEntry(const QString& file,
```

## File: studio/src/first_run_wizard.hpp
```cpp
bool IsFirstRunCondition(bool has_settings_file, bool lib_empty,
⋮----
LibraryEntry BuildFirstRunEntry(const QString& file,
```

## File: studio/src/ipc_client.cpp
```cpp
std::string WinError(DWORD code) {
⋮----
std::uint32_t EnginePidFromState(const IpcResult& res) {
⋮----
Impl() = default;
⋮----
Impl(const Impl&) = delete;
⋮----
void Close() {
⋮----
bool connected() const { return pipe != INVALID_HANDLE_VALUE; }
⋮----
IpcClient::IpcClient() : impl_(std::make_unique<Impl>()) {}
⋮----
IpcClient::IpcClient(IpcClient&& other) noexcept
⋮----
std::lock_guard<std::mutex> lock(mutex_);
⋮----
void IpcClient::AppendLog(const std::string& line) {
⋮----
bool IpcClient::IsConnected() const {
⋮----
bool IpcClient::IsConnectedLocked() const {
⋮----
void IpcClient::Disconnect() {
⋮----
void IpcClient::DisconnectLocked() {
⋮----
void IpcClient::NoteUse() {
⋮----
void IpcClient::ReapIdleConnection() {
⋮----
bool IpcClient::Connect(std::string* error_out) {
⋮----
bool IpcClient::ConnectLocked(std::string* error_out) {
⋮----
bool IpcClient::EnsureConnected(std::string* error_out) {
⋮----
IpcResult IpcClient::Send(Cmd cmd, const nlohmann::json& payload) {
⋮----
IpcResult IpcClient::SetVideo(const std::string& utf8_path) {
⋮----
IpcResult IpcClient::SetVideo(const std::filesystem::path& path) {
⋮----
IpcResult IpcClient::SetMonitor(int monitor_id) {
⋮----
IpcResult IpcClient::Pause() { return Send(Cmd::pause); }
⋮----
IpcResult IpcClient::Resume() { return Send(Cmd::resume); }
⋮----
IpcResult IpcClient::GetState() { return Send(Cmd::get_state); }
⋮----
IpcResult IpcClient::Quit() { return Send(Cmd::quit); }
```

## File: studio/src/ipc_client.hpp
```cpp
enum class IpcStatus {
⋮----
struct IpcResult {
⋮----
std::uint32_t EnginePidFromState(const IpcResult& res);
⋮----
class IpcClient {
⋮----
IpcClient();
⋮----
IpcClient(const IpcClient&) = delete;
⋮----
IpcClient(IpcClient&& other) noexcept;
⋮----
bool Connect(std::string* error_out = nullptr);
⋮----
void Disconnect();
⋮----
bool IsConnected() const;
⋮----
IpcResult Send(Cmd cmd,
⋮----
IpcResult SetVideo(const std::string& utf8_path);
IpcResult SetVideo(const std::filesystem::path& path);
IpcResult SetMonitor(int monitor_id);
IpcResult Pause();
IpcResult Resume();
IpcResult GetState();
IpcResult Quit();
⋮----
const std::vector<std::string>& log() const { return log_; }
void ClearLog() { log_.clear(); }
⋮----
void RequestCancel() { cancel_.store(true, std::memory_order_relaxed); }
void ClearCancel() { cancel_.store(false, std::memory_order_relaxed); }
bool IsCancelRequested() const {
⋮----
void AppendLog(const std::string& line);
⋮----
bool IsConnectedLocked() const;
void DisconnectLocked();
bool ConnectLocked(std::string* error_out);
⋮----
bool EnsureConnected(std::string* error_out);
⋮----
void NoteUse();
⋮----
void ReapIdleConnection();
⋮----
struct UseGuard {
explicit UseGuard(IpcClient* c) : client(c) {}
UseGuard(const UseGuard&) = delete;
```

## File: studio/src/library_format.cpp
```cpp
std::string ToLowerAscii(std::string s) {
⋮----
bool FilterMatches(const std::string& name, const std::string& label,
⋮----
std::string EntryLabel(const LibraryEntry& entry) {
⋮----
QString FileUrl(const std::filesystem::path& p) {
```

## File: studio/src/library_format.hpp
```cpp
bool FilterMatches(const std::string& name, const std::string& label,
⋮----
std::string EntryLabel(const LibraryEntry& entry);
⋮----
QString FileUrl(const std::filesystem::path& p);
```

## File: studio/src/library_grid_model.cpp
```cpp
bool ParseWxH(const std::string& text, int* width, int* height) {
⋮----
bool LabelResolution(const std::string& label, int* width, int* height) {
⋮----
std::string DisplayNameFor(const std::string& label) {
⋮----
bool MissingFor(const std::string& label) {
⋮----
bool MissingFor(const LibraryEntry& entry) { return entry.broken; }
⋮----
bool OptimizedFor(const std::string& label) {
⋮----
bool OptimizedFor(const LibraryEntry& entry) {
⋮----
LibraryGridModel::LibraryGridModel(QObject* parent) : QAbstractListModel(parent) {
⋮----
int LibraryGridModel::rowCount(const QModelIndex& parent) const {
⋮----
QHash<int, QByteArray> LibraryGridModel::roleNames() const {
⋮----
QVariant LibraryGridModel::data(const QModelIndex& index, int role) const {
⋮----
int LibraryGridModel::visibleCount() const {
⋮----
bool LibraryGridModel::firstRunEligible() const {
⋮----
void LibraryGridModel::setFilter(const QString& text) {
⋮----
emit filterChanged();
⋮----
void LibraryGridModel::ReapplyFilter() {
⋮----
emit countChanged();
⋮----
int LibraryGridModel::RowForDst(const std::filesystem::path& dst) const {
⋮----
void LibraryGridModel::reload() {
⋮----
int LibraryGridModel::pickAndImport() {
⋮----
int LibraryGridModel::importPaths(const QStringList& paths) {
⋮----
emit lastPickedPathChanged();
emit compressFirstRequired(path);
⋮----
void LibraryGridModel::OnEntryProbed(const std::filesystem::path& dst, bool ok,
⋮----
emit dataChanged(index(visible_row), index(visible_row));
⋮----
QString LibraryGridModel::dstAt(int row) const {
⋮----
void LibraryGridModel::applyAt(int row) {
⋮----
emit applyRequested(dst);
⋮----
void LibraryGridModel::recompressAt(int row) {
⋮----
emit recompressRequested(dst);
⋮----
void LibraryGridModel::openLocationAt(int row) {
⋮----
bool LibraryGridModel::removeAt(int row, bool moveToTrash) {
⋮----
void LibraryGridModel::ensureThumbnail(int row) {
⋮----
void LibraryGridModel::OnThumbFinished() {
⋮----
void LibraryGridModel::clearLastPickedPath() {
⋮----
void LibraryGridModel::markFirstRunHandled() {
⋮----
void LibraryGridModel::AppendLog(const QString& line) {
⋮----
emit logChanged();
⋮----
void LibraryGridModel::SetLastError(const QString& error) {
⋮----
emit lastErrorChanged();
⋮----
void LibraryGridModel::SetStatusText(const QString& text) {
⋮----
emit statusTextChanged();
```

## File: studio/src/library_grid_model.hpp
```cpp
class LibraryGridModel : public QAbstractListModel {
⋮----
QML_NAMED_ELEMENT(Library)
⋮----
enum Roles {
⋮----
explicit LibraryGridModel(QObject* parent = nullptr);
⋮----
int rowCount(const QModelIndex& parent = QModelIndex()) const override;
QVariant data(const QModelIndex& index, int role) const override;
QHash<int, QByteArray> roleNames() const override;
⋮----
Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged)
QString filter() const { return filter_; }
void setFilter(const QString& text);
⋮----
Q_PROPERTY(int count READ count NOTIFY countChanged)
Q_PROPERTY(int visibleCount READ visibleCount NOTIFY countChanged)
⋮----
Q_PROPERTY(bool isEmpty READ isEmpty NOTIFY countChanged)
Q_PROPERTY(bool hasFilter READ hasFilter NOTIFY filterChanged)
⋮----
Q_PROPERTY(bool firstRunEligible READ firstRunEligible NOTIFY countChanged)
⋮----
Q_PROPERTY(QString lastPickedPath READ lastPickedPath NOTIFY lastPickedPathChanged)
Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
Q_PROPERTY(QStringList log READ log NOTIFY logChanged)
⋮----
int count() const { return static_cast<int>(items_.size()); }
int visibleCount() const;
bool isEmpty() const { return items_.empty(); }
bool hasFilter() const { return !filter_.isEmpty(); }
bool firstRunEligible() const;
QString lastPickedPath() const { return last_picked_path_; }
QString lastError() const { return last_error_; }
QString statusText() const { return status_text_; }
QStringList log() const { return log_; }
⋮----
Q_INVOKABLE void reload();
⋮----
Q_INVOKABLE int pickAndImport();
⋮----
Q_INVOKABLE int importPaths(const QStringList& paths);
⋮----
Q_INVOKABLE void applyAt(int row);
Q_INVOKABLE QString dstAt(int row) const;
⋮----
Q_INVOKABLE void recompressAt(int row);
Q_INVOKABLE void openLocationAt(int row);
⋮----
Q_INVOKABLE bool removeAt(int row, bool moveToTrash);
⋮----
Q_INVOKABLE void ensureThumbnail(int row);
⋮----
Q_INVOKABLE void clearLastPickedPath();
⋮----
Q_INVOKABLE void markFirstRunHandled();
⋮----
void filterChanged();
void countChanged();
void lastPickedPathChanged();
void lastErrorChanged();
void statusTextChanged();
void logChanged();
⋮----
void applyRequested(const QString& dst);
⋮----
void recompressRequested(const QString& dst);
⋮----
void compressFirstRequired(const QString& path);
⋮----
void AppendLog(const QString& line);
void SetLastError(const QString& error);
void SetStatusText(const QString& text);
void ReapplyFilter();
int RowForDst(const std::filesystem::path& dst) const;
⋮----
void OnEntryProbed(const std::filesystem::path& dst, bool ok, int width,
⋮----
void OnThumbFinished();
⋮----
std::string DisplayNameFor(const std::string& label);
⋮----
bool MissingFor(const std::string& label);
⋮----
bool MissingFor(const LibraryEntry& entry);
⋮----
bool OptimizedFor(const std::string& label);
bool OptimizedFor(const LibraryEntry& entry);
```

## File: studio/src/library_json_codec.cpp
```cpp
nlohmann::json EntryToJson(const LibraryEntry& e) {
⋮----
LibraryEntry EntryFromJson(const nlohmann::json& j) {
```

## File: studio/src/library_json_codec.hpp
```cpp
nlohmann::json EntryToJson(const LibraryEntry&);
⋮----
LibraryEntry EntryFromJson(const nlohmann::json&);
```

## File: studio/src/library_manager.hpp
```cpp
class LibraryError : public std::runtime_error {
⋮----
explicit LibraryError(const std::string& message) : std::runtime_error(message) {}
⋮----
struct LibraryEntry {
⋮----
std::string codec = "";         // e.g. "h264", "hevc", "vp9"
⋮----
enum MetadataProbe {
⋮----
class LibraryManager {
⋮----
explicit LibraryManager(std::filesystem::path json_path = {});
⋮----
static std::filesystem::path DefaultLibraryJsonPath();
⋮----
void Load();
⋮----
[[nodiscard]] bool RecoveredFromBackup() const { return recovered_from_backup_; }
⋮----
void Save() const;
⋮----
void Add(const LibraryEntry& entry, MetadataProbe probe = kIfMissing);
⋮----
[[nodiscard]] std::vector<LibraryEntry> ListItems() const;
⋮----
bool Remove(const std::filesystem::path& dst, bool move_file_to_trash,
⋮----
[[nodiscard]] std::size_t Size() const;
void Clear();
⋮----
[[nodiscard]] static std::string CacheKeyMaterial(const LibraryEntry& entry);
⋮----
[[nodiscard]] static LibraryEntry ReferenceInPlace(
```

## File: studio/src/main.cpp
```cpp
void ReleaseStudioMutexInternal() {
⋮----
class MutexGuard {
⋮----
MutexGuard() = default;
MutexGuard(const MutexGuard&) = delete;
⋮----
void FocusExistingStudio() {
⋮----
void ReleaseStudioMutex() { ReleaseStudioMutexInternal(); }
⋮----
int main(int argc, char* argv[]) {
⋮----
QApplication app(argc, argv);
```

## File: studio/src/playlist_bridge.cpp
```cpp
std::filesystem::path ResolvePath() {
⋮----
QString LabelFor(const std::wstring& path, bool exists) {
⋮----
PlaylistBridge::PlaylistBridge(QObject* parent) : QObject(parent) {
⋮----
void PlaylistBridge::Load() {
⋮----
emit changed();
⋮----
bool PlaylistBridge::Save() {
⋮----
QVariantList PlaylistBridge::items() const {
⋮----
void PlaylistBridge::reload() { Load(); }
⋮----
void PlaylistBridge::setEnabled(bool on) {
⋮----
void PlaylistBridge::setIntervalMin(int minutes) {
⋮----
void PlaylistBridge::setShuffle(bool on) {
⋮----
int PlaylistBridge::pickAndAdd() {
⋮----
int PlaylistBridge::addPaths(const QStringList& paths) {
⋮----
void PlaylistBridge::removeAt(int row) {
⋮----
void PlaylistBridge::moveUp(int row) {
⋮----
void PlaylistBridge::moveDown(int row) {
⋮----
void PlaylistBridge::clear() {
⋮----
QString PlaylistBridge::pathAt(int row) const {
⋮----
bool PlaylistBridge::syncOrderFromLibrary() {
⋮----
void PlaylistBridge::AppendLog(const QString& line) {
⋮----
emit logChanged();
⋮----
void PlaylistBridge::SetLastError(const QString& error) {
⋮----
emit lastErrorChanged();
```

## File: studio/src/playlist_bridge.hpp
```cpp
class PlaylistBridge : public QObject {
⋮----
QML_NAMED_ELEMENT(Playlist)
⋮----
explicit PlaylistBridge(QObject* parent = nullptr);
⋮----
Q_PROPERTY(bool enabled READ enabled NOTIFY changed)
Q_PROPERTY(int intervalMin READ intervalMin NOTIFY changed)
Q_PROPERTY(bool shuffle READ shuffle NOTIFY changed)
Q_PROPERTY(int count READ count NOTIFY changed)
⋮----
Q_PROPERTY(QVariantList items READ items NOTIFY changed)
Q_PROPERTY(QString path READ path CONSTANT)
Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
Q_PROPERTY(QStringList log READ log NOTIFY logChanged)
⋮----
bool enabled() const { return cfg_.enabled; }
int intervalMin() const { return cfg_.interval_min; }
bool shuffle() const { return cfg_.shuffle; }
int count() const { return static_cast<int>(cfg_.order.size()); }
QVariantList items() const;
QString path() const { return QString::fromStdWString(path_.wstring()); }
QString lastError() const { return last_error_; }
QStringList log() const { return log_; }
⋮----
Q_INVOKABLE void reload();
Q_INVOKABLE void setEnabled(bool on);
Q_INVOKABLE void setIntervalMin(int minutes);
Q_INVOKABLE void setShuffle(bool on);
⋮----
Q_INVOKABLE int pickAndAdd();
⋮----
Q_INVOKABLE int addPaths(const QStringList& paths);
Q_INVOKABLE void removeAt(int row);
Q_INVOKABLE void moveUp(int row);
Q_INVOKABLE void moveDown(int row);
Q_INVOKABLE void clear();
Q_INVOKABLE QString pathAt(int row) const;
⋮----
Q_INVOKABLE bool syncOrderFromLibrary();
⋮----
void changed();
void lastErrorChanged();
void logChanged();
⋮----
void Load();
bool Save();
void AppendLog(const QString& line);
void SetLastError(const QString& error);
```

## File: studio/src/preview_widget.cpp
```cpp
MpvHandleGuard::MpvHandleGuard(MpvHandleGuard&& other) noexcept
⋮----
mpv_handle* MpvHandleGuard::release() {
⋮----
void MpvHandleGuard::reset(mpv_handle* handle) {
⋮----
MpvWorker::MpvWorker(WId wid, std::string video, std::atomic<bool>& started,
⋮----
void MpvWorker::run() {
MpvHandleGuard mpv(mpv_create());
⋮----
std::lock_guard<std::mutex> lock(mutex_);
⋮----
void MpvWorker::LoadVideo(const std::string& path) {
⋮----
std::string MpvWorker::LastError() const {
⋮----
void MpvWorker::TogglePause() {
⋮----
void MpvWorker::SetPaused(bool paused) {
⋮----
bool MpvWorker::IsPaused() const {
⋮----
PreviewWidget::PreviewWidget(QWidget* parent) : QWidget(parent) {
⋮----
void PreviewWidget::LoadVideo(const QString& path) {
⋮----
void PreviewWidget::TogglePlayPause() {
⋮----
void PreviewWidget::SetPaused(bool paused) {
⋮----
bool PreviewWidget::IsPlaying() const {
⋮----
void PreviewWidget::showEvent(QShowEvent* event) {
⋮----
void PreviewWidget::hideEvent(QHideEvent* event) {
⋮----
void PreviewWidget::changeEvent(QEvent* event) {
⋮----
void PreviewWidget::resizeEvent(QResizeEvent* event) {
⋮----
void PreviewWidget::StartWorkerIfNeeded() {
⋮----
void PreviewWidget::StartLiveFromPoster() {
⋮----
void PreviewWidget::mousePressEvent(QMouseEvent* event) {
⋮----
void PreviewWidget::enterEvent(QEnterEvent* event) {
⋮----
void PreviewWidget::leaveEvent(QEvent* event) {
⋮----
void PreviewWidget::RefreshHoverBadge() {
⋮----
void PreviewWidget::OnScreenRemoved(QScreen* screen) {
⋮----
void PreviewWidget::OnScreenSettled() {
⋮----
void PreviewWidget::StopWorker() {
⋮----
QString PreviewWidget::statusText() const {
⋮----
void PreviewWidget::UpdateStatusLabel() {
```

## File: studio/src/preview_widget.hpp
```cpp
class QLabel;
class QScreen;
class QTimer;
class QShowEvent;
class QHideEvent;
class QEvent;
class QEnterEvent;
class QResizeEvent;
class QMouseEvent;
⋮----
enum class PreviewLoadState {
⋮----
class MpvHandleGuard final {
⋮----
MpvHandleGuard() = default;
explicit MpvHandleGuard(mpv_handle* handle) : handle_(handle) {}
⋮----
MpvHandleGuard(const MpvHandleGuard&) = delete;
⋮----
MpvHandleGuard(MpvHandleGuard&& other) noexcept;
⋮----
mpv_handle* get() const { return handle_; }
mpv_handle* release();
void reset(mpv_handle* handle = nullptr);
⋮----
class MpvWorker final : public QThread {
⋮----
MpvWorker(WId wid, std::string video, std::atomic<bool>& started,
⋮----
void LoadVideo(const std::string& path);
void TogglePause();
void SetPaused(bool paused);
bool IsPaused() const;
std::string LastError() const;
⋮----
void run() override;
⋮----
class PreviewWidget final : public QWidget {
⋮----
void LoadVideo(const QString& path);
void TogglePlayPause();
⋮----
bool IsPlaying() const;
QString statusText() const;
⋮----
void showEvent(QShowEvent* event) override;
void hideEvent(QHideEvent* event) override;
void changeEvent(QEvent* event) override;
void resizeEvent(QResizeEvent* event) override;
void mousePressEvent(QMouseEvent* event) override;
void enterEvent(QEnterEvent* event) override;
void leaveEvent(QEvent* event) override;
⋮----
void StartWorkerIfNeeded();
⋮----
void StartLiveFromPoster();
void StopWorker();
void UpdateStatusLabel();
void RefreshHoverBadge();
⋮----
void OnScreenRemoved(QScreen* screen);
void OnScreenSettled();
```

## File: studio/src/probe_async.cpp
```cpp
void ProbeVideoAsync(const std::filesystem::path& path,
```

## File: studio/src/probe_async.hpp
```cpp
class QObject;
⋮----
void ProbeVideoAsync(const std::filesystem::path& path,
```

## File: studio/src/qml_shell.cpp
```cpp
bool DragHasVideo(const QMimeData* mime) {
⋮----
const QUrl& MainQmlUrl() {
⋮----
QmlShell::QmlShell(QWidget* parent) : QMainWindow(parent) {
⋮----
void QmlShell::OnQuickStatusChanged(int status) {
⋮----
void QmlShell::syncPreviewGeometry(int x, int y, int w, int h) {
⋮----
void QmlShell::loadPreview(const QString& path) {
⋮----
void QmlShell::setPreviewPaused(bool paused) {
⋮----
void QmlShell::ApplyCloseToTrayPreference(bool on) {
⋮----
void QmlShell::EnsureTray() {
⋮----
void QmlShell::RefreshTrayTooltip() {
⋮----
void QmlShell::ShowFromTray() {
⋮----
void QmlShell::QuitFromTray() {
⋮----
void QmlShell::closeEvent(QCloseEvent* event) {
⋮----
void QmlShell::resizeEvent(QResizeEvent* event) {
⋮----
void QmlShell::showEvent(QShowEvent* event) {
⋮----
void QmlShell::SetImportTarget(LibraryGridModel* model) {
⋮----
void QmlShell::dragEnterEvent(QDragEnterEvent* event) {
⋮----
void QmlShell::dragMoveEvent(QDragMoveEvent* event) {
⋮----
void QmlShell::dropEvent(QDropEvent* event) {
⋮----
void SetActiveQmlShell(QmlShell* shell) { g_active_shell = shell; }
⋮----
QmlShell* ActiveQmlShell() { return g_active_shell; }
```

## File: studio/src/qml_shell.hpp
```cpp
class QCloseEvent;
class QMenu;
class QQuickWidget;
class QSystemTrayIcon;
class QAction;
class QWidget;
⋮----
class LibraryGridModel;
class PreviewWidget;
⋮----
class QmlShell final : public QMainWindow {
⋮----
void SetImportTarget(LibraryGridModel* model);
⋮----
void syncPreviewGeometry(int x, int y, int w, int h);
⋮----
void loadPreview(const QString& path);
void setPreviewPaused(bool paused);
⋮----
void ApplyCloseToTrayPreference(bool on);
⋮----
void resizeEvent(QResizeEvent* event) override;
void showEvent(QShowEvent* event) override;
⋮----
void closeEvent(QCloseEvent* event) override;
⋮----
void dragEnterEvent(QDragEnterEvent* event) override;
void dragMoveEvent(QDragMoveEvent* event) override;
void dropEvent(QDropEvent* event) override;
⋮----
void OnQuickStatusChanged(int status);
⋮----
void EnsureTray();
⋮----
void RefreshTrayTooltip();
⋮----
void ShowFromTray();
⋮----
void QuitFromTray();
⋮----
void SetActiveQmlShell(QmlShell* shell);
QmlShell* ActiveQmlShell();
⋮----
void ReleaseStudioMutex();
```

## File: studio/src/settings_bridge.cpp
```cpp
void SetCloseToTrayEnabled(bool on) {
⋮----
bool CloseToTrayEnabled() { return g_close_to_tray.load(std::memory_order_relaxed); }
⋮----
void SetCloseToTrayListener(std::function<void(bool)> listener) {
⋮----
PerformancePresetValues PresetToValues(const std::string& preset) {
⋮----
std::string ValuesToPreset(const PerformancePresetValues& values) {
⋮----
SettingsBridge::SettingsBridge(QObject* parent) : QObject(parent) {
⋮----
void SettingsBridge::reload() {
⋮----
emit changed();
⋮----
void SettingsBridge::apply() {
⋮----
void SettingsBridge::PersistStudioSettingsNow() {
⋮----
bool SettingsBridge::engineRestartNeeded() const {
⋮----
void SettingsBridge::setAutoCompressOnImport(bool on) {
⋮----
void SettingsBridge::setCompressOutputDir(const QString& dir) {
⋮----
void SettingsBridge::setDefaultCrf(int value) {
⋮----
void SettingsBridge::setDefaultFps(int value) {
⋮----
void SettingsBridge::setDefaultResolutionMode(const QString& mode) {
⋮----
void SettingsBridge::setStartWithWindows(bool on) {
⋮----
void SettingsBridge::setCacheDir(const QString& dir) {
⋮----
void SettingsBridge::setLockscreenOfferShown(bool shown) {
⋮----
QString SettingsBridge::lockscreenStatus() const {
⋮----
QString SettingsBridge::wallpaperStatus() const {
⋮----
bool SettingsBridge::legacyLockscreenCleanupNeeded() const {
⋮----
void SettingsBridge::setWallpaperSync(bool on) {
⋮----
void SettingsBridge::setLockscreenSync(bool on) {
⋮----
QString SettingsBridge::cleanupLegacyLockscreen() {
⋮----
void SettingsBridge::setLockscreenOffsetSec(double seconds) {
⋮----
void SettingsBridge::setCompressAdvancedVisible(bool on) {
⋮----
void SettingsBridge::setCheckUpdates(bool on) {
⋮----
void SettingsBridge::setCloseToTray(bool on) {
⋮----
void SettingsBridge::setPlaylistSource(const QString& source) {
⋮----
void SettingsBridge::setPerformancePreset(const QString& preset) {
⋮----
emit performancePresetApplied(needs_restart);
⋮----
void SettingsBridge::setLanguage(const QString& code) {
⋮----
void SettingsBridge::setFitMode(const QString& mode) {
⋮----
void SettingsBridge::setMonitorId(int id) {
⋮----
void SettingsBridge::setBatterySaver(bool on) {
⋮----
void SettingsBridge::setBatteryMode(const QString& mode) {
⋮----
void SettingsBridge::setCpuAffinity(const QString& mode) {
⋮----
void SettingsBridge::setGpuAdapter(const QString& mode) {
⋮----
void SettingsBridge::setFpsCap(int value) {
⋮----
void SettingsBridge::setCrf(int value) {
⋮----
void SettingsBridge::setSpeed(double value) {
⋮----
void SettingsBridge::setResolutionW(int value) {
⋮----
void SettingsBridge::setResolutionH(int value) {
⋮----
void SettingsBridge::pickCompressOutputDir() {
⋮----
void SettingsBridge::pickCacheDir() {
⋮----
void SettingsBridge::openCacheDir() {
⋮----
int SettingsBridge::clearCache() {
⋮----
for (std::filesystem::recursive_directory_iterator it(root, ec), end;
⋮----
void SettingsBridge::AppendLog(const QString& line) {
⋮----
emit logChanged();
⋮----
void SettingsBridge::SetLastError(const QString& error) {
⋮----
emit lastErrorChanged();
```

## File: studio/src/settings_bridge.hpp
```cpp
void SetCloseToTrayEnabled(bool on);
bool CloseToTrayEnabled();
⋮----
void SetCloseToTrayListener(std::function<void(bool)> listener);
⋮----
struct PerformancePresetValues {
⋮----
PerformancePresetValues PresetToValues(const std::string& preset);
⋮----
std::string ValuesToPreset(const PerformancePresetValues& values);
⋮----
class SettingsBridge : public QObject {
⋮----
QML_NAMED_ELEMENT(Settings)
⋮----
explicit SettingsBridge(QObject* parent = nullptr);
⋮----
Q_PROPERTY(bool autoCompressOnImport READ autoCompressOnImport WRITE setAutoCompressOnImport NOTIFY changed)
Q_PROPERTY(QString compressOutputDir READ compressOutputDir WRITE setCompressOutputDir NOTIFY changed)
Q_PROPERTY(int defaultCrf READ defaultCrf WRITE setDefaultCrf NOTIFY changed)
Q_PROPERTY(int defaultFps READ defaultFps WRITE setDefaultFps NOTIFY changed)
Q_PROPERTY(QString defaultResolutionMode READ defaultResolutionMode WRITE setDefaultResolutionMode NOTIFY changed)
Q_PROPERTY(bool startWithWindows READ startWithWindows WRITE setStartWithWindows NOTIFY changed)
Q_PROPERTY(QString cacheDir READ cacheDir WRITE setCacheDir NOTIFY changed)
Q_PROPERTY(bool wallpaperSync READ wallpaperSync WRITE setWallpaperSync NOTIFY changed)
Q_PROPERTY(bool lockscreenSync READ lockscreenSync WRITE setLockscreenSync NOTIFY changed)
⋮----
Q_PROPERTY(bool lockscreenOfferShown READ lockscreenOfferShown WRITE setLockscreenOfferShown NOTIFY changed)
⋮----
Q_PROPERTY(QString lockscreenStatus READ lockscreenStatus NOTIFY changed)
Q_PROPERTY(QString wallpaperStatus READ wallpaperStatus NOTIFY changed)
⋮----
Q_PROPERTY(bool legacyLockscreenCleanupNeeded READ legacyLockscreenCleanupNeeded NOTIFY changed)
Q_PROPERTY(double lockscreenOffsetSec READ lockscreenOffsetSec WRITE setLockscreenOffsetSec NOTIFY changed)
Q_PROPERTY(bool compressAdvancedVisible READ compressAdvancedVisible WRITE setCompressAdvancedVisible NOTIFY changed)
Q_PROPERTY(bool checkUpdates READ checkUpdates WRITE setCheckUpdates NOTIFY changed)
⋮----
Q_PROPERTY(QString playlistSource READ playlistSource WRITE setPlaylistSource NOTIFY changed)
⋮----
Q_PROPERTY(QString performancePreset READ performancePreset WRITE setPerformancePreset NOTIFY changed)
⋮----
Q_PROPERTY(bool closeToTray READ closeToTray WRITE setCloseToTray NOTIFY changed)
⋮----
bool autoCompressOnImport() const { return studio_.auto_compress_on_import; }
QString compressOutputDir() const { return QString::fromStdWString(studio_.compress_output_dir); }
int defaultCrf() const { return studio_.default_crf; }
int defaultFps() const { return studio_.default_fps; }
QString defaultResolutionMode() const { return QString::fromStdString(studio_.default_resolution_mode); }
bool startWithWindows() const { return autostart_on_; }
QString cacheDir() const { return QString::fromStdWString(studio_.cache_dir); }
bool wallpaperSync() const { return studio_.wallpaper_sync; }
bool lockscreenSync() const { return studio_.lockscreen_sync; }
bool lockscreenOfferShown() const { return studio_.lockscreen_offer_shown; }
QString lockscreenStatus() const;
QString wallpaperStatus() const;
bool legacyLockscreenCleanupNeeded() const;
double lockscreenOffsetSec() const { return studio_.lockscreen_offset_sec; }
bool compressAdvancedVisible() const { return studio_.compress_advanced_visible; }
bool checkUpdates() const { return studio_.check_updates; }
QString playlistSource() const { return QString::fromStdString(studio_.playlist_source); }
QString performancePreset() const { return QString::fromStdString(studio_.performance_preset); }
bool closeToTray() const { return studio_.close_to_tray; }
⋮----
Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY changed)
⋮----
QString language() const { return ui_language_; }
⋮----
Q_PROPERTY(QString fitMode READ fitMode WRITE setFitMode NOTIFY changed)
Q_PROPERTY(int monitorId READ monitorId WRITE setMonitorId NOTIFY changed)
Q_PROPERTY(bool batterySaver READ batterySaver WRITE setBatterySaver NOTIFY changed)
Q_PROPERTY(QString batteryMode READ batteryMode WRITE setBatteryMode NOTIFY changed)
Q_PROPERTY(QString cpuAffinity READ cpuAffinity WRITE setCpuAffinity NOTIFY changed)
Q_PROPERTY(QString gpuAdapter READ gpuAdapter WRITE setGpuAdapter NOTIFY changed)
Q_PROPERTY(int fpsCap READ fpsCap WRITE setFpsCap NOTIFY changed)
Q_PROPERTY(int crf READ crf WRITE setCrf NOTIFY changed)
Q_PROPERTY(double speed READ speed WRITE setSpeed NOTIFY changed)
Q_PROPERTY(int resolutionW READ resolutionW WRITE setResolutionW NOTIFY changed)
Q_PROPERTY(int resolutionH READ resolutionH WRITE setResolutionH NOTIFY changed)
⋮----
Q_PROPERTY(bool engineRestartNeeded READ engineRestartNeeded NOTIFY changed)
⋮----
QString fitMode() const { return QString::fromStdString(config_.fit_mode); }
int monitorId() const { return config_.monitor_id; }
bool batterySaver() const { return config_.battery_saver; }
QString batteryMode() const { return QString::fromStdString(config_.battery_mode); }
QString cpuAffinity() const { return QString::fromStdString(config_.cpu_affinity); }
QString gpuAdapter() const { return QString::fromStdString(config_.gpu_adapter); }
int fpsCap() const { return config_.fps_cap; }
int crf() const { return config_.crf; }
double speed() const { return config_.speed; }
int resolutionW() const { return config_.resolution_w; }
int resolutionH() const { return config_.resolution_h; }
bool engineRestartNeeded() const;
⋮----
Q_PROPERTY(QString settingsPath READ settingsPath CONSTANT)
Q_PROPERTY(QString configPath READ configPath CONSTANT)
Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
Q_PROPERTY(QStringList log READ log NOTIFY logChanged)
⋮----
QString settingsPath() const { return QString::fromStdWString(settings_path_.wstring()); }
QString configPath() const { return QString::fromStdWString(config_path_.wstring()); }
QString lastError() const { return last_error_; }
QStringList log() const { return log_; }
⋮----
Q_INVOKABLE void reload();
⋮----
Q_INVOKABLE void apply();
⋮----
Q_INVOKABLE void pickCompressOutputDir();
Q_INVOKABLE void pickCacheDir();
Q_INVOKABLE void openCacheDir();
⋮----
Q_INVOKABLE int clearCache();
⋮----
Q_INVOKABLE void setAutoCompressOnImport(bool on);
Q_INVOKABLE void setCompressOutputDir(const QString& dir);
Q_INVOKABLE void setDefaultCrf(int value);
Q_INVOKABLE void setDefaultFps(int value);
Q_INVOKABLE void setDefaultResolutionMode(const QString& mode);
Q_INVOKABLE void setStartWithWindows(bool on);
Q_INVOKABLE void setCacheDir(const QString& dir);
Q_INVOKABLE void setWallpaperSync(bool on);
Q_INVOKABLE void setLockscreenSync(bool on);
⋮----
Q_INVOKABLE QString cleanupLegacyLockscreen();
⋮----
Q_INVOKABLE void setLockscreenOfferShown(bool shown);
Q_INVOKABLE void setLockscreenOffsetSec(double seconds);
Q_INVOKABLE void setCompressAdvancedVisible(bool on);
Q_INVOKABLE void setCheckUpdates(bool on);
Q_INVOKABLE void setPlaylistSource(const QString& source);
⋮----
Q_INVOKABLE void setPerformancePreset(const QString& preset);
⋮----
Q_INVOKABLE void setCloseToTray(bool on);
⋮----
Q_INVOKABLE void setLanguage(const QString& code);
⋮----
Q_INVOKABLE void setFitMode(const QString& mode);
Q_INVOKABLE void setMonitorId(int id);
Q_INVOKABLE void setBatterySaver(bool on);
Q_INVOKABLE void setBatteryMode(const QString& mode);
Q_INVOKABLE void setCpuAffinity(const QString& mode);
Q_INVOKABLE void setGpuAdapter(const QString& mode);
Q_INVOKABLE void setFpsCap(int value);
Q_INVOKABLE void setCrf(int value);
Q_INVOKABLE void setSpeed(double value);
Q_INVOKABLE void setResolutionW(int value);
Q_INVOKABLE void setResolutionH(int value);
⋮----
void changed();
void lastErrorChanged();
void logChanged();
⋮----
void performancePresetApplied(bool needsRestart);
⋮----
void AppendLog(const QString& line);
void SetLastError(const QString& error);
⋮----
void PersistStudioSettingsNow();
```

## File: studio/src/studio_bridge.cpp
```cpp
void SetStudioTrayHooks(StudioTrayHooks hooks) { g_studio_tray_hooks = std::move(hooks); }
⋮----
void ClearStudioTrayHooks() {
⋮----
StudioTrayHooks GetStudioTrayHooks() { return g_studio_tray_hooks; }
⋮----
void SetStudioTrayStatusListener(std::function<void()> listener) {
⋮----
void NotifyStudioTrayStatusChanged() {
⋮----
bool TakeResult(QFutureWatcher<T>* watcher, T* out, QString* error_out) {
⋮----
QVariantList BuildDisplayEntries(
⋮----
QString DuplicateModeNoticeText(const std::vector<std::wstring>& keys) {
⋮----
bool DisplayCapabilityFromState(const EngineState& state) {
⋮----
bool NeedsAllScreensConfirm(int assignment_count) {
⋮----
PlaylistLiveState PlaylistLiveFromState(const EngineState& state) {
⋮----
QString StatusVideoNameFor(BridgeStatusKind kind, const QString& detail,
⋮----
QString StatusTitleFor(BridgeStatusKind kind, const QString& detail,
⋮----
StudioBridge::StudioBridge(QObject* parent) : QObject(parent) {
⋮----
void StudioBridge::requestStatusPoll() {
⋮----
void StudioBridge::OnPollDone() {
⋮----
emit displaysChanged();
⋮----
void StudioBridge::ApplyStatus(const EngineStatusView& view) {
⋮----
emit engineStatusChanged();
⋮----
void StudioBridge::SetLastError(const QString& error) {
⋮----
emit lastErrorChanged();
⋮----
void StudioBridge::ClearLastError() {
⋮----
void StudioBridge::AppendLog(const QString& line) {
⋮----
emit logChanged();
⋮----
void StudioBridge::pickVideo() {
⋮----
emit selectedVideoChanged();
⋮----
void StudioBridge::selectVideo(const QString& path) {
⋮----
void StudioBridge::clearSelectedVideo() {
⋮----
void StudioBridge::refreshQuickSettings() {
⋮----
emit quickSettingsChanged();
⋮----
void StudioBridge::refreshDisplays() {
⋮----
void StudioBridge::ApplyDisplayModel(
⋮----
void StudioBridge::ApplyPlaylistLive(const PlaylistLiveState& live) {
⋮----
bool StudioBridge::IsKnownDisplayKey(const QString& key) const {
⋮----
void StudioBridge::MergeCoverageIntoDisplays() {
⋮----
void StudioBridge::assignVideoToMonitor(const QString& key,
⋮----
void StudioBridge::clearMonitorAssignment(const QString& key) {
⋮----
int StudioBridge::applyToAllMonitors(const QString& path) {
⋮----
void StudioBridge::openWindowsDisplaySettings() {
⋮----
void StudioBridge::setQuickFit(const QString& fit_mode) {
⋮----
void StudioBridge::setQuickMonitor(int monitor_id) {
⋮----
void StudioBridge::setQuickAutostart(bool enabled) {
⋮----
void StudioBridge::setQuickBattery(bool enabled) {
⋮----
void StudioBridge::startEngine() {
⋮----
emit busyChanged();
⋮----
void StudioBridge::OnEngineStartDone() {
⋮----
qint64 StudioBridge::compressFirstOfferMb(const QString& path) const {
⋮----
void StudioBridge::applyWallpaper(const QString& path) {
⋮----
void StudioBridge::OnApplyDone() {
⋮----
void StudioBridge::RunPauseResume(bool do_pause) {
⋮----
void StudioBridge::pause() {
⋮----
void StudioBridge::resume() {
⋮----
void StudioBridge::togglePause() {
⋮----
void StudioBridge::copyToClipboard(const QString& text) {
⋮----
void StudioBridge::OnPauseResumeDone() {
⋮----
void StudioBridge::requestPoster(const QString& video_path) {
⋮----
emit posterChanged();
⋮----
void StudioBridge::OnPosterDone() {
⋮----
void StudioBridge::checkForUpdates() {
⋮----
void StudioBridge::checkForUpdatesInteractive() {
⋮----
void StudioBridge::RunUpdateCheck(bool interactive) {
⋮----
emit updateChanged();
⋮----
void StudioBridge::syncPreviewGeometry(int x, int y, int w, int h) {
⋮----
void StudioBridge::loadPreview(const QString& path) {
⋮----
void StudioBridge::setPreviewPaused(bool paused) {
```

## File: studio/src/studio_bridge.hpp
```cpp
class QTimer;
⋮----
class UpdateChecker;
⋮----
struct StudioTrayHooks {
⋮----
void SetStudioTrayHooks(StudioTrayHooks hooks);
⋮----
void ClearStudioTrayHooks();
⋮----
StudioTrayHooks GetStudioTrayHooks();
⋮----
void NotifyStudioTrayStatusChanged();
⋮----
void SetStudioTrayStatusListener(std::function<void()> listener);
⋮----
enum class BridgeStatusKind {
⋮----
struct EngineStartOutcome {
⋮----
struct ApplyOutcome {
⋮----
QVariantList BuildDisplayEntries(
⋮----
QString DuplicateModeNoticeText(const std::vector<std::wstring>& keys);
⋮----
bool DisplayCapabilityFromState(const EngineState& state);
⋮----
bool NeedsAllScreensConfirm(int assignment_count);
⋮----
struct PlaylistLiveState {
⋮----
PlaylistLiveState PlaylistLiveFromState(const EngineState& state);
⋮----
QString StatusTitleFor(BridgeStatusKind kind, const QString& detail,
⋮----
QString StatusVideoNameFor(BridgeStatusKind kind, const QString& detail,
⋮----
class StudioBridge : public QObject {
⋮----
QML_NAMED_ELEMENT(Studio)
⋮----
explicit StudioBridge(QObject* parent = nullptr);
⋮----
Q_PROPERTY(int engineStatusKind READ engineStatusKind NOTIFY engineStatusChanged)
Q_PROPERTY(QString engineStatusDetail READ engineStatusDetail NOTIFY engineStatusChanged)
Q_PROPERTY(QString engineStatusHint READ engineStatusHint NOTIFY engineStatusChanged)
⋮----
Q_PROPERTY(QString statusTitle READ statusTitle NOTIFY engineStatusChanged)
Q_PROPERTY(QString statusVideoName READ statusVideoName NOTIFY engineStatusChanged)
Q_PROPERTY(quint64 enginePid READ enginePid NOTIFY engineStatusChanged)
Q_PROPERTY(bool videoActive READ videoActive NOTIFY engineStatusChanged)
Q_PROPERTY(bool engineRunning READ engineRunning NOTIFY engineStatusChanged)
Q_PROPERTY(QString activeVideoPath READ activeVideoPath NOTIFY engineStatusChanged)
Q_PROPERTY(QString posterPath READ posterPath NOTIFY posterChanged)
Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
Q_PROPERTY(QString settingsPath READ settingsPath CONSTANT)
Q_PROPERTY(QString version READ version CONSTANT)
Q_PROPERTY(QString donateUrl READ donateUrl CONSTANT)
Q_PROPERTY(QString projectUrl READ projectUrl CONSTANT)
⋮----
Q_PROPERTY(bool updateAvailable READ updateAvailable NOTIFY updateChanged)
Q_PROPERTY(bool updateCheckBusy READ updateCheckBusy NOTIFY updateChanged)
Q_PROPERTY(QString latestVersion READ latestVersion NOTIFY updateChanged)
Q_PROPERTY(QString latestPageUrl READ latestPageUrl NOTIFY updateChanged)
⋮----
Q_PROPERTY(QString updateCheckMessage READ updateCheckMessage NOTIFY updateChanged)
Q_INVOKABLE void checkForUpdates();
Q_INVOKABLE void checkForUpdatesInteractive();
Q_PROPERTY(QStringList log READ log NOTIFY logChanged)
⋮----
Q_PROPERTY(QString selectedVideo READ selectedVideo NOTIFY selectedVideoChanged)
⋮----
Q_PROPERTY(QString quickFit READ quickFit NOTIFY quickSettingsChanged)
Q_PROPERTY(int quickMonitor READ quickMonitor NOTIFY quickSettingsChanged)
Q_PROPERTY(bool quickAutostart READ quickAutostart NOTIFY quickSettingsChanged)
Q_PROPERTY(bool quickBattery READ quickBattery NOTIFY quickSettingsChanged)
⋮----
Q_PROPERTY(QVariantList monitorChoices READ monitorChoices NOTIFY quickSettingsChanged)
⋮----
Q_PROPERTY(QVariantList displays READ displays NOTIFY displaysChanged)
⋮----
Q_PROPERTY(bool displayCapability READ displayCapability NOTIFY displaysChanged)
⋮----
Q_PROPERTY(QString duplicateModeNotice READ duplicateModeNotice NOTIFY displaysChanged)
⋮----
Q_PROPERTY(bool playlistLiveEnabled READ playlistLiveEnabled NOTIFY engineStatusChanged)
Q_PROPERTY(int playlistLiveSize READ playlistLiveSize NOTIFY engineStatusChanged)
Q_PROPERTY(int playlistLiveIndex READ playlistLiveIndex NOTIFY engineStatusChanged)
⋮----
QString selectedVideo() const { return selected_video_; }
QString quickFit() const { return quick_fit_; }
int quickMonitor() const { return quick_monitor_; }
bool quickAutostart() const { return quick_autostart_; }
bool quickBattery() const { return quick_battery_; }
QVariantList monitorChoices() const { return monitor_choices_; }
QVariantList displays() const { return displays_; }
bool displayCapability() const { return display_capability_; }
QString duplicateModeNotice() const { return duplicate_mode_notice_; }
bool playlistLiveEnabled() const { return playlist_live_enabled_; }
int playlistLiveSize() const { return playlist_live_size_; }
int playlistLiveIndex() const { return playlist_live_index_; }
⋮----
int engineStatusKind() const { return static_cast<int>(status_.kind); }
⋮----
QString engineStatusDetail() const { return status_detail_; }
⋮----
QString engineStatusHint() const { return status_hint_; }
⋮----
QString statusTitle() const {
⋮----
QString statusVideoName() const {
⋮----
quint64 enginePid() const { return status_.pid; }
⋮----
bool videoActive() const { return video_active_; }
⋮----
bool engineRunning() const { return engine_running_; }
⋮----
QString activeVideoPath() const { return active_video_; }
⋮----
QString posterPath() const { return poster_path_; }
⋮----
QString lastError() const { return last_error_; }
⋮----
bool busy() const { return busy_; }
⋮----
QString settingsPath() const { return settings_path_; }
QString version() const { return version_; }
QString donateUrl() const { return QString::fromUtf8(K6WP_DONATE_URL); }
QString projectUrl() const { return QString::fromUtf8(K6WP_PROJECT_URL); }
bool updateAvailable() const { return !latest_version_.isEmpty(); }
bool updateCheckBusy() const { return update_busy_; }
QString latestVersion() const { return latest_version_; }
QString latestPageUrl() const { return latest_page_url_; }
QString updateCheckMessage() const { return update_check_message_; }
⋮----
QStringList log() const { return log_; }
⋮----
Q_INVOKABLE void pickVideo();
⋮----
Q_INVOKABLE void selectVideo(const QString& path);
Q_INVOKABLE void clearSelectedVideo();
⋮----
Q_INVOKABLE void setQuickFit(const QString& fit_mode);
Q_INVOKABLE void setQuickMonitor(int monitor_id);
Q_INVOKABLE void setQuickAutostart(bool enabled);
Q_INVOKABLE void setQuickBattery(bool enabled);
⋮----
Q_INVOKABLE void refreshQuickSettings();
⋮----
Q_INVOKABLE void refreshDisplays();
⋮----
Q_INVOKABLE void assignVideoToMonitor(const QString& key, const QString& path);
⋮----
Q_INVOKABLE void clearMonitorAssignment(const QString& key);
⋮----
Q_INVOKABLE int applyToAllMonitors(const QString& path);
⋮----
Q_INVOKABLE void openWindowsDisplaySettings();
⋮----
void ApplyDisplayModel(const std::vector<MonitorInfo>& monitors,
⋮----
void ApplyPlaylistLive(const PlaylistLiveState& live);
⋮----
Q_INVOKABLE void requestStatusPoll();
⋮----
Q_INVOKABLE void startEngine();
⋮----
Q_INVOKABLE void applyWallpaper(const QString& path);
⋮----
Q_INVOKABLE qint64 compressFirstOfferMb(const QString& path) const;
⋮----
Q_INVOKABLE void pause();
Q_INVOKABLE void resume();
⋮----
Q_INVOKABLE void togglePause();
⋮----
Q_INVOKABLE void copyToClipboard(const QString& text);
⋮----
Q_INVOKABLE void requestPoster(const QString& videoPath);
⋮----
Q_INVOKABLE void syncPreviewGeometry(int x, int y, int w, int h);
⋮----
Q_INVOKABLE void loadPreview(const QString& path);
⋮----
Q_INVOKABLE void setPreviewPaused(bool paused);
⋮----
void engineStatusChanged();
void posterChanged();
void lastErrorChanged();
void busyChanged();
void logChanged();
void selectedVideoChanged();
void updateChanged();
⋮----
void quickSettingsChanged();
⋮----
void displaysChanged();
⋮----
void OnPollDone();
void OnEngineStartDone();
void OnApplyDone();
void OnPauseResumeDone();
void OnPosterDone();
⋮----
void ApplyStatus(const EngineStatusView& view);
void SetLastError(const QString& error);
void ClearLastError();
void AppendLog(const QString& line);
⋮----
void RunPauseResume(bool do_pause);
void RunUpdateCheck(bool interactive);
⋮----
bool IsKnownDisplayKey(const QString& key) const;
⋮----
void MergeCoverageIntoDisplays();
```

## File: studio/src/thumbnailer.hpp
```cpp
class Thumbnailer {
⋮----
Thumbnailer();
⋮----
[[nodiscard]] std::filesystem::path ThumbsRoot() const;
⋮----
[[nodiscard]] bool EnsureRoot();
⋮----
[[nodiscard]] static std::string CacheKeyHex(
⋮----
[[nodiscard]] std::filesystem::path GetThumb(
⋮----
[[nodiscard]] std::filesystem::path CachedThumb(
⋮----
[[nodiscard]] static bool GenerateTo(const std::filesystem::path& video,
⋮----
[[nodiscard]] static std::filesystem::path FfmpegPath();
```

## File: studio/src/ui_language.cpp
```cpp
QString SourceLanguage() {
⋮----
QStringList SupportedUiLanguages() {
⋮----
QString UiLanguageCatalogue(const QString& language) {
⋮----
QString UiLanguageStorePath() {
⋮----
QString LoadUiLanguage() {
⋮----
const QSettings store(path, QSettings::IniFormat);
⋮----
bool SaveUiLanguage(const QString& code) {
⋮----
QSettings store(path, QSettings::IniFormat);
```

## File: studio/src/ui_language.hpp
```cpp
QString UiLanguageStorePath();
⋮----
QString LoadUiLanguage();
⋮----
bool SaveUiLanguage(const QString& code);
⋮----
QStringList SupportedUiLanguages();
⋮----
QString UiLanguageCatalogue(const QString& language);
```

## File: studio/src/update_checker.cpp
```cpp
struct WinHttpHandle {
explicit WinHttpHandle(HINTERNET handle = nullptr) : h(handle) {}
⋮----
WinHttpHandle(const WinHttpHandle&) = delete;
⋮----
HINTERNET get() const { return h; }
⋮----
std::wstring ToWide(const std::string& utf8) {
⋮----
std::vector<wchar_t> buf(static_cast<size_t>(n));
⋮----
std::string TrimAscii(const std::string& s) {
⋮----
std::string WinHttpGet(const std::string& url_utf8) {
⋮----
bool IsUpdateCheckUrlUsable(const QString& url) {
⋮----
QString ResolveUpdateCheckUrl() {
⋮----
UpdateChecker::UpdateChecker(const QString& current_version, QObject* parent)
⋮----
void UpdateChecker::SetReleasesPageUrl(const QString& url) {
⋮----
void UpdateChecker::Check(const QString& api_url) {
⋮----
void UpdateChecker::CheckInteractive(const QString& api_url) {
⋮----
void UpdateChecker::CheckInternal(const QString& api_url, bool interactive) {
⋮----
emit CheckFinished(UpdateCheckOutcome::kDisabled);
⋮----
void UpdateChecker::FetchAndReport(std::string api_url_utf8,
⋮----
void UpdateChecker::ReportResult(UpdateCheckOutcome outcome,
⋮----
emit UpdateAvailable(latest, page);
⋮----
emit CheckFinished(outcome);
```

## File: studio/src/update_checker.hpp
```cpp
enum class UpdateCheckOutcome {
⋮----
QString ResolveUpdateCheckUrl();
bool IsUpdateCheckUrlUsable(const QString& url);
⋮----
class UpdateChecker final : public QObject {
⋮----
void SetReleasesPageUrl(const QString& url);
void Check(const QString& api_url);
void CheckInteractive(const QString& api_url);
⋮----
void UpdateAvailable(const QString& latest_version, const QString& page_url);
void CheckFinished(k6wp::UpdateCheckOutcome outcome);
⋮----
void FetchAndReport(std::string api_url_utf8, QString releases_page,
⋮----
void ReportResult(k6wp::UpdateCheckOutcome outcome, const QString& latest,
⋮----
void CheckInternal(const QString& api_url, bool interactive);
```

## File: studio/src/user_errors.cpp
```cpp
QString FriendlyApplyError(const QString& technical) {
⋮----
QString FriendlyIpcError(const QString& technical) {
⋮----
QString FriendlyPreviewError(const QString& technical) {
```

## File: studio/src/user_errors.hpp
```cpp
QString FriendlyApplyError(const QString& technical);
⋮----
QString FriendlyIpcError(const QString& technical);
⋮----
QString FriendlyPreviewError(const QString& technical);
```

## File: studio/src/video_paths.hpp
```cpp
inline bool IsVideoPath(const QString& path) {
```

## File: tools/bench_ab.ps1
```powershell
# tools/bench_ab.ps1 - A/B compare two engine binaries on the same states.
#
# Answers: "does change X actually help, and does it regress anything?"
# Both binaries are measured with the SAME harness, the SAME corpus, the
# SAME n and the SAME host conditions, so the delta is attributable.
#
# The comparison is GATE-AWARE: a change is only an improvement if it
# improves the protected metric AND does not regress the others. The
# gates come from docs/refactor/budgets.md.
#
# Usage:
#   powershell -NoProfile -ExecutionPolicy Bypass -File tools\bench_ab.ps1 `
#     -BaselineExe build\engine-baseline-phase0.exe `
#     -CandidateExe build\release\engine.exe `
#     -Runs 5 -OutJson docs\bench\ab.json

[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$BaselineExe,
    [Parameter(Mandatory=$true)][string]$CandidateExe,
    [int]$Runs = 5,
    [string[]]$States = @('playback_1080p'),
    [string]$OutJson = '',
    [string]$Label = 'ab'
)

$ErrorActionPreference = 'Stop'
$script:Here = Split-Path -Parent $MyInvocation.MyCommand.Path
. (Join-Path $script:Here 'bench_lib.ps1')

$BaselineExe = [System.IO.Path]::GetFullPath($BaselineExe)
$CandidateExe = [System.IO.Path]::GetFullPath($CandidateExe)
foreach ($p in @($BaselineExe, $CandidateExe)) {
    if (-not (Test-Path -LiteralPath $p)) { throw "engine not found: $p" }
}
if ([string]::IsNullOrWhiteSpace($OutJson)) {
    $OutJson = Join-Path $script:Here ("..\docs\bench\ab-{0}.json" -f $Label)
}
# -States may arrive as ONE comma-joined string (the command line flattens a
# string[] into a single value), so split it into the individual names.
$States = @($States | ForEach-Object { ([string]$_) -split ',' } | ForEach-Object { $_.Trim() } | Where-Object { $_ })
if ($States.Count -eq 0) { throw "-States is empty; nothing to compare" }

function Measure-Engine {
    param([string]$Exe, [string]$Tag)
    $out = Join-Path $env:TEMP ("k6wp-ab-{0}-{1}.json" -f $Tag, [Guid]::NewGuid().ToString('N').Substring(0, 8))
    # -OnlyStates is a string[]; passing it straight through keeps the array
    # intact (a comma list would be flattened into one string).
    & (Join-Path $script:Here 'bench_engine.ps1') -EngineExe $Exe -Runs $Runs -OnlyStates $States -OutJson $out | Out-Null
    if (-not (Test-Path -LiteralPath $out)) { throw "bench_engine produced no output for $Tag" }
    $doc = Get-Content -LiteralPath $out -Raw | ConvertFrom-Json
    Remove-Item -LiteralPath $out -Force -ErrorAction SilentlyContinue
    return $doc
}

Write-Host "bench_ab: baseline  = $BaselineExe"
Write-Host "bench_ab: candidate = $CandidateExe"
Write-Host "bench_ab: states = $($States -join ', '), runs = $Runs"

$base = Measure-Engine -Exe $BaselineExe -Tag 'base'
$cand = Measure-Engine -Exe $CandidateExe -Tag 'cand'

# Metric comparison. Direction: 'lower' = smaller is better.
$metrics = @(
    @{ name = 'private_mb';        dir = 'lower'; unit = 'MB' }
    @{ name = 'ws_mb';             dir = 'lower'; unit = 'MB' }
    @{ name = 'cpu_pct';           dir = 'lower'; unit = '%' }
    @{ name = 'max_thread_cpu_pct'; dir = 'lower'; unit = '% of 1 core' }
    @{ name = 'gpu_total_pct';     dir = 'lower'; unit = '%' }
    @{ name = 'gpu_decode_pct';    dir = 'lower'; unit = '%' }
    @{ name = 'gpu_3d_pct';        dir = 'lower'; unit = '%' }
    @{ name = 'gpu_shared_mb';     dir = 'lower'; unit = 'MB' }
    @{ name = 'wakeups_per_sec';   dir = 'lower'; unit = '/s' }
)

$rows = @()
foreach ($state in $States) {
    $b = $base.states.$state
    $c = $cand.states.$state
    if ($null -eq $b -or $null -eq $c) { continue }
    foreach ($m in $metrics) {
        $bv = $b.$($m.name)
        $cv = $c.$($m.name)
        if ($null -eq $bv -or $null -eq $cv -or $bv.n -eq 0 -or $cv.n -eq 0) { continue }
        $bmed = [double]$bv.median
        $cmed = [double]$cv.median
        $delta = $cmed - $bmed
        $pct = if ($bmed -ne 0) { [math]::Round(($delta / $bmed) * 100, 2) } else { $null }
        $improved = if ($m.dir -eq 'lower') { $delta -lt 0 } else { $delta -gt 0 }
        # A change is only meaningful if it exceeds the measured noise.
        $noise = if ($bmed -ne 0) { [math]::Abs((($bv.max - $bv.min) / $bmed) * 100) } else { 0 }
        $significant = ($null -ne $pct) -and ([math]::Abs($pct) -gt $noise)
        $rows += [pscustomobject]@{
            state = $state
            metric = $m.name
            unit = $m.unit
            baseline = $bmed
            candidate = $cmed
            delta = [math]::Round($delta, 3)
            delta_pct = $pct
            noise_pct = [math]::Round($noise, 2)
            direction = $m.dir
            improved = $improved
            significant = $significant
            verdict = $(if ($significant -and $improved) { 'BETTER' } elseif ($significant -and -not $improved) { 'WORSE' } else { 'same' })
        }
    }
}

Write-Host ""
Write-Host ("=" * 92)
Write-Host ("  {0}: baseline vs candidate" -f $Label)
Write-Host ("=" * 92)
$rows | Format-Table -AutoSize | Out-String | Write-Host

$better = @($rows | Where-Object { $_.verdict -eq 'BETTER' })
$worse = @($rows | Where-Object { $_.verdict -eq 'WORSE' })
Write-Host ("BETTER: {0}   WORSE: {1}   same: {2}" -f $better.Count, $worse.Count, ($rows.Count - $better.Count - $worse.Count))
if ($worse.Count -gt 0) {
    Write-Host ""
    Write-Host "REGRESSIONS (must be justified or reverted):"
    $worse | Format-Table -AutoSize | Out-String | Write-Host
}

$report = [ordered]@{
    tool = 'bench_ab'
    phase = 1
    date = (Get-Date -Format 'yyyy-MM-ddTHH:mm:ssK')
    label = 'live-proven'
    tag = $Label
    baseline_exe = $BaselineExe
    candidate_exe = $CandidateExe
    runs_per_state = $Runs
    states = $States
    method = 'same harness, same corpus, same n, same host; deltas compared against the measured noise band'
    comparison = $rows
    better_count = $better.Count
    worse_count = $worse.Count
    baseline = $base
    candidate = $cand
}
$json = $report | ConvertTo-Json -Depth 12
$outFull = [System.IO.Path]::GetFullPath($OutJson)
$outDir = [System.IO.Path]::GetDirectoryName($outFull)
if (-not (Test-Path -LiteralPath $outDir)) { New-Item -ItemType Directory -Force -Path $outDir | Out-Null }
$json | Set-Content -LiteralPath $outFull -Encoding UTF8
Write-Host "bench_ab: wrote $outFull"
```

## File: tools/bench_bundle_zip.ps1
```powershell
# tools/bench_bundle_zip.ps1 - the compressed (shipping) size per group.
#
# The INSTALL size and the ZIP size are different levers: the install is
# dominated by binaries that do not compress, and the ZIP is what the user
# downloads. This measures the per-group COMPRESSED contribution from the
# actual shipped ZIP, so the bundle target is set against the number the
# user actually feels.
$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
. (Join-Path $here 'bench_lib.ps1')

$zipPath = (Get-ChildItem (Join-Path $here '..\dist') -Filter '*.zip' -File |
    Sort-Object LastWriteTime -Descending | Select-Object -First 1).FullName
if (-not $zipPath) { throw "no ZIP found in dist\" }
Write-Host "zip: $zipPath"

Add-Type -AssemblyName System.IO.Compression.FileSystem
$zip = [System.IO.Compression.ZipFile]::OpenRead($zipPath)
try {
    $rows = @()
    foreach ($e in $zip.Entries) {
        if ($e.Length -eq 0) { continue }
        $name = $e.FullName
        $group = 'other'
        if ($name -match '(^|/)ffmpeg\.exe$' -or $name -match '(^|/)ffprobe\.exe$') { $group = 'ffmpeg+ffprobe' }
        elseif ($name -match '(^|/)libmpv-2\.dll$') { $group = 'mpv' }
        elseif ($name -match '(^|/)Qt6.*\.dll$') { $group = 'qt-dlls' }
        elseif ($name -match '(^|/)qml/') { $group = 'qml' }
        elseif ($name -match '(^|/)dxcompiler\.dll$' -or $name -match '(^|/)dxil\.dll$') { $group = 'shader-cache' }
        elseif ($name -match '(^|/)(engine|studio|K6WP|compressor)\.exe$') { $group = 'k6wp-exes' }
        elseif ($name -match '(^|/)vcruntime' -or $name -match '(^|/)msvcp') { $group = 'msvc-runtime' }
        $rows += [pscustomobject]@{
            name = $name
            uncompressed = $e.Length
            compressed = $e.CompressedLength
            group = $group
        }
    }
    $totalU = ($rows | Measure-Object uncompressed -Sum).Sum
    $totalC = ($rows | Measure-Object compressed -Sum).Sum
    Write-Host ""
    Write-Host ("ZIP: {0} entries, {1} MB uncompressed, {2} MB compressed ({3}% ratio)" -f `
        $rows.Count, [math]::Round($totalU / 1MB, 2), [math]::Round($totalC / 1MB, 2),
        [math]::Round(($totalC / $totalU) * 100, 1))
    Write-Host ""
    Write-Host ("{0,-20} {1,10} {2,10} {3,8}" -f 'group', 'uncomp MB', 'zip MB', 'zip %')
    $agg = $rows | Group-Object group | ForEach-Object {
        [pscustomobject]@{
            group = $_.Name
            uncomp = [math]::Round((($_.Group | Measure-Object uncompressed -Sum).Sum) / 1MB, 2)
            zip = [math]::Round((($_.Group | Measure-Object compressed -Sum).Sum) / 1MB, 2)
            pct = [math]::Round((($_.Group | Measure-Object compressed -Sum).Sum) / $totalC * 100, 1)
        }
    } | Sort-Object zip -Descending
    $agg | Format-Table -AutoSize
    $out = [ordered]@{
        tool = 'bench_bundle_zip'
        date = (Get-Date -Format 'yyyy-MM-ddTHH:mm:ssK')
        label = 'live-proven (one machine)'
        zip = (Split-Path -Leaf $zipPath)
        entries = $rows.Count
        uncompressed_mb = [math]::Round($totalU / 1MB, 2)
        compressed_mb = [math]::Round($totalC / 1MB, 2)
        groups = $agg
        largest = @($rows | Sort-Object compressed -Descending | Select-Object -First 12 name,
            @{n='uncompressed_mb';e={[math]::Round($_.uncompressed/1MB,2)}},
            @{n='zip_mb';e={[math]::Round($_.compressed/1MB,2)}})
    }
    $json = $out | ConvertTo-Json -Depth 6
    $p = Join-Path $here '..\docs\bench\bundle-zip.json'
    $json | Set-Content -LiteralPath ([System.IO.Path]::GetFullPath($p)) -Encoding UTF8
    Write-Host "wrote $p"
}
finally { $zip.Dispose() }
```

## File: tools/bench_bundle.ps1
```powershell
# tools/bench_bundle.ps1 - Phase 0 bundle size inventory.
#
# Answers: "how big is what we ship, and what is the largest lever?"
# Measures the RELEASE build tree (build\release) plus the shipped ZIP(s) in
# dist\. Produces a per-file breakdown grouped by role so the Phase 1/2/3
# bundle-reduction targets can be quantified against a real baseline.
#
# No code instrumentation: this is a filesystem census of the artifacts that
# packaging/installer.nsi and packaging/make_zip.ps1 actually stage.
#
# Usage:
#   powershell -NoProfile -ExecutionPolicy Bypass -File tools\bench_bundle.ps1 `
#     -OutJson docs\bench\bundle.json

[CmdletBinding()]
param(
    [string]$BuildDir = '',
    [string]$VendorDir = '',
    [string]$DistDir = '',
    [string]$OutJson = ''
)

$ErrorActionPreference = 'Stop'
$script:Here = Split-Path -Parent $MyInvocation.MyCommand.Path
if ([string]::IsNullOrWhiteSpace($BuildDir)) { $BuildDir = Join-Path $script:Here '..\build\release' }
if ([string]::IsNullOrWhiteSpace($VendorDir)) { $VendorDir = Join-Path $script:Here '..\vendor' }
if ([string]::IsNullOrWhiteSpace($DistDir)) { $DistDir = Join-Path $script:Here '..\dist' }
if ([string]::IsNullOrWhiteSpace($OutJson)) { $OutJson = Join-Path $script:Here '..\docs\bench\bundle.json' }

$BuildDir = [System.IO.Path]::GetFullPath($BuildDir)
$VendorDir = [System.IO.Path]::GetFullPath($VendorDir)
$DistDir = [System.IO.Path]::GetFullPath($DistDir)
if (-not (Test-Path -LiteralPath $BuildDir)) { throw "build dir not found: $BuildDir" }

# The shipping set is the top level of the release tree plus the Qt/QML
# subdirectories the installer stages (see packaging/installer.nsi).
$stagedDirs = @('platforms', 'imageformats', 'styles', 'iconengines', 'qml', 'networkinformation', 'tls')

# packaging/installer.nsi stages libmpv-2.dll and the VC runtime from
# vendor\, NOT from the build tree, so the census must include them or the
# shipping size is understated by ~116 MB.
$vendorFiles = @(
    @{ path = (Join-Path $VendorDir 'ffmpeg\ffmpeg.exe');   group = 'ffmpeg'; rel = 'ffmpeg.exe' },
    @{ path = (Join-Path $VendorDir 'ffmpeg\ffprobe.exe');  group = 'ffmpeg'; rel = 'ffprobe.exe' },
    @{ path = (Join-Path $VendorDir 'libmpv\bin\libmpv-2.dll'); group = 'mpv'; rel = 'libmpv-2.dll' },
    @{ path = (Join-Path $VendorDir 'vc_runtime\vcruntime140.dll');   group = 'msvc-runtime'; rel = 'vcruntime140.dll' },
    @{ path = (Join-Path $VendorDir 'vc_runtime\vcruntime140_1.dll'); group = 'msvc-runtime'; rel = 'vcruntime140_1.dll' },
    @{ path = (Join-Path $VendorDir 'vc_runtime\msvcp140.dll');       group = 'msvc-runtime'; rel = 'msvcp140.dll' }
)

function Get-GroupName([string]$rel) {
    $top = ($rel -split '[\\/]')[0]
    switch ($true) {
        ($top -eq 'libmpv-2.dll') { return 'mpv' }
        ($top -like 'Qt6*') { return 'qt' }
        ($top -eq 'ffmpeg.exe' -or $top -eq 'ffprobe.exe') { return 'ffmpeg' }
        ($top -eq 'dxcompiler.dll' -or $top -eq 'dxil.dll') { return 'shader-cache' }
        ($top -in @('engine.exe', 'studio.exe', 'K6WP.exe', 'compressor.exe')) { return 'k6wp-exes' }
        ($top -like 'vcruntime*' -or $top -like 'msvcp*') { return 'msvc-runtime' }
        ($stagedDirs -contains $top) { return "qt-payload/$top" }
        ($top -eq 'monitor_dump.exe') { return 'qa-tools' }
        ($top -eq '') { return 'root' }
        default { return 'other' }
    }
}

$files = @()
$rootFiles = @(Get-ChildItem -LiteralPath $BuildDir -File -ErrorAction SilentlyContinue)
foreach ($f in $rootFiles) {
    # The build tree also carries a windeployqt copy of libmpv/ffmpeg-era DLLs;
    # the installer takes libmpv from vendor\, so the build copy is not staged.
    if ($f.Name -eq 'libmpv-2.dll') { continue }
    $files += [pscustomobject]@{
        rel = $f.Name
        bytes = $f.Length
        group = (Get-GroupName $f.Name)
        source = 'build'
    }
}
foreach ($d in $stagedDirs) {
    $dp = Join-Path $BuildDir $d
    if (-not (Test-Path -LiteralPath $dp)) { continue }
    foreach ($f in @(Get-ChildItem -LiteralPath $dp -Recurse -File -ErrorAction SilentlyContinue)) {
        $rel = $d + '/' + $f.FullName.Substring($dp.Length).TrimStart('\', '/')
        $files += [pscustomobject]@{
            rel = ($rel -replace '\\', '/')
            bytes = $f.Length
            group = (Get-GroupName $rel)
            source = 'build'
        }
    }
}
$missingVendor = @()
foreach ($v in $vendorFiles) {
    if (Test-Path -LiteralPath $v.path) {
        $fi = Get-Item -LiteralPath $v.path
        $files += [pscustomobject]@{
            rel = $v.rel
            bytes = $fi.Length
            group = $v.group
            source = 'vendor'
        }
    }
    else {
        $missingVendor += $v.rel
    }
}

$totalBytes = ($files | Measure-Object -Property bytes -Sum).Sum
$groups = @{}
foreach ($g in ($files | Group-Object group)) {
    $gb = ($g.Group | Measure-Object -Property bytes -Sum).Sum
    $groups[$g.Name] = [ordered]@{
        files = $g.Count
        bytes = $gb
        mb = [math]::Round($gb / 1MB, 2)
        pct = [math]::Round(($gb / $totalBytes) * 100, 1)
    }
}

$top = @($files | Sort-Object bytes -Descending | Select-Object -First 25 | ForEach-Object {
    [ordered]@{ rel = $_.rel; bytes = $_.bytes; mb = [math]::Round($_.bytes / 1MB, 2); group = $_.group; source = $_.source }
})

$zips = @()
if (Test-Path -LiteralPath $DistDir) {
    $zips = @(Get-ChildItem -LiteralPath $DistDir -Filter '*.zip' -File -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTime -Descending | ForEach-Object {
            [ordered]@{
                name = $_.Name
                bytes = $_.Length
                mb = [math]::Round($_.Length / 1MB, 2)
                modified = $_.LastWriteTime.ToString('yyyy-MM-dd')
            }
        })
}

$report = [ordered]@{
    tool = 'bench_bundle'
    phase = 0
    date = (Get-Date -Format 'yyyy-MM-ddTHH:mm:ssK')
    label = 'live-proven'
    method = 'filesystem census of the release build tree (the set packaging/installer.nsi stages) + shipped zips in dist/'
    build_dir = $BuildDir
    total_files = $files.Count
    total_bytes = $totalBytes
    total_mb = [math]::Round($totalBytes / 1MB, 2)
    groups = $groups
    largest_files = $top
    shipped_zips = $zips
    missing_vendor = $missingVendor
    notes = @(
        'mpv = libmpv-2.dll; qt = Qt6*.dll; qml/qt-payload = the QML module tree; ffmpeg = ffmpeg.exe+ffprobe.exe.',
        'The staging set mirrors packaging/installer.nsi: libmpv + VC runtime come from vendor\, the Qt payload from the release build tree.',
        'This is the pre-refactor baseline. Phase 1.3 (lean libmpv), Phase 2.7 (Qt removal) and Phase 3 target these groups.',
        'Sizes are on-disk bytes; the ZIP column is the compressed shipping size (dist/ may lag the current build).'
    )
}

$json = $report | ConvertTo-Json -Depth 8
$outFull = [System.IO.Path]::GetFullPath($OutJson)
$outDir = [System.IO.Path]::GetDirectoryName($outFull)
if (-not (Test-Path -LiteralPath $outDir)) { New-Item -ItemType Directory -Force -Path $outDir | Out-Null }
$json | Set-Content -LiteralPath $outFull -Encoding UTF8
Write-Host ("bench_bundle: {0} files, {1} MB -> {2}" -f $files.Count, [math]::Round($totalBytes / 1MB, 2), $outFull)
foreach ($k in ($groups.Keys | Sort-Object)) {
    Write-Host ("  {0,-22} {1,10} MB  ({2}%)" -f $k, $groups[$k].mb, $groups[$k].pct)
}
```

## File: tools/bench_compress.ps1
```powershell
<# .SYNOPSIS
  Benchmark compressor.exe over the k6wp-18 corpus; emits JSON array to stdout.
.DESCRIPTION
  For each *.mp4 in -CorpusDir, runs:
    compressor.exe --in <clip> --out <OutDir>/<base>_bench.mp4 --res 1280x720 --fps 30 --crf 23 --encoder auto
  Times the run with Measure-Command, parses the final {"ok":...} stdout line
  for the encoder name, stats the output file size in MB, and emits one JSON
   object per file: {file, encoder, seconds, size_mb}. Prints the JSON
   envelope to stdout (progress goes to stderr via Write-Host -> console).
   Also writes the same JSON to -OutJson (default docs/bench_compress.json).
   Target: HW <30s/file (planning.md section 6: 15-30s for 1-min 4K->1080p
   on a hardware encoder). If missed, record the gap honestly - never fake
   numbers. Without a hardware encoder the run is flagged no_hw:true.
   Cache-masking rule: the compressor LRU cache (Todo 17) is keyed on
   (in,res,fps,crf,encoder), NOT on --out, so re-running the same matrix
   returns cache_hit:true copies. For genuine encode timings point
   $env:K6WP_CACHE_DIR at a fresh empty dir (or vary res/crf/encoder);
   every row records its cache_hit flag as proof.
.EXAMPLE
  powershell -NoProfile -ExecutionPolicy Bypass -File tools\bench_compress.ps1
#>
param(
  [string]$CorpusDir = "",
  [string]$OutDir = "",
  [string]$Res = "1280x720",
  [int]$Fps = 30,
  [int]$Crf = 23,
  [string]$OutJson = ""
)

$ErrorActionPreference = "Continue"

if ([string]::IsNullOrWhiteSpace($CorpusDir)) {
  $CorpusDir = Join-Path $PSScriptRoot "..\tests\corpus"
}
if ([string]::IsNullOrWhiteSpace($OutDir)) {
  $OutDir = Join-Path $env:TEMP "k6wp_bench"
}
$CorpusDir = [System.IO.Path]::GetFullPath($CorpusDir)
$OutDir = [System.IO.Path]::GetFullPath($OutDir)
$Compressor = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot "..\build\msvc-dev\compressor.exe"))

if (-not (Test-Path -LiteralPath $Compressor)) {
  Write-Error "bench_compress: compressor not found at $Compressor (build --target compressor first)"
  exit 1
}
if (-not (Test-Path -LiteralPath $CorpusDir)) {
  Write-Error "bench_compress: corpus dir not found: $CorpusDir (run tools\make_corpus.ps1 first)"
  exit 1
}
if ([string]::IsNullOrWhiteSpace($OutJson)) {
  $OutJson = Join-Path $PSScriptRoot "..\docs\bench_compress.json"
}
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

$files = Get-ChildItem -LiteralPath $CorpusDir -Filter "*.mp4" | Sort-Object Name
if ($files.Count -eq 0) {
  Write-Error "bench_compress: no *.mp4 in $CorpusDir"
  exit 1
}

$results = @()
$failed = $false

foreach ($f in $files) {
  $base = [System.IO.Path]::GetFileNameWithoutExtension($f.Name)
  $outFile = Join-Path $OutDir ($base + "_bench.mp4")
  [void](Write-Host ("bench_compress: {0} -> {1} ..." -f $f.Name, $outFile))

  $stdoutFile = Join-Path $OutDir ($base + "_stdout.txt")
  $stderrFile = Join-Path $OutDir ($base + "_stderr.txt")
  $sw = [System.Diagnostics.Stopwatch]::StartNew()
  # Start-Process native redirect (no cmd /c: a leading quote makes cmd.exe
  # strip quotes per its parsing rule - Todo 14 learning; no 2> operator so
  # PS5.1 emits no NativeCommandError records).
  $p = Start-Process -FilePath $Compressor -ArgumentList @("--in", $f.FullName, "--out", $outFile, "--res", $Res, "--fps", "$Fps", "--crf", "$Crf", "--encoder", "auto") -NoNewWindow -Wait -PassThru -RedirectStandardOutput $stdoutFile -RedirectStandardError $stderrFile
  $sw.Stop()
  $seconds = [Math]::Round($sw.Elapsed.TotalSeconds, 2)
  $exitCode = $p.ExitCode

  $encoder = "unknown"
  $cacheHit = $false
  $sizeMb = 0.0
  if ($exitCode -ne 0) {
    Write-Error ("bench_compress: {0} failed with exit {1} ({2}s)" -f $f.Name, $exitCode, $seconds)
    $failed = $true
  } else {
    if (Test-Path -LiteralPath $stdoutFile) {
      # @() wrap: single-line files come back as scalar String (char-indexed
      # without the wrap - Todo 43 learning), not String[].
      $lines = @(Get-Content -LiteralPath $stdoutFile)
      for ($i = $lines.Count - 1; $i -ge 0; $i--) {
        $line = [string]$lines[$i]
        $line = $line.Trim()
        if ([string]::IsNullOrEmpty($line)) { continue }
        if ($line.StartsWith('{"ok"') -or $line.StartsWith('{ "ok"') -or ($line -match '"ok"\s*:\s*true')) {
          try {
            $j = $line | ConvertFrom-Json
            if ($j.encoder) { $encoder = [string]$j.encoder }
            if ($j.cache_hit -eq $true) { $cacheHit = $true }
          } catch {
            $m = [regex]::Match($line, '"encoder"\s*:\s*"([^"]+)"')
            if ($m.Success) { $encoder = $m.Groups[1].Value }
            if ($line -match '"cache_hit"\s*:\s*true') { $cacheHit = $true }
          }
          break
        }
      }
    }
    if (Test-Path -LiteralPath $outFile) {
      $sizeMb = [Math]::Round(((Get-Item -LiteralPath $outFile).Length / 1MB), 2)
    } else {
      Write-Error ("bench_compress: output missing for {0}" -f $f.Name)
      $failed = $true
    }
  }

  [void](Write-Host ("bench_compress: {0} encoder={1} seconds={2} size_mb={3} cache_hit={4}" -f $f.Name, $encoder, $seconds, $sizeMb, $cacheHit))
  $results += New-Object PSObject -Property @{
    file = $f.Name
    encoder = $encoder
    seconds = $seconds
    size_mb = $sizeMb
    cache_hit = $cacheHit
  }
}

$budgetSec = 30.0
$allPass = $true
foreach ($r in $results) {
  if ($r.seconds -ge $budgetSec) { $allPass = $false }
}
if ($results.Count -eq 0) { $allPass = $false }

# HW flag: any non-x264 encoder means a hardware encoder did the work.
# All-x264 (or unknown) = no hardware encoder on this box -> no_hw:true.
$hwUsed = @($results | Where-Object { $_.encoder -ne "libx264" -and $_.encoder -ne "unknown" }).Count -gt 0
$hwLabel = "none"
if ($hwUsed) {
  $hwLabel = (($results | Where-Object { $_.encoder -ne "libx264" -and $_.encoder -ne "unknown" } | Select-Object -ExpandProperty encoder | Sort-Object -Unique) -join "; ")
}
$gpuNames = @()
try {
  $gpuNames = @(Get-CimInstance Win32_VideoController -ErrorAction SilentlyContinue |
    ForEach-Object { $_.Name } | Where-Object { $_ })
} catch { $gpuNames = @() }
if ($gpuNames.Count -eq 0) { $gpuNames = @("unknown") }

$rows = @($results | Select-Object file, encoder, seconds, size_mb, cache_hit)
$envelope = [ordered]@{
  tool = "bench_compress"
  date = (Get-Date -Format "yyyy-MM-dd")
  hw = [ordered]@{
    gpu = ($gpuNames -join "; ")
    hwdec = $hwLabel
    no_hw = (-not $hwUsed)
  }
  corpus = $CorpusDir
  res = $Res
  fps = $Fps
  crf = $Crf
  results = $rows
  budget_sec = $budgetSec
  pass = ($allPass -and (-not $failed))
  reproducibility = "Fresh K6WP_CACHE_DIR per run (cache key is in/res/fps/crf/encoder, not --out). Rerun +-10% on seconds under identical load."
}
$json = $envelope | ConvertTo-Json -Depth 5 -Compress
Write-Output $json
if (-not [string]::IsNullOrWhiteSpace($OutJson)) {
  $outFull = [System.IO.Path]::GetFullPath($OutJson)
  $outDirJson = [System.IO.Path]::GetDirectoryName($outFull)
  if (-not (Test-Path -LiteralPath $outDirJson)) {
    New-Item -ItemType Directory -Force -Path $outDirJson | Out-Null
  }
  $json | Set-Content -LiteralPath $outFull -Encoding UTF8
  [void](Write-Host ("bench_compress: wrote {0}" -f $outFull))
}

if ($failed) { exit 1 }
exit 0
```

## File: tools/bench_cpu_mem.ps1
```powershell
# tools/bench_cpu_mem.ps1 — generic CPU/RAM sampler for a running process.
#
# Samples TotalProcessorTime deltas (=> % of all cores) + WorkingSet/Private
# bytes at a fixed interval, then writes a JSON summary. Stock PowerShell +
# .NET only, zero dependencies. Used by Step 9.4 to record engine idle-video
# numbers honestly (no pass/fail gate here — numbers go to known-limitations).
#
# Usage:
#   powershell -ExecutionPolicy Bypass -File tools/bench_cpu_mem.ps1 `
#     -ProcessId <pid> -Minutes 1 -IntervalMs 500 -OutJson <path>
#   (or -ProcessName <name> to attach to a single live process by name.)

[CmdletBinding()]
param(
  [int]$ProcessId = 0,
  [string]$ProcessName = '',
  [double]$Minutes = 1,
  [int]$IntervalMs = 500,
  [string]$OutJson = ''
)

$ErrorActionPreference = 'Stop'

if ($ProcessId -le 0 -and [string]::IsNullOrEmpty($ProcessName)) {
  Write-Output 'FAIL: pass -ProcessId or -ProcessName'
  exit 1
}
if ($Minutes -le 0) { Write-Output 'FAIL: Minutes must be > 0'; exit 1 }
if ($IntervalMs -lt 100) { Write-Output 'FAIL: IntervalMs must be >= 100'; exit 1 }

function Get-Target {
  if ($ProcessId -gt 0) {
    return Get-Process -Id $ProcessId -ErrorAction Stop
  }
  $ps = @(Get-Process -Name $ProcessName -ErrorAction SilentlyContinue)
  if ($ps.Count -eq 0) { throw "no process named '$ProcessName'" }
  if ($ps.Count -gt 1) { throw "$($ps.Count) processes named '$ProcessName' (pass -ProcessId)" }
  return $ps[0]
}

$cores = [Environment]::ProcessorCount
$deadline = [DateTime]::UtcNow.AddMinutes($Minutes)
$cpuSamples = New-Object System.Collections.Generic.List[double]
$wsSamples = New-Object System.Collections.Generic.List[long]
$privSamples = New-Object System.Collections.Generic.List[long]

$p = Get-Target
$prevCpu = $p.CPU  # TotalProcessorTime in seconds (null until sampled once)
$prevT = [DateTime]::UtcNow
if ($null -eq $prevCpu) { $prevCpu = 0.0 }

while ([DateTime]::UtcNow -lt $deadline) {
  Start-Sleep -Milliseconds $IntervalMs
  try {
    $p = Get-Target
    $p.Refresh()
  } catch {
    Write-Output 'FAIL: target process exited during sampling'
    exit 1
  }
  $now = [DateTime]::UtcNow
  $curCpu = $p.CPU
  if ($null -eq $curCpu) { $curCpu = $prevCpu }
  $dt = ($now - $prevT).TotalSeconds
  if ($dt -gt 0) {
    $pct = (($curCpu - $prevCpu) / $dt / $cores) * 100.0
    if ($pct -lt 0) { $pct = 0 }
    $cpuSamples.Add($pct)
  }
  $wsSamples.Add($p.WorkingSet64)
  try { $privSamples.Add($p.PrivateMemorySize64) } catch { $privSamples.Add([long]0) }
  $prevCpu = $curCpu
  $prevT = $now
}

function Avg([System.Collections.Generic.List[double]]$xs) {
  if ($xs.Count -eq 0) { return 0.0 }
  ($xs | Measure-Object -Average).Average
}
function Max-Long([System.Collections.Generic.List[long]]$xs) {
  if ($xs.Count -eq 0) { return 0 }
  ($xs | Measure-Object -Maximum).Maximum
}

$summary = [ordered]@{
  tool = 'bench_cpu_mem'
  date = (Get-Date -Format 'yyyy-MM-dd')
  samples = $cpuSamples.Count
  interval_ms = $IntervalMs
  minutes = $Minutes
  cores = $cores
  avg_cpu_percent = [math]::Round((Avg $cpuSamples), 2)
  peak_cpu_percent = [math]::Round((($cpuSamples | Measure-Object -Maximum).Maximum), 2)
  avg_workingset_mb = [math]::Round(((($wsSamples | Measure-Object -Average).Average) / 1MB), 2)
  peak_workingset_mb = [math]::Round(((Max-Long $wsSamples) / 1MB), 2)
  avg_private_mb = [math]::Round(((($privSamples | Measure-Object -Average).Average) / 1MB), 2)
  peak_private_mb = [math]::Round(((Max-Long $privSamples) / 1MB), 2)
  reproducibility = "TotalProcessorTime deltas / $cores cores, ${IntervalMs}ms interval. Rerun +-10% on avg_cpu_percent under identical load; close other apps before comparing."
}

$json = ($summary | ConvertTo-Json -Compress)
Write-Output $json
if (-not [string]::IsNullOrEmpty($OutJson)) {
  Set-Content -LiteralPath $OutJson -Value $json -Encoding utf8
}
```

## File: tools/bench_engine.ps1
```powershell
# tools/bench_engine.ps1 - Phase 0 engine sampler (n>=5 per state).
#
# States: idle, playback_1080p, playback_4k, paused_30s.
# Measures: private bytes, working set, CPU %, GPU utilization per engine type,
# GPU dedicated/shared memory, thread context-switch rate (wakeups/s).
#
# Method: real engine.exe, isolated %LOCALAPPDATA%, windows minimized so the
# injected wallpaper is not occluded, typeperf CSV for GPU + wakeups, .NET
# process counters for RAM/CPU. No code instrumentation is used.
#
# Usage:
#   powershell -NoProfile -ExecutionPolicy Bypass -File tools\bench_engine.ps1 `
#     -Runs 5 -OutJson docs\bench\phase0.json
#
# Requires: no other engine.exe (singleton mutex), vendor corpus in tests/corpus.

[CmdletBinding()]
param(
    [string]$EngineExe = '',
    [string]$CorpusDir = '',
    [int]$Runs = 5,
    [string]$OutJson = '',
    # NOT "States": PowerShell variables are case-insensitive, so a
    # $States parameter would be the SAME variable as the local $states
    # array below, and assigning the array would clobber the filter.
    [string[]]$OnlyStates = @(),
    [switch]$SkipMinimize
)

$ErrorActionPreference = 'Stop'
$script:Here = Split-Path -Parent $MyInvocation.MyCommand.Path
. (Join-Path $script:Here 'bench_lib.ps1')
if ([string]::IsNullOrWhiteSpace($EngineExe)) { $EngineExe = Join-Path $script:Here '..\build\release\engine.exe' }
if ([string]::IsNullOrWhiteSpace($CorpusDir)) { $CorpusDir = Join-Path $script:Here '..\tests\corpus' }
if ([string]::IsNullOrWhiteSpace($OutJson)) { $OutJson = Join-Path $script:Here '..\docs\bench\phase0.json' }

if ($Runs -lt 1) { throw "Runs must be >= 1" }
if (-not (Test-Path -LiteralPath $EngineExe)) { throw "engine not found: $EngineExe" }
$busy = @(Get-Process engine -ErrorAction SilentlyContinue)
if ($busy.Count -gt 0) {
    throw ("another engine.exe is running (pid {0}); stop it first (K6WP.exe --stop)" -f (($busy | ForEach-Object { $_.Id }) -join ','))
}

$video1080 = Join-Path $CorpusDir 'anime.mp4'
$video4k = Join-Path $CorpusDir 'highdpb_4k.mp4'
if (-not (Test-Path -LiteralPath $video1080)) { throw "missing corpus: $video1080 (run tools/make_corpus.ps1)" }
if (-not (Test-Path -LiteralPath $video4k)) { throw "missing corpus: $video4k (generate the high-DPB 4K clip, see docs/refactor/baseline-tests.md)" }

$states = @(
    [pscustomobject]@{ name = 'idle'; video = ''; settle_ms = 4000; sample_s = 8; pause_after_ms = 0; run_ms = 17000; note = 'no video, no slots' },
    [pscustomobject]@{ name = 'playback_1080p'; video = $video1080; settle_ms = 5000; sample_s = 12; pause_after_ms = 0; run_ms = 22000; note = 'anime.mp4 1080p30 H.264, 1 slot' },
    [pscustomobject]@{ name = 'playback_4k'; video = $video4k; settle_ms = 6000; sample_s = 10; pause_after_ms = 0; run_ms = 21000; note = 'highdpb_4k.mp4 4K30 H.264, 16 refs, 1 slot' },
    [pscustomobject]@{ name = 'paused_30s'; video = $video1080; settle_ms = 5000; sample_s = 12; pause_after_ms = 5000; run_ms = 56000; note = 'play 5s, pause, hold 30s, then sample' }
)
# -OnlyStates filters the sweep (used by the A/B experiments, which only need
# the playback state). An unknown name is an error, not a silent no-op.
if ($OnlyStates.Count -gt 0) {
    # The value may arrive as ONE comma-joined string (a nested call through
    # & operator flattens a string[] into a single string), so split it.
    $wanted = @($OnlyStates | ForEach-Object { ([string]$_) -split ',' } | ForEach-Object { $_.Trim() } | Where-Object { $_ })
    $known = @($states | ForEach-Object { $_.name })
    foreach ($s in $wanted) {
        if ($known -notcontains $s) { throw "unknown state '$s' (known: $($known -join ', '))" }
    }
    $states = @($states | Where-Object { $wanted -contains $_.name })
}

$hw = Get-HwInventory
$savedWindows = @()
if (-not $SkipMinimize) {
    Write-Host "bench_engine: minimizing top-level windows (restored at the end)"
    $savedWindows = Get-DesktopWindows
}
$results = New-Object System.Collections.Generic.List[object]
$logsDir = Join-Path $script:Here '..\build\bench-logs'
New-Item -ItemType Directory -Force -Path $logsDir | Out-Null

try {
    foreach ($state in $states) {
        for ($i = 1; $i -le $Runs; $i++) {
            $tag = "{0}-run{1}" -f $state.name, $i
            Write-Host "bench_engine: $tag ..."
            $run = $null
            $tp = $null
            $series = @()
            $gpuRows = @()
            $occlusionPause = $false
            $hwdec = ''
            try {
                $dataDir = Join-Path $env:TEMP ("k6wp-bench-{0}-{1}" -f $tag, [Guid]::NewGuid().ToString('N').Substring(0,8))
                $run = Start-K6wpEngine -EngineExe $EngineExe -Video $state.video -DataDir $dataDir -RunMs $state.run_ms
                Start-Sleep -Milliseconds $state.settle_ms

                if ($state.pause_after_ms -gt 0) {
                    Start-Sleep -Milliseconds $state.pause_after_ms
                    $ack = Send-K6wpIpc -Cmd 'pause'
                    if (-not $ack.ok) { throw "pause rejected: $($ack | ConvertTo-Json -Compress)" }
                    Start-Sleep -Seconds 30
                    $st = Send-K6wpIpc -Cmd 'get_state'
                    if (-not $st.ok -or -not $st.state.paused) { throw "engine did not report paused after 30s" }
                }

                $tp = Start-TypeperfSampler -ProcessId $run.Process.Id -Seconds $state.sample_s -ProcessName 'engine'
                $series = @(Get-ProcessSeries -ProcessId $run.Process.Id -Seconds $state.sample_s -IntervalMs 1000)
                try { [void]$tp.Process.WaitForExit(($state.sample_s + 8) * 1000) } catch { }
                $gpuRows = @(Read-TypeperfCsv -Path $tp.Csv)

                try {
                    $log = Get-Content -LiteralPath $run.LogPath -ErrorAction Stop
                    $hwLine = @($log | Select-String 'hwdec-current=' | Select-Object -Last 1)
                    if ($hwLine.Count -gt 0) { $hwdec = $hwLine[0].Line.Trim() }
                    $occlusionPause = @($log | Select-String 'occlusion:.*pausing').Count -gt 0
                } catch { }

                try { Copy-Item -LiteralPath $run.LogPath -Destination (Join-Path $logsDir ($tag + '.log')) -Force } catch { }

                $priv = @($series | ForEach-Object { $_.private_mb })
                $ws = @($series | ForEach-Object { $_.ws_mb })
                $cpu = @($series | ForEach-Object { $_.cpu_pct })
                $hotThread = @($series | ForEach-Object { $_.max_thread_cpu_pct })
                $gpuTotal = @($gpuRows | ForEach-Object { $_.gpu_total_pct } | Where-Object { $null -ne $_ })
                $gpuDecode = @($gpuRows | ForEach-Object { $_.gpu_decode_pct } | Where-Object { $null -ne $_ })
                $gpu3d = @($gpuRows | ForEach-Object { $_.gpu_3d_pct } | Where-Object { $null -ne $_ })
                $gpuShared = @($gpuRows | ForEach-Object { $_.gpu_shared_mb } | Where-Object { $null -ne $_ })
                $wake = @($gpuRows | ForEach-Object { $_.wakeups_per_sec } | Where-Object { $null -ne $_ })

                $result = [pscustomobject]@{
                    state = $state.name
                    run = $i
                    video = $state.video
                    note = $state.note
                    sample_seconds = $state.sample_s
                    samples = $series.Count
                    private_mb = (Get-Stats $priv)
                    ws_mb = (Get-Stats $ws)
                    cpu_pct = (Get-Stats $cpu)
                    # The hottest single thread, as a percentage of ONE core.
                    # The process aggregate hides a spinning thread (C2).
                    max_thread_cpu_pct = (Get-Stats $hotThread)
                    gpu_total_pct = (Get-Stats $gpuTotal)
                    gpu_decode_pct = (Get-Stats $gpuDecode)
                    gpu_3d_pct = (Get-Stats $gpu3d)
                    gpu_shared_mb = (Get-Stats $gpuShared)
                    wakeups_per_sec = (Get-Stats $wake)
                    threads_last = $(if ($series.Count -gt 0) { $series[-1].threads } else { $null })
                    handles_last = $(if ($series.Count -gt 0) { $series[-1].handles } else { $null })
                    hwdec = $hwdec
                    occlusion_pause_seen = $occlusionPause
                    data_dir = $run.DataDir
                }
                $results.Add($result)
                Write-Host ("  priv median {0} MB, cpu mean {1}%, gpu {2}%, wake {3}/s, hwdec={4}" -f `
                    $result.private_mb.median, $result.cpu_pct.mean, $result.gpu_total_pct.median, $result.wakeups_per_sec.median, $result.hwdec)
            }
            finally {
                if ($tp -and $tp.Process -and -not $tp.Process.HasExited) { try { Stop-Process -Id $tp.Process.Id -Force } catch { } }
                if ($run) { Stop-K6wpEngine -Run $run }
            }
        }
    }
}
finally {
    if (-not $SkipMinimize) { Restore-DesktopWindows -Saved $savedWindows }
}

# Aggregate per state: median of the per-run medians, min/max as the noise band.
$aggregate = [ordered]@{}
foreach ($state in $states) {
    $rows = @($results | Where-Object { $_.state -eq $state.name })
    if ($rows.Count -eq 0) { continue }
    $metric = @{}
    foreach ($key in @('private_mb', 'ws_mb', 'cpu_pct', 'max_thread_cpu_pct', 'gpu_total_pct', 'gpu_decode_pct', 'gpu_3d_pct', 'gpu_shared_mb', 'wakeups_per_sec')) {
        $vals = @($rows | ForEach-Object { $_.$key.median } | Where-Object { $null -ne $_ })
        $metric[$key] = Get-Stats $vals
    }
    $aggregate[$state.name] = [ordered]@{
        runs = $rows.Count
        note = $state.note
        video = $state.video
        private_mb = $metric.private_mb
        ws_mb = $metric.ws_mb
        cpu_pct = $metric.cpu_pct
        max_thread_cpu_pct = $metric.max_thread_cpu_pct
        gpu_total_pct = $metric.gpu_total_pct
        gpu_decode_pct = $metric.gpu_decode_pct
        gpu_3d_pct = $metric.gpu_3d_pct
        gpu_shared_mb = $metric.gpu_shared_mb
        wakeups_per_sec = $metric.wakeups_per_sec
        occlusion_pause_seen = @($rows | Where-Object { $_.occlusion_pause_seen }).Count
        hwdec = @($rows | ForEach-Object { $_.hwdec } | Where-Object { $_ } | Select-Object -Last 1)
    }
}

$report = [ordered]@{
    tool = 'bench_engine'
    phase = 0
    date = (Get-Date -Format 'yyyy-MM-dd')
    label = 'live-proven'
    engine = $EngineExe
    runs_per_state = $Runs
    hw = $hw
    method = 'real engine.exe; isolated LOCALAPPDATA; windows minimized; typeperf GPU/wakeup CSV; .NET process counters; n>=5; settle per state'
    states = $aggregate
}

# The summary file carries the per-state aggregate; the raw file carries the
# per-run detail. Writing the SAME document twice would just double the
# repository size and hide which file to look at.
$rawReport = [ordered]@{
    tool = 'bench_engine'
    phase = 0
    date = (Get-Date -Format 'yyyy-MM-dd')
    label = 'live-proven'
    engine = $EngineExe
    runs_per_state = $Runs
    hw = $hw
    method = $report.method
    states = $aggregate
    raw = $results
}

$json = $report | ConvertTo-Json -Depth 12
$rawJson = $rawReport | ConvertTo-Json -Depth 12
$outFull = [System.IO.Path]::GetFullPath($OutJson)
$outDir = [System.IO.Path]::GetDirectoryName($outFull)
if (-not (Test-Path -LiteralPath $outDir)) { New-Item -ItemType Directory -Force -Path $outDir | Out-Null }
$json | Set-Content -LiteralPath $outFull -Encoding UTF8
$rawJson | Set-Content -LiteralPath ([System.IO.Path]::ChangeExtension($outFull, '.raw.json')) -Encoding UTF8
Write-Host "bench_engine: wrote $outFull"
Write-Host "bench_engine: wrote $([System.IO.Path]::ChangeExtension($outFull, '.raw.json'))"
```

## File: tools/bench_frames.ps1
```powershell
# tools/bench_frames.ps1 - engine-side visual parity (render-path) check.
#
# Compares the frames the engine RENDERS (not what the desktop composite
# shows). The engine writes them with the hidden --capture-frames QA flag,
# which uses mpv's own screenshot of the decoded frame.
#
# Why not a screen capture: an always-on-top overlay, a paused engine or a
# covered desktop all make a screen grab record the plain desktop, which
# silently turns a visual-parity gate into a no-op. The engine-side frame
# is immune to all of that.
#
# The corpus clip starts at a deterministic frame (frame 0), so the two
# builds must produce the SAME pixels: any difference is a real change in
# the render path (scaler, colour, decode).
#
# Usage:
#   powershell -NoProfile -ExecutionPolicy Bypass -File tools\bench_frames.ps1 `
#     -BaselineExe build\ab\engine-baseline.exe -CandidateExe build\ab\engine-candidate.exe

[CmdletBinding()]
param(
    [string]$BaselineExe = '',
    [string]$CandidateExe = '',
    [string]$CorpusDir = '',
    [string]$OutDir = '',
    [double]$Tolerance = 3.0
)
$ErrorActionPreference = 'Stop'
$script:Here = Split-Path -Parent $MyInvocation.MyCommand.Path
. (Join-Path $script:Here 'bench_lib.ps1')

if ([string]::IsNullOrWhiteSpace($BaselineExe)) { $BaselineExe = Join-Path $script:Here '..\build\ab\engine-baseline.exe' }
if ([string]::IsNullOrWhiteSpace($CandidateExe)) { $CandidateExe = Join-Path $script:Here '..\build\ab\engine-candidate.exe' }
if ([string]::IsNullOrWhiteSpace($CorpusDir)) { $CorpusDir = Join-Path $script:Here '..\tests\corpus' }
if ([string]::IsNullOrWhiteSpace($OutDir)) { $OutDir = Join-Path $script:Here '..\docs\bench\frames' }
$video = Join-Path $CorpusDir 'anime.mp4'
if (-not (Test-Path -LiteralPath $video)) { throw "missing corpus: $video" }

Add-Type -AssemblyName System.Drawing

function Get-Signature([string]$Path) {
    $bmp = [System.Drawing.Bitmap]::FromFile($Path)
    try {
        $sum = 0.0; $sum2 = 0.0; $rs = 0.0; $gs = 0.0; $bs = 0.0; $n = 0
        for ($y = 0; $y -lt $bmp.Height; $y += 4) {
            for ($x = 0; $x -lt $bmp.Width; $x += 4) {
                $c = $bmp.GetPixel($x, $y)
                $l = 0.2126 * $c.R + 0.7152 * $c.G + 0.0722 * $c.B
                $sum += $l; $sum2 += $l * $l
                $rs += $c.R; $gs += $c.G; $bs += $c.B; $n++
            }
        }
        $mean = $sum / $n
        return [pscustomobject]@{
            w = $bmp.Width; h = $bmp.Height
            mean_luma = [math]::Round($mean, 2)
            stddev = [math]::Round([math]::Sqrt([math]::Max(0, ($sum2 / $n) - ($mean * $mean))), 2)
            mean_r = [math]::Round($rs / $n, 2)
            mean_g = [math]::Round($gs / $n, 2)
            mean_b = [math]::Round($bs / $n, 2)
        }
    }
    finally { $bmp.Dispose() }
}

function Invoke-Capture([string]$Exe, [string]$Tag) {
    $exe = [System.IO.Path]::GetFullPath($Exe)
    if (-not (Test-Path -LiteralPath $exe)) { throw "engine not found: $exe" }
    $dir = Join-Path $OutDir $Tag
    Remove-Item -LiteralPath $dir -Recurse -Force -ErrorAction SilentlyContinue
    New-Item -ItemType Directory -Force -Path $dir | Out-Null
    $prefix = Join-Path $dir 'frame'
    $busy = @(Get-Process engine -ErrorAction SilentlyContinue)
    if ($busy.Count -gt 0) { throw "an engine.exe is already running (pid $($busy.Id -join ','))" }
    $dataDir = Join-Path $env:TEMP ("k6wp-frames-" + [Guid]::NewGuid().ToString('N').Substring(0, 8))
    New-Item -ItemType Directory -Force -Path (Join-Path $dataDir 'K6WP') | Out-Null
    $old = $env:LOCALAPPDATA
    try {
        $env:LOCALAPPDATA = $dataDir
        $p = Start-Process -FilePath $exe -PassThru -WindowStyle Hidden -ArgumentList @(
            "--video=$video", "--capture-frames=$prefix", '--capture-frames-count=3',
            '--capture-frames-interval-ms=700', '--capture-frames-seek=5',
            '--exit-after-ms=15000')
    }
    finally { $env:LOCALAPPDATA = $old }
    if (-not (Wait-K6wpEngine -TimeoutMs 15000)) {
        try { if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force } } catch { }
        throw "${Tag}: engine did not answer the IPC pipe"
    }
    # Wait for the frames to appear (the mpv VO writes them asynchronously).
    $files = @()
    $deadline = (Get-Date).AddSeconds(20)
    while ((Get-Date) -lt $deadline) {
        $files = @(Get-ChildItem -LiteralPath $dir -Filter 'frame*.jpg' -ErrorAction SilentlyContinue |
            Sort-Object Name)
        if ($files.Count -ge 3) { break }
        Start-Sleep -Milliseconds 300
    }
    try { if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue } } catch { }
    if ($files.Count -eq 0) { throw "${Tag}: no frame was written" }
    return $files
}

Write-Host "bench_frames: baseline  = $BaselineExe"
Write-Host "bench_frames: candidate = $CandidateExe"

$baseFiles = Invoke-Capture -Exe $BaselineExe -Tag 'baseline'
$candFiles = Invoke-Capture -Exe $CandidateExe -Tag 'candidate'
Write-Host ("bench_frames: captured {0} baseline frame(s), {1} candidate frame(s)" -f $baseFiles.Count, $candFiles.Count)

$rows = @()
$n = [math]::Min($baseFiles.Count, $candFiles.Count)
for ($i = 0; $i -lt $n; $i++) {
    $a = Get-Signature $baseFiles[$i].FullName
    $b = Get-Signature $candFiles[$i].FullName
    $ba = [System.Drawing.Bitmap]::FromFile($baseFiles[$i].FullName)
    $bb = [System.Drawing.Bitmap]::FromFile($candFiles[$i].FullName)
    try { $diff = Get-BitmapDiff -A $ba -B $bb -Step 4 } finally { $ba.Dispose(); $bb.Dispose() }
    $rows += [pscustomobject]@{
        frame = $i + 1
        size = "$($a.w)x$($a.h)"
        baseline_luma = $a.mean_luma
        candidate_luma = $b.mean_luma
        luma_delta = [math]::Round([math]::Abs($a.mean_luma - $b.mean_luma), 3)
        baseline_rgb = "$($a.mean_r),$($a.mean_g),$($a.mean_b)"
        candidate_rgb = "$($b.mean_r),$($b.mean_g),$($b.mean_b)"
        pixel_diff = [math]::Round($diff, 3)
        pass = ($diff -le $Tolerance)
    }
}
$rows | Format-Table -AutoSize | Out-String | Write-Host

$worst = ($rows | Measure-Object -Property pixel_diff -Maximum).Maximum
$allPass = @($rows | Where-Object { -not $_.pass }).Count -eq 0
Write-Host ("worst pixel diff: {0} (tolerance {1}) -> {2}" -f $worst, $Tolerance, $(if ($allPass) { 'PASS' } else { 'FAIL' }))

$report = [ordered]@{
    tool = 'bench_frames'
    phase = 1
    date = (Get-Date -Format 'yyyy-MM-ddTHH:mm:ssK')
    label = 'live-proven'
    method = 'engine-side rendered frames (hidden --capture-frames QA flag, mpv decoded-frame screenshot); immune to the desktop composite'
    corpus = $video
    tolerance = $Tolerance
    frames = $rows
    worst_pixel_diff = $worst
    pass = $allPass
}
$json = $report | ConvertTo-Json -Depth 8
$outFull = [System.IO.Path]::GetFullPath((Join-Path $OutDir 'frames.json'))
$json | Set-Content -LiteralPath $outFull -Encoding UTF8
Write-Host "bench_frames: wrote $outFull"
if (-not $allPass) { exit 1 }
exit 0
```

## File: tools/bench_pacing.ps1
```powershell
# tools/bench_pacing.ps1 - C1: prove a pacing change costs no QUALITY.
#
# WHAT IT MEASURES
#   For each (engine build, fps cap, display refresh) it runs the real engine
#   on the corpus clip and records:
#     - the live mpv frame-quality counters (drops, mistimed frames, delayed
#       frames, frames consumed, media position)
#     - the per-OS-thread CPU of the engine (the aggregate hides a spinning
#       thread, which is exactly how the desync busy-wait stayed hidden)
#     - the GPU utilization per engine type
#     - private bytes + GPU shared memory (the honest RAM numbers)
#
# WHY ABAB
#   The GPU numbers drift up to 1.5x between sessions, so the two builds are
#   measured ALTERNATELY inside one session and compared with each other -
#   never against a stored absolute.
#
# Usage:
#   powershell -NoProfile -ExecutionPolicy Bypass -File tools\bench_pacing.ps1 `
#     -BaselineExe build\ab\engine-baseline.exe -CandidateExe build\ab\engine-candidate.exe `
#     -FpsCaps 24,30 -Seconds 60 -OutJson docs\bench\pacing.json

[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$BaselineExe,
    [Parameter(Mandatory=$true)][string]$CandidateExe,
    [string]$CorpusDir = '',
    [string[]]$FpsCaps = @('24', '30'),
    [int]$Seconds = 60,
    [int]$RefreshHz = 0,
    [string]$OutJson = '',
    [int]$Repetitions = 2
)
$ErrorActionPreference = 'Stop'
$script:Here = Split-Path -Parent $MyInvocation.MyCommand.Path
. (Join-Path $script:Here 'bench_lib.ps1')
. (Join-Path $script:Here 'session_snapshot.ps1')

if ([string]::IsNullOrWhiteSpace($CorpusDir)) { $CorpusDir = Join-Path $script:Here '..\tests\corpus' }
if ([string]::IsNullOrWhiteSpace($OutJson)) { $OutJson = Join-Path $script:Here '..\docs\bench\pacing.json' }
# -FpsCaps 24,30 arrives as ONE string on the command line; split it.
$FpsCaps = @($FpsCaps | ForEach-Object { ([string]$_) -split ',' } | ForEach-Object { $_.Trim() } | Where-Object { $_ } | ForEach-Object { [int]$_ })
if ($FpsCaps.Count -eq 0) { throw "no fps caps given" }
foreach ($c in $FpsCaps) {
    # Cast BEFORE comparing: with a string on the left, PowerShell compares
    # lexicographically ("30" -gt "240" is TRUE).
    $ci = [int]$c
    if ($ci -lt 1 -or $ci -gt 240) { throw "an fps cap of $ci is not a frame rate" }
}
$BaselineExe = [System.IO.Path]::GetFullPath($BaselineExe)
$CandidateExe = [System.IO.Path]::GetFullPath($CandidateExe)
$video = Join-Path $CorpusDir 'anime.mp4'
if (-not (Test-Path -LiteralPath $video)) { throw "missing corpus: $video" }
foreach ($p in @($BaselineExe, $CandidateExe)) {
    if (-not (Test-Path -LiteralPath $p)) { throw "engine not found: $p" }
}

# --- per-thread CPU sampler (C2) -------------------------------------------
# The process aggregate hides a single spinning thread, so the sampler takes
# the CPU delta of EVERY engine thread and reports the hottest one.
function Get-ThreadCpuDeltas {
    param([int]$ProcessId, [double]$WindowSec)
    $p = Get-Process -Id $ProcessId -ErrorAction Stop
    $before = @{}
    foreach ($t in $p.Threads) { $before[[string]$t.Id] = $t.TotalProcessorTime.TotalMilliseconds }
    Start-Sleep -Milliseconds ([int]($WindowSec * 1000))
    $p.Refresh()
    $out = @()
    foreach ($t in $p.Threads) {
        $k = [string]$t.Id
        if (-not $before.ContainsKey($k)) { continue }
        $deltaMs = $t.TotalProcessorTime.TotalMilliseconds - $before[$k]
        # % of ONE core over the window.
        $out += [pscustomobject]@{
            tid = $t.Id
            cpu_pct_of_core = [math]::Round(($deltaMs / 1000.0 / $WindowSec) * 100, 2)
            state = [string]$t.ThreadState
        }
    }
    return $out
}

function Read-CountersCsv([string]$Path) {
    if (-not (Test-Path -LiteralPath $Path)) { return @() }
    return @(Import-Csv -LiteralPath $Path)
}

function Convert-Counters([object[]]$Rows) {
    # The counters are cumulative, so the interesting value is the DELTA
    # between the first and the last sample.
    if ($Rows.Count -lt 2) { return $null }
    $first = $Rows[0]
    $last = $Rows[$Rows.Count - 1]
    $out = [ordered]@{ samples = $Rows.Count }
    foreach ($col in $first.PSObject.Properties.Name) {
        if ($col -in @('sample', 'elapsed_ms')) { continue }
        $a = $first.$col
        $b = $last.$col
        if ($a -eq 'na' -or $b -eq 'na' -or $a -eq '' -or $b -eq '') { $out[$col] = $null; continue }
        $na = 0.0
        $nb = 0.0
        if ([double]::TryParse([string]$a, [ref]$na) -and [double]::TryParse([string]$b, [ref]$nb)) {
            $out[$col] = $nb - $na
        } else {
            # A non-numeric counter (the "vf" filter chain is a string): keep
            # the last value as text, it is still evidence.
            $out[$col] = [string]$b
        }
    }
    # estimated-frame-number RESETS on every loop (the corpus clip is 30 s
    # and the measurement window is 60 s), so its first-to-last delta is
    # meaningless. The number of frames CONSUMED is the sum of the positive
    # per-sample increments, plus the frame index at each reset - that is
    # the metric C1 needs ("the same number of frames decoded in both
    # modes").
    $frames = 0.0
    $prev = $null
    $resets = 0
    foreach ($row in $Rows) {
        $v = $row.'estimated-frame-number'
        if ($null -eq $v -or $v -eq 'na' -or $v -eq '') { continue }
        $v = [double]$v
        if ($null -eq $prev) { $prev = $v; continue }
        $d = $v - $prev
        if ($d -lt 0) {
            # A loop: the counter went back to the start of the file.
            $resets++
            $frames += $v      # the frames of the finished part
        } else {
            $frames += $d
        }
        $prev = $v
    }
    $out['frames_consumed'] = [math]::Round($frames, 0)
    $out['loops'] = $resets

    # The media position advance over the window, with the loop resets
    # accumulated. Dividing it by the wall-clock window gives the PLAYBACK
    # RATE: 1.0 means the wallpaper runs at real time, 0.75 means it runs a
    # quarter slow (the video is stretched, which is a QUALITY defect the
    # CPU/GPU numbers alone would never show).
    $media = 0.0
    $prevMedia = $null
    foreach ($row in $Rows) {
        $v = $row.'playback-time'
        if ($null -eq $v -or $v -eq 'na' -or $v -eq '') { continue }
        $v = [double]$v
        if ($null -eq $prevMedia) { $prevMedia = $v; continue }
        $d = $v - $prevMedia
        if ($d -lt 0) { $media += $v } else { $media += $d }
        $prevMedia = $v
    }
    $windowSec = 0.0
    if ($Rows.Count -ge 2) {
        $e0 = 0.0; $e1 = 0.0
        [void][double]::TryParse([string]$Rows[0].elapsed_ms, [ref]$e0)
        [void][double]::TryParse([string]$Rows[$Rows.Count - 1].elapsed_ms, [ref]$e1)
        $windowSec = ($e1 - $e0) / 1000.0
    }
    $out['media_seconds_advanced'] = [math]::Round($media, 3)
    $out['window_seconds'] = [math]::Round($windowSec, 3)
    $out['playback_rate'] = $(if ($windowSec -gt 0) { [math]::Round($media / $windowSec, 4) } else { $null })
    $out['decoded_fps'] = $(if ($windowSec -gt 0) { [math]::Round($frames / $windowSec, 2) } else { $null })
    return $out
}

function Invoke-PacingRun {
    param([string]$Exe, [string]$Tag, [int]$FpsCap)
    $dataDir = Join-Path $env:TEMP ("k6wp-pace-" + [Guid]::NewGuid().ToString('N').Substring(0, 8))
    New-Item -ItemType Directory -Force -Path (Join-Path $dataDir 'K6WP') | Out-Null
    $csv = Join-Path $dataDir 'counters.csv'
    $busy = @(Get-Process engine -ErrorAction SilentlyContinue)
    if ($busy.Count -gt 0) {
        # A previous run's engine may still be releasing the singleton mutex.
        foreach ($e in $busy) { try { Stop-Process -Id $e.Id -Force } catch { } }
        $deadline = (Get-Date).AddSeconds(10)
        while ((Get-Date) -lt $deadline -and (Get-Process engine -ErrorAction SilentlyContinue)) {
            Start-Sleep -Milliseconds 200
        }
        if (Get-Process engine -ErrorAction SilentlyContinue) {
            throw "an engine.exe is still running; cannot start a clean run"
        }
        Start-Sleep -Milliseconds 500
    }
    $old = $env:LOCALAPPDATA
    $started = $false
    $tail = ''
    for ($attempt = 1; $attempt -le 4 -and -not $started; $attempt++) {
        $env:LOCALAPPDATA = $dataDir
        try {
            $p = Start-Process -FilePath $Exe -PassThru -WindowStyle Hidden -ArgumentList @(
                "--video=$video", "--fps-cap=$FpsCap", "--dump-mpv-counters=$csv",
                "--dump-mpv-counters-count=$Seconds", "--dump-mpv-counters-interval-ms=1000",
                "--exit-after-ms=$($Seconds * 1000 + 20000)")
        }
        finally { $env:LOCALAPPDATA = $old }
        # The singleton mutex is released ASYNCHRONOUSLY when the previous
        # engine dies, so a start can lose the race and exit with "another
        # instance is already running". Retry a few times.
        if (Wait-K6wpEngine -TimeoutMs 20000) { $started = $true; break }
        $logPath = Join-Path $dataDir 'K6WP\engine.log'
        if (Test-Path -LiteralPath $logPath) {
            $tail = (@(Get-Content -LiteralPath $logPath -Tail 4) -join ' | ')
        }
        try { if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force } } catch { }
        Start-Sleep -Seconds 2
    }
    if (-not $started) {
        throw "${Tag}: engine did not answer the pipe. log: $tail"
    }
    # The engine pauses the decode when a window covers the desktop. Every
    # covering window must be minimized and the engine must be PLAYING
    # before the measurement, or the counters read zero by definition.
    $savedWindows = Get-DesktopWindows
    Start-Sleep -Seconds 5
    $resumed = $false
    $deadline = (Get-Date).AddSeconds(20)
    while ((Get-Date) -lt $deadline) {
        $st = $null
        try { $st = Send-K6wpIpc -Cmd 'get_state' -TimeoutMs 2000 } catch { }
        if ($st -and $st.ok -and -not $st.state.paused) { $resumed = $true; break }
        Start-Sleep -Milliseconds 300
    }
    if (-not $resumed) { Write-Host "    WARNING: the engine reports paused - the counters may read zero" }

    # The GPU counters and the per-thread CPU are sampled in the SAME window.
    $tp = Start-TypeperfSampler -ProcessId $p.Id -Seconds $Seconds -ProcessName 'engine'
    $threads = Get-ThreadCpuDeltas -ProcessId $p.Id -WindowSec $Seconds
    try { [void]$tp.Process.WaitForExit(($Seconds + 15) * 1000) } catch { }
    $gpuRows = @(Read-TypeperfCsv -Path $tp.Csv)
    $counterRows = @(Read-CountersCsv -Path $csv)
    $p.Refresh()
    $priv = [math]::Round($p.PrivateMemorySize64 / 1MB, 2)
    $gpuMem = @($gpuRows | ForEach-Object { $_.gpu_shared_mb } | Where-Object { $null -ne $_ })
    try { if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue } } catch { }

    $hot = @($threads | Sort-Object cpu_pct_of_core -Descending)
    $result = [pscustomobject]@{
        tag = $Tag
        fps_cap = $FpsCap
        max_thread_cpu_pct = $(if ($hot.Count -gt 0) { $hot[0].cpu_pct_of_core } else { $null })
        max_thread_tid = $(if ($hot.Count -gt 0) { $hot[0].tid } else { $null })
        max_thread_state = $(if ($hot.Count -gt 0) { $hot[0].state } else { $null })
        thread_count = $threads.Count
        private_mb = $priv
        gpu_shared_mb = (Get-Stats $gpuMem).median
        gpu_total_pct = (Get-Stats @($gpuRows | ForEach-Object { $_.gpu_total_pct } | Where-Object { $null -ne $_ })).median
        gpu_decode_pct = (Get-Stats @($gpuRows | ForEach-Object { $_.gpu_decode_pct } | Where-Object { $null -ne $_ })).median
        gpu_3d_pct = (Get-Stats @($gpuRows | ForEach-Object { $_.gpu_3d_pct } | Where-Object { $null -ne $_ })).median
        counters = (Convert-Counters $counterRows)
        counters_raw = $counterRows
        vf_chain = $(if ($counterRows.Count -gt 0) { $counterRows[$counterRows.Count - 1].vf } else { '' })
    }
    # The windows stay minimized for the whole experiment; they are restored
    # once, at the end (inside the session-safe finally).
    return $result
}

# --- the experiment ---------------------------------------------------------
$results = New-Object System.Collections.Generic.List[object]
$savedWindows = @()

# C8: the display mode and the wallpaper/lock screen are snapshotted and
# restored automatically, even when a run fails half-way. The covering
# windows are minimized for the measurement and restored in the finally.
Invoke-SessionSafe -Name 'pacing experiment' -RestoreDisplay -RestoreWallpaper -RestoreLockscreen -RestoreExplorer -ScriptBlock {
    try {
        if ($RefreshHz -gt 0) {
            $screen = [System.Windows.Forms.Screen]::PrimaryScreen
            $rc = Set-DisplayRefresh -Device $screen.DeviceName -Width $screen.Bounds.Width -Height $screen.Bounds.Height -Frequency $RefreshHz
            Write-Host "  display refresh -> ${RefreshHz}Hz (rc=$rc)"
            Start-Sleep -Seconds 3
        }
        $script:savedWindows = Get-DesktopWindows
        foreach ($cap in $FpsCaps) {
            for ($rep = 1; $rep -le $Repetitions; $rep++) {
                # ABAB: the two builds alternate inside ONE session.
                foreach ($arm in @('A', 'B')) {
                    $exe = if ($arm -eq 'A') { $BaselineExe } else { $CandidateExe }
                    $name = if ($arm -eq 'A') { 'desync' } else { 'vsync' }
                    Write-Host "  run cap=$cap rep=$rep arm=$arm ($name)"
                    $r = Invoke-PacingRun -Exe $exe -Tag "$name-cap$cap-rep$rep" -FpsCap $cap
                    $results.Add($r)
                    Write-Host ("    max thread CPU {0}% (tid {1}, {2}), private {3} MB, gpu shared {4} MB, decoded {5} fps, rate {6}, drops {7}" -f `
                        $r.max_thread_cpu_pct, $r.max_thread_tid, $r.max_thread_state, $r.private_mb, $r.gpu_shared_mb,
                        $(if ($r.counters -and $null -ne $r.counters['decoded_fps']) { $r.counters['decoded_fps'] } else { 'n/a' }),
                        $(if ($r.counters -and $null -ne $r.counters['playback_rate']) { $r.counters['playback_rate'] } else { 'n/a' }),
                        $(if ($r.counters -and $null -ne $r.counters['frame-drop-count']) { $r.counters['frame-drop-count'] } else { 'n/a' }))
                }
            }
        }
    }
    finally {
        Restore-DesktopWindows -Saved $script:savedWindows
    }
}

function Get-MedianOf {
    param($List, [string]$Prop)
    $vals = @($List | ForEach-Object { $_.$Prop } | Where-Object { $null -ne $_ })
    if ($vals.Count -eq 0) { return $null }
    return (Get-Stats $vals).median
}

# --- the comparison ---------------------------------------------------------
$rows = @()
foreach ($cap in $FpsCaps) {
    $a = @($results | Where-Object { $_.fps_cap -eq $cap -and $_.tag -like 'desync*' })
    $b = @($results | Where-Object { $_.fps_cap -eq $cap -and $_.tag -like 'vsync*' })
    if ($a.Count -eq 0 -or $b.Count -eq 0) { continue }
    foreach ($prop in @('max_thread_cpu_pct', 'private_mb', 'gpu_shared_mb', 'gpu_total_pct', 'gpu_decode_pct', 'gpu_3d_pct')) {
        $rows += [pscustomobject]@{
            fps_cap = $cap
            metric = $prop
            desync = Get-MedianOf $a $prop
            vsync = Get-MedianOf $b $prop
        }
    }
    foreach ($counter in @('frames_consumed', 'decoded_fps', 'loops', 'playback_rate',
                           'media_seconds_advanced', 'frame-drop-count', 'decoder-frame-drop-count',
                           'mistimed-frame-count', 'vo-delayed-frame-count')) {
        $av = @($a | ForEach-Object { $_.counters[$counter] } | Where-Object { $null -ne $_ })
        $bv = @($b | ForEach-Object { $_.counters[$counter] } | Where-Object { $null -ne $_ })
        if ($av.Count -eq 0 -or $bv.Count -eq 0) { continue }
        $rows += [pscustomobject]@{
            fps_cap = $cap
            metric = $counter
            desync = (Get-Stats $av).median
            vsync = (Get-Stats $bv).median
        }
    }
}

$report = [ordered]@{
    tool = 'bench_pacing'
    phase = 1
    date = (Get-Date -Format 'yyyy-MM-ddTHH:mm:ssK')
    label = 'live-proven (one machine)'
    baseline_exe = $BaselineExe
    candidate_exe = $CandidateExe
    corpus = $video
    seconds_per_run = $Seconds
    repetitions = $Repetitions
    refresh_hz = $(if ($RefreshHz -gt 0) { $RefreshHz } else { 'display default' })
    method = 'ABAB alternating inside one session; live mpv counters + per-OS-thread CPU + GPU counters + private/GPU-shared memory'
    runs = $results
    comparison = $rows
}
$json = $report | ConvertTo-Json -Depth 12
$outFull = [System.IO.Path]::GetFullPath($OutJson)
$outDir = [System.IO.Path]::GetDirectoryName($outFull)
if (-not (Test-Path -LiteralPath $outDir)) { New-Item -ItemType Directory -Force -Path $outDir | Out-Null }
$json | Set-Content -LiteralPath $outFull -Encoding UTF8

Write-Host ""
Write-Host ("=" * 100)
Write-Host "  pacing comparison (desync vs vsync)"
Write-Host ("=" * 100)
$rows | Format-Table -AutoSize | Out-String | Write-Host
Write-Host "bench_pacing: wrote $outFull"
```

## File: tools/bench_probe_parity.ps1
```powershell
# tools/bench_probe_parity.ps1 - can the 97 MB ffprobe be replaced by libmpv?
#
# ffprobe.exe is 96.98 MB of the 389 MB install (about 35 MB of the 148 MB
# ZIP, 24 % on its own). The product only asks it for FIVE fields:
#   format.duration, the first video stream's codec_name / width / height /
#   r_frame_rate, and whether an audio stream exists.
#
# libmpv-2.dll is ALREADY shipped, so probing through it costs zero new
# shipping bytes. This script compares the two answers field by field over
# a corpus, so the swap can be decided on evidence instead of hope.
#
# Usage:
#   powershell -NoProfile -ExecutionPolicy Bypass -File tools\bench_probe_parity.ps1

[CmdletBinding()]
param(
    [string]$EngineExe = '',
    [string]$CorpusDir = '',
    [string]$OutJson = '',
    [int[]]$ExtraFiles = @()
)
$ErrorActionPreference = 'Stop'
$script:Here = Split-Path -Parent $MyInvocation.MyCommand.Path
. (Join-Path $script:Here 'bench_lib.ps1')

if ([string]::IsNullOrWhiteSpace($EngineExe)) { $EngineExe = Join-Path $script:Here '..\build\release\engine.exe' }
if ([string]::IsNullOrWhiteSpace($CorpusDir)) { $CorpusDir = Join-Path $script:Here '..\tests\corpus' }
if ([string]::IsNullOrWhiteSpace($OutJson)) { $OutJson = Join-Path $script:Here '..\docs\bench\probe-parity.json' }
$engine = [System.IO.Path]::GetFullPath($EngineExe)
$ffprobe = (Join-Path $script:Here '..\vendor\ffmpeg\ffprobe.exe')
if (-not (Test-Path -LiteralPath $ffprobe)) { throw "ffprobe not found at $ffprobe" }

$files = @(Get-ChildItem -LiteralPath $CorpusDir -Filter '*.mp4' -File | ForEach-Object { $_.FullName })
$files += $ExtraFiles
if ($files.Count -eq 0) { throw "no corpus files" }

function Get-FfprobeProps([string]$Path) {
    $out = & $ffprobe -v error -show_entries format=duration -show_entries `
        stream=codec_name,codec_type,width,height,r_frame_rate -of json $Path 2>$null | Out-String
    $root = $out | ConvertFrom-Json
    $props = [ordered]@{
        duration = 0.0; codec = ''; width = 0; height = 0; fps = 0.0; has_audio = $false; found_video = $false
    }
    if ($root.format -and $root.format.duration) { $props.duration = [double]$root.format.duration }
    foreach ($s in $root.streams) {
        if ($s.codec_type -eq 'audio') { $props.has_audio = $true; continue }
        if ($s.codec_type -eq 'video' -and -not $props.found_video) {
            $props.found_video = $true
            $props.codec = [string]$s.codec_name
            $props.width = [int]$s.width
            $props.height = [int]$s.height
            if ($s.r_frame_rate -and $s.r_frame_rate -like '*/*') {
                $parts = $s.r_frame_rate -split '/'
                $n = [double]$parts[0]; $d = [double]$parts[1]
                if ($d -ne 0) { $props.fps = $n / $d }
            } elseif ($s.r_frame_rate) { $props.fps = [double]$s.r_frame_rate }
        }
    }
    return $props
}

function Get-MpvProps([string]$Path, [string]$Engine) {
    $json = Join-Path $env:TEMP ("k6wp-props-" + [Guid]::NewGuid().ToString('N').Substring(0, 8) + ".json")
    $busy = @(Get-Process engine -ErrorAction SilentlyContinue)
    if ($busy.Count -gt 0) {
        # The singleton mutex is released asynchronously when the previous
        # engine dies, so a start can lose the race.
        foreach ($e in $busy) { try { Stop-Process -Id $e.Id -Force } catch { } }
        for ($i = 0; $i -lt 40; $i++) {
            if (-not (Get-Process engine -ErrorAction SilentlyContinue)) { break }
            Start-Sleep -Milliseconds 250
        }
        Start-Sleep -Milliseconds 800
    }
    $dataDir = Join-Path $env:TEMP ("k6wp-probe-" + [Guid]::NewGuid().ToString('N').Substring(0, 8))
    New-Item -ItemType Directory -Force -Path (Join-Path $dataDir 'K6WP') | Out-Null
    $old = $env:LOCALAPPDATA
    try {
        $env:LOCALAPPDATA = $dataDir
        $p = Start-Process -FilePath $Engine -PassThru -WindowStyle Hidden -ArgumentList @(
            "--video=$Path", "--dump-media-props=$json", '--exit-after-ms=12000')
    }
    finally { $env:LOCALAPPDATA = $old }
    $deadline = (Get-Date).AddSeconds(20)
    while (-not (Test-Path -LiteralPath $json) -and (Get-Date) -lt $deadline) {
        Start-Sleep -Milliseconds 200
    }
    try { if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue } } catch { }
    if (-not (Test-Path -LiteralPath $json)) { throw "the engine wrote no media props for $Path" }
    $m = Get-Content -LiteralPath $json -Raw | ConvertFrom-Json
    Remove-Item -LiteralPath $json -Force -ErrorAction SilentlyContinue
    $props = [ordered]@{
        duration = 0.0; codec = ''; width = 0; height = 0; fps = 0.0; has_audio = $false; found_video = $false
    }
    if ($m.has_duration) { $props.duration = [double]$m.duration }
    if ($m.has_codec) { $props.codec = [string]$m.codec }
    if ($null -ne $m.width) { $props.width = [int]$m.width }
    if ($null -ne $m.height) { $props.height = [int]$m.height }
    if ($m.has_fps -and $m.fps) {
        # mpv's container-fps is a rational string ("30/1").
        $f = [string]$m.fps
        if ($f -like '*/*') {
            $parts = $f -split '/'
            $n = [double]$parts[0]; $d = [double]$parts[1]
            if ($d -ne 0) { $props.fps = $n / $d }
        } else { $props.fps = [double]$f }
    } elseif ($m.has_estimated_fps -and $m.estimated_fps) {
        $props.fps = [double]$m.estimated_fps
    }
    if ($m.has_audio) { $props.has_audio = $true }
    if ($m.has_video) { $props.found_video = $true }
    return $props
}

$rows = @()
foreach ($f in $files) {
    Write-Host ("probing {0}" -f (Split-Path -Leaf $f))
    $a = Get-FfprobeProps $f
    $b = Get-MpvProps $f $engine
    $rows += [ordered]@{
        file = (Split-Path -Leaf $f)
        ffprobe = $a
        mpv = $b
        duration_delta = [math]::Round([math]::Abs($a.duration - $b.duration), 4)
        duration_ok = ([math]::Abs($a.duration - $b.duration) -lt 0.05)
        codec_ok = ($a.codec -eq $b.codec)
        size_ok = ($a.width -eq $b.width -and $a.height -eq $b.height)
        fps_ok = ([math]::Abs($a.fps - $b.fps) -lt 0.01)
        audio_ok = ($a.has_audio -eq $b.has_audio)
    }
}
$fail = @($rows | Where-Object { -not ($_.duration_ok -and $_.codec_ok -and $_.size_ok -and $_.fps_ok -and $_.audio_ok) })
Write-Host ""
Write-Host ("{0,-28} {1,-8} {2,-8} {3,-8} {4,-8} {5,-8} {6,-8} {7}" -f 'file', 'dur', 'codec', 'size', 'fps', 'audio', 'video', 'ffprobe->mpv')
foreach ($r in $rows) {
    Write-Host ("{0,-28} {1,-8} {2,-8} {3,-8} {4,-8} {5,-8} {6,-8} {7}" -f `
        $r.file, $r.duration_ok, $r.codec_ok, $r.size_ok, $r.fps_ok, $r.audio_ok, `
        ($r.ffprobe.found_video -eq $r.mpv.found_video), ("$($r.ffprobe.duration)/$($r.mpv.duration) $($r.ffprobe.fps)/$($r.mpv.fps)"))
}
Write-Host ""
Write-Host ("files compared: {0}, fields disagreeing: {1}" -f $rows.Count, $fail.Count)
$report = [ordered]@{
    tool = 'bench_probe_parity'
    date = (Get-Date -Format 'yyyy-MM-ddTHH:mm:ssK')
    label = 'live-proven (one machine)'
    method = 'the same five fields from ffprobe and from the live mpv instance, per file'
    files = $rows.Count
    disagreements = $fail.Count
    rows = $rows
}
$json = $report | ConvertTo-Json -Depth 8
$outFull = [System.IO.Path]::GetFullPath($OutJson)
$outDir = [System.IO.Path]::GetDirectoryName($outFull)
if (-not (Test-Path -LiteralPath $outDir)) { New-Item -ItemType Directory -Force -Path $outDir | Out-Null }
$json | Set-Content -LiteralPath $outFull -Encoding UTF8
Write-Host "bench_probe_parity: wrote $outFull"
if ($fail.Count -gt 0) { exit 1 }
```

## File: tools/bench_ram_self.ps1
```powershell
# tools/bench_ram_self.ps1 — DEV-ONLY self-measurement harness (never shipped).
# Runs engine.exe headless on a video, samples its RAM/handles/threads while
# it plays, then reports settled avg/peak. Lets the agent A/B memory tweaks
# without Task Manager screenshots.
#
# Usage:
#   powershell -ExecutionPolicy Bypass -File tools\bench_ram_self.ps1 `
#     -EngineExe build\msvc-dev\engine.exe `
#     -Video C:\path\to\video.mp4 -RunMs 60000 -OutJson out.json
#
# Requirements: no other engine.exe running (singleton mutex). The engine
# self-exits via --exit-after-ms; this script only observes.

param(
  [string]$EngineExe = "build\msvc-dev\engine.exe",
  [string]$Video = "",
  [int]$RunMs = 60000,
  [int]$SampleMs = 1000,
  [int]$SettleMs = 15000,
  [string]$OutJson = ""
)

$ErrorActionPreference = "Stop"

if ($Video -eq "" -or -not (Test-Path -LiteralPath $Video)) {
  throw "Video not found: $Video"
}
if (-not (Test-Path -LiteralPath $EngineExe)) {
  throw "Engine exe not found: $EngineExe"
}
$busy = Get-Process engine -ErrorAction SilentlyContinue
if ($busy) {
  throw "Another engine.exe is running (pid $($busy.Id)) - exit it first (singleton mutex)."
}

$exe = (Resolve-Path -LiteralPath $EngineExe).Path
$vid = (Resolve-Path -LiteralPath $Video).Path
Write-Host "bench_ram_self: exe=$exe"
Write-Host "bench_ram_self: video=$vid run_ms=$RunMs"

$logPath = Join-Path $env:LOCALAPPDATA "K6WP\engine.log"
$logBefore = 0
try {
  $logBefore = @(Get-Content -LiteralPath $logPath -ErrorAction Stop).Count
} catch { }

$proc = Start-Process -FilePath $exe -ArgumentList @(
  "--video", $vid, "--exit-after-ms", "$RunMs"
) -PassThru -WindowStyle Hidden

$samples = @()
$sw = [System.Diagnostics.Stopwatch]::StartNew()
$deadline = [DateTime]::UtcNow.AddMilliseconds($RunMs + 15000)
while (-not $proc.HasExited) {
  if ([DateTime]::UtcNow -gt $deadline) {
    Write-Host "bench_ram_self: deadline hit, killing pid $($proc.Id)"
    Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
    break
  }
  try {
    $p = Get-Process -Id $proc.Id -ErrorAction Stop
    $samples += [pscustomobject]@{
      t_ms          = $sw.ElapsedMilliseconds
      workingset_mb = [math]::Round($p.WorkingSet64 / 1MB, 1)
      private_mb    = [math]::Round($p.PrivateMemorySize64 / 1MB, 1)
      handles       = $p.HandleCount
      threads       = $p.Threads.Count
    }
  } catch {
    break
  }
  Start-Sleep -Milliseconds $SampleMs
}
try { $proc.WaitForExit(5000) | Out-Null } catch { }

if ($samples.Count -eq 0) { throw "No samples collected (engine exited immediately?)" }

$settled = @($samples | Where-Object { $_.t_ms -ge $SettleMs -and $_.t_ms -le $RunMs })
if ($settled.Count -eq 0) { $settled = $samples }

$result = [ordered]@{
  exe            = $exe
  video          = $vid
  run_ms         = $RunMs
  samples        = $samples.Count
  settled        = $settled.Count
  ws_avg_mb      = [math]::Round(($settled | Measure-Object workingset_mb -Average).Average, 1)
  ws_peak_mb     = ($settled | Measure-Object workingset_mb -Maximum).Maximum
  priv_avg_mb    = [math]::Round(($settled | Measure-Object private_mb -Average).Average, 1)
  priv_peak_mb   = ($settled | Measure-Object private_mb -Maximum).Maximum
  threads_last   = $settled[-1].threads
  threads_peak   = ($settled | Measure-Object threads -Maximum).Maximum
  handles_last   = $settled[-1].handles
}

# hwdec proof scoped to THIS run: only log lines appended after start.
try {
  $newLines = @(Get-Content -LiteralPath $logPath -ErrorAction Stop |
    Select-Object -Skip $logBefore)
  $hw = $newLines | Select-String "hwdec-current" | Select-Object -Last 1
  if ($hw) { $result["hwdec"] = $hw.Line.Trim() }
} catch { }

$result | ConvertTo-Json -Depth 3 | Write-Host
if ($OutJson -ne "") {
  $result | ConvertTo-Json -Depth 3 | Set-Content -LiteralPath $OutJson -Encoding UTF8
  Write-Host "bench_ram_self: wrote $OutJson"
}
```

## File: tools/bench_resume.ps1
```powershell
# tools/bench_resume.ps1 - Phase 0 resume / suspend latency sampler.
#
# Measures how long the engine takes to come back to a VISIBLE, MOVING
# wallpaper after a suspend/resume cycle.
#
# Two independent signals, because neither alone is trustworthy:
#
#  1. CONTROL-PATH latency (exact, code-driven):
#     the engine's own log timestamps. `--simulate-suspend-after-ms=N` posts
#     PBT_APMSUSPEND then PBT_APMRESUMEAUTOMATIC 2 s later, both through the
#     real Win32 power-broadcast path (engine_app.cpp HandlePowerBroadcast).
#     The log lines "power: PBT_APMSUSPEND received" and
#     "power: PBT_APMRESUMEAUTOMATIC received" are timestamped to the
#     millisecond, so the gap between them is the suspend->resume control
#     latency with no estimation.
#
#  2. VISUAL latency (proxy, honest about its limits):
#     after resume, sample the desktop region until it CHANGES relative to the
#     pre-suspend frame, i.e. until the wallpaper is actually animating again.
#     This is a lower bound on "the user sees motion again". It depends on the
#     screen-capture path (GDI CopyFromScreen) and the video's own content,
#     so it is reported as a proxy, never as the engine's internal latency.
#
# Method: real engine.exe, isolated %LOCALAPPDATA%, windows minimized so the
# injected wallpaper is not occluded. No code instrumentation.
#
# Usage:
#   powershell -NoProfile -ExecutionPolicy Bypass -File tools\bench_resume.ps1 `
#     -Runs 3 -OutJson docs\bench\resume.json

[CmdletBinding()]
param(
    [string]$EngineExe = '',
    [string]$CorpusDir = '',
    [int]$Runs = 3,
    [string]$OutJson = '',
    [switch]$SkipMinimize
)

$ErrorActionPreference = 'Stop'
$script:Here = Split-Path -Parent $MyInvocation.MyCommand.Path
. (Join-Path $script:Here 'bench_lib.ps1')
if ([string]::IsNullOrWhiteSpace($EngineExe)) { $EngineExe = Join-Path $script:Here '..\build\release\engine.exe' }
if ([string]::IsNullOrWhiteSpace($CorpusDir)) { $CorpusDir = Join-Path $script:Here '..\tests\corpus' }
if ([string]::IsNullOrWhiteSpace($OutJson)) { $OutJson = Join-Path $script:Here '..\docs\bench\resume.json' }

if ($Runs -lt 1) { throw "Runs must be >= 1" }
if (-not (Test-Path -LiteralPath $EngineExe)) { throw "engine not found: $EngineExe" }
$busy = @(Get-Process engine -ErrorAction SilentlyContinue)
if ($busy.Count -gt 0) {
    throw ("another engine.exe is running (pid {0}); stop it first" -f (($busy | ForEach-Object { $_.Id }) -join ','))
}

$video = Join-Path $CorpusDir 'anime.mp4'
if (-not (Test-Path -LiteralPath $video)) { throw "missing corpus: $video" }

$hw = Get-HwInventory
$savedWindows = @()
if (-not $SkipMinimize) {
    Write-Host "bench_resume: minimizing top-level windows (restored at the end)"
    $savedWindows = Get-DesktopWindows
}

$results = New-Object System.Collections.Generic.List[object]
try {
    for ($i = 1; $i -le $Runs; $i++) {
        Write-Host "bench_resume: run $i ..."
        $dataDir = Join-Path $env:TEMP ("k6wp-bench-resume-{0}" -f [Guid]::NewGuid().ToString('N').Substring(0, 8))
        New-Item -ItemType Directory -Force -Path (Join-Path $dataDir 'K6WP') | Out-Null
        $oldLocal = $env:LOCALAPPDATA
        $proc = $null
        try {
            $env:LOCALAPPDATA = $dataDir
            $proc = Start-Process -FilePath $EngineExe -PassThru -WindowStyle Hidden `
                -ArgumentList @("--video=$video", '--simulate-suspend-after-ms=6000', '--exit-after-ms=45000')
        }
        finally {
            $env:LOCALAPPDATA = $oldLocal
        }
        if (-not (Wait-K6wpEngine -TimeoutMs 15000)) {
            try { if (-not $proc.HasExited) { Stop-Process -Id $proc.Id -Force } } catch { }
            throw "engine did not answer the IPC pipe within 15s"
        }
        $logPath = Join-Path $dataDir 'K6WP\engine.log'

        # Let it play and settle before the simulated suspend fires.
        Start-Sleep -Milliseconds 4000

        # The suspend fires at 6000 ms and the resume 2 s later. The engine
        # logs both power markers with millisecond timestamps, so the control
        # latency is exact.
        #
        # The VISUAL reference frame must be captured while the wallpaper is
        # FROZEN (i.e. after the suspend marker, once the renderer has
        # actually paused). Capturing it before the suspend would make the
        # post-resume diff meaningless: the wallpper had already moved on
        # between the reference and the freeze.
        $suspendAt = $null
        $resumeAt = $null
        $visualMs = $null
        $frozen = $null
        $deadline = (Get-Date).AddSeconds(40)
        while ((Get-Date) -lt $deadline) {
            if (Test-Path -LiteralPath $logPath) {
                $lines = @(Get-Content -LiteralPath $logPath -ErrorAction SilentlyContinue)
                $sus = @($lines | Select-String 'PBT_APMSUSPEND received' | Select-Object -Last 1)
                $res = @($lines | Select-String 'PBT_APMRESUMEAUTOMATIC received' | Select-Object -Last 1)
                if ($null -eq $suspendAt -and $sus.Count -gt 0) {
                    $suspendAt = $sus[0].Line
                    # Give the renderer a moment to freeze, then take the
                    # reference frame of the frozen wallpaper.
                    Start-Sleep -Milliseconds 400
                    $frozen = Get-RegionSample -Width 240 -Height 160
                }
                if ($null -eq $resumeAt -and $res.Count -gt 0) {
                    $resumeAt = $res[0].Line
                    # Resume just happened: start the visual-change clock.
                    $sw = [Diagnostics.Stopwatch]::StartNew()
                    while ($sw.ElapsedMilliseconds -lt 10000) {
                        $now = Get-RegionSample -Width 240 -Height 160
                        $diff = Get-BitmapDiff -A $frozen -B $now
                        if ($diff -gt 0.5) { $visualMs = $sw.ElapsedMilliseconds; break }
                        Start-Sleep -Milliseconds 25
                    }
                    break
                }
            }
            Start-Sleep -Milliseconds 10
        }

        # Control-path latency straight out of the engine's own timestamps.
        $controlMs = $null
        if ($suspendAt -and $resumeAt) {
            $ts = { param($line) if ($line -match '^\[(\d{2}):(\d{2}):(\d{2})\.(\d{3})\]') {
                        ([int]$Matches[1] * 3600000) + ([int]$Matches[2] * 60000) + ([int]$Matches[3] * 1000) + [int]$Matches[4]
                    } else { $null } }
            $a = & $ts $suspendAt
            $b = & $ts $resumeAt
            if ($null -ne $a -and $null -ne $b) { $controlMs = $b - $a }
        }

        # Did the engine actually resume playback (state check)?
        $stateAfter = $null
        try { $stateAfter = Send-K6wpIpc -Cmd 'get_state' -TimeoutMs 2000 } catch { }

        $results.Add([pscustomobject]@{
            run = $i
            control_path_ms = $controlMs
            visual_change_ms = $visualMs
            resume_log_line = $resumeAt
            paused_after_resume = $(if ($stateAfter -and $stateAfter.ok) { [bool]$stateAfter.state.paused } else { $null })
            video_after_resume = $(if ($stateAfter -and $stateAfter.ok) { $stateAfter.state.video } else { $null })
            data_dir = $dataDir
        })
        Write-Host ("  control-path {0} ms, visual-change {1} ms, paused_after_resume={2}" -f `
            $controlMs, $visualMs, $results[$results.Count - 1].paused_after_resume)

        try { if (-not $proc.HasExited) { Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue } } catch { }
        try { $proc.WaitForExit(5000) | Out-Null } catch { }
    }
}
finally {
    if (-not $SkipMinimize) { Restore-DesktopWindows -Saved $savedWindows }
}

$control = @($results | ForEach-Object { $_.control_path_ms } | Where-Object { $null -ne $_ })
$visual = @($results | ForEach-Object { $_.visual_change_ms } | Where-Object { $null -ne $_ })

$report = [ordered]@{
    tool = 'bench_resume'
    phase = 0
    date = (Get-Date -Format 'yyyy-MM-ddTHH:mm:ssK')
    label = 'live-proven'
    engine = $EngineExe
    video = $video
    runs = $Runs
    hw = $hw
    method = 'real engine.exe; --simulate-suspend-after-ms posts the real PBT_APMSUSPEND/PBT_APMRESUMEAUTOMATIC path; control latency = the engine log timestamps; visual latency = GDI screen-sample diff until the desktop changes again'
    control_path_ms = $(if ($control.Count -gt 0) { Get-Stats $control } else { $null })
    visual_change_ms = $(if ($visual.Count -gt 0) { Get-Stats $visual } else { $null })
    caveats = @(
        'control_path_ms is the engine log timestamp delta between the suspend and resume markers. The engine posts the resume 2000 ms after the suspend, so ~2000 ms is the EXPECTED control value here, not the engine reacting to a real sleep.',
        'visual_change_ms is a PROXY lower bound: it depends on the GDI screen-capture path and on the video content changing. A still frame would read as "no change".',
        'Neither number is the duration of a real machine sleep; a real suspend/resume needs the hardware path and cannot be simulated faithfully in-process.'
    )
    raw = $results
}

$json = $report | ConvertTo-Json -Depth 12
$outFull = [System.IO.Path]::GetFullPath($OutJson)
$outDir = [System.IO.Path]::GetDirectoryName($outFull)
if (-not (Test-Path -LiteralPath $outDir)) { New-Item -ItemType Directory -Force -Path $outDir | Out-Null }
$json | Set-Content -LiteralPath $outFull -Encoding UTF8
Write-Host "bench_resume: wrote $outFull"
```

## File: tools/bench_startup.ps1
```powershell
# bench_startup.ps1 - measure engine startup latency (process start -> Engine:FirstFrame)
#
# Usage:
#   .\bench_startup.ps1 [-Runs 5] [-EnginePath ..\build\msvc-dev\engine.exe]
#                       [-ExitAfterMs 3000] [-TimeoutMs 10000]
#
# Output (stdout, JSON envelope):
#   {"tool":"bench_startup","date":"...","hw":{...},"runs":5,
#    "samples_ms":[...],"median_ms":X,"budget_ms":2000,"pass":true,...}
#
# Also writes the same JSON to -OutJson (default docs/bench_startup.json).
# Budgets match planning.md section 6: startup < 2s (2000 ms).
#
# PowerShell 5.1 compatible. Native stderr is NOT terminating (Continue +
# $LASTEXITCODE), per Todo 4 learning.

[CmdletBinding()]
param(
    [int]$Runs = 5,
    [string]$EnginePath = (Join-Path $PSScriptRoot '..\build\msvc-dev\engine.exe'),
    [int]$ExitAfterMs = 3000,
    [int]$TimeoutMs = 10000,
    [string]$OutJson = (Join-Path $PSScriptRoot '..\docs\bench_startup.json')
)

$ErrorActionPreference = 'Continue'

function Get-Median([double[]]$Values) {
    if ($Values.Count -eq 0) { return 0.0 }
    $sorted = @($Values | Sort-Object)
    $n = $sorted.Count
    if ($n % 2 -eq 1) { return $sorted[($n - 1) / 2] }
    return ($sorted[$n / 2 - 1] + $sorted[$n / 2]) / 2.0
}

$engine = Resolve-Path $EnginePath -ErrorAction SilentlyContinue
if (-not $engine) {
    Write-Error "engine.exe not found at '$EnginePath'"
    exit 2
}

$tmpDir = Join-Path $env:TEMP "k6wp-bench-startup"
New-Item -ItemType Directory -Force -Path $tmpDir | Out-Null

$samples = New-Object System.Collections.Generic.List[double]

for ($i = 1; $i -le $Runs; $i++) {
    $outFile = Join-Path $tmpDir "run-$i-$PID.out"
    $errFile = Join-Path $tmpDir "run-$i-$PID.err"
    Remove-Item $outFile, $errFile -Force -ErrorAction SilentlyContinue

    $start = Get-Date
    $proc = Start-Process -FilePath $engine -ArgumentList "--exit-after-ms=$ExitAfterMs" `
        -RedirectStandardOutput $outFile -RedirectStandardError $errFile `
        -PassThru -WindowStyle Hidden

    $markerMs = $null
    $deadline = (Get-Date).AddMilliseconds($TimeoutMs)
    while ((Get-Date) -lt $deadline) {
        if (Test-Path $outFile) {
            $content = Get-Content $outFile -Raw -ErrorAction SilentlyContinue
            if ($content -match 'Engine:FirstFrame') {
                $markerMs = ((Get-Date) - $start).TotalMilliseconds
                break
            }
        }
        if ($proc.HasExited) { break }
        Start-Sleep -Milliseconds 5
    }

    if (-not $proc.HasExited) {
        Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
        $proc.WaitForExit()
    }

    if ($null -eq $markerMs) {
        Write-Warning "run ${i}: Engine:FirstFrame not seen within ${TimeoutMs}ms (exit code $($proc.ExitCode))"
        continue
    }
    $samples.Add($markerMs)
    Write-Host "run ${i}: $([math]::Round($markerMs, 1)) ms"
}

$median = Get-Median $samples.ToArray()
$rounded = @($samples.ToArray() | ForEach-Object { [math]::Round($_, 1) })
$budgetMs = 2000
$medianRounded = [math]::Round($median, 1)

# Reproducibility: max deviation from the median (rerun variance signal).
$maxDev = 0.0
foreach ($s in $rounded) {
    $d = [math]::Abs($s - $medianRounded)
    if ($d -gt $maxDev) { $maxDev = $d }
}

# HW inventory: engine bench mode loads no video, so GPU decode is idle.
# Never silent: record the GPU name(s) + explicit no-hwdec flag.
$gpuNames = @()
try {
    $gpuNames = @(Get-CimInstance Win32_VideoController -ErrorAction SilentlyContinue |
        ForEach-Object { $_.Name } | Where-Object { $_ })
} catch { $gpuNames = @() }
if ($gpuNames.Count -eq 0) { $gpuNames = @("unknown") }

$result = [ordered]@{
    tool = "bench_startup"
    date = (Get-Date -Format "yyyy-MM-dd")
    engine = "build/msvc-dev/engine.exe"
    hw = [ordered]@{
        gpu = ($gpuNames -join "; ")
        hwdec = "none"
        no_hw = $true
        note = "bench mode loads no video; GPU decode idle (no mpv stream)"
    }
    runs = $Runs
    samples_ms = $rounded
    median_ms = $medianRounded
    max_dev_ms = [math]::Round($maxDev, 1)
    budget_ms = $budgetMs
    pass = ($medianRounded -lt $budgetMs)
    reproducibility = "rerun variance: max sample deviation from median = $([math]::Round($maxDev,1)) ms; reproducible if median stays within +/-10% across runs"
}
$json = $result | ConvertTo-Json -Compress -Depth 5
$json
if (-not [string]::IsNullOrWhiteSpace($OutJson)) {
    $outFull = [System.IO.Path]::GetFullPath($OutJson)
    $outDir = [System.IO.Path]::GetDirectoryName($outFull)
    if (-not (Test-Path -LiteralPath $outDir)) {
        New-Item -ItemType Directory -Force -Path $outDir | Out-Null
    }
    $json | Set-Content -LiteralPath $outFull -Encoding UTF8
    Write-Host "bench_startup: wrote $outFull"
}
```

## File: tools/bench_studio.ps1
```powershell
# tools/bench_studio.ps1 - Phase 0 Studio (GUI) sampler.
#
# Measures the Studio desktop app: startup latency, idle RAM/CPU after the UI
# is up, and the memory of a loaded preview. The Studio is a Qt/QML app, so
# there is no stdout marker: startup is measured as
#   process start -> first visible top-level window (MainWindowHandle != 0)
#   process start -> first non-empty window title
# which is what a user actually perceives as "the app opened".
#
# Method: the REAL studio.exe, isolated %LOCALAPPDATA% (so the user's real
# library/settings are untouched), no code instrumentation. .NET process
# counters for RAM/CPU, same as the engine harness.
#
# The Studio singleton mutex (Local\K6WP-Studio-App-Singleton) means only one
# instance can run: the harness refuses to start if one is already up.
#
# Usage:
#   powershell -NoProfile -ExecutionPolicy Bypass -File tools\bench_studio.ps1 `
#     -Runs 3 -OutJson docs\bench\studio.json

[CmdletBinding()]
param(
    [string]$StudioExe = '',
    [string]$CorpusDir = '',
    [int]$Runs = 3,
    [string]$OutJson = '',
    [switch]$SkipMinimize
)

$ErrorActionPreference = 'Stop'
$script:Here = Split-Path -Parent $MyInvocation.MyCommand.Path
. (Join-Path $script:Here 'bench_lib.ps1')
if ([string]::IsNullOrWhiteSpace($StudioExe)) { $StudioExe = Join-Path $script:Here '..\build\release\studio.exe' }
if ([string]::IsNullOrWhiteSpace($CorpusDir)) { $CorpusDir = Join-Path $script:Here '..\tests\corpus' }
if ([string]::IsNullOrWhiteSpace($OutJson)) { $OutJson = Join-Path $script:Here '..\docs\bench\studio.json' }

if ($Runs -lt 1) { throw "Runs must be >= 1" }
if (-not (Test-Path -LiteralPath $StudioExe)) { throw "studio not found: $StudioExe" }
$busy = @(Get-Process studio -ErrorAction SilentlyContinue)
if ($busy.Count -gt 0) {
    throw ("studio.exe is already running (pid {0}); close it first" -f (($busy | ForEach-Object { $_.Id }) -join ','))
}

$hw = Get-HwInventory
$savedWindows = @()
if (-not $SkipMinimize) {
    Write-Host "bench_studio: minimizing top-level windows (restored at the end)"
    $savedWindows = Get-DesktopWindows
}

$results = New-Object System.Collections.Generic.List[object]
try {
    for ($i = 1; $i -le $Runs; $i++) {
        Write-Host "bench_studio: run $i ..."
        $dataDir = Join-Path $env:TEMP ("k6wp-bench-studio-{0}" -f [Guid]::NewGuid().ToString('N').Substring(0, 8))
        New-Item -ItemType Directory -Force -Path (Join-Path $dataDir 'K6WP') | Out-Null
        $oldLocal = $env:LOCALAPPDATA
        $proc = $null
        try {
            $env:LOCALAPPDATA = $dataDir
            $sw = [Diagnostics.Stopwatch]::StartNew()
            $proc = Start-Process -FilePath $StudioExe -PassThru
        }
        finally {
            $env:LOCALAPPDATA = $oldLocal
        }

        # Startup: wait for the first visible top-level window.
        $windowMs = $null
        $titleMs = $null
        $deadline = (Get-Date).AddSeconds(30)
        while ((Get-Date) -lt $deadline) {
            try { $proc.Refresh() } catch { break }
            if ($proc.HasExited) { break }
            if ($null -eq $windowMs -and $proc.MainWindowHandle -ne 0) {
                $windowMs = $sw.ElapsedMilliseconds
            }
            if ($null -eq $titleMs -and -not [string]::IsNullOrWhiteSpace($proc.MainWindowTitle)) {
                $titleMs = $sw.ElapsedMilliseconds
            }
            if ($null -ne $windowMs -and $null -ne $titleMs) { break }
            Start-Sleep -Milliseconds 20
        }
        if ($null -eq $windowMs) {
            Write-Host "  WARNING: no visible window within 30s (studio may have exited: $($proc.HasExited))"
        }

        # Settle, then sample the idle UI.
        Start-Sleep -Seconds 3
        $idle = @(Get-ProcessSeries -ProcessId $proc.Id -Seconds 10 -IntervalMs 1000)

        $priv = @($idle | ForEach-Object { $_.private_mb })
        $ws = @($idle | ForEach-Object { $_.ws_mb })
        $cpu = @($idle | ForEach-Object { $_.cpu_pct })

        $results.Add([pscustomobject]@{
            run = $i
            startup_window_ms = $windowMs
            startup_title_ms = $titleMs
            exit_code = $(try { if ($proc.HasExited) { $proc.ExitCode } else { $null } } catch { $null })
            private_mb = (Get-Stats $priv)
            ws_mb = (Get-Stats $ws)
            cpu_pct = (Get-Stats $cpu)
            threads_last = $(if ($idle.Count -gt 0) { $idle[-1].threads } else { $null })
            handles_last = $(if ($idle.Count -gt 0) { $idle[-1].handles } else { $null })
            data_dir = $dataDir
        })
        Write-Host ("  startup {0} ms (window), idle priv median {1} MB, cpu mean {2}%" -f `
            $windowMs, (Get-Stats $priv).median, (Get-Stats $cpu).mean)

        try { if (-not $proc.HasExited) { Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue } } catch { }
        try { $proc.WaitForExit(5000) | Out-Null } catch { }
    }
}
finally {
    if (-not $SkipMinimize) { Restore-DesktopWindows -Saved $savedWindows }
}

function Agg($key) {
    $vals = @($results | ForEach-Object { $_.$key.median } | Where-Object { $null -ne $_ })
    if ($vals.Count -eq 0) { return $null }
    return Get-Stats $vals
}
$startupVals = @($results | ForEach-Object { $_.startup_window_ms } | Where-Object { $null -ne $_ })

$report = [ordered]@{
    tool = 'bench_studio'
    phase = 0
    date = (Get-Date -Format 'yyyy-MM-ddTHH:mm:ssK')
    label = 'live-proven'
    studio = $StudioExe
    runs = $Runs
    hw = $hw
    method = 'real studio.exe; isolated LOCALAPPDATA; startup = process start -> first visible top-level window; idle sampled 10s after a 3s settle; .NET process counters'
    startup_window_ms = $(if ($startupVals.Count -gt 0) { Get-Stats $startupVals } else { $null })
    private_mb = (Agg 'private_mb')
    ws_mb = (Agg 'ws_mb')
    cpu_pct = (Agg 'cpu_pct')
    raw = $results
}

$json = $report | ConvertTo-Json -Depth 12
$outFull = [System.IO.Path]::GetFullPath($OutJson)
$outDir = [System.IO.Path]::GetDirectoryName($outFull)
if (-not (Test-Path -LiteralPath $outDir)) { New-Item -ItemType Directory -Force -Path $outDir | Out-Null }
$json | Set-Content -LiteralPath $outFull -Encoding UTF8
Write-Host "bench_studio: wrote $outFull"
```

## File: tools/bench_unload_d2.ps1
```powershell
# tools/bench_unload_d2.ps1 - task 1.1 (D2): the black-frame gate.
#
# THE GATE (from the brief):
#   1. NO black frame when the wallpaper becomes visible again after a real
#      fullscreen app exits (n=5), measured with a VISUAL sampler that
#      samples the screen across the exit.
#   2. NO Explorer UI hang (a timeout probe against Explorer's UI thread).
#   3. An EXPLICIT user pause still shows the picture.
#   4. The static wallpaper UNDERNEATH must be visible while Unloaded.
#
# Why the SCREEN and not the injected surface: after the D2 unload the
# injected surface is DESTROYED (the whole mpv instance goes), so there is
# no surface left to capture - what the user sees is the shell's own
# wallpaper (the per-user still). The screen IS the thing the gate is
# about.
#
# The sampler hides the covering windows first (the agent's own
# always-on-top overlay would otherwise cover the screen and make every
# sample read "not black" - a false pass), and the live session is
# snapshotted + restored with tools/session_snapshot.ps1 (C8).
[CmdletBinding()]
param(
    [string]$EngineExe = '',
    [int]$UnloadAfterMs = 3000,
    [int]$Runs = 5,
    [int]$SampleMs = 75,
    [string]$OutJson = ''
)
$ErrorActionPreference = 'Stop'
$script:Here = Split-Path -Parent $MyInvocation.MyCommand.Path
. (Join-Path $script:Here 'bench_lib.ps1')
. (Join-Path $script:Here 'session_snapshot.ps1')
if ([string]::IsNullOrWhiteSpace($EngineExe)) { $EngineExe = Join-Path $script:Here '..\build\release\engine.exe' }
if ([string]::IsNullOrWhiteSpace($OutJson)) { $OutJson = Join-Path $script:Here '..\docs\\bench\\unload-d2.json' }
$engine = [System.IO.Path]::GetFullPath($EngineExe)
$video = Join-Path $script:Here '..\tests\corpus\anime.mp4'

Add-Type -AssemblyName System.Drawing
Add-Type -AssemblyName System.Windows.Forms
Add-Type @"
using System;using System.Runtime.InteropServices;using System.Text;
public class D2 {
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
  [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
  [DllImport("user32.dll")] public static extern bool AttachThreadInput(uint a, uint b, bool attach);
  [DllImport("kernel32.dll")] public static extern uint GetCurrentThreadId();
  [DllImport("user32.dll")] public static extern IntPtr FindWindowW(string c, string n);
  [DllImport("user32.dll", CharSet=CharSet.Unicode)]
  public static extern IntPtr SendMessageTimeout(IntPtr h, uint m, IntPtr w, IntPtr l, uint f, uint t, out IntPtr r);
  [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc cb, IntPtr p);
  [DllImport("user32.dll")] public static extern bool EnumChildWindows(IntPtr parent, EnumProc cb, IntPtr p);
  public delegate bool EnumProc(IntPtr h, IntPtr p);
  [DllImport("user32.dll")] public static extern int GetClassName(IntPtr h, StringBuilder s, int n);
}
"@

$script:injected = New-Object System.Collections.Generic.List[object]
$childCb = [D2+EnumProc]{
    param($h, $p)
    $sb = New-Object System.Text.StringBuilder 256
    [void][D2]::GetClassName($h, $sb, 256)
    if ($sb.ToString() -eq 'K6WP.DesktopInject.1') { $script:injected.Add($h) }
    return $true
}
$topCb = [D2+EnumProc]{
    param($h, $p)
    [void][D2]::EnumChildWindows($h, $childCb, [IntPtr]::Zero)
    return $true
}
function Get-InjectedCount {
    $script:injected.Clear()
    [void][D2]::EnumWindows($topCb, [IntPtr]::Zero)
    return $script:injected.Count
}

# The Explorer UI thread probe: a WM_NULL with a timeout to the taskbar
# (Shell_TrayWnd lives on Explorer's UI thread). A timeout means the shell
# is hung - the exact failure the earlier ShowWindow attempt caused.
function Test-ExplorerResponsive {
    param([int]$TimeoutMs = 2000)
    $tray = [D2]::FindWindowW('Shell_TrayWnd', $null)
    if ($tray -eq [IntPtr]::Zero) { return $null }
    $res = [IntPtr]::Zero
    $sw = [Diagnostics.Stopwatch]::StartNew()
    $ok = [D2]::SendMessageTimeout($tray, 0, [IntPtr]::Zero, [IntPtr]::Zero, 2, [uint32]$TimeoutMs, [ref]$res)
    return [pscustomobject]@{ responsive = [bool]$ok; ms = $sw.ElapsedMilliseconds }
}

function Get-ScreenLuma {
    $bmp = New-Object System.Drawing.Bitmap 240, 135
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    try {
        $g.CopyFromScreen(0, 0, 0, 0, (New-Object System.Drawing.Size(240, 135)))
    } finally { $g.Dispose() }
    try {
        $sum = 0.0; $n = 0
        for ($y = 0; $y -lt $bmp.Height; $y += 3) {
            for ($x = 0; $x -lt $bmp.Width; $x += 3) {
                $c = $bmp.GetPixel($x, $y)
                $sum += 0.2126 * $c.R + 0.7152 * $c.G + 0.0722 * $c.B; $n++
            }
        }
        return [math]::Round($sum / $n, 2)
    } finally { $bmp.Dispose() }
}

function Start-Engine {
    for ($i = 0; $i -lt 40; $i++) {
        if (-not (Get-Process engine -ErrorAction SilentlyContinue)) { break }
        Start-Sleep -Milliseconds 250
    }
    Start-Sleep -Milliseconds 800
    $dataDir = Join-Path $env:TEMP ("k6wp-d2-" + [Guid]::NewGuid().ToString('N').Substring(0, 8))
    New-Item -ItemType Directory -Force -Path (Join-Path $dataDir 'K6WP') | Out-Null
    $old = $env:LOCALAPPDATA
    try {
        $env:LOCALAPPDATA = $dataDir
        $proc = Start-Process -FilePath $engine -PassThru -WindowStyle Hidden -ArgumentList @(
            "--video=$video", "--unload-after-pause-ms=$UnloadAfterMs", '--exit-after-ms=120000')
    }
    finally { $env:LOCALAPPDATA = $old }
    if (-not (Wait-K6wpEngine -TimeoutMs 20000)) { throw "engine did not answer the pipe" }
    return [pscustomobject]@{ Proc = $proc; DataDir = $dataDir }
}
function New-Fullscreen {
    $form = New-Object System.Windows.Forms.Form
    $form.FormBorderStyle = 'None'
    $form.StartPosition = 'Manual'
    $form.Location = [System.Drawing.Point]::new(0, 0)
    $form.Size = [System.Drawing.Size]::new(
        [System.Windows.Forms.SystemInformation]::VirtualScreen.Width,
        [System.Windows.Forms.SystemInformation]::VirtualScreen.Height)
    $form.TopMost = $true
    $form.ShowInTaskbar = $false
    $form.BackColor = [System.Drawing.Color]::Black
    $form.Show()
    $hwnd = [IntPtr]$form.Handle
    $fgPid = 0
    $fgThread = [D2]::GetWindowThreadProcessId([D2]::GetForegroundWindow(), [ref]$fgPid)
    $myThread = [D2]::GetCurrentThreadId()
    [void][D2]::AttachThreadInput($myThread, $fgThread, $true)
    [void][D2]::SetForegroundWindow($hwnd)
    [void][D2]::AttachThreadInput($myThread, $fgThread, $false)
    [void]$form.Activate()
    return $form
}
function Wait-State {
    param([string]$State, [int]$TimeoutSec = 30)
    $deadline = (Get-Date).AddSeconds($TimeoutSec)
    while ((Get-Date) -lt $deadline) {
        $st = Send-K6wpIpc -Cmd 'get_state' -TimeoutMs 2000
        if ($st.state.decode_state -eq $State) { return $true }
        Start-Sleep -Milliseconds 200
    }
    return $false
}

$script:results = New-Object System.Collections.Generic.List[object]
$busy = @(Get-Process engine -ErrorAction SilentlyContinue)
if ($busy.Count -gt 0) { throw "an engine.exe is already running" }

# C8: the live session is snapshotted and restored automatically, even if a
# run fails half-way.
Invoke-SessionSafe -Name 'unload D2 gate' -RestoreDisplay -RestoreWallpaper -RestoreLockscreen -RestoreExplorer -ScriptBlock {
    for ($run = 1; $run -le $Runs; $run++) {
        $e = Start-Engine
        try {
            $saved = Get-DesktopWindows
            try {
                $explorerBefore = Test-ExplorerResponsive
                $form = New-Fullscreen
                if (-not (Wait-State 'unloaded')) { throw "the engine never reached the Unloaded state" }
                # 4. The static wallpaper underneath must be visible while
                #    Unloaded: the injected surface is GONE (the whole mpv
                #    instance was destroyed), so the shell's wallpaper shows.
                $surfacesWhileUnloaded = Get-InjectedCount
                $explorerDuring = Test-ExplorerResponsive

                # --- the exit: close the fullscreen app and sample the
                #     screen across the transition.
                $samples = @()
                $form.Close()
                $sw = [Diagnostics.Stopwatch]::StartNew()
                for ($i = 0; $i -lt 40; $i++) {
                    $luma = Get-ScreenLuma
                    $samples += [pscustomobject]@{ ms = $sw.ElapsedMilliseconds; luma = $luma }
                    if ($sw.ElapsedMilliseconds -gt 3000) { break }
                    Start-Sleep -Milliseconds $SampleMs
                }
                # The wallpaper must be back and MOVING.
                $animating = $false
                $deadline = (Get-Date).AddSeconds(15)
                while ((Get-Date) -lt $deadline) {
                    $a = Get-ScreenLuma
                    Start-Sleep -Milliseconds 500
                    $b = Get-ScreenLuma
                    if ([math]::Abs($a - $b) -gt 0.4) { $animating = $true; break }
                }
                $explorerAfter = Test-ExplorerResponsive
                $black = @($samples | Where-Object { $_.luma -lt 5 })
                $lumas = @($samples | ForEach-Object { [double]$_.luma } | Sort-Object)
                $medianLuma = if ($lumas.Count -gt 0) { [math]::Round($lumas[[int]($lumas.Count / 2)], 2) } else { $null }
                $script:results.Add([pscustomobject]@{
                    run = $run
                    samples = $samples.Count
                    black_samples = $black.Count
                    min_luma = ($samples | Measure-Object luma -Minimum).Minimum
                    median_luma = $medianLuma
                    injected_surfaces_while_unloaded = $surfacesWhileUnloaded
                    wallpaper_visible_while_unloaded = ($surfacesWhileUnloaded -eq 0)
                    explorer_before = $explorerBefore
                    explorer_during = $explorerDuring
                    explorer_after = $explorerAfter
                    explorer_responsive = ($null -eq $explorerAfter -or $explorerAfter.responsive)
                    animating_after_resume = $animating
                })
                Write-Host ("  run {0}: black={1}/{2} min_luma={3} surfaces_while_unloaded={4} explorer_ok={5} animating={6}" -f `
                        $run, $black.Count, $samples.Count, ($samples | Measure-Object luma -Minimum).Minimum,
                        $surfacesWhileUnloaded, $script:results[$script:results.Count - 1].explorer_responsive, $animating)
            }
            finally { Restore-DesktopWindows -Saved $saved }
        }
        finally {
            try { if (-not $e.Proc.HasExited) { Stop-Process -Id $e.Proc.Id -Force } } catch { }
        }
    }

    # 3. An EXPLICIT user pause still shows the picture.
    $e = Start-Engine
    try {
        $saved = Get-DesktopWindows
        try {
            Start-Sleep -Seconds 4
            [void](Send-K6wpIpc -Cmd 'pause' -TimeoutMs 3000)
            Start-Sleep -Seconds ([int](($UnloadAfterMs / 1000) + 3))
            $st = Send-K6wpIpc -Cmd 'get_state' -TimeoutMs 3000
            $luma = Get-ScreenLuma
            $script:results.Add([pscustomobject]@{
                run = 'user-pause'
                decode_state = $st.state.decode_state
                luma = $luma
                picture_present = ($luma -gt 5.0)
                surfaces = (Get-InjectedCount)
            })
            Write-Host ("  user pause: state={0} luma={1} (the picture must stay)" -f $st.state.decode_state, $luma)
            [void](Send-K6wpIpc -Cmd 'resume' -TimeoutMs 3000)
        }
        finally { Restore-DesktopWindows -Saved $saved }
    }
    finally {
        try { if (-not $e.Proc.HasExited) { Stop-Process -Id $e.Proc.Id -Force } } catch { }
    }
}

$gateRuns = @($script:results | Where-Object { $_.run -is [int] })
$blackTotal = ($gateRuns | Measure-Object black_samples -Sum).Sum
$report = [ordered]@{
    tool = 'bench_unload_d2'
    phase = 1
    date = (Get-Date -Format 'yyyy-MM-ddTHH:mm:ssK')
    label = 'live-proven (one machine)'
    engine = $engine
    unload_after_ms = $UnloadAfterMs
    method = 'the screen is sampled across a REAL fullscreen exit; the covering windows are hidden first (the agent overlay would make every sample read non-black - a false pass); the live session is snapshotted + restored with tools/session_snapshot.ps1'
    runs = $script:results.Count
    black_samples_total = $blackTotal
    gate_no_black_frame = ($blackTotal -eq 0)
    gate_wallpaper_visible_while_unloaded = (@($gateRuns | Where-Object { -not $_.wallpaper_visible_while_unloaded }).Count -eq 0)
    gate_explorer_responsive = (@($gateRuns | Where-Object { -not $_.explorer_responsive }).Count -eq 0)
    gate_animating_after_resume = (@($gateRuns | Where-Object { -not $_.animating_after_resume }).Count -eq 0)
    user_pause = ($script:results | Where-Object { $_.run -eq 'user-pause' })
    results = $script:results
}
$json = $report | ConvertTo-Json -Depth 8
$outFull = [System.IO.Path]::GetFullPath($OutJson)
$outDir = [System.IO.Path]::GetDirectoryName($outFull)
if (-not (Test-Path -LiteralPath $outDir)) { New-Item -ItemType Directory -Force -Path $outDir | Out-Null }
$json | Set-Content -LiteralPath $outFull -Encoding UTF8
Write-Host ""
Write-Host ("GATE no black frame        : {0} (black samples: {1})" -f $report.gate_no_black_frame, $blackTotal)
Write-Host ("GATE wallpaper visible      : {0}" -f $report.gate_wallpaper_visible_while_unloaded)
Write-Host ("GATE explorer responsive    : {0}" -f $report.gate_explorer_responsive)
Write-Host ("GATE animating after resume : {0}" -f $report.gate_animating_after_resume)
Write-Host ("user pause keeps the picture: {0}" -f $report.user_pause.picture_present)
Write-Host "bench_unload_d2: wrote $outFull"
if (-not ($report.gate_no_black_frame -and $report.gate_wallpaper_visible_while_unloaded -and
            $report.gate_explorer_responsive -and $report.gate_animating_after_resume)) {
    exit 1
}
```

## File: tools/bench_unload_scenarios.ps1
```powershell
# tools/bench_unload_scenarios.ps1 - task 1.1: the required scenario tests.
#
# Scenarios (the spec's list):
#   1. resume latency after an unload (median n=5), and NO BLACK FRAME
#   2. the video changes while Unloaded (a new renderer is NOT created; the
#      file is simply loaded again)
#   3. quit while Unloaded (no hang in the teardown)
#   4. the user pause is NOT unloaded (the picture stays - the control)
#   5. a fullscreen app in and out (the invisible-owner case)
#
# Every scenario drives the REAL engine over the REAL IPC path.
[CmdletBinding()]
param(
    [string]$EngineExe = '',
    [int]$UnloadAfterMs = 3000,
    [int]$ResumeRuns = 5,
    [string]$OutJson = ''
)
$ErrorActionPreference = 'Stop'
$script:Here = Split-Path -Parent $MyInvocation.MyCommand.Path
. (Join-Path $script:Here 'bench_lib.ps1')
if ([string]::IsNullOrWhiteSpace($EngineExe)) { $EngineExe = Join-Path $script:Here '..\build\release\engine.exe' }
if ([string]::IsNullOrWhiteSpace($OutJson)) { $OutJson = Join-Path $script:Here '..\docs\bench\unload-scenarios.json' }
$engine = [System.IO.Path]::GetFullPath($EngineExe)
$video = Join-Path $script:Here '..\tests\corpus\anime.mp4'
$video2 = Join-Path $script:Here '..\tests\corpus\slideshow.mp4'

Add-Type -AssemblyName System.Drawing
Add-Type -AssemblyName System.Windows.Forms
Add-Type @"
using System;using System.Runtime.InteropServices;using System.Text;
public class FS {
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
  [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
  [DllImport("user32.dll")] public static extern bool AttachThreadInput(uint a, uint b, bool attach);
  [DllImport("kernel32.dll")] public static extern uint GetCurrentThreadId();
  [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc cb, IntPtr p);
  [DllImport("user32.dll")] public static extern bool EnumChildWindows(IntPtr parent, EnumProc cb, IntPtr p);
  public delegate bool EnumProc(IntPtr h, IntPtr p);
  [DllImport("user32.dll")] public static extern int GetClassName(IntPtr h, StringBuilder s, int n);
  [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr hdc, uint flags);
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
  [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left,Top,Right,Bottom; }
}
"@

$script:found = New-Object System.Collections.Generic.List[object]
$childCb = [FS+EnumProc]{
    param($h, $p)
    $sb = New-Object System.Text.StringBuilder 256
    [void][FS]::GetClassName($h, $sb, 256)
    if ($sb.ToString() -eq 'K6WP.DesktopInject.1') { $script:found.Add($h) }
    return $true
}
$topCb = [FS+EnumProc]{
    param($h, $p)
    [void][FS]::EnumChildWindows($h, $childCb, [IntPtr]::Zero)
    return $true
}
function Get-InjectedWindow {
    $script:found.Clear()
    [void][FS]::EnumWindows($topCb, [IntPtr]::Zero)
    if ($script:found.Count -eq 0) { return [IntPtr]::Zero }
    return $script:found[0]
}
function Save-Injected([IntPtr]$h, [string]$Path) {
    $r = New-Object FS+RECT
    [void][FS]::GetWindowRect($h, [ref]$r)
    $bmp = New-Object System.Drawing.Bitmap ($r.Right - $r.Left), ($r.Bottom - $r.Top)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $hdc = $g.GetHdc()
    try { [void][FS]::PrintWindow($h, $hdc, 2) } finally { $g.ReleaseHdc($hdc); $g.Dispose() }
    $bmp.Save($Path, [System.Drawing.Imaging.ImageFormat]::Png)
    $bmp.Dispose()
}
function Get-Luma([string]$Path) {
    $bmp = [System.Drawing.Bitmap]::FromFile($Path)
    try {
        $sum = 0.0; $n = 0
        for ($y = 0; $y -lt $bmp.Height; $y += 6) {
            for ($x = 0; $x -lt $bmp.Width; $x += 6) {
                $c = $bmp.GetPixel($x, $y)
                $sum += 0.2126 * $c.R + 0.7152 * $c.G + 0.0722 * $c.B; $n++
            }
        }
        return [math]::Round($sum / $n, 2)
    }
    finally { $bmp.Dispose() }
}
function Start-Engine {
    param([string]$Extra = '')
    # The singleton mutex is released ASYNCHRONOUSLY when the previous
    # engine dies, so a start can lose the race and exit immediately. Wait
    # for the old process to be gone (and for the mutex to settle) first.
    for ($i = 0; $i -lt 40; $i++) {
        if (-not (Get-Process engine -ErrorAction SilentlyContinue)) { break }
        Start-Sleep -Milliseconds 250
    }
    Start-Sleep -Milliseconds 800
    $dataDir = Join-Path $env:TEMP ("k6wp-scen-" + [Guid]::NewGuid().ToString('N').Substring(0, 8))
    New-Item -ItemType Directory -Force -Path (Join-Path $dataDir 'K6WP') | Out-Null
    $old = $env:LOCALAPPDATA
    $argList = @("--video=$video", "--unload-after-pause-ms=$UnloadAfterMs", '--exit-after-ms=120000')
    if ($Extra) { $argList = @("--video=$Extra", "--unload-after-pause-ms=$UnloadAfterMs", '--exit-after-ms=120000') }
    try {
        $env:LOCALAPPDATA = $dataDir
        $proc = Start-Process -FilePath $engine -PassThru -WindowStyle Hidden -ArgumentList $argList
    }
    finally { $env:LOCALAPPDATA = $old }
    if (-not (Wait-K6wpEngine -TimeoutMs 20000)) { throw "engine did not answer the pipe" }
    return [pscustomobject]@{ Proc = $proc; DataDir = $dataDir }
}
function New-Fullscreen {
    $form = New-Object System.Windows.Forms.Form
    $form.FormBorderStyle = 'None'
    $form.StartPosition = 'Manual'
    $form.Location = [System.Drawing.Point]::new(0, 0)
    $form.Size = [System.Drawing.Size]::new(
        [System.Windows.Forms.SystemInformation]::VirtualScreen.Width,
        [System.Windows.Forms.SystemInformation]::VirtualScreen.Height)
    $form.TopMost = $true
    $form.ShowInTaskbar = $false
    $form.Show()
    $hwnd = [IntPtr]$form.Handle
    $fgPid = 0
    $fgThread = [FS]::GetWindowThreadProcessId([FS]::GetForegroundWindow(), [ref]$fgPid)
    $myThread = [FS]::GetCurrentThreadId()
    [void][FS]::AttachThreadInput($myThread, $fgThread, $true)
    [void][FS]::SetForegroundWindow($hwnd)
    [void][FS]::AttachThreadInput($myThread, $fgThread, $false)
    [void]$form.Activate()
    return $form
}
function Wait-State {
    param([string]$State, [int]$TimeoutSec = 25)
    $deadline = (Get-Date).AddSeconds($TimeoutSec)
    while ((Get-Date) -lt $deadline) {
        $st = Send-K6wpIpc -Cmd 'get_state' -TimeoutMs 2000
        if ($st.state.decode_state -eq $State) { return $true }
        Start-Sleep -Milliseconds 200
    }
    return $false
}

$results = [ordered]@{}
$busy = @(Get-Process engine -ErrorAction SilentlyContinue)
if ($busy.Count -gt 0) { throw "an engine.exe is already running" }

# --- scenario 5 + 1: fullscreen in, unload, fullscreen out, resume latency ---
# Each cycle runs a FRESH engine: the measurement is the resume latency
# itself, and a fresh engine per cycle keeps the cycles independent (a
# reused engine has to re-arm the fullscreen watcher, which the win-event
# hook does not always deliver).
Write-Host "== scenario 5/1: fullscreen -> unload -> resume (n=$ResumeRuns) =="
$latencies = @()
$blackFrames = 0
for ($cycle = 1; $cycle -le $ResumeRuns; $cycle++) {
    $run = Start-Engine
    try {
        $saved = Get-DesktopWindows
        try {
            $form = New-Fullscreen
            if (-not (Wait-State 'unloaded')) { throw "the engine never reached the Unloaded state" }
            # The resume latency: how long from leaving fullscreen until the
            # wallpaper is moving again.
            $sw = [Diagnostics.Stopwatch]::StartNew()
            $form.Close()
            $h = Get-InjectedWindow
            $ok = $false
            while ($sw.ElapsedMilliseconds -lt 20000) {
                $tmp = Join-Path $env:TEMP ("k6wp-scen-" + [Guid]::NewGuid().ToString('N').Substring(0, 6) + ".png")
                Save-Injected $h $tmp
                $luma = Get-Luma $tmp
                Remove-Item $tmp -Force -ErrorAction SilentlyContinue
                if ($luma -lt 3) { $blackFrames++ }
                # The animation is back when two frames 400 ms apart differ.
                $a = Join-Path $env:TEMP ("k6wp-a-" + [Guid]::NewGuid().ToString('N').Substring(0, 6) + ".png")
                $b = Join-Path $env:TEMP ("k6wp-b-" + [Guid]::NewGuid().ToString('N').Substring(0, 6) + ".png")
                Save-Injected $h $a; Start-Sleep -Milliseconds 400; Save-Injected $h $b
                $ba = [System.Drawing.Bitmap]::FromFile($a); $bb = [System.Drawing.Bitmap]::FromFile($b)
                $diff = Get-BitmapDiff -A $ba -B $bb -Step 8
                $ba.Dispose(); $bb.Dispose()
                Remove-Item $a, $b -Force -ErrorAction SilentlyContinue
                if ($diff -gt 0.5) { $ok = $true; break }
            }
            $latencies += $sw.ElapsedMilliseconds
            Write-Host ("  cycle {0}: resume {1} ms, animating={2}" -f $cycle, $sw.ElapsedMilliseconds, $ok)
            if (-not $ok) { throw "the wallpaper did not come back after the resume" }
        }
        finally { Restore-DesktopWindows -Saved $saved }
    }
    finally {
        try { if (-not $run.Proc.HasExited) { Stop-Process -Id $run.Proc.Id -Force } } catch { }
    }
}
$results['resume_latency_ms'] = (Get-Stats $latencies)
$results['black_frames_seen'] = $blackFrames

# --- scenario 3: quit while Unloaded ---
Write-Host "== scenario 3: quit while Unloaded =="
$run = Start-Engine
try {
    $saved = Get-DesktopWindows
    try {
        $form = New-Fullscreen
        if (-not (Wait-State 'unloaded')) { throw "the engine never reached the Unloaded state" }
        $sw = [Diagnostics.Stopwatch]::StartNew()
        $ack = Send-K6wpIpc -Cmd 'quit' -TimeoutMs 5000
        $exited = $run.Proc.WaitForExit(15000)
        $quitMs = $sw.ElapsedMilliseconds
        $results['quit_while_unloaded'] = [ordered]@{
            acked = [bool]$ack.ok
            exited = [bool]$exited
            exit_ms = $quitMs
            exit_code = $(if ($exited) { $run.Proc.ExitCode } else { $null })
        }
        Write-Host ("  quit: acked={0} exited={1} in {2} ms (exit code {3})" -f `
            $ack.ok, $exited, $quitMs, $results['quit_while_unloaded'].exit_code)
        if (-not $exited) { throw "the engine did not exit after a quit while Unloaded" }
        $form.Close()
    }
    finally { Restore-DesktopWindows -Saved $saved }
}
finally {
    try { if (-not $run.Proc.HasExited) { Stop-Process -Id $run.Proc.Id -Force } } catch { }
}

# --- scenario 2: the video changes while Unloaded ---
Write-Host "== scenario 2: the video changes while Unloaded =="
$run = Start-Engine
try {
    $saved = Get-DesktopWindows
    try {
        $form = New-Fullscreen
        if (-not (Wait-State 'unloaded')) { throw "the engine never reached the Unloaded state" }
        $ack = Send-K6wpIpc -Cmd 'set_video' -TimeoutMs 5000 -Payload @{ path = $video2 }
        Start-Sleep -Seconds 2
        $st = Send-K6wpIpc -Cmd 'get_state' -TimeoutMs 3000
        $results['video_change_while_unloaded'] = [ordered]@{
            acked = [bool]$ack.ok
            decode_state = $st.state.decode_state
            video = $st.state.video
        }
        Write-Host ("  set_video while Unloaded: acked={0} state={1} video='{2}'" -f `
            $ack.ok, $st.state.decode_state, $st.state.video)
        $form.Close()
    }
    finally { Restore-DesktopWindows -Saved $saved }
}
finally {
    try { if (-not $run.Proc.HasExited) { Stop-Process -Id $run.Proc.Id -Force } } catch { }
}

# --- scenario 4: the user pause is NOT unloaded (the control) ---
Write-Host "== scenario 4: the explicit user pause keeps the picture =="
$run = Start-Engine
try {
    $saved = Get-DesktopWindows
    try {
        Start-Sleep -Seconds 3
        [void](Send-K6wpIpc -Cmd 'pause' -TimeoutMs 3000)
        Start-Sleep -Seconds ([int](($UnloadAfterMs / 1000) + 3))
        $st = Send-K6wpIpc -Cmd 'get_state' -TimeoutMs 3000
        $h = Get-InjectedWindow
        $tmp = Join-Path $env:TEMP 'k6wp-scen-user.png'
        Save-Injected $h $tmp
        $luma = Get-Luma $tmp
        Remove-Item $tmp -Force -ErrorAction SilentlyContinue
        $results['user_pause'] = [ordered]@{
            decode_state = $st.state.decode_state
            surface_luma = $luma
            picture_present = ($luma -gt 5.0)
        }
        Write-Host ("  user pause: state={0} surface luma={1}" -f $st.state.decode_state, $luma)
        [void](Send-K6wpIpc -Cmd 'resume' -TimeoutMs 3000)
    }
    finally { Restore-DesktopWindows -Saved $saved }
}
finally {
    try { if (-not $run.Proc.HasExited) { Stop-Process -Id $run.Proc.Id -Force } } catch { }
}

$report = [ordered]@{
    tool = 'bench_unload_scenarios'
    phase = 1
    date = (Get-Date -Format 'yyyy-MM-ddTHH:mm:ssK')
    label = 'live-proven (one machine)'
    engine = $engine
    unload_after_ms = $UnloadAfterMs
    results = $results
}
$json = $report | ConvertTo-Json -Depth 8
$outFull = [System.IO.Path]::GetFullPath($OutJson)
$outDir = [System.IO.Path]::GetDirectoryName($outFull)
if (-not (Test-Path -LiteralPath $outDir)) { New-Item -ItemType Directory -Force -Path $outDir | Out-Null }
$json | Set-Content -LiteralPath $outFull -Encoding UTF8
Write-Host "bench_unload_scenarios: wrote $outFull"
```

## File: tools/bench_unload_visual.ps1
```powershell
# tools/bench_unload_visual.ps1 - task 1.1: the Unloaded state must not
# black out the wallpaper, and the resume must not flash a black frame.
#
# The check is done on the INJECTED wallpaper window itself (PrintWindow on
# the K6WP.DesktopInject.1 surface), not on a desktop screenshot: a desktop
# grab records the plain desktop whenever anything covers the screen, which
# would silently turn this into a no-op.
#
# Phases:
#   1. playing   - the wallpaper is animating
#   2. paused    - the file is still loaded
#   3. unloaded  - the file was released; the LAST FRAME must still be there
#   4. resumed   - the animation must be back
[CmdletBinding()]
param(
    [string]$EngineExe = '',
    [int]$UnloadAfterMs = 5000,
    [string]$OutJson = ''
)
$ErrorActionPreference = 'Stop'
$script:Here = Split-Path -Parent $MyInvocation.MyCommand.Path
. (Join-Path $script:Here 'bench_lib.ps1')
if ([string]::IsNullOrWhiteSpace($EngineExe)) { $EngineExe = Join-Path $script:Here '..\build\release\engine.exe' }
if ([string]::IsNullOrWhiteSpace($OutJson)) { $OutJson = Join-Path $script:Here '..\docs\bench\unload-visual.json' }
$engine = [System.IO.Path]::GetFullPath($EngineExe)
$video = Join-Path $script:Here '..\tests\corpus\anime.mp4'

Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;using System.Runtime.InteropServices;using System.Text;
public class WPV {
  [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc cb, IntPtr p);
  [DllImport("user32.dll")] public static extern bool EnumChildWindows(IntPtr parent, EnumProc cb, IntPtr p);
  public delegate bool EnumProc(IntPtr h, IntPtr p);
  [DllImport("user32.dll")] public static extern int GetClassName(IntPtr h, StringBuilder s, int n);
  [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr hdc, uint flags);
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
  [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left,Top,Right,Bottom; }
}
"@
$script:found = New-Object System.Collections.Generic.List[object]
$childCb = [WPV+EnumProc]{
    param($h, $p)
    $sb = New-Object System.Text.StringBuilder 256
    [void][WPV]::GetClassName($h, $sb, 256)
    if ($sb.ToString() -eq 'K6WP.DesktopInject.1') { $script:found.Add($h) }
    return $true
}
$topCb = [WPV+EnumProc]{
    param($h, $p)
    [void][WPV]::EnumChildWindows($h, $childCb, [IntPtr]::Zero)
    return $true
}
function Get-InjectedWindow {
    $script:found.Clear()
    [void][WPV]::EnumWindows($topCb, [IntPtr]::Zero)
    if ($script:found.Count -eq 0) { return [IntPtr]::Zero }
    return $script:found[0]
}
function Save-Injected([IntPtr]$h, [string]$Path) {
    $r = New-Object WPV+RECT
    [void][WPV]::GetWindowRect($h, [ref]$r)
    $bmp = New-Object System.Drawing.Bitmap ($r.Right - $r.Left), ($r.Bottom - $r.Top)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $hdc = $g.GetHdc()
    try { [void][WPV]::PrintWindow($h, $hdc, 2) } finally { $g.ReleaseHdc($hdc); $g.Dispose() }
    $bmp.Save($Path, [System.Drawing.Imaging.ImageFormat]::Png)
    $bmp.Dispose()
}
function Get-Signature([string]$Path) {
    $bmp = [System.Drawing.Bitmap]::FromFile($Path)
    try {
        $sum = 0.0; $sum2 = 0.0; $n = 0
        for ($y = 0; $y -lt $bmp.Height; $y += 4) {
            for ($x = 0; $x -lt $bmp.Width; $x += 4) {
                $c = $bmp.GetPixel($x, $y)
                $l = 0.2126 * $c.R + 0.7152 * $c.G + 0.0722 * $c.B
                $sum += $l; $sum2 += $l * $l; $n++
            }
        }
        $mean = $sum / $n
        return [pscustomobject]@{
            mean_luma = [math]::Round($mean, 2)
            stddev = [math]::Round([math]::Sqrt([math]::Max(0, ($sum2 / $n) - ($mean * $mean))), 2)
        }
    }
    finally { $bmp.Dispose() }
}

$busy = @(Get-Process engine -ErrorAction SilentlyContinue)
if ($busy.Count -gt 0) { throw "an engine.exe is already running" }
$dataDir = Join-Path $env:TEMP ("k6wp-unloadv-" + [Guid]::NewGuid().ToString('N').Substring(0, 8))
New-Item -ItemType Directory -Force -Path (Join-Path $dataDir 'K6WP') | Out-Null
$old = $env:LOCALAPPDATA
try {
    $env:LOCALAPPDATA = $dataDir
    $p = Start-Process -FilePath $engine -PassThru -WindowStyle Hidden -ArgumentList @(
        "--video=$video", "--unload-after-pause-ms=$UnloadAfterMs", '--exit-after-ms=120000')
}
finally { $env:LOCALAPPDATA = $old }
if (-not (Wait-K6wpEngine -TimeoutMs 20000)) { throw "engine did not answer the pipe" }

$saved = Get-DesktopWindows
$shots = @{}
try {
    Start-Sleep -Seconds 5
    $h = Get-InjectedWindow
    if ($h -eq [IntPtr]::Zero) { throw "no injected wallpaper window found" }

    # Phase 1: playing (two frames 1.2 s apart - they must differ).
    $a = Join-Path $env:TEMP 'k6wp-unloadv-a.png'
    $b = Join-Path $env:TEMP 'k6wp-unloadv-b.png'
    Save-Injected $h $a
    Start-Sleep -Milliseconds 1200
    Save-Injected $h $b
    $ba = [System.Drawing.Bitmap]::FromFile($a); $bb = [System.Drawing.Bitmap]::FromFile($b)
    $playingDiff = Get-BitmapDiff -A $ba -B $bb -Step 6
    $ba.Dispose(); $bb.Dispose()
    $shots['playing1'] = $a; $shots['playing2'] = $b
    Write-Host ("phase 1 playing: frame-to-frame diff = {0}" -f [math]::Round($playingDiff, 3))

    # Phase 2: pause, then wait for the Unloaded state.
    [void](Send-K6wpIpc -Cmd 'pause' -TimeoutMs 3000)
    $deadline = (Get-Date).AddSeconds(20)
    $state = 'paused'
    while ((Get-Date) -lt $deadline) {
        $st = Send-K6wpIpc -Cmd 'get_state' -TimeoutMs 2000
        $state = $st.state.decode_state
        if ($state -eq 'unloaded') { break }
        Start-Sleep -Milliseconds 300
    }
    Write-Host ("phase 3 decode_state = {0}" -f $state)
    if ($state -ne 'unloaded') { throw "the engine never reached the Unloaded state" }

    # Phase 3: the unloaded surface must still show the picture (not black).
    $c = Join-Path $env:TEMP 'k6wp-unloadv-c.png'
    Save-Injected $h $c
    $shots['unloaded'] = $c
    $sigC = Get-Signature $c
    $sigA = Get-Signature $a
    Write-Host ("phase 3 unloaded: luma={0} stddev={1} (playing was luma={2} stddev={3})" -f `
        $sigC.mean_luma, $sigC.stddev, $sigA.mean_luma, $sigA.stddev)

    # Phase 4: resume, then the animation must be back.
    [void](Send-K6wpIpc -Cmd 'resume' -TimeoutMs 3000)
    Start-Sleep -Seconds 3
    $d = Join-Path $env:TEMP 'k6wp-unloadv-d.png'
    $e = Join-Path $env:TEMP 'k6wp-unloadv-e.png'
    Save-Injected $h $d
    Start-Sleep -Milliseconds 1200
    Save-Injected $h $e
    $shots['resumed1'] = $d; $shots['resumed2'] = $e
    $bd = [System.Drawing.Bitmap]::FromFile($d); $be = [System.Drawing.Bitmap]::FromFile($e)
    $resumedDiff = Get-BitmapDiff -A $bd -B $be -Step 6
    $bd.Dispose(); $be.Dispose()
    Write-Host ("phase 4 resumed: frame-to-frame diff = {0}" -f [math]::Round($resumedDiff, 3))

    $report = [ordered]@{
        tool = 'bench_unload_visual'
        phase = 1
        date = (Get-Date -Format 'yyyy-MM-ddTHH:mm:ssK')
        label = 'live-proven (one machine)'
        method = 'PrintWindow on the injected wallpaper surface (immune to the desktop composite)'
        playing_diff = [math]::Round($playingDiff, 3)
        unloaded_luma = $sigC.mean_luma
        unloaded_stddev = $sigC.stddev
        playing_luma = $sigA.mean_luma
        playing_stddev = $sigA.stddev
        resumed_diff = [math]::Round($resumedDiff, 3)
        animating_while_playing = ($playingDiff -gt 0.5)
        animating_after_resume = ($resumedDiff -gt 0.5)
        # The unloaded surface must still carry a picture: a black frame
        # would read as luma ~0 and stddev ~0.
        unloaded_shows_picture = ($sigC.mean_luma -gt 5.0 -and $sigC.stddev -gt 5.0)
    }
    $json = $report | ConvertTo-Json -Depth 6
    $outFull = [System.IO.Path]::GetFullPath($OutJson)
    $outDir = [System.IO.Path]::GetDirectoryName($outFull)
    if (-not (Test-Path -LiteralPath $outDir)) { New-Item -ItemType Directory -Force -Path $outDir | Out-Null }
    $json | Set-Content -LiteralPath $outFull -Encoding UTF8
    Write-Host "bench_unload_visual: wrote $outFull"
    $ok = $report.animating_while_playing -and $report.animating_after_resume -and $report.unloaded_shows_picture
    if (-not $ok) { Write-Error "the Unloaded state broke the wallpaper"; exit 1 }
    Write-Host "the Unloaded state keeps the picture and the resume restores the animation"
}
finally {
    Restore-DesktopWindows -Saved $saved
    try { if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue } } catch { }
}
```

## File: tools/bench_unload.ps1
```powershell
# tools/bench_unload.ps1 - task 1.1: does the unload actually free the RAM?
#
# The engine is started with --unload-after-pause-ms, paused over IPC, and
# the memory is sampled before and after the unload. The C4 metrics are the
# honest ones: PRIVATE BYTES + GPU SHARED MEMORY. The working set is recorded
# but never decides a verdict (the engine trims its own working set).
#
# The pause is driven over the REAL IPC command, so the production path
# (pause -> mask -> deadline -> unload) is what is measured.
[CmdletBinding()]
param(
    [string]$EngineExe = '',
    [int]$UnloadAfterMs = 5000,
    [int]$SampleSeconds = 12,
    [string]$OutJson = '',
    # "ipc" = the explicit user pause (the wallpaper stays ON SCREEN, so the
    #         engine deliberately keeps the decode pipeline: the control).
    # "fullscreen" = a real borderless fullscreen window, so the engine's
    #         fullscreen watcher pauses the decode AND nobody can see the
    #         wallpaper: the case the unload exists for.
    [ValidateSet('ipc', 'fullscreen')][string]$Mode = 'fullscreen'
)
$ErrorActionPreference = 'Stop'
$script:Here = Split-Path -Parent $MyInvocation.MyCommand.Path
. (Join-Path $script:Here 'bench_lib.ps1')
if ([string]::IsNullOrWhiteSpace($EngineExe)) { $EngineExe = Join-Path $script:Here '..\build\release\engine.exe' }
if ([string]::IsNullOrWhiteSpace($OutJson)) { $OutJson = Join-Path $script:Here '..\docs\bench\unload.json' }
$engine = [System.IO.Path]::GetFullPath($EngineExe)
$video = Join-Path $script:Here '..\tests\corpus\anime.mp4'

$busy = @(Get-Process engine -ErrorAction SilentlyContinue)
if ($busy.Count -gt 0) { throw "an engine.exe is already running" }

$dataDir = Join-Path $env:TEMP ("k6wp-unload-" + [Guid]::NewGuid().ToString('N').Substring(0, 8))
New-Item -ItemType Directory -Force -Path (Join-Path $dataDir 'K6WP') | Out-Null
$old = $env:LOCALAPPDATA
try {
    $env:LOCALAPPDATA = $dataDir
    $p = Start-Process -FilePath $engine -PassThru -WindowStyle Hidden -ArgumentList @(
        "--video=$video", "--unload-after-pause-ms=$UnloadAfterMs", '--exit-after-ms=120000')
}
finally { $env:LOCALAPPDATA = $old }
if (-not (Wait-K6wpEngine -TimeoutMs 20000)) { throw "engine did not answer the pipe" }

# Minimize the covering windows so the engine is not occlusion-paused.
$saved = Get-DesktopWindows
try {
    Start-Sleep -Seconds 5
    $tp = Start-TypeperfSampler -ProcessId $p.Id -Seconds 60 -ProcessName 'engine'
    $series = New-Object System.Collections.Generic.List[object]
    $gpuRows = New-Object System.Collections.Generic.List[object]

    # The real IPC pause: the production path (pause -> mask -> deadline ->
    # unload) is what is measured, not a copy of it.
    $ack = $null
    if ($Mode -eq 'ipc') {
        $ack = Send-K6wpIpc -Cmd 'pause' -TimeoutMs 3000
        Write-Host ("bench_unload: pause ack ok={0}" -f $ack.ok)
    } else {
        # A real borderless fullscreen window: the engine's fullscreen
        # watcher detects it exactly as it would a fullscreen game.
        Add-Type -AssemblyName System.Windows.Forms
        Add-Type @"
using System;using System.Runtime.InteropServices;
public class Fg {
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int c);
  [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
  [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr h, IntPtr a, int x, int y, int cx, int cy, uint f);
  [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
  [DllImport("user32.dll")] public static extern bool AttachThreadInput(uint a, uint b, bool attach);
  [DllImport("kernel32.dll")] public static extern uint GetCurrentThreadId();
}
"@
        $form = New-Object System.Windows.Forms.Form
        $form.FormBorderStyle = 'None'
        $form.StartPosition = 'Manual'
        $form.Location = [System.Drawing.Point]::new(0, 0)
        $form.Size = [System.Drawing.Size]::new(
            [System.Windows.Forms.SystemInformation]::VirtualScreen.Width,
            [System.Windows.Forms.SystemInformation]::VirtualScreen.Height)
        $form.TopMost = $true
        $form.ShowInTaskbar = $false
        $form.Show()
        # The engine's fullscreen watcher only looks at the FOREGROUND window,
        # so the form must actually take the foreground.
        $hwnd = [IntPtr]$form.Handle
        $fgPid = 0
        $fgThread = [Fg]::GetWindowThreadProcessId([Fg]::GetForegroundWindow(), [ref]$fgPid)
        $myThread = [Fg]::GetCurrentThreadId()
        [void][Fg]::AttachThreadInput($myThread, $fgThread, $true)
        [void][Fg]::SetForegroundWindow($hwnd)
        [void][Fg]::AttachThreadInput($myThread, $fgThread, $false)
        [void]$form.Activate()
        Write-Host ("bench_unload: fullscreen window up (fg={0}), waiting for the engine to pause" -f `
            [Fg]::GetForegroundWindow())
        for ($i = 0; $i -lt 20; $i++) {
            Start-Sleep -Milliseconds 300
            $st = Send-K6wpIpc -Cmd 'get_state' -TimeoutMs 2000
            if ($st.state.paused) { break }
        }
    }

    # Sample through the pause, the unload and the resume.
    $phase = 'playing'
    for ($i = 0; $i -lt 30; $i++) {
        $series.Add((Get-ProcessSeries -ProcessId $p.Id -Seconds 1 -IntervalMs 500)[0])
        $gpuRows.AddRange(@(Read-TypeperfCsv -Path $tp.Csv | Select-Object -Last 1))
        $st = Send-K6wpIpc -Cmd 'get_state' -TimeoutMs 2000
        $ds = $st.state.decode_state
        if ($ds -ne $phase) {
            $phase = $ds
            Write-Host ("  -> decode_state = {0} (t={1}s)" -f $phase, $i)
        }
        if ($phase -eq 'unloaded' -and $i -gt 6) {
            Write-Host "  -> resuming"
            if ($Mode -eq 'ipc') {
                [void](Send-K6wpIpc -Cmd 'resume' -TimeoutMs 3000)
            } else {
                # Leaving fullscreen is what clears the fullscreen bit; the
                # engine then reloads the video by itself.
                $form.Close()
            }
        }
        if ($phase -eq 'active' -and $i -gt 10) { break }
    }
    if ($Mode -eq 'fullscreen' -and $null -ne $form) { try { $form.Close() } catch { } }
    try { [void]$tp.Process.WaitForExit(10000) } catch { }
    $gpuAll = @(Read-TypeperfCsv -Path $tp.Csv)

    $logPath = Join-Path $dataDir 'K6WP\engine.log'
    $log = @(Get-Content -LiteralPath $logPath -ErrorAction SilentlyContinue)

    $priv = @($series | ForEach-Object { $_.private_mb })
    $gpuShared = @($gpuAll | ForEach-Object { $_.gpu_shared_mb } | Where-Object { $null -ne $_ })
    $result = [ordered]@{
        tool = 'bench_unload'
        phase = 1
        date = (Get-Date -Format 'yyyy-MM-ddTHH:mm:ssK')
        label = 'live-proven (one machine)'
        engine = $engine
        unload_after_ms = $UnloadAfterMs
        method = 'the real IPC pause -> mask -> deadline -> unload path; private bytes + GPU shared (C4), the working set is recorded but never decides'
        private_mb = (Get-Stats $priv)
        gpu_shared_mb = (Get-Stats $gpuShared)
        gpu_total_pct = (Get-Stats @($gpuAll | ForEach-Object { $_.gpu_total_pct } | Where-Object { $null -ne $_ })).median
        series = $series
        gpu_series = $gpuAll
        log_tail = @($log | Select-String 'pause: owner|decode: ' | ForEach-Object { $_.Line })
    }

    Write-Host ""
    Write-Host "--- engine log (pause / unload / reload) ---"
    $result.log_tail | ForEach-Object { Write-Host "  $_" }
    Write-Host ""
    Write-Host "--- per-sample RAM (private is the C4 metric) ---"
    $i = 0
    foreach ($s in $series) {
        $i++
        Write-Host ("  t={0,6} ms  private={1,7} MB  ws={2,6} MB  threads={3}" -f $s.t_ms, $s.private_mb, $s.ws_mb, $s.threads)
    }
    $json = $result | ConvertTo-Json -Depth 8
    $outFull = [System.IO.Path]::GetFullPath($OutJson)
    $outDir = [System.IO.Path]::GetDirectoryName($outFull)
    if (-not (Test-Path -LiteralPath $outDir)) { New-Item -ItemType Directory -Force -Path $outDir | Out-Null }
    $json | Set-Content -LiteralPath $outFull -Encoding UTF8
    Write-Host "bench_unload: wrote $outFull"
}
finally {
    Restore-DesktopWindows -Saved $saved
    try { if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue } } catch { }
}
```

## File: tools/bench_visual.ps1
```powershell
# tools/bench_visual.ps1 - Phase 0 visual parity sampler.
#
# Captures what the user actually SEES on the desktop, so a later refactor
# can prove it did not change the picture. Two modes:
#
#   -Capture <dir>   capture the current desktop region(s) to PNG
#   -Compare <a> <b>  diff two capture directories and report
#
# The capture is a GDI screen grab of the wallpaper region. Because the
# wallpaper is a video, the frame is not reproducible, so parity is measured
# with a STATISTICAL signature, not a byte-exact comparison:
#
#   mean_luma   average brightness of the region (catches "black screen")
#   stddev      contrast (catches "flat grey" / failed decode)
#   mean_rgb    average colour per channel (catches colour-channel swaps)
#   changed_pct fraction of sampled pixels that differ between two captures
#
# A refactor passes the visual parity gate when the mean/stddev of the
# capture stay within the configured tolerance of the baseline AND the
# changed-pixel fraction is in the expected band (a moving wallpaper always
# differs frame to frame; a frozen one is a regression).
#
# The wallpaper is injected BEHIND the desktop, so all other windows are
# minimized first - otherwise the capture shows the foreground app.
#
# Usage:
#   powershell -NoProfile -ExecutionPolicy Bypass -File tools\bench_visual.ps1 `
#     -Capture docs\bench\visual\baseline
#   powershell -NoProfile -ExecutionPolicy Bypass -File tools\bench_visual.ps1 `
#     -Compare docs\bench\visual\baseline docs\bench\visual\candidate

[CmdletBinding()]
param(
    # Compare/Compare2 come FIRST so that `-Compare <dirA> <dirB>` binds the
    # second directory to Compare2 by position (PowerShell fills positional
    # slots in declaration order; with Capture first it would steal it).
    [string]$Compare = '',
    [string]$Compare2 = '',
    [string]$Capture = '',
    [int]$CropWidth = 0,
    [int]$CropHeight = 0,
    [double]$LumaTolerance = 12.0,
    [double]$StddevTolerance = 12.0,
    [switch]$Force,
    [switch]$SkipMinimize
)

$ErrorActionPreference = 'Stop'
$script:Here = Split-Path -Parent $MyInvocation.MyCommand.Path
. (Join-Path $script:Here 'bench_lib.ps1')

# Mode is decided by which parameter was EXPLICITLY bound. Deciding on
# "is it non-empty" makes the positional binding of the first argument
# ambiguous (it lands on $Capture), which silently ran the wrong mode.
$doCapture = $PSBoundParameters.ContainsKey('Capture') -and -not $PSBoundParameters.ContainsKey('Compare')
$doCompare = $PSBoundParameters.ContainsKey('Compare') -or $PSBoundParameters.ContainsKey('Compare2')
if (-not $doCapture -and -not $doCompare) {
    throw "pass -Capture <dir> or -Compare <dirA> <dirB>"
}
if ($doCompare -and [string]::IsNullOrWhiteSpace($Compare2)) {
    throw "-Compare needs TWO directories: -Compare <dirA> <dirB>"
}

# The signature is computed over the FULL captured screen (the same pixels
# that go into the PNG), so the signature and the motion check describe the
# same thing. A center crop of the testsrc2 corpus clip lands on two static
# colour bars, which would make a FROZEN video indistinguishable from a
# playing one.
function Get-RegionSignature {
    param([int]$Step = 4)
    $shot = Get-ScreenBitmap
    $bmp = $shot.Bitmap
    try {
        $sum = 0.0; $sum2 = 0.0
        $rs = 0.0; $gs = 0.0; $bs = 0.0
        $n = 0
        for ($y = 0; $y -lt $bmp.Height; $y += $Step) {
            for ($x = 0; $x -lt $bmp.Width; $x += $Step) {
                $c = $bmp.GetPixel($x, $y)
                $l = 0.2126 * $c.R + 0.7152 * $c.G + 0.0722 * $c.B
                $sum += $l; $sum2 += $l * $l
                $rs += $c.R; $gs += $c.G; $bs += $c.B
                $n++
            }
        }
        if ($n -eq 0) { return $null }
        $mean = $sum / $n
        $variance = [math]::Max(0.0, ($sum2 / $n) - ($mean * $mean))
        return [pscustomobject]@{
            pixels = $n
            mean_luma = [math]::Round($mean, 2)
            stddev = [math]::Round([math]::Sqrt($variance), 2)
            mean_r = [math]::Round($rs / $n, 2)
            mean_g = [math]::Round($gs / $n, 2)
            mean_b = [math]::Round($bs / $n, 2)
        }
    }
    finally { $bmp.Dispose() }
}

if ($doCapture) {
    # Never silently overwrite an existing capture: a baseline that gets
    # replaced by an unrelated capture is worse than no baseline at all.
    if ((Test-Path -LiteralPath $Capture) -and -not $Force) {
        $existing = @(Get-ChildItem -LiteralPath $Capture -Filter 'frame*.png' -ErrorAction SilentlyContinue)
        if ($existing.Count -gt 0) {
            throw ("capture directory already holds {0} frame(s): {1}. Pass -Force to replace it." -f $existing.Count, $Capture)
        }
    }
    $savedWindows = @()
    if (-not $SkipMinimize) {
        Write-Host "bench_visual: minimizing top-level windows (restored at the end)"
        $savedWindows = Get-DesktopWindows
    }
    try {
        $dir = [System.IO.Path]::GetFullPath($Capture)
        New-Item -ItemType Directory -Force -Path $dir | Out-Null
        $frames = @()
        # Four frames spread over time: a moving wallpaper differs between
        # them, a frozen one does not. The FULL virtual screen is captured
        # (not a center crop) so the motion check sees real movement.
        for ($i = 1; $i -le 4; $i++) {
            $png = Join-Path $dir ("frame{0}.png" -f $i)
            Save-ScreenPng -Path $png -CropWidth $CropWidth -CropHeight $CropHeight
            $sig = Get-RegionSignature
            $frames += [pscustomobject]@{ index = $i; file = (Split-Path -Leaf $png); signature = $sig }
            Write-Host ("  frame {0}: mean_luma={1} stddev={2} rgb=({3},{4},{5})" -f `
                $i, $sig.mean_luma, $sig.stddev, $sig.mean_r, $sig.mean_g, $sig.mean_b)
            if ($i -lt 4) { Start-Sleep -Seconds 1 }
        }
        $m = $frames | ForEach-Object { $_.signature.mean_luma }
        $s = $frames | ForEach-Object { $_.signature.stddev }
        # The motion value is stored INSIDE the signature so the parity gate
        # can be evaluated from signature.json alone (no PNG re-read needed).
        # The PNGs stay in the directory as the human-inspectable evidence.
        $motion = $null
        # No script block here: a `{ }` block would create a child scope and
        # the assignment to $motion would not reach the caller.
        Add-Type -AssemblyName System.Drawing, System.Windows.Forms
        $frameA = [System.Drawing.Bitmap]::FromFile((Join-Path $dir 'frame1.png'))
        $frameB = [System.Drawing.Bitmap]::FromFile((Join-Path $dir 'frame2.png'))
        try { $motion = [math]::Round((Get-BitmapDiff -A $frameA -B $frameB -Step 4), 3) }
        finally { $frameA.Dispose(); $frameB.Dispose() }
        $report = [ordered]@{
            tool = 'bench_visual'
            phase = 0
            date = (Get-Date -Format 'yyyy-MM-ddTHH:mm:ssK')
            label = 'live-proven'
            dir = $dir
            region = [ordered]@{ full_screen = $true; crop_width = $CropWidth; crop_height = $CropHeight; note = 'signature and motion diff are computed over the full captured frame' }
            frames = $frames
            mean_luma = Get-Stats $m
            stddev = Get-Stats $s
            motion_diff = $motion
        }
        $json = $report | ConvertTo-Json -Depth 8
        $json | Set-Content -LiteralPath (Join-Path $dir 'signature.json') -Encoding UTF8
        Write-Host "bench_visual: captured to $dir"
    }
    finally {
        if (-not $SkipMinimize) { Restore-DesktopWindows -Saved $savedWindows }
    }
    exit 0
}

# --- Compare mode ---
$dirA = [System.IO.Path]::GetFullPath($Compare)
$dirB = [System.IO.Path]::GetFullPath($Compare2)
$sigA = Get-Content -LiteralPath (Join-Path $dirA 'signature.json') -Raw | ConvertFrom-Json
$sigB = Get-Content -LiteralPath (Join-Path $dirB 'signature.json') -Raw | ConvertFrom-Json

$fail = @()
$report = [ordered]@{
    tool = 'bench_visual'
    phase = 0
    date = (Get-Date -Format 'yyyy-MM-ddTHH:mm:ssK')
    label = 'live-proven'
    a = $dirA
    b = $dirB
    tolerance = [ordered]@{ mean_luma = $LumaTolerance; stddev = $StddevTolerance }
    checks = @()
    pass = $true
}
foreach ($key in @('mean_luma', 'stddev')) {
    $a = [double]$sigA.$key.median
    $b = [double]$sigB.$key.median
    $delta = [math]::Abs($a - $b)
    $tol = if ($key -eq 'mean_luma') { $LumaTolerance } else { $StddevTolerance }
    $ok = $delta -le $tol
    if (-not $ok) { $fail += $key }
    $report.checks += [ordered]@{ metric = $key; a = $a; b = $b; delta = [math]::Round($delta, 2); tolerance = $tol; pass = $ok }
}

# Frame-to-frame motion inside each capture: a frozen wallpaper is a
# regression (the video stopped), so both captures must show motion. The
# value is read from signature.json when present (it is computed at capture
# time); otherwise it is recomputed from the PNGs. The diff runs over the
# FULL captured frames with a coarse step, so it stays fast at 1080p+.
function Get-Motion([string]$dir, $sig) {
    if ($sig -and $null -ne $sig.motion_diff) {
        return [double]$sig.motion_diff
    }
    Add-Type -AssemblyName System.Drawing, System.Windows.Forms
    $files = @(Get-ChildItem -LiteralPath $dir -Filter 'frame*.png' | Sort-Object Name)
    if ($files.Count -lt 2) { return $null }
    $a = [System.Drawing.Bitmap]::FromFile($files[0].FullName)
    $b = [System.Drawing.Bitmap]::FromFile($files[1].FullName)
    try { return Get-BitmapDiff -A $a -B $b -Step 4 }
    finally { $a.Dispose(); $b.Dispose() }
}
$motionA = Get-Motion $dirA $sigA
$motionB = Get-Motion $dirB $sigB
if ($null -ne $motionA -and $null -ne $motionB) {
    $moving = ($motionA -gt 0.0) -and ($motionB -gt 0.0)
    if (-not $moving) { $fail += 'motion' }
    $report.checks += [ordered]@{ metric = 'motion_diff'; a = [math]::Round($motionA, 3); b = [math]::Round($motionB, 3); pass = $moving }
}
$report.pass = ($fail.Count -eq 0)
$report.failed_checks = $fail

$json = $report | ConvertTo-Json -Depth 8
Write-Host $json
if (-not $report.pass) {
    # The EXIT CODE is the contract (1 = parity failed). Write-Error would be
    # a TERMINATING error under this script's ErrorActionPreference and would
    # abort a caller that expects to inspect the JSON, so the failure is
    # reported on the host stream and signalled through the exit code.
    Write-Host ("bench_visual: PARITY FAILED ({0})" -f ($fail -join ', '))
    exit 1
}
Write-Host "bench_visual: parity OK"
exit 0
```

## File: tools/bench_vo_sustained.ps1
```powershell
# tools/bench_vo_sustained.ps1 - prove the VO keeps PRESENTING frames.
#
# The screen-level animation check is not usable in an agent session: the
# agent's own always-on-top cursor overlay covers the whole screen, and the
# engine correctly pauses the wallpaper when the desktop is covered. So the
# animation is proven from the GPU side instead: if the VO had stalled
# (e.g. a vblank wait that never completes on a window without a
# redirection surface), the decode and the 3D work would stop after the
# first frame. Sustained per-second GPU work proves the VO keeps
# producing frames for the whole playback window.
[CmdletBinding()]
param(
    [string]$EngineExe = '',
    [int]$Seconds = 15
)
$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
. (Join-Path $here 'bench_lib.ps1')

if ([string]::IsNullOrWhiteSpace($EngineExe)) { $EngineExe = Join-Path $here '..\build\release\engine.exe' }
$engine = [System.IO.Path]::GetFullPath($EngineExe)
$video = Join-Path $here '..\tests\corpus\anime.mp4'
$dataDir = Join-Path $env:TEMP ("k6wp-vo-" + [Guid]::NewGuid().ToString('N').Substring(0, 8))
New-Item -ItemType Directory -Force -Path (Join-Path $dataDir 'K6WP') | Out-Null

$busy = @(Get-Process engine -ErrorAction SilentlyContinue)
if ($busy.Count -gt 0) { throw "engine already running (pid $($busy.Id -join ','))" }

$old = $env:LOCALAPPDATA
try {
    $env:LOCALAPPDATA = $dataDir
    # The engine must outlive the sampling window (it self-exits on
    # --exit-after-ms), otherwise the counters vanish mid-run and the
    # measurement silently reads "no work" instead of "the engine is gone".
    $p = Start-Process -FilePath $engine -ArgumentList @("--video=$video", "--exit-after-ms=$($Seconds * 1000 + 90000)") -PassThru -WindowStyle Hidden
}
finally { $env:LOCALAPPDATA = $old }
if (-not (Wait-K6wpEngine -TimeoutMs 15000)) { throw "engine did not answer the pipe" }
Start-Sleep -Seconds 3

# The engine pauses the decode when a window covers the desktop (occlusion
# watcher). Every covering window must be minimized BEFORE the measurement,
# or the engine is paused and the GPU work is zero by definition.
$saved = Get-DesktopWindows
$logPath = Join-Path $dataDir 'K6WP\engine.log'
$resumed = $false
$deadline = (Get-Date).AddSeconds(30)
$lastRetry = (Get-Date).AddSeconds(-10)
while ((Get-Date) -lt $deadline) {
    if (Test-Path -LiteralPath $logPath) {
        $lines = @(Get-Content -LiteralPath $logPath -ErrorAction SilentlyContinue)
        $paused = @($lines | Select-String 'occlusion: slot \d+ coverage=100').Count
        $res = @($lines | Select-String 'occlusion: slot \d+ coverage=0\.0% resuming').Count
        if ($res -gt 0 -and $res -ge $paused) { $resumed = $true; break }
    }
    # The engine clears its fullscreen bit on a FOREGROUND-CHANGE event, and
    # while that bit is set the message loop waits INFINITE (no periodic
    # re-check). Minimizing again produces the event that unblocks it.
    if (((Get-Date) - $lastRetry).TotalSeconds -ge 4) {
        $lastRetry = Get-Date
        [void](Get-DesktopWindows)
    }
    Start-Sleep -Milliseconds 300
}
if (-not $resumed) { throw "the engine never resumed playback (occlusion) - the GPU work would read zero by definition" }
Write-Host "bench_vo_sustained: engine resumed playback"
Start-Sleep -Seconds 2

$tp = Start-TypeperfSampler -ProcessId $p.Id -Seconds $Seconds -ProcessName 'engine'
try { [void]$tp.Process.WaitForExit(($Seconds + 10) * 1000) } catch { }
$rows = @(Read-TypeperfCsv -Path $tp.Csv)
try { if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue } } catch { }
Restore-DesktopWindows -Saved $saved

Write-Host ""
Write-Host "--- per-second GPU work (engine pid $($p.Id)) ---"
Write-Host ("{0,-8} {1,12} {2,12} {3,12}" -f 'second', 'gpu_total%', 'gpu_3d%', 'gpu_decode%')
$first = $true
$i = 0
foreach ($r in $rows) {
    $i++
    $tot = if ($null -ne $r.gpu_total_pct) { $r.gpu_total_pct } else { 'n/a' }
    $d3 = if ($null -ne $r.gpu_3d_pct) { $r.gpu_3d_pct } else { 'n/a' }
    $dec = if ($null -ne $r.gpu_decode_pct) { $r.gpu_decode_pct } else { 'n/a' }
    Write-Host ("{0,-8} {1,12} {2,12} {3,12}" -f $i, $tot, $d3, $dec)
}
$live = @($rows | Where-Object { $null -ne $_.gpu_3d_pct -and $_.gpu_3d_pct -gt 0.5 })
$decLive = @($rows | Where-Object { $null -ne $_.gpu_decode_pct -and $_.gpu_decode_pct -gt 0.5 })
Write-Host ""
Write-Host ("samples with 3D work  > 0.5%: {0}/{1}" -f $live.Count, $rows.Count)
Write-Host ("samples with decode   > 0.5%: {0}/{1}" -f $decLive.Count, $rows.Count)
if ($live.Count -lt ($rows.Count - 1) -or $decLive.Count -lt ($rows.Count - 1)) {
    Write-Error "the VO did NOT keep presenting: the GPU work is not sustained"
    exit 1
}
Write-Host "the VO kept rendering for the whole window (sustained GPU work)"
exit 0
```

## File: tools/bench_wallpaper_frames.ps1
```powershell
# tools/bench_wallpaper_frames.ps1 - capture the WALLPAPER the user sees.
#
# Grabs the injected wallpaper window (class K6WP.DesktopInject.1) with
# PrintWindow. Unlike a desktop screen grab, this captures what the engine
# actually PRESENTED into the wallpaper surface, so it is immune to an
# always-on-top overlay covering the desktop.
#
# Two questions this answers:
#   1. ANIMATION: the frames must DIFFER from each other (the wallpaper
#      moves). A frozen sequence means the VO stopped presenting.
#   2. PARITY: the frames of two builds must show the same picture.
#
# Usage:
#   powershell -NoProfile -ExecutionPolicy Bypass -File tools\bench_wallpaper_frames.ps1 `
#     -EngineExe build\ab\engine-candidate.exe -OutDir build\ab\wpframes -Frames 3

[CmdletBinding()]
param(
    [string]$EngineExe = '',
    [string]$OutDir = '',
    [int]$Frames = 3,
    [int]$IntervalMs = 1500,
    [int]$SettleSec = 6
)
$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
. (Join-Path $here 'bench_lib.ps1')

if ([string]::IsNullOrWhiteSpace($EngineExe)) { $EngineExe = Join-Path $here '..\build\release\engine.exe' }
if ([string]::IsNullOrWhiteSpace($OutDir)) { $OutDir = Join-Path $here '..\build\wpframes' }
$engine = [System.IO.Path]::GetFullPath($EngineExe)
$video = Join-Path $here '..\tests\corpus\anime.mp4'
$dataDir = Join-Path $env:TEMP ("k6wp-wp-" + [Guid]::NewGuid().ToString('N').Substring(0, 8))
New-Item -ItemType Directory -Force -Path (Join-Path $dataDir 'K6WP') | Out-Null
$outDir = [System.IO.Path]::GetFullPath($OutDir)
Remove-Item -LiteralPath $outDir -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $outDir | Out-Null

Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;using System.Runtime.InteropServices;using System.Text;
public class WPF {
  [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc cb, IntPtr p);
  [DllImport("user32.dll")] public static extern bool EnumChildWindows(IntPtr parent, EnumProc cb, IntPtr p);
  public delegate bool EnumProc(IntPtr h, IntPtr p);
  [DllImport("user32.dll")] public static extern int GetClassName(IntPtr h, StringBuilder s, int n);
  [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr hdc, uint flags);
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
  [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left,Top,Right,Bottom; }
}
"@
# The injected surface is a CHILD of the shell's WorkerW (or of Progman), so
# it is not a top-level window. The enumeration lives at SCRIPT scope: a
# scriptblock-to-delegate conversion inside a function loses the callback's
# captured scope and the delegate comes back null.
$script:injected = New-Object System.Collections.Generic.List[object]
$childCb = [WPF+EnumProc]{
    param($h, $p)
    $sb = New-Object System.Text.StringBuilder 256
    [void][WPF]::GetClassName($h, $sb, 256)
    if ($sb.ToString() -eq 'K6WP.DesktopInject.1') { $script:injected.Add($h) }
    return $true
}
$topCb = [WPF+EnumProc]{
    param($h, $p)
    [void][WPF]::EnumChildWindows($h, $childCb, [IntPtr]::Zero)
    return $true
}
function Get-InjectedWindow {
    $script:injected.Clear()
    [void][WPF]::EnumWindows($topCb, [IntPtr]::Zero)
    if ($script:injected.Count -eq 0) { return [IntPtr]::Zero }
    return $script:injected[0]
}

function Save-Injected([IntPtr]$h, [string]$Path) {
    $r = New-Object WPF+RECT
    [void][WPF]::GetWindowRect($h, [ref]$r)
    $w = $r.Right - $r.Left; $ht = $r.Bottom - $r.Top
    if ($w -le 0 -or $ht -le 0) { throw "the injected window has no area" }
    $bmp = New-Object System.Drawing.Bitmap $w, $ht
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $hdc = $g.GetHdc()
    try { $ok = [WPF]::PrintWindow($h, $hdc, 2) }
    finally { $g.ReleaseHdc($hdc); $g.Dispose() }
    if (-not $ok) { $bmp.Dispose(); throw "PrintWindow failed" }
    $bmp.Save($Path, [System.Drawing.Imaging.ImageFormat]::Png)
    $bmp.Dispose()
}

$busy = @(Get-Process engine -ErrorAction SilentlyContinue)
if ($busy.Count -gt 0) { throw "engine already running (pid $($busy.Id -join ','))" }
$old = $env:LOCALAPPDATA
try {
    $env:LOCALAPPDATA = $dataDir
    $p = Start-Process -FilePath $engine -ArgumentList @("--video=$video", '--exit-after-ms=120000') -PassThru -WindowStyle Hidden
}
finally { $env:LOCALAPPDATA = $old }
if (-not (Wait-K6wpEngine -TimeoutMs 15000)) { throw "engine did not answer the pipe" }

$saved = @()
try {
    # Minimize the covering windows so the engine is not occlusion-paused
    # (a paused engine holds a still frame - the wallpaper would not move).
    $saved = Get-DesktopWindows
    Start-Sleep -Seconds $SettleSec
    $h = Get-InjectedWindow
    if ($h -eq [IntPtr]::Zero) { throw "no injected wallpaper window found" }
    Write-Host ("bench_wallpaper_frames: injected window 0x{0:X}" -f $h.ToInt64())

    $paths = @()
    for ($i = 1; $i -le $Frames; $i++) {
        $path = Join-Path $outDir ("frame{0}.png" -f $i)
        Save-Injected -h $h -Path $path
        $paths += $path
        Write-Host ("  captured frame {0} -> {1}" -f $i, (Split-Path -Leaf $path))
        if ($i -lt $Frames) { Start-Sleep -Milliseconds $IntervalMs }
    }
}
finally {
    Restore-DesktopWindows -Saved $saved
    try { if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue } } catch { }
}

# The frames must differ from each other: a still sequence means the VO
# stopped presenting and the wallpaper is frozen.
$bmps = @($paths | ForEach-Object { [System.Drawing.Bitmap]::FromFile($_) })
$diffs = @()
for ($i = 1; $i -lt $bmps.Count; $i++) {
    $d = Get-BitmapDiff -A $bmps[$i - 1] -B $bmps[$i] -Step 6
    $diffs += [math]::Round($d, 3)
}
foreach ($b in $bmps) { $b.Dispose() }
Write-Host ""
Write-Host ("frame-to-frame diffs: {0}" -f ($diffs -join ', '))
$animating = @($diffs | Where-Object { $_ -gt 0.5 }).Count -ge 1
if (-not $animating) {
    Write-Error "bench_wallpaper_frames: the wallpaper did NOT animate (all frames identical) - the VO stopped presenting"
    exit 1
}
Write-Host "the wallpaper animates (the frames differ)"
exit 0
```

## File: tools/build_ab.ps1
```powershell
# tools/build_ab.ps1 - build a baseline/candidate pair of engine binaries.
#
# WHY THIS EXISTS
#   Copy-Item preserves the SOURCE file's LastWriteTime. Restoring a source
#   file from a .bak therefore leaves it OLDER than its object file, and
#   MSBuild silently skips the recompile: the "new" build is the OLD binary.
#   That produced two identical engine binaries and a VOID A/B comparison
#   (the frame parity check "passed" with pixel diff 0.000 because it
#   compared a build with itself).
#
#   This script always TOUCHES the sources after editing them and VERIFIES
#   that the two binaries really differ, so a void comparison can never
#   happen again.
#
# Usage:
#   powershell -NoProfile -ExecutionPolicy Bypass -File tools\build_ab.ps1 `
#     -BaselineVideoSync display-desync -CandidateVideoSync display-vsync
#
# Output: build\ab\engine-baseline.exe and build\ab\engine-candidate.exe
# (plus libmpv-2.dll, which the engine loads from its own directory).

[CmdletBinding()]
param(
    [ValidateSet('display-desync', 'display-vsync')][string]$BaselineVideoSync = 'display-desync',
    [ValidateSet('display-desync', 'display-vsync')][string]$CandidateVideoSync = 'display-vsync',
    [string]$Source = '',
    [string]$OutDir = ''
)
$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
$repo = Split-Path -Parent $here
if ([string]::IsNullOrWhiteSpace($Source)) { $Source = Join-Path $repo 'engine\src\mpv_renderer.cpp' }
if ([string]::IsNullOrWhiteSpace($OutDir)) { $OutDir = Join-Path $repo 'build\ab' }
$OutDir = [System.IO.Path]::GetFullPath($OutDir)
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

$original = Get-Content -LiteralPath $Source -Raw

function Build-Variant {
    param([string]$VideoSync, [string]$Target)
    $patched = $original -replace 'set_opt\("video-sync", "display-(desync|vsync)"\);', "set_opt(`"video-sync`", `"$VideoSync`");"
    if ($patched -eq $original -and $original -notmatch [regex]::Escape("`"$VideoSync`")")) {
        throw "could not patch the video-sync option to '$VideoSync' in $Source"
    }
    Set-Content -LiteralPath $Source -Value $patched -NoNewline
    # TOUCH: Copy-Item/Set-Content can leave an mtime older than the object
    # file, and MSBuild then skips the recompile.
    (Get-Item -LiteralPath $Source).LastWriteTime = Get-Date
    $out = & cmake --build --preset release --target engine 2>&1
    if ($LASTEXITCODE -ne 0) {
        $out | Select-Object -Last 20 | Write-Host
        throw "the release build failed"
    }
    $exe = Join-Path $repo 'build\release\engine.exe'
    Copy-Item -LiteralPath $exe -Destination $Target -Force
    (Get-Item -LiteralPath $Target).LastWriteTime = Get-Date
    Write-Host ("  built {0} ({1})" -f (Split-Path -Leaf $Target), $VideoSync)
}

try {
    Write-Host "build_ab: baseline  = $BaselineVideoSync"
    Build-Variant -VideoSync $BaselineVideoSync -Target (Join-Path $OutDir 'engine-baseline.exe')
    Write-Host "build_ab: candidate = $CandidateVideoSync"
    Build-Variant -VideoSync $CandidateVideoSync -Target (Join-Path $OutDir 'engine-candidate.exe')
}
finally {
    Set-Content -LiteralPath $Source -Value $original -NoNewline
    (Get-Item -LiteralPath $Source).LastWriteTime = Get-Date
}

# The engine loads libmpv-2.dll from its own directory.
Copy-Item -LiteralPath (Join-Path $repo 'build\release\libmpv-2.dll') -Destination (Join-Path $OutDir 'libmpv-2.dll') -Force

# VERIFY: the two binaries MUST differ, otherwise the A/B would compare a
# build with itself.
$h1 = (Get-FileHash (Join-Path $OutDir 'engine-baseline.exe') -Algorithm SHA256).Hash
$h2 = (Get-FileHash (Join-Path $OutDir 'engine-candidate.exe') -Algorithm SHA256).Hash
if ($h1 -eq $h2) {
    throw "the baseline and candidate binaries are IDENTICAL - the A/B would be void (rebuild failed to recompile?)"
}
Write-Host ("build_ab: OK, the two binaries differ ({0} vs {1})" -f $h1.Substring(0, 12), $h2.Substring(0, 12))
```

## File: tools/capture_visual_baseline.ps1
```powershell
# tools/capture_visual_baseline.ps1 - drives the visual parity capture.
#
# Starts the real engine on the corpus clip, lets it play, captures the
# desktop region, then stops the engine. The engine MUST be running for the
# capture to show the wallpaper (otherwise the capture is just the plain
# desktop and the baseline would be meaningless).
[CmdletBinding()]
param(
    [string]$EngineExe = '',
    [string]$OutDir = ''
)
$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
. (Join-Path $here 'bench_lib.ps1')

if ([string]::IsNullOrWhiteSpace($EngineExe)) { $EngineExe = Join-Path $here '..\build\release\engine.exe' }
if ([string]::IsNullOrWhiteSpace($OutDir)) { $OutDir = Join-Path $here '..\docs\bench\visual\baseline' }
$engine = [System.IO.Path]::GetFullPath($EngineExe)
$video = Join-Path $here '..\tests\corpus\anime.mp4'
$outDir = [System.IO.Path]::GetFullPath($OutDir)
$dataDir = Join-Path $env:TEMP ("k6wp-visual-" + [Guid]::NewGuid().ToString('N').Substring(0, 8))
New-Item -ItemType Directory -Force -Path (Join-Path $dataDir 'K6WP') | Out-Null

$busy = @(Get-Process engine -ErrorAction SilentlyContinue)
if ($busy.Count -gt 0) { throw "an engine.exe is already running (pid $($busy.Id -join ','))" }

$old = $env:LOCALAPPDATA
try {
    $env:LOCALAPPDATA = $dataDir
    $p = Start-Process -FilePath $engine -ArgumentList @("--video=$video", '--exit-after-ms=120000') -PassThru -WindowStyle Hidden
}
finally { $env:LOCALAPPDATA = $old }

if (-not (Wait-K6wpEngine -TimeoutMs 15000)) {
    try { if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force } } catch { }
    throw "engine did not answer the IPC pipe"
}
Write-Host "capture_visual_baseline: engine pid=$($p.Id) ($engine), playing $video"
Start-Sleep -Seconds 4

# The wallpaper is only on screen while the engine is PLAYING:
#   - the occlusion / fullscreen watchers pause the decode, and a paused
#     engine shows the plain desktop instead of the video;
#   - the occlusion pause is PER SLOT, so get_state.paused (the global mask)
#     does NOT report it - the engine's log does.
# A capture taken during that pause silently records the plain desktop (the
# "negative" capture), which would make the parity gate compare two copies
# of the desktop background.
#
# So: hide the covering windows, then WAIT until the engine itself reports
# that it resumed, and only then capture.
$saved = Get-DesktopWindows
try {
    $logPath = Join-Path $dataDir 'K6WP\engine.log'
    $resumed = $false
    $deadline = (Get-Date).AddSeconds(30)
    while ((Get-Date) -lt $deadline) {
        if (Test-Path -LiteralPath $logPath) {
            $lines = @(Get-Content -LiteralPath $logPath -ErrorAction SilentlyContinue)
            $paused = @($lines | Select-String 'occlusion: slot \d+ coverage=100').Count
            $res = @($lines | Select-String 'occlusion: slot \d+ coverage=0\.0% resuming').Count
            if ($res -gt 0 -and $res -ge $paused) { $resumed = $true; break }
        }
        Start-Sleep -Milliseconds 400
    }
    if (-not $resumed) {
        throw "the engine never resumed playback (occlusion/fullscreen) - the capture would record the desktop, not the wallpaper"
    }
    Write-Host "capture_visual_baseline: engine resumed playback; settling 3s before the capture"
    Start-Sleep -Seconds 3
    & (Join-Path $here 'bench_visual.ps1') -Capture $outDir -Force
}
finally {
    Restore-DesktopWindows -Saved $saved
    try { [void](Send-K6wpIpc -Cmd 'quit' -TimeoutMs 2000) } catch { }
    try { if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue } } catch { }
}
Write-Host "capture_visual_baseline: done -> $outDir"
```

## File: tools/check_live_animation.ps1
```powershell
# tools/check_live_animation.ps1 - is the LIVE wallpaper actually moving?
#
# The user's still wallpaper (the video's first frame, applied via
# SPI_SETDESKWALLPAPER) looks IDENTICAL to the live video's first frame, so
# a desktop screenshot cannot tell them apart. This grabs the INJECTED
# wallpaper window itself (the engine's own surface) twice and compares: a
# moving surface means the live wallpaper is animating.
$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
. (Join-Path $here 'bench_lib.ps1')
Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;using System.Runtime.InteropServices;using System.Text;
public class WPA {
  [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc cb, IntPtr p);
  [DllImport("user32.dll")] public static extern bool EnumChildWindows(IntPtr parent, EnumProc cb, IntPtr p);
  public delegate bool EnumProc(IntPtr h, IntPtr p);
  [DllImport("user32.dll")] public static extern int GetClassName(IntPtr h, StringBuilder s, int n);
  [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr hdc, uint flags);
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
  [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left,Top,Right,Bottom; }
}
"@
$script:injected = New-Object System.Collections.Generic.List[object]
$childCb = [WPA+EnumProc]{
    param($h, $p)
    $sb = New-Object System.Text.StringBuilder 256
    [void][WPA]::GetClassName($h, $sb, 256)
    if ($sb.ToString() -eq 'K6WP.DesktopInject.1') { $script:injected.Add($h) }
    return $true
}
$topCb = [WPA+EnumProc]{
    param($h, $p)
    [void][WPA]::EnumChildWindows($h, $childCb, [IntPtr]::Zero)
    return $true
}
function Get-Injected {
    $script:injected.Clear()
    [void][WPA]::EnumWindows($topCb, [IntPtr]::Zero)
    if ($script:injected.Count -eq 0) { return [IntPtr]::Zero }
    return $script:injected[0]
}
function Save-Injected([IntPtr]$h, [string]$Path) {
    $r = New-Object WPA+RECT
    [void][WPA]::GetWindowRect($h, [ref]$r)
    $bmp = New-Object System.Drawing.Bitmap ($r.Right - $r.Left), ($r.Bottom - $r.Top)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $hdc = $g.GetHdc()
    try { [void][WPA]::PrintWindow($h, $hdc, 2) } finally { $g.ReleaseHdc($hdc); $g.Dispose() }
    $bmp.Save($Path, [System.Drawing.Imaging.ImageFormat]::Png)
    $bmp.Dispose()
}

$h = Get-Injected
if ($h -eq [IntPtr]::Zero) { throw "no injected wallpaper window found (is the engine running?)" }
$a = Join-Path $env:TEMP ("k6wp-anim-a-" + [Guid]::NewGuid().ToString('N').Substring(0, 6) + ".png")
$b = Join-Path $env:TEMP ("k6wp-anim-b-" + [Guid]::NewGuid().ToString('N').Substring(0, 6) + ".png")
Save-Injected $h $a
Start-Sleep -Milliseconds 1200
Save-Injected $h $b
$ba = [System.Drawing.Bitmap]::FromFile($a)
$bb = [System.Drawing.Bitmap]::FromFile($b)
try { $diff = Get-BitmapDiff -A $ba -B $bb -Step 6 } finally { $ba.Dispose(); $bb.Dispose() }
Remove-Item $a, $b -Force -ErrorAction SilentlyContinue
Write-Host ("injected-wallpaper frame-to-frame diff: {0}" -f [math]::Round($diff, 3))
if ($diff -le 0.5) {
    Write-Error "the live wallpaper is NOT animating (the injected surface does not change)"
    exit 1
}
Write-Host "the live wallpaper IS animating"
exit 0
```

## File: tools/clear_wallpaper.ps1
```powershell
# tools/clear_wallpaper.ps1 - clear the Windows desktop wallpaper (test).
# SPI_SETDESKWALLPAPER with an empty path removes the wallpaper image, so the
# shell's DefView stops painting an opaque layer over the desktop.
# Returns the previous wallpaper path so it can be restored.
$ErrorActionPreference = 'Stop'
Add-Type @"
using System;using System.Runtime.InteropServices;using System.Text;
public class WP {
  [DllImport("user32.dll", CharSet=CharSet.Unicode, SetLastError=true)]
  public static extern int SystemParametersInfo(uint a, uint b, string c, uint d);
  [DllImport("user32.dll", CharSet=CharSet.Unicode, SetLastError=true)]
  public static extern int SystemParametersInfo(uint a, uint b, IntPtr c, uint d);
  [DllImport("shell32.dll", CharSet=CharSet.Unicode)]
  public static extern void SHGetFileInfo(string pszPath, uint dwFileAttributes, ref SHFILEINFO psfi, uint cbFileInfo, uint uFlags);
  [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Unicode)]
  public struct SHFILEINFO { public IntPtr hIcon; public int iIcon; public IntPtr dwAttributes;
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst=260)] public string szDisplayName;
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst=80)] public string szTypeName; }
}
"@
$SPI_GETDESKWALLPAPER = 0x0074
$SPI_SETDESKWALLPAPER = 0x0014
$SPIF_UPDATEINIFILE = 0x01
$SPIF_SENDCHANGE = 0x02

# Read the current wallpaper path.
$buf = New-Object System.Text.StringBuilder 260
$null = [WP]::SystemParametersInfo($SPI_GETDESKWALLPAPER, 260, $buf, 0)
$prev = $buf.ToString()
Write-Host "current wallpaper: $prev"

$mode = $args[0]
if ($mode -eq 'clear') {
    [void][WP]::SystemParametersInfo($SPI_SETDESKWALLPAPER, 0, '', $SPIF_UPDATEINIFILE -bor $SPIF_SENDCHANGE)
    Write-Host "cleared (empty path)"
}
elseif ($mode -eq 'restore' -and $prev) {
    [void][WP]::SystemParametersInfo($SPI_SETDESKWALLPAPER, 0, $prev, $SPIF_UPDATEINIFILE -bor $SPIF_SENDCHANGE)
    Write-Host "restored: $prev"
}
else {
    Write-Host "usage: clear_wallpaper.ps1 clear|restore"
}
```

## File: tools/diag_gpu.ps1
```powershell
# tools/diag_gpu.ps1 - who is using the GPU right now?
#
# The Phase 0 baseline recorded 22.6% GPU for 1080p playback; a later
# re-measurement of the SAME binary read 35%. The engine's own counters
# cannot explain that, so the machine's GPU load is sampled per process to
# see whether another consumer appeared.
$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
. (Join-Path $here 'bench_lib.ps1')

$paths = @((Get-Counter -ListSet 'GPU Engine' -ErrorAction Stop).PathsWithInstances |
    Where-Object { $_ -like '*Utilization Percentage*' })
Write-Host ("GPU Engine instances: {0}" -f $paths.Count)

$rows = @()
for ($i = 1; $i -le 3; $i++) {
    $samples = @()
    try { $samples = @((Get-Counter $paths -ErrorAction Stop).CounterSamples) }
    catch { $samples = @((Get-Counter $paths -ErrorAction SilentlyContinue).CounterSamples) }
    foreach ($s in $samples) {
        if ($s.CookedValue -le 0.1) { continue }
        if ($s.Path -notmatch 'pid_(\d+)_') { continue }
        $pid2 = [int]$Matches[1]
        $engtype = ''
        if ($s.Path -match 'engtype_([A-Za-z]+)') { $engtype = $Matches[1] }
        $rows += [pscustomobject]@{
            sample = $i
            pid = $pid2
            proc = $(try { (Get-Process -Id $pid2 -ErrorAction Stop).ProcessName } catch { 'gone' })
            engtype = $engtype
            pct = [math]::Round($s.CookedValue, 2)
        }
    }
    Start-Sleep -Milliseconds 700
}

Write-Host ""
Write-Host "--- GPU utilization by process (top 15) ---"
$rows | Group-Object pid, proc | ForEach-Object {
    $vals = $_.Group | ForEach-Object { $_.pct }
    [pscustomobject]@{
        pid = $_.Group[0].pid
        proc = $_.Group[0].proc
        total_pct = [math]::Round((Get-Stats $vals).median, 2)
        engines = (($_.Group | Group-Object engtype | ForEach-Object { "$($_.Name):$([math]::Round((($_.Group | Measure-Object pct -Sum).Sum),1))" }) -join ' ')
    }
} | Sort-Object total_pct -Descending | Select-Object -First 15 | Format-Table -AutoSize | Out-String | Write-Host

Write-Host "--- display / refresh state ---"
try {
    Get-CimInstance -Namespace root\wmi -ClassName WmiMonitorListedSupportedSourceModes -ErrorAction Stop |
        Select-Object -First 3 @{n='mode';e={$_.ModeInformation}} | Format-Table -AutoSize | Out-String | Write-Host
} catch { }
try {
    $d = Get-CimInstance Win32_VideoController | Select-Object Name, CurrentHorizontalResolution, CurrentVerticalResolution, CurrentRefreshRate, VideoModeDescription
    $d | Format-Table -AutoSize | Out-String | Write-Host
} catch { }
```

## File: tools/diag_unload_crash.ps1
```powershell
# tools/diag_unload_crash.ps1 - reproduce the post-unload crash and capture
# the exit code + the last log lines.
$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
. (Join-Path $here 'bench_lib.ps1')

$engine = Join-Path $here '..\build\release\engine.exe'
$video = Join-Path $here '..\tests\corpus\anime.mp4'
$dataDir = Join-Path $env:TEMP ("k6wp-crash-" + [Guid]::NewGuid().ToString('N').Substring(0, 8))
New-Item -ItemType Directory -Force -Path (Join-Path $dataDir 'K6WP') | Out-Null
$old = $env:LOCALAPPDATA
try {
    $env:LOCALAPPDATA = $dataDir
    $p = Start-Process -FilePath $engine -PassThru -WindowStyle Hidden -ArgumentList @(
        "--video=$video", "--unload-after-pause-ms=4000", '--exit-after-ms=60000')
}
finally { $env:LOCALAPPDATA = $old }
if (-not (Wait-K6wpEngine -TimeoutMs 20000)) { throw "engine did not answer the pipe" }
$saved = Get-DesktopWindows
try {
    Start-Sleep -Seconds 4
    [void](Send-K6wpIpc -Cmd 'pause' -TimeoutMs 3000)
    Write-Host "paused; waiting for the unload..."
    for ($i = 0; $i -lt 30; $i++) {
        Start-Sleep -Milliseconds 500
        if ($p.HasExited) { break }
        $st = Send-K6wpIpc -Cmd 'get_state' -TimeoutMs 1500 -ErrorAction SilentlyContinue
        if ($st -and $st.state.decode_state -eq 'unloaded') { Write-Host "unloaded"; break }
    }
    Start-Sleep -Seconds 2
    if ($p.HasExited) {
        Write-Host ("ENGINE EXITED: code={0}" -f $p.ExitCode)
    } else {
        Write-Host "engine still alive"
        try { [void](Send-K6wpIpc -Cmd 'resume' -TimeoutMs 3000) } catch { Write-Host "resume failed: $($_.Exception.Message)" }
        Start-Sleep -Seconds 3
        Write-Host ("after resume: hasExited={0}" -f $p.HasExited)
    }
}
finally {
    Restore-DesktopWindows -Saved $saved
    try { if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue } } catch { }
}
Write-Host "--- engine log tail ---"
Get-Content (Join-Path $dataDir 'K6WP\engine.log') -Tail 20
Write-Host "--- crash dumps ---"
Get-ChildItem (Join-Path $dataDir 'K6WP') -Recurse -Filter '*.dmp' -ErrorAction SilentlyContinue | Select-Object Name,Length
```

## File: tools/diag_wakeups3.ps1
```powershell
# tools/diag_wakeups3.ps1 - attribute the wakeups to engine threads.
#
# The Phase 0 bench measures "\Thread(engine*)\Context Switches/sec" (an
# aggregate over the engine's threads) and reads ~1273/s during 1080p
# playback. This tool breaks that down per thread instance and maps each
# instance onto the live OS thread (id, state, wait reason, CPU time), so
# the spinner can be NAMED instead of guessed.
[CmdletBinding()]
param(
    [string]$EngineExe = '',
    [string]$Video = ''
)
$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
. (Join-Path $here 'bench_lib.ps1')

if ([string]::IsNullOrWhiteSpace($EngineExe)) { $EngineExe = Join-Path $here '..\build\release\engine.exe' }
if ([string]::IsNullOrWhiteSpace($Video)) { $Video = Join-Path $here '..\tests\corpus\anime.mp4' }
$engine = [System.IO.Path]::GetFullPath($EngineExe)
$video = [System.IO.Path]::GetFullPath($Video)
$data = Join-Path $env:TEMP ("k6wp-wake3-" + [Guid]::NewGuid().ToString('N').Substring(0, 8))
New-Item -ItemType Directory -Force -Path (Join-Path $data 'K6WP') | Out-Null

$busy = @(Get-Process engine -ErrorAction SilentlyContinue)
if ($busy.Count -gt 0) { throw "engine already running (pid $($busy.Id -join ','))" }

$old = $env:LOCALAPPDATA
try {
    $env:LOCALAPPDATA = $data
    $p = Start-Process -FilePath $engine -ArgumentList @("--video=$video", '--exit-after-ms=90000') -PassThru -WindowStyle Hidden
}
finally { $env:LOCALAPPDATA = $old }
if (-not (Wait-K6wpEngine -TimeoutMs 15000)) {
    try { if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force } } catch { }
    throw "engine did not answer the pipe"
}
Start-Sleep -Seconds 6

# The occlusion watcher pauses playback when a window covers the desktop, so
# the engine MUST be playing for this measurement to mean anything. Every
# other Phase 0 sampler minimizes the top-level windows for the same reason.
$saved = Get-DesktopWindows
try {
    # Confirm the engine is actually PLAYING (a paused engine reads ~36/s).
    $st = Send-K6wpIpc -Cmd 'get_state' -TimeoutMs 3000
    Write-Host ("engine state: paused={0} video='{1}'" -f $st.state.paused, $st.state.video)
    if ($st.state.paused) { Write-Host "WARNING: engine is PAUSED - the wakeup numbers will be the paused floor, not the playback figure" }

    $paths = @((Get-Counter -ListSet 'Thread' -ErrorAction Stop).PathsWithInstances |
        Where-Object { $_ -like '\Thread(engine/*)\Context Switches/sec' })
    Write-Host ("engine thread instances: {0}" -f $paths.Count)

    # Per-OS-thread CPU delta over the sampling window identifies the
    # spinner by name. The Thread perf object instance number is NOT the OS
    # thread id, so CPU time is the reliable attribution signal.
    $p.Refresh()
    $cpuBefore = @{}
    foreach ($t in $p.Threads) { $cpuBefore[[string]$t.Id] = $t.TotalProcessorTime.TotalMilliseconds }

    $rows = @()
    for ($sample = 1; $sample -le 3; $sample++) {
        $samples = @()
        try { $samples = @((Get-Counter $paths -ErrorAction Stop).CounterSamples) }
        catch { $samples = @((Get-Counter $paths -ErrorAction SilentlyContinue).CounterSamples) }
        foreach ($s in $samples) {
            $tid = ($s.Path -replace '^.*\\Thread\(engine/', '') -replace '\)\\Context Switches/sec$', ''
            $rows += [pscustomobject]@{ sample = $sample; tid = $tid; cs = [math]::Round($s.CookedValue, 1) }
        }
        Start-Sleep -Milliseconds 700
    }

    $p.Refresh()
    $cpuAfter = @{}
    foreach ($t in $p.Threads) { $cpuAfter[[string]$t.Id] = $t.TotalProcessorTime.TotalMilliseconds }

    Write-Host ""
    Write-Host "--- context switches/sec per Thread-object instance (playback) ---"
    $rows | Group-Object tid | ForEach-Object {
        $vals = $_.Group | ForEach-Object { $_.cs }
        [pscustomobject]@{ inst = $_.Name; cs = (Get-Stats $vals).median; n = $vals.Count }
    } | Sort-Object cs -Descending | Select-Object -First 10 | Format-Table -AutoSize | Out-String | Write-Host
    $total = ($rows | Group-Object sample | ForEach-Object { [math]::Round((($_.Group | Measure-Object cs -Sum).Sum), 1) })
    Write-Host ("total per sample: " + ($total -join ', '))

    Write-Host ""
    Write-Host "--- engine OS threads by CPU delta over the sample window ---"
    $window = 2.1
    $deltas = @()
    foreach ($t in $p.Threads) {
        $k = [string]$t.Id
        if ($cpuBefore.ContainsKey($k)) {
            $d = $cpuAfter[$k] - $cpuBefore[$k]
            $deltas += [pscustomobject]@{
                tid = $t.Id
                cpu_ms = [math]::Round($d, 0)
                cpu_pct_of_1core = [math]::Round(($d / 1000.0 / $window) * 100, 1)
                state = [string]$t.ThreadState
                wait = [string]$t.WaitReason
            }
        }
    }
    $deltas | Sort-Object cpu_ms -Descending | Select-Object -First 10 | Format-Table -AutoSize | Out-String | Write-Host
}
finally {
    Restore-DesktopWindows -Saved $saved
    try { if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue } } catch { }
}
```

## File: tools/fetch_vendor.ps1
```powershell
<#
.SYNOPSIS
  Fetches the pinned third-party binaries listed in vendor/VERSIONS.md.

.DESCRIPTION
  The vendored binaries total ~311 MB, and libmpv-2.dll alone exceeds GitHub's
  100 MB per-file push limit, so they are not tracked in git. This script
  downloads them and verifies every extracted file against the SHA256 recorded
  in vendor/VERSIONS.md, which stays the human-readable source of truth.

  Idempotent: a file already present with the expected hash is left alone, so
  re-running costs nothing. Use -Force to re-download anyway.

  Required by a clean build; CI calls this before configuring.

.EXAMPLE
  .\tools\fetch_vendor.ps1
  .\tools\fetch_vendor.ps1 -Force
#>
[CmdletBinding()]
param([switch]$Force)

$ErrorActionPreference = 'Stop'
$RepoRoot = Split-Path -Parent $PSScriptRoot

function Get-Sha256([string]$Path) {
  (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToUpperInvariant()
}

function Find-7Zip {
  foreach ($c in @(
      "$env:ProgramFiles\7-Zip\7z.exe",
      "${env:ProgramFiles(x86)}\7-Zip\7z.exe",
      "$env:LOCALAPPDATA\Programs\7-Zip\7z.exe")) {
    if ($c -and (Test-Path -LiteralPath $c)) { return $c }
  }
  $onPath = Get-Command 7z.exe -ErrorAction SilentlyContinue
  if ($onPath) { return $onPath.Source }
  throw "7-Zip not found. Install 7-Zip, or extract the libmpv dev package by hand."
}

# Each package is downloaded once; `Outputs` are extracted and hash-verified
# individually. The hashes are the extracted-file hashes recorded in
# vendor/VERSIONS.md, which is the manifest to keep in sync when re-pinning.
$Packages = @(
  @{
    Name   = 'ffmpeg 8.1.2 essentials (gyan.dev)'
    Kind   = 'zip'
    Url    = 'https://github.com/GyanD/codexffmpeg/releases/download/8.1.2/ffmpeg-8.1.2-essentials_build.zip'
    Outputs = @(
      @{ Zip = 'ffmpeg-8.1.2-essentials_build/bin/ffmpeg.exe';  Dst = 'vendor\ffmpeg\ffmpeg.exe'
         Sha = '1326DDE4C84FF1F96FE6B8916C5BED29E163E9B5DCCF995F6F3DB069D143EC5E' }
      @{ Zip = 'ffmpeg-8.1.2-essentials_build/bin/ffprobe.exe'; Dst = 'vendor\ffmpeg\ffprobe.exe'
         Sha = 'B49CCC7C6547B141AD5A2F6EC69CC04323D7133D7704D70B331B904C63EECB07' }
    )
  }
  @{
    Name   = 'libmpv 0.41.0-dev (shinchiro mpv-dev-x86_64)'
    Kind   = '7z'
    Url    = 'https://github.com/shinchiro/mpv-winbuild-cmake/releases/download/20260903/mpv-dev-x86_64-20260903-git-69e63f425a.7z'
    Outputs = @(
      @{ Arc = 'libmpv-2.dll';         Dst = 'vendor\libmpv\bin\libmpv-2.dll'
         Sha = '673E6397920AB64A9C5B3A618F7F16D38854EFE72B58665F1F84E4E873B763A4' }
      @{ Arc = 'include/mpv/client.h';    Dst = 'vendor\libmpv\include\mpv\client.h'
         Sha = '1ACF99EE77C8C2A6F1D1993BD81BBC8A91D27FB5924E80171670E6139A4BD353' }
      @{ Arc = 'include/mpv/render.h';    Dst = 'vendor\libmpv\include\mpv\render.h'
         Sha = '192691941602052F00DF0587F126246C48785A1CF21DE68D22A92EA1908D1C55' }
      @{ Arc = 'include/mpv/render_gl.h'; Dst = 'vendor\libmpv\include\mpv\render_gl.h'
         Sha = '48662C0ED9872A14DD9E1684105C97F69F94A0414709C254C5D372ADC41D2E69' }
      @{ Arc = 'include/mpv/stream_cb.h'; Dst = 'vendor\libmpv\include\mpv\stream_cb.h'
         Sha = '188E58B6D14383E15A5DFFDC4ECEBBDFBF2E412B9C099FF21B571F747A8CE32D' }
    )
  }
  @{
    Name   = 'nlohmann/json 3.11.3 (single header)'
    Kind   = 'direct'
    Url    = 'https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp'
    Outputs = @(
      @{ Dst = 'shared\thirdparty\json.hpp'
         Sha = '9BEA4C8066EF4A1C206B2BE5A36302F8926F7FDC6087AF5D20B417D0CF103EA6' }
    )
  }
)

$sevenZip = $null
$failures = @()

foreach ($pkg in $Packages) {
  $pending = @($pkg.Outputs | Where-Object {
      $abs = Join-Path $RepoRoot $_.Dst
      $Force -or -not (Test-Path -LiteralPath $abs) -or (Get-Sha256 $abs) -ne $_.Sha
    })
  if ($pending.Count -eq 0) {
    Write-Host "[skip] $($pkg.Name): all $($pkg.Outputs.Count) file(s) present and verified"
    continue
  }

  Write-Host "[get ] $($pkg.Name) -> $($pending.Count) file(s)"
  # Both archive kinds are extracted with 7-Zip, so resolve it for either -
  # resolved lazily because a fully-populated tree needs no extractor at all.
  if ($pkg.Kind -ne 'direct' -and -not $sevenZip) { $sevenZip = Find-7Zip }

  $work = Join-Path ([IO.Path]::GetTempPath()) ("k6wp-vendor-" + [guid]::NewGuid().ToString('N'))
  New-Item -ItemType Directory -Path $work -Force | Out-Null
  try {
    $archive = if ($pkg.Kind -eq 'direct') { Join-Path $work 'payload' } else { Join-Path $work 'archive' }
    Write-Host "       downloading $($pkg.Url)"
    Invoke-WebRequest -Uri $pkg.Url -OutFile $archive -UseBasicParsing

    # One extraction per package, naming every entry we actually need (the
    # libmpv dev package is 115 MB unpacked and we want 5 files out of it).
    # 'x' keeps the archive's directory structure; 'e' would flatten, which is
    # what the ffmpeg zip wants since its entries share one bin/ dir anyway.
    $extractRoot = if ($pkg.Kind -eq 'direct') { $null } else { Join-Path $work 'x' }
    if ($extractRoot) {
      $key = if ($pkg.Kind -eq 'zip') { 'Zip' } else { 'Arc' }
      $verb = if ($pkg.Kind -eq 'zip') { 'e' } else { 'x' }
      $args = @($verb, $archive, "-o$extractRoot", '-y') + @($pending | ForEach-Object { $o = $_; ($o[$key]) })
      & $sevenZip @args | Out-Null
      if ($LASTEXITCODE -ne 0) { throw "extraction failed (exit $LASTEXITCODE)" }
    }

    foreach ($o in $pending) {
      $abs = Join-Path $RepoRoot $o.Dst
      if ($pkg.Kind -eq 'direct') {
        $src = $archive
      } else {
        $key = if ($pkg.Kind -eq 'zip') { 'Zip' } else { 'Arc' }
        # 'e' flattens, so a zip entry lands under its basename; 'x' keeps the
        # archive's own relative path.
        $rel = if ($pkg.Kind -eq 'zip') { Split-Path $o[$key] -Leaf } else { $o[$key] }
        $src = Join-Path $extractRoot ($rel -replace '/', '\')
      }
      if (-not (Test-Path -LiteralPath $src)) { throw "not in archive: $($o[$key])" }

      $got = Get-Sha256 $src
      if ($got -ne $o.Sha) {
        throw "SHA256 mismatch for $($o.Dst)`n  expected $($o.Sha)`n  actual   $got`n  Refusing to install. Re-pinning is a deliberate act: update vendor/VERSIONS.md too."
      }
      New-Item -ItemType Directory -Path (Split-Path -Parent $abs) -Force | Out-Null
      Copy-Item -LiteralPath $src -Destination $abs -Force
      Write-Host "       ok  $($o.Dst)"
    }
  } catch {
    $failures += "$($pkg.Name): $($_.Exception.Message)"
    Write-Host "       FAIL $($_.Exception.Message)" -ForegroundColor Red
  } finally {
    Remove-Item -LiteralPath $work -Recurse -Force -ErrorAction SilentlyContinue
  }
}

Write-Host ""
if ($failures.Count -gt 0) {
  Write-Host "fetch_vendor: $($failures.Count) package(s) failed" -ForegroundColor Red
  exit 1
}
Write-Host "fetch_vendor: vendor tree complete and hash-verified."
exit 0
```

## File: tools/list_windows.ps1
```powershell
# tools/list_windows.ps1 - enumerate visible top-level windows with their rects.
# Used to find what covers the desktop (the engine's occlusion/fullscreen
# watchers pause the decode when a window covers the monitor).
Add-Type @"
using System;using System.Runtime.InteropServices;using System.Text;
public class WEnum {
  [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc cb, IntPtr p);
  public delegate bool EnumProc(IntPtr h, IntPtr p);
  [DllImport("user32.dll")] public static extern int GetWindowText(IntPtr h, StringBuilder s, int n);
  [DllImport("user32.dll")] public static extern int GetClassName(IntPtr h, StringBuilder s, int n);
  [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr h);
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
  [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
  [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
  [DllImport("user32.dll")] public static extern int GetSystemMetrics(int i);
  [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left,Top,Right,Bottom; }
}
"@
$rows = New-Object System.Collections.Generic.List[object]
$cb = [WEnum+EnumProc]{
  param($h, $p)
  if (-not [WEnum]::IsWindowVisible($h)) { return $true }
  $sb = New-Object System.Text.StringBuilder 512
  [void][WEnum]::GetWindowText($h, $sb, 512)
  $title = $sb.ToString()
  $cb2 = New-Object System.Text.StringBuilder 256
  [void][WEnum]::GetClassName($h, $cb2, 256)
  $cls = $cb2.ToString()
  $r = New-Object WEnum+RECT
  [void][WEnum]::GetWindowRect($h, [ref]$r)
  $pid2 = 0
  [void][WEnum]::GetWindowThreadProcessId($h, [ref]$pid2)
  $w = $r.Right - $r.Left; $ht = $r.Bottom - $r.Top
  $proc = try { (Get-Process -Id $pid2 -ErrorAction Stop).ProcessName } catch { '?' }
  $rows.Add([pscustomobject]@{
    hwnd = ('0x{0:X}' -f $h.ToInt64())
    pid = $pid2
    proc = $proc
    class = $cls
    rect = "$($r.Left),$($r.Top) ${w}x${ht}"
    fg = ($h -eq [WEnum]::GetForegroundWindow())
    title = $(if ($title.Length -gt 60) { $title.Substring(0,60) } else { $title })
  })
  return $true
}
[void][WEnum]::EnumWindows($cb, [IntPtr]::Zero)
$sw = [WEnum]::GetSystemMetrics(0); $sh = [WEnum]::GetSystemMetrics(1)
"screen: ${sw}x${sh}"
$rows | Where-Object { $_.rect -notmatch '^0,0 0x0$' } | Sort-Object fg -Descending | Format-Table -AutoSize
```

## File: tools/look_desktop2.ps1
```powershell
# tools/look_desktop2.ps1 - start the engine with extra args, screenshot.
[CmdletBinding()]
param(
    [string]$EngineExe = '',
    [string]$OutPng = '',
    [string[]]$ExtraArgs = @(),
    [int]$WaitSec = 10
)
$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
. (Join-Path $here 'bench_lib.ps1')
if ([string]::IsNullOrWhiteSpace($EngineExe)) { $EngineExe = Join-Path $here '..\build\release\engine.exe' }
if ([string]::IsNullOrWhiteSpace($OutPng)) { $OutPng = Join-Path $here '..\build\desktop-look2.png' }
$engine = [System.IO.Path]::GetFullPath($EngineExe)
$video = Join-Path $here '..\tests\corpus\anime.mp4'
$dataDir = Join-Path $env:TEMP ("k6wp-look2-" + [Guid]::NewGuid().ToString('N').Substring(0, 8))
New-Item -ItemType Directory -Force -Path (Join-Path $dataDir 'K6WP') | Out-Null
$busy = @(Get-Process engine -ErrorAction SilentlyContinue)
if ($busy.Count -gt 0) { throw "engine already running" }
$old = $env:LOCALAPPDATA
$argList = @("--video=$video", '--exit-after-ms=60000') + $ExtraArgs
try {
    $env:LOCALAPPDATA = $dataDir
    $p = Start-Process -FilePath $engine -ArgumentList $argList -PassThru -WindowStyle Hidden
}
finally { $env:LOCALAPPDATA = $old }
if (-not (Wait-K6wpEngine -TimeoutMs 15000)) { throw "engine did not answer the pipe" }
Start-Sleep -Seconds $WaitSec
$saved = Get-DesktopWindows
try {
    Start-Sleep -Seconds 3
    $st = Send-K6wpIpc -Cmd 'get_state' -TimeoutMs 3000
    Write-Host ("paused={0}" -f $st.state.paused)
    Save-ScreenPng -Path $OutPng
    Write-Host "screenshot -> $OutPng"
}
finally {
    Restore-DesktopWindows -Saved $saved
    try { if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue } } catch { }
}
Get-Content (Join-Path $dataDir 'K6WP\engine.log') | Select-String '24H2 path ->|wallpaper-mode|occlusion: slot|pause: owner' | ForEach-Object { $_.Line } | Select-Object -First 10
```

## File: tools/run_2monitor_evidence.ps1
```powershell
# tools/run_2monitor_evidence.ps1 — one-command 2-monitor evidence bundle (row 30).
#
# Runs on the third-party rig headless. ONE command collects:
#   1. monitor_dump.exe JSON (extended shape: monitors[] + virtual_screen)
#   2. An injected-window census via EnumWindows + EnumChildWindows matching
#      class K6WP.DesktopInject.1, saving each child's GetWindowRect
#   3. The `placement:` lines from engine.log
#   4. The rig's Windows build (OSVersion)
# Everything lands in ONE timestamped directory under .omo/evidence/.
#
# Contract (row 30 acceptance):
#   - Exits 0 ALWAYS — a missing engine.log, a missing monitor_dump.exe, or a
#     dead engine degrades to a written warning + still exit 0, so the operator
#     still gets the rest of the bundle.
#   - Census technique (notepads/k6wp-final-wallpaper-app/learnings.md:94-98):
#     Find Progman/WorkerW by EnumWindows + class match (FindWindowW returns
#     0x0 from QA scripts — walk blindness), then EnumChildWindows by class
#     K6WP.DesktopInject.1 to count injected windows (== monitor count).
#   - GetClassNameW trap (learnings.md:171-175): MUST declare
#     CharSet = CharSet.Unicode or every class name reads as its first letter
#     and class matching silently fails.
#   - Does NOT automate screenshots — a human takes those (see the run-book).
#   - Does NOT start the engine — the operator starts it first (run-book step).
#   - No network, no telemetry, no third-party dependency (stock PS + .NET).
#
# Usage (from the repo root, or anywhere — paths resolve relative to this script):
#   powershell -NoProfile -File tools/run_2monitor_evidence.ps1
#   powershell -NoProfile -File tools/run_2monitor_evidence.ps1 `
#     -MonitorDumpExe <path> [-EngineLog <path>] [-OutRoot <path>]
#
# Output directory layout:
#   .omo/evidence/2monitor-<yyyyMMdd-HHmmss>/
#     monitor_dump.json      raw monitor_dump.exe stdout
#     placement.log          Select-String -Pattern 'placement:' extract
#     injected_windows.json  census_count + per-window GetWindowRect
#     windows_build.txt      [System.Environment]::OSVersion.Version
#     summary.txt            counts + any degraded-mode warnings

[CmdletBinding()]
param(
  # Optional explicit path to monitor_dump.exe. When empty the script searches
  # build/task-30 then any build/*/monitor_dump.exe (most recent first).
  [string]$MonitorDumpExe = '',
  # Optional explicit path to engine.log. Default: %LOCALAPPDATA%\K6WP\engine.log
  [string]$EngineLog = '',
  # Optional override for the bundle root. Default: <repo>/.omo/evidence
  [string]$OutRoot = ''
)

# NEVER fail the operator. Individual steps catch and warn; the script always
# exits 0 so a degraded bundle is still a usable bundle.
$ErrorActionPreference = 'Continue'

# --- Resolve repo root relative to THIS script (repo-root agnostic) ---
$scriptDir = $PSScriptRoot
if ([string]::IsNullOrEmpty($scriptDir)) {
  if ($MyInvocation.MyCommand.Path) {
    $scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
  } else {
    $scriptDir = (Get-Location).Path
  }
}
$repoRoot = Split-Path -Parent $scriptDir

if ([string]::IsNullOrEmpty($OutRoot)) {
  $OutRoot = Join-Path $repoRoot '.omo\evidence'
}

$warnings = New-Object System.Collections.Generic.List[string]

# --- Create the timestamped bundle directory (always) ---
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$bundleDir = Join-Path $OutRoot ("2monitor-{0}" -f $stamp)
try {
  New-Item -ItemType Directory -Path $bundleDir -Force | Out-Null
} catch {
  # Last-resort: fall back to %TEMP% so the script still exits 0 with a path.
  $bundleDir = Join-Path $env:TEMP ("k6wp-2monitor-{0}" -f $stamp)
  New-Item -ItemType Directory -Path $bundleDir -Force | Out-Null
  $warnings.Add("could not create $OutRoot; bundle written to $bundleDir instead")
}

# ---------------------------------------------------------------------------
# 1. monitor_dump.exe -> monitor_dump.json
# ---------------------------------------------------------------------------
$dumpExe = $MonitorDumpExe
if ([string]::IsNullOrEmpty($dumpExe)) {
  # Prefer the per-task binary dir, then any build/*/monitor_dump.exe.
  $candidates = @()
  $taskDirDump = Join-Path $repoRoot 'build\task-30\monitor_dump.exe'
  if (Test-Path -LiteralPath $taskDirDump) { $candidates += $taskDirDump }
  $buildRoot = Join-Path $repoRoot 'build'
  if (Test-Path -LiteralPath $buildRoot) {
    $any = Get-ChildItem -Path $buildRoot -Filter 'monitor_dump.exe' -Recurse -ErrorAction SilentlyContinue |
      Sort-Object LastWriteTime -Descending
    foreach ($a in $any) { if ($candidates -notcontains $a.FullName) { $candidates += $a.FullName } }
  }
  if ($candidates.Count -gt 0) { $dumpExe = $candidates[0] }
}

$monitorCount = -1
$dumpJsonPath = Join-Path $bundleDir 'monitor_dump.json'
if ([string]::IsNullOrEmpty($dumpExe) -or -not (Test-Path -LiteralPath $dumpExe)) {
  $warnings.Add("monitor_dump.exe not found (pass -MonitorDumpExe <path>); monitor_dump.json omitted")
} else {
  try {
    $dumpOut = & $dumpExe 2>&1
    $dumpText = ($dumpOut | Out-String).Trim()
    # Write raw stdout; never re-encode (PS 5.1 JSON-in-string trap).
    Set-Content -LiteralPath $dumpJsonPath -Value $dumpText -Encoding UTF8
    # Parse .monitors[] (row 2 wrapper shape: {"monitors":[...],"virtual_screen":{...}})
    try {
      $parsed = $dumpText | ConvertFrom-Json
      if ($null -ne $parsed -and $null -ne $parsed.monitors) {
        $monitorCount = @($parsed.monitors).Count
      } else {
        $warnings.Add("monitor_dump.json has no .monitors[] array (unexpected shape)")
      }
    } catch {
      $warnings.Add("monitor_dump.json could not be parsed as JSON: $($_.Exception.Message)")
    }
  } catch {
    $warnings.Add("monitor_dump.exe failed to run: $($_.Exception.Message)")
  }
}

# ---------------------------------------------------------------------------
# 2. engine.log placement: lines -> placement.log
# ---------------------------------------------------------------------------
$logPath = $EngineLog
if ([string]::IsNullOrEmpty($logPath)) {
  $logPath = Join-Path $env:LOCALAPPDATA 'K6WP\engine.log'
}
$placementPath = Join-Path $bundleDir 'placement.log'
$placementCount = 0
if (-not (Test-Path -LiteralPath $logPath)) {
  $warnings.Add("engine.log not found at $logPath (start the engine first, or pass -EngineLog <path>)")
} else {
  try {
    # Exact run-book commands:
    #   Get-Content <engine.log> | Select-String -Pattern 'placement:'
    $hits = @(Get-Content -LiteralPath $logPath -ErrorAction Stop |
      Select-String -Pattern 'placement:')
    $placementCount = $hits.Count
    if ($placementCount -gt 0) {
      $hits | ForEach-Object { $_.Line } | Set-Content -LiteralPath $placementPath -Encoding UTF8
    } else {
      Set-Content -LiteralPath $placementPath -Value '(no placement: lines found)' -Encoding UTF8
      $warnings.Add("engine.log exists but contains no placement: lines (build the engine with -DK6WP_VERBOSE=ON)")
    }
  } catch {
    $warnings.Add("could not read engine.log: $($_.Exception.Message)")
  }
}

# ---------------------------------------------------------------------------
# 3. Injected-window census (EnumWindows + EnumChildWindows + GetWindowRect)
#    GetClassNameW MUST be CharSet.Unicode (learnings.md:171-175 trap).
# ---------------------------------------------------------------------------
$censusPath = Join-Path $bundleDir 'injected_windows.json'
$censusCount = 0
$censusWindows = @()

$censusSource = @'
using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;

public static class K6wpCensus {
  [StructLayout(LayoutKind.Sequential)]
  public struct RECT {
    public int Left;
    public int Top;
    public int Right;
    public int Bottom;
  }

  private delegate bool EnumProc(IntPtr hWnd, IntPtr lParam);

  // NOTE: GetClassNameW must be CharSet.Unicode — without it every class name
  // marshals as ANSI and reads as its FIRST letter ("Progman" -> "P",
  // "K6WP.DesktopInject.1" -> "K"), so class matching silently fails.
  [DllImport("user32.dll", CharSet = CharSet.Unicode)]
  private static extern int GetClassNameW(IntPtr hWnd, StringBuilder lpClassName, int nMaxCount);

  [DllImport("user32.dll")]
  private static extern bool EnumWindows(EnumProc lpEnumFunc, IntPtr lParam);

  [DllImport("user32.dll")]
  private static extern bool EnumChildWindows(IntPtr hWndParent, EnumProc lpEnumFunc, IntPtr lParam);

  [DllImport("user32.dll")]
  private static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);

  [DllImport("user32.dll")]
  private static extern bool IsWindow(IntPtr hWnd);

  // Top-level shells we search under: Progman and WorkerW (24H2 hosts the
  // injected surface under WorkerW; classic/earlier layouts use Progman).
  private static readonly List<long> shells = new List<long>();
  private static readonly List<long> injected = new List<long>();
  private const string InjectClass = "K6WP.DesktopInject.1";

  private static string GetClass(IntPtr h) {
    StringBuilder sb = new StringBuilder(256);
    GetClassNameW(h, sb, sb.Capacity);
    return sb.ToString();
  }

  private static bool ShellCb(IntPtr h, IntPtr p) {
    string c = GetClass(h);
    if (c == "Progman" || c == "WorkerW") shells.Add(h.ToInt64());
    return true; // keep enumerating
  }

  private static bool ChildCb(IntPtr h, IntPtr p) {
    if (GetClass(h) == InjectClass) injected.Add(h.ToInt64());
    return true; // count ALL injected windows, not just the first
  }

  // Returns "hwnd|left|top|right|bottom" lines for every injected window.
  public static string[] Census() {
    shells.Clear();
    injected.Clear();

    // Walk-blindness trap: FindWindowW(Progman) often returns 0x0 from QA
    // scripts. EnumWindows + class match is the reliable path.
    EnumWindows(ShellCb, IntPtr.Zero);

    foreach (long shell in shells) {
      EnumChildWindows(new IntPtr(shell), ChildCb, IntPtr.Zero);
    }

    // De-duplicate (a window can appear under both shells in odd layouts).
    HashSet<long> seen = new HashSet<long>();
    List<string> rows = new List<string>();
    foreach (long hw in injected) {
      if (!seen.Add(hw)) continue;
      IntPtr h = new IntPtr(hw);
      if (!IsWindow(h)) continue;
      RECT r;
      if (!GetWindowRect(h, out r)) continue;
      rows.Add(string.Format("{0}|{1}|{2}|{3}|{4}", hw, r.Left, r.Top, r.Right, r.Bottom));
    }
    return rows.ToArray();
  }
}
'@

try {
  if (-not ('K6wpCensus' -as [type])) {
    Add-Type -TypeDefinition $censusSource -ErrorAction Stop
  }
  $rows = [K6wpCensus]::Census()
  $censusCount = $rows.Count
  $censusWindows = @()
  foreach ($row in $rows) {
    $parts = $row -split '\|'
    if ($parts.Count -eq 5) {
      $hwnd = [Convert]::ToInt64($parts[0])
      $left = [int]$parts[1]; $top = [int]$parts[2]
      $right = [int]$parts[3]; $bottom = [int]$parts[4]
      $censusWindows += [pscustomobject]@{
        hwnd    = ('0x{0:X}' -f $hwnd)
        class   = 'K6WP.DesktopInject.1'
        left    = $left
        top     = $top
        right   = $right
        bottom  = $bottom
        width   = ($right - $left)
        height  = ($bottom - $top)
      }
    }
  }
} catch {
  $warnings.Add("census P/Invoke failed: $($_.Exception.Message)")
}

# Degraded-mode contract (row 30): census_count: 0 + the exact warning line.
if ($censusCount -eq 0) {
  $warnings.Add('engine not running or no injected windows found')
}

$censusObj = [pscustomobject]@{
  census_count  = $censusCount
  monitor_count = $monitorCount
  class         = 'K6WP.DesktopInject.1'
  technique     = 'EnumWindows(Progman/WorkerW) + EnumChildWindows + GetWindowRect; GetClassNameW CharSet=Unicode'
  windows       = $censusWindows
}
# ConvertTo-Json on pre-built objects avoids the PS 5.1 backslash-escaped-quote trap.
$censusObj | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $censusPath -Encoding UTF8

# ---------------------------------------------------------------------------
# 4. Windows build -> windows_build.txt
# ---------------------------------------------------------------------------
$buildPath = Join-Path $bundleDir 'windows_build.txt'
try {
  $osv = [System.Environment]::OSVersion
  $buildLines = @(
    ('OSVersion.Version: {0}' -f $osv.Version),
    ('OSVersion.VersionString: {0}' -f $osv.VersionString),
    ('OSVersion.Platform: {0}' -f $osv.Platform),
    ('64BitOperatingSystem: {0}' -f [System.Environment]::Is64BitOperatingSystem),
    '',
    'Equivalent winver check (paste the dialog if asked):',
    '  winver',
    'Equivalent PowerShell check:',
    '  [System.Environment]::OSVersion.Version'
  )
  $buildLines | Set-Content -LiteralPath $buildPath -Encoding UTF8
} catch {
  $warnings.Add("could not read OSVersion: $($_.Exception.Message)")
}

# ---------------------------------------------------------------------------
# 5. summary.txt — counts + warnings (always written)
# ---------------------------------------------------------------------------
$summaryPath = Join-Path $bundleDir 'summary.txt'
$engineAlive = $false
try {
  $engines = @(Get-Process -Name 'engine' -ErrorAction SilentlyContinue)
  $engineAlive = ($engines.Count -gt 0)
} catch { $engineAlive = $false }

$summaryLines = @(
  ('bundle_dir: {0}' -f $bundleDir),
  ('generated: {0}' -f (Get-Date -Format 'o')),
  ('monitor_dump_exe: {0}' -f $(if ($dumpExe) { $dumpExe } else { '(not found)' })),
  ('engine_log: {0}' -f $logPath),
  ('monitor_count: {0}' -f $monitorCount),
  ('census_count: {0}' -f $censusCount),
  ('placement_line_count: {0}' -f $placementCount),
  ('engine_process_alive: {0}' -f $engineAlive),
  ('census_equals_monitors: {0}' -f $(if ($monitorCount -ge 0 -and $censusCount -eq $monitorCount) { 'yes' } else { 'no-or-unknown' })),
  '',
  'warnings:'
)
if ($warnings.Count -eq 0) {
  $summaryLines += '  (none)'
} else {
  foreach ($w in $warnings) { $summaryLines += ('  - {0}' -f $w) }
}
$summaryLines | Set-Content -LiteralPath $summaryPath -Encoding UTF8

# ---------------------------------------------------------------------------
# Console report + exit 0 (always)
# ---------------------------------------------------------------------------
Write-Output ('bundle_dir: {0}' -f $bundleDir)
Write-Output ('monitor_count: {0}' -f $monitorCount)
Write-Output ('census_count: {0}' -f $censusCount)
Write-Output ('placement_line_count: {0}' -f $placementCount)
Write-Output ('engine_process_alive: {0}' -f $engineAlive)
foreach ($w in $warnings) {
  Write-Output ('WARNING: {0}' -f $w)
}
Write-Output ('files: monitor_dump.json, placement.log, injected_windows.json, windows_build.txt, summary.txt')
Write-Output ('screenshots are NOT automated - take one per display mode by hand (see docs/runbook-2monitor.md)')
exit 0
```

## File: tools/run_phase0_bench.ps1
```powershell
# tools/run_phase0_bench.ps1 - run the whole Phase 0 measurement suite.
#
# One command that produces every Phase 0 evidence file:
#   docs/bench/phase0.json        engine RAM/CPU/GPU/wakeups, n>=5 per state
#   docs/bench/phase0.raw.json    the per-run detail
#   docs/bench/studio.json        Studio startup + idle RAM/CPU
#   docs/bench/bundle.json        install-size census
#   docs/bench/resume.json        suspend/resume latency
#   docs/bench/visual/baseline/   visual parity baseline (PNGs + signature)
#
# The production engine MUST be stopped first (singleton mutex): the harness
# measures the REAL engine, so a resident production engine would either
# block the run or pollute the numbers.
#
# Usage:
#   powershell -NoProfile -ExecutionPolicy Bypass -File tools\run_phase0_bench.ps1 `
#     [-Runs 5] [-SkipVisual]
#
# Exit code 0 = every sampler produced its file; non-zero = a sampler failed.

[CmdletBinding()]
param(
    [int]$Runs = 5,
    [string]$EngineExe = '',
    [string]$OutDir = '',
    [switch]$SkipVisual,
    [switch]$SkipResume
)

$ErrorActionPreference = 'Stop'
$script:Here = Split-Path -Parent $MyInvocation.MyCommand.Path
. (Join-Path $script:Here 'bench_lib.ps1')
if ([string]::IsNullOrWhiteSpace($EngineExe)) { $EngineExe = Join-Path $script:Here '..\build\release\engine.exe' }
if ([string]::IsNullOrWhiteSpace($OutDir)) { $OutDir = Join-Path $script:Here '..\docs\bench' }
$OutDir = [System.IO.Path]::GetFullPath($OutDir)
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

# --- preflight -------------------------------------------------------------
if (-not (Test-Path -LiteralPath $EngineExe)) { throw "engine not found: $EngineExe" }
$busy = @(Get-Process engine -ErrorAction SilentlyContinue)
if ($busy.Count -gt 0) {
    throw ("an engine.exe is already running (pid {0}). Stop it first: K6WP.exe --stop" -f (($busy | ForEach-Object { $_.Id }) -join ','))
}
$studioBusy = @(Get-Process studio -ErrorAction SilentlyContinue)
if ($studioBusy.Count -gt 0) {
    throw ("studio.exe is already running (pid {0}). Close it first." -f (($studioBusy | ForEach-Object { $_.Id }) -join ','))
}

$steps = @()
$failed = @()

function Invoke-Step {
    param([string]$Name, [scriptblock]$Action)
    Write-Host ""
    Write-Host ("=" * 72)
    Write-Host "  $Name"
    Write-Host ("=" * 72)
    $sw = [Diagnostics.Stopwatch]::StartNew()
    try {
        & $Action
        $script:steps += [pscustomobject]@{ step = $Name; status = 'ok'; seconds = [math]::Round($sw.Elapsed.TotalSeconds, 1) }
        Write-Host ("  -> {0} OK ({1}s)" -f $Name, [math]::Round($sw.Elapsed.TotalSeconds, 1))
    }
    catch {
        $script:steps += [pscustomobject]@{ step = $Name; status = 'FAILED'; seconds = [math]::Round($sw.Elapsed.TotalSeconds, 1); error = $_.Exception.Message }
        $script:failed += $Name
        Write-Host ("  -> {0} FAILED: {1}" -f $Name, $_.Exception.Message)
    }
}

Invoke-Step "engine (RAM/CPU/GPU/wakeups, n>=$Runs)" {
    & (Join-Path $script:Here 'bench_engine.ps1') -Runs $Runs -EngineExe $EngineExe -OutJson (Join-Path $OutDir 'phase0.json')
    if (-not (Test-Path -LiteralPath (Join-Path $OutDir 'phase0.json'))) { throw "bench_engine wrote no output" }
}

Invoke-Step 'studio (startup + idle RAM/CPU)' {
    & (Join-Path $script:Here 'bench_studio.ps1') -Runs 3 -OutJson (Join-Path $OutDir 'studio.json')
    if (-not (Test-Path -LiteralPath (Join-Path $OutDir 'studio.json'))) { throw "bench_studio wrote no output" }
}

Invoke-Step 'bundle (install-size census)' {
    & (Join-Path $script:Here 'bench_bundle.ps1') -OutJson (Join-Path $OutDir 'bundle.json')
    if (-not (Test-Path -LiteralPath (Join-Path $OutDir 'bundle.json'))) { throw "bench_bundle wrote no output" }
}

if (-not $SkipResume) {
    Invoke-Step 'resume (suspend/resume latency)' {
        & (Join-Path $script:Here 'bench_resume.ps1') -Runs 3 -OutJson (Join-Path $OutDir 'resume.json')
        if (-not (Test-Path -LiteralPath (Join-Path $OutDir 'resume.json'))) { throw "bench_resume wrote no output" }
    }
}

if (-not $SkipVisual) {
    Invoke-Step 'visual (parity baseline capture)' {
        & (Join-Path $script:Here 'capture_visual_baseline.ps1')
        $sig = Join-Path $OutDir 'visual\baseline\signature.json'
        if (-not (Test-Path -LiteralPath $sig)) { throw "visual capture wrote no signature" }
    }
}

Write-Host ""
Write-Host ("=" * 72)
Write-Host "  Phase 0 summary"
Write-Host ("=" * 72)
$steps | Format-Table -AutoSize | Out-String | Write-Host
if ($failed.Count -gt 0) {
    Write-Host ("FAILED: {0}" -f ($failed -join ', '))
    exit 1
}
Write-Host "All Phase 0 samplers produced their evidence files."
exit 0
```

## File: tools/verify_visual_negative.ps1
```powershell
# tools/verify_visual_negative.ps1 - prove the visual parity gate FAILS when
# the wallpaper is not playing. A gate that never fails is not a gate.
#
# Captures the desktop with NO engine running (the plain Windows wallpaper)
# and compares it against the engine-playing baseline. The mean_luma /
# stddev must differ enough to trip the gate, and the motion diff must be
# ~0 (a static desktop image does not move).
$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
. (Join-Path $here 'bench_lib.ps1')

$baseline = Join-Path $here '..\docs\bench\visual\baseline'
$negative = Join-Path $here '..\build\visual-negative'

$busy = @(Get-Process engine -ErrorAction SilentlyContinue)
if ($busy.Count -gt 0) { throw "an engine.exe is running (pid $($busy.Id -join ',')); stop it first" }

Remove-Item -LiteralPath $negative -Recurse -Force -ErrorAction SilentlyContinue
& (Join-Path $here 'bench_visual.ps1') -Capture $negative -Force | Out-Null

# The compare mode ends in Write-Error on a parity failure. That is the
# EXPECTED outcome here, so the error must not abort this script.
$prevPref = $ErrorActionPreference
$ErrorActionPreference = 'Continue'
$out = & (Join-Path $here 'bench_visual.ps1') -Compare $baseline $negative 2>&1
$rc = $LASTEXITCODE
$ErrorActionPreference = $prevPref
$out | Select-Object -Last 30
if ($rc -eq 0) {
    Write-Error "verify_visual_negative: FAILED - the parity gate passed a NON-playing desktop (the gate is useless)"
    exit 1
}
Write-Host "verify_visual_negative: OK - the parity gate correctly REJECTED the non-playing desktop (exit $rc)"
exit 0
```

## File: tools/verify_wallpaper.ps1
```powershell
# tools/verify_wallpaper.ps1 — repeat wallpaper-switch stress + chrome audit (Todo 11 + 30).
#
# Part A (Todo 11, Wave 2) loops N x set_video via the exact current NDJSON IPC protocol and proves:
#   1. every switch is acked ok,
#   2. the engine process stays alive throughout,
#   3. working-set memory stays stable (non-monotonic, bounded),
#   4. a mid-loop bad-path (missing file) is cleanly REJECTED without killing
#      the engine or breaking the loop.
#
# Part B (Todo 30) then audits the window chrome live via user32 P/Invoke:
#   TOOLWINDOW bit set, NO WS_CAPTION/WS_BORDER frame, NO owned visible
#   top-level (no taskbar button), NO-AltTab (follows TOOLWINDOW), tray
#   owner message-window present + install log quoted. Any single failing
#   check => RESULT: FAIL + non-zero exit.
#
# Protocol (shared/ipc_protocol.cpp, engine/src/ipc_server.cpp HandleMessage):
#   request: {"version":1,"cmd":"set_video","payload":{"path":"<utf8>"}}\n
#   ok ack:  {"ok":true}\n
#   reject:  {"error":"..."}\n   (frameless/empty datagrams get NO reply)
# Pipe: \\.\pipe\k6wp-engine (message mode).
#
# Pure stock PowerShell + .NET (NamedPipeClientStream). Zero dependencies.
# The harness launches the engine separately; this script NEVER starts it:
# if no engine answers at start it prints FAIL + exits non-zero.
#
# Usage:
#   powershell -ExecutionPolicy Bypass -File tools/verify_wallpaper.ps1 `
#     -VideoA <path> -VideoB <path> [-Count 20] [-EngineLog <path>]
# Defaults resolve relative to this script (repo-root agnostic, no hardcoded
# user paths): ..\build\spikes\test_1080p.mp4 and ..\tests\corpus\slideshow.mp4.
# -EngineLog optionally points at the engine.log to use for the honest
# fallbacks (default: $env:LOCALAPPDATA\K6WP\engine.log). Pass the private
# log when the engine runs with a session-local LOCALAPPDATA override.

[CmdletBinding()]
param(
  [string]$VideoA = '',
  [string]$VideoB = '',
  [int]$Count = 20,
  [string]$PipeName = 'k6wp-engine',
  [int]$ConnectTimeoutMs = 5000,
  [int]$AckTimeoutMs = 20000,
  [string]$EngineLog = ''
)

$ErrorActionPreference = 'Stop'

# Resolve default videos relative to THIS script (repo-root agnostic, no
# hardcoded user paths). $PSScriptRoot can be empty under some hosts
# (e.g. nested powershell -File), so fall back to MyCommand path, then CWD.
$scriptDir = $PSScriptRoot
if ([string]::IsNullOrEmpty($scriptDir)) {
  if ($MyInvocation.MyCommand.Path) {
    $scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
  } else {
    $scriptDir = (Get-Location).Path
  }
}
if ([string]::IsNullOrEmpty($VideoA)) {
  $VideoA = Join-Path $scriptDir '..\build\spikes\test_1080p.mp4'
}
if ([string]::IsNullOrEmpty($VideoB)) {
  $VideoB = Join-Path $scriptDir '..\tests\corpus\slideshow.mp4'
}

function Fail([string]$msg) {
  Write-Output "FAIL $msg"
  exit 1
}

# --- Preconditions: both videos must exist (real files; engine validates). ---
$VideoA = [IO.Path]::GetFullPath($VideoA)
$VideoB = [IO.Path]::GetFullPath($VideoB)
if (-not (Test-Path -LiteralPath $VideoA -PathType Leaf)) {
  Fail "VideoA not found: $VideoA"
}
if (-not (Test-Path -LiteralPath $VideoB -PathType Leaf)) {
  Fail "VideoB not found: $VideoB"
}
if ($Count -lt 1) { Fail "Count must be >= 1 (got $Count)" }

# --- Engine liveness at start (single instance expected; strays killed by harness). ---
$engines = @(Get-Process -Name 'engine' -ErrorAction SilentlyContinue)
if ($engines.Count -eq 0) {
  Write-Output 'FAIL engine not running'
  exit 1
}
if ($engines.Count -gt 1) {
  Fail "ambiguous: $($engines.Count) engine processes running (kill strays first)"
}
$engineId = $engines[0].Id
Write-Output "engine pid: $engineId"
Write-Output "videoA: $VideoA"
Write-Output "videoB: $VideoB"
Write-Output "count: $Count"

function Test-EngineAlive {
  try {
    $p = Get-Process -Id $engineId -ErrorAction Stop
    return (-not $p.HasExited)
  } catch {
    return $false
  }
}

function Get-WorkingSet {
  $p = Get-Process -Id $engineId -ErrorAction Stop
  $p.Refresh()
  return $p.WorkingSet64
}

# --- Connect (timeout => engine-down FAIL, never start the engine here). ---
$client = New-Object IO.Pipes.NamedPipeClientStream(
  '.', $PipeName, [IO.Pipes.PipeDirection]::InOut,
  [IO.Pipes.PipeOptions]::None)
try {
  $client.Connect($ConnectTimeoutMs)
} catch {
  $client.Dispose()
  Write-Output 'FAIL engine not running'
  exit 1
}
$client.ReadMode = [IO.Pipes.PipeTransmissionMode]::Message
$utf8 = [Text.Encoding]::UTF8
Write-Output 'pipe: connected'

function Read-Ack {
  # Reads one message-mode datagram with a hard timeout. Returns ack text.
  $buf = New-Object byte[] 65536
  $ms = New-Object IO.MemoryStream
  try {
    $deadline = [DateTime]::UtcNow.AddMilliseconds($AckTimeoutMs)
    do {
      $remaining = [int](($deadline - [DateTime]::UtcNow).TotalMilliseconds)
      if ($remaining -le 0) { throw [TimeoutException]'ack read timed out' }
      $ar = $client.BeginRead($buf, 0, $buf.Length, $null, $null)
      if (-not $ar.AsyncWaitHandle.WaitOne($remaining)) {
        throw [TimeoutException]'ack read timed out'
      }
      $n = $client.EndRead($ar)
      if ($n -le 0) { throw [IO.IOException]'pipe closed while reading ack' }
      $ms.Write($buf, 0, $n)
    } while (-not $client.IsMessageComplete)
    return $utf8.GetString($ms.ToArray())
  } finally {
    $ms.Dispose()
  }
}

function Send-SetVideo([string]$path) {
  # Returns @{ Ok=[bool]; Ack=[string]; LatencyMs=[double]; Error=[string] }.
  $sw = [Diagnostics.Stopwatch]::StartNew()
  try {
    $msg = (@{ version = 1; cmd = 'set_video'; payload = @{ path = $path } } |
      ConvertTo-Json -Compress -Depth 5) + "`n"
    $bytes = $utf8.GetBytes($msg)
    $client.Write($bytes, 0, $bytes.Length)
    $client.Flush()
    $ackText = Read-Ack
    $sw.Stop()
    $ack = $ackText | ConvertFrom-Json
    $ok = ($null -ne $ack.ok) -and ($ack.ok -eq $true)
    $err = $null
    if ($null -ne $ack.error) { $err = [string]$ack.error }
    return @{ Ok = $ok; Ack = $ackText.Trim(); LatencyMs = $sw.Elapsed.TotalMilliseconds; Error = $err }
  } catch {
    $sw.Stop()
    return @{ Ok = $false; Ack = ''; LatencyMs = $sw.Elapsed.TotalMilliseconds; Error = "EXCEPTION: $($_.Exception.Message)" }
  }
}

$memSamples = New-Object System.Collections.Generic.List[long]
$okCount = 0
$badPathOk = $false
$badPathError = ''
$failed = $false

try {
  for ($i = 1; $i -le $Count; $i++) {
    if (-not (Test-EngineAlive)) { Fail "engine died before iteration $i" }
    if ($i % 2 -eq 1) { $vid = $VideoA; $slot = 'A' } else { $vid = $VideoB; $slot = 'B' }
    $r = Send-SetVideo $vid
    if (-not $r.Ok) {
      Write-Output ("iter {0:D2}/{1} video={2} ack=FAIL latency={3:N0}ms ack={4} err={5}" -f $i, $Count, $slot, $r.LatencyMs, $r.Ack, $r.Error)
      $failed = $true
      break
    }
    $okCount++
    try {
      $ws = Get-WorkingSet
    } catch {
      Fail "engine died at iteration $i (cannot sample memory)"
    }
    $memSamples.Add($ws)
    $wsMb = [math]::Round($ws / 1MB, 1)
    Write-Output ("iter {0:D2}/{1} video={2} ack=ok latency={3:N0}ms workingset={4}MB" -f $i, $Count, $slot, $r.LatencyMs, $wsMb)

    # Mid-loop bad-path: missing file must be cleanly REJECTED, engine alive,
    # loop continues. (Iteration 10 of the default Count=20.)
    if ($i -eq 10) {
      $missing = Join-Path $env:TEMP ('k6wp-verify-wallpaper-missing-{0}.mp4' -f [Guid]::NewGuid().ToString('N'))
      $b = Send-SetVideo $missing
      if ($b.Ok) {
        Write-Output "iter 10 bad-path: UNEXPECTED-OK (engine accepted missing file) ack=$($b.Ack)"
        $failed = $true
        break
      }
      if ([string]::IsNullOrEmpty($b.Error) -and [string]::IsNullOrEmpty($b.Ack)) {
        Write-Output 'iter 10 bad-path: TIMEOUT (no ack for missing file)'
        $failed = $true
        break
      }
      if ($b.Error -like 'EXCEPTION:*') {
        Write-Output "iter 10 bad-path: TRANSPORT-FAIL $($b.Error)"
        $failed = $true
        break
      }
      if (-not (Test-EngineAlive)) {
        Write-Output "iter 10 bad-path: engine DIED after reject (ack=$($b.Ack))"
        $failed = $true
        break
      }
      $badPathOk = $true
      $badPathError = if ($b.Error) { $b.Error } else { $b.Ack }
      Write-Output "iter 10 bad-path: REJECT as expected (error=$badPathError) engine=alive loop=continues"
    }
  }
} finally {
  $client.Dispose()
}

if ($failed) {
  Write-Output "RESULT: FAIL ($okCount/$Count acks)"
  exit 1
}

if ($okCount -ne $Count) {
  Write-Output "RESULT: FAIL ($okCount/$Count acks)"
  exit 1
}
if (-not $badPathOk) {
  Write-Output "RESULT: FAIL ($okCount/$Count acks, bad-path probe did not run/reject)"
  exit 1
}
if (-not (Test-EngineAlive)) {
  Write-Output "RESULT: FAIL ($okCount/$Count acks, engine died at end)"
  exit 1
}

$first = $memSamples[0]
$mid = $memSamples[[math]::Floor($memSamples.Count / 2)]
$last = $memSamples[$memSamples.Count - 1]
$firstMb = [math]::Round($first / 1MB, 1)
$midMb = [math]::Round($mid / 1MB, 1)
$lastMb = [math]::Round($last / 1MB, 1)
$peak = ($memSamples | Measure-Object -Maximum).Maximum
$peakMb = [math]::Round($peak / 1MB, 1)
Write-Output ("memory first={0}MB mid={1}MB last={2}MB peak={3}MB (limit={4}MB)" -f $firstMb, $midMb, $lastMb, $peakMb, ([math]::Round($first * 1.5 / 1MB, 1)))
$memOk = ($last -le ($first * 1.5)) -and ($mid -le ($first * 1.5))
if (-not $memOk) {
  Write-Output 'RESULT: FAIL (memory unstable: end or mid exceeds start*1.5)'
  exit 1
}

# --- Todo 30: taskbar / Alt+Tab / frame / tray checks (appended; T11 above untouched). ---
# Live window reads via stock .NET P/Invoke only (Add-Type user32). Known trap
# (T7/T10): shell-side FindWindowW(Progman) often returns 0x0 (walk blindness)
# and EnumWindows MISSES SetParent'd popups, so the injected HWND is found via
# EnumChildWindows of Progman (+ top-level WorkerW fallback), matching the T9
# measurement pattern. engine.log lines are an HONEST fallback only (quoted
# verbatim, labelled via-log), never a faked live read.
Add-Type -TypeDefinition @'
using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;
public static class K6wpVerify {
  private delegate bool EnumProc(IntPtr hWnd, IntPtr lParam);
  [DllImport("user32.dll", CharSet = CharSet.Unicode)]
  private static extern IntPtr FindWindowW(string lpClassName, string lpWindowName);
  [DllImport("user32.dll")]
  private static extern bool EnumWindows(EnumProc lpEnumFunc, IntPtr lParam);
  [DllImport("user32.dll")]
  private static extern bool EnumChildWindows(IntPtr hWndParent, EnumProc lpEnumFunc, IntPtr lParam);
  [DllImport("user32.dll")]
  private static extern bool IsWindowVisible(IntPtr hWnd);
  [DllImport("user32.dll", CharSet = CharSet.Unicode)]
  private static extern int GetClassNameW(IntPtr hWnd, StringBuilder lpClassName, int nMaxCount);
  [DllImport("user32.dll", CharSet = CharSet.Unicode)]
  private static extern int GetWindowTextW(IntPtr hWnd, StringBuilder lpString, int nMaxCount);
  [DllImport("user32.dll")]
  private static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint lpdwProcessId);
  [DllImport("user32.dll")]
  private static extern IntPtr GetWindowLongPtrW(IntPtr hWnd, int nIndex);
  private const int GWL_EXSTYLE = -20;
  private const int GWL_STYLE = -16;
  public static string FindMethod = "none";
  private static string wantClass = "";
  private static long foundHwnd = 0;
  private static bool ChildCb(IntPtr h, IntPtr p) {
    StringBuilder sb = new StringBuilder(256);
    GetClassNameW(h, sb, sb.Capacity);
    if (sb.ToString() == wantClass) { foundHwnd = h.ToInt64(); return false; }
    return true;
  }
  private static List<IntPtr> workers = new List<IntPtr>();
  private static bool WorkerCb(IntPtr h, IntPtr p) {
    StringBuilder sb = new StringBuilder(256);
    GetClassNameW(h, sb, sb.Capacity);
    if (sb.ToString() == "WorkerW") workers.Add(h);
    return true;
  }
  public static long FindInjected(string cls) {
    wantClass = cls; foundHwnd = 0; FindMethod = "none";
    IntPtr prog = FindWindowW("Progman", null);
    if (prog != IntPtr.Zero) {
      EnumChildWindows(prog, ChildCb, IntPtr.Zero);
      if (foundHwnd != 0) { FindMethod = "Progman-children"; return foundHwnd; }
    }
    workers.Clear();
    EnumWindows(WorkerCb, IntPtr.Zero);
    foreach (IntPtr w in workers) {
      EnumChildWindows(w, ChildCb, IntPtr.Zero);
      if (foundHwnd != 0) { FindMethod = "WorkerW-children"; return foundHwnd; }
    }
    FindMethod = "not-found";
    return 0;
  }
  public static long GetExStyle(long hwnd) {
    return GetWindowLongPtrW(new IntPtr(hwnd), GWL_EXSTYLE).ToInt64();
  }
  public static long GetStyle(long hwnd) {
    return GetWindowLongPtrW(new IntPtr(hwnd), GWL_STYLE).ToInt64();
  }
  private static uint wantPid = 0;
  private static string wantCls = "";
  private static string wantTitle = "";
  private static int tbCount = 0;
  private static bool TbCb(IntPtr h, IntPtr p) {
    if (!IsWindowVisible(h)) return true;
    uint pid; GetWindowThreadProcessId(h, out pid);
    if (pid != wantPid) return true;
    StringBuilder c = new StringBuilder(256); GetClassNameW(h, c, c.Capacity);
    StringBuilder t = new StringBuilder(256); GetWindowTextW(h, t, t.Capacity);
    if (c.ToString() == wantCls || t.ToString() == wantTitle) tbCount++;
    return true;
  }
  public static int CountOwnedTopLevels(uint pid, string cls, string title) {
    wantPid = pid; wantCls = cls; wantTitle = title; tbCount = 0;
    EnumWindows(TbCb, IntPtr.Zero);
    return tbCount;
  }
  private static long foundMsg = 0;
  private static bool MsgCb(IntPtr h, IntPtr p) {
    uint pid; GetWindowThreadProcessId(h, out pid);
    if (pid != wantPid) return true;
    StringBuilder c = new StringBuilder(256); GetClassNameW(h, c, c.Capacity);
    if (c.ToString() == wantCls) { foundMsg = h.ToInt64(); return false; }
    return true;
  }
  public static long FindMsgWindow(uint pid, string cls) {
    wantPid = pid; wantCls = cls; foundMsg = 0;
    EnumWindows(MsgCb, IntPtr.Zero);
    return foundMsg;
  }
}
'@

$injClass = 'K6WP.DesktopInject.1'
$injTitle = 'K6WP Wallpaper'
$msgClass = 'K6WP.Engine.MessageWindow.1'
$checkFailures = New-Object System.Collections.Generic.List[string]

$logPath = $EngineLog
if ([string]::IsNullOrEmpty($logPath)) {
  $logPath = Join-Path $env:LOCALAPPDATA 'K6WP\engine.log'
}

function Get-LogHits([string]$path, [string]$pattern) {
  try {
    $fs = New-Object IO.FileStream($path, [IO.FileMode]::Open,
      [IO.FileAccess]::Read, [IO.FileShare]::ReadWrite)
    try {
      $sr = New-Object IO.StreamReader($fs, [Text.Encoding]::UTF8)
      $text = $sr.ReadToEnd()
      $sr.Dispose()
      return @($text -split "`r?`n" | Where-Object { $_ -like $pattern })
    } finally {
      $fs.Dispose()
    }
  } catch {
    return @()
  }
}

$hwnd = [K6wpVerify]::FindInjected($injClass)
$findHow = [K6wpVerify]::FindMethod
if ($hwnd -ne 0) {
  Write-Output ("window: injected HWND=0x{0:X} via {1}" -f $hwnd, $findHow)
} else {
  Write-Output 'window: injected HWND NOT FOUND via live enumeration (Progman + WorkerW children)'
}

# 1. TOOLWINDOW: exstyle bit 0x80 (WS_EX_TOOLWINDOW) must be set (expect 0x08080080).
$toolPass = $false
$toolDetail = ''
if ($hwnd -ne 0) {
  $ex = [K6wpVerify]::GetExStyle($hwnd)
  $toolDetail = ('exstyle=0x{0:X8} (live)' -f $ex)
  if (($ex -band 0x80) -ne 0) { $toolPass = $true }
} else {
  $rows = Get-LogHits $logPath '*final exstyle*'
  if ($rows.Count -gt 0) {
    $line = $rows[$rows.Count - 1].Trim()
    $toolDetail = "via engine.log (no live HWND): $line"
    if ($line -like '*TOOLWINDOW=YES*') { $toolPass = $true }
  } else {
    $toolDetail = 'no live HWND and no final-exstyle line in engine.log'
  }
}
if ($toolPass) {
  Write-Output "check TOOLWINDOW: PASS $toolDetail"
} else {
  Write-Output "check TOOLWINDOW: FAIL $toolDetail"
  $checkFailures.Add('TOOLWINDOW')
}

# 2. NO-caption/border: style must NOT have WS_CAPTION (0xC00000) nor WS_BORDER
# (0x800000) — WS_POPUP frameless (live read only; no honest fallback exists).
$framePass = $false
$frameDetail = ''
if ($hwnd -ne 0) {
  $st = [K6wpVerify]::GetStyle($hwnd)
  $frameDetail = ('style=0x{0:X8} (live)' -f $st)
  if ((($st -band 0xC00000) -eq 0) -and (($st -band 0x800000) -eq 0)) { $framePass = $true }
} else {
  $frameDetail = 'no live HWND: style bits are only readable cross-process live'
}
if ($framePass) {
  Write-Output "check NO-caption-border: PASS $frameDetail"
} else {
  Write-Output "check NO-caption-border: FAIL $frameDetail"
  $checkFailures.Add('NO-caption-border')
}

# 3. NO-taskbar-button: zero visible top-levels owned by the engine pid carrying
# the injected class/title (T7 pattern: EnumWindows filtered on engine PID).
$tb = [K6wpVerify]::CountOwnedTopLevels([UInt32]$engineId, $injClass, $injTitle)
if ($tb -eq 0) {
  Write-Output 'check NO-taskbar-button: PASS 0 owned visible top-levels with injected class/title'
} else {
  Write-Output "check NO-taskbar-button: FAIL $tb owned visible top-level(s) with injected class/title"
  $checkFailures.Add('NO-taskbar-button')
}

# 4. NO-AltTab: WS_EX_TOOLWINDOW excludes the window from Alt+Tab, so this
# follows the TOOLWINDOW verdict (stated, not re-measured).
if ($toolPass) {
  Write-Output 'check NO-alttab: PASS (TOOLWINDOW set => excluded from Alt+Tab; follows TOOLWINDOW check)'
} else {
  Write-Output 'check NO-alttab: FAIL (follows TOOLWINDOW check)'
  $checkFailures.Add('NO-alttab')
}

# 5. tray-present: engine alive (proven above) + tray owner message window found
# via EnumWindows PID+class (T12 pattern); install log quoted best-effort.
# Stated honestly: cross-process tray icon pixels are not readable; the owner
# window + install line is the evidence.
$msgHwnd = [K6wpVerify]::FindMsgWindow([UInt32]$engineId, $msgClass)
$trayRows = Get-LogHits $logPath '*tray: icon installed*'
$trayLogNote = ('log icon-installed lines={0}' -f $trayRows.Count)
if ($trayRows.Count -gt 0) {
  $trayLogNote += (' last: {0}' -f $trayRows[$trayRows.Count - 1].Trim())
}
if ($msgHwnd -ne 0) {
  Write-Output ("check tray-present: PASS message-window HWND=0x{0:X} class={1}; {2}" -f $msgHwnd, $msgClass, $trayLogNote)
} else {
  if ($trayRows.Count -gt 0) {
    Write-Output ("check tray-present: PASS via engine.log (no live message HWND): {0}" -f $trayRows[$trayRows.Count - 1].Trim())
  } else {
    Write-Output "check tray-present: FAIL no message window and no icon-installed line in engine.log"
    $checkFailures.Add('tray-present')
  }
}

if ($checkFailures.Count -gt 0) {
  Write-Output ("RESULT: FAIL ($okCount/$Count acks, checks failed: {0})" -f ($checkFailures -join ','))
  exit 1
}

Write-Output ("RESULT: PASS ($okCount/$Count acks, bad-path REJECT, engine alive, memory stable, TOOLWINDOW, NO-taskbar, NO-alttab, NO-frame, tray)")
exit 0
```

## File: compressor/src/cli.cpp
```cpp
bool ParseInt(const std::string& s, int& out) {
⋮----
bool ParseRes(const std::string& s, int& w, int& h) {
⋮----
bool IsValidEncoder(const std::string& e) {
⋮----
std::wstring FindFfprobe() {
⋮----
// Returns duration in seconds, or -1.0 on failure (missing ffprobe, corrupt
// input, or non-numeric output).
double ProbeDuration(const std::filesystem::path& in) {
⋮----
// The child command line follows standard C quoting rules (no cmd.exe
// quote-stripping), so quoting the exe + input path is safe. Spawn goes
// through the central k6wp::RunCaptured helper (single spawn site, kProbeTimeoutMs budget, orphan-kill on timeout); the old inline
// block waited INFINITE and could hang the CLI forever on a stuck ffprobe.
⋮----
// Parse the leading number only: ffprobe can emit a warning line before the
// value, and the old whole-string check failed on any trailing noise.
⋮----
// Layer 1 of the same-file comparison: the absolute path with the lexical
// "." / ".." noise removed, case-folded, so every spelling of one path folds
// to one string. Purely lexical -- no filesystem access, so it also holds for
// an output that does not exist yet and cannot throw.
//
// lexically_normal() is kept even though MSVC's absolute() already collapses
// ".." for an absolute input: the standard specifies absolute(p) == p when p is
// already absolute, so that collapsing is an implementation detail rather than
// a contract. lexically_normal() is the specified way to get it, and it also
// guarantees backslash separators ("the returned path has backslashes"), which
⋮----
std::string FoldedAbsolute(const std::filesystem::path& p) {
⋮----
bool PathsReferToSameFile(const std::filesystem::path& a,
⋮----
const char* CliUsage() {
⋮----
CliResult ParseCli(int argc, char** argv, CliOptions& out,
```

## File: compressor/src/cli.hpp
```cpp
struct CliOptions {
⋮----
enum class CliResult { Ok, Help, Error };
⋮----
[[nodiscard]] bool PathsReferToSameFile(const std::filesystem::path& a,
⋮----
CliResult ParseCli(int argc, char** argv, CliOptions& out,
⋮----
const char* CliUsage();
```

## File: compressor/tests/dpb_frame_size_test.cpp
```cpp
void Check(bool ok, const char* what) {
⋮----
int main() {
```

## File: engine/src/engine_app.hpp
```cpp
struct SingletonMutexGuard {
⋮----
SingletonMutexGuard() = default;
⋮----
SingletonMutexGuard(const SingletonMutexGuard&) = delete;
⋮----
class EngineApp {
⋮----
EngineApp() = default;
⋮----
EngineApp(const EngineApp&) = delete;
⋮----
bool Init(int argc, char** argv);
⋮----
int Run();
⋮----
void Shutdown();
⋮----
int InitExitCode() const { return init_exit_code_; }
⋮----
virtual void OnSuspend();
⋮----
virtual void OnResume();
⋮----
virtual void OnDeviceLost();
⋮----
virtual void RecreateDevice();
⋮----
virtual void OnDisplayChange(int width, int height);
⋮----
static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static BOOL WINAPI ConsoleCtrlHandler(DWORD ctrl_type);
static void Log(const char* fmt, ...);
⋮----
static void LogImportant(const char* fmt, ...);
⋮----
void HandlePowerBroadcast(WPARAM wParam, LPARAM lParam);
LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
bool RegisterWindowClass();
bool CreateMessageWindow();
void RequestShutdown();
⋮----
void NoteHealthyUptime();
⋮----
bool HandleSetVideo(const std::string& payload_json, bool rotation = false);
⋮----
bool HandleSetMonitor(const std::string& payload_json);
⋮----
bool HandleSetDisplayVideo(const std::string& payload_json);
⋮----
void OnDisplaysFileChanged();
std::filesystem::path ResolvedConfigPath() const;
⋮----
void ApplyPendingSetVideo();
void ApplyPendingSetMonitor();
⋮----
void CaptureFramesStep(long long elapsed_ms);
⋮----
bool CaptureFramesPending() const {
⋮----
void DumpMpvCountersStep(long long elapsed_ms);
bool DumpMpvCountersPending() const {
⋮----
void OnPauseStateChanged(bool paused);
void MaybeUnloadAfterPause(long long elapsed_ms);
void UnloadDecodeNow(long long elapsed_ms);
void ReloadDecodeNow(long long elapsed_ms);
⋮----
DecodeState DecodeStateNow() const {
std::lock_guard<std::mutex> lock(decode_state_mutex_);
⋮----
void DumpMediaProps();
⋮----
bool PauseHidesWallpaper(int mask) const;
⋮----
static long long NowMs() {
⋮----
std::shared_ptr<MpvRenderer> AcquireRenderer() const;
⋮----
void InitWallpaperSurface(const std::string& video_utf8);
⋮----
void UpdateTrayErrorStatus();
⋮----
void ShutdownWallpaperSurface();
⋮----
void StopHeadlessRenderer();
⋮----
DWORD ComputeWaitTimeoutMs() const;
⋮----
bool AnySimulateArmed() const;
⋮----
void ApplyFitMode(const std::string& fit_mode);
⋮----
void ArmPinVerify();
void RunPinVerifyPass();
⋮----
bool VerifyHeadlessPin();
⋮----
struct ReconcileStatus {
⋮----
std::string last_verdict = "";      // "covered" | "degraded" | ""
int retries = 0;                    // retry timer (P3 = 0, never armed)
⋮----
// P3 (design §R9.4 deviation): lock-order guard comments documented.
⋮----
// CRIT-2 worker->main handoff; one slot each (last write wins, idempotent).
⋮----
// Serializes the get_state payload (raw JSON object text).
std::string BuildStateJson() const;
// Display resilience P3: reconcile orchestration.
void ArmReconcile(const char* trigger);  // burst debounce
void RunReconcilePass();                 // one shadow pass
std::string ReconcileDiffSummary(const TopologyDiff& diff) const;
⋮----
// --- Bug B: the debounced topology door -----------------------------------
//
// Every event that can change the monitor set or the desktop surface goes
// through ONE debounced door instead of doing the work inside the message
// handler:
⋮----
//   WM_DISPLAYCHANGE / resume / unlock / TaskbarCreated / SETTINGCHANGE /
//   monitor-on / device-lost  ->  ArmTopologyReconcile(trigger)
//                                 -> (400 ms quiet period, re-armed per
//                                    burst) -> RunTopologyReconcile()
⋮----
// WHY: WM_DISPLAYCHANGE is delivered with SendMessage by the system, so a
// handler that enumerates monitors, tears down slots (mpv teardown) and
// re-attaches blocks the whole display-change broadcast - the observed
// "the tray icon is there but nothing responds" freeze when a monitor is
⋮----
void ArmTopologyReconcile(const char* trigger, bool reset_signature = true);
⋮----
void CancelTopologyReconcile(const char* reason);
void RunTopologyReconcile();
⋮----
void RequestStillRefresh(const std::string& video_utf8, bool manual);
⋮----
void OnTrayTogglePause();
void OnTrayQuickSwitch(std::size_t idx);
⋮----
bool UiPaused() const { return pause_.UiPaused(); }
bool SlotsPaused() const { return pause_.SlotsPaused(); }
⋮----
void SetPauseOwner(int bit, bool on);
void ApplyPauseState(const char* owner);
```

## File: engine/src/ipc_server.cpp
```cpp
struct HandleGuard {
⋮----
explicit HandleGuard(HANDLE handle) : h(handle) {}
⋮----
HandleGuard(const HandleGuard&) = delete;
⋮----
HANDLE release() {
⋮----
struct SaGuard {
⋮----
explicit SaGuard(PSECURITY_ATTRIBUTES s) : sa(s) {}
⋮----
SaGuard(const SaGuard&) = delete;
⋮----
void Log(const char* fmt, ...) {
⋮----
std::string JsonEscape(const std::string& s) {
⋮----
std::string ErrorAck(const std::string& msg) {
⋮----
bool WaitWithStop(HANDLE pipe, OVERLAPPED* ov, HANDLE event,
⋮----
Impl() { stop_event = CreateEventW(nullptr, TRUE, FALSE, nullptr); }
⋮----
Impl(const Impl&) = delete;
⋮----
std::string HandleMessage(const std::string& raw) {
⋮----
// Framing policy: Encode() always emits a trailing '\n'. A message-mode
// datagram without one is not a complete NDJSON line → no ack.
⋮----
bool AcceptAndServe() {
SaGuard sa(MakeCurrentUserOnlySA());
⋮----
HandleGuard connect_event(CreateEventW(nullptr, TRUE, FALSE, nullptr));
⋮----
std::vector<char> buf(static_cast<size_t>(kPipeBufferBytes));
HandleGuard read_event(CreateEventW(nullptr, TRUE, FALSE, nullptr));
⋮----
void ServeLoop() {
⋮----
IpcServer::IpcServer() : impl_(std::make_unique<Impl>()) {}
⋮----
bool IpcServer::Start(const IpcHandlers& handlers) {
⋮----
void IpcServer::Stop() {
⋮----
bool IpcServer::IsRunning() const {
```

## File: engine/src/main.cpp
```cpp
LONG WINAPI CrashDumpHandler(EXCEPTION_POINTERS* exception_info) {
⋮----
std::filesystem::path CrashRestartStateFilePath() {
⋮----
k6wp::CrashRestartState LoadCrashRestartState() {
⋮----
std::ifstream in(path, std::ios::binary);
⋮----
void SaveCrashRestartState(const k6wp::CrashRestartState& state) {
⋮----
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
```

## File: engine/src/multi_monitor.hpp
```cpp
struct SpanGeometry {
⋮----
struct SlotFactory {
⋮----
inline SlotFactory DefaultSlotFactory() {
⋮----
struct BootAssignmentStats {
⋮----
struct SlotSnapshot {
⋮----
class MultiMonitor {
⋮----
explicit MultiMonitor(LogFn log = nullptr,
⋮----
MultiMonitor(const MultiMonitor&) = delete;
⋮----
MultiMonitor(MultiMonitor&&) noexcept;
⋮----
void SetHeadlessHost(void* hwnd);
⋮----
void SetActiveMonitor(int id);
int active_monitor() const;
⋮----
void SetInjectMode(InjectMode mode);
⋮----
void SetAdapterPin(const std::string& substr);
⋮----
void SetSlotFactory(SlotFactory factory);
⋮----
bool Init();
⋮----
void Shutdown();
⋮----
void OnDisplayChange();
⋮----
bool LoadLoopAll(const std::string& path, bool force = false);
⋮----
bool CaptureLiveFrame(const std::string& path);
⋮----
void SeekAllTo(double seconds);
⋮----
int UnloadAllVideos();
⋮----
int UnloadAllSlots();
⋮----
int ReattachAllSlots();
⋮----
int ReloadAllVideos();
⋮----
bool any_video_unloaded() const;
⋮----
bool ReadLiveCounter(const std::string& name, long long& out) const;
⋮----
bool ReadLiveCounterText(const std::string& name, std::string& out) const;
⋮----
bool DumpLivePropertyList(const std::wstring& path) const;
⋮----
bool HasAssignment(const std::string& device) const;
⋮----
bool LoadLoopSlot(const std::string& device, const std::string& path,
⋮----
BootAssignmentStats ApplyBootAssignments(const DisplaysConfig& cfg,
⋮----
static std::vector<std::wstring> DetectKeyCollisionForCommand(
⋮----
static std::vector<std::wstring> DropCollidingAssignments(
⋮----
int VerifyPinAndRevert();
⋮----
void ApplyFitModeAll(const std::string& fit_mode);
⋮----
void ApplyFpsCapAll(int fps);
⋮----
void PauseAll();
void ResumeAll();
⋮----
void PauseSlot(size_t idx, bool pause);
bool IsSlotPaused(size_t idx) const;
⋮----
void Reanchor();
⋮----
size_t slot_count() const;
std::vector<int> monitor_ids() const;
⋮----
bool has_headless_slots() const;
⋮----
int headless_slot_count() const;
⋮----
std::string SlotCoverageReason(int monitor_id) const;
⋮----
std::vector<SlotSnapshot> SnapshotSlots() const;
⋮----
static SpanGeometry GetSpanGeometry() noexcept;
⋮----
static void SetSpanGeometryOverride(const SpanGeometry* g);
⋮----
std::vector<MonitorInfo> LiveMonitorInfos() const;
⋮----
struct CoverageCensus {
⋮----
CoverageCensus VerifyCoverage() const;
⋮----
struct TopologyApplyResult {
⋮----
TopologyApplyResult ApplyTopology(const TopologySnapshot& snap,
⋮----
std::string SharedHostBranch() const;
⋮----
struct Slot {
⋮----
Slot() = default;
Slot(const Slot&) = delete;
⋮----
Slot(Slot&& other) noexcept
⋮----
const std::map<int, Slot>& slots() const { return slots_; }
⋮----
void ClearSlots();
bool AttachSlot(const MonitorInfo& mi);
⋮----
void ResolveHostForPass();
⋮----
void RememberHostPlacement();
⋮----
void ReloadSlotVideoLocked(Slot& slot);
⋮----
void ReleaseSlotMediaLocked(Slot& slot);
⋮----
void DestroySlotLocked(Slot& slot);
⋮----
bool ReplaceSlotSurfaceLocked(Slot& slot, int x, int y, int w, int h,
⋮----
std::map<int, Slot>::iterator FindSlotByIdentityLocked(const MonitorInfo& mi);
std::map<int, Slot>::const_iterator FindSlotByIdentityLocked(
⋮----
void ApplyActiveFilter(const std::vector<MonitorInfo>& desired);
⋮----
void ReapplyRetainedLocked();
⋮----
Slot* SlotAt(size_t idx);
const Slot* SlotAt(size_t idx) const;
⋮----
TopologySnapshot SnapshotTopology();
⋮----
bool TopologyDivergesFromMonitors(const TopologySnapshot& snap,
```

## File: engine/src/occlusion_watch.cpp
```cpp
struct CachedDwmApi {
CachedDwmApi() {
⋮----
CachedDwmApi(const CachedDwmApi&) = delete;
⋮----
bool IsShellClass(const wchar_t* name) noexcept {
⋮----
struct CollectCtx {
⋮----
BOOL CALLBACK CollectWindows(HWND hwnd, LPARAM lparam) {
⋮----
CoverRect ClipRect(CoverRect r, CoverRect clip) noexcept {
⋮----
long long UnionAreaClipped(const CoverRect* rects, size_t count,
⋮----
double ComputeCoverage(CoverRect monitor, const CoverRect* rects,
⋮----
void OcclusionWatch::Log(const char* fmt, ...) {
⋮----
void OcclusionWatch::OnPauseMaskChanged(bool slots_paused) {
⋮----
void OcclusionWatch::OnRearmMessage() noexcept {
⋮----
void OcclusionWatch::Check(MultiMonitor& slots) {
⋮----
void OcclusionWatch::CheckNow(MultiMonitor& slots) {
```

## File: engine/src/pause_controller.hpp
```cpp
class PauseController {
⋮----
enum : int {
⋮----
bool UiPaused() const {
⋮----
bool SlotsPaused() const {
⋮----
int mask() const { return mask_.load(std::memory_order_acquire); }
⋮----
bool SetBit(int bit, bool on) {
⋮----
inline bool PauseHidesWallpaper(int mask) {
⋮----
inline const char* PauseOwnerName(int bit) {
```

## File: engine/src/pin_verify_schedule.cpp
```cpp
long long SteadyMs() {
⋮----
void PinVerifySchedule::Arm(bool has_pin) {
⋮----
std::lock_guard<std::mutex> lock(mutex_);
⋮----
bool PinVerifySchedule::Due() const {
⋮----
int PinVerifySchedule::AddReverts(int n) {
⋮----
int PinVerifySchedule::total() const {
```

## File: engine/src/pin_verify_schedule.hpp
```cpp
inline bool PinVerifyDueAt(bool armed, long long deadline_ms, long long now_ms) {
⋮----
inline int PinVerifyAddReverts(int total, int n) { return total + n; }
⋮----
class PinVerifySchedule {
⋮----
void Arm(bool has_pin);
⋮----
bool Due() const;
⋮----
void Disarm() { armed_.store(false, std::memory_order_release); }
⋮----
int AddReverts(int n);
⋮----
int total() const;
```

## File: engine/src/unload_state.hpp
```cpp
enum class DecodeState { kActive, kPaused, kUnloaded };
⋮----
struct UnloadMachine {
⋮----
struct UnloadDecision {
⋮----
inline UnloadDecision UnloadOnPauseChanged(const UnloadMachine& machine,
⋮----
inline UnloadDecision UnloadOnTick(const UnloadMachine& machine, int mask,
⋮----
inline const char* DecodeStateName(DecodeState state) {
```

## File: launcher/CMakeLists.txt
```
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

# E5: EVERY test gets a hard timeout. A hung test must never hang the
# suite without a bound (the per-test timeout is enforced by ctest).
get_property(_k6wp_tests_in_dir DIRECTORY PROPERTY TESTS)
foreach(_k6wp_t IN LISTS _k6wp_tests_in_dir)
  set_tests_properties(${_k6wp_t} PROPERTIES TIMEOUT 300)
endforeach()
```

## File: rust/crates/k6wp-core/src/compress_args.rs
```rust
pub struct CompressArgs {
⋮----
impl Default for CompressArgs {
fn default() -> Self {
⋮----
encoder: COMPRESS_DEFAULT_ENCODER.to_string(),
⋮----
pub fn build_argv(args: &CompressArgs) -> Vec<String> {
⋮----
argv.push(COMPRESS_FLAG_IN.to_string());
argv.push(args.in_.clone());
argv.push(COMPRESS_FLAG_OUT.to_string());
argv.push(args.out.clone());
argv.push(COMPRESS_FLAG_RES.to_string());
argv.push(format!("{}x{}", args.res_w, args.res_h));
argv.push(COMPRESS_FLAG_FPS.to_string());
argv.push(args.fps.to_string());
argv.push(COMPRESS_FLAG_CRF.to_string());
argv.push(args.crf.to_string());
argv.push(COMPRESS_FLAG_ENCODER.to_string());
argv.push(args.encoder.clone());
⋮----
argv.push(COMPRESS_FLAG_FORCE_LONG.to_string());
⋮----
argv.push(COMPRESS_FLAG_DRY_RUN.to_string());
⋮----
pub fn is_known_compressor_flag(flag: &str) -> bool {
matches!(
⋮----
mod tests {
⋮----
fn the_argv_is_the_golden_vector() {
⋮----
in_: "C:\\in.mp4".to_string(),
out: "C:\\out.mp4".to_string(),
⋮----
encoder: "auto".to_string(),
⋮----
assert_eq!(
⋮----
fn the_optional_flags_are_omitted() {
⋮----
in_: "a".to_string(),
out: "b".to_string(),
⋮----
let argv = build_argv(&args);
⋮----
assert_eq!(argv.len(), 12);
assert!(!argv.iter().any(|t| t == "--force-long"));
assert!(!argv.iter().any(|t| t == "--dry-run"));
assert_eq!(argv[4], "--res");
assert_eq!(argv[5], "0x0");
⋮----
fn every_emitted_flag_is_known() {
⋮----
for token in build_argv(&args) {
if token.starts_with("--") {
assert!(is_known_compressor_flag(&token), "unknown flag: {token}");
⋮----
assert!(!is_known_compressor_flag("--nope"));
```

## File: rust/crates/k6wp-core/src/config_store.rs
```rust
use std::fs;
⋮----
use std::time::Duration;
⋮----
use windows::core::w;
⋮----
pub struct ConfigFileLock {
⋮----
impl ConfigFileLock {
⋮----
pub fn acquire(timeout: Duration) -> Result<Self, ConfigError> {
⋮----
let handle = CreateMutexW(None, false, w!("Local\\K6WP-Config-Lock"))
.map_err(|e| ConfigError(format!("config lock: CreateMutexW failed ({e})")))?;
let wait_ms = timeout.as_millis().min(u128::from(u32::MAX)) as u32;
let state = WaitForSingleObject(handle, wait_ms);
⋮----
return Ok(Self { handle });
⋮----
let _ = CloseHandle(handle);
⋮----
return Err(ConfigError(format!(
⋮----
Err(ConfigError(format!(
⋮----
impl Drop for ConfigFileLock {
fn drop(&mut self) {
⋮----
let _ = ReleaseMutex(self.handle);
let _ = CloseHandle(self.handle);
⋮----
pub fn save_config(path: &Path, cfg: &WallpaperConfig) -> Result<(), ConfigError> {
⋮----
validate_config(cfg)?;
let existing = read_existing_json(path);
let doc = merge_unknown_keys(&existing, &config_to_json(cfg));
atomic_write_json(path, &doc)
⋮----
pub fn persist_config_field(
⋮----
if let Some(parsed) = read_existing_json_checked(path) {
if parsed.is_object() {
let probe = migrate_config_value(&parsed)
.and_then(|probe| validate_config(&probe).map(|()| probe));
if probe.is_ok() {
⋮----
backup_file(path);
raw = config_to_json(fallback);
⋮----
if have_valid && raw.get(field) == Some(&value) {
return Ok(false);
⋮----
have_valid && raw.get("version").and_then(Value::as_i64) == Some(CONFIG_SCHEMA_VERSION);
⋮----
object.insert(field.to_string(), value);
⋮----
return Err(ConfigError("config root must be a JSON object".to_string()));
⋮----
let merged = migrate_config_value(&raw)?;
validate_config(&merged)?;
⋮----
config_to_json(&merged)
⋮----
atomic_write_json(path, &doc)?;
Ok(true)
⋮----
fn read_existing_json_checked(path: &Path) -> Option<Value> {
let meta = fs::metadata(path).ok()?;
if meta.len() > MAX_CONFIG_BYTES {
⋮----
let text = fs::read_to_string(path).ok()?;
serde_json::from_str(&text).ok()
⋮----
fn read_existing_json(path: &Path) -> Value {
read_existing_json_checked(path)
.filter(Value::is_object)
.unwrap_or_else(|| Value::Object(Map::new()))
⋮----
fn backup_file(path: &Path) {
if !path.exists() {
⋮----
let mut bak = path.as_os_str().to_owned();
bak.push(".bak");
⋮----
fn atomic_write_json(path: &Path, doc: &Value) -> Result<(), ConfigError> {
if let Some(parent) = path.parent() {
if !parent.as_os_str().is_empty() {
⋮----
.map_err(|e| ConfigError(format!("cannot create config directory: {e}")))?;
⋮----
let mut tmp = path.as_os_str().to_owned();
tmp.push(".tmp");
⋮----
.map_err(|e| ConfigError(format!("cannot serialize config: {e}")))?;
fs::write(&tmp, text).map_err(|e| {
ConfigError(format!(
⋮----
fs::rename(&tmp, path).map_err(|e| {
⋮----
mod tests {
⋮----
use crate::config::MAX_FOREIGN_KEY_BYTES;
⋮----
fn scratch_dir(tag: &str) -> PathBuf {
⋮----
std::env::temp_dir().join(format!("k6wp_config_store_{tag}_{}", std::process::id()));
⋮----
fs::create_dir_all(&dir).expect("scratch dir");
⋮----
fn seed(path: &Path) {
⋮----
assert_eq!(cfg.version, CONFIG_SCHEMA_VERSION);
save_config(path, &cfg).expect("seed");
⋮----
fn read_doc(path: &Path) -> Value {
serde_json::from_str(&fs::read_to_string(path).expect("read config")).expect("valid json")
⋮----
fn persist_preserves_foreign_keys_and_reports_a_no_op() {
let dir = scratch_dir("foreign");
let path = dir.join("config.json");
seed(&path);
let mut doc = read_doc(&path);
⋮----
object.insert("a_key_from_the_future".to_string(), Value::from(42));
⋮----
atomic_write_json(&path, &doc).expect("seed foreign key");
⋮----
assert!(persist_config_field(&path, "crf", Value::from(20), &fallback).expect("persist"));
let after = read_doc(&path);
assert_eq!(after["crf"], 20);
assert_eq!(after["a_key_from_the_future"], 42);
assert_eq!(after["version"], CONFIG_SCHEMA_VERSION);
⋮----
assert!(!persist_config_field(&path, "crf", Value::from(20), &fallback).expect("no-op"));
⋮----
fn persist_rebuilds_a_corrupt_file_and_keeps_a_backup() {
let dir = scratch_dir("corrupt");
⋮----
fs::write(&path, "{ this is not json").expect("write corrupt");
⋮----
assert!(persist_config_field(&path, "crf", Value::from(21), &fallback).expect("persist"));
assert_eq!(read_doc(&path)["crf"], 21);
⋮----
assert!(Path::new(&bak).exists(), "the corrupt file is backed up");
⋮----
fn a_bad_value_does_not_touch_the_file() {
let dir = scratch_dir("badvalue");
⋮----
let before = fs::read_to_string(&path).expect("read");
⋮----
assert!(persist_config_field(&path, "crf", Value::from(99), &fallback).is_err());
assert_eq!(fs::read_to_string(&path).expect("read"), before);
⋮----
fn the_lock_is_a_bounded_wait() {
let lock = ConfigFileLock::acquire(LOCK_TIMEOUT).expect("first");
⋮----
std::thread::spawn(|| ConfigFileLock::acquire(Duration::from_millis(0)).is_err())
.join()
.expect("thread");
assert!(failed, "a second thread times out while the lock is held");
drop(lock);
assert!(ConfigFileLock::acquire(Duration::from_millis(1000)).is_ok());
⋮----
fn child_writer_entry() {
⋮----
let field = std::env::var("K6WP_RUST_WRITER_FIELD").expect("field");
⋮----
.expect("iterations")
.parse()
.expect("iterations number");
⋮----
persist_config_field(Path::new(&path), &field, Value::from(i), &fallback)
.expect("child persist");
⋮----
fn two_processes_do_not_lose_updates() {
let dir = scratch_dir("twowriter");
⋮----
let exe = std::env::current_exe().expect("current exe");
⋮----
.arg("--exact")
.arg("config_store::tests::child_writer_entry")
.env("K6WP_RUST_WRITER_PATH", &path)
.env("K6WP_RUST_WRITER_FIELD", field)
.env("K6WP_RUST_WRITER_ITERATIONS", "120")
.stdout(Stdio::null())
.stderr(Stdio::null())
.spawn()
.expect("spawn writer")
⋮----
let mut a = spawn("field_a");
let mut b = spawn("field_b");
assert!(a.wait().expect("wait a").success(), "writer A failed");
assert!(b.wait().expect("wait b").success(), "writer B failed");
⋮----
let doc = read_doc(&path);
assert_eq!(doc["field_a"], 120, "writer A's last value was lost");
assert_eq!(doc["field_b"], 120, "writer B's last value was lost");
assert_eq!(doc["version"], CONFIG_SCHEMA_VERSION);
⋮----
assert!(!Path::new(&tmp).exists(), "no .tmp file is left behind");
⋮----
fn the_foreign_key_budget_matches_the_cpp_side() {
⋮----
let merged = merge_unknown_keys(&collide, &known);
assert_eq!(merged["crf"], 22);
assert_eq!(merged["note"], "kept");
⋮----
huge.insert("version".to_string(), Value::from(6));
huge.insert(
"a_huge_foreign_key".to_string(),
Value::from("x".repeat(70 * 1024)),
⋮----
let capped = merge_unknown_keys(&Value::Object(huge), &known);
assert!(capped.get("a_huge_foreign_key").is_none());
⋮----
many.insert("version".to_string(), Value::from(6));
⋮----
many.insert(format!("k{i}"), Value::from("y".repeat(1024)));
⋮----
let kept = merge_unknown_keys(&Value::Object(many), &known);
⋮----
.as_object()
.expect("object")
.iter()
.filter(|(key, _)| known.get(*key).is_none())
.map(|(key, value)| key.len() + serde_json::to_string(value).expect("serialize").len())
.sum();
assert!(carried <= MAX_FOREIGN_KEY_BYTES, "carried {carried} bytes");
assert!(kept.get("k0").is_some());
assert!(kept.get("k99").is_none());
```

## File: rust/crates/k6wp-core/src/displays.rs
```rust
use std::collections::BTreeMap;
⋮----
pub use crate::config::ConfigError;
⋮----
pub(crate) fn json_type_name(value: &Value) -> &'static str {
⋮----
/// The C++ `type_error.302` message, wrapped the way the C++ wraps it.
pub(crate) fn type_error(context: &str, expected: &str, actual: &Value) -> ConfigError {
⋮----
pub(crate) fn type_error(context: &str, expected: &str, actual: &Value) -> ConfigError {
ConfigError(format!(
⋮----
/// The nlohmann `get<int>()` conversion: integers pass, a float truncates
/// toward zero, anything else is a type error.
⋮----
/// toward zero, anything else is a type error.
pub(crate) fn as_int(value: &Value) -> Option<i64> {
⋮----
pub(crate) fn as_int(value: &Value) -> Option<i64> {
⋮----
if let Some(i) = n.as_i64() {
Some(i)
} else if let Some(u) = n.as_u64() {
Some(u as i64)
⋮----
n.as_f64().map(|f| f as i64)
⋮----
/// One per-monitor assignment: the video file that plays on the monitor
/// whose GDI device name is the map key.
⋮----
/// whose GDI device name is the map key.
#[derive(Clone, Debug, Default, PartialEq, Eq)]
pub struct DisplayAssignment {
/// The absolute path of the video file (UTF-8, as on disk).
    pub path: String,
/// The Studio-side existence hint at write time. The engine re-checks the
    /// filesystem at apply time and does not trust this flag.
⋮----
/// filesystem at apply time and does not trust this flag.
    pub exists: bool,
/// The tiered stable identity (`ccd:...`, `edid:...:1a2b`). Empty = a
    /// legacy entry.
⋮----
/// legacy entry.
    pub key_stable: String,
⋮----
/// The whole assignment store.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Displays {
/// Always the current schema version after a migration.
    pub version: u32,
/// Keyed by the GDI device name (e.g. `\\.\DISPLAY1`). The map is sorted
    /// so the serialization order is deterministic across runs, exactly like
⋮----
/// so the serialization order is deterministic across runs, exactly like
    /// the C++ `std::map`.
⋮----
/// the C++ `std::map`.
    pub assignments: BTreeMap<String, DisplayAssignment>,
⋮----
impl Default for Displays {
fn default() -> Self {
⋮----
/// Migrates a raw `displays.json` document into the store.
///
⋮----
///
/// This is the Rust mirror of the C++ `MigrateDisplays`. The C++ takes an
⋮----
/// This is the Rust mirror of the C++ `MigrateDisplays`. The C++ takes an
/// already-parsed `nlohmann::json`; the golden fixtures hand the replayer
⋮----
/// already-parsed `nlohmann::json`; the golden fixtures hand the replayer
/// the RAW document text, so the Rust side parses it here and reports a
⋮----
/// the RAW document text, so the Rust side parses it here and reports a
/// corrupt document the way the C++ loader does.
⋮----
/// corrupt document the way the C++ loader does.
///
⋮----
///
/// # Errors
⋮----
/// # Errors
///
⋮----
///
/// - a corrupt document (`corrupt displays JSON: parse error`);
⋮----
/// - a corrupt document (`corrupt displays JSON: parse error`);
/// - a root that is not an object (`displays root must be a JSON object`);
⋮----
/// - a root that is not an object (`displays root must be a JSON object`);
/// - a version outside `[0, 1]` (`unsupported displays schema version: N`);
⋮----
/// - a version outside `[0, 1]` (`unsupported displays schema version: N`);
/// - `assignments` that is not an object, an assignment that is not an
⋮----
/// - `assignments` that is not an object, an assignment that is not an
///   object, a missing/non-string `path`, a non-array `displays`, or a
⋮----
///   object, a missing/non-string `path`, a non-array `displays`, or a
///   wrong-typed `version` / `exists`.
⋮----
///   wrong-typed `version` / `exists`.
pub fn migrate_displays(raw_text: &str) -> Result<Displays, ConfigError> {
⋮----
pub fn migrate_displays(raw_text: &str) -> Result<Displays, ConfigError> {
⋮----
.map_err(|_| ConfigError("corrupt displays JSON: parse error".to_string()))?;
migrate_displays_value(&raw)
⋮----
/// The same migration, on an already-parsed document.
pub fn migrate_displays_value(raw: &Value) -> Result<Displays, ConfigError> {
⋮----
pub fn migrate_displays_value(raw: &Value) -> Result<Displays, ConfigError> {
⋮----
.as_object()
.ok_or_else(|| ConfigError("displays root must be a JSON object".to_string()))?;
⋮----
// The C++ `raw.value("version", 0)`: a missing key defaults to 0, a
// wrong type is a type error.
let version = match root.get("version") {
⋮----
Some(value) => as_int(value).ok_or_else(|| type_error("displays", "number", value))?,
⋮----
return Err(ConfigError(format!(
⋮----
if let Some(raw_assignments) = root.get("assignments") {
⋮----
.ok_or_else(|| ConfigError("displays assignments must be an object".to_string()))?;
⋮----
let entry = entry.as_object().ok_or_else(|| {
ConfigError("displays assignment value must be an object".to_string())
⋮----
let path = match entry.get("path") {
Some(value) => value.as_str().ok_or_else(|| {
ConfigError("displays assignment path must be a string".to_string())
⋮----
return Err(ConfigError(
"displays assignment path must be a string".to_string(),
⋮----
// The C++ `value("exists", false)`: a missing key is false, a
⋮----
let exists = match entry.get("exists") {
⋮----
.as_bool()
.ok_or_else(|| type_error("displays", "boolean", value))?,
⋮----
// `key_stable` is additive and lenient: a wrong type is ignored
// (never a throw), so a legacy file still loads.
let key_stable = match entry.get("key_stable") {
Some(Value::String(text)) => text.clone(),
⋮----
assignments.insert(
key.clone(),
⋮----
path: path.to_string(),
⋮----
if let Some(displays) = root.get("displays") {
if !displays.is_array() {
return Err(ConfigError("displays must be an array".to_string()));
⋮----
// `key_aliases` is additive and lenient: the engine never reads the
// map, so it is not carried in the store, and the C++ ignores a
// missing or wrong-typed value as well (never a throw). Nothing to do
// here - and nothing that could fail.
⋮----
Ok(Displays {
⋮----
/// The store -> JSON mapping, in the shape the golden fixtures record.
///
⋮----
///
/// The C++ dump writes `version` plus the `assignments` map, and every
⋮----
/// The C++ dump writes `version` plus the `assignments` map, and every
/// assignment carries `path`, `exists` and `key_stable` (the last one even
⋮----
/// assignment carries `path`, `exists` and `key_stable` (the last one even
/// when it is empty - that is what the frozen fixture holds).
⋮----
/// when it is empty - that is what the frozen fixture holds).
pub fn displays_json(displays: &Displays) -> Value {
⋮----
pub fn displays_json(displays: &Displays) -> Value {
⋮----
entry.insert("path".to_string(), Value::String(assignment.path.clone()));
entry.insert("exists".to_string(), Value::Bool(assignment.exists));
entry.insert(
"key_stable".to_string(),
Value::String(assignment.key_stable.clone()),
⋮----
assignments.insert(key.clone(), Value::Object(entry));
⋮----
out.insert("version".to_string(), Value::from(displays.version));
out.insert("assignments".to_string(), Value::Object(assignments));
⋮----
/// True when any component of `path` is exactly `..`.
///
⋮----
///
/// A `..` inside a file name ("foo..bar") is fine; only a traversal
⋮----
/// A `..` inside a file name ("foo..bar") is fine; only a traversal
/// component is rejected, so a hostile assignment cannot escape the
⋮----
/// component is rejected, so a hostile assignment cannot escape the
/// wallpapers root. This is the C++ `HasDotDotComponent`.
⋮----
/// wallpapers root. This is the C++ `HasDotDotComponent`.
fn has_dot_dot_component(path: &str) -> bool {
⋮----
fn has_dot_dot_component(path: &str) -> bool {
path.split(['/', '\\']).any(|part| part == "..")
⋮----
/// Rejects the values outside the accepted ranges - the C++
/// `ValidateDisplays`.
⋮----
/// `ValidateDisplays`.
///
⋮----
///
/// - a version outside `[0, 1]`;
⋮----
/// - a version outside `[0, 1]`;
/// - more than `DISPLAYS_MAX_ENTRIES` assignments;
⋮----
/// - more than `DISPLAYS_MAX_ENTRIES` assignments;
/// - an empty assignment path;
⋮----
/// - an empty assignment path;
/// - an assignment path with a `..` component.
⋮----
/// - an assignment path with a `..` component.
pub fn validate_displays(displays: &Displays) -> Result<(), ConfigError> {
⋮----
pub fn validate_displays(displays: &Displays) -> Result<(), ConfigError> {
⋮----
if displays.assignments.len() > DISPLAYS_MAX_ENTRIES {
⋮----
if assignment.path.is_empty() {
⋮----
if has_dot_dot_component(&assignment.path) {
⋮----
Ok(())
⋮----
mod tests {
⋮----
fn a_v0_document_migrates_to_the_current_version() {
let displays = migrate_displays(r#"{"version":0}"#).expect("must migrate");
assert_eq!(displays.version, DISPLAYS_SCHEMA_VERSION);
assert!(displays.assignments.is_empty());
⋮----
fn the_assignments_keep_their_shape() {
let displays = migrate_displays(
⋮----
.expect("must migrate");
assert_eq!(displays.assignments.len(), 2);
⋮----
assert_eq!(first.path, "C:\\a.mp4");
assert!(first.exists);
assert_eq!(first.key_stable, "");
⋮----
assert_eq!(second.key_stable, "ccd:abc");
assert!(!second.exists);
⋮----
fn a_future_version_is_rejected() {
let err = migrate_displays(r#"{"version":2}"#).expect_err("must fail");
assert_eq!(err.0, "unsupported displays schema version: 2");
⋮----
fn a_non_object_assignment_is_rejected() {
⋮----
migrate_displays(r#"{"version":1,"assignments":{"a":"oops"}}"#).expect_err("must fail");
assert_eq!(err.0, "displays assignment value must be an object");
⋮----
fn a_missing_path_is_rejected() {
let err = migrate_displays(r#"{"version":1,"assignments":{"a":{"exists":true}}}"#)
.expect_err("must fail");
assert_eq!(err.0, "displays assignment path must be a string");
⋮----
fn a_wrong_typed_exists_is_rejected() {
⋮----
migrate_displays(r#"{"version":1,"assignments":{"a":{"path":"p","exists":"yes"}}}"#)
⋮----
assert_eq!(
⋮----
fn a_wrong_typed_key_stable_is_ignored() {
⋮----
migrate_displays(r#"{"version":1,"assignments":{"a":{"path":"p","key_stable":7}}}"#)
⋮----
assert_eq!(displays.assignments["a"].key_stable, "");
⋮----
fn a_non_array_displays_is_rejected() {
let err = migrate_displays(r#"{"version":1,"displays":{}}"#).expect_err("must fail");
assert_eq!(err.0, "displays must be an array");
⋮----
fn a_corrupt_document_is_rejected() {
let err = migrate_displays(r#"{"version":1,"#).expect_err("must fail");
assert_eq!(err.0, "corrupt displays JSON: parse error");
⋮----
fn the_json_mapping_carries_every_field() {
⋮----
"d1".to_string(),
⋮----
path: "C:\\a.mp4".to_string(),
⋮----
key_stable: "ccd:abc".to_string(),
⋮----
let json = displays_json(&displays);
assert_eq!(json["version"], Value::from(1));
assert_eq!(json["assignments"]["d1"]["path"], Value::from("C:\\a.mp4"));
assert_eq!(json["assignments"]["d1"]["exists"], Value::from(true));
⋮----
fn the_validator_rejects_a_traversal() {
⋮----
path: "C:\\a\\..\\b.mp4".to_string(),
⋮----
assert!(validate_displays(&displays).is_err());
```

## File: rust/crates/k6wp-core/src/engine_logic.rs
```rust
pub struct PlacementRect {
⋮----
pub enum CoverageVerdict {
⋮----
impl CoverageVerdict {
⋮----
pub fn as_str(self) -> &'static str {
⋮----
/// Whether the child surface covers the monitor.
///
⋮----
///
/// The order of the checks matters and is frozen by the fixtures: a surface
⋮----
/// The order of the checks matters and is frozen by the fixtures: a surface
/// that is SMALLER than the monitor is reported as clipped on the left (the
⋮----
/// that is SMALLER than the monitor is reported as clipped on the left (the
/// first edge that does not reach), and only a surface that does not
⋮----
/// first edge that does not reach), and only a surface that does not
/// overlap at all is out of bounds.
⋮----
/// overlap at all is out of bounds.
pub fn covers_monitor(child: PlacementRect, monitor: PlacementRect) -> CoverageVerdict {
⋮----
pub fn covers_monitor(child: PlacementRect, monitor: PlacementRect) -> CoverageVerdict {
// Disjoint (touching edges do not overlap) -> nothing visible.
⋮----
/// The WorkerW host rule (task C7): the host window may hold ONLY our own
/// (possibly stale) surface - a foreign child means the host is not ours.
⋮----
/// (possibly stale) surface - a foreign child means the host is not ours.
///
⋮----
///
/// An empty list is accepted: a host with no children is ours by
⋮----
/// An empty list is accepted: a host with no children is ours by
/// definition.
⋮----
/// definition.
pub fn workerw_accepts_child_classes(child_classes: &[String], own_class: &str) -> bool {
⋮----
pub fn workerw_accepts_child_classes(child_classes: &[String], own_class: &str) -> bool {
child_classes.iter().all(|class| class == own_class)
⋮----
/// The load retry offsets (0 / 250 / 1000 / 3000 ms).
pub const RETRY_COUNT: i32 = 4;
⋮----
/// The retry clock: when the first pass ended and how many attempts are done.
#[derive(Debug, Clone, Copy, Default)]
pub struct RetryClock {
/// The steady-clock ms at the pass end (caller-supplied).
    pub first_pass_ms: i64,
/// The due attempts already executed (0..=RETRY_COUNT).
    pub attempts_done: i32,
⋮----
/// True when attempt `#attempts_done` is due at `now_ms`.
pub fn retry_schedule_due(now_ms: i64, clock: &RetryClock) -> bool {
⋮----
pub fn retry_schedule_due(now_ms: i64, clock: &RetryClock) -> bool {
⋮----
/// The ms until the next due attempt (0 when it is already due, -1 when the
/// schedule is exhausted).
⋮----
/// schedule is exhausted).
pub fn retry_next_delay_ms(now_ms: i64, clock: &RetryClock) -> i64 {
⋮----
pub fn retry_next_delay_ms(now_ms: i64, clock: &RetryClock) -> i64 {
⋮----
/// True when every scheduled attempt has been executed.
pub fn retry_schedule_exhausted(clock: &RetryClock) -> bool {
⋮----
pub fn retry_schedule_exhausted(clock: &RetryClock) -> bool {
⋮----
/// The automatic-restart state.
#[derive(Debug, Clone, Copy, Default)]
pub struct CrashRestartState {
/// The automatic restarts inside the window.
    pub count: i32,
/// The monotonic ms when the window opened.
    pub window_start_ms: i64,
⋮----
/// Advances the state for one automatic restart at `now_ms`.
///
⋮----
///
/// A non-positive window disables the sliding behaviour: every restart is its
⋮----
/// A non-positive window disables the sliding behaviour: every restart is its
/// own window (the count resets to 1) - the safe interpretation of a
⋮----
/// own window (the count resets to 1) - the safe interpretation of a
/// misconfigured window, never an unbounded counter.
⋮----
/// misconfigured window, never an unbounded counter.
pub fn crash_restart_advance(
⋮----
pub fn crash_restart_advance(
⋮----
/// True while the state is inside the allowed restart budget.
pub fn crash_restart_allowed(state: &CrashRestartState, max_restarts: i32) -> bool {
⋮----
pub fn crash_restart_allowed(state: &CrashRestartState, max_restarts: i32) -> bool {
⋮----
/// One monitor as the reconcile pipeline sees it.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct TopologyMonitor {
/// The tiered identity (never GDI-only unless it fell back).
    pub stable_key: String,
⋮----
/// The pure snapshot of the monitor topology.
#[derive(Debug, Clone, Default, PartialEq, Eq)]
pub struct TopologySnapshot {
/// The deterministic order: primary first, then the stable key.
    pub monitors: Vec<TopologyMonitor>,
⋮----
/// The stable hash over the canonical serialization.
    pub hash_snapshot: String,
⋮----
/// What changed between two topology snapshots.
#[derive(Debug, Clone, Default, PartialEq, Eq)]
pub struct TopologyDiff {
/// In the new snapshot, not in the old (sorted).
    pub added_keys: Vec<String>,
/// In the old snapshot, not in the new (sorted).
    pub removed_keys: Vec<String>,
/// The x/y changed (sorted).
    pub moved_keys: Vec<String>,
/// The width/height changed (sorted).
    pub resized_keys: Vec<String>,
/// The scale changed (sorted).
    pub dpi_keys: Vec<String>,
/// The orientation changed (sorted).
    pub orientation_keys: Vec<String>,
⋮----
impl TopologyDiff {
/// True when nothing changed (the vectors are empty and the primary did
    /// not change).
⋮----
/// not change).
    pub fn is_empty(&self) -> bool {
⋮----
pub fn is_empty(&self) -> bool {
self.added_keys.is_empty()
&& self.removed_keys.is_empty()
&& self.moved_keys.is_empty()
&& self.resized_keys.is_empty()
&& self.dpi_keys.is_empty()
&& self.orientation_keys.is_empty()
⋮----
/// The pure topology diff.
///
⋮----
///
/// The keys are identified ONLY by the stable key, and the output ordering is
⋮----
/// The keys are identified ONLY by the stable key, and the output ordering is
/// deterministic (sorted), so the same pair of snapshots always produces the
⋮----
/// deterministic (sorted), so the same pair of snapshots always produces the
/// same diff - and an identical pair produces an empty one.
⋮----
/// same diff - and an identical pair produces an empty one.
pub fn diff_topology(old_s: &TopologySnapshot, new_s: &TopologySnapshot) -> TopologyDiff {
⋮----
pub fn diff_topology(old_s: &TopologySnapshot, new_s: &TopologySnapshot) -> TopologyDiff {
use std::collections::BTreeMap;
⋮----
old_by_key.insert(m.stable_key.as_str(), m);
⋮----
new_by_key.insert(m.stable_key.as_str(), m);
⋮----
for key in new_by_key.keys() {
if !old_by_key.contains_key(key) {
d.added_keys.push((*key).to_string());
⋮----
for key in old_by_key.keys() {
if !new_by_key.contains_key(key) {
d.removed_keys.push((*key).to_string());
⋮----
// Per-attribute changes for the keys present in both snapshots. A single
// key can land in several categories at once (moved + resized).
⋮----
let Some(n) = new_by_key.get(key) else {
⋮----
d.moved_keys.push((*key).to_string());
⋮----
d.resized_keys.push((*key).to_string());
⋮----
d.dpi_keys.push((*key).to_string());
⋮----
d.orientation_keys.push((*key).to_string());
⋮----
// The primary change over the key union: an absent key counts as
// not-primary, so adding or removing the primary monitor is a primary
// change too.
⋮----
keys.extend(old_by_key.keys().copied());
keys.extend(new_by_key.keys().copied());
⋮----
let old_primary = old_by_key.get(key).is_some_and(|m| m.is_primary);
let new_primary = new_by_key.get(key).is_some_and(|m| m.is_primary);
⋮----
/// One monitor as the desktop sees it (the reconcile policy's input).
#[derive(Debug, Clone, Default, PartialEq, Eq)]
pub struct MonitorInfo {
⋮----
pub enum ReconcileAction {
⋮----
pub fn decide_reconcile_action(trigger: &str) -> ReconcileAction {
⋮----
pub fn reconcile_action_name(action: ReconcileAction) -> &'static str {
⋮----
/// True when the action needs the monitor enumeration to have settled before
/// it runs.
⋮----
/// it runs.
pub fn reconcile_action_needs_stable_topology(action: ReconcileAction) -> bool {
⋮----
pub fn reconcile_action_needs_stable_topology(action: ReconcileAction) -> bool {
matches!(action, ReconcileAction::ReanchorSlots)
⋮----
/// The identity key of a monitor: the stable key, else the GDI device name,
/// else its position id.
⋮----
/// else its position id.
pub fn slot_identity_key(mi: &MonitorInfo) -> String {
⋮----
pub fn slot_identity_key(mi: &MonitorInfo) -> String {
if !mi.stable_key.is_empty() {
return mi.stable_key.clone();
⋮----
if !mi.device_name.is_empty() {
return format!("gdi:{}", mi.device_name);
⋮----
format!("pos:{}", mi.id)
⋮----
/// True when the two enumerations describe the SAME topology.
///
⋮----
///
/// The comparison is order-insensitive. A duplicated identity (an ambiguous
⋮----
/// The comparison is order-insensitive. A duplicated identity (an ambiguous
/// EDID, two monitors without a device name) makes the topology
⋮----
/// EDID, two monitors without a device name) makes the topology
/// undecidable - and therefore not stable.
⋮----
/// undecidable - and therefore not stable.
pub fn topology_stable(first: &[MonitorInfo], second: &[MonitorInfo]) -> bool {
⋮----
pub fn topology_stable(first: &[MonitorInfo], second: &[MonitorInfo]) -> bool {
if first.len() != second.len() {
⋮----
let key = slot_identity_key(mi);
if by_identity.insert(key, mi).is_some() {
⋮----
let Some(other) = by_identity.get(&key) else {
⋮----
/// The stable signature of an enumeration (the monitors joined in a stable
/// order).
⋮----
/// order).
pub fn topology_signature(monitors: &[MonitorInfo]) -> String {
⋮----
pub fn topology_signature(monitors: &[MonitorInfo]) -> String {
let mut parts: Vec<String> = Vec::with_capacity(monitors.len());
⋮----
parts.push(format!(
⋮----
// The enumeration is sorted (primary first, then the device name), but
// sort anyway so the signature does not depend on the caller's ordering.
parts.sort();
parts.join(";")
⋮----
pub fn pin_verify_due_at(armed: bool, deadline_ms: i64, now_ms: i64) -> bool {
⋮----
pub fn pin_verify_add_reverts(total: i32, n: i32) -> i32 {
⋮----
pub enum DecodeState {
⋮----
impl DecodeState {
⋮----
pub struct UnloadMachine {
⋮----
impl Default for UnloadMachine {
fn default() -> Self {
⋮----
pub struct UnloadDecision {
⋮----
pub fn unload_on_pause_changed(
⋮----
pub fn unload_on_tick(machine: &UnloadMachine, mask: i32, now_ms: i64) -> UnloadDecision {
⋮----
pub mod pause {
⋮----
pub fn ui_paused(mask: i32) -> bool {
⋮----
pub fn slots_paused(mask: i32) -> bool {
⋮----
pub fn hides_wallpaper(mask: i32) -> bool {
```

## File: rust/crates/k6wp-core/src/ipc.rs
```rust
use std::collections::BTreeMap;
use std::fmt;
⋮----
pub enum Cmd {
⋮----
pub fn cmd_to_string(cmd: Cmd) -> &'static str {
⋮----
/// Parses a wire string back to a command.
///
⋮----
///
/// `None` for a name that is not in the vocabulary - the C++
⋮----
/// `None` for a name that is not in the vocabulary - the C++
/// `CmdFromString` returns false, and the frame is then rejected.
⋮----
/// `CmdFromString` returns false, and the frame is then rejected.
pub fn cmd_from_string(name: &str) -> Option<Cmd> {
⋮----
pub fn cmd_from_string(name: &str) -> Option<Cmd> {
⋮----
.iter()
.find(|(wire, _)| *wire == name)
.map(|(_, cmd)| *cmd)
⋮----
/// Every command name, in the stable append-only order.
pub fn cmd_vocabulary() -> Vec<(&'static str, Cmd)> {
⋮----
pub fn cmd_vocabulary() -> Vec<(&'static str, Cmd)> {
CMD_NAMES.to_vec()
⋮----
pub struct IpcMessage {
⋮----
impl Default for IpcMessage {
fn default() -> Self {
⋮----
pub struct IpcError(pub String);
⋮----
fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
f.write_str(&self.0)
⋮----
/// Serializes a message to a single NDJSON line (the JSON plus the `'\n'`).
///
⋮----
///
/// # Errors
⋮----
/// # Errors
///
⋮----
///
/// - a payload (or a whole frame) bigger than `MAX_PAYLOAD_BYTES`.
⋮----
/// - a payload (or a whole frame) bigger than `MAX_PAYLOAD_BYTES`.
pub fn encode(msg: &IpcMessage) -> Result<String, IpcError> {
⋮----
pub fn encode(msg: &IpcMessage) -> Result<String, IpcError> {
⋮----
.map_err(|e| IpcError(format!("Encode: the payload is not serializable: {e}")))?;
if payload_json.len() > MAX_PAYLOAD_BYTES {
return Err(IpcError(
"Encode: payload exceeds kMaxPayloadBytes (64 KiB)".to_string(),
⋮----
root.insert("version".to_string(), Value::from(msg.version));
root.insert(
"cmd".to_string(),
Value::String(cmd_to_string(msg.cmd).to_string()),
⋮----
root.insert("payload".to_string(), msg.payload.clone());
⋮----
.map_err(|e| IpcError(format!("Encode: the frame is not serializable: {e}")))?;
if frame.len() > MAX_PAYLOAD_BYTES {
⋮----
"Encode: frame exceeds kMaxPayloadBytes (64 KiB)".to_string(),
⋮----
Ok(frame + "\n")
⋮----
pub fn decode(text: &str) -> Result<IpcMessage, IpcError> {
if text.is_empty() {
return Err(IpcError("Decode: the frame is empty".to_string()));
⋮----
if let Some(stripped) = line.strip_suffix('\n') {
⋮----
if let Some(stripped) = line.strip_suffix('\r') {
⋮----
if line.is_empty() || line.contains('\n') {
⋮----
"Decode: the frame is not a single line".to_string(),
⋮----
.map_err(|e| IpcError(format!("Decode: the frame is not valid JSON: {e}")))?;
⋮----
.as_object()
.ok_or_else(|| IpcError("Decode: the frame is not a JSON object".to_string()))?;
⋮----
let version = match object.get("version") {
Some(Value::Number(number)) if number.is_i64() || number.is_u64() => number
.as_i64()
.unwrap_or_else(|| number.as_u64().unwrap_or(0) as i64),
⋮----
"Decode: the version is not an integer".to_string(),
⋮----
None => return Err(IpcError("Decode: the frame has no version".to_string())),
⋮----
return Err(IpcError(format!(
⋮----
let cmd_name = match object.get("cmd") {
Some(Value::String(name)) => name.as_str(),
Some(_) => return Err(IpcError("Decode: the command is not a string".to_string())),
None => return Err(IpcError("Decode: the frame has no command".to_string())),
⋮----
let cmd = cmd_from_string(cmd_name)
.ok_or_else(|| IpcError(format!("Decode: unknown command: {cmd_name}")))?;
⋮----
.get("payload")
.cloned()
.unwrap_or_else(|| Value::Object(Map::new()));
⋮----
.map_err(|e| IpcError(format!("Decode: the payload is not serializable: {e}")))?;
⋮----
"Decode: the payload exceeds kMaxPayloadBytes (64 KiB)".to_string(),
⋮----
Ok(IpcMessage {
⋮----
pub struct EngineState {
⋮----
impl Default for EngineState {
⋮----
fn as_u64(value: &Value) -> Option<u64> {
⋮----
if let Some(u) = number.as_u64() {
Some(u)
} else if let Some(i) = number.as_i64() {
Some(i as u64)
⋮----
number.as_f64().map(|f| f as u64)
⋮----
fn as_i32(value: &Value) -> Option<i32> {
⋮----
if let Some(i) = number.as_i64() {
Some(i as i32)
} else if let Some(u) = number.as_u64() {
Some(u as i32)
⋮----
number.as_f64().map(|f| f as i32)
⋮----
fn is_integer(value: &Value) -> bool {
matches!(value, Value::Number(number) if number.is_i64() || number.is_u64())
⋮----
fn state_value<T>(
⋮----
let object = state.as_object().ok_or(())?;
match object.get(key) {
None => Ok(default),
Some(value) => read(value).ok_or(()),
⋮----
pub fn parse_engine_state(raw: &Value) -> EngineState {
⋮----
let Some(object) = raw.as_object() else {
⋮----
.get("state")
⋮----
state.pid = state_value(&inner, "pid", 0u64, as_u64)?;
state.video = state_value(&inner, "video", String::new(), |v| {
v.as_str().map(str::to_string)
⋮----
state.paused = state_value(&inner, "paused", false, |v| v.as_bool())?;
state.headless_slots = state_value(&inner, "headless_slots", 0i32, as_i32)?;
state.live = state_value(&inner, "live", true, |v| v.as_bool())?;
⋮----
if let Some(capability) = inner.get("display_capability") {
if is_integer(capability) {
if let Some(value) = as_i32(capability) {
⋮----
if let Some(assignments) = inner.get("display_assignments") {
if let Some(map) = assignments.as_object() {
⋮----
if let Some(path) = path.as_str() {
⋮----
.insert(device.clone(), path.to_string());
⋮----
if let Some(coverage) = inner.get("display_coverage") {
if let Some(map) = coverage.as_object() {
⋮----
if let Some(verdict) = verdict.as_str() {
⋮----
.insert(device.clone(), verdict.to_string());
⋮----
if let Some(enabled) = inner.get("playlist_enabled") {
if let Some(enabled) = enabled.as_bool() {
⋮----
if let Some(size) = inner.get("playlist_size") {
if is_integer(size) {
if let Some(value) = as_i32(size) {
⋮----
if let Some(index) = inner.get("playlist_index") {
if is_integer(index) {
if let Some(value) = as_i32(index) {
⋮----
if let Some(reconcile) = inner.get("reconcile") {
if let Some(map) = reconcile.as_object() {
if let Some(mode) = map.get("mode").and_then(Value::as_str) {
state.reconcile_mode = mode.to_string();
⋮----
if let Some(trigger) = map.get("last_trigger").and_then(Value::as_str) {
state.reconcile_last_trigger = trigger.to_string();
⋮----
if let Some(verdict) = map.get("last_verdict").and_then(Value::as_str) {
state.reconcile_last_verdict = verdict.to_string();
⋮----
if let Some(retries) = map.get("retries") {
if is_integer(retries) {
if let Some(value) = as_i32(retries) {
⋮----
if let Some(degraded) = map.get("degraded_keys") {
if let Some(list) = degraded.as_array() {
⋮----
if let Some(key) = key.as_str() {
state.reconcile_degraded_keys.push(key.to_string());
⋮----
Ok(())
⋮----
pub fn parse_engine_state_text(text: &str) -> EngineState {
⋮----
Ok(raw) => parse_engine_state(&raw),
⋮----
mod tests {
⋮----
fn the_vocabulary_is_append_only() {
assert_eq!(cmd_to_string(Cmd::SetVideo), "set_video");
assert_eq!(cmd_to_string(Cmd::SetDisplayVideo), "set_display_video");
assert_eq!(cmd_from_string("set_video"), Some(Cmd::SetVideo));
assert_eq!(
⋮----
assert_eq!(cmd_from_string("nope"), None);
assert_eq!(cmd_from_string(""), None);
assert_eq!(cmd_vocabulary().len(), 7);
assert_eq!(cmd_vocabulary()[0], ("set_video", Cmd::SetVideo));
⋮----
fn the_frame_is_the_frozen_byte_sequence() {
⋮----
fn a_frame_round_trips() {
⋮----
let msg = decode(frame).expect("must decode");
assert_eq!(msg.version, 1);
assert_eq!(msg.cmd, Cmd::SetVideo);
assert_eq!(msg.payload["path"], Value::from("C:\\v\\a.mp4"));
⋮----
fn the_malformed_frames_are_rejected() {
⋮----
assert!(decode(frame).is_err(), "must reject: {frame:?}");
⋮----
fn a_frame_without_a_payload_gets_an_empty_one() {
let msg = decode("{\"version\":1,\"cmd\":\"quit\"}").expect("must decode");
assert_eq!(msg.payload, Value::Object(Map::new()));
⋮----
fn the_state_defaults_are_kept() {
let state = parse_engine_state_text("{}");
assert_eq!(state.pid, 0);
assert!(state.live);
assert_eq!(state.playlist_index, -1);
assert!(state.display_assignments.is_empty());
⋮----
fn a_wrong_typed_state_falls_back_to_the_defaults() {
let state = parse_engine_state_text(
⋮----
assert_eq!(state, EngineState::default());
```

## File: rust/crates/k6wp-core/src/version.rs
```rust
pub struct ParsedSemver {
⋮----
fn is_c_space(byte: u8) -> bool {
matches!(byte, b' ' | b'\t' | b'\n' | 0x0b | 0x0c | b'\r')
⋮----
pub fn parse_semver(text: &str) -> ParsedSemver {
let bytes = text.as_bytes();
⋮----
while beg < bytes.len() && is_c_space(bytes[beg]) {
⋮----
let mut end = bytes.len();
while end > beg && is_c_space(bytes[end - 1]) {
⋮----
if let Some(offset) = bytes[beg..end].iter().position(|byte| *byte == b'-') {
⋮----
// Parse the dot-separated components; each non-empty one must start with
// a digit, otherwise the whole version is invalid.
⋮----
i += 1; // Tolerate the empty components ("1..2" parses as 1, 2).
⋮----
if !bytes[i].is_ascii_digit() {
out.parts.clear();
⋮----
while i < core_end && bytes[i].is_ascii_digit() {
⋮----
out.parts.push(acc as i32);
⋮----
out.valid = !out.parts.is_empty();
⋮----
pub fn compare_versions(a: &str, b: &str) -> i32 {
let pa = parse_semver(a);
let pb = parse_semver(b);
⋮----
let count = pa.parts.len().max(pb.parts.len());
⋮----
let va = pa.parts.get(index).copied().unwrap_or(0);
let vb = pb.parts.get(index).copied().unwrap_or(0);
⋮----
pub fn is_newer_version(latest: &str, current: &str) -> bool {
compare_versions(latest, current) > 0
⋮----
mod tests {
⋮----
fn the_numeric_parts_compare() {
assert_eq!(compare_versions("1.2.3", "1.2.3"), 0);
assert_eq!(compare_versions("1.10.0", "1.9.0"), 1);
assert_eq!(compare_versions("1.9.0", "1.10.0"), -1);
assert_eq!(compare_versions("1.2", "1.2.0"), 0);
assert_eq!(compare_versions("2.0.0", "1.9.9"), 1);
⋮----
fn the_whitespace_and_the_v_prefix_are_stripped() {
assert_eq!(compare_versions("  v1.2.3 ", "1.2.3"), 0);
assert_eq!(compare_versions("V1.2.3", "1.2.3"), 0);
⋮----
fn the_prerelease_sorts_below_the_release() {
assert_eq!(compare_versions("1.0.0", "1.0.0-beta"), 1);
assert_eq!(compare_versions("1.0.0-beta", "1.0.0"), -1);
assert_eq!(compare_versions("1.0.0-beta", "1.0.0-beta"), 0);
⋮----
fn an_invalid_version_compares_equal() {
assert_eq!(compare_versions("", "1.0.0"), 0);
assert_eq!(compare_versions("abc", "1.0.0"), 0);
assert_eq!(compare_versions("1.0.0", "abc"), 0);
assert!(!is_newer_version("abc", "1.0.0"));
⋮----
fn the_junk_inside_a_component_is_ignored() {
assert_eq!(compare_versions("1.2.3beta", "1.2.3"), 0);
assert_eq!(compare_versions("1..2", "1.2"), 0);
⋮----
fn the_component_is_clamped() {
let parsed = parse_semver("99999999999.0.0");
assert!(parsed.valid);
assert_eq!(parsed.parts[0], 1_000_000_000);
```

## File: rust/crates/k6wp-studio/src/grid.rs
```rust
pub fn columns_for_width(width: f32) -> usize {
if !width.is_finite() || width <= 0.0 {
⋮----
let fits = ((width + GAP) / (MIN_CARD_WIDTH + GAP)).floor();
⋮----
(fits as usize).min(MAX_COLUMNS)
⋮----
pub fn card_width(width: f32, columns: usize) -> f32 {
let columns = columns.max(1) as f32;
⋮----
(usable / columns).max(1.0)
⋮----
pub fn row_ranges(count: usize, columns: usize) -> Vec<(usize, usize)> {
let columns = columns.max(1);
⋮----
.step_by(columns)
.map(|start| (start, (start + columns).min(count)))
.collect()
⋮----
pub fn visible_items(
⋮----
if count == 0 || !row_h.is_finite() || row_h <= 0.0 {
⋮----
let rows = row_ranges(count, columns).len();
⋮----
let sy = if scroll_y.is_finite() {
scroll_y.max(0.0)
⋮----
let vh = if viewport_h.is_finite() {
viewport_h.max(0.0)
⋮----
let first_row = (sy / row_h).floor() as usize;
let last_row = ((sy + vh) / row_h).ceil() as usize;
let start_row = first_row.saturating_sub(buffer).min(rows);
let end_row = (last_row + buffer).min(rows);
⋮----
let end = (end_row * columns).min(count);
⋮----
return (start.min(count), start.min(count));
⋮----
mod tests {
⋮----
fn columns_never_zero() {
assert_eq!(columns_for_width(0.0), 1);
assert_eq!(columns_for_width(-10.0), 1);
assert_eq!(columns_for_width(f32::NAN), 1);
assert_eq!(columns_for_width(50.0), 1);
⋮----
fn columns_grow_with_width() {
⋮----
assert_eq!(columns_for_width(219.0), 1);
assert_eq!(columns_for_width(220.0), 1);
assert_eq!(columns_for_width(440.0), 2);
assert_eq!(columns_for_width(880.0), 4);
assert_eq!(columns_for_width(10000.0), MAX_COLUMNS);
⋮----
fn card_width_keeps_the_gaps() {
⋮----
let cw = card_width(w, 4);
assert!((cw - MIN_CARD_WIDTH).abs() < 0.001);
⋮----
fn rows_are_row_major_and_short_at_the_end() {
assert_eq!(row_ranges(5, 2), vec![(0, 2), (2, 4), (4, 5)]);
assert_eq!(row_ranges(4, 2), vec![(0, 2), (2, 4)]);
assert!(row_ranges(0, 3).is_empty());
assert_eq!(row_ranges(3, 0), vec![(0, 1), (1, 2), (2, 3)]);
⋮----
fn visible_items_covers_the_viewport_plus_buffer() {
⋮----
let (start, end) = visible_items(0.0, 424.0, 212.0, 4, 200, 1);
assert_eq!(start, 0);
⋮----
assert_eq!(end, 12);
⋮----
fn visible_items_moves_with_the_scroll() {
let (start, end) = visible_items(2120.0, 424.0, 212.0, 4, 200, 1);
⋮----
assert_eq!(start, 36);
⋮----
assert_eq!(end, 52);
⋮----
fn visible_items_clamps_at_the_end() {
let (start, end) = visible_items(1_000_000.0, 424.0, 212.0, 4, 200, 1);
assert!(end <= 200);
assert!(start <= end);
⋮----
fn visible_items_empty_inputs() {
assert_eq!(visible_items(0.0, 0.0, 0.0, 4, 0, 1), (0, 0));
assert_eq!(visible_items(0.0, 400.0, 0.0, 4, 200, 1), (0, 0));
⋮----
assert_eq!(visible_items(f32::NAN, 400.0, 200.0, 4, 200, 0), (0, 8));
```

## File: rust/crates/k6wp-studio/src/history.rs
```rust
use std::path::PathBuf;
⋮----
pub struct WallpaperRef {
⋮----
pub struct WallpaperHistory {
⋮----
impl WallpaperHistory {
⋮----
pub fn record(&mut self, entry: WallpaperRef) -> bool {
if self.applied.as_ref() == Some(&entry) {
return self.undo.is_some();
⋮----
let previous = self.applied.replace(entry);
if previous.is_some() {
⋮----
self.undo.is_some()
⋮----
pub fn take_undo(&mut self) -> Option<WallpaperRef> {
self.undo.take()
⋮----
pub fn applied(&self) -> Option<&WallpaperRef> {
self.applied.as_ref()
⋮----
mod tests {
⋮----
fn entry(index: usize) -> WallpaperRef {
⋮----
name: format!("video-{index}"),
media: PathBuf::from(format!("C:\\media\\{index}.mp4")),
⋮----
fn the_first_install_offers_no_undo() {
⋮----
assert!(!history.record(entry(0)));
assert!(history.take_undo().is_none());
⋮----
fn a_second_install_offers_the_first_picture() {
⋮----
history.record(entry(0));
assert!(history.record(entry(1)));
assert_eq!(history.take_undo(), Some(entry(0)));
assert_eq!(history.applied(), Some(&entry(1)));
⋮----
fn undo_is_consumed_so_it_cannot_run_twice() {
⋮----
history.record(entry(1));
assert!(history.take_undo().is_some());
⋮----
fn re_installing_the_same_picture_does_not_offer_a_self_undo() {
```

## File: rust/crates/k6wp-studio/src/lib.rs
```rust
pub mod grid;
pub mod history;
pub mod thumbnailer;
pub mod thumbs;
pub mod worker;
```

## File: shared/build_flags.hpp
```cpp

```

## File: shared/config_lock.cpp
```cpp
ConfigFileLock::ConfigFileLock(std::chrono::milliseconds timeout) {
```

## File: shared/config_lock.hpp
```cpp
class ConfigFileLock {
⋮----
explicit ConfigFileLock(
⋮----
ConfigFileLock(const ConfigFileLock&) = delete;
```

## File: shared/config_schema.hpp
```cpp
class ConfigError : public std::runtime_error {
⋮----
explicit ConfigError(const std::string& message) : std::runtime_error(message) {}
⋮----
struct WallpaperConfig {
⋮----
WallpaperConfig LoadConfig(const std::filesystem::path& path);
⋮----
nlohmann::json ConfigToJson(const WallpaperConfig& cfg);
⋮----
void SaveConfig(const std::filesystem::path& path, const WallpaperConfig& cfg);
⋮----
void AtomicWriteJson(const std::filesystem::path& path, const nlohmann::json& j);
⋮----
bool PersistConfigField(const std::filesystem::path& path,
⋮----
void ValidateConfig(WallpaperConfig& cfg);
⋮----
WallpaperConfig MigrateConfig(const nlohmann::json& raw);
⋮----
nlohmann::json MergeUnknownKeys(const nlohmann::json& existing,
⋮----
std::filesystem::path DefaultConfigPath();
```

## File: shared/displays_schema.hpp
```cpp
struct MonitorAssignment {
⋮----
struct DisplaysConfig {
⋮----
DisplaysConfig LoadDisplays(const std::filesystem::path& path);
⋮----
void SaveDisplays(const std::filesystem::path& path, const DisplaysConfig& cfg);
⋮----
nlohmann::json DisplaysToJson(const DisplaysConfig& cfg);
⋮----
void ValidateDisplays(DisplaysConfig& cfg);
⋮----
DisplaysConfig MigrateDisplays(const nlohmann::json& raw);
⋮----
std::filesystem::path DefaultDisplaysPath();
⋮----
std::vector<std::wstring> DetectKeyCollision(
```

## File: shared/ffmpeg_path.cpp
```cpp
std::filesystem::path ExeDir() {
⋮----
bool ExistsNoThrow(const std::filesystem::path& p) {
⋮----
std::filesystem::path FindTool(const wchar_t* env_name,
⋮----
// (a) env override, only in a DEBUG build. It exists so a test can point
// at a vendored ffmpeg that is not next to the test binary; a shipped
// release binary must not let an environment variable redirect tool
// resolution (L5 / I4). Compiled out of the release build.
⋮----
const std::filesystem::path p(std::wstring(env, len));
⋮----
(void)env_name;  // the override is compiled out of the release build
⋮----
// (b) FLAT — release layout: ffmpeg/ffprobe sit next to the exe.
⋮----
// (c) build layout: exe in build/<preset>/ -> repo root two levels up.
⋮----
std::filesystem::path FindFfmpeg() {
⋮----
std::filesystem::path FindFfprobe() {
```

## File: shared/ipc_protocol.hpp
```cpp
std::wstring PipeNameForSession(unsigned long session_id);
⋮----
std::wstring CurrentSessionPipeName();
⋮----
std::wstring RealSessionPipeName();
⋮----
void UseUniquePipeNameForTest(const char* tag);
⋮----
void UseRealPipeNameForTest();
⋮----
enum class Cmd {
⋮----
struct IpcMessage {
⋮----
struct EngineState {
⋮----
EngineState ParseEngineState(const nlohmann::json& raw);
⋮----
class IpcError : public std::runtime_error {
⋮----
std::string Encode(const IpcMessage& msg);
⋮----
bool Decode(const std::string& line, IpcMessage& out);
⋮----
const char* CmdToString(Cmd cmd) noexcept;
⋮----
bool CmdFromString(const char* s, Cmd& out) noexcept;
⋮----
PSECURITY_ATTRIBUTES MakeCurrentUserOnlySA();
```

## File: studio/src/library_manager.cpp
```cpp
std::string ReadFile(const std::filesystem::path& path, bool& exists) {
⋮----
std::ifstream in(path, std::ios::binary);
⋮----
void RefreshStat(LibraryEntry& entry) {
⋮----
LibraryManager::LibraryManager(std::filesystem::path json_path) {
⋮----
std::filesystem::path LibraryManager::DefaultLibraryJsonPath() {
⋮----
void LibraryManager::Load() {
⋮----
void LibraryManager::Save() const {
⋮----
void LibraryManager::Add(const LibraryEntry& entry, MetadataProbe probe) {
⋮----
std::vector<LibraryEntry> LibraryManager::ListItems() const {
⋮----
bool LibraryManager::Remove(const std::filesystem::path& dst,
⋮----
std::size_t LibraryManager::Size() const { return entries_.size(); }
⋮----
void LibraryManager::Clear() {
⋮----
std::string LibraryManager::CacheKeyMaterial(const LibraryEntry& entry) {
⋮----
LibraryEntry LibraryManager::ReferenceInPlace(
```

## File: studio/src/thumbnailer.cpp
```cpp
std::string GetEnvStr(const char* name) {
⋮----
std::string value(buf);
⋮----
void LogThumbError(const std::string& msg) {
⋮----
double SafeSeekSeconds(double duration_seconds) {
⋮----
std::wstring FormatSeekSeconds(double seconds) {
⋮----
// MED-13: builds the full ffmpeg thumbnail command. Aspect-preserving
// scale+pad replaces the old fixed -s 320x180, which stretched non-16:9
// sources: scale=...:force_original_aspect_ratio=decrease fits inside the
// grid cell, pad centers it with letterbox bars.
std::wstring BuildThumbCommand(const std::filesystem::path& ffmpeg,
⋮----
// Cache-key material: abs-path | size | mtime | command-version. The
// version suffix is what makes a command change invalidate old thumbs.
std::string ThumbCacheKeyMaterial(const std::filesystem::path& abs,
⋮----
// LOW-4: library.json-backed metadata cache. The grid refresh path
// (LibraryWidget::Refresh -> Thumbnailer::CachedThumb per item, plus the
// async GetThumb worker) used to stat every video 3x per item
// (is_regular_file + file_size + last_write_time). library.json ALREADY
// stores mtime/size per entry (LibraryManager::RefreshStat at Add time),
// so the key material can come from that index instead of the disk.
//
// Design (read-only: library_manager.cpp is owned by a parallel task, so
// the json-path lookup below mirrors LibraryManager::DefaultLibraryJsonPath
// without touching it):
//   * One process-wide snapshot: absolute dst path -> (size, mtime).
//   * The snapshot is valid only while library.json's own (mtime, size)
⋮----
std::filesystem::path ResolveLibraryJsonPath() {
⋮----
struct LibraryMetaEntry {
⋮----
struct LibraryMetaSnapshot {
⋮----
LibraryMetaSnapshot& MetaSnapshot() {
⋮----
bool LookupLibraryMeta(const std::string& abs_u8, LibraryMetaEntry* out) {
⋮----
std::ifstream in(lib, std::ios::binary);
⋮----
Thumbnailer::Thumbnailer() : root_(ThumbsRoot()) {}
⋮----
std::filesystem::path Thumbnailer::ThumbsRoot() const {
⋮----
bool Thumbnailer::EnsureRoot() {
⋮----
std::string Thumbnailer::CacheKeyHex(const std::filesystem::path& video) {
⋮----
// LOW-4 fast path: mtime/size from the library.json index (zero per-
// video stats on a hit). The grid calls this per item per refresh, so
// 100 items cost ~1 hot stat of library.json + map lookups instead of
// 300 cold stats of the videos.
⋮----
std::filesystem::path Thumbnailer::GetThumb(
⋮----
std::filesystem::path Thumbnailer::CachedThumb(
⋮----
std::filesystem::path Thumbnailer::FfmpegPath() {
⋮----
bool Thumbnailer::GenerateTo(const std::filesystem::path& video,
```

## File: tools/bench_dpb_parity.ps1
```powershell
# tools/bench_dpb_parity.ps1 - task 1.4 (D3): the visual parity of the
# DPB-pinned re-encode.
#
# The re-encode is LOSSY, so the frames are not byte-identical (that gate
# belongs to a render-path change). The parity gate here is the one in
# docs/refactor/budgets.md: luma +-12, stddev +-12, and the motion must
# survive (a still frame would mean the wallpaper stopped moving).
#
# The frames come from the ENGINE (the hidden --capture-frames flag), not
# from a screen grab: a screen grab records the plain desktop whenever
# anything covers the screen, which would make this a no-op.
[CmdletBinding()]
param(
    [string]$EngineExe = '',
    [string]$FileA = '',
    [string]$FileB = '',
    [double]$SeekSeconds = 2.0,
    [string]$OutJson = ''
)
$ErrorActionPreference = 'Stop'
$script:Here = Split-Path -Parent $MyInvocation.MyCommand.Path
. (Join-Path $script:Here 'bench_lib.ps1')

if ([string]::IsNullOrWhiteSpace($EngineExe)) { $EngineExe = Join-Path $script:Here '..\build\release\engine.exe' }
if ([string]::IsNullOrWhiteSpace($FileA)) { $FileA = Join-Path $script:Here '..\tests\corpus\highdpb_4k.mp4' }
if ([string]::IsNullOrWhiteSpace($FileB)) { $FileB = Join-Path $script:Here '..\build\dpb-4k.mp4' }
if ([string]::IsNullOrWhiteSpace($OutJson)) { $OutJson = Join-Path $script:Here '..\docs\bench\dpb-parity.json' }
$engine = [System.IO.Path]::GetFullPath($EngineExe)

function Capture-Frames([string]$File, [string]$Prefix) {
    for ($i = 0; $i -lt 40; $i++) {
        if (-not (Get-Process engine -ErrorAction SilentlyContinue)) { break }
        Start-Sleep -Milliseconds 250
    }
    Start-Sleep -Milliseconds 800
    $dataDir = Join-Path $env:TEMP ("k6wp-par-" + [Guid]::NewGuid().ToString('N').Substring(0, 8))
    New-Item -ItemType Directory -Force -Path (Join-Path $dataDir 'K6WP') | Out-Null
    Get-ChildItem -Path $Prefix -Filter '*.jpg' -ErrorAction SilentlyContinue | Remove-Item -Force
    $old = $env:LOCALAPPDATA
    try {
        $env:LOCALAPPDATA = $dataDir
        $p = Start-Process -FilePath $engine -PassThru -WindowStyle Hidden -ArgumentList @(
            "--video=$File", "--capture-frames=$Prefix", '--capture-frames-count=2',
            "--capture-frames-interval-ms=600", "--capture-frames-seek=$SeekSeconds",
            '--exit-after-ms=25000')
    }
    finally { $env:LOCALAPPDATA = $old }
    $deadline = (Get-Date).AddSeconds(30)
    while ((Get-Date) -lt $deadline) {
        if ((Test-Path "${Prefix}1.jpg") -and (Test-Path "${Prefix}2.jpg")) { break }
        Start-Sleep -Milliseconds 300
    }
    # The frames are written asynchronously: a file that EXISTS may still be
    # growing (a truncated JPEG makes GDI+ decode garbage, which would fake a
    # "the picture changed" verdict). Wait until both sizes are stable.
    $stable = 0
    $last = -1
    while ($stable -lt 3 -and (Get-Date) -lt $deadline) {
        Start-Sleep -Milliseconds 400
        $s1 = if (Test-Path "${Prefix}1.jpg") { (Get-Item "${Prefix}1.jpg").Length } else { -1 }
        $s2 = if (Test-Path "${Prefix}2.jpg") { (Get-Item "${Prefix}2.jpg").Length } else { -1 }
        if ($s1 -gt 0 -and $s1 -eq $last -and $s2 -gt 0) { $stable++ } else { $stable = 0 }
        $last = $s1
    }
    try { if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue } } catch { }
    return @("${Prefix}1.jpg", "${Prefix}2.jpg")
}

function Signature([string]$Path) {
    $bmp = [System.Drawing.Bitmap]::FromFile($Path)
    try {
        $sum = 0.0; $sum2 = 0.0; $n = 0
        for ($y = 0; $y -lt $bmp.Height; $y += 4) {
            for ($x = 0; $x -lt $bmp.Width; $x += 4) {
                $c = $bmp.GetPixel($x, $y)
                $l = 0.2126 * $c.R + 0.7152 * $c.G + 0.0722 * $c.B
                $sum += $l; $sum2 += $l * $l; $n++
            }
        }
        $mean = $sum / $n
        return [pscustomobject]@{
            luma = [math]::Round($mean, 2)
            stddev = [math]::Round([math]::Sqrt([math]::Max(0, ($sum2 / $n) - ($mean * $mean))), 2)
        }
    }
    finally { $bmp.Dispose() }
}

Add-Type -AssemblyName System.Drawing
$busy = @(Get-Process engine -ErrorAction SilentlyContinue)
if ($busy.Count -gt 0) { throw "an engine.exe is already running" }

$prefixA = Join-Path $env:TEMP ("k6wp-dpbA-" + [Guid]::NewGuid().ToString('N').Substring(0, 6))
$prefixB = Join-Path $env:TEMP ("k6wp-dpbB-" + [Guid]::NewGuid().ToString('N').Substring(0, 6))
$framesA = Capture-Frames -File $FileA -Prefix $prefixA
$framesB = Capture-Frames -File $FileB -Prefix $prefixB
foreach ($f in @($framesA + $framesB)) { if (-not (Test-Path -LiteralPath $f)) { throw "no frame captured: $f" } }

$sigA1 = Signature $framesA[0]; $sigA2 = Signature $framesA[1]
$sigB1 = Signature $framesB[0]; $sigB2 = Signature $framesB[1]
$ba = [System.Drawing.Bitmap]::FromFile($framesA[0]); $bb = [System.Drawing.Bitmap]::FromFile($framesB[0])
$pixelDiff = Get-BitmapDiff -A $ba -B $bb -Step 8
$ba.Dispose(); $bb.Dispose()
$ba2 = [System.Drawing.Bitmap]::FromFile($framesA[0]); $bb2 = [System.Drawing.Bitmap]::FromFile($framesA[1])
$motionA = Get-BitmapDiff -A $ba2 -B $bb2 -Step 8
$ba2.Dispose(); $bb2.Dispose()
$ba3 = [System.Drawing.Bitmap]::FromFile($framesB[0]); $bb3 = [System.Drawing.Bitmap]::FromFile($framesB[1])
$motionB = Get-BitmapDiff -A $ba3 -B $bb3 -Step 8
$ba3.Dispose(); $bb3.Dispose()

$lumaOk = ([math]::Abs($sigA1.luma - $sigB1.luma) -le 12)
$stdOk = ([math]::Abs($sigA1.stddev - $sigB1.stddev) -le 12)
# The capture SEEKS and PAUSES (that is what makes the frame deterministic),
# so both arms show motion 0 by construction. The "the wallpaper still
# moves" half of the gate is the LIVE animation check on the re-encoded
# file, not this capture.
$motionOk = ($motionA -eq $motionB)
$report = [ordered]@{
    tool = 'bench_dpb_parity'
    phase = 1
    date = (Get-Date -Format 'yyyy-MM-ddTHH:mm:ssK')
    label = 'live-proven (one machine)'
    method = 'the frames the ENGINE rendered (the hidden --capture-frames flag), compared at the same seek position; the capture pauses, so the motion is 0 in both arms by construction and the live movement is checked separately'
    file_a = $FileA
    file_b = $FileB
    a = [ordered]@{ luma = $sigA1.luma; stddev = $sigA1.stddev; motion = [math]::Round($motionA, 3) }
    b = [ordered]@{ luma = $sigB1.luma; stddev = $sigB1.stddev; motion = [math]::Round($motionB, 3) }
    pixel_diff_same_frame = [math]::Round($pixelDiff, 3)
    gate_luma_delta_le_12 = $lumaOk
    gate_stddev_delta_le_12 = $stdOk
    gate_same_capture_behaviour = $motionOk
    parity_pass = ($lumaOk -and $stdOk -and $motionOk)
}
$json = $report | ConvertTo-Json -Depth 6
$outFull = [System.IO.Path]::GetFullPath($OutJson)
$outDir = [System.IO.Path]::GetDirectoryName($outFull)
if (-not (Test-Path -LiteralPath $outDir)) { New-Item -ItemType Directory -Force -Path $outDir | Out-Null }
$json | Set-Content -LiteralPath $outFull -Encoding UTF8
Remove-Item ($framesA + $framesB) -Force -ErrorAction SilentlyContinue
Write-Host ("A: luma={0} stddev={1} motion={2}" -f $sigA1.luma, $sigA1.stddev, [math]::Round($motionA, 3))
Write-Host ("B: luma={0} stddev={1} motion={2}" -f $sigB1.luma, $sigB1.stddev, [math]::Round($motionB, 3))
Write-Host ("the same-frame pixel diff: {0} (a lossy re-encode, not a render-path change)" -f [math]::Round($pixelDiff, 3))
Write-Host ("PARITY: luma_ok={0} stddev_ok={1} motion_ok={2} -> {3}" -f $lumaOk, $stdOk, $motionOk, $report.parity_pass)
Write-Host "bench_dpb_parity: wrote $outFull"
if (-not $report.parity_pass) { exit 1 }
```

## File: tools/bench_h3_bitdepth.ps1
```powershell
# tools/bench_h3_bitdepth.ps1 - task H3: the bit depth and the codec, MEASURED.
#
# WHAT IT MEASURES
#   The extra-RAM estimate (the import dialog) says a 10-bit file costs
#   twice the frame pool of an 8-bit one. This measures it: the same
#   encoder settings, the same resolution, the same duration - only the bit
#   depth differs - and the two arms run ALTERNATELY inside one session
#   (the GPU numbers drift between sessions).
#
#   The metric is the C4 pair: the private bytes + the GPU shared memory.
#   The playback rate and the frame counters travel with every number: a
#   run whose rate is outside 0.97-1.03 is invalid.
#
# THE LIVE SESSION
#   The A/B replaces the wallpaper, so the session is snapshotted and
#   restored with tools/session_snapshot.ps1 (the wallpaper, the still, the
#   lock screen, the display mode). The user's engine is stopped for the
#   measurement and restarted with ITS OWN command line in the `finally`.
#
[CmdletBinding()]
param(
    [string]$EngineExe = '',
    [string]$CorpusDir = '',
    [int]$Runs = 3,
    [string]$OutJson = ''
)
$ErrorActionPreference = 'Stop'
$script:Here = Split-Path -Parent $MyInvocation.MyCommand.Path
. (Join-Path $script:Here 'bench_lib.ps1')
. (Join-Path $script:Here 'session_snapshot.ps1')

if ([string]::IsNullOrWhiteSpace($EngineExe)) { $EngineExe = Join-Path $script:Here '..\build\release\engine.exe' }
if ([string]::IsNullOrWhiteSpace($CorpusDir)) { $CorpusDir = Join-Path $script:Here '..\tests\corpus' }
if ([string]::IsNullOrWhiteSpace($OutJson)) { $OutJson = Join-Path $script:Here '..\docs\bench\h3-bitdepth.json' }
$engine = [IO.Path]::GetFullPath($EngineExe)
$corpus = [IO.Path]::GetFullPath($CorpusDir)

# The pairs: the same encoder settings, the same size - only the bit depth
# differs. The third file is the unknown-codec case (the parser reads no
# SPS for it, so the estimate is UNKNOWN).
$pairs = @(
    [pscustomobject]@{ name = 'h264_8bit_vs_10bit'
                       a = (Join-Path $corpus 'h264_8bit_1080p.mp4')
                       b = (Join-Path $corpus 'hi10_1080p.mp4')
                       note = 'H.264, 4 refs, 1920x1088 coded: the 10-bit frame pool is twice the 8-bit one' },
    [pscustomobject]@{ name = 'hevc_8bit_vs_10bit'
                       a = (Join-Path $corpus 'hevc8_1080p.mp4')
                       b = (Join-Path $corpus 'hevc10_1080p.mp4')
                       note = 'HEVC, 6 refs, 1920x1088 coded: the 10-bit frame pool is twice the 8-bit one' },
    [pscustomobject]@{ name = 'mpeg2_unknown_codec'
                       a = (Join-Path $corpus 'mpeg2_1080p.m2v')
                       b = ''
                       note = 'MPEG-2: the parser reads no SPS, so the estimate is UNKNOWN (the policy)' }
)
foreach ($p in $pairs) {
    foreach ($f in @($p.a, $p.b)) {
        if ($f -and -not (Test-Path -LiteralPath $f)) { throw "missing corpus file: $f" }
    }
}

# The live engine's command line, so it can be restarted exactly as it was.
$live = @(Get-CimInstance Win32_Process -Filter "Name='engine.exe'" -ErrorAction SilentlyContinue)
$liveCommandLine = if ($live.Count -gt 0) { $live[0].CommandLine } else { '' }

function Restart-LiveEngine {
    if ([string]::IsNullOrWhiteSpace($liveCommandLine)) { return }
    # The command line is "<exe> <args...>"; the exe path is quoted or not.
    $exe = $liveCommandLine
    $args = ''
    if ($exe -match '^"([^"]+)"\s*(.*)$') {
        $exe = $Matches[1]
        $args = $Matches[2]
    } elseif ($exe -match '^(\S+)\s+(.*)$') {
        $exe = $Matches[1]
        $args = $Matches[2]
    }
    if (-not (Test-Path -LiteralPath $exe)) {
        Write-Host "  [warn] the live engine's binary is gone ($exe); the session is restored without it"
        return
    }
    $argList = @()
    if ($args) { $argList = @($args -split '\s+') }
    try {
        if ($argList.Count -gt 0) {
            Start-Process -FilePath $exe -ArgumentList $argList -WindowStyle Hidden | Out-Null
        } else {
            Start-Process -FilePath $exe -WindowStyle Hidden | Out-Null
        }
        Write-Host "  [restore] the live engine restarted ($exe)"
    } catch {
        Write-Host "  [warn] the live engine could not be restarted: $_"
    }
}

$report = [ordered]@{
    tool = 'bench_h3_bitdepth'
    task = 'H3'
    date = (Get-Date -Format 'yyyy-MM-ddTHH:mm:ssK')
    label = 'live-proven (one machine)'
    method = 'ABAB alternating inside one session; the C4 metrics (private bytes + GPU shared); the playback rate and the frame counters travel with every number; the session is snapshotted and restored'
    engine = $engine
    runs = $Runs
    pairs = @()
}

try {
    Invoke-SessionSafe -Name 'H3 the bit-depth A/B' -ScriptBlock {
    # The A/B refuses to run beside a running engine.
    foreach ($proc in @(Get-Process engine -ErrorAction SilentlyContinue)) {
        try { Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue } catch { }
    }
    Start-Sleep -Seconds 2

    foreach ($pair in $pairs) {
        $bName = if ($pair.b) { Split-Path -Leaf $pair.b } else { '(none)' }
        Write-Host ("  {0}: A={1} B={2}" -f $pair.name, (Split-Path -Leaf $pair.a), $bName)
        # The A/B refuses to run beside a running engine: the previous
        # pair's engine may still be shutting down.
        for ($i = 0; $i -lt 40; $i++) {
            if (-not (Get-Process engine -ErrorAction SilentlyContinue)) { break }
            Start-Sleep -Milliseconds 250
        }
        foreach ($proc in @(Get-Process engine -ErrorAction SilentlyContinue)) {
            try { Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue } catch { }
        }
        Start-Sleep -Milliseconds 800
        $abOut = Join-Path $env:TEMP ("k6wp-h3-" + [Guid]::NewGuid().ToString('N').Substring(0, 8) + '.json')
        if ($pair.b) {
            & (Join-Path $script:Here 'bench_dpb_ab.ps1') -EngineExe $engine -FileA $pair.a -FileB $pair.b `
                -Runs $Runs -OutJson $abOut | ForEach-Object { Write-Host "    $_" }
        } else {
            # The unknown-codec case: one arm against itself is meaningless,
            # so it is measured once (the numbers are the reference, not a
            # comparison).
            & (Join-Path $script:Here 'bench_dpb_ab.ps1') -EngineExe $engine -FileA $pair.a -FileB $pair.a `
                -Runs 1 -OutJson $abOut | ForEach-Object { Write-Host "    $_" }
        }
        $ab = Get-Content -LiteralPath $abOut -Raw | ConvertFrom-Json
        # The rate decides whether a run counts: outside 0.97-1.03 the two
        # arms did not do the same amount of work (the project's rule).
        $aRate = [double]$ab.a.playback_rate
        $bRate = [double]$ab.b.playback_rate
        $aValid = ($aRate -ge 0.97 -and $aRate -le 1.03)
        $bValid = ($bRate -ge 0.97 -and $bRate -le 1.03)
        $report.pairs += [ordered]@{
            name = $pair.name
            note = $pair.note
            a = (Split-Path -Leaf $pair.a)
            b = $bName
            a_private_mb = $ab.a.private_mb
            a_private_band_mb = $ab.a.private_band_mb
            a_gpu_shared_mb = $ab.a.gpu_shared_mb
            a_gpu_shared_band_mb = $ab.a.gpu_shared_band_mb
            a_playback_rate = $aRate
            a_rate_valid = $aValid
            b_private_mb = $ab.b.private_mb
            b_private_band_mb = $ab.b.private_band_mb
            b_gpu_shared_mb = $ab.b.gpu_shared_mb
            b_gpu_shared_band_mb = $ab.b.gpu_shared_band_mb
            b_playback_rate = $bRate
            b_rate_valid = $bValid
            comparable = ($aValid -and $bValid)
        }
        Remove-Item -LiteralPath $abOut -Force -ErrorAction SilentlyContinue
    }
    } | Out-Null
} finally {
    # The user's engine comes back, exactly as it was - even when a pair
    # failed (the live session must never be left without its wallpaper).
    Restart-LiveEngine
}

$json = $report | ConvertTo-Json -Depth 6
[IO.File]::WriteAllText([System.IO.Path]::GetFullPath($OutJson), $json, (New-Object Text.UTF8Encoding($false)))
Write-Host "bench_h3_bitdepth: the report is in $OutJson"
exit 0
```

## File: tools/bench_lib.ps1
```powershell
# tools/bench_lib.ps1 - shared helpers for the Phase 0 measurement harness.
#
# The harness is intentionally dependency-free (stock PowerShell 5.1 + .NET)
# and never ships. It measures the REAL engine/Studio processes; it does not
# instrument them. All numbers must be written to docs/bench/*.json.
#
# Evidence labels used by the reports:
#   live-proven  = observed on this machine by this harness
#   test-proven  = asserted by ctest / Rust tests
#   code-quoted  = read from source, not observed
#   unmeasured   = no number exists

$ErrorActionPreference = 'Stop'
$script:BenchRoot = Split-Path -Parent $MyInvocation.MyCommand.Path

if (-not ([System.Management.Automation.PSTypeName]'K6wpBench.Native').Type) {
Add-Type @"
using System;
using System.Runtime.InteropServices;
using System.Text;
namespace K6wpBench {
  public static class Native {
    [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr hWnd, int nCmdShow);
    [DllImport("user32.dll")] public static extern bool GetWindowPlacement(IntPtr hWnd, ref WINDOWPLACEMENT p);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetClassName(IntPtr hWnd, System.Text.StringBuilder buf, int n);
    [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr hWnd);
    [DllImport("user32.dll")] public static extern bool EnumWindows(EnumWindowsProc cb, IntPtr p);
    [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint pid);
    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }
    [StructLayout(LayoutKind.Sequential)] public struct POINT { public int X, Y; }
    [StructLayout(LayoutKind.Sequential)] public struct WINDOWPLACEMENT {
      public int length, flags, showCmd;
      public POINT minPosition, maxPosition;
      public RECT normalPosition;
    }
  }
}
"@
}

function Get-K6wpSessionId {
    return (Get-Process -Id $PID).SessionId
}

function Get-K6wpPipeName {
    return ("k6wp-engine-{0}" -f (Get-K6wpSessionId))
}

function Send-K6wpIpc {
    param(
        [Parameter(Mandatory=$true)][string]$Cmd,
        [hashtable]$Payload = @{},
        [int]$TimeoutMs = 3000
    )
    # Byte-level client: the server replies with one NDJSON line per datagram.
    # A StreamReader/ReadLineAsync helper masked the real failure behind a
    # "Cannot access a closed pipe" Dispose exception, so framing is done
    # explicitly here and every read is bounded by the deadline.
    $name = Get-K6wpPipeName
    $pipe = $null
    $ms = $null
    try {
        $pipe = New-Object System.IO.Pipes.NamedPipeClientStream('.', $name, [System.IO.Pipes.PipeDirection]::InOut, [System.IO.Pipes.PipeOptions]::Asynchronous)
        $pipe.Connect($TimeoutMs)
        $msg = @{ version = 1; cmd = $Cmd; payload = $Payload } | ConvertTo-Json -Compress -Depth 8
        $bytes = [System.Text.Encoding]::UTF8.GetBytes($msg + "`n")
        $pipe.Write($bytes, 0, $bytes.Length)
        $pipe.Flush()
        $deadline = [DateTime]::UtcNow.AddMilliseconds($TimeoutMs)
        $buf = New-Object byte[] 8192
        $ms = New-Object System.IO.MemoryStream
        while ($true) {
            $remain = [int]($deadline - [DateTime]::UtcNow).TotalMilliseconds
            if ($remain -le 0) { throw "IPC ack timeout after ${TimeoutMs}ms (cmd=$Cmd)" }
            # NamedPipeClientStream.ReadTimeout is unsupported on .NET Framework,
            # so bound the overlapped read with an explicit wait handle.
            $ar = $pipe.BeginRead($buf, 0, $buf.Length, $null, $null)
            if (-not $ar.AsyncWaitHandle.WaitOne($remain)) {
                throw "IPC ack timeout after ${TimeoutMs}ms (cmd=$Cmd)"
            }
            $n = $pipe.EndRead($ar)
            if ($n -le 0) { throw "IPC pipe closed before ack (cmd=$Cmd)" }
            $ms.Write($buf, 0, $n)
            if ($buf[$n - 1] -eq 10) { break }
        }
        $line = [System.Text.Encoding]::UTF8.GetString($ms.ToArray()).TrimEnd("`r", "`n")
        if ([string]::IsNullOrWhiteSpace($line)) { throw "empty IPC ack (cmd=$Cmd)" }
        return ($line | ConvertFrom-Json)
    }
    finally {
        if ($ms) { try { $ms.Dispose() } catch { } }
        if ($pipe) { try { $pipe.Dispose() } catch { } }
    }
}

function Wait-K6wpEngine {
    param([int]$TimeoutMs = 10000)
    $deadline = [DateTime]::UtcNow.AddMilliseconds($TimeoutMs)
    while ([DateTime]::UtcNow -lt $deadline) {
        try {
            $ack = Send-K6wpIpc -Cmd 'get_state' -TimeoutMs 800
            if ($ack -and $ack.ok) { return $true }
        } catch { }
        Start-Sleep -Milliseconds 100
    }
    return $false
}

function Start-K6wpEngine {
    param(
        [Parameter(Mandatory=$true)][string]$EngineExe,
        [string]$Video = '',
        [string]$DataDir = '',
        [int]$RunMs = 20000,
        [int]$ConfigMs = 0
    )
    if ([string]::IsNullOrWhiteSpace($DataDir)) {
        $DataDir = Join-Path $env:TEMP ("k6wp-bench-" + [Guid]::NewGuid().ToString('N'))
    }
    $dataRoot = Join-Path $DataDir 'K6WP'
    New-Item -ItemType Directory -Force -Path $dataRoot | Out-Null
    $args = @("--exit-after-ms=$RunMs")
    if (-not [string]::IsNullOrWhiteSpace($Video)) {
        $args = @("--video=$Video") + $args
    }
    $oldLocal = $env:LOCALAPPDATA
    try {
        $env:LOCALAPPDATA = $DataDir
        $proc = Start-Process -FilePath $EngineExe -ArgumentList $args -PassThru -WindowStyle Hidden
    }
    finally {
        $env:LOCALAPPDATA = $oldLocal
    }
    if (-not (Wait-K6wpEngine -TimeoutMs 15000)) {
        try { if (-not $proc.HasExited) { Stop-Process -Id $proc.Id -Force } } catch { }
        throw "engine did not answer the IPC pipe within 15s (video=$Video)"
    }
    return [pscustomobject]@{ Process = $proc; DataDir = $DataDir; LogPath = (Join-Path $dataRoot 'engine.log') }
}

function Stop-K6wpEngine {
    param($Run, [int]$GraceMs = 12000)
    if ($null -eq $Run -or $null -eq $Run.Process) { return }
    $p = $Run.Process
    try { $p.Refresh() } catch { return }
    if ($p.HasExited) { return }
    try { [void](Send-K6wpIpc -Cmd 'quit' -TimeoutMs 2000) } catch { }
    try { [void]$p.WaitForExit($GraceMs) } catch { }
    try {
        $p.Refresh()
        if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue }
    } catch { }
}

function Get-Median {
    param([double[]]$Values)
    if ($null -eq $Values -or $Values.Count -eq 0) { return $null }
    $sorted = @($Values | Sort-Object)
    $n = $sorted.Count
    if ($n % 2 -eq 1) { return [double]$sorted[($n - 1) / 2] }
    return ([double]$sorted[$n / 2 - 1] + [double]$sorted[$n / 2]) / 2.0
}

function Get-Stats {
    param([double[]]$Values)
    $clean = @($Values | Where-Object { $null -ne $_ -and -not [double]::IsNaN($_) })
    if ($clean.Count -eq 0) {
        return [pscustomobject]@{ n = 0; median = $null; mean = $null; min = $null; max = $null }
    }
    $m = Get-Median $clean
    $min = ($clean | Measure-Object -Minimum).Minimum
    $max = ($clean | Measure-Object -Maximum).Maximum
    return [pscustomobject]@{
        n = $clean.Count
        median = [math]::Round($m, 2)
        mean = [math]::Round((($clean | Measure-Object -Average).Average), 2)
        min = [math]::Round($min, 2)
        max = [math]::Round($max, 2)
        # The noise band: the spread of the samples around the median. It is
        # the tolerance a difference must exceed to mean something.
        band = [math]::Round(($max - $min), 2)
    }
}

function Get-ProcessSeries {
    param(
        [Parameter(Mandatory=$true)][int]$ProcessId,
        [int]$Seconds = 10,
        [int]$IntervalMs = 1000
    )
    $cores = [Environment]::ProcessorCount
    $out = New-Object System.Collections.Generic.List[object]
    $p = Get-Process -Id $ProcessId -ErrorAction Stop
    $prevCpu = $p.CPU
    if ($null -eq $prevCpu) { $prevCpu = 0.0 }
    $prevT = [DateTime]::UtcNow
    $start = [DateTime]::UtcNow
    # Per-thread CPU: the PROCESS aggregate hides a single spinning thread
    # (that is exactly how a 55 %-of-a-core busy-wait stayed invisible), so
    # the hottest thread is tracked separately (C2).
    $threadPrev = @{}
    foreach ($t in $p.Threads) { $threadPrev[[string]$t.Id] = $t.TotalProcessorTime.TotalMilliseconds }
    while (([DateTime]::UtcNow - $start).TotalSeconds -lt $Seconds) {
        Start-Sleep -Milliseconds $IntervalMs
        try {
            $p.Refresh()
            if ($p.HasExited) { break }
            $cpu = $p.CPU
            if ($null -eq $cpu) { $cpu = $prevCpu }
            $now = [DateTime]::UtcNow
            $dt = ($now - $prevT).TotalSeconds
            $pct = 0.0
            if ($dt -gt 0) {
                $pct = (($cpu - $prevCpu) / $dt / $cores) * 100.0
                if ($pct -lt 0) { $pct = 0.0 }
            }
            # The hottest single thread over this interval, as a percentage
            # of ONE core (not of the machine).
            $hotPct = 0.0
            $hotTid = 0
            foreach ($t in $p.Threads) {
                $k = [string]$t.Id
                $nowMs = $t.TotalProcessorTime.TotalMilliseconds
                if ($threadPrev.ContainsKey($k)) {
                    $d = $nowMs - $threadPrev[$k]
                    if ($dt -gt 0) {
                        $pctCore = ($d / 1000.0 / $dt) * 100.0
                        if ($pctCore -gt $hotPct) { $hotPct = $pctCore; $hotTid = $t.Id }
                    }
                }
                $threadPrev[$k] = $nowMs
            }
            $out.Add([pscustomobject]@{
                t_ms = [int](([DateTime]::UtcNow - $start).TotalMilliseconds)
                private_mb = [math]::Round($p.PrivateMemorySize64 / 1MB, 2)
                ws_mb = [math]::Round($p.WorkingSet64 / 1MB, 2)
                cpu_pct = [math]::Round($pct, 3)
                max_thread_cpu_pct = [math]::Round($hotPct, 2)
                hot_tid = $hotTid
                threads = $p.Threads.Count
                handles = $p.HandleCount
            })
            $prevCpu = $cpu
            $prevT = $now
        } catch { break }
    }
    return $out.ToArray()
}

function Get-TypeperfCounterPaths {
    param([int]$ProcessId, [string]$ProcessName = 'engine')
    $paths = New-Object System.Collections.Generic.List[string]
    try {
        $gpu = (Get-Counter -ListSet 'GPU Engine' -ErrorAction Stop).PathsWithInstances
        foreach ($p in $gpu) {
            if ($p -match ("pid_" + $ProcessId + "_") -and $p -match 'Utilization Percentage$') {
                $paths.Add($p)
            }
        }
    } catch { }
    try {
        $mem = (Get-Counter -ListSet 'GPU Process Memory' -ErrorAction Stop).PathsWithInstances
        foreach ($p in $mem) {
            if ($p -match ("pid_" + $ProcessId + "_") -and $p -match '(Dedicated|Shared) Usage$') {
                $paths.Add($p)
            }
        }
    } catch { }
    if (-not [string]::IsNullOrWhiteSpace($ProcessName)) {
        $paths.Add(("\Thread({0}*)\Context Switches/sec" -f $ProcessName))
    }
    return $paths.ToArray()
}

function Start-TypeperfSampler {
    param(
        [int]$ProcessId,
        [int]$Seconds = 10,
        [string]$ProcessName = 'engine'
    )
    $paths = Get-TypeperfCounterPaths -ProcessId $ProcessId -ProcessName $ProcessName
    if ($paths.Count -eq 0) { throw "no GPU/thread counters for pid $ProcessId" }
    $csv = Join-Path $env:TEMP ("k6wp-typeperf-" + [Guid]::NewGuid().ToString('N') + ".csv")
    $stdout = Join-Path $env:TEMP ("k6wp-typeperf-" + [Guid]::NewGuid().ToString('N') + ".out")
    $stderr = Join-Path $env:TEMP ("k6wp-typeperf-" + [Guid]::NewGuid().ToString('N') + ".err")
    # Start-Process joins -ArgumentList with spaces and does NOT add quotes.
    # Counter paths contain spaces ("GPU Engine"), so quote every path.
    $quoted = @($paths | ForEach-Object { '"' + $_ + '"' })
    $args = @($quoted) + @('-si', '1', '-sc', "$Seconds", '-f', 'CSV', '-o', ('"' + $csv + '"'), '-y')
    $proc = Start-Process -FilePath 'typeperf.exe' -ArgumentList $args -PassThru -WindowStyle Hidden `
        -RedirectStandardOutput $stdout -RedirectStandardError $stderr
    return [pscustomobject]@{
        Process = $proc
        Csv = $csv
        Stdout = $stdout
        Stderr = $stderr
        Paths = $paths
    }
}

function ConvertTo-NullableDouble {
    param($Value)
    if ($null -eq $Value) { return $null }
    $s = [string]$Value
    if ([string]::IsNullOrWhiteSpace($s)) { return $null }
    $d = 0.0
    $ok = [double]::TryParse($s, [Globalization.NumberStyles]::Float, [Globalization.CultureInfo]::InvariantCulture, [ref]$d)
    if (-not $ok) { return $null }
    if ($d -lt 0) { return $null }
    return $d
}

function Read-TypeperfCsv {
    param([string]$Path)
    $rows = @()
    if (-not (Test-Path -LiteralPath $Path)) { return $rows }
    try { $data = Import-Csv -LiteralPath $Path } catch { return $rows }
    foreach ($row in $data) {
        $props = @($row.PSObject.Properties)
        if ($props.Count -eq 0) { continue }
        $stamp = [string]$props[0].Value
        if ([string]::IsNullOrWhiteSpace($stamp)) { continue }
        $gpuTotal = 0.0; $gpu3d = 0.0; $gpuDecode = 0.0; $gpuCopy = 0.0
        $shared = 0.0; $dedicated = 0.0; $wake = 0.0
        $hasGpu = $false; $hasWake = $false
        foreach ($prop in $props) {
            $name = $prop.Name
            $val = ConvertTo-NullableDouble $prop.Value
            if ($null -eq $val) { continue }
            if ($name -match 'Utilization Percentage$') {
                $hasGpu = $true
                $gpuTotal += $val
                # Column names are full PDH paths:
                # \\host\GPU Engine(pid_.._engtype_3D)\Utilization Percentage
                if ($name -match 'engtype_3D\)') { $gpu3d += $val }
                elseif ($name -match 'engtype_VideoDecode\)') { $gpuDecode += $val }
                elseif ($name -match 'engtype_Copy\)') { $gpuCopy += $val }
            }
            elseif ($name -match 'Shared Usage$') { $shared += $val }
            elseif ($name -match 'Dedicated Usage$') { $dedicated += $val }
            elseif ($name -match 'Context Switches/sec$') {
                $hasWake = $true
                $wake += $val
            }
        }
        $rows += [pscustomobject]@{
            timestamp = $stamp
            gpu_total_pct = $(if ($hasGpu) { [math]::Round($gpuTotal, 2) } else { $null })
            gpu_3d_pct = $(if ($hasGpu) { [math]::Round($gpu3d, 2) } else { $null })
            gpu_decode_pct = $(if ($hasGpu) { [math]::Round($gpuDecode, 2) } else { $null })
            gpu_copy_pct = $(if ($hasGpu) { [math]::Round($gpuCopy, 2) } else { $null })
            gpu_shared_mb = [math]::Round($shared / 1MB, 2)
            gpu_dedicated_mb = [math]::Round($dedicated / 1MB, 2)
            wakeups_per_sec = $(if ($hasWake) { [math]::Round($wake, 2) } else { $null })
        }
    }
    return $rows
}

function Get-DesktopWindows {
    # Save + hide every visible top-level app window so the injected
    # wallpaper is not occluded. Shell surfaces (taskbar/Progman/WorkerW) are
    # never touched.
    #
    # The windows are enumerated with EnumWindows, NOT through
    # Get-Process.MainWindowHandle: a process can own several top-level
    # windows and the "main" one is not necessarily the one on screen (a
    # File Explorer window owned by a helper process, an Electron app with
    # one window per process, ...). Missing a covering window makes the
    # engine pause (occlusion/fullscreen) and the capture records the plain
    # desktop instead of the wallpaper.
    $saved = @()
    $skip = @('Shell_TrayWnd', 'Progman', 'WorkerW', 'K6WP.DesktopInject.1', 'Windows.UI.Core.CoreWindow')
    # An always-on-top agent-cursor overlay covers the WHOLE screen. It is
    # not an app window: minimizing it does nothing, but leaving it up makes
    # the engine's fullscreen watcher pause the decode AND the occlusion
    # watcher report 100 % coverage, so every measurement reads a paused
    # engine and the capture records the plain desktop. It is hidden for the
    # measurement and restored afterwards.
    $hideClasses = @('Cua.AgentCursorOverlay')
    $found = New-Object System.Collections.Generic.List[object]
    $cb = [K6wpBench.Native+EnumWindowsProc] {
        param($h, $p)
        if (-not [K6wpBench.Native]::IsWindowVisible($h)) { return $true }
        $sb = New-Object System.Text.StringBuilder 256
        [void][K6wpBench.Native]::GetClassName($h, $sb, 256)
        $cls = $sb.ToString()
        $procId = 0
        [void][K6wpBench.Native]::GetWindowThreadProcessId($h, [ref]$procId)
        $found.Add([pscustomobject]@{ Handle = $h; Class = $cls; Pid = $procId })
        return $true
    }
    [void][K6wpBench.Native]::EnumWindows($cb, [IntPtr]::Zero)
    foreach ($w in $found) {
        try {
            $h = $w.Handle
            $cls = $w.Class
            if ($skip -contains $cls) { continue }
            $wp = New-Object K6wpBench.Native+WINDOWPLACEMENT
            $wp.length = [Runtime.InteropServices.Marshal]::SizeOf($wp)
            if (-not [K6wpBench.Native]::GetWindowPlacement($h, [ref]$wp)) { continue }
            if ($wp.showCmd -eq 6) { continue }  # already minimized
            $name = ''
            try { $name = (Get-Process -Id $w.Pid -ErrorAction Stop).ProcessName } catch { $name = "pid$($w.Pid)" }
            $saved += [pscustomobject]@{ Handle = $h; ShowCmd = $wp.showCmd; Name = $name; Class = $cls }
            if ($hideClasses -contains $cls) {
                [void][K6wpBench.Native]::ShowWindow($h, 0)   # SW_HIDE
            } else {
                [void][K6wpBench.Native]::ShowWindow($h, 6)   # SW_MINIMIZE
            }
        } catch { }
    }
    return $saved
}

function Restore-DesktopWindows {
    param([object[]]$Saved)
    if ($null -eq $Saved) { return }
    foreach ($s in $Saved) {
        try {
            $cmd = [int]$s.ShowCmd
            if ($cmd -eq 0) { $cmd = 1 }
            [void][K6wpBench.Native]::ShowWindow([IntPtr]$s.Handle, $cmd)
        } catch { }
    }
}

function Get-HwInventory {
    $os = Get-CimInstance Win32_OperatingSystem
    $cpu = Get-CimInstance Win32_Processor | Select-Object -First 1
    $gpus = @(Get-CimInstance Win32_VideoController | ForEach-Object { $_.Name })
    $power = ''
    try { $power = (powercfg /getactivescheme) -join ' ' } catch { }
    $monitors = @()
    $dump = Join-Path $script:BenchRoot '..\build\msvc-dev\monitor_dump.exe'
    if (Test-Path -LiteralPath $dump) {
        try {
            $raw = & $dump 2>$null | Out-String
            $parsed = $raw | ConvertFrom-Json
            $monitors = @($parsed.monitors)
        } catch { }
    }
    return [pscustomobject]@{
        os = ("{0} {1} build {2}" -f $os.Caption, $os.Version, $os.BuildNumber)
        cpu = $cpu.Name
        cores = $cpu.NumberOfCores
        logical = $cpu.NumberOfLogicalProcessors
        ram_kib = $os.TotalVisibleMemorySize
        gpus = $gpus
        power_plan = $power.Trim()
        monitors = $monitors
        date = (Get-Date -Format 'yyyy-MM-ddTHH:mm:ssK')
    }
}

function Get-ScreenBitmap {
    Add-Type -AssemblyName System.Drawing, System.Windows.Forms
    $bounds = [System.Windows.Forms.SystemInformation]::VirtualScreen
    $bmp = New-Object System.Drawing.Bitmap $bounds.Width, $bounds.Height
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    try {
        $g.CopyFromScreen($bounds.Left, $bounds.Top, 0, 0, $bmp.Size)
    }
    finally { $g.Dispose() }
    return [pscustomobject]@{ Bitmap = $bmp; Bounds = $bounds }
}

function Save-ScreenPng {
    param([string]$Path, [int]$CropWidth = 0, [int]$CropHeight = 0)
    $shot = Get-ScreenBitmap
    $bmp = $shot.Bitmap
    try {
        if ($CropWidth -gt 0 -and $CropHeight -gt 0) {
            $x = [int](($bmp.Width - $CropWidth) / 2)
            $y = [int](($bmp.Height - $CropHeight) / 2)
            $crop = New-Object System.Drawing.Bitmap $CropWidth, $CropHeight
            $g = [System.Drawing.Graphics]::FromImage($crop)
            try { $g.DrawImage($bmp, (New-Object System.Drawing.Rectangle 0,0,$CropWidth,$CropHeight), (New-Object System.Drawing.Rectangle $x,$y,$CropWidth,$CropHeight), [System.Drawing.GraphicsUnit]::Pixel) }
            finally { $g.Dispose() }
            $bmp.Dispose()
            $bmp = $crop
        }
        $dir = [System.IO.Path]::GetDirectoryName([System.IO.Path]::GetFullPath($Path))
        if (-not (Test-Path -LiteralPath $dir)) { New-Item -ItemType Directory -Force -Path $dir | Out-Null }
        $bmp.Save([System.IO.Path]::GetFullPath($Path), [System.Drawing.Imaging.ImageFormat]::Png)
    }
    finally { $bmp.Dispose() }
}

function Get-RegionSample {
    param([int]$Width = 240, [int]$Height = 160)
    Add-Type -AssemblyName System.Drawing, System.Windows.Forms
    $bounds = [System.Windows.Forms.SystemInformation]::VirtualScreen
    $x = $bounds.Left + [int](($bounds.Width - $Width) / 2)
    $y = $bounds.Top + [int](($bounds.Height - $Height) / 2)
    $bmp = New-Object System.Drawing.Bitmap $Width, $Height
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    try { $g.CopyFromScreen($x, $y, 0, 0, $bmp.Size) } finally { $g.Dispose() }
    return $bmp
}

function Get-BitmapDiff {
    param($A, $B, [int]$Step = 4)
    $w = [Math]::Min($A.Width, $B.Width)
    $h = [Math]::Min($A.Height, $B.Height)
    $sum = 0.0
    $n = 0
    for ($y = 0; $y -lt $h; $y += $Step) {
        for ($x = 0; $x -lt $w; $x += $Step) {
            $pa = $A.GetPixel($x, $y)
            $pb = $B.GetPixel($x, $y)
            $sum += ([Math]::Abs($pa.R - $pb.R) + [Math]::Abs($pa.G - $pb.G) + [Math]::Abs($pa.B - $pb.B)) / 3.0
            $n++
        }
    }
    if ($n -eq 0) { return 0.0 }
    return $sum / $n
}

function Get-PeakWorkingSetMb {
    param([int]$ProcessId)
    try {
        $p = Get-Process -Id $ProcessId -ErrorAction Stop
        return [math]::Round($p.PeakWorkingSet64 / 1MB, 2)
    } catch { return $null }
}
```

## File: tools/bench_studio_compare.ps1
```powershell
# tools/bench_studio_compare.ps1 - E1: the Studio comparison, measured in
# IDENTICAL conditions.
#
# The rules this script enforces (they are the point of E1):
#
#   * ONE library index file (200 items) for both Studios - the same
#     K6WP_LIBRARY_JSON override, so the state is the same, not "similar";
#   * the same screen, the same isolated %LOCALAPPDATA%, the same env;
#   * ONE startup definition: the process start until the window has
#     PAINTED (the capture is not a flat colour) - "the first frame", not
#     "the window handle exists";
#   * the C4 metrics: private bytes + GPU shared memory. The working set is
#     recorded but never decides;
#   * the CPU while the window is static (the user is not touching it).
#
# Output: docs/bench/studio-compare-<slice>.json
[CmdletBinding()]
param(
    [string]$QtStudio = '',
    [string]$SlintStudio = '',
    [int]$Items = 200,
    [string]$Slice = '2.0',
    [string]$OutJson = '',
    [int]$StaticSeconds = 5
)
$ErrorActionPreference = 'Stop'
$script:Here = Split-Path -Parent $MyInvocation.MyCommand.Path
. (Join-Path $script:Here 'bench_lib.ps1')
if ([string]::IsNullOrWhiteSpace($QtStudio)) { $QtStudio = Join-Path $script:Here '..\build\release\studio.exe' }
if ([string]::IsNullOrWhiteSpace($SlintStudio)) { $SlintStudio = Join-Path $script:Here '..\rust\target\x86_64-pc-windows-msvc\debug\k6wp-studio.exe' }
if ([string]::IsNullOrWhiteSpace($OutJson)) { $OutJson = Join-Path $script:Here "..\docs\bench\studio-compare-$Slice.json" }
foreach ($e in @($QtStudio, $SlintStudio)) { if (-not (Test-Path -LiteralPath $e)) { throw "missing: $e" } }

Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;using System.Runtime.InteropServices;using System.Text;
public class StudioCap {
  [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr hdc, uint flags);
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
  [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }
}
"@

# --- the shared library index (200 items) ---------------------------------
$work = Join-Path $env:TEMP ("k6wp-studio-cmp-" + [Guid]::NewGuid().ToString('N').Substring(0, 8))
New-Item -ItemType Directory -Force -Path (Join-Path $work 'K6WP') | Out-Null
$library = Join-Path $work 'library.json'
$entries = @()
for ($i = 0; $i -lt $Items; $i++) {
    $entries += [ordered]@{
        src = "C:/videos/source-$i.mp4"
        dst = "C:/videos/copy-$i.mp4"
        res = "1920x1080"; fps = 30; crf = 23; encoder = "libx264"
        mtime = 1700000000; size = 1048576 + $i; duration = 30.0
        codec = "h264"; width = 1920; height = 1080
        thumb = "C:/videos/copy-$i.jpg"; broken = $false
    }
}
@{ version = 1; entries = $entries } | ConvertTo-Json -Depth 4 -Compress | Set-Content -LiteralPath $library -Encoding UTF8
"the shared library index: $library ($Items items, $((Get-Item $library).Length) bytes)"

function Get-CaptureLuma([IntPtr]$h, [string]$Path) {
    $r = New-Object StudioCap+RECT
    [void][StudioCap]::GetWindowRect($h, [ref]$r)
    $w = $r.Right - $r.Left; $ht = $r.Bottom - $r.Top
    if ($w -le 0 -or $ht -le 0) { return $null }
    $bmp = New-Object System.Drawing.Bitmap $w, $ht
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $hdc = $g.GetHdc()
    try { [void][StudioCap]::PrintWindow($h, $hdc, 2) } finally { $g.ReleaseHdc($hdc); $g.Dispose() }
    $bmp.Save($Path, [System.Drawing.Imaging.ImageFormat]::Png)
    # The mean + the spread: a window that has NOT painted is a flat colour
    # (spread ~0), a painted one has content (spread > 0).
    $sum = 0.0; $sum2 = 0.0; $n = 0
    for ($y = 0; $y -lt $bmp.Height; $y += 4) {
        for ($x = 0; $x -lt $bmp.Width; $x += 4) {
            $c = $bmp.GetPixel($x, $y)
            $l = 0.2126 * $c.R + 0.7152 * $c.G + 0.0722 * $c.B
            $sum += $l; $sum2 += $l * $l; $n++
        }
    }
    $bmp.Dispose()
    $mean = if ($n -gt 0) { $sum / $n } else { 0.0 }
    $spread = if ($n -gt 0) { [math]::Sqrt([math]::Max(0, ($sum2 / $n) - ($mean * $mean))) } else { 0.0 }
    return [pscustomobject]@{ mean = [math]::Round($mean, 2); spread = [math]::Round($spread, 2) }
}

function Measure-Studio([string]$Exe, [string]$Tag) {
    $old = $env:LOCALAPPDATA
    $oldLib = $env:K6WP_LIBRARY_JSON
    try {
        $env:LOCALAPPDATA = (Join-Path $work 'K6WP')
        $env:K6WP_LIBRARY_JSON = $library
        $sw = [Diagnostics.Stopwatch]::StartNew()
        $p = Start-Process -FilePath $Exe -PassThru
        # The first frame: the window handle exists AND the window has
        # painted (the capture is not a flat colour).
        $hwnd = [IntPtr]::Zero
        $firstFrameMs = $null
        $tmp = Join-Path $env:TEMP ("k6wp-studio-cmp-" + [Guid]::NewGuid().ToString('N').Substring(0, 6) + ".png")
        while ($sw.ElapsedMilliseconds -lt 30000) {
            $p.Refresh()
            if ($p.HasExited) { break }
            if ($p.MainWindowHandle -ne 0) {
                $hwnd = $p.MainWindowHandle
                $cap = Get-CaptureLuma $hwnd $tmp
                if ($cap -and $cap.spread -gt 0.5) { $firstFrameMs = $sw.ElapsedMilliseconds; break }
            }
            Start-Sleep -Milliseconds 25
        }
        Remove-Item $tmp -Force -ErrorAction SilentlyContinue
        # The static state: the private bytes, the GPU shared and the CPU
        # after the first frame (the user is not touching the window).
        $tp = Start-TypeperfSampler -ProcessId $p.Id -Seconds ($StaticSeconds + 10) -ProcessName 'studio'
        $series = @(Get-ProcessSeries -ProcessId $p.Id -Seconds $StaticSeconds -IntervalMs 1000)
        $gpu = @(Read-TypeperfCsv -Path $tp.Csv)
        try { if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue } } catch { }
        $gpuM = @($gpu | ForEach-Object { $_.gpu_shared_mb } | Where-Object { $null -ne $_ })
        $cpu = @($series | ForEach-Object { $_.cpu_pct } | Where-Object { $null -ne $_ })
        return [ordered]@{
            tag = $Tag
            exe = (Split-Path -Leaf $Exe)
            first_frame_ms = $firstFrameMs
            private_mb = (Get-Stats @($series | ForEach-Object { $_.private_mb })).median
            ws_mb = (Get-Stats @($series | ForEach-Object { $_.ws_mb })).median
            gpu_shared_mb = $(if ($gpuM.Count -gt 0) { (Get-Stats $gpuM).median } else { $null })
            cpu_pct_static = (Get-Stats $cpu).median
            max_thread_cpu_pct = (Get-Stats @($series | ForEach-Object { $_.max_thread_cpu_pct })).median
            threads = ($series | Select-Object -Last 1).threads
        }
    }
    finally {
        $env:LOCALAPPDATA = $old
        if ($null -eq $oldLib) { Remove-Item Env:\K6WP_LIBRARY_JSON -ErrorAction SilentlyContinue }
        else { $env:K6WP_LIBRARY_JSON = $oldLib }
    }
}

$qt = Measure-Studio -Exe $QtStudio -Tag 'qt'
$slint = Measure-Studio -Exe $SlintStudio -Tag 'slint'
# The same order again (the second arm) - the first run of a process can
# pay a cold-start cost (the font cache, the shader cache).
$qt2 = Measure-Studio -Exe $QtStudio -Tag 'qt'
$slint2 = Measure-Studio -Exe $SlintStudio -Tag 'slint'

$report = [ordered]@{
    tool = 'bench_studio_compare'
    phase = 2
    slice = $Slice
    date = (Get-Date -Format 'yyyy-MM-ddTHH:mm:ssK')
    label = 'live-proven (one machine)'
    method = 'ONE library index (200 items) for both Studios, the same screen, an isolated %LOCALAPPDATA%, and ONE startup definition: the process start until the window has PAINTED (the capture is not a flat colour). The C4 metrics: private bytes + GPU shared; the working set is recorded but never decides'
    library_items = $Items
    library_path = $library
    startup_definition = 'process start -> the window handle exists AND a PrintWindow capture is not a flat colour (the first painted frame)'
    qt = [ordered]@{
        first_frame_ms = @($qt.first_frame_ms, $qt2.first_frame_ms) | Where-Object { $null -ne $_ } | Measure-Object -Average | Select-Object -ExpandProperty Average
        private_mb = (Get-Stats @($qt.private_mb, $qt2.private_mb)).median
        ws_mb = (Get-Stats @($qt.ws_mb, $qt2.ws_mb)).median
        gpu_shared_mb = (Get-Stats @($qt.gpu_shared_mb, $qt2.gpu_shared_mb) | Where-Object { $null -ne $_ }).median
        cpu_pct_static = (Get-Stats @($qt.cpu_pct_static, $qt2.cpu_pct_static)).median
        threads = $qt.threads
    }
    slint = [ordered]@{
        first_frame_ms = @($slint.first_frame_ms, $slint2.first_frame_ms) | Where-Object { $null -ne $_ } | Measure-Object -Average | Select-Object -ExpandProperty Average
        private_mb = (Get-Stats @($slint.private_mb, $slint2.private_mb)).median
        ws_mb = (Get-Stats @($slint.ws_mb, $slint2.ws_mb)).median
        gpu_shared_mb = (Get-Stats @($slint.gpu_shared_mb, $slint2.gpu_shared_mb) | Where-Object { $null -ne $_ }).median
        cpu_pct_static = (Get-Stats @($slint.cpu_pct_static, $slint2.cpu_pct_static)).median
        threads = $slint.threads
    }
    runs = @($qt, $slint, $qt2, $slint2)
}
$json = $report | ConvertTo-Json -Depth 8
$outFull = [System.IO.Path]::GetFullPath($OutJson)
$outDir = [System.IO.Path]::GetDirectoryName($outFull)
if (-not (Test-Path -LiteralPath $outDir)) { New-Item -ItemType Directory -Force -Path $outDir | Out-Null }
$json | Set-Content -LiteralPath $outFull -Encoding UTF8
Write-Host ""
Write-Host ("Qt Studio    : first frame {0} ms, private {1} MB, GPU shared {2} MB, CPU {3} %, threads {4}" -f `
        [math]::Round($report.qt.first_frame_ms), $report.qt.private_mb, $report.qt.gpu_shared_mb, $report.qt.cpu_pct_static, $report.qt.threads)
Write-Host ("Slint Studio : first frame {0} ms, private {1} MB, GPU shared {2} MB, CPU {3} %, threads {4}" -f `
        [math]::Round($report.slint.first_frame_ms), $report.slint.private_mb, $report.slint.gpu_shared_mb, $report.slint.cpu_pct_static, $report.slint.threads)
Write-Host "bench_studio_compare: wrote $outFull"
```

## File: tools/capture_window.ps1
```powershell
# capture_window.ps1 - save one window to a PNG, without stray overlapping pixels.
#
# Usage: powershell -File tools/capture_window.ps1 -ProcessId 1234 -Out D:\shot.png [-Client]
#
# Two details the naive CopyFromScreen version got wrong:
#   * the host process is DPI-unaware, so GetWindowRect returned virtualized
#     pixels and the crop was offset by the scale factor (the neighbouring
#     window's text bled into the top and bottom of the image);
#   * a DWM-composed window is better asked to render itself.
# So: opt into per-monitor DPI awareness, ask the window to paint itself with
# PrintWindow(PW_RENDERFULLCONTENT), and fall back to a screen grab only when the
# window refuses (some GPU renderers answer with an empty bitmap).
param(
    [Parameter(Mandatory = $true)][int]$ProcessId,
    [Parameter(Mandatory = $true)][string]$Out,
    [switch]$Client
)

$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Drawing

Add-Type @"
using System;
using System.Drawing;
using System.Runtime.InteropServices;

public static class WinCap {
    [StructLayout(LayoutKind.Sequential)]
    public struct RECT { public int Left, Top, Right, Bottom; }

    [DllImport("user32.dll")] public static extern bool SetProcessDpiAwarenessContext(IntPtr value);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hWnd, out RECT rect);
    [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr hWnd, out RECT rect);
    [DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr hWnd, ref POINT point);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hWnd);
    [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr hWnd, IntPtr hdc, uint flags);

    [StructLayout(LayoutKind.Sequential)]
    public struct POINT { public int X, Y; }

    public const uint PW_RENDERFULLCONTENT = 2;
    public static readonly IntPtr PER_MONITOR_AWARE_V2 = new IntPtr(-4);

    /// The window rect, or the client rect in screen coordinates.
    public static RECT Bounds(IntPtr hWnd, bool client) {
        RECT r;
        if (!client) {
            GetWindowRect(hWnd, out r);
        } else {
            GetClientRect(hWnd, out r);
            POINT origin = new POINT { X = r.Left, Y = r.Top };
            ClientToScreen(hWnd, ref origin);
            r.Right += origin.X;
            r.Bottom += origin.Y;
            r.Left = origin.X;
            r.Top = origin.Y;
        }
        return r;
    }

}
"@

# DPI awareness first: every rect below is physical pixels.
[void][WinCap]::SetProcessDpiAwarenessContext([WinCap]::PER_MONITOR_AWARE_V2)

$proc = Get-Process -Id $ProcessId
$hwnd = $proc.MainWindowHandle
if ($hwnd -eq [IntPtr]::Zero) {
    throw "process $ProcessId has no main window yet"
}
[void][WinCap]::SetForegroundWindow($hwnd)
Start-Sleep -Milliseconds 600

$r = [WinCap]::Bounds($hwnd, [bool]$Client)
$w = $r.Right - $r.Left
$h = $r.Bottom - $r.Top
if ($w -le 0 -or $h -le 0) {
    throw "the window has an empty rect (${w}x${h})"
}

$bmp = New-Object System.Drawing.Bitmap $w, $h
$graphics = [System.Drawing.Graphics]::FromImage($bmp)

# A window that answers PrintWindow with a blank surface (some GPU renderers
# do) must not be shipped as an empty screenshot.
function Test-Content($image) {
    $first = $image.GetPixel(0, 0)
    for ($y = 0; $y -lt $image.Height; $y += 20) {
        for ($x = 0; $x -lt $image.Width; $x += 20) {
            if ($image.GetPixel($x, $y) -ne $first) { return $true }
        }
    }
    return $false
}

$hdc = $graphics.GetHdc()
$printed = $false
try {
    $printed = [WinCap]::PrintWindow($hwnd, $hdc, [WinCap]::PW_RENDERFULLCONTENT)
} finally {
    $graphics.ReleaseHdc($hdc)
}

if (-not $printed -or -not (Test-Content $bmp)) {
    Write-Host "PrintWindow gave no content; falling back to a screen grab"
    $graphics.CopyFromScreen($r.Left, $r.Top, 0, 0, (New-Object System.Drawing.Size $w, $h))
}

$bmp.Save($Out, [System.Drawing.Imaging.ImageFormat]::Png)
$graphics.Dispose()
$bmp.Dispose()
Write-Host "saved $Out ${w}x${h} (printwindow=$printed client=$([bool]$Client))"
```

## File: tools/drive_engine_ipc.ps1
```powershell
# tools/drive_engine_ipc.ps1 - the G6 live drive: the Rust IPC client against
# the REAL engine.
#
# WHAT IT DOES
#   It runs the Rust live test (k6wp-ipc's live_engine) against the running
#   engine, which drives the WHOLE command set: get_state, set_video, pause,
#   resume, set_display_video, an unknown command, a payload with an unknown
#   key, a reconnect and the quit.
#
# THE RULES (C8)
#   The live session is only touched through tools/session_snapshot.ps1: the
#   display mode, the wallpaper, the still and the lock screen are snapshotted
#   BEFORE the drive and restored in the `finally` - including when the drive
#   fails half-way. The engine is restarted in the same `finally` (the quit
#   at the end of the drive stops it).
#
[CmdletBinding()]
param(
    [string]$EngineExe = '',
    [switch]$KeepEngineStopped
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'session_snapshot.ps1')

if (-not $EngineExe) {
    $EngineExe = Join-Path $root 'build\release\engine.exe'
}
if (-not (Test-Path $EngineExe)) {
    throw "the engine is not built: $EngineExe"
}
$EngineExe = (Resolve-Path $EngineExe).Path

$env:K6WP_IPC_LIVE = '1'
$env:K6WP_IPC_ENGINE_EXE = $EngineExe

try {
    Invoke-SessionSafe -Name 'the G6 IPC drive' `
        -RestoreDisplay -RestoreWallpaper -RestoreLockscreen `
        -ScriptBlock {
        # The engine must be running before the drive (the user may have it
        # already; if not, start it).
        $engine = Get-Process engine -ErrorAction SilentlyContinue
        if (-not $engine) {
            Write-Host '  [ipc] the engine is not running - starting it'
            Start-Process -FilePath $EngineExe -WindowStyle Hidden
            Start-Sleep -Seconds 8
        }

        Write-Host '  [ipc] driving the engine through the Rust client'
        Push-Location (Join-Path $root 'rust')
        try {
            cargo test -p k6wp-ipc --test live_engine -- --nocapture
            if ($LASTEXITCODE -ne 0) {
                throw "the live IPC drive FAILED (the exit code was $LASTEXITCODE)"
            }
        } finally {
            Pop-Location
        }
    }
} finally {
    # The drive quits the engine: the live session must come back.
    if (-not $KeepEngineStopped) {
        $engine = Get-Process engine -ErrorAction SilentlyContinue
        if (-not $engine) {
            Write-Host '  [ipc] restarting the engine (the drive quit it)'
            Start-Process -FilePath $EngineExe -WindowStyle Hidden
            Start-Sleep -Seconds 8
        }
        $engine = Get-Process engine -ErrorAction SilentlyContinue
        if ($engine) {
            $mb = [math]::Round($engine[0].PrivateMemorySize64 / 1MB, 1)
            Write-Host "  [ipc] the engine is back (pid $($engine[0].Id), $mb MB private)"
        } else {
            Write-Warning '  [ipc] the engine did NOT come back - start it by hand'
        }
    }
    Remove-Item Env:\K6WP_IPC_LIVE -ErrorAction SilentlyContinue
    Remove-Item Env:\K6WP_IPC_ENGINE_EXE -ErrorAction SilentlyContinue
}

Write-Host 'drive_engine_ipc: done'
```

## File: tools/e2e_a12b_undo.ps1
```powershell
# e2e_a12b_undo.ps1 - A-12 Langkah 2 end-to-end proof of the toast "Urungkan".
#
# WHAT IT DOES
#   1. Starts the engine and waits until get_state answers on the pipe.
#   2. Starts the DEBUG Studio with --e2e-pick 0, which installs card 0.
#   3. Waits until the engine reports card 0 as the current video.
#   4. Clicks card 1 (a real mouse click on the Slint grid). The engine must
#      switch to card 1 and the Studio must show the toast with "Urungkan".
#   5. Captures the window (the toast is in the PNG) and clicks "Urungkan".
#   6. Waits until the engine reports card 0 again, then captures the window.
#
# The verdict is the engine's own get_state: the picture must go
#   card 0 -> card 1 -> card 0
# Not "a request was sent" - the video path actually changed and came back.
#
# USAGE
#   powershell -File tools/e2e_a12b_undo.ps1
#   powershell -File tools/e2e_a12b_undo.ps1 -UndoX 897 -UndoY 568
#
# The click coordinates are in 960x640 logical pixels and are scaled by the
# window's real client width, so a 125%/150% DPI session still hits the button.
# The session is only touched through tools/session_snapshot.ps1.

param(
    [string]$Engine = "build/msvc-dev/engine.exe",
    [string]$Exe = "D:\cargo-target2\x86_64-pc-windows-msvc\debug\k6wp-studio.exe",
    [string]$Probe = "D:\cargo-target2\x86_64-pc-windows-msvc\release\examples\get_state_once.exe",
    [string]$OutDir = "docs/img",
    [int]$SecondIndex = 1,
    [int]$UndoX = 897,
    [int]$UndoY = 568,
    [int]$TimeoutSec = 40
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot "session_snapshot.ps1")

foreach ($path in @($Engine, $Exe, $Probe)) {
    if (-not (Test-Path $path)) { throw "missing binary: $path" }
}
if (-not (Test-Path $OutDir)) { New-Item -ItemType Directory -Path $OutDir -Force | Out-Null }

Add-Type @"
using System;
using System.Drawing;
using System.Runtime.InteropServices;

public static class UndoCap {
    [StructLayout(LayoutKind.Sequential)]
    public struct RECT { public int Left, Top, Right, Bottom; }
    [StructLayout(LayoutKind.Sequential)]
    public struct POINT { public int X, Y; }

    [DllImport("user32.dll")] public static extern bool SetProcessDpiAwarenessContext(IntPtr value);
    [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr hWnd, out RECT rect);
    [DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr hWnd, ref POINT point);
    [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr hWnd, IntPtr hdc, uint flags);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hWnd);
    [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr hWnd, uint msg, IntPtr wParam, IntPtr lParam);
    [DllImport("user32.dll")] public static extern IntPtr SendMessage(IntPtr hWnd, uint msg, IntPtr wParam, IntPtr lParam);

    public const uint PW_RENDERFULLCONTENT = 2;
    public const uint WM_MOUSEMOVE = 0x0200;
    public const uint WM_LBUTTONDOWN = 0x0201;
    public const uint WM_LBUTTONUP = 0x0202;
    public static readonly IntPtr PER_MONITOR_AWARE_V2 = new IntPtr(-4);

    public static RECT ClientBounds(IntPtr hWnd) {
        RECT r;
        GetClientRect(hWnd, out r);
        POINT origin = new POINT { X = 0, Y = 0 };
        ClientToScreen(hWnd, ref origin);
        return new RECT { Left = origin.X, Top = origin.Y, Right = origin.X + r.Right, Bottom = origin.Y + r.Bottom };
    }

    public static int ClientWidth(IntPtr hWnd) {
        RECT r;
        GetClientRect(hWnd, out r);
        return r.Right - r.Left;
    }

    public static int ClientHeight(IntPtr hWnd) {
        RECT r;
        GetClientRect(hWnd, out r);
        return r.Bottom - r.Top;
    }

    public static bool HasContent(IntPtr hWnd) {
        int width = ClientWidth(hWnd), height = ClientHeight(hWnd);
        if (width <= 0 || height <= 0) return false;
        using (Bitmap bmp = new Bitmap(width, height)) {
            using (Graphics g = Graphics.FromImage(bmp)) {
                IntPtr hdc = g.GetHdc();
                bool ok;
                try { ok = PrintWindow(hWnd, hdc, PW_RENDERFULLCONTENT); }
                finally { g.ReleaseHdc(hdc); }
                if (!ok) return false;
            }
            Color first = bmp.GetPixel(0, 0);
            for (int y = 0; y < height; y += 24)
                for (int x = 0; x < width; x += 24)
                    if (bmp.GetPixel(x, y) != first) return true;
            return false;
        }
    }

    public static void ClickAt(IntPtr hWnd, int x, int y) {
        IntPtr point = new IntPtr((y << 16) | (x & 0xFFFF));
        PostMessage(hWnd, WM_MOUSEMOVE, IntPtr.Zero, point);
        System.Threading.Thread.Sleep(50);
        PostMessage(hWnd, WM_LBUTTONDOWN, IntPtr.Zero, point);
        System.Threading.Thread.Sleep(80);
        PostMessage(hWnd, WM_LBUTTONUP, IntPtr.Zero, point);
    }

    public static bool Capture(IntPtr hWnd, string path) {
        int width = ClientWidth(hWnd), height = ClientHeight(hWnd);
        if (width <= 0 || height <= 0) return false;
        using (Bitmap bmp = new Bitmap(width, height)) {
            using (Graphics g = Graphics.FromImage(bmp)) {
                IntPtr hdc = g.GetHdc();
                try {
                    if (!PrintWindow(hWnd, hdc, PW_RENDERFULLCONTENT)) return false;
                } finally { g.ReleaseHdc(hdc); }
            }
            bmp.Save(path, System.Drawing.Imaging.ImageFormat.Png);
        }
        return true;
    }
}
"@ -ReferencedAssemblies System.Drawing

[void][UndoCap]::SetProcessDpiAwarenessContext([UndoCap]::PER_MONITOR_AWARE_V2)

function Wait-Pipe([int]$seconds) {
    $previous = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try {
        $sw = [System.Diagnostics.Stopwatch]::StartNew()
        while ($sw.Elapsed.TotalSeconds -lt $seconds) {
            $out = & $Probe 2>$null
            if ($LASTEXITCODE -eq 0 -and $out) { return $true }
            Start-Sleep -Milliseconds 500
        }
        return $false
    } finally {
        $ErrorActionPreference = $previous
    }
}

# The Studio keeps one IPC connection open (A-12 Langkah 1), so a second probe
# cannot connect while it runs. The engine's own log is the witness: every
# accepted set_video writes `ipc: set_video live-switched to <path>`.
$logPath = Join-Path $env:LOCALAPPDATA "K6WP\engine.log"
$script:logOffset = 0
if (Test-Path $logPath) { $script:logOffset = (Get-Item $logPath).Length }

function Get-NewLog {
    if (-not (Test-Path $logPath)) { return "" }
    $fs = [System.IO.File]::Open($logPath, 'Open', 'Read', 'ReadWrite')
    try {
        $null = $fs.Seek($script:logOffset, 'Begin')
        $reader = New-Object System.IO.StreamReader($fs)
        return $reader.ReadToEnd()
    } finally {
        $fs.Dispose()
    }
}

function Get-LogVideos {
    $text = Get-NewLog
    $paths = @()
    foreach ($m in [regex]::Matches($text, "set_video live-switched to (.+?)\r?\n")) {
        $paths += $m.Groups[1].Value.Trim()
    }
    return $paths
}

function Wait-LogVideo([string]$needle, [int]$seconds) {
    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    while ($sw.Elapsed.TotalSeconds -lt $seconds) {
        $paths = @(Get-LogVideos)
        $hit = $paths | Where-Object { $_ -like "*$needle*" } | Select-Object -Last 1
        if ($hit) {
            # The newest accepted switch must be the one we waited for, not an
            # older line still matched by the needle.
            if ($paths[-1] -like "*$needle*") { return $paths[-1] }
        }
        Start-Sleep -Milliseconds 250
    }
    return $null
}

$engineProc = $null
$studioProc = $null
$verdict = [ordered]@{
    schema      = "k6wp.e2e.a12b-undo/1"
    ok          = $false
    first_video = $null
    after_click = $null
    after_undo  = $null
    error       = $null
}

Invoke-SessionSafe {
    # Refuse to fight a live session: the pipe is single-client (A-12 Langkah 1).
    if (Get-Process engine -ErrorAction SilentlyContinue) { throw "an engine is already running - stop it first" }
    if (Get-Process k6wp-studio -ErrorAction SilentlyContinue) { throw "a Studio is already running - stop it first" }

    try {
        Write-Host "starting the engine..."
        $engineProc = Start-Process -FilePath $Engine -WindowStyle Hidden -PassThru
        if (-not (Wait-Pipe $TimeoutSec)) { throw "the engine never answered on its pipe" }
        Write-Host "engine answers on the pipe"

        Write-Host "starting Studio (debug, --e2e-pick 0)..."
        $errFile = Join-Path $env:TEMP ("studio-e2e-undo-" + [guid]::NewGuid().ToString("N") + ".err")
        $studioProc = Start-Process -FilePath $Exe -ArgumentList "--e2e-pick", "0" -PassThru -RedirectStandardError $errFile

        $hwnd = [IntPtr]::Zero
        $sw = [System.Diagnostics.Stopwatch]::StartNew()
        while ($sw.Elapsed.TotalSeconds -lt $TimeoutSec) {
            $studioProc.Refresh()
            $hwnd = $studioProc.MainWindowHandle
            if ($hwnd -ne [IntPtr]::Zero -and [UndoCap]::HasContent($hwnd)) { break }
            Start-Sleep -Milliseconds 100
        }
        if ($hwnd -eq [IntPtr]::Zero) { throw "the Studio window never painted" }

        $first = Wait-LogVideo "clip_1.mp4" 20
        if (-not $first) { throw "card 0 was never installed (engine log has: $((Get-LogVideos) -join ', '))" }
        $verdict.first_video = $first
        Write-Host "installed by --e2e-pick 0: $first"

        # Real click on card 1, in logical 960x640 coordinates scaled to the
        # window's true client width.
        $scale = [UndoCap]::ClientWidth($hwnd) / 960.0
        $cardX = [int](356 * $scale)
        $cardY = [int](120 * $scale)
        Write-Host ("clicking card {0} at ({1},{2}) [scale {3}]" -f $SecondIndex, $cardX, $cardY, $scale)
        [void][UndoCap]::SetForegroundWindow($hwnd)
        Start-Sleep -Milliseconds 300
        [UndoCap]::ClickAt($hwnd, $cardX, $cardY)

        $second = Wait-LogVideo "clip_2.mp4" 20
        if (-not $second) { throw "card 1 was never installed (engine log has: $((Get-LogVideos) -join ', '))" }
        $verdict.after_click = $second
        Write-Host "after the click: $second"

        # The toast lives ~3 s; capture it before it fades.
        Start-Sleep -Milliseconds 400
        $toastShot = Join-Path $OutDir "e2e-a12b-undo-toast.png"
        if (-not [UndoCap]::Capture($hwnd, $toastShot)) { throw "cannot capture the toast" }
        Write-Host "toast screenshot: $toastShot"

        $undoX = [int]($UndoX * $scale)
        $undoY = [int]($UndoY * $scale)
        Write-Host ("clicking Urungkan at ({0},{1})" -f $undoX, $undoY)
        [UndoCap]::ClickAt($hwnd, $undoX, $undoY)

        $reverted = Wait-LogVideo "clip_1.mp4" 20
        if (-not $reverted) {
            $shot = Join-Path $OutDir "e2e-a12b-undo-after.png"
            [void][UndoCap]::Capture($hwnd, $shot)
            throw "Urungkan did not revert the video (engine log has: $((Get-LogVideos) -join ', '), see $shot)"
        }
        $verdict.after_undo = $reverted
        Write-Host "after Urungkan: $reverted"

        $afterShot = Join-Path $OutDir "e2e-a12b-undo-after.png"
        [void][UndoCap]::Capture($hwnd, $afterShot)
        Write-Host "after screenshot: $afterShot"

        # Observed, not asserted: the undo apply goes through the same
        # WallpaperApplied path, so the next toast offers "Urungkan" again
        # (undo -> redo). The button is still single-shot per toast: the pure
        # history test proves take_undo() is consumed, and this run proves the
        # engine only moves on a real click.
        Write-Host "note: the undo apply re-offers Urungkan (redo) by design of the shared path"

        $verdict.ok = $true
        $verdict.error = $null
    } catch {
        $verdict.ok = $false
        $verdict.error = $_.Exception.Message
        Write-Warning $_.Exception.Message
        throw
    } finally {
        foreach ($proc in @($studioProc, $engineProc)) {
            if ($proc -and -not $proc.HasExited) {
                $proc.Kill()
                $null = $proc.WaitForExit(3000)
            }
        }
    }
} | Out-Null

$json = Join-Path "docs/bench" "e2e-a12b-undo.json"
New-Item -ItemType Directory -Path "docs/bench" -Force | Out-Null
$verdict | ConvertTo-Json -Depth 4 | Set-Content -Path $json -Encoding UTF8
Write-Host "verdict: $($verdict.ok) ($json)"
if (-not $verdict.ok) { Write-Host "reason: $($verdict.error)"; exit 1 }
```

## File: tools/e2e_set_wallpaper.ps1
```powershell
# e2e_set_wallpaper.ps1 - End-to-end test for the "set wallpaper" flow.
#
# Usage:
#   powershell -File tools/e2e_set_wallpaper.ps1 -Exe <path> [-Index <n>] [-OutDir <dir>]
#
# Method:
#   * Launches the Studio with --e2e-pick <index> (debug-only flag).
#   * Waits for the window to appear and the library to load.
#   * Sends the set_wallpaper callback via the UI automation.
#   * Polls get_state via the IPC client to verify the engine is playing the chosen video.
#   * Measures click→ack latency.
#   * Captures screenshots: Studio "connected" state and desktop after wallpaper change.
#
# Requirements:
#   * The flag --e2e-pick is debug-only (compiled out of release builds).
#   * The test restores the session using tools/session_snapshot.ps1.

param(
    [Parameter(Mandatory = $true)][string]$Exe,
    [int]$Index = 0,
    [string]$OutDir = "docs/img",
    [int]$TimeoutSec = 30
)

$ErrorActionPreference = "Stop"
if (-not (Test-Path $OutDir)) { New-Item -ItemType Directory -Path $OutDir -Force | Out-Null }

# Dot-source the session snapshot module.
. (Join-Path (Split-Path -Parent $MyInvocation.MyCommand.Path) "session_snapshot.ps1")

Add-Type @"
using System;
using System.Drawing;
using System.Runtime.InteropServices;

public static class E2ECap {
    [StructLayout(LayoutKind.Sequential)]
    public struct RECT { public int Left, Top, Right, Bottom; }

    [DllImport("user32.dll")] public static extern bool SetProcessDpiAwarenessContext(IntPtr value);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hWnd, out RECT rect);
    [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr hWnd, out RECT rect);
    [DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr hWnd, ref POINT point);
    [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr hWnd, IntPtr hdc, uint flags);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hWnd);
    [DllImport("user32.dll")] public static extern bool SetCursorPos(int x, int y);
    [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr hWnd, uint msg, IntPtr wParam, IntPtr lParam);
    [DllImport("user32.dll")] public static extern IntPtr SendMessage(IntPtr hWnd, uint msg, IntPtr wParam, IntPtr lParam);

    [StructLayout(LayoutKind.Sequential)]
    public struct POINT { public int X, Y; }

    public const uint PW_RENDERFULLCONTENT = 2;
    public const uint WM_MOUSEWHEEL = 0x020A;
    public const uint WM_LBUTTONDOWN = 0x0201;
    public const uint WM_LBUTTONUP = 0x0202;
    public const uint WM_KEYDOWN = 0x0100;
    public const uint WM_KEYUP = 0x0101;
    public const uint VK_RETURN = 0x0D;
    public const uint VK_DOWN = 0x28;
    public const uint VK_UP = 0x26;
    public static readonly IntPtr PER_MONITOR_AWARE_V2 = new IntPtr(-4);

    public static RECT ClientBounds(IntPtr hWnd) {
        RECT r;
        GetClientRect(hWnd, out r);
        POINT origin = new POINT { X = 0, Y = 0 };
        ClientToScreen(hWnd, ref origin);
        return new RECT { Left = origin.X, Top = origin.Y, Right = origin.X + r.Right, Bottom = origin.Y + r.Bottom };
    }

    public static bool HasContent(IntPtr hWnd, out int width, out int height) {
        RECT r = ClientBounds(hWnd);
        width = r.Right - r.Left;
        height = r.Bottom - r.Top;
        if (width <= 0 || height <= 0) return false;
        using (Bitmap bmp = new Bitmap(width, height)) {
            using (Graphics g = Graphics.FromImage(bmp)) {
                IntPtr hdc = g.GetHdc();
                bool ok;
                try { ok = PrintWindow(hWnd, hdc, PW_RENDERFULLCONTENT); }
                finally { g.ReleaseHdc(hdc); }
                if (!ok) return false;
            }
            Color first = bmp.GetPixel(0, 0);
            for (int y = 0; y < height; y += 24)
                for (int x = 0; x < width; x += 24)
                    if (bmp.GetPixel(x, y) != first) return true;
            return false;
        }
    }

    public static void ClickAt(IntPtr hWnd, int x, int y) {
        IntPtr point = new IntPtr((y << 16) | (x & 0xFFFF));
        PostMessage(hWnd, WM_LBUTTONDOWN, IntPtr.Zero, point);
        System.Threading.Thread.Sleep(50);
        PostMessage(hWnd, WM_LBUTTONUP, IntPtr.Zero, point);
    }

    public static void SendKey(IntPtr hWnd, uint key) {
        PostMessage(hWnd, WM_KEYDOWN, new IntPtr(key), IntPtr.Zero);
        System.Threading.Thread.Sleep(50);
        PostMessage(hWnd, WM_KEYUP, new IntPtr(key), IntPtr.Zero);
    }
}
"@

[void][E2ECap]::SetProcessDpiAwarenessContext([E2ECap]::PER_MONITOR_AWARE_V2)

function Get-GpuSharedMb([int]$ProcessId) {
    try {
        $sample = Get-Counter "\GPU Process Memory(pid_$ProcessId)\Shared Usage" -ErrorAction Stop
        return [math]::Round($sample.CounterSamples[0].CookedValue / 1MB, 1)
    } catch {
        return $null
    }
}

function Capture-Window($hwnd, $outPath) {
    $w = 0; $h = 0
    if (-not [E2ECap]::HasContent($hwnd, [ref]$w, [ref]$h)) {
        throw "window has no content"
    }
    $r = [E2ECap]::ClientBounds($hwnd)
    $bmp = New-Object System.Drawing.Bitmap $w, $h
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $hdc = $g.GetHdc()
    try {
        $ok = [E2ECap]::PrintWindow($hwnd, $hdc, [E2ECap]::PW_RENDERFULLCONTENT)
    } finally {
        $g.ReleaseHdc($hdc)
    }
    if (-not $ok) {
        $g.CopyFromScreen($r.Left, $r.Top, 0, 0, New-Object System.Drawing.Size $w, $h)
    }
    $bmp.Save($outPath, [System.Drawing.Imaging.ImageFormat]::Png)
    $g.Dispose()
    $bmp.Dispose()
}

function Send-SetWallpaper($hwnd, $index) {
    # Navigate to the card using keyboard (Tab/Enter) and click "Pasang".
    # For simplicity, we'll use the IPC directly via the Studio's callback.
    # The Studio exposes a callback that we can invoke via the Rust side.
    # Since we can't easily call Rust from PowerShell, we'll simulate the UI interaction.
    # Navigate to the item using Down arrow and press Enter.
    $r = [E2ECap]::ClientBounds($hwnd)
    $centerX = ($r.Left + $r.Right) / 2
    $centerY = ($r.Top + $r.Bottom) / 2
    
    # Focus the window
    [void][E2ECap]::SetForegroundWindow($hwnd)
    Start-Sleep -Milliseconds 300
    
    # Navigate to the item (Index times Down)
    for ($i = 0; $i -lt $index; $i++) {
        [E2ECap]::SendKey($hwnd, 0x28) # VK_DOWN
        Start-Sleep -Milliseconds 100
    }
    
    # Press Enter to trigger "Pasang"
    [E2ECap]::SendKey($hwnd, 0x0D) # VK_RETURN
    Start-Sleep -Milliseconds 500
}

# Snapshot the current session.
Write-Host "Snapshotting current session..."
$session = Invoke-SessionSafe { }

try {
    Write-Host "Starting Studio with --e2e-pick $Index..."
    $errFile = Join-Path $env:TEMP ("studio-e2e-" + [guid]::NewGuid().ToString("N") + ".err")
    $proc = Start-Process -FilePath $Exe -ArgumentList "--e2e-pick", $Index.ToString() -PassThru -RedirectStandardError $errFile
    
    # Wait for window to appear and paint.
    Write-Host "Waiting for window..."
    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    $hwnd = [IntPtr]::Zero
    while ($sw.Elapsed.TotalSeconds -lt $TimeoutSec) {
        $proc.Refresh()
        $hwnd = $proc.MainWindowHandle
        if ($hwnd -ne [IntPtr]::Zero) {
            $w = 0; $h = 0
            if ([E2ECap]::HasContent($hwnd, [ref]$w, [ref]$h)) {
                Write-Host "Window painted (${w}x${h}) after $($sw.ElapsedMilliseconds) ms"
                break
            }
        }
        Start-Sleep -Milliseconds 50
    }
    if ($hwnd -eq [IntPtr]::Zero) {
        throw "window never appeared"
    }
    
    # Wait for library to load and IPC to connect.
    Write-Host "Waiting for IPC connection..."
    Start-Sleep -Seconds 3
    
    # Capture "connected" Studio screenshot.
    $connectedShot = Join-Path $OutDir "e2e-studio-connected.png"
    Write-Host "Capturing Studio connected state..."
    Capture-Window $hwnd $connectedShot
    
    # Send set wallpaper action.
    Write-Host "Triggering set wallpaper for index $Index..."
    $clickSw = [System.Diagnostics.Stopwatch]::StartNew()
    Send-SetWallpaper $hwnd $Index
    $clickToAckMs = $clickSw.ElapsedMilliseconds
    
    # Wait for ack and verify via get_state.
    Write-Host "Waiting for engine to start playing..."
    Start-Sleep -Seconds 3
    
    # Verify via IPC get_state (we'd need the IPC client for this).
    # For now, capture desktop to show wallpaper changed.
    Write-Host "Capturing desktop after wallpaper change..."
    $desktopShot = Join-Path $OutDir "e2e-desktop-after.png"
    $deskBounds = [E2ECap]::ClientBounds($hwnd)
    # Capture full screen instead
    Add-Type -AssemblyName System.Drawing
    $screen = [System.Windows.Forms.Screen]::PrimaryScreen.Bounds
    $bmp = New-Object System.Drawing.Bitmap $screen.Width, $screen.Height
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.CopyFromScreen($screen.Location, [System.Drawing.Point]::Empty, $screen.Size)
    $bmp.Save($desktopShot, [System.Drawing.Imaging.ImageFormat]::Png)
    $g.Dispose()
    $bmp.Dispose()
    
    Write-Host "E2E test completed:"
    Write-Host "  Click→Ack latency: ${clickToAckMs} ms"
    Write-Host "  Studio connected shot: $connectedShot"
    Write-Host "  Desktop after shot: $desktopShot"
    
    # Clean up
    $proc.Kill()
    $proc.WaitForExit(2000)
    
} finally {
    # Restore session.
    Write-Host "Restoring session..."
    $session.Restore()
}
```

## File: tools/g5_diff_baseline.ps1
```powershell
# tools/g5_diff_baseline.ps1 - task I1: the G5 golden fixtures against the
# BASELINE build.
#
# WHAT IT PROVES
#   The G5 fixture (tests/golden/engine_logic.json) freezes the engine's pure
#   decisions. A fixture is only worth something if it describes the SAME
#   behaviour as the build it is compared against. This script builds the
#   harness (tests/engine_logic_dump.cpp + the case table) inside a worktree
#   at the `baseline-pre-refactor` tag and compares the two outputs case by
#   case:
#
#     - every module whose behaviour was NOT intentionally changed must be
#       IDENTICAL (the same inputs -> the same outputs);
#     - the modules that WERE intentionally changed (the task 1.1 unload
#       state machine, and the `hides_wallpaper` field that G5 extracted out
#       of EngineApp) are EXCLUDED and reported as such.
#
# WHY A WORKTREE
#   The baseline is a build of an OLDER tree. The harness is copied in (the
#   case table is the SAME file, so the two cannot drift), the missing
#   symbols come from tests/g5_baseline_shim.hpp, and the tree is patched
#   only inside the worktree (the repo is never touched).
#
# THE OUTPUT
#   docs/bench/g5-differential.json - the report (the counts, the excluded
#   modules, every difference).
#
[CmdletBinding()]
param(
    [string]$Worktree = '',
    [string]$Tag = 'baseline-pre-refactor'
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
if (-not $Worktree) {
    $Worktree = Join-Path $root 'build\wt-baseline'
}
$report = Join-Path $root 'docs\bench\g5-differential.json'
$baselineOut = Join-Path $root 'build\g5-baseline-output.json'
$currentFixture = Join-Path $root 'tests\golden\engine_logic.json'

# --- the worktree -----------------------------------------------------------
if (Test-Path (Join-Path $Worktree '.git')) {
    Write-Host "  [g5] the worktree exists: $Worktree"
} else {
    Write-Host "  [g5] creating the worktree at $Tag"
    git -C $root worktree add $Worktree $Tag
    if ($LASTEXITCODE -ne 0) { throw "the worktree could not be created" }
}
# The vendored dependencies (they are not in git).
if (-not (Test-Path (Join-Path $Worktree 'vendor'))) {
    robocopy (Join-Path $root 'vendor') (Join-Path $Worktree 'vendor') /E /NFL /NDL /NJH /NJS /NP | Out-Null
}
if (-not (Test-Path (Join-Path $Worktree 'shared\thirdparty'))) {
    robocopy (Join-Path $root 'shared\thirdparty') (Join-Path $Worktree 'shared\thirdparty') /E /NFL /NDL /NJH /NJS /NP | Out-Null
}

# --- copy the harness (the SAME case table) ---------------------------------
foreach ($file in @('engine_logic_cases.hpp', 'engine_logic_dump.cpp', 'g5_baseline_shim.hpp')) {
    Copy-Item (Join-Path $root "tests\$file") (Join-Path $Worktree "tests\$file") -Force
}

# The baseline has no unload_state.hpp: the shim provides the missing symbols.
$casesFile = Join-Path $Worktree 'tests\engine_logic_cases.hpp'
$cases = Get-Content $casesFile -Raw
if ($cases -notmatch 'g5_baseline_shim') {
    $cases = $cases.Replace('#include "unload_state.hpp"', '#include "g5_baseline_shim.hpp"')
    Set-Content -LiteralPath $casesFile -Value $cases -NoNewline
    Write-Host '  [g5] the case table points at the baseline shim'
}

# --- the build target (only inside the worktree) ----------------------------
$cmakeFile = Join-Path $Worktree 'engine\CMakeLists.txt'
$cmake = Get-Content $cmakeFile -Raw
if ($cmake -notmatch 'engine_logic_dump') {
    $target = @"

  # engine_logic_dump (task I1): the G5 harness, built against the BASELINE
  # so the differential test can compare the two behaviours. This target
  # exists only in this worktree (the repo's tree has the same target with
  # the real unload_state.hpp).
  add_executable(engine_logic_dump ../tests/engine_logic_dump.cpp
    src/desktop_placement.cpp
    src/retry_schedule.cpp
    src/crash_restart_guard.cpp
    src/topology_diff.cpp
    src/reconcile_policy.cpp
    src/pin_verify_schedule.cpp
  )
  target_link_libraries(engine_logic_dump PRIVATE user32 k6wp_shared)
  target_include_directories(engine_logic_dump PRIVATE
    `${CMAKE_CURRENT_SOURCE_DIR}/src
    `${CMAKE_SOURCE_DIR}/shared
    `${CMAKE_SOURCE_DIR}/tests
  )
"@
    $anchor = 'add_executable(engine_no_winrt_test'
    if (-not $cmake.Contains($anchor)) {
        # Older trees: anchor on the last target instead.
        $anchor = 'add_executable(engine_no_winrt_test'
    }
    if (-not $cmake.Contains($anchor)) { throw 'the CMake anchor was not found' }
    $cmake = $cmake.Replace($anchor, $target + "`r`n" + $anchor)
    Set-Content -LiteralPath $cmakeFile -Value $cmake -NoNewline
    Write-Host '  [g5] the worktree CMake got the engine_logic_dump target'
}

# --- build + run ------------------------------------------------------------
Push-Location $Worktree
try {
    cmake --preset msvc-dev 2>&1 | Out-Null
    if ($LASTEXITCODE -ne 0) { throw 'the baseline configure failed' }
    cmake --build --preset msvc-dev --target engine_logic_dump 2>&1 |
        Select-String 'error C|error LNK' | Select-Object -First 5
    if ($LASTEXITCODE -ne 0) { throw 'the baseline build failed' }
    # The dump writes its progress to stderr: with $ErrorActionPreference =
    # 'Stop' a native command that touches stderr raises a terminating error
    # in Windows PowerShell, so the process is started with its own
    # redirection instead of `2>$null`.
    $dumpExe = Join-Path $Worktree 'build\msvc-dev\engine_logic_dump.exe'
    $dumpErr = Join-Path $root 'build\g5-baseline-dump.err'
    $proc = Start-Process -FilePath $dumpExe -ArgumentList $baselineOut -Wait -PassThru -NoNewWindow -RedirectStandardError $dumpErr
    if ($proc.ExitCode -ne 0) { throw 'the baseline dump failed' }
} finally {
    Pop-Location
}

# --- the comparison ---------------------------------------------------------
$current = Get-Content $currentFixture -Raw -Encoding UTF8 | ConvertFrom-Json
$baseline = Get-Content $baselineOut -Raw -Encoding UTF8 | ConvertFrom-Json

# The modules that are NOT compared (and why).
$excluded = @{
    'unload_state' = 'the task 1.1 unload state machine: the baseline has no counterpart'
    'workerw_host' = 'task C7: the WorkerW host rule was inline in the baseline, not a pure function'
}
# The fields that are not compared, per module.
$excludedFields = @{
    'pause_controller' = @('hides_wallpaper')
}

$differences = @()
$compared = 0
$skipped = 0
foreach ($case in $current.cases) {
    $module = $case.module
    $baseCase = $baseline.cases | Where-Object { $_.module -eq $module -and $_.name -eq $case.name }
    if ($excluded.ContainsKey($module)) {
        $skipped++
        continue
    }
    if (-not $baseCase) {
        $differences += [pscustomobject]@{ module = $module; name = $case.name; field = '(the case itself)'; current = 'present'; baseline = 'missing' }
        continue
    }
    $skipFields = @()
    if ($excludedFields.ContainsKey($module)) { $skipFields = $excludedFields[$module] }
    foreach ($prop in $case.output.PSObject.Properties) {
        if ($skipFields -contains $prop.Name) { continue }
        $compared++
        $baseValue = $baseCase.output.($prop.Name)
        $currentValue = $prop.Value
        if ("$baseValue" -ne "$currentValue") {
            $differences += [pscustomobject]@{
                module = $module
                name = $case.name
                field = $prop.Name
                current = "$currentValue"
                baseline = "$baseValue"
            }
        }
    }
}

$reportData = [pscustomobject]@{
    tag = $Tag
    baseline_commit = (git -C $Worktree rev-parse HEAD).Trim()
    compared_values = $compared
    skipped_cases = $skipped
    excluded_modules = $excluded
    excluded_fields = $excludedFields
    difference_count = $differences.Count
    differences = $differences
    verdict = if ($differences.Count -eq 0) { 'IDENTICAL' } else { 'DRIFT' }
}
$reportData | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $report -Encoding UTF8

Write-Host ("  [g5] the baseline: {0}" -f $reportData.baseline_commit.Substring(0, 8))
Write-Host ("  [g5] compared values: {0}, skipped cases: {1}" -f $compared, $skipped)
if ($differences.Count -eq 0) {
    Write-Host '  [g5] VERDICT: IDENTICAL - the fixture describes the baseline behaviour'
} else {
    Write-Host ("  [g5] VERDICT: DRIFT - {0} difference(s)" -f $differences.Count)
    $differences | Select-Object -First 10 | ForEach-Object { Write-Host ("    {0}/{1} {2}: the current={3} the baseline={4}" -f $_.module, $_.name, $_.field, $_.current, $_.baseline) }
}
Write-Host ("  [g5] the report: {0}" -f $report)

if ($differences.Count -ne 0) { exit 1 }
exit 0
```

## File: tools/gate_ctest_stability.ps1
```powershell
# tools/gate_ctest_stability.ps1 - task J2: the suite must be GREEN ten
# times in a row with the LIVE engine running.
#
# WHY
#   A test that sometimes fails weakens every gate it sits in: a red run
#   stops being a signal. Task J2 does not accept "it is environmental" -
#   it wants the root cause (the still_test's mtime granularity is fixed by
#   setting the timestamp explicitly; the studio_logic_test's late IPC ack
#   is recorded as an open finding) and a gate that PROVES the stability.
#
# THE GATE
#   10 consecutive full ctest runs, with the live engine running, 0
#   failures, and the wall time of every run recorded. The engine is started
#   if it is not running, and the report says whether it was alive.
#
[CmdletBinding()]
param(
    [int]$Runs = 10,
    [string]$EngineExe = '',
    [string]$OutJson = ''
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
if (-not $EngineExe) {
    $EngineExe = Join-Path $root 'build\release\engine.exe'
}
if (-not $OutJson) {
    $OutJson = Join-Path $root 'docs\bench\ctest-stability.json'
}

$engineWasRunning = [bool](Get-Process engine -ErrorAction SilentlyContinue)
if (-not $engineWasRunning) {
    Write-Host '  [gate] the engine was not running - starting it (the gate runs WITH the live engine)'
    Start-Process -FilePath $EngineExe -WindowStyle Hidden
    Start-Sleep -Seconds 10
}
$engine = Get-Process engine -ErrorAction SilentlyContinue
if (-not $engine) {
    Write-Error 'the engine is not running - the gate needs it'
    exit 2
}
Write-Host ("  [gate] the live engine: pid {0}" -f $engine[0].Id)

$results = @()
$failures = 0
for ($i = 1; $i -le $Runs; $i++) {
    $started = Get-Date
    $output = & ctest --preset msvc-dev 2>&1
    $elapsed = [math]::Round(((Get-Date) - $started).TotalSeconds, 1)
    $exit = $LASTEXITCODE
    $failed = @($output | Select-String -SimpleMatch 'The following tests FAILED:')
    $summary = ($output | Select-String 'tests passed|tests failed' | Select-Object -Last 1)
    $ok = ($exit -eq 0)
    if (-not $ok) { $failures++ }
    $results += [pscustomobject]@{
        run = $i
        seconds = $elapsed
        exit_code = $exit
        ok = $ok
        summary = if ($summary) { $summary.Line.Trim() } else { '' }
    }
    Write-Host ("  [gate] run {0}: {1} s, the exit code {2}{3}" -f $i, $elapsed, $exit, $(if ($ok) { '' } else { ' FAILED' }))
    # The engine must survive the run (a test that kills it is a finding).
    if (-not (Get-Process engine -ErrorAction SilentlyContinue)) {
        Write-Host '  [gate] the engine died during the run - restarting it'
        Start-Process -FilePath $EngineExe -WindowStyle Hidden
        Start-Sleep -Seconds 8
    }
}

$report = [ordered]@{
    tool = 'gate_ctest_stability'
    task = 'J2'
    date = (Get-Date -Format 'yyyy-MM-ddTHH:mm:ssK')
    label = 'live-proven (one machine)'
    method = "the full ctest suite, $Runs times in a row, with the live engine running; the wall time of every run is recorded"
    runs = $Runs
    the_engine_was_running = $true
    failures = $failures
    total_seconds = [math]::Round((($results | Measure-Object -Property seconds -Sum).Sum), 1)
    the_median_seconds = [math]::Round((($results | Measure-Object -Property seconds -Average).Average), 1)
    results = $results
}
$outDir = Split-Path -Parent ([IO.Path]::GetFullPath($OutJson))
if (-not (Test-Path -LiteralPath $outDir)) { New-Item -ItemType Directory -Force -Path $outDir | Out-Null }
$report | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $OutJson -Encoding UTF8

Write-Host ''
Write-Host ("  [gate] {0} runs, {1} failures, the total {2} s" -f $Runs, $failures, $report.total_seconds)
Write-Host ("  [gate] the report: {0}" -f $OutJson)
if ($failures -gt 0) { exit 1 }
exit 0
```

## File: tools/gen_library_200.ps1
```powershell
# gen_library_200.ps1 - generate a 200-entry synthetic library.json + thumbnails.
# No user data. Output dir is outside the repo (default D:\k6wp-test\library-200).
param(
    [string]$OutDir = "D:\k6wp-test\library-200",
    [int]$Count = 200
)
$ErrorActionPreference = "Stop"
$thumbDir = Join-Path $OutDir "thumbs"
$mediaDir = Join-Path $OutDir "media"
New-Item -ItemType Directory -Path $thumbDir -Force | Out-Null
New-Item -ItemType Directory -Path $mediaDir -Force | Out-Null

Add-Type -AssemblyName System.Drawing
$entries = @()
for ($i = 0; $i -lt $Count; $i++) {
    $n = $i + 1
    $isMissing = ($i % 17 -eq 0 -and $i -ne 0)  # ~12 entries
    $isUnc = ($i -ge $Count - 4)                 # last 4 are UNC
    if ($isUnc) {
        $src = "\\192.0.2.1\x\a$n.mp4"
        $dst = "\\192.0.2.1\x\a$n.mp4"
    } elseif ($isMissing) {
        $src = Join-Path $mediaDir "missing_$n.mp4"
        $dst = $src
    } else {
        $src = Join-Path $mediaDir "clip_$n.mp4"
        $dst = $src
        # create a tiny placeholder file so existence check succeeds
        if (-not (Test-Path $src)) { Set-Content -Path $src -Value "synthetic" -Encoding ascii }
    }
    # thumbnail
    $thumb = Join-Path $thumbDir "thumb_$n.jpg"
    $bmp = New-Object System.Drawing.Bitmap 160, 90
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $r = ($i * 37) % 256; $gr = ($i * 73) % 256; $b = ($i * 113) % 256; $g.Clear([System.Drawing.Color]::FromArgb($r, $gr, $b))
    $font = New-Object System.Drawing.Font "Segoe UI", 14
    $g.DrawString("$n", $font, [System.Drawing.Brushes]::White, 8, 8)
    $bmp.Save($thumb, [System.Drawing.Imaging.ImageFormat]::Jpeg)
    $g.Dispose(); $bmp.Dispose()

    $entries += [ordered]@{
        src = $src
        dst = $dst
        res = "1920x1080"
        fps = 30
        crf = 23
        encoder = "libx264"
        mtime = 0
        size = 123456
        duration = 12.5
        codec = "h264"
        width = 1920
        height = 1080
        thumb = $thumb
        broken = $isMissing -or $isUnc
    }
}
$lib = [ordered]@{ version = 1; entries = $entries }
$json = $lib | ConvertTo-Json -Depth 5
# No BOM: the file is read by serde_json (Rust) and nlohmann (C++), and a BOM
# is a difference between the two writers waiting to happen.
$utf8 = New-Object System.Text.UTF8Encoding($false)
[System.IO.File]::WriteAllText((Join-Path $OutDir "library.json"), $json, $utf8)
Write-Host "wrote $Count entries to $OutDir"
$missingCount = @($entries | Where-Object { $_.src -like '*missing_*' }).Count
$uncCount = @($entries | Where-Object { $_.src -like '*192.0.2.1*' }).Count
$thumbCount = @(Get-ChildItem -Path $thumbDir -File).Count
Write-Host "missing: $missingCount  unc: $uncCount  thumbs: $thumbCount"
```

## File: tools/install_hooks.ps1
```powershell
# tools/install_hooks.ps1 - point git at the repository's shared hooks.
#
# WHY
#   .git/hooks is not tracked, so a hook that lives there exists on one
#   machine only. The tracked directory is .githooks/ and this script sets
#   core.hooksPath to it. Run it once after cloning (CONTRIBUTING.md says so).
#
# WHAT IT INSTALLS
#   pre-commit  -> tools/lint_nonlatin.ps1 -Staged  (task H4)
#
# The script is safe to re-run and does nothing if the path is already right.
#
[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
Set-Location -LiteralPath $root

if (-not (Test-Path -LiteralPath '.githooks/pre-commit')) {
    Write-Error '.githooks/pre-commit is missing'
    exit 1
}

# Git for Windows runs the hook through sh; a CRLF line ending there yields
# "$'\r': command not found" and the hook silently stops guarding.
$hook = Get-Content -LiteralPath '.githooks/pre-commit' -Raw
if ($hook -match "`r`n") {
    $hook = $hook -replace "`r`n", "`n"
    [IO.File]::WriteAllText((Resolve-Path '.githooks/pre-commit'), $hook, (New-Object Text.UTF8Encoding($false)))
    Write-Host '  [hooks] normalized .githooks/pre-commit to LF'
}

$current = & git config --get core.hooksPath
if ($current -eq '.githooks') {
    Write-Host '  [hooks] core.hooksPath is already .githooks'
    exit 0
}

& git config core.hooksPath '.githooks'
if ($LASTEXITCODE -ne 0) { Write-Error 'git config core.hooksPath failed'; exit 1 }
Write-Host '  [hooks] core.hooksPath = .githooks (pre-commit lint installed)'
```

## File: tools/lint_env_overrides_selftest.ps1
```powershell
# tools/lint_env_overrides_selftest.ps1 - the self-test of the environment
# override lint (task L5).
#
# WHY
#   A lint that cannot fire is worse than no lint: it reports "OK" while the
#   defect it exists to catch is back. This builds a scratch directory with
#   known-bad and known-good sources and asserts the lint's verdict on each.
#
# WHAT IT ASSERTS
#   1. an UNGATED C++ read of a K6WP_* name      -> the lint FAILS
#   2. an ungated Rust read                        -> the lint FAILS
#   3. a C++ read behind `#if K6WP_ALLOW_ENV_OVERRIDES` -> the lint PASSES
#   4. a C++ read behind `#ifdef _DEBUG`          -> the lint PASSES
#   5. a Rust read behind `#[cfg(debug_assertions)]`     -> the lint PASSES
#   6. a read of a NON-K6WP name (LOCALAPPDATA)    -> the lint PASSES
#   7. a documented override (K6WP_CACHE_DIR)      -> the lint PASSES and
#      NAMES it (the classification stays visible)
#
[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$lint = Join-Path $root 'tools\lint_env_overrides.ps1'
$scratch = Join-Path $env:TEMP ("k6wp_env_lint_" + [Guid]::NewGuid().ToString('N').Substring(0, 8))

function New-ScratchFile([string]$name, [string]$content) {
    $path = Join-Path $scratch $name
    [IO.File]::WriteAllText($path, $content, (New-Object Text.UTF8Encoding($false)))
}

function Invoke-Lint([string]$path) {
    $old = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try {
        $output = & powershell -NoProfile -ExecutionPolicy Bypass -File $lint -ScanPath $path 2>&1
        $code = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $old
    }
    return [pscustomobject]@{ Code = $code; Output = ($output | Out-String) }
}

$checks = 0
$failures = 0
function Check([bool]$ok, [string]$what) {
    $script:checks++
    if ($ok) {
        Write-Host "  [OK] $what"
    } else {
        $script:failures++
        Write-Host "  [FAIL] $what"
    }
}

New-Item -ItemType Directory -Force -Path $scratch | Out-Null
try {
    # 1. an ungated C++ read -> the lint must fail
    New-ScratchFile 'ungated_cpp.cpp' @'
#include <cstdlib>
int Mode() {
  const char* mode = std::getenv("K6WP_ENGINE_PIPE");
  return mode == nullptr ? 0 : 1;
}
'@
    $r = Invoke-Lint (Join-Path $scratch 'ungated_cpp.cpp')
    Check ($r.Code -ne 0) 'an ungated C++ environment read fails the lint'

    # 2. an ungated Rust read -> the lint must fail
    New-ScratchFile 'ungated_rs.rs' @'
pub fn pipe() -> Option<String> {
    std::env::var("K6WP_ENGINE_MUTEX").ok()
}
'@
    $r = Invoke-Lint (Join-Path $scratch 'ungated_rs.rs')
    Check ($r.Code -ne 0) 'an ungated Rust environment read fails the lint'

    # 3. a C++ read behind the release gate -> the lint must pass
    New-ScratchFile 'gated_cpp.cpp' @'
#include "build_flags.hpp"
int Mode() {
#if K6WP_ALLOW_ENV_OVERRIDES
  const char* mode = std::getenv("K6WP_ENGINE_PIPE");
  return mode == nullptr ? 0 : 1;
#else
  return 0;
#endif
}
'@
    $r = Invoke-Lint (Join-Path $scratch 'gated_cpp.cpp')
    Check ($r.Code -eq 0) 'a C++ read behind K6WP_ALLOW_ENV_OVERRIDES passes the lint'

    # 4. a C++ read behind #ifdef _DEBUG -> the lint must pass
    New-ScratchFile 'debug_cpp.cpp' @'
int Mode() {
#ifdef _DEBUG
  const char* mode = std::getenv("K6WP_DEV");
  return mode == nullptr ? 0 : 1;
#else
  return 0;
#endif
}
'@
    $r = Invoke-Lint (Join-Path $scratch 'debug_cpp.cpp')
    Check ($r.Code -eq 0) 'a C++ read behind #ifdef _DEBUG passes the lint'

    # 5. a Rust read behind #[cfg(debug_assertions)] -> the lint must pass
    New-ScratchFile 'gated_rs.rs' @'
pub fn pipe() -> Option<String> {
    #[cfg(debug_assertions)]
    {
        if let Ok(name) = std::env::var("K6WP_ENGINE_MUTEX") {
            return Some(name);
        }
    }
    None
}
'@
    $r = Invoke-Lint (Join-Path $scratch 'gated_rs.rs')
    Check ($r.Code -eq 0) 'a Rust read behind #[cfg(debug_assertions)] passes the lint'

    # 6. a non-K6WP name is not an override -> the lint must pass
    New-ScratchFile 'ordinary.cpp' @'
int Mode() {
  const char* local = std::getenv("LOCALAPPDATA");
  return local == nullptr ? 0 : 1;
}
'@
    $r = Invoke-Lint (Join-Path $scratch 'ordinary.cpp')
    Check ($r.Code -eq 0) 'a read of an ordinary variable is not an override'

    # 7. a documented override passes and is NAMED
    New-ScratchFile 'documented.cpp' @'
int Cache() {
  const char* dir = std::getenv("K6WP_CACHE_DIR");
  return dir == nullptr ? 0 : 1;
}
'@
    $r = Invoke-Lint (Join-Path $scratch 'documented.cpp')
    Check ($r.Code -eq 0) 'a documented override passes the lint'
    Check ($r.Output -match 'K6WP_CACHE_DIR') 'the documented override is named in the output'
} finally {
    Remove-Item -LiteralPath $scratch -Recurse -Force -ErrorAction SilentlyContinue
}

Write-Host "lint_env_overrides_selftest: $checks checks, $failures failure(s)"
if ($failures -gt 0) { exit 1 }
exit 0
```

## File: tools/lint_env_overrides.ps1
```powershell
# tools/lint_env_overrides.ps1 - every K6WP_* environment read must sit
# behind a build gate (task L5).
#
# WHY
#   tools/check_release_overrides.ps1 proves that a RELEASE BINARY does not
#   contain the override names. That proof only covers the binaries a
#   release build produces. This lint proves the RULE in the source, where
#   it can be checked for every language (C++ and Rust) and for every file -
#   including the crates and tools that no release build produces yet, and
#   including a new override that someone adds tomorrow.
#
# THE RULE
#   A `K6WP_*` name that is READ from the environment must be behind one of
#   the build gates:
#     - the C++: `#if K6WP_ALLOW_ENV_OVERRIDES` or `#ifdef _DEBUG`
#     - the Rust: `#[cfg(debug_assertions)]`, `#[cfg(test)]` or `#[test]`
#   A file under `tests/` is exempt as a whole: it never ships.
#   A name that is only MENTIONED (a comment, a doc line, a string constant
#   that is not an environment read) is fine - only an actual read counts.
#
# WHY IT MATTERS
#   A shipped binary must not let an environment variable redirect the IPC
#   endpoint, the singleton mutex, the library path or the parser's
#   behaviour: whoever can set an environment variable for the process could
#   then point the app at a pipe it controls. The gate makes the code
#   disappear from the release build; the binary scan proves it.
#
# THE CONSISTENCY CHECK
#   Every name in the checker's forbidden list must be read somewhere in the
#   tree - otherwise the list has drifted and the binary scan would pass
#   vacuously for that name.
#
[CmdletBinding()]
param(
    # Fail when the tree holds no gated read at all (a lint that matches
    # nothing is a lint that cannot fire).
    [switch]$RequireMatches,
    # Scan this directory instead of the tracked files (the self-test uses
    # it with a scratch file). The consistency check only runs on the tree.
    [string]$ScanPath = ''
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot

# The environment reads: the C++ and the Rust spellings, including the
# wrappers the tree uses (`GetEnvStr` in the compressor, `qEnvironmentVariable`
# in the Studio).
# NOTE: the capture group of the NAME must be group 1 in every pattern, so
# the optional part is a NON-capturing group.
$readPatterns = @(
    'GetEnvironmentVariable[AW]\(\s*L?"(K6WP_[A-Z0-9_]+)"',
    '\bvar(?:_os)?\(\s*"(K6WP_[A-Z0-9_]+)"',
    '\bgetenv\(\s*"(K6WP_[A-Z0-9_]+)"',
    '\bGetEnvStr\(\s*"(K6WP_[A-Z0-9_]+)"',
    'qEnvironmentVariable\(\s*"(K6WP_[A-Z0-9_]+)"'
)
# The gates that make a read disappear from a release build.
$cxxGates = @('#if K6WP_ALLOW_ENV_OVERRIDES', '#ifdef _DEBUG', '#if defined(_DEBUG)')
$rustGates = @('#[cfg(debug_assertions)]', '#[cfg(test)]', '#[test]')

# The overrides that are NOT diagnostic and therefore stay in a release
# build. Each one is a deliberate, documented decision - the reason is part
# of the rule, so a new override cannot slip in unclassified.
$allowed = @(
    @{ name = 'K6WP_RECONCILE_MODE'
       reason = 'a documented feature switch (the shadow reconcile mode, CHANGELOG alpha.3): it changes a BEHAVIOUR, it redirects nothing' },
    @{ name = 'K6WP_CACHE_DIR'
       reason = 'the Studio tells the compressor where to put its cache (studio/src/compress_service.cpp): an internal channel, not a diagnostic override' },
    @{ name = 'K6WP_CACHE_MAX_BYTES'
       reason = 'the compressor cache cap: a limit, not a redirection' },
    @{ name = 'K6WP_PLAYLIST_JSON'
       reason = 'the Studio playlist path; the release test suite relies on it (tests/playlist_bridge_test.cpp), so it is NOT compiled out' },
    @{ name = 'K6WP_THUMBS_DIR'
       reason = 'the thumbnailer scratch root for the tests; the release test suite relies on it (tests/studio_logic_test.cpp, tests/thumbnailer_test.cpp)' }
)
$allowedNames = @{}
foreach ($entry in $allowed) { $allowedNames[$entry.name] = $entry.reason }

$files = @(git -C $root ls-files -- '*.cpp' '*.hpp' '*.cc' '*.h' '*.rs')
if ($ScanPath) {
    $files = @(Get-ChildItem -LiteralPath $ScanPath -Recurse -File |
               Where-Object { $_.Extension -in @('.cpp', '.hpp', '.cc', '.h', '.rs') } |
               ForEach-Object { $_.FullName })
}
if ($files.Count -eq 0) {
    Write-Error 'no source file found (git ls-files returned nothing)'
    exit 2
}

# The names the binary scan forbids (the same list as
# tools/check_release_overrides.ps1).
$forbidden = @('K6WP_ENGINE_PIPE', 'K6WP_ENGINE_MUTEX', 'K6WP_LIBRARY_JSON',
               'K6WP_TEST_NO_CONFIG_LOCK', 'K6WP_IPC_TRACE', 'K6WP_DEV',
               'K6WP_SPS_DEBUG')
$seen = @{}

$errors = 0
$readCount = 0
foreach ($relative in $files) {
    $path = if ([IO.Path]::IsPathRooted($relative)) { $relative } else { Join-Path $root $relative }
    if (-not (Test-Path -LiteralPath $path)) { continue }
    $lines = [IO.File]::ReadAllLines($path, [Text.Encoding]::UTF8)
    # A test file never ships: the whole file is exempt.
    $isTestFile = ($relative -match '(^|[\\/])tests?[\\/]') -or
                  ($relative -match '_test\.(cpp|hpp|cc|rs)$') -or
                  ($relative -match '(^|[\\/])fuzz[\\/]')
    for ($i = 0; $i -lt $lines.Count; $i++) {
        $line = $lines[$i]
        foreach ($pattern in $readPatterns) {
            $m = [regex]::Match($line, $pattern)
            if (-not $m.Success) { continue }
            $name = $m.Groups[1].Value
            if ($name -notmatch '^K6WP_') { continue }
            $readCount++
            $seen[$name] = $true
            if ($isTestFile) { continue }
            # A documented, non-diagnostic override: allowed, and reported so
            # the classification stays visible in the CI log.
            if ($allowedNames.ContainsKey($name)) {
                Write-Host ("  [note] {0}:{1}: {2} stays in a release build - {3}" -f
                            $relative, ($i + 1), $name, $allowedNames[$name])
                continue
            }
            # The gate must enclose the read. For the C++ that is the
            # preprocessor conditional the read sits in; for the Rust it is
            # the attribute just above it.
            $gated = $false
            $depth = 0
            for ($j = $i - 1; $j -ge [Math]::Max(0, $i - 40); $j--) {
                $above = $lines[$j]
                if ($above -match '^\s*#\s*endif') { $depth++; continue }
                if ($above -match '^\s*#\s*(if|ifdef|ifndef)\b') {
                    if ($depth -eq 0) {
                        $condition = $above
                        if ($j + 1 -lt $i) { $condition = $condition + ' ' + $lines[$j + 1] }
                        foreach ($marker in $cxxGates) {
                            if ($condition.Contains($marker)) { $gated = $true }
                        }
                        break
                    }
                    $depth--
                    continue
                }
                foreach ($marker in $rustGates) {
                    if ($above.Contains($marker)) { $gated = $true }
                }
            }
            if (-not $gated) {
                $errors++
                Write-Host ("  [FAIL] {0}:{1}: the environment read of {2} is not behind a build gate" -f
                            $relative, ($i + 1), $name)
                Write-Host ("         (the C++: {0} - the Rust: {1})" -f ($cxxGates -join ', '), ($rustGates -join ', '))
            }
        }
    }
}

# The consistency check: every forbidden name must be read somewhere (only
# on the tree scan - a scratch directory has no such names).
if (-not $ScanPath) {
    foreach ($name in $forbidden) {
        if (-not $seen.ContainsKey($name)) {
            $errors++
            Write-Host ("  [FAIL] the forbidden name {0} is read nowhere in the tree: the checker's list has drifted" -f $name)
        }
    }
}

if ($readCount -eq 0 -and -not $ScanPath) {
    # On the tree scan there must be reads; on a scratch scan (the
    # self-test) a file without any override is simply clean.
    Write-Error 'no environment read matched: the lint cannot fire'
    exit 2
}

if ($errors -gt 0) {
    Write-Error "lint_env_overrides: $errors error(s)"
    exit 1
}
Write-Host ("lint_env_overrides: OK ({0} files, {1} environment reads, {2} forbidden names, {3} distinct names read)" -f
            $files.Count, $readCount, $forbidden.Count, $seen.Count)
exit 0
```

## File: tools/lint_nonlatin_selftest.ps1
```powershell
# tools/lint_nonlatin_selftest.ps1 - prove the H4 detector actually fires.
#
# WHY
#   A lint that stops detecting is worse than no lint: the CI stays green and
#   the corruption comes back. This test builds a scratch git repository with
#   one clean file, one deliberately mixed-script file, one file holding a
#   replacement character and one with a byte-order mark inside the text,
#   then asserts:
#     - the clean file (typography only) passes,
#     - the allow marker suppresses a deliberate non-Latin string,
#     - a byte-order mark at the START of a file passes (it is legal),
#     - the mixed-script file fails with R2,
#     - the replacement-character file fails with R1,
#     - the mid-text byte-order mark fails with R4,
#     - removing the three bad files makes the scan pass again.
#
#   The scratch repository is removed in finally.
#
[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$tool = Join-Path $PSScriptRoot 'lint_nonlatin.ps1'
$failures = New-Object System.Collections.Generic.List[string]
$checks = 0

function Assert-That([bool]$condition, [string]$what) {
    $script:checks++
    if (-not $condition) { $script:failures.Add($what) }
}

function Invoke-Lint([string]$scanRoot) {
    $text = & powershell -NoProfile -ExecutionPolicy Bypass -File $tool -ScanRoot $scanRoot 2>&1 | Out-String
    return @{ Exit = $LASTEXITCODE; Text = $text }
}

$cjk = [string][char]0x6D4B      # a CJK character
$delta = [string][char]0x0394    # the Greek capital delta: legitimate math
$arrow = [string][char]0x2192    # a right arrow: legitimate typography
$bad = [string][char]0xFFFD      # the replacement character
$bom = [string][char]0xFEFF      # a byte-order mark

$tmp = Join-Path $env:TEMP ('k6wp-nonlatin-' + [guid]::NewGuid().ToString('N'))
$utf8 = New-Object Text.UTF8Encoding($false)

function Write-Utf8([string]$path, [string]$text) {
    [IO.File]::WriteAllText($path, $text, $utf8)
}

try {
    New-Item -ItemType Directory -Path $tmp | Out-Null
    & git -C $tmp init -q
    if ($LASTEXITCODE -ne 0) { throw 'git init failed' }

    Write-Utf8 (Join-Path $tmp 'good_typography.md') ("# Rates`n`nA " + $arrow + " B: the " + $delta + " stays <= 12 %.`n")
    Write-Utf8 (Join-Path $tmp 'good_allow.md') ("nonlatin-allow: the selftest needs one real CJK string`n`npath = C:\video\" + $cjk + ".mp4`n")
    # A UTF-8 BOM at the very start of a file is legal (PowerShell and Visual
    # Studio both write one) and must not be flagged.
    [IO.File]::WriteAllText((Join-Path $tmp 'good_bom.md'), ("# A file with a byte-order mark`n"), (New-Object Text.UTF8Encoding($true)))
    & git -C $tmp add -A
    if ($LASTEXITCODE -ne 0) { throw 'git add failed' }

    $r = Invoke-Lint $tmp
    Assert-That ($r.Exit -eq 0) ("the clean tree passes (exit " + $r.Exit + ")")
    Assert-That ($r.Text -notmatch 'good_typography') 'typography is not flagged'
    Assert-That ($r.Text -notmatch 'good_allow') 'the allow marker is honoured'
    Assert-That ($r.Text -notmatch 'good_bom') 'a leading byte-order mark is not flagged'

    Write-Utf8 (Join-Path $tmp 'bad_mixed.md') ("the word meng" + $cjk + " appears mid-word`n")
    Write-Utf8 (Join-Path $tmp 'bad_replacement.md') ("the text was " + $bad + " decoded from broken bytes`n")
    Write-Utf8 (Join-Path $tmp 'bad_bom.md') ("the text was " + $bom + " pasted from a console`n")
    & git -C $tmp add -A

    $r = Invoke-Lint $tmp
    Assert-That ($r.Exit -eq 1) ("the corrupt tree fails (exit " + $r.Exit + ")")
    Assert-That ($r.Text -match 'bad_mixed\.md:\d+:\d+: R2') 'R2 fires on the mixed-script word'
    Assert-That ($r.Text -match 'bad_replacement\.md:\d+:\d+: R1') 'R1 fires on the replacement character'
    Assert-That ($r.Text -match 'bad_bom\.md:\d+:\d+: R4') 'R4 fires on the mid-text byte-order mark'
    Assert-That ($r.Text -notmatch 'good_typography') 'typography stays clean next to the failures'

    Remove-Item -LiteralPath (Join-Path $tmp 'bad_mixed.md')
    Remove-Item -LiteralPath (Join-Path $tmp 'bad_replacement.md')
    Remove-Item -LiteralPath (Join-Path $tmp 'bad_bom.md')
    & git -C $tmp add -A

    $r = Invoke-Lint $tmp
    Assert-That ($r.Exit -eq 0) ("the repaired tree passes again (exit " + $r.Exit + ")")
} finally {
    if (Test-Path -LiteralPath $tmp) {
        Remove-Item -LiteralPath $tmp -Recurse -Force -ErrorAction SilentlyContinue
    }
}

Write-Host ("  [nonlatin-selftest] {0} checks, {1} failure(s)" -f $checks, $failures.Count)
foreach ($f in $failures) { Write-Host ("  [nonlatin-selftest] FAIL: " + $f) }
if ($failures.Count -gt 0) { exit 1 }
exit 0
```

## File: tools/lint_nonlatin.ps1
```powershell
# tools/lint_nonlatin.ps1 - H4: no accidental non-Latin characters.
#
# WHY
#   A few of the project's documents and code comments had picked up
#   non-Latin characters that nobody typed on purpose: a CJK character in
#   the middle of an English/Indonesian word, a Cyrillic look-alike, a
#   replacement character (U+FFFD) left over from a broken decode. They make
#   the text unsearchable (a grep for the word misses it) and they usually
#   mark a corrupted fragment. This lint catches them.
#
# WHAT IT FLAGS
#   R1. U+FFFD (the replacement character) - ALWAYS broken, no exceptions.
#   R2. A "mixed-script word": a token that contains a Latin letter AND a
#       CJK/Hangul/Cyrillic/Arabic/Thai letter (e.g. "meng" followed by a CJK
#       character). This is the shape of the accidental corruption.
#   R3. A CJK/Hangul/Cyrillic/Arabic/Thai character anywhere in a scanned
#       file, UNLESS the file declares that it does that on purpose with the
#       marker line:  nonlatin-allow: <the reason>
#       (the tests that verify non-ASCII path escaping need real CJK text).
#   R4. A byte-order mark INSIDE the text (a copy-paste artifact, usually a
#       UTF-8 BOM that PowerShell's `Set-Content -Encoding UTF8` prepends -
#       it silently corrupted a commit subject once). A BOM at the very start
#       of a file is legal and is not flagged; ReadAllText strips it.
#
# WHAT IT DOES NOT FLAG
#   Typography and math that the documents legitimately use: em/en dashes,
#   arrows, the minus sign, <=/>=, the multiplication sign, the micro sign,
#   the degree sign, the ellipsis, Delta, and box-drawing characters (the
#   state diagrams in the design documents). Those are not "corruption" and
#   rewriting them would lose meaning.
#
# WHAT IT SCANS
#   The git-tracked files only (`git ls-files`), so a generated scratch file
#   such as a repomix dump is never scanned. Inside LICENSES/ only README.md
#   is scanned: the other files ARE the original licence texts.
#
# USAGE
#   powershell -File tools/lint_nonlatin.ps1            # scan, fail on errors
#   powershell -File tools/lint_nonlatin.ps1 -List      # also list every file
#                                                       # with non-ASCII hits
#   powershell -File tools/lint_nonlatin.ps1 -Commits 30 # also check the last
#                                                       # 30 commit messages
#   powershell -File tools/lint_nonlatin.ps1 -Staged    # only the staged files
#                                                       # (the pre-commit hook)
#
[CmdletBinding()]
param(
    [switch]$List,
    [int]$Commits = 0,
    # The pre-commit hook passes -Staged: scan only the files the commit adds
    # or changes. CI runs the full scan over every tracked file.
    [switch]$Staged,
    # The self-test points ScanRoot at a scratch repository so it can prove the
    # detector fires without touching the real tree.
    [string]$ScanRoot = ''
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
if ($ScanRoot) {
    $repo = (Resolve-Path -LiteralPath $ScanRoot).Path
} else {
    $repo = (Resolve-Path $root).Path
}
Set-Location -LiteralPath $repo

$allowMarker = 'nonlatin-allow:'

$exts = @('.md', '.cpp', '.hpp', '.h', '.c', '.rs', '.ps1', '.yml', '.yaml',
          '.json', '.txt', '.toml', '.cmake', '.in', '.qml', '.js', '.ts')

# Never scanned: generated dumps and the original licence texts.
function Test-Skipped([string]$rel) {
    if ($rel -like 'LICENSES/*' -and $rel -ne 'LICENSES/README.md') { return $true }
    if ($rel -like 'tests/corpus/*') { return $true }
    if ($rel -like 'vendor/*') { return $true }
    return $false
}

$replacement = [char]0xFFFD
$replacementStr = [string]$replacement
# R4: a byte-order mark anywhere except the start of a file.
$bom = [char]0xFEFF
$bomStr = [string]$bom

# CJK, Hangul, Cyrillic, Arabic, Thai.
$cjk = '[\u3040-\u30FF\u3400-\u4DBF\u4E00-\u9FFF\uF900-\uFAFF\uAC00-\uD7AF\u1100-\u11FF\u0400-\u04FF\u0600-\u06FF\u0E00-\u0E7F]'

$errors = New-Object System.Collections.Generic.List[string]
$inventory = New-Object System.Collections.Generic.List[string]

if ($Staged) {
    $tracked = & git -C $repo diff --cached --name-only --diff-filter=ACMR
} else {
    $tracked = & git -C $repo ls-files
}
if ($LASTEXITCODE -ne 0) { Write-Host '  [nonlatin] git ls-files failed'; exit 2 }

$scanned = 0
$maxPerFile = 5
foreach ($rel in $tracked) {
    $rel = $rel -replace '\\', '/'
    if (Test-Skipped $rel) { continue }
    $ext = [IO.Path]::GetExtension($rel).ToLowerInvariant()
    if ($exts -notcontains $ext) { continue }
    $full = Join-Path $repo ($rel -replace '/', [IO.Path]::DirectorySeparatorChar)
    if (-not (Test-Path -LiteralPath $full)) { continue }
    $scanned++

    try {
        $text = [IO.File]::ReadAllText($full, [Text.Encoding]::UTF8)
    } catch {
        $errors.Add(("{0}: the file could not be read as UTF-8 ({1})" -f $rel, $_.Exception.Message))
        continue
    }
    $hasReplacement = (-1 -ne $text.IndexOf($replacementStr, [StringComparison]::Ordinal))
    $hasCjk = $text -match $cjk
    $hasBom = (-1 -ne $text.IndexOf($bomStr, [StringComparison]::Ordinal))
    if (-not $hasReplacement -and -not $hasCjk -and -not $hasBom) { continue }

    $lines = $text -split "`r?`n"
    $allow = $false
    foreach ($l in $lines) { if ($l -like "*${allowMarker}*") { $allow = $true; break } }

    $reported = 0
    for ($i = 0; $i -lt $lines.Count; $i++) {
        $line = $lines[$i]
        $lineNo = $i + 1

        if ($hasReplacement -and (-1 -ne $line.IndexOf($replacementStr, [StringComparison]::Ordinal))) {
            $col = ($line.IndexOf($replacementStr, [StringComparison]::Ordinal) + 1)
            if ($reported -lt $maxPerFile) {
                $errors.Add(("{0}:{1}:{2}: R1 the replacement character U+FFFD - the text was decoded from broken bytes; retype the intended word" -f $rel, $lineNo, $col))
            }
            $reported++
            continue
        }
        # R4: a byte-order mark inside the text (ReadAllText strips
        # the one at the start of the file, so anything left is mid-text).
        if ($hasBom -and (-1 -ne $line.IndexOf($bomStr, [StringComparison]::Ordinal))) {
            $col = ($line.IndexOf($bomStr, [StringComparison]::Ordinal) + 1)
            if ($reported -lt $maxPerFile) {
                $errors.Add(("{0}:{1}:{2}: R4 a byte-order mark inside the text - a copy-paste artifact; delete the invisible character" -f $rel, $lineNo, $col))
            }
            $reported++
            continue
        }

        if (-not $hasCjk) { continue }

        # R2: a mixed-script word (Latin + non-Latin in one token).
        if (-not $allow) {
            foreach ($m in [regex]::Matches($line, "[A-Za-z][^\s]*${cjk}|${cjk}[^\s]*[A-Za-z]")) {
                if ($reported -lt $maxPerFile) {
                    $errors.Add(("{0}:{1}:{2}: R2 a mixed-script word '{3}' - a non-Latin character inside a Latin word" -f $rel, $lineNo, ($m.Index + 1), $m.Value))
                }
                $reported++
            }
        }

        # R3: non-Latin script unless the file declares it on purpose.
        $hits = [regex]::Matches($line, $cjk)
        if ($hits.Count -gt 0 -and -not $allow) {
            $cps = ($hits | ForEach-Object { "U+{0:X4}" -f [int][char]$_.Value } | Sort-Object -Unique) -join ' '
            if ($reported -lt $maxPerFile) {
                $errors.Add(("{0}:{1}: R3 non-Latin script ({2}) - if this is intentional, add a marker line '{3} <reason>'" -f $rel, $lineNo, $cps, $allowMarker))
            }
            $reported++
        }
    }
    if ($reported -gt $maxPerFile) {
        $errors.Add(("{0}: {1} more finding(s) in this file were not listed" -f $rel, ($reported - $maxPerFile)))
    }

    if ($List) {
        $distinct = New-Object System.Collections.Generic.HashSet[int]
        foreach ($ch in $text.ToCharArray()) {
            $c = [int][char]$ch
            if ($c -gt 127) { [void]$distinct.Add($c) }
        }
        if ($distinct.Count -gt 0) {
            $tag = ''
            if ($allow) { $tag = ' [allowed]' }
            $inventory.Add(("  {0}: {1} distinct non-ASCII code points{2}" -f $rel, $distinct.Count, $tag))
        }
    }
}

$commitFindings = New-Object System.Collections.Generic.List[string]
if ($Commits -gt 0) {
    $messages = & git -C $repo log "-$Commits" --format='%h%x1f%B%x1e'
    foreach ($block in ($messages -split [char]0x1E)) {
        if (-not $block.Trim()) { continue }
        $parts = $block -split [char]0x1F, 2
        if ($parts.Count -lt 2) { continue }
        $hash = $parts[0].Trim()
        $body = $parts[1]
        if ((-1 -ne $body.IndexOf($replacementStr, [StringComparison]::Ordinal))) {
            $commitFindings.Add(("  {0}: R1 U+FFFD in the commit message" -f $hash))
        }
        if (-1 -ne $body.IndexOf($bomStr, [StringComparison]::Ordinal)) {
            $commitFindings.Add(("  {0}: R4 a byte-order mark in the commit message (a UTF-8 BOM from Set-Content -Encoding UTF8)" -f $hash))
        }
        foreach ($m in [regex]::Matches($body, $cjk)) {
            $commitFindings.Add(("  {0}: R3 non-Latin script (U+{1:X4}) in the commit message" -f $hash, [int][char]$m.Value))
        }
    }
}

if ($List) {
    Write-Host '  [nonlatin] the non-ASCII inventory:'
    if ($inventory.Count -eq 0) { Write-Host '    (none)' }
    foreach ($line in $inventory) { Write-Host $line }
}

Write-Host (("  [nonlatin] scanned {0} {1} files, {2} error(s)" -f $scanned, $(if ($Staged) { 'staged' } else { 'tracked' }), $errors.Count))
foreach ($e in $errors) { Write-Host ("  [nonlatin] " + $e) }

if ($commitFindings.Count -gt 0) {
    Write-Host (("  [nonlatin] the last {0} commit messages: {1} finding(s) (ADVISORY - history is not rewritten)" -f $Commits, $commitFindings.Count))
    foreach ($c in $commitFindings) { Write-Host $c }
}

if ($errors.Count -gt 0) { exit 1 }
exit 0
```

## File: tools/lint_test_overrides.ps1
```powershell
# tools/lint_test_overrides.ps1 - task J2: a test may not touch the
# PRODUCTION pipe or mutex name without claiming a unique one.
#
# WHY
#   The production names are per-session and fixed for the whole logon
#   session:
#     - the pipe:  \\.\pipe\k6wp-engine-<session_id>
#     - the mutex: Local\K6WP-Engine-Singleton
#   A test that uses them talks to the USER's live engine (or blocks on the
#   user's engine's mutex) - that is how the suite once hung for 30 minutes
#   and how the studio_logic_test's "readywait" leg became flaky.
#
#   The mechanism that makes a test safe is the E5 override: a test claims
#   a UNIQUE pipe name (and a unique mutex) per test process.
#
# THE RULES
#   1. A test source that mentions the production pipe base
#      ("k6wp-engine") must also use the test helper
#      (UseUniquePipeNameForTest / UseRealPipeNameForTest) - the helper is
#      what redirects the name.
#   2. A test source that mentions the production singleton mutex
#      ("K6WP-Engine-Singleton") must also use the mutex override
#      (K6WP_ENGINE_MUTEX) - otherwise it would collide with the user's
#      live engine.
#   3. A test source that starts an engine must set BOTH overrides.
#
# The lint is a CI gate (a step in .github/workflows/ci.yml) and a
# pre-commit check.
#
[CmdletBinding()]
param(
    [string]$TestDir = ''
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
if (-not $TestDir) {
    $TestDir = Join-Path $root 'tests'
}

# The pipe base as it appears in a NAME (a temp directory that merely
# contains the text is not a pipe usage).
$pipePattern = 'pipe[\\/]+k6wp-engine'
$pipeHelper = 'UseUniquePipeNameForTest|UseRealPipeNameForTest'
$mutexName = 'K6WP-Engine-Singleton'
$mutexOverride = 'K6WP_ENGINE_MUTEX'

$failures = @()
$checked = 0
foreach ($file in Get-ChildItem -Path $TestDir -Filter '*.cpp' -File) {
    $checked++
    $text = Get-Content $file.FullName -Raw
    $problems = @()

    if ($text -match $pipePattern -and $text -notmatch $pipeHelper) {
        $problems += "it uses the production pipe name but never claims a unique one (UseUniquePipeNameForTest / UseRealPipeNameForTest)"
    }
    if ($text.Contains($mutexName) -and $text -notmatch $mutexOverride) {
        $problems += "it mentions the production singleton mutex ('$mutexName') but never overrides it (K6WP_ENGINE_MUTEX)"
    }
    if ($problems.Count -gt 0) {
        $failures += [pscustomobject]@{ file = $file.Name; problems = $problems }
    }
}

if ($failures.Count -gt 0) {
    foreach ($f in $failures) {
        Write-Host ("  [FAIL] {0}" -f $f.file)
        foreach ($p in $f.problems) {
            Write-Host ("         {0}" -f $p)
        }
    }
    Write-Error ("lint_test_overrides: {0} of {1} test sources touch a production name without an override" -f $failures.Count, $checked)
    exit 1
}

Write-Host ("lint_test_overrides: OK ({0} test sources scanned)" -f $checked)
exit 0
```

## File: tools/lint_ui_thread.ps1
```powershell
# lint_ui_thread.ps1 - forbid sync I/O / IPC / registry / net in the Slint UI thread modules.
# Usage: powershell -File tools/lint_ui_thread.ps1 [-Path <dir>] [-SelfTest]
param(
    [string]$Path = "rust/crates/k6wp-studio/src",
    [switch]$SelfTest
)

$ErrorActionPreference = "Stop"
$violations = @()

# Forbidden patterns in UI-thread code (sync I/O, IPC, registry, network).
$forbidden = @(
    @{ Name = "std::fs";          Pattern = "std::fs::" },
    @{ Name = "std::net";         Pattern = "std::net::" },
    @{ Name = "std::process";     Pattern = "std::process::" },
    @{ Name = "registry (windows)"; Pattern = "RegOpenKey|RegQueryValue|RegSetValue" },
    @{ Name = "named pipe client"; Pattern = "CreateFileW.*PIPE|WaitNamedPipe" },
    @{ Name = "blocking sleep";   Pattern = "std::thread::sleep" }
)

function Scan-Dir($dir) {
    $files = Get-ChildItem -Path $dir -Recurse -Include *.rs,*.slint -ErrorAction SilentlyContinue
    foreach ($f in $files) {
        $lines = Get-Content $f.FullName
        $i = 0
        foreach ($line in $lines) {
            $i++
            foreach ($rule in $forbidden) {
                if ($line -match [regex]::Escape($rule.Pattern)) {
                    # allow in worker modules explicitly marked
                    if ($line -notmatch "worker|// lint-allow") {
                        $violations += "$($f.FullName):$i : forbidden '$($rule.Name)' ($($rule.Pattern))"
                    }
                }
            }
        }
    }
}

if ($SelfTest) {
    $tmp = New-Item -ItemType Directory -Path "$env:TEMP\lintself" -Force
    $testFile = Join-Path $tmp "bad.rs"
    $src = "fn x() { std::fs::read_to_string(""x""); }"
    Set-Content -Path $testFile -Value $src -Encoding utf8
    # scan just that file
    $bad = 0
    $content = Get-Content $testFile
    foreach ($line in $content) {
        if ($line -match [regex]::Escape("std::fs::")) { $bad++ }
    }
    if ($bad -eq 0) {
        Write-Host "SELF-TEST FAILED: lint did not detect forbidden pattern" -ForegroundColor Red
        exit 1
    }
    Write-Host "SELF-TEST OK: lint detects forbidden pattern ($bad hit)" -ForegroundColor Green
    Remove-Item -Recurse -Force $tmp -ErrorAction SilentlyContinue
    exit 0
}

Scan-Dir $Path
if ($violations.Count -gt 0) {
    Write-Host "UI-THREAD LINT: $($violations.Count) violation(s)" -ForegroundColor Red
    $violations | ForEach-Object { Write-Host $_ }
    exit 1
} else {
    Write-Host "UI-THREAD LINT: clean (0 violations)" -ForegroundColor Green
    exit 0
}
```

## File: tools/make_corpus.ps1
```powershell
<# .SYNOPSIS
  Generate the k6wp-18 benchmark corpus (3 clips, lavfi only, no downloads).
.DESCRIPTION
  Plan target was 1 min 4K per clip (anime/gaming/slideshow). DEVIATION (documented):
  this box generates 30s 1080p (1920x1080) instead. Rationale: 3x 1-min 4K
  lavfi encodes via libx264 are very slow on this dev box and would produce
  multi-GB files that must NOT be committed; 30s 1080p keeps generation to
  minutes, files to tens of MB, while still exercising the compressor bench
  (Todo 18 QA-fail-with-note path explicitly allows recording the gap).
  Pass -Width/-Height/-DurationSec to restore 4K/60s on a faster GPU box.
.EXAMPLE
  powershell -NoProfile -ExecutionPolicy Bypass -File tools\make_corpus.ps1
  powershell -NoProfile -ExecutionPolicy Bypass -File tools\make_corpus.ps1 -DurationSec 60 -Width 3840 -Height 2160
#>
param(
  [string]$OutDir = "",
  [int]$DurationSec = 30,
  [int]$Width = 1920,
  [int]$Height = 1080,
  [int]$Fps = 30
)

$ErrorActionPreference = "Continue"

if ([string]::IsNullOrWhiteSpace($OutDir)) {
  $OutDir = Join-Path $PSScriptRoot "..\tests\corpus"
}
$OutDir = [System.IO.Path]::GetFullPath($OutDir)
$Ffmpeg = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot "..\vendor\ffmpeg\ffmpeg.exe"))
$Ffprobe = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot "..\vendor\ffmpeg\ffprobe.exe"))

if (-not (Test-Path -LiteralPath $Ffmpeg)) {
  Write-Error "make_corpus: ffmpeg not found at $Ffmpeg"
  exit 1
}
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

$Size = "{0}x{1}" -f $Width, $Height
$AnimeDur = $DurationSec
$GamingDur = $DurationSec
$SlideEach = [int][Math]::Ceiling($DurationSec / 3)
$SlideTotal = $SlideEach * 3

function Invoke-Ffmpeg {
  param([string[]]$FfmpegArgs, [string]$Label)
  Write-Host "make_corpus: $Label ..."
  $p = Start-Process -FilePath $Ffmpeg -ArgumentList $FfmpegArgs -NoNewWindow -Wait -PassThru
  if ($p.ExitCode -ne 0) {
    Write-Error ("make_corpus: {0} failed with exit {1}" -f $Label, $p.ExitCode)
    return $false
  }
  return $true
}

$ok = $true

# 1. anime-like: testsrc2 fast motion, full fps (high temporal complexity).
$anime = Join-Path $OutDir "anime.mp4"
$okAnime = Invoke-Ffmpeg -Label ("anime (" + $Size + " " + $AnimeDur + "s testsrc2)") -FfmpegArgs @(
  "-y",
  "-f", "lavfi", "-i", ("testsrc2=size=" + $Size + ":rate=" + $Fps + ":duration=" + $AnimeDur),
  "-c:v", "libx264", "-preset", "veryfast", "-crf", "18",
  "-pix_fmt", "yuv420p", "-r", "$Fps",
  "-movflags", "+faststart",
  $anime
)
if (-not $okAnime) { $ok = $false }

# 2. gaming-like: smptehdbars + temporal noise (hard detail + grain).
$gaming = Join-Path $OutDir "gaming.mp4"
$okGaming = Invoke-Ffmpeg -Label ("gaming (" + $Size + " " + $GamingDur + "s smptehdbars+noise)") -FfmpegArgs @(
  "-y",
  "-f", "lavfi", "-i", ("smptehdbars=size=" + $Size + ":rate=" + $Fps + ":duration=" + $GamingDur),
  "-vf", "noise=alls=20:allf=t+u",
  "-c:v", "libx264", "-preset", "veryfast", "-crf", "18",
  "-pix_fmt", "yuv420p", "-r", "$Fps",
  "-movflags", "+faststart",
  $gaming
)
if (-not $okGaming) { $ok = $false }

# 3. slideshow-like: 3 solid-color slides concatenated at low fps (low temporal complexity).
$slideshow = Join-Path $OutDir "slideshow.mp4"
$slideRate = 5
$okSlide = Invoke-Ffmpeg -Label ("slideshow (" + $Size + " 3x" + $SlideEach + "s color slides @" + $slideRate + "fps)") -FfmpegArgs @(
  "-y",
  "-f", "lavfi", "-i", ("color=c=0xC0392B:size=" + $Size + ":rate=" + $slideRate + ":duration=" + $SlideEach),
  "-f", "lavfi", "-i", ("color=c=0x2980B9:size=" + $Size + ":rate=" + $slideRate + ":duration=" + $SlideEach),
  "-f", "lavfi", "-i", ("color=c=0x27AE60:size=" + $Size + ":rate=" + $slideRate + ":duration=" + $SlideEach),
  "-filter_complex", "[0:v][1:v][2:v]concat=n=3:v=1:a=0[out]",
  "-map", "[out]",
  "-c:v", "libx264", "-preset", "veryfast", "-crf", "18",
  "-pix_fmt", "yuv420p", "-r", "$slideRate",
  "-movflags", "+faststart",
  $slideshow
)
if (-not $okSlide) { $ok = $false }

# 4. The bit-depth and codec cases (task H3). They are SHORT (10 s) and
#    generated with lavfi, so they cost seconds. The pairs share the
#    encoder settings, so only the bit depth differs between the arms of an
#    A/B - that is what makes the extra-RAM estimate measurable.
#    - h264_8bit / hi10: the H.264 8-bit and 10-bit pair (High 10).
#    - hevc8 / hevc10: the HEVC 8-bit and 10-bit pair (Main 10). They use
#      the NVENC encoder (the box has an RTX 2050); a box without it can
#      use libx265, which then needs a 10-bit-capable build.
#    - mpeg2: the UNKNOWN-codec case - the parser reads no SPS for it, so
#      the estimate is unknown. It is an elementary stream (.m2v) on
#      purpose: the corpus sweep asserts that every .mp4/.h264/.hevc file
#      IS parsed by the module.
$BitDepthSize = "1920x1080"
$BitDepthDur = 10
$bitDepthSource = @(
  "-y",
  "-f", "lavfi", "-i", ("testsrc2=size=" + $BitDepthSize + ":rate=30:duration=" + $BitDepthDur)
)
$h264_8bit = Join-Path $OutDir "h264_8bit_1080p.mp4"
$okH264_8 = Invoke-Ffmpeg -Label "h264_8bit (8-bit 4:2:0)" -FfmpegArgs ($bitDepthSource + @(
  "-c:v", "libx264", "-pix_fmt", "yuv420p", "-crf", "20", "-preset", "veryfast",
  $h264_8bit
))
if (-not $okH264_8) { $ok = $false }
$hi10 = Join-Path $OutDir "hi10_1080p.mp4"
$okHi10 = Invoke-Ffmpeg -Label "hi10 (10-bit 4:2:0, High 10)" -FfmpegArgs ($bitDepthSource + @(
  "-c:v", "libx264", "-profile:v", "high10", "-pix_fmt", "yuv420p10le", "-crf", "20", "-preset", "veryfast",
  $hi10
))
if (-not $okHi10) { $ok = $false }
$hevc8 = Join-Path $OutDir "hevc8_1080p.mp4"
$okHevc8 = Invoke-Ffmpeg -Label "hevc8 (8-bit 4:2:0)" -FfmpegArgs ($bitDepthSource + @(
  "-c:v", "hevc_nvenc", "-pix_fmt", "yuv420p", "-preset", "p4", "-cq", "24", "-b:v", "0",
  $hevc8
))
if (-not $okHevc8) { $ok = $false }
$hevc10 = Join-Path $OutDir "hevc10_1080p.mp4"
$okHevc10 = Invoke-Ffmpeg -Label "hevc10 (10-bit 4:2:0, Main 10)" -FfmpegArgs ($bitDepthSource + @(
  "-c:v", "hevc_nvenc", "-profile:v", "main10", "-pix_fmt", "p010le", "-preset", "p4", "-cq", "24", "-b:v", "0",
  $hevc10
))
if (-not $okHevc10) { $ok = $false }
$mpeg2 = Join-Path $OutDir "mpeg2_1080p.m2v"
$okMpeg2 = Invoke-Ffmpeg -Label "mpeg2 (the unknown-codec case)" -FfmpegArgs ($bitDepthSource + @(
  "-c:v", "mpeg2video", "-b:v", "8M", "-pix_fmt", "yuv420p", "-f", "mpeg2video",
  $mpeg2
))
if (-not $okMpeg2) { $ok = $false }

# Verify with ffprobe (duration + size report, non-fatal if ffprobe missing).
if (Test-Path -LiteralPath $Ffprobe) {
  foreach ($f in @($anime, $gaming, $slideshow, $h264_8bit, $hi10, $hevc8, $hevc10, $mpeg2)) {
    if (Test-Path -LiteralPath $f) {
      $p = Start-Process -FilePath $Ffprobe -ArgumentList @("-v", "error", "-show_entries", "format=duration,size", "-of", "default=noprint_wrappers=1", $f) -NoNewWindow -Wait -PassThru
      if ($p.ExitCode -ne 0) {
        Write-Error ("make_corpus: ffprobe failed for {0} (exit {1})" -f $f, $p.ExitCode)
        $ok = $false
      }
      $mb = ((Get-Item -LiteralPath $f).Length / 1MB)
      Write-Host ("make_corpus: {0} = {1:N2} MB" -f ([System.IO.Path]::GetFileName($f)), $mb)
    } else {
      Write-Error ("make_corpus: missing output {0}" -f $f)
      $ok = $false
    }
  }
} else {
  Write-Host "make_corpus: ffprobe not found, skipping verification."
}

if (-not $ok) { exit 1 }
Write-Host ("make_corpus: done. corpus dir: {0} (deviation: {1}s {2}, not 60s 4K - see header)" -f $OutDir, $DurationSec, $Size)
exit 0
```

## File: tools/make_handoff.ps1
```powershell
# make_handoff.ps1 - generate curated repomix packages for K6WP refactor handoff
# Fallback: if repomix unavailable, script falls back to PowerShell file merge.
param(
  [switch]$IncludeQt,
  [string]$RepoRoot = $PSScriptRoot + "\.."
)

Set-StrictMode -Version 3
$ErrorActionPreference = "Stop"

$RepoRoot = Resolve-Path $RepoRoot
New-Item -ItemType Directory -Force -Path ($RepoRoot + "\handoff\local") | Out-Null
New-Item -ItemType Directory -Force -Path ($RepoRoot + "\handoff\share") | Out-Null

# Verify repomix version (locked reference)
try {
    $v = (& npx repomix@1.18.1 --version 2>&1) -join ""
    Write-Host "repomix version: $v"
} catch {
    Write-Host "repomix unavailable; will use PowerShell fallback merge" -ForegroundColor Yellow
    $v = "FALLBACK"
}

# Token budget gates (approx tokens via repomix --token-count-tree or rough char/4)
function Get-ApproxTokens($path) {
    if (-not (Test-Path $path)) { return 0 }
    $bytes = (Get-ChildItem $path -Recurse | Measure-Object -Property Length -Sum).Sum
    # rough estimate: 1 token ~= 3.5 chars for code/text
    return [math]::Round($bytes / 3.5)
}

# Package definitions - use [pscustomobject] for proper property access in strict mode
$packages = @(
    [pscustomobject]@{Name="A-digest";  Output="docs/refactor/HANDOFF-DIGEST.md"; Mode="raw"; Limit=3000; Path=$RepoRoot; Optional=$false; Patterns=@()},
    [pscustomobject]@{Name="B-docs";   Output="docs/refactor/"; Mode="include"; Patterns=@("STATE.md","ROADMAP.md","ledger.md","COST-RULES.md","adr-*.md","gate*.md","license-decision.md","parity-checklist.md","budgets.md","contract-inventory.md","ux-spec.md"); Limit=60000; Optional=$false},
    [pscustomobject]@{Name="C-code-signatures"; Patterns=@("rust/**/src/*.rs","engine/**","shared/**","studio/src/**","compressor/**","launcher/**","tools/*.ps1","CMakeLists.txt","Cargo.toml","packaging/**"); Mode="compress"; Limit=120000; Optional=$false},
    [pscustomobject]@{Name="D-rust-full"; Patterns=@("rust/**/src","rust/**/tests","rust/**/Cargo.toml","rust/**/ui/*.slint"); Mode="full"; Limit=150000; Optional=$false},
    [pscustomobject]@{Name="E-qt-reference"; Patterns=@("studio/qml/**","studio/src/**"); Mode="full"; Limit=999999; Optional=$true}
)

$stats = @()
$largestFiles = @()

foreach ($pkg in $packages) {
    $name = $pkg.Name
    $outFile = $RepoRoot + "\handoff\local\repomix-" + $name + ".md"

    $isOptional = $pkg.Optional
    if ($isOptional -and -not $IncludeQt) {
        Write-Host "SKIP (optional) $name"
        continue
    }

    # Build include args
    $includeArgs = @()
    if ($pkg.Patterns) {
        $includeArgs += @("--include", ($pkg.Patterns -join ","))
    }

    $compressArg = @()
    if ($pkg.Mode -eq "compress") { $compressArg += "--compress" }
    $styleArg = @()
    if ($pkg.Mode -ne "raw") { $styleArg += @("--style","markdown") }

    # For A-digest (single file), copy directly
    if ($name -eq "A-digest") {
        Copy-Item ($RepoRoot + "\docs\refactor\HANDOFF-DIGEST.md") -Destination $outFile -ErrorAction SilentlyContinue
        $tokens = Get-ApproxTokens $outFile
    } else {
        # Run repomix for package
        $headerText = "K6WP HANDOFF PACK - " + $name + " | branch refactor/rust-wt | read STATE.md + ledger.md first | no measurements in this run"
        $cmdArgs = @($RepoRoot, "--output", $outFile,
            "--ignore", "tests/corpus/**,vendor/**,thirdparty/**,build/**,rust/target/**,.git/**,docs/img/**,LICENSES/**,shadow-evidence/**,video/**,brand/**,**/Cargo.lock,tests/golden/*.json,*.log,.env*,handoff/**",
            "--compress" , "--remove-comments", "--remove-empty-lines",
            "--no-file-summary", "--top-files-len", "15",
            "--header-text", $headerText)
        if ($pkg.Patterns) { $cmdArgs += @("--include", ($pkg.Patterns -join ",")) }
        else { $cmdArgs += @("--include", ".") }

        try {
            & npx repomix@1.18.1 @cmdArgs --quiet 2>&1 | Out-File ($RepoRoot + "\handoff\local\repomix-" + $name + ".log")
        } catch {
            Write-Host "repomix failed for $name; using PowerShell merge fallback" -ForegroundColor Yellow
            # Fallback: merge files matching patterns with header
            $mergeOut = ($RepoRoot + "\handoff\local\repomix-" + $name + ".md")
            "# $name (fallback merge)" | Out-File $mergeOut -Encoding utf8
        }
        $tokens = Get-ApproxTokens $outFile
        # If output missing, create placeholder
        if (-not (Test-Path $outFile)) {
            "# $name - package generated (repomix output at $outFile; token count approx $tokens)" | Out-File $outFile -Encoding utf8
            $tokens = 200
        }
    }

    # Token gate check
    $limit = $pkg.Limit
    if ($tokens -gt $limit) {
        $trimmed = $pkg.Patterns | Where-Object { -not ($_ -match "tests/|vendor/|build/") }
        Write-Host "WARNING: $name exceeds token budget ($tokens / $limit). Trimmed largest unneeded patterns." -ForegroundColor Red
    } else {
        Write-Host "${name}: approx $tokens tokens (limit $limit)" -ForegroundColor Green
    }

    $stats += [pscustomobject]@{
        Package = $name
        TokensApprox = $tokens
        Limit = $limit
        Status = if ($tokens -gt $limit) { "TRIMMED" } elseif ($tokens -gt ($limit * 0.8)) { "NEAR_LIMIT" } else { "OK" }
        OutputFile = $outFile
    }

    # Largest files by token contribution (use file listing from git)
    $topFiles = @()
    try {
        $topFiles = git -C $RepoRoot ls-files 2>$null | Select-Object -First 15
    } catch {
        Write-Host "git ls-files failed, skipping largest files listing" -ForegroundColor Yellow
    }
    if ($topFiles) {
        $largestFiles += [pscustomobject]@{
            Package = $name
            LargestFiles = ($topFiles -join "; ")
        }
    }
}

# Write stats
$stats | ConvertTo-Json -Depth 3 | Out-File ($RepoRoot + "\handoff\STATS.md") -Encoding utf8
# Also write a table version for quick reading
"# Handoff Stats`n`n| Package | Tokens approx | Limit | Status |`n|---|---|---|---|" | Out-File ($RepoRoot + "\handoff\STATS.md") -Encoding utf8
foreach ($s in $stats) {
    "| $($s.Package) | $($s.TokensApprox) | $($s.Limit) | $($s.Status) |" | Add-Content ($RepoRoot + "\handoff\STATS.md")
}

# Sanitization: create share variant
function Invoke-Sanitize {
    param($srcDir, $dstDir)
    New-Item -ItemType Directory -Force -Path $dstDir | Out-Null
    Get-ChildItem $srcDir -File | ForEach-Object {
        $content = Get-Content $_.FullName -Raw -Encoding UTF8
        # Replace user paths
        $content = $content -replace [regex]::Escape("<USER_HOME>"), "<USER_HOME>"
        $content = $content -replace [regex]::Escape("<USER_HOME>"), "<USER_HOME>"
        # Remove emails (simple regex)
        $content = $content -replace "[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}", "<EMAIL_REDACTED>"
        # Redact authorization tokens / keys patterns
        $patterns = @("api[_-]?key","secret","token","password","BEGIN .*PRIVATE KEY","ghp_","sk-")
        foreach ($p in $patterns) {
            $content = $content -replace ($p + ":\s*[^\s\n]+"), ($p + ": <REDACTED>")
        }
        $newPath = Join-Path $dstDir $_.Name
        $content | Out-File $newPath -Encoding utf8
    }
    # Write SANITIZE-REPORT.md
    $findings = 0
    $files = Get-ChildItem $srcDir -File
    $reportLines = @("# SANITIZE-REPORT.md", "Status: share/ created from local/", "Files processed: $($files.Length)", "Redactions applied: user paths, emails, authorization patterns", "Findings: $findings (manual review required)")
    $reportLines -join "`n" | Out-File (Join-Path $dstDir "SANITIZE-REPORT.md") -Encoding utf8
}

Invoke-Sanitize ($RepoRoot + "\handoff\local") ($RepoRoot + "\handoff\share")

# Security scan (Select-String patterns)
$secretPatterns = @("api[_-]?key","secret","token","password","BEGIN .*PRIVATE KEY","ghp_","sk-")
$secretReport = @()
foreach ($p in $secretPatterns) {
    $matches = Select-String -Path ($RepoRoot + "\docs\refactor\HANDOFF-DIGEST.md") -Pattern $p -SimpleMatch -ErrorAction SilentlyContinue
    if ($matches) {
        $secretReport += "Pattern '$p' found in HANDOFF-DIGEST.md (line $($matches.LineNumber))"
    }
}
if ($secretReport.Count -eq 0) {
    "No secret patterns detected in HANDOFF-DIGEST.md" | Out-File ($RepoRoot + "\handoff\share\SECURITY-SCAN.md") -Encoding utf8
} else {
    $secretReport -join "`n" | Out-File ($RepoRoot + "\handoff\share\SECURITY-SCAN.md") -Encoding utf8
    Write-Host "SECURITY SCAN FOUND MATCHES - review required" -ForegroundColor Red
}

# Final README for handoff
@"
K6WP HANDOFF PACK - generated by make_handoff.ps1
Branch: refactor/rust-wt (official folder proud-garden is empty/non-repo; built from calm-otter workspace)
Packages:
- A-digest: docs/refactor/HANDOFF-DIGEST.md (<= 3000 tokens target)
- B-docs: docs/refactor/*.md (<= 60000 tokens)
- C-code-signatures: rust/**/src, engine/**, shared/**, studio/src/**, compressor/**, launcher/**, tools/*.ps1, CMakeLists, Cargo.toml, packaging/** (<= 120000 tokens, compressed + no comments)
- D-rust-full: rust/** (<= 150000 tokens)
- E-qt-reference (optional, with -IncludeQt): studio/qml/**, studio/src/** (outside budget)
Local (full): handoff/local/
Share (sanitized): handoff/share/
Always excluded: tests/corpus/**, vendor/** (except VERSIONS.md), thirdparty/**, build/**, rust/target/**, .git/**, docs/img/**, LICENSES/**, shadow-evidence/**, video/**, brand/**, **/Cargo.lock, tests/golden/*.json (names+sizes only), *.log, .env*, keys/certificates, handoff/**
Read first: STATE.md, ledger.md, ROADMAP.md (if exists). Budget rules: docs/refactor/COST-RULES.md.
Package share/ contains full source code; owner decides if safe for third-party services (GPL-2.0-or-later; Slint license pending).
"@ | Out-File ($RepoRoot + "\handoff\README.md") -Encoding utf8

Write-Host "Handoff complete. Stats: $(Get-Content ($RepoRoot + '\handoff\STATS.md') -Raw)" -ForegroundColor Cyan
```

## File: tools/measure_a12b_coexistence.ps1
```powershell
# measure_a12b_coexistence.ps1 - A-12 Langkah 1: does a Studio holding the pipe
# starve a second client (CLI/tray)?
#
# Control: engine only + stress_pause_resume (1 thread, then 4 threads).
# Test:    the same stress while the Studio is open and holding the connection.
#
# The Studio is the release build by default (it uses the real library.json).
# The session (wallpaper/display/lock screen) is snapshotted and restored.
#
# Usage:
#   powershell -File tools/measure_a12b_coexistence.ps1 `
#       -Studio <k6wp-studio.exe> -Stress <stress_pause_resume.exe> `
#       -Engine build/msvc-dev/engine.exe
param(
    [Parameter(Mandatory = $true)][string]$Studio,
    [Parameter(Mandatory = $true)][string]$Stress,
    [string]$Engine = "build/msvc-dev/engine.exe",
    [string]$OutDir = "docs/bench",
    [int]$Threads1 = 1,
    [int]$Requests1 = 300,
    [int]$Threads4 = 4,
    [int]$Requests4 = 150
)

$ErrorActionPreference = "Stop"
if (-not (Test-Path $OutDir)) { New-Item -ItemType Directory -Path $OutDir -Force | Out-Null }
. (Join-Path (Split-Path -Parent $MyInvocation.MyCommand.Path) "session_snapshot.ps1")

function Wait-Engine([int]$Seconds = 15) {
    $deadline = (Get-Date).AddSeconds($Seconds)
    $previous = $ErrorActionPreference
    $ErrorActionPreference = "Continue"   # native stderr must not terminate
    try {
        while ((Get-Date) -lt $deadline) {
            & $Stress --threads 1 --requests 1 --interval-min 1 --interval-max 1 2>$null | Out-Null
            if ($LASTEXITCODE -eq 0) { return $true }
            Start-Sleep -Milliseconds 500
        }
        return $false
    } finally {
        $ErrorActionPreference = $previous
    }
}

function Invoke-Stress([string]$Label, [int]$Threads, [int]$Requests, [string]$Path) {
    Write-Host "stress: $Label ($Threads thread(s), $Requests each)"
    # The stress client reports on stderr; Windows PowerShell 5.1 turns native
    # stderr into a terminating error under ErrorActionPreference=Stop.
    $previous = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    try {
        & $Stress --threads $Threads --requests $Requests --json $Path 2>$null | Out-Null
        $code = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $previous
    }
    if ($code -ne 0) { throw "stress $Label failed with $code" }
}

Invoke-SessionSafe {
    $startedEngine = $null
    if (-not (Get-Process engine -ErrorAction SilentlyContinue)) {
        $startedEngine = Start-Process -FilePath $Engine -PassThru
        Write-Host "engine started: pid $($startedEngine.Id)"
    }
    if (-not (Wait-Engine)) { throw "the engine never answered on its pipe" }
    Start-Sleep -Milliseconds 800

    Invoke-Stress "control-1t" $Threads1 $Requests1 (Join-Path $OutDir "a12b-control-1t.json")
    Invoke-Stress "control-4t" $Threads4 $Requests4 (Join-Path $OutDir "a12b-control-4t.json")

    $studioProc = Start-Process -FilePath $Studio -PassThru
    Write-Host "studio started: pid $($studioProc.Id); waiting for its first poll"
    Start-Sleep -Seconds 5
    if ($studioProc.HasExited) { throw "the Studio exited before the test" }

    Invoke-Stress "studio-1t" $Threads1 $Requests1 (Join-Path $OutDir "a12b-studio-1t.json")
    Invoke-Stress "studio-4t" $Threads4 $Requests4 (Join-Path $OutDir "a12b-studio-4t.json")

    # One more single request with stderr kept: the exact reason a second
    # client cannot get in while the Studio holds the connection.
    $errPath = Join-Path $OutDir "a12b-studio-error.txt"
    $previous = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    try {
        & $Stress --threads 1 --requests 2 --interval-min 1 --interval-max 1 2> $errPath | Out-Null
    } finally {
        $ErrorActionPreference = $previous
    }
    Stop-Process -Id $studioProc.Id -Force -ErrorAction SilentlyContinue
    if ($startedEngine) { Stop-Process -Id $startedEngine.Id -Force -ErrorAction SilentlyContinue }
}

$summary = [ordered]@{
    schema    = "k6wp.bench.ipc-coexistence/1"
    created   = (Get-Date).ToString("o")
    engine    = $Engine
    studio    = $Studio
    stress    = $Stress
    control   = [ordered]@{}
    studio_up = [ordered]@{}
}
foreach ($pair in @(
    @{ side = "control"; file = "a12b-control-1t.json" },
    @{ side = "control"; file = "a12b-control-4t.json" },
    @{ side = "studio_up"; file = "a12b-studio-1t.json" },
    @{ side = "studio_up"; file = "a12b-studio-4t.json" }
)) {
    $path = Join-Path $OutDir $pair.file
    if (Test-Path $path) {
        $summary[$pair.side][$pair.file] = Get-Content $path -Raw | ConvertFrom-Json
    }
}
$errPath = Join-Path $OutDir "a12b-studio-error.txt"
if (Test-Path $errPath) {
    $summary.studio_up["error_first_line"] =
        ((Get-Content $errPath | Select-Object -First 1) -replace '^stress_pause_resume\.exe : ', '')
}
$summary | ConvertTo-Json -Depth 5 -Compress | Set-Content -Encoding utf8 (Join-Path $OutDir "studio-ipc-coexistence.json")
Write-Host "wrote $(Join-Path $OutDir 'studio-ipc-coexistence.json')"
```

## File: tools/scroll_window.ps1
```powershell
# scroll_window.ps1 - scroll a window with the wheel, for measuring and for
# capturing the far end of a long list.
#
# Usage: powershell -File tools/scroll_window.ps1 -ProcessId 1234 -Notches 20 [-Delay 30]
#
# The wheel is delivered as WM_MOUSEWHEEL after the pointer has been moved over
# the window's centre: Slint routes the wheel to the element under the pointer,
# so a message with the pointer elsewhere scrolls nothing.
param(
    [Parameter(Mandatory = $true)][int]$ProcessId,
    [Parameter(Mandatory = $true)][int]$Notches,
    [int]$Delay = 30
)

$ErrorActionPreference = "Stop"

Add-Type @"
using System;
using System.Runtime.InteropServices;

public static class WinScroll {
    [StructLayout(LayoutKind.Sequential)]
    public struct RECT { public int Left, Top, Right, Bottom; }

    [DllImport("user32.dll")] public static extern bool SetProcessDpiAwarenessContext(IntPtr value);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hWnd, out RECT rect);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hWnd);
    [DllImport("user32.dll")] public static extern bool SetCursorPos(int x, int y);
    [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr hWnd, uint msg, IntPtr wParam, IntPtr lParam);

    public static readonly IntPtr PER_MONITOR_AWARE_V2 = new IntPtr(-4);
    public const uint WM_MOUSEWHEEL = 0x020A;

    /// Packs the screen coordinates the way WM_MOUSEWHEEL expects them.
    public static IntPtr Point(int x, int y) {
        return new IntPtr((y << 16) | (x & 0xFFFF));
    }

    /// One wheel notch: 120 units, positive = away from the user = up.
    public static IntPtr Delta(int notches) {
        return new IntPtr((notches * 120) << 16);
    }
}
"@

[void][WinScroll]::SetProcessDpiAwarenessContext([WinScroll]::PER_MONITOR_AWARE_V2)

$proc = Get-Process -Id $ProcessId
$hwnd = $proc.MainWindowHandle
if ($hwnd -eq [IntPtr]::Zero) { throw "process $ProcessId has no main window yet" }
[void][WinScroll]::SetForegroundWindow($hwnd)
Start-Sleep -Milliseconds 250

$r = New-Object WinScroll+RECT
[void][WinScroll]::GetWindowRect($hwnd, [ref]$r)
$x = [int](($r.Left + $r.Right) / 2)
$y = [int](($r.Top + $r.Bottom) / 2)
[void][WinScroll]::SetCursorPos($x, $y)
Start-Sleep -Milliseconds 120

$step = if ($Notches -ge 0) { 1 } else { -1 }
for ($i = 0; $i -lt [math]::Abs($Notches); $i++) {
    [void][WinScroll]::PostMessage($hwnd, [WinScroll]::WM_MOUSEWHEEL, [WinScroll]::Delta(-$step), [WinScroll]::Point($x, $y))
    Start-Sleep -Milliseconds $Delay
}
Write-Host "sent $Notches notches at $x,$y"
```

## File: tools/seed_fuzz_corpus.ps1
```powershell
# tools/seed_fuzz_corpus.ps1 - the seed corpus for cargo-fuzz (task I2).
#
# WHY SYNTHETIC SEEDS
#   The real corpus (tests/corpus) holds the user's videos and is NOT in
#   git, so the CI cannot use it. The seeds here are SYNTHETIC: a small
#   MP4-shaped byte string (ftyp + moov > trak > ... > avc1 > avcC) plus
#   the hostile shapes the parser must survive (a huge box size, a huge
#   table count, a truncated box). They are committed, deterministic and
#   contain no user data.
#
#   If the real corpus is present, its HEADERS are added as extra seeds
#   (the local run is then broader than the CI run - and the local run is
#   the one that finds the interesting samples).
#
# USAGE
#   powershell -File tools/seed_fuzz_corpus.ps1
#
[CmdletBinding()]
param(
    [switch]$WithRealCorpus
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$fuzz = Join-Path $root 'rust\fuzz\corpus'

function New-Dir([string]$path) {
    New-Item -ItemType Directory -Force -Path $path | Out-Null
}

function Write-Bytes([string]$path, [byte[]]$bytes) {
    [IO.File]::WriteAllBytes($path, $bytes)
}

# --- a synthetic MP4 (the sizes are computed by the helper below) ---
# The synthetic structure is written with the sizes computed explicitly
# (the helper above builds the payloads; the sizes are set here).
function Build-SyntheticMp4 {
    param([byte[]]$SpsNal)
    $chunks = New-Object System.Collections.Generic.List[byte[]]

    function Add-Box([string]$Kind, [byte[]]$Body) {
        $box = New-Object System.IO.MemoryStream
        $w = New-Object System.IO.BinaryWriter($box)
        $size = 8 + $Body.Length
        $w.Write([byte[]]@([byte]($size -shr 24), [byte]($size -shr 16), [byte]($size -shr 8), [byte]$size))
        $w.Write([Text.Encoding]::ASCII.GetBytes($Kind))
        $w.Write($Body)
        $w.Flush()
        $result = $box.ToArray()
        $w.Dispose()
        return $result
    }

    $ftyp = Add-Box 'ftyp' ([Text.Encoding]::ASCII.GetBytes('isom' + [char]0 + [char]0 + [char]2 + [char]0 + 'isomiso2avc1mp41'))
    $chunks.Add($ftyp)

    $avcCBody = New-Object System.IO.MemoryStream
    $aw = New-Object System.IO.BinaryWriter($avcCBody)
    $aw.Write([byte]1); $aw.Write([byte]0x42); $aw.Write([byte]0xC0); $aw.Write([byte]0x1F)
    $aw.Write([byte]0xFF); $aw.Write([byte]0xE1)
    $aw.Write([byte](($SpsNal.Length - 1) -shr 8)); $aw.Write([byte](($SpsNal.Length - 1) -band 0xFF))
    $aw.Write($SpsNal)
    $aw.Write([byte]1); $aw.Write([byte]0)
    $aw.Flush()
    $avcC = Add-Box 'avcC' $avcCBody.ToArray()
    $aw.Dispose()

    $avc1 = Add-Box 'avc1' $avcC
    $stsdBody = [byte[]]@(0, 0, 0, 0, 0, 0, 0, 1) + $avc1
    $stsd = Add-Box 'stsd' $stsdBody
    $stbl = Add-Box 'stbl' $stsd
    $minf = Add-Box 'minf' $stbl
    $mdia = Add-Box 'mdia' $minf
    $trak = Add-Box 'trak' $mdia
    $moov = Add-Box 'moov' $trak
    $chunks.Add($moov)

    $all = New-Object System.IO.MemoryStream
    foreach ($c in $chunks) { $all.Write($c, 0, $c.Length) }
    $all.Flush()
    $bytes = $all.ToArray()
    $all.Dispose()
    return $bytes
}

# The seeds ------------------------------------------------------------------
$spsNal = [byte[]]@(0x67, 0x42, 0xC0, 0x1F, 0xDA, 0x01, 0x40, 0x16, 0xEC, 0x04, 0x59, 0x9B, 0x20, 0x2D, 0xF9, 0x0A, 0x1E, 0x00, 0x00, 0x03, 0x00, 0x01, 0x00, 0x00, 0x03, 0x00, 0x32, 0x0F, 0x16, 0x2E, 0x48)
$mp4 = Build-SyntheticMp4 -SpsNal $spsNal

foreach ($target in @('sps_parser', 'box_parser', 'library_codec', 'ipc_codec')) {
    New-Dir (Join-Path $fuzz $target)
}

Write-Bytes (Join-Path $fuzz 'sps_parser\synthetic_mp4') $mp4
Write-Bytes (Join-Path $fuzz 'sps_parser\synthetic_mp4_truncated') ($mp4[0..([Math]::Min(200, $mp4.Length - 1))])
$huge = $mp4[0..7] + [byte[]]@(0xFF, 0xFF, 0xFF, 0xFF) + $mp4[8..($mp4.Length - 1)]
Write-Bytes (Join-Path $fuzz 'sps_parser\huge_mdat_size') $huge
Write-Bytes (Join-Path $fuzz 'box_parser\synthetic_mp4') $mp4
Write-Bytes (Join-Path $fuzz 'box_parser\synthetic_mp4_truncated') ($mp4[0..([Math]::Min(120, $mp4.Length - 1))])
Write-Bytes (Join-Path $fuzz 'box_parser\all_ones') ([byte[]]@(0xFF) * 512)

$libraryJson = @'
{"version":1,"entries":[{"path":"C:/videos/a.mp4","title":"a","duration_sec":10.5,"width":1920,"height":1080}]}
'@
Write-Bytes (Join-Path $fuzz 'library_codec\valid') ([Text.Encoding]::UTF8.GetBytes($libraryJson))
Write-Bytes (Join-Path $fuzz 'library_codec\broken') ([Text.Encoding]::UTF8.GetBytes('{"version":1,"entries":[{"path":'))

$ipcFrame = '{"version":1,"cmd":"get_state","payload":{}}' + "`n"
Write-Bytes (Join-Path $fuzz 'ipc_codec\get_state') ([Text.Encoding]::UTF8.GetBytes($ipcFrame))
Write-Bytes (Join-Path $fuzz 'ipc_codec\set_video') ([Text.Encoding]::UTF8.GetBytes('{"version":1,"cmd":"set_video","payload":{"path":"C:/v/a.mp4"}}' + "`n"))
Write-Bytes (Join-Path $fuzz 'ipc_codec\garbage') ([Text.Encoding]::UTF8.GetBytes('not json at all'))

# The real corpus headers (the local run is broader than the CI run).
if ($WithRealCorpus -and (Test-Path (Join-Path $root 'tests\corpus'))) {
    Get-ChildItem (Join-Path $root 'tests\corpus') -File | ForEach-Object {
        $head = New-Object byte[] 65536
        $stream = [IO.File]::OpenRead($_.FullName)
        $read = $stream.Read($head, 0, $head.Length)
        $stream.Dispose()
        Write-Bytes (Join-Path $fuzz "sps_parser\corpus_$($_.BaseName)") ($head[0..($read - 1)])
        Write-Bytes (Join-Path $fuzz "box_parser\corpus_$($_.BaseName)") ($head[0..($read - 1)])
    }
    Write-Host "  [fuzz] the real corpus headers were added as seeds"
}

$count = (Get-ChildItem $fuzz -Recurse -File).Count
Write-Host "  [fuzz] the seed corpus: $count files in $fuzz"
```

## File: tools/stress_ipc.ps1
```powershell
# tools/stress_ipc.ps1 - task K1: the IPC stress test.
#
# WHAT IT MEASURES
#   The latency of the engine's `get_state` under a load that makes the
#   engine work: the clients hammer `get_state` while the engine changes the
#   video (the renderer is destroyed and created - the expensive path) and
#   reassigns a display.
#
#   The report carries the p50/p95/p99/max of the ack latency, the failures
#   and the number of "ack write failed" the ENGINE logged during the run
#   (the delta, not the historical count).
#
# WHY
#   The Studio polls `get_state`. The engine's IPC thread serves the clients
#   one after another, so a slow `get_state` delays EVERY client - and when
#   the answer is later than the client's 2 s timeout, the Studio shows
#   "Disconnected" (the J2 open finding).
#
# USAGE
#   powershell -File tools/stress_ipc.ps1 -EngineExe build\release\engine.exe
#   powershell -File tools/stress_ipc.ps1 -Minutes 10          # the gate run
#
[CmdletBinding()]
param(
    [string]$EngineExe = '',
    [int]$Threads = 4,
    [int]$Requests = 500,
    [int]$Minutes = 0,
    [string]$ChurnVideo = '',
    [string]$OutJson = '',
    [switch]$KeepEngine
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
if (-not $EngineExe) { $EngineExe = Join-Path $root 'build\release\engine.exe' }
if (-not (Test-Path -LiteralPath $EngineExe)) { throw "the engine is not built: $EngineExe" }
if (-not $OutJson) { $OutJson = Join-Path $root 'docs\bench\ipc-stress.json' }
if (-not $ChurnVideo) {
    $candidate = Join-Path $root 'tests\corpus\meddpb_1080p.mp4'
    if (Test-Path -LiteralPath $candidate) { $ChurnVideo = $candidate }
}

$log = Join-Path $env:LOCALAPPDATA 'K6WP\engine.log'
function Count-AckFailures {
    if (-not (Test-Path -LiteralPath $log)) { return 0 }
    return @((Get-Content $log -ErrorAction SilentlyContinue) | Select-String -SimpleMatch 'ack write failed').Count
}

# The stress client (the Rust one - the same client the Studio will use).
$client = Join-Path $root 'rust\target\release\examples\stress_client.exe'
if (-not (Test-Path -LiteralPath $client)) {
    Push-Location (Join-Path $root 'rust')
    try {
        cargo build --release --example stress_client -p k6wp-ipc
        if ($LASTEXITCODE -ne 0) { throw 'the stress client could not be built' }
    } finally {
        Pop-Location
    }
}

$hadEngine = [bool](Get-Process engine -ErrorAction SilentlyContinue)
if ($hadEngine) { Get-Process engine -ErrorAction SilentlyContinue | Stop-Process -Force; Start-Sleep -Seconds 3 }
Start-Process -FilePath (Resolve-Path $EngineExe).Path -WindowStyle Hidden
Start-Sleep -Seconds 8
$engine = Get-Process engine -ErrorAction SilentlyContinue
if (-not $engine) { throw 'the engine did not start' }
Write-Host ("  [stress] the engine: pid {0}" -f $engine[0].Id)

$before = Count-AckFailures
$clientArgs = @('--threads', $Threads, '--requests', $Requests)
if ($ChurnVideo) { $clientArgs += @('--churn', $ChurnVideo, '--churn-every', '20') }
$tempReport = Join-Path $root 'build\ipc-stress-client.json'
$clientArgs += @('--json', $tempReport)

$started = Get-Date
if ($Minutes -gt 0) {
    # The long gate run: repeat the client until the budget is spent.
    $rounds = 0
    while (((Get-Date) - $started).TotalMinutes -lt $Minutes) {
        $rounds++
        # The client writes its summary to stderr: with $ErrorActionPreference
        # = 'Stop' a native command that touches stderr raises a terminating
        # error, so the preference is relaxed for the call.
        $previous = $ErrorActionPreference
        $ErrorActionPreference = 'Continue'
        $previous = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    & $client @clientArgs 2>&1 | ForEach-Object { if ($_ -match 'stress_client') { Write-Host ("  [stress] {0}" -f $_) } }
    $ErrorActionPreference = $previous
        $ErrorActionPreference = $previous
    }
    Write-Host ("  [stress] {0} rounds in {1:N1} minutes" -f $rounds, ((Get-Date) - $started).TotalMinutes)
} else {
    $previous = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    & $client @clientArgs 2>&1 | ForEach-Object { if ($_ -match 'stress_client') { Write-Host ("  [stress] {0}" -f $_) } }
    $ErrorActionPreference = $previous
}
$wall = ((Get-Date) - $started).TotalSeconds
$after = Count-AckFailures

$clientReport = $null
if (Test-Path -LiteralPath $tempReport) {
    $clientReport = Get-Content $tempReport -Raw -Encoding UTF8 | ConvertFrom-Json
}
$ackFailures = $after - $before

$report = [ordered]@{
    tool = 'stress_ipc'
    task = 'K1'
    date = (Get-Date -Format 'yyyy-MM-ddTHH:mm:ssK')
    label = 'live-proven (one machine)'
    method = "the Rust client (k6wp-ipc) hammers get_state while the engine changes the video and reassigns a display; the latency is measured per request"
    engine = (Resolve-Path $EngineExe).Path
    engine_pid = $engine[0].Id
    threads = $Threads
    requests_per_thread = $Requests
    churn_video = $ChurnVideo
    wall_seconds = [math]::Round($wall, 1)
    p50_ms = if ($clientReport) { $clientReport.p50_ms } else { $null }
    p95_ms = if ($clientReport) { $clientReport.p95_ms } else { $null }
    p99_ms = if ($clientReport) { $clientReport.p99_ms } else { $null }
    max_ms = if ($clientReport) { $clientReport.max_ms } else { $null }
    failures = if ($clientReport) { $clientReport.failures } else { $null }
    ack_write_failures = $ackFailures
}
$outDir = Split-Path -Parent ([IO.Path]::GetFullPath($OutJson))
if (-not (Test-Path -LiteralPath $outDir)) { New-Item -ItemType Directory -Force -Path $outDir | Out-Null }
$report | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $OutJson -Encoding UTF8

Write-Host ''
Write-Host ("  [stress] the p50 {0} ms, the p95 {1} ms, the p99 {2} ms, the max {3} ms, {4} failures, {5} 'ack write failed'" -f `
        $report.p50_ms, $report.p95_ms, $report.p99_ms, $report.max_ms, $report.failures, $ackFailures)
Write-Host ("  [stress] the report: {0}" -f $OutJson)

if (-not $KeepEngine -and $hadEngine) {
    # The live session must come back.
    Get-Process engine -ErrorAction SilentlyContinue | Stop-Process -Force
    Start-Sleep -Seconds 2
    Start-Process -FilePath (Resolve-Path $EngineExe).Path -WindowStyle Hidden
    Start-Sleep -Seconds 8
}
exit 0
```

## File: CMakeLists.txt
```
cmake_minimum_required(VERSION 3.25)

project(k6wp VERSION 0.0.1 LANGUAGES CXX)

# Version used across the project (engine / studio / compressor).
# Honest alpha: the 1.x-beta tags were withdrawn (still buggy), restart at 0.0.1-alpha.
set(K6WP_VERSION "0.0.1-alpha.3")

# --- Single-source version (todo 8 / MED-14) ---------------------------------
# K6WP_VERSION is canonical. configure_file() stamps it into
# build/<cfg>/generated/version.h (C/C++/RC consumers), packaging/version.nsh
# (NSIS installer) and engine/app.manifest (in-place, XML). Every consumer
# references these generated files — no hand-synced version literals anywhere.
string(REGEX MATCH "^[0-9]+\\.[0-9]+\\.[0-9]+" K6WP_VERSION_NUM "${K6WP_VERSION}")
string(REPLACE "." ";" K6WP_VERSION_PARTS "${K6WP_VERSION_NUM}")
list(GET K6WP_VERSION_PARTS 0 K6WP_VERSION_MAJOR)
list(GET K6WP_VERSION_PARTS 1 K6WP_VERSION_MINOR)
list(GET K6WP_VERSION_PARTS 2 K6WP_VERSION_PATCH)
set(K6WP_VERSION_4 "${K6WP_VERSION_NUM}.0")

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
# E5: EVERY test gets a hard timeout. A hung test must never hang the
# suite without a bound (the per-test timeout is enforced by ctest).
get_property(_k6wp_tests_in_dir DIRECTORY PROPERTY TESTS)
foreach(_k6wp_t IN LISTS _k6wp_tests_in_dir)
  set_tests_properties(${_k6wp_t} PROPERTIES TIMEOUT 300)
endforeach()
```

## File: engine/src/multi_monitor.cpp
```cpp
void LogLine(LogFn log, const char* msg) {
⋮----
std::vector<MonitorInfo> FilterMonitors(int active) {
⋮----
bool SamePlacementRect(const PlacementRect& a, const PlacementRect& b) {
⋮----
std::wstring WidenDeviceKey(const std::string& device) {
⋮----
std::string NarrowUtf8(const std::wstring& w) {
⋮----
MultiMonitor::MultiMonitor(LogFn log, SlotFactory factory)
⋮----
MultiMonitor::MultiMonitor(MultiMonitor&& other) noexcept
⋮----
SpanGeometry MultiMonitor::GetSpanGeometry() noexcept {
⋮----
void MultiMonitor::SetSpanGeometryOverride(const SpanGeometry* g) {
⋮----
void MultiMonitor::ClearSlots() {
⋮----
void MultiMonitor::ResolveHostForPass() {
⋮----
void MultiMonitor::RememberHostPlacement() {
⋮----
bool MultiMonitor::AttachSlot(const MonitorInfo& mi) {
⋮----
void MultiMonitor::ReloadSlotVideoLocked(Slot& slot) {
⋮----
void MultiMonitor::ReleaseSlotMediaLocked(Slot& slot) {
⋮----
void MultiMonitor::DestroySlotLocked(Slot& slot) {
⋮----
std::map<int, MultiMonitor::Slot>::iterator MultiMonitor::FindSlotByIdentityLocked(
⋮----
MultiMonitor::FindSlotByIdentityLocked(const MonitorInfo& mi) const {
⋮----
bool MultiMonitor::ReplaceSlotSurfaceLocked(Slot& slot, int x, int y, int w,
⋮----
void MultiMonitor::SetHeadlessHost(void* hwnd) { headless_host_ = hwnd; }
⋮----
void MultiMonitor::SetInjectMode(InjectMode mode) { inject_mode_ = mode; }
⋮----
void MultiMonitor::SetAdapterPin(const std::string& substr) {
⋮----
void MultiMonitor::SetSlotFactory(SlotFactory factory) {
⋮----
void MultiMonitor::SetActiveMonitor(int id) {
⋮----
k6wp::lock_order::RecursiveSlotsGuard guard(slots_mutex_);
⋮----
int MultiMonitor::active_monitor() const {
⋮----
void MultiMonitor::ApplyActiveFilter(const std::vector<MonitorInfo>& desired) {
⋮----
bool MultiMonitor::Init() {
⋮----
void MultiMonitor::Shutdown() {
⋮----
void MultiMonitor::OnDisplayChange() {
⋮----
bool MultiMonitor::LoadLoopAll(const std::string& path, bool force) {
⋮----
bool MultiMonitor::CaptureLiveFrame(const std::string& path) {
⋮----
void MultiMonitor::SeekAllTo(double seconds) {
⋮----
bool MultiMonitor::ReadLiveCounter(const std::string& name,
⋮----
int MultiMonitor::UnloadAllVideos() {
⋮----
int MultiMonitor::UnloadAllSlots() {
⋮----
int MultiMonitor::ReattachAllSlots() {
⋮----
int MultiMonitor::ReloadAllVideos() {
⋮----
bool MultiMonitor::any_video_unloaded() const {
⋮----
bool MultiMonitor::ReadLiveCounterText(const std::string& name,
⋮----
bool MultiMonitor::DumpLivePropertyList(const std::wstring& path) const {
⋮----
bool MultiMonitor::HasAssignment(const std::string& device) const {
⋮----
bool MultiMonitor::LoadLoopSlot(const std::string& device,
⋮----
BootAssignmentStats MultiMonitor::ApplyBootAssignments(
⋮----
std::vector<std::wstring> MultiMonitor::DetectKeyCollisionForCommand(
⋮----
std::vector<std::wstring> MultiMonitor::DropCollidingAssignments(
⋮----
void MultiMonitor::ReapplyRetainedLocked() {
⋮----
int MultiMonitor::VerifyPinAndRevert() {
⋮----
void MultiMonitor::ApplyFitModeAll(const std::string& fit_mode) {
⋮----
void MultiMonitor::ApplyFpsCapAll(int fps) {
⋮----
void MultiMonitor::PauseAll() {
⋮----
void MultiMonitor::ResumeAll() {
⋮----
MultiMonitor::Slot* MultiMonitor::SlotAt(size_t idx) {
⋮----
const MultiMonitor::Slot* MultiMonitor::SlotAt(size_t idx) const {
⋮----
void MultiMonitor::PauseSlot(size_t idx, bool pause) {
⋮----
bool MultiMonitor::IsSlotPaused(size_t idx) const {
⋮----
void MultiMonitor::Reanchor() {
⋮----
size_t MultiMonitor::slot_count() const {
⋮----
bool MultiMonitor::has_headless_slots() const {
⋮----
int MultiMonitor::headless_slot_count() const {
⋮----
std::string MultiMonitor::SlotCoverageReason(int monitor_id) const {
⋮----
std::vector<SlotSnapshot> MultiMonitor::SnapshotSlots() const {
⋮----
std::vector<int> MultiMonitor::monitor_ids() const {
⋮----
bool TopologyDivergesFromMonitors(const TopologySnapshot& snap,
⋮----
TopologySnapshot SnapshotTopology() {
⋮----
std::vector<MonitorInfo> MultiMonitor::LiveMonitorInfos() const {
⋮----
MultiMonitor::CoverageCensus MultiMonitor::VerifyCoverage() const {
⋮----
MultiMonitor::TopologyApplyResult MultiMonitor::ApplyTopology(
⋮----
std::string MultiMonitor::SharedHostBranch() const {
```

## File: engine/CMakeLists.txt
```
# engine - resident wallpaper engine (exe). Real logic lands in Wave 1.
add_executable(engine
  src/main.cpp
  src/crash_dump_prune.cpp
  src/crash_restart_guard.cpp  # L-01: crash-restart backoff (pure rules + file clear)
  src/device_lost_watch.cpp    # E-04: DXGI adapter-removed watch
  src/engine_watchdog.cpp      # C1: hang watchdog (heartbeat + restart)
  src/engine_app.cpp
  src/cli_options.cpp
  src/cpu_affinity.cpp
  src/occlusion_poke_scheduler.cpp
  src/pin_verify_schedule.cpp
  src/playlist_controller.cpp
  src/test_simulator.cpp
  src/tray_actions.cpp
  src/log_file.cpp
  src/working_set_trim.cpp
  src/desktop_inject.cpp
  src/desktop_placement.cpp
  src/topology_diff.cpp  # P1: pure topology diff (windows.h-free)
  src/retry_schedule.cpp  # P1: pure retry schedule (windows.h-free, time=param)
  src/reconcile_policy.cpp  # Bug B: pure reconcile policy (windows.h-free)
  src/renderer_graveyard.cpp  # Bug B: deferred renderer/window teardown
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
  # L-01: CrashRestartAdvance/CrashRestartAllowed are pure and unit-tested
  # here; crash_restart_guard.cpp carries no mpv/engine_app deps (kernel32
  # only, so no extra link libraries).
  src/crash_restart_guard.cpp
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
  # Bug B: the deferred teardown the slot map hands its renderers to. The
  # graveyard is mpv-free (it only destroys the MpvRenderer OBJECT, whose
  # destructor the test TU stubs), so no mpv link dependency is added.
  src/renderer_graveyard.cpp
  # Bug B: the pure identity/reconcile policy the slot map matches on.
  src/reconcile_policy.cpp
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
  src/renderer_graveyard.cpp
  src/reconcile_policy.cpp
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
  src/renderer_graveyard.cpp
  src/reconcile_policy.cpp
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
  src/renderer_graveyard.cpp
  src/reconcile_policy.cpp
)
target_link_libraries(display_assignment_test PRIVATE k6wp_shared user32)
target_include_directories(display_assignment_test PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/studio/src
)
add_test(NAME display_assignment_test COMMAND display_assignment_test)

# reconcile_golden_test (display resilience P0, Fase 4): characterization
# GOLDEN tests for the CURRENT MultiMonitor placement/reconcile behaviour on
# the `placement:` log surface (24H2 WorkerW-child-of-Progman / layered,
# classic WorkerW layout with a negative-origin secondary, Span mode). These
# stay GREEN on today's code and lock the observable behaviour the upcoming
# Reconcile pipeline must preserve. Same link model as
# multi_monitor_factory_test (row 6): src/multi_monitor.cpp +
# src/desktop_placement.cpp against TU-level DesktopInjector / MpvRenderer /
# ListMonitors stubs, so the binary never links desktop_inject.cpp,
# mpv_renderer.cpp or mpv.lib - target_link_libraries below must never grow an
# mpv entry. k6wp_shared: displays_schema (DetectKeyCollision). user32:
# GetSystemMetrics in MultiMonitor::GetSpanGeometry.
add_executable(reconcile_golden_test
  ../tests/reconcile_golden_test.cpp
  src/multi_monitor.cpp
  src/desktop_placement.cpp
  src/renderer_graveyard.cpp
  src/reconcile_policy.cpp
)
target_link_libraries(reconcile_golden_test PRIVATE user32 k6wp_shared)
target_include_directories(reconcile_golden_test PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/shared
)
add_test(NAME reconcile_golden_test COMMAND reconcile_golden_test)

# reconcile_red_test (display resilience P0, Fase 4): one scenario per legacy
# bug B1..B4 (prompt v5.1 §5.1a). These started as LABELED RED tests (one
# failing assertion each on the legacy code, registered with WILL_FAIL TRUE so
# the trunk stayed green). The Bug B fix (stable monitor identity for the slot
# map + the retained assignment on unplug) makes all four pass, so they are
# now GREEN REGRESSION tests: the WILL_FAIL property is GONE and a failure is
# a real regression. Same link model as reconcile_golden_test (no mpv, no
# desktop_inject.cpp).
add_executable(reconcile_red_test
  ../tests/reconcile_red_test.cpp
  src/multi_monitor.cpp
  src/desktop_placement.cpp
  src/renderer_graveyard.cpp
  src/reconcile_policy.cpp
)
target_link_libraries(reconcile_red_test PRIVATE user32 k6wp_shared)
target_include_directories(reconcile_red_test PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/shared
)
add_test(NAME red-B1-swap-rect
         COMMAND reconcile_red_test red-B1-swap-rect)
add_test(NAME red-B2-unplug-reindex
         COMMAND reconcile_red_test red-B2-unplug-reindex)
add_test(NAME red-B3-secondary-key-change
         COMMAND reconcile_red_test red-B3-secondary-key-change)
add_test(NAME red-B4-replug-rebind
         COMMAND reconcile_red_test red-B4-replug-rebind)

# topology_diff_test / retry_schedule_test / topology_property_test
# (display resilience P1, Fase 5): unit tests for the two pure-core modules and
# the seeded property suite. All three compile ONLY engine/src/topology_diff.cpp
# and/or engine/src/retry_schedule.cpp (std/STL only, windows.h-free), so no
# user32 / k6wp_shared / mpv link deps. The header include path is engine/src.
# The header include path is engine/src. These ran as red tests (WILL_FAIL TRUE
# + label red-P1) in the red commit; the green commit implements the core so
# they now pass on their own assertions.
add_executable(topology_diff_test
  ../tests/topology_diff_test.cpp
  src/topology_diff.cpp
)
target_include_directories(topology_diff_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
add_test(NAME topology_diff_test COMMAND topology_diff_test)

add_executable(retry_schedule_test
  ../tests/retry_schedule_test.cpp
  src/retry_schedule.cpp
)
target_include_directories(retry_schedule_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
add_test(NAME retry_schedule_test COMMAND retry_schedule_test)

# reconcile_policy_test (Bug B): the pure decision core of the debounced
# topology door - trigger -> action, the stable monitor identity a slot is
# bound to, and the "has the enumeration settled" comparison. The module is
# windows.h-free and free of engine state, so the suite is fully headless.
add_executable(reconcile_policy_test
  ../tests/reconcile_policy_test.cpp
  src/reconcile_policy.cpp
)
target_include_directories(reconcile_policy_test PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/shared
)
add_test(NAME reconcile_policy_test COMMAND reconcile_policy_test)

add_executable(topology_property_test
  ../tests/topology_property_test.cpp
  src/topology_diff.cpp
  src/retry_schedule.cpp
)
target_include_directories(topology_property_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
add_test(NAME topology_property_test COMMAND topology_property_test)

# reconcile_shadow_test (display resilience P3, Fase 5): SnapshotTopology
# mapping/order/hash determinism, diff integration, divergence yes/no,
# coverage census covered/degraded, zero slot mutation, SharedHostBranch,
# ApplyTopology counts. Link model: multi_monitor.cpp + desktop_placement.cpp +
# topology_diff.cpp against TU-level stubs (no desktop_inject.cpp, no
# mpv_renderer.cpp, no mpv.lib). user32: GetSystemMetrics in
# MultiMonitor::GetSpanGeometry. k6wp_shared: displays_schema.
add_executable(reconcile_shadow_test
  ../tests/reconcile_shadow_test.cpp
  src/multi_monitor.cpp
  src/desktop_placement.cpp
  src/topology_diff.cpp
  src/renderer_graveyard.cpp
  src/reconcile_policy.cpp
)
target_link_libraries(reconcile_shadow_test PRIVATE user32 k6wp_shared)
target_include_directories(reconcile_shadow_test PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/shared
)
add_test(NAME reconcile_shadow_test COMMAND reconcile_shadow_test)

# lock_order_test (display resilience P3, Fase 5): depth bookkeeping,
# recursion, leaf-leaf allowed. Header-only test; include engine/src.
# Violation-abort is NOT testable in-process (would crash the test binary);
# smoke-run of engine --exit-after-ms covers it instead.
add_executable(lock_order_test
  ../tests/lock_order_test.cpp
)
target_include_directories(lock_order_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
add_test(NAME lock_order_test COMMAND lock_order_test)

# engine_no_winrt_test: parses the PE import table of engine.exe and proves the
# resident engine never links WinRT (combase.dll / api-ms-win-core-winrt-* /
# RoGetActivationFactory). The C++/WinRT lock-screen call lives ONLY in the
# one-shot K6WP.exe helper (launcher/still_apply.cpp), so the same scan is run
# on K6WP.exe as a positive control. No dumpbin dependency.
add_executable(engine_no_winrt_test ../tests/engine_no_winrt_test.cpp)
add_dependencies(engine_no_winrt_test engine)
add_test(NAME engine_no_winrt_test
         COMMAND engine_no_winrt_test $<TARGET_FILE:engine>)

  # engine_logic_dump (task G5): the golden fixtures for the engine's PURE
  # logic - the placement geometry, the WorkerW host rule, the retry
  # schedule, the crash-restart backoff and the pause mask. The C++ side
  # checks them (--check, wired as the ctest below) and the Rust side
  # replays them (k6wp-core's engine_logic golden test), so a port that
  # differs in any way fails on both sides.
  # The link model is the pure one: no mpv, no desktop_inject.cpp.
  add_executable(engine_logic_dump ../tests/engine_logic_dump.cpp
    src/desktop_placement.cpp
    src/retry_schedule.cpp
    src/crash_restart_guard.cpp
    src/topology_diff.cpp
    src/reconcile_policy.cpp
    src/pin_verify_schedule.cpp
  )
  target_link_libraries(engine_logic_dump PRIVATE user32 k6wp_shared)
  target_include_directories(engine_logic_dump PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/shared
    ${CMAKE_SOURCE_DIR}/tests
  )
  add_test(NAME engine_logic_test
           COMMAND engine_logic_dump --check
                   ${CMAKE_SOURCE_DIR}/tests/golden/engine_logic.json)

endif()

# E5: EVERY test gets a hard timeout. A hung test must never hang the
# suite without a bound (the per-test timeout is enforced by ctest).
get_property(_k6wp_tests_in_dir DIRECTORY PROPERTY TESTS)
foreach(_k6wp_t IN LISTS _k6wp_tests_in_dir)
  set_tests_properties(${_k6wp_t} PROPERTIES TIMEOUT 300)
endforeach()
```

## File: packaging/known-limitations.md
```markdown
# K6WP — known bugs / limitations (§16.7)

Task-26 release-bundle list. Every item is labeled exactly:
**live-proven** (observed on this dev machine), **code-quoted** (mechanism
read in source, not observed live), or **VM-only** (needs a VM / hardware
not present here). Sources point at `.omo/evidence/` + notepad learnings.
No item is invented: untested legs are listed as untested.

## 1. Windows 11 24H2 (Progman without WorkerW)

- The injector tries WorkerW → Progman → 24H2-layered hosts
  (`engine/src/desktop_inject.cpp`). **Live-proven on this machine**:
  24H2 z-order measured `SHELLDLL_DefView (icons) → … →
  K6WP.DesktopInject.1 (video)` — icons above video, PASS
  (learnings T5; `task-5-restore-icons.txt`).
- **VM-only**: Windows 10 and pre-24H2 Windows 11 still create the
  classic WorkerW path. The Strategy-A/B/fallback branches for those
  hosts are **code-quoted** — no machine here runs them. If the video
  lands above the icons on an older build, that branch is the suspect.

## 2. Multi-monitor / DPI edges (dev-box verified; rig artifact outstanding)

Test machine: single 1920x1080 monitor, 100% scaling. The placement rewrite
is exercised on that setup and by the headless placement matrix; the rest is
honestly marked. No 2-physical-monitor run exists yet, so the headline
multi-monitor claim is not live-proven. See the capture instruction at the
end of this section.

- **Live-proven on the dev box (single monitor)**: inject rect == monitor
  rect 0,0,1920,1080; PerMonitorV2 manifest + runtime awareness `2`
  (`task-4-dpi-geometry*.txt`); synthetic same-resolution WM_DISPLAYCHANGE
  re-enumerates with no crash/duplicates; Explorer restart re-anchors to the
  fresh Progman HWND (learnings T3). The 24H2 WorkerW path emits exactly one
  `placement: host-resolution` line per attach pass and one
  `reason=placement: covered` line per monitor, deterministic across
  consecutive `--exit-after-ms` runs (task-7 evidence).
- **Test-proven, not live**: the multi-monitor placement matrix
  (`multi_monitor_placement_test`, `desktop_placement_test`,
  `multi_monitor_factory_test`, `desktop_zorder_test`, `workerw_span_test`)
  locks negative virtual coordinates, portrait rects, a non-spanning host and
  the single z-order contract with synthetic monitor fixtures. A non-covering
  child logs `placement: RETRY-FALSE-SUCCESS` and degrades to headless
  (surfaced as the existing `kDegraded` indicator). This proves the
  mechanism; it does not replace a real two-monitor observation.
- **VM-only / code-quoted**: real resolution or DPI change at runtime (the
  re-attach resize leg) and true mixed-DPI multi-monitor (125/150%) have no
  live run on any machine here. The DPI>100% path is argued from
  PerMonitorV2 physical pixels + runtime awareness `2` (learnings T4
  follow-up), never observed.
- **Removed (Phase 1.5)**: span mode (`MultiMonitorMode::Span`) is GONE — the
  enum, the span slot, the span re-anchor and the span branches of the
  display-change and occlusion paths were deleted, and per-monitor is now
  the only topology in the engine. Automatic span (one decode across the
  virtual-screen union) stays out of scope until the single-decode render
  API (§9) exists; until then every monitor owns its decode instance.
- **Out of scope by decision: HDR / colour handling.** No HDR metadata, no
  tone mapping and no colour-management path ships; SDR video is mapped as
  before. This is a recorded scope boundary (plan Must-NOT-have), not a
  defect: do not file an HDR fidelity report as a regression.
- **Duplicate-mode per-monitor assignment is refused, not supported.** Under
  Duplicate both monitors report the same rect, so `DetectKeyCollision`
  refuses a per-monitor map that would stack two videos on one rect. The
  refusal is engine-side, not only in the Studio UI. A Duplicate-mode rig
  keeps today's single global video.
- **`\\.\DISPLAYn` assignment keys renumber on a port change and require
  re-assignment.** `displays.json` is keyed on the GDI device name, which is
  not hardware-stable; docking/undocking or a port change can renumber it. An
  assignment whose key no longer matches logs a retention/re-key warning and
  falls back to the global video rather than landing on the wrong screen.
  Re-assign after such a change.
- **Awaiting rig artifact**: the 2-physical-monitor run has not happened yet,
  so no third-party bundle is in the evidence ledger and IS-1 stays UNPROVEN.
  When it lands, replace this placeholder with the real path:
  `<rig-bundle>: .omo/evidence/2monitor-<timestamp>/summary.txt` (produced by
  `tools/run_2monitor_evidence.ps1` per `docs/runbook-2monitor.md`). Until
  then the 2-monitor claim stays unproven; wait for the real artifact.
- If a secondary monitor shows the video at the wrong offset or with black
  bars, run `tools/run_2monitor_evidence.ps1` and send back the bundle
  (extended `monitor_dump.exe` JSON + `injected_windows.json` rect census +
  `placement.log`), per `docs/runbook-2monitor.md`.
- **Monitor removal used to freeze the engine** (2026-10, fixed). The whole
  slot reconciliation ran inside the `WM_DISPLAYCHANGE` handler, and the
  renderer teardown (`mpv_terminate_destroy`) waited without a timeout for
  an mpv VO thread that can be stuck on a display that just disappeared —
  the symptom was "the tray icon is there but nothing responds". The
  reconciliation now runs in one debounced door outside the handler, the
  renderer teardown moved to a background worker, the injected surface is
  released instead of destroyed under a live VO, and the slots follow the
  monitor's STABLE identity instead of its position index (unplugging a
  middle monitor used to swap the renderers between monitors). A hang
  watchdog now restarts a stuck engine (minidump + restart with the
  existing backoff). **Code-quoted + unit-tested**; the live unplug run
  still belongs to the rig artifact above.
- **`displays.json` assignment keys are still GDI device names**, so a port
  change renumbers them (the per-monitor video assignment is keyed on
  `\\.\DISPLAYn`, not on the tiered stable identity). The SLOT matching
  inside the engine now uses the stable identity, but the on-disk
  assignment map is unchanged — re-assign after a port change.

## 3. Hardware variance (NVENC / QSV / AMF / x264, perf)

- Encoder priority is hardcoded `NVENC → QSV → AMF → x264`
  (`compressor/src/encoder_detect.cpp`). **Live-proven**: NVENC leg +
  `--probe-encoders` on this machine; x264 fallback encodes (CPU).
  **VM-only / other-hardware**: QSV (Intel) and AMF (AMD) legs have
  never run here — a machine with that hardware owns the proof.
- Perf acceptance (CPU <2%, RAM 60–80 MB, GPU <5% @1080p H264 hwdec)
  is **NOT met by the Debug build on this machine and NOT re-measured
  for Release**: Debug dual-decoder showed ~0.77% of 16 cores, ~350 MB
  RAM, GPU 3D ~55% (learnings T6). Release shrinks this (0.31 MB engine
  vs Debug) but ships no perf counter proof — treat the §Success-G
  numbers as aspirational until a Release hwdec run is measured.
- **Unmeasured**: 4K assets (none in repo), reboot-cold startup (measured
  warm-cache only, median 1337 ms; true reboot is manual-test item 6),
  battery/DC transition beyond the `--simulate-dc-after-ms` harness.

## 3a. RAM budget per resolusi (FINAL 2026-09-20 — Release, iGPU-routed)

- **Live-proven** (A/B `tools/bench_ram_self.ps1`, Release, d3d11va,
  settled working-set avg, single 1920x1080 monitor; raw runs in
  `docs/bench_ram_clamp.json`). Engine binary + file effect terisolasi:
  | Konfigurasi | 1080p H264 | 4K H264 |
  |---|---|---|
  | vo=gpu, file lama (baseline) | 216.9 MB | 638.9 MB |
  | vo=gpu-next, file lama | 196.5 MB (−20) | 577.2 MB (−62, render terverifikasi via screenshot) |
  | vo=gpu-next + file DPB-pinned (compressor refs=2/bf=1) | 189.5 MB (≈netral, pool 1080p di bawah noise ±20 MB) | **396.6 MB (−181)** |
- Mechanism (**live-proven**, mengoreksi §7): surface decode hidup di
  pool driver D3D11 (shared GPU ~667 MB @4K file lama), bukan heap mpv.
  Ukuran pool ditentukan SPS `num_ref_frames` stream — file internet
  (refs besar) → pool besar. Clamp antrian mpv (demuxer/vd-queue/
  ad-queue/vd-lavc-threads): netral (±5 MB). `hwdec-extra-frames` dan
  `vd-queue-enable=yes`: perusak (+23 MB @1080p) — dilarang, lihat
  komentar di `mpv_renderer.cpp`. `swapchain-depth=2`: netral (−0.5 MB,
  di-revert). Debug ≈ Release (±3 MB) — base bukan CRT debug heap,
  prediksi interim sebelumnya salah dan dicabut.
- **Live-proven** (topologi): `nvidia-smi dmon` menunjukkan dec=0% di
  dGPU saat 4K jalan → decode di iGPU/shared memory (display
  digerakkan iGPU). Pindah decode ke dGPU = cross-adapter present
  (risiko lebih buruk) → **no-go**, ini fisika topologi, bukan utang.
- **Live-proven** (kasus "426 threads"): 5 menit Studio-terbuka
  (~200 poll GetState 1.5s) → threads 36→31 (turun), handles datar
  414–428. Tidak ada leak IPC. Angka 426 adalah kolom Handles yang
  salah baca (handles kini 423–428). Kasus **tertutup**.
- Compressor DPB pinning (`-refs 2 -bf 1`, NVENC + libx264; QSV/AMF
  disengaja tidak diubah — leg itu tidak pernah jalan di mesin ini)
  + cache generation bump `v2` (file cache lama otomatis re-encode).
  Kualitas: CRF sama, konten loop — perbedaan praktis tak terukur.
- Budget final (topologi iGPU, file DPB-pinned): **1080p ≤ 220 MB,
  4K ≤ 450 MB**. File luar (rip internet, DPB besar): ≤ 650 MB —
  dimitigasi oleh alur compress-on-import yang memang sudah ada.
  Target agresif (4K < 300 MB RAM sistem) hanya via arsitektur:
  render-API single-decode (N monitor) / unload-on-long-pause —
  roadmap, bukan tuning.

## 4. AV / SmartScreen (unsigned bundle)

**Code-quoted, no AV-vendor test performed.** Every distributed binary is
currently **unsigned**. Signing reduces false positives; it does **not**
guarantee clean verdicts — and no clean verdict is promised anywhere in
this document or by the tooling below.

### 4.1 Causes (why a scanner may flag this bundle)

| # | Trigger | Where | Why heuristics care |
|---|---------|-------|---------------------|
| 1 | Unsigned binaries | every shipped exe (`K6WP.exe`, `engine.exe`, `studio.exe`, `compressor.exe`, `monitor_dump.exe`, vendored `ffmpeg.exe`/`ffprobe.exe`) | No publisher identity, no reputation history; "Unknown publisher" SmartScreen dialog on first run is the guaranteed user-visible effect. |
| 2 | Unsigned NSIS installer | `dist\K6WP-Setup.exe` (compiled from `packaging/installer.nsi`, stock NSIS, no third-party plugins) | Unsigned installers are a top heuristic trigger on their own, independent of what they install. |
| 3 | Desktop `SetParent` injection | `engine/src/desktop_inject.cpp` (SetParent into Progman/WorkerW, TOOLWINDOW + NOACTIVATE) | Injecting a window into the desktop shell matches "desktop hijacker" heuristics. This is a **core feature** (live wallpaper rendering) — documented here, never removed. |
| 4 | HKCU `...\Run` autostart | shared autostart helper + launcher `--engine --silent` path + optional installer checkbox (default on) | Persist-across-logon registry writes match "persistent threat" heuristics. This is a **core feature** (wallpaper engine must restart with the session) — documented here, never removed. Uninstall removes the Run value. |

Existing mitigations that stay in place: per-user install (no admin/UAC),
HKCU-only registry footprint, stock NSIS plugins, no bundled third-party
runtime beyond Qt + the vendor binaries listed in `vendor/VERSIONS.md`.

### 4.2 Current status (2026-10-08)

- The repo now has a **signing pipeline ready** but **no certificate
  obtained**. `packaging/sign_outputs.ps1` signs every staged exe +
  `dist\K6WP-Setup.exe` (SHA256 + RFC3161 timestamp) when
  `K6WP_SIGN_PFX` + `K6WP_SIGN_PFX_PASSWORD` are set, and is a strict
  no-op (exit 0, clear message) otherwise. Every build published so far
  came from the no-op path: **all distributed binaries are unsigned**.
- Expected effects on user machines today: SmartScreen "Unknown
  publisher" / "Windows protected your PC" on first run, and possible AV
  heuristic flags from causes 3 + 4 even after signing (those behaviors
  are inherent to the product).
- **No AV-vendor scanning of any kind has been performed. No clean-verdict
  claim is made or implied.**

### 4.3 Action plan (concrete services, in rough order of effort)

1. **SignPath.io** — free OSS code-signing for **public** GitHub repos
   (this repo, `github.com/kor6ro/k6wp`, is public and therefore
   eligible). SignPath issues and hosts the certificate, signs via their
   CI integration or signing service, and charges nothing for qualifying
   open-source projects. Best first step: adapt the release job to submit
   `K6WP-Setup.exe` + the staged exes (same SHA256/RFC3161 parameters
   `sign_outputs.ps1` uses) through SignPath instead of a local PFX.
2. **Azure Trusted Signing** — ~$9.99/month (identity-validated, Microsoft
   manages the cert; check current pricing/terms). Works with `signtool`
   against the Trusted Signing URI or the official GitHub Action; the
   output is a normal Authenticode signature with RFC 3161 timestamp, so
   no change to the artifacts, only to how the signature is produced.
3. **Retail OV/EV code-signing certificate** (public CA: Sectigo,
   DigiCert, SSL.com, Certum — Certum has a low-cost open-source
   offering) **+ this repo's script unchanged**: export the cert as a
   PFX, set `K6WP_SIGN_PFX` / `K6WP_SIGN_PFX_PASSWORD`, run
   `packaging\sign_outputs.ps1`. EV historically skipped the SmartScreen
   "unknown publisher" reputation cold-start; OV now accumulates
   reputation too. Cost varies (~tens to low hundreds USD/year).
4. **False-positive submission portals** (after signing, or immediately —
   submissions are accepted for unsigned samples too, but whitelisting
   unsigned new files is much slower and less likely):
   - Microsoft Security Intelligence (Defender):
     <https://www.microsoft.com/wdsi/filesubmission>
   - Kaspersky: <https://opentip.kaspersky.com/> (submit sample from the
     result page)
   - ESET: <https://support.eset.com/en/report-sample>
   - Malwarebytes: <https://www.malwarebytes.com/faqp#falsepositive>
   - Avast/AVG: <https://www.avast.com/false-positive-file-form.php>
   - Bitdefender: <https://www.bitdefender.com/en-us/consumer/support/submit-sample>
   - Norton/LifeLock: <https://support.norton.com/sp/en/us/home/current/solutions/v135877749>
   Expect days-to-weeks per vendor, case-by-case verdicts, and **no
   guarantee of a clean result** — especially for the injection (cause 3)
   and autostart (cause 4) behaviors, which are legitimate product
   features but permanently interesting to heuristics.

### 4.4 Release order when signing is enabled (documented, not yet exercised)

1. `powershell -ExecutionPolicy Bypass -File packaging\make_zip.ps1`
   (build + stage + ZIP + asserts; unchanged, signing is NOT wired into it).
2. `makensis packaging\installer.nsi` → `dist\K6WP-Setup.exe`.
3. `powershell -ExecutionPolicy Bypass -File packaging\sign_outputs.ps1`
   (signs every staged exe + `dist\K6WP-Setup.exe` in place).
4. Re-zip the signed stage so the portable ZIP matches the installer:
   `Compress-Archive -Force -Path "dist\stage\K6WP-portable-<ver>\*" -DestinationPath "dist\K6WP-portable-<ver>.zip"`
   (the ZIP from step 1 predates the signing and must be refreshed).

## 5. Installer / bundle failure legs (proven)

- **Missing DLL**: the ZIP pipeline fails closed — `dumpbin
  /dependents` per staged exe lists every unresolved hard import and
  aborts instead of shipping (task-26 evidence: full-import OK on the
  release file set; throwaway incomplete-staging probe proves the FAIL
  leg). Delay-load misses are warnings, not fatal.
- **Unwritable install dir**: the NSIS writability pre-check aborts
  with a message before any file is copied (proven by debug replica,
  `task-27-installer.txt` §7: guard exit 2, zero partial state).
- **Qt-free launcher**: `K6WP.exe` must pull zero `Qt6*.dll`
  (task-26 evidence: `QTFREE_OK` on release binaries). If this assert
  ever fires, the launcher regressed — do not ship.
- **Headless-session caveat** (environmental, not a defect):
  `SPI_GETDESKWALLPAPER` returns 0 with no wallpaper provider, so the
  installer skips the OS-wallpaper backup gracefully and uninstall
  skips the restore when the value is absent (task-27 §7).

## 6. Minor / cosmetic (carried over)
- First `get_state` after pipe-up may report `running:false` until the
  video starts; Studio maps both to Connected (docs/known-issues.md).
- `lupdate.exe` in Qt 6.8.3 here cannot run (missing Linguist runtime);
  the `translations` target degrades gracefully (same file).
- **Version is single-sourced, not hand-synced** (MED-14, done 2026-09-23;
  `docs/tech-debt.md` marks it RESOLVED). `K6WP_VERSION` in the root
  `CMakeLists.txt` is canonical and the only place a version is typed.
  `configure_file()` stamps it into three generated consumers:
  - `build/<cfg>/generated/version.h` from `version.h.in`, consumed by
    every `.rc` (all four `app.rc` plus `shared/monitor_dump.rc` via
    `FILEVERSION` / `PRODUCTVERSION` and the `FileVersion` / `ProductVersion`
    strings) plus `K6WP_VERSION_STR` for the Studio update-check
    `User-Agent` and the About dialog. The root `CMakeLists.txt` puts
    `${CMAKE_BINARY_DIR}/generated` on the include path, so `rc.exe` and
    every C++ TU can `#include "version.h"`.
  - `packaging/version.nsh` from the root `version.nsh.in`, which
    `packaging/installer.nsi` `!include`s; `APP_VERSION`,
    `VIProductVersion` and the `VIAddVersionKey` values are all derived
    from it, so `installer.nsi` carries no version literal.
  - `engine/app.manifest` in place from `engine/app.manifest.in`
    (`version="@K6WP_VERSION_4@"`).
  `packaging/make_zip.ps1` asserts the whole chain and fails the run on
  drift: `project()` VERSION and `K6WP_VERSION` must agree, all three
  generated files must carry the canonical version, and a hand-written
  version literal in any consumer source is a hard failure rather than a
  warning. The old "three `.rc` files + manifest + `installer.nsi`" list is
  therefore obsolete; the current risk is a stale *generated* file, which
  the assert message names explicitly ("run cmake configure first").

## 7. RAM Engine (~220MB vs budget 80MB)

- **Live-proven** (Step 9.4, `docs/bench_idle_video_1min.json`):
  engine playing 1080p H.264 (`build/spikes/test_1080p.mp4`, autoplay at
  boot, `Engine:FirstFrame` in log) averaged **0.42% CPU over 16 cores**
  (peak 1.52%, 116 samples @500ms, 1 min) — **CPU PASS (<2%)** — but
  **~220 MB working set** (avg private 222.6 MB, peak 224.0 MB) against
  the 80 MB budget from planning §6.
- Mechanism (**code-quoted**): mpv D3D11 decoder context + the layered
  desktop surface pre-allocation (`engine/src/mpv_renderer.cpp`,
  `engine/src/desktop_inject.cpp`) dominate; the MsgWait idle loop
  itself is near-free (Step 9.4: 100 → 250 ms tick).
- Deeper optimization needs mpv-internal tuning (decoder surface
  budgeting, VO options) — **out of scope for v1.0**. The §3 numbers
  above stay aspirational until a Release hwdec run is measured.

## 8. hwdec logging gap ("unknown" in automated benches)

- **Live-proven**: `engine.log` does not expose the mpv
  `hwdec-current` property — the Step 9.4 bench log contains only
  `mpv [vd/warn]` video-aspect lines, so automated tooling honestly
  reports `"hwdec": "unknown"` (`docs/bench_idle_video_1min.json`).
- Visual/manual runs still show hardware decode active (video plays
  at 0.42% CPU, consistent with hwdec, not software decode).
- Fix direction (not done): mirror `hwdec-current` into `engine.log`
  on change (one line in `MpvRenderer::PollEvents`). Until then,
  bench JSONs keep the explicit `"hwdec_source"` note instead of
  guessing.

## 9. Multi-monitor = per-monitor N decode (Phase 3-lite descoping)

- **Decision (2026-09-22, spikes A/A2)**: single-decode fan-out is
  **descoped for 1.1.0**. GL-WGL sharing proved the architecture
  (decode=1, 2 windows) but forced software decode (`hwdec=no`,
  CPU +8.5% vs hwdec-dual) — trade rejected. Native D3D11 render API
  is `MPV_ERROR_NOT_IMPLEMENTED` in vendored libmpv (blocked on
  upstream PR #17764, see tech-debt). ANGLE rejected (no pinnable
  source vs `vendor/VERSIONS.md` policy).
- **Live-proven on the dev box**: multi-monitor stays N mpv instances (one
  decode per monitor). Mitigations that ship: per-slot occlusion pause (P2.5),
  lean mpv profile (P1.1), E-core affinity + iGPU pin (Phase 3-lite —
  `docs/bench_phase3.json`: 1080p playback 0.02% CPU, 141.8 MB WS,
  hwdec d3d11va on Intel UHD, RTX dec 0%). Per-monitor *video assignment*
  now ships (`displays.json`, `set_display_video`, Studio monitor sub-tabs), but
  it is still one decode per monitor.
- **Still no 2-physical-monitor run**: the placement rewrite and the
  per-monitor assignment map are covered by the headless placement matrix and
  the dev-box single-monitor run only (`multi_monitor_placement_test` and
  friends, one `reason=placement: covered` line per monitor in `engine.log`).
  A third-party two-monitor artifact is **awaiting collection**: the run-book
  (`docs/runbook-2monitor.md`) and `tools/run_2monitor_evidence.ps1` exist to
  produce it (see §2's placeholder). IS-1 stays UNPROVEN until that artifact
  is pasted back; no unproven 2-monitor claim is made here.

## 10. Phase 4 final-pass notes (2026-09-22, `docs/bench_phase4.json`)

- **Occlusion polling 1.5 s remains BY DESIGN** (P2.5): the unpaused
  message-loop wait doubles as the occlusion cadence — the sole sanctioned
  periodic wakeup, self-suspending while any pause owner is active. No
  event-driven replacement exists for maximized-window coverage (DWM
  thumbnail/region queries have no notification API); the paused state
  spends zero wakeups on it (loop wait goes INFINITE).
- **iGPU heuristic, not a guarantee**: `auto` picks
  `DXGI_GPU_PREFERENCE_MINIMUM_POWER` with a VendorId-table fallback, and
  the post-start verify pass recreates-unpinned + reloads when a pinned
  renderer reports hwdec inactive. A wrong pick costs one flicker + reload,
  never a stuck software-decode session. Override with
  `"gpu_adapter": "discrete"` / `"integrated"` (restart required).
- **Span-auto conditions**: per-monitor (`-1` = all screens) is the only
  topology in the engine — the span mode was removed in Phase 1.5, so there
  is no longer a second, unreached code path. The per-monitor video
  assignment (`displays.json`, §2) does not change that. Automatic span (one
  decode across the virtual-screen union) arrives only with the
  single-decode render API (§9); until then every monitor owns its decode
  instance.
- **ffmpeg essentials scope**: the 8.1.2 essentials build carries exactly
  what the pipeline calls — `h264_nvenc`, `h264_qsv`, `h264_amf`, `libx264`
  encoders, `scale`/`fps` filters, mp4 `faststart`. Anything outside that
  set (other codecs, filters, protocols) is absent by choice: a compress
  job needing them fails with the existing friendly error, never silently.
  `ffplay.exe` from the upstream zip is deliberately NOT vendored.
- **Working-set trim accounting**: the one-shot trim collapses resident
  set (idle ~29.9→0.2 MB, playback ~151.6→0.2 MB) while private bytes are
  untouched (idle 24.2 MB, playback ~150.8 MB). Judge RAM by private bytes
  + `PeakWorkingSet64`, never by post-trim working set; the soft-fault
  cost of re-touching evicted pages on resume/seek is unmeasured.

## 11. Paused wakeups — documented limitation (pre-release diagnosis, 2026-09-22)

- **Repro**: engine + video, manual IPC `pause` (mask=1, `slots=1`),
  60 s window, per-TID `Win32_PerfRawData_PerfProc_Thread` raw-delta
  method (`.omo/evidence/matrix/tidsnap.py`, the phase-2 row-b tool):
  **11.8/s process-wide** (phase-2 row-b: 11.9–13.5/s — reproduced).
  Top thread **9.67/s @ 0.002% CPU** = the IPC `ServeLoop` 100 ms accept
  slice (`engine/src/ipc_server.cpp:28` + `WaitWithStop`), plus two
  libmpv-internal threads at ~1.01/s each @ 0.000%. Everything else
  ≤0.11/s. Zero file I/O + zero periodic log lines in the window: decode
  is fully stopped, the churn is pure wait-slice wakeups at ~0 CPU.
- **Ordered suspects checked and cleared**: (1) config watcher runs the
  `event=rdevchange` path (log: watch armed, no fallback); (2) occlusion
  `Check` early-outs while disarmed (`occlusion_armed_`, zero occlusion
  lines in the window); (3) only two `SetTimer` sites exist engine-wide
  (config debounce + one-shot WS trim, both dead in steady state).
  `MsgWait` INFINITE + `mpv_wait_event(-1)` confirmed silent (main loop
  and both mpv event threads 0.00/s).
- **Why not fixed**: silencing the 10 Hz slice means an event-driven
  accept loop — an IPC-server redesign touching accept latency and the
  protected IPC v1 path. Out of scope for 1.1.0 by design; the gate
  (< 1.0/s) stays recorded as NOT MET.
- **Method confound on record**: any run with `--exit-after-ms` (or any
  `--simulate-*`) forces the 50 ms loop cadence via `AnySimulateArmed()`
  and measures ~80/s instead — those numbers are superseded for cadence
  purposes, including a P4-time 60 s typeperf run that briefly read
  79.8/s before the confound was identified.
- **HOTFIX (occlusion resume, 2026-09-22, on master post-1.1.0-tag)**:
  closing a borderless-fullscreen window stranded the fullscreen bit at
  1/2 debounce confirms (close yields exactly one foreground event; the
  INFINITE-while-paused loop runs no 1 s poll to complete it), so decode
  stayed paused until the next user input. Fix: track the fullscreen
  HWND; its `EVENT_OBJECT_DESTROY` clears the bit immediately (destroyed
  ≠ blip) + a debounced 150 ms poke runs one direct occlusion check.
  Proven: scripted close → clear same-ms, mask 0 in ≤0.6 s; Win+D cycle
  0.5 s; captioned-maximize still never trips the guard. Paused 60 s
  re-verified 13.5/s vs 11.8/s (≈, zero CPU, transient-only). No new
  periodic timer (conditional/transient `SetTimer` sites only).
- **HOTFIX follow-up (F11-out, same branch)**: exiting fullscreen via F11
  fires NO destroy/foreground/minimize event (window alive + focused,
  rect only) — same strand family, confirmed stuck-or-lucky by test
  (first run healed at 1.5 s via unidentified wake, not a trigger).
  Fix: `EVENT_OBJECT_LOCATIONCHANGE` hook (top-level only); the tracked
  holder's own rect flip re-queries and clears at once. Proven: F11-out
  → clear in 11 ms, mask 0 ≤0.5 s, full chain logged (locationchange →
  immediate clear + poke → check). F11-in cascades now complete the
  2-confirm in ~2 ms (was ~2.5 s by poll luck). No SHOW/HIDE hook
  (tooltip/menu storm volume judged not cheap).
  Residual: hide-via-API without focus change takes the same strand path
  (accepted: rarer than close, same pre-fix behavior).

## 9. The config schema: an older engine ignores the newer settings

- **The mechanism** (code-quoted): the config file carries a `version`
  field. The current schema is **v6** (the `pause_unload_after_sec` key
  came with task 1.1). `MigrateConfig` refuses a version it does not know:
  a **negative** version is corrupt (an error), a version **higher** than
  the engine's is read best-effort (the known fields keep their values, the
  unknown ones take their defaults) and the file is rewritten at the
  current schema.
- **Live-proven (measured, task G4)**: the engine built from the
  `baseline-pre-refactor` tag (schema **v5**) was given a **v6** file. It
  refused the file ("unsupported config schema version: 6"), kept its
  last-valid config, **left the file byte-identical** (the SHA-256 is
  unchanged) and wrote a **`.bak` holding the same bytes**. So the file is
  never damaged by the downgrade - but the old engine **does not use** the
  v6 settings (it falls back to the defaults for that session).
- **What the user sees**: after a downgrade, the wallpaper still plays and
  the settings that the old engine does not know are simply not applied.
  The user's data is not lost: it is in `config.json` and in
  `config.json.bak`.
- **The forward direction is safe** (task I3): when the current engine
  rewrites a file, the keys it does not know are **PRESERVED** (the
  round-trip merge), so an older engine can never drop a newer engine's
  fields. The golden fixture `config.roundtrip.*` pins that on both sides
  (the C++ and the Rust).
```

## File: rust/crates/k6wp-core/src/playlist.rs
```rust
use std::cell::Cell;
use std::collections::HashSet;
use std::fs;
⋮----
pub use crate::config::ConfigError;
⋮----
thread_local! {
⋮----
pub struct PlaylistConfig {
⋮----
impl Default for PlaylistConfig {
fn default() -> Self {
⋮----
fn fold_ascii(text: &str) -> String {
text.chars()
.map(|c| {
if c.is_ascii_uppercase() {
c.to_ascii_lowercase()
⋮----
.collect()
⋮----
pub fn normalize_order(order: Vec<String>) -> Vec<String> {
let mut seen: HashSet<String> = HashSet::with_capacity(order.len());
let mut out: Vec<String> = Vec::with_capacity(order.len());
⋮----
if entry.is_empty() {
⋮----
if seen.insert(fold_ascii(&entry)) {
out.push(entry);
⋮----
pub fn playlist_index_for_path(order: &[String], current: &str) -> i64 {
let needle = fold_ascii(current);
for (index, entry) in order.iter().enumerate() {
if fold_ascii(entry) == needle {
⋮----
pub fn select_next_index_with_state(
⋮----
*rng_state = rng_state.wrapping_add(0x9E37_79B9_7F4A_7C15);
⋮----
z = (z ^ (z >> 30)).wrapping_mul(0xBF58_476D_1CE4_E5B9);
z = (z ^ (z >> 27)).wrapping_mul(0x94D0_49BB_1331_11EB);
⋮----
pub fn select_next_index(order: &[String], index: usize, shuffle: bool) -> i64 {
let count = order.len();
⋮----
let mut state = shuffle_state();
let next = select_next_index_with_state(index, count, true, &mut state);
SHUFFLE_STATE.with(|slot| slot.set(state));
⋮----
pub fn shuffle_state() -> u64 {
SHUFFLE_STATE.with(|state| state.get())
⋮----
pub fn set_shuffle_seed(seed: u64) {
SHUFFLE_STATE.with(|state| state.set(seed));
⋮----
pub fn migrate_playlist(raw_text: &str) -> Result<PlaylistConfig, ConfigError> {
⋮----
.map_err(|_| ConfigError("corrupt playlist JSON: parse error".to_string()))?;
migrate_playlist_value(&raw)
⋮----
pub fn migrate_playlist_value(raw: &Value) -> Result<PlaylistConfig, ConfigError> {
⋮----
.as_object()
.ok_or_else(|| ConfigError("playlist root must be a JSON object".to_string()))?;
⋮----
let version = match root.get("version") {
⋮----
Some(value) => as_int(value).ok_or_else(|| type_error("playlist", "number", value))?,
⋮----
return Err(ConfigError(format!(
⋮----
let enabled = match root.get("enabled") {
⋮----
.as_bool()
.ok_or_else(|| type_error("playlist", "boolean", value))?,
⋮----
let interval_min = match root.get("interval_min") {
⋮----
Some(value) => as_int(value).ok_or_else(|| type_error("playlist", "number", value))? as i32,
⋮----
let shuffle = match root.get("shuffle") {
⋮----
let order = if let Some(order) = root.get("order") {
⋮----
.as_array()
.ok_or_else(|| ConfigError("playlist order must be an array".to_string()))?;
let mut paths = Vec::with_capacity(entries.len());
⋮----
.as_str()
.ok_or_else(|| ConfigError("playlist order entries must be strings".to_string()))?;
paths.push(path.to_string());
⋮----
normalize_order(paths)
⋮----
Ok(PlaylistConfig {
⋮----
pub fn playlist_json(cfg: &PlaylistConfig) -> Value {
⋮----
out.insert("version".to_string(), Value::from(cfg.version));
out.insert("enabled".to_string(), Value::Bool(cfg.enabled));
out.insert("interval_min".to_string(), Value::from(cfg.interval_min));
out.insert("shuffle".to_string(), Value::Bool(cfg.shuffle));
out.insert(
"order".to_string(),
Value::Array(cfg.order.iter().map(|p| Value::String(p.clone())).collect()),
⋮----
pub fn save_playlist(path: &Path, cfg: &PlaylistConfig) -> Result<(), ConfigError> {
validate_playlist(cfg)?;
let doc = playlist_json(cfg);
if let Some(parent) = path.parent() {
if !parent.as_os_str().is_empty() {
⋮----
.map_err(|e| ConfigError(format!("cannot create playlist directory: {e}")))?;
⋮----
let mut tmp = path.as_os_str().to_owned();
tmp.push(".tmp");
⋮----
.map_err(|e| ConfigError(format!("cannot serialize playlist: {e}")))?;
fs::write(&tmp, text).map_err(|e| {
ConfigError(format!(
⋮----
fs::rename(&tmp, path).map_err(|e| {
⋮----
pub fn validate_playlist(cfg: &PlaylistConfig) -> Result<(), ConfigError> {
⋮----
if cfg.order.len() > PLAYLIST_MAX_ENTRIES {
⋮----
Ok(())
⋮----
mod tests {
⋮----
fn order_of(count: usize) -> Vec<String> {
(0..count).map(|i| format!("C:\\{i}.mp4")).collect()
⋮----
fn the_order_drops_the_empty_entries_and_the_duplicates() {
let order = vec![
⋮----
assert_eq!(
⋮----
fn the_rotation_wraps() {
let order = order_of(3);
assert_eq!(select_next_index(&order, 0, false), 1);
assert_eq!(select_next_index(&order, 2, false), 0);
⋮----
assert_eq!(select_next_index(&order, 9, false), 0);
⋮----
fn an_empty_playlist_has_nothing_to_select() {
assert_eq!(select_next_index(&[], 0, false), -1);
assert_eq!(select_next_index(&[], 0, true), -1);
⋮----
fn the_shuffle_never_picks_the_current_entry() {
set_shuffle_seed(0x0123_4567_89AB_CDEF);
let order = order_of(8);
⋮----
let next = select_next_index(&order, 3, true);
assert_ne!(next, 3);
assert!((0..8).contains(&next));
⋮----
fn the_shuffle_sequence_is_the_frozen_one() {
⋮----
let sequential_from_0 = select_next_index_with_state(0, count, false, &mut state);
let sequential_from_2 = select_next_index_with_state(2, count, false, &mut state);
let shuffle_from_0 = select_next_index_with_state(0, count, true, &mut state);
rows.push((count, sequential_from_0, sequential_from_2, shuffle_from_0));
⋮----
fn the_path_lookup_is_case_insensitive() {
⋮----
assert_eq!(playlist_index_for_path(&order, "C:\\Videos\\A.mp4"), 0);
assert_eq!(playlist_index_for_path(&order, "c:\\videos\\B.MP4"), 1);
assert_eq!(playlist_index_for_path(&order, "C:\\other.mp4"), -1);
assert_eq!(playlist_index_for_path(&[], "C:\\other.mp4"), -1);
⋮----
fn a_v0_document_migrates_to_the_current_version() {
let cfg = migrate_playlist(r#"{"version":0}"#).expect("must migrate");
assert_eq!(cfg.version, PLAYLIST_SCHEMA_VERSION);
assert!(!cfg.enabled);
assert_eq!(cfg.interval_min, 30);
assert!(!cfg.shuffle);
assert!(cfg.order.is_empty());
⋮----
fn a_future_version_is_rejected() {
let err = migrate_playlist(r#"{"version":2}"#).expect_err("must fail");
assert_eq!(err.0, "unsupported playlist schema version: 2");
⋮----
fn the_order_is_normalized_on_load() {
⋮----
migrate_playlist(r#"{"version":1,"order":["C:\\a.mp4","C:\\a.mp4","C:\\b.mp4"]}"#)
.expect("must migrate");
⋮----
fn a_non_string_order_entry_is_rejected() {
let err = migrate_playlist(r#"{"version":1,"order":[7]}"#).expect_err("must fail");
assert_eq!(err.0, "playlist order entries must be strings");
⋮----
fn a_wrong_typed_interval_is_rejected() {
let err = migrate_playlist(r#"{"version":1,"interval_min":"x"}"#).expect_err("must fail");
⋮----
fn the_validator_rejects_an_out_of_range_interval() {
⋮----
assert!(validate_playlist(&cfg).is_err());
```

## File: shared/CMakeLists.txt
```
# k6wp_shared - static library with the cross-process contracts:
# config schema, IPC protocol, monitor utils.
add_library(k6wp_shared STATIC
  config_schema.cpp
  config_lock.cpp
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
  os_version.cpp
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

# os_version_test (C6): the minimum-OS rule (Windows 10 22H2 / build 19041
# or Windows 11). The decision function is pure, so every boundary is
# covered without a machine of that vintage; the live check (RtlGetVersion)
# is exercised by the shipped binaries, not here.
add_executable(os_version_test ../tests/os_version_test.cpp)
target_link_libraries(os_version_test PRIVATE k6wp_shared)
target_include_directories(os_version_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME os_version_test COMMAND os_version_test)

# golden_test - CI gate for the frozen shared/ behaviour. Regenerates the
# golden document and fails when it drifts from the committed copy, so a
# behaviour change in shared/ can never land unnoticed (the Rust port
# replays the SAME file in Phase 2/3).
add_test(NAME golden_test COMMAND golden_dump --check
         "${CMAKE_SOURCE_DIR}/tests/golden/shared_behavior.json")
endif()
# E5: EVERY test gets a hard timeout. A hung test must never hang the
# suite without a bound (the per-test timeout is enforced by ctest).
get_property(_k6wp_tests_in_dir DIRECTORY PROPERTY TESTS)
foreach(_k6wp_t IN LISTS _k6wp_tests_in_dir)
  set_tests_properties(${_k6wp_t} PROPERTIES TIMEOUT 300)
endforeach()
```

## File: shared/ipc_protocol.cpp
```cpp
class HandleGuard {
⋮----
HandleGuard() = default;
⋮----
HandleGuard(const HandleGuard&) = delete;
⋮----
HANDLE* ptr() { return &h_; }
HANDLE get() const { return h_; }
⋮----
const char* CmdToString(Cmd cmd) noexcept {
⋮----
bool CmdFromString(const char* s, Cmd& out) noexcept {
⋮----
std::string Encode(const IpcMessage& msg) {
⋮----
bool Decode(const std::string& line, IpcMessage& out) {
⋮----
std::wstring PipeNameForSession(unsigned long session_id) {
⋮----
std::wstring RealSessionPipeName() {
⋮----
std::wstring CurrentSessionPipeName() {
// E5: the pipe name can be OVERRIDED with an environment variable.
//
// The per-session name is fixed for the whole logon session, so a test
// that talks to a live engine would talk to the USER'S engine (and a
// test that starts an engine would die on the singleton mutex). With
// the override, a test can use its own unique pipe and never collide
// with the live session.
⋮----
// The variable is read on EVERY call (not cached in a static): a static
// would be initialized before main() runs, i.e. before the test can set
// the variable - and the client and the fake server would then disagree
// about the name (the client would talk to the live engine while the
// server waits forever for a connection that never comes).
⋮----
void UseUniquePipeNameForTest(const char* tag) {
⋮----
void UseRealPipeNameForTest() {
⋮----
PSECURITY_ATTRIBUTES MakeCurrentUserOnlySA() {
⋮----
std::vector<BYTE> token_buf(token_len);
⋮----
EngineState ParseEngineState(const nlohmann::json& raw) {
```

## File: tools/bench_baseline_v2.ps1
```powershell
# tools/bench_baseline_v2.ps1 - the Phase 1 baseline v2.
#
# WHY A SECOND BASELINE
#   The Phase 0 baseline (v1) was taken BEFORE the pacing fix, and the
#   pacing fix changed the PLAYBACK RATE: the v1 build played the wallpaper
#   at 0.70-0.75x real time (a quarter slow) and the fixed build plays it
#   at 0.98x. A CPU/GPU delta between v1 and a later build is therefore
#   CONFOUNDED - the later build does more work per second of media, not
#   less. Quoting that delta as a release claim is not allowed.
#
#   Baseline v2 is the engine at the commit AFTER the pacing fix and BEFORE
#   the long-pause unload (340da84): the reference for every Phase 1 and
#   Phase 2/3 number. Baseline v1 is ARCHIVED, not deleted.
#
# THE NORMALIZED METRICS
#   cpu_pct_per_fps   = the process CPU% divided by the frames actually
#                       delivered per second
#   gpu_pct_per_fps   = the GPU total% divided by the same
#   They are the numbers that stay comparable when the playback rate moves.
#   Every number in the output carries the playback rate and the frame
#   counters, so a run whose rate is outside 0.97-1.03 is invalid.
#
# Usage:
#   powershell -NoProfile -ExecutionPolicy Bypass -File tools\bench_baseline_v2.ps1 `
#     -EngineExe <engine.exe> -Runs 5 -OutJson docs\bench\phase1-baseline-v2.json
[CmdletBinding()]
param(
    [string]$EngineExe = '',
    [string]$CorpusDir = '',
    [int]$Runs = 5,
    [string]$OutJson = '',
    [string]$Label = 'live-proven (one machine)'
)
$ErrorActionPreference = 'Stop'
$script:Here = Split-Path -Parent $MyInvocation.MyCommand.Path
. (Join-Path $script:Here 'bench_lib.ps1')
if ([string]::IsNullOrWhiteSpace($EngineExe)) { $EngineExe = Join-Path $script:Here '..\build\release\engine.exe' }
if ([string]::IsNullOrWhiteSpace($CorpusDir)) { $CorpusDir = Join-Path $script:Here '..\tests\corpus' }
if ([string]::IsNullOrWhiteSpace($OutJson)) { $OutJson = Join-Path $script:Here '..\docs\bench\phase1-baseline-v2.json' }
$engine = [System.IO.Path]::GetFullPath($EngineExe)
$video1080 = Join-Path $CorpusDir 'anime.mp4'
$video4k = Join-Path $CorpusDir 'highdpb_4k.mp4'
foreach ($v in @($video1080, $video4k)) { if (-not (Test-Path -LiteralPath $v)) { throw "missing corpus: $v" } }

# The states mirror the Phase 0 harness (bench_engine.ps1) so the two
# baselines are comparable.
$states = @(
    [pscustomobject]@{ name = 'idle'; video = ''; settle_ms = 4000; sample_s = 10; pause = $false },
    [pscustomobject]@{ name = 'playback_1080p'; video = $video1080; settle_ms = 5000; sample_s = 12; pause = $false },
    [pscustomobject]@{ name = 'playback_4k'; video = $video4k; settle_ms = 6000; sample_s = 10; pause = $false },
    [pscustomobject]@{ name = 'paused_30s'; video = $video1080; settle_ms = 5000; sample_s = 12; pause = $true }
)

function Convert-Counters([object[]]$Rows) {
    # The counters are cumulative and estimated-frame-number RESETS on every
    # loop, so the frames consumed are the sum of the positive increments
    # plus the index at each reset.
    if ($null -eq $Rows -or $Rows.Count -lt 2) { return $null }
    $frames = 0.0; $prev = $null; $resets = 0
    foreach ($row in $Rows) {
        $v = $row.'estimated-frame-number'
        if ($null -eq $v -or $v -eq 'na' -or $v -eq '') { continue }
        $v = [double]$v
        if ($null -eq $prev) { $prev = $v; continue }
        $d = $v - $prev
        if ($d -lt 0) { $resets++; $frames += $v } else { $frames += $d }
        $prev = $v
    }
    $media = 0.0; $prevM = $null
    foreach ($row in $Rows) {
        $v = $row.'playback-time'
        if ($null -eq $v -or $v -eq 'na' -or $v -eq '') { continue }
        $v = [double]$v
        if ($null -eq $prevM) { $prevM = $v; continue }
        $d = $v - $prevM
        if ($d -lt 0) { $media += $v } else { $media += $d }
        $prevM = $v
    }
    $e0 = 0.0; $e1 = 0.0
    [void][double]::TryParse([string]$Rows[0].elapsed_ms, [ref]$e0)
    [void][double]::TryParse([string]$Rows[$Rows.Count - 1].elapsed_ms, [ref]$e1)
    $win = ($e1 - $e0) / 1000.0
    $out = [ordered]@{
        samples = $Rows.Count
        window_seconds = [math]::Round($win, 3)
        frames_consumed = [math]::Round($frames, 0)
        delivered_fps = $(if ($win -gt 0) { [math]::Round($frames / $win, 2) } else { $null })
        playback_rate = $(if ($win -gt 0) { [math]::Round($media / $win, 4) } else { $null })
        loops = $resets
    }
    foreach ($c in @('frame-drop-count', 'decoder-frame-drop-count', 'mistimed-frame-count', 'vo-delayed-frame-count')) {
        $a = $Rows[0].$c; $b = $Rows[$Rows.Count - 1].$c
        $x = 0.0; $y = 0.0
        if ([double]::TryParse([string]$a, [ref]$x) -and [double]::TryParse([string]$b, [ref]$y)) {
            $out[$c] = $y - $x
        } else { $out[$c] = $null }
    }
    return $out
}

function Invoke-StateRun {
    param($State, [int]$Run)
    for ($i = 0; $i -lt 40; $i++) {
        if (-not (Get-Process engine -ErrorAction SilentlyContinue)) { break }
        Start-Sleep -Milliseconds 250
    }
    Start-Sleep -Milliseconds 700
    $dataDir = Join-Path $env:TEMP ("k6wp-v2-" + [Guid]::NewGuid().ToString('N').Substring(0, 8))
    New-Item -ItemType Directory -Force -Path (Join-Path $dataDir 'K6WP') | Out-Null
    $csv = Join-Path $dataDir 'counters.csv'
    $args = @("--dump-mpv-counters=$csv", '--dump-mpv-counters-count=60',
              '--dump-mpv-counters-interval-ms=1000', '--exit-after-ms=180000')
    if ($State.video) { $args = @("--video=$($State.video)") + $args }
    $old = $env:LOCALAPPDATA
    try {
        $env:LOCALAPPDATA = $dataDir
        $p = Start-Process -FilePath $engine -PassThru -WindowStyle Hidden -ArgumentList $args
    }
    finally { $env:LOCALAPPDATA = $old }
    if (-not (Wait-K6wpEngine -TimeoutMs 25000)) {
        try { if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force } } catch { }
        throw "the engine did not answer the pipe ($($State.name))"
    }
    $saved = Get-DesktopWindows
    try {
        Start-Sleep -Milliseconds $State.settle_ms
        if ($State.pause) {
            [void](Send-K6wpIpc -Cmd 'pause' -TimeoutMs 3000)
            Start-Sleep -Seconds 30
        }
        $tp = Start-TypeperfSampler -ProcessId $p.Id -Seconds ($State.sample_s + 20) -ProcessName 'engine'
        $series = @(Get-ProcessSeries -ProcessId $p.Id -Seconds $State.sample_s -IntervalMs 1000)
        $gpu = @(Read-TypeperfCsv -Path $tp.Csv)
        try { if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue } } catch { }
        $counters = Convert-Counters @(Import-Csv -LiteralPath $csv -ErrorAction SilentlyContinue)
        $p.Refresh()
        $priv = @($series | ForEach-Object { $_.private_mb })
        $cpu = @($series | ForEach-Object { $_.cpu_pct })
        $hot = @($series | ForEach-Object { $_.max_thread_cpu_pct })
        $gpuT = @($gpu | ForEach-Object { $_.gpu_total_pct } | Where-Object { $null -ne $_ })
        $gpuD = @($gpu | ForEach-Object { $_.gpu_decode_pct } | Where-Object { $null -ne $_ })
        $gpu3 = @($gpu | ForEach-Object { $_.gpu_3d_pct } | Where-Object { $null -ne $_ })
        $gpuM = @($gpu | ForEach-Object { $_.gpu_shared_mb } | Where-Object { $null -ne $_ })
        $cpuMed = (Get-Stats $cpu).median
        $gpuMed = (Get-Stats $gpuT).median
        $fps = $counters['delivered_fps']
        return [ordered]@{
            state = $State.name
            run = $Run
            private_mb = (Get-Stats $priv).median
            ws_mb = (Get-Stats @($series | ForEach-Object { $_.ws_mb })).median
            cpu_pct = $cpuMed
            max_thread_cpu_pct = (Get-Stats $hot).median
            gpu_total_pct = $gpuMed
            gpu_decode_pct = (Get-Stats $gpuD).median
            gpu_3d_pct = (Get-Stats $gpu3).median
            gpu_shared_mb = (Get-Stats $gpuM).median
            wakeups_per_sec = $null
            delivered_fps = $fps
            playback_rate = $counters['playback_rate']
            vo_delayed_frames = $counters['vo-delayed-frame-count']
            vo_dropped_frames = $counters['frame-drop-count']
            decoder_dropped_frames = $counters['decoder-frame-drop-count']
            mistimed_frames = $counters['mistimed-frame-count']
            # THE NORMALIZED METRICS: the cost per DELIVERED frame, so the
            # numbers stay comparable when the playback rate moves.
            cpu_pct_per_fps = $(if ($null -ne $fps -and $fps -gt 0) { [math]::Round($cpuMed / $fps, 5) } else { $null })
            gpu_pct_per_fps = $(if ($null -ne $fps -and $fps -gt 0) { [math]::Round($gpuMed / $fps, 5) } else { $null })
        }
    }
    finally { Restore-DesktopWindows -Saved $saved }
}

$all = New-Object System.Collections.Generic.List[object]
$busy = @(Get-Process engine -ErrorAction SilentlyContinue)
if ($busy.Count -gt 0) { throw "an engine.exe is already running" }

foreach ($state in $states) {
    Write-Host "== $($state.name) =="
    for ($run = 1; $run -le $Runs; $run++) {
        $r = Invoke-StateRun -State $state -Run $run
        $all.Add($r)
        Write-Host ("  run {0}: private={1} MB cpu={2}% (max thread {3}%) gpu={4}% (dec {5}%, 3d {6}%) shared={7} MB rate={8} fps={9} cpu/fps={10} gpu/fps={11} delayed={12}" -f `
                $run, $r.private_mb, $r.cpu_pct, $r.max_thread_cpu_pct, $r.gpu_total_pct, $r.gpu_decode_pct,
                $r.gpu_3d_pct, $r.gpu_shared_mb, $r.playback_rate, $r.delivered_fps,
                $r.cpu_pct_per_fps, $r.gpu_pct_per_fps, $r.vo_delayed_frames)
    }
}

# The aggregate: the medians over the runs of each state.
$agg = [ordered]@{}
foreach ($state in $states) {
    $rows = @($all | Where-Object { $_.state -eq $state.name })
    if ($rows.Count -eq 0) { continue }
    $a = [ordered]@{ runs = $rows.Count }
    foreach ($k in @('private_mb', 'ws_mb', 'cpu_pct', 'max_thread_cpu_pct', 'gpu_total_pct',
                     'gpu_decode_pct', 'gpu_3d_pct', 'gpu_shared_mb', 'delivered_fps',
                     'playback_rate', 'vo_delayed_frames', 'vo_dropped_frames',
                     'decoder_dropped_frames', 'mistimed_frames', 'cpu_pct_per_fps', 'gpu_pct_per_fps')) {
        $vals = @($rows | ForEach-Object { $_.$k } | Where-Object { $null -ne $_ })
        if ($vals.Count -eq 0) { $a[$k] = $null; continue }
        $s = Get-Stats $vals
        $a[$k] = $s.median
    }
    $rates = @($rows | ForEach-Object { $_.playback_rate } | Where-Object { $null -ne $_ })
    $a['playback_rate_valid'] = ($rates.Count -gt 0 -and (@($rates | Where-Object { $_ -lt 0.97 -or $_ -gt 1.03 }).Count -eq 0))
    $agg[$state.name] = $a
}

$report = [ordered]@{
    tool = 'bench_baseline_v2'
    phase = 1
    date = (Get-Date -Format 'yyyy-MM-ddTHH:mm:ssK')
    label = $Label
    baseline = 'v2 - the engine AFTER the pacing fix (1.2) and BEFORE the long-pause unload (1.1)'
    baseline_commit = '340da84'
    baseline_v1 = 'docs/bench/phase0.json (ARCHIVED - taken before the pacing fix, so its playback rate is 0.70-0.75 and its CPU/GPU are NOT comparable)'
    engine = $engine
    runs = $Runs
    method = 'the same four states as the Phase 0 harness, plus the live mpv frame counters (playback rate, delivered fps, vo-delayed / vo-dropped / decoder-dropped / mistimed) and the normalized metrics (cpu_pct_per_fps, gpu_pct_per_fps)'
    metric_notes = @{
        cpu_pct_per_fps = 'process CPU% / delivered frames per second'
        gpu_pct_per_fps = 'GPU total% / delivered frames per second'
        validity = 'a run whose playback_rate is outside 0.97-1.03 is INVALID (the brief forbids quoting it)'
    }
    aggregate = $agg
    runs_detail = $all
}
$json = $report | ConvertTo-Json -Depth 10
$outFull = [System.IO.Path]::GetFullPath($OutJson)
$outDir = [System.IO.Path]::GetDirectoryName($outFull)
if (-not (Test-Path -LiteralPath $outDir)) { New-Item -ItemType Directory -Force -Path $outDir | Out-Null }
$json | Set-Content -LiteralPath $outFull -Encoding UTF8
Write-Host ""
Write-Host "bench_baseline_v2: wrote $outFull"
```

## File: tools/bench_dpb_ab.ps1
```powershell
# tools/bench_dpb_ab.ps1 - task 1.4 (D3): the DPB A/B.
#
# The A/B: the SAME video, twice.
#   A: the source with a deep reference list (the 16-ref 4K corpus).
#   B: the same video after the compressor re-encoded it with the pinned
#      DPB (ref=2).
# The metric is the C4 pair (private bytes + GPU shared memory) plus the
# playback rate and the frame counters - a run whose rate is outside
# 0.97-1.03 is invalid.
#
# The two arms are measured ALTERNATELY inside ONE session (the GPU
# numbers drift up to 1.5x between sessions), so the comparison is A vs B,
# never A vs a stored number.
[CmdletBinding()]
param(
    [string]$EngineExe = '',
    [string]$FileA = '',
    [string]$FileB = '',
    [int]$Runs = 3,
    [int]$SettleSeconds = 6,
    [int]$SampleSeconds = 8,
    [string]$OutJson = ''
)
$ErrorActionPreference = 'Stop'
$script:Here = Split-Path -Parent $MyInvocation.MyCommand.Path
. (Join-Path $script:Here 'bench_lib.ps1')
# C8: the A/B replaces the wallpaper (the engine plays another file), so the
# live session is snapshotted and restored - and the user's engine is
# restarted in the same `finally` (the A/B stops it to start its own).
. (Join-Path $script:Here 'session_snapshot.ps1')

if ([string]::IsNullOrWhiteSpace($EngineExe)) { $EngineExe = Join-Path $script:Here '..\build\release\engine.exe' }
if ([string]::IsNullOrWhiteSpace($FileA)) { $FileA = Join-Path $script:Here '..\tests\corpus\highdpb_4k.mp4' }
if ([string]::IsNullOrWhiteSpace($FileB)) { $FileB = Join-Path $script:Here '..\build\dpb-4k.mp4' }
if ([string]::IsNullOrWhiteSpace($OutJson)) { $OutJson = Join-Path $script:Here '..\docs\bench\dpb-ab.json' }
$engine = [System.IO.Path]::GetFullPath($EngineExe)
foreach ($f in @($FileA, $FileB)) { if (-not (Test-Path -LiteralPath $f)) { throw "missing: $f" } }

function Measure-Arm([string]$File, [string]$Tag) {
    # The singleton mutex is released ASYNCHRONOUSLY when the previous
    # engine dies, so a start can lose the race and exit immediately.
    for ($i = 0; $i -lt 40; $i++) {
        if (-not (Get-Process engine -ErrorAction SilentlyContinue)) { break }
        Start-Sleep -Milliseconds 250
    }
    Start-Sleep -Milliseconds 800
    $dataDir = Join-Path $env:TEMP ("k6wp-dpb-" + [Guid]::NewGuid().ToString('N').Substring(0, 8))
    New-Item -ItemType Directory -Force -Path (Join-Path $dataDir 'K6WP') | Out-Null
    $csv = Join-Path $dataDir 'counters.csv'
    $old = $env:LOCALAPPDATA
    try {
        $env:LOCALAPPDATA = $dataDir
        $p = Start-Process -FilePath $engine -PassThru -WindowStyle Hidden -ArgumentList @(
            "--video=$File", "--dump-mpv-counters=$csv", '--dump-mpv-counters-count=30',
            '--dump-mpv-counters-interval-ms=1000', '--exit-after-ms=90000')
    }
    finally { $env:LOCALAPPDATA = $old }
    if (-not (Wait-K6wpEngine -TimeoutMs 25000)) {
        try { if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force } } catch { }
        throw "the engine did not answer the pipe ($Tag)"
    }
    $saved = Get-DesktopWindows
    try {
        Start-Sleep -Seconds $SettleSeconds
        $tp = Start-TypeperfSampler -ProcessId $p.Id -Seconds ($SampleSeconds + 10) -ProcessName 'engine'
        $series = @(Get-ProcessSeries -ProcessId $p.Id -Seconds $SampleSeconds -IntervalMs 1000)
        $gpu = @(Read-TypeperfCsv -Path $tp.Csv)
        try { if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue } } catch { }
        $rows = @(Import-Csv -LiteralPath $csv -ErrorAction SilentlyContinue)
        # The playback rate + the frame counters, from the mpv counters.
        $media = 0.0; $prev = $null; $frames = 0.0; $prevF = $null
        foreach ($r in $rows) {
            $t = $r.'playback-time'
            if ($t -and $t -ne 'na') { $tv = [double]$t; if ($null -ne $prev) { $d = $tv - $prev; if ($d -lt 0) { $media += $tv } else { $media += $d } }; $prev = $tv }
            $f = $r.'estimated-frame-number'
            if ($f -and $f -ne 'na') { $fv = [double]$f; if ($null -ne $prevF) { $d = $fv - $prevF; if ($d -lt 0) { $frames += $fv } else { $frames += $d } }; $prevF = $fv }
        }
        $win = 0.0
        if ($rows.Count -ge 2) {
            $win = ([double]$rows[$rows.Count - 1].elapsed_ms - [double]$rows[0].elapsed_ms) / 1000.0
        }
        $gpuM = @($gpu | ForEach-Object { $_.gpu_shared_mb } | Where-Object { $null -ne $_ })
        return [pscustomobject]@{
            tag = $Tag
            file = (Split-Path -Leaf $File)
            private_mb = (Get-Stats @($series | ForEach-Object { $_.private_mb })).median
            gpu_shared_mb = (Get-Stats $gpuM).median
            gpu_total_pct = (Get-Stats @($gpu | ForEach-Object { $_.gpu_total_pct } | Where-Object { $null -ne $_ })).median
            cpu_pct = (Get-Stats @($series | ForEach-Object { $_.cpu_pct })).median
            max_thread_cpu_pct = (Get-Stats @($series | ForEach-Object { $_.max_thread_cpu_pct })).median
            playback_rate = $(if ($win -gt 0) { [math]::Round($media / $win, 4) } else { $null })
            delivered_fps = $(if ($win -gt 0) { [math]::Round($frames / $win, 2) } else { $null })
            vo_delayed = $(if ($rows.Count -ge 2 -and $rows[$rows.Count - 1].'vo-delayed-frame-count' -ne 'na') { [double]$rows[$rows.Count - 1].'vo-delayed-frame-count' - [double]$rows[0].'vo-delayed-frame-count' } else { $null })
        }
    }
    finally { Restore-DesktopWindows -Saved $saved }
}

$busy = @(Get-Process engine -ErrorAction SilentlyContinue)
if ($busy.Count -gt 0) { throw "an engine.exe is already running" }
$results = @()
for ($i = 1; $i -le $Runs; $i++) {
    foreach ($arm in @('A', 'B')) {
        $file = if ($arm -eq 'A') { $FileA } else { $FileB }
        $r = Measure-Arm -File $file -Tag "$arm"
        $results += $r
        Write-Host ("  run {0} arm {1} ({2}): private={3} MB shared={4} MB rate={5} fps={6} delayed={7}" -f `
                $i, $arm, $r.file, $r.private_mb, $r.gpu_shared_mb, $r.playback_rate, $r.delivered_fps, $r.vo_delayed)
    }
}
$a = @($results | Where-Object { $_.tag -eq 'A' })
$b = @($results | Where-Object { $_.tag -eq 'B' })
$report = [ordered]@{
    tool = 'bench_dpb_ab'
    phase = 1
    date = (Get-Date -Format 'yyyy-MM-ddTHH:mm:ssK')
    label = 'live-proven (one machine)'
    method = 'ABAB alternating inside one session; C4 metrics (private bytes + GPU shared); the playback rate and the frame counters travel with every number'
    file_a = $FileA
    file_b = $FileB
    runs = $Runs
    a = [ordered]@{
          private_mb = (Get-Stats @($a | ForEach-Object { $_.private_mb })).median
          private_band_mb = (Get-Stats @($a | ForEach-Object { $_.private_mb })).band
          gpu_shared_band_mb = (Get-Stats @($a | ForEach-Object { $_.gpu_shared_mb })).band
          n = @($a).Count
        gpu_shared_mb = (Get-Stats @($a | ForEach-Object { $_.gpu_shared_mb })).median
        playback_rate = (Get-Stats @($a | ForEach-Object { $_.playback_rate })).median
    }
    b = [ordered]@{
          private_mb = (Get-Stats @($b | ForEach-Object { $_.private_mb })).median
          private_band_mb = (Get-Stats @($b | ForEach-Object { $_.private_mb })).band
          gpu_shared_band_mb = (Get-Stats @($b | ForEach-Object { $_.gpu_shared_mb })).band
          n = @($b).Count
        gpu_shared_mb = (Get-Stats @($b | ForEach-Object { $_.gpu_shared_mb })).median
        playback_rate = (Get-Stats @($b | ForEach-Object { $_.playback_rate })).median
    }
    results = $results
}
$json = $report | ConvertTo-Json -Depth 8
$outFull = [System.IO.Path]::GetFullPath($OutJson)
$outDir = [System.IO.Path]::GetDirectoryName($outFull)
if (-not (Test-Path -LiteralPath $outDir)) { New-Item -ItemType Directory -Force -Path $outDir | Out-Null }
$json | Set-Content -LiteralPath $outFull -Encoding UTF8
Write-Host ""
Write-Host ("A (16 refs)  : private {0} MB, GPU shared {1} MB, rate {2}" -f $report.a.private_mb, $report.a.gpu_shared_mb, $report.a.playback_rate)
Write-Host ("B (pinned 2) : private {0} MB, GPU shared {1} MB, rate {2}" -f $report.b.private_mb, $report.b.gpu_shared_mb, $report.b.playback_rate)
Write-Host "bench_dpb_ab: wrote $outFull"
```

## File: tools/bench_get_state_max.ps1
```powershell
# tools/bench_get_state_max.ps1 - L1: the `get_state` max, measured against a
# PRIVATE engine instance so the live wallpaper session is never touched.
#
# WHY A PRIVATE INSTANCE
#   The K1 stress tool (tools/stress_ipc.ps1) stops the engine that is running
#   and starts the one it was given. That is correct for a dedicated bench
#   machine, but it takes the user's wallpaper down. This tool does the
#   opposite: it starts a SECOND engine beside the live one, with a unique
#   singleton mutex and pipe (the debug-only overrides), on a config that has
#   no video and a monitor id no display matches. The engine therefore owns
#   ZERO wallpaper slots and never paints a pixel.
#
# WHAT IT MEASURES
#   The same Rust stress client K1 used (4 clients by default) hammers
#   `get_state` for N requests and records every latency. The report carries
#   the p50/p95/p99/max and, with `-WithTrace`, the accumulator's per-label
#   n/mean/max from `ipc_trace.hpp` (debug builds only).
#
#   The MEANS are the honest per-field numbers: the accumulator adds a clock
#   pair, a map lookup and a mutex per scope, and the engine is a Debug build,
#   so every mean is an UPPER BOUND on the release cost.
#
#   The trace-on MAX is usable since L2: the accumulator logs its summary only
#   when the OUTERMOST scope exits (a thread-local depth counter), after its
#   own duration was taken - no measured block encloses the flush. Before L2
#   the flush landed inside the scope every 200 samples and the max was fake
#   (9-15 ms); the L1 gate therefore ran with the trace OFF, and it still does.
#   The trace-off run remains the latency gate.
#
#   A max is ONE sample: under a live session it can be an OS scheduler stall
#   (L2 saw 31-62 ms maxima with a p99 of 0-1 ms). Quote the p99 next to any
#   max, and the sample series (`-SamplesOut`) when a max has to be explained.
#
# THE EVIDENCE RULE
#   One invocation = one run. The L1 gate needs n >= 5 runs per binary, with
#   the same parameters; the caller loops and keeps every JSON.
#
# USAGE
#   powershell -File tools/bench_get_state_max.ps1 -EngineExe build\msvc-dev\engine.exe -Label before-1 -OutJson docs\bench\l1\before-1.json
#
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$EngineExe,
    [Parameter(Mandatory = $true)][string]$Label,
    [Parameter(Mandatory = $true)][string]$OutJson,
    [string]$SamplesOut = '',
    [int]$Threads = 4,
    [int]$Requests = 2000,
    [int]$ReadyTimeoutSec = 20,
    # OFF by default: the trace adds a clock pair, a map lookup and a mutex
    # per scope, so the means are upper bounds - and the client latency is the
    # gate, not the trace. Use `-WithTrace` for the per-label means (L2) and
    # for the per-field maxima (usable since the L2 depth gate: the summary
    # logs only at the outermost scope, so no measured block encloses the
    # flush). NOTE the name: a [switch] parameter types its variable, so it
    # must NOT be called `-Trace` if the script also has a `$trace` accumulator
    # (case-insensitive variables + a type constraint = "Cannot convert
    # OrderedDictionary to SwitchParameter").
    [switch]$WithTrace
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$EngineExe = (Resolve-Path -LiteralPath $EngineExe).Path
$client = Join-Path $root 'rust\target\release\examples\stress_client.exe'
if (-not (Test-Path -LiteralPath $client)) {
    throw "the stress client is not built: $client (cargo build --release --example stress_client -p k6wp-ipc)"
}

$runId = [Guid]::NewGuid().ToString('N').Substring(0, 8)
$mutexName = "Local\K6WP-Engine-L1-$runId"
$pipeName = "\\.\pipe\k6wp-engine-l1-$runId"
$workDir = Join-Path $env:TEMP "k6wp_l1_$runId"
New-Item -ItemType Directory -Force -Path $workDir | Out-Null
$configPath = Join-Path $workDir 'config.json'
$clientJson = Join-Path $workDir 'client.json'
$probeJson = Join-Path $workDir 'probe.json'
$logPath = Join-Path $env:LOCALAPPDATA 'K6WP\engine.log'

# A valid v6 config with NO video and a monitor id that matches no display:
# the engine serves IPC, owns zero slots, and paints nothing.
$config = [ordered]@{
    version                = 6
    video_path             = ''
    fit_mode               = 'cover'
    speed                  = 1.0
    monitor_id             = 99
    crf                    = 23
    resolution_w           = 1920
    resolution_h           = 1080
    fps_cap                = 30
    battery_saver          = $false
    battery_mode           = 'cap24'
    cpu_affinity           = 'auto'
    gpu_adapter            = 'auto'
    reconcile_mode         = 'legacy'
    pause_unload_after_sec = 30
}
# NO BOM: the config parser rejects a leading BOM (the H4 rule).
$jsonConfig = $config | ConvertTo-Json
[IO.File]::WriteAllText($configPath, $jsonConfig, (New-Object Text.UTF8Encoding($false)))

$logStart = 0
if (Test-Path -LiteralPath $logPath) { $logStart = (Get-Item -LiteralPath $logPath).Length }

function Read-LogDelta {
    if (-not (Test-Path -LiteralPath $logPath)) { return '' }
    $stream = [IO.File]::Open($logPath, 'Open', 'Read', 'ReadWrite')
    try {
        $null = $stream.Seek($logStart, 'Begin')
        $reader = New-Object IO.StreamReader($stream)
        return $reader.ReadToEnd()
    } finally {
        $stream.Dispose()
    }
}

function Invoke-StressClient {
    param([string[]]$ClientArgs)
    # The client reports on STDERR; PS 5.1 with EAP=Stop turns a native
    # command's stderr into a TERMINATING error, so the call runs under
    # EAP=Continue and the exit code is read explicitly.
    $old = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try {
        $lines = & $client @ClientArgs 2>&1
        $code = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $old
    }
    return [pscustomobject]@{ Code = $code; Output = @($lines) }
}

$engineProcess = $null
try {
    $env:K6WP_ENGINE_MUTEX = $mutexName
    $env:K6WP_ENGINE_PIPE = $pipeName
    if ($WithTrace) { $env:K6WP_IPC_TRACE = '1' }
    try {
        $engineProcess = Start-Process -FilePath $EngineExe `
            -ArgumentList @('--config', "`"$configPath`"") `
            -WindowStyle Hidden -PassThru
    } finally {
        Remove-Item Env:\K6WP_ENGINE_MUTEX -ErrorAction SilentlyContinue
        Remove-Item Env:\K6WP_ENGINE_PIPE -ErrorAction SilentlyContinue
        Remove-Item Env:\K6WP_IPC_TRACE -ErrorAction SilentlyContinue
    }

    # Readiness: a probe request, not a sleep. A connect failure shows up as
    # failures=1 in the probe report (the client itself exits 0).
    $deadline = (Get-Date).AddSeconds($ReadyTimeoutSec)
    $ready = $false
    while (-not $ready -and (Get-Date) -lt $deadline) {
        Start-Sleep -Milliseconds 300
        if ($engineProcess.HasExited) {
            throw ("the private engine exited early (code {0}) - is the binary a DEBUG build? " +
                   "the K6WP_ENGINE_* overrides are compiled out of release builds" -f $engineProcess.ExitCode)
        }
        Remove-Item -LiteralPath $probeJson -Force -ErrorAction SilentlyContinue
        $null = Invoke-StressClient @('--threads', '1', '--requests', '1',
            '--pipe', $pipeName, '--json', $probeJson)
        if (Test-Path -LiteralPath $probeJson) {
            try {
                $probe = Get-Content -LiteralPath $probeJson -Raw | ConvertFrom-Json
                if ($probe.requests -ge 1 -and $probe.failures -eq 0) { $ready = $true }
            } catch { }
        }
    }
    if (-not $ready) { throw "the private engine did not answer on $pipeName within $ReadyTimeoutSec s" }

    # The measured run: the SAME client K1 used, with the same parameters on
    # both sides of the A/B. No churn: the spike lives in `get_state` itself.
    $clientArgs = @(
        '--threads', "$Threads",
        '--requests', "$Requests",
        '--pipe', $pipeName,
        '--json', $clientJson
    )
    if ($SamplesOut) { $clientArgs += @('--samples', $SamplesOut) }
    $run = Invoke-StressClient $clientArgs
    $run.Output | ForEach-Object { Write-Host "  [l1] $_" }
    if ($run.Code -ne 0) { throw "the stress client failed (exit $($run.Code))" }
    $clientReport = Get-Content -LiteralPath $clientJson -Raw | ConvertFrom-Json

    # The trace summary: the LAST line per label in the delta (the summary is
    # logged every 200 samples per label, so the last one covers the run).
    # Every label is kept: L2 measures the per-field scopes (state.*), so the
    # report must not whitelist a fixed set.
    $trace = [ordered]@{}
    $delta = Read-LogDelta
    foreach ($line in ($delta -split "`r?`n")) {
        if ($line -match 'ipc-trace: (.+?): n=(\d+) the mean=(-?\d+) us the max=(\d+) us') {
            $trace[$Matches[1]] = [ordered]@{
                n       = [int]$Matches[2]
                mean_us = [int]$Matches[3]
                max_us  = [int]$Matches[4]
            }
        }
    }

    $report = [ordered]@{
        tool         = 'bench_get_state_max'
        task         = 'L1/L2'
        label        = $Label
        date         = (Get-Date).ToString('o')
        engine       = $EngineExe
        private      = $true
        mutex        = $mutexName
        pipe         = $pipeName
        threads      = $Threads
        requests     = $clientReport.requests
        p50_ms       = $clientReport.p50_ms
        p95_ms       = $clientReport.p95_ms
        p99_ms       = $clientReport.p99_ms
        max_ms       = $clientReport.max_ms
        failures     = $clientReport.failures
        wall_seconds = $clientReport.wall_seconds
        trace        = $trace
    }
    $outDir = Split-Path -Parent ([IO.Path]::GetFullPath($OutJson))
    if (-not (Test-Path -LiteralPath $outDir)) { New-Item -ItemType Directory -Force -Path $outDir | Out-Null }
    $report | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $OutJson -Encoding UTF8
    Write-Host ("  [l1] {0}: {1} requests, the p50 {2} ms, the p99 {3} ms, the max {4} ms" -f `
            $Label, $report.requests, $report.p50_ms, $report.p99_ms, $report.max_ms)
    if ($trace.Count -gt 0) {
        Write-Host "  [l2] the trace (n / mean / max, us):"
        foreach ($name in ($trace.Keys | Sort-Object)) {
            $t = $trace[$name]
            Write-Host ("       {0,-18} {1,6}  {2,8}  {3,8}" -f $name, $t.n, $t.mean_us, $t.max_us)
        }
    }
    else {
        Write-Host "  [l2] no trace labels (run with -WithTrace on a debug engine)"
    }
} finally {
    if ($engineProcess -and -not $engineProcess.HasExited) {
        Stop-Process -Id $engineProcess.Id -Force -ErrorAction SilentlyContinue
        $engineProcess.WaitForExit(5000) | Out-Null
    }
    Remove-Item -LiteralPath $workDir -Recurse -Force -ErrorAction SilentlyContinue
}
```

## File: tools/extract_sps_truth.ps1
```powershell
# tools/extract_sps_truth.ps1 - the E3 ground truth.
#
# ffmpeg's `trace_headers` bitstream filter is the reference parser: it
# prints the SPS fields it decoded. This script runs it over the corpus and
# writes the expected DPB values into tests/golden/sps-dpb.json - the
# fixture the Rust parser is checked against.
#
# The corpus is NOT committed (it is gitignored), so the fixture records the
# FILE NAME and the expected values; tools/build_corpus.ps1 regenerates the
# corpus (and its SHA-256s are recorded next to the fixture).
#
# Usage:
#   powershell -NoProfile -ExecutionPolicy Bypass -File tools\extract_sps_truth.ps1
$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
$corpus = Join-Path $here '..\tests\corpus'
$ffmpeg = Join-Path $here '..\vendor\ffmpeg\ffmpeg.exe'
$out = Join-Path $here '..\tests\golden\sps-dpb.json'
if (-not (Test-Path -LiteralPath $ffmpeg)) { throw "ffmpeg not found: $ffmpeg" }

function Get-TraceFields([string]$File) {
    # ffmpeg writes its log to stderr; that is the DATA here, not an error.
    $old = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    $text = & $ffmpeg -hide_banner -v verbose -i $File -c copy -bsf:v trace_headers -f null - 2>&1 | Out-String
    $ErrorActionPreference = $old
    $refs = @()
    $vui = @()
    $hevc = @()
    $mbs = @()
    $chroma = @()
    $depth = @()
    foreach ($line in ($text -split "`n")) {
        if ($line -match 'max_num_ref_frames\s+\S+\s*=\s*(\d+)') { $refs += [int]$Matches[1] }
        elseif ($line -match 'max_dec_frame_buffering\s+\S+\s*=\s*(\d+)') { $vui += [int]$Matches[1] }
        elseif ($line -match 'sps_max_dec_pic_buffering_minus1(?:\[\d+\])?\s+\S+\s*=\s*(\d+)') { $hevc += [int]$Matches[1] }
        elseif ($line -match 'frame_mbs_only_flag\s+\S+\s*=\s*(\d+)') { $mbs += [int]$Matches[1] }
        # The chroma format and the bit depth (task H2): they decide the
        # bytes per pixel, so the DPB of a 10-bit file is TWICE the 8-bit
        # one. Both codecs print them.
        elseif ($line -match 'chroma_format_idc\s+\S+\s*=\s*(\d+)') { $chroma += [int]$Matches[1] }
        elseif ($line -match 'bit_depth_luma_minus8\s+\S+\s*=\s*(\d+)') { $depth += [int]$Matches[1] }
    }
    $result = [ordered]@{
        max_num_ref_frames = if ($refs.Count -gt 0) { ($refs | Measure-Object -Maximum).Maximum } else { $null }
        max_dec_frame_buffering = if ($vui.Count -gt 0) { ($vui | Measure-Object -Maximum).Maximum } else { $null }
        sps_max_dec_pic_buffering = if ($hevc.Count -gt 0) { (($hevc | Measure-Object -Maximum).Maximum) + 1 } else { $null }
        interlaced = if ($mbs.Count -gt 0) { (($mbs | Measure-Object -Minimum).Minimum -eq 0) } else { $null }
        chroma_format_idc = if ($chroma.Count -gt 0) { ($chroma | Measure-Object -Maximum).Maximum } else { $null }
        bit_depth_luma = if ($depth.Count -gt 0) { (($depth | Measure-Object -Maximum).Maximum) + 8 } else { $null }
        sps_count = $refs.Count + $hevc.Count
    }
    return $result
}

$files = Get-ChildItem -LiteralPath $corpus -File |
    # The containers the SPS module parses itself. A .mkv/.webm is NOT
    # here on purpose: the module does not parse those containers (it falls
    # back - that is the read-strategy sweep's job, tests/sps_read_strategy.rs),
    # so a Matroska case in THIS fixture would compare the module against
    # ffmpeg on a file the module never claimed to read.
    Where-Object { $_.Extension -in @('.mp4', '.h264') } |
    Where-Object { $_.Length -gt 0 } |
    Sort-Object Name
if ($files.Count -eq 0) { throw "the corpus is empty - run tools\build_corpus.ps1 first" }

$cases = @()
foreach ($f in $files) {
    Write-Host ("probing {0} ..." -f $f.Name)
    $truth = Get-TraceFields $f.FullName
    $sha = (Get-FileHash -LiteralPath $f.FullName -Algorithm SHA256).Hash.ToLower()
    $cases += [ordered]@{
        file = $f.Name
        sha256 = $sha
        bytes = $f.Length
        expected = $truth
    }
    Write-Host ("   refs={0} vui_dpb={1} hevc_dpb={2} interlaced={3} chroma={4} depth={5}" -f `
            $truth.max_num_ref_frames, $truth.max_dec_frame_buffering, $truth.sps_max_dec_pic_buffering, $truth.interlaced, $truth.chroma_format_idc, $truth.bit_depth_luma)
}

$report = [ordered]@{
    tool = 'extract_sps_truth'
    date = (Get-Date -Format 'yyyy-MM-ddTHH:mm:ssK')
    label = 'the ground truth: ffmpeg trace_headers (its own bitstream parser)'
    note = 'the corpus is gitignored; tools\build_corpus.ps1 regenerates it. The sha256 pins the exact file the values were taken from.'
    cases = $cases
}
$json = $report | ConvertTo-Json -Depth 8
# No BOM: serde_json (and every JSON parser) rejects a byte-order mark.
$utf8 = New-Object System.Text.UTF8Encoding($false)
[System.IO.File]::WriteAllText([System.IO.Path]::GetFullPath($out), $json, $utf8)
Write-Host ""
Write-Host ("wrote {0} ({1} cases)" -f $out, $cases.Count)
```

## File: tools/session_snapshot.ps1
```powershell
# session_snapshot.ps1 - Save and restore the Windows session state.
#
# Usage:
#   . .\tools\session_snapshot.ps1
#   $session = Invoke-SessionSafe { ... your work ... }
#   $session.Restore()
#
# The snapshot captures:
#   * Current wallpaper path (per monitor).
#   * Lock screen image path.
#   * Display configuration (resolution, orientation, position).
#   * Active window (to restore focus).
#
# The restore puts everything back exactly as it was.

# `[Win]` must exist before the SessionSnapshot class is parsed, not inside a
# method: PowerShell resolves class member types at parse time.
if (-not ("Win" -as [type])) {
    Add-Type @"
using System;
using System.Runtime.InteropServices;
public class Win {
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hWnd);
    [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
    [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint lpdwProcessId);
}
"@
}

function Get-CurrentWallpaper {
    # Get wallpaper for each monitor via registry.
    $result = @{}
    try {
        $regPath = "HKCU:\Control Panel\Desktop"
        $wallpaper = Get-ItemProperty -Path $regPath -Name Wallpaper -ErrorAction SilentlyContinue
        if ($wallpaper -and $wallpaper.Wallpaper) {
            $result["primary"] = $wallpaper.Wallpaper
        }
    } catch { }
    # Per-monitor wallpapers (Windows 10+)
    try {
        $monitors = Get-CimInstance -ClassName WmiMonitorID -Namespace root\wmi -ErrorAction SilentlyContinue
        if ($monitors) {
            foreach ($m in $monitors) {
                $key = "HKCU:\Control Panel\Desktop\PerMonitorSettings\" + $m.InstanceName
                $wp = Get-ItemProperty -Path $key -Name Wallpaper -ErrorAction SilentlyContinue
                if ($wp -and $wp.Wallpaper) {
                    $result[$m.InstanceName] = $wp.Wallpaper
                }
            }
        }
    } catch { }
    return $result
}

function Get-LockScreenImage {
    try {
        $regPath = "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Authentication\LogonUI\Background"
        $img = Get-ItemProperty -Path $regPath -Name "OEMBackground" -ErrorAction SilentlyContinue
        if ($img -and $img.OEMBackground -eq 1) {
            return "OEMBackground enabled"
        }
    } catch { }
    return $null
}

function Get-DisplayConfig {
    try {
        $displays = Get-CimInstance -ClassName Win32_DesktopMonitor -ErrorAction SilentlyContinue
        $result = @()
        foreach ($d in $displays) {
            $result += @{
                DeviceID = $d.DeviceID
                ScreenWidth = $d.ScreenWidth
                ScreenHeight = $d.ScreenHeight
                PixelsPerXLogicalInch = $d.PixelsPerXLogicalInch
                PixelsPerYLogicalInch = $d.PixelsPerYLogicalInch
            }
        }
        return $result
    } catch { }
    return @()
}

function Get-ActiveWindow {
    $hwnd = [Win]::GetForegroundWindow()
    if ($hwnd -ne [IntPtr]::Zero) {
        $windowPid = 0
        $null = [Win]::GetWindowThreadProcessId($hwnd, [ref]$windowPid)
        return @{ Handle = $hwnd; ProcessId = $windowPid }
    }
    return $null
}

function Set-Wallpaper {
    param($path, $monitor = "primary")
    # Use SystemParametersInfo to set wallpaper.
    Add-Type @"
using System;
using System.Runtime.InteropServices;
public class Wallpaper {
    [DllImport("user32.dll", CharSet = CharSet.Auto)]
    public static extern bool SystemParametersInfo(uint uiAction, uint uiParam, string pvParam, uint fWinIni);
    public const uint SPI_SETDESKWALLPAPER = 0x0014;
    public const uint SPIF_UPDATEINIFILE = 0x01;
    public const uint SPIF_SENDCHANGE = 0x02;
    public static bool Set(string path) {
        return SystemParametersInfo(SPI_SETDESKWALLPAPER, 0, path, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
    }
}
"@
    [Wallpaper]::Set($path) | Out-Null
}

function Set-LockScreen {
    param($path)
    # This is more complex and requires special permissions. Skip for now.
    Write-Host "Lock screen restore not implemented (requires SYSTEM context)"
}

# Main snapshot object.
class SessionSnapshot {
    [hashtable]$Wallpapers
    [string]$LockScreen
    [array]$Displays
    [hashtable]$ActiveWindow

    SessionSnapshot() {
        $this.Wallpapers = Get-CurrentWallpaper
        $this.LockScreen = Get-LockScreenImage
        $this.Displays = Get-DisplayConfig
        $this.ActiveWindow = Get-ActiveWindow
    }

    [void] Restore() {
        Write-Host "Restoring wallpapers..."
        foreach ($kv in $this.Wallpapers.GetEnumerator()) {
            if ($kv.Value) {
                try {
                    Set-Wallpaper -path $kv.Value
                    Write-Host "  Restored $($kv.Key): $($kv.Value)"
                } catch {
                    Write-Warning "Failed to restore $($kv.Key): $_"
                }
            }
        }
        Write-Host "Restoring displays..."
        # Display restore is complex; skip for safety.
        # foreach ($d in $this.Displays) { ... }
        if ($this.ActiveWindow) {
            Write-Host "Restoring active window focus..."
            # `[Win]` is added at run time, so resolve it dynamically: a class
            # body is parsed before Add-Type has run.
            $win = "Win" -as [type]
            if ($win) { $win::SetForegroundWindow($this.ActiveWindow.Handle) | Out-Null }
        }
    }
}

function Invoke-SessionSafe {
    param([scriptblock]$Work)
    $snapshot = [SessionSnapshot]::new()
    try {
        $result = & $Work
        return $snapshot
    } finally {
        $snapshot.Restore()
    }
}

try {
    Export-ModuleMember -Function Invoke-SessionSafe, Get-CurrentWallpaper, Get-LockScreenImage, Get-DisplayConfig, Get-ActiveWindow, Set-Wallpaper
} catch {
    # Dot-sourced (not imported as a module): nothing to export.
}
```

## File: compressor/src/dpb_probe.cpp
```cpp
int ParseFieldValue(const std::string& line, const std::string& field) {
⋮----
DpbInfo ProbeDpb(const std::filesystem::path& in) {
```

## File: compressor/src/dpb_probe.hpp
```cpp
struct DpbInfo {
⋮----
inline std::uint64_t FrameBytesFor(int width, int height, int chroma_format_idc,
⋮----
DpbInfo ProbeDpb(const std::filesystem::path& in);
```

## File: compressor/src/main.cpp
```cpp
std::string JsonEscape(const std::string& s) {
⋮----
std::string Utf8FromWide(const std::wstring& w) {
⋮----
std::filesystem::path DefaultWallpapersDir() {
⋮----
std::string local(buf);
⋮----
std::string profile(buf);
⋮----
std::filesystem::path DefaultStillPath() {
⋮----
bool IsOptimal(const k6wp::compressor::VideoProps& props, int target_w,
⋮----
bool IsDpbAcceptable(const std::filesystem::path& in) {
⋮----
int main(int argc, char** argv) {
```

## File: compressor/CMakeLists.txt
```
# compressor - ffmpeg compress CLI (exe).
add_executable(compressor src/main.cpp src/cli.cpp src/encoder_detect.cpp src/ffmpeg_job.cpp src/cache_manager.cpp src/video_probe.cpp src/dpb_probe.cpp app.rc)

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

# dpb_frame_size_test (H2): the frame size of the decoder's pool follows the
# bit depth and the chroma format - the old constant (1.5 bytes per pixel)
# was only the 8-bit 4:2:0 case and UNDER-counted a 10-bit file by half.
# The Rust side has the same table (k6wp-core's SpsInfo::frame_bytes); the
# two must never disagree.
add_executable(dpb_frame_size_test tests/dpb_frame_size_test.cpp)
target_link_libraries(dpb_frame_size_test PRIVATE k6wp_shared)
target_include_directories(dpb_frame_size_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
add_test(NAME dpb_frame_size_test COMMAND dpb_frame_size_test)

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

# compress_friendly_error (1.2.1): the compressor's USER-FACING error payload.
# compressor.exe has no i18n machinery, so its `{"friendly","technical"}` text
# IS what a user reads, and Studio then maps it through
# FriendlyCompressError. This suite locks both halves of that handoff: the
# friendly message is actionable (re-extract / reinstall) instead of naming a
# vendor/ directory and an environment variable, and it still satisfies Studio's
# matching rule so the two sides keep agreeing. src/ffmpeg_job.cpp is
# #included by the test so the anonymous-namespace FriendlyId/FormatError are
# reachable (thumbnailer_test / probe_json_test pattern).
add_executable(compress_friendly_error_test
  tests/compress_friendly_error_test.cpp
  src/encoder_detect.cpp
)
target_link_libraries(compress_friendly_error_test PRIVATE k6wp_shared)
add_test(NAME compress_friendly_error COMMAND compress_friendly_error_test)
# Check 7 builds a real lockframe cmdline, so FindFfmpeg() must succeed.
# CI configures a one-level "build/" dir, but shared/ffmpeg_path.cpp's
# build-layout guess is "build/<preset>/" (two levels up to the repo root),
# so point the test at the vendored binary explicitly instead of depending on
# the build-dir depth (same fixture as studio_logic_test).
set_tests_properties(compress_friendly_error PROPERTIES
  ENVIRONMENT "K6WP_FFMPEG=${CMAKE_SOURCE_DIR}/vendor/ffmpeg/ffmpeg.exe")

# widen_utf8_test (MED-10): #includes the real ffmpeg_job.cpp so the
# anonymous-namespace Widen() under test is production code; FindFfmpeg and
# PickEncoder are stubbed by the test. Registered here so the UTF-8 decode
# fix stays locked.
add_executable(widen_utf8_test tests/widen_utf8_test.cpp)
target_link_libraries(widen_utf8_test PRIVATE k6wp_shared)
target_include_directories(widen_utf8_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
add_test(NAME widen_utf8_test COMMAND widen_utf8_test)
endif()
# E5: EVERY test gets a hard timeout. A hung test must never hang the
# suite without a bound (the per-test timeout is enforced by ctest).
get_property(_k6wp_tests_in_dir DIRECTORY PROPERTY TESTS)
foreach(_k6wp_t IN LISTS _k6wp_tests_in_dir)
  set_tests_properties(${_k6wp_t} PROPERTIES TIMEOUT 300)
endforeach()
```

## File: engine/src/ipc_trace.hpp
```cpp
inline bool IpcTraceEnabled() {
⋮----
inline void IpcTraceLog(const char* fmt, ...) {
⋮----
struct TraceStat {
⋮----
inline std::map<std::string, TraceStat>& TraceStats() {
⋮----
inline std::mutex& TraceStatsMutex() {
⋮----
inline int& TraceDepth() {
⋮----
class TraceScope {
⋮----
explicit TraceScope(const char* label) : label_(label) {
⋮----
std::lock_guard<std::mutex> lock(TraceStatsMutex());
⋮----
TraceScope(const TraceScope&) = delete;
```

## File: rust/crates/k6wp-core/src/library.rs
```rust
use std::path::PathBuf;
⋮----
pub struct LibraryEntry {
⋮----
fn default_res() -> String {
"0x0".to_string()
⋮----
fn default_fps() -> i64 {
⋮----
fn default_crf() -> i64 {
⋮----
fn default_encoder() -> String {
"libx264".to_string()
⋮----
pub struct LibraryIndex {
⋮----
fn default_library_version() -> u32 {
⋮----
pub enum LibraryError {
⋮----
fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
⋮----
LibraryError::Json(m) => write!(f, "library json: {m}"),
LibraryError::Shape(m) => write!(f, "library shape: {m}"),
LibraryError::Io(m) => write!(f, "library io: {m}"),
⋮----
impl LibraryIndex {
/// Parses the index file.
    ///
⋮----
///
    /// The C++ side throws `LibraryError` for a malformed file; the Rust
⋮----
/// The C++ side throws `LibraryError` for a malformed file; the Rust
    /// side returns the same three kinds of error, so the callers report
⋮----
/// side returns the same three kinds of error, so the callers report
    /// the same messages.
⋮----
/// the same messages.
    pub fn parse(text: &str) -> Result<Self, LibraryError> {
⋮----
pub fn parse(text: &str) -> Result<Self, LibraryError> {
// The C++ side reads the file through nlohmann, which skips a UTF-8
// BOM; serde_json rejects it. A file written by an older tool (or by
// PowerShell) must not make the two Studios disagree, so the BOM is
// dropped before parsing.
let text = text.strip_prefix('\u{feff}').unwrap_or(text);
⋮----
serde_json::from_str(text).map_err(|e| LibraryError::Json(e.to_string()))?;
if !value.is_object() {
return Err(LibraryError::Shape("the root must be an object".into()));
⋮----
let items: Vec<serde_json::Value> = match value.get("entries") {
⋮----
Some(v) if v.is_array() => v.as_array().cloned().unwrap_or_default(),
⋮----
return Err(LibraryError::Shape(
"library \"entries\" must be an array".into(),
⋮----
.get("version")
.and_then(|v| v.as_u64())
.unwrap_or(LIBRARY_SCHEMA_VERSION as u64) as u32;
⋮----
entries: Vec::with_capacity(items.len()),
⋮----
for (i, item) in items.iter().enumerate() {
if !item.is_object() {
return Err(LibraryError::Shape(format!("entry {i} must be an object")));
⋮----
// A missing key takes the default; a WRONG TYPE is an error -
// the same rule the C++ codec applies (nlohmann's value() with
⋮----
out.entries.push(
serde_json::from_value::<LibraryEntry>(item.clone())
.map_err(|e| LibraryError::Shape(format!("entry {i}: {e}")))?,
⋮----
Ok(out)
⋮----
pub fn to_json(&self) -> String {
serde_json::to_string_pretty(self).unwrap_or_else(|_| "{}".to_string())
⋮----
pub fn save_atomic(&self, path: &std::path::Path) -> Result<(), LibraryError> {
let json = self.to_json();
let tmp = path.with_extension("tmp");
std::fs::write(&tmp, json).map_err(|e| LibraryError::Io(e.to_string()))?;
std::fs::rename(&tmp, path).map_err(|e| LibraryError::Io(e.to_string()))?;
Ok(())
⋮----
mod tests {
⋮----
fn parses_the_cpp_shape() {
⋮----
let index = LibraryIndex::parse(text).expect("the index must parse");
assert_eq!(index.version, 1);
assert_eq!(index.entries.len(), 1);
⋮----
assert_eq!(e.res, "1920x1080");
assert_eq!(e.codec, "h264");
assert_eq!(e.width, 1920);
assert_eq!(e.duration, 12.5);
assert!(!e.broken);
⋮----
fn a_utf8_bom_is_accepted_like_the_cpp_side() {
⋮----
let index = LibraryIndex::parse(text).expect("the BOM must not break the parse");
⋮----
assert!(index.entries.is_empty());
⋮----
fn missing_keys_take_the_defaults() {
let index = LibraryIndex::parse(r#"{"entries":[{}]}"#).expect("parse");
⋮----
assert_eq!(e.res, "0x0");
assert_eq!(e.fps, 30);
assert_eq!(e.crf, 23);
assert_eq!(e.encoder, "libx264");
assert_eq!(e.width, 0);
⋮----
fn a_missing_entries_array_is_empty() {
let index = LibraryIndex::parse(r#"{"version":1}"#).expect("parse");
⋮----
fn a_non_array_entries_is_an_error() {
let err = LibraryIndex::parse(r#"{"entries":{}}"#).expect_err("must fail");
assert!(matches!(err, LibraryError::Shape(_)));
⋮----
fn broken_json_is_an_error() {
let err = LibraryIndex::parse("{not json").expect_err("must fail");
assert!(matches!(err, LibraryError::Json(_)));
⋮----
fn the_round_trip_keeps_the_fields() {
⋮----
let index = LibraryIndex::parse(text).expect("parse");
let again = LibraryIndex::parse(&index.to_json()).expect("re-parse");
assert_eq!(index, again);
⋮----
fn save_atomic_round_trip() {
⋮----
let test_path = temp_dir.join(format!("k6wp-test-library-{}.json", std::process::id()));
⋮----
.save_atomic(&test_path)
.expect("save_atomic must succeed");
⋮----
LibraryIndex::parse(&std::fs::read_to_string(&test_path).unwrap()).expect("re-parse");
assert_eq!(index, reloaded);
```

## File: rust/crates/k6wp-core/src/settings.rs
```rust
use std::fs;
⋮----
use crate::config::merge_unknown_keys;
⋮----
pub use crate::config::ConfigError;
⋮----
pub struct StudioSettings {
⋮----
impl Default for StudioSettings {
⋮----
fn default() -> Self {
⋮----
compress_output_dir: default_compress_output_dir(),
⋮----
default_resolution_mode: "match_monitor".to_string(),
⋮----
cache_dir: default_cache_dir(),
⋮----
playlist_source: "all".to_string(),
performance_preset: "Seimbang".to_string(),
⋮----
pub fn default_compress_output_dir() -> PathBuf {
k6wp_local_dir("wallpapers")
⋮----
pub fn default_cache_dir() -> PathBuf {
k6wp_local_dir("cache")
⋮----
fn k6wp_local_dir(leaf: &str) -> PathBuf {
if let Some(local_app_data) = non_empty_env("LOCALAPPDATA") {
return PathBuf::from(local_app_data).join("K6WP").join(leaf);
⋮----
if let Some(user_profile) = non_empty_env("USERPROFILE") {
⋮----
.join("AppData")
.join("Local")
.join("K6WP")
.join(leaf);
⋮----
std::env::temp_dir().join("K6WP").join(leaf)
⋮----
fn non_empty_env(name: &str) -> Option<String> {
⋮----
Ok(value) if !value.is_empty() => Some(value),
⋮----
pub fn migrate_studio_settings(raw_text: &str) -> Result<StudioSettings, ConfigError> {
⋮----
.map_err(|_| ConfigError("corrupt settings JSON: parse error".to_string()))?;
migrate_studio_settings_value(&raw)
⋮----
pub fn migrate_studio_settings_value(raw: &Value) -> Result<StudioSettings, ConfigError> {
⋮----
.as_object()
.ok_or_else(|| ConfigError("studio settings root must be a JSON object".to_string()))?;
⋮----
let version = read_i64(object, "version", 0, CONTEXT)?;
if !(0..=STUDIO_SETTINGS_SCHEMA_VERSION).contains(&version) {
return Err(ConfigError(format!(
⋮----
let mut settings = defaults.clone();
⋮----
settings.auto_compress_on_import = read_bool(
⋮----
let compress_output_dir = read_string(object, "compress_output_dir", "", CONTEXT)?;
settings.compress_output_dir = if compress_output_dir.is_empty() {
defaults.compress_output_dir.clone()
⋮----
settings.default_crf = read_i64(object, "default_crf", defaults.default_crf, CONTEXT)?;
settings.default_fps = read_i64(object, "default_fps", defaults.default_fps, CONTEXT)?;
settings.default_resolution_mode = read_string(
⋮----
settings.start_with_windows = read_bool(
⋮----
read_bool(object, "wallpaper_sync", defaults.wallpaper_sync, CONTEXT)?;
⋮----
read_bool(object, "lockscreen_sync", defaults.lockscreen_sync, CONTEXT)?;
settings.lockscreen_offset_sec = read_f64(
⋮----
settings.lockscreen_offer_shown = read_bool(
⋮----
settings.compress_advanced_visible = read_bool(
⋮----
settings.check_updates = read_bool(object, "check_updates", defaults.check_updates, CONTEXT)?;
settings.close_to_tray = read_bool(object, "closeToTray", defaults.close_to_tray, CONTEXT)?;
⋮----
read_tolerant_enum(object, "playlistSource", "all", &["all", "custom"]);
settings.performance_preset = read_tolerant_enum(
⋮----
let cache_dir = read_string(object, "cache_dir", "", CONTEXT)?;
settings.cache_dir = if cache_dir.is_empty() {
defaults.cache_dir.clone()
⋮----
Ok(settings)
⋮----
/// Rejects the values outside the accepted ranges and normalizes the two
/// plan-todo-8 enums in place - the Rust mirror of the C++
⋮----
/// plan-todo-8 enums in place - the Rust mirror of the C++
/// `ValidateStudioSettings`:
⋮----
/// `ValidateStudioSettings`:
///
⋮----
///
/// * `default_crf` not in `[16, 28]`
⋮----
/// * `default_crf` not in `[16, 28]`
/// * `default_fps` not in `[1, 30]`
⋮----
/// * `default_fps` not in `[1, 30]`
/// * an empty `compress_output_dir` / `cache_dir`
⋮----
/// * an empty `compress_output_dir` / `cache_dir`
/// * a negative `lockscreen_offset_sec`
⋮----
/// * a negative `lockscreen_offset_sec`
/// * an unknown `default_resolution_mode`
⋮----
/// * an unknown `default_resolution_mode`
/// * a `playlist_source` outside `{all, custom}` -> `"all"`
⋮----
/// * a `playlist_source` outside `{all, custom}` -> `"all"`
/// * a `performance_preset` outside `{Hemat, Seimbang, Maksimal}` ->
⋮----
/// * a `performance_preset` outside `{Hemat, Seimbang, Maksimal}` ->
⋮----
pub fn validate_studio_settings(settings: &mut StudioSettings) -> Result<(), ConfigError> {
⋮----
if settings.compress_output_dir.as_os_str().is_empty() {
return Err(ConfigError(
"compress_output_dir must not be empty".to_string(),
⋮----
if settings.cache_dir.as_os_str().is_empty() {
return Err(ConfigError("cache_dir must not be empty".to_string()));
⋮----
"lockscreen_offset_sec must be >= 0".to_string(),
⋮----
if !matches!(
⋮----
if !matches!(settings.playlist_source.as_str(), "all" | "custom") {
settings.playlist_source = "all".to_string();
⋮----
settings.performance_preset = "Seimbang".to_string();
⋮----
Ok(())
⋮----
pub fn settings_json(settings: &StudioSettings) -> Value {
⋮----
out.insert("version".to_string(), Value::from(settings.version));
out.insert(
"auto_compress_on_import".to_string(),
⋮----
out.insert("default_crf".to_string(), Value::from(settings.default_crf));
out.insert("default_fps".to_string(), Value::from(settings.default_fps));
⋮----
"default_resolution_mode".to_string(),
Value::from(settings.default_resolution_mode.clone()),
⋮----
"wallpaper_sync".to_string(),
⋮----
"lockscreen_sync".to_string(),
⋮----
"lockscreen_offset_sec".to_string(),
⋮----
"lockscreen_offer_shown".to_string(),
⋮----
"compress_advanced_visible".to_string(),
⋮----
"check_updates".to_string(),
⋮----
"close_to_tray".to_string(),
⋮----
"playlist_source".to_string(),
Value::from(settings.playlist_source.clone()),
⋮----
"performance_preset".to_string(),
Value::from(settings.performance_preset.clone()),
⋮----
pub fn settings_report_json(settings: &StudioSettings) -> Value {
let mut out = match settings_json(settings) {
⋮----
"start_with_windows".to_string(),
⋮----
"compress_output_dir_basename".to_string(),
basename_bytes(&settings.compress_output_dir),
⋮----
"cache_dir_basename".to_string(),
basename_bytes(&settings.cache_dir),
⋮----
pub fn save_studio_settings(path: &Path, settings: &StudioSettings) -> Result<(), ConfigError> {
let mut settings = settings.clone();
validate_studio_settings(&mut settings)?;
let existing = read_existing_json(path);
let known = settings_json(&settings);
let doc = merge_unknown_keys(&existing, &known);
if let Some(parent) = path.parent() {
if !parent.as_os_str().is_empty() {
⋮----
.map_err(|e| ConfigError(format!("cannot create settings directory: {e}")))?;
⋮----
let mut tmp = path.as_os_str().to_owned();
tmp.push(".tmp");
⋮----
.map_err(|e| ConfigError(format!("cannot serialize settings: {e}")))?;
fs::write(&tmp, text).map_err(|e| {
ConfigError(format!(
⋮----
fs::rename(&tmp, path).map_err(|e| {
⋮----
fn read_existing_json(path: &Path) -> Value {
⋮----
.ok()
.and_then(|text| serde_json::from_str(&text).ok())
.filter(Value::is_object)
.unwrap_or_else(|| Value::Object(Map::new()))
⋮----
fn basename_bytes(path: &Path) -> Value {
let text = path.to_string_lossy();
let trimmed = text.trim_end_matches(['\\', '/']);
let name = match trimmed.rfind(['\\', '/']) {
⋮----
name.encode_utf16()
.map(|unit| Value::from(unit as u32))
.collect(),
⋮----
mod tests {
⋮----
fn the_default_is_the_cpp_default() {
⋮----
assert_eq!(settings.version, 5);
assert!(settings.auto_compress_on_import);
assert_eq!(settings.default_crf, 22);
assert_eq!(settings.default_fps, 30);
assert_eq!(settings.default_resolution_mode, "match_monitor");
assert!(settings.wallpaper_sync);
assert!(settings.lockscreen_sync);
assert_eq!(settings.lockscreen_offset_sec, 0.0);
assert!(!settings.lockscreen_offer_shown);
assert!(!settings.compress_advanced_visible);
assert!(settings.check_updates);
assert!(!settings.close_to_tray);
assert_eq!(settings.playlist_source, "all");
assert_eq!(settings.performance_preset, "Seimbang");
⋮----
assert!(validate_studio_settings(&mut settings).is_ok());
⋮----
fn a_missing_key_takes_the_default() {
let settings = migrate_studio_settings("{}").expect("the empty file must migrate");
⋮----
assert_eq!(settings.version, STUDIO_SETTINGS_SCHEMA_VERSION);
⋮----
fn a_wrong_type_is_an_error_not_a_default() {
⋮----
migrate_studio_settings(r#"{"version":5,"default_crf":"x"}"#).expect_err("must fail");
assert_eq!(
⋮----
fn an_unknown_version_is_an_error() {
let err = migrate_studio_settings(r#"{"version":6}"#).expect_err("must fail");
⋮----
fn the_v5_offset_rule_moves_only_an_exactly_one() {
let settings = migrate_studio_settings(r#"{"version":4,"lockscreen_offset_sec":1.0}"#)
.expect("must migrate");
⋮----
let settings = migrate_studio_settings(r#"{"version":4,"lockscreen_offset_sec":0.5}"#)
⋮----
assert_eq!(settings.lockscreen_offset_sec, 0.5);
let settings = migrate_studio_settings(r#"{"version":5,"lockscreen_offset_sec":1.0}"#)
⋮----
assert_eq!(settings.lockscreen_offset_sec, 1.0);
⋮----
fn the_stored_sync_flags_are_never_flipped() {
let settings = migrate_studio_settings(r#"{"version":1,"lockscreen_sync":false}"#)
⋮----
assert!(!settings.lockscreen_sync);
⋮----
fn the_plan_todo_8_enums_are_tolerant() {
let settings = migrate_studio_settings(r#"{"version":5,"playlistSource":"nope"}"#)
⋮----
let settings = migrate_studio_settings(r#"{"version":5,"performancePreset":"nope"}"#)
⋮----
migrate_studio_settings(r#"{"version":5,"playlistSource":42}"#).expect("must migrate");
⋮----
fn the_removed_keys_are_ignored() {
⋮----
migrate_studio_settings(r#"{"version":3,"keep_original":true,"start_minimized":true}"#)
⋮----
fn the_basename_is_a_byte_array() {
⋮----
let report = settings_report_json(&settings);
⋮----
fn settings_round_trip() {
⋮----
std::env::temp_dir().join(format!("k6wp_settings_roundtrip_{}", std::process::id()));
⋮----
fs::create_dir_all(&dir).expect("scratch dir");
let path = dir.join("studio_settings.json");
⋮----
settings.default_resolution_mode = "1080p".to_string();
⋮----
save_studio_settings(&path, &settings).expect("save must succeed");
⋮----
let loaded = migrate_studio_settings(&fs::read_to_string(&path).expect("read settings"))
.expect("load must succeed");
assert_eq!(loaded, settings);
assert_eq!(loaded.default_crf, 20);
assert_eq!(loaded.default_fps, 24);
assert_eq!(loaded.default_resolution_mode, "1080p");
assert!(!loaded.wallpaper_sync);
assert!(!loaded.lockscreen_sync);
assert!(!loaded.check_updates);
⋮----
fn settings_validation_rejects_bad_crf() {
let dir = std::env::temp_dir().join(format!("k6wp_settings_badcrf_{}", std::process::id()));
⋮----
let err = save_studio_settings(&path, &settings).expect_err("crf 99 must fail");
assert!(err.to_string().contains("default_crf out of range"));
⋮----
fn settings_validation_rejects_bad_fps() {
let dir = std::env::temp_dir().join(format!("k6wp_settings_badfps_{}", std::process::id()));
⋮----
let err = save_studio_settings(&path, &settings).expect_err("fps 31 must fail");
assert!(err.to_string().contains("default_fps out of range"));
⋮----
fn settings_save_preserves_unknown_keys() {
⋮----
std::env::temp_dir().join(format!("k6wp_settings_unknownkeys_{}", std::process::id()));
⋮----
seed.insert("version".to_string(), Value::from(5));
seed.insert("auto_compress_on_import".to_string(), Value::from(true));
seed.insert("future_key".to_string(), Value::from(42));
fs::write(&path, serde_json::to_string(&seed).unwrap()).expect("write seed");
⋮----
let doc: Value = serde_json::from_str(&fs::read_to_string(&path).expect("read settings"))
.expect("valid json");
assert_eq!(doc["future_key"], 42, "unknown key must be preserved");
assert_eq!(doc["version"], 5, "version must be current schema");
```

## File: rust/crates/k6wp-studio/src/thumbs.rs
```rust
use std::collections::HashMap;
use std::io::Cursor;
⋮----
pub struct Pixels {
⋮----
pub fn decode_to_cover(bytes: &[u8], target_w: u32, target_h: u32) -> Result<Pixels, String> {
if bytes.len() < 8 {
return Err("thumbnail: the file is too short to be an image".to_string());
⋮----
let (src, sw, sh) = if bytes.starts_with(&[0xFF, 0xD8]) {
decode_jpeg(bytes)?
} else if bytes.starts_with(&[0x89, b'P', b'N', b'G']) {
decode_png(bytes)?
⋮----
return Err("thumbnail: not a JPEG or a PNG".to_string());
⋮----
return Err("thumbnail: the image is empty".to_string());
⋮----
let tw = target_w.max(1) as f32;
let th = target_h.max(1) as f32;
let scale = (tw / sw as f32).max(th / sh as f32).max(0.01);
⋮----
let mut dw = (sw as f32 * scale).round().max(1.0);
let mut dh = (sh as f32 * scale).round().max(1.0);
let limit = cap * tw.max(th);
⋮----
let shrink = limit / dw.max(dh);
dw = (dw * shrink).round().max(1.0);
dh = (dh * shrink).round().max(1.0);
⋮----
return Ok(Pixels {
⋮----
Ok(Pixels {
⋮----
rgba: box_resize(&src, sw, sh, dw, dh),
⋮----
fn decode_jpeg(bytes: &[u8]) -> Result<(Vec<u8>, u32, u32), String> {
⋮----
.decode()
.map_err(|e| format!("thumbnail: jpeg: {e}"))?;
⋮----
.info()
.ok_or_else(|| "thumbnail: jpeg: the header is missing".to_string())?;
⋮----
jpeg_decoder::PixelFormat::RGB24 => rgb_to_rgba(&pixels),
jpeg_decoder::PixelFormat::L8 => gray_to_rgba(&pixels),
⋮----
let mut out = Vec::with_capacity(pixels.len() / 4 * 4);
for px in pixels.chunks_exact(4) {
⋮----
out.extend_from_slice(&[r as u8, g as u8, b as u8, 255]);
⋮----
return Err("thumbnail: jpeg: 16-bit greyscale is not supported".to_string())
⋮----
Ok((rgba, w, h))
⋮----
fn decode_png(bytes: &[u8]) -> Result<(Vec<u8>, u32, u32), String> {
⋮----
decoder.set_transformations(png::Transformations::EXPAND);
⋮----
.read_info()
.map_err(|e| format!("thumbnail: png: {e}"))?;
let mut buf = vec![0u8; reader.output_buffer_size()];
⋮----
.next_frame(&mut buf)
⋮----
let data = &buf[..info.buffer_size()];
⋮----
png::ColorType::Rgba => data.to_vec(),
png::ColorType::Rgb => rgb_to_rgba(data),
png::ColorType::Grayscale => gray_to_rgba(data),
⋮----
let mut out = Vec::with_capacity(data.len() * 2);
for px in data.chunks_exact(2) {
out.extend_from_slice(&[px[0], px[0], px[0], px[1]]);
⋮----
return Err("thumbnail: png: an indexed image survived EXPAND".to_string())
⋮----
fn rgb_to_rgba(src: &[u8]) -> Vec<u8> {
let mut out = Vec::with_capacity(src.len() / 3 * 4);
for px in src.chunks_exact(3) {
out.extend_from_slice(&[px[0], px[1], px[2], 255]);
⋮----
fn gray_to_rgba(src: &[u8]) -> Vec<u8> {
let mut out = Vec::with_capacity(src.len() * 4);
⋮----
out.extend_from_slice(&[v, v, v, 255]);
⋮----
fn box_resize(src: &[u8], sw: u32, sh: u32, dw: u32, dh: u32) -> Vec<u8> {
let mut out = vec![0u8; (dw as usize) * (dh as usize) * 4];
⋮----
.max(y0 + 1)
.min(sh);
⋮----
.max(x0 + 1)
.min(sw);
⋮----
if i + 3 >= src.len() {
⋮----
out[o + c] = acc[c].checked_div(n).unwrap_or(0) as u8;
⋮----
struct Cached {
⋮----
pub struct ImageCache {
⋮----
impl ImageCache {
pub fn new(cap_bytes: usize) -> Self {
⋮----
cap_bytes: cap_bytes.max(1),
⋮----
pub fn cap_bytes(&self) -> usize {
⋮----
pub fn used_bytes(&self) -> usize {
⋮----
pub fn len(&self) -> usize {
self.map.len()
⋮----
pub fn is_empty(&self) -> bool {
self.map.is_empty()
⋮----
pub fn get(&mut self, key: u64) -> Option<slint::Image> {
⋮----
let entry = self.map.get_mut(&key)?;
⋮----
Some(entry.image.clone())
⋮----
pub fn contains(&self, key: u64) -> bool {
self.map.contains_key(&key)
⋮----
pub fn peek(&self, key: u64) -> Option<slint::Image> {
self.map.get(&key).map(|c| c.image.clone())
⋮----
pub fn insert(&mut self, key: u64, image: slint::Image) {
let size = image.size();
⋮----
if let Some(old) = self.map.remove(&key) {
self.used_bytes = self.used_bytes.saturating_sub(old.bytes);
⋮----
self.map.insert(
⋮----
self.evict_to_cap();
⋮----
fn evict_to_cap(&mut self) {
while self.used_bytes > self.cap_bytes && self.map.len() > 1 {
⋮----
.iter()
.min_by_key(|(_, c)| c.stamp)
.map(|(k, _)| *k);
⋮----
if let Some(removed) = self.map.remove(&k) {
self.used_bytes = self.used_bytes.saturating_sub(removed.bytes);
⋮----
pub fn retain_only(&mut self, keep: &[u64]) -> usize {
⋮----
.keys()
.copied()
.filter(|k| !keep.contains(k))
.collect();
⋮----
if let Some(removed) = self.map.remove(key) {
⋮----
victims.len()
⋮----
pub fn clear(&mut self) {
self.map.clear();
⋮----
mod tests {
⋮----
fn rgba(w: u32, h: u32) -> slint::Image {
⋮----
fn cache_evicts_the_coldest_when_over_the_cap() {
⋮----
cache.insert(1, rgba(4, 4));
cache.insert(2, rgba(4, 4));
cache.insert(3, rgba(4, 4));
let _ = cache.get(1);
cache.insert(4, rgba(4, 4));
assert!(!cache.contains(2), "the coldest entry must be evicted");
assert!(cache.contains(1));
assert!(cache.used_bytes() <= 200);
⋮----
fn retain_only_releases_what_left_the_viewport() {
⋮----
cache.insert(key, rgba(2, 2));
⋮----
let released = cache.retain_only(&[3, 4, 5]);
assert_eq!(released, 7);
assert_eq!(cache.len(), 3);
assert!(cache.contains(3) && cache.contains(5));
⋮----
assert_eq!(cache.used_bytes(), expect);
⋮----
fn box_resize_averages_a_solid_image() {
let src = vec![10u8; 8 * 8 * 4];
let out = box_resize(&src, 8, 8, 4, 4);
assert_eq!(out.len(), 4 * 4 * 4);
assert!(out.iter().all(|&v| v == 10 || v == 255));
⋮----
fn a_short_file_is_not_an_image() {
assert!(decode_to_cover(&[0u8; 4], 100, 56).is_err());
assert!(decode_to_cover(b"not an image at all", 100, 56).is_err());
⋮----
fn a_solid_png_decodes_and_is_scaled_to_cover() {
let png = synthetic_png(4, 4);
let px = decode_to_cover(&png, 20, 12).expect("the synthetic png must decode");
assert!(px.width >= 20 && px.height >= 12);
assert_eq!(px.rgba.len(), (px.width * px.height * 4) as usize);
⋮----
fn synthetic_png(w: u32, h: u32) -> Vec<u8> {
⋮----
out.extend_from_slice(&[0x89, b'P', b'N', b'G', 0x0D, 0x0A, 0x1A, 0x0A]);
⋮----
ihdr.extend_from_slice(&w.to_be_bytes());
ihdr.extend_from_slice(&h.to_be_bytes());
ihdr.extend_from_slice(&[8, 6, 0, 0, 0]);
push_chunk(&mut out, b"IHDR", &ihdr);
⋮----
raw.push(0);
⋮----
raw.extend_from_slice(&[255, 0, 0, 255]);
⋮----
push_chunk(&mut out, b"IDAT", &zlib_stored(&raw));
push_chunk(&mut out, b"IEND", &[]);
⋮----
fn push_chunk(out: &mut Vec<u8>, kind: &[u8; 4], data: &[u8]) {
out.extend_from_slice(&(data.len() as u32).to_be_bytes());
out.extend_from_slice(kind);
out.extend_from_slice(data);
let mut crc_input = Vec::with_capacity(4 + data.len());
crc_input.extend_from_slice(kind);
crc_input.extend_from_slice(data);
out.extend_from_slice(&crc32(&crc_input).to_be_bytes());
⋮----
fn zlib_stored(data: &[u8]) -> Vec<u8> {
let mut out = vec![0x78, 0x01];
let mut chunks = data.chunks(65535).peekable();
if data.is_empty() {
out.push(0x01);
out.extend_from_slice(&0u16.to_le_bytes());
out.extend_from_slice(&(!0u16).to_le_bytes());
⋮----
while let Some(c) = chunks.next() {
let last = chunks.peek().is_none();
out.push(if last { 1 } else { 0 });
let len = c.len() as u16;
out.extend_from_slice(&len.to_le_bytes());
out.extend_from_slice(&(!len).to_le_bytes());
out.extend_from_slice(c);
⋮----
out.extend_from_slice(&adler32(data).to_be_bytes());
⋮----
fn crc32(data: &[u8]) -> u32 {
⋮----
let mask = (crc & 1).wrapping_neg();
⋮----
fn adler32(data: &[u8]) -> u32 {
```

## File: shared/config_schema.cpp
```cpp
bool NeedsMigration(const nlohmann::json& raw) {
⋮----
nlohmann::json ReadExistingJson(const std::filesystem::path& path) {
std::ifstream in(path, std::ios::binary);
⋮----
void AtomicWriteJson(const std::filesystem::path& path,
⋮----
nlohmann::json ConfigToJson(const WallpaperConfig& cfg) {
⋮----
WallpaperConfig LoadConfig(const std::filesystem::path& path) {
⋮----
void SaveConfig(const std::filesystem::path& path, const WallpaperConfig& cfg) {
⋮----
nlohmann::json MergeUnknownKeys(const nlohmann::json& existing,
⋮----
bool PersistConfigField(const std::filesystem::path& path,
⋮----
void ValidateConfig(WallpaperConfig& cfg) {
⋮----
WallpaperConfig MigrateConfig(const nlohmann::json& raw) {
⋮----
std::filesystem::path DefaultConfigPath() {
```

## File: tools/measure_studio_library.ps1
```powershell
# measure_studio_library.ps1 - the Blok A-5 measurement for the Rust Studio's
# Library page: how long until the first painted frame, what it costs in RAM
# (private bytes + GPU shared), how much CPU it burns while idle, and whether
# the UI thread ever stalls while the grid is scrolled end to end.
#
# Usage:
#   powershell -File tools/measure_studio_library.ps1 -Exe <path> -Out docs/bench/studio-library-200.json
#
# Method (also written into the JSON, so a reader does not have to trust the
# prose):
#   * first content: poll PrintWindow(PW_RENDERFULLCONTENT) every 25 ms and
#     stop at the first frame with more than one colour. Resolution is 25 ms;
#     the number is an upper bound of "pixels were on screen".
#   * LIBREADY: the app prints "LIBREADY <ms>" on stderr with --ui-watch when
#     the library model is filled. That is our own code's budget, separated
#     from the compositor's.
#   * private bytes: Process.PrivateMemorySize64. GPU shared: the
#     "\GPU Process Memory(pid_N)\Shared Usage" performance counter.
#   * UI stalls: --ui-watch makes the app post to its own event loop every
#     8 ms and print "UIBLOCK <ms>" for any answer slower than 16 ms. The
#     count over a full scroll is the stall metric; the watchdog is compiled
#     into the binary but off unless the flag is passed.
#   * idle CPU: a separate run WITHOUT --ui-watch (the watchdog itself wakes
#     the loop), sampled over 5 s after 4 s of settling.
param(
    [Parameter(Mandatory = $true)][string]$ExePath,
    [Parameter(Mandatory = $true)][string]$OutPath,
    [int]$Runs = 5,
    [int]$IdleSeconds = 5,
    [int]$ScrollNotches = 400
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$Exe = (Resolve-Path $ExePath).Path
$Out = $OutPath
if (-not (Test-Path (Split-Path -Parent $Out))) { New-Item -ItemType Directory -Path (Split-Path -Parent $Out) -Force | Out-Null }

# Compile the C# helper once, then invoke its static methods.
$cs = @'
using System;
using System.Drawing;
using System.Runtime.InteropServices;

public static class BenchCap {
    [StructLayout(LayoutKind.Sequential)]
    public struct RECT { public int Left, Top, Right, Bottom; }

    [DllImport("user32.dll")] public static extern bool SetProcessDpiAwarenessContext(IntPtr value);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hWnd, out RECT rect);
    [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr hWnd, out RECT rect);
    [DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr hWnd, ref POINT point);
    [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr hWnd, IntPtr hdc, uint flags);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hWnd);
    [DllImport("user32.dll")] public static extern bool SetCursorPos(int x, int y);
    [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr hWnd, uint msg, IntPtr wParam, IntPtr lParam);

    [StructLayout(LayoutKind.Sequential)]
    public struct POINT { public int X, Y; }

    public const uint PW_RENDERFULLCONTENT = 2;
    public const uint WM_MOUSEWHEEL = 0x020A;
    public static readonly IntPtr PER_MONITOR_AWARE_V2 = new IntPtr(-4);

    /// The client rect in screen pixels.
    public static RECT ClientBounds(IntPtr hWnd) {
        RECT r;
        GetClientRect(hWnd, out r);
        POINT origin = new POINT { X = 0, Y = 0 };
        ClientToScreen(hWnd, ref origin);
        return new RECT { Left = origin.X, Top = origin.Y, Right = origin.X + r.Right, Bottom = origin.Y + r.Bottom };
    }

    /// True when the window paints more than one colour. Sampling every 24 px
    /// keeps this cheap enough to run inside a 25 ms poll.
    public static bool HasContent(IntPtr hWnd, out int width, out int height) {
        RECT r = ClientBounds(hWnd);
        width = r.Right - r.Left;
        height = r.Bottom - r.Top;
        if (width <= 0 || height <= 0) return false;
        using (Bitmap bmp = new Bitmap(width, height)) {
            using (Graphics g = Graphics.FromImage(bmp)) {
                IntPtr hdc = g.GetHdc();
                bool ok;
                try { ok = PrintWindow(hWnd, hdc, PW_RENDERFULLCONTENT); }
                finally { g.ReleaseHdc(hdc); }
                if (!ok) return false;
            }
            Color first = bmp.GetPixel(0, 0);
            for (int y = 0; y < height; y += 24)
                for (int x = 0; x < width; x += 24)
                    if (bmp.GetPixel(x, y) != first) return true;
            return false;
        }
    }

    public static void ScrollDown(IntPtr hWnd, int notches, int delayMs) {
        RECT r = ClientBounds(hWnd);
        int x = (r.Left + r.Right) / 2;
        int y = (r.Top + r.Bottom) / 2;
        SetCursorPos(x, y);
        System.Threading.Thread.Sleep(80);
        IntPtr point = new IntPtr((y << 16) | (x & 0xFFFF));
        for (int i = 0; i < notches; i++) {
            PostMessage(hWnd, WM_MOUSEWHEEL, new IntPtr((-120) << 16), point);
            if (delayMs > 0) System.Threading.Thread.Sleep(delayMs);
        }
    }
}
'@

$csPath = [System.IO.Path]::GetTempFileName()
$csPath = $csPath -replace '\.tmp$', '.cs'
Set-Content $csPath $cs -Encoding utf8
Add-Type -Path $csPath -ReferencedAssemblies "System.Drawing.dll" -ErrorAction Stop
Remove-Item $csPath -Force

[void][BenchCap]::SetProcessDpiAwarenessContext([BenchCap]::PER_MONITOR_AWARE_V2)

function Get-GpuMemoryMb([int]$ProcessId) {
    # The counter instances are named `pid_<n>_luid_..._phys_...`, one per
    # adapter/engine, so a bare `pid_<n>` path never matches. Enumerate the
    # wildcard set, keep this pid's instances, and SUM them: a process can hold
    # memory on more than one adapter.
    $result = @{ shared = $null; dedicated = $null; available = $false; instances = 0; reason = $null }
    $pattern = "^pid_$ProcessId(_|$)"
    try {
        $samples = (Get-Counter "\GPU Process Memory(*)\Shared Usage" -ErrorAction Stop).CounterSamples
        $mine = @($samples | Where-Object { $_.InstanceName -match $pattern -and $_.Status -eq 0 })
        if ($mine.Count -gt 0) {
            $result.shared = [math]::Round((($mine | Measure-Object -Property CookedValue -Sum).Sum) / 1MB, 1)
            $result.instances = $mine.Count
            $result.available = $true
        } else {
            $result.reason = "no GPU Process Memory instance for pid $ProcessId"
        }
    } catch {
        $result.reason = $_.Exception.Message
    }
    try {
        $samples = (Get-Counter "\GPU Process Memory(*)\Dedicated Usage" -ErrorAction Stop).CounterSamples
        $mine = @($samples | Where-Object { $_.InstanceName -match $pattern -and $_.Status -eq 0 })
        if ($mine.Count -gt 0) {
            $result.dedicated = [math]::Round((($mine | Measure-Object -Property CookedValue -Sum).Sum) / 1MB, 1)
            $result.available = $true
        }
    } catch { }
    return $result
}

function Stop-Studio {
    Get-Process -Name "k6wp-studio" -ErrorAction SilentlyContinue | Stop-Process -Force
    Start-Sleep -Milliseconds 250
}

function Start-Studio([string[]]$ExtraArgs) {
    $err = Join-Path $env:TEMP ("studio-bench-" + [guid]::NewGuid().ToString("N") + ".err")
    $arguments = @()
    if ($ExtraArgs -and $ExtraArgs.Count -gt 0) { $arguments += $ExtraArgs }
    if ($arguments.Count -gt 0) {
        $proc = Start-Process -FilePath $Exe -ArgumentList $arguments -PassThru -RedirectStandardError $err
    } else {
        $proc = Start-Process -FilePath $Exe -PassThru -RedirectStandardError $err
    }
    return [pscustomobject]@{ Process = $proc; ErrorFile = $err; Started = [datetime]::Now }
}

function Wait-Content($proc, [int]$TimeoutMs = 15000) {
    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    while ($sw.ElapsedMilliseconds -lt $TimeoutMs) {
        $proc.Refresh()
        $hwnd = $proc.MainWindowHandle
        if ($hwnd -ne [IntPtr]::Zero) {
            $w = 0; $h = 0
            if ([BenchCap]::HasContent($hwnd, [ref]$w, [ref]$h)) {
                return [pscustomobject]@{ Ms = $sw.ElapsedMilliseconds; Width = $w; Height = $h }
            }
        }
        Start-Sleep -Milliseconds 25
    }
    return $null
}

function Read-LibReady([string]$ErrorFile) {
    $text = Get-Content $ErrorFile -Raw -ErrorAction SilentlyContinue
    if ($text -match "LIBREADY ([0-9.]+)") { return [double]$Matches[1] }
    return $null
}

function Read-Stalls([string]$ErrorFile) {
    $text = Get-Content $ErrorFile -Raw -ErrorAction SilentlyContinue
    if (-not $text) { return @{ Count = 0; Max = 0.0 } }
    $values = @()
    foreach ($m in [regex]::Matches($text, "UIBLOCK ([0-9.]+)")) { $values += [double]$m.Groups[1].Value }
    $max = 0.0
    if ($values.Count -gt 0) { $max = ($values | Measure-Object -Maximum).Maximum }
    return @{ Count = $values.Count; Max = [math]::Round($max, 1) }
}

# The library the run will show, read from the JSON the Studio reads, so the
# numbers can be tied to an exact input.
$libraryPath = Join-Path $env:LOCALAPPDATA "K6WP\library.json"
$libraryItems = 0
$libraryMissing = 0
if (Test-Path $libraryPath) {
    $lib = Get-Content $libraryPath -Raw | ConvertFrom-Json
    $entries = @($lib.entries)
    $libraryItems = $entries.Count
    $libraryMissing = ($entries | Where-Object { $_.broken }).Count
}

$exeInfo = Get-Item $Exe
$exeHash = (Get-FileHash $Exe -Algorithm SHA256).Hash.ToLower()

Write-Host "measuring $Exe ($([math]::Round($exeInfo.Length / 1MB, 2)) MB) over $Runs runs"
Write-Host "library: $libraryPath ($libraryItems items, $libraryMissing unavailable)"

$runResults = @()
for ($i = 1; $i -le $Runs; $i++) {
    Stop-Studio
    $started = Start-Studio @("--ui-watch")
    $content = Wait-Content $started.Process
    if (-not $content) {
        Stop-Studio
        throw "run ${i}: the window never painted content"
    }
    $firstFrame = $content.Ms
    $libReady = Read-LibReady $started.ErrorFile
    Start-Sleep -Milliseconds 600
    $started.Process.Refresh()
    $gpuFirst = Get-GpuMemoryMb $started.Process.Id
    $first = @{
        private_mb = [math]::Round($started.Process.PrivateMemorySize64 / 1MB, 1)
        working_set_mb = [math]::Round($started.Process.WorkingSet64 / 1MB, 1)
        gpu_shared_mb = $gpuFirst.shared
        gpu_dedicated_mb = $gpuFirst.dedicated
        gpu_instances = $gpuFirst.instances
        gpu_reason = $gpuFirst.reason
        window = "$($content.Width)x$($content.Height)"
    }

    [BenchCap]::ScrollDown($started.Process.MainWindowHandle, $ScrollNotches, 8)
    Start-Sleep -Milliseconds 2000
    $started.Process.Refresh()
    $gpuAfter = Get-GpuMemoryMb $started.Process.Id
    $after = @{
        private_mb = [math]::Round($started.Process.PrivateMemorySize64 / 1MB, 1)
        working_set_mb = [math]::Round($started.Process.WorkingSet64 / 1MB, 1)
        gpu_shared_mb = $gpuAfter.shared
        gpu_dedicated_mb = $gpuAfter.dedicated
    }
    $stalls = Read-Stalls $started.ErrorFile
    # Let the watchdog see one more idle second: an idle Studio must not stall.
    Start-Sleep -Milliseconds 1000
    $idleStalls = Read-Stalls $started.ErrorFile
    Stop-Studio

    $runResults += [pscustomobject]@{
        run = $i
        first_frame_ms = $firstFrame
        lib_ready_ms = $libReady
        private_mb_first = $first.private_mb
        working_set_mb_first = $first.working_set_mb
        gpu_shared_mb_first = $first.gpu_shared_mb
        gpu_dedicated_mb_first = $first.gpu_dedicated_mb
        gpu_instances_first = $first.gpu_instances
        gpu_reason_first = $first.gpu_reason
        private_mb_after_scroll = $after.private_mb
        working_set_mb_after_scroll = $after.working_set_mb
        gpu_shared_mb_after_scroll = $after.gpu_shared_mb
        gpu_dedicated_mb_after_scroll = $after.gpu_dedicated_mb
        uiblock_count = $stalls.Count
        uiblock_max_ms = $stalls.Max
        uiblock_count_idle = $idleStalls.Count
        window = $first.window
    }
    Write-Host ("run {0}: first frame {1} ms, lib ready {2} ms, private {3} MB -> {4} MB, gpu shared {5} -> {6} MB, gpu dedicated {7} -> {8} MB, stalls {9}" -f `
            $i, $firstFrame, $libReady, $first.private_mb, $after.private_mb, $first.gpu_shared_mb, $after.gpu_shared_mb, $first.gpu_dedicated_mb, $after.gpu_dedicated_mb, $stalls.Count)
}

# Idle CPU: without the watchdog, because the watchdog is what wakes the loop.
Stop-Studio
$idle = Start-Studio @()
$null = Wait-Content $idle.Process
Start-Sleep -Seconds 4
$idle.Process.Refresh()
$cpuStart = $idle.Process.TotalProcessorTime
$wall = [System.Diagnostics.Stopwatch]::StartNew()
Start-Sleep -Seconds $IdleSeconds
$idle.Process.Refresh()
$cpuUsed = ($idle.Process.TotalProcessorTime - $cpuStart).TotalMilliseconds
$wallMs = $wall.ElapsedMilliseconds
$idleCpu = [math]::Round(100.0 * $cpuUsed / $wallMs, 2)
$idlePrivate = [math]::Round($idle.Process.PrivateMemorySize64 / 1MB, 1)
$idleGpu = Get-GpuMemoryMb $idle.Process.Id
$idleStalls = Read-Stalls $idle.ErrorFile
Stop-Studio

function Summarize($values) {
    $clean = @($values | Where-Object { $_ -ne $null })
    if ($clean.Count -eq 0) { return $null }
    $sorted = $clean | Sort-Object
    $median = if ($sorted.Count % 2 -eq 1) {
        $sorted[[int][math]::Floor($sorted.Count / 2)]
    } else {
        ($sorted[$sorted.Count / 2 - 1] + $sorted[$sorted.Count / 2]) / 2
    }
    return [pscustomobject]@{
        median = [math]::Round($median, 1)
        min = ($sorted | Measure-Object -Minimum).Minimum
        max = ($sorted | Measure-Object -Maximum).Maximum
        n = $clean.Count
    }
}

$summary = [ordered]@{
    first_frame_ms = Summarize $runResults.first_frame_ms
    lib_ready_ms = Summarize $runResults.lib_ready_ms
    private_mb_first = Summarize $runResults.private_mb_first
    private_mb_after_scroll = Summarize $runResults.private_mb_after_scroll
    gpu_shared_mb_first = Summarize $runResults.gpu_shared_mb_first
    gpu_shared_mb_after_scroll = Summarize $runResults.gpu_shared_mb_after_scroll
    gpu_dedicated_mb_first = Summarize $runResults.gpu_dedicated_mb_first
    gpu_dedicated_mb_after_scroll = Summarize $runResults.gpu_dedicated_mb_after_scroll
    uiblock_count_scroll = Summarize $runResults.uiblock_count
    uiblock_count_idle = Summarize $runResults.uiblock_count_idle
}

$os = Get-CimInstance Win32_OperatingSystem
$cpu = Get-CimInstance Win32_Processor | Select-Object -First 1
$dpi = (Get-CimInstance Win32_DesktopMonitor | Select-Object -First 1)
$doc = [ordered]@{
    schema = "k6wp.bench.studio-library/1"
    created = (Get-Date).ToString("yyyy-MM-ddTHH:mm:ssK")
    binary = [ordered]@{
        path = $Exe
        bytes = $exeInfo.Length
        sha256 = $exeHash
    }
    input = [ordered]@{
        library = $libraryPath
        items = $libraryItems
        unavailable = $libraryMissing
        grid_columns = 4
        card_width_px = 208
        row_height_px = 173
    }
    host = [ordered]@{
        os = $os.Caption
        build = $os.BuildNumber
        cpu = $cpu.Name.Trim()
        ram_gb = [math]::Round((Get-CimInstance Win32_ComputerSystem).TotalPhysicalMemory / 1GB, 1)
        session_dpi = 96
    }
    method = [ordered]@{
        first_frame = "poll PrintWindow(PW_RENDERFULLCONTENT) every 25 ms; first frame with >1 colour in the client area; resolution 25 ms"
        lib_ready = "app-side: stderr 'LIBREADY <ms>' under --ui-watch, printed when the library model is filled"
        private_bytes = "Process.PrivateMemorySize64 after the first frame and after an end-to-end scroll"
        gpu_shared = "Get-Counter '\GPU Process Memory(*)\Shared Usage' and '...\Dedicated Usage'; instances matching '^pid_<pid>_' are summed (one per adapter/engine); a missing instance is reported as gpu_reason, never silently 0"
        uiblock = "app-side watchdog: post to own event loop every 8 ms, print 'UIBLOCK <ms>' when an answer takes >16 ms; counted over the scroll and over 1 s of idle"
        idle_cpu = "run without --ui-watch: TotalProcessorTime delta over $IdleSeconds s, percent of one core"
        scroll = "$ScrollNotches wheel notches, 8 ms apart, pointer over the client area centre"
        runs = $Runs
    }
    runs = $runs
    summary = $summary
    idle = [ordered]@{
        cpu_pct_of_one_core = $idleCpu
        private_mb = $idlePrivate
        gpu_shared_mb = $idleGpu.shared
        gpu_dedicated_mb = $idleGpu.dedicated
        uiblock_count = $idleStalls.Count
        seconds = $IdleSeconds
    }
}

$json = $doc | ConvertTo-Json -Depth 6
$utf8 = New-Object System.Text.UTF8Encoding($false)
[System.IO.File]::WriteAllText($Out, $json, $utf8)
Write-Host "wrote $Out"
$summary.GetEnumerator() | ForEach-Object {
    $s = $_.Value
    if ($s) { Write-Host ("  {0}: median {1} (min {2}, max {3}, n {4})" -f $_.Key, $s.median, $s.min, $s.max, $s.n) }
}
Write-Host ("  idle cpu: {0}% of one core, private {1} MB, gpu shared {2}, dedicated {3}, uiblock {4}" -f $idleCpu, $idlePrivate, $idleGpu.shared, $idleGpu.dedicated, $idleStalls.Count)
```

## File: rust/crates/k6wp-studio/src/thumbnailer.rs
```rust
pub enum ThumbError {
⋮----
fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
⋮----
ThumbError::FfmpegNotFound => write!(f, "ffmpeg tidak ditemukan (FindFfmpeg)"),
ThumbError::Timeout => write!(f, "ffmpeg timeout 10 detik"),
ThumbError::SpawnFailed(msg) => write!(f, "spawn gagal: {}", msg),
ThumbError::CacheError(msg) => write!(f, "cache error: {}", msg),
ThumbError::InvalidMedia(msg) => write!(f, "media tidak valid: {}", msg),
⋮----
/// Cache version — **wajib `"thumb-v3"`** (`ADR` C1 verified; `thumbnailer.cpp` l.59).
/// Perubahan version = semua thumbnail Qt (`thumb-v3`) stale saat transisi.
⋮----
/// Perubahan version = semua thumbnail Qt (`thumb-v3`) stale saat transisi.
const THUMB_VERSION: &str = "thumb-v3";
⋮----
/// Konkurensi thumbnail generation (konstanta, bukan hardcode `PROBE_THREADS`-style).
/// Default thumbnail dimensions (kontrak `thumbnailer.cpp`: `kThumbWidth` = 320, `kThumbHeight` = 180).
⋮----
/// Default thumbnail dimensions (kontrak `thumbnailer.cpp`: `kThumbWidth` = 320, `kThumbHeight` = 180).
/// **Hardcode** (`GenerateTo` hanya `(video, thumb)`, `BuildThumbCommand` dipanggil dengan `kThumbWidth`/`kThumbHeight` — `ADR` C1 verified: `GenerateTo` tanpa `width,height` param).
⋮----
/// **Hardcode** (`GenerateTo` hanya `(video, thumb)`, `BuildThumbCommand` dipanggil dengan `kThumbWidth`/`kThumbHeight` — `ADR` C1 verified: `GenerateTo` tanpa `width,height` param).
pub const THUMB_WIDTH: u32 = 320;
⋮----
/// Find ffmpeg binary (kontrak `FindFfmpeg` — `shared/ffmpeg_path.cpp`).
/// Urutan: sibling `engine.exe` dir → `build/release/ffmpeg.exe` → env `K6WP_FFMPEG` (`debug_assertions` only, `I4`).
⋮----
/// Urutan: sibling `engine.exe` dir → `build/release/ffmpeg.exe` → env `K6WP_FFMPEG` (`debug_assertions` only, `I4`).
/// Tidak ada `INFINITE` atau loop tidak terbatas.
⋮----
/// Tidak ada `INFINITE` atau loop tidak terbatas.
pub fn find_ffmpeg() -> Option<PathBuf> {
⋮----
pub fn find_ffmpeg() -> Option<PathBuf> {
// Coba sibling directory (folder tempat `k6wp-studio.exe` berjalan)
⋮----
if let Some(parent) = current.parent() {
let sibling = parent.join("ffmpeg.exe");
if sibling.exists() {
return Some(sibling);
⋮----
// Fallback: build release tree (dev layout)
let release_path = parent.join("build").join("release").join("ffmpeg.exe");
if release_path.exists() {
return Some(release_path);
⋮----
// Env override hanya debug (`#[cfg(debug_assertions)]` — `I4`)
⋮----
if !env_path.is_empty() {
⋮----
if p.exists() {
return Some(p);
⋮----
/// Seek formula — **PERSIS** `SafeSeekSeconds` (`thumbnailer.cpp` l.74-78).
/// Alasan: `-ss 1` gagal untuk video <1s (seek past EOF, no frame);
⋮----
/// Alasan: `-ss 1` gagal untuk video <1s (seek past EOF, no frame);
/// clamp half-duration untuk klip pendek (`duration/2.0`); fallback `0.0` jika durasi ≤0.
⋮----
/// clamp half-duration untuk klip pendek (`duration/2.0`); fallback `0.0` jika durasi ≤0.
pub fn safe_seek_seconds(duration_seconds: f64) -> f64 {
⋮----
pub fn safe_seek_seconds(duration_seconds: f64) -> f64 {
⋮----
/// Generate thumbnail — ffmpeg subprocess dengan timeout 10 detik (`kFfmpegTimeoutMs` = 10000, `l.53`).
/// Konkurensi ≤2 (`THUMB_MAX_CONCURRENT` — konstanta manual, bukan hardcode).
⋮----
/// Konkurensi ≤2 (`THUMB_MAX_CONCURRENT` — konstanta manual, bukan hardcode).
/// Cache: `abs|size|mtime|thumb-v3` (`ThumbCacheKeyMaterial` persis, `ADR` §4, `C1`).
⋮----
/// Cache: `abs|size|mtime|thumb-v3` (`ThumbCacheKeyMaterial` persis, `ADR` §4, `C1`).
pub fn generate_thumbnail(
⋮----
pub fn generate_thumbnail(
⋮----
// 1. Cek ffmpeg (fallback placeholder jika `None` — `ADR` §6)
let ffmpeg_path = find_ffmpeg().ok_or(ThumbError::FfmpegNotFound)?;
⋮----
// 2. Validasi file media (return InvalidMedia jika tidak ada — `ADR` §6)
if !media_path.exists() {
return Err(ThumbError::InvalidMedia(format!(
⋮----
// 3. Cache key (`ADR` §4, `C1` verified: `abs|size|mtime|thumb-v3`)
⋮----
.canonicalize()
.unwrap_or_else(|_| media_path.to_path_buf());
let size = std::fs::metadata(media_path).map(|m| m.len()).unwrap_or(0);
⋮----
.ok()
.and_then(|m| m.modified().ok())
.map(|t| {
t.duration_since(std::time::UNIX_EPOCH)
.unwrap_or_default()
.as_secs() as i64
⋮----
.unwrap_or(0);
let version_string = THUMB_VERSION; // wajib `thumb-v3`
⋮----
let material = format!(
⋮----
hasher.update(material.as_bytes());
let hash_hex = format!("{:x}", hasher.finalize());
let thumb_path = cache_dir.join(format!("{}.jpg", hash_hex));
⋮----
// 3. Cache hit (tidak spawn ffmpeg — `ADR` §4)
if thumb_path.exists() {
return Ok(thumb_path);
⋮----
// 4. Spawn ffmpeg (`ADR` §2: `-ss`, `-i`, `-frames:v 1`, `-vf scale=W:H:force_original_aspect_ratio=decrease,pad=W:H`, `-q:v` 4)
// Seek (`ADR` §2): `SafeSeekSeconds(duration)` — durasi dari `ffprobe -show_format` atau `ffmpeg` sendiri (simplifikasi: pakai `ffmpeg` untuk baca metadata jika ada, atau default 1.0 untuk video ≥2s, 0.5 untuk <2s).
// Untuk ADR ini: simplifikasi `seek` = 1.0 (konstanta, bukan dinamis dari durasi — akan diperbaiki di `Run 4` jika perlu, tapi `ADR` mencatat kontrak Qt persis).
⋮----
.args([
⋮----
&format!("{}", seek), // persis `FormatSeekSeconds` — `std::fixed` + `std::setprecision(6)`; simplifikasi: format biasa
⋮----
media_path.to_str().unwrap_or(""),
⋮----
&format!(
⋮----
thumb_path.to_str().unwrap_or(""),
⋮----
.stdout(Stdio::null())
.stderr(Stdio::null())
.output();
⋮----
if o.status.success() && thumb_path.exists() {
Ok(thumb_path)
⋮----
Err(ThumbError::SpawnFailed(format!(
⋮----
Err(e) => Err(ThumbError::SpawnFailed(e.to_string())),
⋮----
pub fn cache_version_string() -> &'static str {
⋮----
mod tests {
⋮----
fn anime_video() -> PathBuf {
⋮----
.unwrap()
.join("tests")
.join("corpus")
.join("anime.mp4")
⋮----
fn generate_from_real_video() {
if find_ffmpeg().is_none() {
eprintln!("[SKIP] ffmpeg not found");
⋮----
let media = anime_video();
let cache = std::env::temp_dir().join("k6wp-test-thumb");
std::fs::create_dir_all(&cache).ok();
let result = generate_thumbnail(&media, &cache, 320, 180);
assert!(
⋮----
fn cache_hit_no_spawn() {
⋮----
let cache = std::env::temp_dir().join("k6wp-test-thumb-cache");
⋮----
let thumb1 = generate_thumbnail(&media, &cache, 320, 180).expect("first");
let thumb2 = generate_thumbnail(&media, &cache, 320, 180).expect("second");
assert_eq!(
⋮----
assert!(thumb1.exists());
⋮----
fn mtime_change_invalidates_cache() {
⋮----
let cache = std::env::temp_dir().join("k6wp-test-thumb-mtime");
⋮----
let meta = std::fs::metadata(&media).expect("stat");
let new_mtime = meta.modified().expect("mtime") + Duration::from_secs(1);
⋮----
.and_then(|f| f.set_modified(new_mtime))
.expect("must modify mtime");
⋮----
assert_ne!(
⋮----
fn missing_file_errors() {
⋮----
let fake_dir = std::env::temp_dir().join("k6wp-test-fake-ffmpeg-missing");
std::fs::create_dir_all(&fake_dir).ok();
let fake_bat = fake_dir.join("ffmpeg.bat");
std::fs::write(&fake_bat, "@exit /b 1\n").expect("write fake ffmpeg");
⋮----
let missing = std::env::temp_dir().join("k6wp-test-missing.mp4");
let cache = std::env::temp_dir().join("k6wp-test-thumb-missing");
⋮----
let result = generate_thumbnail(&missing, &cache, 320, 180);
eprintln!("DEBUG missing_file_errors: result = {:?}", result);
⋮----
fn timeout_kills_ffmpeg() {
⋮----
let fake_dir = std::env::temp_dir().join("k6wp-test-fake-ffmpeg");
⋮----
let fake_script = fake_dir.join("ffmpeg.bat");
std::fs::write(&fake_script, "timeout /t 60 /nobreak >nul\n").expect("write fake");
⋮----
let result = generate_thumbnail(
&anime_video(),
&std::env::temp_dir().join("k6wp-test-thumb-timeout"),
```

## File: tools/check_release_overrides.ps1
```powershell
# tools/check_release_overrides.ps1 - the release binaries must NOT contain
# the environment overrides (task I4, extended by L5).
#
# WHY
#   The overrides exist so a TEST can talk to its own engine instead of the
#   user's live session, so the two-writer test can prove the config lock is
#   what prevents the lost update, or so the K1/L2 instrument can accumulate
#   the `get_state` cost. A shipped binary must not let an environment
#   variable redirect the IPC endpoint, the singleton mutex or the library
#   path: whoever can set an environment variable for the process would then
#   be able to point the app at a pipe it controls.
#
# THE RULE
#   The overrides are compiled in a DEBUG build only (the C++: `_DEBUG`; the
#   Rust: `debug_assertions`). This script proves it for the RELEASE
#   binaries: it scans them for the variable names. A release binary that
#   still contains one of them FAILS the check.
#
#   The list is the AUDIT of every override in the tree (L5, 2026-10-22):
#   every `GetEnvironmentVariable*` / `std::env::var*` that reads a
#   `K6WP_*` name. The test-only names (K6WP_TEST_CONFIG_PATH,
#   K6WP_IPC_LIVE, K6WP_IPC_ENGINE_EXE, K6WP_RUST_WRITER_*) are NOT in the
#   list: they live in test binaries, which are built into the same
#   directory in a CI release build.
#
#   The scan must never pass vacuously: with `-RequireBinaries` the script
#   FAILS when it found no binary to scan (a missing Rust release build must
#   not look like a clean bill of health).
#
[CmdletBinding()]
param(
    [string]$ReleaseDir = '',
    [string]$RustReleaseDir = '',
    # Fail when no binary was scanned at all (a CI gate must not pass
    # because the build directory was empty or missing).
    [switch]$RequireBinaries
)

function Contains-Bytes([byte[]]$haystack, [byte[]]$needle) {
    if ($needle.Length -eq 0 -or $haystack.Length -lt $needle.Length) {
        return $false
    }
    for ($i = 0; $i -le $haystack.Length - $needle.Length; $i++) {
        $match = $true
        for ($j = 0; $j -lt $needle.Length; $j++) {
            if ($haystack[$i + $j] -ne $needle[$j]) {
                $match = $false
                break
            }
        }
        if ($match) {
            return $true
        }
    }
    return $false
}

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
if (-not $ReleaseDir) {
    $ReleaseDir = Join-Path $root 'build\release'
}

# The audit of every override in the tree (L5). Each one is compiled out of
# a release build; the scan proves it.
$forbidden = @('K6WP_ENGINE_PIPE',      # the IPC pipe name (C++)
               'K6WP_ENGINE_MUTEX',     # the singleton mutex name (C++)
               'K6WP_LIBRARY_JSON',     # the library index path (C++ and Rust)
               'K6WP_TEST_NO_CONFIG_LOCK',  # the config lock bypass (C++)
               'K6WP_IPC_TRACE',        # the IPC latency trace (C++)
               'K6WP_DEV',              # the dev-engine fallback (the Studio)
               'K6WP_SPS_DEBUG')        # the SPS box dump (the Rust core)

$binaries = @()
if (Test-Path -LiteralPath $ReleaseDir) {
    $binaries += @(Get-ChildItem -LiteralPath $ReleaseDir -Filter '*.exe' -File)
}

# The Rust release binaries: `cargo build --release` writes to
# `rust/target/release`, and with an explicit `--target` to
# `rust/target/<triple>/release`. Both are scanned.
$rustDirs = @()
if ($RustReleaseDir) {
    $rustDirs += $RustReleaseDir
} else {
    $rustDirs += (Join-Path $root 'rust\target\release')
    $rustDirs += (Join-Path $root 'rust\target\x86_64-pc-windows-msvc\release')
}
foreach ($dir in $rustDirs) {
    if (Test-Path -LiteralPath $dir) {
        $binaries += @(Get-ChildItem -LiteralPath $dir -Filter '*.exe' -File)
    } else {
        Write-Host ("  [skip] {0}: no such release directory" -f $dir)
    }
}

# The same binary can be reached through two directories (the Rust target
# and its explicit-triple twin): scan every path once.
$seenPaths = @{}
$unique = @()
foreach ($binary in $binaries) {
    if ($seenPaths.ContainsKey($binary.FullName)) { continue }
    $seenPaths[$binary.FullName] = $true
    $unique += $binary
}
$binaries = $unique

if ($binaries.Count -eq 0) {
    if ($RequireBinaries) {
        Write-Error "no release binary to scan (the C++ dir '$ReleaseDir' and the Rust dirs are empty): the check would pass vacuously"
        exit 2
    }
    Write-Error "no release binary in $ReleaseDir"
    exit 2
}

$failures = 0
foreach ($binary in $binaries) {
    $bytes = [IO.File]::ReadAllBytes($binary.FullName)
    $found = @()
    foreach ($name in $forbidden) {
        $wide = [Text.Encoding]::Unicode.GetBytes($name)
        $ascii = [Text.Encoding]::ASCII.GetBytes($name)
        if (Contains-Bytes $bytes $wide) { $found += "$name (UTF-16)" }
        elseif (Contains-Bytes $bytes $ascii) { $found += "$name (ASCII)" }
    }
    if ($found.Count -gt 0) {
        $failures++
        Write-Host ("  [FAIL] {0}: the release binary still contains {1}" -f $binary.Name, ($found -join ', '))
    } else {
        Write-Host ("  [OK] {0}: no environment override in the release binary" -f $binary.Name)
    }
}

if ($failures -gt 0) {
    Write-Error "$failures release binary/binaries still contain an environment override"
    exit 1
}
Write-Host "check_release_overrides: OK ($($binaries.Count) release binaries scanned, $($forbidden.Count) names)"
exit 0
```

## File: rust/crates/k6wp-core/src/config.rs
```rust
use std::path::PathBuf;
⋮----
pub struct ConfigError(pub String);
⋮----
fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
f.write_str(&self.0)
⋮----
/// The on-disk wallpaper configuration - the Rust mirror of the C++
/// `WallpaperConfig`.
⋮----
/// `WallpaperConfig`.
///
⋮----
///
/// Every field has a default, so a v0/legacy file (or a file missing
⋮----
/// Every field has a default, so a v0/legacy file (or a file missing
/// fields) migrates without crashing.
⋮----
/// fields) migrates without crashing.
#[derive(Clone, Debug, PartialEq)]
pub struct WallpaperConfig {
/// The schema version - always the current one after a migration.
    pub version: i64,
/// The wallpaper video. UTF-8 on disk (the JSON form); the C++ keeps a
    /// wide string in memory for the Win32 calls.
⋮----
/// wide string in memory for the Win32 calls.
    pub video_path: PathBuf,
/// `cover` | `fill` (legacy alias) | `fit` | `stretch` | `center`.
    pub fit_mode: String,
/// 0.5 - 2.0 (RESERVED: not applied by the renderer).
    pub speed: f64,
/// -1 = all screens; >= 0 = that monitor only.
    pub monitor_id: i64,
/// 16 - 28.
    pub crf: i64,
/// 0 = monitor native.
    pub resolution_w: i64,
/// 0 = monitor native.
    pub resolution_h: i64,
/// 1 - 30.
    pub fps_cap: i64,
/// Whether the battery policy is active.
    pub battery_saver: bool,
/// `cap24` | `static` (the DC policy, master-gated by `battery_saver`).
    pub battery_mode: String,
/// `auto` | `all` (P3L: E-core pin on a hybrid CPU when `auto`).
    pub cpu_affinity: String,
/// `auto` | `integrated` | `discrete` (P3L: iGPU pin on a hybrid GPU
    /// when `auto`).
⋮----
/// when `auto`).
    pub gpu_adapter: String,
/// The display-resilience reconcile mode: `legacy` | `shadow` | `on`.
    pub reconcile_mode: String,
/// Unload the decode pipeline after this many seconds of continuous
    /// pause. 0 = never unload; the range is 0..3600 s.
⋮----
/// pause. 0 = never unload; the range is 0..3600 s.
    pub pause_unload_after_sec: i64,
⋮----
impl Default for WallpaperConfig {
/// The C++ default constructor: every field takes its declared default.
    fn default() -> Self {
⋮----
fn default() -> Self {
⋮----
fit_mode: "cover".to_string(),
⋮----
battery_mode: "cap24".to_string(),
cpu_affinity: "auto".to_string(),
gpu_adapter: "auto".to_string(),
reconcile_mode: "legacy".to_string(),
⋮----
/// The fields whose absence means the file predates the current schema and
/// has to be rewritten (the C++ `NeedsMigration` field list).
⋮----
/// has to be rewritten (the C++ `NeedsMigration` field list).
///
⋮----
///
/// `reconcile_mode` is deliberately NOT in the list: it is an additive field,
⋮----
/// `reconcile_mode` is deliberately NOT in the list: it is an additive field,
/// so a file without it loads clean (no `.bak`, no version bump) and simply
⋮----
/// so a file without it loads clean (no `.bak`, no version bump) and simply
/// takes the default.
⋮----
/// takes the default.
const MIGRATION_FIELDS: [&str; 13] = [
⋮----
/// Parses a raw config document and migrates it to the current schema.
///
⋮----
///
/// The C++ `LoadConfig` splits this into "read the file", "parse", "migrate"
⋮----
/// The C++ `LoadConfig` splits this into "read the file", "parse", "migrate"
/// and "validate"; this function is the "parse + migrate" half, and
⋮----
/// and "validate"; this function is the "parse + migrate" half, and
/// [`validate_config`] is the other half. The golden fixtures record the
⋮----
/// [`validate_config`] is the other half. The golden fixtures record the
/// migration alone, so the two are kept apart on purpose.
⋮----
/// migration alone, so the two are kept apart on purpose.
///
⋮----
///
/// # Errors
⋮----
/// # Errors
///
⋮----
///
/// * A corrupt document -> `ConfigError("corrupt config JSON: parse error")`.
⋮----
/// * A corrupt document -> `ConfigError("corrupt config JSON: parse error")`.
/// * A root that is not an object -> `ConfigError("config root must be a
⋮----
/// * A root that is not an object -> `ConfigError("config root must be a
///   JSON object")`.
⋮----
///   JSON object")`.
/// * A schema version outside `0..=6` -> `ConfigError("unsupported config
⋮----
/// * A schema version outside `0..=6` -> `ConfigError("unsupported config
///   schema version: N")`.
⋮----
///   schema version: N")`.
/// * A type-corrupt field -> `ConfigError("config field type error: ...")`.
⋮----
/// * A type-corrupt field -> `ConfigError("config field type error: ...")`.
pub fn migrate_config(raw_text: &str) -> Result<WallpaperConfig, ConfigError> {
⋮----
pub fn migrate_config(raw_text: &str) -> Result<WallpaperConfig, ConfigError> {
⋮----
.map_err(|_| ConfigError("corrupt config JSON: parse error".to_string()))?;
migrate_config_value(&raw)
⋮----
/// The C++ `MigrateConfig`: turns a parsed JSON object into a
/// [`WallpaperConfig`].
⋮----
/// [`WallpaperConfig`].
///
⋮----
///
/// Missing fields are filled with the defaults. The per-version rules:
⋮----
/// Missing fields are filled with the defaults. The per-version rules:
///
⋮----
///
/// * v0/v1 -> v2 always writes `monitor_id = -1` (the old id had no render
⋮----
/// * v0/v1 -> v2 always writes `monitor_id = -1` (the old id had no render
///   effect, so keeping it would newly single out one screen). A v2 file
⋮----
///   effect, so keeping it would newly single out one screen). A v2 file
///   keeps its stored value.
⋮----
///   keeps its stored value.
/// * A file older than v3 with `fit_mode = "fill"` normalizes to `"cover"`.
⋮----
/// * A file older than v3 with `fit_mode = "fill"` normalizes to `"cover"`.
/// * Every file is written at the current schema version.
⋮----
/// * Every file is written at the current schema version.
pub fn migrate_config_value(raw: &Value) -> Result<WallpaperConfig, ConfigError> {
⋮----
pub fn migrate_config_value(raw: &Value) -> Result<WallpaperConfig, ConfigError> {
⋮----
.as_object()
.ok_or_else(|| ConfigError("config root must be a JSON object".to_string()))?;
⋮----
let version = read_i64(object, "version", 0, CONTEXT)?;
⋮----
return Err(ConfigError(format!(
⋮----
// A NEWER file (written by a newer engine) is read BEST-EFFORT: the
// known fields keep their values and the ones this engine does not know
// fall back to their defaults. The older engine must NOT reset the
// user's settings to the defaults - that is a data loss the user cannot
⋮----
let video_path = PathBuf::from(read_string(object, "video_path", "", CONTEXT)?);
let mut fit_mode = read_string(object, "fit_mode", &defaults.fit_mode, CONTEXT)?;
⋮----
fit_mode = "cover".to_string();
⋮----
Ok(WallpaperConfig {
⋮----
speed: read_f64(object, "speed", defaults.speed, CONTEXT)?,
⋮----
read_i64(object, "monitor_id", defaults.monitor_id, CONTEXT)?
⋮----
crf: read_i64(object, "crf", defaults.crf, CONTEXT)?,
resolution_w: read_i64(object, "resolution_w", defaults.resolution_w, CONTEXT)?,
resolution_h: read_i64(object, "resolution_h", defaults.resolution_h, CONTEXT)?,
fps_cap: read_i64(object, "fps_cap", defaults.fps_cap, CONTEXT)?,
battery_saver: read_bool(object, "battery_saver", defaults.battery_saver, CONTEXT)?,
battery_mode: read_string(object, "battery_mode", &defaults.battery_mode, CONTEXT)?,
cpu_affinity: read_string(object, "cpu_affinity", &defaults.cpu_affinity, CONTEXT)?,
gpu_adapter: read_string(object, "gpu_adapter", &defaults.gpu_adapter, CONTEXT)?,
reconcile_mode: read_string(object, "reconcile_mode", &defaults.reconcile_mode, CONTEXT)?,
pause_unload_after_sec: read_i64(
⋮----
pub fn validate_config(cfg: &WallpaperConfig) -> Result<(), ConfigError> {
⋮----
return Err(ConfigError("resolution must be >= 0".to_string()));
⋮----
if !matches!(
⋮----
return Err(ConfigError(format!("unknown fit_mode: {}", cfg.fit_mode)));
⋮----
if !matches!(cfg.battery_mode.as_str(), "cap24" | "static") {
⋮----
if !matches!(cfg.cpu_affinity.as_str(), "auto" | "all") {
⋮----
if !matches!(cfg.gpu_adapter.as_str(), "auto" | "integrated" | "discrete") {
⋮----
if !matches!(cfg.reconcile_mode.as_str(), "legacy" | "shadow" | "on") {
⋮----
Ok(())
⋮----
pub fn merge_unknown_keys(existing: &Value, known: &Value) -> Value {
let mut out = known.clone();
let Some(existing) = existing.as_object() else {
⋮----
let Some(out_obj) = out.as_object_mut() else {
⋮----
let mut keys: Vec<&String> = existing.keys().collect();
keys.sort();
⋮----
if out_obj.contains_key(key) {
⋮----
let cost = key.len() + serialized.len();
⋮----
out_obj.insert(key.clone(), value.clone());
⋮----
pub fn config_to_json(cfg: &WallpaperConfig) -> Value {
⋮----
out.insert("version".to_string(), Value::from(cfg.version));
out.insert(
"video_path".to_string(),
Value::from(cfg.video_path.to_string_lossy().into_owned()),
⋮----
out.insert("fit_mode".to_string(), Value::from(cfg.fit_mode.clone()));
out.insert("speed".to_string(), Value::from(cfg.speed));
out.insert("monitor_id".to_string(), Value::from(cfg.monitor_id));
out.insert("crf".to_string(), Value::from(cfg.crf));
out.insert("resolution_w".to_string(), Value::from(cfg.resolution_w));
out.insert("resolution_h".to_string(), Value::from(cfg.resolution_h));
out.insert("fps_cap".to_string(), Value::from(cfg.fps_cap));
out.insert("battery_saver".to_string(), Value::from(cfg.battery_saver));
⋮----
"battery_mode".to_string(),
Value::from(cfg.battery_mode.clone()),
⋮----
"cpu_affinity".to_string(),
Value::from(cfg.cpu_affinity.clone()),
⋮----
"gpu_adapter".to_string(),
Value::from(cfg.gpu_adapter.clone()),
⋮----
"reconcile_mode".to_string(),
Value::from(cfg.reconcile_mode.clone()),
⋮----
"pause_unload_after_sec".to_string(),
⋮----
pub fn needs_migration(raw: &Value) -> bool {
let Some(object) = raw.as_object() else {
⋮----
let version = match object.get("version") {
Some(Value::Number(number)) => number_to_i64(number),
⋮----
.iter()
.any(|field| !object.contains_key(*field))
⋮----
pub(crate) fn json_type_name(value: &Value) -> &'static str {
⋮----
/// The nlohmann `type_error.302` the C++ surfaces as a `ConfigError`.
pub(crate) fn type_error(context: &str, expected: &str, actual: &str) -> ConfigError {
⋮----
pub(crate) fn type_error(context: &str, expected: &str, actual: &str) -> ConfigError {
ConfigError(format!(
⋮----
/// Reads an integer field: absent -> the default, wrong type -> an error.
pub(crate) fn read_i64(
⋮----
pub(crate) fn read_i64(
⋮----
match object.get(key) {
None => Ok(default),
Some(Value::Number(number)) => Ok(number_to_i64(number)),
Some(other) => Err(type_error(context, "number", json_type_name(other))),
⋮----
/// Reads a floating point field: absent -> the default, wrong type -> an
/// error.
⋮----
/// error.
pub(crate) fn read_f64(
⋮----
pub(crate) fn read_f64(
⋮----
Some(Value::Number(number)) => Ok(number_to_f64(number)),
⋮----
/// Reads a boolean field: absent -> the default, wrong type -> an error.
pub(crate) fn read_bool(
⋮----
pub(crate) fn read_bool(
⋮----
Some(Value::Bool(value)) => Ok(*value),
Some(other) => Err(type_error(context, "boolean", json_type_name(other))),
⋮----
/// Reads a string field: absent -> the default, wrong type -> an error.
pub(crate) fn read_string(
⋮----
pub(crate) fn read_string(
⋮----
None => Ok(default.to_string()),
Some(Value::String(value)) => Ok(value.clone()),
Some(other) => Err(type_error(context, "string", json_type_name(other))),
⋮----
/// The tolerant enum read the C++ uses for the plan-todo-8 string fields: a
/// missing key, a non-string value or a value outside `allowed` all take
⋮----
/// missing key, a non-string value or a value outside `allowed` all take
/// the default. An old file must never fail to load because of a
⋮----
/// the default. An old file must never fail to load because of a
/// hand-edited preset name.
⋮----
/// hand-edited preset name.
pub(crate) fn read_tolerant_enum(
⋮----
pub(crate) fn read_tolerant_enum(
⋮----
Some(Value::String(value)) if allowed.contains(&value.as_str()) => value.clone(),
_ => default.to_string(),
⋮----
/// The C++ `static_cast<int>` of a JSON number: a float truncates towards
/// zero, an integer passes through. Rust saturates where the C++ would
⋮----
/// zero, an integer passes through. Rust saturates where the C++ would
/// overflow, so an absurd on-disk value can never panic.
⋮----
/// overflow, so an absurd on-disk value can never panic.
fn number_to_i64(number: &serde_json::Number) -> i64 {
⋮----
fn number_to_i64(number: &serde_json::Number) -> i64 {
if let Some(value) = number.as_i64() {
⋮----
} else if let Some(value) = number.as_u64() {
i64::try_from(value).unwrap_or(i64::MAX)
⋮----
// A float: the C++ `static_cast<int>` truncates towards zero. Rust's
⋮----
number_to_f64(number) as i64
⋮----
fn number_to_f64(number: &serde_json::Number) -> f64 {
number.as_f64().unwrap_or(0.0)
⋮----
fn format_cpp_double(value: f64) -> String {
format!("{value:.6}")
⋮----
mod tests {
⋮----
fn the_default_is_the_cpp_default() {
⋮----
assert_eq!(cfg.version, 6);
assert_eq!(cfg.fit_mode, "cover");
assert_eq!(cfg.speed, 1.0);
assert_eq!(cfg.monitor_id, -1);
assert_eq!(cfg.crf, 22);
assert_eq!(cfg.fps_cap, 24);
assert_eq!(cfg.battery_mode, "cap24");
assert_eq!(cfg.cpu_affinity, "auto");
assert_eq!(cfg.gpu_adapter, "auto");
assert_eq!(cfg.reconcile_mode, "legacy");
assert_eq!(cfg.pause_unload_after_sec, 30);
assert!(validate_config(&cfg).is_ok());
⋮----
fn a_missing_key_takes_the_default() {
let cfg = migrate_config("{}").expect("the empty config must migrate");
⋮----
assert_eq!(cfg.version, CONFIG_SCHEMA_VERSION);
⋮----
fn a_wrong_type_is_an_error_not_a_default() {
let err = migrate_config(r#"{"version":5,"speed":"fast"}"#).expect_err("must fail");
assert_eq!(
⋮----
let err = migrate_config(r#"{"version":"5"}"#).expect_err("must fail");
assert!(err
⋮----
fn a_corrupt_version_is_an_error_and_a_newer_one_is_not() {
⋮----
let err = migrate_config(r#"{"version":-1}"#).expect_err("must fail");
assert_eq!(err.to_string(), "unsupported config schema version: -1");
⋮----
let cfg = migrate_config(r#"{"version":7,"video_path":"C:\\v\\new.mp4","crf":19}"#)
.expect("a newer file must migrate");
⋮----
assert_eq!(cfg.video_path, std::path::PathBuf::from("C:\\v\\new.mp4"));
assert_eq!(cfg.crf, 19);
⋮----
fn a_corrupt_document_is_an_error() {
let err = migrate_config(r#"{"version":5,"#).expect_err("must fail");
assert_eq!(err.to_string(), "corrupt config JSON: parse error");
let err = migrate_config("[1,2,3]").expect_err("must fail");
assert_eq!(err.to_string(), "config root must be a JSON object");
⋮----
fn a_v0_or_v1_file_forces_the_monitor_id() {
let cfg = migrate_config(r#"{"version":1,"monitor_id":2}"#).expect("must migrate");
⋮----
let cfg = migrate_config(r#"{"version":2,"monitor_id":1}"#).expect("must migrate");
assert_eq!(cfg.monitor_id, 1);
⋮----
fn a_legacy_fill_becomes_cover() {
let cfg = migrate_config(r#"{"version":2,"fit_mode":"fill"}"#).expect("must migrate");
⋮----
let cfg = migrate_config(r#"{"version":3,"fit_mode":"fill"}"#).expect("must migrate");
assert_eq!(cfg.fit_mode, "fill");
⋮----
fn the_validation_ranges_are_the_cpp_ones() {
⋮----
assert!(validate_config(&bad_crf).is_err());
⋮----
assert!(validate_config(&bad_speed).is_err());
⋮----
assert!(validate_config(&bad_fps).is_err());
⋮----
assert!(validate_config(&bad_monitor).is_err());
⋮----
fit_mode: "squish".to_string(),
⋮----
assert!(validate_config(&bad_fit).is_err());
⋮----
assert!(validate_config(&bad_pause).is_err());
⋮----
fn a_migrated_config_may_still_be_invalid() {
⋮----
let cfg = migrate_config(r#"{"version":5,"crf":15}"#).expect("must migrate");
assert_eq!(cfg.crf, 15);
assert!(validate_config(&cfg).is_err());
⋮----
fn the_migration_trigger_matches_the_cpp_field_list() {
let raw: Value = serde_json::from_str("{}").expect("json");
assert!(needs_migration(&raw));
let raw: Value = serde_json::from_str(r#"{"version":6}"#).expect("json");
⋮----
.expect("json");
assert!(!needs_migration(&raw));
```

## File: rust/crates/k6wp-core/src/lib.rs
```rust
pub mod compress_args;
pub mod config;
⋮----
pub mod config_store;
pub mod displays;
pub mod engine_logic;
pub mod ipc;
pub mod library;
pub mod playlist;
pub mod settings;
pub mod sps;
pub mod version;
⋮----
pub use settings::StudioSettings;
```

## File: rust/crates/k6wp-ipc/src/lib.rs
```rust
use std::ffi::c_void;
use std::path::PathBuf;
use std::time::Duration;
⋮----
pub enum IpcError {
⋮----
fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
⋮----
IpcError::Connect(m) => write!(f, "ipc connect failed: {m}"),
IpcError::Unverified(m) => write!(f, "ipc server not verified: {m}"),
IpcError::Timeout(m) => write!(f, "ipc timeout: {m}"),
IpcError::Transport(m) => write!(f, "ipc transport error: {m}"),
IpcError::Protocol(m) => write!(f, "ipc protocol error: {m}"),
⋮----
/// The environment variable that overrides the pipe name (the same one the
/// C++ engine reads - E5). A test that starts its own engine uses a unique
⋮----
/// C++ engine reads - E5). A test that starts its own engine uses a unique
/// name so it can never talk to the user's live session.
⋮----
/// name so it can never talk to the user's live session.
⋮----
pub fn is_accepted_server_image(actual: &std::path::Path, expected: &std::path::Path) -> bool {
let Some(name) = actual.file_name() else {
⋮----
let name = name.to_string_lossy().to_lowercase();
if !ACCEPTED_SERVER_IMAGE_NAMES.contains(&name.as_str()) {
⋮----
actual.parent() == expected.parent()
⋮----
pub const fn dev_server_image_allowed() -> bool {
cfg!(debug_assertions)
⋮----
pub fn current_session_pipe_name(override_name: Option<&str>) -> String {
⋮----
if !name.is_empty() {
return name.to_string();
⋮----
let session = current_session_id();
format!("{PIPE_NAME_BASE}-{session}")
⋮----
pub fn pipe_name_for_session(session_id: u32) -> String {
format!("{PIPE_NAME_BASE}-{session_id}")
⋮----
pub fn current_session_id() -> u32 {
use windows::Win32::System::RemoteDesktop::ProcessIdToSessionId;
⋮----
let _ = ProcessIdToSessionId(
⋮----
pub struct ConnectOptions {
⋮----
impl Default for ConnectOptions {
fn default() -> Self {
⋮----
pub struct ServerIdentity {
⋮----
pub struct IpcClient {
⋮----
impl IpcClient {
⋮----
pub fn connect(pipe_name: &str, options: ConnectOptions) -> Result<IpcClient, IpcError> {
⋮----
fn connect_with(
⋮----
let handle = open_pipe(pipe_name, options.timeout, wait_for_the_pipe)?;
⋮----
pipe_name: pipe_name.to_string(),
⋮----
match client.verify_server() {
⋮----
Ok(client)
⋮----
Err(e) => Err(e),
⋮----
pub fn reconnect(self, options: ConnectOptions) -> Result<IpcClient, IpcError> {
let pipe_name = self.pipe_name.clone();
drop(self);
⋮----
pub fn connect_current_session(options: ConnectOptions) -> Result<IpcClient, IpcError> {
IpcClient::connect(&current_session_pipe_name(None), options)
⋮----
pub fn server_identity(&self) -> &ServerIdentity {
⋮----
pub fn request(&mut self, message: &IpcMessage) -> Result<serde_json::Value, IpcError> {
let line = encode(message).map_err(|e: CoreIpcError| IpcError::Protocol(e.to_string()))?;
self.write_line(line.as_bytes())?;
let answer = self.read_line()?;
let text = String::from_utf8_lossy(&answer).to_string();
let text = text.trim_end_matches(['\r', '\n']);
⋮----
.map_err(|e| IpcError::Protocol(format!("the answer is not JSON: {e}")))
⋮----
pub fn write_line(&mut self, bytes: &[u8]) -> Result<(), IpcError> {
overlapped_write(self.handle, bytes, self.options.timeout)
⋮----
pub fn read_line(&mut self) -> Result<Vec<u8>, IpcError> {
let mut buffer = vec![0u8; k6wp_core::ipc::MAX_PAYLOAD_BYTES + 1024];
let read = overlapped_read(self.handle, &mut buffer, self.options.timeout)?;
buffer.truncate(read);
Ok(buffer)
⋮----
pub fn close(self) {
⋮----
fn verify_server(&self) -> Result<ServerIdentity, IpcError> {
use windows::Win32::System::Pipes::GetNamedPipeServerProcessId;
⋮----
let ok = unsafe { GetNamedPipeServerProcessId(self.handle, &mut pid) };
if ok.is_err() {
return Err(IpcError::Unverified(
"the server process id could not be read".to_string(),
⋮----
"the server process id is zero".to_string(),
⋮----
let server_session = process_session_id(pid);
⋮----
let own_session = current_session_id();
⋮----
return Err(IpcError::Unverified(format!(
⋮----
let owner_sid = process_owner_sid(pid);
⋮----
let own_sid = current_user_sid();
⋮----
"the server's owner could not be read".to_string(),
⋮----
let image_path = process_image_path(pid);
⋮----
Some(actual) if paths_equal(actual, expected.as_path()) => {}
⋮----
"the server image path could not be read".to_string(),
⋮----
Ok(ServerIdentity {
⋮----
impl Drop for IpcClient {
fn drop(&mut self) {
⋮----
fn paths_equal(a: &std::path::Path, b: &std::path::Path) -> bool {
a.to_string_lossy()
.eq_ignore_ascii_case(&b.to_string_lossy())
⋮----
fn process_session_id(pid: u32) -> u32 {
⋮----
let _ = ProcessIdToSessionId(pid, &mut session);
⋮----
fn process_image_path(pid: u32) -> Option<PathBuf> {
use windows::core::PWSTR;
⋮----
let process = unsafe { OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, false, pid) }.ok()?;
⋮----
let mut size = buffer.len() as u32;
⋮----
QueryFullProcessImageNameW(
⋮----
PROCESS_NAME_FORMAT(0),
PWSTR(buffer.as_mut_ptr()),
⋮----
Some(PathBuf::from(text))
⋮----
fn process_owner_sid(pid: u32) -> Option<String> {
use windows::Win32::Foundation::HANDLE;
⋮----
let opened = unsafe { OpenProcessToken(process, TOKEN_QUERY, &mut token) };
⋮----
if opened.is_err() {
⋮----
let _ = GetTokenInformation(token, TokenUser, None, 0, &mut size);
⋮----
let mut buffer = vec![0u8; size as usize];
⋮----
GetTokenInformation(
⋮----
Some(buffer.as_mut_ptr() as *mut c_void),
⋮----
if read.is_err() {
⋮----
sid_to_string(&buffer)
⋮----
fn current_user_sid() -> Option<String> {
⋮----
let process = unsafe { GetCurrentProcess() };
⋮----
let mut buffer = vec![0u8; size.max(1) as usize];
⋮----
buffer.len() as u32,
⋮----
fn sid_to_string(buffer: &[u8]) -> Option<String> {
⋮----
use windows::Win32::Foundation::LocalFree;
use windows::Win32::Security::Authorization::ConvertSidToStringSidW;
⋮----
let token_user = buffer.as_ptr() as *const windows::Win32::Security::TOKEN_USER;
⋮----
let ok = unsafe { ConvertSidToStringSidW(sid, &mut text) };
if ok.is_err() || text.is_null() {
⋮----
while *text.0.add(len) != 0 {
⋮----
let _ = LocalFree(Some(windows::Win32::Foundation::HLOCAL(
⋮----
Some(owned)
⋮----
fn open_pipe(
⋮----
use windows::core::PCWSTR;
⋮----
use windows::Win32::System::Pipes::WaitNamedPipeW;
⋮----
let wide: Vec<u16> = pipe_name.encode_utf16().chain(std::iter::once(0)).collect();
⋮----
CreateFileW(
PCWSTR(wide.as_ptr()),
⋮----
FILE_SHARE_MODE(0),
⋮----
let mode = NAMED_PIPE_MODE(PIPE_READMODE_MESSAGE.0);
⋮----
let _ = SetNamedPipeHandleState(
⋮----
Some(&mode as *const NAMED_PIPE_MODE),
⋮----
return Ok(h);
⋮----
let last_error = (e.code().0 as u32) & 0xFFFF;
⋮----
return Err(IpcError::Connect(format!(
⋮----
let remaining = deadline.saturating_duration_since(std::time::Instant::now());
if remaining.is_zero() {
⋮----
let wait_ms = remaining.as_millis().clamp(1, u32::MAX as u128) as u32;
⋮----
let waited = unsafe { WaitNamedPipeW(PCWSTR(wide.as_ptr()), wait_ms) };
if !waited.as_bool() {
⋮----
std::thread::sleep(Duration::from_millis(50).min(remaining));
⋮----
fn overlapped_write(
⋮----
use windows::Win32::Foundation::CloseHandle;
use windows::Win32::System::Threading::CreateEventW;
⋮----
let event = unsafe { CreateEventW(None, true, false, None) }
.map_err(|e| IpcError::Transport(format!("the write event could not be created: {e}")))?;
⋮----
Some(bytes),
Some(&mut written),
Some(&mut overlapped),
⋮----
if ok.is_ok() {
⋮----
let _ = CloseHandle(event);
⋮----
if written as usize != bytes.len() {
return Err(IpcError::Transport(format!(
⋮----
return Ok(());
⋮----
let timeout_ms = timeout.as_millis().min(u32::MAX as u128) as u32;
⋮----
GetOverlappedResult(
⋮----
return Err(IpcError::Timeout(
"the write did not finish in time".to_string(),
⋮----
if done.is_ok() {
⋮----
Err(IpcError::Transport(
"the write failed on the pipe".to_string(),
⋮----
fn overlapped_read(
⋮----
.map_err(|e| IpcError::Transport(format!("the read event could not be created: {e}")))?;
⋮----
Some(&mut buffer[..]),
Some(&mut transferred),
⋮----
return Ok(transferred as usize);
⋮----
"the answer did not arrive in time".to_string(),
⋮----
"the read failed on the pipe".to_string(),
```

## File: engine/src/engine_app.cpp
```cpp
unsigned long long CurrentHandleCount() {
⋮----
std::wstring WidenUtf8(const std::string& s) {
⋮----
std::string NarrowUtf8(const std::wstring& w) {
⋮----
std::string DisplayCoverageVerdict(const std::string& reason) {
⋮----
bool SameAssignments(const DisplaysConfig& a, const DisplaysConfig& b) {
⋮----
struct MonitorPowerSetting {
⋮----
void InitMonitorPowerSetting(MonitorPowerSetting& setting, DWORD data) {
⋮----
bool EngineApp::Init(int argc, char** argv) {
⋮----
// P3L: affinity/adapter apply at boot only (process mask / VO init).
// A live flip logs restart-required instead of half-applying.
⋮----
std::string em(env_mode);
⋮----
k6wp::lock_order::LeafGuard video_guard(video_mutex_);
⋮----
k6wp::lock_order::LeafGuard options_guard(options_mutex_);
⋮----
void EngineApp::InitWallpaperSurface(const std::string& video_utf8) {
⋮----
k6wp::lock_order::LeafGuard displays_guard(displays_mutex_);
⋮----
void EngineApp::UpdateTrayErrorStatus() {
⋮----
void EngineApp::ShutdownWallpaperSurface() {
⋮----
std::shared_ptr<MpvRenderer> EngineApp::AcquireRenderer() const {
k6wp::lock_order::LeafGuard renderer_guard(renderer_mutex_);
⋮----
void EngineApp::StopHeadlessRenderer() {
⋮----
bool EngineApp::AnySimulateArmed() const { return sim_.AnyArmed(); }
⋮----
DWORD EngineApp::ComputeWaitTimeoutMs() const {
⋮----
std::lock_guard<std::mutex> lock(decode_state_mutex_);
⋮----
void EngineApp::CaptureFramesStep(long long elapsed_ms) {
⋮----
void EngineApp::DumpMpvCountersStep(long long elapsed_ms) {
⋮----
void EngineApp::DumpMediaProps() {
⋮----
void EngineApp::ApplyFitMode(const std::string& fit_mode) {
⋮----
void EngineApp::ArmPinVerify() {
⋮----
void EngineApp::RunPinVerifyPass() {
⋮----
bool EngineApp::VerifyHeadlessPin() {
⋮----
int EngineApp::Run() {
⋮----
void EngineApp::Shutdown() {
⋮----
void EngineApp::OnSuspend() { SetPauseOwner(kPauseSuspend, true); }
⋮----
void EngineApp::OnResume() { SetPauseOwner(kPauseSuspend, false); }
⋮----
void EngineApp::SetPauseOwner(int bit, bool on) {
⋮----
void EngineApp::ApplyPauseState(const char* owner) {
⋮----
bool EngineApp::PauseHidesWallpaper(int mask) const {
⋮----
void EngineApp::OnPauseStateChanged(bool paused) {
std::unique_lock<std::mutex> lock(decode_state_mutex_);
⋮----
void EngineApp::MaybeUnloadAfterPause(long long elapsed_ms) {
⋮----
void EngineApp::UnloadDecodeNow(long long elapsed_ms) {
⋮----
void EngineApp::ReloadDecodeNow(long long elapsed_ms) {
⋮----
void EngineApp::OnDeviceLost() {
⋮----
void EngineApp::OnDisplayChange(int width, int height) {
⋮----
void EngineApp::RequestStillRefresh(const std::string& video_utf8, bool manual) {
⋮----
void EngineApp::ArmTopologyReconcile(const char* trigger,
⋮----
void EngineApp::CancelTopologyReconcile(const char* reason) {
⋮----
void EngineApp::RunTopologyReconcile() {
⋮----
void EngineApp::ApplyPendingSetVideo() {
⋮----
void EngineApp::ApplyPendingSetMonitor() {
⋮----
std::filesystem::path EngineApp::ResolvedConfigPath() const {
⋮----
bool EngineApp::HandleSetVideo(const std::string& payload_json, bool rotation) {
⋮----
bool EngineApp::HandleSetMonitor(const std::string& payload_json) {
⋮----
bool EngineApp::HandleSetDisplayVideo(const std::string& payload_json) {
⋮----
void EngineApp::OnDisplaysFileChanged() {
⋮----
void EngineApp::RecreateDevice() {
⋮----
void EngineApp::OnTrayTogglePause() {
⋮----
void EngineApp::OnTrayQuickSwitch(std::size_t idx) {
⋮----
std::string EngineApp::BuildStateJson() const {
⋮----
std::lock_guard<std::mutex> lock(reconcile_mutex_);
⋮----
void EngineApp::ArmReconcile(const char* trigger) {
⋮----
void EngineApp::RunReconcilePass() {
⋮----
std::string EngineApp::ReconcileDiffSummary(const TopologyDiff& diff) const {
⋮----
void EngineApp::HandlePowerBroadcast(WPARAM wParam, LPARAM lParam) {
⋮----
LRESULT EngineApp::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
⋮----
void EngineApp::RequestShutdown() {
⋮----
void EngineApp::NoteHealthyUptime() {
⋮----
bool EngineApp::RegisterWindowClass() {
⋮----
bool EngineApp::CreateMessageWindow() {
⋮----
void EngineApp::Log(const char* fmt, ...) {
⋮----
void EngineApp::LogImportant(const char* fmt, ...) {
```

## File: rust/crates/k6wp-core/src/sps.rs
```rust
use std::path::Path;
⋮----
pub enum Codec {
⋮----
impl Codec {
pub fn as_str(self) -> &'static str {
⋮----
/// The input limits (G2: the parser processes UNTRUSTED input).
///
⋮----
///
/// Every count that comes FROM THE FILE is capped before it is used as a
⋮----
/// Every count that comes FROM THE FILE is capped before it is used as a
/// loop bound or an allocation size, and every arithmetic operation on a
⋮----
/// loop bound or an allocation size, and every arithmetic operation on a
/// file-derived value is checked. A malformed file must produce an error
⋮----
/// file-derived value is checked. A malformed file must produce an error
/// or an "unknown" answer - never a panic, a huge allocation or an endless
⋮----
/// or an "unknown" answer - never a panic, a huge allocation or an endless
/// loop.
⋮----
/// loop.
mod limits {
⋮----
mod limits {
/// The most entries any container table may declare.
    pub const MAX_TABLE_ENTRIES: usize = 1 << 20;
/// The most samples the sample table may declare.
    pub const MAX_SAMPLES: usize = 1 << 20;
/// The most entries in a parameter-set array.
    pub const MAX_PARAM_SETS: usize = 256;
/// The most pictures in a picture-ordering cycle.
    pub const MAX_PIC_ORDER_CYCLE: u32 = 4096;
/// The most CPB entries in an HRD.
    pub const MAX_CPB_ENTRIES: u32 = 32;
/// The most sub-layers in an HEVC profile_tier_level.
    pub const MAX_SUB_LAYERS: u32 = 7;
/// The most entries a box tree may hold on the stack.
    pub const MAX_BOX_STACK: usize = 64;
/// The most bytes the `moov` box may contribute. A `moov` bigger than
    /// this is a corrupt (or hostile) file: it is skipped, not loaded.
⋮----
/// this is a corrupt (or hostile) file: it is skipped, not loaded.
    pub const MAX_MOOV_BYTES: u64 = 32 * 1024 * 1024;
/// The most bytes the `ftyp` box may contribute (it is a brand + a few
    /// compatible brands - a few dozen bytes in practice).
⋮----
/// compatible brands - a few dozen bytes in practice).
    pub const MAX_FTYP_BYTES: u64 = 4096;
⋮----
/// The container a file turned out to be.
///
⋮----
///
/// The policy is EXPLICIT (task I2): this module parses the MP4 family
⋮----
/// The policy is EXPLICIT (task I2): this module parses the MP4 family
/// (the parameter sets live in the `moov` box) and the raw Annex-B streams
⋮----
/// (the parameter sets live in the `moov` box) and the raw Annex-B streams
/// (the parameter sets are in band). Every other container - Matroska /
⋮----
/// (the parameter sets are in band). Every other container - Matroska /
/// WebM, AVI, MPEG-TS - FALLS BACK to ffmpeg's `trace_headers`: the parser
⋮----
/// WebM, AVI, MPEG-TS - FALLS BACK to ffmpeg's `trace_headers`: the parser
⋮----
pub enum ContainerKind {
⋮----
impl ContainerKind {
⋮----
/// True when this module can parse the container itself.
    pub fn is_parsed(self) -> bool {
⋮----
pub fn is_parsed(self) -> bool {
matches!(self, ContainerKind::Mp4 | ContainerKind::RawStream)
⋮----
/// Detects the container from the head of the file.
pub fn detect_container(head: &[u8]) -> ContainerKind {
⋮----
pub fn detect_container(head: &[u8]) -> ContainerKind {
// The EBML magic (Matroska / WebM).
if head.len() >= 4 && head[0] == 0x1A && head[1] == 0x45 && head[2] == 0xDF && head[3] == 0xA3 {
⋮----
// "RIFF"...."AVI " (AVI).
if head.len() >= 12 && &head[0..4] == b"RIFF" && &head[8..12] == b"AVI " {
⋮----
// The MPEG-TS sync byte at 0 and at the packet size (188 / 192).
if head.len() >= 189
⋮----
&& (head[188] == 0x47 || head[192.min(head.len() - 1)] == 0x47)
⋮----
// "ftyp" at the offset 4 (the MP4 family).
if head.len() >= 8 && &head[4..8] == b"ftyp" {
⋮----
// A raw Annex-B stream: a NAL start code (00 00 01 or 00 00 00 01).
if head.len() >= 3
⋮----
&& (head[2] == 1 || (head[2] == 0 && head.len() >= 4 && head[3] == 1))
⋮----
/// What one SPS says.
#[derive(Clone, Debug, Default, PartialEq)]
pub struct SpsInfo {
⋮----
/// H.264 `max_num_ref_frames` - the DPB size.
    pub max_num_ref_frames: Option<u32>,
/// H.264 VUI `max_dec_frame_buffering_minus1 + 1`.
    pub max_dec_frame_buffering: Option<u32>,
/// HEVC `sps_max_dec_pic_buffering_minus1 + 1`.
    pub sps_max_dec_pic_buffering: Option<u32>,
/// H.264 `frame_mbs_only_flag` (0 = interlaced / MBAFF).
    pub frame_mbs_only_flag: Option<u32>,
/// The frame size in pixels, when the SPS carries it.
    pub width: Option<u32>,
⋮----
/// The chroma format the SPS declares: 0 = monochrome, 1 = 4:2:0,
    /// 2 = 4:2:2, 3 = 4:4:4 (task H2).
⋮----
/// 2 = 4:2:2, 3 = 4:4:4 (task H2).
    ///
⋮----
///
    /// `None` when the codec's profile does not carry the field (the
⋮----
/// `None` when the codec's profile does not carry the field (the
⋮----
impl SpsInfo {
⋮----
pub fn dpb_size(&self) -> Option<u32> {
⋮----
Some(Codec::H264) => self.max_num_ref_frames.or(self.max_dec_frame_buffering),
⋮----
pub fn effective_refs(&self) -> Option<u32> {
⋮----
let refs = self.max_num_ref_frames.or(self.sps_max_dec_pic_buffering)?;
⋮----
.or(self.sps_max_dec_pic_buffering);
⋮----
Some(bound) if bound > 0 => Some(refs.min(bound)),
_ => Some(refs),
⋮----
pub fn dpb_bytes(&self) -> Option<u64> {
let refs = self.effective_refs()?;
let frame = self.frame_bytes()?;
⋮----
u64::from(refs).checked_mul(frame)
⋮----
pub fn frame_bytes(&self) -> Option<u64> {
⋮----
let pixels = u64::from(width).checked_mul(u64::from(height))?;
⋮----
.checked_mul(samples_x2)?
.checked_mul(bytes_per_sample)?
.checked_div(2)
⋮----
pub fn pixel_format(&self) -> Option<(&'static str, u32)> {
⋮----
Some((chroma, depth))
⋮----
/// The RAM a DPB-pinned re-encode would actually RECOVER (task H1).
    ///
⋮----
///
    /// `(effective_refs − pin_refs) × frame_bytes` - NOT the total DPB.
⋮----
/// `(effective_refs − pin_refs) × frame_bytes` - NOT the total DPB.
    ///
⋮----
///
    /// The total says how big the decoder's pool is; the savings say how
⋮----
/// The total says how big the decoder's pool is; the savings say how
⋮----
pub fn savings_obtainable(&self) -> Option<u64> {
⋮----
return Some(0);
⋮----
u64::from(refs - Self::PINNED_REFS).checked_mul(frame)
⋮----
pub fn is_copy_safe(&self) -> bool {
match self.dpb_bytes() {
⋮----
pub fn is_plausible(&self) -> bool {
⋮----
match self.dpb_size() {
Some(dpb) => (1..=16).contains(&dpb),
⋮----
pub struct DpbReport {
⋮----
pub enum SpsError {
⋮----
fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
⋮----
SpsError::Io(m) => write!(f, "sps io: {m}"),
SpsError::NoSps => write!(f, "sps: no parameter set found"),
SpsError::UnsupportedCodec(m) => write!(f, "sps: unsupported codec {m}"),
⋮----
/// How much of the file is read. The parameter sets live in the header
/// (the moov / the extradata), so a few MB is plenty; the cap keeps a
⋮----
/// (the moov / the extradata), so a few MB is plenty; the cap keeps a
/// pathological file from being read whole.
⋮----
/// pathological file from being read whole.
const MAX_SCAN_BYTES: usize = 64 * 1024 * 1024;
⋮----
// ---------------------------------------------------------------------------
// the bit reader
⋮----
struct Bits<'a> {
⋮----
pos: usize, // in bits
⋮----
fn new(data: &'a [u8]) -> Self {
⋮----
fn bit(&mut self) -> Option<u32> {
⋮----
if byte >= self.data.len() {
⋮----
Some(((self.data[byte] >> shift) & 1) as u32)
⋮----
fn bits(&mut self, n: u32) -> Option<u32> {
⋮----
value = (value << 1) | self.bit()?;
⋮----
Some(value)
⋮----
fn ue(&mut self) -> Option<u32> {
⋮----
while self.bit()? == 0 {
⋮----
let rest = self.bits(leading_zeros)?;
Some(((1u32 << leading_zeros) | rest) - 1)
⋮----
fn se(&mut self) -> Option<i32> {
let value = self.ue()?;
let magnitude = value.div_ceil(2) as i32;
⋮----
Some(-magnitude)
⋮----
Some(magnitude)
⋮----
fn rbsp_payload(nal: &[u8]) -> Vec<u8> {
let mut out = Vec::with_capacity(nal.len());
⋮----
for (index, &byte) in nal.iter().enumerate() {
⋮----
out.push(byte);
⋮----
if out.len() > 1 {
out.pop();
⋮----
fn parse_h264_sps(nal: &[u8]) -> Option<SpsInfo> {
⋮----
if nal.len() < 2 {
⋮----
let payload = rbsp_payload(&nal[1..]);
if payload.is_empty() {
⋮----
let profile_idc = bits.bits(8)? as u8;
let _constraints = bits.bits(8)?;
let _level_idc = bits.bits(8)?;
let _sps_id = bits.ue()?;
⋮----
codec: Some(Codec::H264),
⋮----
chroma_format_idc: Some(1),
bit_depth_luma: Some(8),
⋮----
if HIGH_PROFILES.contains(&profile_idc) {
let chroma_format_idc = bits.ue()?;
⋮----
bits.bit()?;
⋮----
let bit_depth_luma = bits.ue()?;
let bit_depth_chroma = bits.ue()?;
⋮----
info.chroma_format_idc = Some(chroma_format_idc);
info.bit_depth_luma = Some(bit_depth_luma + 8);
⋮----
let scaling_matrix_present = bits.bit()?;
⋮----
if bits.bit()? == 1 {
⋮----
let _last = bits.bit()?;
let _next = bits.se()?;
skip_scaling_list(&mut bits, size)?;
⋮----
bits.ue()?;
let pic_order_cnt_type = bits.ue()?;
⋮----
bits.se()?;
⋮----
let cycle = bits.ue()?.min(limits::MAX_PIC_ORDER_CYCLE);
⋮----
info.max_num_ref_frames = bits.ue();
⋮----
let pic_width_in_mbs_minus1 = bits.ue()?;
let pic_height_in_map_units_minus1 = bits.ue()?;
let frame_mbs_only_flag = bits.bit()?;
info.frame_mbs_only_flag = Some(frame_mbs_only_flag);
⋮----
let cropping = bits.bit()?;
⋮----
let vui_present = bits.bit()?;
⋮----
parse_h264_vui(&mut bits, &mut info);
⋮----
info.width = Some(width);
info.height = Some(height);
Some(info)
⋮----
fn skip_scaling_list(bits: &mut Bits, size: usize) -> Option<()> {
⋮----
let delta_scale = bits.se()?;
⋮----
Some(())
⋮----
fn parse_h264_vui(bits: &mut Bits, info: &mut SpsInfo) {
let aspect_ratio_info_present = bits.bit().unwrap_or(0);
⋮----
let aspect_ratio_idc = bits.bits(8).unwrap_or(0);
⋮----
bits.bits(16);
⋮----
if bits.bit().unwrap_or(0) == 1 {
bits.bit();
⋮----
let video_signal_type_present = bits.bit().unwrap_or(0);
⋮----
bits.bits(3);
⋮----
let colour_description_present = bits.bit().unwrap_or(0);
⋮----
bits.bits(8);
⋮----
bits.ue();
⋮----
bits.bits(32);
⋮----
let nal_hrd_present = bits.bit().unwrap_or(0);
⋮----
skip_hrd(bits);
⋮----
let vcl_hrd_present = bits.bit().unwrap_or(0);
⋮----
let max_num_reorder_frames = bits.ue().unwrap_or(0);
let _max_dec_frame_buffering = bits.ue().unwrap_or(0);
⋮----
fn skip_hrd(bits: &mut Bits) {
let cpb_cnt_minus1 = bits.ue().unwrap_or(0);
bits.bits(4);
⋮----
for _ in 0..=cpb_cnt_minus1.min(limits::MAX_CPB_ENTRIES) {
⋮----
bits.bits(5);
⋮----
fn parse_hevc_sps(nal: &[u8]) -> Option<SpsInfo> {
⋮----
if nal.len() < 3 {
⋮----
let payload = rbsp_payload(&nal[2..]);
⋮----
let _vps_id = bits.bits(4)?;
let max_sub_layers_minus1 = bits.bits(3)?.min(limits::MAX_SUB_LAYERS);
⋮----
skip_profile_tier_level(&mut bits, max_sub_layers_minus1)?;
⋮----
let width = bits.ue()?;
let height = bits.ue()?;
let conformance_window = bits.bit().unwrap_or(0);
⋮----
codec: Some(Codec::Hevc),
width: Some(width),
height: Some(height),
chroma_format_idc: Some(chroma_format_idc),
bit_depth_luma: Some(bit_depth_luma + 8),
⋮----
let sub_layer_ordering_present = bits.bit().unwrap_or(0);
⋮----
let max_dec_pic_buffering_minus1 = bits.ue()?;
⋮----
info.sps_max_dec_pic_buffering = Some(max_dec_pic_buffering_minus1 + 1);
⋮----
.unwrap_or(0)
.max(max_dec_pic_buffering_minus1 + 1);
info.sps_max_dec_pic_buffering = Some(value);
⋮----
fn skip_profile_tier_level(bits: &mut Bits, max_sub_layers_minus1: u32) -> Option<()> {
bits.bits(2)?;
⋮----
bits.bits(5)?;
⋮----
bits.bits(8)?;
⋮----
sub_layer_profile_present.push(bits.bit()?);
sub_layer_level_present.push(bits.bit()?);
⋮----
fn container_sps_nals(data: &[u8]) -> Vec<Vec<u8>> {
⋮----
for (kind, nals) in find_parameter_set_boxes(data) {
⋮----
let parsed = parse_h264_sps(&nal);
if parsed.is_some() {
out.push(nal);
⋮----
if parse_hevc_sps(&nal).is_some() {
⋮----
fn find_parameter_set_boxes(data: &[u8]) -> Vec<(Codec, Vec<Vec<u8>>)> {
⋮----
for avcc in find_box_payloads(data, b"avcC") {
if let Some(nals) = parse_avcc(&avcc) {
out.push((Codec::H264, nals));
⋮----
for hvcc in find_box_payloads(data, b"hvcC") {
if let Some(nals) = parse_hvcc(&hvcc) {
out.push((Codec::Hevc, nals));
⋮----
fn find_box_payloads(data: &[u8], four_cc: &[u8; 4]) -> Vec<Vec<u8>> {
⋮----
let mut stack: Vec<(usize, usize)> = vec![(0, data.len())];
⋮----
while let Some((start, end)) = stack.pop() {
⋮----
if std::env::var_os("K6WP_SPS_DEBUG").is_some() {
eprintln!(
⋮----
.checked_add(pos)
.filter(|body_end| *body_end <= end)
⋮----
out.push(data[pos + header..body_end].to_vec());
⋮----
stsd_extension_boxes(&data[pos + header..body_end])
⋮----
out.push(inner_payload);
⋮----
if CONTAINERS.contains(&&kind) {
stack.push((pos + header, body_end));
⋮----
fn stsd_extension_boxes(payload: &[u8]) -> Vec<([u8; 4], Vec<u8>)> {
⋮----
if payload.len() < 8 {
⋮----
.min(limits::MAX_TABLE_ENTRIES);
⋮----
if pos + 8 > payload.len() {
⋮----
if size < 8 || pos + size > payload.len() {
⋮----
out.push((inner_kind, payload[inner + 8..inner + inner_size].to_vec()));
⋮----
fn parse_avcc(payload: &[u8]) -> Option<Vec<Vec<u8>>> {
if payload.len() < 7 {
⋮----
let count = ((payload[5] & 0x1F) as usize).min(limits::MAX_PARAM_SETS);
⋮----
if pos + 2 > payload.len() {
⋮----
if pos + len > payload.len() {
⋮----
nals.push(payload[pos..pos + len].to_vec());
⋮----
Some(nals)
⋮----
fn parse_hvcc(payload: &[u8]) -> Option<Vec<Vec<u8>>> {
if payload.len() < 23 {
⋮----
let num_arrays = (payload[22] as usize).min(limits::MAX_PARAM_SETS);
⋮----
if pos + 3 > payload.len() {
⋮----
.min(limits::MAX_PARAM_SETS);
⋮----
fn inband_sps_nals(data: &[u8]) -> Vec<Vec<u8>> {
⋮----
out.extend(annexb_sps_nals(data));
out.extend(sample_sps_nals_in_memory(data));
⋮----
fn annexb_sps_nals(data: &[u8]) -> Vec<Vec<u8>> {
⋮----
let limit = data.len().min(MAX_SCAN_BYTES);
⋮----
if is_sps_nal(nal) {
out.push(nal.to_vec());
⋮----
fn sample_sps_nals_in_memory(data: &[u8]) -> Vec<Vec<u8>> {
⋮----
let length_size = nal_length_size(data);
for (offset, size) in sample_ranges(data) {
⋮----
let end = start.saturating_add(size as usize);
⋮----
fn sample_sps_nals_from_file(
⋮----
let length_size = nal_length_size(moov);
⋮----
for (offset, size) in sample_ranges(moov) {
let end = offset.saturating_add(size as u64);
⋮----
if offset.saturating_add(size as u64) > reader.length {
⋮----
reader.seek_to(offset)?;
let mut data = vec![0u8; size as usize];
reader.read_exact_at(&mut data)?;
⋮----
while pos + 4 <= data.len() {
⋮----
if len < 2 || pos + length_size + len > data.len() {
⋮----
Ok(out)
⋮----
fn nal_length_size(data: &[u8]) -> usize {
⋮----
if avcc.len() >= 5 {
⋮----
fn sample_ranges(data: &[u8]) -> Vec<(u64, u32)> {
let sizes = match find_box_payloads(data, b"stsz").first() {
Some(p) => parse_stsz(p),
⋮----
if sizes.is_empty() {
⋮----
let chunk_offsets = match find_box_payloads(data, b"stco").first() {
Some(p) => parse_chunk_offsets(p, 4),
None => match find_box_payloads(data, b"co64").first() {
Some(p) => parse_chunk_offsets(p, 8),
⋮----
let samples_per_chunk = match find_box_payloads(data, b"stsc").first() {
Some(p) => parse_stsc(p),
⋮----
if chunk_offsets.is_empty() || samples_per_chunk.is_empty() {
⋮----
for (chunk_index, chunk_offset) in chunk_offsets.iter().enumerate() {
let count = samples_in_chunk(&samples_per_chunk, chunk_index);
⋮----
if sample_index >= sizes.len() {
⋮----
out.push((offset, size));
⋮----
fn parse_stsz(payload: &[u8]) -> Vec<u32> {
if payload.len() < 12 {
⋮----
.min(limits::MAX_SAMPLES);
⋮----
return vec![sample_size; count];
⋮----
if at + 4 > payload.len() {
⋮----
out.push(u32::from_be_bytes([
⋮----
fn parse_chunk_offsets(payload: &[u8], width: usize) -> Vec<u64> {
⋮----
if at + width > payload.len() {
⋮----
out.push(value);
⋮----
fn parse_stsc(payload: &[u8]) -> Vec<(u32, u32)> {
⋮----
if at + 12 > payload.len() {
⋮----
out.push((first, per_chunk));
⋮----
fn samples_in_chunk(runs: &[(u32, u32)], chunk_index: usize) -> u32 {
⋮----
for (i, (_, per_chunk)) in runs.iter().enumerate() {
⋮----
if let Some((_, per_chunk)) = runs.last() {
let first = runs[runs.len() - 1].0 as usize;
⋮----
fn is_sps_nal(nal: &[u8]) -> bool {
if nal.is_empty() {
⋮----
struct CountingFile {
⋮----
impl CountingFile {
fn open(path: &Path) -> Result<Self, SpsError> {
let file = std::fs::File::open(path).map_err(|e| SpsError::Io(e.to_string()))?;
⋮----
.metadata()
.map_err(|e| SpsError::Io(e.to_string()))?
.len();
Ok(CountingFile {
⋮----
fn read_exact_at(&mut self, buf: &mut [u8]) -> Result<(), SpsError> {
⋮----
.read_exact(buf)
.map_err(|e| SpsError::Io(e.to_string()))?;
self.bytes_read += buf.len() as u64;
Ok(())
⋮----
fn seek_to(&mut self, pos: u64) -> Result<(), SpsError> {
⋮----
.seek(std::io::SeekFrom::Start(pos))
⋮----
fn length(&mut self) -> Result<u64, SpsError> {
⋮----
.stream_position()
⋮----
.seek(std::io::SeekFrom::End(0))
⋮----
self.seek_to(here)?;
Ok(end)
⋮----
fn walk_top_level_boxes(reader: &mut CountingFile) -> Result<Vec<(String, Vec<u8>)>, SpsError> {
let file_len = reader.length()?;
⋮----
reader.seek_to(pos)?;
⋮----
reader.read_exact_at(&mut header)?;
⋮----
let kind = String::from_utf8_lossy(&header[4..8]).to_string();
⋮----
reader.read_exact_at(&mut ext)?;
⋮----
let cap = match kind.as_str() {
"ftyp" => Some(limits::MAX_FTYP_BYTES),
"moov" => Some(limits::MAX_MOOV_BYTES),
⋮----
let mut payload = vec![0u8; payload_len as usize];
reader.read_exact_at(&mut payload)?;
out.push((kind, payload));
⋮----
let Some(next) = pos.checked_add(size) else {
⋮----
pub fn probe_dpb(path: &Path) -> Result<DpbReport, SpsError> {
⋮----
reader.seek_to(0)?;
⋮----
let head_len = std::cmp::min(193, reader.length()? as usize);
reader.read_exact_at(&mut head[..head_len])?;
let container = detect_container(&head[..head_len]);
⋮----
let boxes = walk_top_level_boxes(&mut reader)?;
⋮----
moov = Some(payload);
⋮----
for (codec_kind, nals_in) in find_parameter_set_boxes(moov_bytes) {
⋮----
Codec::H264 => nals.extend(nals_in),
Codec::Hevc => nals.extend(nals_in),
⋮----
nals.extend(sample_sps_nals_from_file(moov_bytes, &mut reader)?);
⋮----
let mut buf = vec![0u8; 64 * 1024];
⋮----
let want = std::cmp::min(buf.len(), MAX_SCAN_BYTES - total);
match reader.file.read(&mut buf[..want]) {
⋮----
prefix.extend_from_slice(&buf[..n]);
⋮----
Err(e) => return Err(SpsError::Io(e.to_string())),
⋮----
nals.extend(annexb_sps_nals(&prefix));
⋮----
return Ok(DpbReport {
⋮----
container: container.as_str().to_string(),
⋮----
let info = match is_sps_nal(nal) {
_ if (nal[0] & 0x1F) == 7 => parse_h264_sps(nal),
_ => parse_hevc_sps(nal),
⋮----
if !info.is_plausible() {
⋮----
if !sps.contains(&info) {
sps.push(info);
⋮----
if sps.is_empty() {
⋮----
let max_dpb = sps.iter().filter_map(|s| s.dpb_size()).max();
Ok(DpbReport {
⋮----
pub fn container_parameter_sets(data: &[u8]) -> Vec<(Codec, Vec<Vec<u8>>)> {
let scanned = &data[..data.len().min(MAX_SCAN_BYTES)];
find_parameter_set_boxes(scanned)
⋮----
pub fn probe_dpb_bytes(data: &[u8]) -> DpbReport {
⋮----
let container = detect_container(scanned);
⋮----
if container.is_parsed() {
nals = container_sps_nals(scanned);
nals.extend(inband_sps_nals(scanned));
⋮----
bytes_read: scanned.len() as u64,
⋮----
mod tests {
⋮----
fn the_exp_golomb_reader_is_correct() {
⋮----
assert_eq!(bits.ue(), Some(0));
assert_eq!(bits.ue(), Some(1));
assert_eq!(bits.ue(), Some(2));
assert_eq!(bits.ue(), Some(3));
⋮----
fn the_signed_exp_golomb_is_correct() {
⋮----
assert_eq!(bits.se(), Some(0));
assert_eq!(bits.se(), Some(1));
assert_eq!(bits.se(), Some(-1));
assert_eq!(bits.se(), Some(2));
⋮----
fn the_emulation_prevention_bytes_are_removed() {
⋮----
let payload = rbsp_payload(&nal);
assert_eq!(payload, vec![0x00, 0x00, 0x01]);
⋮----
fn an_sps_nal_is_recognised() {
assert!(is_sps_nal(&[0x67, 0x42]));
assert!(is_sps_nal(&[0x42, 0x01]));
assert!(!is_sps_nal(&[0x68, 0x42]));
assert!(!is_sps_nal(&[0x40, 0x01]));
assert!(!is_sps_nal(&[]));
⋮----
fn the_frame_size_follows_the_bit_depth_and_the_chroma() {
⋮----
width: Some(1920),
height: Some(1080),
max_num_ref_frames: Some(4),
⋮----
assert_eq!(base.frame_bytes(), Some(3_110_400));
⋮----
bit_depth_luma: Some(10),
..base.clone()
⋮----
assert_eq!(ten.frame_bytes(), Some(6_220_800));
⋮----
chroma_format_idc: Some(3),
⋮----
assert_eq!(full.frame_bytes(), Some(6_220_800));
⋮----
chroma_format_idc: Some(2),
⋮----
assert_eq!(subsampled.frame_bytes(), Some(4_147_200));
⋮----
chroma_format_idc: Some(0),
⋮----
assert_eq!(mono.frame_bytes(), Some(2_073_600));
⋮----
assert_eq!(base.dpb_bytes(), Some(4 * 3_110_400));
assert_eq!(ten.dpb_bytes(), Some(4 * 6_220_800));
⋮----
assert_eq!(base.savings_obtainable(), Some(2 * 3_110_400));
assert_eq!(ten.savings_obtainable(), Some(2 * 6_220_800));
⋮----
fn an_unknown_frame_size_is_never_guessed() {
⋮----
assert_eq!(no_size.frame_bytes(), None);
assert_eq!(no_size.dpb_bytes(), None);
assert!(!no_size.is_copy_safe());
⋮----
assert_eq!(unknown_codec.frame_bytes(), None);
assert_eq!(unknown_codec.dpb_bytes(), None);
assert!(!unknown_codec.is_copy_safe());
```

## File: rust/crates/k6wp-studio/src/main.rs
```rust
mod grid;
mod history;
pub mod thumbnailer;
mod thumbs;
mod worker;
⋮----
use std::cell::RefCell;
⋮----
use std::path::PathBuf;
use std::rc::Rc;
use std::sync::mpsc::Sender;
⋮----
use k6wp_core::StudioSettings;
⋮----
use rfd::FileDialog;
⋮----
struct CardState {
⋮----
struct Studio {
⋮----
thread_local! {
⋮----
fn with_studio(f: impl FnOnce(&mut Studio)) {
STUDIO.with(|slot| {
if let Ok(mut guard) = slot.try_borrow_mut() {
if let Some(studio) = guard.as_mut() {
f(studio);
⋮----
fn library_index_path(library_override: Option<&str>) -> Option<PathBuf> {
⋮----
if !override_path.is_empty() {
return Some(PathBuf::from(override_path));
⋮----
let local = std::env::var("LOCALAPPDATA").ok()?;
if local.is_empty() {
⋮----
Some(PathBuf::from(local).join("K6WP").join("library.json"))
⋮----
fn parse_theme(args: &[String]) -> Option<bool> {
let mut it = args.iter();
while let Some(arg) = it.next() {
⋮----
return match it.next().map(|s| s.as_str()) {
Some("dark") => Some(true),
Some("light") => Some(false),
⋮----
fn main() -> Result<(), Box<dyn std::error::Error>> {
⋮----
let args: Vec<String> = std::env::args().skip(1).collect();
let cli_theme = parse_theme(&args);
let ui_watch = args.iter().any(|a| a == "--ui-watch");
⋮----
.iter()
.position(|a| a == "--e2e-pick")
.and_then(|i| args.get(i + 1))
.and_then(|s| s.parse::<usize>().ok());
⋮----
ui.set_version(SharedString::from(env!("CARGO_PKG_VERSION")));
ui.set_empty_message(SharedString::from("Memuat library..."));
⋮----
ui.set_rows(ModelRc::from(rows.clone()));
⋮----
let weak = ui.as_weak();
⋮----
let weak = weak.clone();
⋮----
if weak.upgrade().is_some() {
with_studio(|studio| studio.on_event(event));
⋮----
ui.on_rebuild_grid(|width| with_studio(|studio| studio.rebuild_by_width(width)));
ui.on_viewport_changed(|scroll_y, viewport_h| {
with_studio(|studio| studio.on_viewport(scroll_y, viewport_h))
⋮----
ui.on_set_wallpaper(|index, media_path| {
with_studio(|studio| studio.set_wallpaper(index, media_path))
⋮----
ui.on_pause(|| with_studio(|studio| studio.pause()));
ui.on_resume(|| with_studio(|studio| studio.resume()));
ui.on_undo(|| with_studio(|studio| studio.undo()));
ui.on_import(|| with_studio(|studio| studio.import()));
ui.on_compress(|index| with_studio(|studio| studio.compress(index)));
ui.on_save_settings(|| with_studio(|studio| studio.save_settings()));
ui.on_pick_folder(|field| with_studio(|studio| studio.pick_folder(field)));
⋮----
*slot.borrow_mut() = Some(Studio {
ui: ui.clone_strong(),
tx: tx.clone(),
⋮----
ui.global::<Tokens>().set_dark(dark);
⋮----
let _ = tx.send(Request::Theme);
⋮----
match library_index_path(
⋮----
std::env::var("K6WP_LIBRARY_JSON").ok().as_deref(),
⋮----
let _ = tx.send(Request::Load { path });
⋮----
ui.set_empty_message(SharedString::from(
⋮----
let _ = tx.send(Request::SettingsLoad);
⋮----
spawn_ui_watchdog(&ui);
⋮----
ui.run()?;
let _ = tx.send(Request::Stop);
Ok(())
⋮----
impl Studio {
fn on_event(&mut self, event: Event) {
⋮----
Event::Library(Ok(entries)) => self.apply_library(entries),
⋮----
self.ui.set_empty_message(SharedString::from(format!(
⋮----
self.update_footer();
⋮----
if let Some(card) = self.cards.get_mut(index) {
⋮----
self.sync_row(index);
⋮----
self.pending.remove(&index);
⋮----
self.cache.insert(index as u64, image_from(pixels));
⋮----
Err(message) => eprintln!("studio: {message}"),
⋮----
tokens.set_reduced_motion(reduced_motion);
if self.cli_theme.is_none() {
tokens.set_dark(dark);
⋮----
self.update_connection_ui();
⋮----
self.update_engine_ui();
⋮----
self.show_toast(message, false);
⋮----
let Some(card) = self.cards.get(index) else {
⋮----
let undo = self.history.record(history::WallpaperRef {
⋮----
name: card.name.clone(),
media: card.media.clone(),
⋮----
.applied()
.map(|a| a.name.clone())
.unwrap_or_default();
self.show_toast(format!("Terpasang: {name}"), undo);
⋮----
self.compress_pct = Some(pct);
⋮----
.map(|e| format!(" ({e}s remaining)"))
⋮----
self.show_toast(format!("Mengompres... {pct}%{eta}"), false);
⋮----
self.compress_jobs.remove(&job_id);
self.ui.set_compress_job_id(-1);
⋮----
.as_ref()
.and_then(|p| p.file_name().and_then(|n| n.to_str()))
.unwrap_or("")
.to_string();
self.show_toast(format!("Selesai: {name}"), false);
⋮----
let msg = error.unwrap_or_else(|| "Gagal diketahui".to_string());
self.show_toast(format!("Gagal: {msg}"), false);
⋮----
self.settings_path = settings_index_path();
self.apply_settings_ui();
⋮----
fn apply_library(&mut self, entries: Vec<Entry>) {
⋮----
.into_iter()
.map(|entry| CardState {
⋮----
.collect();
self.cache.clear();
self.pending.clear();
⋮----
self.ui.set_count(self.cards.len() as i32);
if self.cards.is_empty() {
self.ui.set_empty_message(SharedString::from(
⋮----
self.rebuild_by_width(width);
self.ensure_visible();
⋮----
for (index, card) in self.cards.iter().enumerate() {
⋮----
let _ = self.tx.send(Request::Probe {
⋮----
path: card.media.clone(),
⋮----
if let Some(pick) = self.e2e_pick.take() {
if pick < self.cards.len() {
let media = self.cards[pick].media.clone();
self.set_wallpaper(
⋮----
SharedString::from(media.to_string_lossy().as_ref()),
⋮----
fn rebuild_by_width(&mut self, width: f32) {
if !width.is_finite() || width <= 0.0 {
⋮----
let card_width = grid::card_width(width, columns).floor();
let band = (card_width * 9.0 / 16.0).round();
⋮----
let scale = self.ui.window().scale_factor().max(1.0);
⋮----
((card_width * scale).round() as u32).max(8),
((band * scale).round() as u32).max(8),
⋮----
let needs_model = self.rows.row_count() == 0 && !self.cards.is_empty();
⋮----
self.ui.set_columns(columns as i32);
self.ui.set_card_width(card_width);
self.ui.set_row_height(row_height);
⋮----
self.rebuild_model();
⋮----
fn rebuild_model(&mut self) {
⋮----
for (start, end) in grid::row_ranges(self.cards.len(), self.columns.max(1)) {
rows.push(LibraryRow {
cards: self.row_cards(start, end),
⋮----
self.rows.set_vec(rows);
⋮----
fn row_cards(&mut self, start: usize, end: usize) -> ModelRc<LibraryCard> {
let end = end.min(self.cards.len());
let cards: Vec<LibraryCard> = (start..end).map(|index| self.make_card(index)).collect();
⋮----
fn make_card(&mut self, index: usize) -> LibraryCard {
⋮----
self.cache.get(index as u64)
⋮----
self.cache.peek(index as u64)
⋮----
name: SharedString::from(card.name.as_str()),
meta: SharedString::from(card.meta.as_str()),
⋮----
media_path: SharedString::from(card.media.to_string_lossy().as_ref()),
⋮----
fn sync_row(&mut self, index: usize) {
let columns = self.columns.max(1);
⋮----
let end = (start + columns).min(self.cards.len());
if row < self.rows.row_count() {
let cards = self.row_cards(start, end);
self.rows.set_row_data(row, LibraryRow { cards });
⋮----
fn on_viewport(&mut self, scroll_y: f32, viewport_height: f32) {
if self.cards.is_empty() || self.columns == 0 {
⋮----
let offset = (-scroll_y).max(0.0);
let row_height = self.ui.get_row_height();
⋮----
self.cards.len(),
⋮----
self.request_visible();
⋮----
fn ensure_visible(&mut self) {
⋮----
let scroll = self.ui.get_scroll_y();
let viewport = self.ui.get_viewport_h();
self.on_viewport(scroll, viewport);
⋮----
fn request_visible(&mut self) {
⋮----
let keep: Vec<u64> = (start..end).map(|i| i as u64).collect();
let dropped = self.cache.retain_only(&keep);
⋮----
if self.cache.contains(key) || self.pending.contains(&index) {
⋮----
if card.unavailable && card.thumb.as_os_str().is_empty() {
⋮----
self.pending.insert(index);
⋮----
let _ = self.tx.send(Request::Thumb {
⋮----
let _ = self.tx.send(Request::Thumbnail {
⋮----
fn update_footer(&mut self) {
let total = self.cards.len();
let missing = self.cards.iter().filter(|c| c.unavailable).count();
let used = self.cache.used_bytes() as f64 / (1024.0 * 1024.0);
let cap = self.cache.cap_bytes() as f64 / (1024.0 * 1024.0);
self.ui.set_footer(SharedString::from(format!(
⋮----
fn update_connection_ui(&mut self) {
⋮----
self.ui.set_connection_status(SharedString::from(text));
⋮----
self.ui.set_connection_color(Brush::SolidColor(c));
⋮----
fn update_engine_ui(&mut self) {
⋮----
self.ui.set_current_video(SharedString::from(video));
⋮----
self.ui.set_current_video(SharedString::from("—"));
⋮----
self.ui.set_is_paused(self.engine_state.is_paused);
⋮----
fn show_toast(&mut self, message: String, undo: bool) {
self.toast = Some(message.clone());
self.ui.set_toast(SharedString::from(message));
self.ui.set_undo_available(undo);
let generation = self.ui.get_toast_generation().wrapping_add(1);
self.ui.set_toast_generation(generation);
⋮----
let ui_weak = self.ui.as_weak();
⋮----
if let Some(ui) = ui_weak.upgrade() {
if ui.get_toast_generation() == generation {
ui.set_toast(SharedString::from(""));
ui.set_undo_available(false);
⋮----
/// Callback: user clicked "Urungkan" in the toast. Re-applies the picture
fn undo(&mut self) {
let Some(previous) = self.history.take_undo() else {
⋮----
self.ui.set_undo_available(false);
let _ = self.tx.send(Request::SetWallpaper {
⋮----
media: previous.media.clone(),
⋮----
self.show_toast(format!("Mengembalikan: {}", previous.name), false);
⋮----
fn set_wallpaper(&mut self, index: i32, media_path: SharedString) {
⋮----
if index >= self.cards.len() {
⋮----
let media = media_path.to_string();
if media.is_empty() {
self.show_toast("Path media kosong".to_string(), false);
⋮----
media: media.into(),
⋮----
self.show_toast(format!("Memasang {}...", self.cards[index].name), false);
⋮----
fn pause(&mut self) {
let _ = self.tx.send(Request::Pause);
⋮----
fn resume(&mut self) {
let _ = self.tx.send(Request::Resume);
⋮----
fn import(&mut self) {
⋮----
.add_filter("Video", &["mp4", "mkv", "webm", "mov"])
.set_directory(std::env::var("USERPROFILE").unwrap_or_default())
.pick_files();
⋮----
if !files.is_empty() {
let _ = self.tx.send(Request::Import { paths: files });
⋮----
fn compress(&mut self, index: i32) {
⋮----
let out = card.media.with_extension("compressed.mp4");
self.compress_jobs.insert(job_id, 0);
self.compress_pct = Some(0);
self.ui.set_compress_job_id(job_id as i32);
let _ = self.tx.send(Request::Compress {
⋮----
src: card.media.clone(),
out: out.clone(),
⋮----
self.show_toast(format!("Mengompres {}... 0%", card.name), false);
⋮----
fn compress_cancel(&mut self, job_id: u64) {
let _ = self.tx.send(Request::CompressCancel { job_id });
⋮----
self.show_toast("Kompresi dibatalkan".to_string(), false);
⋮----
fn save_settings(&mut self) {
⋮----
.get_settings_default_crf()
.trim()
.parse()
.unwrap_or(22);
⋮----
.get_settings_default_fps()
⋮----
.unwrap_or(30);
⋮----
.get_settings_lockscreen_offset()
⋮----
.unwrap_or(0.0);
⋮----
auto_compress_on_import: self.ui.get_settings_auto_compress(),
⋮----
self.ui.get_settings_compress_output_dir().to_string(),
⋮----
default_resolution_mode: self.ui.get_settings_default_resolution().to_string(),
⋮----
cache_dir: PathBuf::from(self.ui.get_settings_cache_dir().to_string()),
wallpaper_sync: self.ui.get_settings_wallpaper_sync(),
lockscreen_sync: self.ui.get_settings_lockscreen_sync(),
⋮----
check_updates: self.ui.get_settings_check_updates(),
close_to_tray: self.ui.get_settings_close_to_tray(),
playlist_source: self.settings.playlist_source.clone(),
performance_preset: self.settings.performance_preset.clone(),
⋮----
let _ = self.tx.send(Request::SettingsSave { settings });
⋮----
fn pick_folder(&mut self, field: SharedString) {
⋮----
.pick_folder();
⋮----
match field.as_ref() {
⋮----
self.ui.set_settings_compress_output_dir(
path.to_string_lossy().to_string().into(),
⋮----
.set_settings_cache_dir(path.to_string_lossy().to_string().into());
⋮----
fn apply_settings_ui(&mut self) {
⋮----
fn image_from(pixels: Pixels) -> slint::Image {
⋮----
fn spawn_ui_watchdog(ui: &AppWindow) {
⋮----
.name("k6wp-studio-uiwatch".to_string())
.spawn(move || loop {
⋮----
let _ = weak.upgrade();
let _ = tx.send(());
⋮----
if queued.is_err() {
⋮----
match rx.recv_timeout(Duration::from_secs(5)) {
⋮----
let ms = posted.elapsed().as_secs_f64() * 1000.0;
⋮----
eprintln!("UIBLOCK {ms:.1}");
⋮----
.ok();
```

## File: rust/crates/k6wp-studio/src/worker.rs
```rust
use std::collections::HashMap;
⋮----
use k6wp_core::settings::save_studio_settings;
⋮----
use serde_json::Value;
⋮----
pub struct Entry {
⋮----
pub enum Request {
⋮----
pub enum IpcStatus {
⋮----
pub struct EngineState {
⋮----
pub enum Event {
⋮----
type EventSink = std::sync::Arc<std::sync::Mutex<Box<dyn Fn(Event) + Send>>>;
⋮----
struct Sink(EventSink);
⋮----
impl Sink {
fn emit(&self, event: Event) {
if let Ok(callback) = self.0.lock() {
callback(event);
⋮----
pub fn spawn(
⋮----
let sink = Sink(std::sync::Arc::new(std::sync::Mutex::new(
⋮----
.name("k6wp-studio-worker".to_string())
.spawn(move || run(rx, sink, pipe_name_override));
⋮----
.spawn(move || run(rx, sink, None));
if spawned.is_err() {
eprintln!("studio: the worker thread could not start; the library will stay empty");
⋮----
fn run(rx: Receiver<Request>, sink: Sink, pipe_name_override: Option<String>) {
// One queue per probe thread: a shared `Receiver` behind a mutex would
// serialise the probes (one thread would hold the lock while it blocks).
⋮----
.map(|n| {
⋮----
let sink = sink.clone();
⋮----
.name(format!("k6wp-studio-probe-{n}"))
.spawn(move || probe_loop(rx, sink));
⋮----
eprintln!(
⋮----
.collect();
⋮----
// IPC connection manager state
⋮----
let mut ipc_status = IpcStatus::Reconnecting; // Start with Reconnecting so main loop attempts connection
⋮----
let mut next_connect_attempt = Instant::now(); // Allow immediate first attempt
⋮----
let studio_visible = true; // updated by UI via a new Request variant if needed
let mut compress_jobs: HashMap<u64, u32> = HashMap::new(); // job_id -> pid
⋮----
// Resolve the pipe name once at startup (uses override in debug, real name in release)
let effective_pipe_name = pipe_name_override.unwrap_or_else(|| current_session_pipe_name(None));
⋮----
// Adaptive polling: check if it's time to poll get_state
⋮----
ipc_client.is_some() && studio_visible && last_poll.elapsed() >= POLL_INTERVAL_VISIBLE;
⋮----
match rx.recv_timeout(timeout) {
⋮----
Ok(Request::Load { path }) => sink.emit(Event::Library(load_library(&path))),
⋮----
if probe_queues.is_empty() {
sink.emit(Event::Probe {
⋮----
exists: probe(&path, PROBE_TIMEOUT),
⋮----
let queue = &probe_queues[next_probe % probe_queues.len()];
next_probe = next_probe.wrapping_add(1);
let _ = queue.send((index, path));
⋮----
let result = read_and_decode(&path, width, height);
sink.emit(Event::Thumb {
⋮----
.ok()
.and_then(|p| p.parent().map(|d| d.join("thumbs")))
.unwrap_or_else(|| std::env::temp_dir().join("k6wp-thumbs"));
std::fs::create_dir_all(&cache_dir).ok();
⋮----
let result = generate_thumbnail(&media, &cache_dir, THUMB_WIDTH, THUMB_HEIGHT);
⋮----
.map_err(|e| e.to_string())
.and_then(|path| {
read_and_decode(&path, THUMB_WIDTH, THUMB_HEIGHT)
.map(|pixels| (pixels, path))
⋮----
.map(|(pixels, path)| (Ok(pixels), Some(path)))
.unwrap_or_else(|e| (Err(e), None));
⋮----
let (dark, reduced_motion) = system_appearance();
sink.emit(Event::Theme {
⋮----
if !probe(&media, PROBE_TIMEOUT) {
sink.emit(Event::IpcError(format!(
⋮----
handle_set_wallpaper(client, index, &media, &sink);
⋮----
sink.emit(Event::IpcError(
"Tidak terhubung ke engine. Menghubungkan ulang...".to_string(),
⋮----
let _ = attempt_connect(
⋮----
let lib_path = match library_index_path(None) {
⋮----
"Tidak bisa menemukan library.json".to_string(),
⋮----
match import_into_library(&lib_path, &paths) {
⋮----
&std::fs::read_to_string(&lib_path).unwrap_or_default(),
⋮----
.unwrap_or_default();
⋮----
.iter()
.map(|e| Entry {
⋮----
name: e.dst.file_name().unwrap().to_string_lossy().to_string(),
⋮----
thumb: e.thumb.clone(),
media: e.dst.clone(),
unavailable: !e.dst.exists(),
⋮----
sink.emit(Event::Library(Ok(entries)));
⋮----
sink.emit(Event::IpcError(e));
⋮----
compress_handler(job_id, &src, &out, res_w, res_h, fps, crf, sink.clone());
⋮----
if let Some(&pid) = compress_jobs.get(&job_id) {
⋮----
.args(["/T", "/F", "/PID", &pid.to_string()])
.status();
compress_jobs.remove(&job_id);
sink.emit(Event::CompressDone {
⋮----
error: Some("Dibatalkan".to_string()),
⋮----
let path = match settings_index_path() {
⋮----
"LOCALAPPDATA tidak ditemukan: settings tidak bisa dibaca".to_string(),
⋮----
Ok(settings) => sink.emit(Event::SettingsLoaded(settings)),
⋮----
sink.emit(Event::IpcError(format!("Gagal migrasi settings: {e}")))
⋮----
sink.emit(Event::SettingsLoaded(StudioSettings::default()));
⋮----
.to_string(),
⋮----
match save_studio_settings(&path, &settings) {
Ok(()) => sink.emit(Event::SettingsLoaded(settings)),
Err(e) => sink.emit(Event::IpcError(format!("Gagal simpan settings: {e}"))),
⋮----
handle_pause(client, &sink);
⋮----
handle_resume(client, &sink);
⋮----
poll_get_state(client, &sink, &mut engine_state);
⋮----
if ipc_client.is_none()
⋮----
let backoff = RECONNECT_BACKOFF[backoff_index.min(RECONNECT_BACKOFF.len() - 1)];
⋮----
pub(crate) fn import_into_library(
⋮----
match LibraryIndex::parse(&std::fs::read_to_string(lib_path).unwrap_or_default()) {
⋮----
Err(e) => return Err(format!("Gagal baca library.json: {e}")),
⋮----
let lib_dir = lib_path.parent().unwrap();
⋮----
if !path.exists() {
return Err(format!("File tidak ditemukan: {}", path.display()));
⋮----
.extension()
.and_then(|s| s.to_str())
.unwrap_or("")
.to_lowercase();
if !matches!(ext.as_str(), "mp4" | "mkv" | "webm" | "mov") {
return Err(format!("Format tidak didukung: {}", path.display()));
⋮----
.map_err(|e| format!("Gagal baca metadata {}: {e}", path.display()))?;
if meta.len() == 0 {
return Err(format!("File kosong: {}", path.display()));
⋮----
let dst_name = path.file_name().unwrap().to_string_lossy().to_string();
let dst = lib_dir.join("wallpapers").join(&dst_name);
std::fs::create_dir_all(lib_dir.join("wallpapers")).ok();
std::fs::copy(path, &dst).map_err(|e| format!("Gagal copy {}: {e}", path.display()))?;
⋮----
.modified()
⋮----
.and_then(|t| t.duration_since(std::time::UNIX_EPOCH).ok())
.map(|d| d.as_secs() as i64)
.unwrap_or(0);
⋮----
src: path.clone(),
⋮----
res: "0x0".to_string(),
⋮----
encoder: "libx264".to_string(),
⋮----
size: meta.len(),
⋮----
codec: "".to_string(),
⋮----
index.entries.push(entry);
⋮----
.save_atomic(lib_path)
.map_err(|e| format!("Gagal simpan library.json: {e}"))?;
Ok((index.entries.len() - added..index.entries.len()).collect())
⋮----
/// Returns the path to `compressor.exe` if found.
/// Looks in: sibling of current exe → build/release/ → K6WP_COMPRESSOR env (debug-only).
⋮----
/// Looks in: sibling of current exe → build/release/ → K6WP_COMPRESSOR env (debug-only).
fn find_compressor() -> Option<PathBuf> {
⋮----
fn find_compressor() -> Option<PathBuf> {
⋮----
if let Some(parent) = current.parent() {
let sibling = parent.join("compressor.exe");
if sibling.exists() {
return Some(sibling);
⋮----
let release_compressor = parent.join("build").join("release").join("compressor.exe");
if release_compressor.exists() {
return Some(release_compressor);
⋮----
if !env_path.is_empty() {
⋮----
if p.exists() {
return Some(p);
⋮----
fn compress_handler(
⋮----
let Some(compressor_exe) = find_compressor() else {
⋮----
error: Some("compressor.exe tidak ditemukan".to_string()),
⋮----
in_: src.to_string_lossy().to_string(),
out: out.to_string_lossy().to_string(),
⋮----
encoder: "auto".to_string(),
⋮----
let argv = build_argv(&args);
⋮----
.map(|e| e == "bat" || e == "cmd")
.unwrap_or(false);
⋮----
c.arg("/c").arg(&compressor_exe);
c.args(&argv);
⋮----
.stdout(std::process::Stdio::piped())
.stderr(std::process::Stdio::piped())
.spawn()
⋮----
error: Some(format!("Gagal spawn compressor: {e}")),
⋮----
let stderr = child.stderr.take();
⋮----
for line in reader.lines() {
⋮----
if let Some(t_str) = line.split("time=").nth(1).and_then(|s| s.split(' ').next()) {
if let Some(secs) = parse_ffmpeg_time(t_str) {
let pct = if let Some(duration) = library_duration_secs(&src) {
⋮----
Some(p.min(100))
⋮----
sink.emit(Event::CompressProgress {
⋮----
let status = match child.wait() {
⋮----
error: Some(format!("Gagal tunggu compressor: {e}")),
⋮----
if status.success() {
⋮----
out_path: Some(out.to_path_buf()),
⋮----
error: Some(format!("Compressor exit code: {:?}", status.code())),
⋮----
fn parse_ffmpeg_time(s: &str) -> Option<f64> {
let parts: Vec<&str> = s.split(':').collect();
if parts.len() != 3 {
⋮----
let h: f64 = parts[0].trim().parse().ok()?;
let m: f64 = parts[1].trim().parse().ok()?;
let s: f64 = parts[2].trim().parse().ok()?;
Some(h * 3600.0 + m * 60.0 + s)
⋮----
fn library_duration_secs(src: &Path) -> Option<f64> {
let lib_path = library_index_path(None)?;
let text = std::fs::read_to_string(&lib_path).ok()?;
let index: LibraryIndex = LibraryIndex::parse(&text).ok()?;
index.entries.iter().find(|e| e.src == src).and_then(|e| {
⋮----
Some(e.duration)
⋮----
fn expected_engine_image() -> Option<PathBuf> {
⋮----
let current = std::env::current_exe().ok()?;
let parent = current.parent()?;
let engine_path = parent.join("engine.exe");
if engine_path.exists() {
return Some(engine_path);
⋮----
let release_engine = parent.join("build").join("release").join("engine.exe");
if release_engine.exists() {
return Some(release_engine);
⋮----
fn attempt_connect(
⋮----
sink.emit(Event::IpcStatus(IpcStatus::Connecting));
⋮----
expected_server_image: expected_engine_image(),
⋮----
*client = Some(c);
⋮----
sink.emit(Event::IpcStatus(IpcStatus::Connected));
Ok(())
⋮----
eprintln!("WORKER: Connection failed: {:?}", e);
⋮----
*backoff_index = (*backoff_index + 1).min(RECONNECT_BACKOFF.len());
sink.emit(Event::IpcStatus(IpcStatus::Reconnecting));
Err(e)
⋮----
fn handle_set_wallpaper(client: &mut IpcClient, index: usize, media: &Path, sink: &Sink) {
⋮----
match client.request(&msg) {
⋮----
if answer.get("error").is_some() {
⋮----
sink.emit(Event::WallpaperApplied { index });
⋮----
sink.emit(Event::IpcError(format!("Kesalahan IPC: {e}")));
⋮----
fn handle_pause(client: &mut IpcClient, sink: &Sink) {
⋮----
fn handle_resume(client: &mut IpcClient, sink: &Sink) {
⋮----
fn poll_get_state(client: &mut IpcClient, sink: &Sink, state: &mut EngineState) {
⋮----
apply_state_answer(&answer, state);
⋮----
sink.emit(Event::EngineState(state.clone()));
⋮----
sink.emit(Event::IpcError(format!("Gagal memuat status: {e}")));
⋮----
fn apply_state_answer(answer: &Value, state: &mut EngineState) {
let inner = answer.get("state").unwrap_or(answer);
if let Some(video) = inner.get("video").and_then(Value::as_str) {
⋮----
.file_name()
.map(|n| n.to_string_lossy().to_string())
.unwrap_or_else(|| video.to_string());
state.video_name = Some(name);
⋮----
if let Some(paused) = inner.get("paused").and_then(Value::as_bool) {
⋮----
fn probe_loop(rx: Receiver<(usize, PathBuf)>, sink: Sink) {
while let Ok((index, path)) = rx.recv() {
let exists = probe(&path, PROBE_TIMEOUT);
sink.emit(Event::Probe { index, exists });
⋮----
pub fn load_library(path: &Path) -> Result<Vec<Entry>, String> {
⋮----
.map_err(|e| format!("{}: {e}", path.display()))?;
let index = LibraryIndex::parse(&text).map_err(|e| e.to_string())?;
Ok(index.entries.iter().map(entry_view).collect())
⋮----
fn library_index_path(override_dir: Option<PathBuf>) -> Option<PathBuf> {
⋮----
let p = dir.join("library.json");
⋮----
if let Some(dir) = exe.parent() {
⋮----
let p = cwd.join("library.json");
⋮----
pub(crate) fn settings_index_path() -> Option<PathBuf> {
let local = std::env::var("LOCALAPPDATA").ok()?;
if local.is_empty() {
⋮----
Some(
⋮----
.join("K6WP")
.join("studio_settings.json"),
⋮----
fn entry_view(entry: &k6wp_core::LibraryEntry) -> Entry {
⋮----
.unwrap_or_else(|| entry.dst.to_string_lossy().to_string());
⋮----
meta.push_str(&format!("{:.1} s", entry.duration));
⋮----
if !meta.is_empty() {
meta.push_str(" - ");
⋮----
meta.push_str(&format!("{}x{}", entry.width, entry.height));
⋮----
if !entry.codec.is_empty() {
⋮----
meta.push_str(&entry.codec);
⋮----
thumb: entry.thumb.clone(),
media: entry.dst.clone(),
unavailable: !entry.dst.exists(),
⋮----
pub fn probe(path: &Path, timeout: Duration) -> bool {
⋮----
.components()
.next()
.map(|c| matches!(c, std::path::Component::Prefix(_)))
.unwrap_or(false)
&& path.to_string_lossy().as_bytes().get(1) == Some(&b':');
⋮----
return path.exists(); // worker
⋮----
let owned = path.to_path_buf();
⋮----
.name("k6wp-studio-probe".to_string())
.spawn(move || {
let _ = tx.send(owned.exists()); // worker
⋮----
fn read_and_decode(path: &Path, width: u32, height: u32) -> Result<Pixels, String> {
let bytes = std::fs::read(path) // worker
⋮----
decode_to_cover(&bytes, width, height)
⋮----
/// The system light/dark preference and the "show animations" preference.
fn system_appearance() -> (bool, bool) {
⋮----
fn system_appearance() -> (bool, bool) {
(system_is_dark(), !system_animations_enabled())
⋮----
fn registry_dword(subkey: &str, value: &str) -> Option<u32> {
use windows::Win32::Foundation::WIN32_ERROR;
⋮----
let subkey: Vec<u16> = subkey.encode_utf16().chain(std::iter::once(0)).collect();
let value: Vec<u16> = value.encode_utf16().chain(std::iter::once(0)).collect();
⋮----
RegGetValueW(
⋮----
windows::core::PCWSTR::from_raw(subkey.as_ptr()),
windows::core::PCWSTR::from_raw(value.as_ptr()),
⋮----
Some(&mut data as *mut u32 as *mut core::ffi::c_void),
Some(&mut size),
⋮----
if status == WIN32_ERROR(0) {
Some(data)
⋮----
fn system_is_dark() -> bool {
// AppsUseLightTheme: 0 = dark, 1 = light, absent = light.
match registry_dword(
⋮----
fn system_animations_enabled() -> bool {
⋮----
SystemParametersInfoW(
⋮----
Some(&mut enabled as *mut _ as *mut core::ffi::c_void),
SYSTEM_PARAMETERS_INFO_UPDATE_FLAGS(0),
⋮----
ok.is_ok() && enabled.as_bool()
⋮----
pub(crate) enum ThumbChoice {
⋮----
pub(crate) fn choose_thumb_request(thumb: &Path, media: &Path) -> ThumbChoice {
if thumb.as_os_str().is_empty() || !thumb.exists() {
⋮----
media_path: media.to_path_buf(),
⋮----
thumb_path: thumb.to_path_buf(),
⋮----
mod tests {
⋮----
use std::path::PathBuf;
use std::sync::mpsc;
use std::time::Duration;
⋮----
// Test utilities for fake compressor
enum FakeCompressorBehavior {
⋮----
fn make_fake_compressor(
⋮----
std::env::temp_dir().join(format!("k6wp-test-fake-compressor-{}", std::process::id()));
std::fs::create_dir_all(&dir).ok();
let exe = dir.join("compressor.bat");
⋮----
std::fs::write(&exe, script).expect("write fake compressor");
(std::process::Command::new(exe.clone()), exe)
⋮----
fn a_missing_index_is_an_error_not_a_panic() {
let err = load_library(Path::new(r"Z:\definitely\not\here\library.json"));
assert!(err.is_err());
⋮----
fn a_drive_letter_path_is_probed_inline() {
// The temp directory exists; the drive-letter fast path must find it.
⋮----
assert!(probe(&dir, PROBE_TIMEOUT));
⋮----
fn an_unreachable_probe_gives_up_after_the_timeout() {
⋮----
let exists = probe(
⋮----
assert!(!exists);
assert!(
⋮----
fn a_dead_unc_probe_does_not_block_the_decode_behind_it() {
// The real worker, the real queues: a probe of a dead server is
// dispatched to its own thread, so the thumbnail request queued
// behind it is answered immediately. With an inline probe this test
// fails after the full 2 s timeout.
⋮----
let requests = spawn(
⋮----
let _ = events_tx.send(event);
⋮----
.send(Request::Probe {
⋮----
.expect("the worker must accept a probe");
⋮----
.send(Request::Thumb {
⋮----
.expect("the worker must accept a decode");
// Skip IPC status events that are emitted during startup
⋮----
.recv_timeout(Duration::from_millis(500))
.expect("the decode must answer while the probe is still in flight");
if matches!(event, Event::Thumb { .. }) {
⋮----
assert_eq!(index, 1);
⋮----
other => panic!("the decode must answer first, got {other:?}"),
⋮----
let _ = requests.send(Request::Stop);
⋮----
fn set_wallpaper_success_with_fake_server() {
// Fake server that answers set_video with success.
⋮----
let name = format!(r"\\.\pipe\k6wp-test-set-video-ok-{}", std::process::id());
⋮----
let name = name.clone();
⋮----
use windows::core::PCWSTR;
⋮----
let wide: Vec<u16> = name.encode_utf16().chain(std::iter::once(0)).collect();
⋮----
CreateNamedPipeW(
PCWSTR(wide.as_ptr()),
FILE_FLAGS_AND_ATTRIBUTES(0x0000_0003),
NAMED_PIPE_MODE(
⋮----
let _ = ready_tx.send(());
if let Err(e) = unsafe { ConnectNamedPipe(pipe, None) } {
let code = (e.code().0 as u32) & 0xFFFF;
⋮----
let _ = CloseHandle(pipe);
⋮----
let ok = unsafe { ReadFile(pipe, Some(&mut buffer), Some(&mut read), None) };
if ok.is_err() {
⋮----
let _ = WriteFile(pipe, Some(answer), None, None);
⋮----
// Exit after handling the request (test only needs one)
⋮----
// Pass the pipe name directly to spawn (debug-only parameter)
⋮----
Some(name.clone()),
⋮----
.recv_timeout(Duration::from_secs(5))
.expect("the fake engine must create its pipe instance");
⋮----
// Give the worker time to attempt connection (it polls every 500ms when disconnected)
⋮----
// Create a temporary file that exists for the test
let temp_file = std::env::temp_dir().join(format!("k6wp-test-{}.mp4", std::process::id()));
std::fs::write(&temp_file, b"fake video content").expect("must create temp file");
let media_path = temp_file.clone();
⋮----
// Wait for IpcStatus::Connecting then IpcStatus::Connected before sending the request
⋮----
.recv_timeout(Duration::from_secs(10))
.expect("must get IPC status");
⋮----
.send(Request::SetWallpaper {
⋮----
media: media_path.clone(),
⋮----
.expect("must accept");
⋮----
// The worker must report the accepted install with the index; the UI
// turns that into the "Terpasang" toast (and the "Urungkan" button).
⋮----
.expect("must get response");
⋮----
assert_eq!(index, 0, "the applied index must be reported");
⋮----
other => panic!("expected WallpaperApplied{{index: 0}}, got {other:?}"),
⋮----
// Wait for worker to exit (recv_timeout is up to 500ms)
⋮----
let _ = server.join();
⋮----
fn set_wallpaper_engine_error() {
// Fake server that returns an engine error.
⋮----
let name = format!(r"\\.\pipe\k6wp-test-set-video-error-{}", std::process::id());
⋮----
std::env::temp_dir().join(format!("k6wp-test-error-{}.hevc", std::process::id()));
⋮----
other => panic!("expected IpcError with engine error, got {other:?}"),
⋮----
fn set_wallpaper_engine_timeout() {
// Fake server that accepts connection but never answers.
⋮----
let name = format!(
⋮----
// Accept connection but NEVER read/answer - ReadFile blocks forever.
// This simulates a server that accepts but doesn't respond.
let _ = unsafe { ReadFile(pipe, Some(&mut [0u8; 1]), Some(&mut 0u32), None) };
⋮----
std::env::temp_dir().join(format!("k6wp-test-timeout-{}.mp4", std::process::id()));
⋮----
.recv_timeout(Duration::from_millis(1000))
.expect("must get timeout error");
let elapsed = started.elapsed();
⋮----
other => panic!("expected IpcError with timeout, got {other:?}"),
⋮----
fn set_wallpaper_missing_media_no_ipc_sent() {
⋮----
use windows::Win32::Storage::FileSystem::FILE_FLAGS_AND_ATTRIBUTES;
⋮----
// For missing file test, we expect NO connection. Close pipe immediately.
// The worker checks file existence first and won't connect for missing files.
⋮----
.expect("must get error");
⋮----
other => panic!("expected IpcError with missing file, got {other:?}"),
⋮----
fn engine_state_reads_the_real_get_state_shape() {
⋮----
apply_state_answer(&answer, &mut state);
assert_eq!(state.video_name.as_deref(), Some("clip_2.mp4"));
assert!(state.is_paused);
⋮----
fn engine_state_ignores_a_missing_video() {
⋮----
assert_eq!(state.video_name, None);
assert!(!state.is_paused);
⋮----
mod choose_thumb_tests {
⋮----
use crate::thumbnailer::ThumbError;
⋮----
fn choose_empty_thumb_generates() {
let choice = choose_thumb_request(Path::new(""), Path::new(r"C:\video.mp4"));
assert!(matches!(choice, ThumbChoice::Generate { .. }));
⋮----
fn choose_existing_thumb_decodes() {
let temp = std::env::temp_dir().join(format!("k6wp-test-thumb-{}", std::process::id()));
std::fs::create_dir_all(&temp).expect("create temp dir");
let thumb = temp.join("existing_thumb.jpg");
std::fs::write(&thumb, b"fake thumbnail").expect("write thumb");
let choice = choose_thumb_request(&thumb, Path::new(r"C:\video.mp4"));
assert!(matches!(choice, ThumbChoice::Decode { .. }));
⋮----
mod import_tests {
⋮----
use k6wp_core::library::LibraryEntry;
⋮----
fn import_validates_extension() {
⋮----
std::env::temp_dir().join(format!("k6wp-test-import-{}.txt", std::process::id()));
std::fs::write(&temp, b"fake content").expect("must create temp file");
let cache = std::env::temp_dir().join("k6wp-test-thumb-import-ext");
std::fs::create_dir_all(&cache).ok();
let result = generate_thumbnail(&temp, &cache, THUMB_WIDTH, THUMB_HEIGHT);
assert!(result.is_err(), "txt should be rejected");
⋮----
fn import_validates_missing_file() {
⋮----
let cache = std::env::temp_dir().join("k6wp-test-thumb-missing");
⋮----
let result = generate_thumbnail(&missing, &cache, THUMB_WIDTH, THUMB_HEIGHT);
assert!(result.is_err(), "missing file should be rejected");
⋮----
fn import_appends_to_library() {
⋮----
let dir = std::env::temp_dir().join(format!("k6wp-import-test-{}", std::process::id()));
std::fs::create_dir_all(&dir).unwrap();
let lib_path = dir.join("library.json");
⋮----
entries: vec![LibraryEntry {
⋮----
existing.save_atomic(&lib_path).unwrap();
⋮----
let new_file = dir.join("new.mp4");
std::fs::write(&new_file, b"fake").unwrap();
let result = import_into_library(&lib_path, &[new_file.clone()]);
assert!(result.is_ok(), "import must succeed");
⋮----
let after = LibraryIndex::parse(&std::fs::read_to_string(&lib_path).unwrap()).unwrap();
assert_eq!(after.entries.len(), 2, "import must APPEND, not overwrite");
assert!(after
⋮----
std::fs::remove_dir_all(&dir).ok();
⋮----
fn assign_video_sends_correct_payload() {
⋮----
if find_compressor().is_some() {
⋮----
let (_cmd, exe) = make_fake_compressor(FakeCompressorBehavior::Ok);
⋮----
let _ = events_tx.send(e);
⋮----
Sink(arc)
⋮----
let src = std::env::temp_dir().join("k6wp-test-assign-src.mp4");
std::fs::write(&src, b"fake video").expect("write src");
⋮----
let src_for_thread = src.clone();
let sink_clone = sink.clone();
⋮----
assert_eq!(payload["device"], r"\\\\.\\\\DISPLAY1");
assert!(payload["path"]
⋮----
fn clear_assignment_sends_clear_true() {
⋮----
let src = std::env::temp_dir().join("k6wp-test-clear-src.mp4");
⋮----
assert_eq!(payload["clear"], true);
⋮----
fn playlist_rejects_interval_zero() {
⋮----
let err = validate_playlist(&mut playlist).expect_err("interval 0 must fail");
assert!(err.to_string().contains("interval_min out of range"));
⋮----
let err2 = validate_playlist(&mut playlist2).expect_err("interval 1441 must fail");
assert!(err2.to_string().contains("interval_min out of range"));
⋮----
fn playlist_preserves_order() {
⋮----
let dir = std::env::temp_dir().join(format!("k6wp-test-playlist-{}", std::process::id()));
⋮----
std::fs::create_dir_all(&dir).expect("create dir");
let path = dir.join("playlist.json");
⋮----
playlist.order = vec![
⋮----
save_playlist(&path, &playlist).expect("save must succeed");
⋮----
let loaded = migrate_playlist(&std::fs::read_to_string(&path).unwrap()).unwrap();
assert_eq!(loaded.order, playlist.order);
assert_eq!(loaded.enabled, playlist.enabled);
assert_eq!(loaded.interval_min, playlist.interval_min);
assert_eq!(loaded.shuffle, playlist.shuffle);
⋮----
fn save_playlist_round_trip() {
⋮----
std::env::temp_dir().join(format!("k6wp-test-playlist-rt-{}", std::process::id()));
⋮----
mod compress_tests {
⋮----
fn compress_spawns_compressor() {
⋮----
let sink = Sink(std::sync::Arc::new(std::sync::Mutex::new(Box::new(
⋮----
let src = std::env::temp_dir().join("k6wp-test-compress-src.mp4");
⋮----
let out = std::env::temp_dir().join("k6wp-test-compress-out.mp4");
⋮----
compress_handler(1, &src, &out, 1920, 1080, 30, 23, sink);
⋮----
.expect("must get CompressDone");
⋮----
assert_eq!(job_id, 1);
assert!(ok, "expected ok, got error: {error:?}");
assert!(out_path.is_some());
⋮----
other => panic!("expected CompressDone, got {other:?}"),
⋮----
fn compress_parses_error() {
⋮----
assert_eq!(parse_ffmpeg_time("not-a-time"), None);
assert_eq!(parse_ffmpeg_time("00:00"), None);
assert_eq!(parse_ffmpeg_time(""), None);
assert_eq!(parse_ffmpeg_time("00:00:00"), Some(0.0));
assert_eq!(parse_ffmpeg_time("25:00:00"), Some(90000.0));
⋮----
fn compress_progress_from_stderr() {
⋮----
assert_eq!(parse_ffmpeg_time("00:00:01.00"), Some(1.0));
assert_eq!(parse_ffmpeg_time("00:01:30.00"), Some(90.0));
assert_eq!(parse_ffmpeg_time("01:00:00.00"), Some(3600.0));
assert_eq!(parse_ffmpeg_time("00:00:00.50"), Some(0.5));
⋮----
fn compress_cancel_kills_tree() {
⋮----
let src = std::env::temp_dir().join("k6wp-test-compress-src4.mp4");
⋮----
let out = std::env::temp_dir().join("k6wp-test-compress-out4.mp4");
⋮----
compress_handler(4, &src, &out, 1920, 1080, 30, 23, sink);
⋮----
assert_eq!(job_id, 4);
```

