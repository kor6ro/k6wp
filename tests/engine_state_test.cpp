// engine_state_test.cpp - unit tests for the components extracted from
// EngineApp: PinVerifySchedule (deadline + revert counter) and
// PlaylistController (reload + snapshot + one-shot rotation) with fake hooks.

#include "ipc_command_marshal.hpp"
#include "ipc_marshal.hpp"
#include "occlusion_poke_scheduler.hpp"
#include "pin_verify_schedule.hpp"
#include "playlist_controller.hpp"

// Row 15: EngineState additive display fields + displays.json io (linked
// via k6wp_shared PUBLIC include of shared/).
#include "displays_schema.hpp"
#include "ipc_protocol.hpp"
#include "playlist.hpp"
#include "thirdparty/json.hpp"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

// Row 17: boot-time assignment convergence drives the REAL MultiMonitor
// (src/multi_monitor.cpp is compiled into this suite) against link-level
// DesktopInjector / MpvRenderer / ListMonitors stubs below.
#include <cstdarg>
#include <map>
#include <utility>
#include <vector>

#include "multi_monitor.hpp"

namespace {

int g_failures = 0;

void Check(bool ok, const char* what) {
  std::printf("%s %s\n", ok ? "PASS" : "FAIL", what);
  if (!ok) {
    ++g_failures;
  }
}

std::filesystem::path TempDir() {
  std::error_code ec;
  const auto dir = std::filesystem::temp_directory_path() / "k6wp_engine_state_test";
  std::filesystem::remove_all(dir, ec);
  std::filesystem::create_directories(dir, ec);
  return dir;
}

void Touch(const std::filesystem::path& p) {
  std::ofstream out(p, std::ios::binary | std::ios::trunc);
  out << "x";
}

// ---- Row 17: recording stubs for the MultiMonitor boot-convergence checks --
// Link model (multi_monitor_factory_test pattern): multi_monitor.cpp is
// compiled into this suite; every DesktopInjector / MpvRenderer /
// ListMonitors / ResolveSharedHost symbol it references is defined here, so
// the binary never links desktop_inject.cpp, mpv_renderer.cpp or mpv.lib.
// The stubs record what production passes (LoadLoop paths in call order,
// per-renderer last path) so the tests assert BEHAVIOUR, not mock magic.
// Member definitions live at namespace k6wp scope (C2888: never inside an
// anonymous namespace) but read the recording state declared here.

std::vector<std::string> g_mm_logs;    // LogFn lines, in order
std::vector<std::string> g_mm_loads;   // LoadLoop paths, in call order
std::map<const k6wp::MpvRenderer*, std::string> g_mm_last_path;
std::vector<k6wp::MonitorInfo> g_mm_monitors;  // ListMonitors fixture
bool g_mm_loadloop_ok = true;
k6wp::SharedHost g_mm_resolve_host{};  // ResolveSharedHost fixture
// Per-injector recording (factory-test g_inj pattern): Impl is not usable
// for storage here (test-TU body differs from desktop_inject.cpp).
struct MmInjState {
  void* hwnd = nullptr;
  bool shared_set = false;
};
std::map<const void*, MmInjState> g_mm_inj;

// Sentinel handed to SetHeadlessHost; must reach MpvRenderer::Create only on
// the headless fallback path.
void* const kMmHeadlessHost = reinterpret_cast<void*>(static_cast<uintptr_t>(0x77));

void MmReset() {
  g_mm_logs.clear();
  g_mm_loads.clear();
  g_mm_last_path.clear();
  g_mm_monitors.clear();
  g_mm_inj.clear();
  g_mm_loadloop_ok = true;
  k6wp::SharedHost sh;
  sh.host = reinterpret_cast<void*>(static_cast<uintptr_t>(0xBEEF));
  sh.progman = reinterpret_cast<void*>(static_cast<uintptr_t>(0xFEED));
  sh.def_view = reinterpret_cast<void*>(static_cast<uintptr_t>(0xDEA1));
  sh.insert_after = sh.def_view;
  sh.client_rect.left = 0;
  sh.client_rect.top = 0;
  sh.client_rect.right = 1920;
  sh.client_rect.bottom = 1080;
  g_mm_resolve_host = sh;
}

void MmQuietLog(const char* /*fmt*/, ...) {}

void MmRecordLog(const char* fmt, ...) {
  char buf[512];
  va_list args;
  va_start(args, fmt);
  std::vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  g_mm_logs.push_back(buf);
}

bool MmLogContains(const std::string& needle) {
  for (const std::string& line : g_mm_logs) {
    if (line.find(needle) != std::string::npos) return true;
  }
  return false;
}

k6wp::MonitorInfo MmMakeMonitor(int id, const wchar_t* device, int x, int y,
                                int w, int h) {
  k6wp::MonitorInfo mi;
  mi.id = id;
  mi.x = x;
  mi.y = y;
  mi.width = w;
  mi.height = h;
  mi.is_primary = (id == 0);
  mi.device_name = device;
  return mi;
}

// Last LoadLoop path recorded for the live slot whose GDI device name matches
// (empty string = no slot / never loaded). Reads MultiMonitor::slots() the
// same way EngineApp::BuildStateJson does (public accessor, auto& binding).
std::string MmLastPathForDevice(const k6wp::MultiMonitor& mm,
                                const std::wstring& device) {
  for (const auto& kv : mm.slots()) {
    if (kv.second.info.device_name == device && kv.second.renderer) {
      const auto it = g_mm_last_path.find(kv.second.renderer.get());
      if (it != g_mm_last_path.end()) return it->second;
    }
  }
  return {};
}

}  // namespace

namespace k6wp {

// Complete the Pimpl so the stubbed ~DesktopInjector can destroy its
// unique_ptr<Impl>. desktop_inject.cpp (the real Impl) is NOT linked.
struct DesktopInjector::Impl {};

// ---- link-level stubs: DesktopInjector (desktop_inject.cpp NOT linked) ----

DesktopInjector::DesktopInjector(LogFn /*log*/) { g_mm_inj[this] = MmInjState{}; }

DesktopInjector::~DesktopInjector() { g_mm_inj.erase(this); }

void DesktopInjector::SetInjectMode(InjectMode /*mode*/) {}

void DesktopInjector::SetSharedHost(const SharedHost& host) {
  MmInjState& s = g_mm_inj[this];
  s.shared_set = host.host != nullptr;
}

bool DesktopInjector::Attach(int /*x*/, int /*y*/, int /*width*/,
                             int /*height*/) {
  MmInjState& s = g_mm_inj[this];
  if (!s.shared_set || !g_mm_resolve_host.host) return false;
  s.hwnd = reinterpret_cast<void*>(static_cast<uintptr_t>(0x1000));
  return true;
}

void DesktopInjector::Detach() { g_mm_inj[this].hwnd = nullptr; }

void DesktopInjector::OnDisplayChange(int /*x*/, int /*y*/, int /*width*/,
                                      int /*height*/) {}

void* DesktopInjector::injected_hwnd() const {
  const auto it = g_mm_inj.find(this);
  return it == g_mm_inj.end() ? nullptr : it->second.hwnd;
}

void DesktopInjector::ReassertFrameless() {}

std::string DesktopInjector::last_coverage_reason() const { return {}; }

// The free ResolveSharedHost stub: returns the configurable fixture
// (g_mm_resolve_host = {} simulates Progman absent -> attach refuses).
SharedHost ResolveSharedHost(InjectMode /*mode*/, LogFn /*log*/) {
  return g_mm_resolve_host;
}

// ---- link-level stubs: MpvRenderer (mpv_renderer.cpp/mpv.lib NOT linked) --

MpvRenderer::MpvRenderer() {}

MpvRenderer::~MpvRenderer() {}

bool MpvRenderer::Create(void* /*hwnd*/) { return true; }

void MpvRenderer::SetHWND(void* /*hwnd*/) {}

void MpvRenderer::SetAdapterPin(const std::string& /*substr*/) {}

bool MpvRenderer::pin_active() const { return false; }

bool MpvRenderer::LoadLoop(const std::string& path, bool /*force*/) {
  g_mm_last_path[this] = path;
  g_mm_loads.push_back(path);
  return g_mm_loadloop_ok;
}

void MpvRenderer::SetFitMode(const std::string& /*fit_mode*/,
                             double /*window_aspect*/) {}

void MpvRenderer::Pause() {}
void MpvRenderer::Resume() {}
void MpvRenderer::SetFpsCap(int /*fps*/) {}
void MpvRenderer::Wakeup() {}
void MpvRenderer::SetMessageWindow(void* /*hwnd*/) {}
void MpvRenderer::OnHwdecPropertyChange() {}

// ---- link-level stubs: monitor enumeration ---------------------------------
// Deterministic fixture: whatever g_mm_monitors holds (set per scenario).

std::vector<MonitorInfo> ListMonitors() noexcept { return g_mm_monitors; }

MonitorInfo GetPrimaryMonitor() noexcept {
  return g_mm_monitors.empty() ? MonitorInfo{} : g_mm_monitors.front();
}

}  // namespace k6wp

int main() {
  // PinVerifySchedule: deadline gating + lifetime counter.
  {
    k6wp::PinVerifySchedule s;
    Check(!s.Due(), "pin: fresh schedule not due");
    s.Arm(false);
    Check(!s.Due(), "pin: Arm(false) does not arm");
    s.Arm(true);
    Check(!s.Due(), "pin: armed but the 6s deadline has not elapsed");
    Check(s.AddReverts(3) == 3, "pin: add 3 returns 3");
    Check(s.AddReverts(2) == 5, "pin: add 2 returns 5");
    Check(s.total() == 5, "pin: total is 5");
    s.Disarm();
    Check(!s.Due(), "pin: disarmed schedule not due");
  }

  // PlaylistController: enabled playlist -> snapshot + one-shot rotation.
  {
    const auto dir = TempDir();
    const auto file = dir / "playlist.json";
    const auto a = dir / "a.mp4";
    const auto b = dir / "b.mp4";
    Touch(a);
    Touch(b);

    k6wp::PlaylistConfig cfg;
    cfg.enabled = true;
    cfg.interval_min = 30;
    cfg.shuffle = false;
    cfg.order = {a.wstring(), b.wstring()};
    k6wp::SavePlaylist(file, cfg);

    k6wp::PlaylistController pc;
    std::string last_set;
    pc.SetHooks(
        [&]() { return file; },
        [&]() { return std::string(a.u8string()); },
        [&](const std::string& p) {
          last_set = p;
          return true;
        });
    pc.Reload();
    Check(pc.enabled(), "pl: enabled after reload");
    Check(pc.has_order(), "pl: has order");
    Check(pc.get_enabled(), "pl: snapshot enabled");
    Check(pc.get_size() == 2, "pl: snapshot size 2");
    Check(pc.get_index() == 0, "pl: snapshot index 0 for current a");
    Check(pc.Fire(), "pl: Fire returns true");
    Check(last_set == std::string(b.u8string()), "pl: Fire switched to b");
  }

  // PlaylistController: disabled playlist -> no rotation.
  {
    const auto dir = TempDir();
    const auto file = dir / "playlist.json";
    const auto a = dir / "a.mp4";
    const auto b = dir / "b.mp4";
    Touch(a);
    Touch(b);

    k6wp::PlaylistConfig cfg;
    cfg.enabled = false;
    cfg.interval_min = 30;
    cfg.order = {a.wstring(), b.wstring()};
    k6wp::SavePlaylist(file, cfg);

    k6wp::PlaylistController pc;
    int calls = 0;
    pc.SetHooks([&]() { return file; },
                [&]() { return std::string(a.u8string()); },
                [&](const std::string&) {
                  ++calls;
                  return true;
                });
    pc.Reload();
    Check(!pc.enabled(), "pl: disabled reload not enabled");
    Check(!pc.Fire(), "pl: Fire on disabled returns false");
    Check(calls == 0, "pl: disabled Fire did not set a video");
  }

  // PlaylistController: missing file -> disarmed snapshot.
  {
    const auto dir = TempDir();
    const auto file = dir / "absent" / "playlist.json";
    k6wp::PlaylistController pc;
    pc.SetHooks([&]() { return file; },
                []() { return std::string(); },
                [](const std::string&) { return true; });
    pc.Reload();
    Check(!pc.get_enabled(), "pl: missing file -> snapshot disabled");
    Check(pc.get_size() == 0, "pl: missing file -> size 0");
    Check(!pc.Fire(), "pl: missing file -> Fire false");
  }

  // OcclusionPokeScheduler: the timer handler runs the check only when live.
  {
    k6wp::OcclusionPokeScheduler s;
    int checks = 0;
    bool live = true;
    s.SetHooks(nullptr, [&]() { return live; }, [&]() { return true; },
               [&]() { ++checks; });
    s.OnTimer();
    Check(checks == 1, "poke: OnTimer runs the check when live");
    live = false;
    s.OnTimer();
    Check(checks == 1, "poke: OnTimer skips the check when not live");
  }

  // IpcCommandMarshal: a well-formed command is validated then dropped when no
  // hidden window is set; a bad payload is rejected before anything is queued.
  {
    const auto dir = TempDir();
    const auto file = dir / "v.mp4";
    Touch(file);
    k6wp::IpcCommandMarshal m;
    std::string out;

    Check(!m.QueueVideo("{\"path\":\"\"}"),
          "marshal: empty path rejected");
    Check(!m.TakeVideo(&out), "marshal: nothing queued after reject");

    nlohmann::json j;
    j["path"] = file.u8string();
    Check(!m.QueueVideo(j.dump()),
          "marshal: valid path dropped without a window");
    Check(!m.TakeVideo(&out), "marshal: dropped video not left queued");

    Check(!m.QueueMonitor("{\"monitor\":0}"),
          "marshal: monitor dropped without a window");
    Check(!m.TakeMonitor(&out), "marshal: dropped monitor not left queued");
    Check(!m.QueueMonitor("{\"monitor\":\"x\"}"),
          "marshal: non-integer monitor rejected");
  }

  // Row 14: set_display_video rides the same CRIT-2 marshal path. Reject
  // shape (row 13 contract): clear+path together is a strict reject, so
  // nothing is queued. Failure shape (mirrors set_monitor without window):
  // a valid assign payload is validated then dropped when no hidden window
  // is set. Happy shape: with a message-only window, QueueDisplayVideo
  // posts kSetDisplayVideoMessage (WM_APP+0x56) and TakeDisplayVideo
  // returns the payload byte-identically — the main-loop consume path.
  {
    k6wp::IpcCommandMarshal m;
    std::string out;

    Check(!m.QueueDisplayVideo(
              R"({"device":"\\\\.\\DISPLAY1","path":"C:/a.mp4","clear":true})"),
          "marshal: clear+path display video rejected");
    Check(!m.TakeDisplayVideo(&out),
          "marshal: nothing queued after display video reject");

    const std::string assign =
        R"({"device":"\\\\.\\DISPLAY1","path":"C:/Videos/a.mp4"})";
    Check(!m.QueueDisplayVideo(assign),
          "marshal: display video dropped without a window");
    Check(!m.TakeDisplayVideo(&out),
          "marshal: dropped display video not left queued");

    Check(k6wp::kSetDisplayVideoMessage == WM_APP + 0x56u,
          "marshal: display video message id is WM_APP+0x56");
    Check(k6wp::kSetDisplayVideoMessage != k6wp::kSetMonitorMessage &&
              k6wp::kSetDisplayVideoMessage != k6wp::kSetVideoMessage,
          "marshal: display video message id distinct from monitor/video");

    WNDCLASSW wc{};
    wc.lpfnWndProc = DefWindowProcW;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"K6WP.EngineStateTest.DisplayVideoWindow";
    const ATOM atom = RegisterClassW(&wc);
    HWND hwnd = nullptr;
    if (atom != 0) {
      hwnd = CreateWindowExW(0, wc.lpszClassName, L"", 0, 0, 0, 0, 0,
                             HWND_MESSAGE, nullptr, wc.hInstance, nullptr);
    }
    Check(hwnd != nullptr, "marshal: test message window created");
    if (hwnd != nullptr) {
      m.SetWindow(hwnd);
      Check(m.QueueDisplayVideo(assign),
            "marshal: valid display video queued with a window");
      MSG msg{};
      const BOOL got = PeekMessageW(&msg, hwnd, k6wp::kSetDisplayVideoMessage,
                                    k6wp::kSetDisplayVideoMessage, PM_REMOVE);
      Check(got == TRUE && msg.message == k6wp::kSetDisplayVideoMessage,
            "marshal: WM_APP+0x56 posted to the hidden window");
      Check(m.TakeDisplayVideo(&out) && out == assign,
            "marshal: display video round-trips byte-identically");
      m.ClearWindow();
      DestroyWindow(hwnd);
    }
    if (atom != 0) {
      UnregisterClassW(wc.lpszClassName, wc.hInstance);
    }
  }

  // Row 15: additive get_state display fields + displays.json round-trip.
  //
  // (i) Old-engine compat: a get_state payload WITHOUT the new keys parses
  //     with display_capability 0 and empty maps (Studio feature-detect).
  // (ii) With keys present, ParseEngineState surfaces capability + maps.
  // (iii) Unknown-device contract for HandleSetDisplayVideo: \\.\DISPLAY9 is
  //      shape-valid for row 13's validator (assign AND clear forms), so the
  //      handler's false + "ipc: set_display_video rejected (unknown
  //      device)" + displays.json byte-identity come from slot resolution
  //      (MultiMonitor::HasAssignment / LoadLoopSlot), not payload parsing.
  //      This suite links the validator + shared schema but NOT engine_app /
  //      multi_monitor (engine/CMakeLists.txt test blocks are row 12's
  //      concurrent lane; no CMake edit this row); the handler-level
  //      observable is proven live in the task's failure evidence.
  // (iv) A valid assignment round-trips through SaveDisplays/LoadDisplays
  //      (row 5 store) — the persist path HandleSetDisplayVideo uses
  //      instead of PersistConfigField.
  {
    nlohmann::json old_raw;
    old_raw["state"] = {{"pid", 42ULL},
                        {"video", "C:/v.mp4"},
                        {"paused", false},
                        {"headless_slots", 0},
                        {"live", true}};
    const k6wp::EngineState old_s = k6wp::ParseEngineState(old_raw);
    Check(old_s.pid == 42ULL, "state: old-engine pid preserved");
    Check(old_s.video == "C:/v.mp4", "state: old-engine video preserved");
    Check(old_s.display_capability == 0,
          "state: old-engine payload -> display_capability 0");
    Check(old_s.display_assignments.empty(),
          "state: old-engine payload -> empty display_assignments");
    Check(old_s.display_coverage.empty(),
          "state: old-engine payload -> empty display_coverage");

    nlohmann::json new_raw;
    new_raw["state"] = {
        {"pid", 7ULL},
        {"display_capability", 1},
        {"display_assignments",
         {{"\\\\.\\DISPLAY1", "C:/Videos/a.mp4"},
          {"\\\\.\\DISPLAY2", "C:/Videos/b.mp4"}}},
        {"display_coverage",
         {{"\\\\.\\DISPLAY1", "covered"},
          {"\\\\.\\DISPLAY2", "clipped-left"}}},
    };
    const k6wp::EngineState new_s = k6wp::ParseEngineState(new_raw);
    Check(new_s.pid == 7ULL, "state: extended payload pid preserved");
    Check(new_s.display_capability == 1,
          "state: display_capability parsed as 1");
    Check(new_s.display_assignments.size() == 2,
          "state: display_assignments size 2");
    Check(new_s.display_assignments.at("\\\\.\\DISPLAY1") ==
              "C:/Videos/a.mp4",
          "state: display_assignments DISPLAY1 -> a.mp4");
    Check(new_s.display_coverage.at("\\\\.\\DISPLAY2") == "clipped-left",
          "state: display_coverage DISPLAY2 -> clipped-left");

    nlohmann::json bad_raw;
    bad_raw["state"] = {{"display_capability", "nope"},
                        {"display_assignments", nlohmann::json::array()},
                        {"display_coverage", 3}};
    const k6wp::EngineState bad_s = k6wp::ParseEngineState(bad_raw);
    Check(bad_s.display_capability == 0,
          "state: wrong-type capability -> default 0");
    Check(bad_s.display_assignments.empty(),
          "state: wrong-type assignments -> empty map");
    Check(bad_s.display_coverage.empty(),
          "state: wrong-type coverage -> empty map");

    const auto unk = k6wp::ParseSetDisplayVideoPayload(
        R"({"device":"\\\\.\\DISPLAY9","path":"C:/Videos/a.mp4"})");
    Check(unk.has_value(),
          "display: unknown-device assign payload parses (shape valid)");
    Check(unk && unk->device == "\\\\.\\DISPLAY9" && !unk->clear,
          "display: unknown-device assign carries device + clear=false");
    const auto unk_clear = k6wp::ParseSetDisplayVideoPayload(
        R"({"device":"\\\\.\\DISPLAY9","clear":true})");
    Check(unk_clear.has_value() && unk_clear->clear &&
              unk_clear->path.empty(),
          "display: unknown-device clear payload parses (same reject layer)");

    const auto dir = TempDir();
    const auto file = dir / "displays.json";
    k6wp::DisplaysConfig cfg;
    k6wp::MonitorAssignment a;
    a.path = L"C:\\Videos\\a.mp4";
    a.exists = true;
    cfg.assignments[L"\\\\.\\DISPLAY1"] = a;
    k6wp::SaveDisplays(file, cfg);
    const k6wp::DisplaysConfig loaded = k6wp::LoadDisplays(file);
    Check(loaded.assignments.count(L"\\\\.\\DISPLAY1") == 1,
          "display: assignment survives SaveDisplays/LoadDisplays");
    Check(loaded.assignments.at(L"\\\\.\\DISPLAY1").path ==
              L"C:\\Videos\\a.mp4",
          "display: round-trip path intact");
    Check(loaded.assignments.at(L"\\\\.\\DISPLAY1").exists,
          "display: round-trip exists flag intact");
  }

  // Row 17: boot-time assignment convergence over the real MultiMonitor
  // (link-level stubs above). Mirrors the production boot sequence:
  // Init -> LoadLoopAll(default) -> ApplyBootAssignments(displays.json).
  // (i)   empty map -> LoadLoopAll exactly once with the default path and
  //       zero per-slot LoadLoop calls (IS-4 regression guard).
  // (ii)  assignment for a device absent from ListMonitors() -> retained;
  //       OnDisplayChange with the device present re-applies the path.
  // (iii) assignment whose path fails exists -> slot keeps the default and
  //       the degraded log line is asserted verbatim.
  {
    const auto dir = TempDir();
    const auto default_mp4 = dir / "default.mp4";
    const auto assigned_mp4 = dir / "assigned.mp4";
    const auto missing_mp4 = dir / "does_not_exist.mp4";
    Touch(default_mp4);
    Touch(assigned_mp4);
    const std::string default_u8(default_mp4.u8string());
    const std::string assigned_u8(assigned_mp4.u8string());
    const std::string missing_u8(missing_mp4.u8string());

    auto make_cfg = [](const wchar_t* device, const std::wstring& path) {
      k6wp::DisplaysConfig cfg;
      k6wp::MonitorAssignment a;
      a.path = path;
      a.exists = true;
      cfg.assignments[device] = a;
      return cfg;
    };

    // (i) IS-4: empty map leaves the boot path byte-identical.
    {
      MmReset();
      g_mm_monitors.push_back(
          MmMakeMonitor(0, L"\\\\.\\DISPLAY1", 0, 0, 1920, 1080));
      k6wp::MultiMonitor mm(MmQuietLog);
      mm.SetHeadlessHost(kMmHeadlessHost);
      Check(mm.Init(k6wp::MultiMonitorMode::PerMonitor),
            "boot17-i: Init attaches the live monitor");
      Check(mm.LoadLoopAll(default_u8),
            "boot17-i: LoadLoopAll accepts the default path");
      const k6wp::DisplaysConfig empty;
      const k6wp::BootAssignmentStats stats = mm.ApplyBootAssignments(empty, "");
      Check(g_mm_loads.size() == 1 && g_mm_loads[0] == default_u8,
            "boot17-i: LoadLoopAll called exactly once with the default path");
      Check(stats.applied == 0 && stats.retained == 0 &&
                stats.skipped_missing == 0,
            "boot17-i: empty map yields zero applied/retained/skipped");
      Check(!MmLogContains("display: applied") &&
                !MmLogContains("display: retained assignment") &&
                !MmLogContains("display: assignment path missing"),
            "boot17-i: empty map emits no assignment log lines");
    }

    // (ii) absent device -> retained -> re-applied on OnDisplayChange (IS-6).
    {
      MmReset();
      g_mm_monitors.push_back(
          MmMakeMonitor(0, L"\\\\.\\DISPLAY1", 0, 0, 1920, 1080));
      k6wp::MultiMonitor mm(MmRecordLog);
      mm.SetHeadlessHost(kMmHeadlessHost);
      mm.Init(k6wp::MultiMonitorMode::PerMonitor);
      mm.LoadLoopAll(default_u8);
      const k6wp::DisplaysConfig cfg = make_cfg(L"\\\\.\\DISPLAY9",
                                                 assigned_mp4.wstring());
      const k6wp::BootAssignmentStats stats = mm.ApplyBootAssignments(cfg, "");
      Check(stats.retained == 1 && stats.applied == 0,
            "boot17-ii: absent-device assignment is retained");
      Check(MmLogContains(
                "display: retained assignment for absent \\\\.\\DISPLAY9"),
            "boot17-ii: retained log line names the absent device");
      Check(mm.slot_count() == 1,
            "boot17-ii: no slot appears for the absent device");
      Check(MmLastPathForDevice(mm, L"\\\\.\\DISPLAY9").empty(),
            "boot17-ii: absent device has no loaded path yet");

      g_mm_monitors.push_back(
          MmMakeMonitor(8, L"\\\\.\\DISPLAY9", 1920, 0, 2560, 1440));
      mm.OnDisplayChange();
      Check(mm.slot_count() == 2,
            "boot17-ii: OnDisplayChange attaches the returning device");
      Check(MmLastPathForDevice(mm, L"\\\\.\\DISPLAY9") == assigned_u8,
            "boot17-ii: returning slot receives the retained path");
      Check(MmLogContains("display: applied " + assigned_u8 +
                              " to \\\\.\\DISPLAY9"),
            "boot17-ii: applied log line names path and device");
      const std::size_t applied_logs_after_first =
          g_mm_loads.size();
      mm.OnDisplayChange();
      Check(g_mm_loads.size() == applied_logs_after_first,
            "boot17-ii: retained entry is consumed (no re-apply on next pass)");
    }

    // (iii) missing path -> slot keeps the default + degraded log (rule b).
    {
      MmReset();
      g_mm_monitors.push_back(
          MmMakeMonitor(0, L"\\\\.\\DISPLAY1", 0, 0, 1920, 1080));
      k6wp::MultiMonitor mm(MmRecordLog);
      mm.SetHeadlessHost(kMmHeadlessHost);
      mm.Init(k6wp::MultiMonitorMode::PerMonitor);
      mm.LoadLoopAll(default_u8);
      const k6wp::DisplaysConfig cfg = make_cfg(L"\\\\.\\DISPLAY1",
                                                 missing_mp4.wstring());
      const k6wp::BootAssignmentStats stats = mm.ApplyBootAssignments(cfg, "");
      Check(stats.skipped_missing == 1 && stats.applied == 0,
            "boot17-iii: missing-path assignment is skipped, not applied");
      Check(MmLogContains("display: assignment path missing for "
                          "\\\\.\\DISPLAY1: " + missing_u8 +
                          " - falling back to default"),
            "boot17-iii: degraded log line asserted verbatim");
      Check(MmLastPathForDevice(mm, L"\\\\.\\DISPLAY1") == default_u8,
            "boot17-iii: slot keeps the default path");
      Check(g_mm_loads.size() == 1 && g_mm_loads[0] == default_u8,
            "boot17-iii: no per-slot LoadLoop for the missing path");
    }
  }

  std::printf(g_failures == 0 ? "RESULT: ALL ENGINE-STATE CHECKS PASSED\n"
                              : "RESULT: %d CHECK(S) FAILED\n",
              g_failures);
  return g_failures == 0 ? 0 : 1;
}
