#pragma once

// Single source of truth for "is this a video Studio accepts". The import
// picker, drag-and-drop and the library filter must agree; keeping copies in
// two .cpp files let them drift (a format added in one place, missed in the
// other).

#include <QString>

namespace k6wp {

inline bool IsVideoPath(const QString& path) {
  const QString lower = path.toLower();
  return lower.endsWith(QStringLiteral(".mp4")) ||
         lower.endsWith(QStringLiteral(".webm")) ||
         lower.endsWith(QStringLiteral(".avi")) ||
         lower.endsWith(QStringLiteral(".mkv")) ||
         lower.endsWith(QStringLiteral(".mov")) ||
         lower.endsWith(QStringLiteral(".wmv"));
}

}  // namespace k6wp
