// Mini unit tests for shared/playlist (wallpaper playlist + rotation).
// No external test framework: plain asserts with a pass/fail counter.
// Exit code 0 = all pass.
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

#include "config_schema.hpp"
#include "playlist.hpp"
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
  auto dir = std::filesystem::temp_directory_path() / "k6wp_playlist_test";
  std::error_code ec;
  std::filesystem::remove_all(dir, ec);  // fresh per run
  std::filesystem::create_directories(dir, ec);
  return dir;
}

std::string ReadText(const std::filesystem::path& p) {
  std::ifstream in(p, std::ios::binary);
  return std::string(std::istreambuf_iterator<char>(in),
                     std::istreambuf_iterator<char>());
}

void WriteText(const std::filesystem::path& p, const std::string& text) {
  std::ofstream out(p, std::ios::binary | std::ios::trunc);
  out << text;
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

}  // namespace

int main() {
  const std::filesystem::path dir = TempDir();

  // 1. Defaults.
  {
    k6wp::PlaylistConfig cfg;
    Check(cfg.version == k6wp::kPlaylistSchemaVersion, "1. default version");
    Check(cfg.enabled == false, "1. default enabled false");
    Check(cfg.interval_min == 30, "1. default interval 30");
    Check(cfg.shuffle == false, "1. default shuffle false");
    Check(cfg.order.empty(), "1. default order empty");
  }

  // 2. PlaylistToJson field set + version stamp.
  {
    k6wp::PlaylistConfig cfg;
    cfg.enabled = true;
    cfg.interval_min = 15;
    cfg.shuffle = true;
    cfg.order = {L"C:\\v\\a.mp4", L"C:\\v\\b.webm"};
    const nlohmann::json j = k6wp::PlaylistToJson(cfg);
    Check(j.at("version") == k6wp::kPlaylistSchemaVersion, "2. json version");
    Check(j.at("enabled") == true, "2. json enabled");
    Check(j.at("interval_min") == 15, "2. json interval");
    Check(j.at("shuffle") == true, "2. json shuffle");
    Check(j.at("order").is_array() && j.at("order").size() == 2,
          "2. json order array size");
  }

  // 3. Save/Load round-trip.
  {
    const auto path = dir / "t3_playlist.json";
    k6wp::PlaylistConfig cfg;
    cfg.enabled = true;
    cfg.interval_min = 7;
    cfg.shuffle = true;
    cfg.order = {L"C:\\Videos\\one.mp4", L"C:\\Videos\\two.webm"};
    k6wp::SavePlaylist(path, cfg);
    const k6wp::PlaylistConfig back = k6wp::LoadPlaylist(path);
    Check(back.enabled && back.interval_min == 7 && back.shuffle,
          "3. round-trip scalar fields");
    Check(back.order.size() == 2, "3. round-trip order size");
    Check(back.order[0] == L"C:\\Videos\\one.mp4", "3. round-trip order[0]");
    Check(back.order[1] == L"C:\\Videos\\two.webm", "3. round-trip order[1]");
  }

  // 4. NormalizeOrder: dedup case-insensitive, drop empties, keep order.
  {
    std::vector<std::wstring> in = {L"C:\\V\\A.mp4", L"", L"c:\\v\\a.MP4",
                                    L"C:\\V\\b.mp4"};
    const auto out = k6wp::NormalizeOrder(in);
    Check(out.size() == 2, "4. normalize drops empty + dup (size)");
    Check(out[0] == L"C:\\V\\A.mp4", "4. normalize keeps first occurrence");
    Check(out[1] == L"C:\\V\\b.mp4", "4. normalize keeps later unique");
  }

  // 5. PlaylistIndexForPath (case-insensitive).
  {
    const std::vector<std::wstring> order = {L"C:\\V\\a.mp4", L"C:\\V\\b.webm"};
    Check(k6wp::PlaylistIndexForPath(order, L"C:\\V\\a.mp4") == 0,
          "5. index found");
    Check(k6wp::PlaylistIndexForPath(order, L"c:\\v\\B.WEBM") == 1,
          "5. index case-insensitive");
    Check(k6wp::PlaylistIndexForPath(order, L"C:\\V\\nope.mp4") == -1,
          "5. index missing -> -1");
  }

  // 6. SelectNextIndex.
  {
    std::uint64_t rng = 12345;
    Check(k6wp::SelectNextIndex(0, 3, false, rng) == 1, "6. sequential +1");
    Check(k6wp::SelectNextIndex(2, 3, false, rng) == 0, "6. sequential wrap");
    Check(k6wp::SelectNextIndex(0, 0, false, rng) == 0, "6. empty -> 0");
    Check(k6wp::SelectNextIndex(9, 3, false, rng) == 0,
          "6. out-of-range current wraps to start");
    Check(k6wp::SelectNextIndex(0, 1, false, rng) == 0, "6. single -> 0");
    Check(k6wp::SelectNextIndex(0, 1, true, rng) == 0, "6. shuffle single -> 0");

    // Shuffle never returns current and is reproducible from a fixed seed.
    std::uint64_t seed_a = 0xDEADBEEF;
    std::uint64_t seed_b = 0xDEADBEEF;
    bool excludes_current = true;
    bool moves = false;
    for (int i = 0; i < 200; ++i) {
      const std::size_t n = k6wp::SelectNextIndex(1, 4, true, seed_a);
      if (n == 1) excludes_current = false;
      if (n != 1) moves = true;
    }
    Check(excludes_current, "6. shuffle excludes current");
    Check(moves, "6. shuffle moves");
    const std::size_t r1 = k6wp::SelectNextIndex(1, 4, true, seed_b);
    std::uint64_t seed_c = 0xDEADBEEF;
    const std::size_t r2 = k6wp::SelectNextIndex(1, 4, true, seed_c);
    Check(r1 == r2, "6. shuffle deterministic from seed");
  }

  // 7. ValidatePlaylist bounds.
  {
    k6wp::PlaylistConfig low;
    low.interval_min = 0;
    Check(ThrowsConfigError([&] { k6wp::ValidatePlaylist(low); }),
          "7. interval 0 rejected");
    k6wp::PlaylistConfig high;
    high.interval_min = 1441;
    Check(ThrowsConfigError([&] { k6wp::ValidatePlaylist(high); }),
          "7. interval 1441 rejected");
    k6wp::PlaylistConfig ok;
    ok.interval_min = 1440;
    Check(!ThrowsConfigError([&] { k6wp::ValidatePlaylist(ok); }),
          "7. interval 1440 accepted");
  }

  // 8. Migrate: missing fields default; wrong types throw; bad version throws.
  {
    const auto cfg =
        k6wp::MigratePlaylist(nlohmann::json::parse("{\"version\":1}"));
    Check(cfg.enabled == false && cfg.interval_min == 30 && !cfg.shuffle,
          "8. migrate fills defaults");
    Check(ThrowsConfigError([&] {
            k6wp::MigratePlaylist(
                nlohmann::json::parse("{\"version\":1,\"order\":\"x\"}"));
          }),
          "8. order non-array rejected");
    Check(ThrowsConfigError([&] {
            k6wp::MigratePlaylist(nlohmann::json::parse("{\"version\":99}"));
          }),
          "8. future version rejected");
    Check(ThrowsConfigError([&] {
            k6wp::MigratePlaylist(nlohmann::json::parse("[]"));
          }),
          "8. non-object root rejected");
  }

  // 9. Corrupt file throws + writes .bak.
  {
    const auto path = dir / "t9_corrupt.json";
    WriteText(path, "{ not json");
    Check(ThrowsConfigError([&] { (void)k6wp::LoadPlaylist(path); }),
          "9. corrupt throws ConfigError");
    Check(std::filesystem::exists(BakPath(path)), "9. corrupt writes .bak");
    Check(ReadText(BakPath(path)) == "{ not json", "9. .bak keeps raw bytes");
  }

  // 10. Oversize file throws (before parse).
  {
    const auto big = dir / "t10_big.json";
    WriteText(big, std::string(2 * 1024 * 1024, 'x'));
    try {
      (void)k6wp::LoadPlaylist(big);
      Check(false, "10. oversize throws (got success)");
    } catch (const k6wp::ConfigError& e) {
      Check(std::string(e.what()).find("exceeds maximum size") !=
                std::string::npos,
            "10. oversize throws (exceeds maximum size)");
    } catch (...) {
      Check(false, "10. oversize throws (wrong type)");
    }
  }

  // 11. Missing file throws ConfigError (caller treats as "no playlist").
  {
    const auto missing = dir / "t11_missing.json";
    Check(ThrowsConfigError([&] { (void)k6wp::LoadPlaylist(missing); }),
          "11. missing file throws ConfigError");
  }

  // 12. PlaylistPathForConfig is a sibling playlist.json.
  {
    const auto cfg_path = std::filesystem::path(L"C:\\things\\config.json");
    const auto pl = k6wp::PlaylistPathForConfig(cfg_path);
    Check(pl.filename() == L"playlist.json", "12. sibling filename");
    Check(pl.parent_path() == cfg_path.parent_path(), "12. sibling parent");
  }

  std::cout << "\n" << g_checks << " checks, " << g_failures << " failures\n";
  return g_failures == 0 ? 0 : 1;
}
