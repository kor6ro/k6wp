#pragma once

#include <QString>
#include <QStringList>

namespace k6wp {

// Appends `line` to `log`, trimming the oldest lines so at most `cap` remain.
// Shared by the QML bridges' AppendLog; each bridge still emits its own
// logChanged signal.
inline void AppendCapped(QStringList& log, const QString& line, int cap) {
  log.append(line);
  while (log.size() > cap) {
    log.removeFirst();
  }
}

}  // namespace k6wp
