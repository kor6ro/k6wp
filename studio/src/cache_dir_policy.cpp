#include "cache_dir_policy.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shlobj.h>

#include <algorithm>
#include <cwctype>
#include <fstream>
#include <string>
#include <vector>

namespace k6wp {

namespace {

// --- what clearCache() is allowed to sweep -----------------------------------
//
// cache_dir is free text: ValidateStudioSettings (shared/studio_settings.cpp)
// rejects only the empty string, so it can be a drive root, a user-content
// folder, or whatever a mistyped path resolves to. The sweep removes every
// regular file under it with no trash and no undo, so the decision is made
// here, before the first file is touched.

// %LOCALAPPDATA%\K6WP subdirectories holding the user's own files rather than
// cache: the compress output and the thumbnail store.
constexpr const wchar_t* kNonCacheDataDirs[] = {L"wallpapers", L"thumbs"};

// Windows compares paths case-insensitively and ignores a "\\?\" prefix, so
// the guard has to as well: one folder reached as "C:\Users\me\Videos" and
// "\\?\c:\users\me\videos\" is one folder. A volume root keeps its separator,
// because stripping it would leave a bare drive-relative "c:".
std::wstring FoldPath(const std::filesystem::path& path) {
  std::wstring folded = path.wstring();
  if (folded.rfind(L"\\\\?\\", 0) == 0) {
    folded.erase(0, 4);
  }
  std::replace(folded.begin(), folded.end(), L'/', L'\\');
  while (folded.size() > 3 && folded.back() == L'\\') {
    folded.pop_back();
  }
  for (wchar_t& c : folded) {
    c = static_cast<wchar_t>(towlower(c));
  }
  return folded;
}

bool IsSamePath(const std::filesystem::path& a,
                const std::filesystem::path& b) {
  return FoldPath(a) == FoldPath(b);
}

// True when `child` is strictly below `ancestor`, not equal to it.
bool IsUnder(const std::filesystem::path& child,
             const std::filesystem::path& ancestor) {
  const std::wstring parent = FoldPath(ancestor);
  if (parent.empty()) {
    return false;
  }
  const std::wstring prefix =
      parent.back() == L'\\' ? parent : parent + L'\\';
  const std::wstring path = FoldPath(child);
  return path.size() > prefix.size() &&
         path.compare(0, prefix.size(), prefix) == 0;
}

bool IsVolumeRoot(const std::filesystem::path& path) {
  return path.has_root_path() && !path.has_relative_path();
}

// Where the sweep would actually go. A junction or a symlink is a name, not a
// location, so resolving it is the difference between checking the folder the
// user picked and checking the folder that gets emptied. False means the
// target could not be resolved, and the caller refuses rather than guessing.
bool ResolveTarget(const std::filesystem::path& path,
                   std::filesystem::path* target) {
  std::error_code ec;
  target->assign(std::filesystem::canonical(path, ec));
  if (!ec) {
    return true;
  }
  ec.clear();
  target->assign(std::filesystem::weakly_canonical(path, ec));
  return !ec;
}

// A Windows known folder (Videos, Documents, ...), or false when the shell has
// none. Asked of the shell rather than assembled from %USERPROFILE% so a folder
// OneDrive has moved off the profile is still recognised.
bool KnownFolder(const KNOWNFOLDERID& id, std::filesystem::path* path) {
  PWSTR resolved = nullptr;
  if (FAILED(SHGetKnownFolderPath(id, 0, nullptr, &resolved)) ||
      resolved == nullptr) {
    return false;
  }
  *path = std::filesystem::path(resolved);
  CoTaskMemFree(resolved);
  return true;
}

// Directories that are never a cache, whatever cache_dir says: the K6WP data
// root (studio_settings.json, config.json, library.json live there), its
// parent, the profile itself, and the six folders Windows treats as the user's
// own content. The profile-relative spellings are the fallback for the case
// where the shell cannot answer.
std::vector<std::filesystem::path> ProtectedDirs(
    const std::filesystem::path& data_root) {
  std::vector<std::filesystem::path> dirs;
  dirs.push_back(data_root);
  dirs.push_back(data_root.parent_path());
  wchar_t profile[MAX_PATH] = {};
  if (GetEnvironmentVariableW(L"USERPROFILE", profile, MAX_PATH) > 0) {
    const std::filesystem::path home(profile);
    dirs.push_back(home);
    static const wchar_t* const kContentDirs[] = {
        L"Desktop", L"Documents", L"Downloads", L"Music", L"Pictures",
        L"Videos"};
    for (const wchar_t* leaf : kContentDirs) {
      dirs.push_back(home / leaf);
    }
  }
  static const KNOWNFOLDERID kContentIds[] = {
      FOLDERID_Desktop, FOLDERID_Documents, FOLDERID_Downloads,
      FOLDERID_Music,   FOLDERID_Pictures,  FOLDERID_Videos};
  for (const KNOWNFOLDERID& id : kContentIds) {
    std::filesystem::path resolved;
    if (KnownFolder(id, &resolved)) {
      dirs.push_back(std::move(resolved));
    }
  }
  return dirs;
}

// True only when `dir` is PROVEN to hold no regular file at all, i.e. there is
// nothing a sweep could destroy. false also covers an unreadable walk, which
// the caller treats as a refusal.
bool HoldsNoRegularFile(const std::filesystem::path& dir) {
  std::error_code ec;
  for (std::filesystem::recursive_directory_iterator it(dir, ec), end;
       it != end; it.increment(ec)) {
    if (ec) {
      return false;
    }
    if (it->is_regular_file(ec) && !ec) {
      return false;
    }
    if (ec) {
      return false;
    }
  }
  return !ec;
}

// What the refusal names: what the user typed, plus where it actually points
// when those differ, because a junction is a name and not a location.
QString DescribePath(const std::filesystem::path& configured,
                     const std::filesystem::path& target) {
  const QString shown = QString::fromStdWString(configured.wstring());
  if (IsSamePath(configured, target)) {
    return shown;
  }
  return shown + QStringLiteral(" -> ") +
         QString::fromStdWString(target.wstring());
}

}  // namespace

bool CacheDirIsClearable(const std::filesystem::path& configured,
                         const std::filesystem::path& data_root,
                         QString* refusal) {
  std::error_code ec;
  if (!std::filesystem::is_directory(configured, ec) || ec) {
    *refusal = QStringLiteral("Cache ditolak: %1 bukan folder.").arg(
        QString::fromStdWString(configured.wstring()));
    return false;
  }
  if (IsVolumeRoot(configured)) {
    *refusal = QStringLiteral(
                   "Cache ditolak: %1 adalah akar drive, bukan folder cache.")
                   .arg(QString::fromStdWString(configured.wstring()));
    return false;
  }
  std::filesystem::path target;
  if (!ResolveTarget(configured, &target)) {
    *refusal = QStringLiteral(
                   "Cache ditolak: %1 tidak bisa dipastikan lokasinya.")
                   .arg(QString::fromStdWString(configured.wstring()));
    return false;
  }
  if (IsVolumeRoot(target)) {
    *refusal = QStringLiteral(
                   "Cache ditolak: %1 menunjuk ke akar drive.")
                   .arg(DescribePath(configured, target));
    return false;
  }
  for (const std::filesystem::path& protected_dir : ProtectedDirs(data_root)) {
    if (IsSamePath(protected_dir, configured) ||
        IsSamePath(protected_dir, target)) {
      *refusal =
          QStringLiteral("Cache ditolak: %1 bukan folder cache K6WP (data "
                         "pengguna, bukan cache).")
              .arg(DescribePath(configured, target));
      return false;
    }
  }
  if (IsUnder(target, data_root)) {
    for (const wchar_t* leaf : kNonCacheDataDirs) {
      if (IsSamePath(data_root / leaf, target) ||
          IsUnder(target, data_root / leaf)) {
        *refusal =
            QStringLiteral("Cache ditolak: %1 bukan cache (berisi hasil "
                           "kompres dan thumbnail).")
                .arg(DescribePath(configured, target));
        return false;
      }
    }
    return true;
  }
  if (std::filesystem::exists(target / kCacheOptInMarker, ec) && !ec) {
    return true;
  }
  if (HoldsNoRegularFile(target)) {
    return true;
  }
  *refusal = QStringLiteral(
                 "Cache ditolak: %1 belum ditandai sebagai cache K6WP. Pilih "
                 "ulang folder itu lewat tombol picker, atau kosongkan "
                 "isinya dulu.")
                 .arg(QString::fromStdWString(configured.wstring()));
  return false;
}

bool MarkCacheDirOptedIn(const std::filesystem::path& dir) {
  std::error_code ec;
  const std::filesystem::path marker = dir / kCacheOptInMarker;
  if (std::filesystem::exists(marker, ec) && !ec) {
    return true;
  }
  std::ofstream out(marker, std::ios::binary | std::ios::trunc);
  if (!out) {
    return false;
  }
  out << "k6wp cache dir v1\n";
  out.flush();
  return static_cast<bool>(out);
}

}  // namespace k6wp
