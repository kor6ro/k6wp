// Cache-clear confirmation dialog (Pengaturan tab). Dialog lifecycle: onOpened /
// onClosed feed the root dialogOpened / dialogClosed pair so the native mpv
// preview HWND steps aside while this modal is showing.
//
// Surface / header / buttons are painted from Theme tokens for the same
// reason as AppDialogs.qml: the popup-local Material attached object does not
// inherit the window root's Material.theme across the popup boundary, so the
// stock Material Dialog background stayed white in dark mode. The popup-local
// Material theme is pinned to Theme.dark for the overlay dim and any residual
// Material-derived ink.
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Dialog {
    id: cacheDialog
    property var dialogOpened: function () {}
    property var dialogClosed: function () {}

    title: qsTr("Bersihkan cache")
    modal: true
    anchors.centerIn: Overlay.overlay
    // Task 32: tighter than the Material 24 default.
    padding: Theme.space3
    Material.theme: Theme.dark ? Material.Dark : Material.Light
    Material.accent: Theme.accent
    onOpened: dialogOpened()
    onClosed: dialogClosed()

    // 40px token button: shared Theme.glyph icon, ink focus ring (never the
    // blue accent), Enter/Space, Accessible.name. Mirrors
    // AppDialogs.DialogActionButton; extracting a shared file would need
    // CMake/qrc registration, so the small duplication is deliberate.
    component DialogActionButton: Button {
        id: cacheActionButton
        property bool primary: false
        property string glyph: ""
        implicitHeight: 40
        // Task 33: zero vertical padding keeps the 40dp content box centred
        // (Material's verticalPadding otherwise squeezed it to 12dp).
        topPadding: 0
        bottomPadding: 0
        focusPolicy: Qt.StrongFocus
        leftPadding: Theme.space2
        rightPadding: Theme.space2
        Accessible.name: text
        Keys.onReturnPressed: {
            cacheActionButton.clicked()
            event.accepted = true
        }
        Keys.onEnterPressed: {
            cacheActionButton.clicked()
            event.accepted = true
        }
        contentItem: RowLayout {
            spacing: Theme.space1

            Text {
                visible: cacheActionButton.glyph.length > 0
                Layout.alignment: Qt.AlignVCenter
                text: cacheActionButton.glyph
                font.family: Theme.glyphFont
                font.pixelSize: Theme.fontM
                color: cacheActionButton.primary ? Theme.accentText : Theme.text
                verticalAlignment: Text.AlignVCenter
                Accessible.ignored: true
            }

            Text {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
                text: cacheActionButton.text
                font.pixelSize: Theme.fontM
                font.weight: cacheActionButton.primary ? Theme.fontWeightSemibold
                                                       : Theme.fontWeightRegular
                color: cacheActionButton.primary ? Theme.accentText : Theme.text
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }
        }
        background: Rectangle {
            radius: Theme.radiusS
            color: cacheActionButton.primary
                   ? (cacheActionButton.down ? Qt.darker(Theme.accent, 1.15)
                                             : Theme.accent)
                   : (cacheActionButton.down ? Theme.pressedSurface
                      : (cacheActionButton.hovered
                         || cacheActionButton.activeFocus
                         ? Theme.surface2 : Theme.surface))
            border.width: cacheActionButton.activeFocus ? 2 : 0
            border.color: cacheActionButton.primary ? Theme.accentText
                                                    : Theme.focusRing
        }
    }

    background: Rectangle {
        color: Theme.surface
        radius: Theme.radiusL
        border.width: 1
        border.color: Theme.surface2
    }

    // Title row: shared Theme.glyph icon + the dialog title.
    header: RowLayout {
        spacing: Theme.space2

        Label {
            Layout.leftMargin: Theme.space3
            Layout.topMargin: Theme.space3
            Layout.alignment: Qt.AlignVCenter
            text: Theme.glyph.deleteFile
            font.family: Theme.glyphFont
            font.pixelSize: Theme.fontL
            color: Theme.text2
            Accessible.ignored: true
        }

        Label {
            Layout.topMargin: Theme.space3
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            text: cacheDialog.title
            elide: Label.ElideRight
            color: Theme.text
            font.pixelSize: Theme.fontL
            font.weight: Theme.fontWeightSemibold
        }
    }

    contentItem: Label {
        text: qsTr("Hapus semua file sementara di:\n%1")
              .arg(Settings.cacheDir)
        wrapMode: Text.WordWrap
        color: Theme.text
    }

    // Yes/Cancel replace the stock standardButtons so both actions keep
    // their exact behaviour (Hapus = Settings.clearCache()) with token
    // colours in both themes.
    footer: RowLayout {
        spacing: Theme.space2
        Item {
            Layout.fillWidth: true
        }
        DialogActionButton {
            id: cacheConfirmButton
            objectName: "cacheConfirm"
            primary: true
            text: qsTr("Ya")
            glyph: Theme.glyph.check
            onClicked: {
                cacheDialog.close()
                Settings.clearCache()
            }
        }
        DialogActionButton {
            id: cacheCancelButton
            objectName: "cacheCancel"
            text: qsTr("Batal")
            glyph: Theme.glyph.close
            onClicked: cacheDialog.close()
        }
    }
}
