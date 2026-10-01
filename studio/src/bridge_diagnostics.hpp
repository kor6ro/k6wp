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

// Assigns `error` to `last_error`; returns true when it changed, so the caller
// emits its own lastErrorChanged and appends a non-empty error to the log.
inline bool SetChangedError(QString& last_error, const QString& error) {
  if (last_error == error) {
    return false;
  }
  last_error = error;
  return true;
}

}  // namespace k6wp
