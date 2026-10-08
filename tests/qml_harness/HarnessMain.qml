// HarnessMain.qml — task 25 offscreen proof shell for WarningBanner
// kind="duplicate". Loaded from disk (NOT compiled into K6WPHarness) so
// qt_add_qml_module only sees studio/qml/* files — a QML source outside
// CMAKE_CURRENT_SOURCE_DIR makes qmlcache emit a ".." segment and MSBuild
// fails with MSB3191. import K6WPHarness resolves Theme (singleton) and
// WarningBanner from the qrc module; Studio is a C++ context property.
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import K6WPHarness

Item {
    id: root
    width: 640
    height: 80
    Material.theme: Material.Light

    WarningBanner {
        objectName: "harnessDuplicateBanner"
        kind: "duplicate"
        anchors.fill: parent
    }
}
