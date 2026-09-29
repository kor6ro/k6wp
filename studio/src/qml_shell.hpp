#pragma once

// Phase 1 of the Widgets -> QML migration: the hybrid window.
//
// QmlShell is a QMainWindow whose central widget is a QQuickWidget, with the
// native PreviewWidget floating ON TOP of it as an unmanaged child of the
// window. It is deliberately still a Widgets window rather than a QQuickWindow:
//
//   * QQuickWidget hosts QQuickItems, so a top-level QML Window is not
//     involved at all (main.cpp deliberately does not use
//     QQmlApplicationEngine);
//   * PreviewWidget is a QWidget that hands its own winId() to mpv as the
//     `wid`, so it MUST own a real HWND. A prior spike proved the vendored
//     libmpv has no D3D11 render backend (mpv_render_context_create with
//     api="d3d11" returns rc=-19) and that the OpenGL path drops to
//     hwdec-current=no, which is exactly why the preview cannot become a
//     QQuickItem yet and is parented directly under the QMainWindow.
//
// The QML tree reserves the preview's rectangle with a placeholder Item and
// calls StudioBridge.syncPreviewGeometry() whenever it moves; the bridge
// forwards the call here, where the QML scene coordinates are converted into
// QMainWindow client coordinates and applied to the widget.

#include <QMainWindow>
#include <QString>

class QQuickWidget;
class QWidget;

namespace k6wp {

class LibraryGridModel;
class PreviewWidget;

class QmlShell final : public QMainWindow {
  Q_OBJECT

 public:
  explicit QmlShell(QWidget* parent = nullptr);
  ~QmlShell() override;

  // The Library singleton is constructed lazily by the QML engine, i.e. after
  // this shell exists, so drops route through a pointer the bridge sets later.
  void SetImportTarget(LibraryGridModel* model);

  // --- Called by StudioBridge (the QML-facing indirection) ------------------

  // x/y/w/h are QML SCENE coordinates (the root item's coordinate space, which
  // coincides with the QQuickWidget's content area). They are mapped into
  // this window's client coordinates and applied to the native preview. A
  // non-positive w/h hides the preview rather than creating a 0x0 HWND, so
  // tab switches that collapse the placeholder do not leave a stale surface.
  void syncPreviewGeometry(int x, int y, int w, int h);

  // PreviewWidget::LoadVideo / SetPaused passthroughs. QML never touches
  // PreviewWidget itself; it only calls the bridge, which forwards here.
  void loadPreview(const QString& path);
  void setPreviewPaused(bool paused);

 protected:
  void resizeEvent(QResizeEvent* event) override;
  void showEvent(QShowEvent* event) override;
  // Drops are handled on the WINDOW rather than a QML DropArea: these mirror
  // MainWindow's own handlers, which were the proven path for native file
  // drops into this UI.
  void dragEnterEvent(QDragEnterEvent* event) override;
  void dragMoveEvent(QDragMoveEvent* event) override;
  void dropEvent(QDropEvent* event) override;

 private:
  // QQuickWidget::statusChanged handler. On Error the QML engine's own
  // diagnostics (qmlErrors()) are written to stderr, so a broken module or a
  // typo in Main.qml is visible on the console instead of showing a blank
  // window. This is permanent code, not a temporary debug dump.
  void OnQuickStatusChanged(int status);

  // Central widget hosting the QQuickWidget.
  QWidget* host_ = nullptr;
  QQuickWidget* quick_ = nullptr;
  // The native mpv preview. Child of THIS window (not of the layout-managed
  // central widget) so it gets its own HWND; geometry is driven entirely by
  // syncPreviewGeometry().
  PreviewWidget* preview_ = nullptr;
  // Drop target, set by the Library model once the QML engine constructs it.
  LibraryGridModel* import_target_ = nullptr;
};

// --- Active-shell registry --------------------------------------------------
//
// StudioBridge is a QML_SINGLETON, so the QML engine constructs it lazily -
// that is, AFTER QmlShell exists and with no way to hand it the shell. The
// shell therefore publishes itself here in its constructor and clears the
// pointer in its destructor; the bridge reads ActiveQmlShell() when it needs
// to forward a preview invokable. Exactly one QmlShell exists per process
// (main.cpp's singleton mutex guarantees it), so a single pointer is enough,
// and every read happens on the GUI thread.
void SetActiveQmlShell(QmlShell* shell);
QmlShell* ActiveQmlShell();

}  // namespace k6wp
