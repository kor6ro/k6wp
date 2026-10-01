#include <QApplication>
#include <QIcon>
#include <QQuickStyle>
#include <QTranslator>

// windows.h for the Studio singleton mutex + focus-existing (Step 2.1).
// Kept in this .cpp only; no Win32 types leak into headers.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include "qml_shell.hpp"
#include "ui_language.hpp"

namespace {
// Step 2.1: Studio-owned singleton. MUST differ from the launcher's
// Local\K6WP-Studio-Singleton (held by the launcher while Studio lives, so
// reusing it would make a launcher-born Studio see ALREADY_EXISTS and exit).
constexpr wchar_t kStudioAppMutex[] = L"Local\\K6WP-Studio-App-Singleton";

// Minimal RAII guard: holds the mutex until process exit. Never closes an
// invalid handle; never copied.
class MutexGuard {
 public:
  explicit MutexGuard(HANDLE h) : handle_(h) {}
  MutexGuard(const MutexGuard&) = delete;
  MutexGuard& operator=(const MutexGuard&) = delete;
  ~MutexGuard() {
    if (handle_ != nullptr && handle_ != INVALID_HANDLE_VALUE) {
      CloseHandle(handle_);
    }
  }

 private:
  HANDLE handle_;
};

// Focus the existing "K6WP Studio" top-level window. Retries ~2s for the
// startup race (second instance arrives before the first window exists).
void FocusExistingStudio() {
  const ULONGLONG start = GetTickCount64();
  for (;;) {
    const HWND hwnd = FindWindowW(nullptr, L"K6WP Studio");
    if (hwnd != nullptr) {
      ShowWindow(hwnd, SW_RESTORE);
      SetForegroundWindow(hwnd);
      return;
    }
    if (GetTickCount64() - start >= 2000) {
      return;
    }
    Sleep(100);
  }
}
}  // namespace

int main(int argc, char* argv[]) {
  // DLL-planting hardening: per-user install dir is user-writable; restrict
  // DLL search to the application directory and System32 only. Ignore return
  // value explicitly.
  (void)SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_APPLICATION_DIR |
                                 LOAD_LIBRARY_SEARCH_SYSTEM32);
  // Singleton gate BEFORE any QApplication/window is created.
  HANDLE mutex = CreateMutexW(nullptr, FALSE, kStudioAppMutex);
  if (mutex == nullptr) {
    // No singleton guard possible; continue normally (same as launcher's
    // CreateMutexW-failure path: warn-free here, Studio still starts).
  } else if (GetLastError() == ERROR_ALREADY_EXISTS) {
    // Second instance: focus the first, exit fast without creating UI.
    CloseHandle(mutex);
    FocusExistingStudio();
    return 0;
  } else {
    // Own the mutex for the process lifetime; never close while in use.
    static MutexGuard guard(mutex);
    (void)guard;
  }

  // QApplication, NOT QGuiApplication: the Phase 1 shell hosts QML through a
  // QQuickWidget (a QWidget) and the mpv preview is a QWidget that hands its
  // winId() to libmpv, so both need the Widgets stack. A QQuickWindow-based
  // port is a later phase, once the preview can move to a QQuickItem.
  QApplication app(argc, argv);
  app.setApplicationName(QStringLiteral("K6WP Studio"));
  app.setOrganizationName(QStringLiteral("K6WP"));
  app.setWindowIcon(QIcon(":/icons/k6wp-on.ico"));

  // UI language. MUST happen before anything creates a window or loads QML:
  // the QmlShell constructor below calls setSource(), which instantiates the
  // translated bindings, and a QTranslator installed after that only reaches
  // bindings created later - so it would have to be a restart to see it.
  //
  // The source strings are Indonesian, so the default needs no catalogue at
  // all (that is why there is no studio_id.ts); only a non-source language
  // loads a .qm. The language comes from a free function rather than from
  // SettingsBridge so nothing has to construct the bridge (or the QML shell)
  // first, and a bad stored value falls back to Indonesian silently.
  const QString language = k6wp::LoadUiLanguage();
  QTranslator translator;
  bool translated = false;
  if (language != QString::fromLatin1(k6wp::kUiLanguageSource)) {
    // The .qm is copied next to studio.exe by the studio POST_BUILD step
    // (windeployqt's --no-translations only excludes Qt's OWN catalogues, and
    // is deliberately left alone so we do not ship several MB of them).
    // "i18n" is where our POST_BUILD copy lands; "translations" is the
    // conventional Qt layout, checked so a deployed/installer build finds the
    // same catalogue without a second copy step.
    const QString exe_dir = QCoreApplication::applicationDirPath();
    const QString catalogue = k6wp::UiLanguageCatalogue(language);
    for (const QString& sub_dir :
         {QStringLiteral("i18n"), QStringLiteral("translations")}) {
      translated = translator.load(exe_dir + QStringLiteral("/") + sub_dir +
                                   QStringLiteral("/") + catalogue);
      if (translated) {
        break;
      }
    }
  }
  if (translated) {
    app.installTranslator(&translator);
  }

  // Must precede the first QML load, which happens inside the QmlShell
  // constructor: a later setStyle leaves the already-instantiated controls on
  // the default style.
  QQuickStyle::setStyle(QStringLiteral("Material"));

  k6wp::QmlShell window;
  window.showMaximized();
  // Re-applied after show() so the icon lands on a real HWND. Do not verify
  // this with WM_GETICON: Qt intercepts it and answers from its own state, so
  // it reports 0 even when the icon is drawn correctly.
  window.setWindowIcon(QIcon(":/icons/k6wp-on.ico"));
  return app.exec();
}
