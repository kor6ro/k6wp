// Unit tests for the injectable SlotFactory seam in MultiMonitor (plan row 6,
// GAP-11 / IS-5). No external test framework: plain Check() counter, same
// style as tests/occlusion_test.cpp / tests/config_test.cpp. Exit 0 = pass.
//
// LINK MODEL (the point of this suite): only engine/src/multi_monitor.cpp is
// compiled from the engine. Every DesktopInjector / MpvRenderer /
// ListMonitors symbol multi_monitor.cpp references is DEFINED HERE as a
// recording stub, so the binary never links desktop_inject.cpp,
// mpv_renderer.cpp or mpv.lib - a headless MultiMonitor. The stubs are not
// silent no-ops: they record the exact arguments production code passes
// (LogFn, InjectMode, monitor rect, hwnd, adapter pin) and the tests assert
// on those records, which is what makes the default-factory run a
// production-invariance check.
//
// Coverage: happy (fake factory + 2-monitor fixture -> slot_count()==2 with
// each stub's Attach receiving its own monitor rect), default factory (the
// inline DefaultSlotFactory production path: same arguments, same order),
// and the headless failure contract (Attach returns false ->
// headless_slot_count()==2 / has_headless_slots()==true without any real
// MpvRenderer in the binary).
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "multi_monitor.hpp"

namespace k6wp {

// Complete the Pimpl so the stubbed ~DesktopInjector can destroy its
// unique_ptr<Impl> member. desktop_inject.cpp (the real Impl) is NOT linked,
// so this is the only definition in the binary - no ODR conflict.
struct DesktopInjector::Impl {};

namespace {

// Per-instance DesktopInjector recording. Keyed by `this` because Impl is
// not usable for storage here (different body than the real one).
struct InjState {
  LogFn log = nullptr;
  InjectMode mode = InjectMode::kAuto;
  bool mode_set = false;
  // Row 7: the pass's shared host as received via SetSharedHost (recorded
  // per injector; deliberately NOT a g_events entry so row 6's verbatim
  // construction-order assertion stays untouched).
  bool shared_set = false;
  SharedHost shared{};
  void* hwnd = nullptr;  // injected_hwnd() while "attached"
  int attach_calls = 0;
  int detach_calls = 0;
  int reassert_calls = 0;
  int on_display_change_calls = 0;
  // Row 11: per-instance last Attach rect + last OnDisplayChange args, so a
  // host-rect-only scenario can tell WHICH slot re-placed and with what
  // monitor rect (the global g_attach_rects has no injector identity).
  int att_x = 0, att_y = 0, att_w = 0, att_h = 0;
  int odc_x = 0, odc_y = 0, odc_w = 0, odc_h = 0;
  // Row 9: last CoverageReason token the attach path produced (mirrors
  // DesktopInjector::last_coverage_reason / last_coverage_reason_).
  std::string last_cov;
};

// Per-instance MpvRenderer recording.
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
std::vector<std::string> g_events;  // ordered construction/call trace
std::vector<Rect4> g_attach_rects;  // Attach() args in call order
std::vector<void*> g_create_hwnds;  // MpvRenderer::Create hwnds in call order
std::vector<MonitorInfo> g_monitors;
bool g_attach_ok = true;
int g_inj_ctors = 0, g_inj_dtors = 0;
int g_ren_ctors = 0, g_ren_dtors = 0;
int g_factory_injectors = 0, g_factory_renderers = 0;
// Row 7: what the link-level ResolveSharedHost stub returns for the whole
// attach pass (real ResolveSharedHost lives in desktop_inject.cpp, NOT
// linked here). Default = a fake resolved host; the null-host failure test
// clears it to simulate ResolveSharedHost finding no Progman.
SharedHost g_resolve_result{};
int g_resolve_calls = 0;
InjectMode g_resolve_mode = InjectMode::kAuto;
// Log capture so the failure test can assert the EXISTING headless fallback
// line (multi_monitor.cpp:117-122) without a real LogFn.
std::vector<std::string> g_logs;
// Row 9: per-attach-call "reported child rect" fixture (SCREEN space, what
// production reads back with GetWindowRect after CreateAndAttach). Consumed
// in Attach call order; an empty queue skips the coverage check so every
// pre-row-9 scenario keeps its exact behaviour.
std::vector<PlacementRect> g_reported_child_queue;

// Sentinel passed to SetHeadlessHost: must be the hwnd Create() receives
// when injection failed, and must never appear when injection succeeded.
void* const kHeadlessHost = reinterpret_cast<void*>(static_cast<uintptr_t>(0x77));

// Non-null fake resolved host handed out by the ResolveSharedHost stub unless
// a test clears it (null-Progman fixture).
SharedHost MakeStubSharedHost() {
  SharedHost sh;
  sh.host = reinterpret_cast<void*>(static_cast<uintptr_t>(0xBEEF));
  sh.progman = reinterpret_cast<void*>(static_cast<uintptr_t>(0xFEED));
  sh.def_view = reinterpret_cast<void*>(static_cast<uintptr_t>(0xDEA1));
  sh.insert_after = sh.def_view;
  sh.layered = true;
  sh.branch = "stub branch";
  sh.client_rect.left = 0;
  sh.client_rect.top = 0;
  sh.client_rect.right = 1920;
  sh.client_rect.bottom = 1080;
  return sh;
}

// Row 11: same host handles, client rect shifted +5px in screen space -
// the Explorer-restart / DPI-change shape where the HOST moved but every
// monitor rect is untouched.
SharedHost MakeMovedHost() {
  SharedHost sh = MakeStubSharedHost();
  sh.client_rect.left = 5;
  sh.client_rect.top = 5;
  sh.client_rect.right = 1925;
  sh.client_rect.bottom = 1085;
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
  g_resolve_result = MakeStubSharedHost();
  g_resolve_calls = 0;
  g_resolve_mode = InjectMode::kAuto;
}

// Unique non-null fake HWND handed out by the stub Attach on success.
void* NextStubHwnd() {
  static int n = 0;
  ++n;
  return reinterpret_cast<void*>(static_cast<uintptr_t>(0x1000 + n));
}

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

// The LogFn under test: identity-compared against what the stub ctors saw.
void QuietLog(const char* /*fmt*/, ...) {}

// Recording LogFn for the null-host failure test: captures the EXISTING
// headless fallback line verbatim (multi_monitor.cpp:117-122).
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

void SetupMonitors() {
  MonitorInfo a;
  a.id = 0;
  a.x = 0;
  a.y = 0;
  a.width = 1920;
  a.height = 1080;
  a.is_primary = true;
  MonitorInfo b;
  b.id = 1;
  b.x = 1920;
  b.y = 0;
  b.width = 2560;
  b.height = 1440;
  g_monitors.clear();
  g_monitors.push_back(a);
  g_monitors.push_back(b);
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
  // Row 7 production contract mirrored here: Attach refuses when the pass's
  // shared host is unresolved (null Progman) - that refusal is exactly what
  // drives AttachSlot's existing headless fallback line.
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
  // Row 9 production contract mirrored here (desktop_inject.cpp
  // AttachToDesktop): when the fixture supplies a "reported child rect" for
  // this Attach call, compare it against the requested monitor rect in
  // SCREEN space via the REAL CoversMonitor. Not covered -> false success:
  // honest RETRY-FALSE-SUCCESS log, no injected hwnd (headless census),
  // return false so AttachSlot degrades the slot. Empty queue = legacy
  // scenarios unchanged.
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

// Row 4 wiring: MultiMonitor::AttachSlot copies this into Slot.coverage_reason.
// Returns by value like the real one; no event so the construction-order
// assertion above stays purely about slot construction. Row 9: returns the
// stub's last recorded verdict token ("" until an attach path sets one).
std::string DesktopInjector::last_coverage_reason() const {
  const auto it = g_inj.find(this);
  return it == g_inj.end() ? std::string{} : it->second.last_cov;
}

// ---- link-level stub: row 7 shared-host plumbing (desktop_inject.cpp NOT linked)

// MultiMonitor resolves the host ONCE per attach pass and hands the result to
// every slot injector through this before Attach (AttachSlot,
// ReattachSpanLocked, Reanchor's survivor loop).
void DesktopInjector::SetSharedHost(const SharedHost& host) {
  InjState& s = g_inj[this];
  s.shared = host;
  s.shared_set = true;
}

// The free ResolveSharedHost stub: counts calls (the "one resolution per pass
// regardless of slot count" contract) and returns the configurable fixture -
// g_resolve_result = {} simulates ResolveSharedHost finding no Progman.
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

// ---- link-level stubs: monitor enumeration (k6wp_shared NOT linked) -------
// The deterministic 2-monitor fixture: ids 0/1, disjoint rects, primary
// first - exactly the ListMonitors() sort contract.

std::vector<MonitorInfo> ListMonitors() noexcept { return g_monitors; }

MonitorInfo GetPrimaryMonitor() noexcept {
  return g_monitors.empty() ? MonitorInfo{} : g_monitors.front();
}

// A fake factory with the exact DefaultSlotFactory shape: same type, same
// argument, plus construction counters so the test can prove WHICH factory
// built the slots.
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

// Production order inside AttachSlot (multi_monitor.cpp), asserted verbatim:
// injector ctor(log) -> SetInjectMode -> Attach -> renderer ctor ->
// SetAdapterPin -> Create. Two slots = the sequence twice.
const std::vector<std::string>& ExpectedSequence() {
  static const std::vector<std::string> kSeq = {
      "inj-ctor", "setmode", "attach", "ren-ctor", "setpin", "create",
      "inj-ctor", "setmode", "attach", "ren-ctor", "setpin", "create"};
  return kSeq;
}

bool InjectedHwndMatchesCreatedHwnds() {
  // Happy path: every Create() hwnd must be an injected (non-null,
  // non-headless-host) hwnd, and the sets must match pairwise.
  std::vector<void*> injected;
  for (const auto& kv : g_inj) {
    if (kv.second.hwnd == nullptr || kv.second.hwnd == kHeadlessHost) {
      return false;
    }
    injected.push_back(kv.second.hwnd);
  }
  if (injected.size() != g_create_hwnds.size()) {
    return false;
  }
  for (void* h : g_create_hwnds) {
    if (h == nullptr || h == kHeadlessHost) {
      return false;
    }
    bool found = false;
    for (void* i : injected) {
      if (i == h) {
        found = true;
        break;
      }
    }
    if (!found) {
      return false;
    }
  }
  return true;
}

// QA happy scenario: fake factory + 2-monitor fixture, Init(PerMonitor) ->
// slot_count()==2, both stubs' Attach received their own monitor rect.
void TestHappyInjectedFactory() {
  ResetStubs();
  g_attach_ok = true;
  MultiMonitor mm(&QuietLog, MakeRecordingFactory());
  mm.SetHeadlessHost(kHeadlessHost);
  mm.SetInjectMode(InjectMode::kProgman);
  mm.SetAdapterPin("Intel");

  const bool ok = mm.Init(MultiMonitorMode::PerMonitor);
  Check(ok, "happy: Init(PerMonitor) returns true with 2 monitors");
  Check(mm.slot_count() == 2, "happy: slot_count()==2");
  Check(mm.slots().size() == 2, "happy: slots() exposes both slots");
  Check(mm.slots().count(0) == 1 && mm.slots().count(1) == 1,
        "happy: slots() keyed by monitor id 0 and 1");
  const std::vector<int> want_ids = {0, 1};
  Check(mm.monitor_ids() == want_ids, "happy: monitor_ids()=={0,1}");
  Check(mm.mode() == MultiMonitorMode::PerMonitor,
        "happy: mode()==PerMonitor");
  Check(g_factory_injectors == 2, "happy: injected factory built 2 injectors");
  Check(g_factory_renderers == 2, "happy: injected factory built 2 renderers");

  bool logs_ok = g_inj.size() == 2;
  bool modes_ok = g_inj.size() == 2;
  for (const auto& kv : g_inj) {
    logs_ok = logs_ok && (kv.second.log == &QuietLog);
    modes_ok = modes_ok && kv.second.mode_set &&
               kv.second.mode == InjectMode::kProgman;
  }
  Check(logs_ok, "happy: every injector got the MultiMonitor LogFn argument");
  Check(modes_ok, "happy: SetInjectMode(kProgman) reached both injectors");

  // Row 7: ONE ResolveSharedHost for the whole 2-slot pass, thread mode
  // through, and the resolved host delivered to every slot injector.
  Check(g_resolve_calls == 1,
        "happy: ResolveSharedHost ran exactly once for 2 slots");
  Check(g_resolve_mode == InjectMode::kProgman,
        "happy: ResolveSharedHost received the injector mode (kProgman)");
  bool shared_ok = g_inj.size() == 2;
  for (const auto& kv : g_inj) {
    shared_ok = shared_ok && kv.second.shared_set &&
                kv.second.shared.host == g_resolve_result.host &&
                kv.second.shared.host != nullptr && kv.second.shared.layered;
  }
  Check(shared_ok,
        "happy: both injectors received the pass's shared host via "
        "SetSharedHost");

  Check(g_attach_rects.size() == 2, "happy: Attach called exactly twice");
  if (g_attach_rects.size() == 2) {
    Check(g_attach_rects[0].x == 0 && g_attach_rects[0].y == 0 &&
              g_attach_rects[0].w == 1920 && g_attach_rects[0].h == 1080,
          "happy: stub #1 Attach received monitor 0 rect (0,0,1920x1080)");
    Check(g_attach_rects[1].x == 1920 && g_attach_rects[1].y == 0 &&
              g_attach_rects[1].w == 2560 && g_attach_rects[1].h == 1440,
          "happy: stub #2 Attach received monitor 1 rect (1920,0,2560x1440)");
  }
  Check(g_events == ExpectedSequence(),
        "happy: construction order matches pre-seam AttachSlot sequence");

  bool pins_ok = g_ren.size() == 2;
  for (const auto& kv : g_ren) {
    pins_ok = pins_ok && kv.second.created && kv.second.adapter_pin == "Intel";
  }
  Check(pins_ok, "happy: adapter pin 'Intel' reached both renderers");
  Check(InjectedHwndMatchesCreatedHwnds(),
        "happy: each Create() got its injector's injected hwnd");
  Check(!mm.has_headless_slots(),
        "happy: has_headless_slots()==false after successful attach");
  Check(mm.headless_slot_count() == 0, "happy: headless_slot_count()==0");

  mm.Shutdown();
  Check(mm.slot_count() == 0, "happy: Shutdown clears all slots");
  Check(g_inj_dtors == 2 && g_ren_dtors == 2,
        "happy: Shutdown destroyed both stub pairs");
}

// Production invariance: construct WITHOUT a factory so the compiler
// materialises the default argument - the inline DefaultSlotFactory, i.e.
// the exact make_unique<DesktopInjector>(log) / make_unique<MpvRenderer>()
// code production runs. Same arguments, same order, mpv-free link.
void TestDefaultFactoryProductionShape() {
  ResetStubs();
  g_attach_ok = true;
  {
    MultiMonitor mm(&QuietLog);
    mm.SetHeadlessHost(kHeadlessHost);
    mm.SetInjectMode(InjectMode::kProgman);
    mm.SetAdapterPin("Intel");
    const bool ok = mm.Init(MultiMonitorMode::PerMonitor);
    Check(ok, "default: Init(PerMonitor) returns true");
    Check(mm.slot_count() == 2, "default: slot_count()==2");
    Check(g_factory_injectors == 0 && g_factory_renderers == 0,
          "default: DefaultSlotFactory used (recording factory never ran)");

    bool logs_ok = g_inj.size() == 2;
    for (const auto& kv : g_inj) {
      logs_ok = logs_ok && (kv.second.log == &QuietLog);
    }
    Check(logs_ok,
          "default: default factory passes the same LogFn as pre-seam code");
    Check(g_attach_rects.size() == 2, "default: Attach called exactly twice");
    if (g_attach_rects.size() == 2) {
      Check(g_attach_rects[0].x == 0 && g_attach_rects[0].w == 1920 &&
                g_attach_rects[0].h == 1080,
            "default: Attach got monitor 0 rect unchanged");
      Check(g_attach_rects[1].x == 1920 && g_attach_rects[1].w == 2560 &&
                g_attach_rects[1].h == 1440,
            "default: Attach got monitor 1 rect unchanged");
    }
    Check(g_events == ExpectedSequence(),
          "default: same construction order as pre-seam AttachSlot");
    Check(!mm.has_headless_slots() && mm.headless_slot_count() == 0,
          "default: no headless slots on the success path");
  }
  Check(g_inj_dtors == 2 && g_ren_dtors == 2,
        "default: RAII teardown destroyed both stub pairs");
}

// QA failure scenario: a stub whose Attach returns false -> the existing
// headless contract (multi_monitor.cpp:538-555) reports 2 headless slots,
// and the renderers were built by THIS TU's MpvRenderer stub - no real
// MpvRenderer (mpv) exists in the binary.
void TestHeadlessFallbackContract() {
  ResetStubs();
  g_attach_ok = false;
  MultiMonitor mm(&QuietLog);              // production default materialised...
  mm.SetSlotFactory(MakeRecordingFactory());  // ...then overridden by the seam
  mm.SetHeadlessHost(kHeadlessHost);
  const bool ok = mm.Init(MultiMonitorMode::PerMonitor);
  Check(ok, "headless: Init still returns true (headless counts as live)");
  Check(mm.slot_count() == 2, "headless: slot_count()==2");
  Check(mm.slots().size() == 2, "headless: slots() exposes both slots");
  Check(g_factory_injectors == 2 && g_factory_renderers == 2,
        "headless: SetSlotFactory replaced the default (recording counters)");
  Check(g_attach_rects.size() == 2, "headless: Attach called exactly twice");
  if (g_attach_rects.size() == 2) {
    Check(g_attach_rects[0].x == 0 && g_attach_rects[0].w == 1920 &&
              g_attach_rects[0].h == 1080,
          "headless: stub #1 Attach still received monitor 0 rect");
    Check(g_attach_rects[1].x == 1920 && g_attach_rects[1].w == 2560 &&
              g_attach_rects[1].h == 1440,
          "headless: stub #2 Attach still received monitor 1 rect");
  }
  Check(mm.headless_slot_count() == 2,
        "headless: headless_slot_count()==2 (contract :547-555)");
  Check(mm.has_headless_slots(),
        "headless: has_headless_slots()==true (contract :538-545)");
  Check(g_ren_ctors == 2,
        "headless: 2 renderers built by THIS TU's stub ctor (no real "
        "MpvRenderer in this binary)");
  bool host_ok = g_create_hwnds.size() == 2;
  for (void* h : g_create_hwnds) {
    host_ok = host_ok && (h == kHeadlessHost);
  }
  Check(host_ok,
        "headless: renderer Create() got the hidden host for both slots");
  mm.Shutdown();
  Check(mm.slot_count() == 0, "headless: Shutdown clears all slots");
}

// Row 7 failure scenario: ResolveSharedHost with a null Progman (fixture
// g_resolve_result cleared -> the stub returns a null host) must (a) return
// null, (b) be passed down to every injector by AttachSlot, and (c) leave
// AttachSlot on its EXISTING headless path - the
// "Attach failed for monitor N ..., headless renderer fallback" line
// (multi_monitor.cpp:117-122) - with no crash, both renderers embedded in
// the hidden host.
void TestNullSharedHostDegradesHeadless() {
  ResetStubs();
  g_attach_ok = true;                    // injectors are willing...
  g_resolve_result = SharedHost{};       // ...but ResolveSharedHost found no Progman
  g_logs.clear();

  MultiMonitor mm(&RecordLog, MakeRecordingFactory());
  mm.SetHeadlessHost(kHeadlessHost);

  Check(!g_resolve_result.host,
        "nullhost: fixture ResolveSharedHost returns a null host (null Progman)");
  const bool ok = mm.Init(MultiMonitorMode::PerMonitor);
  Check(ok, "nullhost: Init still returns true (headless slots count as live)");
  Check(mm.slot_count() == 2,
        "nullhost: slot_count()==2 (no crash, both slots exist)");
  Check(g_resolve_calls == 1,
        "nullhost: ResolveSharedHost ran exactly once for 2 slots");

  bool passed_null = g_inj.size() == 2;
  for (const auto& kv : g_inj) {
    passed_null =
        passed_null && kv.second.shared_set && kv.second.shared.host == nullptr;
  }
  Check(passed_null,
        "nullhost: AttachSlot passed the null host down via SetSharedHost");

  Check(LogContains("Attach failed for monitor 0") &&
            LogContains("Attach failed for monitor 1") &&
            LogContains("headless renderer fallback"),
        "nullhost: existing headless fallback line (multi_monitor.cpp:117-122) "
        "emitted for both monitors");

  Check(mm.headless_slot_count() == 2,
        "nullhost: headless_slot_count()==2");
  Check(mm.has_headless_slots(), "nullhost: has_headless_slots()==true");
  bool host_ok = g_create_hwnds.size() == 2;
  for (void* h : g_create_hwnds) {
    host_ok = host_ok && (h == kHeadlessHost);
  }
  Check(host_ok,
        "nullhost: renderer Create() got the hidden host for both slots");
  for (const auto& kv : g_inj) {
    Check(kv.second.attach_calls == 1,
          "nullhost: Attach was attempted exactly once per injector");
    Check(kv.second.hwnd == nullptr,
          "nullhost: injector reports no injected hwnd (headless)");
  }

  mm.Shutdown();
  Check(mm.slot_count() == 0, "nullhost: Shutdown clears all slots");
  // Print the captured fallback lines verbatim so the failure evidence file
  // carries the actual log output, not just the assertion that it matched.
  for (const std::string& l : g_logs) {
    if (l.find("Attach failed") != std::string::npos) {
      std::cout << "[LOG] " << l << "\n";
    }
  }
}

// Row 9 (GAP-8): honest Progman retry / post-attach verification. A child
// rect that does not cover its monitor is a FALSE SUCCESS: the injector must
// return false so AttachSlot takes its headless path (headless_slot_count(),
// tray error, Studio kDegraded) instead of reporting a live slot.
//
// Layer 1 locks the SCREEN-space rule with the REAL desktop_placement.cpp
// (linked into this suite): disjoint screen rect -> kOutOfBounds; a child
// that fully covers a NEGATIVE-origin monitor in screen space -> kCovered
// (oracle note 2: comparing in host-client space would mis-flag it).
//
// Layer 2 drives the seam contract: the stub injector is handed a
// non-covering "reported child rect" for monitor 1 via
// g_reported_child_queue; AttachSlot must end with headless_slot_count()==1,
// an honest SlotCoverageReason, and a `placement: RETRY-FALSE-SUCCESS` log
// line - then print the get_state-shaped JSON the engine would serve.
void TestFalseSuccessHeadless() {
  // Layer 1: real CoversMonitor / CoverageReason, screen space.
  {
    const PlacementRect child_oob{0, 0, 100, 100};
    const PlacementRect mon1{1920, 0, 4480, 1440};
    const CoverageVerdict v = CoversMonitor(child_oob, mon1);
    Check(v == CoverageVerdict::kOutOfBounds,
          "false-success geom: disjoint screen rect -> kOutOfBounds");
    Check(std::string(CoverageReason(v)) == "placement: OUT-OF-BOUNDS",
          "false-success geom: kOutOfBounds reason token");
    const PlacementRect child_neg{-1920, 0, 0, 1080};
    const PlacementRect mon_neg{-1920, 0, 0, 1080};
    Check(CoversMonitor(child_neg, mon_neg) == CoverageVerdict::kCovered,
          "false-success geom: negative-origin screen rect fully covered -> "
          "kCovered (screen space, not host-client)");
  }

  // Layer 2: seam contract. Monitor 0's child covers; monitor 1's child is
  // disjoint (the plan's out-of-bounds failure scenario).
  ResetStubs();
  g_attach_ok = true;
  g_logs.clear();
  g_reported_child_queue = {
      PlacementRect{0, 0, 1920, 1080},      // monitor 0: exact cover
      PlacementRect{0, 0, 100, 100},        // monitor 1: disjoint -> OOB
  };

  MultiMonitor mm(&RecordLog, MakeRecordingFactory());
  mm.SetHeadlessHost(kHeadlessHost);
  const bool ok = mm.Init(MultiMonitorMode::PerMonitor);
  Check(ok, "false-success: Init still returns true (headless counts as live)");
  Check(mm.slot_count() == 2, "false-success: slot_count()==2");
  Check(mm.headless_slot_count() == 1,
        "false-success: headless_slot_count()==1 (only the non-covering slot)");
  Check(mm.has_headless_slots(),
        "false-success: has_headless_slots()==true");
  Check(LogContains("placement: RETRY-FALSE-SUCCESS"),
        "false-success: log contains placement: RETRY-FALSE-SUCCESS");
  Check(mm.SlotCoverageReason(1) == "placement: OUT-OF-BOUNDS",
        "false-success: SlotCoverageReason(1) carries the honest OOB token");
  Check(mm.SlotCoverageReason(0) == "placement: covered",
        "false-success: SlotCoverageReason(0) stays 'placement: covered'");
  int headless_creates = 0;
  int injected_creates = 0;
  for (void* h : g_create_hwnds) {
    if (h == kHeadlessHost) {
      ++headless_creates;
    } else if (h != nullptr) {
      ++injected_creates;
    }
  }
  Check(headless_creates == 1 && injected_creates == 1,
        "false-success: one Create() got the hidden host (non-covering slot), "
        "one got the covering slot's injected hwnd");

  const int headless = mm.headless_slot_count();
  const bool live = mm.slot_count() > 0;
  std::printf("get_state: {\"state\":{\"headless_slots\":%d,\"live\":%s}}\n",
              headless, live ? "true" : "false");
  Check(headless == 1 && live,
        "false-success: get_state shape is headless_slots:1 live:true");

  mm.Shutdown();
  Check(mm.slot_count() == 0, "false-success: Shutdown clears all slots");
  for (const std::string& l : g_logs) {
    if (l.find("RETRY-FALSE-SUCCESS") != std::string::npos) {
      std::cout << "[LOG] " << l << "\n";
    }
  }
}

// Row 11 happy QA: two-monitor fixture, HOST rect changes ONLY, every
// monitor rect untouched (Explorer restart / DPI change shape). The row-11
// regression: OnDisplayChange used to compare only the monitor rect against
// Slot.info and skipped the re-attach when unchanged - children stayed
// parented to a stale host client origin. Assert the stub injectors'
// OnDisplayChange is re-invoked (it would NOT be on the pre-row-11 code).
void TestHostMoveTriggersReattach() {
  ResetStubs();
  g_attach_ok = true;
  MultiMonitor mm(&QuietLog, MakeRecordingFactory());
  mm.SetHeadlessHost(kHeadlessHost);
  const bool ok = mm.Init(MultiMonitorMode::PerMonitor);
  Check(ok, "hostmove: Init(PerMonitor) returns true");
  Check(mm.slot_count() == 2, "hostmove: slot_count()==2 after Init");
  bool baseline = g_inj.size() == 2;
  for (const auto& kv : g_inj) {
    baseline = baseline && kv.second.attach_calls == 1 &&
               kv.second.on_display_change_calls == 0;
  }
  Check(baseline,
        "hostmove: baseline attach_calls==1 / on_display_change_calls==0");

  g_resolve_result = MakeMovedHost();
  mm.OnDisplayChange();

  Check(mm.slot_count() == 2, "hostmove: OnDisplayChange keeps both slots");
  bool reattached = g_inj.size() == 2;
  int odc_total = 0;
  for (const auto& kv : g_inj) {
    reattached = reattached && kv.second.on_display_change_calls == 1;
    odc_total += kv.second.on_display_change_calls;
  }
  Check(reattached,
        "hostmove: host-rect-only change re-invoked OnDisplayChange on BOTH "
        "injectors (regression: pre-row-11 code skips when the monitor rect "
        "is unchanged)");
  Check(odc_total == 2,
        "hostmove: exactly one re-attach per survivor (no storm, no skip)");
  Check(mm.slots().count(0) == 1 && mm.slots().count(1) == 1,
        "hostmove: both monitor slots still live (survivors, not re-created)");
  bool shared_new = g_inj.size() == 2;
  for (const auto& kv : g_inj) {
    shared_new = shared_new && kv.second.shared_set &&
                 kv.second.shared.host == g_resolve_result.host &&
                 kv.second.shared.client_rect.left ==
                     g_resolve_result.client_rect.left &&
                 kv.second.shared.client_rect.top ==
                     g_resolve_result.client_rect.top &&
                 kv.second.shared.client_rect.right ==
                     g_resolve_result.client_rect.right &&
                 kv.second.shared.client_rect.bottom ==
                     g_resolve_result.client_rect.bottom;
  }
  Check(shared_new,
        "hostmove: injectors received the refreshed shared host (client_rect "
        "B) before re-attach");
  bool odc_path = g_inj.size() == 2;
  for (const auto& kv : g_inj) {
    odc_path = odc_path && kv.second.attach_calls == 1 &&
               kv.second.on_display_change_calls == 1;
  }
  Check(odc_path,
        "hostmove: re-place went through injector->OnDisplayChange "
        "(attach_calls stays 1, not Detach+Attach)");
  mm.Shutdown();
  Check(mm.slot_count() == 0, "hostmove: Shutdown clears all slots");
}

// Row 11 companion: monitor-only path unchanged by the fix - a monitor rect
// change with an unchanged host rect re-invokes ONLY the affected slot's
// OnDisplayChange, carrying the NEW monitor rect.
void TestMonitorOnlyReattachUnchanged() {
  ResetStubs();
  g_attach_ok = true;
  MultiMonitor mm(&QuietLog, MakeRecordingFactory());
  mm.SetHeadlessHost(kHeadlessHost);
  const bool ok = mm.Init(MultiMonitorMode::PerMonitor);
  Check(ok, "mononly: Init(PerMonitor) returns true");
  g_monitors[0].height = 1440;
  mm.OnDisplayChange();
  const InjState* mon0 = nullptr;
  const InjState* mon1 = nullptr;
  for (const auto& kv : g_inj) {
    if (kv.second.att_x == 0 && kv.second.att_y == 0 && kv.second.att_w == 1920) {
      mon0 = &kv.second;
    } else {
      mon1 = &kv.second;
    }
  }
  Check(mon0 != nullptr && mon1 != nullptr,
        "mononly: both injectors identified by their Init attach rect");
  Check(mon0 != nullptr && mon0->on_display_change_calls == 1 &&
            mon0->odc_x == 0 && mon0->odc_y == 0 && mon0->odc_w == 1920 &&
            mon0->odc_h == 1440,
        "mononly: monitor 0 got OnDisplayChange with the NEW rect (0,0,1920x1440)");
  Check(mon1 != nullptr && mon1->on_display_change_calls == 0,
        "mononly: monitor 1 (unchanged rect) got NO re-attach");
  Check(mon0 != nullptr && mon0->attach_calls == 1 && mon1 != nullptr &&
            mon1->attach_calls == 1,
        "mononly: monitor-only path kept attach_calls==1 "
        "(OnDisplayChange, not Detach+Attach)");
  g_monitors[0].height = 1080;
  mm.Shutdown();
}

// Row 11 guard: neither geometry nor host moved -> NO re-attach. Locks the
// skip path so the host-move fix can never degrade into unconditional re-placing.
void TestNoGeometryOrHostChangeSkips() {
  ResetStubs();
  g_attach_ok = true;
  MultiMonitor mm(&QuietLog, MakeRecordingFactory());
  mm.SetHeadlessHost(kHeadlessHost);
  const bool ok = mm.Init(MultiMonitorMode::PerMonitor);
  Check(ok, "nochange: Init(PerMonitor) returns true");
  mm.OnDisplayChange();
  bool none = g_inj.size() == 2;
  int odc = 0;
  for (const auto& kv : g_inj) {
    none = none && kv.second.on_display_change_calls == 0;
    odc += kv.second.on_display_change_calls;
  }
  Check(none, "nochange: unchanged monitor+host rect skips re-attach entirely");
  Check(odc == 0, "nochange: on_display_change_calls stays 0");
  mm.Shutdown();
}

// Row 11 span host-move: identical virtual-screen geometry, HOST rect moved
// only. The span slot must re-attach Detach+Attach style (Reanchor's
// ReattachSpanLocked funnel) against the refreshed shared host - not via
// injector->OnDisplayChange, and not skipped.
void TestSpanHostMoveReattaches() {
  ResetStubs();
  static SpanGeometry fake{0, 0, 1920, 1080};
  MultiMonitor::SetSpanGeometryOverride(&fake);
  g_attach_ok = true;
  MultiMonitor mm(&QuietLog, MakeRecordingFactory());
  mm.SetHeadlessHost(kHeadlessHost);
  const bool ok = mm.Init(MultiMonitorMode::Span);
  Check(ok, "spanhost: Init(Span) returns true");
  Check(mm.slot_count() == 1, "spanhost: slot_count()==1 after Init");
  const InjState* inj = nullptr;
  for (const auto& kv : g_inj) inj = &kv.second;
  Check(inj != nullptr && inj->attach_calls == 1 &&
            inj->on_display_change_calls == 0,
        "spanhost: baseline one Attach, zero OnDisplayChange");

  g_resolve_result = MakeMovedHost();
  mm.OnDisplayChange();

  Check(mm.slot_count() == 1, "spanhost: span slot survives");
  Check(inj != nullptr && inj->attach_calls == 2,
        "spanhost: host-move re-attached via Detach+Attach (attach_calls==2)");
  Check(inj != nullptr && inj->on_display_change_calls == 0,
        "spanhost: span re-attach did NOT go through injector->OnDisplayChange");
  Check(inj != nullptr && inj->detach_calls >= 1,
        "spanhost: Detach ran before the re-Attach");
  Check(inj != nullptr && inj->shared_set &&
            inj->shared.client_rect.left == 5 &&
            inj->shared.client_rect.right == 1925,
        "spanhost: span re-attach received the refreshed host client_rect B");
  MultiMonitor::SetSpanGeometryOverride(nullptr);
  mm.Shutdown();
}

// Row 11 failure QA: virtual-screen query yields non-positive dimensions
// (SetSpanGeometryOverride {0,0,0,0}) AND the resolved host client_rect is
// non-positive too. OnDisplayChange must not throw (catch at OnDisplayChange)
// and ReattachSpanLocked must still fall back to the primary resolution
// (same policy as AttachSpanSlot's virtual-screen fallback).
void TestSpanNonPositiveGeometryPrimaryFallback() {
  ResetStubs();
  static SpanGeometry zero{0, 0, 0, 0};
  MultiMonitor::SetSpanGeometryOverride(&zero);
  SharedHost broken = MakeStubSharedHost();
  broken.client_rect = {0, 0, 0, 0};
  g_resolve_result = broken;
  g_logs.clear();
  const MonitorInfo primary = GetPrimaryMonitor();

  MultiMonitor mm(&RecordLog, MakeRecordingFactory());
  mm.SetHeadlessHost(kHeadlessHost);
  const bool ok = mm.Init(MultiMonitorMode::Span);
  Check(ok, "spanfail: Init(Span) returns true with non-positive span geometry");
  Check(mm.slot_count() == 1, "spanfail: one span slot after primary fallback");
  const InjState* inj = nullptr;
  for (const auto& kv : g_inj) inj = &kv.second;
  Check(inj != nullptr && inj->att_x == primary.x && inj->att_y == primary.y &&
            inj->att_w == primary.width && inj->att_h == primary.height,
        "spanfail: Init fell back to primary resolution "
        "(AttachSpanSlot virtual-screen fallback)");

  broken.client_rect = {1, 1, 0, 0};
  g_resolve_result = broken;
  bool threw = false;
  try {
    mm.OnDisplayChange();
  } catch (...) {
    threw = true;
  }
  Check(!threw, "spanfail: OnDisplayChange did not throw (catch swallows)");
  Check(mm.slot_count() == 1, "spanfail: span slot still live");
  Check(inj != nullptr && inj->attach_calls == 2,
        "spanfail: re-attach attempted after the host-rect change");
  Check(inj != nullptr && inj->att_x == primary.x && inj->att_y == primary.y &&
            inj->att_w == primary.width && inj->att_h == primary.height,
        "spanfail: ReattachSpanLocked fell back to primary resolution "
        "(non-positive w/h -> GetPrimaryMonitor)");
  Check(inj != nullptr && inj->shared_set && inj->shared.host == broken.host,
        "spanfail: refreshed shared host was still passed down");
  MultiMonitor::SetSpanGeometryOverride(nullptr);
  mm.Shutdown();
}

}  // namespace k6wp

int main(int argc, char** argv) {
  k6wp::SetupMonitors();
  const std::string sel = argc > 1 ? argv[1] : "all";
  if (sel == "all" || sel == "happy") {
    k6wp::TestHappyInjectedFactory();
  }
  if (sel == "all" || sel == "default") {
    k6wp::TestDefaultFactoryProductionShape();
  }
  if (sel == "all" || sel == "headless") {
    k6wp::TestHeadlessFallbackContract();
  }
  if (sel == "all" || sel == "nullhost") {
    k6wp::TestNullSharedHostDegradesHeadless();
  }
  if (sel == "all" || sel == "false-success") {
    k6wp::TestFalseSuccessHeadless();
  }
  if (sel == "all" || sel == "hostmove") {
    k6wp::TestHostMoveTriggersReattach();
  }
  if (sel == "all" || sel == "mononly") {
    k6wp::TestMonitorOnlyReattachUnchanged();
  }
  if (sel == "all" || sel == "nochange") {
    k6wp::TestNoGeometryOrHostChangeSkips();
  }
  if (sel == "all" || sel == "spanhost") {
    k6wp::TestSpanHostMoveReattaches();
  }
  if (sel == "all" || sel == "spanfail") {
    k6wp::TestSpanNonPositiveGeometryPrimaryFallback();
  }
  std::cout << k6wp::g_checks << " checks, " << k6wp::g_failures
            << " failures\n";
  return k6wp::g_failures == 0 ? 0 : 1;
}
