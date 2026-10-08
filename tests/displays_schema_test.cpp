// Mini unit tests for shared/displays_schema (per-monitor assignment store).
// No external test framework: plain asserts with a pass/fail counter.
// Exit code 0 = all pass.
//
// Covers the plan row 5 contract:
//   - LoadDisplays / SaveDisplays / ValidateDisplays / MigrateDisplays
//   - AtomicWriteJson publish, .bak-on-corrupt, 1 MiB read cap
//   - DefaultDisplaysPath -> %LOCALAPPDATA%/K6WP/displays.json
//   - DetectKeyCollision (IS-7: clone-mode duplicated szDevice / shared rect)
//   - exactly the on-disk JSON keys: version, assignments, displays
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <vector>

#include "config_schema.hpp"
#include "displays_schema.hpp"
#include "monitor_util.hpp"
#include "thirdparty/json.hpp"

namespace {

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

std::filesystem::path TempDir() {
  auto dir = std::filesystem::temp_directory_path() / "k6wp_displays_test";
  std::error_code ec;
  std::filesystem::remove_all(dir, ec);  // fresh per run
  std::filesystem::create_directories(dir, ec);
  return dir;
}

void WriteText(const std::filesystem::path& p, const std::string& text) {
  std::ofstream out(p, std::ios::binary | std::ios::trunc);
  out << text;
}

std::string ReadText(const std::filesystem::path& p) {
  std::ifstream in(p, std::ios::binary);
  return std::string(std::istreambuf_iterator<char>(in),
                     std::istreambuf_iterator<char>());
}

std::filesystem::path BakPath(const std::filesystem::path& p) {
  std::filesystem::path bak = p;
  bak += L".bak";
  return bak;
}

bool ThrowsConfigError(const std::function<void()>& fn) {
  try {
    fn();
  } catch (const k6wp::ConfigError&) {
    return true;
  } catch (...) {
    return false;
  }
  return false;
}

// Fixture monitors. device_name comes from MONITORINFOEXW.szDevice via
// monitor_util; tests construct the struct directly (no OS enumeration).
k6wp::MonitorInfo MakeMon(const std::wstring& name, int x, int y, int w,
                          int h) {
  k6wp::MonitorInfo m;
  m.device_name = name;
  m.x = x;
  m.y = y;
  m.width = w;
  m.height = h;
  return m;
}

bool VecHas(const std::vector<std::wstring>& v, const std::wstring& s) {
  for (const auto& e : v) {
    if (e == s) return true;
  }
  return false;
}

}  // namespace

int main() {
  const std::filesystem::path dir = TempDir();

  // 1. Defaults.
  {
    k6wp::DisplaysConfig cfg;
    Check(cfg.version == k6wp::kDisplaysSchemaVersion, "1. default version");
    Check(cfg.assignments.empty(), "1. default assignments empty");
    Check(cfg.displays.is_array() && cfg.displays.empty(),
          "1. default displays empty array");
  }

  // 2. DisplaysToJson: exactly the JSON keys named in the row.
  {
    k6wp::DisplaysConfig cfg;
    cfg.assignments[L"\\\\.\\DISPLAY1"] = {L"C:\\Videos\\a.mp4", true};
    cfg.assignments[L"\\\\.\\DISPLAY2"] = {L"C:\\Videos\\b.webm", false};
    cfg.displays = nlohmann::json::array({{{"name", "primary"}}});
    const nlohmann::json j = k6wp::DisplaysToJson(cfg);

    Check(j.size() == 3, "2. json has exactly 3 top-level keys");
    Check(j.contains("version") && j.contains("assignments") &&
              j.contains("displays"),
          "2. json keys are version/assignments/displays");
    Check(!j.contains("order") && !j.contains("enabled") &&
              !j.contains("interval_min"),
          "2. json has no playlist keys");
    Check(j.at("version") == k6wp::kDisplaysSchemaVersion,
          "2. json version stamped");

    const auto& a1 = j.at("assignments").at("\\\\.\\DISPLAY1");
    Check(a1.at("path") == "C:\\Videos\\a.mp4", "2. assignment path utf-8");
    Check(a1.at("exists") == true, "2. assignment exists true");
    Check(a1.size() == 2, "2. assignment value has exactly path+exists");
    Check(j.at("assignments").at("\\\\.\\DISPLAY2").at("exists") == false,
          "2. assignment exists false");
    Check(j.at("assignments").size() == 2, "2. assignments object size 2");
    Check(j.at("displays").is_array() && j.at("displays").size() == 1,
          "2. displays array passthrough");
  }

  // 3. Save/Load round trip + byte-equal re-save (row 5 happy QA scenario).
  {
    const auto p1 = dir / "t3_a.json";
    const auto p2 = dir / "t3_b.json";
    k6wp::DisplaysConfig cfg;
    cfg.assignments[L"\\\\.\\DISPLAY1"] = {L"C:\\Videos\\one.mp4", true};
    cfg.assignments[L"\\\\.\\DISPLAY2"] = {L"C:\\Videos\\two.webm", false};
    k6wp::SaveDisplays(p1, cfg);
    const std::string bytes1 = ReadText(p1);

    const k6wp::DisplaysConfig back = k6wp::LoadDisplays(p1);
    Check(back.version == k6wp::kDisplaysSchemaVersion, "3. round-trip version");
    Check(back.assignments.size() == 2, "3. round-trip assignments size");
    Check(back.assignments.at(L"\\\\.\\DISPLAY1").path == L"C:\\Videos\\one.mp4",
          "3. round-trip DISPLAY1 path");
    Check(back.assignments.at(L"\\\\.\\DISPLAY1").exists == true,
          "3. round-trip DISPLAY1 exists");
    Check(back.assignments.at(L"\\\\.\\DISPLAY2").path == L"C:\\Videos\\two.webm",
          "3. round-trip DISPLAY2 path");
    Check(back.assignments.at(L"\\\\.\\DISPLAY2").exists == false,
          "3. round-trip DISPLAY2 exists");

    k6wp::SaveDisplays(p2, back);
    const std::string bytes2 = ReadText(p2);
    Check(bytes1 == bytes2, "3. byte-equal round trip");

    const auto j = nlohmann::json::parse(ReadText(p1));
    Check(j.size() == 3 && j.contains("version") && j.contains("assignments") &&
              j.contains("displays"),
          "3. on-disk keys exactly version/assignments/displays");
  }

  // 4. DetectKeyCollision happy (row 5 QA): third key shares rect with first.
  {
    k6wp::DisplaysConfig cfg;
    cfg.assignments[L"\\\\.\\DISPLAY1"] = {L"C:\\v\\1.mp4", true};
    cfg.assignments[L"\\\\.\\DISPLAY2"] = {L"C:\\v\\2.mp4", true};
    cfg.assignments[L"\\\\.\\DISPLAY3"] = {L"C:\\v\\3.mp4", true};
    // DISPLAY3 is a clone of DISPLAY1: same physical rect.
    const std::vector<k6wp::MonitorInfo> monitors = {
        MakeMon(L"\\\\.\\DISPLAY1", 0, 0, 1920, 1080),
        MakeMon(L"\\\\.\\DISPLAY2", 1920, 0, 1920, 1080),
        MakeMon(L"\\\\.\\DISPLAY3", 0, 0, 1920, 1080),
    };
    const auto hits = k6wp::DetectKeyCollision(cfg, monitors);
    Check(hits.size() == 2, "4. collision reports exactly 2 keys");
    Check(VecHas(hits, L"\\\\.\\DISPLAY3"),
          "4. third key (same rect as first) is reported");
    Check(VecHas(hits, L"\\\\.\\DISPLAY1"),
          "4. first key (shared rect) is reported");
    Check(!VecHas(hits, L"\\\\.\\DISPLAY2"),
          "4. distinct-rect key is not reported");
  }

  // 5. DetectKeyCollision: duplicated szDevice in the live monitor list.
  {
    k6wp::DisplaysConfig cfg;
    cfg.assignments[L"\\\\.\\DISPLAY1"] = {L"C:\\v\\1.mp4", true};
    cfg.assignments[L"\\\\.\\DISPLAY2"] = {L"C:\\v\\2.mp4", true};
    // Clone mode: GetMonitorInfoW reports the same szDevice twice.
    const std::vector<k6wp::MonitorInfo> monitors = {
        MakeMon(L"\\\\.\\DISPLAY1", 0, 0, 1920, 1080),
        MakeMon(L"\\\\.\\DISPLAY1", 0, 0, 1920, 1080),
        MakeMon(L"\\\\.\\DISPLAY2", 1920, 0, 1920, 1080),
    };
    const auto hits = k6wp::DetectKeyCollision(cfg, monitors);
    Check(VecHas(hits, L"\\\\.\\DISPLAY1"),
          "5. duplicated szDevice key is reported (IS-7)");
    Check(!VecHas(hits, L"\\\\.\\DISPLAY2"),
          "5. distinct-rect key not reported with dup szDevice");
  }

  // 6. DetectKeyCollision: no collision / unresolvable keys / empty inputs.
  {
    k6wp::DisplaysConfig cfg;
    cfg.assignments[L"\\\\.\\DISPLAY1"] = {L"C:\\v\\1.mp4", true};
    cfg.assignments[L"\\\\.\\DISPLAY9"] = {L"C:\\v\\9.mp4", true};  // no monitor
    const std::vector<k6wp::MonitorInfo> monitors = {
        MakeMon(L"\\\\.\\DISPLAY1", 0, 0, 1920, 1080),
        MakeMon(L"\\\\.\\DISPLAY2", 1920, 0, 1920, 1080),
    };
    Check(k6wp::DetectKeyCollision(cfg, monitors).empty(),
          "6. distinct rects -> no collision");
    Check(k6wp::DetectKeyCollision(k6wp::DisplaysConfig{}, monitors).empty(),
          "6. empty assignments -> no collision");
    k6wp::DisplaysConfig only_orphan;
    only_orphan.assignments[L"\\\\.\\DISPLAY9"] = {L"C:\\v\\9.mp4", true};
    Check(k6wp::DetectKeyCollision(only_orphan, monitors).empty(),
          "6. unresolvable key ignored");
  }

  // 7. ValidateDisplays bounds (version range, non-empty path, reject ..,
  //    cap 100 entries).
  {
    k6wp::DisplaysConfig ok;
    ok.assignments[L"\\\\.\\DISPLAY1"] = {L"C:\\Videos\\a.mp4", true};
    Check(!ThrowsConfigError([&] { k6wp::ValidateDisplays(ok); }),
          "7. valid config accepted");

    k6wp::DisplaysConfig empty_path = ok;
    empty_path.assignments[L"\\\\.\\DISPLAY1"].path = L"";
    Check(ThrowsConfigError([&] { k6wp::ValidateDisplays(empty_path); }),
          "7. empty path rejected");

    k6wp::DisplaysConfig dotdot = ok;
    dotdot.assignments[L"\\\\.\\DISPLAY1"].path = L"C:\\Videos\\..\\evil.mp4";
    Check(ThrowsConfigError([&] { k6wp::ValidateDisplays(dotdot); }),
          "7. '..' component rejected");

    k6wp::DisplaysConfig rel_dotdot = ok;
    rel_dotdot.assignments[L"\\\\.\\DISPLAY1"].path = L"..\\evil.mp4";
    Check(ThrowsConfigError([&] { k6wp::ValidateDisplays(rel_dotdot); }),
          "7. leading '..' rejected");

    k6wp::DisplaysConfig ok_dots = ok;
    ok_dots.assignments[L"\\\\.\\DISPLAY1"].path = L"C:\\foo..bar\\video.mp4";
    Check(!ThrowsConfigError([&] { k6wp::ValidateDisplays(ok_dots); }),
          "7. '..' inside filename component accepted");

    k6wp::DisplaysConfig v0;
    v0.version = 0;
    Check(!ThrowsConfigError([&] { k6wp::ValidateDisplays(v0); }),
          "7. version 0 accepted");
    k6wp::DisplaysConfig v2;
    v2.version = 2;
    Check(ThrowsConfigError([&] { k6wp::ValidateDisplays(v2); }),
          "7. version 2 (future) rejected");
    k6wp::DisplaysConfig vm1;
    vm1.version = -1;
    Check(ThrowsConfigError([&] { k6wp::ValidateDisplays(vm1); }),
          "7. version -1 rejected");

    k6wp::DisplaysConfig big;
    for (int i = 0; i < 101; ++i) {
      big.assignments[L"\\\\.\\DISPLAY" + std::to_wstring(i)] = {
          L"C:\\v\\x.mp4", true};
    }
    Check(ThrowsConfigError([&] { k6wp::ValidateDisplays(big); }),
          "7. 101 entries rejected (cap 100)");

    k6wp::DisplaysConfig at_cap;
    for (int i = 0; i < 100; ++i) {
      at_cap.assignments[L"\\\\.\\DISPLAY" + std::to_wstring(i)] = {
          L"C:\\v\\x.mp4", true};
    }
    Check(!ThrowsConfigError([&] { k6wp::ValidateDisplays(at_cap); }),
          "7. 100 entries accepted");
  }

  // 8. MigrateDisplays: defaults, type errors, future version, non-object.
  {
    const auto cfg =
        k6wp::MigrateDisplays(nlohmann::json::parse(R"({"version":1})"));
    Check(cfg.version == k6wp::kDisplaysSchemaVersion,
          "8. migrate keeps current version");
    Check(cfg.assignments.empty(), "8. migrate defaults assignments empty");
    Check(cfg.displays.is_array() && cfg.displays.empty(),
          "8. migrate defaults displays empty");

    const auto v0 =
        k6wp::MigrateDisplays(nlohmann::json::parse(R"({"version":0})"));
    Check(v0.version == k6wp::kDisplaysSchemaVersion,
          "8. v0 migrates to current version");

    Check(ThrowsConfigError([&] {
            k6wp::MigrateDisplays(
                nlohmann::json::parse(R"({"version":1,"assignments":[]})"));
          }),
          "8. assignments non-object rejected");
    Check(ThrowsConfigError([&] {
            k6wp::MigrateDisplays(nlohmann::json::parse(
                R"({"version":1,"assignments":{"A":"x"}})"));
          }),
          "8. assignment value non-object rejected");
    Check(ThrowsConfigError([&] {
            k6wp::MigrateDisplays(nlohmann::json::parse(
                R"({"version":1,"assignments":{"A":{"exists":true}}})"));
          }),
          "8. assignment missing path rejected");
    Check(ThrowsConfigError([&] {
            k6wp::MigrateDisplays(
                nlohmann::json::parse(R"({"version":1,"displays":{}})"));
          }),
          "8. displays non-array rejected");
    Check(ThrowsConfigError([&] {
            k6wp::MigrateDisplays(nlohmann::json::parse(R"({"version":99})"));
          }),
          "8. future version rejected");
    Check(ThrowsConfigError([&] {
            k6wp::MigrateDisplays(nlohmann::json::parse("[]"));
          }),
          "8. non-object root rejected");

    const auto partial = k6wp::MigrateDisplays(nlohmann::json::parse(
        R"({"version":1,"assignments":{"\\\\.\\DISPLAY1":{"path":"C:\\v\\a.mp4"}}})"));
    Check(partial.assignments.at(L"\\\\.\\DISPLAY1").exists == false,
          "8. missing exists defaults false");
    Check(partial.assignments.at(L"\\\\.\\DISPLAY1").path == L"C:\\v\\a.mp4",
          "8. migrate reads assignment path");
  }

  // 9. Corrupt file throws ConfigError + .bak keeps raw bytes (fail QA).
  {
    const auto path = dir / "t9_corrupt.json";
    WriteText(path, "{ not json");
    Check(ThrowsConfigError([&] { (void)k6wp::LoadDisplays(path); }),
          "9. corrupt throws ConfigError");
    Check(std::filesystem::exists(BakPath(path)), "9. corrupt writes .bak");
    Check(ReadText(BakPath(path)) == "{ not json",
          "9. .bak keeps raw bytes");
  }

  // 10. Unknown version:99 throws + .bak keeps original bytes (fail QA).
  {
    const auto path = dir / "t10_v99.json";
    const std::string original = R"({"version":99,"assignments":{},"displays":[]})";
    WriteText(path, original);
    Check(ThrowsConfigError([&] { (void)k6wp::LoadDisplays(path); }),
          "10. version:99 throws ConfigError");
    Check(std::filesystem::exists(BakPath(path)),
          "10. version:99 writes .bak");
    Check(ReadText(BakPath(path)) == original,
          "10. .bak keeps original bytes");
  }

  // 11. Oversize file throws before parse (1 MiB cap, kMaxConfigBytes).
  {
    const auto big = dir / "t11_big.json";
    WriteText(big, std::string(2 * 1024 * 1024, 'x'));
    try {
      (void)k6wp::LoadDisplays(big);
      Check(false, "11. oversize throws (got success)");
    } catch (const k6wp::ConfigError& e) {
      Check(std::string(e.what()).find("exceeds maximum size") !=
                std::string::npos,
            "11. oversize throws (exceeds maximum size)");
    } catch (...) {
      Check(false, "11. oversize throws (wrong type)");
    }
  }

  // 12. Missing file throws ConfigError.
  {
    const auto missing = dir / "t12_missing.json";
    Check(ThrowsConfigError([&] { (void)k6wp::LoadDisplays(missing); }),
          "12. missing file throws ConfigError");
  }

  // 13. DefaultDisplaysPath -> %LOCALAPPDATA%/K6WP/displays.json.
  {
    const auto p = k6wp::DefaultDisplaysPath();
    Check(p.filename() == L"displays.json", "13. filename displays.json");
    Check(p.parent_path().filename() == L"K6WP", "13. parent dir K6WP");
    Check(p.parent_path() == k6wp::DefaultConfigPath().parent_path(),
          "13. same parent as config.json");
  }

  // 14. Migration self-heal: old file backed up and rewritten at v1.
  {
    const auto path = dir / "t14_v0.json";
    const std::string original = R"({"version":0,"assignments":{}})";
    WriteText(path, original);
    const auto cfg = k6wp::LoadDisplays(path);
    Check(cfg.version == k6wp::kDisplaysSchemaVersion,
          "14. v0 loads as current version");
    Check(std::filesystem::exists(BakPath(path)),
          "14. migration writes .bak");
    Check(ReadText(BakPath(path)) == original,
          "14. migration .bak keeps original bytes");
    const auto rewritten = nlohmann::json::parse(ReadText(path));
    Check(rewritten.at("version") == k6wp::kDisplaysSchemaVersion,
          "14. file rewritten at current version");
    Check(rewritten.contains("assignments") && rewritten.contains("displays"),
          "14. rewritten file has assignments+displays");
  }

  // 15. displays array passthrough survives save/load.
  {
    const auto path = dir / "t15_displays.json";
    k6wp::DisplaysConfig cfg;
    cfg.assignments[L"\\\\.\\DISPLAY1"] = {L"C:\\Videos\\a.mp4", true};
    cfg.displays = nlohmann::json::array({
        {{"name", "primary"}, {"orientation", 0}},
        {{"name", "secondary"}, {"orientation", 1}},
    });
    k6wp::SaveDisplays(path, cfg);
    const auto back = k6wp::LoadDisplays(path);
    Check(back.displays == cfg.displays, "15. displays array passthrough");
  }

  // 16. Save rejects invalid config with ConfigError and writes no file.
  {
    const auto path = dir / "t16_bad.json";
    k6wp::DisplaysConfig bad;
    bad.assignments[L"\\\\.\\DISPLAY1"] = {L"", true};
    Check(ThrowsConfigError([&] { k6wp::SaveDisplays(path, bad); }),
          "16. save rejects empty path");
    Check(!std::filesystem::exists(path), "16. rejected save writes no file");
  }

  // 17. Save creates parent directories (AtomicWriteJson contract).
  {
    const auto nested = dir / "sub" / "dir" / "t17.json";
    k6wp::DisplaysConfig cfg;
    cfg.assignments[L"\\\\.\\DISPLAY1"] = {L"C:\\Videos\\a.mp4", true};
    k6wp::SaveDisplays(nested, cfg);
    Check(std::filesystem::exists(nested), "17. save creates parent dirs");
    Check(k6wp::LoadDisplays(nested).assignments.size() == 1,
          "17. nested load works");
  }

  std::cout << "\n" << g_checks << " checks, " << g_failures << " failures\n";
  return g_failures == 0 ? 0 : 1;
}
