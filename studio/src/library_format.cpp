#include "library_format.hpp"

#include <QDir>
#include <QUrl>

#include <algorithm>
#include <cctype>

namespace k6wp {

namespace {

std::string ToLowerAscii(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return s;
}

}  // namespace

bool FilterMatches(const std::string& name, const std::string& label,
                   const std::string& query) {
  if (query.empty()) {
    return true;
  }
  const std::string q = ToLowerAscii(query);
  return ToLowerAscii(name).find(q) != std::string::npos ||
         ToLowerAscii(label).find(q) != std::string::npos;
}

std::string EntryLabel(const LibraryEntry& entry) {
  const std::string res =
      (entry.res.empty() || entry.res == "0x0") ? "\xE2\x80\x94" : entry.res;

  std::string dur_str;
  if (entry.duration > 0.0) {
    const int total_sec = static_cast<int>(entry.duration);
    const int h = total_sec / 3600;
    const int m = (total_sec % 3600) / 60;
    const int s = total_sec % 60;
    if (h > 0) {
      dur_str = std::to_string(h) + " jam " + std::to_string(m) + " mnt";
    } else if (m > 0) {
      dur_str = std::to_string(m) + " mnt " + std::to_string(s) + " dtk";
    } else {
      dur_str = std::to_string(s) + " dtk";
    }
  } else {
    dur_str = "\xE2\x80\x94";
  }

  std::string label = entry.dst.filename().u8string() + "\n" + res +
                      " \xE2\x80\xA2 " + dur_str;
  if (entry.broken) {
    label += "\n[file hilang]";
  } else if (!entry.src.empty() && entry.src == entry.dst) {
    label += "\n[belum dioptimasi]";
  }
  return label;
}

QString FileUrl(const std::filesystem::path& p) {
  if (p.empty()) {
    return QString();
  }
  return QUrl::fromLocalFile(
             QDir::toNativeSeparators(QString::fromStdWString(p.wstring())))
      .toString();
}

}  // namespace k6wp
