import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

Window {
    id: win
    width: 1200
    height: 420
    visible: true
    color: "#202020"
    title: "PROBE waiting"

    ColumnLayout {
        anchors.fill: parent
        spacing: 4

        // Case A: like the app - left fillWidth with a wide child (832),
        // rail preferredWidth 340 minimumWidth 300
        RowLayout {
            id: rowA
            Layout.fillWidth: true
            Layout.preferredHeight: 90
            spacing: 8
            ColumnLayout {
                Layout.fillWidth: true
                Rectangle { Layout.preferredWidth: 832; implicitHeight: 40; color: "#d03030" }
            }
            ColumnLayout {
                id: railA
                Layout.preferredWidth: 340
                Layout.minimumWidth: 300
                Rectangle { id: rectA; Layout.fillWidth: true; implicitHeight: 40; color: "#30d030" }
            }
        }

        // Case B: same, rail pinned with maximumWidth 340
        RowLayout {
            id: rowB
            Layout.fillWidth: true
            Layout.preferredHeight: 90
            spacing: 8
            ColumnLayout {
                Layout.fillWidth: true
                Rectangle { Layout.preferredWidth: 832; implicitHeight: 40; color: "#d0d030" }
            }
            ColumnLayout {
                id: railB
                Layout.preferredWidth: 340
                Layout.minimumWidth: 300
                Layout.maximumWidth: 340
                Rectangle { id: rectB; Layout.fillWidth: true; implicitHeight: 40; color: "#3030d0" }
            }
        }

        // Case C: left fillWidth with small child (isolate implicit effect)
        RowLayout {
            id: rowC
            Layout.fillWidth: true
            Layout.preferredHeight: 90
            spacing: 8
            ColumnLayout {
                Layout.fillWidth: true
                Rectangle { Layout.preferredWidth: 200; implicitHeight: 40; color: "#d060d0" }
            }
            ColumnLayout {
                id: railC
                Layout.preferredWidth: 340
                Layout.minimumWidth: 300
                Rectangle { id: rectC; Layout.fillWidth: true; implicitHeight: 40; color: "#60d0d0" }
            }
        }
    }

    Timer {
        interval: 700
        running: true
        repeat: true
        onTriggered: win.title = "PROBE A=" + rowA.width + ":" + railA.width
                             + " B=" + rowB.width + ":" + railB.width
                             + " C=" + rowC.width + ":" + railC.width
    }
}
