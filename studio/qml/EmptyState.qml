// Empty collection state - copy deck C-1 verbatim:
//   "Belum ada video di koleksi kamu"
//   "Tambahkan video favoritmu, lalu klik untuk memasangnya."
//   [+ Pilih video]
//   "atau seret & letakkan file ke sini"
//
// Drag-and-drop is handled by QmlShell (C++ WM_DROPFILES), not by a QML
// DropArea - the hint is text only. Shown by CollectionPage when the library
// itself is empty (a live search with no matches uses the C-4 line instead).
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Item {
    id: emptyState

    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(parent.width - 2 * Theme.space4, 420)
        spacing: Theme.space2

        // Restrained illustration: a rounded "screen" with a play glyph.
        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            implicitWidth: 148
            implicitHeight: 92
            radius: Theme.radiusL
            color: Theme.surface2
            Accessible.ignored: true

            Label {
                anchors.centerIn: parent
                text: "\u25B6\uFE0E"
                color: Theme.text2
                opacity: 0.7
                font.pixelSize: 32
                Accessible.ignored: true
            }
        }

        Label {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            font.pixelSize: Theme.fontL
            font.weight: Theme.fontWeightSemibold
            color: Theme.text
            text: qsTr("Belum ada video di koleksi kamu")
        }

        Label {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            font.pixelSize: Theme.fontM
            color: Theme.text2
            text: qsTr("Tambahkan video favoritmu, lalu klik untuk memasangnya.")
        }

        Button {
            id: pickButton
            objectName: "emptyPickVideo"
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("+ Pilih video")
            implicitHeight: 40
            focusPolicy: Qt.StrongFocus
            Accessible.name: text
            onClicked: Library.pickAndImport()
            Keys.onReturnPressed: {
                pickButton.clicked()
                event.accepted = true
            }
            Keys.onEnterPressed: {
                pickButton.clicked()
                event.accepted = true
            }
            contentItem: Text {
                text: pickButton.text
                font.pixelSize: Theme.fontM
                font.weight: Theme.fontWeightSemibold
                color: Theme.accentText
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            background: Rectangle {
                radius: Theme.radiusS
                color: Theme.accent
                border.width: pickButton.activeFocus ? 2 : 0
                border.color: Theme.accentText
            }
        }

        Label {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            font.pixelSize: Theme.fontS
            color: Theme.text2
            text: qsTr("atau seret & letakkan file ke sini")
        }
    }
}
