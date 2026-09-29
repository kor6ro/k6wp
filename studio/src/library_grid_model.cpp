// Phase 5 of the Widgets -> QML migration: the library grid's model.
// See library_grid_model.hpp for why the throwing LibraryManager API is
// contained here rather than exposed to QML.

#include "library_grid_model.hpp"

#include "compress_first_offer.hpp"
#include "first_run_wizard.hpp"
#include "probe_async.hpp"
#include "qml_shell.hpp"
#include "studio_settings.hpp"

#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QtConcurrent/QtConcurrentRun>

#include <algorithm>
#include <filesystem>

namespace k6wp {

namespace {

// Ported verbatim from LibraryWidget::FilterMatches.
std::string ToLowerAscii(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return s;
}

bool FilterMatches(const std::string& name, const std::string& label,
                   const std::string& query) {
  if (query.empty()) {
    return true;
  }
  const std::string q = ToLowerAscii(query);
  return ToLowerAscii(name).find(q) != std::string::npos ||
         ToLowerAscii(label).find(q) != std::string::npos;
}

// Ported verbatim from LibraryWidget::EntryLabel, so the grid shows exactly the
// text the Widgets grid did (same em-dashes, same badges).
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

bool IsVideoPath(const QString& path) {
  const QString lower = path.toLower();
  return lower.endsWith(QStringLiteral(".mp4")) ||
         lower.endsWith(QStringLiteral(".webm")) ||
         lower.endsWith(QStringLiteral(".avi")) ||
         lower.endsWith(QStringLiteral(".mkv")) ||
         lower.endsWith(QStringLiteral(".mov")) ||
         lower.endsWith(QStringLiteral(".wmv"));
}

QString FileUrl(const std::filesystem::path& p) {
  if (p.empty()) {
    return QString();
  }
  return QUrl::fromLocalFile(QDir::toNativeSeparators(
                                 QString::fromStdWString(p.wstring())))
      .toString();
}

}  // namespace

LibraryGridModel::LibraryGridModel(QObject* parent) : QAbstractListModel(parent) {
  if (QmlShell* shell = ActiveQmlShell()) {
    shell->SetImportTarget(this);
  }
  connect(&thumb_watch_, &QFutureWatcherBase::finished, this,
          [this]() { OnThumbFinished(); });
  reload();
}

LibraryGridModel::~LibraryGridModel() {
  // Disconnect before the future can be destroyed: the watcher is a member, so
  // its ~QFutureWatcher blocks on the running task, and a finished() delivered
  // into a half-destroyed model would be a use-after-free. Once disconnected,
  // GetThumb is simply abandoned and its result is thrown away.
  thumb_watch_.disconnect(this);
}

// --- model plumbing ---------------------------------------------------------

int LibraryGridModel::rowCount(const QModelIndex& parent) const {
  if (parent.isValid()) {
    return 0;
  }
  return static_cast<int>(visible_.size());
}

QHash<int, QByteArray> LibraryGridModel::roleNames() const {
  return {
      {kNameRole, "name"},
      {kLabelRole, "label"},
      {kThumbUrlRole, "thumbUrl"},
      {kResRole, "res"},
      {kDstRole, "dst"},
      {kDurationRole, "duration"},
      {kCodecRole, "codec"},
      {kFpsRole, "fps"},
      {kSizeRole, "sizeBytes"},
      {kBrokenRole, "broken"},
  };
}

QVariant LibraryGridModel::data(const QModelIndex& index, int role) const {
  const int row = index.row();
  if (row < 0 || row >= static_cast<int>(visible_.size())) {
    return QVariant();
  }
  const LibraryEntry& e = items_[static_cast<std::size_t>(visible_[static_cast<std::size_t>(row)])];
  switch (role) {
    case kNameRole:
      return QString::fromStdWString(e.dst.filename().wstring());
    case kLabelRole:
      return QString::fromStdString(EntryLabel(e));
    case kThumbUrlRole:
      return FileUrl(e.thumb);
    case kResRole:
      return (e.res.empty() || e.res == "0x0")
                 ? QString()
                 : QString::fromStdString(e.res);
    case kDstRole:
      return QString::fromStdWString(e.dst.wstring());
    case kDurationRole:
      return e.duration;
    case kCodecRole:
      return QString::fromStdString(e.codec);
    case kFpsRole:
      return e.fps;
    case kSizeRole:
      return static_cast<qulonglong>(e.size);
    case kBrokenRole:
      return e.broken;
    default:
      return QVariant();
  }
}

int LibraryGridModel::visibleCount() const {
  return static_cast<int>(visible_.size());
}

bool LibraryGridModel::firstRunEligible() const {
  std::error_code ec;
  const bool has_settings =
      std::filesystem::exists(DefaultStudioSettingsPath(), ec) && !ec;
  return IsFirstRunCondition(has_settings, items_.empty(), true);
}

void LibraryGridModel::setFilter(const QString& text) {
  if (filter_ == text) {
    return;
  }
  filter_ = text;
  emit filterChanged();
  ReapplyFilter();
}

void LibraryGridModel::ReapplyFilter() {
  const std::string q = filter_.toStdString();
  beginResetModel();
  visible_.clear();
  for (std::size_t i = 0; i < items_.size(); ++i) {
    const LibraryEntry& e = items_[i];
    const std::string name = e.dst.filename().u8string();
    if (FilterMatches(name, EntryLabel(e), q)) {
      visible_.push_back(static_cast<int>(i));
    }
  }
  endResetModel();
  emit countChanged();
}

int LibraryGridModel::RowForDst(const std::filesystem::path& dst) const {
  for (std::size_t i = 0; i < items_.size(); ++i) {
    if (items_[i].dst == dst) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

// --- loading ----------------------------------------------------------------

void LibraryGridModel::reload() {
  std::vector<LibraryEntry> items;
  bool loaded = true;
  try {
    // Reload from disk: mutators persist immediately, so this stays consistent
    // with imports landing anywhere in the process.
    library_.Load();
    items = library_.ListItems();
  } catch (const LibraryError& e) {
    // Corrupt/unreadable library.json: show the empty state and say why,
    // instead of letting the exception unwind into the QML engine.
    items.clear();
    loaded = false;
    SetLastError(QStringLiteral("Perpustakaan rusak: %1")
                     .arg(QString::fromUtf8(e.what())));
  }
  beginResetModel();
  items_ = std::move(items);
  endResetModel();
  ReapplyFilter();
  // Clearing unconditionally would also wipe an error the caller just set
  // (importPaths sets one before calling reload) and hide the failure above.
  if (loaded) {
    SetLastError(QString());
  }
  SetStatusText(items_.empty()
                    ? QStringLiteral("Perpustakaan kosong — impor video lewat "
                                     "Berkas > Impor atau seret & letakkan")
                    : QStringLiteral("Perpustakaan disegarkan (%1 entri)")
                          .arg(items_.size()));
}

int LibraryGridModel::pickAndImport() {
  const QStringList picked = QFileDialog::getOpenFileNames(
      nullptr, QStringLiteral("Impor Video"), QString(),
      QStringLiteral("Video (*.mp4 *.webm *.avi *.mkv *.mov *.wmv);"
                     "Semua File (*)"));
  if (picked.isEmpty()) {
    return 0;
  }
  return importPaths(picked);
}

int LibraryGridModel::importPaths(const QStringList& paths) {
  int added = 0;
  int skipped = 0;
  int offered = 0;
  QStringList failed;
  for (const QString& path : paths) {
    if (!IsVideoPath(path)) {
      ++skipped;
      continue;
    }
    // Compress-first offer, as MainWindow::MaybeOfferCompressFirst did on this
    // path: a large file is offered to the compressor instead of being
    // referenced as-is, and only the first one is offered so a multi-select
    // cannot stack dialogs.
    if (offered == 0 && CompressFirstOfferMb(path) > 0) {
      ++offered;
      // The pick still counts as the user's choice: the wizard needs a path it
      // can preview even though nothing was imported here, or an over-threshold
      // first pick leaves firstRunFile empty and the wizard unfinishable.
      last_picked_path_ = path;
      emit lastPickedPathChanged();
      emit compressFirstRequired(path);
      continue;
    }
    const std::filesystem::path src(path.toStdWString());
    try {
      // In-place reference: src == dst, nothing is copied or moved.
      // kNever because FfprobeHelper::Probe blocks up to 10s: probed inline it
      // froze the UI for 10s per picked file. The reference is persisted first
      // (the row appears at once) and ProbeVideoAsync fills the metadata in
      // OnEntryProbed.
      library_.Add(LibraryManager::ReferenceInPlace(src), kNever);
      pending_probes_.push_back(src);
      last_picked_path_ = path;
      emit lastPickedPathChanged();
      ++added;
    } catch (const std::exception& e) {
      // ReferenceInPlace throws when the source is missing or not a file.
      failed << QStringLiteral("%1 (%2)").arg(path, QString::fromUtf8(e.what()));
    }
  }
  if (!failed.isEmpty()) {
    AppendLog(QStringLiteral("Impor gagal untuk: %1").arg(failed.join("; ")));
    SetLastError(QStringLiteral("Impor gagal untuk %1 file").arg(failed.size()));
  }
  if (skipped > 0) {
    AppendLog(QStringLiteral("File non-video diabaikan: %1").arg(skipped));
  }
  reload();
  if (added > 0) {
    AppendLog(QStringLiteral("Diimpor %1 video").arg(added));
    SetStatusText(QStringLiteral("Diimpor %1 video").arg(added));
  }
  // Started after reload() so the rows are already on screen when the worker
  // finishes, and the placeholder is the normal "metadata pending" state.
  for (const std::filesystem::path& dst : pending_probes_) {
    ProbeVideoAsync(
        dst,
        [this, dst](bool ok, VideoMetadata meta) {
          OnEntryProbed(dst, ok, meta.width, meta.height,
                        QString::fromStdString(meta.codec), meta.duration,
                        meta.fps);
        },
        this);
  }
  pending_probes_.clear();
  return added;
}

void LibraryGridModel::OnEntryProbed(const std::filesystem::path& dst, bool ok,
                                     int width, int height,
                                     const QString& codec, double duration,
                                     double fps) {
  // Rows can be filtered, reloaded or deleted between the probe starting and
  // it landing, so the entry is matched by dst and never by a captured index.
  const int row = RowForDst(dst);
  if (!ok) {
    AppendLog(QStringLiteral("Metadata video tidak terbaca: %1")
                  .arg(QString::fromStdWString(dst.wstring())));
  } else if (row < 0) {
    return;
  } else {
    LibraryEntry& e = items_[static_cast<std::size_t>(row)];
    e.width = width;
    e.height = height;
    e.codec = codec.toStdString();
    e.duration = duration;
    if (fps > 0.0) {
      e.fps = static_cast<int>(fps + 0.5);
    }
    e.res = std::to_string(width) + "x" + std::to_string(height);
    try {
      // kNever: the metadata is already known, and a probe here would block the
      // GUI thread all over again.
      library_.Add(e, kNever);
    } catch (const LibraryError& err) {
      AppendLog(QStringLiteral("Library: gagal menyimpan metadata: %1")
                    .arg(QString::fromUtf8(err.what())));
    }
  }
  reload();
}

// --- row actions ------------------------------------------------------------

QString LibraryGridModel::dstAt(int row) const {
  if (row < 0 || row >= static_cast<int>(visible_.size())) {
    return QString();
  }
  const LibraryEntry& e =
      items_[static_cast<std::size_t>(visible_[static_cast<std::size_t>(row)])];
  return QString::fromStdWString(e.dst.wstring());
}

void LibraryGridModel::applyAt(int row) {
  const QString dst = dstAt(row);
  if (dst.isEmpty()) {
    return;
  }
  AppendLog(QStringLiteral("Dipilih: %1").arg(dst));
  emit applyRequested(dst);
}

void LibraryGridModel::recompressAt(int row) {
  const QString dst = dstAt(row);
  if (dst.isEmpty()) {
    return;
  }
  AppendLog(QStringLiteral("Kompres-ulang: %1").arg(dst));
  emit recompressRequested(dst);
}

void LibraryGridModel::openLocationAt(int row) {
  const QString dst = dstAt(row);
  if (dst.isEmpty()) {
    return;
  }
  // Explorer with the file selected, which is what "Buka Lokasi" did.
  QDesktopServices::openUrl(
      QUrl::fromLocalFile(QFileInfo(dst).absolutePath()));
}

bool LibraryGridModel::removeAt(int row, bool moveToTrash) {
  if (row < 0 || row >= static_cast<int>(visible_.size())) {
    return false;
  }
  const std::filesystem::path dst =
      items_[static_cast<std::size_t>(visible_[static_cast<std::size_t>(row)])].dst;
  std::string error;
  try {
    const bool removed = library_.Remove(dst, moveToTrash, &error);
    if (!removed) {
      return false;
    }
    if (!error.empty()) {
      // A trash failure is reported, never silently upgraded to a permanent
      // delete: the metadata is gone but the file is still on disk.
      AppendLog(QStringLiteral("Entri dipertahankan. %1")
                    .arg(QString::fromStdString(error)));
      SetLastError(QStringLiteral("Entri dipertahankan. %1")
                       .arg(QString::fromStdString(error)));
    }
    AppendLog(QStringLiteral("Library: removed %1")
                  .arg(QString::fromStdWString(dst.wstring())));
  } catch (const LibraryError& e) {
    SetLastError(QString::fromUtf8(e.what()));
    return false;
  }
  reload();
  return true;
}

void LibraryGridModel::ensureThumbnail(int row) {
  if (thumb_busy_ || row < 0 || row >= static_cast<int>(visible_.size())) {
    return;
  }
  const std::size_t idx = static_cast<std::size_t>(visible_[static_cast<std::size_t>(row)]);
  const LibraryEntry& e = items_[idx];
  if (!e.thumb.empty() && QFileInfo::exists(QString::fromStdWString(e.thumb.wstring()))) {
    return;
  }
  const std::filesystem::path dst = e.dst;
  thumb_busy_ = true;
  thumb_dst_ = dst;
  // GetThumb spawns ffmpeg and waits on it, so it runs on a worker thread. A
  // QTimer::singleShot deferral only moved the wait onto the GUI thread, which
  // is why scrolling to a fresh row used to freeze the window for up to 10s.
  // The Thumbnailer is a worker-local, so this lambda captures no `this` and a
  // destroyed model cannot be touched from the worker. The watcher's finished
  // handler (wired in the ctor) is the only path back, and it runs on the
  // model's thread.
  thumb_watch_.setFuture(QtConcurrent::run([dst]() {
    Thumbnailer worker;
    return worker.GetThumb(dst);
  }));
}

void LibraryGridModel::OnThumbFinished() {
  thumb_busy_ = false;
  const std::filesystem::path made = thumb_watch_.result();
  if (made.empty()) {
    return;  // silent: the delegate keeps its placeholder
  }
  // Matched by the submitted dst, not by an index captured before the worker
  // started: a reload or filter in between would otherwise patch the wrong row.
  const int row = RowForDst(thumb_dst_);
  if (row < 0) {
    return;
  }
  LibraryEntry& e = items_[static_cast<std::size_t>(row)];
  e.thumb = made;
  // reload() re-reads library.json, so an in-memory-only thumb is lost and the
  // cell repaints blank. Add() is a cheap upsert by dst here, not an import:
  // the entry already has its dimensions, so no probe runs.
  try {
    library_.Add(e, kNever);
  } catch (const LibraryError& err) {
    AppendLog(QStringLiteral("Library: gagal menyimpan thumbnail: %1")
                  .arg(QString::fromUtf8(err.what())));
  }
  reload();
}

// --- first run ---------------------------------------------------------------

void LibraryGridModel::clearLastPickedPath() {
  if (last_picked_path_.isEmpty()) {
    return;
  }
  last_picked_path_.clear();
  emit lastPickedPathChanged();
}

void LibraryGridModel::markFirstRunHandled() {
  // The settings file is the durable first-run marker: firstRunEligible() reads
  // its existence. Creating it is what makes "Nanti saja" stick across restarts;
  // an in-memory flag could not, which is why the wizard came back forever and
  // Finish then re-imported the file the user had declined.
  const std::filesystem::path path = DefaultStudioSettingsPath();
  std::error_code ec;
  if (std::filesystem::exists(path, ec) && !ec) {
    // Already marked: the gate is closed and the user may have real preferences
    // in that file, so it is never rewritten with defaults.
    clearLastPickedPath();
    return;
  }
  try {
    SaveStudioSettings(path, DefaultStudioSettings());
  } catch (const std::exception& e) {
    AppendLog(QStringLiteral("Pengaturan awal gagal disimpan: %1")
                  .arg(QString::fromUtf8(e.what())));
    return;
  }
  clearLastPickedPath();
}

// --- diagnostics ------------------------------------------------------------

void LibraryGridModel::AppendLog(const QString& line) {
  log_.append(line);
  emit logChanged();
}

void LibraryGridModel::SetLastError(const QString& error) {
  if (last_error_ == error) {
    return;
  }
  last_error_ = error;
  emit lastErrorChanged();
  if (!error.isEmpty()) {
    AppendLog(error);
  }
}

void LibraryGridModel::SetStatusText(const QString& text) {
  if (status_text_ == text) {
    return;
  }
  status_text_ = text;
  emit statusTextChanged();
}

}  // namespace k6wp
