#pragma once

#include <QObject>
#include <QPointer>
#include <QString>

#include <atomic>
#include <string>

namespace k6wp {

// Resolves the releases API URL for the optional update check:
// $K6WP_UPDATE_URL wins when non-empty (local test servers in QA),
// otherwise K6WP_UPDATE_CHECK_URL (shared/links.hpp, a real URL).
// Returns empty when the check is disabled — callers must not
// issue any request then, so offline/disabled stays fully silent.
QString ResolveUpdateCheckUrl();
bool IsUpdateCheckUrlUsable(const QString& url);

// Optional "new version available" check (Studio-only). Never downloads or
// installs anything: one WinHTTP GET of the releases API on a worker
// thread, compares tag_name against the running version, and reports.
// All network/parse failures are silent (no signal except
// CheckFinished(false)). The UpdateAvailable/CheckFinished signals always
// fire on the Qt event-loop thread (queued from the worker).
class UpdateChecker final : public QObject {
  Q_OBJECT

 public:
  explicit UpdateChecker(const QString& current_version,
                         QObject* parent = nullptr);
  void SetReleasesPageUrl(const QString& url);
  void Check(const QString& api_url);

 signals:
  void UpdateAvailable(const QString& latest_version, const QString& page_url);
  void CheckFinished(bool update_available);

 private:
  // Blocking WinHTTP GET + parse on the worker thread; posts ReportResult
  // back to the event-loop thread. Copies (not members) cross the thread.
  void FetchAndReport(std::string api_url_utf8, QString releases_page,
                      QString current_version, QPointer<UpdateChecker> guard);
  // Runs on the event-loop thread; emits the signals.
  void ReportResult(bool newer, const QString& latest, const QString& page);

  QString current_version_;
  QString releases_page_url_;
  std::atomic<bool> busy_{false};
};

}  // namespace k6wp
