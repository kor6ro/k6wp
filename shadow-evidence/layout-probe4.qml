import QtQuick
import QtQuick.Layouts

Window {
    id: win
    width: 1200
    height: 640
    visible: true
    color: "#202020"
    title: "PROBE4 waiting"

    ColumnLayout {
        anchors.fill: parent
        spacing: 4

        // F1: left capped lifted + rail fillWidth false
        RowLayout {
            id: rowA
            Layout.fillWidth: true
            Layout.preferredHeight: 70
            spacing: 8
            ColumnLayout {
                Layout.fillWidth: true
                Layout.maximumWidth: 1000000
                Rectangle { Layout.preferredWidth: 832; implicitHeight: 30; color: "#d03030" }
            }
            ColumnLayout {
                id: railA
                Layout.preferredWidth: 340
                Layout.minimumWidth: 300
                Layout.fillWidth: false
                Rectangle { Layout.fillWidth: true; implicitHeight: 30; color: "#30d030" }
            }
        }

        // F2: plus maximumWidth on rail
        RowLayout {
            id: rowB
            Layout.fillWidth: true
            Layout.preferredHeight: 70
            spacing: 8
            ColumnLayout {
                Layout.fillWidth: true
                Layout.maximumWidth: 1000000
                Rectangle { Layout.preferredWidth: 832; implicitHeight: 30; color: "#d0d030" }
            }
            ColumnLayout {
                id: railB
                Layout.preferredWidth: 340
                Layout.minimumWidth: 300
                Layout.maximumWidth: 340
                Layout.fillWidth: false
                Rectangle { Layout.fillWidth: true; implicitHeight: 30; color: "#3030d0" }
            }
        }

        // F4: left lifted, rail untouched (default fill for layouts) - expect proportional split
        RowLayout {
            id: rowD
            Layout.fillWidth: true
            Layout.preferredHeight: 70
            spacing: 8
            ColumnLayout {
                Layout.fillWidth: true
                Layout.maximumWidth: 1000000
                Rectangle { Layout.preferredWidth: 832; implicitHeight: 30; color: "#d060d0" }
            }
            ColumnLayout {
                id: railD
                Layout.preferredWidth: 340
                Layout.minimumWidth: 300
                Rectangle { Layout.fillWidth: true; implicitHeight: 30; color: "#60d0d0" }
            }
        }

        // F5: left lifted with Infinity
        RowLayout {
            id: rowE
            Layout.fillWidth: true
            Layout.preferredHeight: 70
            spacing: 8
            ColumnLayout {
                Layout.fillWidth: true
                Layout.maximumWidth: Number.POSITIVE_INFINITY
                Rectangle { Layout.preferredWidth: 832; implicitHeight: 30; color: "#d09030" }
            }
            ColumnLayout {
                id: railE
                Layout.preferredWidth: 340
                Layout.minimumWidth: 300
                Layout.fillWidth: false
                Rectangle { Layout.fillWidth: true; implicitHeight: 30; color: "#3090d0" }
            }
        }
    }

    Timer {
        interval: 700
        running: true
        repeat: true
        onTriggered: win.title = "P4 A=" + rowA.width + "-" + rowA.children[1].width
                             + " B=" + rowB.width + "-" + railB.width
                             + " D=" + rowD.width + "-" + railD.width
                             + " E=" + rowE.width + "-" + railE.width
    }
}
