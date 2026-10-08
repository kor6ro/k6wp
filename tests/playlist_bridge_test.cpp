// PlaylistBridge round-trip tests (playlist.json via the QML-facing bridge).
// QCoreApplication (no widgets shown); QFileDialog is never invoked because the
// test drives addPaths() directly. Exit code 0 = all pass.
#include <QByteArray>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QFile>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "library_manager.hpp"
#include "playlist_bridge.hpp"

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
  auto dir = std::filesystem::temp_directory_path() / "k6wp_playlist_bridge_test";
  std::error_code ec;
  std::filesystem::remove_all(dir, ec);
  std::filesystem::create_directories(dir, ec);
  return dir;
}

void MakeFile(const std::filesystem::path& p) {
  std::ofstream out(p, std::ios::binary | std::ios::trunc);
  out << "x";
}

QString Q(const std::filesystem::path& p) {
  return QString::fromStdWString(p.wstring());
}

std::string FileHash(const std::filesystem::path& p) {
  QFile f(QString::fromStdWString(p.wstring()));
  if (!f.open(QIODevice::ReadOnly)) {
    return {};
  }
  return QCryptographicHash::hash(f.readAll(), QCryptographicHash::Sha256)
      .toHex()
      .toStdString();
}

void SeedLibrary(const std::filesystem::path& lib_json,
                 const std::vector<std::filesystem::path>& dsts) {
  k6wp::LibraryManager mgr(lib_json);
  mgr.Load();
  for (const auto& d : dsts) {
    k6wp::LibraryEntry e;
    e.src = e.dst = d;
    e.width = 1920;
    e.height = 1080;
    e.codec = "h264";
    e.res = "1920x1080";
    e.fps = 30;
    mgr.Add(e, k6wp::kNever);
  }
}

}  // namespace

int main(int argc, char** argv) {
  QCoreApplication app(argc, argv);
  const std::filesystem::path dir = TempDir();
  const std::filesystem::path pl = dir / "playlist.json";
  const std::filesystem::path a = dir / "a.mp4";
  const std::filesystem::path b = dir / "b.webm";
  const std::filesystem::path txt = dir / "c.txt";
  MakeFile(a);
  MakeFile(b);
  MakeFile(txt);
  qputenv("K6WP_PLAYLIST_JSON", QByteArray::fromStdString(pl.string()));

  {
    k6wp::PlaylistBridge bridge;
    Check(bridge.count() == 0, "1. empty playlist on first run");
    Check(!bridge.enabled(), "1. disabled by default");
    Check(bridge.intervalMin() == 30, "1. default interval 30");
    Check(!bridge.shuffle(), "1. shuffle off by default");
    Check(bridge.path().endsWith(QStringLiteral("playlist.json")),
          "1. path points at playlist.json");

    const int added =
        bridge.addPaths({Q(a), Q(b), Q(txt)});
    Check(added == 2, "2. addPaths accepts 2 videos and skips .txt");
    Check(bridge.count() == 2, "2. count is 2");
    Check(bridge.addPaths({Q(a)}) == 0, "2. duplicate path is skipped");
    Check(bridge.items().size() == 2, "2. items size is 2");

    bridge.setEnabled(true);
    bridge.setIntervalMin(5);
    bridge.setShuffle(true);
    Check(bridge.enabled() && bridge.intervalMin() == 5 && bridge.shuffle(),
          "3. scalar setters applied");

    bridge.setIntervalMin(0);
    Check(bridge.intervalMin() == 1, "3. interval clamped to 1");
    bridge.setIntervalMin(99999);
    Check(bridge.intervalMin() == 1440, "3. interval clamped to 1440");
    bridge.setIntervalMin(5);

    const QString first = bridge.pathAt(0);
    bridge.moveDown(0);
    Check(bridge.pathAt(1) == first, "4. moveDown swaps entries");
    bridge.moveUp(1);
    Check(bridge.pathAt(0) == first, "4. moveUp restores entries");
    Check(bridge.pathAt(-1).isEmpty(), "4. pathAt out of range is empty");

    const QVariantList items = bridge.items();
    const QVariantMap first_item = items.at(0).toMap();
    Check(first_item.value(QStringLiteral("exists")).toBool(),
          "5. existing entry marks exists=true");
    Check(first_item.value(QStringLiteral("path")).toString() == first,
          "5. item path matches pathAt");
  }

  {
    k6wp::PlaylistBridge bridge;
    Check(bridge.count() == 2, "6. reload sees 2 entries");
    Check(bridge.enabled(), "6. reload sees enabled");
    Check(bridge.intervalMin() == 5, "6. reload sees interval");
    Check(bridge.shuffle(), "6. reload sees shuffle");
    bridge.removeAt(0);
    Check(bridge.count() == 1, "6. removeAt drops one");
    bridge.removeAt(99);
    Check(bridge.count() == 1, "6. removeAt out of range is a no-op");
    bridge.clear();
    Check(bridge.count() == 0, "6. clear empties the playlist");
  }

  {
    k6wp::PlaylistBridge bridge;
    Check(bridge.count() == 0, "7. cleared playlist persists");
    Check(bridge.enabled(), "7. enabled flag survives clear");
  }

  // 8. syncOrderFromLibrary materialization (plan todo 9): library dst
  //    paths (existing only) become cfg_.order via the atomic Save();
  //    missing files (exists == false) are skipped.
  {
    const auto lib_json = dir / "library.json";
    const auto gone = dir / "gone.mp4";  // never written to disk
    SeedLibrary(lib_json, {a, b, gone});
    qputenv("K6WP_LIBRARY_JSON", QByteArray::fromStdString(lib_json.string()));

    k6wp::PlaylistBridge bridge;
    Check(bridge.syncOrderFromLibrary(),
          "8. syncOrderFromLibrary returns true for a small library");
    Check(bridge.count() == 2,
          "8. order materialized from 2 existing library files (missing skipped)");
    // ListItems sorts by dst filename: a.mp4 < b.webm
    Check(bridge.pathAt(0).endsWith(QStringLiteral("a.mp4")),
          "8. order follows ListItems filename sort (a first)");
    Check(bridge.pathAt(1).endsWith(QStringLiteral("b.webm")),
          "8. second materialized entry present");
    Check(bridge.lastError().isEmpty(), "8. successful sync leaves lastError empty");

    k6wp::PlaylistBridge fresh;
    Check(fresh.count() == 2,
          "8. materialized order persists across bridge instances");
  }

  // 9. Idempotence: two consecutive syncs over an unchanged library leave
  //    playlist.json byte-identical (hash proof).
  {
    k6wp::PlaylistBridge bridge;
    Check(bridge.syncOrderFromLibrary(), "9. first sync succeeds");
    const std::string h1 = FileHash(pl);
    Check(!h1.empty(), "9. first-sync playlist.json hash captured");
    Check(bridge.syncOrderFromLibrary(), "9. second sync succeeds");
    const std::string h2 = FileHash(pl);
    Check(!h2.empty() && h1 == h2,
          "9. two consecutive syncs -> byte-identical file (hash proof)");
    std::cout << "[INFO] idempotence hash1=" << h1 << " hash2=" << h2 << "\n";
  }

  // 10. >500 guard: 501 valid library files -> false + message, and
  //     playlist.json stays byte-identical (hash proof).
  {
    k6wp::PlaylistBridge bridge;
    bridge.addPaths({Q(a)});  // ensure a known on-disk playlist.json exists
    const std::string before = FileHash(pl);
    Check(!before.empty(), "10. baseline playlist.json hash captured");

    std::vector<std::filesystem::path> many;
    many.reserve(501);
    for (int i = 0; i < 501; ++i) {
      char name[32];
      std::snprintf(name, sizeof(name), "many%03d.mp4", i);
      const auto p = dir / name;
      MakeFile(p);
      many.push_back(p);
    }
    const auto lib_many = dir / "library_many.json";
    SeedLibrary(lib_many, many);
    qputenv("K6WP_LIBRARY_JSON", QByteArray::fromStdString(lib_many.string()));

    k6wp::PlaylistBridge over;
    Check(!over.syncOrderFromLibrary(),
          "10. 501-entry library rejected by the >500 guard");
    Check(!over.lastError().isEmpty(), "10. rejection carries a message");
    Check(over.lastError().contains(QStringLiteral("500")) ||
              over.lastError().contains(QStringLiteral("501")),
          "10. rejection message names the 500-entry cap");
    const std::string after = FileHash(pl);
    Check(before == after,
          "10. playlist.json byte-identical after reject (hash proof)");
    std::cout << "[INFO] 501-reject hash before=" << before
              << " after=" << after << "\n";

    // A second attempt stays rejected and still does not touch the file.
    Check(!over.syncOrderFromLibrary(),
          "10. second reject attempt also returns false");
    Check(FileHash(pl) == after,
          "10. playlist.json still byte-identical after second reject");
  }

  std::cout << "\n" << g_checks << " checks, " << g_failures << " failures\n";
  return g_failures == 0 ? 0 : 1;
}
