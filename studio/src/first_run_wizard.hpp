#pragma once

// First-run helpers (MED-5 part 1, trimmed): the interactive wizard was
// replaced by the QML dialog, so only the pure gate and the entry builder
// remain (both are exercised by studio_logic_test).

#include <QString>

#include <filesystem>

#include "ffprobe_helper.hpp"
#include "library_manager.hpp"

namespace k6wp {

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

}  // namespace k6wp
