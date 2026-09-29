#include "update_checker.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <winhttp.h>

#include <QMetaObject>
#include <QPointer>

#include <cctype>
#include <thread>
#include <vector>

#include "thirdparty/json.hpp"

#include "links.hpp"
#include "version.h"
#include "version_compare.hpp"

namespace k6wp {

namespace {

// P4.2: WinHTTP plumbing for the update check. Everything here is
// synchronous/blocking and runs on a detached worker thread (never the Qt
// event loop). Any failure returns empty/false — the caller stays silent,
// matching the previous QNetworkAccessManager semantics.

struct WinHttpHandle {
  explicit WinHttpHandle(HINTERNET handle = nullptr) : h(handle) {}
  ~WinHttpHandle() {
    if (h != nullptr) WinHttpCloseHandle(h);
  }
  WinHttpHandle(const WinHttpHandle&) = delete;
  WinHttpHandle& operator=(const WinHttpHandle&) = delete;
  HINTERNET get() const { return h; }

 private:
  HINTERNET h = nullptr;
};

std::wstring ToWide(const std::string& utf8) {
  if (utf8.empty()) return std::wstring();
  const int n = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
  if (n <= 0) return std::wstring();
  std::vector<wchar_t> buf(static_cast<size_t>(n));
  if (MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, buf.data(), n) <= 0) {
    return std::wstring();
  }
  return std::wstring(buf.data());
}

std::string TrimAscii(const std::string& s) {
  size_t beg = 0;
  while (beg < s.size() &&
         std::isspace(static_cast<unsigned char>(s[beg])) != 0) {
    ++beg;
  }
  size_t end = s.size();
  while (end > beg &&
         std::isspace(static_cast<unsigned char>(s[end - 1])) != 0) {
    --end;
  }
  return s.substr(beg, end - beg);
}

// Blocking GET with 10 s timeouts. Returns the response body, or empty on
// any failure (DNS, connect, TLS, non-200 status, read error, oversize).
std::string WinHttpGet(const std::string& url_utf8) {
  static constexpr DWORD kTimeoutMs = 10000;
  static constexpr size_t kMaxBody = 256 * 1024;  // releases API is ~few KB
  const std::wstring wurl = ToWide(url_utf8);
  if (wurl.empty()) return std::string();

  wchar_t host[256] = {};
  wchar_t url_path[2048] = {};
  URL_COMPONENTS uc{};
  uc.dwStructSize = sizeof(uc);
  uc.lpszHostName = host;
  uc.dwHostNameLength = static_cast<DWORD>(std::size(host));
  uc.lpszUrlPath = url_path;
  uc.dwUrlPathLength = static_cast<DWORD>(std::size(url_path));
  if (WinHttpCrackUrl(wurl.c_str(), 0, 0, &uc) == FALSE) {
    return std::string();
  }
  if (uc.nScheme != INTERNET_SCHEME_HTTP &&
      uc.nScheme != INTERNET_SCHEME_HTTPS) {
    return std::string();
  }
  const bool secure = (uc.nScheme == INTERNET_SCHEME_HTTPS);

  WinHttpHandle session(WinHttpOpen(
      L"K6WP/" K6WP_VERSION_STR, WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
      WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0));
  if (session.get() == nullptr) return std::string();
  if (WinHttpSetTimeouts(session.get(), kTimeoutMs, kTimeoutMs, kTimeoutMs,
                         kTimeoutMs) == FALSE) {
    return std::string();
  }
  WinHttpHandle conn(
      WinHttpConnect(session.get(), host, uc.nPort, 0));
  if (conn.get() == nullptr) return std::string();
  WinHttpHandle req(WinHttpOpenRequest(
      conn.get(), L"GET", url_path, nullptr, WINHTTP_NO_REFERER,
      WINHTTP_DEFAULT_ACCEPT_TYPES, secure ? WINHTTP_FLAG_SECURE : 0));
  if (req.get() == nullptr) return std::string();
  if (WinHttpAddRequestHeaders(
          req.get(), L"Accept: application/vnd.github+json",
          static_cast<DWORD>(-1L),
          WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE) == FALSE) {
    return std::string();
  }
  if (WinHttpSendRequest(req.get(), WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                         WINHTTP_NO_REQUEST_DATA, 0, 0, 0) == FALSE) {
    return std::string();
  }
  if (WinHttpReceiveResponse(req.get(), nullptr) == FALSE) {
    return std::string();
  }
  DWORD status = 0;
  DWORD status_len = sizeof(status);
  if (WinHttpQueryHeaders(req.get(),
                          WINHTTP_QUERY_STATUS_CODE |
                              WINHTTP_QUERY_FLAG_NUMBER,
                          WINHTTP_HEADER_NAME_BY_INDEX, &status, &status_len,
                          WINHTTP_NO_HEADER_INDEX) == FALSE ||
      status != 200) {
    return std::string();
  }
  std::string body;
  for (;;) {
    DWORD avail = 0;
    if (WinHttpQueryDataAvailable(req.get(), &avail) == FALSE) {
      return std::string();
    }
    if (avail == 0) break;
    if (body.size() + avail > kMaxBody) return std::string();
    const size_t base = body.size();
    body.resize(base + avail);
    DWORD read = 0;
    if (WinHttpReadData(req.get(), &body[base], avail, &read) == FALSE) {
      return std::string();
    }
    body.resize(base + read);
    if (read == 0) break;
  }
  return body;
}

}  // namespace

bool IsUpdateCheckUrlUsable(const QString& url) {
  const QString t = url.trimmed();
  if (t.isEmpty() || t.startsWith(QStringLiteral("TODO"))) {
    return false;
  }
  return t.startsWith(QStringLiteral("http://")) ||
         t.startsWith(QStringLiteral("https://"));
}

QString ResolveUpdateCheckUrl() {
  QString url =
      QString::fromLocal8Bit(qgetenv("K6WP_UPDATE_URL")).trimmed();
  if (url.isEmpty()) {
    url = QString::fromUtf8(K6WP_UPDATE_CHECK_URL);
  }
  if (!IsUpdateCheckUrlUsable(url)) {
    return QString();
  }
  return url;
}

UpdateChecker::UpdateChecker(const QString& current_version, QObject* parent)
    : QObject(parent), current_version_(current_version) {
  releases_page_url_ = QString::fromUtf8(K6WP_RELEASES_URL);
}

void UpdateChecker::SetReleasesPageUrl(const QString& url) {
  releases_page_url_ = url;
}

void UpdateChecker::Check(const QString& api_url) {
  if (!IsUpdateCheckUrlUsable(api_url)) {
    return;  // Disabled: no request, fully silent.
  }
  bool expected = false;
  if (!busy_.compare_exchange_strong(expected, true)) {
    return;  // One check at a time.
  }
  // Copies cross the thread (members stay on the event-loop thread).
  QPointer<UpdateChecker> guard(this);
  const std::string url_utf8 = api_url.trimmed().toStdString();
  const QString page = releases_page_url_;
  const QString current = current_version_;
  std::thread([this, guard, url_utf8, page, current]() {
    FetchAndReport(url_utf8, page, current, guard);
  }).detach();
}

void UpdateChecker::FetchAndReport(std::string api_url_utf8,
                                   QString releases_page,
                                   QString current_version,
                                   QPointer<UpdateChecker> guard) {
  bool newer = false;
  QString latest;
  QString page = releases_page;
  const std::string body = WinHttpGet(api_url_utf8);
  if (!body.empty()) {
    try {
      const nlohmann::json doc = nlohmann::json::parse(body);
      if (doc.is_object()) {
        std::string tag;
        std::string html;
        const auto tag_it = doc.find("tag_name");
        if (tag_it != doc.end() && tag_it->is_string()) {
          tag = TrimAscii(tag_it->get<std::string>());
        }
        const auto html_it = doc.find("html_url");
        if (html_it != doc.end() && html_it->is_string()) {
          html = TrimAscii(html_it->get<std::string>());
        }
        if (!tag.empty() &&
            IsNewerVersion(tag, current_version.toStdString())) {
          newer = true;
          latest = QString::fromStdString(tag);
          if (!html.empty()) {
            page = QString::fromStdString(html);
          }
        }
      }
    } catch (const std::exception&) {
      // Silent: network/parse failures never surface (existing semantics).
    }
  }
  busy_.store(false);
  // A deleted checker drops its queued events in the QObject dtor, so a
  // post-after-destroy can never fire on a dangling pointer; the guard
  // skips the post entirely when the window already went away.
  if (guard) {
    QMetaObject::invokeMethod(
        guard,
        [guard, newer, latest, page]() {
          if (guard) guard->ReportResult(newer, latest, page);
        },
        Qt::QueuedConnection);
  }
}

void UpdateChecker::ReportResult(bool newer, const QString& latest,
                                 const QString& page) {
  if (newer) {
    emit UpdateAvailable(latest, page);
  }
  emit CheckFinished(newer);
}

}  // namespace k6wp
