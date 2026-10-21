// tests/golden_dump.cpp - Phase 0 task 0.6: freeze the pure C++ behaviour
// as golden JSON that a Rust port can replay.
//
// WHAT THIS IS
//   A behaviour dump, not a test. It exercises the deterministic, side-effect
//   free parts of shared/ (config schema, studio settings, playlist,
//   displays schema, IPC protocol) over a fixed table of inputs and writes
//   the RESULT of every call to one JSON document.
//
// WHY
//   Phase 2/3 replaces this C++ with Rust. The only honest way to prove the
//   port is behaviour-preserving is to compare the old implementation's
//   output against the new implementation's output on the same inputs. This
//   file is the fixed input table plus the frozen expected outputs, so the
//   Rust port can replay it (a Rust test reads the same JSON, produces the
//   same document, and the two are diffed).
//
// WHAT IS IN SCOPE (pure functions only, no filesystem, no Win32 state)
//   - MigrateConfig / ValidateConfig      (config_schema)
//   - MigrateStudioSettings / ValidateStudioSettings (studio_settings)
//   - MigratePlaylist / ValidatePlaylist / SelectNextIndex /
//     PlaylistIndexForPath / NormalizeOrder (playlist)
//   - MigrateDisplays / ValidateDisplays  (displays_schema)
//   - Encode / Decode / CmdToString / CmdFromString (ipc_protocol)
//   - ParseEngineState                    (ipc_protocol)
//
// WHAT IS NOT IN SCOPE (and why)
//   - LoadX/SaveX: they touch the filesystem (temp dirs, .bak files, atomic
//     renames). Their CONTRACT is covered by the existing ctest suite; the
//     golden file freezes the pure VALUE semantics only.
//   - Anything that reads the clock, the registry, a monitor or the network.
//
// OUTPUT FORMAT (stable, diffable)
//   {
//     "schema": "k6wp.golden.v1",
//     "cases": [ { "id": "...", "fn": "...", "input": {...},
//                 "output": {...} | "error": "..." }, ... ]
//   }
//   Every case has an "id" (stable slug) so a diff points at ONE case.
//
// USAGE
//   golden_dump <output.json>          write the golden document
//   golden_dump --check <golden.json>  regenerate and compare; exit 1 on drift
//
// The --check mode is the CI gate: a behaviour change in shared/ that is not
// accompanied by an intentional golden update FAILS the build.
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "config_schema.hpp"
#include "displays_schema.hpp"
#include "ipc_protocol.hpp"
#include "playlist.hpp"
#include "studio_settings.hpp"

using nlohmann::json;

namespace {

// One frozen case. "output" and "error" are mutually exclusive.
struct Case {
  std::string id;
  std::string fn;
  json input;
  json output = json();
  std::string error;
  bool has_error = false;
};

std::vector<Case> g_cases;

void Ok(const std::string& id, const std::string& fn, const json& in, const json& out) {
  Case c;
  c.id = id;
  c.fn = fn;
  c.input = in;
  c.output = out;
  g_cases.push_back(std::move(c));
}

void Err(const std::string& id, const std::string& fn, const json& in, const std::string& msg) {
  Case c;
  c.id = id;
  c.fn = fn;
  c.input = in;
  c.has_error = true;
  c.error = msg;
  g_cases.push_back(std::move(c));
}

// Runs `fn`, capturing either its value or the ConfigError message.
// `in` is the INPUT that gets recorded in the golden file: it must be the
// exact document handed to the function, otherwise a replayer has nothing
// to feed.
template <typename F>
void Capture(const std::string& id, const std::string& name, const json& in, F fn) {
  try {
    Ok(id, name, in, fn());
  } catch (const k6wp::ConfigError& e) {
    Err(id, name, in, e.what());
  } catch (const std::exception& e) {
    Err(id, name, in, std::string("std::exception: ") + e.what());
  }
}

json ParseOrObject(const std::string& text) {
  return json::parse(text, nullptr, false);
}

// nlohmann::json has no std::wstring serialiser, so wide paths go through
// the filesystem UTF-8 conversion (the same one the production code uses
// via std::filesystem::u8path).
std::string ToUtf8(const std::wstring& w) {
  return std::filesystem::path(w).u8string();
}

// --- config schema ---------------------------------------------------------

void DumpConfig() {
  // Default-constructed struct, as a fresh install sees it.
  {
    k6wp::WallpaperConfig cfg;
    Ok("config.default", "WallpaperConfig::default", json::object(),
       k6wp::ConfigToJson(cfg));
  }

  const std::vector<std::pair<std::string, std::string>> inputs = {
      {"v0_legacy", R"({"version":0,"video_path":"C:\\v\\a.mp4"})"},
      {"v0_empty", R"({})"},
      {"v1_monitor", R"({"version":1,"video_path":"C:\\v\\a.mp4","monitor_id":2})"},
      {"v2_fill", R"({"version":2,"fit_mode":"fill"})"},
      {"v2_fill_explicit_monitor", R"({"version":2,"fit_mode":"fill","monitor_id":1})"},
      {"v3_battery", R"({"version":3,"battery_mode":"static"})"},
      {"v4_affinity", R"({"version":4,"cpu_affinity":"all","gpu_adapter":"discrete"})"},
      {"v5_current", R"({"version":5,"video_path":"C:\\v\\b.mp4","fit_mode":"fit","speed":1.5,"monitor_id":0,"crf":18,"resolution_w":1920,"resolution_h":1080,"fps_cap":30,"battery_saver":true,"battery_mode":"static","cpu_affinity":"all","gpu_adapter":"integrated","reconcile_mode":"shadow"})"},
      {"v5_reconcile_on", R"({"version":5,"reconcile_mode":"on"})"},
      {"v5_reconcile_bogus", R"({"version":5,"reconcile_mode":"bogus"})"},
      {"v6_future", R"({"version":6})"},
      // A file from a NEWER engine (G4): the known fields must keep their
      // values and the unknown ones fall back to their defaults - the
      // older engine must not reset the user's settings.
      {"v7_newer",
       R"({"version":7,"video_path":"C:\\v\\new.mp4","fit_mode":"fit","crf":19,"fps_cap":30,"unknown_future_key":"keep-me-not"})"},
      {"v_neg", R"({"version":-1})"},
      {"unknown_fit", R"({"version":5,"fit_mode":"squish"})"},
      {"crf_low", R"({"version":5,"crf":15})"},
      {"crf_high", R"({"version":5,"crf":29})"},
      {"speed_low", R"({"version":5,"speed":0.4})"},
      {"speed_high", R"({"version":5,"speed":2.1})"},
      {"fps_zero", R"({"version":5,"fps_cap":0})"},
      {"fps_high", R"({"version":5,"fps_cap":31})"},
      {"res_negative", R"({"version":5,"resolution_w":-1})"},
      {"monitor_below_minus1", R"({"version":5,"monitor_id":-2})"},
      {"battery_mode_bogus", R"({"version":5,"battery_mode":"zzz"})"},
      {"affinity_bogus", R"({"version":5,"cpu_affinity":"zzz"})"},
      {"adapter_bogus", R"({"version":5,"gpu_adapter":"zzz"})"},
      {"type_corrupt_speed", R"({"version":5,"speed":"fast"})"},
      {"type_corrupt_version", R"({"version":"5"})"},
      {"not_an_object", R"([1,2,3])"},
      {"corrupt_json", R"({"version":5,)"},
  };
  for (const auto& kv : inputs) {
    const std::string id = "config.migrate." + kv.first;
    // The recorded input is the RAW document text (a JSON string), so a
    // replayer feeds the exact same bytes. Corrupt inputs are recorded as
    // their raw text too - that is the whole point of the case.
    const json raw_text = kv.second;
    Capture(id, "MigrateConfig", raw_text, [&]() {
      json raw = ParseOrObject(kv.second);
      if (raw.is_discarded()) {
        throw k6wp::ConfigError("corrupt config JSON: parse error");
      }
      return k6wp::ConfigToJson(k6wp::MigrateConfig(raw));
    });
  }
}

// --- studio settings -------------------------------------------------------

void DumpStudioSettings() {
  {
    k6wp::StudioSettings s = k6wp::DefaultStudioSettings();
    json out = json::object();
    out["version"] = s.version;
    out["auto_compress_on_import"] = s.auto_compress_on_import;
    out["default_crf"] = s.default_crf;
    out["default_fps"] = s.default_fps;
    out["default_resolution_mode"] = s.default_resolution_mode;
    out["start_with_windows"] = s.start_with_windows;
    out["wallpaper_sync"] = s.wallpaper_sync;
    out["lockscreen_sync"] = s.lockscreen_sync;
    out["lockscreen_offset_sec"] = s.lockscreen_offset_sec;
    out["lockscreen_offer_shown"] = s.lockscreen_offer_shown;
    out["compress_advanced_visible"] = s.compress_advanced_visible;
    out["check_updates"] = s.check_updates;
    out["close_to_tray"] = s.close_to_tray;
    out["playlist_source"] = s.playlist_source;
    out["performance_preset"] = s.performance_preset;
    // The two directory fields are environment-dependent, so only their
    // basenames are frozen (the full path would make the golden file
    // machine-specific).
    out["compress_output_dir_basename"] =
        std::filesystem::path(s.compress_output_dir).filename().wstring();
    out["cache_dir_basename"] =
        std::filesystem::path(s.cache_dir).filename().wstring();
    Ok("settings.default", "DefaultStudioSettings", json::object(), out);
  }

  const std::vector<std::pair<std::string, std::string>> inputs = {
      {"v0_legacy", R"({"version":0})"},
      {"v1_lockscreen", R"({"version":1,"lockscreen_sync":false})"},
      {"v2_advanced", R"({"version":2,"compress_advanced_visible":true})"},
      {"v3_ui_prefs", R"({"version":3,"closeToTray":true,"playlistSource":"custom","performancePreset":"Maksimal"})"},
      {"v4_offer", R"({"version":4,"lockscreen_offer_shown":true})"},
      {"v4_offset_one", R"({"version":4,"lockscreen_offset_sec":1.0})"},
      {"v4_offset_half", R"({"version":4,"lockscreen_offset_sec":0.5})"},
      {"v4_offset_zero", R"({"version":4,"lockscreen_offset_sec":0.0})"},
      {"v5_current", R"({"version":5,"auto_compress_on_import":false,"default_crf":18,"default_fps":24,"default_resolution_mode":"source","wallpaper_sync":false,"lockscreen_sync":true,"lockscreen_offset_sec":2.5})"},
      {"v6_future", R"({"version":6})"},
      {"playlist_source_bogus", R"({"version":5,"playlistSource":"nope"})"},
      {"performance_preset_bogus", R"({"version":5,"performancePreset":"nope"})"},
      {"crf_low", R"({"version":5,"default_crf":15})"},
      {"fps_high", R"({"version":5,"default_fps":31})"},
      {"resolution_mode_bogus", R"({"version":5,"default_resolution_mode":"nope"})"},
      {"removed_keys", R"({"version":3,"keep_original":true,"start_minimized":true})"},
      {"type_corrupt", R"({"version":5,"default_crf":"x"})"},
      {"not_an_object", R"("hello")"},
  };
  for (const auto& kv : inputs) {
    const std::string id = "settings.migrate." + kv.first;
    Capture(id, "MigrateStudioSettings", json(kv.second), [&]() {
      json raw = ParseOrObject(kv.second);
      if (raw.is_discarded()) {
        throw k6wp::ConfigError("corrupt settings JSON: parse error");
      }
      k6wp::StudioSettings s = k6wp::MigrateStudioSettings(raw);
      json out = json::object();
      out["version"] = s.version;
      out["auto_compress_on_import"] = s.auto_compress_on_import;
      out["default_crf"] = s.default_crf;
      out["default_fps"] = s.default_fps;
      out["default_resolution_mode"] = s.default_resolution_mode;
      out["wallpaper_sync"] = s.wallpaper_sync;
      out["lockscreen_sync"] = s.lockscreen_sync;
      out["lockscreen_offset_sec"] = s.lockscreen_offset_sec;
      out["lockscreen_offer_shown"] = s.lockscreen_offer_shown;
      out["compress_advanced_visible"] = s.compress_advanced_visible;
      out["check_updates"] = s.check_updates;
      out["close_to_tray"] = s.close_to_tray;
      out["playlist_source"] = s.playlist_source;
      out["performance_preset"] = s.performance_preset;
      return out;
    });
  }
}

// --- playlist --------------------------------------------------------------

void DumpPlaylist() {
  const std::vector<std::pair<std::string, std::string>> inputs = {
      {"v0_legacy", R"({"version":0})"},
      {"v1_current", R"({"version":1,"enabled":true,"interval_min":15,"shuffle":true,"order":["C:\\a.mp4","C:\\b.mp4"]})"},
      {"v2_future", R"({"version":2})"},
      {"interval_zero", R"({"version":1,"interval_min":0})"},
      {"interval_high", R"({"version":1,"interval_min":1441})"},
      {"order_dupes", R"({"version":1,"order":["C:\\a.mp4","C:\\a.mp4","C:\\b.mp4"]})"},
      {"order_empty_entry", R"({"version":1,"order":["C:\\a.mp4",""]})"},
      {"not_an_object", R"([])"},
  };
  for (const auto& kv : inputs) {
    const std::string id = "playlist.migrate." + kv.first;
    Capture(id, "MigratePlaylist", json(kv.second), [&]() {
      json raw = ParseOrObject(kv.second);
      if (raw.is_discarded()) {
        throw k6wp::ConfigError("corrupt playlist JSON: parse error");
      }
      k6wp::PlaylistConfig c = k6wp::MigratePlaylist(raw);
      json out = json::object();
      out["version"] = c.version;
      out["enabled"] = c.enabled;
      out["interval_min"] = c.interval_min;
      out["shuffle"] = c.shuffle;
      out["order"] = json::array();
      for (const auto& w : c.order) {
        out["order"].push_back(ToUtf8(w));
      }
      return out;
    });
  }

  // Pure rotation helpers. The shuffle RNG is seeded explicitly so the
  // sequence is reproducible across runs and machines.
  {
    std::uint64_t state = 0x0123456789ABCDEFull;
    json out = json::array();
    for (std::size_t count : {std::size_t(0), std::size_t(1), std::size_t(3), std::size_t(8)}) {
      json row = json::object();
      row["count"] = count;
      row["sequential_from_0"] = k6wp::SelectNextIndex(0, count, false, state);
      row["sequential_from_2"] = k6wp::SelectNextIndex(2, count, false, state);
      row["shuffle_from_0"] = k6wp::SelectNextIndex(0, count, true, state);
      out.push_back(row);
    }
    json in = json::object();
    in["rng_seed"] = "0x0123456789ABCDEF";
    in["note"] =
        "The shuffle RNG state is carried across the calls in this list, in "
        "order. A replayer must use the same seed and the same call order.";
    in["calls"] = json::array();
    for (std::size_t count : {std::size_t(0), std::size_t(1), std::size_t(3), std::size_t(8)}) {
      for (int seq = 0; seq < 2; ++seq) {
        json c = json::object();
        c["fn"] = "SelectNextIndex";
        c["current"] = (seq == 0) ? 0 : 2;
        c["count"] = count;
        c["shuffle"] = false;
        in["calls"].push_back(c);
      }
      json c = json::object();
      c["fn"] = "SelectNextIndex";
      c["current"] = 0;
      c["count"] = count;
      c["shuffle"] = true;
      in["calls"].push_back(c);
    }
    Ok("playlist.select_next_index", "SelectNextIndex", in, out);
  }
  {
    std::vector<std::wstring> order = {L"C:\\Videos\\A.mp4", L"C:\\Videos\\b.mp4"};
    json out = json::object();
    out["found_exact"] = k6wp::PlaylistIndexForPath(order, L"C:\\Videos\\A.mp4");
    out["found_case_insensitive"] = k6wp::PlaylistIndexForPath(order, L"c:\\videos\\B.MP4");
    out["not_found"] = k6wp::PlaylistIndexForPath(order, L"C:\\other.mp4");
    out["empty_order"] = k6wp::PlaylistIndexForPath({}, L"C:\\other.mp4");
    json in = json::object();
    in["order"] = json::array();
    for (const auto& w : order) {
      in["order"].push_back(ToUtf8(w));
    }
    in["queries"] = json::array({"C:\\Videos\\A.mp4", "c:\\videos\\B.MP4", "C:\\other.mp4"});
    in["note"] = "The output keys map to the queries in order, plus an empty-order query.";
    Ok("playlist.index_for_path", "PlaylistIndexForPath", in, out);
  }
  {
    std::vector<std::wstring> in_w = {L"C:\\a.mp4", L"", L"C:\\A.mp4", L"C:\\b.mp4", L"C:\\b.mp4"};
    json out = json::array();
    for (const auto& w : k6wp::NormalizeOrder(in_w)) {
      out.push_back(ToUtf8(w));
    }
    json in = json::object();
    in["order"] = json::array();
    for (const auto& w : in_w) {
      in["order"].push_back(ToUtf8(w));
    }
    Ok("playlist.normalize_order", "NormalizeOrder", in, out);
  }
}

// --- displays schema -------------------------------------------------------

void DumpDisplays() {
  const std::vector<std::pair<std::string, std::string>> inputs = {
      {"v0_legacy", R"({"version":0})"},
      {"v1_current",
       R"({"version":1,"assignments":{"\\\\.\\DISPLAY1":{"path":"C:\\a.mp4","exists":true},"\\\\.\\DISPLAY2":{"path":"C:\\b.mp4","exists":false,"key_stable":"ccd:abc"}}})"},
      {"v1_with_metadata",
       R"({"version":1,"assignments":{},"displays":[{"device":"\\\\.\\DISPLAY1"}],"key_aliases":{"\\\\.\\DISPLAY1":"ccd:abc"}})"},
      {"v2_future", R"({"version":2})"},
      {"assignment_not_object", R"({"version":1,"assignments":{"\\\\.\\DISPLAY1":"oops"}})"},
      {"not_an_object", R"({})"},
  };
  for (const auto& kv : inputs) {
    const std::string id = "displays.migrate." + kv.first;
    Capture(id, "MigrateDisplays", json(kv.second), [&]() {
      json raw = ParseOrObject(kv.second);
      if (raw.is_discarded()) {
        throw k6wp::ConfigError("corrupt displays JSON: parse error");
      }
      k6wp::DisplaysConfig c = k6wp::MigrateDisplays(raw);
      json out = json::object();
      out["version"] = c.version;
      out["assignments"] = json::object();
      for (const auto& kv2 : c.assignments) {
        json e = json::object();
        e["path"] = ToUtf8(kv2.second.path);
        e["exists"] = kv2.second.exists;
        e["key_stable"] = ToUtf8(kv2.second.key_stable);
        out["assignments"][ToUtf8(kv2.first)] = e;
      }
      return out;
    });
  }
}

// --- IPC protocol ----------------------------------------------------------

void DumpIpc() {
  // Command name <-> enum mapping (the append-only wire vocabulary).
  {
    json out = json::object();
    for (int i = 0; i <= 6; ++i) {
      const auto cmd = static_cast<k6wp::Cmd>(i);
      out[k6wp::CmdToString(cmd)] = i;
    }
    json from_string = json::object();
    for (const char* name : {"set_video", "pause", "resume", "set_monitor",
                             "get_state", "quit", "set_display_video", "nope",
                             ""}) {
      k6wp::Cmd c = k6wp::Cmd::get_state;
      // The empty command name is spelled "<empty>" as the KEY: an empty JSON
      // key is legal but breaks Windows PowerShell 5.1's ConvertFrom-Json,
      // and the repo's own tooling must be able to read this file.
      const std::string key = (name[0] == '\0') ? "<empty>" : name;
      from_string[key] = k6wp::CmdFromString(name, c) ? static_cast<int>(c) : -1;
    }
    out["from_string"] = from_string;
    out["protocol_version"] = k6wp::kProtocolVersion;
    out["max_payload_bytes"] = k6wp::kMaxPayloadBytes;
    Ok("ipc.cmd_vocabulary", "CmdToString/CmdFromString", json::object(), out);
  }

  // Encode: exact wire bytes for each command with a representative payload.
  {
    const std::vector<std::pair<k6wp::Cmd, json>> inputs = {
        {k6wp::Cmd::set_video, {{"path", "C:\\v\\a.mp4"}}},
        {k6wp::Cmd::pause, json::object()},
        {k6wp::Cmd::resume, json::object()},
        {k6wp::Cmd::set_monitor, {{"monitor_id", 1}}},
        {k6wp::Cmd::get_state, json::object()},
        {k6wp::Cmd::quit, json::object()},
        {k6wp::Cmd::set_display_video, {{"device", "\\\\.\\DISPLAY1"}, {"path", "C:\\v\\b.mp4"}}},
    };
    for (const auto& kv : inputs) {
      k6wp::IpcMessage msg;
      msg.cmd = kv.first;
      msg.payload = kv.second;
      const std::string frame = k6wp::Encode(msg);
      Ok(std::string("ipc.encode.") + k6wp::CmdToString(kv.first), "Encode",
         kv.second, json(frame));
    }
  }

  // Decode: accept/reject table over malformed and edge-case frames.
  {
    const std::vector<std::pair<std::string, std::string>> inputs = {
        {"valid_set_video", "{\"version\":1,\"cmd\":\"set_video\",\"payload\":{\"path\":\"C:\\\\v\\\\a.mp4\"}}\n"},
        {"valid_no_newline", "{\"version\":1,\"cmd\":\"get_state\",\"payload\":{}}"},
        {"valid_crlf", "{\"version\":1,\"cmd\":\"pause\",\"payload\":{}}\r\n"},
        {"empty", ""},
        {"newline_only", "\n"},
        {"bad_json", "{\"version\":1,"},
        {"not_object", "[1,2,3]"},
        {"missing_version", "{\"cmd\":\"get_state\"}"},
        {"version_string", "{\"version\":\"1\",\"cmd\":\"get_state\"}"},
        {"wrong_version", "{\"version\":2,\"cmd\":\"get_state\"}"},
        {"missing_cmd", "{\"version\":1}"},
        {"cmd_not_string", "{\"version\":1,\"cmd\":5}"},
        {"unknown_cmd", "{\"version\":1,\"cmd\":\"explode\"}"},
        {"embedded_newline", "{\"version\":1,\"cmd\":\"get_state\"}\n{\"version\":1,\"cmd\":\"quit\"}"},
    };
    for (const auto& kv : inputs) {
      const std::string id = "ipc.decode." + kv.first;
      k6wp::IpcMessage msg;
      const bool ok = k6wp::Decode(kv.second, msg);
      if (ok) {
        json out = json::object();
        out["version"] = msg.version;
        out["cmd"] = k6wp::CmdToString(msg.cmd);
        out["payload"] = msg.payload;
        Ok(id, "Decode", json(kv.second), out);
      } else {
        Err(id, "Decode", json(kv.second), "decode rejected the frame");
      }
    }
  }

  // ParseEngineState: every get_state field, plus the old-engine compat
  // rules (missing keys default safely, wrong types fall back).
  {
    const std::vector<std::pair<std::string, std::string>> inputs = {
        {"full",
         R"({"state":{"pid":1234,"video":"C:\\v\\a.mp4","paused":true,"headless_slots":1,"live":false,"display_capability":1,"display_assignments":{"\\\\.\\DISPLAY1":"C:\\v\\a.mp4"},"display_coverage":{"\\\\.\\DISPLAY1":"covered"},"playlist_enabled":true,"playlist_size":4,"playlist_index":2,"reconcile":{"mode":"on","last_trigger":"resume","last_verdict":"ok","retries":2,"degraded_keys":["\\\\.\\DISPLAY2"]}}})"},
        {"minimal", R"({"state":{"pid":7}})"},
        {"empty_state", R"({})"},
        {"no_state_key", R"({"other":1})"},
        {"wrong_types", R"({"state":{"pid":"x","paused":"yes","display_capability":"y","display_assignments":[],"playlist_size":"z","reconcile":"nope"}})"},
    };
    for (const auto& kv : inputs) {
      const std::string id = "ipc.parse_state." + kv.first;
      Capture(id, "ParseEngineState", json(kv.second), [&]() {
        json raw = ParseOrObject(kv.second);
        if (raw.is_discarded()) {
          throw k6wp::ConfigError("corrupt state JSON: parse error");
        }
        k6wp::EngineState s = k6wp::ParseEngineState(raw);
        json out = json::object();
        out["pid"] = s.pid;
        out["video"] = s.video;
        out["paused"] = s.paused;
        out["headless_slots"] = s.headless_slots;
        out["live"] = s.live;
        out["display_capability"] = s.display_capability;
        out["display_assignments"] = json::object();
        for (const auto& a : s.display_assignments) {
          out["display_assignments"][a.first] = a.second;
        }
        out["display_coverage"] = json::object();
        for (const auto& c : s.display_coverage) {
          out["display_coverage"][c.first] = c.second;
        }
        out["playlist_enabled"] = s.playlist_enabled;
        out["playlist_size"] = s.playlist_size;
        out["playlist_index"] = s.playlist_index;
        out["reconcile_mode"] = s.reconcile_mode;
        out["reconcile_last_trigger"] = s.reconcile_last_trigger;
        out["reconcile_last_verdict"] = s.reconcile_last_verdict;
        out["reconcile_retries"] = s.reconcile_retries;
        out["reconcile_degraded_keys"] = json::array();
        for (const auto& d : s.reconcile_degraded_keys) {
          out["reconcile_degraded_keys"].push_back(d);
        }
        return out;
      });
    }
  }
}

json BuildDocument() {
  g_cases.clear();
  DumpConfig();
  DumpStudioSettings();
  DumpPlaylist();
  DumpDisplays();
  DumpIpc();

  json doc = json::object();
  doc["schema"] = "k6wp.golden.v1";
  doc["note"] =
      "Frozen behaviour of the pure C++ shared/ layer. A Rust port must "
      "reproduce every case's output byte-for-byte. Regenerate with "
      "golden_dump <file>; verify with golden_dump --check <file>.";
  doc["cases"] = json::array();
  for (const auto& c : g_cases) {
    json j = json::object();
    j["id"] = c.id;
    j["fn"] = c.fn;
    j["input"] = c.input;
    if (c.has_error) {
      j["error"] = c.error;
    } else {
      j["output"] = c.output;
    }
    doc["cases"].push_back(j);
  }
  return doc;
}

// Stable serialization: nlohmann's dump() orders object keys alphabetically,
// so the same content always produces the same bytes.
std::string Serialize(const json& doc) {
  return doc.dump(2) + "\n";
}

}  // namespace

int main(int argc, char** argv) {
  const json doc = BuildDocument();

  if (argc >= 3 && std::string(argv[1]) == "--check") {
    const std::filesystem::path golden = argv[2];
    std::ifstream in(golden, std::ios::binary);
    if (!in) {
      std::cerr << "golden_dump: cannot open " << golden << "\n";
      return 2;
    }
    std::stringstream ss;
    ss << in.rdbuf();
    const std::string expected = ss.str();
    const std::string actual = Serialize(doc);
    if (expected == actual) {
      std::cout << "golden_dump: OK (" << doc["cases"].size() << " cases match)\n";
      return 0;
    }
    // Report the FIRST differing case so the failure points at one behaviour.
    json want = json::parse(expected, nullptr, false);
    if (!want.is_discarded() && want.contains("cases") && want["cases"].is_array() &&
        want["cases"].size() == doc["cases"].size()) {
      for (std::size_t i = 0; i < want["cases"].size(); ++i) {
        if (want["cases"][i] != doc["cases"][i]) {
          std::cerr << "golden_dump: DRIFT in case '" << doc["cases"][i]["id"].get<std::string>()
                    << "'\n";
          std::cerr << "  expected: " << want["cases"][i].dump() << "\n";
          std::cerr << "  actual:   " << doc["cases"][i].dump() << "\n";
          break;
        }
      }
    } else {
      std::cerr << "golden_dump: DRIFT (document shape changed)\n";
    }
    return 1;
  }

  if (argc >= 2) {
    const std::filesystem::path out = argv[1];
    if (out.has_parent_path()) {
      std::error_code ec;
      std::filesystem::create_directories(out.parent_path(), ec);
    }
    std::ofstream f(out, std::ios::binary);
    if (!f) {
      std::cerr << "golden_dump: cannot write " << out << "\n";
      return 2;
    }
    f << Serialize(doc);
    std::cout << "golden_dump: wrote " << out << " (" << doc["cases"].size()
              << " cases)\n";
    return 0;
  }

  std::cout << Serialize(doc);
  return 0;
}
