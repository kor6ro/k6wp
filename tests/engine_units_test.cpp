// engine_units_test.cpp - pure-logic coverage for the engine units split out
// of engine_app.cpp: cli_options (argv parsing), PauseController (owner
// bitmask), PendingCommandQueue (worker->main handoff), TestSimulator
// (hidden QA flag schedule) and (row 16) ConfigWatcher's second-file
// (displays.json) mtime/size + reload-callback path. No windows.h, no mpv,
// no engine_app.

#include "cli_options.hpp"
#include "config_watch.hpp"
#include "pause_controller.hpp"
#include "pending_queue.hpp"
#include "test_simulator.hpp"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <stdlib.h>
#include <string>
#include <vector>

namespace {

int g_failures = 0;

void Check(bool ok, const char* what) {
  std::printf("%s %s\n", ok ? "PASS" : "FAIL", what);
  if (!ok) {
    ++g_failures;
  }
}

int RunCli(std::initializer_list<const char*> args, k6wp::CliOptions& out) {
  std::vector<char*> argv;
  argv.reserve(args.size());
  for (const char* a : args) {
    argv.push_back(const_cast<char*>(a));
  }
  return k6wp::ParseCli(static_cast<int>(argv.size()), argv.data(), out);
}

void TestCliOptions() {
  k6wp::CliOptions out;

  out = k6wp::CliOptions{};
  Check(RunCli({"engine.exe", "--video", "C:/v.mp4"}, out) == 0 &&
            out.video_path == std::filesystem::u8path("C:/v.mp4").wstring(),
        "cli: --video value");

  out = k6wp::CliOptions{};
  Check(RunCli({"engine.exe", "--config=C:/c.json"}, out) == 0 &&
            out.config_path == std::filesystem::u8path("C:/c.json").wstring(),
        "cli: --config=value inline form");

  out = k6wp::CliOptions{};
  Check(RunCli({"engine.exe", "--wallpaper-mode", "workerw"}, out) == 0 &&
            out.wallpaper_mode == k6wp::WallpaperMode::kWorkerW &&
            std::string(k6wp::WallpaperModeToString(out.wallpaper_mode)) ==
                "workerw",
        "cli: --wallpaper-mode workerw");

  out = k6wp::CliOptions{};
  Check(RunCli({"engine.exe", "--wallpaper-mode", "progman"}, out) == 0 &&
            out.wallpaper_mode == k6wp::WallpaperMode::kProgman,
        "cli: --wallpaper-mode progman");

  out = k6wp::CliOptions{};
  Check(RunCli({"engine.exe", "--wallpaper-mode", "bogus"}, out) == 2,
        "cli: unknown --wallpaper-mode rejected");

  out = k6wp::CliOptions{};
  Check(RunCli({"engine.exe", "--minimized"}, out) == 0 && out.minimized,
        "cli: --minimized sets flag");

  out = k6wp::CliOptions{};
  Check(RunCli({"engine.exe", "--engine"}, out) == 0 && out.minimized,
        "cli: --engine alias");

  out = k6wp::CliOptions{};
  Check(RunCli({"engine.exe", "--silent"}, out) == 0 && out.minimized,
        "cli: --silent alias");

  out = k6wp::CliOptions{};
  Check(RunCli({"engine.exe", "--restarted"}, out) == 0 && !out.minimized,
        "cli: --restarted accepted and ignored");

  out = k6wp::CliOptions{};
  Check(RunCli({"engine.exe", "--minimized=1"}, out) == 2,
        "cli: --minimized rejects a value");

  out = k6wp::CliOptions{};
  Check(RunCli({"engine.exe", "--exit-after-ms", "5000"}, out) == 0 &&
            out.exit_after_ms == 5000,
        "cli: --exit-after-ms value");

  out = k6wp::CliOptions{};
  Check(RunCli({"engine.exe", "--exit-after-ms", "-1"}, out) == 2,
        "cli: negative --exit-after-ms rejected");

  out = k6wp::CliOptions{};
  Check(RunCli({"engine.exe", "--exit-after-ms", "abc"}, out) == 2,
        "cli: non-integer --exit-after-ms rejected");

  out = k6wp::CliOptions{};
  Check(RunCli({"engine.exe", "--exit-after-ms"}, out) == 2,
        "cli: missing --exit-after-ms value rejected");

  out = k6wp::CliOptions{};
  Check(RunCli({"engine.exe", "--simulate-monitor-off-after-ms", "250"}, out) ==
            0 &&
            out.simulate_monitor_off_after_ms == 250,
        "cli: hidden --simulate-monitor-off-after-ms parsed");

  out = k6wp::CliOptions{};
  Check(RunCli({"engine.exe", "--bogus"}, out) == 2,
        "cli: unknown flag rejected");

  out = k6wp::CliOptions{};
  Check(RunCli({"engine.exe", "--help"}, out) == 1, "cli: --help returns 1");

  out = k6wp::CliOptions{};
  Check(RunCli({"engine.exe", "positional"}, out) == 0,
        "cli: bare positional ignored with a warning");
}

void TestPauseController() {
  k6wp::PauseController pc;
  Check(!pc.UiPaused() && !pc.SlotsPaused() && pc.mask() == 0,
        "pause: fresh mask is zero");

  Check(pc.SetBit(k6wp::PauseController::kUser, true),
        "pause: user bit first set reports change");
  Check(pc.UiPaused() && pc.SlotsPaused(), "pause: user pauses UI and slots");
  Check(!pc.SetBit(k6wp::PauseController::kUser, true),
        "pause: redundant set reports no change");
  Check(pc.SetBit(k6wp::PauseController::kUser, false),
        "pause: clearing user reports change");
  Check(!pc.UiPaused() && !pc.SlotsPaused(), "pause: cleared mask is zero");

  Check(pc.SetBit(k6wp::PauseController::kPower, true),
        "pause: power bit set");
  Check(!pc.UiPaused() && pc.SlotsPaused(),
        "pause: power pauses slots but not UI");

  Check(pc.SetBit(k6wp::PauseController::kScreenOff, true),
        "pause: screen-off bit set");
  Check(pc.UiPaused(), "pause: screen-off pauses UI");
  Check(pc.mask() == (k6wp::PauseController::kPower |
                      k6wp::PauseController::kScreenOff),
        "pause: merged mask has both bits");
}

void TestPendingQueue() {
  k6wp::PendingCommandQueue q;
  std::string out;

  Check(!q.TakeVideo(&out), "queue: take on empty video returns false");
  Check(!q.TakeMonitor(&out), "queue: take on empty monitor returns false");

  q.SetVideo("a");
  Check(q.TakeVideo(&out) && out == "a", "queue: set then take video");
  Check(!q.TakeVideo(&out), "queue: take consumes video");

  q.SetVideo("x");
  q.SetVideo("y");
  Check(q.TakeVideo(&out) && out == "y", "queue: last write wins");

  q.SetVideo("z");
  Check(q.TakeVideo(nullptr), "queue: take with null out still consumes");
  Check(!q.TakeVideo(&out), "queue: consumed after null take");

  q.SetVideo("keep");
  q.ClearVideo();
  Check(!q.TakeVideo(&out), "queue: clear discards video");

  q.SetMonitor("m1");
  Check(q.TakeMonitor(&out) && out == "m1", "queue: set then take monitor");
  q.SetMonitor("m2");
  q.ClearMonitor();
  Check(!q.TakeMonitor(&out), "queue: clear discards monitor");
}

void TestSimulator() {
  k6wp::CliOptions opts;
  k6wp::TestSimulator sim;

  sim.Configure(opts);
  Check(!sim.AnyArmed(), "sim: nothing armed by default");
  Check(!sim.ExitReached(100), "sim: exit not reached when unset");

  opts.exit_after_ms = 1000;
  sim.Configure(opts);
  Check(sim.AnyArmed(), "sim: exit after ms arms the sim");
  Check(!sim.ExitReached(999) && sim.ExitReached(1000),
        "sim: exit threshold is inclusive");

  k6wp::CliOptions device;
  device.simulate_device_lost_after_ms = 100;
  sim.Configure(device);
  Check(!sim.Tick(50).device_lost, "sim: device-lost not before deadline");
  Check(sim.Tick(100).device_lost, "sim: device-lost fires once");
  Check(!sim.Tick(200).device_lost, "sim: device-lost does not re-fire");

  k6wp::CliOptions suspend;
  suspend.simulate_suspend_after_ms = 100;
  sim.Configure(suspend);
  Check(sim.Tick(100).suspend, "sim: suspend fires at deadline");
  Check(!sim.Tick(2099).resume, "sim: resume not before +2000ms");
  Check(sim.Tick(2100).resume, "sim: resume fires at +2000ms");

  k6wp::CliOptions dc;
  dc.simulate_dc_after_ms = 100;
  sim.Configure(dc);
  const auto dc_on = sim.Tick(100);
  Check(dc_on.dc_on && sim.dc_latched(), "sim: forced DC latches on");
  const auto dc_off = sim.Tick(3000);
  Check(dc_off.dc_restore && !sim.dc_latched(), "sim: forced DC clears at +2000ms");

  k6wp::CliOptions mon;
  mon.simulate_monitor_off_after_ms = 100;
  sim.Configure(mon);
  Check(sim.Tick(100).monitor_off, "sim: monitor-off fires at deadline");
  Check(sim.Tick(3000).monitor_on, "sim: monitor-on fires at +2000ms");
}

// Row 16: drives ConfigWatcher's second-file (displays.json) change detection
// through the REAL watcher with no Win32 message window — an mtime/size change
// must fire the registered reload callback, an unchanged stat must not, and
// Stop() must tear the registration down. The stat path is deterministic
// (size delta), so the check cannot flake on mtime granularity.
void TestDisplaysSecondFileWatch() {
  namespace fs = std::filesystem;
  std::error_code ec;
  const fs::path dir =
      fs::temp_directory_path(ec) / "k6wp-engine-units-row16";
  if (ec) {
    Check(false, "watch2: temp directory available");
    return;
  }
  fs::remove_all(dir, ec);
  fs::create_directories(dir, ec);

  // Route the watcher's diagnostic log into the throwaway dir: the suite must
  // never append to a real %LOCALAPPDATA%\K6WP\engine.log. log_file.cpp opens
  // it lazily via GetEnvironmentVariableW, and _wputenv_s keeps the CRT and
  // Win32 environment blocks in sync.
  _wputenv_s(L"LOCALAPPDATA", dir.wstring().c_str());

  const fs::path displays = dir / "displays.json";
  k6wp::ConfigWatcher watcher;  // no Start(): no Win32 handles are opened
  int second_calls = 0;
  watcher.WatchSecondFile(displays, [&second_calls]() { ++second_calls; });

  Check(second_calls == 0,
        "watch2: registration does not invoke the reload callback");

  {
    std::ofstream out(displays, std::ios::binary | std::ios::trunc);
    out << "{\"version\":1,\"assignments\":{}}";
  }
  Check(watcher.CheckSecondForChange(),
        "watch2: creating displays.json is detected as a change");
  Check(second_calls == 1, "watch2: first change fires the reload callback");

  Check(!watcher.CheckSecondForChange(),
        "watch2: unchanged mtime/size is a no-op");
  Check(second_calls == 1, "watch2: no redundant reload callback");

  {
    std::ofstream out(displays, std::ios::binary | std::ios::app);
    out << " ";
  }
  Check(watcher.CheckSecondForChange(),
        "watch2: size delta is detected as a change");
  Check(second_calls == 2, "watch2: recovery write fires the callback again");

  watcher.Stop();
  Check(!watcher.CheckSecondForChange(),
        "watch2: Stop() clears the second-file registration");
  Check(second_calls == 2, "watch2: no callback after Stop()");

  fs::remove_all(dir, ec);
}

}  // namespace

int main() {
  TestCliOptions();
  TestPauseController();
  TestPendingQueue();
  TestSimulator();
  TestDisplaysSecondFileWatch();

  std::printf(g_failures == 0 ? "RESULT: ALL ENGINE-UNIT CHECKS PASSED\n"
                              : "RESULT: %d CHECK(S) FAILED\n",
              g_failures);
  return g_failures == 0 ? 0 : 1;
}
