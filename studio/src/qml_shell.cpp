// Phase 1 of the Widgets -> QML migration: the hybrid window.
// See qml_shell.hpp for why this is still a QMainWindow and why the mpv
// preview is a native child widget rather than a QQuickItem.

#include "qml_shell.hpp"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QIcon>
#include <QMenu>
#include <QMimeData>
#include <QPoint>
#include <QQmlEngine>
#include <QQmlError>
#include <QQuickWidget>
#include <QRect>
#include <QResizeEvent>
#include <QShowEvent>
#include <QSize>
#include <QSystemTrayIcon>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>

#include "preview_widget.hpp"

#include "library_grid_model.hpp"
#include "settings_bridge.hpp"
#include "studio_bridge.hpp"
#include "video_paths.hpp"

namespace k6wp {

namespace {

// True when the drag carries at least one local video, so the cursor can show
// the copy affordance only for drops Studio will actually take.
bool DragHasVideo(const QMimeData* mime) {
  if (mime == nullptr || !mime->hasUrls()) {
    return false;
  }
  for (const QUrl& url : mime->urls()) {
    if (url.isLocalFile() && IsVideoPath(url.toLocalFile())) {
      return true;
    }
  }
  return false;
}

// QML module root object. studio/CMakeLists.txt declares
// `qt_add_qml_module(studio URI K6WP VERSION 1.0 RESOURCE_PREFIX "/qt/qml"
// QML_FILES qml/Main.qml)` and pins the source file's QT_RESOURCE_ALIAS to the
// bare name, so the file is compiled into the binary as exactly this path:
//
//   /qt/qml/K6WP/qmldir     (generated, declares "Main 1.0 Main.qml")
//   /qt/qml/K6WP/Main.qml
//
// The "qml/" source subdirectory is therefore NOT part of the resource path -
// without the alias the file would land at /qt/qml/K6WP/qml/Main.qml, outside
// the directory its own qmldir describes, and `import K6WP` would not resolve
// the type. The generated .qrc was checked against this constant; if the CMake
// declaration changes, this URL changes with it and OnQuickStatusChanged below
// reports the failure loudly instead of showing a blank window.
const QUrl& MainQmlUrl() {
  static const QUrl url(QStringLiteral("qrc:/qt/qml/K6WP/Main.qml"));
  return url;
}

// The process' one QmlShell; see SetActiveQmlShell/ActiveQmlShell.
QmlShell* g_active_shell = nullptr;

}  // namespace

QmlShell::QmlShell(QWidget* parent) : QMainWindow(parent) {
  setWindowTitle(QStringLiteral("K6WP Studio"));
  setWindowIcon(QIcon(QStringLiteral(":/icons/k6wp-on.ico")));
  setAcceptDrops(true);
  resize(960, 640);

  // The central widget is layout-managed; the QQuickWidget is its only child.
  // The preview is NOT in this layout (it is an unmanaged child of the window),
  // so nothing here can fight syncPreviewGeometry() over its rectangle.
  host_ = new QWidget(this);
  auto* layout = new QVBoxLayout(host_);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);

  quick_ = new QQuickWidget(host_);
  // Root object tracks the widget so Main.qml's `anchors.fill: parent` works.
  quick_->setResizeMode(QQuickWidget::SizeRootObjectToView);
  // A usable minimum keeps the tab bar and the placeholder from collapsing
  // when the window is restored small.
  quick_->setMinimumSize(480, 320);
  layout->addWidget(quick_);

  setCentralWidget(host_);

  // The native mpv preview, parented directly under the QMainWindow so
  // PreviewWidget::winId() gives mpv its own HWND. Created AFTER the central
  // widget so it starts on top; raise() keeps it there.
  preview_ = new PreviewWidget(this);
  preview_->hide();

  connect(quick_, &QQuickWidget::statusChanged, this,
          [this](QQuickWidget::Status status) {
            OnQuickStatusChanged(static_cast<int>(status));
          });
  // Runtime binding errors (ReferenceError on an undefined identifier, a bad
  // property, a type error) do NOT flip QQuickWidget::status to Error, so the
  // handler above never sees them and they reach the console only through the
  // engine's warning signal. Without this a silently-unbound property looks
  // exactly like a property that is legitimately empty.
  connect(quick_->engine(), &QQmlEngine::warnings, this,
          [](const QList<QQmlError>& warnings) {
            for (const QQmlError& w : warnings) {
              qCritical("QmlShell: QML warning: %s", qPrintable(w.toString()));
            }
          });
  OnQuickStatusChanged(static_cast<int>(quick_->status()));

  // setSource() returns void in Qt 6; a bad URL / broken module / QML error is
  // reported through statusChanged, which OnQuickStatusChanged turns into a
  // loud stderr dump instead of a silently blank window.
  quick_->setSource(MainQmlUrl());

  SetActiveQmlShell(this);

  // Plan todo 17: a stored closeToTray=true must take effect on this launch's
  // first close, so seed the tray from the process-global mirror (written by
  // SettingsBridge::reload during setSource above). The listener covers
  // runtime toggles from SettingsPage.
  SetCloseToTrayListener([this](bool on) { ApplyCloseToTrayPreference(on); });
  ApplyCloseToTrayPreference(CloseToTrayEnabled());
}

QmlShell::~QmlShell() {
  SetCloseToTrayListener(nullptr);
  ClearStudioTrayHooks();
  if (g_active_shell == this) {
    g_active_shell = nullptr;
  }
}

void QmlShell::OnQuickStatusChanged(int status) {
  const auto state = static_cast<QQuickWidget::Status>(status);
  if (state != QQuickWidget::Error) {
    return;
  }
  // Permanent diagnostic: a missing qml module, a wrong qrc path or a QML
  // error would otherwise present as a silently blank window.
  const QList<QQmlError> errors = quick_->errors();
  qCritical("QmlShell: QML load failed for %s (%d error(s))", qPrintable(MainQmlUrl().toString()),
            errors.size());
  for (const QQmlError& error : errors) {
    qCritical("QmlShell:   %s", qPrintable(error.toString()));
  }
}

void QmlShell::syncPreviewGeometry(int x, int y, int w, int h) {
  if (preview_ == nullptr) {
    return;
  }
  if (w <= 0 || h <= 0) {
    // The placeholder is collapsed (hidden tab): drop the native surface
    // instead of parking a 0x0 HWND on top of the page.
    preview_->hide();
    return;
  }
  // QML scene coordinates are the root item's space, which coincides with the
  // QQuickWidget's content area, so mapping the QQuickWidget's own widget
  // position into THIS window's client coordinates is the correct transform.
  // The window client area is exactly the coordinate space preview_ lives in
  // (it is a child of the QMainWindow, not of the layout-managed host_), and
  // mapTo accounts for the menu/status bars that host_ is offset by.
  const QPoint origin = quick_->mapTo(this, QPoint(x, y));
  preview_->setGeometry(QRect(origin, QSize(w, h)));
  if (!preview_->isVisible()) {
    preview_->show();
  }
  // Keep the native surface above the QQuickWidget's scene graph.
  preview_->raise();
}

void QmlShell::loadPreview(const QString& path) {
  if (preview_ == nullptr || path.isEmpty()) {
    return;
  }
  preview_->LoadVideo(path);
}

void QmlShell::setPreviewPaused(bool paused) {
  if (preview_ == nullptr) {
    return;
  }
  preview_->SetPaused(paused);
}

// --- Plan todo 17 / B10: optional close-to-tray lifecycle --------------------

void QmlShell::ApplyCloseToTrayPreference(bool on) {
  if (on) {
    EnsureTray();
    return;
  }
  if (tray_ != nullptr) {
    // OFF returns to the pre-todo-17 surface: no Studio tray icon at all.
    tray_->hide();
  }
  // A prior ON in this session flipped this to false; OFF must restore the
  // Qt default so a later close exits instead of hanging in a trayless hide.
  QApplication::setQuitOnLastWindowClosed(true);
}

void QmlShell::EnsureTray() {
  if (tray_ != nullptr || !QSystemTrayIcon::isSystemTrayAvailable()) {
    return;
  }
  tray_ = new QSystemTrayIcon(QIcon(QStringLiteral(":/icons/k6wp-on.ico")), this);
  tray_menu_ = new QMenu(this);
  // C-19 verbatim menu — the ONLY items. Do NOT copy the engine tray's
  // quick-switch MRU (engine_app.cpp); this tray is a controller, not a
  // playlist switcher.
  tray_open_ = tray_menu_->addAction(tr("Buka K6WP Studio"));
  tray_pause_ = tray_menu_->addAction(tr("Jeda"));
  tray_resume_ = tray_menu_->addAction(tr("Lanjut"));
  tray_menu_->addSeparator();
  tray_quit_ = tray_menu_->addAction(tr("Keluar"));
  tray_->setContextMenu(tray_menu_);
  tray_->setIcon(QIcon(QStringLiteral(":/icons/k6wp-on.ico")));

  connect(tray_open_, &QAction::triggered, this, &QmlShell::ShowFromTray);
  connect(tray_pause_, &QAction::triggered, this, [this]() {
    const StudioTrayHooks hooks = GetStudioTrayHooks();
    if (hooks.pause) {
      hooks.pause();
    }
  });
  connect(tray_resume_, &QAction::triggered, this, [this]() {
    const StudioTrayHooks hooks = GetStudioTrayHooks();
    if (hooks.resume) {
      hooks.resume();
    }
  });
  connect(tray_quit_, &QAction::triggered, this, &QmlShell::QuitFromTray);
  connect(tray_menu_, &QMenu::aboutToShow, this, &QmlShell::RefreshTrayTooltip);

  SetStudioTrayStatusListener([this]() { RefreshTrayTooltip(); });
  RefreshTrayTooltip();
  tray_->show();
}

void QmlShell::RefreshTrayTooltip() {
  if (tray_ == nullptr) {
    return;
  }
  // Friendly status only — glossary §5: never the word "engine".
  const StudioTrayHooks hooks = GetStudioTrayHooks();
  const QString status = hooks.status_tip ? hooks.status_tip() : QString();
  tray_->setToolTip(status.isEmpty()
                        ? QStringLiteral("K6WP Studio")
                        : QStringLiteral("K6WP Studio — %1").arg(status));
}

void QmlShell::ShowFromTray() {
  QApplication::setQuitOnLastWindowClosed(true);
  showNormal();
  raise();
  activateWindow();
}

void QmlShell::QuitFromTray() {
  // Explicit quit (C-19 "Keluar"). Release the singleton mutex first so a
  // second Studio launch is not blocked by this process's teardown window.
  ReleaseStudioMutex();
  qApp->quit();
}

void QmlShell::closeEvent(QCloseEvent* event) {
  if (!CloseToTrayEnabled()) {
    // OFF (default): pre-todo-17 behavior, byte-for-byte.
    QApplication::setQuitOnLastWindowClosed(true);
    QMainWindow::closeEvent(event);
    return;
  }
  // ON: hide to tray instead of exiting. The process stays alive; "Keluar"
  // in the tray menu (or a second launch's focus path) is how it ends.
  event->ignore();
  hide();
  QApplication::setQuitOnLastWindowClosed(false);
  EnsureTray();
  if (tray_ != nullptr) {
    tray_->show();
    RefreshTrayTooltip();
  }
}

void QmlShell::resizeEvent(QResizeEvent* event) {
  QMainWindow::resizeEvent(event);
  // A resize re-lays-out the central widget; re-assert the z-order so the
  // QQuickWidget never ends up painting over the native preview.
  if (preview_ != nullptr) {
    preview_->raise();
  }
}

void QmlShell::showEvent(QShowEvent* event) {
  QMainWindow::showEvent(event);
  if (preview_ != nullptr) {
    preview_->raise();
  }
}

void QmlShell::SetImportTarget(LibraryGridModel* model) {
  import_target_ = model;
}

void QmlShell::dragEnterEvent(QDragEnterEvent* event) {
  if (DragHasVideo(event->mimeData())) {
    event->acceptProposedAction();
  }
}

void QmlShell::dragMoveEvent(QDragMoveEvent* event) {
  if (DragHasVideo(event->mimeData())) {
    event->acceptProposedAction();
  }
}

void QmlShell::dropEvent(QDropEvent* event) {
  if (import_target_ == nullptr || !DragHasVideo(event->mimeData())) {
    return;
  }
  QStringList files;
  for (const QUrl& url : event->mimeData()->urls()) {
    if (url.isLocalFile()) {
      files.append(url.toLocalFile());
    }
  }
  if (files.isEmpty()) {
    return;
  }
  event->acceptProposedAction();
  import_target_->importPaths(files);
}

void SetActiveQmlShell(QmlShell* shell) { g_active_shell = shell; }

QmlShell* ActiveQmlShell() { return g_active_shell; }

}  // namespace k6wp
