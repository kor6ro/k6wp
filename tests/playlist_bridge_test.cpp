// PlaylistBridge round-trip tests (playlist.json via the QML-facing bridge).
// QCoreApplication (no widgets shown); QFileDialog is never invoked because the
// test drives addPaths() directly. Exit code 0 = all pass.
#include <QByteArray>
#include <QCoreApplication>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

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

  std::cout << "\n" << g_checks << " checks, " << g_failures << " failures\n";
  return g_failures == 0 ? 0 : 1;
}
