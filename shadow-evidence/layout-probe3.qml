import QtQuick
import QtQuick.Layouts

Window {
    id: win
    width: 1200
    height: 640
    visible: true
    color: "#202020"
    title: "PROBE3 waiting"

    ColumnLayout {
        anchors.fill: parent
        spacing: 4

        // A: baseline - left fillWidth, rail default
        RowLayout {
            id: rowA
            Layout.fillWidth: true
            Layout.preferredHeight: 70
            spacing: 8
            ColumnLayout {
                Layout.fillWidth: true
                Rectangle { Layout.preferredWidth: 832; implicitHeight: 30; color: "#d03030" }
            }
            ColumnLayout {
                id: railA
                Layout.preferredWidth: 340
                Layout.minimumWidth: 300
                Rectangle { Layout.fillWidth: true; implicitHeight: 30; color: "#30d030" }
            }
        }

        // B: rail explicit fillWidth:false
        RowLayout {
            id: rowB
            Layout.fillWidth: true
            Layout.preferredHeight: 70
            spacing: 8
            ColumnLayout {
                Layout.fillWidth: true
                Rectangle { Layout.preferredWidth: 832; implicitHeight: 30; color: "#d0d030" }
            }
            ColumnLayout {
                id: railB
                Layout.preferredWidth: 340
                Layout.minimumWidth: 300
                Layout.fillWidth: false
                Rectangle { Layout.fillWidth: true; implicitHeight: 30; color: "#3030d0" }
            }
        }

        // D: plain Rectangle as filler on the left
        RowLayout {
            id: rowD
            Layout.fillWidth: true
            Layout.preferredHeight: 70
            spacing: 8
            Rectangle {
                Layout.preferredWidth: 832
                Layout.fillWidth: true
                implicitHeight: 30
                color: "#d060d0"
            }
            ColumnLayout {
                id: railD
                Layout.preferredWidth: 340
                Layout.minimumWidth: 300
                Rectangle { Layout.fillWidth: true; implicitHeight: 30; color: "#60d0d0" }
            }
        }

        // E: explicit maximum on the left layout
        RowLayout {
            id: rowE
            Layout.fillWidth: true
            Layout.preferredHeight: 70
            spacing: 8
            ColumnLayout {
                Layout.fillWidth: true
                Layout.maximumWidth: 99999
                Rectangle { Layout.preferredWidth: 832; implicitHeight: 30; color: "#d09030" }
            }
            ColumnLayout {
                id: railE
                Layout.preferredWidth: 340
                Layout.minimumWidth: 300
                Rectangle { Layout.fillWidth: true; implicitHeight: 30; color: "#3090d0" }
            }
        }
    }

    Timer {
        interval: 700
        running: true
        repeat: true
        onTriggered: win.title = "P3 A=" + rowA.width + "/" + railA.width
                             + " B=" + rowB.width + "/" + railB.width
                             + " D=" + rowD.width + "/" + railD.width
                             + " E=" + rowE.width + "/" + railE.width
    }
}
