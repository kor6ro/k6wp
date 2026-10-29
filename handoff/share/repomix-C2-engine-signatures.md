# User Provided Header
K6WP HANDOFF PACK - C2-engine-signatures | branch refactor/rust-wt | read STATE.md + ledger.md first | no measurements in this run

# Directory Structure
```
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

