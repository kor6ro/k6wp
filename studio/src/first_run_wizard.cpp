// First-run helpers (MED-5 part 1, trimmed): the QWizard was replaced by the
// QML first-run dialog, so this TU keeps only the pure gate and the entry
// builder that the QML flow (and studio_logic_test) still use.

#include "first_run_wizard.hpp"

namespace k6wp {

bool IsFirstRunCondition(bool has_settings_file, bool lib_empty,
                         bool no_current) {
  if (has_settings_file) {
    return false;
  }
  return lib_empty && no_current;
}

LibraryEntry BuildFirstRunEntry(const QString& file,
                                const QString& library_dst, bool probe_ok,
                                const VideoMetadata& meta,
                                const std::filesystem::path& thumb) {
  LibraryEntry entry;
  entry.src = std::filesystem::path(file.toStdWString());
  entry.dst = std::filesystem::path(library_dst.toStdWString());
  if (probe_ok) {
    entry.duration = meta.duration;
    entry.codec = meta.codec;
    entry.width = meta.width;
    entry.height = meta.height;
    entry.fps = static_cast<int>(meta.fps + 0.5);
    if (meta.width > 0 && meta.height > 0) {
      entry.res =
          std::to_string(meta.width) + "x" + std::to_string(meta.height);
    }
  }
  if (!thumb.empty()) {
    entry.thumb = thumb;
  }
  return entry;
}

}  // namespace k6wp
