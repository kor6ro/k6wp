// Library grid delegate: one card per library entry. The right-click rowMenu
// and the remove confirmation Dialog move WITH the delegate (they were
// previously inline in Main.qml's GridView delegate). Delegate context
// (index / dst / cellWidth) is received via required properties — no
// root.* lookups.
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Item {
    id: libraryCell

    required property int index
    required property string label
    required property string thumbUrl
    required property string dst

    // Sizing + state from Main root, passed as properties.
    property int cellWidth: 190
    property int cellHeight: 170
    property var armAssign: function (path) { return false }
    property var armedAssignPath: ""
    property var dialogOpened: function () {}
    property var dialogClosed: function () {}

    width: cellWidth
    height: cellHeight

    // thumbUrl stays empty until a thumbnail
    // exists, and an Image with no source never
    // changes status - so the onStatusChanged
    // trigger below can never fire on a first
    // run. Request the thumbnail here instead.
    Component.onCompleted: if (thumbUrl.length === 0)
        Library.ensureThumbnail(index)

    Pane {
        anchors.fill: parent
        anchors.margins: 4
        Material.elevation: 1

        // Row 42: the cell-wide click surface
        // is declared first so the thumbnail,
        // filename and assign affordance all
        // paint above it. Images and labels do
        // not handle mouse events in Qt Quick,
        // so clicks on them still reach this
        // area; only the assign button consumes
        // its own clicks, keeping the existing
        // double-click / right-click behavior.
        MouseArea {
            id: dragArea
            anchors.fill: parent
            hoverEnabled: true
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            onDoubleClicked: Library.applyAt(index)
            onClicked: function (mouse) {
                if (mouse.button === Qt.RightButton)
                    rowMenu.popup()
            }
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 8
            spacing: 4

            Image {
                Layout.fillWidth: true
                Layout.fillHeight: true
                fillMode: Image.PreserveAspectFit
                asynchronous: true
                visible: status === Image.Ready
                source: thumbUrl
                // Ask the model for a thumbnail once
                // the row is realised; the model
                // debounces so a scroll cannot
                // spawn ffmpeg repeatedly.
                onStatusChanged: if (status !== Image.Ready)
                                     Library.ensureThumbnail(index)
            }

            // Row 42: the filename keeps its
            // two lines but reserves the
            // card's bottom-right corner, where
            // the assign affordance sits.
            Label {
                Layout.fillWidth: true
                Layout.rightMargin: 58
                maximumLineCount: 2
                elide: Text.ElideRight
                wrapMode: Text.WordWrap
                font.pixelSize: 11
                text: label
            }
        }

        // Row 39 click-to-assign affordance,
        // moved by row 42 into the card's
        // bottom-right corner (inside the
        // filename strip): it can no longer
        // cover the thumbnail, and the row's
        // click surface below stays intact.
        // Disabled for a broken entry (empty
        // dst), the same non-empty-path gate
        // the old drag target used.
        Button {
            id: libraryAssignButton
            objectName: "libraryAssignAffordance"
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: 8
            text: qsTr("Tandai")
            flat: true
            implicitWidth: 52
            implicitHeight: 22
            // Material's 8px vertical button
            // padding survives `padding: 0`
            // (the style binds top/bottom
            // padding separately) and would
            // leave a 22px-tall button with
            // only 6px of content height,
            // which clips the label away - the
            // compact size owns its paddings
            // explicitly.
            topPadding: 3
            bottomPadding: 3
            leftPadding: 6
            rightPadding: 6
            font.pixelSize: 10
            enabled: libraryCell.dst.length > 0
            onClicked: armAssign(libraryCell.dst)
            // Row 42 tooltip fix: the arm
            // click shifts the whole view down
            // while the pointer is stationary,
            // and Qt does not re-evaluate hover
            // without a mouse move - so the
            // bubble stayed pinned over the
            // search box. It now hides the
            // moment the armed banner takes
            // over (the banner carries the next
            // instruction).
            ToolTip.visible: hovered && armedAssignPath.length === 0
            ToolTip.text: qsTr("Tandai video ini untuk ditugaskan ke layar")
        }

        Menu {
            id: rowMenu
            MenuItem {
                text: qsTr("Terapkan")
                onTriggered: Library.applyAt(index)
            }
            MenuItem {
                text: qsTr("Kompres-ulang")
                onTriggered: Library.recompressAt(index)
            }
            MenuItem {
                text: qsTr("Buka Lokasi")
                onTriggered: Library.openLocationAt(index)
            }
            MenuItem {
                text: qsTr("Hapus")
                onTriggered: removeDialog.open()
            }
        }
    }

    Dialog {
        id: removeDialog
        title: qsTr("Hapus Entri Perpustakaan")
        modal: true
        anchors.centerIn: Overlay.overlay
        standardButtons: Dialog.Yes | Dialog.No
        onAccepted: Library.removeAt(index, true)
        onOpened: dialogOpened()
        onClosed: dialogClosed()

        contentItem: Label {
            text: qsTr("Hapus entri untuk:\n%1"
                       + "\n\nFile dipindahkan ke Recycle Bin.")
                  .arg(dst)
            wrapMode: Text.WordWrap
        }
    }
}
