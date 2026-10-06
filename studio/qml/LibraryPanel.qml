// Library panel: the Wallpaper left-column library section. Contains the
// search/import row, lastError, empty-state label, and the GridView whose
// delegate is LibraryDelegate.qml (the card + rowMenu + removeDialog moved
// into that file).
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: libraryPanel
    property var armAssign: function (path) { return false }
    property var armedAssignPath: ""
    property var dialogOpened: function () {}
    property var dialogClosed: function () {}

    Layout.fillWidth: true
    Layout.fillHeight: true
    spacing: 8

    // --- Perpustakaan (library grid) -----------------
    RowLayout {
        Layout.fillWidth: true
        spacing: 8

        Label {
            text: qsTr("Perpustakaan:")
            font.bold: true
        }

        TextField {
            Layout.fillWidth: true
            placeholderText: qsTr("Cari video...")
            onTextChanged: Library.filter = text
        }

        Button {
            text: qsTr("&Impor Video")
            onClicked: Library.pickAndImport()
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Impor video ke perpustakaan untuk mulai")
        }
    }

    Label {
        Layout.fillWidth: true
        visible: Library.lastError.length > 0
        wrapMode: Text.WordWrap
        color: "red"
        text: Library.lastError
    }

    // Empty state: distinct copy for "nothing imported
    // yet" vs "no search results", which is what the
    // old grid_widget empty label keyed off.
    Label {
        Layout.fillWidth: true
        Layout.preferredHeight: 48
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        wrapMode: Text.WordWrap
        visible: Library.visibleCount === 0
        opacity: 0.7
        text: Library.isEmpty
              ? qsTr("Perpustakaan kosong — impor video lewat Berkas > Impor atau seret & letakkan")
              : qsTr("Tidak ada hasil untuk pencarian ini")
    }

    GridView {
        id: libraryGrid
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.minimumHeight: 140
        Layout.preferredHeight: 220
        clip: true
        visible: Library.visibleCount > 0
        model: Library

        cellWidth: 190
        cellHeight: 170
        boundsBehavior: Flickable.StopAtBounds

        delegate: LibraryDelegate {
            cellWidth: libraryGrid.cellWidth
            cellHeight: libraryGrid.cellHeight
            armAssign: libraryPanel.armAssign
            armedAssignPath: libraryPanel.armedAssignPath
            dialogOpened: libraryPanel.dialogOpened
            dialogClosed: libraryPanel.dialogClosed
        }
    }
}
