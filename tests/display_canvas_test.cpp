// display_canvas_test.cpp — plan row 20: offscreen QQmlComponent harness for
// studio/qml/DisplayCanvas.qml (the read-only proportional monitor canvas).
//
// Pattern note: this is the first QQmlComponent test in the repo, so it also
// establishes the harness. Two deliberate choices:
//
//  * QGuiApplication + QT_QPA_PLATFORM=offscreen (the CTest env property,
//    copied from studio_async_test): QtQuick items cannot instantiate without
//    a QPA platform, which is exactly why studio_logic_test (QCoreApplication,
//    documented at studio/CMakeLists.txt:237-239) is not the host for this.
//  * The QML file is loaded from SOURCE via K6WP_DISPLAY_CANVAS_QML (a compile
//    definition set in studio/CMakeLists.txt). The compiled copy lives inside
//    the studio executable's qrc, not in this test binary, so loading the
//    on-disk file is what keeps the test hermetic. Data enters through the
//    component's OWN properties (displaysModel / posterSource / posterPath) —
//    no Studio singleton, no IPC engine, plain JS fixtures.
//
// Covered here (row 20 acceptance):
//   happy: 2-monitor fixture, (0,0,1920,1080) primary + (1920,0,1080,1920)
//          portrait; union (0,0)-(3000,1920) into a 600x500 canvas must map
//          x through 1920/3000 (== 384 of the 600 width, within 1 px) with the
//          vertical centering offset (500 - 1920*0.2)/2 == 58; portrait rect
//          taller than wide; primary badge on the primary only; per-rect
//          label/resolution/orientation/scale/refresh readouts; assigned
//          filename fallback + posterPath thumbnail; one DropArea per rect;
//          no Drag.active anywhere in the source (Q2: read-only, not draggable).
//   failure: single-monitor fixture (union == the only rect) renders with no
//          divide-by-zero; zero-size and empty fixtures render no rects and
//          raise no QML error.

#include <QGuiApplication>
#include <QPointF>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQmlError>
#include <QQuickItem>
#include <QQuickStyle>
#include <QUrl>
#include <QVariantList>
#include <QVariantMap>

#include <cmath>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

#ifndef K6WP_DISPLAY_CANVAS_QML
#error "K6WP_DISPLAY_CANVAS_QML must be defined by studio/CMakeLists.txt (path to qml/DisplayCanvas.qml)"
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

int Finish() {
  std::printf("\n%d checks, %d failures\n", g_checks, g_failures);
  return g_failures == 0 ? 0 : 1;
}

bool Near(double a, double b, double tol) { return std::fabs(a - b) <= tol; }

// Recursive VISUAL tree walk. QObject::findChildren() is not enough here:
// Repeater delegates are visual children of the Repeater's parent item but are
// not in that item's QObject child chain, so the QML item tree (childItems())
// is the authoritative traversal for instantiated delegates.
void CollectItems(QQuickItem* parent, QList<QQuickItem*>* out) {
  if (parent == nullptr) {
    return;
  }
  const QList<QQuickItem*> kids = parent->childItems();
  for (QQuickItem* kid : kids) {
    out->append(kid);
    CollectItems(kid, out);
  }
}

QQuickItem* Child(QQuickItem* parent, const char* name) {
  QList<QQuickItem*> items;
  CollectItems(parent, &items);
  for (QQuickItem* item : items) {
    if (item->objectName() == QLatin1String(name)) {
      return item;
    }
  }
  return nullptr;
}

QList<QQuickItem*> Children(QQuickItem* parent, const char* name) {
  QList<QQuickItem*> items;
  CollectItems(parent, &items);
  QList<QQuickItem*> out;
  for (QQuickItem* item : items) {
    if (item->objectName() == QLatin1String(name)) {
      out.append(item);
    }
  }
  return out;
}

// The per-monitor Rectangle whose `monitorKey` property equals `key`.
QQuickItem* RectByKey(QQuickItem* canvas, const QString& key) {
  const QList<QQuickItem*> rects = Children(canvas, "monitorRect");
  for (QQuickItem* rect : rects) {
    if (rect->property("monitorKey").toString() == key) {
      return rect;
    }
  }
  return nullptr;
}

// The DECLARED visible flag (not effective visibility): no window is shown in
// this offscreen harness, so property("visible") is the honest assertion.
bool DeclaredVisible(QQuickItem* item) {
  return item != nullptr && item->property("visible").toBool();
}

QString TextOf(QQuickItem* item) {
  return item == nullptr ? QString() : item->property("text").toString();
}

// One BuildDisplayEntries-shaped map (studio_bridge.hpp:109).
QVariantMap Monitor(const QString& key, const QString& label, int x, int y,
                    int w, int h, bool primary, const QString& orientation,
                    int scalePercent, int refreshHz,
                    const QString& assignedPath = QString(),
                    bool assignedExists = false,
                    const QString& coverage = QStringLiteral("covered")) {
  QVariantMap m;
  m[QStringLiteral("key")] = key;
  m[QStringLiteral("label")] = label;
  m[QStringLiteral("x")] = x;
  m[QStringLiteral("y")] = y;
  m[QStringLiteral("width")] = w;
  m[QStringLiteral("height")] = h;
  m[QStringLiteral("isPrimary")] = primary;
  m[QStringLiteral("orientation")] = orientation;
  m[QStringLiteral("scalePercent")] = scalePercent;
  m[QStringLiteral("refreshHz")] = refreshHz;
  m[QStringLiteral("resolutionLabel")] =
      QStringLiteral("%1x%2").arg(w).arg(h);
  m[QStringLiteral("assignedPath")] = assignedPath;
  m[QStringLiteral("assignedExists")] = assignedExists;
  m[QStringLiteral("coverage")] = coverage;
  return m;
}

struct Canvas {
  QObject* object = nullptr;
  QQuickItem* item = nullptr;
  bool ok() const { return item != nullptr; }
};

// Instantiates DisplayCanvas with the component-property contract the row-21
// binding will use. Returns the root Item (nullptr on a hard failure).
Canvas MakeCanvas(QQmlEngine* engine, QQmlComponent* component,
                  const QVariantList& model, double w, double h,
                  const QString& posterSource = QString(),
                  const QString& posterPath = QString()) {
  Q_UNUSED(engine);
  QVariantMap props;
  props[QStringLiteral("displaysModel")] = model;
  props[QStringLiteral("width")] = w;
  props[QStringLiteral("height")] = h;
  if (!posterSource.isEmpty()) {
    props[QStringLiteral("posterSource")] = posterSource;
  }
  if (!posterPath.isEmpty()) {
    props[QStringLiteral("posterPath")] = posterPath;
  }
  Canvas canvas;
  canvas.object = component->createWithInitialProperties(props);
  canvas.item = qobject_cast<QQuickItem*>(canvas.object);
  return canvas;
}

}  // namespace

int main(int argc, char** argv) {
  QGuiApplication app(argc, argv);
  // Mirror studio/src/main.cpp:130 — the canvas consumes Material attached
  // tokens, so the harness must run the same style the app selects.
  QQuickStyle::setStyle(QStringLiteral("Material"));

  // --- source assertions --------------------------------------------------
  std::ifstream source_in(K6WP_DISPLAY_CANVAS_QML, std::ios::binary);
  const std::string source((std::istreambuf_iterator<char>(source_in)),
                           std::istreambuf_iterator<char>());
  Check(!source.empty(),
        "source: DisplayCanvas.qml is readable at K6WP_DISPLAY_CANVAS_QML");
  // Q2/Must-NOT-have: a read-only mirror of the Windows topology, so the rects
  // must never become re-arrangement drag sources. This is the executable
  // form of acceptance (iii).
  Check(!source.empty() && source.find("Drag.active") == std::string::npos,
        "source: no Drag.active (rects are not draggable)");

  // --- component load -----------------------------------------------------
  QQmlEngine engine;
  QList<QString> warnings;
  QObject::connect(&engine, &QQmlEngine::warnings,
                   [&warnings](const QList<QQmlError>& list) {
                     for (const QQmlError& e : list) {
                       warnings.append(e.toString());
                     }
                   });

  QQmlComponent component(&engine,
                         QUrl::fromLocalFile(QStringLiteral(
                             K6WP_DISPLAY_CANVAS_QML)));
  Check(component.status() == QQmlComponent::Ready,
        "component: DisplayCanvas compiles and is Ready");
  if (component.status() != QQmlComponent::Ready) {
    for (const QQmlError& e : component.errors()) {
      std::printf("  QML error: %s\n", qUtf8Printable(e.toString()));
    }
    return Finish();
  }

  auto dump_warnings = [&warnings](const char* fixture) {
    for (const QString& w : warnings) {
      std::printf("  QML warning (%s): %s\n", fixture, qUtf8Printable(w));
    }
  };

  auto dump_state = [](const char* fixture, Canvas* c) {
    const QVariantMap fit = c->item->property("fit").toMap();
    const QVariantMap uni = c->item->property("union").toMap();
    std::printf(
        "  state(%s): root=%.1fx%.1f model=%d valid=%d fit.valid=%d "
        "fit.scale=%.4f union.valid=%d union=(%.1f,%.1f %.1fx%.1f)\n",
        fixture, c->item->width(), c->item->height(),
        static_cast<int>(c->item->property("displaysModel").toList().size()),
        static_cast<int>(c->item->property("validDisplays").toList().size()),
        fit.value("valid").toBool() ? 1 : 0, fit.value("scale").toDouble(),
        uni.value("valid").toBool() ? 1 : 0, uni.value("x").toDouble(),
        uni.value("y").toDouble(), uni.value("width").toDouble(),
        uni.value("height").toDouble());
  };

  // === Fixture A: happy 2-monitor virtual desktop ==========================
  // Union (0,0)-(3000,1920); canvas 600x500 -> scale = min(0.2, 0.2604) = 0.2,
  // offsetX = 0, offsetY = (500 - 384)/2 = 58. DISPLAY2 starts at x=1920.
  const QVariantList two_monitors = {
      Monitor(QStringLiteral("\\\\.\\DISPLAY1"),
              QStringLiteral("Layar 1 (utama)"), 0, 0, 1920, 1080, true,
              QStringLiteral("landscape"), 100, 60),
      Monitor(QStringLiteral("\\\\.\\DISPLAY2"), QStringLiteral("Layar 2"),
              1920, 0, 1080, 1920, false, QStringLiteral("portrait"), 125, 60,
              QStringLiteral("C:\\Videos\\portrait.mp4"), true),
  };
  warnings.clear();
  Canvas a = MakeCanvas(&engine, &component, two_monitors, 600.0, 500.0);
  Check(a.ok(), "fixture A: DisplayCanvas instantiates with a 2-monitor model");
  if (!a.ok()) {
    dump_warnings("fixture A");
    return Finish();
  }
  Check(warnings.isEmpty(), "fixture A: no QML warnings on create");
  dump_warnings("fixture A");
  dump_state("fixture A", &a);

  const QList<QQuickItem*> rects = Children(a.item, "monitorRect");
  Check(rects.size() == 2,
        "fixture A: exactly 2 child monitor Rectangles are rendered");

  QQuickItem* r1 = RectByKey(a.item, QStringLiteral("\\\\.\\DISPLAY2"));
  QQuickItem* r0 = RectByKey(a.item, QStringLiteral("\\\\.\\DISPLAY1"));
  Check(r0 != nullptr && r1 != nullptr,
        "fixture A: both rects are addressable by monitorKey");

  if (r0 != nullptr && r1 != nullptr) {
    const QPointF p1 = r1->mapToItem(a.item, QPointF(0, 0));
    const QPointF p0 = r0->mapToItem(a.item, QPointF(0, 0));
    const double expected_x = 600.0 * 1920.0 / 3000.0;  // == 384.0
    std::printf(
        "QA-HAPPY aspect fit: DISPLAY2 origin=(%.2f, %.2f); expected x = "
        "600*1920/3000 = %.2f; expected y = (500-1920*0.2)/2 = 58.00\n",
        p1.x(), p1.y(), expected_x);
    Check(Near(p1.x(), expected_x, 1.0),
          "fixture A: (1920,0) in the (0,0)-(3000,1920) union maps to "
          "x == 600*1920/3000 within 1 px");
    Check(Near(p1.y(), 58.0, 1.0),
          "fixture A: vertical centering offset is (500-1920*0.2)/2 == 58");
    Check(Near(p0.x(), 0.0, 1.0) && Near(p0.y(), 58.0, 1.0),
          "fixture A: the (0,0) monitor keeps the union origin (offsetY 58)");
    Check(Near(r1->width(), 216.0, 1.0) && Near(r1->height(), 384.0, 1.0),
          "fixture A: portrait 1080x1920 scales to 216x384 (aspect preserved)");
    Check(Near(r0->width(), 384.0, 1.0) && Near(r0->height(), 216.0, 1.0),
          "fixture A: landscape 1920x1080 scales to 384x216");
    Check(r1->width() < r1->height(),
          "fixture A: portrait monitor rect is taller than wide");
    Check(r0->width() > r0->height(),
          "fixture A: landscape monitor rect is wider than tall");

    Check(DeclaredVisible(Child(r0, "primaryBadge")),
          "fixture A: primary badge visible on the primary monitor");
    Check(!DeclaredVisible(Child(r1, "primaryBadge")),
          "fixture A: primary badge hidden on the non-primary monitor");

    Check(TextOf(Child(r1, "monitorTitle")).contains(QStringLiteral("Layar 2")),
          "fixture A: title label carries the entry label");
    Check(TextOf(Child(r1, "monitorResolution")) ==
              QStringLiteral("1080x1920"),
          "fixture A: resolution label carries resolutionLabel");
    const QString info = TextOf(Child(r1, "monitorInfoLine"));
    Check(info.contains(QStringLiteral("portrait")) &&
              info.contains(QStringLiteral("125")) &&
              info.contains(QStringLiteral("60")),
          "fixture A: orientation / scalePercent / refreshHz readout present");

    QQuickItem* assigned1 = Child(r1, "assignedLabel");
    Check(DeclaredVisible(assigned1) &&
              TextOf(assigned1) == QStringLiteral("portrait.mp4"),
          "fixture A: assigned label shows the assigned filename");
    Check(!DeclaredVisible(Child(r0, "assignedLabel")),
          "fixture A: unassigned monitor shows no assigned label");
    QQuickItem* thumb1 = Child(r1, "assignedThumb");
    Check(thumb1 != nullptr && !DeclaredVisible(thumb1) &&
              thumb1->property("source").toString().isEmpty(),
          "fixture A: no posterPath -> thumbnail empty, filename fallback");
    Check(!DeclaredVisible(Child(r0, "degradedOverlay")) &&
              !DeclaredVisible(Child(r1, "degradedOverlay")),
          "fixture A: no degraded tint when assignments exist / are absent");
    Check(!DeclaredVisible(Child(r0, "coverageWarning")) &&
              !DeclaredVisible(Child(r1, "coverageWarning")),
          "fixture A: coverage == covered shows no warning");
  }
  Check(Children(a.item, "monitorDrop").size() == 2,
        "fixture A: one DropArea per monitor rect");

  // === Fixture A2: thumbnail contract (posterSource + posterPath) ==========
  warnings.clear();
  Canvas a2 = MakeCanvas(&engine, &component, two_monitors, 600.0, 500.0,
                         QStringLiteral("C:\\Videos\\portrait.mp4"),
                         QStringLiteral("file:///C:/thumbs/portrait.jpg"));
  QQuickItem* a2r1 = RectByKey(a2.item, QStringLiteral("\\\\.\\DISPLAY2"));
  QQuickItem* a2thumb = Child(a2r1, "assignedThumb");
  Check(a2thumb != nullptr && DeclaredVisible(a2thumb) &&
            a2thumb->property("source").toString().contains(
                QStringLiteral("portrait.jpg")),
        "fixture A2: posterPath cache entry becomes the assigned thumbnail");
  dump_warnings("fixture A2");

  // === Fixture B: single monitor (union == the only rect) ==================
  const QVariantList one_monitor = {
      Monitor(QStringLiteral("\\\\.\\DISPLAY1"),
              QStringLiteral("Layar 1 (utama)"), 0, 0, 1920, 1080, true,
              QStringLiteral("landscape"), 100, 60),
  };
  warnings.clear();
  Canvas b = MakeCanvas(&engine, &component, one_monitor, 480.0, 270.0);
  Check(b.ok(), "fixture B: single-monitor union instantiates");
  Check(warnings.isEmpty(),
        "fixture B: no divide-by-zero and no QML warning on a 1-element union");
  if (b.ok()) {
    dump_state("fixture B", &b);
    const QList<QQuickItem*> brects = Children(b.item, "monitorRect");
    Check(brects.size() == 1,
          "fixture B: single-monitor fixture renders exactly 1 rect");
    if (brects.size() == 1) {
      const QPointF bp = brects[0]->mapToItem(b.item, QPointF(0, 0));
      std::printf(
          "QA-FAIL(single) single-monitor rect: origin=(%.2f, %.2f) "
          "size=%.2fx%.2f (canvas 480x270, scale 0.25)\n",
          bp.x(), bp.y(), brects[0]->width(), brects[0]->height());
      Check(Near(bp.x(), 0.0, 0.5) && Near(bp.y(), 0.0, 0.5),
            "fixture B: single-monitor union maps to the canvas origin");
      Check(Near(brects[0]->width(), 480.0, 0.5) &&
                Near(brects[0]->height(), 270.0, 0.5),
            "fixture B: single-monitor rect fills the 0.25-scaled canvas");
    }
  }
  dump_warnings("fixture B");

  // === Fixture C: zero-size + empty fixtures ==============================
  QVariantList zero_size;
  zero_size.append(Monitor(QStringLiteral("\\\\.\\DISPLAY1"),
                           QStringLiteral("Layar 1"), 0, 0, 0, 0, true,
                           QStringLiteral("landscape"), 100, 60));
  warnings.clear();
  Canvas c0 = MakeCanvas(&engine, &component, zero_size, 400.0, 300.0);
  Check(c0.ok(), "fixture C: zero-size fixture instantiates");
  Check(warnings.isEmpty(),
        "fixture C: zero-size fixture raises no QML error/warning");
  Check(Children(c0.item, "monitorRect").isEmpty(),
        "fixture C: zero-size fixture renders no monitor rects");
  std::printf(
      "QA-FAIL(zero-size) zero-size fixture: %d monitor rects, %d QML "
      "warnings\n",
      static_cast<int>(Children(c0.item, "monitorRect").size()),
      static_cast<int>(warnings.size()));

  warnings.clear();
  Canvas c1 = MakeCanvas(&engine, &component, QVariantList(), 400.0, 300.0);
  Check(c1.ok() && Children(c1.item, "monitorRect").isEmpty(),
        "fixture C: empty model renders no rects and no error");
  dump_warnings("fixture C");

  // === Fixture D: degraded assignment + non-covered coverage ===============
  const QVariantList degraded = {
      Monitor(QStringLiteral("\\\\.\\DISPLAY1"),
              QStringLiteral("Layar 1 (utama)"), 0, 0, 1920, 1080, true,
              QStringLiteral("landscape"), 100, 60,
              QStringLiteral("C:\\Videos\\hilang.mp4"), false,
              QStringLiteral("headless")),
  };
  warnings.clear();
  Canvas d = MakeCanvas(&engine, &component, degraded, 400.0, 300.0);
  QQuickItem* dr = RectByKey(d.item, QStringLiteral("\\\\.\\DISPLAY1"));
  Check(DeclaredVisible(Child(dr, "degradedOverlay")),
        "fixture D: assignedExists == false shows the degraded tint");
  QQuickItem* cw = Child(dr, "coverageWarning");
  Check(DeclaredVisible(cw) &&
            TextOf(cw).contains(QStringLiteral("headless")),
        "fixture D: coverage != covered shows a visible warning naming the "
        "token");
  Check(TextOf(Child(dr, "assignedLabel")) ==
            QStringLiteral("hilang.mp4"),
        "fixture D: degraded assignment still shows its filename");
  dump_warnings("fixture D");

  return Finish();
}
