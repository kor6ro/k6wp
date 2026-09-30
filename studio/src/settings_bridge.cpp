// Phase 4 of the Widgets -> QML migration: the Pengaturan tab's backend.
// See settings_bridge.hpp for why autostart is written through immediately
// while everything else waits for apply().

#include "settings_bridge.hpp"

#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QUrl>

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
#include <filesystem>
#include <fstream>
#include <vector>

#include "autostart.hpp"
#include "config_schema.hpp"

namespace k6wp {
namespace {

// --- what clearCache() is allowed to sweep -----------------------------------
//
// cache_dir is free text: ValidateStudioSettings (shared/studio_settings.cpp)
// rejects only the empty string, so it can be a drive root, a user-content
// folder, or whatever a mistyped path resolves to. The sweep removes every
// regular file under it with no trash and no undo, so the decision is made
// here, before the first file is touched.

// The opt-in marker pickCacheDir() drops in a folder the user chose. Its
// presence IS the opt-in: an external cache is a folder the user picked in the
// dialog, and nothing else.
constexpr wchar_t kCacheOptInMarker[] = L".k6wp-cache";

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

// What may be swept:
//   * a volume root never is - the walk would take a whole drive;
//   * never a protected directory, checked on the configured path AND on the
//     resolved target, so a junction cannot launder one past the check;
//   * inside the K6WP data root, anything except the root itself and the two
//     sibling data dirs the user's own files live in;
//   * outside it, only a folder the user opted in through pickCacheDir(), or
//     one that holds no regular file at all - an empty folder has nothing to
//     lose, so "point the cache at a fresh folder on D:\" keeps working
//     without asking the user to re-pick it.
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

// Drops the opt-in marker in a folder the user just picked, which is what lets
// clearCache() sweep it. Idempotent; false means the folder is not opted in and
// the caller has to say so rather than pretend it is.
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

}  // namespace

SettingsBridge::SettingsBridge(QObject* parent) : QObject(parent) {
  settings_path_ = DefaultStudioSettingsPath();
  config_path_ = DefaultConfigPath();
  // Read before reload() so a QML binding on `language` sees the stored value
  // on its first evaluation instead of flashing the default.
  ui_language_ = LoadUiLanguage();
  reload();
}

SettingsBridge::~SettingsBridge() = default;

void SettingsBridge::reload() {
  bool loaded = true;
  try {
    studio_ = LoadStudioSettings(settings_path_);
  } catch (const ConfigError& e) {
    // LoadStudioSettings already preserved the bad bytes as
    // studio_settings.json.bak; fall back to defaults and say so.
    studio_ = DefaultStudioSettings();
    loaded = false;
    SetLastError(QStringLiteral("Pengaturan studio rusak, kembali ke bawaan: %1")
                     .arg(QString::fromUtf8(e.what())));
  }
  try {
    config_ = LoadConfig(config_path_);
  } catch (const ConfigError& e) {
    config_ = WallpaperConfig{};
    AppendLog(QStringLiteral("Config engine tidak bisa dibaca: %1")
                  .arg(QString::fromUtf8(e.what())));
  }
  autostart_on_ = IsAutostart();
  // Clearing unconditionally wiped the message set a few lines above, so a
  // corrupt studio_settings.json reset the user to defaults in silence - the
  // QML banner had nothing left to show. Same guard, and for the same reason,
  // as LibraryGridModel::reload.
  if (loaded) {
    SetLastError(QString());
  }
  emit changed();
}

void SettingsBridge::apply() {
  QString error;
  // Independent writes: a config.json failure must not discard the preferences
  // that did save.
  try {
    ValidateStudioSettings(studio_);
    SaveStudioSettings(settings_path_, studio_);
    AppendLog(QStringLiteral("Pengaturan studio tersimpan"));
  } catch (const ConfigError& e) {
    error = QStringLiteral("Gagal menyimpan pengaturan studio: %1")
                .arg(QString::fromUtf8(e.what()));
  }
  try {
    SaveConfig(config_path_, config_);
    if (error.isEmpty()) {
      AppendLog(QStringLiteral("Pengaturan engine tersimpan"));
    }
  } catch (const ConfigError& e) {
    const QString msg = QStringLiteral("Gagal menyimpan konfigurasi:\n%1")
                            .arg(QString::fromUtf8(e.what()));
    error = error.isEmpty() ? msg : error + QLatin1String("\n") + msg;
  }
  if (error.isEmpty()) {
    SetLastError(QString());
  } else {
    SetLastError(error);
  }
}

bool SettingsBridge::engineRestartNeeded() const {
  return config_.cpu_affinity != "auto" || config_.gpu_adapter != "auto";
}

// --- studio settings setters ------------------------------------------------

void SettingsBridge::setAutoCompressOnImport(bool on) {
  studio_.auto_compress_on_import = on;
  emit changed();
}

void SettingsBridge::setCompressOutputDir(const QString& dir) {
  if (dir.isEmpty()) {
    return;  // validation rejects an empty dir; ignore rather than save junk
  }
  studio_.compress_output_dir = dir.toStdWString();
  emit changed();
}

void SettingsBridge::setDefaultCrf(int value) {
  if (value < 16 || value > 28) {
    return;
  }
  studio_.default_crf = value;
  emit changed();
}

void SettingsBridge::setDefaultFps(int value) {
  if (value < 1 || value > 30) {
    return;
  }
  studio_.default_fps = value;
  emit changed();
}

void SettingsBridge::setDefaultResolutionMode(const QString& mode) {
  static const QStringList kModes = {"match_monitor", "source", "720p", "1080p",
                                     "2160p"};
  if (!kModes.contains(mode)) {
    return;
  }
  studio_.default_resolution_mode = mode.toStdString();
  emit changed();
}

void SettingsBridge::setStartWithWindows(bool on) {
  std::string err;
  if (!SetAutostart(on, &err)) {
    SetLastError(QStringLiteral("Gagal menyimpan mulai otomatis: %1")
                     .arg(QString::fromStdString(err)));
  } else {
    AppendLog(QStringLiteral("Mulai otomatis %1")
                  .arg(on ? QStringLiteral("ON") : QStringLiteral("OFF")));
  }
  // Re-read rather than trusting the argument: on failure the Run value is
  // unchanged and the checkbox must snap back to reality.
  autostart_on_ = IsAutostart();
  emit changed();
}

void SettingsBridge::setCacheDir(const QString& dir) {
  if (dir.isEmpty()) {
    return;
  }
  studio_.cache_dir = dir.toStdWString();
  emit changed();
}

void SettingsBridge::setLockscreenSync(bool on) {
  studio_.lockscreen_sync = on;
  emit changed();
}

void SettingsBridge::setLockscreenOffsetSec(double seconds) {
  if (seconds < 0.0) {
    return;
  }
  studio_.lockscreen_offset_sec = seconds;
  emit changed();
}

void SettingsBridge::setCompressAdvancedVisible(bool on) {
  studio_.compress_advanced_visible = on;
  emit changed();
}

void SettingsBridge::setCheckUpdates(bool on) {
  studio_.check_updates = on;
  emit changed();
}

// --- UI language --------------------------------------------------------------

void SettingsBridge::setLanguage(const QString& code) {
  // Write-through (like setStartWithWindows): the translator is installed at
  // startup, so waiting for apply() would drop the user's choice.
  if (!SaveUiLanguage(code)) {
    // Unsupported code or an unwritable store: leave both the file and the
    // property alone so the picker snaps back to what is actually persisted.
    return;
  }
  // Re-read rather than echoing the argument: SaveUiLanguage normalises
  // (trimmed, lower-cased) and the property must show what is on disk.
  ui_language_ = LoadUiLanguage();
  emit changed();
}

// --- engine config setters --------------------------------------------------

void SettingsBridge::setFitMode(const QString& mode) {
  static const QStringList kModes = {"fill", "cover", "fit", "stretch", "center"};
  if (!kModes.contains(mode)) {
    return;
  }
  config_.fit_mode = mode.toStdString();
  emit changed();
}

void SettingsBridge::setMonitorId(int id) {
  if (id < -1) {
    return;
  }
  config_.monitor_id = id;
  emit changed();
}

void SettingsBridge::setBatterySaver(bool on) {
  config_.battery_saver = on;
  emit changed();
}

void SettingsBridge::setBatteryMode(const QString& mode) {
  if (mode != "cap24" && mode != "static") {
    return;
  }
  config_.battery_mode = mode.toStdString();
  emit changed();
}

void SettingsBridge::setCpuAffinity(const QString& mode) {
  if (mode != "auto" && mode != "all") {
    return;
  }
  config_.cpu_affinity = mode.toStdString();
  emit changed();
}

void SettingsBridge::setGpuAdapter(const QString& mode) {
  if (mode != "auto" && mode != "integrated" && mode != "discrete") {
    return;
  }
  config_.gpu_adapter = mode.toStdString();
  emit changed();
}

void SettingsBridge::setFpsCap(int value) {
  if (value < 1 || value > 30) {
    return;
  }
  config_.fps_cap = value;
  emit changed();
}

void SettingsBridge::setCrf(int value) {
  if (value < 16 || value > 28) {
    return;
  }
  config_.crf = value;
  emit changed();
}

void SettingsBridge::setSpeed(double value) {
  if (value < 0.5 || value > 2.0) {
    return;
  }
  config_.speed = value;
  emit changed();
}

void SettingsBridge::setResolutionW(int value) {
  if (value < 0) {
    return;
  }
  config_.resolution_w = value;
  emit changed();
}

void SettingsBridge::setResolutionH(int value) {
  if (value < 0) {
    return;
  }
  config_.resolution_h = value;
  emit changed();
}

// --- directories ------------------------------------------------------------

void SettingsBridge::pickCompressOutputDir() {
  const QString dir = QFileDialog::getExistingDirectory(
      nullptr, QStringLiteral("Pilih Folder Output"),
      QString::fromStdWString(studio_.compress_output_dir));
  if (!dir.isEmpty()) {
    setCompressOutputDir(dir);
  }
}

void SettingsBridge::pickCacheDir() {
  const QString dir = QFileDialog::getExistingDirectory(
      nullptr, QStringLiteral("Pilih Folder Cache"),
      QString::fromStdWString(studio_.cache_dir));
  if (dir.isEmpty()) {
    return;
  }
  // Choosing a folder IS the opt-in for sweeping it: clearCache() refuses any
  // folder outside %LOCALAPPDATA%\K6WP that does not carry the marker this
  // writes. Without it, a mistyped path in the field is one click away from
  // deleting itself.
  if (!MarkCacheDirOptedIn(std::filesystem::path(dir.toStdWString()))) {
    SetLastError(QStringLiteral("Folder cache tidak bisa ditandai, jadi tidak "
                                "akan bisa dibersihkan: %1")
                     .arg(dir));
  }
  setCacheDir(dir);
}

void SettingsBridge::openCacheDir() {
  const QString dir = cacheDir();
  if (!dir.isEmpty()) {
    QDesktopServices::openUrl(
        QUrl::fromLocalFile(QDir::toNativeSeparators(dir)));
  }
}

int SettingsBridge::clearCache() {
  const QString dir = cacheDir();
  if (dir.isEmpty()) {
    return 0;
  }
  const std::filesystem::path root(dir.toStdWString());
  std::error_code ec;
  if (!std::filesystem::exists(root, ec) || ec) {
    AppendLog(QStringLiteral("Cache tidak ada: %1").arg(dir));
    return 0;
  }
  // cache_dir is free text, so this is the only thing standing between one
  // click and the contents of, say, C:\Users\<me>\Videos. Refuse instead of
  // sweeping, and name the folder in the refusal: refusing silently reads as
  // "the cache was already empty", which is how the user finds out.
  QString refusal;
  if (!CacheDirIsClearable(root, settings_path_.parent_path(), &refusal)) {
    SetLastError(refusal);
    return 0;
  }
  int removed = 0;
  int failed = 0;
  // Recursive, regular files only: never follows a directory symlink out of the
  // cache, and never deletes the cache root itself.
  for (std::filesystem::recursive_directory_iterator it(root, ec), end;
       it != end && !ec; it.increment(ec)) {
    // The opt-in has to outlive the sweep it authorised, or the second clear
    // of the same folder would be refused.
    if (it->path().filename() == std::filesystem::path(kCacheOptInMarker)) {
      continue;
    }
    if (!it->is_regular_file(ec) || ec) {
      continue;
    }
    if (std::filesystem::remove(it->path(), ec) && !ec) {
      ++removed;
    } else {
      ++failed;
    }
  }
  if (failed > 0) {
    // Both channels: lastError is what the QML banner shows, the log line is
    // what a QA screenshot has. Logging "cache dibersihkan" after a partial
    // sweep is how a user learns their cache is only half-cleared.
    SetLastError(QStringLiteral("Sebagian cache gagal dihapus: %1").arg(failed));
    return removed;
  }
  // A clean sweep supersedes the refusal an earlier attempt reported, the same
  // way a successful apply() does - otherwise a corrected path still shows the
  // old "ditolak" banner over a sweep that worked.
  SetLastError(QString());
  AppendLog(QStringLiteral("Cache dibersihkan (%1 entri dihapus).").arg(removed));
  return removed;
}

// --- diagnostics ------------------------------------------------------------

void SettingsBridge::AppendLog(const QString& line) {
  log_.append(line);
  emit logChanged();
}

void SettingsBridge::SetLastError(const QString& error) {
  if (last_error_ == error) {
    return;
  }
  last_error_ = error;
  emit lastErrorChanged();
  if (!error.isEmpty()) {
    AppendLog(error);
  }
}

}  // namespace k6wp
