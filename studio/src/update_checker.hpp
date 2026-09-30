#pragma once

#include <QObject>
#include <QPointer>
#include <QString>

#include <atomic>
#include <string>

namespace k6wp {

// The old signal was a bare bool, so "up to date" and "the network is down"
// both arrived as false and an explicit Help -> "Check for updates" could not
// tell the user anything.
enum class UpdateCheckOutcome {
  kDisabled,         // the check is off in Settings; no request was issued
  kUpToDate,         // the server answered and nothing newer exists
  kUpdateAvailable,  // the server answered with a newer tag
  kFailed,           // the request or the parse failed
};

Q_DECLARE_METATYPE(k6wp::UpdateCheckOutcome)

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
// The automatic start-up check stays silent: Check() emits nothing at all
// when the check is disabled or already running. CheckInteractive() instead
// always reports one outcome, so the user-initiated menu action can say what
// happened. UpdateAvailable/CheckFinished always fire on the Qt event-loop
// thread (queued from the worker).
class UpdateChecker final : public QObject {
  Q_OBJECT

 public:
  explicit UpdateChecker(const QString& current_version,
                         QObject* parent = nullptr);
  void SetReleasesPageUrl(const QString& url);
  void Check(const QString& api_url);
  void CheckInteractive(const QString& api_url);

 signals:
  void UpdateAvailable(const QString& latest_version, const QString& page_url);
  void CheckFinished(k6wp::UpdateCheckOutcome outcome);

 private:
  // Blocking WinHTTP GET + parse on the worker thread; posts ReportResult
  // back to the event-loop thread. Copies (not members) cross the thread.
  void FetchAndReport(std::string api_url_utf8, QString releases_page,
                      QString current_version, QPointer<UpdateChecker> guard);
  // Runs on the event-loop thread; emits the signals.
  void ReportResult(k6wp::UpdateCheckOutcome outcome, const QString& latest,
                    const QString& page);
  // interactive == false keeps the start-up path fully silent.
  void CheckInternal(const QString& api_url, bool interactive);

  QString current_version_;
  QString releases_page_url_;
  std::atomic<bool> busy_{false};
};

}  // namespace k6wp
