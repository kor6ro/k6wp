// reconcile_golden_test.cpp - Engine Display Resilience, P0/Fase 4.
//
// Characterization GOLDEN tests for the CURRENT (legacy) MultiMonitor
// placement/reconcile behavior via the `placement:` log surface. These tests
// must stay GREEN on today's code: they lock the observable behaviour the
// upcoming Reconcile pipeline must preserve (prompt v5.1 §5.1 step 1, plan 03
// P0). Three scenarios: Win11 24H2 (WorkerW child of Progman / layered Progman
// child), classic layout (Strategy A/B WorkerW) and Span mode.
//
// LINK MODEL (inherited from tests/multi_monitor_factory_test.cpp): only
// engine/src/multi_monitor.cpp + src/desktop_placement.cpp are compiled from
// the engine. DesktopInjector / MpvRenderer / ListMonitors / GetPrimaryMonitor
// / ResolveSharedHost are recording stubs in this TU, so the binary never
// links desktop_inject.cpp, mpv_renderer.cpp or mpv.lib. The stubs are not
// silent: ResolveSharedHost re-emits the production `placement: host-resolution`
// line; Attach re-emits the production `placement: branch=...` line through the
// REAL CoversMonitor / CoverageReason helpers (desktop_placement.cpp), which is
// exactly the per-attempt placement decision LogPlacement makes in
// desktop_inject.cpp. `WorkerWSpansRect` (row 10 strategy-B span gate) is also
// the real helper.
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

#include "desktop_placement.hpp"  // real CoversMonitor / CoverageReason / WorkerWSpansRect

namespace k6wp {

// Complete the Pimpl so the stubbed ~DesktopInjector can destroy its
// unique_ptr<Impl> member (desktop_inject.cpp is NOT linked).
struct DesktopInjector::Impl {};

namespace {

// ---- recording state (link-level stubs read this) -------------------------

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
                    const std::wstring& device) {
  MonitorInfo m;
  m.id = id;
  m.x = x;
  m.y = y;
  m.width = w;
  m.height = h;
  m.is_primary = primary;
  m.device_name = device;
  return m;
}

// Resolved-host fixture. `branch` mirrors the production branch labels
// ("24H2 path" / "classic path" / "forced progman" / "forced workerw").
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

void* NextStubHwnd() {
  static int n = 0;
  ++n;
  return reinterpret_cast<void*>(static_cast<uintptr_t>(0x1000 + n));
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

}  // namespace (recording state + harness)

// ---- link-level stubs: DesktopInjector (desktop_inject.cpp NOT linked) ----

void DesktopInjector::HideVoChildren() {}

DesktopInjector::DesktopInjector(LogFn log) {
  g_inj[this].log = log;
  ++g_inj_ctors;
}

DesktopInjector::~DesktopInjector() {
  ++g_inj_dtors;
  g_inj.erase(this);
}

void DesktopInjector::SetInjectMode(InjectMode mode) {
  InjState& s = g_inj[this];
  s.mode = mode;
  s.mode_set = true;
}

// Mirrors DesktopInjector::AttachToDesktop + LogPlacement (desktop_inject.cpp)
// over the pass's resolved shared host: child placed from the host WINDOW
// origin, monitor mapped from the host CLIENT origin, verdict via the REAL
// CoversMonitor. Emits the production-shaped `placement: branch=...` line so
// the golden test locks the +verbatim log surface.
bool DesktopInjector::Attach(int x, int y, int width, int height) {
  InjState& s = g_inj[this];
  ++s.attach_calls;
  Rect4 r;
  r.x = x;
  r.y = y;
  r.w = width;
  r.h = height;
  g_attach_rects.push_back(r);
  s.att_x = x;
  s.att_y = y;
  s.att_w = width;
  s.att_h = height;

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
  s.hwnd = ok ? NextStubHwnd() : nullptr;

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

void DesktopInjector::OnDisplayChange(int /*x*/, int /*y*/, int /*width*/,
                                      int /*height*/) {}

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

// The free ResolveSharedHost stub: counts calls (the "one resolution per
// attach pass" contract) and re-emits the production-shaped
// `placement: host-resolution` line.
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

// ---- link-level stubs: MpvRenderer (mpv_renderer.cpp/mpv.lib NOT linked) --

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

bool MpvRenderer::LoadLoop(const std::string& /*path*/, bool /*force*/) {
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

// ---- link-level stubs: monitor enumeration (monitor_util.cpp NOT pulled) --

std::vector<MonitorInfo> ListMonitors() noexcept { return g_monitors; }

MonitorInfo GetPrimaryMonitor() noexcept {
  return g_monitors.empty() ? MonitorInfo{} : g_monitors.front();
}

// ---------------------------------------------------------------------------
// Golden 1: Win11 24H2 (WorkerW child of Progman / layered). Two monitors side
// by side; the resolved host spans both; every slot is covered, no headless.
// ---------------------------------------------------------------------------
void TestGolden24H2() {
  ResetStubs();
  g_monitors = {
      MakeMon(0, 0, 0, 1920, 1080, true, L"\\\\.\\DISPLAY1"),
      MakeMon(1, 1920, 0, 1920, 1080, false, L"\\\\.\\DISPLAY2"),
  };
  g_resolve_result = MakeHost("24H2 path", true, 0, 0, 3840, 1080);

  MultiMonitor mm(&RecordLog, MakeRecordingFactory());
  mm.SetHeadlessHost(kHeadlessHost);
  const bool ok = mm.Init(MultiMonitorMode::PerMonitor);
  Check(ok, "golden-24h2: Init(PerMonitor) returns true");
  Check(mm.slot_count() == 2, "golden-24h2: slot_count()==2");
  Check(g_resolve_calls == 1,
        "golden-24h2: ResolveSharedHost ran exactly once for the pass");
  Check(g_resolve_mode == InjectMode::kAuto,
        "golden-24h2: default inject mode kAuto reached ResolveSharedHost");

  bool shared_ok = g_inj.size() == 2;
  for (const auto& kv : g_inj) {
    shared_ok = shared_ok && kv.second.shared_set &&
                std::string(kv.second.shared.branch) == "24H2 path" &&
                kv.second.shared.layered && kv.second.shared.host != nullptr;
  }
  Check(shared_ok,
        "golden-24h2: both injectors got the pass host (branch='24H2 path', "
        "layered)");

  Check(mm.SlotCoverageReason(0) == "placement: covered" &&
            mm.SlotCoverageReason(1) == "placement: covered",
        "golden-24h2: both slots covered (24H2 span host)");
  Check(mm.headless_slot_count() == 0,
        "golden-24h2: headless_slot_count()==0");
  Check(!mm.has_headless_slots(),
        "golden-24h2: has_headless_slots()==false");
  Check(g_attach_rects.size() == 2 && g_attach_rects[0].x == 0 &&
            g_attach_rects[0].w == 1920 && g_attach_rects[1].x == 1920 &&
            g_attach_rects[1].w == 1920,
        "golden-24h2: each slot attached at its monitor rect");

  Check(LogContains("placement: host-resolution branch=24H2 path"),
        "golden-24h2: `placement: host-resolution branch=24H2 path` emitted");
  Check(LogContains(
            "placement: branch=24H2 path ok=1 reason=placement: covered"),
        "golden-24h2: per-attach `placement:` line carries branch + covered "
        "verdict");
  mm.Shutdown();
}

// ---------------------------------------------------------------------------
// Golden 2: classic layout (Strategy A/B WorkerW). Secondary monitor at a
// NEGATIVE origin (-1920,0); the resolved host spans the union, so the
// negative-origin child is still covered (screen-space rule).
// ---------------------------------------------------------------------------
void TestGoldenClassic() {
  ResetStubs();
  g_monitors = {
      MakeMon(0, 0, 0, 1920, 1080, true, L"\\\\.\\DISPLAY1"),
      MakeMon(1, -1920, 0, 1920, 1080, false, L"\\\\.\\DISPLAY2"),
  };
  const PlacementRect host{-1920, 0, 1920, 1080};
  const PlacementRect mon_neg{-1920, 0, 0, 1080};
  Check(CoversMonitor(host, mon_neg) == CoverageVerdict::kCovered,
        "golden-classic: host union covers the negative-origin monitor");
  g_resolve_result = MakeHost("classic path", true, -1920, 0, 1920, 1080);

  MultiMonitor mm(&RecordLog, MakeRecordingFactory());
  mm.SetHeadlessHost(kHeadlessHost);
  const bool ok = mm.Init(MultiMonitorMode::PerMonitor);
  Check(ok, "golden-classic: Init(PerMonitor) returns true");
  Check(mm.slot_count() == 2, "golden-classic: slot_count()==2");
  Check(g_resolve_calls == 1,
        "golden-classic: ResolveSharedHost ran exactly once for the pass");
  bool shared_ok = g_inj.size() == 2;
  for (const auto& kv : g_inj) {
    shared_ok = shared_ok &&
                std::string(kv.second.shared.branch) == "classic path";
  }
  Check(shared_ok, "golden-classic: both injectors got branch='classic path'");
  Check(mm.SlotCoverageReason(0) == "placement: covered" &&
            mm.SlotCoverageReason(1) == "placement: covered",
        "golden-classic: both slots covered (classic span host, negative "
        "origin)");
  Check(mm.headless_slot_count() == 0,
        "golden-classic: headless_slot_count()==0");
  Check(LogContains("placement: host-resolution branch=classic path"),
        "golden-classic: `placement: host-resolution branch=classic path` "
        "emitted");
  mm.Shutdown();
}

// ---------------------------------------------------------------------------
// Golden 3: Span mode. Virtual screen forced via SetSpanGeometryOverride (the
// test seam); one slot keyed by kSpanSlotId (-1); row-10 `WorkerWSpansRect`
// confirms the resolved host spans the virtual screen.
// ---------------------------------------------------------------------------
void TestGoldenSpan() {
  ResetStubs();
  SpanGeometry fake{0, 0, 3840, 1080};
  MultiMonitor::SetSpanGeometryOverride(&fake);
  const PlacementRect virtual_rect{0, 0, 3840, 1080};
  const PlacementRect host_rect{0, 0, 3840, 1080};
  Check(WorkerWSpansRect(host_rect, virtual_rect),
        "golden-span: WorkerWSpansRect(host covering virtual screen)==true");
  g_resolve_result = MakeHost("24H2 path", true, 0, 0, 3840, 1080);

  MultiMonitor mm(&RecordLog, MakeRecordingFactory());
  mm.SetHeadlessHost(kHeadlessHost);
  const bool ok = mm.Init(MultiMonitorMode::Span);
  Check(ok, "golden-span: Init(Span) returns true");
  Check(mm.slot_count() == 1, "golden-span: slot_count()==1");
  Check(mm.mode() == MultiMonitorMode::Span, "golden-span: mode()==Span");
  Check(mm.monitor_ids() == std::vector<int>{-1},
        "golden-span: span slot keyed by kSpanSlotId(-1)");
  Check(g_resolve_calls == 1,
        "golden-span: ResolveSharedHost ran exactly once for the span pass");
  Check(g_attach_rects.size() == 1 && g_attach_rects[0].x == 0 &&
            g_attach_rects[0].y == 0 && g_attach_rects[0].w == 3840 &&
            g_attach_rects[0].h == 1080,
        "golden-span: single attach covering the virtual screen");
  Check(mm.SlotCoverageReason(-1) == "placement: covered",
        "golden-span: span slot covered");
  Check(mm.headless_slot_count() == 0, "golden-span: headless_slot_count()==0");
  Check(LogContains("placement: host-resolution branch=24H2 path"),
        "golden-span: `placement: host-resolution` emitted for the span pass");
  MultiMonitor::SetSpanGeometryOverride(nullptr);
  mm.Shutdown();
}

}  // namespace k6wp

int main(int argc, char** argv) {
  const std::string sel = argc > 1 ? argv[1] : "all";
  if (sel == "all" || sel == "golden-24h2") {
    k6wp::TestGolden24H2();
  }
  if (sel == "all" || sel == "golden-classic") {
    k6wp::TestGoldenClassic();
  }
  if (sel == "all" || sel == "golden-span") {
    k6wp::TestGoldenSpan();
  }
  std::cout << k6wp::g_checks << " checks, " << k6wp::g_failures
            << " failures\n";
  return k6wp::g_failures == 0 ? 0 : 1;
}
