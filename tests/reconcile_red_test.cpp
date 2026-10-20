// reconcile_red_test.cpp - Engine Display Resilience, P0/Fase 4.
//
// ORIGINALLY LABELED RED tests: one failing characterization test per legacy
// bug B1..B4 (prompt v5.1 §5.1a). Each test is a self-contained scenario
// that encodes the POST-FIX contract; on the legacy MultiMonitor it FAILED
// at exactly ONE intended assertion.
//
// THEY ARE GREEN NOW. The Bug B fix (stable monitor identity for the slot
// map, see engine/src/reconcile_policy.hpp) makes the slot follow the
// MONITOR instead of the position index, and the teardown now retains the
// per-monitor assignment so a re-plugged monitor gets it back. The four
// scenarios are kept as REGRESSION tests: they must keep passing.
//
//   red-B1-swap-rect          positions swapped AND primary moves ->
//                             the sorted id<->device mapping permutes; a
//                             device must keep its own assigned video.
//   red-B2-unplug-reindex     the primary is unplugged; the survivor is
//                             re-indexed to id0 and must keep ITS video.
//   red-B3-secondary-key-change a secondary is replaced by a different
//                             device at the same id; the newcomer must not
//                             inherit the replaced device's video.
//   red-B4-replug-rebind      a monitor is unplugged then replugged; its
//                             assignment must be re-applied to the fresh
//                             renderer.
//
// ROOT CAUSE (validated against the legacy source): ListMonitors()
// (shared/monitor_util.cpp:188-197) sorts primary-first then by device name
// and sets id = sorted index; MultiMonitor keyed slots_ by that id
// (multi_monitor.cpp). OnDisplayChange matched survivors by id and refreshed
// their info, but never re-bound the renderer, so when the id<->device
// mapping shifted the wrong renderer/video survived.
//
// LINK MODEL (identical to tests/reconcile_golden_test.cpp /
// multi_monitor_factory_test.cpp): only engine/src/multi_monitor.cpp +
// src/desktop_placement.cpp + src/reconcile_policy.cpp +
// src/renderer_graveyard.cpp are compiled from the engine. DesktopInjector /
// MpvRenderer / ListMonitors / GetPrimaryMonitor / ResolveSharedHost are
// recording stubs in this TU, so the binary never links desktop_inject.cpp,
// mpv_renderer.cpp or mpv.lib.
//
// No external test framework: plain Check() counter; exit 0 = pass,
// exit 1 = at least one failure.
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "multi_monitor.hpp"

namespace k6wp {

// Complete the Pimpl so the stubbed ~DesktopInjector can destroy its
// unique_ptr<Impl> member (desktop_inject.cpp is NOT linked).
struct DesktopInjector::Impl {};

namespace {

// ---- recording state (link-level stubs read this) -------------------------

struct InjState {
  void* hwnd = nullptr;
  bool shared_set = false;
};

std::map<const DesktopInjector*, InjState> g_inj;
// Per-renderer last accepted LoadLoop path (this is the "assignment" the
// red tests track). A renderer is destroyed when its slot is torn down, so
// its entry disappears with it.
std::map<const MpvRenderer*, std::string> g_path;
std::vector<MonitorInfo> g_monitors;
SharedHost g_resolve_host{};
int g_resolve_calls = 0;

void* const kHeadlessHost =
    reinterpret_cast<void*>(static_cast<uintptr_t>(0x77));

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

void ResetStubs() {
  g_inj.clear();
  g_path.clear();
  g_monitors.clear();
  g_resolve_calls = 0;
  SharedHost sh;
  sh.host = reinterpret_cast<void*>(static_cast<uintptr_t>(0xBEEF));
  sh.progman = reinterpret_cast<void*>(static_cast<uintptr_t>(0xFEED));
  sh.def_view = reinterpret_cast<void*>(static_cast<uintptr_t>(0xDEA1));
  sh.insert_after = sh.def_view;
  sh.layered = true;
  sh.branch = "24H2 path";
  sh.client_rect.left = 0;
  sh.client_rect.top = 0;
  sh.client_rect.right = 3840;
  sh.client_rect.bottom = 1080;
  g_resolve_host = sh;
}

SlotFactory MakeFactory() {
  SlotFactory f;
  f.make_injector = [](LogFn log) {
    return std::make_unique<DesktopInjector>(log);
  };
  f.make_renderer = []() { return std::make_unique<MpvRenderer>(); };
  return f;
}

// Last LoadLoop path recorded for the live slot whose GDI device name equals
// `device`; empty when no slot matches or the renderer never loaded a video.
std::string LoadedPathForDevice(const MultiMonitor& mm,
                                const std::wstring& device) {
  for (const auto& kv : mm.slots()) {
    if (kv.second.info.device_name == device && kv.second.renderer) {
      const auto it = g_path.find(kv.second.renderer.get());
      if (it != g_path.end()) return it->second;
    }
  }
  return {};
}

// Two side-by-side monitors: DISPLAY1 primary at (0,0), DISPLAY2 at
// (1920,0). Mirrors ListMonitors()' primary-first then device-name sort.
void TwoMonitors() {
  g_monitors = {
      MakeMon(0, 0, 0, 1920, 1080, true, L"\\\\.\\DISPLAY1"),
      MakeMon(1, 1920, 0, 1920, 1080, false, L"\\\\.\\DISPLAY2"),
  };
}

const char* const kA = "A.mp4";
const char* const kB = "B.mp4";

// The assignment re-apply checks that the path EXISTS on disk (a missing
// file must never be re-applied), so the fixtures are real files in a
// throwaway temp directory.
std::filesystem::path g_fixture_dir;

void MakeFixtureFiles() {
  if (!g_fixture_dir.empty()) return;
  std::error_code ec;
  g_fixture_dir =
      std::filesystem::temp_directory_path(ec) / "k6wp-reconcile-fixture";
  std::filesystem::create_directories(g_fixture_dir, ec);
  for (const char* name : {kA, kB}) {
    std::ofstream out(g_fixture_dir / name, std::ios::binary);
    out << "fixture";
  }
}

std::string FixturePath(const char* name) {
  MakeFixtureFiles();
  return (g_fixture_dir / name).string();
}

void CleanupFixtureFiles() {
  if (g_fixture_dir.empty()) return;
  std::error_code ec;
  std::filesystem::remove_all(g_fixture_dir, ec);
  g_fixture_dir.clear();
}

}  // namespace (recording state + harness)

// ---- link-level stubs: DesktopInjector (desktop_inject.cpp NOT linked) ----

DesktopInjector::DesktopInjector(LogFn /*log*/) { g_inj[this] = InjState{}; }

DesktopInjector::~DesktopInjector() { g_inj.erase(this); }

void DesktopInjector::SetInjectMode(InjectMode /*mode*/) {}

void DesktopInjector::SetSharedHost(const SharedHost& host) {
  g_inj[this].shared_set = host.host != nullptr;
}

bool DesktopInjector::Attach(int /*x*/, int /*y*/, int /*width*/,
                             int /*height*/) {
  InjState& s = g_inj[this];
  const bool ok = s.shared_set && g_resolve_host.host != nullptr;
  s.hwnd = ok ? reinterpret_cast<void*>(static_cast<uintptr_t>(0x2000))
              : nullptr;
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
  return "placement: covered";
}

SharedHost ResolveSharedHost(InjectMode /*mode*/, LogFn /*log*/) {
  ++g_resolve_calls;
  return g_resolve_host;
}

// ---- link-level stubs: MpvRenderer (mpv_renderer.cpp/mpv.lib NOT linked) --

MpvRenderer::MpvRenderer() {}

// Bug B: the renderer teardown runs on the RENDERER GRAVEYARD worker, so
// this destructor must not mutate the shared recording map (it would race
// with the test thread reading it). The entry of a dead renderer is
// harmless: the lookups are keyed by the LIVE renderer's pointer.
MpvRenderer::~MpvRenderer() {}

bool MpvRenderer::Create(void* /*hwnd*/) { return true; }

void MpvRenderer::SetHWND(void* /*hwnd*/) {}

// Bug A: the live-frame capture (mpv screenshot-to-file). The stub refuses
// the capture, so the lock-screen path falls back to the file-based extract.
bool MpvRenderer::ScreenshotToFile(const std::string& /*path*/) { return false; }
bool MpvRenderer::SeekTo(double /*seconds*/) { return false; }

void MpvRenderer::SetAdapterPin(const std::string& /*substr*/) {}

bool MpvRenderer::pin_active() const { return false; }

bool MpvRenderer::LoadLoop(const std::string& path, bool /*force*/) {
  // Stub: do not touch the filesystem (the fixtures are fictitious paths).
  if (path.empty()) return false;
  g_path[this] = path;
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
// red-B1-swap-rect: positions swapped AND the primary moves to the other
// device. The sorted id<->device mapping permutes (id0 was DISPLAY1, now
// DISPLAY2). Each device must keep its OWN assigned video; legacy keeps the
// renderer bound to the slot id, so the videos cross over.
// ---------------------------------------------------------------------------
void TestRedB1SwapRect() {
  ResetStubs();
  TwoMonitors();
  const std::string path_a = FixturePath(kA);
  const std::string path_b = FixturePath(kB);
  MultiMonitor mm(&QuietLog, MakeFactory());
  mm.SetHeadlessHost(kHeadlessHost);
  Check(mm.Init(MultiMonitorMode::PerMonitor),
        "red-B1: Init attaches both monitors");
  Check(mm.slot_count() == 2, "red-B1: slot_count()==2 before the swap");
  Check(mm.LoadLoopSlot("\\\\.\\DISPLAY1", path_a, ""),
        "red-B1: DISPLAY1 accepts A.mp4");
  Check(mm.LoadLoopSlot("\\\\.\\DISPLAY2", path_b, ""),
        "red-B1: DISPLAY2 accepts B.mp4");
  Check(LoadedPathForDevice(mm, L"\\\\.\\DISPLAY1") == path_a &&
            LoadedPathForDevice(mm, L"\\\\.\\DISPLAY2") == path_b,
        "red-B1: precondition - each device carries its own video");

  // Swap the physical positions and move primary to DISPLAY2.
  g_monitors = {
      MakeMon(0, 1920, 0, 1920, 1080, true, L"\\\\.\\DISPLAY2"),
      MakeMon(1, 0, 0, 1920, 1080, false, L"\\\\.\\DISPLAY1"),
  };
  mm.OnDisplayChange();
  Check(mm.slot_count() == 2, "red-B1: slot_count()==2 after the swap");

  Check(LoadedPathForDevice(mm, L"\\\\.\\DISPLAY1") == path_a &&
            LoadedPathForDevice(mm, L"\\\\.\\DISPLAY2") == path_b,
        "red-B1-swap-rect: each device keeps its own assignment across a "
        "position/primary swap (assignment must follow the GDI device, not "
        "the slot index)");
  mm.Shutdown();
}

// ---------------------------------------------------------------------------
// red-B2-unplug-reindex: the PRIMARY (DISPLAY1) is unplugged; the survivor
// (DISPLAY2) is re-indexed from id1 to id0 and must keep ITS video. Legacy
// promotes the surviving slot with DISPLAY1's renderer still bound.
// ---------------------------------------------------------------------------
void TestRedB2UnplugReindex() {
  ResetStubs();
  TwoMonitors();
  const std::string path_a = FixturePath(kA);
  const std::string path_b = FixturePath(kB);
  MultiMonitor mm(&QuietLog, MakeFactory());
  mm.SetHeadlessHost(kHeadlessHost);
  Check(mm.Init(MultiMonitorMode::PerMonitor),
        "red-B2: Init attaches both monitors");
  Check(mm.LoadLoopSlot("\\\\.\\DISPLAY1", path_a, ""),
        "red-B2: DISPLAY1 accepts A.mp4");
  Check(mm.LoadLoopSlot("\\\\.\\DISPLAY2", path_b, ""),
        "red-B2: DISPLAY2 accepts B.mp4");
  Check(LoadedPathForDevice(mm, L"\\\\.\\DISPLAY2") == path_b,
        "red-B2: precondition - DISPLAY2 carries B.mp4");

  // Unplug the primary: DISPLAY2 survives and becomes id0 (primary-first
  // sort). The survivor slot id0 still holds DISPLAY1's renderer/video.
  g_monitors = {
      MakeMon(0, 1920, 0, 1920, 1080, true, L"\\\\.\\DISPLAY2"),
  };
  mm.OnDisplayChange();
  Check(mm.slot_count() == 1,
        "red-B2: slot_count()==1 after unplugging the primary");
  Check(mm.HasAssignment("\\\\.\\DISPLAY2"),
        "red-B2: DISPLAY2 still has a live slot");

  Check(LoadedPathForDevice(mm, L"\\\\.\\DISPLAY2") == path_b,
        "red-B2-unplug-reindex: the surviving monitor keeps its own video "
        "after the primary is unplugged and ids shift (id is not device "
        "identity)");
  mm.Shutdown();
}

// ---------------------------------------------------------------------------
// red-B3-secondary-key-change: the secondary is replaced by a DIFFERENT
// device (DISPLAY3) at the same slot id and rect. The newcomer must NOT
// inherit the replaced device's video; legacy leaves the old renderer bound.
// ---------------------------------------------------------------------------
void TestRedB3SecondaryKeyChange() {
  ResetStubs();
  TwoMonitors();
  const std::string path_a = FixturePath(kA);
  const std::string path_b = FixturePath(kB);
  MultiMonitor mm(&QuietLog, MakeFactory());
  mm.SetHeadlessHost(kHeadlessHost);
  Check(mm.Init(MultiMonitorMode::PerMonitor),
        "red-B3: Init attaches both monitors");
  Check(mm.LoadLoopSlot("\\\\.\\DISPLAY1", path_a, ""),
        "red-B3: DISPLAY1 accepts A.mp4");
  Check(mm.LoadLoopSlot("\\\\.\\DISPLAY2", path_b, ""),
        "red-B3: DISPLAY2 accepts B.mp4");
  Check(LoadedPathForDevice(mm, L"\\\\.\\DISPLAY2") == path_b,
        "red-B3: precondition - DISPLAY2 carries B.mp4");

  // Same rect, same id, different GDI device: DISPLAY3 replaces DISPLAY2.
  g_monitors = {
      MakeMon(0, 0, 0, 1920, 1080, true, L"\\\\.\\DISPLAY1"),
      MakeMon(1, 1920, 0, 1920, 1080, false, L"\\\\.\\DISPLAY3"),
  };
  mm.OnDisplayChange();
  Check(mm.slot_count() == 2, "red-B3: slot_count()==2 after the change");
  Check(mm.HasAssignment("\\\\.\\DISPLAY3"),
        "red-B3: DISPLAY3 has a live slot at the old id");
  Check(LoadedPathForDevice(mm, L"\\\\.\\DISPLAY1") == path_a,
        "red-B3: the untouched primary keeps A.mp4");

  Check(LoadedPathForDevice(mm, L"\\\\.\\DISPLAY3") != path_b,
        "red-B3-secondary-key-change: a newly arrived device does not inherit "
        "the replaced device's assignment on the same slot id");
  mm.Shutdown();
}

// ---------------------------------------------------------------------------
// red-B4-replug-rebind: DISPLAY2 is unplugged then replugged at the same
// rect. Its assignment must be re-applied to the fresh renderer; legacy
// attaches an empty renderer and drops the video.
// ---------------------------------------------------------------------------
void TestRedB4ReplugRebind() {
  ResetStubs();
  TwoMonitors();
  const std::string path_a = FixturePath(kA);
  const std::string path_b = FixturePath(kB);
  MultiMonitor mm(&QuietLog, MakeFactory());
  mm.SetHeadlessHost(kHeadlessHost);
  Check(mm.Init(MultiMonitorMode::PerMonitor),
        "red-B4: Init attaches both monitors");
  Check(mm.LoadLoopSlot("\\\\.\\DISPLAY1", path_a, ""),
        "red-B4: DISPLAY1 accepts A.mp4");
  Check(mm.LoadLoopSlot("\\\\.\\DISPLAY2", path_b, ""),
        "red-B4: DISPLAY2 accepts B.mp4");
  Check(LoadedPathForDevice(mm, L"\\\\.\\DISPLAY2") == path_b,
        "red-B4: precondition - DISPLAY2 carries B.mp4");

  // Unplug DISPLAY2.
  g_monitors = {
      MakeMon(0, 0, 0, 1920, 1080, true, L"\\\\.\\DISPLAY1"),
  };
  mm.OnDisplayChange();
  Check(mm.slot_count() == 1, "red-B4: slot_count()==1 after unplugging DISPLAY2");
  Check(LoadedPathForDevice(mm, L"\\\\.\\DISPLAY1") == path_a,
        "red-B4: the primary keeps A.mp4 while alone");

  // Replug DISPLAY2 at the same rect.
  TwoMonitors();
  mm.OnDisplayChange();
  Check(mm.slot_count() == 2, "red-B4: slot_count()==2 after replugging DISPLAY2");
  Check(mm.HasAssignment("\\\\.\\DISPLAY2"),
        "red-B4: DISPLAY2 has a live slot after replug");

  Check(LoadedPathForDevice(mm, L"\\\\.\\DISPLAY2") == path_b,
        "red-B4-replug-rebind: a re-plugged monitor gets its assignment "
        "re-applied to the fresh renderer");
  mm.Shutdown();
}

}  // namespace k6wp

int main(int argc, char** argv) {
  const std::string sel = argc > 1 ? argv[1] : "all";
  if (sel == "all" || sel == "red-B1-swap-rect") {
    k6wp::TestRedB1SwapRect();
  }
  if (sel == "all" || sel == "red-B2-unplug-reindex") {
    k6wp::TestRedB2UnplugReindex();
  }
  if (sel == "all" || sel == "red-B3-secondary-key-change") {
    k6wp::TestRedB3SecondaryKeyChange();
  }
  if (sel == "all" || sel == "red-B4-replug-rebind") {
    k6wp::TestRedB4ReplugRebind();
  }
  std::cout << k6wp::g_checks << " checks, " << k6wp::g_failures
            << " failures\n";
  k6wp::CleanupFixtureFiles();
  return k6wp::g_failures == 0 ? 0 : 1;
}
