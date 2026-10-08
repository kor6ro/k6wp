// The hole for the native mpv preview. Its rectangle is pushed to QmlShell on
// every change; the native PreviewWidget then paints exactly on top of it, so
// this Pane is what the user sees before mpv has its first frame (and if the
// preview is ever hidden).
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Pane {
    id: previewHole
    property var syncPreview: function () {}
    signal geometryChanged

    Layout.fillWidth: true
    Layout.fillHeight: true
    Layout.minimumHeight: 200
    Layout.preferredHeight: 340
    Material.elevation: 2

    onXChanged: { geometryChanged(); syncPreview() }
    onYChanged: { geometryChanged(); syncPreview() }
    onWidthChanged: { geometryChanged(); syncPreview() }
    onHeightChanged: { geometryChanged(); syncPreview() }
    // StackLayout toggles `visible` when the tab
    // changes, so the preview follows the page.
    onVisibleChanged: { geometryChanged(); syncPreview() }
    Component.onCompleted: { geometryChanged(); syncPreview() }

    HoverHandler {
        id: holeHover
    }
    ToolTip.visible: holeHover.hovered
    ToolTip.text: qsTr("Klik untuk jeda/jalan")

    // Cached thumbnail, so the hole is not black
    // while libmpv starts. Covered by the native
    // surface once mpv renders. `visible: status ===
    // Image.Ready` keeps a missing/blank file
    // silent.
    Image {
        anchors.fill: parent
        fillMode: Image.PreserveAspectFit
        asynchronous: true
        visible: status === Image.Ready
        source: Studio.posterPath.length > 0
                ? "file:///" + Studio.posterPath.replace(/\\/g, "/")
                : ""
    }
}
