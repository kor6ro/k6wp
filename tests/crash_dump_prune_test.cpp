// Unit tests for the crash-dump directory cap (engine/src/crash_dump_prune).
// No external test framework: plain asserts with a pass/fail counter, same
// style as tests/occlusion_test.cpp. Exit code 0 = all pass.
//
// Everything runs in a throwaway %TEMP% subdirectory, never in the real
// %LOCALAPPDATA%\K6WP\crashes - the filter's own directory is created and
// destroyed per test case. mtimes are set with SetFileTime because the prune
// orders by ftLastWriteTime, and the writer's own timestamp format
// (engine-YYYYMMDD-HHMMSS.dmp) is reproduced so the name tie-break is
// exercised the way the real writer triggers it.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <cstdio>
#include <string>
#include <vector>

#include "crash_dump_prune.hpp"

namespace {

int g_checks = 0;
int g_failures = 0;

void Check(bool cond, const std::string& name) {
  ++g_checks;
  if (cond) {
    std::printf("[PASS] %s\n", name.c_str());
  } else {
    ++g_failures;
    std::printf("[FAIL] %s\n", name.c_str());
  }
}

std::wstring g_dir;  // %TEMP%\k6wp-prune-test-<pid>, created once

std::wstring Join(const wchar_t* name) {
  std::wstring p = g_dir;
  p += L'\\';
  p += name;
  return p;
}

// engine-YYYYMMDD-HHMMSS.dmp with an ordinal-derived timestamp, so index order
// == chronological order exactly as in production.
std::wstring DumpName(int index) {
  wchar_t buf[64] = {};
  swprintf_s(buf, 64, L"engine-20260927-%06d.dmp", index);
  return std::wstring(buf);
}

bool MakeDump(const std::wstring& path, unsigned long long ticks) {
  HANDLE f = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                         FILE_ATTRIBUTE_NORMAL, nullptr);
  if (f == INVALID_HANDLE_VALUE) {
    return false;
  }
  DWORD written = 0;
  WriteFile(f, "dmp", 3, &written, nullptr);
  CloseHandle(f);
  FILETIME ft = {};
  ft.dwLowDateTime = static_cast<DWORD>(ticks & 0xFFFFFFFFull);
  ft.dwHighDateTime = static_cast<DWORD>(ticks >> 32);
  HANDLE attr = CreateFileW(path.c_str(), FILE_WRITE_ATTRIBUTES,
                            FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (attr == INVALID_HANDLE_VALUE) {
    return false;
  }
  const BOOL ok = SetFileTime(attr, nullptr, nullptr, &ft);
  CloseHandle(attr);
  return ok != FALSE;
}

void MakePlainFile(const wchar_t* name) {
  HANDLE f = CreateFileW(Join(name).c_str(), GENERIC_WRITE, 0, nullptr,
                         CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (f != INVALID_HANDLE_VALUE) {
    DWORD written = 0;
    WriteFile(f, "x", 1, &written, nullptr);
    CloseHandle(f);
  }
}

bool Exists(const std::wstring& path) {
  const DWORD attrs = GetFileAttributesW(path.c_str());
  return attrs != INVALID_FILE_ATTRIBUTES &&
         (attrs & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

std::vector<std::wstring> ListDumps() {
  std::vector<std::wstring> out;
  WIN32_FIND_DATAW fd = {};
  const std::wstring pattern = g_dir + L"\\engine-*.dmp";
  HANDLE find = FindFirstFileW(pattern.c_str(), &fd);
  if (find == INVALID_HANDLE_VALUE) {
    return out;
  }
  do {
    if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
      out.push_back(fd.cFileName);
    }
  } while (FindNextFileW(find, &fd) != FALSE);
  FindClose(find);
  return out;
}

bool Contains(const std::vector<std::wstring>& v, const std::wstring& s) {
  for (const auto& e : v) {
    if (e == s) return true;
  }
  return false;
}

void ResetDir() {
  for (const auto& e : ListDumps()) {
    DeleteFileW(Join(e.c_str()).c_str());
  }
  for (const wchar_t* extra : {L"crash-notes.txt", L"engine.log.1", L"old.dmp"}) {
    DeleteFileW(Join(extra).c_str());
  }
}

// 25 dumps, distinct ascending mtimes: after a prune that leaves one slot for
// the dump about to be written, exactly 9 remain and they are the newest 9.
void TestKeepsNewestBelowCap() {
  ResetDir();
  for (int i = 1; i <= 25; ++i) {
    if (!MakeDump(Join(DumpName(i).c_str()).c_str(), 0x10000000ull + i)) {
      Check(false, "prune: fixture dump could not be created");
      return;
    }
  }
  Check(ListDumps().size() == 25, "prune: fixture holds 25 dumps");
  k6wp::PruneOldCrashDumps(g_dir.c_str());
  const std::vector<std::wstring> left = ListDumps();
  Check(left.size() == k6wp::kMaxCrashDumps - 1,
        "prune: over-cap directory is trimmed to one slot below the cap");
  bool newest_survived = true;
  bool oldest_gone = true;
  for (int i = 1; i <= 25; ++i) {
    const bool present = Contains(left, DumpName(i));
    if (i >= 25 - static_cast<int>(k6wp::kMaxCrashDumps) + 2 && !present) {
      newest_survived = false;
    }
    if (i <= 25 - static_cast<int>(k6wp::kMaxCrashDumps) && present) {
      oldest_gone = false;
    }
  }
  Check(newest_survived, "prune: the newest dumps survive");
  Check(oldest_gone, "prune: the oldest dumps are deleted");
}

// 8 dumps is under the cap: the prune must delete nothing.
void TestUnderCapIsUntouched() {
  ResetDir();
  for (int i = 1; i <= 8; ++i) {
    MakeDump(Join(DumpName(i).c_str()).c_str(), 0x10000000ull + i);
  }
  k6wp::PruneOldCrashDumps(g_dir.c_str());
  Check(ListDumps().size() == 8, "prune: an under-cap directory is left alone");
}

// Identical mtimes (coarse filesystem granularity) must still order
// deterministically: the zero-padded name decides, so the oldest names go.
void TestTimestampTieBreaksOnName() {
  ResetDir();
  const unsigned long long same = 0x20000000ull;
  for (int i = 1; i <= 14; ++i) {
    MakeDump(Join(DumpName(i).c_str()).c_str(), same);
  }
  k6wp::PruneOldCrashDumps(g_dir.c_str());
  const std::vector<std::wstring> left = ListDumps();
  Check(left.size() == k6wp::kMaxCrashDumps - 1,
        "prune: equal mtimes still reach the cap");
  bool newest_kept = true;
  for (int i = 1; i <= 5; ++i) {
    if (Contains(left, DumpName(i))) {
      newest_kept = false;
    }
  }
  Check(newest_kept, "prune: with equal mtimes the oldest names are deleted");
  Check(Contains(left, DumpName(14)), "prune: with equal mtimes the newest name is kept");
}

// A locked oldest file must abort the prune quietly, not throw and not delete
// anything else.
void TestLockedFileIsNonFatal() {
  ResetDir();
  for (int i = 1; i <= 15; ++i) {
    MakeDump(Join(DumpName(i).c_str()).c_str(), 0x10000000ull + i);
  }
  const std::wstring oldest = Join(DumpName(1).c_str());
  HANDLE lock = CreateFileW(oldest.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL, nullptr);
  Check(lock != INVALID_HANDLE_VALUE, "prune: oldest dump could be locked");
  if (lock == INVALID_HANDLE_VALUE) {
    return;
  }
  k6wp::PruneOldCrashDumps(g_dir.c_str());  // must return, not fail loudly
  Check(ListDumps().size() == 15, "prune: a locked dump leaves the rest in place");
  Check(Exists(oldest), "prune: a locked dump is not deleted");
  CloseHandle(lock);
  k6wp::PruneOldCrashDumps(g_dir.c_str());
  Check(ListDumps().size() == k6wp::kMaxCrashDumps - 1,
        "prune: pruning resumes once the lock is gone");
}

// Nothing in the directory that is not an engine-*.dmp is touched.
void TestForeignFilesSurvive() {
  ResetDir();
  for (int i = 1; i <= 15; ++i) {
    MakeDump(Join(DumpName(i).c_str()).c_str(), 0x10000000ull + i);
  }
  MakePlainFile(L"crash-notes.txt");
  MakePlainFile(L"engine.log.1");
  MakePlainFile(L"old.dmp");
  k6wp::PruneOldCrashDumps(g_dir.c_str());
  Check(Exists(Join(L"crash-notes.txt")) && Exists(Join(L"engine.log.1")) &&
            Exists(Join(L"old.dmp")),
        "prune: non-dump files in the directory are never deleted");
}

// Work is bounded: a directory far over the cap returns after
// kMaxCrashPrunePasses deletions instead of looping until the cap.
void TestWorkIsBounded() {
  ResetDir();
  const int total = static_cast<int>(k6wp::kMaxCrashDumps +
                                     k6wp::kMaxCrashPrunePasses + 5);
  for (int i = 1; i <= total; ++i) {
    MakeDump(Join(DumpName(i).c_str()).c_str(), 0x10000000ull + i);
  }
  k6wp::PruneOldCrashDumps(g_dir.c_str());
  Check(ListDumps().size() ==
            static_cast<std::size_t>(total - k6wp::kMaxCrashPrunePasses),
        "prune: work is bounded by kMaxCrashPrunePasses deletes");
}

void TestMissingDirIsNoOp() {
  const std::wstring missing = g_dir + L"\\does-not-exist";
  k6wp::PruneOldCrashDumps(missing.c_str());
  k6wp::PruneOldCrashDumps(nullptr);
  Check(true, "prune: missing directory and null path are no-ops");
}

}  // namespace

int main() {
  wchar_t temp[MAX_PATH] = {};
  const DWORD len = GetTempPathW(MAX_PATH, temp);
  if (len == 0 || len >= MAX_PATH) {
    std::printf("[FAIL] prune: no usable TEMP directory\n");
    return 1;
  }
  wchar_t dir[MAX_PATH] = {};
  if (swprintf_s(dir, MAX_PATH, L"%sk6wp-prune-test-%lu", temp,
                 static_cast<unsigned long>(GetCurrentProcessId())) < 0) {
    std::printf("[FAIL] prune: temp path too long\n");
    return 1;
  }
  g_dir = dir;
  CreateDirectoryW(g_dir.c_str(), nullptr);

  TestKeepsNewestBelowCap();
  TestUnderCapIsUntouched();
  TestTimestampTieBreaksOnName();
  TestLockedFileIsNonFatal();
  TestForeignFilesSurvive();
  TestWorkIsBounded();
  TestMissingDirIsNoOp();

  ResetDir();
  RemoveDirectoryW(g_dir.c_str());

  std::printf("%d checks, %d failures\n", g_checks, g_failures);
  return g_failures == 0 ? 0 : 1;
}
