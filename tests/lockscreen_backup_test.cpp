// Mini unit tests for launcher/main.cpp lockscreen backup escape/decode
// (Todo 19 HIGH-2). No external test framework: plain asserts with a
// pass/fail counter. Exit code 0 = all pass.
//
// The functions under test live in an anonymous namespace inside
// launcher/main.cpp (single-TU dispatcher by design), so this test includes
// the .cpp directly. wWinMain is compiled but unused (console subsystem
// entry point is main()).
#include "../launcher/main.cpp"

#include <iostream>

namespace {

int g_checks = 0;
int g_failures = 0;

void Check(bool cond, const std::string& name) {
  ++g_checks;
  if (cond) {
    std::cout << "[PASS] " << name << "\n";
  } else {
    ++g_failures;
    std::cout << "[FAIL] " << name << "\n";
  }
}

std::filesystem::path TempDir() {
  auto dir = std::filesystem::temp_directory_path() / "k6wp_lockscreen_test";
  std::error_code ec;
  std::filesystem::remove_all(dir, ec);  // fresh per run
  std::filesystem::create_directories(dir, ec);
  return dir;
}

void WriteText(const std::filesystem::path& p, const std::string& text) {
  std::ofstream out(p, std::ios::binary | std::ios::trunc);
  out << text;
}

void TestEscapeBackupJson() {
  using k6wp::launcher::EscapeBackupJson;
  Check(EscapeBackupJson(L"") == "", "escape: empty");
  Check(EscapeBackupJson(L"plain") == "plain", "escape: ascii");
  Check(EscapeBackupJson(L"a\\b\"c") == "a\\\\b\\\"c",
        "escape: backslash + quote");
  Check(EscapeBackupJson(L"a\nb") == "a\\u000ab", "escape: control char");
  // Non-ASCII must survive as UTF-8, never '?'.
  Check(EscapeBackupJson(L"José") == "Jos\xc3\xa9",
        "escape: accent -> UTF-8");
  Check(EscapeBackupJson(L"测试") == "\xe6\xb5\x8b\xe8\xaf\x95",
        "escape: CJK -> UTF-8");
  Check(EscapeBackupJson(L"\U0001F600") == "\xf0\x9f\x98\x80",
        "escape: emoji surrogate pair -> UTF-8");
  Check(EscapeBackupJson(L"José").find('?') == std::string::npos,
        "escape: no '?' for non-ASCII");
}

void TestRoundTrip(const std::wstring& path, const std::string& name) {
  const auto dir = TempDir();
  const auto backup = dir / "backup.json";
  Check(k6wp::launcher::WriteLockscreenBackup(backup, true, path),
        "round-trip write: " + name);
  bool had = false;
  std::wstring got;
  Check(k6wp::launcher::ReadLockscreenBackup(backup, had, got),
        "round-trip read: " + name);
  Check(had, "round-trip had_value: " + name);
  Check(got == path, "round-trip identical: " + name);
}

void TestRoundTrips() {
  TestRoundTrip(L"C:\\Users\\José\\Pictures\\wallpaper.jpg",
                "accent path");
  TestRoundTrip(L"C:\\Users\\测试\\图片\\壁纸.jpg", "CJK path");
  TestRoundTrip(L"C:\\Users\\José\\测试\\emoji \U0001F600\\wall.jpg",
                "mixed CJK + emoji path");
  TestRoundTrip(L"C:\\path with \"quotes\" and \\backslash\\x.jpg",
                "quotes + backslashes");
  TestRoundTrip(L"C:\\tab\there\\newline\nhere.jpg", "control chars");
}

void TestUnicodeEscapeDecode() {
  const auto dir = TempDir();
  const auto backup = dir / "u.json";
  // \u00e9 = é, \u6d4b = 测, \u8bd5 = 试, surrogate pair \ud83d\ude00 = 😀
  WriteText(backup,
            "{\"had_value\":true,\"value\":\"Jos\\u00e9 \\u6d4b\\u8bd5 "
            "\\ud83d\\ude00\"}");
  bool had = false;
  std::wstring got;
  Check(k6wp::launcher::ReadLockscreenBackup(backup, had, got),
        "u-escape: read");
  Check(had, "u-escape: had_value");
  Check(got == L"José 测试 \U0001F600", "u-escape: decoded correctly");
}

void TestLegacyQuestionMark() {
  const auto dir = TempDir();
  const auto backup = dir / "legacy.json";
  // Legacy writer (pre-fix) replaced every non-ASCII wchar with '?'.
  WriteText(backup,
            "{\"had_value\":true,\"value\":\"C:\\\\Users\\\\Jos?\\\\x.jpg\"}");
  bool had = false;
  std::wstring got;
  Check(k6wp::launcher::ReadLockscreenBackup(backup, had, got),
        "legacy: read");
  Check(had, "legacy: had_value");
  Check(got == L"C:\\Users\\Jos?\\x.jpg", "legacy: value preserved as-is");
}

}  // namespace

int main() {
  TestEscapeBackupJson();
  TestRoundTrips();
  TestUnicodeEscapeDecode();
  TestLegacyQuestionMark();
  std::cout << (g_failures == 0 ? "ALL PASS" : "FAILURES") << " ("
            << g_checks << " checks, " << g_failures << " failures)\n";
  return g_failures == 0 ? 0 : 1;
}