// The Studio UI-language store. See ui_language.hpp for why this is an INI of
// its own and not a StudioSettings field.

#include "ui_language.hpp"

#include <QDir>
#include <QFileInfo>
#include <QSettings>

#include "config_schema.hpp"
#include "studio_settings.hpp"

namespace k6wp {
namespace {

// A slash in the key makes QSettings write it as an [ui] section header, so
// the file stays readable if Studio ever grows a second UI-only preference.
const QString kLanguageKey = QStringLiteral("ui/language");

QString SourceLanguage() {
  return QString::fromLatin1(kUiLanguageSource);
}

}  // namespace

QStringList SupportedUiLanguages() {
  // Source first: it is what every fallback resolves to.
  return {QString::fromLatin1(kUiLanguageSource),
          QString::fromLatin1(kUiLanguageEnglish)};
}

QString UiLanguageCatalogue(const QString& language) {
  return QStringLiteral("studio_") + language + QStringLiteral(".qm");
}

QString UiLanguageStorePath() {
  // DefaultStudioSettingsPath() owns the %LOCALAPPDATA%\K6WP resolution
  // (LOCALAPPDATA, else USERPROFILE\AppData\Local); its parent IS that data
  // dir, so the .ini lands beside studio_settings.json without a second copy
  // of the environment walk.
  try {
    return QString::fromStdWString(
        (DefaultStudioSettingsPath().parent_path() / L"studio_ui.ini")
            .wstring());
  } catch (const ConfigError&) {
    return QString();
  }
}

QString LoadUiLanguage() {
  const QString path = UiLanguageStorePath();
  if (path.isEmpty()) {
    return SourceLanguage();
  }
  // QSettings never throws: an absent file, a read-only one and a hand-edited
  // value all arrive as an empty or unknown QString, which the membership
  // check turns into the source language. Silent on purpose - a bad preference
  // is not worth an error dialog at startup.
  const QSettings store(path, QSettings::IniFormat);
  const QString code =
      store.value(kLanguageKey).toString().trimmed().toLower();
  if (!SupportedUiLanguages().contains(code)) {
    return SourceLanguage();
  }
  return code;
}

bool SaveUiLanguage(const QString& code) {
  const QString normalized = code.trimmed().toLower();
  if (!SupportedUiLanguages().contains(normalized)) {
    return false;
  }
  const QString path = UiLanguageStorePath();
  if (path.isEmpty()) {
    return false;
  }
  // The data dir normally exists (studio_settings.json lives there), but a
  // first run on a clean profile must not lose the choice to a missing parent.
  QDir().mkpath(QFileInfo(path).absolutePath());
  QSettings store(path, QSettings::IniFormat);
  store.setValue(kLanguageKey, normalized);
  store.sync();
  return store.status() == QSettings::NoError;
}

}  // namespace k6wp
