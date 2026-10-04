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
  void* hwnd = nullptr;  // injected_hwnd() while "attached"
  int attach_calls = 0;
  int detach_calls = 0;
  int reassert_calls = 0;
  int on_display_change_calls = 0;
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

// Sentinel passed to SetHeadlessHost: must be the hwnd Create() receives
// when injection failed, and must never appear when injection succeeded.
void* const kHeadlessHost = reinterpret_cast<void*>(static_cast<uintptr_t>(0x77));

void ResetStubs() {
  g_inj.clear();
  g_ren.clear();
  g_events.clear();
  g_attach_rects.clear();
  g_create_hwnds.clear();
  g_attach_ok = true;
  g_inj_ctors = g_inj_dtors = 0;
  g_ren_ctors = g_ren_dtors = 0;
  g_factory_injectors = g_factory_renderers = 0;
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
  s.hwnd = g_attach_ok ? NextStubHwnd() : nullptr;
  Rect4 r;
  r.x = x;
  r.y = y;
  r.w = width;
  r.h = height;
  g_attach_rects.push_back(r);
  g_events.push_back("attach");
  return g_attach_ok;
}

void DesktopInjector::Detach() {
  InjState& s = g_inj[this];
  s.hwnd = nullptr;
  ++s.detach_calls;
  g_events.push_back("detach");
}

void DesktopInjector::OnDisplayChange(int /*x*/, int /*y*/, int /*width*/,
                                      int /*height*/) {
  ++g_inj[this].on_display_change_calls;
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
// assertion above stays purely about slot construction.
std::string DesktopInjector::last_coverage_reason() const { return {}; }

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
  std::cout << k6wp::g_checks << " checks, " << k6wp::g_failures
            << " failures\n";
  return k6wp::g_failures == 0 ? 0 : 1;
}
