#include "lockscreen_backup.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <cstdio>
#include <fstream>
#include <iterator>

namespace k6wp::launcher {

std::string EscapeBackupJson(const std::wstring& wide) {
  // Todo 19 (HIGH-2): non-ASCII used to flatten to '?' (data loss). Convert
  // the whole string to UTF-8 first so surrogate pairs survive, then JSON-
  // escape only what JSON requires; UTF-8 bytes >= 0x80 pass through raw.
  std::string utf8;
  if (!wide.empty()) {
    const int utf8_len = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(),
                                             static_cast<int>(wide.size()),
                                             nullptr, 0, nullptr, nullptr);
    if (utf8_len > 0) {
      utf8.resize(static_cast<std::size_t>(utf8_len));
      WideCharToMultiByte(CP_UTF8, 0, wide.c_str(),
                          static_cast<int>(wide.size()), utf8.data(),
                          utf8_len, nullptr, nullptr);
    }
  }
  std::string out;
  for (const unsigned char c : utf8) {
    if (c == '\\') {
      out += "\\\\";
    } else if (c == '"') {
      out += "\\\"";
    } else if (c < 0x20) {
      char hex[8] = {};
      std::snprintf(hex, sizeof(hex), "\\u%04x", c);
      out += hex;
    } else {
      out += static_cast<char>(c);
    }
  }
  return out;
}

bool WriteLockscreenBackup(const std::filesystem::path& backup_path,
                           bool had_value, const std::wstring& value) {
  // Crash-safe publish (same contract as config.json): write a sibling .tmp,
  // then replace atomically. Losing this file loses the user's pre-K6WP policy
  // permanently, so a truncated in-place write is not acceptable here.
  const std::filesystem::path tmp =
      backup_path.parent_path() /
      (backup_path.filename().wstring() + L".tmp");
  {
    std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
    if (!out) return false;
    out << "{\"had_value\":" << (had_value ? "true" : "false") << ",\"value\":\""
        << EscapeBackupJson(value) << "\"}";
    out.flush();
    if (!out) {
      DeleteFileW(tmp.c_str());
      return false;
    }
  }
  if (!MoveFileExW(tmp.c_str(), backup_path.c_str(),
                   MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
    DeleteFileW(tmp.c_str());
    return false;
  }
  return true;
}

bool ReadLockscreenBackup(const std::filesystem::path& backup_path,
                          bool& had_value, std::wstring& value) {
  had_value = false;
  value.clear();
  std::ifstream in(backup_path, std::ios::binary);
  if (!in) return false;
  const std::string text((std::istreambuf_iterator<char>(in)),
                         std::istreambuf_iterator<char>());
  had_value = text.find("\"had_value\":true") != std::string::npos;
  const auto vpos = text.find("\"value\":\"");
  if (vpos == std::string::npos) return had_value;
  // Todo 19 (HIGH-2): decode JSON escapes properly (\\, \", \uXXXX with
  // surrogate pairs) into UTF-8, then convert to wide. The old byte-for-byte
  // copy mangled every escape and every multi-byte UTF-8 sequence.
  std::string utf8;
  bool esc = false;
  wchar_t pending_high = 0;
  for (std::size_t i = vpos + 9; i < text.size(); ++i) {
    const char c = text[i];
    if (esc) {
      esc = false;
      if (c == '\\') {
        utf8 += '\\';
      } else if (c == '"') {
        utf8 += '"';
      } else if (c == 'u') {
        if (i + 4 < text.size()) {
          unsigned int cp = 0;
          bool ok = true;
          for (std::size_t j = i + 1; j <= i + 4; ++j) {
            const char h = text[j];
            cp <<= 4;
            if (h >= '0' && h <= '9') {
              cp |= static_cast<unsigned int>(h - '0');
            } else if (h >= 'a' && h <= 'f') {
              cp |= static_cast<unsigned int>(h - 'a' + 10);
            } else if (h >= 'A' && h <= 'F') {
              cp |= static_cast<unsigned int>(h - 'A' + 10);
            } else {
              ok = false;
              break;
            }
          }
          i += 4;
          if (ok) {
            const wchar_t unit = static_cast<wchar_t>(cp);
            char buf[4] = {};
            int n = 0;
            if (pending_high != 0) {
              const wchar_t pair[2] = {pending_high, unit};
              pending_high = 0;
              n = WideCharToMultiByte(CP_UTF8, 0, pair, 2, buf,
                                      static_cast<int>(sizeof(buf)), nullptr,
                                      nullptr);
            } else if (unit >= 0xD800 && unit <= 0xDBFF) {
              pending_high = unit;
            } else {
              n = WideCharToMultiByte(CP_UTF8, 0, &unit, 1, buf,
                                      static_cast<int>(sizeof(buf)), nullptr,
                                      nullptr);
            }
            if (n > 0) {
              utf8.append(buf, static_cast<std::size_t>(n));
            }
          }
        }
      } else {
        utf8 += c;  // unknown escape: keep the literal char
      }
    } else if (c == '\\') {
      esc = true;
    } else if (c == '"') {
      break;
    } else {
      utf8 += c;
    }
  }
  if (!utf8.empty()) {
    const int wide_len = MultiByteToWideChar(CP_UTF8, 0, utf8.data(),
                                             static_cast<int>(utf8.size()),
                                             nullptr, 0);
    if (wide_len <= 0) return false;
    value.resize(static_cast<std::size_t>(wide_len));
    MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()),
                        value.data(), wide_len);
  }
  // Todo 19 (HIGH-2) backward compat: a pre-fix backup flattened every
  // non-ASCII char to '?'. Warn explicitly instead of silently restoring a
  // corrupted path.
  if (value.find(L'?') != std::wstring::npos) {
    std::fwprintf(stderr,
                  L"K6WP: warning: lockscreen backup looks corrupted (legacy "
                  L"'?' escaping) -- original path may be unrecoverable.\n");
  }
  return true;
}

}  // namespace k6wp::launcher
