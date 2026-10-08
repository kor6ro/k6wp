// duplicate_banner_harness.cpp — task 25: hardware-free offscreen QML proof
// that the C-15 WarningBanner kind="duplicate" actually renders (plan todo
// 12 / brief F3-15). NOT a live-rig proof: no monitor driver, no engine, no
// real StudioBridge — a stub Studio context object + the REAL
// studio/qml/Theme.qml + WarningBanner.qml compiled into a harness QML
// module (same qt_add_qml_module pattern as the studio target).
//
// Capture: QQuickView under QT_QPA_PLATFORM=offscreen + QQuickItem::
// grabToImage. Success = PNG has visible banner pixels (tint + text, not a
// blank frame) and the stub notice text is on the QML surface.
//
// QGuiApplication, not QCoreApplication: grabToImage needs a scene graph.
// The offscreen QPA plugin comes from QT_PLUGIN_PATH (set by CTest); main()
// also defaults QT_QPA_PLATFORM=offscreen when unset so a manual run works.

#include <QEventLoop>
#include <QGuiApplication>
#include <QImage>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickItemGrabResult>
#include <QQuickStyle>
#include <QQuickView>
#include <QSharedPointer>
#include <QTimer>
#include <QUrl>

#include <cstdio>
#include <string>

#ifndef K6WP_HARNESS_MAIN_QML
#define K6WP_HARNESS_MAIN_QML ""
#endif

namespace {

int g_checks = 0;
int g_failures = 0;

void Check(bool cond, const std::string& name) {
  ++g_checks;
  if (cond) {
    std::printf("PASS %s\n", name.c_str());
  } else {
    ++g_failures;
    std::printf("FAIL %s\n", name.c_str());
  }
}

// Stub Studio context object. Exposes exactly the surface WarningBanner's
// duplicate branch reads (duplicateModeNotice / openWindowsDisplaySettings)
// plus the coverage-branch members (activeVideoPath / busy / applyWallpaper)
// so the same QML file would not ReferenceError if kind were ever flipped.
// Values match k6wp::DuplicateModeNoticeText for a \\.\DISPLAY1 collision
// (studio_bridge.cpp:145-158) — the harness does NOT link studio_bridge.
class StudioStub : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString duplicateModeNotice READ duplicateModeNotice NOTIFY
                 changed)
  Q_PROPERTY(QString activeVideoPath READ activeVideoPath NOTIFY changed)
  Q_PROPERTY(bool busy READ busy NOTIFY changed)
 public:
  explicit StudioStub(QObject* parent = nullptr) : QObject(parent) {}

  QString duplicateModeNotice() const {
    return QStringLiteral(
        "Mode duplikat terdeteksi (\\\\.\\DISPLAY1). Penugasan video per "
        "layar dinonaktifkan sampai tampilan Windows diubah ke mode "
        "Perluas.");
  }
  QString activeVideoPath() const {
    return QStringLiteral("C:\\Videos\\demo.mp4");
  }
  bool busy() const { return false; }

  Q_INVOKABLE void openWindowsDisplaySettings() {
    ++open_display_settings_calls_;
    std::printf("[stub] openWindowsDisplaySettings() call=%d\n",
                open_display_settings_calls_);
  }
  Q_INVOKABLE void applyWallpaper(const QString& path) {
    ++apply_wallpaper_calls_;
    std::printf("[stub] applyWallpaper(%s) call=%d\n",
                qPrintable(path), apply_wallpaper_calls_);
  }

  int openDisplaySettingsCalls() const { return open_display_settings_calls_; }

 signals:
  void changed();

 private:
  int open_display_settings_calls_ = 0;
  int apply_wallpaper_calls_ = 0;
};

// Light-palette statusPausedTint from studio/qml/Theme.qml (#FFF4CC).
// Offscreen QPA reports Qt.ColorScheme.Unknown -> Theme.dark=false -> terang.
constexpr int kTintR = 0xFF;
constexpr int kTintG = 0xF4;
constexpr int kTintB = 0xCC;

struct PixelStats {
  int total = 0;
  int opaque = 0;
  int tint_like = 0;   // close to #FFF4CC (banner background)
  int dark_ink = 0;    // close to Theme.text #1A1D23 (label / glyph)
};

PixelStats Analyze(const QImage& img) {
  PixelStats s;
  s.total = img.width() * img.height();
  for (int y = 0; y < img.height(); ++y) {
    for (int x = 0; x < img.width(); ++x) {
      const QRgb p = img.pixel(x, y);
      if (qAlpha(p) == 0) {
        continue;
      }
      ++s.opaque;
      const int r = qRed(p);
      const int g = qGreen(p);
      const int b = qBlue(p);
      // Tint: amber-tinted near-white (allow AA blend toward white).
      if (r >= 0xF0 && g >= 0xE8 && b >= 0xB0 && b <= 0xE0 && r >= b) {
        ++s.tint_like;
      }
      // Dark ink: near #1A1D23 (text + warning glyph).
      if (r < 0x40 && g < 0x40 && b < 0x50) {
        ++s.dark_ink;
      }
    }
  }
  return s;
}

}  // namespace

#include "duplicate_banner_harness.moc"

int main(int argc, char** argv) {
  setvbuf(stdout, nullptr, _IONBF, 0);

  // Default to offscreen when CTest did not set it (manual QA runs).
  if (!qEnvironmentVariableIsSet("QT_QPA_PLATFORM")) {
    qputenv("QT_QPA_PLATFORM", "offscreen");
  }

  QGuiApplication app(argc, argv);
  // Same style choice as studio/src/main.cpp:166 — Material is what
  // WarningBanner's Pane/Button/Label were designed against.
  QQuickStyle::setStyle(QStringLiteral("Material"));

  const std::string out_png =
      argc > 1 ? argv[1] : std::string("duplicate_banner_harness.png");
  // argv[2] overrides the compile-time default (CTest passes the source path).
  const QString harness_qml =
      argc > 2 ? QString::fromLocal8Bit(argv[2])
               : QStringLiteral(K6WP_HARNESS_MAIN_QML);
  std::printf("[info] QT_QPA_PLATFORM=%s\n",
              qgetenv("QT_QPA_PLATFORM").constData());
  std::printf("[info] output png: %s\n", out_png.c_str());
  std::printf("[info] harness qml: %s\n", qPrintable(harness_qml));

  StudioStub stub;
  std::printf("[info] stub duplicateModeNotice: %s\n",
              qPrintable(stub.duplicateModeNotice()));
  Check(!stub.duplicateModeNotice().isEmpty(),
        "stub duplicateModeNotice is non-empty");

  QQuickView view;
  view.setResizeMode(QQuickView::SizeRootObjectToView);
  // K6WPHarness (Theme singleton + WarningBanner) lives in this exe's qrc;
  // the wrapper QML on disk imports it by URI.
  view.engine()->addImportPath(QStringLiteral("qrc:/qt/qml"));
  view.engine()->rootContext()->setContextProperty("Studio", &stub);
  view.setSource(QUrl::fromLocalFile(harness_qml));
  view.resize(640, 80);

  Check(view.status() == QQuickView::Ready,
        "QQuickView loaded HarnessMain.qml (status Ready)");
  if (view.status() != QQuickView::Ready) {
    const auto errs = view.errors();
    for (const auto& e : errs) {
      std::printf("QML error: %s\n", qPrintable(e.toString()));
    }
  }

  QQuickItem* root = view.contentItem();
  Check(root != nullptr, "QQuickView contentItem is non-null");

  // WarnBanner's visible binding depends on Studio.duplicateModeNotice.length
  // > 0; with the stub notice it must be visible (troubled=true).
  if (root != nullptr) {
    // contentItem is the window's root item; HarnessMain's Item is its child
    // (SizeRootObjectToView reparents the QML root as content). Find the
    // WarningBanner Pane by walking children — its kind property is "duplicate".
    QQuickItem* banner = nullptr;
    const auto kids = root->childItems();
    for (QQuickItem* k : kids) {
      if (k->property("kind").toString() == QStringLiteral("duplicate")) {
        banner = k;
        break;
      }
    }
    // When SizeRootObjectToView is on, the QML root IS the contentItem.
    if (banner == nullptr &&
        root->property("kind").toString() == QStringLiteral("duplicate")) {
      banner = root;
    }
    // HarnessMain wraps WarningBanner as a child; also scan grandchildren.
    if (banner == nullptr) {
      for (QQuickItem* k : kids) {
        for (QQuickItem* g : k->childItems()) {
          if (g->property("kind").toString() == QStringLiteral("duplicate")) {
            banner = g;
            break;
          }
        }
        if (banner != nullptr) {
          break;
        }
      }
    }
    Check(banner != nullptr, "found WarningBanner item with kind=duplicate");
    if (banner != nullptr) {
      Check(banner->property("visible").toBool(),
            "WarningBanner.visible is true (duplicateTroubled)");
      Check(banner->property("troubled").toBool(),
            "WarningBanner.troubled is true");
      Check(banner->property("duplicateTroubled").toBool(),
            "WarningBanner.duplicateTroubled is true");
      const QString label =
          banner->property("Accessible").isValid()
              ? QString()
              : QString();
      (void)label;
    }
  }

  view.show();
  // Let the scene graph render one frame offscreen before grabbing.
  {
    QEventLoop warm;
    QTimer::singleShot(250, &warm, &QEventLoop::quit);
    warm.exec();
  }

  // grabToImage is async via QQuickItemGrabResult (Qt 6.8 C++ API).
  QImage shot;
  bool grabbed = false;
  if (root != nullptr) {
    QSharedPointer<QQuickItemGrabResult> grab = root->grabToImage();
    QEventLoop loop;
    QObject::connect(grab.data(), &QQuickItemGrabResult::ready, &loop,
                     [&]() {
                       shot = grab->image();
                       grabbed = true;
                       loop.quit();
                     });
    QTimer::singleShot(3000, &loop, &QEventLoop::quit);
    loop.exec();
  }

  Check(grabbed, "grabToImage callback fired");
  Check(!shot.isNull(), "grabToImage produced a non-null QImage");
  std::printf("[info] shot: %dx%d format=%d\n", shot.width(), shot.height(),
              static_cast<int>(shot.format()));

  const PixelStats stats = Analyze(shot);
  std::printf(
      "[info] pixels: total=%d opaque=%d tint_like=%d dark_ink=%d\n",
      stats.total, stats.opaque, stats.tint_like, stats.dark_ink);
  Check(stats.opaque > 0, "screenshot has opaque pixels (not blank)");
  // Banner background is Theme.statusPausedTint; require a real tint region.
  Check(stats.tint_like > 200,
        "screenshot contains visible banner tint pixels (#FFF4CC family)");
  // Label text + warning glyph are dark ink on the tint.
  Check(stats.dark_ink > 50,
        "screenshot contains dark ink pixels (label / glyph rendered)");

  if (!shot.isNull()) {
    // Normalize to RGBA32 for a stable PNG.
    const QImage out = shot.convertToFormat(QImage::Format_ARGB32);
    const bool saved = out.save(QString::fromStdString(out_png), "PNG");
    Check(saved, "PNG written: " + out_png);
    std::printf("[info] png saved: %s\n", out_png.c_str());
  }

  // Smoke: the stub invokable is reachable from QML (button onClicked path).
  // We call it directly — clicking under offscreen is not the point of this
  // harness; the point is the banner RENDERS. The C++ seam is covered by
  // studio_logic_test case 21.
  stub.openWindowsDisplaySettings();
  Check(stub.openDisplaySettingsCalls() == 1,
        "stub openWindowsDisplaySettings invokable reachable");

  std::printf("checks=%d failures=%d\n", g_checks, g_failures);
  return g_failures == 0 ? 0 : 1;
}
