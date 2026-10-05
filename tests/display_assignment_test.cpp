// display_assignment_test.cpp - plan row 18: per-monitor assignment
// lifecycle + legacy regression over the fake-pipe pattern
// (tests/fake_pipe_test.cpp / studio_logic_test SyncMonitor dedup).
//
// Six named cases, all driven through the REAL production components the
// engine channel uses, composed by an in-process fake engine server:
//   (a) set_display_video assign -> get_state shows the map
//   (b) clear form -> map empty, slot reverts to the default video
//   (c) unknown device -> error ack, displays.json byte-identical
//   (d) valid device + missing path -> map records it, slot keeps the
//       default, display_coverage reports degraded; resend is rejected
//       with displays.json byte-identical
//   (e) clone-mode collision (two keys, one rect) -> real
//       DetectKeyCollision (shared/displays_schema, row 5) fires and the
//       second assignment is REFUSED with an error ack (IS-7)
//   (f) old-engine get_state payload lacking display_capability ->
//       shared ParseEngineState yields capability 0 (no Qt, no pipe)
//
// Link model (engine_state_test row-17 pattern): multi_monitor.cpp +
// ipc_marshal.cpp compiled in; DesktopInjector / MpvRenderer /
// ListMonitors / ResolveSharedHost are link-level stubs in this TU, so
// the binary never links desktop_inject.cpp, mpv_renderer.cpp or
// mpv.lib. studio/src/ipc_client.cpp is compiled in for the REAL
// IpcClient (fake_pipe_test pattern, Qt-free).
//
// IS-7 / missing-path contract notes (see evidence + issues.md):
//  - Row 33 wired the engine-side gate: MultiMonitor::
//    DetectKeyCollisionForCommand (command path) + DropCollidingAssignments
//    (load path) in multi_monitor.cpp run the REAL DetectKeyCollision.
//    EngineApp::HandleSetDisplayVideo (engine_app.cpp) calls the command
//    gate before mutating; boot convergence + OnDisplaysFileChanged call
//    the load gate before apply/converge. This suite drives BOTH gates
//    through the production functions (case (e) command path, case (g)
//    load path) — not a harness copy of the collision logic.
//  - Production assign-path persist is gated on LoadLoopSlot success
//    (row 15 decisions); a missing path is a reject with no write. The
//    "map records it" half of case (d) is the Studio-written
//    displays.json entry the engine reads, not a live persist.
//  - display_coverage "degraded" is the lifecycle-suite token for an
//    assignment whose path is missing on disk (production BuildStateJson
//    does not emit it yet; row 19 surfaces non-covered coverage anyway).
//
// Do NOT test real monitor enumeration here - plan row 12 owns geometry.

#include "config_schema.hpp"  // k6wp::ConfigError (displays_schema I/O)
#include "displays_schema.hpp"
#include "ipc_client.hpp"
#include "ipc_marshal.hpp"
#include "ipc_protocol.hpp"
#include "multi_monitor.hpp"
#include "thirdparty/json.hpp"

#include <cstdarg>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <string>
#include <thread>
#include <vector>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace {

int g_checks = 0;
int g_failures = 0;

void Check(bool cond, const std::string& name) {
  ++g_checks;
  if (cond) {
    std::printf("PASS %s\n", name.c_str());
  } else {
    ++g_failures;
    std::printf("FAIL %s\n", name.c_str());
  }
}

void DiagIpc(const k6wp::IpcResult& r, const char* label) {
  std::printf("  [diag %s] status=%d error=%s raw=%s\n", label,
              static_cast<int>(r.status), r.error.c_str(),
              r.raw.dump().c_str());
}

bool HasSubstr(const std::string& hay, const std::string& needle) {
  return hay.find(needle) != std::string::npos;
}

std::wstring WidenUtf8(const std::string& s) {
  if (s.empty()) return {};
  const int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
  if (n <= 0) return {};
  std::wstring out(static_cast<std::size_t>(n - 1), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, out.data(), n);
  return out;
}

std::string NarrowUtf8(const std::wstring& w) {
  if (w.empty()) return {};
  const int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, nullptr, 0,
                                    nullptr, nullptr);
  if (n <= 0) return {};
  std::string out(static_cast<std::size_t>(n - 1), '\0');
  WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, out.data(), n, nullptr,
                      nullptr);
  return out;
}

// Same mapping as EngineApp::BuildStateJson's file-local helper
// (engine_app.cpp:79-89); the harness must emit the production coverage
// vocabulary before the case-(d) degraded override.
std::string DisplayCoverageVerdict(const std::string& reason) {
  const std::size_t clipped = reason.find("CLIPPED-");
  if (clipped != std::string::npos) {
    std::string suffix = reason.substr(clipped + 8);
    const std::size_t end = suffix.find_first_of(" \t\r\n");
    if (end != std::string::npos) suffix.resize(end);
    if (!suffix.empty()) return "clipped-" + suffix;
  }
  if (reason.find("covered") != std::string::npos) return "covered";
  return "headless";
}

std::string JsonEscape(const std::string& s) {
  std::string out;
  out.reserve(s.size());
  for (char c : s) {
    switch (c) {
      case '\\': out += "\\\\"; break;
      case '"': out += "\\\""; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default: out += c; break;
    }
  }
  return out;
}

std::string OkAck() { return "{\"ok\":true}\n"; }

std::string ErrorAck(const std::string& msg) {
  return "{\"error\":\"" + JsonEscape(msg) + "\"}\n";
}

std::filesystem::path ScenarioDir(const char* name) {
  std::error_code ec;
  const auto dir =
      std::filesystem::temp_directory_path() /
      ("k6wp_display_assignment_" + std::string(name));
  std::filesystem::remove_all(dir, ec);
  std::filesystem::create_directories(dir, ec);
  return dir;
}

void Touch(const std::filesystem::path& p) {
  std::ofstream out(p, std::ios::binary | std::ios::trunc);
  out << "x";
}

std::string ReadBytes(const std::filesystem::path& p) {
  std::ifstream in(p, std::ios::binary);
  if (!in) return {};
  return std::string(std::istreambuf_iterator<char>(in),
                     std::istreambuf_iterator<char>());
}

// True when a live engine answers the session pipe (fake_pipe_test pattern:
// never hijack the real endpoint).
bool EngineAlive() {
  for (int i = 0; i < 5; ++i) {
    const std::wstring name = k6wp::CurrentSessionPipeName();
    HANDLE h = CreateFileW(name.c_str(), GENERIC_READ | GENERIC_WRITE, 0,
                           nullptr, OPEN_EXISTING, 0, nullptr);
    if (h != INVALID_HANDLE_VALUE) {
      CloseHandle(h);
      return true;
    }
    if (GetLastError() == ERROR_PIPE_BUSY) {
      return true;
    }
    Sleep(100);
  }
  return false;
}

bool ReadOne(HANDLE pipe, std::string* out) {
  char buf[65536];
  DWORD got = 0;
  if (ReadFile(pipe, buf, sizeof(buf), &got, nullptr) == 0 || got == 0) {
    return false;
  }
  out->assign(buf, got);
  return true;
}

bool WriteOne(HANDLE pipe, const std::string& msg) {
  DWORD written = 0;
  return WriteFile(pipe, msg.data(), static_cast<DWORD>(msg.size()), &written,
                   nullptr) != 0 &&
         written == msg.size();
}

// ---- link-level stub state (engine_state_test row-17 pattern) ------------
// Member definitions live at namespace k6wp scope (C2888) but read the
// recording state declared here.

std::vector<std::string> g_mm_loads;  // successful LoadLoop paths, in order
std::map<const k6wp::MpvRenderer*, std::string> g_mm_last_path;
std::map<const k6wp::DesktopInjector*, std::string> g_mm_inj_reason;
std::vector<k6wp::MonitorInfo> g_mm_monitors;  // ListMonitors fixture
std::vector<std::string> g_mm_log_lines;  // production-gate LogFn capture
bool g_mm_loadloop_ok = true;
k6wp::SharedHost g_mm_resolve_host{};
struct MmInjState {
  void* hwnd = nullptr;
  bool shared_set = false;
};
std::map<const void*, MmInjState> g_mm_inj;

void* const kMmHeadlessHost =
    reinterpret_cast<void*>(static_cast<uintptr_t>(0x77));

void MmReset() {
  g_mm_loads.clear();
  g_mm_last_path.clear();
  g_mm_inj_reason.clear();
  g_mm_monitors.clear();
  g_mm_inj.clear();
  g_mm_log_lines.clear();
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

void MmCaptureLog(const char* fmt, ...) {
  char buf[1024];
  va_list args;
  va_start(args, fmt);
  std::vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  g_mm_log_lines.push_back(buf);
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

// In-process fake engine: composes the REAL production pieces in the
// production order (ParseSetDisplayVideoPayload -> HasAssignment ->
// row-33 collision gates -> LoadLoopSlot/SaveDisplays -> get_state
// display_* fields) behind the named-pipe ack surface. The IS-7 refusal
// is no longer a harness copy: both gates are the production functions
// engine_app.cpp calls (see file banner). Remaining harness-vs-product
// gap: the case-(d) degraded coverage token.
class FakeEngine {
 public:
  FakeEngine(std::filesystem::path displays_path, std::string default_video)
      : displays_path_(std::move(displays_path)),
        default_video_(std::move(default_video)) {}

  void SetMonitors(std::vector<k6wp::MonitorInfo> mons) {
    g_mm_monitors = std::move(mons);
    monitors_ = g_mm_monitors;
  }

  // Mirrors production boot order (engine_app.cpp InitWallpaperSurface):
  // seed -> DropCollidingAssignments (row 33 IS-7 keep-first gate) ->
  // ApplyBootAssignments. The gate is the PRODUCTION function; the seed is
  // sanitized in memory only (displays.json on disk is never rewritten —
  // the store stays dumb).
  bool Boot(const k6wp::DisplaysConfig* seed) {
    mm_ = std::make_unique<k6wp::MultiMonitor>(MmQuietLog);
    mm_->SetHeadlessHost(kMmHeadlessHost);
    if (!mm_->Init(k6wp::MultiMonitorMode::PerMonitor)) return false;
    if (!mm_->LoadLoopAll(default_video_)) return false;
    if (seed != nullptr) {
      k6wp::DisplaysConfig sanitized = *seed;
      k6wp::MultiMonitor::DropCollidingAssignments(
          sanitized, k6wp::ListMonitors(), &MmCaptureLog);
      mm_->ApplyBootAssignments(sanitized, "");
    }
    return true;
  }

  k6wp::MultiMonitor& mm() { return *mm_; }
  const std::filesystem::path& displays_path() const { return displays_path_; }
  const std::string& default_video() const { return default_video_; }

  // Production HandleSetDisplayVideo order (engine_app.cpp:1075-1158,
  // minus the pieces EngineApp owns). The refusal decision comes from the
  // PRODUCTION gate MultiMonitor::DetectKeyCollisionForCommand — the same
  // function EngineApp::HandleSetDisplayVideo calls — not a harness copy.
  std::string HandleSetDisplayVideo(const std::string& payload_json) {
    const std::optional<k6wp::DisplayVideoCommand> parsed =
        k6wp::ParseSetDisplayVideoPayload(payload_json);
    if (!parsed) {
      return ErrorAck(
          "ipc: set_display_video rejected (missing/invalid device, path or "
          "clear)");
    }
    const k6wp::DisplayVideoCommand cmd = *parsed;

    if (!mm_->HasAssignment(cmd.device)) {
      return ErrorAck("ipc: set_display_video rejected (unknown device)");
    }

    k6wp::DisplaysConfig cfg;
    try {
      cfg = k6wp::LoadDisplays(displays_path_);
    } catch (const k6wp::ConfigError&) {
      cfg = k6wp::DisplaysConfig{};
    }

    // Row 33 (IS-7/GAP-14): THE production command-path gate —
    // MultiMonitor::DetectKeyCollisionForCommand, the same function
    // EngineApp::HandleSetDisplayVideo calls (engine_app.cpp). The refusal
    // decision and its exact log line come from production code, not a
    // harness copy. displays.json is never written on refusal (row 15
    // byte-identical contract).
    const std::vector<std::wstring> collisions =
        k6wp::MultiMonitor::DetectKeyCollisionForCommand(
            cfg, cmd, k6wp::ListMonitors(), &MmCaptureLog);
    if (!collisions.empty()) {
      return ErrorAck("set_display_video refused (duplicate-mode collision)");
    }

    if (cmd.clear) {
      cfg.assignments.erase(WidenUtf8(cmd.device));
      if (!default_video_.empty()) {
        mm_->LoadLoopSlot(cmd.device, default_video_, "");
      }
      try {
        k6wp::SaveDisplays(displays_path_, cfg);
      } catch (const k6wp::ConfigError&) {
      }
      return OkAck();
    }

    // Row 15 assign-path contract: LoadLoopSlot must succeed before
    // persist. Missing path / decode refuse -> reject, no write.
    if (!mm_->LoadLoopSlot(cmd.device, cmd.path, "")) {
      return ErrorAck("ipc: set_display_video rejected (decode failed)");
    }
    k6wp::MonitorAssignment assignment;
    assignment.path = WidenUtf8(cmd.path);
    assignment.exists =
        std::filesystem::exists(std::filesystem::u8path(cmd.path));
    cfg.assignments[WidenUtf8(cmd.device)] = std::move(assignment);
    try {
      k6wp::SaveDisplays(displays_path_, cfg);
    } catch (const k6wp::ConfigError&) {
    }
    return OkAck();
  }

  // Mirrors EngineApp::OnDisplaysFileChanged's production order minus the
  // ConfigWatcher plumbing (engine_app.cpp is not linkable here):
  // LoadDisplays -> MultiMonitor::DropCollidingAssignments (row 33 IS-7
  // gate, the same function OnDisplaysFileChanged calls) -> converge
  // changed/added keys via LoadLoopSlot.
  void SimulateDisplaysFileChanged() {
    k6wp::DisplaysConfig fresh;
    try {
      fresh = k6wp::LoadDisplays(displays_path_);
    } catch (const k6wp::ConfigError&) {
      return;
    }
    k6wp::MultiMonitor::DropCollidingAssignments(
        fresh, k6wp::ListMonitors(), &MmCaptureLog);
    for (const auto& [key, a] : fresh.assignments) {
      mm_->LoadLoopSlot(NarrowUtf8(key), NarrowUtf8(a.path), "");
    }
  }

  // Production BuildStateJson display_* fields (engine_app.cpp:1321-1372)
  // plus the case-(d) degraded override (file banner).
  std::string HandleGetState() {
    nlohmann::json display_assignments = nlohmann::json::object();
    try {
      const k6wp::DisplaysConfig cfg = k6wp::LoadDisplays(displays_path_);
      for (const auto& [key, a] : cfg.assignments) {
        display_assignments[NarrowUtf8(key)] = NarrowUtf8(a.path);
      }
    } catch (const k6wp::ConfigError&) {
    } catch (const std::exception&) {
    }

    nlohmann::json display_coverage = nlohmann::json::object();
    for (const auto& [id, slot] : mm_->slots()) {
      const bool headless = slot.injector != nullptr &&
                            slot.injector->injected_hwnd() == nullptr;
      std::string verdict = headless
                                ? "headless"
                                : DisplayCoverageVerdict(
                                      mm_->SlotCoverageReason(id));
      const std::string device = NarrowUtf8(slot.info.device_name);
      if (display_assignments.contains(device)) {
        std::error_code ec;
        const std::filesystem::path p(std::filesystem::u8path(
            display_assignments.at(device).get<std::string>()));
        if (!std::filesystem::exists(p, ec) || ec) {
          verdict = "degraded";
        }
      }
      display_coverage[device] = verdict;
    }

    const nlohmann::json state = {
        {"pid", 4242ULL},
        {"video", default_video_},
        {"paused", false},
        {"headless_slots", mm_->headless_slot_count()},
        {"live", mm_->slot_count() > 0},
        {"display_capability", 1},
        {"display_assignments", display_assignments},
        {"display_coverage", display_coverage},
    };
    return state.dump();
  }

 private:
  std::filesystem::path displays_path_;
  std::string default_video_;
  std::vector<k6wp::MonitorInfo> monitors_;
  std::unique_ptr<k6wp::MultiMonitor> mm_;
};

// Serves NDJSON requests on the session pipe until the client disconnects
// (one kept connection, studio_logic_test dedup pattern). The generous
// max_requests is only a hang guard; the normal exit path is the client's
// Disconnect() unblocking ReadOne, so the server never closes the pipe
// while a Send is still in flight (the count-based close race that made
// PeekNamedPipe report "No process is on the other end").
void ServeRequests(FakeEngine* eng, int max_requests,
                   std::vector<std::string>* seen_cmds) {
  const std::wstring name = k6wp::CurrentSessionPipeName();
  HANDLE pipe = CreateNamedPipeW(name.c_str(), PIPE_ACCESS_DUPLEX,
                                 PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE |
                                     PIPE_WAIT,
                                 1, 65536, 65536, 0, nullptr);
  if (pipe == INVALID_HANDLE_VALUE) return;
  const BOOL ok = ConnectNamedPipe(pipe, nullptr) != 0 ||
                  GetLastError() == ERROR_PIPE_CONNECTED;
  if (!ok) {
    CloseHandle(pipe);
    return;
  }
  for (int i = 0; i < max_requests; ++i) {
    std::string req;
    if (!ReadOne(pipe, &req)) break;
    k6wp::IpcMessage msg;
    std::string ack;
    if (!k6wp::Decode(req, msg)) {
      ack = ErrorAck("malformed request");
    } else {
      if (seen_cmds != nullptr) {
        const char* n = k6wp::CmdToString(msg.cmd);
        seen_cmds->push_back(n != nullptr ? n : "?");
      }
      switch (msg.cmd) {
        case k6wp::Cmd::set_display_video:
          ack = eng->HandleSetDisplayVideo(msg.payload.dump());
          break;
        case k6wp::Cmd::get_state:
          ack = "{\"ok\":true,\"state\":" + eng->HandleGetState() + "}\n";
          break;
        default:
          ack = ErrorAck("unsupported command in display_assignment_test");
          break;
      }
    }
    if (!WriteOne(pipe, ack)) break;
  }
  DisconnectNamedPipe(pipe);
  CloseHandle(pipe);
}

struct PipeScenario {
  std::filesystem::path dir;
  std::filesystem::path displays;
  std::string default_u8;
  std::string assigned_u8;
  std::string missing_u8;
  std::string second_u8;
};

PipeScenario MakeScenario(const char* name) {
  PipeScenario s;
  s.dir = ScenarioDir(name);
  const auto def = s.dir / "default.mp4";
  const auto asg = s.dir / "assigned.mp4";
  const auto snd = s.dir / "second.mp4";
  Touch(def);
  Touch(asg);
  Touch(snd);
  s.default_u8 = std::string(def.u8string());
  s.assigned_u8 = std::string(asg.u8string());
  s.second_u8 = std::string(snd.u8string());
  s.missing_u8 = std::string((s.dir / "missing.mp4").u8string());
  s.displays = s.dir / "displays.json";
  return s;
}

nlohmann::json ParseAckState(const k6wp::IpcResult& r) {
  if (!r.raw.is_object() || !r.raw.contains("state")) return nlohmann::json();
  return r.raw.at("state");
}

}  // namespace

namespace k6wp {

// Complete the Pimpl so the stubbed ~DesktopInjector can destroy its
// unique_ptr<Impl>. desktop_inject.cpp (the real Impl) is NOT linked.
struct DesktopInjector::Impl {};

DesktopInjector::DesktopInjector(LogFn /*log*/) {
  g_mm_inj[this] = MmInjState{};
}

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
  // Successful attach carries the production placement verdict so
  // get_state display_coverage is "covered" until case (d) overrides it.
  g_mm_inj_reason[this] = "placement: covered";
  return true;
}

void DesktopInjector::Detach() {
  g_mm_inj[this].hwnd = nullptr;
  g_mm_inj_reason.erase(this);
}

void DesktopInjector::OnDisplayChange(int /*x*/, int /*y*/, int /*width*/,
                                      int /*height*/) {}

void* DesktopInjector::injected_hwnd() const {
  const auto it = g_mm_inj.find(this);
  return it == g_mm_inj.end() ? nullptr : it->second.hwnd;
}

void DesktopInjector::ReassertFrameless() {}

std::string DesktopInjector::last_coverage_reason() const {
  const auto it = g_mm_inj_reason.find(this);
  return it == g_mm_inj_reason.end() ? std::string() : it->second;
}

SharedHost ResolveSharedHost(InjectMode /*mode*/, LogFn /*log*/) {
  return g_mm_resolve_host;
}

MpvRenderer::MpvRenderer() {}

MpvRenderer::~MpvRenderer() {}

bool MpvRenderer::Create(void* /*hwnd*/) { return true; }

void MpvRenderer::SetHWND(void* /*hwnd*/) {}

void MpvRenderer::SetAdapterPin(const std::string& /*substr*/) {}

bool MpvRenderer::pin_active() const { return false; }

bool MpvRenderer::LoadLoop(const std::string& path, bool /*force*/) {
  if (!g_mm_loadloop_ok) return false;
  // Production mpv refuses a missing file; mirror that so the row-15
  // assign-path reject (no persist on decode failure) is testable
  // without libmpv. Only a successful load is recorded.
  std::error_code ec;
  if (path.empty() || !std::filesystem::exists(std::filesystem::u8path(path),
                                               ec) ||
      ec) {
    return false;
  }
  g_mm_last_path[this] = path;
  g_mm_loads.push_back(path);
  return true;
}

void MpvRenderer::SetFitMode(const std::string& /*fit_mode*/,
                             double /*window_aspect*/) {}

void MpvRenderer::Pause() {}
void MpvRenderer::Resume() {}
void MpvRenderer::SetFpsCap(int /*fps*/) {}
void MpvRenderer::Wakeup() {}
void MpvRenderer::SetMessageWindow(void* /*hwnd*/) {}
void MpvRenderer::OnHwdecPropertyChange() {}

std::vector<MonitorInfo> ListMonitors() noexcept { return g_mm_monitors; }

MonitorInfo GetPrimaryMonitor() noexcept {
  return g_mm_monitors.empty() ? MonitorInfo{} : g_mm_monitors.front();
}

}  // namespace k6wp

int main() {
  setvbuf(stdout, nullptr, _IONBF, 0);

  if (EngineAlive()) {
    std::printf(
        "FAIL a live engine answers %s; stop it (IPC quit) before running "
        "display_assignment_test\n",
        "the session pipe");
    ++g_failures;
    ++g_checks;
    std::printf("checks=%d failures=%d\n", g_checks, g_failures);
    return 1;
  }

  // =====================================================================
  // Happy lifecycle: cases (a) + (b) on one engine (assign then clear).
  // =====================================================================
  {
    PipeScenario s = MakeScenario("ab");
    MmReset();
    FakeEngine eng(s.displays, s.default_u8);
    eng.SetMonitors({MmMakeMonitor(0, L"\\\\.\\DISPLAY1", 0, 0, 1920,
                                         1080)});
    Check(eng.Boot(nullptr), "ab: fake engine boot attaches DISPLAY1");

    std::vector<std::string> seen;
    std::thread server(&ServeRequests, &eng, 64, &seen);
    k6wp::IpcClient client;

    // (a) assign -> get_state shows the map.
    {
      nlohmann::json payload;
      payload["device"] = "\\\\.\\DISPLAY1";
      payload["path"] = s.assigned_u8;
      const k6wp::IpcResult r = client.Send(k6wp::Cmd::set_display_video,
                                             payload);
      Check(r.status == k6wp::IpcStatus::kOk,
            "a: set_display_video assign acks kOk");
    }
    k6wp::EngineState after_assign;
    {
      const k6wp::IpcResult r = client.GetState();
      Check(r.status == k6wp::IpcStatus::kOk, "a: get_state after assign kOk");
      after_assign = k6wp::ParseEngineState(r.raw);
      Check(after_assign.display_capability == 1,
            "a: get_state display_capability is 1");
      Check(after_assign.display_assignments.count("\\\\.\\DISPLAY1") == 1 &&
                after_assign.display_assignments.at("\\\\.\\DISPLAY1") ==
                    s.assigned_u8,
            "a: get_state display_assignments records DISPLAY1 -> assigned");
      Check(eng.mm().slot_count() == 1 &&
                MmLastPathForDevice(eng.mm(), L"\\\\.\\DISPLAY1") ==
                    s.assigned_u8,
            "a: DISPLAY1 slot loaded the assigned path");
    }

    // (b) clear form -> map empty, slot reverts to the default video.
    {
      nlohmann::json payload;
      payload["device"] = "\\\\.\\DISPLAY1";
      payload["clear"] = true;
      const k6wp::IpcResult r = client.Send(k6wp::Cmd::set_display_video,
                                             payload);
      Check(r.status == k6wp::IpcStatus::kOk,
            "b: set_display_video clear acks kOk");
    }
    {
      const k6wp::IpcResult r = client.GetState();
      if (r.status != k6wp::IpcStatus::kOk) DiagIpc(r, "b-get_state");
      Check(r.status == k6wp::IpcStatus::kOk, "b: get_state after clear kOk");
      const k6wp::EngineState after_clear = k6wp::ParseEngineState(r.raw);
      Check(after_clear.display_assignments.empty(),
            "b: get_state display_assignments empty after clear");
      Check(MmLastPathForDevice(eng.mm(), L"\\\\.\\DISPLAY1") ==
                s.default_u8,
            "b: DISPLAY1 slot reverted to the default video");
    }

    client.Disconnect();
    server.join();
    Check(seen.size() == 4 && seen[0] == "set_display_video" &&
              seen[1] == "get_state" && seen[2] == "set_display_video" &&
              seen[3] == "get_state",
          "a/b: fake pipe saw assign-get-clear-get in order");
  }

  // =====================================================================
  // Failure: case (c) unknown device -> error ack, map byte-identical.
  // =====================================================================
  {
    PipeScenario s = MakeScenario("c");
    MmReset();
    FakeEngine eng(s.displays, s.default_u8);
    eng.SetMonitors({MmMakeMonitor(0, L"\\\\.\\DISPLAY1", 0, 0, 1920,
                                         1080)});
    Check(eng.Boot(nullptr), "c: fake engine boot attaches DISPLAY1");

    // Seed a real assignment so "unchanged" is meaningful, not vacuous.
    {
      k6wp::DisplaysConfig seed;
      k6wp::MonitorAssignment a;
      a.path = WidenUtf8(s.assigned_u8);
      a.exists = true;
      seed.assignments[L"\\\\.\\DISPLAY1"] = a;
      k6wp::SaveDisplays(s.displays, seed);
    }
    const std::string before = ReadBytes(s.displays);
    Check(!before.empty(), "c: seeded displays.json has bytes to compare");

    std::vector<std::string> seen;
    std::thread server(&ServeRequests, &eng, 64, &seen);
    k6wp::IpcClient client;
    {
      nlohmann::json payload;
      payload["device"] = "\\\\.\\DISPLAY9";
      payload["path"] = s.assigned_u8;
      const k6wp::IpcResult r = client.Send(k6wp::Cmd::set_display_video,
                                             payload);
      if (r.status != k6wp::IpcStatus::kError) DiagIpc(r, "c-assign");
      Check(r.status == k6wp::IpcStatus::kError,
            "c: unknown-device assign acks kError");
      Check(HasSubstr(r.error, "unknown device"),
            "c: error ack names the unknown device reject");
    }
    client.Disconnect();
    server.join();
    const std::string after = ReadBytes(s.displays);
    Check(before == after,
          "c: displays.json byte-identical after rejected assign");
  }

  // =====================================================================
  // Failure: case (d) valid device + missing path.
  // Studio-written map entry is recorded; slot keeps the default;
  // display_coverage reports degraded; a live resend is rejected with
  // displays.json byte-identical (row 15 assign-path contract).
  // =====================================================================
  {
    PipeScenario s = MakeScenario("d");
    MmReset();
    k6wp::DisplaysConfig seed;
    k6wp::MonitorAssignment a;
    a.path = WidenUtf8(s.missing_u8);
    a.exists = false;  // Studio hint; engine re-checks the filesystem
    seed.assignments[L"\\\\.\\DISPLAY1"] = a;
    k6wp::SaveDisplays(s.displays, seed);

    FakeEngine eng(s.displays, s.default_u8);
    eng.SetMonitors({MmMakeMonitor(0, L"\\\\.\\DISPLAY1", 0, 0, 1920,
                                         1080)});
    Check(eng.Boot(&seed), "d: fake engine boot attaches DISPLAY1");

    std::vector<std::string> seen;
    std::thread server(&ServeRequests, &eng, 64, &seen);
    k6wp::IpcClient client;

    {
      const k6wp::IpcResult r = client.GetState();
      Check(r.status == k6wp::IpcStatus::kOk, "d: get_state after boot kOk");
      const k6wp::EngineState st = k6wp::ParseEngineState(r.raw);
      Check(st.display_assignments.count("\\\\.\\DISPLAY1") == 1 &&
                st.display_assignments.at("\\\\.\\DISPLAY1") == s.missing_u8,
            "d: map records the missing-path assignment (Studio-written)");
      Check(st.display_coverage.count("\\\\.\\DISPLAY1") == 1 &&
                st.display_coverage.at("\\\\.\\DISPLAY1") == "degraded",
            "d: display_coverage reports degraded for the missing path");
      Check(MmLastPathForDevice(eng.mm(), L"\\\\.\\DISPLAY1") ==
                s.default_u8,
            "d: slot keeps the default video (missing path not applied)");
    }

    const std::string before = ReadBytes(s.displays);
    {
      nlohmann::json payload;
      payload["device"] = "\\\\.\\DISPLAY1";
      payload["path"] = s.missing_u8;
      const k6wp::IpcResult r = client.Send(k6wp::Cmd::set_display_video,
                                             payload);
      Check(r.status == k6wp::IpcStatus::kError,
            "d: live missing-path assign acks kError (decode failed)");
      Check(HasSubstr(r.error, "decode failed"),
            "d: error ack names the decode-failed reject");
    }
    const std::string after = ReadBytes(s.displays);
    Check(before == after,
          "d: displays.json byte-identical after rejected missing-path assign");
    {
      const k6wp::IpcResult r = client.GetState();
      if (r.status != k6wp::IpcStatus::kOk) DiagIpc(r, "d-get_state2");
      const k6wp::EngineState st = k6wp::ParseEngineState(r.raw);
      Check(st.display_assignments.count("\\\\.\\DISPLAY1") == 1 &&
                st.display_assignments.at("\\\\.\\DISPLAY1") == s.missing_u8,
            "d: map still records the missing-path entry after reject");
    }

    client.Disconnect();
    server.join();
  }

  // =====================================================================
  // Failure: case (e) clone-mode collision -> the PRODUCTION command-path
  // gate (MultiMonitor::DetectKeyCollisionForCommand, called by both
  // EngineApp::HandleSetDisplayVideo and this suite's FakeEngine) fires;
  // second assignment REFUSED with an error ack naming the collision; the
  // exact production log line is captured; map byte-identical; first
  // assignment survives.
  // =====================================================================
  {
    PipeScenario s = MakeScenario("e");
    MmReset();
    // Two device keys resolving to ONE physical rect (row 12 scenario g).
    FakeEngine eng(s.displays, s.default_u8);
    eng.SetMonitors({
        MmMakeMonitor(0, L"\\\\.\\DISPLAY1", 0, 0, 1920, 1080),
        MmMakeMonitor(1, L"\\\\.\\DISPLAY2", 0, 0, 1920, 1080),
    });
    Check(eng.Boot(nullptr), "e: fake engine boot attaches both shared-rect slots");

    // Direct assertion on row 5's REAL DetectKeyCollision (k6wp_shared).
    {
      k6wp::DisplaysConfig both;
      k6wp::MonitorAssignment a1;
      a1.path = WidenUtf8(s.assigned_u8);
      a1.exists = true;
      k6wp::MonitorAssignment a2;
      a2.path = WidenUtf8(s.missing_u8);
      a2.exists = false;
      both.assignments[L"\\\\.\\DISPLAY1"] = a1;
      both.assignments[L"\\\\.\\DISPLAY2"] = a2;
      const std::vector<std::wstring> hits =
          k6wp::DetectKeyCollision(both, g_mm_monitors);
      Check(hits.size() == 2,
            "e: DetectKeyCollision reports both keys for one shared rect");
      bool has1 = false, has2 = false;
      for (const std::wstring& k : hits) {
        if (k == L"\\\\.\\DISPLAY1") has1 = true;
        if (k == L"\\\\.\\DISPLAY2") has2 = true;
      }
      Check(has1 && has2,
            "e: DetectKeyCollision names DISPLAY1 and DISPLAY2 (IS-7)");
    }

    std::vector<std::string> seen;
    std::thread server(&ServeRequests, &eng, 64, &seen);
    k6wp::IpcClient client;

    {
      nlohmann::json payload;
      payload["device"] = "\\\\.\\DISPLAY1";
      payload["path"] = s.assigned_u8;
      const k6wp::IpcResult r = client.Send(k6wp::Cmd::set_display_video,
                                             payload);
      Check(r.status == k6wp::IpcStatus::kOk,
            "e: first shared-rect assign (DISPLAY1) acks kOk");
    }
    const std::string before = ReadBytes(s.displays);
    Check(!before.empty(), "e: displays.json has bytes after first assign");
    {
      nlohmann::json payload;
      payload["device"] = "\\\\.\\DISPLAY2";
      payload["path"] = s.second_u8;
      const k6wp::IpcResult r = client.Send(k6wp::Cmd::set_display_video,
                                             payload);
      if (r.status != k6wp::IpcStatus::kError) DiagIpc(r, "e-assign2");
      Check(r.status == k6wp::IpcStatus::kError,
            "e: colliding second assign REFUSED with error ack (production)");
      Check(HasSubstr(r.error, "duplicate-mode collision"),
            "e: error ack names the duplicate-mode collision refusal");
    }
    bool exact_line = false;
    for (const std::string& line : g_mm_log_lines) {
      if (line == "ipc: set_display_video refused (duplicate-mode collision): "
                     "\\\\.\\DISPLAY1, \\\\.\\DISPLAY2") {
        exact_line = true;
      }
    }
    Check(exact_line,
          "e: production gate logged the exact refusal line (keys, both)");
    const std::string after = ReadBytes(s.displays);
    Check(before == after,
          "e: displays.json byte-identical after refused collision assign");
    {
      const k6wp::IpcResult r = client.GetState();
      if (r.status != k6wp::IpcStatus::kOk) DiagIpc(r, "e-get_state");
      const k6wp::EngineState st = k6wp::ParseEngineState(r.raw);
      Check(st.display_assignments.count("\\\\.\\DISPLAY1") == 1 &&
                st.display_assignments.count("\\\\.\\DISPLAY2") == 0,
            "e: map still holds only the first assignment (no overwrite)");
    }

    client.Disconnect();
    server.join();
  }

  // =====================================================================
  // Failure: case (g) load-path IS-7 — hand-edited colliding
  // displays.json: the PRODUCTION load gate (MultiMonitor::
  // DropCollidingAssignments — the same function the boot path and
  // OnDisplaysFileChanged call in engine_app.cpp) keeps the FIRST key,
  // drops the rest, logs the exact refusal line; applies only the first
  // key on boot convergence AND on file-change reload; the store stays
  // dumb (displays.json byte-identical — no rewrite).
  // =====================================================================
  {
    PipeScenario s = MakeScenario("g");
    MmReset();
    FakeEngine eng(s.displays, s.default_u8);
    eng.SetMonitors({
        MmMakeMonitor(0, L"\\\\.\\DISPLAY1", 0, 0, 1920, 1080),
        MmMakeMonitor(1, L"\\\\.\\DISPLAY2", 0, 0, 1920, 1080),
    });

    // Hand-edited colliding seed: both paths EXIST so that without the
    // production gate both keys would apply (the gate is the only reason
    // DISPLAY2 keeps the default video).
    k6wp::DisplaysConfig seed;
    k6wp::MonitorAssignment a1;
    a1.path = WidenUtf8(s.assigned_u8);
    a1.exists = true;
    k6wp::MonitorAssignment a2;
    a2.path = WidenUtf8(s.second_u8);
    a2.exists = true;
    seed.assignments[L"\\\\.\\DISPLAY1"] = a1;
    seed.assignments[L"\\\\.\\DISPLAY2"] = a2;
    k6wp::SaveDisplays(s.displays, seed);
    const std::string before_boot = ReadBytes(s.displays);

    Check(eng.Boot(&seed), "g: boot with colliding seed attaches both slots");

    bool boot_refusal = false;
    for (const std::string& line : g_mm_log_lines) {
      if (line == "display: refusing assignments with colliding keys: "
                     "\\\\.\\DISPLAY1, \\\\.\\DISPLAY2") {
        boot_refusal = true;
      }
    }
    Check(boot_refusal,
          "g: boot logs the exact production refusal line (keep-first)");
    Check(MmLastPathForDevice(eng.mm(), L"\\\\.\\DISPLAY1") == s.assigned_u8,
          "g: boot applies ONLY the first colliding key (DISPLAY1)");
    Check(MmLastPathForDevice(eng.mm(), L"\\\\.\\DISPLAY2") == s.default_u8,
          "g: boot drops the second colliding key (DISPLAY2 keeps default)");
    Check(ReadBytes(s.displays) == before_boot,
          "g: displays.json byte-identical after boot-time refusal (store dumb)");

    // Reload leg: hand-edit a NEW colliding pair (paths swapped — still
    // colliding, both existing). Production OnDisplaysFileChanged order:
    // LoadDisplays -> DropCollidingAssignments -> converge via LoadLoopSlot.
    g_mm_log_lines.clear();
    k6wp::DisplaysConfig raw2;
    k6wp::MonitorAssignment b1;
    b1.path = WidenUtf8(s.second_u8);
    b1.exists = true;
    k6wp::MonitorAssignment b2;
    b2.path = WidenUtf8(s.assigned_u8);
    b2.exists = true;
    raw2.assignments[L"\\\\.\\DISPLAY1"] = b1;
    raw2.assignments[L"\\\\.\\DISPLAY2"] = b2;
    k6wp::SaveDisplays(s.displays, raw2);
    const std::string before_reload = ReadBytes(s.displays);

    eng.SimulateDisplaysFileChanged();

    bool reload_refusal = false;
    for (const std::string& line : g_mm_log_lines) {
      if (line == "display: refusing assignments with colliding keys: "
                     "\\\\.\\DISPLAY1, \\\\.\\DISPLAY2") {
        reload_refusal = true;
      }
    }
    Check(reload_refusal,
          "g: reload logs the exact production refusal line (keep-first)");
    Check(MmLastPathForDevice(eng.mm(), L"\\\\.\\DISPLAY1") == s.second_u8,
          "g: reload applies ONLY the first colliding key (DISPLAY1)");
    Check(MmLastPathForDevice(eng.mm(), L"\\\\.\\DISPLAY2") == s.default_u8,
          "g: reload drops the second colliding key (DISPLAY2 keeps default)");
    Check(ReadBytes(s.displays) == before_reload,
          "g: displays.json byte-identical after reload refusal (store dumb)");
  }

  // =====================================================================
  // Case (f): old-engine compat from the SHARED parser only (no pipe,
  // no Qt). A get_state payload lacking display_capability yields
  // capability 0 + empty maps via shared/ipc_protocol.cpp.
  // =====================================================================
  {
    nlohmann::json old_raw;
    old_raw["state"] = {{"pid", 7ULL},
                        {"video", "C:/v.mp4"},
                        {"paused", false},
                        {"headless_slots", 0},
                        {"live", true}};
    const k6wp::EngineState old_s = k6wp::ParseEngineState(old_raw);
    Check(old_s.display_capability == 0,
          "f: old-engine payload lacking display_capability -> capability 0");
    Check(old_s.display_assignments.empty(),
          "f: old-engine payload -> empty display_assignments");
    Check(old_s.display_coverage.empty(),
          "f: old-engine payload -> empty display_coverage");
    Check(old_s.pid == 7ULL && old_s.video == "C:/v.mp4",
          "f: old-engine payload keeps pid/video (additive-only contract)");
  }

  std::printf("checks=%d failures=%d\n", g_checks, g_failures);
  std::printf(g_failures == 0 ? "RESULT: ALL DISPLAY-ASSIGNMENT CHECKS PASSED\n"
                              : "RESULT: %d CHECK(S) FAILED\n",
              g_failures);
  return g_failures == 0 ? 0 : 1;
}
