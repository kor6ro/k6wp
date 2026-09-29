#pragma once

// First-run wizard (MED-5 part 1): extracted from main_window.{hpp,cpp} as a
// pure refactor — no behavior change. MainWindow keeps only the trigger
// (MaybeShowFirstRunWizard), the Finish-chain state (FirstRunPending), and
// the post-Finish pipeline (ApplyFirstRunResult/OnFirstRunProbed); everything
// about the wizard pages themselves lives here.
//
// Translation contexts are preserved byte-for-byte: page-internal tr() calls
// keep their FirstRunPickPage context (the class moved verbatim), and the
// dialog-builder strings keep the "MainWindow" context via Ftr() (they used
// to be MainWindow member calls). i18n scaffolding was removed per MED-11,
// so no .qm exists today — this just keeps a future revival identical.

#include <QString>

#include <filesystem>

#include <QWizardPage>

#include "ffprobe_helper.hpp"
#include "library_manager.hpp"

class QCheckBox;
class QLabel;
class QWidget;

namespace k6wp {

// Page 1 of the wizard: pick a video. Next stays disabled until a file is
// chosen (isComplete gate); no Q_OBJECT needed — only a base-signal emit.
class FirstRunPickPage final : public QWizardPage {
 public:
  explicit FirstRunPickPage(QWidget* parent = nullptr);
  bool isComplete() const override;
  QString chosen() const;

 private:
  QString chosen_;
  QLabel* chosen_label_ = nullptr;
};

// Outcome of an accepted wizard run (filled by RunFirstRunWizard).
struct FirstRunChoices {
  QString file;
  bool autostart = false;
  bool auto_compress = true;
};

// Pure first-run gate (was MainWindow::IsFirstRun): the settings file is
// written exactly once on wizard Finish, so its absence means "never ran
// setup". Library/current-video emptiness alone is not enough (a returning
// user may have an empty library).
bool IsFirstRunCondition(bool has_settings_file, bool lib_empty,
                         bool no_current);

// Pure LibraryEntry builder for the first-run probe continuation (was the
// entry-assembly block inside MainWindow::OnFirstRunProbed): metadata lands
// only when the async probe succeeded; the thumbnail lands only when the
// import produced one.
LibraryEntry BuildFirstRunEntry(const QString& file,
                                const QString& library_dst, bool probe_ok,
                                const VideoMetadata& meta,
                                const std::filesystem::path& thumb);

// Builds + execs the welcome -> pick -> options -> tip wizard modally.
// Returns true on Finish (choices filled), false on cancel (nothing was
// written — settings + import happen only on Finish, same as before).
bool RunFirstRunWizard(QWidget* parent, FirstRunChoices* out);

}  // namespace k6wp
