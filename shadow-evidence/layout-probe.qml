import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

Window {
    id: win
    width: 1200
    height: 320
    visible: true
    color: "#202020"

    ColumnLayout {
        anchors.fill: parent
        spacing: 4

        // Case A: left fillWidth + rail preferredWidth 340 (the app's shape)
        RowLayout {
            id: rowA
            Layout.fillWidth: true
            Layout.preferredHeight: 120
            spacing: 8

            ColumnLayout {
                Layout.fillWidth: true
                Rectangle { id: rectA; Layout.fillWidth: true; implicitHeight: 60; color: "#d03030" }
            }
            ColumnLayout {
                id: railA
                Layout.preferredWidth: 340
                Layout.minimumWidth: 300
                Rectangle { Layout.fillWidth: true; implicitHeight: 60; color: "#30d030" }
            }
        }

        // Case B: rail pinned with maximumWidth
        RowLayout {
            id: rowB
            Layout.fillWidth: true
            Layout.preferredHeight: 120
            spacing: 8

            ColumnLayout {
                Layout.fillWidth: true
                Rectangle { id: rectB; Layout.fillWidth: true; implicitHeight: 60; color: "#d0d030" }
            }
            ColumnLayout {
                id: railB
                Layout.preferredWidth: 340
                Layout.minimumWidth: 300
                Layout.maximumWidth: 340
                Rectangle { Layout.fillWidth: true; implicitHeight: 60; color: "#3030d0" }
            }
        }
    }

}
