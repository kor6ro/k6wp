// Compress-first offer: the "this file is big, compress it to 1080p before it
// becomes your wallpaper?" prompt.
//
// This existed as MainWindow::MaybeOfferCompressFirst before the Widgets -> QML
// migration and was deleted with the legacy widget tree in f03c850 without a
// QML replacement, so importing or applying a large video silently skipped the
// suggestion. The threshold and the wording are restored verbatim from that
// function; the dialog itself now lives in Main.qml.
#pragma once

#include <QFileInfo>
#include <QString>

namespace k6wp {

// Original cutoff: "if (!info.exists() || info.size() <= 20 MB) return false".
inline constexpr qint64 kCompressFirstThresholdBytes = 20LL * 1024 * 1024;

// Size in whole MB when `path` is an existing file over the threshold, else -1.
// -1 is the "do not offer" answer, so callers can test the sign directly.
inline qint64 CompressFirstOfferMb(const QString& path) {
  const QFileInfo info(path);
  if (!info.exists() || !info.isFile() ||
      info.size() <= kCompressFirstThresholdBytes) {
    return -1;
  }
  return info.size() / (1024 * 1024);
}

}  // namespace k6wp
