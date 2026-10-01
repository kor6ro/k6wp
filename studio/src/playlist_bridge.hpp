#pragma once

// Phase 6 (playlist): the Wallpaper tab's playlist backend.
//
// Owns playlist.json (shared/playlist.hpp) and exposes it to QML as the
// `Playlist` singleton. The engine READS this file (and rotates); Studio only
// writes it here. Every mutation persists immediately, so a Studio crash or a
// forced close cannot lose a playlist edit.
//
// The file is intentionally NOT config.json: older Studio builds rewrite
// config.json from their own WallpaperConfig struct and would erase a playlist
// stored there. See shared/playlist.hpp.

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

#include <filesystem>

#include "playlist.hpp"

namespace k6wp {

// NOT `final`: QML registration instantiates a QQmlElement<T> subclass.
class PlaylistBridge : public QObject {
  Q_OBJECT
  QML_NAMED_ELEMENT(Playlist)
  QML_ELEMENT
  QML_SINGLETON

 public:
  explicit PlaylistBridge(QObject* parent = nullptr);
  ~PlaylistBridge() override;

  Q_PROPERTY(bool enabled READ enabled NOTIFY changed)
  Q_PROPERTY(int intervalMin READ intervalMin NOTIFY changed)
  Q_PROPERTY(bool shuffle READ shuffle NOTIFY changed)
  Q_PROPERTY(int count READ count NOTIFY changed)
  // [{ "path": "...", "label": "name.mp4", "exists": true }, ...]
  Q_PROPERTY(QVariantList items READ items NOTIFY changed)
  Q_PROPERTY(QString path READ path CONSTANT)
  Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
  Q_PROPERTY(QStringList log READ log NOTIFY logChanged)

  bool enabled() const { return cfg_.enabled; }
  int intervalMin() const { return cfg_.interval_min; }
  bool shuffle() const { return cfg_.shuffle; }
  int count() const { return static_cast<int>(cfg_.order.size()); }
  QVariantList items() const;
  QString path() const { return QString::fromStdWString(path_.wstring()); }
  QString lastError() const { return last_error_; }
  QStringList log() const { return log_; }

  // Re-reads playlist.json, discarding unsaved edits.
  Q_INVOKABLE void reload();
  Q_INVOKABLE void setEnabled(bool on);
  Q_INVOKABLE void setIntervalMin(int minutes);
  Q_INVOKABLE void setShuffle(bool on);
  // Opens the native multi-select picker; appends existing video files.
  Q_INVOKABLE int pickAndAdd();
  // Appends the given paths (videos, case-insensitive extension); returns how
  // many new entries landed. Non-video / missing / duplicate paths are skipped.
  Q_INVOKABLE int addPaths(const QStringList& paths);
  Q_INVOKABLE void removeAt(int row);
  Q_INVOKABLE void moveUp(int row);
  Q_INVOKABLE void moveDown(int row);
  Q_INVOKABLE void clear();
  Q_INVOKABLE QString pathAt(int row) const;

 signals:
  void changed();
  void lastErrorChanged();
  void logChanged();

 private:
  void Load();
  bool Save();
  void AppendLog(const QString& line);
  void SetLastError(const QString& error);

  PlaylistConfig cfg_;
  std::filesystem::path path_;
  QString last_error_;
  QStringList log_;
};

}  // namespace k6wp
