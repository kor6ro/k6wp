#pragma once

// Phase 5 of the Widgets -> QML migration: the library grid's model.
//
// LibraryManager is a plain class whose Load/Add/Remove THROW LibraryError.
// QML cannot catch a C++ exception, so the throwing API is contained here: the
// model wraps every call, converts a failure into a status line plus a
// lastError property, and never lets an exception escape into the QML engine.
//
// The display label and the search filter are ported verbatim from
// LibraryWidget::EntryLabel / FilterMatches so the grid reads exactly like the
// Widgets one did, including the em-dashes and the [file hilang] /
// [belum dioptimasi] badges.

#include <QAbstractListModel>
#include <QFutureWatcher>
#include <QString>
#include <QStringList>
#include <QUrl>

#include <QtQml/qqmlregistration.h>

#include <vector>

#include "library_manager.hpp"
#include "thumbnailer.hpp"

namespace k6wp {

// NOT `final`: QML registration instantiates a QQmlElement<T> subclass.
class LibraryGridModel : public QAbstractListModel {
  // Registered as `Library`; QML_NAMED_ELEMENT is required because bare
  // QML_ELEMENT would register `LibraryGridModel`.
  Q_OBJECT
  QML_NAMED_ELEMENT(Library)
  QML_ELEMENT
  QML_SINGLETON

 public:
  enum Roles {
    kNameRole = Qt::UserRole + 1,
    kLabelRole,
    kThumbUrlRole,
    kResRole,
    kDstRole,
    kDurationRole,
    kCodecRole,
    kFpsRole,
    kSizeRole,
    kBrokenRole,
  };
  Q_ENUM(Roles)

  explicit LibraryGridModel(QObject* parent = nullptr);
  ~LibraryGridModel() override;

  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  QVariant data(const QModelIndex& index, int role) const override;
  QHash<int, QByteArray> roleNames() const override;

  // Live filter text; matches name OR the display label, case-insensitively.
  Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged)
  QString filter() const { return filter_; }
  void setFilter(const QString& text);

  Q_PROPERTY(int count READ count NOTIFY countChanged)
  Q_PROPERTY(int visibleCount READ visibleCount NOTIFY countChanged)
  // True when the library itself is empty, which is what the empty-state text
  // keys off (as opposed to "no search results").
  Q_PROPERTY(bool isEmpty READ isEmpty NOTIFY countChanged)
  Q_PROPERTY(bool hasFilter READ hasFilter NOTIFY filterChanged)
  // Deliberately NOT the whole first-run condition: this covers only the
  // settings-file and library arguments, and QML must AND it with
  // !Studio.videoActive (the gate's third argument) or the wizard would
  // reappear for users who already have a video applied.
  Q_PROPERTY(bool firstRunEligible READ firstRunEligible NOTIFY countChanged)
  // The most recent over-threshold pick, even though it was NOT imported (that
  // is the whole point of the compress-first offer). The wizard used to read
  // Library.dstAt(0), which is empty for exactly that case, so an over-threshold
  // first pick could never show a preview and the dialog was unfinishable.
  Q_PROPERTY(QString lastPickedPath READ lastPickedPath NOTIFY lastPickedPathChanged)
  Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
  Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
  Q_PROPERTY(QStringList log READ log NOTIFY logChanged)

  int count() const { return static_cast<int>(items_.size()); }
  int visibleCount() const;
  bool isEmpty() const { return items_.empty(); }
  bool hasFilter() const { return !filter_.isEmpty(); }
  bool firstRunEligible() const;
  QString lastPickedPath() const { return last_picked_path_; }
  QString lastError() const { return last_error_; }
  QString statusText() const { return status_text_; }
  QStringList log() const { return log_; }

  // --- Invokables ---------------------------------------------------------

  // Re-reads library.json and re-applies the filter.
  Q_INVOKABLE void reload();

  // Opens the native multi-select file dialog and imports what comes back.
  // QML has no multi-file dialog of its own, so the picker lives here for the
  // same reason pickVideo lives on StudioBridge. Returns how many were added.
  Q_INVOKABLE int pickAndImport();

  // In-place import (no copy): references each file where it already lives.
  // Non-video files are skipped and reported, matching the drop-import flow.
  Q_INVOKABLE int importPaths(const QStringList& paths);

  // Double-click / "Terapkan": publishes the row's dst so QML can call
  // Studio.applyWallpaper() itself.
  Q_INVOKABLE void applyAt(int row);
  Q_INVOKABLE QString dstAt(int row) const;

  Q_INVOKABLE void recompressAt(int row);
  Q_INVOKABLE void openLocationAt(int row);
  // Metadata-only removal; moveToTrash is a separate explicit choice.
  Q_INVOKABLE bool removeAt(int row, bool moveToTrash);

  // Generates the cached thumbnail for a row if it has none yet.
  Q_INVOKABLE void ensureThumbnail(int row);

  // Called by QML once the first-run wizard is on screen, so a later reopen (or
  // a restart before a pick lands) does not rearm the wizard.
  Q_INVOKABLE void clearLastPickedPath();

  // Called by the wizard's "Nanti saja": writes the default studio settings so
  // the first-run condition is decided by the settings file, not by an
  // in-memory flag that a restart would forget. Without this the wizard
  // reappeared forever and Finish then re-imported the file the user skipped.
  Q_INVOKABLE void markFirstRunHandled();

 signals:
  void filterChanged();
  void countChanged();
  void lastPickedPathChanged();
  void lastErrorChanged();
  void statusTextChanged();
  void logChanged();
  // QML listens and routes it to Studio.applyWallpaper(dst).
  void applyRequested(const QString& dst);
  // QML listens and routes it to Compress.setSourcePath(dst) + the Kompresor tab.
  void recompressRequested(const QString& dst);
  // A picked/dropped file was over the compress-first threshold, so it was not
  // imported. QML shows the "Video Besar" offer and compresses it instead.
  void compressFirstRequired(const QString& path);

 private:
  void AppendLog(const QString& line);
  void SetLastError(const QString& error);
  void SetStatusText(const QString& text);
  void ReapplyFilter();
  int RowForDst(const std::filesystem::path& dst) const;
  // Probe completion for one importPaths() batch item; runs on this object's
  // thread (ProbeVideoAsync is given `this` as the context object).
  void OnEntryProbed(const std::filesystem::path& dst, bool ok, int width,
                     int height, const QString& codec, double duration,
                     double fps);
  // Watcher completion for the ffmpeg thumbnail job; runs on this object's
  // thread because thumb_watch_ is a member of it.
  void OnThumbFinished();

  LibraryManager library_;
  std::vector<LibraryEntry> items_;
  // Row indices into items_ that survive the current filter, in order.
  std::vector<int> visible_;
  QString filter_;
  QString last_picked_path_;
  QString last_error_;
  QString status_text_;
  QStringList log_;
  // True while a thumbnail is being generated, so a scroll does not spawn the
  // same ffmpeg repeatedly.
  bool thumb_busy_ = false;
  // A member (not a raw pointer) so the watcher is parentless, lives and dies
  // with the model, and therefore can only be finished on the model's thread.
  QFutureWatcher<std::filesystem::path> thumb_watch_;
  // The dst handed to the running thumb job: the watcher only reports the
  // result path, which is the .jpg and cannot be matched against items_[].dst.
  std::filesystem::path thumb_dst_;
  // dst values with a probe still in flight, so importPaths() can report
  // "metadata pending" without racing a completion that already landed.
  std::vector<std::filesystem::path> pending_probes_;
};

}  // namespace k6wp
