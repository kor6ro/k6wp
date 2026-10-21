// reconcile_shadow_test.cpp - Engine Display Resilience, P3/Fase 5.
//
// Tests the P3 shadow reconcile pipeline:
//   * SnapshotTopology: mapping/order/hash determinism
//   * diff_topology integration (added/removed/moved/resized/dpi/orient/primary/hash)
//   * TopologyDivergesFromMonitors: yes/no
//   * VerifyCoverage: covered/degraded census
//   * zero slot mutation (ApplyTopology is a read-only no-op in shadow)
//   * SharedHostBranch
//   * ApplyTopology returns zero counts (shadow no-op)
//
// LINK MODEL: engine/src/multi_monitor.cpp + src/desktop_placement.cpp +
// src/topology_diff.cpp against TU-level stubs (this TU). No
// desktop_inject.cpp, no mpv_renderer.cpp, no mpv.lib. user32:
// GetSystemMetrics in MultiMonitor::GetSpanGeometry. k6wp_shared:
// displays_schema.
//
// No external test framework: plain Check() counter; exit 0 = pass.

#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "multi_monitor.hpp"
#include "topology_diff.hpp"
#include "topology_snapshot.hpp"

#include "desktop_placement.hpp"

namespace k6wp {

struct DesktopInjector::Impl {};

namespace {

struct InjState {
  LogFn log = nullptr;
  InjectMode mode = InjectMode::kAuto;
  bool mode_set = false;
  bool shared_set = false;
  SharedHost shared{};
  void* hwnd = nullptr;
  int attach_calls = 0;
  std::string last_cov;
  int att_x = 0, att_y = 0, att_w = 0, att_h = 0;
};

struct RenState {
  bool created = false;
  void* create_hwnd = nullptr;
  std::string adapter_pin;
};

struct Rect4 {
  int x = 0, y = 0, w = 0, h = 0;
};

std::map<const DesktopInjector*, InjState> g_inj;
std::map<const MpvRenderer*, RenState> g_ren;
std::vector<Rect4> g_attach_rects;
std::vector<MonitorInfo> g_monitors;
bool g_attach_ok = true;
int g_inj_ctors = 0, g_inj_dtors = 0;
int g_ren_ctors = 0, g_ren_dtors = 0;
int g_factory_injectors = 0, g_factory_renderers = 0;
SharedHost g_resolve_result{};
int g_resolve_calls = 0;
InjectMode g_resolve_mode = InjectMode::kAuto;
std::vector<std::string> g_logs;

void* const kHeadlessHost = reinterpret_cast<void*>(static_cast<uintptr_t>(0x77));

int g_checks = 0;
int g_failures = 0;

void Check(bool cond, const std::string& name) {
  ++g_checks;
  if (cond) {
    std::cout << "[PASS] " << name << "\n";
  } else {
    ++g_failures;
    std::cout << "[FAIL] " << name << "\n";
  }
}

void QuietLog(const char* /*fmt*/, ...) {}

void RecordLog(const char* fmt, ...) {
  char buf[512];
  va_list args;
  va_start(args, fmt);
  std::vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  g_logs.push_back(buf);
}

bool LogContains(const std::string& needle) {
  for (const std::string& line : g_logs) {
    if (line.find(needle) != std::string::npos) return true;
  }
  return false;
}

MonitorInfo MakeMon(int id, int x, int y, int w, int h, bool primary,
                    const std::wstring& device,
                    const std::string& stable_key = "",
                    bool degraded = false) {
  MonitorInfo m;
  m.id = id;
  m.x = x;
  m.y = y;
  m.width = w;
  m.height = h;
  m.is_primary = primary;
  m.device_name = device;
  m.stable_key = stable_key;
  m.identity_degraded = degraded;
  m.scale_pct = 100;
  m.orientation = 0;
  return m;
}

SharedHost MakeHost(const char* branch, bool layered, int left, int top,
                    int right, int bottom) {
  SharedHost sh;
  sh.host = reinterpret_cast<void*>(static_cast<uintptr_t>(0xBEEF));
  sh.progman = reinterpret_cast<void*>(static_cast<uintptr_t>(0xFEED));
  sh.def_view = reinterpret_cast<void*>(static_cast<uintptr_t>(0xDEA1));
  sh.insert_after = sh.def_view;
  sh.layered = layered;
  sh.branch = branch;
  sh.client_rect.left = left;
  sh.client_rect.top = top;
  sh.client_rect.right = right;
  sh.client_rect.bottom = bottom;
  return sh;
}

void ResetStubs() {
  g_inj.clear();
  g_ren.clear();
  g_attach_rects.clear();
  g_logs.clear();
  g_attach_ok = true;
  g_inj_ctors = g_inj_dtors = 0;
  g_ren_ctors = g_ren_dtors = 0;
  g_factory_injectors = g_factory_renderers = 0;
  g_resolve_result = MakeHost("shared host", true, 0, 0, 1920, 1080);
  g_resolve_calls = 0;
  g_resolve_mode = InjectMode::kAuto;
}

SlotFactory MakeRecordingFactory() {
  SlotFactory f;
  f.make_injector = [](LogFn log) {
    ++g_factory_injectors;
    return std::make_unique<DesktopInjector>(log);
  };
  f.make_renderer = []() {
    ++g_factory_renderers;
    return std::make_unique<MpvRenderer>();
  };
  return f;
}

}  // namespace

// ---- link-level stubs ----

void DesktopInjector::HideVoChildren() {}

DesktopInjector::DesktopInjector(LogFn log) {
  g_inj[this].log = log;
  ++g_inj_ctors;
}

DesktopInjector::~DesktopInjector() { ++g_inj_dtors; }

void DesktopInjector::SetInjectMode(InjectMode mode) {
  InjState& s = g_inj[this];
  s.mode = mode;
  s.mode_set = true;
}

bool DesktopInjector::Attach(int x, int y, int width, int height) {
  InjState& s = g_inj[this];
  ++s.attach_calls;
  Rect4 r{x, y, width, height};
  g_attach_rects.push_back(r);
  s.att_x = x; s.att_y = y; s.att_w = width; s.att_h = height;
  const bool host_ok = s.shared_set && s.shared.host != nullptr;
  const bool base_ok = g_attach_ok && host_ok;
  const PlacementRect mon_pr{x, y, x + width, y + height};
  const PlacementRect host_win = s.shared.client_rect;
  const PlacementRect host_cli = s.shared.client_rect;
  const ClientOffset placed = HostClientOffset(host_win, mon_pr);
  const PlacementRect child_in_client =
      ChildRectInClient(placed, mon_pr, host_cli);
  const PlacementRect mon_as_client{mon_pr.left - host_cli.left,
                                    mon_pr.top - host_cli.top,
                                    mon_pr.right - host_cli.left,
                                    mon_pr.bottom - host_cli.top};
  const CoverageVerdict verdict = CoversMonitor(child_in_client, mon_as_client);
  const char* reason = CoverageReason(verdict);
  s.last_cov = reason ? reason : "";
  const bool ok = base_ok && verdict == CoverageVerdict::kCovered;
  s.hwnd = ok ? reinterpret_cast<void*>(static_cast<uintptr_t>(0x1000 + s.attach_calls))
              : nullptr;
  if (s.log) {
    char final_buf[64];
    std::snprintf(final_buf, sizeof(final_buf), "(%d,%d,%d,%d)", mon_pr.left,
                  mon_pr.top, mon_pr.right, mon_pr.bottom);
    const char* branch =
        (s.shared.branch != nullptr && s.shared.branch[0] != '\0')
            ? s.shared.branch
            : "shared host";
    s.log("placement: branch=%s ok=%d reason=%s host=%ls(0x%p) "
          "host_win=(%d,%d,%d,%d) host_client=(%d,%d,%d,%d) "
          "monitor=(%d,%d,%d,%d) delta=(%d,%d) child_final=%s",
          branch, ok ? 1 : 0, s.last_cov.c_str(), L"WorkerW", s.shared.host,
          host_win.left, host_win.top, host_win.right, host_win.bottom,
          host_cli.left, host_cli.top, host_cli.right, host_cli.bottom, x, y,
          width, height, placed.dx, placed.dy, final_buf);
  }
  return ok;
}

void DesktopInjector::Detach() { g_inj[this].hwnd = nullptr; }

void DesktopInjector::OnDisplayChange(int, int, int, int) {}

void* DesktopInjector::injected_hwnd() const {
  const auto it = g_inj.find(this);
  return it == g_inj.end() ? nullptr : it->second.hwnd;
}

// Bug B: the deferred teardown releases the surface instead of destroying it
// under a live mpv VO. The stub hands the handle over and forgets it.
void* DesktopInjector::ReleaseSurface() {
  const auto it = g_inj.find(this);
  if (it == g_inj.end()) return nullptr;
  void* hwnd = it->second.hwnd;
  it->second.hwnd = nullptr;
  return hwnd;
}

bool DesktopInjector::IsSurfaceAlive() const {
  return injected_hwnd() != nullptr;
}

void DesktopInjector::ReassertFrameless() {}

std::string DesktopInjector::last_coverage_reason() const {
  const auto it = g_inj.find(this);
  return it == g_inj.end() ? std::string{} : it->second.last_cov;
}

void DesktopInjector::SetSharedHost(const SharedHost& host) {
  InjState& s = g_inj[this];
  s.shared = host;
  s.shared_set = true;
}

SharedHost ResolveSharedHost(InjectMode mode, LogFn log) {
  ++g_resolve_calls;
  g_resolve_mode = mode;
  if (log != nullptr) {
    log("placement: host-resolution branch=%s host=%ls(0x%p) "
        "insert_after=0x%p layered=%s client=(%d,%d,%d,%d)",
        g_resolve_result.branch, L"WorkerW", g_resolve_result.host,
        g_resolve_result.insert_after, g_resolve_result.layered ? "YES" : "NO",
        g_resolve_result.client_rect.left, g_resolve_result.client_rect.top,
        g_resolve_result.client_rect.right, g_resolve_result.client_rect.bottom);
  }
  return g_resolve_result;
}

MpvRenderer::MpvRenderer() {
  g_ren[this] = RenState();
  ++g_ren_ctors;
}

MpvRenderer::~MpvRenderer() {
  // Bug B: the renderer teardown runs on the RENDERER GRAVEYARD worker, so
  // this destructor must not mutate the shared recording map (it would race
  // with the test thread reading it).
  ++g_ren_dtors;
}

bool MpvRenderer::Create(void* hwnd) {
  RenState& s = g_ren[this];
  s.created = true;
  s.create_hwnd = hwnd;
  return true;
}

void MpvRenderer::SetHWND(void* hwnd) { g_ren[this].create_hwnd = hwnd; }

// Bug A: the live-frame capture (mpv screenshot-to-file). The stub refuses
// the capture, so the lock-screen path falls back to the file-based extract.
bool MpvRenderer::ScreenshotToFile(const std::string& /*path*/) { return false; }
bool MpvRenderer::SeekTo(double /*seconds*/) { return false; }
bool MpvRenderer::GetInt64Property(const std::string& /*name*/, long long& /*out*/) const { return false; }
bool MpvRenderer::GetPropertyValue(const std::string& /*name*/, std::string& /*out*/) const { return false; }
bool MpvRenderer::DumpPropertyList(const std::wstring& /*path*/) const { return false; }
bool MpvRenderer::UnloadVideo() { return false; }
bool MpvRenderer::ReloadVideo() { return false; }void MpvRenderer::SetAdapterPin(const std::string& substr) {
  g_ren[this].adapter_pin = substr;
}

bool MpvRenderer::pin_active() const {
  const auto it = g_ren.find(this);
  return it != g_ren.end() && !it->second.adapter_pin.empty();
}

bool MpvRenderer::LoadLoop(const std::string&, bool) { return true; }

void MpvRenderer::SetFitMode(const std::string&, double) {}
void MpvRenderer::Pause() {}
void MpvRenderer::Resume() {}
void MpvRenderer::SetFpsCap(int) {}
void MpvRenderer::Wakeup() {}
void MpvRenderer::SetMessageWindow(void*) {}
void MpvRenderer::OnHwdecPropertyChange() {}

std::vector<MonitorInfo> ListMonitors() noexcept { return g_monitors; }

MonitorInfo GetPrimaryMonitor() noexcept {
  return g_monitors.empty() ? MonitorInfo{} : g_monitors.front();
}

// ---- shadow tests ----

void TestSnapshotTopologyMapping() {
  ResetStubs();
  g_monitors = {
      MakeMon(0, 0, 0, 1920, 1080, true, L"\\\\.\\DISPLAY1", "ccd:path1"),
      MakeMon(1, 1920, 0, 1920, 1080, false, L"\\\\.\\DISPLAY2", "edid:DEL:4098:1a2b"),
  };

  TopologySnapshot snap = SnapshotTopology();

  Check(snap.monitors.size() == 2, "shadow-snap: 2 monitors in snapshot");
  // Primary first, then stable_key order
  Check(snap.monitors[0].is_primary == true, "shadow-snap: monitor[0] is primary");
  Check(snap.monitors[0].stable_key == "ccd:path1",
        "shadow-snap: monitor[0] stable_key=ccd:path1");
  Check(snap.monitors[1].is_primary == false, "shadow-snap: monitor[1] not primary");
  Check(snap.monitors[1].stable_key == "edid:DEL:4098:1a2b",
        "shadow-snap: monitor[1] stable_key=edid:DEL:4098:1a2b");
  // gdi_name is UTF-8 of device_name
  Check(snap.monitors[0].gdi_name == "\\\\.\\DISPLAY1",
        "shadow-snap: monitor[0] gdi_name");
  Check(snap.monitors[1].gdi_name == "\\\\.\\DISPLAY2",
        "shadow-snap: monitor[1] gdi_name");
  // Geometry copied
  Check(snap.monitors[0].x == 0 && snap.monitors[0].y == 0 &&
            snap.monitors[0].width == 1920 && snap.monitors[0].height == 1080,
        "shadow-snap: monitor[0] geometry");
  Check(snap.monitors[1].x == 1920 && snap.monitors[1].y == 0 &&
            snap.monitors[1].width == 1920 && snap.monitors[1].height == 1080,
        "shadow-snap: monitor[1] geometry");
  // scale_pct/orientation/is_primary
  Check(snap.monitors[0].scale_pct == 100, "shadow-snap: scale_pct 100");
  Check(snap.monitors[0].orientation == 0, "shadow-snap: orientation 0");
  Check(snap.monitors[0].is_primary == true, "shadow-snap: is_primary true");
  // virtual screen from GetSpanGeometry
  Check(snap.virtual_width > 0 && snap.virtual_height > 0,
        "shadow-snap: virtual screen positive size");
  // hash_snapshot non-empty
  Check(!snap.hash_snapshot.empty(), "shadow-snap: hash_snapshot non-empty");
  Check(snap.hash_snapshot.size() == 16,
        "shadow-snap: hash_snapshot 16 hex chars");
}

void TestSnapshotTopologyHashDeterminism() {
  ResetStubs();
  g_monitors = {
      MakeMon(0, 0, 0, 1920, 1080, true, L"\\\\.\\DISPLAY1", "key-a"),
      MakeMon(1, 1920, 0, 1920, 1080, false, L"\\\\.\\DISPLAY2", "key-b"),
  };
  TopologySnapshot snap1 = SnapshotTopology();
  TopologySnapshot snap2 = SnapshotTopology();
  Check(snap1.hash_snapshot == snap2.hash_snapshot,
        "shadow-hash: deterministic across calls");

  // Same monitors, different order in ListMonitors -> same sorted result
  g_monitors = {
      MakeMon(1, 1920, 0, 1920, 1080, false, L"\\\\.\\DISPLAY2", "key-b"),
      MakeMon(0, 0, 0, 1920, 1080, true, L"\\\\.\\DISPLAY1", "key-a"),
  };
  TopologySnapshot snap3 = SnapshotTopology();
  Check(snap1.hash_snapshot == snap3.hash_snapshot,
        "shadow-hash: order-independent (sorted)");
}

void TestSnapshotTopologyDegraded() {
  ResetStubs();
  g_monitors = {
      MakeMon(0, 0, 0, 1920, 1080, true, L"\\\\.\\DISPLAY1", "gdi:\\\\.\\DISPLAY1", true),
  };
  TopologySnapshot snap = SnapshotTopology();
  Check(snap.monitors[0].degraded == true, "shadow-degraded: degraded flag set");
}

void TestDiffTopologyIntegration() {
  TopologySnapshot old_s, new_s;
  // old: one monitor
  TopologyMonitor m1;
  m1.stable_key = "key-a"; m1.gdi_name = "\\\\.\\DISPLAY1";
  m1.x = 0; m1.y = 0; m1.width = 1920; m1.height = 1080;
  m1.scale_pct = 100; m1.orientation = 0; m1.is_primary = true;
  old_s.monitors.push_back(m1);
  old_s.hash_snapshot = "oldhash";

  // new: two monitors, key-a moved, key-b added
  TopologyMonitor m1m = m1;
  m1m.x = 100; m1m.y = 50;  // moved
  TopologyMonitor m2;
  m2.stable_key = "key-b"; m2.gdi_name = "\\\\.\\DISPLAY2";
  m2.x = 1920; m2.y = 0; m2.width = 1920; m2.height = 1080;
  m2.scale_pct = 125; m2.orientation = 0; m2.is_primary = false;
  new_s.monitors.push_back(m1m);
  new_s.monitors.push_back(m2);
  new_s.hash_snapshot = "newhash";

  TopologyDiff d = diff_topology(old_s, new_s);
  Check(d.added_keys.size() == 1 && d.added_keys[0] == "key-b",
        "shadow-diff: key-b added");
  Check(d.removed_keys.empty(), "shadow-diff: no removed");
  Check(d.moved_keys.size() == 1 && d.moved_keys[0] == "key-a",
        "shadow-diff: key-a moved");
  Check(d.resized_keys.empty(), "shadow-diff: no resized");
  Check(d.dpi_keys.empty(), "shadow-diff: no dpi (different scale)");
  Check(d.orientation_keys.empty(), "shadow-diff: no orientation change");
  Check(d.primary_changed == false, "shadow-diff: primary unchanged");
  Check(d.hash_changed == true, "shadow-diff: hash changed");
  Check(d.empty() == false, "shadow-diff: not empty");
}

void TestDiffTopologyEmpty() {
  TopologySnapshot s1, s2;
  s1.hash_snapshot = "abc";
  s2.hash_snapshot = "abc";
  TopologyDiff d = diff_topology(s1, s2);
  Check(d.empty() == true, "shadow-diff-empty: identical snapshots -> empty");
  Check(d.hash_changed == false, "shadow-diff-empty: hash unchanged");
}

void TestDivergesFromMonitorsYes() {
  ResetStubs();
  g_monitors = {
      MakeMon(0, 0, 0, 1920, 1080, true, L"\\\\.\\DISPLAY1", "key-a"),
  };
  TopologySnapshot snap = SnapshotTopology();
  // Modify geometry so it diverges from live
  snap.monitors[0].x = 9999;
  auto live = std::vector<MonitorInfo>{g_monitors[0]};
  Check(TopologyDivergesFromMonitors(snap, live) == true,
        "shadow-diverges: geometry changed -> divergent=yes");
}

void TestDivergesFromMonitorsNo() {
  ResetStubs();
  g_monitors = {
      MakeMon(0, 0, 0, 1920, 1080, true, L"\\\\.\\DISPLAY1", "key-a"),
  };
  TopologySnapshot snap = SnapshotTopology();
  auto live = std::vector<MonitorInfo>{g_monitors[0]};
  Check(TopologyDivergesFromMonitors(snap, live) == false,
        "shadow-diverges: identical -> divergent=no");
}

void TestVerifyCoverage() {
  ResetStubs();
  g_monitors = {
      MakeMon(0, 0, 0, 1920, 1080, true, L"\\\\.\\DISPLAY1", "key-a", false),
      MakeMon(1, 1920, 0, 1920, 1080, false, L"\\\\.\\DISPLAY2", "key-b", true),
  };
  MultiMonitor mm(&QuietLog, MakeRecordingFactory());
  mm.Init(MultiMonitorMode::PerMonitor);

  // VerifyCoverage read-only: counts live slots, covered if coverage_reason
  // == "placement: covered" (set during Attach via the stub), degraded keys
  // from identity_degraded flag.
  MultiMonitor::CoverageCensus c = mm.VerifyCoverage();
  Check(c.total == 2, "shadow-census: total==2");
  Check(c.degraded_keys.size() == 1, "shadow-census: 1 degraded key");
  Check(c.degraded_keys[0] == "key-b", "shadow-census: degraded key is key-b");

  mm.Shutdown();
}

void TestApplyTopologyShadowNoOp() {
  ResetStubs();
  g_monitors = {
      MakeMon(0, 0, 0, 1920, 1080, true, L"\\\\.\\DISPLAY1", "key-a"),
  };
  MultiMonitor mm(&QuietLog, MakeRecordingFactory());
  mm.Init(MultiMonitorMode::PerMonitor);
  size_t slots_before = mm.slot_count();

  TopologySnapshot snap = SnapshotTopology();
  TopologyDiff empty_diff;
  MultiMonitor::TopologyApplyResult r = mm.ApplyTopology(snap, empty_diff);
  Check(r.added == 0 && r.removed == 0 && r.reattached == 0,
        "shadow-apply: zero counts (no-op)");
  Check(mm.slot_count() == slots_before, "shadow-apply: slot_count unchanged");

  mm.Shutdown();
}

void TestSharedHostBranch() {
  ResetStubs();
  g_monitors = {
      MakeMon(0, 0, 0, 1920, 1080, true, L"\\\\.\\DISPLAY1", "key-a"),
  };
  MultiMonitor mm(&QuietLog, MakeRecordingFactory());
  // Before Init: shared_host_ is default (zero rect) -> "(none)"
  Check(mm.SharedHostBranch() == "(none)",
        "shadow-branch: default-constructed -> (none)");
  mm.Init(MultiMonitorMode::PerMonitor);
  // After Init: host resolved via ResolveHostForPass -> branch string
  Check(mm.SharedHostBranch() != "(none)",
        "shadow-branch: after Init -> non-empty branch");
  mm.Shutdown();
}

}  // namespace k6wp

int main(int argc, char** argv) {
  const std::string sel = argc > 1 ? argv[1] : "all";
  auto run = [&](const char* name, void (*fn)()) {
    if (sel == "all" || sel == name) fn();
  };
  run("snapshot-mapping", k6wp::TestSnapshotTopologyMapping);
  run("snapshot-hash", k6wp::TestSnapshotTopologyHashDeterminism);
  run("snapshot-degraded", k6wp::TestSnapshotTopologyDegraded);
  run("diff-integration", k6wp::TestDiffTopologyIntegration);
  run("diff-empty", k6wp::TestDiffTopologyEmpty);
  run("diverges-yes", k6wp::TestDivergesFromMonitorsYes);
  run("diverges-no", k6wp::TestDivergesFromMonitorsNo);
  run("coverage", k6wp::TestVerifyCoverage);
  run("apply-noop", k6wp::TestApplyTopologyShadowNoOp);
  run("shared-branch", k6wp::TestSharedHostBranch);
  std::cout << k6wp::g_checks << " checks, " << k6wp::g_failures
            << " failures\n";
  return k6wp::g_failures == 0 ? 0 : 1;
}
