// Unit tests for the multi-monitor placement matrix (plan row 12, GAP-11 /
// IS-1 / IS-6 / IS-7). No external test framework: plain Check() counter,
// same style as tests/occlusion_test.cpp / tests/config_test.cpp. Exit 0 =
// pass.
//
// LINK MODEL (inherited from tests/multi_monitor_factory_test.cpp, rows
// 6/7/9/11): only engine/src/multi_monitor.cpp + src/desktop_placement.cpp
// are compiled from the engine. Every DesktopInjector / MpvRenderer /
// ListMonitors / ResolveSharedHost symbol multi_monitor.cpp references is
// DEFINED HERE as a recording stub, so the binary never links
// desktop_inject.cpp, mpv_renderer.cpp or mpv.lib - a headless
// MultiMonitor. k6wp_shared is linked ONLY for DetectKeyCollision (row 5,
// shared/displays_schema.hpp); the TU still owns ListMonitors /
// GetPrimaryMonitor, so monitor_util.obj is never pulled and no duplicate
// symbol arises. target_link_libraries must never grow an mpv entry.
//
// Coverage (the eight fixture scenarios the plan row enumerates):
//   (a) one monitor (0,0,1920,1080)
//   (b) two side by side (0,0)+(1920,0) - happy QA evidence
//   (c) second monitor LEFT of primary (-1920,0) - negative virtual coords,
//       the case packaging/known-limitations.md:31-35 marks unproven; host
//       covering only (0,0,1920,1080) -> stub reports false, headless
//       degradation, NOT a silent success
//   (d) second monitor above primary (0,-1080)
//   (e) portrait secondary (rect 1920,0,3000,1920 -> 1080x1920)
//   (f) three monitors mixed sizes
//   (g) duplicate mode - two monitors reporting the IDENTICAL rect
//       (0,0,1920,1080): DetectKeyCollision (row 5) fires, both slots still
//       attach
//   (h) mixed-DPI - same rect, two different scale_pct values: MonitorInfo
//       carries both, placement (physical px) unchanged; plus a companion
//       distinct-rect case proving the attach rect is physical px
//
// Per case: slot_count, each stub's received rect, the CoversMonitor verdict
// (direct geometry + the stub-produced SlotCoverageReason token), and
// headless_slot_count().
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "multi_monitor.hpp"

#include "desktop_placement.hpp"   // row 3: real CoversMonitor/CoverageReason
#include "displays_schema.hpp"     // row 5: DetectKeyCollision (k6wp_shared)

namespace k6wp {

// Complete the Pimpl so the stubbed ~DesktopInjector can destroy its
// unique_ptr<Impl> member. desktop_inject.cpp (the real Impl) is NOT linked,
// so this is the only definition in the binary - no ODR conflict.
struct DesktopInjector::Impl {};

namespace {

// ---- recording state (link-level stubs read this; same shape as row 6) ----

struct InjState {
  LogFn log = nullptr;
  InjectMode mode = InjectMode::kAuto;
  bool mode_set = false;
  bool shared_set = false;
  SharedHost shared{};
  void* hwnd = nullptr;
  int attach_calls = 0;
  int detach_calls = 0;
  int reassert_calls = 0;
  int on_display_change_calls = 0;
  int att_x = 0, att_y = 0, att_w = 0, att_h = 0;
  int odc_x = 0, odc_y = 0, odc_w = 0, odc_h = 0;
  std::string last_cov;
};

struct RenState {
  bool created = false;
  void* create_hwnd = nullptr;
  bool create_ok = true;
  std::string adapter_pin;
  int set_hwnd_calls = 0;
  int pause_calls = 0;
  int resume_calls = 0;
  int load_loop_calls = 0;
  int set_fit_calls = 0;
};

struct Rect4 {
  int x = 0, y = 0, w = 0, h = 0;
};

std::map<const DesktopInjector*, InjState> g_inj;
std::map<const MpvRenderer*, RenState> g_ren;
std::vector<std::string> g_events;
std::vector<Rect4> g_attach_rects;
std::vector<void*> g_create_hwnds;
std::vector<MonitorInfo> g_monitors;
bool g_attach_ok = true;
int g_inj_ctors = 0, g_inj_dtors = 0;
int g_ren_ctors = 0, g_ren_dtors = 0;
int g_factory_injectors = 0, g_factory_renderers = 0;
SharedHost g_resolve_result{};
int g_resolve_calls = 0;
InjectMode g_resolve_mode = InjectMode::kAuto;
std::vector<std::string> g_logs;
std::vector<PlacementRect> g_reported_child_queue;

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

// Monitor fixture builder - fidelity with shared/monitor_util.cpp:24-55
// (x/y = rcMonitor origin, width/height = current resolution in PHYSICAL
// pixels, orientation from DEVMODEW.dmDisplayOrientation, scale_pct from
// DEVICE_SCALE_FACTOR percent domain).
MonitorInfo MakeMon(int id, int x, int y, int w, int h, bool primary,
                    const std::wstring& device, int scale_pct = 100,
                    int orientation = 0) {
  MonitorInfo m;
  m.id = id;
  m.x = x;
  m.y = y;
  m.width = w;
  m.height = h;
  m.is_primary = primary;
  m.device_name = device;
  m.scale_pct = scale_pct;
  m.orientation = orientation;
  return m;
}

// Resolved-host fixture: non-null handles so AttachSlot's shared-host path
// runs, with an explicit SCREEN-space client_rect for the coverage cases.
SharedHost MakeHost(int left, int top, int right, int bottom) {
  SharedHost sh;
  sh.host = reinterpret_cast<void*>(static_cast<uintptr_t>(0xBEEF));
  sh.progman = reinterpret_cast<void*>(static_cast<uintptr_t>(0xFEED));
  sh.def_view = reinterpret_cast<void*>(static_cast<uintptr_t>(0xDEA1));
  sh.insert_after = sh.def_view;
  sh.layered = true;
  sh.branch = "placement-matrix host";
  sh.client_rect.left = left;
  sh.client_rect.top = top;
  sh.client_rect.right = right;
  sh.client_rect.bottom = bottom;
  return sh;
}

void ResetStubs() {
  g_inj.clear();
  g_ren.clear();
  g_events.clear();
  g_attach_rects.clear();
  g_create_hwnds.clear();
  g_logs.clear();
  g_reported_child_queue.clear();
  g_attach_ok = true;
  g_inj_ctors = g_inj_dtors = 0;
  g_ren_ctors = g_ren_dtors = 0;
  g_factory_injectors = g_factory_renderers = 0;
  g_resolve_result = MakeHost(0, 0, 1920, 1080);
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

// Find the stub injector whose Attach received the given monitor rect.
const InjState* FindInj(int x, int y, int w, int h) {
  for (const auto& kv : g_inj) {
    if (kv.second.att_x == x && kv.second.att_y == y &&
        kv.second.att_w == w && kv.second.att_h == h) {
      return &kv.second;
    }
  }
  return nullptr;
}

// True when every Attach rect in the call order equals `want` (rect i of
// the monitor fixture).
bool AttachRectsEqual(const std::vector<Rect4>& want) {
  if (g_attach_rects.size() != want.size()) return false;
  for (size_t i = 0; i < want.size(); ++i) {
    if (g_attach_rects[i].x != want[i].x || g_attach_rects[i].y != want[i].y ||
        g_attach_rects[i].w != want[i].w || g_attach_rects[i].h != want[i].h) {
      return false;
    }
  }
  return true;
}

}  // namespace (recording state + harness)

// ---- link-level stubs: DesktopInjector (desktop_inject.cpp NOT linked) ----
// Member definitions live at namespace scope (C2888: never inside the
// anonymous namespace) but read the recording state above.

DesktopInjector::DesktopInjector(LogFn log) {
  g_inj[this].log = log;
  ++g_inj_ctors;
  g_events.push_back("inj-ctor");
}

DesktopInjector::~DesktopInjector() {
  ++g_inj_dtors;
  g_events.push_back("inj-dtor");
  g_inj.erase(this);
}

void DesktopInjector::SetInjectMode(InjectMode mode) {
  InjState& s = g_inj[this];
  s.mode = mode;
  s.mode_set = true;
  g_events.push_back("setmode");
}

bool DesktopInjector::Attach(int x, int y, int width, int height) {
  InjState& s = g_inj[this];
  ++s.attach_calls;
  const bool host_ok = s.shared_set && s.shared.host != nullptr;
  const bool base_ok = g_attach_ok && host_ok;
  Rect4 r;
  r.x = x;
  r.y = y;
  r.w = width;
  r.h = height;
  g_attach_rects.push_back(r);
  s.att_x = x; s.att_y = y; s.att_w = width; s.att_h = height;
  g_events.push_back("attach");
  // Row 9 production contract mirrored here: when the fixture supplies a
  // "reported child rect" for this Attach call, compare it against the
  // requested monitor rect in SCREEN space via the REAL CoversMonitor.
  // Not covered -> honest false (headless census), never a silent success.
  const size_t call_idx = g_attach_rects.size() - 1;
  if (base_ok && call_idx < g_reported_child_queue.size()) {
    const PlacementRect child_screen = g_reported_child_queue[call_idx];
    const PlacementRect mon_screen{x, y, x + width, y + height};
    const CoverageVerdict v = CoversMonitor(child_screen, mon_screen);
    const char* reason = CoverageReason(v);
    s.last_cov = reason ? reason : "";
    if (v != CoverageVerdict::kCovered) {
      s.hwnd = nullptr;
      if (s.log) {
        s.log("placement: RETRY-FALSE-SUCCESS reason=%s "
              "child=(%d,%d,%d,%d) monitor=(%d,%d,%d,%d)",
              s.last_cov.c_str(), child_screen.left, child_screen.top,
              child_screen.right, child_screen.bottom, x, y, x + width,
              y + height);
      }
      return false;
    }
  }
  s.hwnd = base_ok ? NextStubHwnd() : nullptr;
  return base_ok;
}

void DesktopInjector::Detach() {
  InjState& s = g_inj[this];
  s.hwnd = nullptr;
  ++s.detach_calls;
  g_events.push_back("detach");
}

void DesktopInjector::OnDisplayChange(int x, int y, int width, int height) {
  InjState& s = g_inj[this];
  ++s.on_display_change_calls;
  s.odc_x = x; s.odc_y = y; s.odc_w = width; s.odc_h = height;
  g_events.push_back("on-display-change");
}

void* DesktopInjector::injected_hwnd() const {
  const auto it = g_inj.find(this);
  return it == g_inj.end() ? nullptr : it->second.hwnd;
}

void DesktopInjector::ReassertFrameless() {
  ++g_inj[this].reassert_calls;
  g_events.push_back("reassert");
}

std::string DesktopInjector::last_coverage_reason() const {
  const auto it = g_inj.find(this);
  return it == g_inj.end() ? std::string{} : it->second.last_cov;
}

void DesktopInjector::SetSharedHost(const SharedHost& host) {
  InjState& s = g_inj[this];
  s.shared = host;
  s.shared_set = true;
}

SharedHost ResolveSharedHost(InjectMode mode, LogFn /*log*/) {
  ++g_resolve_calls;
  g_resolve_mode = mode;
  return g_resolve_result;
}

// ---- link-level stubs: MpvRenderer (mpv_renderer.cpp/mpv.lib NOT linked) --

MpvRenderer::MpvRenderer() {
  g_ren[this] = RenState();
  ++g_ren_ctors;
  g_events.push_back("ren-ctor");
}

MpvRenderer::~MpvRenderer() {
  ++g_ren_dtors;
  g_events.push_back("ren-dtor");
  g_ren.erase(this);
}

bool MpvRenderer::Create(void* hwnd) {
  RenState& s = g_ren[this];
  s.created = true;
  s.create_hwnd = hwnd;
  g_create_hwnds.push_back(hwnd);
  g_events.push_back("create");
  return s.create_ok;
}

void MpvRenderer::SetHWND(void* hwnd) {
  RenState& s = g_ren[this];
  s.create_hwnd = hwnd;
  ++s.set_hwnd_calls;
  g_events.push_back("set-hwnd");
}

void MpvRenderer::SetAdapterPin(const std::string& substr) {
  g_ren[this].adapter_pin = substr;
  g_events.push_back("setpin");
}

bool MpvRenderer::pin_active() const {
  const auto it = g_ren.find(this);
  return it != g_ren.end() && !it->second.adapter_pin.empty();
}

bool MpvRenderer::LoadLoop(const std::string& /*path*/, bool /*force*/) {
  ++g_ren[this].load_loop_calls;
  g_events.push_back("load-loop");
  return true;
}

void MpvRenderer::SetFitMode(const std::string& /*fit_mode*/,
                             double /*window_aspect*/) {
  ++g_ren[this].set_fit_calls;
}

void MpvRenderer::Pause() {
  ++g_ren[this].pause_calls;
  g_events.push_back("pause");
}

void MpvRenderer::Resume() {
  ++g_ren[this].resume_calls;
  g_events.push_back("resume");
}

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
// Scenario (a): one monitor (0,0,1920,1080). Happy baseline - the exact
// inject rect known-limitations.md:26 records as live-proven on the single
// 1920x1080 test machine.
// ---------------------------------------------------------------------------
void TestA_OneMonitorPrimary() {
  ResetStubs();
  g_monitors = {MakeMon(0, 0, 0, 1920, 1080, true, L"\\\\.\\DISPLAY1", 100)};
  g_resolve_result = MakeHost(0, 0, 1920, 1080);
  g_reported_child_queue = {PlacementRect{0, 0, 1920, 1080}};

  // Geometry rule, locked directly against the row-3 helper.
  const PlacementRect mon{0, 0, 1920, 1080};
  Check(CoversMonitor(mon, mon) == CoverageVerdict::kCovered,
        "a: CoversMonitor(monitor, monitor) == kCovered (screen space)");
  Check(std::string(CoverageReason(CoversMonitor(mon, mon))) ==
            "placement: covered",
        "a: kCovered reason token is 'placement: covered'");

  MultiMonitor mm(&QuietLog, MakeRecordingFactory());
  mm.SetHeadlessHost(kHeadlessHost);
  mm.SetInjectMode(InjectMode::kProgman);
  const bool ok = mm.Init(MultiMonitorMode::PerMonitor);
  Check(ok, "a: Init(PerMonitor) returns true with 1 monitor");
  Check(mm.slot_count() == 1, "a: slot_count()==1");
  Check(AttachRectsEqual({{0, 0, 1920, 1080}}),
        "a: stub Attach received the exact monitor rect (0,0,1920x1080)");
  Check(mm.SlotCoverageReason(0) == "placement: covered",
        "a: SlotCoverageReason(0) == 'placement: covered'");
  Check(mm.headless_slot_count() == 0, "a: headless_slot_count()==0");
  Check(!mm.has_headless_slots(), "a: has_headless_slots()==false");
  mm.Shutdown();
  Check(mm.slot_count() == 0, "a: Shutdown clears all slots");
}

// ---------------------------------------------------------------------------
// Scenario (b): two monitors side by side (0,0)+(1920,0). HAPPY QA EVIDENCE
// (plan row 12 QA scenario): shared host covering (0,0,3840,1080), both
// stubs receive their own rect, both CoversMonitor verdicts are kCovered.
// ---------------------------------------------------------------------------
void TestB_TwoSideBySideHappy() {
  ResetStubs();
  g_monitors = {
      MakeMon(0, 0, 0, 1920, 1080, true, L"\\\\.\\DISPLAY1", 100),
      MakeMon(1, 1920, 0, 1920, 1080, false, L"\\\\.\\DISPLAY2", 100),
  };
  // Shared host spans BOTH monitors in screen space.
  g_resolve_result = MakeHost(0, 0, 3840, 1080);
  g_reported_child_queue = {
      PlacementRect{0, 0, 1920, 1080},      // mon0: exact cover
      PlacementRect{1920, 0, 3840, 1080},   // mon1: exact cover
  };

  // Geometry rule: the shared host covers each monitor; each monitor rect
  // covered by its own child rect.
  const PlacementRect mon0{0, 0, 1920, 1080};
  const PlacementRect mon1{1920, 0, 3840, 1080};
  const PlacementRect host{0, 0, 3840, 1080};
  Check(CoversMonitor(host, mon0) == CoverageVerdict::kCovered,
        "b: CoversMonitor(host(0,0,3840x1080), mon0) == kCovered");
  Check(CoversMonitor(host, mon1) == CoverageVerdict::kCovered,
        "b: CoversMonitor(host(0,0,3840x1080), mon1) == kCovered");
  Check(CoversMonitor(mon0, mon0) == CoverageVerdict::kCovered &&
            CoversMonitor(mon1, mon1) == CoverageVerdict::kCovered,
        "b: each child rect covers its own monitor rect");

  MultiMonitor mm(&QuietLog, MakeRecordingFactory());
  mm.SetHeadlessHost(kHeadlessHost);
  const bool ok = mm.Init(MultiMonitorMode::PerMonitor);
  Check(ok, "b: Init(PerMonitor) returns true with 2 monitors");
  Check(mm.slot_count() == 2, "b: slot_count()==2");
  Check(AttachRectsEqual({{0, 0, 1920, 1080}, {1920, 0, 1920, 1080}}),
        "b: stubs received their own rects (0,0,1920x1080) and "
        "(1920,0,1920x1080)");
  const InjState* inj0 = FindInj(0, 0, 1920, 1080);
  const InjState* inj1 = FindInj(1920, 0, 1920, 1080);
  Check(inj0 != nullptr && inj1 != nullptr,
        "b: both injectors identified by their received rect");
  Check(inj0 != nullptr && inj0->hwnd != nullptr && inj1 != nullptr &&
            inj1->hwnd != nullptr,
        "b: both injectors reported a live injected hwnd");
  Check(mm.SlotCoverageReason(0) == "placement: covered" &&
            mm.SlotCoverageReason(1) == "placement: covered",
        "b: both CoversMonitor verdicts are kCovered "
        "(SlotCoverageReason 0 and 1)");
  bool shared_ok = g_inj.size() == 2;
  for (const auto& kv : g_inj) {
    shared_ok = shared_ok && kv.second.shared_set &&
                kv.second.shared.client_rect.left == 0 &&
                kv.second.shared.client_rect.top == 0 &&
                kv.second.shared.client_rect.right == 3840 &&
                kv.second.shared.client_rect.bottom == 1080;
  }
  Check(shared_ok,
        "b: both injectors received the shared host covering "
        "(0,0,3840,1080)");
  Check(mm.headless_slot_count() == 0, "b: headless_slot_count()==0");
  Check(!mm.has_headless_slots(), "b: has_headless_slots()==false");
  mm.Shutdown();
  Check(mm.slot_count() == 0, "b: Shutdown clears all slots");
}

// ---------------------------------------------------------------------------
// Scenario (c): second monitor LEFT of primary (-1920,0) - negative virtual
// coords, the exact case known-limitations.md:31-35 marks VM-only/unproven.
// FAILURE QA EVIDENCE (plan row 12 QA scenario): host covering ONLY
// (0,0,1920,1080) with a monitor at (-1920,0) -> the stub reports false
// (the child actually visible covers the host rect, not the negative
// monitor), so the slot degrades to headless instead of a silent success.
//
// Contract note: multi_monitor.cpp:151-178 keeps a failed-attach slot LIVE
// but headless (row 9's green false-success test asserts the same), so
// slot_count()==2 and headless_slot_count()==1. The plan text's
// "slot_count()==1" expectation contradicts that locked contract; this suite
// asserts the production contract and reports the discrepancy (see evidence
// + issues.md). The plan's intent - stub reports false, no silent success -
// is fully honoured.
// ---------------------------------------------------------------------------
void TestC_SecondMonitorLeftNegative() {
  ResetStubs();
  g_monitors = {
      MakeMon(0, 0, 0, 1920, 1080, true, L"\\\\.\\DISPLAY1", 100),
      MakeMon(1, -1920, 0, 1920, 1080, false, L"\\\\.\\DISPLAY2", 100),
  };
  // Host covers ONLY the primary - the negative-origin monitor is outside it.
  g_resolve_result = MakeHost(0, 0, 1920, 1080);
  // What a host-origin placement actually leaves visible for monitor 1:
  // the child sits on the host rect (0,0,1920,1080), disjoint from
  // (-1920,0,0,1080) -> CoversMonitor kOutOfBounds -> Attach returns false.
  g_reported_child_queue = {
      PlacementRect{0, 0, 1920, 1080},     // mon0: covered
      PlacementRect{0, 0, 1920, 1080},     // mon1: disjoint from (-1920,0)
  };

  // Geometry rule, locked directly: the host rect does NOT cover the
  // negative-origin monitor; a correctly-placed child there WOULD.
  const PlacementRect host_only{0, 0, 1920, 1080};
  const PlacementRect mon_left{-1920, 0, 0, 1080};
  const PlacementRect child_on_left{-1920, 0, 0, 1080};
  Check(CoversMonitor(host_only, mon_left) == CoverageVerdict::kOutOfBounds,
        "c: CoversMonitor(host(0,0,1920x1080), mon(-1920,0,0x1080)) == "
        "kOutOfBounds");
  Check(CoversMonitor(child_on_left, mon_left) == CoverageVerdict::kCovered,
        "c: correctly-placed child on the negative-origin monitor IS "
        "kCovered (screen space, not host-client)");
  Check(CoversMonitor(PlacementRect{0, 0, 1920, 1080}, mon_left) ==
            CoverageVerdict::kOutOfBounds,
        "c: child anchored at the host origin is kOutOfBounds vs the "
        "negative monitor (the failure fixture)");

  MultiMonitor mm(&RecordLog, MakeRecordingFactory());
  mm.SetHeadlessHost(kHeadlessHost);
  const bool ok = mm.Init(MultiMonitorMode::PerMonitor);
  Check(ok, "c: Init(PerMonitor) returns true (headless counts as live)");
  Check(mm.slot_count() == 2,
        "c: slot_count()==2 (both slots live - failed attach degrades to "
        "headless, it does not erase the slot; multi_monitor.cpp:151-178)");
  Check(AttachRectsEqual({{0, 0, 1920, 1080}, {-1920, 0, 1920, 1080}}),
        "c: stubs still received their own rects including the NEGATIVE "
        "origin (-1920,0,1920x1080)");
  const InjState* inj0 = FindInj(0, 0, 1920, 1080);
  const InjState* inj_left = FindInj(-1920, 0, 1920, 1080);
  Check(inj0 != nullptr && inj_left != nullptr,
        "c: both injectors identified by their received rect");
  Check(inj_left != nullptr && inj_left->attach_calls == 1 &&
            inj_left->hwnd == nullptr,
        "c: the negative-origin stub reported FALSE (no injected hwnd)");
  Check(mm.headless_slot_count() == 1,
        "c: headless_slot_count()==1 (only the non-covering slot)");
  Check(mm.SlotCoverageReason(0) == "placement: covered",
        "c: SlotCoverageReason(0) == 'placement: covered' (primary)");
  Check(mm.SlotCoverageReason(1) == "placement: OUT-OF-BOUNDS",
        "c: SlotCoverageReason(1) == 'placement: OUT-OF-BOUNDS' - NOT a "
        "silent success");
  Check(LogContains("placement: RETRY-FALSE-SUCCESS") &&
            LogContains("Attach failed for monitor 1"),
        "c: honest failure log lines emitted (RETRY-FALSE-SUCCESS + "
        "headless fallback)");
  // Print the census + log lines so the failure evidence file carries the
  // actual observed output, not just the assertion names.
  std::printf("get_state: {\"state\":{\"headless_slots\":%d,\"live\":%s}}\n",
              mm.headless_slot_count(), mm.slot_count() > 0 ? "true" : "false");
  for (const std::string& l : g_logs) {
    if (l.find("Attach failed") != std::string::npos ||
        l.find("RETRY-FALSE-SUCCESS") != std::string::npos) {
      std::cout << "[LOG] " << l << "\n";
    }
  }
  mm.Shutdown();
  Check(mm.slot_count() == 0, "c: Shutdown clears all slots");
}

// ---------------------------------------------------------------------------
// Scenario (d): second monitor ABOVE primary (0,-1080) - negative Y origin.
// ---------------------------------------------------------------------------
void TestD_SecondMonitorAbove() {
  ResetStubs();
  g_monitors = {
      MakeMon(0, 0, 0, 1920, 1080, true, L"\\\\.\\DISPLAY1", 100),
      MakeMon(1, 0, -1080, 1920, 1080, false, L"\\\\.\\DISPLAY2", 100),
  };
  // Shared host spans both: union (0,-1080)-(1920,1080).
  g_resolve_result = MakeHost(0, -1080, 1920, 1080);
  g_reported_child_queue = {
      PlacementRect{0, 0, 1920, 1080},       // mon0
      PlacementRect{0, -1080, 1920, 0},      // mon1 (negative Y)
  };

  const PlacementRect mon0{0, 0, 1920, 1080};
  const PlacementRect mon_up{0, -1080, 1920, 0};
  const PlacementRect host{0, -1080, 1920, 1080};
  Check(CoversMonitor(host, mon0) == CoverageVerdict::kCovered,
        "d: CoversMonitor(host(0,-1080,1920x2160), mon0) == kCovered");
  Check(CoversMonitor(host, mon_up) == CoverageVerdict::kCovered,
        "d: CoversMonitor(host, mon(0,-1080,1920x1080)) == kCovered");
  Check(CoversMonitor(mon_up, mon_up) == CoverageVerdict::kCovered,
        "d: correctly-placed child on the above monitor is kCovered");

  MultiMonitor mm(&QuietLog, MakeRecordingFactory());
  mm.SetHeadlessHost(kHeadlessHost);
  const bool ok = mm.Init(MultiMonitorMode::PerMonitor);
  Check(ok, "d: Init(PerMonitor) returns true");
  Check(mm.slot_count() == 2, "d: slot_count()==2");
  Check(AttachRectsEqual({{0, 0, 1920, 1080}, {0, -1080, 1920, 1080}}),
        "d: stubs received (0,0,1920x1080) and the negative-Y rect "
        "(0,-1080,1920x1080)");
  Check(mm.SlotCoverageReason(0) == "placement: covered" &&
            mm.SlotCoverageReason(1) == "placement: covered",
        "d: both CoversMonitor verdicts are kCovered");
  Check(mm.headless_slot_count() == 0, "d: headless_slot_count()==0");
  mm.Shutdown();
  Check(mm.slot_count() == 0, "d: Shutdown clears all slots");
}

// ---------------------------------------------------------------------------
// Scenario (e): portrait secondary - rect 1920,0,3000,1920 -> width 1080,
// height 1920 (height > width = IsPortrait()).
// ---------------------------------------------------------------------------
void TestE_PortraitSecondary() {
  ResetStubs();
  g_monitors = {
      MakeMon(0, 0, 0, 1920, 1080, true, L"\\\\.\\DISPLAY1", 100, 0),
      // Portrait: DEVMODEW.dmDisplayOrientation = DMDO_90; width/height
      // already reflect the rotated resolution (monitor_util.cpp fidelity).
      MakeMon(1, 1920, 0, 1080, 1920, false, L"\\\\.\\DISPLAY2", 100, 1),
  };
  g_resolve_result = MakeHost(0, 0, 3000, 1920);
  g_reported_child_queue = {
      PlacementRect{0, 0, 1920, 1080},
      PlacementRect{1920, 0, 3000, 1920},   // portrait: 1080x1920
  };

  Check(g_monitors[1].IsPortrait(),
        "e: fixture secondary IsPortrait() == true (1080 < 1920)");
  Check(!g_monitors[0].IsPortrait(),
        "e: fixture primary IsPortrait() == false");
  const PlacementRect mon_port{1920, 0, 3000, 1920};
  const PlacementRect host{0, 0, 3000, 1920};
  Check(CoversMonitor(host, mon_port) == CoverageVerdict::kCovered,
        "d/e: CoversMonitor(host(0,0,3000x1920), portrait monitor) == "
        "kCovered");

  MultiMonitor mm(&QuietLog, MakeRecordingFactory());
  mm.SetHeadlessHost(kHeadlessHost);
  const bool ok = mm.Init(MultiMonitorMode::PerMonitor);
  Check(ok, "e: Init(PerMonitor) returns true");
  Check(mm.slot_count() == 2, "e: slot_count()==2");
  Check(AttachRectsEqual({{0, 0, 1920, 1080}, {1920, 0, 1080, 1920}}),
        "e: stubs received (0,0,1920x1080) and portrait (1920,0,1080x1920)");
  Check(mm.slots().count(1) == 1 && mm.slots().at(1).info.IsPortrait() &&
            mm.slots().at(1).info.orientation == 1,
        "e: Slot.info for monitor 1 carries IsPortrait() and orientation 1");
  Check(mm.SlotCoverageReason(0) == "placement: covered" &&
            mm.SlotCoverageReason(1) == "placement: covered",
        "e: both CoversMonitor verdicts are kCovered");
  Check(mm.headless_slot_count() == 0, "e: headless_slot_count()==0");
  mm.Shutdown();
  Check(mm.slot_count() == 0, "e: Shutdown clears all slots");
}

// ---------------------------------------------------------------------------
// Scenario (f): three monitors mixed sizes.
// ---------------------------------------------------------------------------
void TestF_ThreeMixedSizes() {
  ResetStubs();
  g_monitors = {
      MakeMon(0, 0, 0, 1920, 1080, true, L"\\\\.\\DISPLAY1", 100),
      MakeMon(1, 1920, 0, 2560, 1440, false, L"\\\\.\\DISPLAY2", 100),
      MakeMon(2, 0, 1080, 1280, 720, false, L"\\\\.\\DISPLAY3", 100),
  };
  // Union: x 0..4480, y 0..1800.
  g_resolve_result = MakeHost(0, 0, 4480, 1800);
  g_reported_child_queue = {
      PlacementRect{0, 0, 1920, 1080},
      PlacementRect{1920, 0, 4480, 1440},
      PlacementRect{0, 1080, 1280, 1800},
  };

  const PlacementRect host{0, 0, 4480, 1800};
  const PlacementRect mons[3] = {{0, 0, 1920, 1080},
                                  {1920, 0, 4480, 1440},
                                  {0, 1080, 1280, 1800}};
  bool host_covers_all = true;
  for (const PlacementRect& m : mons) {
    host_covers_all = host_covers_all &&
                      CoversMonitor(host, m) == CoverageVerdict::kCovered;
  }
  Check(host_covers_all,
        "f: shared host (0,0,4480x1800) covers all three monitors");

  MultiMonitor mm(&QuietLog, MakeRecordingFactory());
  mm.SetHeadlessHost(kHeadlessHost);
  const bool ok = mm.Init(MultiMonitorMode::PerMonitor);
  Check(ok, "f: Init(PerMonitor) returns true with 3 monitors");
  Check(mm.slot_count() == 3, "f: slot_count()==3");
  Check(AttachRectsEqual({{0, 0, 1920, 1080},
                           {1920, 0, 2560, 1440},
                           {0, 1080, 1280, 720}}),
        "f: three stubs received their own rects (mixed sizes preserved)");
  Check(mm.SlotCoverageReason(0) == "placement: covered" &&
            mm.SlotCoverageReason(1) == "placement: covered" &&
            mm.SlotCoverageReason(2) == "placement: covered",
        "f: all three CoversMonitor verdicts are kCovered");
  Check(mm.headless_slot_count() == 0, "f: headless_slot_count()==0");
  mm.Shutdown();
  Check(mm.slot_count() == 0, "f: Shutdown clears all slots");
}

// ---------------------------------------------------------------------------
// Scenario (g): duplicate mode - two monitors reporting the IDENTICAL rect
// (0,0,1920,1080). DetectKeyCollision (row 5, real k6wp_shared symbol)
// fires; BOTH slots still attach.
// ---------------------------------------------------------------------------
void TestG_DuplicateModeCollision() {
  ResetStubs();
  g_monitors = {
      MakeMon(0, 0, 0, 1920, 1080, true, L"\\\\.\\DISPLAY1", 100),
      MakeMon(1, 0, 0, 1920, 1080, false, L"\\\\.\\DISPLAY2", 100),
  };
  g_resolve_result = MakeHost(0, 0, 1920, 1080);
  g_reported_child_queue = {
      PlacementRect{0, 0, 1920, 1080},
      PlacementRect{0, 0, 1920, 1080},   // IDENTICAL rect
  };

  // Rect rule: both fixtures share the exact same physical rect.
  Check(g_monitors[0].x == g_monitors[1].x &&
            g_monitors[0].y == g_monitors[1].y &&
            g_monitors[0].width == g_monitors[1].width &&
            g_monitors[0].height == g_monitors[1].height,
        "g: fixture monitors report the IDENTICAL rect (0,0,1920x1080)");

  // Row 5: the REAL DetectKeyCollision from shared/displays_schema.hpp
  // (k6wp_shared linked). Two assignments, two keys resolving to one
  // physical rect -> both keys reported (IS-7 shared-rect clause).
  DisplaysConfig cfg;
  cfg.assignments[L"\\\\.\\DISPLAY1"] = {L"C:\\v\\1.mp4", true};
  cfg.assignments[L"\\\\.\\DISPLAY2"] = {L"C:\\v\\2.mp4", true};
  const std::vector<std::wstring> hits =
      DetectKeyCollision(cfg, g_monitors);
  Check(hits.size() == 2,
        "g: DetectKeyCollision reports exactly 2 keys (shared rect)");
  bool has1 = false, has2 = false;
  for (const std::wstring& k : hits) {
    if (k == L"\\\\.\\DISPLAY1") has1 = true;
    if (k == L"\\\\.\\DISPLAY2") has2 = true;
  }
  Check(has1 && has2,
        "g: both assignment keys reported (IS-7 shared physical rect)");

  // Companion: duplicated szDevice in the live monitor list (exact clone
  // mode - GetMonitorInfoW reports the same szDevice for two HMONITORs).
  const std::vector<MonitorInfo> clone_list = {
      MakeMon(0, 0, 0, 1920, 1080, true, L"\\\\.\\DISPLAY1", 100),
      MakeMon(1, 0, 0, 1920, 1080, false, L"\\\\.\\DISPLAY1", 100),
      MakeMon(2, 1920, 0, 1920, 1080, false, L"\\\\.\\DISPLAY3", 100),
  };
  DisplaysConfig cfg2;
  cfg2.assignments[L"\\\\.\\DISPLAY1"] = {L"C:\\v\\1.mp4", true};
  cfg2.assignments[L"\\\\.\\DISPLAY3"] = {L"C:\\v\\3.mp4", true};
  const std::vector<std::wstring> hits2 = DetectKeyCollision(cfg2, clone_list);
  bool dup_hit = false, dist_ok = true;
  for (const std::wstring& k : hits2) {
    if (k == L"\\\\.\\DISPLAY1") dup_hit = true;
    if (k == L"\\\\.\\DISPLAY3") dist_ok = false;
  }
  Check(dup_hit && dist_ok,
        "g: duplicated szDevice key reported (IS-7 clone mode), "
        "distinct-rect key not reported");

  // Both slots still attach despite the collision.
  MultiMonitor mm(&QuietLog, MakeRecordingFactory());
  mm.SetHeadlessHost(kHeadlessHost);
  const bool ok = mm.Init(MultiMonitorMode::PerMonitor);
  Check(ok, "g: Init(PerMonitor) returns true in duplicate mode");
  Check(mm.slot_count() == 2,
        "g: slot_count()==2 - BOTH slots still attach despite the "
        "collision");
  Check(AttachRectsEqual({{0, 0, 1920, 1080}, {0, 0, 1920, 1080}}),
        "g: both stubs received the identical rect (0,0,1920x1080)");
  Check(mm.SlotCoverageReason(0) == "placement: covered" &&
            mm.SlotCoverageReason(1) == "placement: covered",
        "g: both CoversMonitor verdicts are kCovered");
  Check(mm.headless_slot_count() == 0, "g: headless_slot_count()==0");
  mm.Shutdown();
  Check(mm.slot_count() == 0, "g: Shutdown clears all slots");
}

// ---------------------------------------------------------------------------
// Scenario (h): mixed-DPI - same rect, two different scale_pct values.
// MonitorInfo carries both; placement (physical px) is unchanged. Companion
// case: distinct physical rects at different scale_pct - the attach rect is
// exactly the physical px (no DPI arithmetic).
// ---------------------------------------------------------------------------
void TestH_MixedDpi() {
  // h1: identical rect, different scale_pct.
  ResetStubs();
  g_monitors = {
      MakeMon(0, 0, 0, 1920, 1080, true, L"\\\\.\\DISPLAY1", 100),
      MakeMon(1, 0, 0, 1920, 1080, false, L"\\\\.\\DISPLAY2", 150),
  };
  g_resolve_result = MakeHost(0, 0, 1920, 1080);
  g_reported_child_queue = {
      PlacementRect{0, 0, 1920, 1080},
      PlacementRect{0, 0, 1920, 1080},
  };

  Check(g_monitors[0].scale_pct == 100 && g_monitors[1].scale_pct == 150,
        "h: fixture scale_pct values differ (100 vs 150) on the same rect");
  Check(g_monitors[0].ScalePercent() == 100 &&
            g_monitors[1].ScalePercent() == 150,
        "h: ScalePercent() readout carries both DPI values");

  MultiMonitor mm(&QuietLog, MakeRecordingFactory());
  mm.SetHeadlessHost(kHeadlessHost);
  const bool ok = mm.Init(MultiMonitorMode::PerMonitor);
  Check(ok, "h: Init(PerMonitor) returns true with mixed-DPI fixtures");
  Check(mm.slot_count() == 2, "h: slot_count()==2");
  Check(AttachRectsEqual({{0, 0, 1920, 1080}, {0, 0, 1920, 1080}}),
        "h: placement unchanged - both stubs received the identical "
        "physical rect (0,0,1920x1080) despite different scale_pct");
  Check(mm.slots().count(0) == 1 && mm.slots().count(1) == 1 &&
            mm.slots().at(0).info.scale_pct == 100 &&
            mm.slots().at(1).info.scale_pct == 150,
        "h: MonitorInfo carried through Slot.info keeps BOTH scale_pct "
        "values (100 and 150)");
  Check(mm.SlotCoverageReason(0) == "placement: covered" &&
            mm.SlotCoverageReason(1) == "placement: covered",
        "h: both CoversMonitor verdicts are kCovered");
  Check(mm.headless_slot_count() == 0, "h: headless_slot_count()==0");
  mm.Shutdown();
  Check(mm.slot_count() == 0, "h: Shutdown clears all slots (h1)");

  // h2 companion: distinct physical rects at different scale_pct - the
  // attach rect must be exactly the physical px, never DPI-scaled.
  ResetStubs();
  g_monitors = {
      MakeMon(0, 0, 0, 1920, 1080, true, L"\\\\.\\DISPLAY1", 100),
      MakeMon(1, 1920, 0, 2560, 1440, false, L"\\\\.\\DISPLAY2", 150),
  };
  g_resolve_result = MakeHost(0, 0, 4480, 1440);
  g_reported_child_queue = {
      PlacementRect{0, 0, 1920, 1080},
      PlacementRect{1920, 0, 4480, 1440},
  };
  MultiMonitor mm2(&QuietLog, MakeRecordingFactory());
  mm2.SetHeadlessHost(kHeadlessHost);
  const bool ok2 = mm2.Init(MultiMonitorMode::PerMonitor);
  Check(ok2, "h2: Init(PerMonitor) returns true with mixed-DPI rects");
  Check(mm2.slot_count() == 2, "h2: slot_count()==2");
  Check(AttachRectsEqual({{0, 0, 1920, 1080}, {1920, 0, 2560, 1440}}),
        "h2: attach rects are physical px (1920x1080 + 2560x1440) - "
        "NOT divided/multiplied by the 150% scale");
  Check(mm2.slots().at(0).info.scale_pct == 100 &&
            mm2.slots().at(1).info.scale_pct == 150,
        "h2: Slot.info still carries both scale_pct values");
  Check(mm2.headless_slot_count() == 0, "h2: headless_slot_count()==0");
  mm2.Shutdown();
  Check(mm2.slot_count() == 0, "h2: Shutdown clears all slots");
}

}  // namespace k6wp

int main(int argc, char** argv) {
  const std::string sel = argc > 1 ? argv[1] : "all";
  if (sel == "all" || sel == "a") {
    k6wp::TestA_OneMonitorPrimary();
  }
  if (sel == "all" || sel == "b") {
    k6wp::TestB_TwoSideBySideHappy();
  }
  if (sel == "all" || sel == "c") {
    k6wp::TestC_SecondMonitorLeftNegative();
  }
  if (sel == "all" || sel == "d") {
    k6wp::TestD_SecondMonitorAbove();
  }
  if (sel == "all" || sel == "e") {
    k6wp::TestE_PortraitSecondary();
  }
  if (sel == "all" || sel == "f") {
    k6wp::TestF_ThreeMixedSizes();
  }
  if (sel == "all" || sel == "g") {
    k6wp::TestG_DuplicateModeCollision();
  }
  if (sel == "all" || sel == "h") {
    k6wp::TestH_MixedDpi();
  }
  std::cout << k6wp::g_checks << " checks, " << k6wp::g_failures
            << " failures\n";
  return k6wp::g_failures == 0 ? 0 : 1;
}
