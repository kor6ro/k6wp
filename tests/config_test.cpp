// Mini unit tests for shared/config_schema (Todo 5). No external test
// framework: plain asserts with a pass/fail counter. Exit code 0 = all pass.
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include "config_schema.hpp"
#include "studio_settings.hpp"
#include "thirdparty/json.hpp"
#include "version_compare.hpp"

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
  auto dir = std::filesystem::temp_directory_path() / "k6wp_config_test";
  std::error_code ec;
  std::filesystem::remove_all(dir, ec);  // fresh per run
  std::filesystem::create_directories(dir, ec);
  return dir;
}

void WriteText(const std::filesystem::path& p, const std::string& text) {
  std::ofstream out(p, std::ios::binary | std::ios::trunc);
  out << text;
}

void WriteJson(const std::filesystem::path& p, const nlohmann::json& j) {
  WriteText(p, j.dump(2));
}

std::string ReadText(const std::filesystem::path& p) {
  std::ifstream in(p, std::ios::binary);
  return std::string(std::istreambuf_iterator<char>(in),
                     std::istreambuf_iterator<char>());
}

// Mirrors the loader's "<config>.bak" convention (filename + ".bak").
std::filesystem::path BakPath(const std::filesystem::path& p) {
  std::filesystem::path bak = p;
  bak += L".bak";
  return bak;
}

nlohmann::json ValidJson() {
  return {
      {"version", 5},
      {"video_path", "C:\\videos\\test.mp4"},
      {"fit_mode", "cover"},
      {"speed", 1.0},
      {"monitor_id", 0},
      {"crf", 23},
      {"resolution_w", 1920},
      {"resolution_h", 1080},
      {"fps_cap", 30},
      {"battery_saver", false},
      {"battery_mode", "cap24"},
      {"cpu_affinity", "auto"},
      {"gpu_adapter", "auto"},
  };
}

// LoadConfig must throw ConfigError (structured error, not a crash).
void ExpectLoadThrows(const std::filesystem::path& p, const std::string& name) {
  try {
    (void)k6wp::LoadConfig(p);
    Check(false, name + " (expected ConfigError, got success)");
  } catch (const k6wp::ConfigError&) {
    Check(true, name);
  } catch (const std::exception& e) {
    Check(false, name + " (wrong exception type: " + e.what() + ")");
  }
}

// LoadConfig must succeed and return a config.
bool ExpectLoadOk(const std::filesystem::path& p, const std::string& name,
                  k6wp::WallpaperConfig* out) {
  try {
    *out = k6wp::LoadConfig(p);
    Check(true, name);
    return true;
  } catch (const std::exception& e) {
    Check(false, name + " (unexpected throw: " + e.what() + ")");
    return false;
  }
}

}  // namespace

// Structural-review guard: the human-readable config_schema.json must track
// the code contract (kConfigSchemaVersion + the ConfigToJson field set).
// CTest passes the schema path as argv[1]; without it the section skips.
void CheckSchemaDocInSync(const char* schema_path) {
  if (schema_path == nullptr || *schema_path == '\0') {
    std::cout << "[SKIP] schema-doc sync (no path passed)\n";
    return;
  }
  std::error_code ec;
  if (!std::filesystem::is_regular_file(schema_path, ec)) {
    Check(false, std::string("schema doc readable: ") + schema_path);
    return;
  }
  try {
    const auto schema = nlohmann::json::parse(ReadText(schema_path));
    const auto& props = schema.at("properties");
    const auto& ver = props.at("version");
    Check(ver.at("maximum") == k6wp::kConfigSchemaVersion,
          "schema doc version.maximum == kConfigSchemaVersion");

    const nlohmann::json on_disk = k6wp::ConfigToJson(k6wp::WallpaperConfig{});
    for (auto it = on_disk.begin(); it != on_disk.end(); ++it) {
      Check(props.contains(it.key()),
            "schema doc documents ConfigToJson field '" + it.key() + "'");
    }
    for (auto it = props.begin(); it != props.end(); ++it) {
      Check(on_disk.contains(it.key()),
            "ConfigToJson covers schema doc field '" + it.key() + "'");
    }
  } catch (const std::exception& e) {
    Check(false, std::string("schema doc parse: ") + e.what());
  }
}

int main(int argc, char* argv[]) {
  const auto dir = TempDir();

  // 1. Load valid config -> fields correct.
  {
    const auto p = dir / "valid.json";
    WriteJson(p, ValidJson());
    k6wp::WallpaperConfig cfg;
    if (ExpectLoadOk(p, "1. load valid config", &cfg)) {
      Check(cfg.version == 5, "1. version == 5");
      Check(cfg.video_path == L"C:\\videos\\test.mp4", "1. video_path correct");
      Check(cfg.fit_mode == "cover", "1. fit_mode correct");
      Check(cfg.speed == 1.0, "1. speed correct");
      Check(cfg.monitor_id == 0, "1. monitor_id kept (v2 stores as-is)");
      Check(cfg.crf == 23, "1. crf correct");
      Check(cfg.resolution_w == 1920 && cfg.resolution_h == 1080, "1. resolution correct");
      Check(cfg.fps_cap == 30, "1. fps_cap correct");
      Check(cfg.battery_saver == false, "1. battery_saver correct");
      Check(cfg.battery_mode == "cap24", "1. battery_mode correct");
      Check(cfg.cpu_affinity == "auto", "1. cpu_affinity correct");
      Check(cfg.gpu_adapter == "auto", "1. gpu_adapter correct");
    }
  }

  // 2. CRF 0 -> rejected.
  {
    const auto p = dir / "crf0.json";
    auto j = ValidJson();
    j["crf"] = 0;
    WriteJson(p, j);
    ExpectLoadThrows(p, "2. crf 0 rejected");
  }

  // 3. CRF 51 -> rejected.
  {
    const auto p = dir / "crf51.json";
    auto j = ValidJson();
    j["crf"] = 51;
    WriteJson(p, j);
    ExpectLoadThrows(p, "3. crf 51 rejected");
  }

  // 4. CRF 20 -> accepted.
  {
    const auto p = dir / "crf20.json";
    auto j = ValidJson();
    j["crf"] = 20;
    WriteJson(p, j);
    k6wp::WallpaperConfig cfg;
    if (ExpectLoadOk(p, "4. crf 20 accepted", &cfg)) {
      Check(cfg.crf == 20, "4. crf == 20");
    }
  }

  // 5. speed 0.1 -> rejected.
  {
    const auto p = dir / "speed01.json";
    auto j = ValidJson();
    j["speed"] = 0.1;
    WriteJson(p, j);
    ExpectLoadThrows(p, "5. speed 0.1 rejected");
  }

  // 6. speed 3.0 -> rejected.
  {
    const auto p = dir / "speed30.json";
    auto j = ValidJson();
    j["speed"] = 3.0;
    WriteJson(p, j);
    ExpectLoadThrows(p, "6. speed 3.0 rejected");
  }

  // 7. speed 1.5 -> accepted.
  {
    const auto p = dir / "speed15.json";
    auto j = ValidJson();
    j["speed"] = 1.5;
    WriteJson(p, j);
    k6wp::WallpaperConfig cfg;
    if (ExpectLoadOk(p, "7. speed 1.5 accepted", &cfg)) {
      Check(cfg.speed == 1.5, "7. speed == 1.5");
    }
  }

  // 8. Corrupt JSON (garbage bytes) -> structured ConfigError, no crash.
  {
    const auto p = dir / "garbage.json";
    WriteText(p, std::string("\xFF\xFE\x00\x01garbage\xFF\xFF", 10));
    ExpectLoadThrows(p, "8. corrupt JSON -> ConfigError");
  }

  // 9. Migrate v0 (missing new fields) -> v5 with defaults filled, no crash.
  // Step 5: v0/v1 monitor_id is forced to -1 (the old id had no render
  // effect; preserving it would newly single-out one screen).
  {
    const nlohmann::json v0 = {{"video_path", "C:\\videos\\old.mp4"}};
    const k6wp::WallpaperConfig cfg = k6wp::MigrateConfig(v0);
    Check(cfg.version == 5, "9. migrated version == 5");
    Check(cfg.video_path == L"C:\\videos\\old.mp4", "9. video_path preserved");
    Check(cfg.fit_mode == "cover", "9. fit_mode defaulted");
    Check(cfg.speed == 1.0, "9. speed defaulted");
    Check(cfg.monitor_id == -1, "9. monitor_id forced to -1 (all screens)");
    Check(cfg.crf == 22, "9. crf defaulted");
    Check(cfg.resolution_w == 0 && cfg.resolution_h == 0, "9. resolution defaulted");
    Check(cfg.fps_cap == 24, "9. fps_cap defaulted");
    Check(cfg.battery_saver == false, "9. battery_saver defaulted");
    Check(cfg.battery_mode == "cap24", "9. battery_mode defaulted");
    Check(cfg.cpu_affinity == "auto", "9. cpu_affinity defaulted");
    Check(cfg.gpu_adapter == "auto", "9. gpu_adapter defaulted");
  }

  // 10. Round-trip: Save -> Load -> identical fields (incl. UTF-8 path).
  // Schema v5; monitor_id 2 stays 2 and stretch is unaffected by the
  // fill->cover normalization.
  {
    k6wp::WallpaperConfig cfg;
    cfg.version = 5;
    cfg.video_path = L"C:\\videos\\배경화면 wallpaper.mp4";
    cfg.fit_mode = "stretch";
    cfg.speed = 1.5;
    cfg.monitor_id = 2;
    cfg.crf = 20;
    cfg.resolution_w = 1920;
    cfg.resolution_h = 1080;
    cfg.fps_cap = 24;
    cfg.battery_saver = true;
    cfg.cpu_affinity = "all";
    cfg.gpu_adapter = "discrete";
    const auto p = dir / "roundtrip" / "config.json";
    k6wp::SaveConfig(p, cfg);
    const k6wp::WallpaperConfig loaded = k6wp::LoadConfig(p);
    Check(loaded.version == cfg.version, "10. version identical");
    Check(loaded.video_path == cfg.video_path, "10. video_path identical (UTF-8)");
    Check(loaded.fit_mode == cfg.fit_mode, "10. fit_mode identical");
    Check(loaded.speed == cfg.speed, "10. speed identical");
    Check(loaded.monitor_id == cfg.monitor_id, "10. monitor_id identical");
    Check(loaded.crf == cfg.crf, "10. crf identical");
    Check(loaded.resolution_w == cfg.resolution_w && loaded.resolution_h == cfg.resolution_h,
          "10. resolution identical");
    Check(loaded.fps_cap == cfg.fps_cap, "10. fps_cap identical");
    Check(loaded.battery_saver == cfg.battery_saver, "10. battery_saver identical");
    Check(loaded.battery_mode == cfg.battery_mode, "10. battery_mode identical");
    Check(loaded.cpu_affinity == cfg.cpu_affinity, "10. cpu_affinity identical");
    Check(loaded.gpu_adapter == cfg.gpu_adapter, "10. gpu_adapter identical");
  }

  // 11. Migration via LoadConfig: v0 minimal (no version, only video_path)
  // -> v5 defaults + "<config>.bak" backup of the original bytes, and the
  // file itself is rewritten at v5 (self-healing: reloads without migrating).
  // Step 5: monitor_id migrates to -1 (all screens).
  {
    const auto p = dir / "mig_v0.json";
    const std::string original = R"({"video_path": "C:\\videos\\old.mp4"})";
    WriteText(p, original);
    k6wp::WallpaperConfig cfg;
    if (ExpectLoadOk(p, "11. v0 minimal migrates", &cfg)) {
      Check(cfg.version == 5, "11. version == 5");
      Check(cfg.video_path == L"C:\\videos\\old.mp4", "11. video_path preserved");
      Check(cfg.fit_mode == "cover" && cfg.speed == 1.0 && cfg.monitor_id == -1,
            "11. fit/speed defaulted, monitor forced -1");
      Check(cfg.crf == 22 && cfg.resolution_w == 0 && cfg.resolution_h == 0,
            "11. crf/resolution defaulted");
      Check(cfg.fps_cap == 24 && cfg.battery_saver == false,
            "11. fps_cap/battery_saver defaulted");
      Check(cfg.battery_mode == "cap24" && cfg.cpu_affinity == "auto" &&
                cfg.gpu_adapter == "auto",
            "11. battery_mode/cpu_affinity/gpu_adapter defaulted");
    }
    const auto bak = BakPath(p);
    Check(std::filesystem::exists(bak), "11. .bak backup written");
    Check(ReadText(bak) == original, "11. .bak holds original bytes");
    k6wp::WallpaperConfig cfg2;
    if (ExpectLoadOk(p, "11. rewritten file loads clean", &cfg2)) {
      Check(cfg2.version == 5 && cfg2.battery_saver == false,
            "11. rewritten v5 stable");
    }
  }

  // 12. Migration via LoadConfig: v5 missing battery_saver (field added
  // after v5 files existed) -> defaulted false + .bak backup.
  {
    const auto p = dir / "mig_nosaver.json";
    auto j = ValidJson();
    j.erase("battery_saver");
    WriteJson(p, j);
    const std::string original = ReadText(p);
    k6wp::WallpaperConfig cfg;
    if (ExpectLoadOk(p, "12. v5-no-battery-saver migrates", &cfg)) {
      Check(cfg.version == 5, "12. version == 5");
      Check(cfg.battery_saver == false, "12. battery_saver defaulted");
      Check(cfg.crf == 23 && cfg.monitor_id == 0, "12. other fields preserved");
    }
    const auto bak = BakPath(p);
    Check(std::filesystem::exists(bak), "12. .bak backup written");
    Check(ReadText(bak) == original, "12. .bak holds original bytes");
  }

  // 13. Migration via LoadConfig: v5 missing monitor fields (monitor_id,
  // resolution_w/h) -> monitor_id -1 (all screens) + .bak backup.
  {
    const auto p = dir / "mig_nomonitor.json";
    auto j = ValidJson();
    j.erase("monitor_id");
    j.erase("resolution_w");
    j.erase("resolution_h");
    WriteJson(p, j);
    const std::string original = ReadText(p);
    k6wp::WallpaperConfig cfg;
    if (ExpectLoadOk(p, "13. v5-no-monitor migrates", &cfg)) {
      Check(cfg.version == 5, "13. version == 5");
      Check(cfg.monitor_id == -1, "13. monitor_id defaulted to -1");
      Check(cfg.resolution_w == 0 && cfg.resolution_h == 0,
            "13. resolution defaulted");
      Check(cfg.fit_mode == "cover" && cfg.battery_saver == false,
            "13. other fields preserved");
    }
    const auto bak = BakPath(p);
    Check(std::filesystem::exists(bak), "13. .bak backup written");
    Check(ReadText(bak) == original, "13. .bak holds original bytes");
  }

  // 14. QA-fail path: corrupt JSON still throws ConfigError (caller falls
  // back to safe defaults) AND leaves a .bak backup of the bad bytes.
  {
    const auto p = dir / "mig_garbage.json";
    const std::string original("\xFF\xFE\x00\x01garbage\xFF\xFF", 10);
    WriteText(p, original);
    ExpectLoadThrows(p, "14. corrupt -> ConfigError");
    const auto bak = BakPath(p);
    Check(std::filesystem::exists(bak), "14. .bak backup written for corrupt input");
    Check(ReadText(bak) == original, "14. .bak holds corrupt bytes");
  }

  // 15. QA-fail path: unknown fit_mode (valid JSON, no migration needed)
  // -> ConfigError + .bak backup of the original bytes, so the caller can
  // fall back to last-valid/safe defaults without losing the bad input.
  {
    const auto p = dir / "bogus_fit.json";
    auto j = ValidJson();
    j["fit_mode"] = "bogus";
    WriteJson(p, j);
    const std::string original = ReadText(p);
    ExpectLoadThrows(p, "15. unknown fit_mode rejected");
    const auto bak = BakPath(p);
    Check(std::filesystem::exists(bak), "15. .bak backup written for invalid fit_mode");
    Check(ReadText(bak) == original, "15. .bak holds original bytes");
  }

  // 16. Studio defaults match the plan example exactly (crf 22, fps 30,
  // match_monitor, wallpapers dir, start_with_windows false,
  // auto_compress_on_import true). v3 removed keep_original/start_minimized.
  // NOTE (P3L.1): studio_settings schema stays v3 — the v4→v5 bump is
  // engine-config only, so every "version == 3" below in scenarios 16, 17,
  // 21, 25, 26, 29, 30 is correct as-is.
  {
    const k6wp::StudioSettings s = k6wp::DefaultStudioSettings();
    Check(s.version == 3, "16. version == 3");
    Check(s.auto_compress_on_import == true,
          "16. auto_compress_on_import default true");
    Check(s.default_crf == 22, "16. default_crf == 22");
    Check(s.default_fps == 30, "16. default_fps == 30");
    Check(s.default_resolution_mode == "match_monitor",
          "16. default_resolution_mode == match_monitor");
    Check(s.start_with_windows == false,
          "16. start_with_windows default false");
    Check(std::filesystem::path(s.compress_output_dir).filename() ==
              L"wallpapers",
          "16. compress_output_dir defaults to wallpapers dir");
    Check(std::filesystem::path(s.cache_dir).filename() == L"cache",
          "16. cache_dir defaults to cache dir");
  }

  // 17. Studio v0 (missing version/fields) -> v3 defaults + .bak backup,
  // and the file is rewritten at v3 (reloads without migrating).
  {
    const auto p = dir / "studio_v0.json";
    const std::string original = R"({})";
    WriteText(p, original);
    k6wp::StudioSettings s;
    try {
      s = k6wp::LoadStudioSettings(p);
      Check(true, "17. studio v0 migrates");
    } catch (const std::exception& e) {
      Check(false,
            std::string("17. studio v0 migrates (threw: ") + e.what() + ")");
    }
    Check(s.version == 3, "17. version == 3");  // studio stays v3 (see 16 note)
    Check(s.auto_compress_on_import == true && s.default_crf == 22 &&
              s.default_fps == 30 &&
              s.default_resolution_mode == "match_monitor" &&
              s.start_with_windows == false,
          "17. plan-example defaults filled");
    const auto bak = BakPath(p);
    Check(std::filesystem::exists(bak), "17. .bak backup written");
    Check(ReadText(bak) == original, "17. .bak holds original bytes");
    try {
      const k6wp::StudioSettings s2 = k6wp::LoadStudioSettings(p);
      Check(s2.version == 3 && s2.default_crf == 22,
            "17. rewritten v3 stable");  // studio stays v3 (see 16 note)
    } catch (const std::exception& e) {
      Check(false,
            std::string("17. rewritten v3 stable (threw: ") + e.what() + ")");
    }
  }
  // 18. Studio corrupt JSON -> ConfigError + .bak, and the caller-side
  // last-valid fallback survives without throwing further.
  {
    const auto p = dir / "studio_garbage.json";
    const std::string original("\xFF\xFE\x00\x01garbage\xFF\xFF", 10);
    WriteText(p, original);
    try {
      (void)k6wp::LoadStudioSettings(p);
      Check(false, "18. corrupt studio JSON rejected (got success)");
    } catch (const k6wp::ConfigError&) {
      Check(true, "18. corrupt studio JSON rejected");
    } catch (const std::exception& e) {
      Check(false, std::string("18. corrupt studio JSON rejected (wrong type: ") +
                                  e.what() + ")");
    }
    const auto bak = BakPath(p);
    Check(std::filesystem::exists(bak),
          "18. .bak backup written for corrupt input");
    Check(ReadText(bak) == original, "18. .bak holds corrupt bytes");
    // Caller-side last-valid fallback: a failed load must not throw out of
    // the fallback itself.
    k6wp::StudioSettings last_valid = k6wp::DefaultStudioSettings();
    bool fallback_ok = false;
    try {
      last_valid = k6wp::LoadStudioSettings(p);
    } catch (const k6wp::ConfigError&) {
      fallback_ok = true;  // kept last_valid, no further throw
    }
    Check(fallback_ok && last_valid.default_crf == 22,
          "18. last-valid fallback survives without throw");
  }

  // 19. Studio round-trip: Save -> Load -> identical fields.
  {
    k6wp::StudioSettings s = k6wp::DefaultStudioSettings();
    s.start_with_windows = true;
    s.default_crf = 20;
    s.default_resolution_mode = "1080p";
    const auto p = dir / "studio_roundtrip" / "studio_settings.json";
    k6wp::SaveStudioSettings(p, s);
    try {
      const k6wp::StudioSettings loaded = k6wp::LoadStudioSettings(p);
      Check(loaded.auto_compress_on_import == s.auto_compress_on_import,
            "19. auto_compress identical");
      Check(loaded.compress_output_dir == s.compress_output_dir,
            "19. compress_output_dir identical");
      Check(loaded.default_crf == 20, "19. default_crf identical");
      Check(loaded.default_fps == s.default_fps, "19. default_fps identical");
      Check(loaded.default_resolution_mode == "1080p",
            "19. resolution_mode identical");
      Check(loaded.start_with_windows == true,
            "19. start_with_windows identical");
      Check(loaded.cache_dir == s.cache_dir, "19. cache_dir identical");
    } catch (const std::exception& e) {
      Check(false,
            std::string("19. round-trip loads (threw: ") + e.what() + ")");
    }
  }

  // 20. Studio invalid values (bad crf, unknown resolution mode) ->
  // ConfigError + .bak, engine-style config untouched by the split.
  {
    const auto p = dir / "studio_badcrf.json";
    nlohmann::json j = {
        {"version", 1},
        {"auto_compress_on_import", true},
        {"compress_output_dir", "C:\\K6WP\\wallpapers"},
        {"default_crf", 99},
        {"default_fps", 30},
        {"default_resolution_mode", "match_monitor"},
        {"start_with_windows", false},
        {"cache_dir", "C:\\K6WP\\cache"},
    };
    WriteJson(p, j);
    const std::string original = ReadText(p);
    try {
      (void)k6wp::LoadStudioSettings(p);
      Check(false, "20. bad default_crf rejected (got success)");
    } catch (const k6wp::ConfigError&) {
      Check(true, "20. bad default_crf rejected");
    }
    Check(std::filesystem::exists(BakPath(p)),
          "20. .bak backup written for invalid crf");
    Check(ReadText(BakPath(p)) == original,
          "20. .bak holds original bytes");

    const auto q = dir / "studio_badmode.json";
    j["default_crf"] = 22;
    j["default_resolution_mode"] = "bogus";
    WriteJson(q, j);
    try {
      (void)k6wp::LoadStudioSettings(q);
      Check(false, "20. unknown resolution mode rejected (got success)");
    } catch (const k6wp::ConfigError&) {
      Check(true, "20. unknown resolution mode rejected");
    }
    Check(std::filesystem::exists(BakPath(q)),
          "20. .bak backup written for invalid mode");
  }

  // 21. Plan-example JSON (exact keys from the todo quote) migrates to the
  // exact example values.
  {
    const nlohmann::json example = {
        {"auto_compress_on_import", true},
        {"compress_output_dir",
         k6wp::DefaultCompressOutputDir().u8string()},
        {"default_crf", 22},
        {"default_fps", 30},
        {"default_resolution_mode", "match_monitor"},
        {"start_with_windows", false},
    };
    const k6wp::StudioSettings s = k6wp::MigrateStudioSettings(example);
    Check(s.version == 3, "21. example version == 3");  // studio stays v3 (see 16 note)
    Check(s.auto_compress_on_import == true &&
              s.compress_output_dir ==
                  k6wp::DefaultCompressOutputDir().wstring(),
          "21. example output dir kept");
    Check(s.default_crf == 22 && s.default_fps == 30 &&
              s.default_resolution_mode == "match_monitor",
          "21. example crf/fps/mode exact");
    Check(s.start_with_windows == false,
          "21. example flags exact");
    Check(std::filesystem::path(k6wp::DefaultStudioSettingsPath())
                  .filename() == L"studio_settings.json",
          "21. default path is studio_settings.json");
  }

  // 22. Step 5: v1 file carrying monitor_id 0 migrates to -1 (the v1 id
  // had no render effect) + .bak holds the original bytes.
  {
    const auto p = dir / "mig_v1mon0.json";
    auto j = ValidJson();
    j["version"] = 1;
    j["monitor_id"] = 0;
    WriteJson(p, j);
    const std::string original = ReadText(p);
    k6wp::WallpaperConfig cfg;
    if (ExpectLoadOk(p, "22. v1 monitor_id 0 migrates", &cfg)) {
      Check(cfg.version == 5, "22. version == 5");
      Check(cfg.monitor_id == -1, "22. monitor_id forced to -1");
    }
    const auto bak = BakPath(p);
    Check(std::filesystem::exists(bak), "22. .bak backup written");
    Check(ReadText(bak) == original, "22. .bak holds original bytes");
  }

  // 23. Step 5: monitor_id -2 is rejected (ConfigError); -1 is accepted.
  {
    const auto p = dir / "mon_neg2.json";
    auto j = ValidJson();
    j["monitor_id"] = -2;
    WriteJson(p, j);
    ExpectLoadThrows(p, "23. monitor_id -2 rejected");
  }
  {
    const auto p = dir / "mon_neg1.json";
    auto j = ValidJson();
    j["monitor_id"] = -1;
    WriteJson(p, j);
    k6wp::WallpaperConfig cfg;
    if (ExpectLoadOk(p, "23. monitor_id -1 accepted", &cfg)) {
      Check(cfg.monitor_id == -1, "23. monitor_id -1 kept");
    }
  }

  // 24. v2 file with legacy fit_mode "fill" loads as "cover" (v3
  // normalization) + .bak holds the original bytes + rewrite is stable.
  {
    const auto p = dir / "mig_v2fill.json";
    auto j = ValidJson();
    j["version"] = 2;
    j["fit_mode"] = "fill";
    WriteJson(p, j);
    const std::string original = ReadText(p);
    k6wp::WallpaperConfig cfg;
    if (ExpectLoadOk(p, "24. v2 fill migrates", &cfg)) {
      Check(cfg.version == 5, "24. version == 5");
      Check(cfg.fit_mode == "cover", "24. fill normalized to cover");
    }
    const auto bak = BakPath(p);
    Check(std::filesystem::exists(bak), "24. .bak backup written");
    Check(ReadText(bak) == original, "24. .bak holds original bytes");
    k6wp::WallpaperConfig cfg2;
    if (ExpectLoadOk(p, "24. rewritten file loads clean", &cfg2)) {
      Check(cfg2.version == 5 && cfg2.fit_mode == "cover",
            "24. rewritten v5 stable");
    }
  }

  // 25. Studio v1 file migrates to v3 with compress_advanced_visible=false
  // + .bak holds the original bytes + rewrite is stable.
  {
    const auto p = dir / "studio_v1.json";
    nlohmann::json j = {
        {"version", 1},
        {"auto_compress_on_import", true},
        {"compress_output_dir", "C:\\K6WP\\wallpapers"},
        {"default_crf", 22},
        {"default_fps", 30},
        {"default_resolution_mode", "match_monitor"},
        {"start_with_windows", false},
        {"cache_dir", "C:\\K6WP\\cache"},
    };
    WriteJson(p, j);
    const std::string original = ReadText(p);
    try {
      const k6wp::StudioSettings s = k6wp::LoadStudioSettings(p);
      Check(true, "25. studio v1 migrates");
      Check(s.version == 3, "25. version == 3");  // studio stays v3 (see 16 note)
      Check(s.compress_advanced_visible == false,
            "25. compress_advanced_visible defaulted false");
    } catch (const std::exception& e) {
      Check(false,
            std::string("25. studio v1 migrates (threw: ") + e.what() + ")");
    }
    const auto bak = BakPath(p);
    Check(std::filesystem::exists(bak), "25. .bak backup written");
    Check(ReadText(bak) == original, "25. .bak holds original bytes");
    try {
      const k6wp::StudioSettings s2 = k6wp::LoadStudioSettings(p);
      Check(s2.version == 3 && s2.compress_advanced_visible == false,
            "25. rewritten v3 stable");  // studio stays v3 (see 16 note)
    } catch (const std::exception& e) {
      Check(false,
            std::string("25. rewritten v3 stable (threw: ") + e.what() + ")");
    }
  }

  // 26. Studio v2 file carrying the removed keep_original/start_minimized
  // keys migrates to v3: keys ignored (no error), .bak holds the original
  // bytes, rewrite is stable and drops the removed keys.
  {
    const auto p = dir / "studio_v2_deadkeys.json";
    nlohmann::json j = {
        {"version", 2},
        {"auto_compress_on_import", true},
        {"compress_output_dir", "C:\\K6WP\\wallpapers"},
        {"default_crf", 22},
        {"default_fps", 30},
        {"default_resolution_mode", "match_monitor"},
        {"keep_original", true},
        {"start_with_windows", false},
        {"start_minimized", true},
        {"cache_dir", "C:\\K6WP\\cache"},
        {"lockscreen_sync", false},
        {"lockscreen_offset_sec", 1.0},
        {"compress_advanced_visible", true},
    };
    WriteJson(p, j);
    const std::string original = ReadText(p);
    try {
      const k6wp::StudioSettings s = k6wp::LoadStudioSettings(p);
      Check(true, "26. studio v2 dead-keys migrates");
      Check(s.version == 3, "26. version == 3");  // studio stays v3 (see 16 note)
      Check(s.compress_advanced_visible == true &&
                s.default_crf == 22 && s.start_with_windows == false,
            "26. surviving fields preserved");
    } catch (const std::exception& e) {
      Check(false,
            std::string("26. studio v2 dead-keys migrates (threw: ") +
                e.what() + ")");
    }
    const auto bak = BakPath(p);
    Check(std::filesystem::exists(bak), "26. .bak backup written");
    Check(ReadText(bak) == original, "26. .bak holds original bytes");
    try {
      const k6wp::StudioSettings s2 = k6wp::LoadStudioSettings(p);
      Check(s2.version == 3, "26. rewritten v3 stable");  // studio stays v3 (see 16 note)
      const std::string rewritten = ReadText(p);
      Check(rewritten.find("keep_original") == std::string::npos &&
                rewritten.find("start_minimized") == std::string::npos,
            "26. rewritten file drops removed keys");
    } catch (const std::exception& e) {
      Check(false,
            std::string("26. rewritten v3 stable (threw: ") + e.what() + ")");
    }
  }

  // 27. check_updates defaults to true in fresh defaults.
  {
    const k6wp::StudioSettings s = k6wp::DefaultStudioSettings();
    Check(s.check_updates == true, "27. check_updates default true");
  }

  // 28. v0 (empty object) migrates check_updates to true.
  {
    const k6wp::StudioSettings s =
        k6wp::MigrateStudioSettings(nlohmann::json::object());
    Check(s.check_updates == true, "28. v0 migrates check_updates true");
  }

  // 29. Explicit false survives Migrate + Save/Load round-trip.
  {
    const nlohmann::json j = {{"version", 3}, {"check_updates", false}};
    const k6wp::StudioSettings m = k6wp::MigrateStudioSettings(j);
    Check(m.check_updates == false, "29. migrate keeps explicit false");
    k6wp::StudioSettings s = k6wp::DefaultStudioSettings();
    s.check_updates = false;
    const auto p = dir / "studio_roundtrip_noupdate" / "studio_settings.json";
    k6wp::SaveStudioSettings(p, s);
    try {
      const k6wp::StudioSettings loaded = k6wp::LoadStudioSettings(p);
      Check(loaded.check_updates == false, "29. round-trip keeps false");
    } catch (const std::exception& e) {
      Check(false,
            std::string("29. round-trip loads (threw: ") + e.what() + ")");
    }
  }

  // 30. v3 file missing check_updates loads true + .bak + stable rewrite.
  {
    const auto p = dir / "studio_nocheck.json";
    const nlohmann::json j = {
        {"version", 3},
        {"auto_compress_on_import", true},
        {"compress_output_dir", "C:\\K6WP\\wallpapers"},
        {"default_crf", 22},
        {"default_fps", 30},
        {"default_resolution_mode", "match_monitor"},
        {"start_with_windows", false},
        {"cache_dir", "C:\\K6WP\\cache"},
        {"lockscreen_sync", false},
        {"lockscreen_offset_sec", 1.0},
        {"compress_advanced_visible", false},
    };
    WriteJson(p, j);
    const std::string original = ReadText(p);
    try {
      const k6wp::StudioSettings s = k6wp::LoadStudioSettings(p);
      Check(true, "30. v3-no-check_updates migrates");
      Check(s.check_updates == true, "30. check_updates defaulted true");
    } catch (const std::exception& e) {
      Check(false,
            std::string("30. v3-no-check_updates migrates (threw: ") +
                e.what() + ")");
    }
    const auto bak = BakPath(p);
    Check(std::filesystem::exists(bak), "30. .bak backup written");
    Check(ReadText(bak) == original, "30. .bak holds original bytes");
    try {
      const k6wp::StudioSettings s2 = k6wp::LoadStudioSettings(p);
      Check(s2.version == 3 && s2.check_updates == true,
            "30. rewritten v3 stable");  // studio stays v3 (see 16 note)
    } catch (const std::exception& e) {
      Check(false,
            std::string("30. rewritten v3 stable (threw: ") + e.what() + ")");
    }
  }

  // 31. Semver compare helper (shared/version_compare.hpp).
  {
    Check(k6wp::IsNewerVersion("1.0.1", "1.0.0"), "31. patch newer");
    Check(!k6wp::IsNewerVersion("1.0.0", "1.0.0"), "31. equal not newer");
    Check(!k6wp::IsNewerVersion("1.0.0", "1.0.1"), "31. older not newer");
    Check(k6wp::IsNewerVersion("v2.0.0", "1.9.9"),
          "31. leading v stripped");
    Check(!k6wp::IsNewerVersion("1.2", "1.2.0"),
          "31. missing part is zero");
    Check(k6wp::IsNewerVersion("1.10.0", "1.9.0"),
          "31. numeric not lexical");
    Check(k6wp::IsNewerVersion("1.0.0", "1.0.0-beta"),
          "31. release beats prerelease");
    Check(!k6wp::IsNewerVersion("1.0.0-beta", "1.0.0"),
          "31. prerelease not newer");
    Check(!k6wp::IsNewerVersion("abc", "1.0.0"),
          "31. invalid latest silent");
    Check(!k6wp::IsNewerVersion("1.0.0", ""),
          "31. invalid current silent");
    Check(!k6wp::IsNewerVersion("", ""), "31. both invalid silent");
    Check(k6wp::CompareSemver("1.2.3", "1.2.3") == 0,
          "31. compare equal");
    Check(k6wp::CompareSemver("2.0", "10.0") < 0, "31. compare major");
    // MED-8: ParseSemver must not overflow int32 on absurdly long numeric
    // components. "1.4294967309" wraps to 13 mid-parse on MSVC (signed
    // overflow) and "1.2147483648" wraps to INT_MIN; both must clamp to 1e9.
    Check(k6wp::CompareSemver("1.4294967309", "1.13") > 0,
          "31. long component clamps (no int overflow)");
    Check(k6wp::CompareSemver("1.2147483648", "1.0") > 0,
          "31. INT_MAX+1 component clamps (no wrap)");
    Check(k6wp::CompareSemver("1.99999999999999999999", "1.1000000000") == 0,
          "31. absurd long component clamps to 1e9");
    Check(k6wp::CompareSemver(
              "99999999999999999999999999999999999999999999999999.0",
              "1000000000.0") == 0,
          "31. 50-digit component clamps to 1e9");
    const auto pv = k6wp::ParseSemver("1.99999999999999999999");
    Check(pv.valid && pv.parts.size() == 2 && pv.parts[1] == 1000000000,
          "31. ParseSemver clamps long component to 1e9");
  }

  // 32. P1.3: fps_cap defaults to 24 for new configs (missing key).
  {
    const k6wp::WallpaperConfig fresh;
    Check(fresh.fps_cap == 24, "32. default-constructed fps_cap == 24");
    const nlohmann::json v0 = {{"video_path", "C:\\videos\\new.mp4"}};
    const k6wp::WallpaperConfig cfg = k6wp::MigrateConfig(v0);
    Check(cfg.fps_cap == 24, "32. missing fps_cap migrates to 24");
  }

  // 33. P1.3: explicit fps_cap 30 is preserved (never lowered).
  {
    const nlohmann::json j = {{"video_path", "C:\\videos\\old.mp4"},
                              {"fps_cap", 30}};
    const k6wp::WallpaperConfig cfg = k6wp::MigrateConfig(j);
    Check(cfg.fps_cap == 30, "33. explicit fps_cap 30 preserved");
    k6wp::WallpaperConfig out;
    out.video_path = L"C:\\videos\\old.mp4";
    out.fps_cap = 30;
    const auto p = dir / "fps30" / "config.json";
    k6wp::SaveConfig(p, out);
    const k6wp::WallpaperConfig loaded = k6wp::LoadConfig(p);
    Check(loaded.fps_cap == 30, "33. explicit fps_cap 30 round-trips");
  }

  // 34. P2.7: v3 file (predates battery_mode) migrates to v5 with
  // battery_mode defaulted to "cap24" + .bak holds the original bytes +
  // rewrite is stable at v5.
  {
    const auto p = dir / "mig_v3_nobatterymode.json";
    auto j = ValidJson();
    j["version"] = 3;
    j.erase("battery_mode");
    WriteJson(p, j);
    const std::string original = ReadText(p);
    k6wp::WallpaperConfig cfg;
    if (ExpectLoadOk(p, "34. v3 migrates to v5", &cfg)) {
      Check(cfg.version == 5, "34. version == 5");
      Check(cfg.battery_mode == "cap24", "34. battery_mode defaulted to cap24");
      Check(cfg.video_path == L"C:\\videos\\test.mp4" && cfg.crf == 23 &&
                cfg.monitor_id == 0 && cfg.battery_saver == false,
            "34. other fields preserved");
    }
    const auto bak = BakPath(p);
    Check(std::filesystem::exists(bak), "34. .bak backup written");
    Check(ReadText(bak) == original, "34. .bak holds original bytes");
    k6wp::WallpaperConfig cfg2;
    if (ExpectLoadOk(p, "34. rewritten file loads clean", &cfg2)) {
      Check(cfg2.version == 5 && cfg2.battery_mode == "cap24",
            "34. rewritten v5 stable");
    }
  }

  // 35. P2.7: existing battery_mode "static" is preserved across a
  // load/save round-trip (not clobbered by the cap24 default).
  {
    const auto p = dir / "static_mode.json";
    auto j = ValidJson();
    j["battery_mode"] = "static";
    j["battery_saver"] = true;
    WriteJson(p, j);
    k6wp::WallpaperConfig cfg;
    if (ExpectLoadOk(p, "35. static mode loads", &cfg)) {
      Check(cfg.version == 5 && cfg.battery_mode == "static",
            "35. static preserved on load");
    }
    const auto q = dir / "static_mode" / "config.json";
    k6wp::SaveConfig(q, cfg);
    k6wp::WallpaperConfig reloaded;
    if (ExpectLoadOk(q, "35. static mode reloads after save", &reloaded)) {
      Check(reloaded.battery_mode == "static" &&
                reloaded.battery_saver == true,
            "35. static preserved across round-trip");
    }
  }

  // 36. P2.7: unknown battery_mode (valid JSON, current version) ->
  // ConfigError + .bak backup of the original bytes, so the caller can
  // fall back to last-valid/safe defaults without losing the bad input.
  {
    const auto p = dir / "bogus_batterymode.json";
    auto j = ValidJson();
    j["battery_mode"] = "turbo";
    WriteJson(p, j);
    const std::string original = ReadText(p);
    ExpectLoadThrows(p, "36. unknown battery_mode rejected");
    const auto bak = BakPath(p);
    Check(std::filesystem::exists(bak),
          "36. .bak backup written for invalid battery_mode");
    Check(ReadText(bak) == original, "36. .bak holds original bytes");
  }

  // 37. P3L.1: v5 round-trip stable (load -> save -> load identical, no
  // spurious .bak on either clean load).
  {
    const auto p = dir / "v5_stable.json";
    WriteJson(p, ValidJson());
    k6wp::WallpaperConfig cfg1;
    if (ExpectLoadOk(p, "37. v5 loads clean", &cfg1)) {
      Check(cfg1.version == 5 && cfg1.battery_mode == "cap24",
            "37. version/mode correct");
      Check(cfg1.cpu_affinity == "auto" && cfg1.gpu_adapter == "auto",
            "37. affinity/adapter defaults correct");
    }
    Check(!std::filesystem::exists(BakPath(p)),
          "37. no .bak on clean v5 load");
    const auto q = dir / "v5_stable" / "config.json";
    k6wp::SaveConfig(q, cfg1);
    k6wp::WallpaperConfig cfg2;
    if (ExpectLoadOk(q, "37. v5 reloads after save", &cfg2)) {
      Check(cfg2.version == cfg1.version &&
                cfg2.video_path == cfg1.video_path &&
                cfg2.fit_mode == cfg1.fit_mode && cfg2.speed == cfg1.speed &&
                cfg2.monitor_id == cfg1.monitor_id && cfg2.crf == cfg1.crf &&
                cfg2.resolution_w == cfg1.resolution_w &&
                cfg2.resolution_h == cfg1.resolution_h &&
                cfg2.fps_cap == cfg1.fps_cap &&
                cfg2.battery_saver == cfg1.battery_saver &&
                cfg2.battery_mode == cfg1.battery_mode &&
                cfg2.cpu_affinity == cfg1.cpu_affinity &&
                cfg2.gpu_adapter == cfg1.gpu_adapter,
            "37. load->save->load identical");
    }
    Check(!std::filesystem::exists(BakPath(q)),
          "37. no spurious .bak on second load");
  }

  // 38. P3L.1: fresh defaults carry cpu_affinity/gpu_adapter "auto".
  {
    const k6wp::WallpaperConfig fresh;
    Check(fresh.cpu_affinity == "auto", "38. default cpu_affinity == auto");
    Check(fresh.gpu_adapter == "auto", "38. default gpu_adapter == auto");
  }

  // 39. P3L.1: explicit non-default affinity/adapter survive a load.
  {
    const auto p = dir / "nondefault_aff.json";
    auto j = ValidJson();
    j["cpu_affinity"] = "all";
    j["gpu_adapter"] = "discrete";
    WriteJson(p, j);
    k6wp::WallpaperConfig cfg;
    if (ExpectLoadOk(p, "39. non-default affinity/adapter load", &cfg)) {
      Check(cfg.cpu_affinity == "all", "39. cpu_affinity preserved");
      Check(cfg.gpu_adapter == "discrete", "39. gpu_adapter preserved");
    }
    Check(!std::filesystem::exists(BakPath(p)),
          "39. no .bak on clean v5 load");
  }

  // 40. P3L.1: unknown cpu_affinity -> ConfigError + .bak.
  {
    const auto p = dir / "bogus_affinity.json";
    auto j = ValidJson();
    j["cpu_affinity"] = "turbo";
    WriteJson(p, j);
    const std::string original = ReadText(p);
    ExpectLoadThrows(p, "40. unknown cpu_affinity rejected");
    const auto bak = BakPath(p);
    Check(std::filesystem::exists(bak),
          "40. .bak backup written for invalid cpu_affinity");
    Check(ReadText(bak) == original, "40. .bak holds original bytes");
  }

  // 41. P3L.1: unknown gpu_adapter -> ConfigError + .bak.
  {
    const auto p = dir / "bogus_adapter.json";
    auto j = ValidJson();
    j["gpu_adapter"] = "quantum";
    WriteJson(p, j);
    const std::string original = ReadText(p);
    ExpectLoadThrows(p, "41. unknown gpu_adapter rejected");
    const auto bak = BakPath(p);
    Check(std::filesystem::exists(bak),
          "41. .bak backup written for invalid gpu_adapter");
    Check(ReadText(bak) == original, "41. .bak holds original bytes");
  }

  // 42. P3L.1: v4 file (predates both keys) migrates to v5 with
  // cpu_affinity/gpu_adapter defaulted to "auto" + .bak + stable rewrite.
  {
    const auto p = dir / "mig_v4_noaff.json";
    auto j = ValidJson();
    j["version"] = 4;
    j.erase("cpu_affinity");
    j.erase("gpu_adapter");
    WriteJson(p, j);
    const std::string original = ReadText(p);
    k6wp::WallpaperConfig cfg;
    if (ExpectLoadOk(p, "42. v4 migrates to v5", &cfg)) {
      Check(cfg.version == 5, "42. version == 5");
      Check(cfg.cpu_affinity == "auto" && cfg.gpu_adapter == "auto",
            "42. affinity/adapter defaulted to auto");
      Check(cfg.video_path == L"C:\\videos\\test.mp4" && cfg.crf == 23 &&
                cfg.monitor_id == 0 && cfg.battery_mode == "cap24",
            "42. other fields preserved");
    }
    const auto bak = BakPath(p);
    Check(std::filesystem::exists(bak), "42. .bak backup written");
    Check(ReadText(bak) == original, "42. .bak holds original bytes");
    k6wp::WallpaperConfig cfg2;
    if (ExpectLoadOk(p, "42. rewritten file loads clean", &cfg2)) {
      Check(cfg2.version == 5 && cfg2.cpu_affinity == "auto" &&
                cfg2.gpu_adapter == "auto",
            "42. rewritten v5 stable");
    }
  }

  // 43. P3L.1 full-chain v0->v5 (PATCH D): minimal v0 migrates through
  // every historical rule — old fields preserved/normalized, both new
  // keys defaulted, save-load round-trip stable.
  {
    const auto p = dir / "mig_v0_fullchain.json";
    const std::string original =
        R"({"video_path": "C:\\videos\\old.mp4", "fit_mode": "fill"})";
    WriteText(p, original);
    k6wp::WallpaperConfig cfg;
    if (ExpectLoadOk(p, "43. v0 full-chain migrates", &cfg)) {
      Check(cfg.version == 5, "43. version == 5");
      Check(cfg.video_path == L"C:\\videos\\old.mp4",
            "43. video_path preserved");
      Check(cfg.fit_mode == "cover",
            "43. v<3 fill normalized to cover through the chain");
      Check(cfg.monitor_id == -1,
            "43. v<2 monitor_id forced to -1 through the chain");
      Check(cfg.cpu_affinity == "auto" && cfg.gpu_adapter == "auto",
            "43. new keys defaulted through the chain");
    }
    Check(std::filesystem::exists(BakPath(p)),
          "43. .bak backup written");
    const auto q = dir / "mig_v0_fullchain" / "config.json";
    k6wp::SaveConfig(q, cfg);
    k6wp::WallpaperConfig reloaded;
    if (ExpectLoadOk(q, "43. round-trips after save", &reloaded)) {
      Check(reloaded.version == 5 && reloaded.fit_mode == "cover" &&
                reloaded.monitor_id == -1 &&
                reloaded.cpu_affinity == "auto" &&
                reloaded.gpu_adapter == "auto",
            "43. save->load stable at v5");
    }
    Check(!std::filesystem::exists(BakPath(q)),
          "43. no spurious .bak on second load");
  }

  // 44. HIGH-4: PersistConfigField is a single-field JSON merge — a unique
  // dummy field survives, other fields are untouched, a no-op repeat skips
  // the write, and a missing file is created from the fallback.
  {
    const auto p = dir / "t14_dummy.json";
    auto j = ValidJson();
    const std::string dummy = "t14-quux-7f3a9c";
    j["__t14_dummy"] = dummy;
    j["video_path"] = "C:\\videos\\before.mp4";
    WriteJson(p, j);
    k6wp::WallpaperConfig before;
    if (ExpectLoadOk(p, "44. seed loads", &before)) {
      const bool wrote = k6wp::PersistConfigField(
          p, "video_path", nlohmann::json("C:\\videos\\after.mp4"), before);
      Check(wrote, "44. changed field writes");
      nlohmann::json after =
          nlohmann::json::parse(ReadText(p));
      Check(after.value("__t14_dummy", "") == dummy,
            "44. dummy field preserved");
      Check(after.value("video_path", "") == "C:\\videos\\after.mp4",
            "44. video_path updated");
      Check(after.value("monitor_id", -999) == 0 &&
                after.value("crf", -999) == 23,
            "44. sibling fields untouched");
      Check(!std::filesystem::exists(BakPath(p)),
            "44. no .bak on clean single-field persist");
      std::filesystem::path tmp = p;
      tmp += L".tmp";
      Check(!std::filesystem::exists(tmp), "44. no stray .tmp after save");
      const bool rewrote = k6wp::PersistConfigField(
          p, "video_path", nlohmann::json("C:\\videos\\after.mp4"), before);
      Check(!rewrote, "44. identical repeat skips the write");
      after = nlohmann::json::parse(ReadText(p));
      Check(after.value("__t14_dummy", "") == dummy,
            "44. dummy still present after no-op");
    }
    const auto q = dir / "t14_missing" / "config.json";
    const bool created = k6wp::PersistConfigField(
        q, "monitor_id", nlohmann::json(2), k6wp::WallpaperConfig{});
    Check(created, "44. missing file created from fallback");
    k6wp::WallpaperConfig created_cfg;
    if (ExpectLoadOk(q, "44. created file loads", &created_cfg)) {
      Check(created_cfg.monitor_id == 2, "44. created field value correct");
    }
  }

  // 45. HIGH-4: an orphan "<config>.tmp" never shadows the main file, and a
  // truncated main file throws (with .bak backup) so the caller falls back
  // to the last-valid .bak per the old contract.
  {
    const auto p = dir / "t14_orphan.json";
    auto j = ValidJson();
    j["__t14_dummy"] = "orphan-guard-42";
    WriteJson(p, j);
    std::filesystem::path tmp = p;
    tmp += L".tmp";
    WriteText(tmp, "{TRUNCATED-GARBAGE-{{{{");
    k6wp::WallpaperConfig cfg;
    if (ExpectLoadOk(p, "45. load ignores orphan .tmp", &cfg)) {
      Check(cfg.monitor_id == 0, "45. main file wins over orphan .tmp");
    }
    k6wp::SaveConfig(p, cfg);
    Check(!std::filesystem::exists(tmp),
          "45. save reclaims the orphan .tmp name");
    k6wp::WallpaperConfig reloaded;
    if (ExpectLoadOk(p, "45. reloads after reclaim", &reloaded)) {
      Check(reloaded.monitor_id == 0, "45. content intact after reclaim");
    }
    const auto t = dir / "t14_trunc.json";
    auto tj = ValidJson();
    tj["__t14_dummy"] = "bak-carry-17";
    WriteJson(t, tj);
    const std::string truncated = "{\"version\": 5, \"video_path\": ";
    WriteText(t, truncated);
    ExpectLoadThrows(t, "45. truncated main throws ConfigError");
    const auto tbak = BakPath(t);
    Check(std::filesystem::exists(tbak),
          "45. .bak backup written for truncated main");
    Check(ReadText(tbak) == truncated,
          "45. .bak holds the failed bytes for forensics");
    // The .bak last-valid fallback per the old contract: a migrated file
    // leaves its pre-migration bytes (dummy included) in .bak; when the
    // main file is then lost entirely, the caller loads .bak instead.
    const auto u = dir / "t14_bakfallback.json";
    auto uj = ValidJson();
    uj["version"] = 4;
    uj.erase("cpu_affinity");
    uj.erase("gpu_adapter");
    uj["__t14_dummy"] = "bak-carry-17";
    WriteJson(u, uj);
    k6wp::WallpaperConfig umig;
    if (ExpectLoadOk(u, "45. v4 seed migrates", &umig)) {
      Check(umig.version == 5, "45. migrated to v5");
    }
    const auto ubak = BakPath(u);
    Check(std::filesystem::exists(ubak),
          "45. migration left a .bak with the original bytes");
    std::error_code rm_ec;
    std::filesystem::remove(u, rm_ec);
    ExpectLoadThrows(u, "45. missing main throws ConfigError");
    Check(std::filesystem::exists(ubak),
          "45. missing main leaves .bak untouched");
    // NOTE: read the raw .bak bytes BEFORE loading it — LoadConfig
    // self-heals a v4 file in place (struct rewrite drops unknown keys).
    const nlohmann::json ubak_raw =
        nlohmann::json::parse(ReadText(ubak));
    Check(ubak_raw.value("__t14_dummy", "") == "bak-carry-17",
          "45. dummy field carried in .bak");
    k6wp::WallpaperConfig from_bak;
    if (ExpectLoadOk(ubak, "45. .bak loads as last-valid fallback",
                     &from_bak)) {
      Check(from_bak.monitor_id == 0, "45. .bak field values intact");
    }
  }

  // 46. MED-18: files larger than kMaxConfigBytes (1 MiB) are rejected
  // BEFORE reading — LoadConfig/LoadStudioSettings throw ConfigError whose
  // message contains "exceeds maximum size". A normal ~1 KiB file loads fine.
  {
    const auto big = dir / "t29_big_config.json";
    WriteText(big, std::string(2 * 1024 * 1024, 'x'));
    Check(std::filesystem::file_size(big) > k6wp::kMaxConfigBytes,
          "46. 2 MiB fixture exceeds kMaxConfigBytes");
    try {
      (void)k6wp::LoadConfig(big);
      Check(false, "46. oversized config throws ConfigError (got success)");
    } catch (const k6wp::ConfigError& e) {
      Check(std::string(e.what()).find("exceeds maximum size") !=
                std::string::npos,
            "46. oversized config throws ConfigError (exceeds maximum size)");
    } catch (...) {
      Check(false, "46. oversized config throws ConfigError (wrong type)");
    }
    const auto sbig = dir / "t29_big_settings.json";
    WriteText(sbig, std::string(2 * 1024 * 1024, 'x'));
    try {
      (void)k6wp::LoadStudioSettings(sbig);
      Check(false, "46. oversized settings throw ConfigError (got success)");
    } catch (const k6wp::ConfigError& e) {
      Check(std::string(e.what()).find("exceeds maximum size") !=
                std::string::npos,
            "46. oversized settings throw ConfigError (exceeds maximum size)");
    } catch (...) {
      Check(false, "46. oversized settings throw ConfigError (wrong type)");
    }
    const auto small = dir / "t29_small.json";
    WriteJson(small, ValidJson());
    Check(std::filesystem::file_size(small) < k6wp::kMaxConfigBytes,
          "46. 1 KiB-class fixture under kMaxConfigBytes");
    k6wp::WallpaperConfig scfg;
    if (ExpectLoadOk(small, "46. normal-size config loads with no error",
                     &scfg)) {
      Check(scfg.crf == 23, "46. normal-size config fields intact");
    }
  }

  // 47. Schema-doc sync guard (see CheckSchemaDocInSync above).
  CheckSchemaDocInSync(argc > 1 ? argv[1] : "");

  std::cout << "\n" << g_checks << " checks, " << g_failures << " failures\n";
  std::cout << "DefaultConfigPath: " << k6wp::DefaultConfigPath().string() << "\n";
  return g_failures == 0 ? 0 : 1;
}