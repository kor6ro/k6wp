#pragma once

// The Studio UI language, stored on its own at
// %LOCALAPPDATA%\K6WP\studio_ui.ini (a QSettings INI, one key).
//
// Deliberately NOT a field of shared/studio_settings.hpp's StudioSettings:
// StudioSettings is a versioned JSON schema (currently v3) where every added
// field is a migration, and the file is read by shared/lockscreen.cpp and the
// engine (lockscreen_sync) - neither of which should have to know which
// language the Studio shell speaks. One string does not justify a schema
// version bump, so the preference gets its own store in the same data dir.
// The dir is still derived from shared/: DefaultStudioSettingsPath()'s parent
// is %LOCALAPPDATA%\K6WP, so the LOCALAPPDATA -> USERPROFILE walk stays in one
// place instead of being reimplemented here.
//
// The source language is Indonesian: the strings in the C++/QML sources ARE
// the Indonesian translation, so "id" is the default, needs no .qm and needs
// no QTranslator. There is deliberately no studio_id.ts - translating a
// language into itself only adds a file that can drift from its source.

#include <QString>
#include <QStringList>

namespace k6wp {

// Stored values, not display names: the .qm is studio_<code>.qm.
inline constexpr const char kUiLanguageSource[] = "id";
inline constexpr const char kUiLanguageEnglish[] = "en";

// %LOCALAPPDATA%\K6WP\studio_ui.ini, or an empty string when neither
// LOCALAPPDATA nor USERPROFILE resolves (same failure DefaultStudioSettingsPath
// throws on).
QString UiLanguageStorePath();

// Always returns a supported code. A missing file, an unreadable one or a
// hand-edited/unknown value all yield the source language, silently: a stale
// preference must never cost the user a usable UI.
QString LoadUiLanguage();

// Persists `code` when it is supported and returns true. Returns false (and
// writes nothing) for an unknown code, so a bad QML binding cannot poison the
// store with a value the loader would reject.
bool SaveUiLanguage(const QString& code);

// The codes a picker may offer, source language first. A single-element list
// once only Indonesian exists, so the QML side can hide the control itself.
QStringList SupportedUiLanguages();

// "studio_<language>.qm", e.g. studio_en.qm. Must match the catalogue name the
// lrelease target builds in studio/CMakeLists.txt.
QString UiLanguageCatalogue(const QString& language);

}  // namespace k6wp
