// engine_state_test.cpp - unit tests for the components extracted from
// EngineApp: PinVerifySchedule (deadline + revert counter) and
// PlaylistController (reload + snapshot + one-shot rotation) with fake hooks.

#include "occlusion_poke_scheduler.hpp"
#include "pin_verify_schedule.hpp"
#include "playlist_controller.hpp"

#include "playlist.hpp"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

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

}  // namespace

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

  std::printf(g_failures == 0 ? "RESULT: ALL ENGINE-STATE CHECKS PASSED\n"
                              : "RESULT: %d CHECK(S) FAILED\n",
              g_failures);
  return g_failures == 0 ? 0 : 1;
}
