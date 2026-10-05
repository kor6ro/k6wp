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
//
// Row 23 adds the drop-acceptance coverage:
//   * the gating the DropArea handlers delegate to (root.handleDrop): empty
//     path, displayCapability=false, duplicateModeNotice, busy, missing file;
//   * the per-rect dropActive highlight (root.setDropActive and the REAL
//     onExited signal on the DropArea);
//   * the signal contract row 21 binds (assignRequested / clearRequested) and
//     the B2 source assertions (getDataAsString, never the drop's text
//     property). The canvas still never touches the Studio singleton - the
//     test acts as the row-21 connection and spies the signal.
//
// Row 24 completes the visible assignment state (IS-3 feedback loop):
//   * assigned filename + cached thumbnail (fixture K seeds posterSource +
//     posterPath so thumbFor() yields a non-empty Image source);
//   * the 'ganti' replace affordance (re-arms the rect, mutates nothing) and
//     the 'hapus' clear affordance (emits clearRequested(key) exactly once -
//     the signal row 21 connects to Studio.clearMonitorAssignment);
//   * degraded states: assignedExists === false -> the red-ish
//     'file tidak ditemukan' label, coverage !== "covered" -> the
//     'cek engine.log' warning, both visible simultaneously (fixture M);
//   * empty assignedPath -> the neutral 'tarik video ke sini' drop hint.

#include <QColor>
#include <QGuiApplication>
#include <QJSValue>
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
// extraProps carries the row-23 gates and the injected existence probe, so a
// fixture never has to mutate the instance after creation.
Canvas MakeCanvas(QQmlEngine* engine, QQmlComponent* component,
                  const QVariantList& model, double w, double h,
                  const QString& posterSource = QString(),
                  const QString& posterPath = QString(),
                  const QVariantMap& extraProps = QVariantMap()) {
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
  for (auto it = extraProps.constBegin(); it != extraProps.constEnd(); ++it) {
    props[it.key()] = it.value();
  }
  Canvas canvas;
  canvas.object = component->createWithInitialProperties(props);
  canvas.item = qobject_cast<QQuickItem*>(canvas.object);
  return canvas;
}

// Row 21 owns the real connection (`onAssignRequested:
// Studio.assignVideoToMonitor`) in Main.qml, and the canvas is not allowed to
// reference the Studio singleton. The harness plays that role instead: a tiny
// inline QML object declares `Connections { target: canvas; function
// onAssignRequested(...) }` and exposes the recorded emission as count/key/path
// properties the C++ checks read back. (Qt 6.8 has no string-signal + functor
// connect overload and this target does not link QtTest.)
QObject* MakeAssignRecorder(QQmlEngine* engine, QObject* canvas) {
  static const char kRecorderQml[] = R"QML(
import QtQuick
Item {
    id: recorder
    property int count: 0
    property string key: ""
    property string path: ""
    property QtObject target: null
    Connections {
        target: recorder.target
        function onAssignRequested(k, p) {
            recorder.count = recorder.count + 1
            recorder.key = k
            recorder.path = p
        }
    }
}
)QML";
  QQmlComponent component(engine);
  component.setData(kRecorderQml,
                    QUrl(QStringLiteral("inline:/assign_recorder.qml")));
  if (component.status() != QQmlComponent::Ready) {
    for (const QQmlError& e : component.errors()) {
      std::printf("  recorder QML error: %s\n", qUtf8Printable(e.toString()));
    }
    return nullptr;
  }
  QVariantMap props;
  props[QStringLiteral("target")] = QVariant::fromValue(canvas);
  QObject* recorder = component.createWithInitialProperties(props);
  if (recorder != nullptr && canvas != nullptr) {
    recorder->setParent(canvas);
  }
  return recorder;
}

struct AssignSpy {
  AssignSpy(QQmlEngine* engine, QObject* canvas)
      : object(MakeAssignRecorder(engine, canvas)) {}
  int Count() const {
    return object != nullptr ? object->property("count").toInt() : 0;
  }
  QString Key() const {
    return object != nullptr ? object->property("key").toString() : QString();
  }
  QString Path() const {
    return object != nullptr ? object->property("path").toString() : QString();
  }
  QObject* object = nullptr;
};

// Row 24: the same inline-Connections pattern for clearRequested(key), the
// signal the 'hapus' button emits (row 21 connects it to
// Studio.clearMonitorAssignment).
QObject* MakeClearRecorder(QQmlEngine* engine, QObject* canvas) {
  static const char kRecorderQml[] = R"QML(
import QtQuick
Item {
    id: recorder
    property int count: 0
    property string key: ""
    property QtObject target: null
    Connections {
        target: recorder.target
        function onClearRequested(k) {
            recorder.count = recorder.count + 1
            recorder.key = k
        }
    }
}
)QML";
  QQmlComponent component(engine);
  component.setData(kRecorderQml,
                    QUrl(QStringLiteral("inline:/clear_recorder.qml")));
  if (component.status() != QQmlComponent::Ready) {
    for (const QQmlError& e : component.errors()) {
      std::printf("  recorder QML error: %s\n", qUtf8Printable(e.toString()));
    }
    return nullptr;
  }
  QVariantMap props;
  props[QStringLiteral("target")] = QVariant::fromValue(canvas);
  QObject* recorder = component.createWithInitialProperties(props);
  if (recorder != nullptr && canvas != nullptr) {
    recorder->setParent(canvas);
  }
  return recorder;
}

struct ClearSpy {
  ClearSpy(QQmlEngine* engine, QObject* canvas)
      : object(MakeClearRecorder(engine, canvas)) {}
  int Count() const {
    return object != nullptr ? object->property("count").toInt() : 0;
  }
  QString Key() const {
    return object != nullptr ? object->property("key").toString() : QString();
  }
  QObject* object = nullptr;
};

// QML functions with untyped parameters are registered as QVariant arguments,
// which is how the DropArea handlers (and the test) call into the root.
bool CallQml(QObject* obj, const char* method, const QVariant& a1,
             const QVariant& a2) {
  return QMetaObject::invokeMethod(obj, method, Q_ARG(QVariant, a1),
                                   Q_ARG(QVariant, a2));
}

QString RefusalOf(QQuickItem* canvas) {
  return TextOf(Child(canvas, "refusalLabel"));
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
  // Row 23 / B2: the drop path must come from the custom MIME via
  // DragEvent.getDataAsString. The row-22 drag sources set no text/plain, so
  // the drop's text property would resolve to an empty string - a handler
  // using it would fail these assertions and the engine would reject every
  // assignment.
  Check(!source.empty() &&
            source.find("getDataAsString(\"application/x-k6wp-assignment\")") !=
                std::string::npos,
        "source: onDropped reads the assignment via "
        "getDataAsString(\"application/x-k6wp-assignment\") (B2)");
  Check(!source.empty() && source.find("drop.text") == std::string::npos,
        "source: never uses the drop's text property as the assignment "
        "source (B2)");
  Check(!source.empty() &&
            source.find("keys: [\"application/x-k6wp-assignment\"]") !=
                std::string::npos,
        "source: DropArea keys filter accepts only the assignment MIME");
  Check(!source.empty() && source.find("onEntered") != std::string::npos &&
            source.find("onExited") != std::string::npos &&
            source.find("onDropped") != std::string::npos,
        "source: onEntered / onExited / onDropped handlers are wired");
  Check(!source.empty() &&
            source.find("signal assignRequested(string key, string path)") !=
                std::string::npos,
        "source: assignRequested(key, path) signal declared for row 21");
  Check(!source.empty() &&
            source.find("signal clearRequested(string key)") !=
                std::string::npos,
        "source: clearRequested(key) signal declared for row 21/24");
  // Row 24 state-surface wording, all qsTr-wrapped for row 25's catalogue:
  // the degraded label matches the playlist exists wording and the coverage
  // warning keeps the studio_bridge "cek engine.log" hint style.
  Check(!source.empty() &&
            source.find("qsTr(\"file tidak ditemukan\")") !=
                std::string::npos,
        "source: degraded label 'file tidak ditemukan' is qsTr-wrapped");
  Check(!source.empty() &&
            source.find("qsTr(\"tarik video ke sini\")") !=
                std::string::npos,
        "source: unassigned drop hint 'tarik video ke sini' is qsTr-wrapped");
  Check(!source.empty() &&
            source.find("qsTr(\"Ganti\")") != std::string::npos &&
            source.find("qsTr(\"Hapus\")") != std::string::npos,
        "source: 'Ganti'/'Hapus' affordance labels are qsTr-wrapped");

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

  // === Fixture E: displayCapability == false refuses every drop ============
  const QString display1 = QStringLiteral("\\\\.\\DISPLAY1");
  const QString valid_path = QStringLiteral("C:\\Videos\\valid.mp4");

  QVariantMap capability_off;
  capability_off[QStringLiteral("displayCapability")] = false;
  warnings.clear();
  Canvas e = MakeCanvas(&engine, &component, one_monitor, 400.0, 300.0,
                        QString(), QString(), capability_off);
  Check(e.ok(), "fixture E: canvas instantiates with displayCapability=false");
  if (!e.ok()) {
    dump_warnings("fixture E");
    return Finish();
  }
  AssignSpy e_spy(&engine, e.object);
  QQuickItem* e_rect = RectByKey(e.item, display1);
  Check(CallQml(e.item, "setDropActive", display1, true) &&
            e_rect != nullptr &&
            e_rect->property("dropActive").toBool(),
        "fixture E: setDropActive raises the per-rect highlight (the "
        "onEntered path)");
  CallQml(e.item, "handleDrop", display1, valid_path);
  std::printf("QA-FAIL(capability) emissions=%d refusal=\"%s\"\n", e_spy.Count(),
              qUtf8Printable(RefusalOf(e.item)));
  Check(e_spy.Count() == 0,
        "fixture E: displayCapability=false -> zero assignRequested emissions");
  Check(DeclaredVisible(Child(e.item, "refusalLabel")) &&
            RefusalOf(e.item).contains(QStringLiteral("Engine lama")),
        "fixture E: refusal label visible with the engine-age message");
  Check(e.item->property("refusalMessage")
            .toString()
            .contains(QStringLiteral("Engine lama")),
        "fixture E: refusalMessage root property carries the refused message");
  Check(e_rect != nullptr && !e_rect->property("dropActive").toBool(),
        "fixture E: the refused drop clears the highlight");

  // The real onExited signal on the DropArea must clear the highlight too:
  // invoke it through the meta-object exactly as Qt does when the drag
  // leaves the rect.
  QQuickItem* e_drop =
      e_rect != nullptr ? Child(e_rect, "monitorDrop") : nullptr;
  Check(e_drop != nullptr, "fixture E: monitorDrop DropArea is addressable");
  if (e_drop != nullptr) {
    CallQml(e.item, "setDropActive", display1, true);
    QMetaObject::invokeMethod(e_drop, "exited");
    Check(!e_rect->property("dropActive").toBool(),
          "fixture E: the real onExited handler clears the highlight");
  }
  dump_warnings("fixture E");

  // === Fixture F: an empty drop path refuses with a message ================
  warnings.clear();
  Canvas f = MakeCanvas(&engine, &component, one_monitor, 400.0, 300.0);
  AssignSpy f_spy(&engine, f.object);
  CallQml(f.item, "handleDrop", display1, QString());
  std::printf("QA-FAIL(empty) emissions=%d refusal=\"%s\"\n", f_spy.Count(),
              qUtf8Printable(RefusalOf(f.item)));
  Check(f_spy.Count() == 0,
        "fixture F: empty path -> zero assignRequested emissions");
  Check(DeclaredVisible(Child(f.item, "refusalLabel")) &&
            RefusalOf(f.item).contains(QStringLiteral("tidak dikenali")),
        "fixture F: empty path shows a visible refusal message");

  // === Fixture G: duplicateModeNotice refuses and echoes the notice =======
  const QString duplicate_notice = QStringLiteral(
      "Mode duplikat terdeteksi (\\\\.\\DISPLAY1). Penugasan video per layar "
      "dinonaktifkan sampai tampilan Windows diubah ke mode Perluas.");
  QVariantMap duplicate;
  duplicate[QStringLiteral("duplicateModeNotice")] = duplicate_notice;
  warnings.clear();
  Canvas g = MakeCanvas(&engine, &component, one_monitor, 400.0, 300.0,
                        QString(), QString(), duplicate);
  AssignSpy g_spy(&engine, g.object);
  CallQml(g.item, "handleDrop", display1, valid_path);
  std::printf("QA-FAIL(duplicate) emissions=%d refusal=\"%s\"\n", g_spy.Count(),
              qUtf8Printable(RefusalOf(g.item)));
  Check(g_spy.Count() == 0,
        "fixture G: duplicateModeNotice -> zero assignRequested emissions");
  Check(DeclaredVisible(Child(g.item, "refusalLabel")) &&
            RefusalOf(g.item) == duplicate_notice,
        "fixture G: the refusal label echoes the duplicate-mode notice");

  // === Fixture H: busy refuses with a brief message ========================
  QVariantMap busy_flag;
  busy_flag[QStringLiteral("busy")] = true;
  warnings.clear();
  Canvas h = MakeCanvas(&engine, &component, one_monitor, 400.0, 300.0,
                        QString(), QString(), busy_flag);
  AssignSpy h_spy(&engine, h.object);
  CallQml(h.item, "handleDrop", display1, valid_path);
  std::printf("QA-FAIL(busy) emissions=%d refusal=\"%s\"\n", h_spy.Count(),
              qUtf8Printable(RefusalOf(h.item)));
  Check(h_spy.Count() == 0,
        "fixture H: busy -> zero assignRequested emissions");
  Check(DeclaredVisible(Child(h.item, "refusalLabel")) &&
            RefusalOf(h.item).contains(QStringLiteral("sibuk")),
        "fixture H: busy shows a brief visible refusal message");

  // === Fixture I: injected existence probe gates a missing file ===========
  QVariantMap missing_probe;
  missing_probe[QStringLiteral("fileExistsProbe")] = QVariant::fromValue(
      engine.evaluate(QStringLiteral("(function(path) { return false })")));
  warnings.clear();
  Canvas i = MakeCanvas(&engine, &component, one_monitor, 400.0, 300.0,
                        QString(), QString(), missing_probe);
  AssignSpy i_spy(&engine, i.object);
  CallQml(i.item, "handleDrop", display1,
          QStringLiteral("C:\\Videos\\sudah-dihapus.mp4"));
  std::printf("QA-FAIL(missing) emissions=%d refusal=\"%s\"\n", i_spy.Count(),
              qUtf8Printable(RefusalOf(i.item)));
  Check(i_spy.Count() == 0,
        "fixture I: probe reports the dropped file missing -> zero "
        "assignRequested emissions");
  Check(DeclaredVisible(Child(i.item, "refusalLabel")) &&
            RefusalOf(i.item).contains(QStringLiteral("tidak ditemukan")),
        "fixture I: missing file shows a visible refusal message");
  QVariantMap present_probe;
  present_probe[QStringLiteral("fileExistsProbe")] = QVariant::fromValue(
      engine.evaluate(QStringLiteral("(function(path) { return true })")));
  warnings.clear();
  Canvas i2 = MakeCanvas(&engine, &component, one_monitor, 400.0, 300.0,
                         QString(), QString(), present_probe);
  AssignSpy i2_spy(&engine, i2.object);
  CallQml(i2.item, "handleDrop", display1, valid_path);
  Check(i2_spy.Count() == 1,
        "fixture I: probe reports the file present -> the drop is accepted");
  dump_warnings("fixture I");

  // === Fixture J: happy path emits assignRequested exactly once ===========
  warnings.clear();
  Canvas j = MakeCanvas(&engine, &component, one_monitor, 400.0, 300.0);
  Check(j.ok(), "fixture J: happy canvas instantiates");
  Check(j.item->metaObject()->indexOfSignal(
            "assignRequested(QString,QString)") >= 0,
        "fixture J: assignRequested(QString,QString) signal exists (row 21 "
        "binds it)");
  Check(j.item->metaObject()->indexOfSignal("clearRequested(QString)") >= 0,
        "fixture J: clearRequested(QString) signal exists (row 21/24)");
  AssignSpy j_spy(&engine, j.object);
  QQuickItem* j_rect = RectByKey(j.item, display1);
  const bool raised = CallQml(j.item, "setDropActive", display1, true);
  CallQml(j.item, "handleDrop", display1, valid_path);
  std::printf(
      "QA-HAPPY(drop) setDropActive=%d emissions=%d key=\"%s\" path=\"%s\" "
      "highlight=%d message=\"%s\"\n",
      raised ? 1 : 0, j_spy.Count(), qUtf8Printable(j_spy.Key()),
      qUtf8Printable(j_spy.Path()),
      j_rect != nullptr && j_rect->property("dropActive").toBool() ? 1 : 0,
      qUtf8Printable(RefusalOf(j.item)));
  Check(raised && j_rect != nullptr &&
            j_rect->property("dropActive").toBool() == false,
        "fixture J: the accepted drop clears the raised highlight");
  Check(j_spy.Count() == 1 && j_spy.Key() == display1 &&
            j_spy.Path() == valid_path,
        "fixture J: exactly one assignRequested(key, path) with the dropped "
        "path");
  Check(!DeclaredVisible(Child(j.item, "refusalLabel")) &&
            j.item->property("refusalMessage").toString().isEmpty(),
        "fixture J: an accepted drop shows no refusal");
  dump_warnings("fixture J");

  // === Fixture K: assigned + existing -> filename, thumbnail, affordances ==
  // The thumbnail rule is thumbFor(path) == posterPath only when path ==
  // posterSource, so this fixture seeds BOTH root properties consistently.
  const QVariantList assigned_existing = {
      Monitor(display1, QStringLiteral("Layar 1 (utama)"), 0, 0, 1920, 1080,
              true, QStringLiteral("landscape"), 100, 60, valid_path, true,
              QStringLiteral("covered")),
  };
  warnings.clear();
  Canvas k = MakeCanvas(&engine, &component, assigned_existing, 400.0, 300.0,
                        valid_path,
                        QStringLiteral("file:///C:/thumbs/valid.jpg"));
  Check(k.ok(), "fixture K: assigned+existing canvas instantiates");
  if (!k.ok()) {
    dump_warnings("fixture K");
    return Finish();
  }
  QQuickItem* kr = RectByKey(k.item, display1);
  Check(kr != nullptr, "fixture K: the assigned rect is addressable by key");
  if (kr == nullptr) {
    dump_warnings("fixture K");
    return Finish();
  }
  QQuickItem* k_assigned = Child(kr, "assignedLabel");
  QQuickItem* k_thumb = Child(kr, "assignedThumb");
  QQuickItem* k_ganti = Child(kr, "replaceAffordance");
  QQuickItem* k_hapus = Child(kr, "clearButton");
  const QString k_thumb_source =
      k_thumb != nullptr ? k_thumb->property("source").toString() : QString();
  std::printf(
      "QA-HAPPY(assignment) filename=\"%s\" thumbSource=\"%s\" "
      "ganti=\"%s\" hapus=\"%s\"\n",
      qUtf8Printable(TextOf(k_assigned)), qUtf8Printable(k_thumb_source),
      qUtf8Printable(TextOf(k_ganti)), qUtf8Printable(TextOf(k_hapus)));
  Check(DeclaredVisible(k_assigned) &&
            TextOf(k_assigned) == QStringLiteral("valid.mp4"),
        "fixture K: assigned+existing shows the assigned filename label");
  Check(DeclaredVisible(k_thumb) && !k_thumb_source.isEmpty() &&
            k_thumb_source.contains(QStringLiteral("valid.jpg")),
        "fixture K: posterSource/posterPath drive a non-empty thumbnail "
        "source through thumbFor");
  Check(!DeclaredVisible(Child(kr, "degradedLabel")),
        "fixture K: an existing assignment shows no degraded label");
  Check(!DeclaredVisible(Child(kr, "dropHint")),
        "fixture K: an assigned rect shows no drop hint");
  Check(!DeclaredVisible(Child(kr, "coverageWarning")),
        "fixture K: a covered assignment shows no coverage warning");
  Check(DeclaredVisible(k_ganti) && TextOf(k_ganti) == QStringLiteral("Ganti"),
        "fixture K: the 'ganti' replace affordance is visible");
  Check(DeclaredVisible(k_hapus) && TextOf(k_hapus) == QStringLiteral("Hapus"),
        "fixture K: the 'hapus' clear affordance is visible");

  // === Fixture K2: 'ganti' re-arms without mutating assignment state =======
  AssignSpy k_assign(&engine, k.object);
  ClearSpy k_clear(&engine, k.object);
  const QString k_assigned_before = kr->property("assignedPath").toString();
  if (k_ganti != nullptr) {
    QMetaObject::invokeMethod(k_ganti, "clicked");
  }
  std::printf(
      "QA-HAPPY(ganti) armed=%d assignEmissions=%d clearEmissions=%d "
      "assignedPath=\"%s\"\n",
      kr->property("dropActive").toBool() ? 1 : 0, k_assign.Count(),
      k_clear.Count(), qUtf8Printable(kr->property("assignedPath").toString()));
  Check(k_ganti != nullptr && kr->property("dropActive").toBool(),
        "fixture K2: 'ganti' click re-arms the rect (dropActive raised)");
  Check(k_assign.Count() == 0 && k_clear.Count() == 0,
        "fixture K2: 'ganti' emits neither assignRequested nor clearRequested");
  Check(kr->property("assignedPath").toString() == k_assigned_before &&
            kr->property("hasAssignment").toBool(),
        "fixture K2: 'ganti' mutates no assignment state");

  // === Fixture K3: 'hapus' emits clearRequested(key) exactly once ==========
  if (k_hapus != nullptr) {
    QMetaObject::invokeMethod(k_hapus, "clicked");
  }
  std::printf("QA-HAPPY(clear) clearEmissions=%d key=\"%s\"\n",
              k_clear.Count(), qUtf8Printable(k_clear.Key()));
  Check(k_clear.Count() == 1 && k_clear.Key() == display1,
        "fixture K3: 'hapus' click emits clearRequested(monitorKey) once");
  Check(k_assign.Count() == 0,
        "fixture K3: 'hapus' click emits no assignRequested");
  dump_warnings("fixture K");

  // === Fixture L: empty assignedPath -> neutral drop hint ==================
  warnings.clear();
  Canvas l = MakeCanvas(&engine, &component, one_monitor, 400.0, 300.0);
  Check(l.ok(), "fixture L: unassigned canvas instantiates");
  if (l.ok()) {
    QQuickItem* lr = RectByKey(l.item, display1);
    QQuickItem* l_hint = Child(lr, "dropHint");
    std::printf("QA-HAPPY(hint) visible=%d text=\"%s\"\n",
                DeclaredVisible(l_hint) ? 1 : 0,
                qUtf8Printable(TextOf(l_hint)));
    Check(DeclaredVisible(l_hint) &&
              TextOf(l_hint).contains(QStringLiteral("tarik video ke sini")),
          "fixture L: empty assignedPath shows the neutral drop hint");
    Check(!DeclaredVisible(Child(lr, "assignedLabel")) &&
              !DeclaredVisible(Child(lr, "degradedLabel")),
          "fixture L: an unassigned rect shows no assignment/degraded label");
    Check(!DeclaredVisible(Child(lr, "replaceAffordance")) &&
              !DeclaredVisible(Child(lr, "clearButton")),
          "fixture L: an unassigned rect hides replace/clear affordances");
  }
  dump_warnings("fixture L");

  // === Fixture M (failure): deleted file -> BOTH warnings simultaneously ===
  const QVariantList deleted = {
      Monitor(display1, QStringLiteral("Layar 1 (utama)"), 0, 0, 1920, 1080,
              true, QStringLiteral("landscape"), 100, 60,
              QStringLiteral("C:\\Videos\\terhapus.mp4"), false,
              QStringLiteral("headless")),
  };
  warnings.clear();
  Canvas m = MakeCanvas(&engine, &component, deleted, 400.0, 300.0);
  QQuickItem* mr = RectByKey(m.item, display1);
  QQuickItem* m_degraded = Child(mr, "degradedLabel");
  QQuickItem* m_coverage = Child(mr, "coverageWarning");
  std::printf(
      "QA-FAIL(deleted-file) degraded=\"%s\" visible=%d; coverage=\"%s\" "
      "visible=%d; both=%d\n",
      qUtf8Printable(TextOf(m_degraded)), DeclaredVisible(m_degraded) ? 1 : 0,
      qUtf8Printable(TextOf(m_coverage)), DeclaredVisible(m_coverage) ? 1 : 0,
      DeclaredVisible(m_degraded) && DeclaredVisible(m_coverage) ? 1 : 0);
  Check(DeclaredVisible(m_degraded) &&
            TextOf(m_degraded).contains(QStringLiteral("file tidak ditemukan")),
        "fixture M: the deleted file shows the 'file tidak ditemukan' label");
  if (m_degraded != nullptr) {
    const QColor degraded_color =
        m_degraded->property("color").value<QColor>();
    Check(degraded_color.red() > degraded_color.green() &&
              degraded_color.red() > degraded_color.blue(),
          "fixture M: the degraded label is red-ish");
  }
  Check(DeclaredVisible(m_coverage) &&
            TextOf(m_coverage).contains(QStringLiteral("cek engine.log")),
        "fixture M: coverage != covered shows the 'cek engine.log' warning");
  Check(DeclaredVisible(m_degraded) && DeclaredVisible(m_coverage),
        "fixture M: the degraded label and coverage warning render "
        "simultaneously (neither masks the other)");
  Check(DeclaredVisible(Child(mr, "degradedOverlay")) &&
            DeclaredVisible(Child(mr, "assignedLabel")) &&
            TextOf(Child(mr, "assignedLabel")) ==
                QStringLiteral("terhapus.mp4"),
        "fixture M: the degraded tint and the stale filename stay visible");
  Check(DeclaredVisible(Child(mr, "clearButton")),
        "fixture M: the 'hapus' affordance is offered for a stale assignment");
  dump_warnings("fixture M");

  return Finish();
}
