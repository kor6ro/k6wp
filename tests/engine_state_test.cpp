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

  std::printf(g_failures == 0 ? "RESULT: ALL ENGINE-STATE CHECKS PASSED\n"
                              : "RESULT: %d CHECK(S) FAILED\n",
              g_failures);
  return g_failures == 0 ? 0 : 1;
}
