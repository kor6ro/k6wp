// Playlist panel: small editing surface for playlist.json. The engine reads
// that file and rotates on its own, so Studio only adds / removes / reorders
// entries here.
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Pane {
    id: playlistPanel
    property int spinWidth: 140
    property var armAssign: function (path) { return false }
    property var armedAssignPath: ""

    Layout.fillWidth: true
    Layout.preferredHeight: 170
    Material.elevation: 1
    padding: 12

    ColumnLayout {
        anchors.fill: parent
        spacing: 6

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Label {
                text: qsTr("Daftar putar (%1)").arg(Playlist.count)
                font.bold: true
            }

            Item {
                Layout.fillWidth: true
            }

            Button {
                text: qsTr("Tambah...")
                onClicked: Playlist.pickAndAdd()
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Tambah video ke daftar putar")
            }

            Button {
                text: qsTr("Kosongkan")
                enabled: Playlist.count > 0
                onClicked: Playlist.clear()
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Hapus semua entri daftar putar")
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            CheckBox {
                text: qsTr("Putar otomatis")
                checked: Playlist.enabled
                onToggled: Playlist.setEnabled(checked)
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Ganti wallpaper secara berkala")
            }

            Label {
                text: qsTr("Interval:")
            }

            SpinBox {
                Layout.preferredWidth: spinWidth
                from: 1
                to: 1440
                editable: true
                value: Playlist.intervalMin
                onValueModified: Playlist.setIntervalMin(value)
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Menit antar pergantian (1-1440)")
            }

            Label {
                text: qsTr(" menit")
            }

            Item {
                Layout.fillWidth: true
            }

            CheckBox {
                text: qsTr("Acak")
                checked: Playlist.shuffle
                onToggled: Playlist.setShuffle(checked)
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Putar daftar putar dalam urutan acak")
            }
        }

        Label {
            Layout.fillWidth: true
            visible: Playlist.lastError.length > 0
            wrapMode: Text.WordWrap
            color: "red"
            text: Playlist.lastError
        }

        Label {
            Layout.fillWidth: true
            Layout.fillHeight: true
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.WordWrap
            visible: Playlist.count === 0
            opacity: 0.7
            text: qsTr("Belum ada video di daftar putar — tambah dari perpustakaan atau berkas")
        }

        ListView {
            id: playlistList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            visible: Playlist.count > 0
            model: Playlist.items
            boundsBehavior: Flickable.StopAtBounds
            spacing: 2

            delegate: RowLayout {
                id: playlistCell
                required property var modelData
                required property int index

                width: playlistList.width
                spacing: 6
                // Existing missing-file dim.
                opacity: (modelData.exists === false ? 0.6 : 1.0)

                Label {
                    Layout.fillWidth: true
                    elide: Text.ElideMiddle
                    text: modelData.label
                    color: modelData.exists === false
                           ? "red"
                           : Material.foreground

                    // The label is the drag handle; the Naik /
                    // Turun / Hapus buttons must stay clickable,
                    // so they deliberately have no MouseArea.
                    MouseArea {
                        id: playlistDragArea
                        anchors.fill: parent
                    }
                }

                // Row 39 click-to-assign affordance,
                // disabled for a broken entry (empty
                // path). Row 42 tooltip fix: same
                // armed guard as the library
                // affordance - the arm click shifts
                // the layout under a stationary
                // pointer, so the bubble must not
                // outlive its hint.
                Button {
                    id: playlistAssignButton
                    objectName: "playlistAssignAffordance"
                    text: qsTr("Tandai")
                    enabled: playlistCell.modelData.path.length > 0
                    onClicked: armAssign(playlistCell.modelData.path)
                    ToolTip.visible: hovered && armedAssignPath.length === 0
                    ToolTip.text: qsTr("Tandai video ini untuk ditugaskan ke layar")
                }

                Button {
                    text: qsTr("Naik")
                    enabled: index > 0
                    onClicked: Playlist.moveUp(index)
                }

                Button {
                    text: qsTr("Turun")
                    enabled: index < Playlist.count - 1
                    onClicked: Playlist.moveDown(index)
                }

                Button {
                    text: qsTr("Hapus")
                    onClicked: Playlist.removeAt(index)
                }
            }
        }
    }
}
