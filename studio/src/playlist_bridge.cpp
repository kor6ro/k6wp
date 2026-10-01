#include "playlist_bridge.hpp"

#include <QFileDialog>
#include <QVariantMap>

#include <algorithm>
#include <system_error>

#include "config_schema.hpp"
#include "video_paths.hpp"

namespace k6wp {
namespace {

std::filesystem::path ResolvePath() {
  const QString env = qEnvironmentVariable("K6WP_PLAYLIST_JSON");
  if (!env.isEmpty()) {
    return std::filesystem::path(env.toStdWString());
  }
  return DefaultPlaylistPath();
}

QString LabelFor(const std::wstring& path, bool exists) {
  QString name =
      QString::fromStdWString(std::filesystem::path(path).filename().wstring());
  if (name.isEmpty()) {
    name = QString::fromStdWString(path);
  }
  return exists ? name : QStringLiteral("%1 (hilang)").arg(name);
}

}  // namespace

PlaylistBridge::PlaylistBridge(QObject* parent) : QObject(parent) {
  try {
    path_ = ResolvePath();
  } catch (const ConfigError& e) {
    SetLastError(QString::fromUtf8(e.what()));
    return;
  }
  Load();
}

PlaylistBridge::~PlaylistBridge() = default;

void PlaylistBridge::Load() {
  try {
    cfg_ = LoadPlaylist(path_);
    AppendLog(QStringLiteral("playlist: %1 entri dimuat").arg(cfg_.order.size()));
  } catch (const ConfigError& e) {
    std::error_code ec;
    if (std::filesystem::exists(path_, ec)) {
      SetLastError(QString::fromUtf8(e.what()));
    }
    // Missing file is the normal first-run case: start disabled + empty.
    cfg_ = PlaylistConfig{};
  }
  emit changed();
}

bool PlaylistBridge::Save() {
  try {
    SavePlaylist(path_, cfg_);
    return true;
  } catch (const ConfigError& e) {
    SetLastError(QString::fromUtf8(e.what()));
    return false;
  }
}

QVariantList PlaylistBridge::items() const {
  QVariantList out;
  out.reserve(static_cast<qsizetype>(cfg_.order.size()));
  for (const std::wstring& p : cfg_.order) {
    std::error_code ec;
    const bool exists = std::filesystem::is_regular_file(p, ec);
    QVariantMap m;
    m.insert(QStringLiteral("path"), QString::fromStdWString(p));
    m.insert(QStringLiteral("label"), LabelFor(p, exists));
    m.insert(QStringLiteral("exists"), exists);
    out.push_back(m);
  }
  return out;
}

void PlaylistBridge::reload() { Load(); }

void PlaylistBridge::setEnabled(bool on) {
  if (cfg_.enabled == on) {
    return;
  }
  cfg_.enabled = on;
  if (Save()) {
    emit changed();
  }
}

void PlaylistBridge::setIntervalMin(int minutes) {
  const int clamped = std::clamp(minutes, kPlaylistMinIntervalMin,
                                 kPlaylistMaxIntervalMin);
  if (cfg_.interval_min == clamped) {
    return;
  }
  cfg_.interval_min = clamped;
  if (Save()) {
    emit changed();
  }
}

void PlaylistBridge::setShuffle(bool on) {
  if (cfg_.shuffle == on) {
    return;
  }
  cfg_.shuffle = on;
  if (Save()) {
    emit changed();
  }
}

int PlaylistBridge::pickAndAdd() {
  const QStringList picked = QFileDialog::getOpenFileNames(
      nullptr, QStringLiteral("Tambah ke Daftar Putar"), QString(),
      QStringLiteral("Video (*.mp4 *.webm *.avi *.mkv *.mov *.wmv);"
                     "Semua File (*)"));
  if (picked.isEmpty()) {
    return 0;
  }
  return addPaths(picked);
}

int PlaylistBridge::addPaths(const QStringList& paths) {
  const std::size_t before = cfg_.order.size();
  std::vector<std::wstring> order = cfg_.order;
  for (const QString& p : paths) {
    if (!IsVideoPath(p)) {
      continue;
    }
    std::error_code ec;
    const std::filesystem::path input(p.toStdWString());
    if (!std::filesystem::is_regular_file(input, ec)) {
      continue;
    }
    const std::filesystem::path abs = std::filesystem::absolute(input, ec);
    order.push_back((ec ? input : abs).wstring());
  }
  cfg_.order = NormalizeOrder(std::move(order));
  const int added = static_cast<int>(cfg_.order.size() - before);
  if (added > 0) {
    if (Save()) {
      AppendLog(QStringLiteral("playlist: %1 entri ditambahkan").arg(added));
      emit changed();
    }
  }
  return added;
}

void PlaylistBridge::removeAt(int row) {
  if (row < 0 || row >= count()) {
    return;
  }
  cfg_.order.erase(cfg_.order.begin() + row);
  if (Save()) {
    emit changed();
  }
}

void PlaylistBridge::moveUp(int row) {
  if (row <= 0 || row >= count()) {
    return;
  }
  std::swap(cfg_.order[row], cfg_.order[row - 1]);
  if (Save()) {
    emit changed();
  }
}

void PlaylistBridge::moveDown(int row) {
  if (row < 0 || row >= count() - 1) {
    return;
  }
  std::swap(cfg_.order[row], cfg_.order[row + 1]);
  if (Save()) {
    emit changed();
  }
}

void PlaylistBridge::clear() {
  if (cfg_.order.empty()) {
    return;
  }
  cfg_.order.clear();
  if (Save()) {
    emit changed();
  }
}

QString PlaylistBridge::pathAt(int row) const {
  if (row < 0 || row >= count()) {
    return QString();
  }
  return QString::fromStdWString(cfg_.order[row]);
}

void PlaylistBridge::AppendLog(const QString& line) {
  log_.append(line);
  if (log_.size() > 200) {
    log_.removeFirst();
  }
  emit logChanged();
}

void PlaylistBridge::SetLastError(const QString& error) {
  last_error_ = error;
  emit lastErrorChanged();
}

}  // namespace k6wp
