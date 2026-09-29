// Crash-dump directory cap. See crash_dump_prune.hpp for the contract.
//
// QE-guarded like the crash filter that calls it (main.cpp): no CRT heap, no
// std::filesystem, no mutex, no recursion, and only two WIN32_FIND_DATAW on
// the stack at a time - the faulting thread's stack may already be nearly
// exhausted, so a full file listing is never materialised.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <cwchar>

#include "crash_dump_prune.hpp"

namespace k6wp {
namespace {

unsigned long long FileTimeTicks(const FILETIME& ft) {
  return (static_cast<unsigned long long>(ft.dwHighDateTime) << 32) |
         ft.dwLowDateTime;
}

}  // namespace

void PruneOldCrashDumps(const wchar_t* crash_dir) {
  if (crash_dir == nullptr) {
    return;
  }
  wchar_t pattern[MAX_PATH] = {};
  if (swprintf_s(pattern, MAX_PATH, L"%s\\engine-*.dmp", crash_dir) < 0) {
    return;
  }
  for (unsigned pass = 0; pass < kMaxCrashPrunePasses; ++pass) {
    WIN32_FIND_DATAW fd = {};
    WIN32_FIND_DATAW oldest = {};
    bool have_oldest = false;
    unsigned present = 0;
    HANDLE find = FindFirstFileW(pattern, &fd);
    if (find == INVALID_HANDLE_VALUE) {
      return;
    }
    do {
      if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
        continue;
      }
      ++present;
      if (!have_oldest) {
        oldest = fd;
        have_oldest = true;
        continue;
      }
      const unsigned long long t = FileTimeTicks(fd.ftLastWriteTime);
      const unsigned long long oldest_t = FileTimeTicks(oldest.ftLastWriteTime);
      if (t < oldest_t ||
          (t == oldest_t && wcscmp(fd.cFileName, oldest.cFileName) < 0)) {
        oldest = fd;
      }
    } while (FindNextFileW(find, &fd) != FALSE);
    FindClose(find);
    // The new dump is written after this returns, so stop one below the cap to
    // leave exactly one slot free for it.
    if (!have_oldest || present < kMaxCrashDumps) {
      return;
    }
    wchar_t victim[MAX_PATH] = {};
    if (swprintf_s(victim, MAX_PATH, L"%s\\%s", crash_dir, oldest.cFileName) < 0 ||
        DeleteFileW(victim) == FALSE) {
      return;  // locked by AV / another process: keep what is there
    }
  }
}

}  // namespace k6wp
