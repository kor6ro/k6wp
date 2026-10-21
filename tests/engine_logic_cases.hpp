// engine_logic_cases.hpp - the G5 golden fixtures for the engine's PURE logic.
//
// WHY THIS FILE EXISTS
//   Phase 3 replaces the C++ engine with a Rust one. The decisions below are
//   the ones a user can SEE: where the wallpaper lands on a multi-monitor
//   desktop, when a load is retried, when the engine gives up restarting
//   itself, which pause owner hides the picture, which host window may hold
//   our surface. They are pure (no clock, no Win32, no process), so a table
//   of inputs -> outputs is a complete specification of them.
//
//   The table lives in ONE place (this header) and is used by two programs:
//     - engine_logic_dump (writes the fixture, or checks it with --check);
//     - the Rust replayer (k6wp-core's engine_logic golden test).
//   So a change in the C++ logic shows up as a DRIFT in the C++ check AND as
//   a failure in the Rust replay.
//
// HOW A CASE IS EVALUATED
//   The evaluator calls the REAL production functions - never a re-implementation
//   - so the fixture records what the engine actually does.
//
// THE MODULES
//   desktop_placement - the coverage verdict of the surface on the monitor
//                       (the negative-origin and portrait cases are the ones
//                       that broke the placement before).
//   workerw_host      - which host window may hold our surface (task C7).
//   retry_schedule    - the load retry offsets (0/250/1000/3000 ms).
//   crash_restart_guard- the automatic-restart backoff (3 in 10 minutes).
//   pause_controller  - the pause mask semantics (the UI pause, the slot pause
//                       and the "this owner hides the wallpaper" rule that the
//                       unload state machine runs on).

#pragma once

#include <string>
#include <vector>

#include "crash_restart_guard.hpp"
#include "desktop_placement.hpp"
#include "pause_controller.hpp"
#include "pin_verify_schedule.hpp"
#include "reconcile_policy.hpp"
#include "retry_schedule.hpp"
#include "thirdparty/json.hpp"
#include "topology_diff.hpp"
#include "topology_snapshot.hpp"
#include "unload_state.hpp"

namespace k6wp_test {

using json = nlohmann::json;

struct EngineLogicCase {
  std::string module;
  std::string name;
  json input;
};

inline json Rect(int l, int t, int r, int b) {
  json j = json::object();
  j["left"] = l;
  j["top"] = t;
  j["right"] = r;
  j["bottom"] = b;
  return j;
}

// A monitor as the fixture writes it (the JSON mirrors TopologyMonitor).
inline json TopologyMonitorJson(const char* stable_key, int x, int y, int width,
                                 int height, int scale_pct, int orientation,
                                 bool is_primary) {
  json m = json::object();
  m["stable_key"] = stable_key;
  m["x"] = x;
  m["y"] = y;
  m["width"] = width;
  m["height"] = height;
  m["scale_pct"] = scale_pct;
  m["orientation"] = orientation;
  m["is_primary"] = is_primary;
  m["degraded"] = false;
  return m;
}

// A snapshot as the fixture writes it.
inline json TopologySnapshotJson(const std::vector<json>& monitors,
                                 const char* hash) {
  json s = json::object();
  s["monitors"] = monitors;
  s["virtual_x"] = 0;
  s["virtual_y"] = 0;
  s["virtual_width"] = 1920;
  s["virtual_height"] = 1080;
  s["hash_snapshot"] = hash;
  return s;
}

// A monitor as the fixture writes it for the reconcile policy (the JSON
// mirrors MonitorInfo - the wide device name becomes UTF-8).
inline json MonitorInfoJson(int id, const char* stable_key, const char* device,
                            int x, int y, int width, int height, bool primary) {
  json m = json::object();
  m["id"] = id;
  m["stable_key"] = stable_key;
  m["device_name"] = device;
  m["x"] = x;
  m["y"] = y;
  m["width"] = width;
  m["height"] = height;
  m["is_primary"] = primary;
  m["orientation"] = 0;
  m["refresh_hz"] = 60;
  m["scale_pct"] = 100;
  m["identity_degraded"] = false;
  return m;
}

inline k6wp::TopologyMonitor ParseTopologyMonitor(const json& m) {
  k6wp::TopologyMonitor out;
  out.stable_key = m["stable_key"].get<std::string>();
  out.x = m["x"].get<int>();
  out.y = m["y"].get<int>();
  out.width = m["width"].get<int>();
  out.height = m["height"].get<int>();
  out.scale_pct = m["scale_pct"].get<int>();
  out.orientation = m["orientation"].get<int>();
  out.is_primary = m["is_primary"].get<bool>();
  out.degraded = m["degraded"].get<bool>();
  return out;
}

inline k6wp::TopologySnapshot ParseTopologySnapshot(const json& s) {
  k6wp::TopologySnapshot out;
  for (const auto& m : s["monitors"]) {
    out.monitors.push_back(ParseTopologyMonitor(m));
  }
  out.virtual_x = s["virtual_x"].get<int>();
  out.virtual_y = s["virtual_y"].get<int>();
  out.virtual_width = s["virtual_width"].get<int>();
  out.virtual_height = s["virtual_height"].get<int>();
  out.hash_snapshot = s["hash_snapshot"].get<std::string>();
  return out;
}

inline std::vector<k6wp::MonitorInfo> ParseMonitors(const json& arr) {
  std::vector<k6wp::MonitorInfo> out;
  for (const auto& m : arr) {
    k6wp::MonitorInfo mi;
    mi.id = m["id"].get<int>();
    mi.stable_key = m["stable_key"].get<std::string>();
    // The wide device name: the fixture stores it as UTF-8 (the fixture's
    // device names are ASCII). The string must be kept alive - the
    // iterators below point into it.
    const std::string device_utf8 = m["device_name"].get<std::string>();
    mi.device_name =
        std::wstring(device_utf8.begin(), device_utf8.end());
    mi.x = m["x"].get<int>();
    mi.y = m["y"].get<int>();
    mi.width = m["width"].get<int>();
    mi.height = m["height"].get<int>();
    mi.is_primary = m["is_primary"].get<bool>();
    mi.orientation = m["orientation"].get<int>();
    mi.refresh_hz = m["refresh_hz"].get<int>();
    mi.scale_pct = m["scale_pct"].get<int>();
    mi.identity_degraded = m["identity_degraded"].get<bool>();
    out.push_back(mi);
  }
  return out;
}

// The fixed input table. The EXPECTED values are NOT here: they are what the
// production functions produce, and the fixture freezes them.
inline std::vector<EngineLogicCase> EngineLogicCases() {
  std::vector<EngineLogicCase> cases;
  using namespace k6wp;

  // --- desktop_placement ---------------------------------------------------
  {
    struct PlacementCase {
      const char* name;
      PlacementRect child;
      PlacementRect monitor;
    };
    const PlacementCase placements[] = {
        {"exact_cover", {0, 0, 1920, 1080}, {0, 0, 1920, 1080}},
        {"clipped_bottom", {0, 0, 1920, 1000}, {0, 0, 1920, 1080}},
        {"clipped_right", {0, 0, 1000, 1080}, {0, 0, 1920, 1080}},
        {"smaller_than_monitor", {100, 100, 200, 200}, {0, 0, 1920, 1080}},
        {"negative_origin", {-1920, 0, 0, 1080}, {-1920, 0, 0, 1080}},
        {"negative_origin_clipped", {-1920, 0, -100, 1080}, {-1920, 0, 0, 1080}},
        {"portrait", {0, 0, 1080, 1920}, {0, 0, 1080, 1920}},
        {"portrait_clipped", {0, 0, 1080, 1000}, {0, 0, 1080, 1920}},
        {"disjoint_right", {2000, 0, 3000, 1080}, {0, 0, 1920, 1080}},
    };
    for (const auto& p : placements) {
      EngineLogicCase c;
      c.module = "desktop_placement";
      c.name = p.name;
      c.input = json::object();
      c.input["child"] =
          Rect(p.child.left, p.child.top, p.child.right, p.child.bottom);
      c.input["monitor"] =
          Rect(p.monitor.left, p.monitor.top, p.monitor.right, p.monitor.bottom);
      cases.push_back(std::move(c));
    }
  }

  // --- workerw_host (task C7) ----------------------------------------------
  {
    const char* const kOwn = "K6WP.DesktopInject.1";
    struct HostCase {
      const char* name;
      std::vector<std::string> children;
    };
    const HostCase hosts[] = {
        {"empty", {}},
        {"own_only", {kOwn}},
        {"own_twice", {kOwn, kOwn}},
        {"foreign", {"SHELLDLL_DefView"}},
        {"own_and_foreign", {kOwn, "SHELLDLL_DefView"}},
        {"foreign_then_own", {"SHELLDLL_DefView", kOwn}},
    };
    for (const auto& h : hosts) {
      EngineLogicCase c;
      c.module = "workerw_host";
      c.name = h.name;
      c.input = json::object();
      c.input["child_classes"] = h.children;
      c.input["own_class"] = kOwn;
      cases.push_back(std::move(c));
    }
  }

  // --- retry_schedule --------------------------------------------------------
  {
    struct RetryCase {
      const char* name;
      long long now_ms;
      long long first_pass_ms;
      int attempts_done;
    };
    const RetryCase retries[] = {
        {"first_due", 0, 0, 0},
        {"first_not_due", 100, 0, 0},
        {"second_due", 250, 0, 1},
        {"second_not_due", 200, 0, 1},
        {"third_due", 1000, 0, 2},
        {"last_due", 3000, 0, 3},
        {"exhausted", 5000, 0, 4},
        {"offset_clock", 1250, 1000, 2},
    };
    for (const auto& r : retries) {
      EngineLogicCase c;
      c.module = "retry_schedule";
      c.name = r.name;
      c.input = json::object();
      c.input["now_ms"] = r.now_ms;
      c.input["first_pass_ms"] = r.first_pass_ms;
      c.input["attempts_done"] = r.attempts_done;
      cases.push_back(std::move(c));
    }
  }

  // --- crash_restart_guard --------------------------------------------------
  {
    struct CrashCase {
      const char* name;
      int count;
      long long window_start_ms;
      long long now_ms;
      int max_restarts;
      long long window_ms;
    };
    // The production values: 3 restarts in 10 minutes.
    const CrashCase crashes[] = {
        {"first_restart", 0, 0, 1000, 3, 600000},
        {"second_in_window", 1, 1000, 2000, 3, 600000},
        {"third_in_window", 2, 1000, 2000, 3, 600000},
        {"over_the_budget", 3, 1000, 2000, 3, 600000},
        {"window_expired", 1, 1000, 1000 + 600000, 3, 600000},
        {"window_expired_plus_one", 1, 1000, 1000 + 600001, 3, 600000},
        {"zero_window", 1, 1000, 2000, 3, 0},
        {"clock_went_backwards", 1, 5000, 4000, 3, 600000},
    };
    for (const auto& c : crashes) {
      EngineLogicCase c2;
      c2.module = "crash_restart_guard";
      c2.name = c.name;
      c2.input = json::object();
      c2.input["count"] = c.count;
      c2.input["window_start_ms"] = c.window_start_ms;
      c2.input["now_ms"] = c.now_ms;
      c2.input["max_restarts"] = c.max_restarts;
      c2.input["window_ms"] = c.window_ms;
      cases.push_back(std::move(c2));
    }
  }

  // --- pause_controller ------------------------------------------------------
  {
    struct PauseCase {
      const char* name;
      int mask;
    };
    const PauseCase pauses[] = {
        {"none", 0},
        {"user", PauseController::kUser},
        {"fullscreen", PauseController::kFullscreen},
        {"suspend", PauseController::kSuspend},
        {"power", PauseController::kPower},
        {"session_lock", PauseController::kSessionLock},
        {"screen_off", PauseController::kScreenOff},
        {"user_and_fullscreen",
         PauseController::kUser | PauseController::kFullscreen},
        {"all_invisible", PauseController::kFullscreen | PauseController::kScreenOff |
                              PauseController::kSessionLock |
                              PauseController::kSuspend},
        {"all", 63},
    };
    for (const auto& p : pauses) {
      EngineLogicCase c;
      c.module = "pause_controller";
      c.name = p.name;
      c.input = json::object();
      c.input["mask"] = p.mask;
      cases.push_back(std::move(c));
    }
  }

  // --- topology_diff --------------------------------------------------------
  {
    const json primary = TopologyMonitorJson("edid:AAA", 0, 0, 1920, 1080, 100, 0, true);
    const json secondary = TopologyMonitorJson("edid:BBB", -1920, 0, 1920, 1080, 100, 0, false);
    struct DiffCase {
      const char* name;
      std::vector<json> old_monitors;
      std::vector<json> new_monitors;
      const char* old_hash;
      const char* new_hash;
    };
    const DiffCase diffs[] = {
        {"identical",
         {primary},
         {primary},
         "h1",
         "h1"},
        {"added",
         {primary},
         {primary, TopologyMonitorJson("edid:CCC", 1920, 0, 1920, 1080, 100, 0, false)},
         "h1",
         "h2"},
        {"removed",
         {primary, secondary},
         {primary},
         "h1",
         "h2"},
        {"moved",
         {primary},
         {TopologyMonitorJson("edid:AAA", 0, 100, 1920, 1080, 100, 0, true)},
         "h1",
         "h2"},
        {"resized",
         {primary},
         {TopologyMonitorJson("edid:AAA", 0, 0, 1280, 720, 100, 0, true)},
         "h1",
         "h2"},
        {"dpi_changed",
         {primary},
         {TopologyMonitorJson("edid:AAA", 0, 0, 1920, 1080, 125, 0, true)},
         "h1",
         "h2"},
        {"orientation_changed",
         {primary},
         {TopologyMonitorJson("edid:AAA", 0, 0, 1920, 1080, 100, 1, true)},
         "h1",
         "h2"},
        {"primary_changed",
         {primary},
         {TopologyMonitorJson("edid:AAA", 0, 0, 1920, 1080, 100, 0, false)},
         "h1",
         "h2"},
        {"hash_only",
         {primary},
         {primary},
         "h1",
         "h2"},
        {"swap",
         {primary, secondary},
         {secondary, primary},
         "h1",
         "h2"},
    };
    for (const auto& d : diffs) {
      EngineLogicCase c;
      c.module = "topology_diff";
      c.name = d.name;
      c.input = json::object();
      c.input["old"] = TopologySnapshotJson(d.old_monitors, d.old_hash);
      c.input["new"] = TopologySnapshotJson(d.new_monitors, d.new_hash);
      cases.push_back(std::move(c));
    }
  }

  // --- reconcile_policy -----------------------------------------------------
  {
    struct TriggerCase {
      const char* name;
      const char* trigger;
    };
    const TriggerCase triggers[] = {
        {"display_change", "display-change"},
        {"monitor_on", "monitor-on"},
        {"device_lost", "device-lost"},
        {"resume", "resume"},
        {"session_unlock", "session-unlock"},
        {"taskbar_created", "taskbar-created"},
        {"setting_change", "setting-change"},
        {"unknown_trigger", "something-else"},
        {"empty_trigger", ""},
    };
    for (const auto& t : triggers) {
      EngineLogicCase c;
      c.module = "reconcile_policy";
      c.name = t.name;
      c.input = json::object();
      c.input["trigger"] = t.trigger;
      cases.push_back(std::move(c));
    }
    // The topology stability: the same enumeration in another order is stable,
    // a moved monitor is not, a different count is not, and a duplicated
    // identity is undecidable (not stable).
    const json m0 = MonitorInfoJson(0, "edid:AAA", "\\\\.\\DISPLAY1", 0, 0, 1920, 1080, true);
    const json m1 = MonitorInfoJson(1, "edid:BBB", "\\\\.\\DISPLAY2", -1920, 0, 1920, 1080, false);
    struct StableCase {
      const char* name;
      std::vector<json> first;
      std::vector<json> second;
    };
    const StableCase stables[] = {
        {"same_order", {m0, m1}, {m0, m1}},
        {"reordered", {m0, m1}, {m1, m0}},
        {"moved", {m0, m1}, {m0, MonitorInfoJson(1, "edid:BBB", "\\\\.\\DISPLAY2", -1920, 50, 1920, 1080, false)}},
        {"count_changed", {m0}, {m0, m1}},
        {"duplicate_identity", {m0, m1}, {m0, MonitorInfoJson(1, "edid:AAA", "\\\\.\\DISPLAY2", -1920, 0, 1920, 1080, false)}},
    };
    for (const auto& s : stables) {
      EngineLogicCase c;
      c.module = "reconcile_policy";
      c.name = s.name;
      c.input = json::object();
      c.input["first"] = s.first;
      c.input["second"] = s.second;
      cases.push_back(std::move(c));
    }
  }

  // --- unload_state (the task 1.1 state machine) ---------------------------
  {
    struct UnloadCase {
      const char* name;
      const char* op;        // "pause_changed" or "tick"
      DecodeState state;
      long long pause_started_ms;
      int unload_after_ms;
      bool paused;           // the pause_changed input
      int mask;              // the tick input
      long long now_ms;
    };
    const int kUser = PauseController::kUser;
    const int kFullscreen = PauseController::kFullscreen;
    const int kScreenOff = PauseController::kScreenOff;
    const int kSessionLock = PauseController::kSessionLock;
    const int kSuspend = PauseController::kSuspend;
    const int kPower = PauseController::kPower;
    const UnloadCase unloads[] = {
        // The pause starts.
        {"pause_starts", "pause_changed", DecodeState::kActive, 0, 30000, true,
         0, 1000},
        // A second owner pauses while the pause already runs: the deadline
        // must NOT be pushed back.
        {"pause_keeps_the_deadline", "pause_changed", DecodeState::kPaused,
         1000, 30000, true, 0, 5000},
        // The mask is clear while active: nothing happens.
        {"active_stays_active", "pause_changed", DecodeState::kActive, 0,
         30000, false, 0, 1000},
        // The mask is clear while paused: active again, the clock is reset.
        {"resume_from_paused", "pause_changed", DecodeState::kPaused, 1000,
         30000, false, 0, 5000},
        // The mask is clear while unloaded: the engine must reload NOW.
        {"resume_from_unloaded_reloads", "pause_changed",
         DecodeState::kUnloaded, 1000, 30000, false, 0, 5000},
        // A pause while unloaded: the state stays unloaded (no reload).
        {"pause_while_unloaded", "pause_changed", DecodeState::kUnloaded, 1000,
         30000, true, 0, 5000},
        // The deadline elapsed and an owner hides the wallpaper: UNLOAD.
        {"deadline_unloads", "tick", DecodeState::kPaused, 1000, 30000, false,
         kFullscreen, 31000},
        // The deadline has NOT elapsed.
        {"deadline_not_reached", "tick", DecodeState::kPaused, 1000, 30000,
         false, kFullscreen, 30999},
        // The deadline exactly at the boundary: due.
        {"deadline_exactly", "tick", DecodeState::kPaused, 1000, 30000, false,
         kFullscreen, 31000},
        // An explicit user pause keeps the picture: no unload.
        {"user_pause_keeps_the_picture", "tick", DecodeState::kPaused, 1000,
         30000, false, kUser, 31000},
        // The DC power cap keeps the picture too: no unload.
        {"power_cap_keeps_the_picture", "tick", DecodeState::kPaused, 1000,
         30000, false, kPower, 31000},
        // Every invisible owner qualifies.
        {"screen_off_unloads", "tick", DecodeState::kPaused, 1000, 30000, false,
         kScreenOff, 31000},
        {"session_lock_unloads", "tick", DecodeState::kPaused, 1000, 30000,
         false, kSessionLock, 31000},
        {"suspend_unloads", "tick", DecodeState::kPaused, 1000, 30000, false,
         kSuspend, 31000},
        // The unload is disabled (0 = never).
        {"never_unloads", "tick", DecodeState::kPaused, 1000, 0, false,
         kFullscreen, 31000},
        // The state is not paused: the tick does nothing.
        {"active_tick_does_nothing", "tick", DecodeState::kActive, 0, 30000,
         false, kFullscreen, 31000},
        // The pause never started (0): the tick does nothing.
        {"no_pause_start_does_nothing", "tick", DecodeState::kPaused, 0, 30000,
         false, kFullscreen, 31000},
        // The state is already unloaded: the tick does nothing.
        {"unloaded_tick_does_nothing", "tick", DecodeState::kUnloaded, 1000,
         30000, false, kFullscreen, 31000},
    };
    for (const auto& u : unloads) {
      EngineLogicCase c;
      c.module = "unload_state";
      c.name = u.name;
      c.input = json::object();
      c.input["op"] = u.op;
      c.input["state"] = DecodeStateName(u.state);
      c.input["pause_started_ms"] = u.pause_started_ms;
      c.input["unload_after_ms"] = u.unload_after_ms;
      c.input["paused"] = u.paused;
      c.input["mask"] = u.mask;
      c.input["now_ms"] = u.now_ms;
      cases.push_back(std::move(c));
    }
  }

  // --- pin_verify_schedule (the pure core) --------------------------------
  {
    struct PinCase {
      const char* name;
      bool armed;
      long long deadline_ms;
      long long now_ms;
    };
    const PinCase pins[] = {
        {"not_armed", false, 6000, 7000},
        {"before_the_deadline", true, 6000, 5999},
        {"at_the_deadline", true, 6000, 6000},
        {"after_the_deadline", true, 6000, 6001},
    };
    for (const auto& p : pins) {
      EngineLogicCase c;
      c.module = "pin_verify_schedule";
      c.name = p.name;
      c.input = json::object();
      c.input["armed"] = p.armed;
      c.input["deadline_ms"] = p.deadline_ms;
      c.input["now_ms"] = p.now_ms;
      cases.push_back(std::move(c));
    }
    // The lifetime revert counter.
    struct RevertCase {
      const char* name;
      int total;
      int n;
    };
    const RevertCase reverts[] = {
        {"first_revert", 0, 1},
        {"no_revert", 0, 0},
        {"accumulates", 2, 1},
        {"several_at_once", 1, 3},
    };
    for (const auto& r : reverts) {
      EngineLogicCase c;
      c.module = "pin_verify_schedule";
      c.name = r.name;
      c.input = json::object();
      c.input["total"] = r.total;
      c.input["reverts"] = r.n;
      cases.push_back(std::move(c));
    }
  }

  return cases;
}

// The evaluator: it calls the PRODUCTION functions. The output is the frozen
// behaviour.
inline json EvaluateEngineLogicCase(const EngineLogicCase& c) {
  using namespace k6wp;
  json out = json::object();

  if (c.module == "desktop_placement") {
    const auto child = c.input["child"];
    const auto monitor = c.input["monitor"];
    PlacementRect child_rect{child["left"], child["top"], child["right"],
                             child["bottom"]};
    PlacementRect monitor_rect{monitor["left"], monitor["top"],
                               monitor["right"], monitor["bottom"]};
    out["verdict"] = CoverageReason(CoversMonitor(child_rect, monitor_rect));
    return out;
  }

  if (c.module == "workerw_host") {
    std::vector<std::string> children;
    for (const auto& cls : c.input["child_classes"]) {
      children.push_back(cls.get<std::string>());
    }
    out["accepted"] = WorkerWAcceptsChildClasses(
        children, c.input["own_class"].get<std::string>());
    return out;
  }

  if (c.module == "retry_schedule") {
    RetryClock clock;
    clock.first_pass_ms = c.input["first_pass_ms"].get<long long>();
    clock.attempts_done = c.input["attempts_done"].get<int>();
    const long long now_ms = c.input["now_ms"].get<long long>();
    out["due"] = retry_schedule_due(now_ms, clock);
    out["next_delay_ms"] = retry_next_delay_ms(now_ms, clock);
    out["exhausted"] = retry_schedule_exhausted(clock);
    return out;
  }

  if (c.module == "crash_restart_guard") {
    CrashRestartState state;
    state.count = c.input["count"].get<int>();
    state.window_start_ms = c.input["window_start_ms"].get<long long>();
    const long long now_ms = c.input["now_ms"].get<long long>();
    const int max_restarts = c.input["max_restarts"].get<int>();
    const long long window_ms = c.input["window_ms"].get<long long>();
    const CrashRestartState advanced =
        CrashRestartAdvance(state, now_ms, max_restarts, window_ms);
    out["count"] = advanced.count;
    out["window_start_ms"] = advanced.window_start_ms;
    out["allowed"] = CrashRestartAllowed(advanced, max_restarts);
    return out;
  }

  if (c.module == "pause_controller") {
    const int mask = c.input["mask"].get<int>();
    PauseController controller;
    // The controller has no mask setter: the mask is built from the owner
    // bits, exactly like the engine does.
    for (int bit = 1; bit <= 32; bit <<= 1) {
      if (mask & bit) controller.SetBit(bit, true);
    }
    out["mask"] = controller.mask();
    out["ui_paused"] = controller.UiPaused();
    out["slots_paused"] = controller.SlotsPaused();
    out["hides_wallpaper"] = PauseHidesWallpaper(mask);
    return out;
  }

  if (c.module == "topology_diff") {
    const TopologySnapshot old_s = ParseTopologySnapshot(c.input["old"]);
    const TopologySnapshot new_s = ParseTopologySnapshot(c.input["new"]);
    const TopologyDiff d = diff_topology(old_s, new_s);
    out["added_keys"] = d.added_keys;
    out["removed_keys"] = d.removed_keys;
    out["moved_keys"] = d.moved_keys;
    out["resized_keys"] = d.resized_keys;
    out["dpi_keys"] = d.dpi_keys;
    out["orientation_keys"] = d.orientation_keys;
    out["primary_changed"] = d.primary_changed;
    out["hash_changed"] = d.hash_changed;
    out["empty"] = d.empty();
    return out;
  }

  if (c.module == "reconcile_policy") {
    if (c.input.contains("trigger")) {
      const ReconcileAction action =
          DecideReconcileAction(c.input["trigger"].get<std::string>());
      out["action"] = ReconcileActionName(action);
      out["needs_stable_topology"] =
          ReconcileActionNeedsStableTopology(action);
      return out;
    }
    const std::vector<MonitorInfo> first = ParseMonitors(c.input["first"]);
    const std::vector<MonitorInfo> second = ParseMonitors(c.input["second"]);
    out["stable"] = TopologyStable(first, second);
    out["signature"] = TopologySignature(first);
    return out;
  }

  if (c.module == "unload_state") {
    UnloadMachine machine;
    const std::string state = c.input["state"].get<std::string>();
    machine.state = state == "paused"     ? DecodeState::kPaused
                     : state == "unloaded" ? DecodeState::kUnloaded
                                           : DecodeState::kActive;
    machine.pause_started_ms = c.input["pause_started_ms"].get<long long>();
    machine.unload_after_ms = c.input["unload_after_ms"].get<int>();
    const long long now_ms = c.input["now_ms"].get<long long>();

    const UnloadDecision d =
        c.input["op"].get<std::string>() == "tick"
            ? UnloadOnTick(machine, c.input["mask"].get<int>(), now_ms)
            : UnloadOnPauseChanged(machine, c.input["paused"].get<bool>(),
                                   now_ms);
    out["state"] = DecodeStateName(d.state);
    out["pause_started_ms"] = d.pause_started_ms;
    out["unload_due"] = d.unload_due;
    out["reload_due"] = d.reload_due;
    return out;
  }

  if (c.module == "pin_verify_schedule") {
    if (c.input.contains("armed")) {
      out["due"] = PinVerifyDueAt(c.input["armed"].get<bool>(),
                                  c.input["deadline_ms"].get<long long>(),
                                  c.input["now_ms"].get<long long>());
      return out;
    }
    out["total"] = PinVerifyAddReverts(c.input["total"].get<int>(),
                                       c.input["reverts"].get<int>());
    return out;
  }

  out["error"] = "unknown module: " + c.module;
  return out;
}

}  // namespace k6wp_test
