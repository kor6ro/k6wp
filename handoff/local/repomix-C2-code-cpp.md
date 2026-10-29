# User Provided Header
K6WP HANDOFF PACK - C2-code-cpp | branch refactor/rust-wt | read STATE.md + ledger.md first | no measurements in this run

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
// none. Asked of the shell rather than assembled from %USERPROFILE% so a folder
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
