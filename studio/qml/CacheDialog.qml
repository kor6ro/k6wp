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
    Material.theme: Theme.dark ? Material.Dark : Material.Light
    Material.accent: Theme.accent
    onOpened: dialogOpened()
    onClosed: dialogClosed()

    // 40px token button: visible focus ring, Enter/Space, Accessible.name.
    // Mirrors AppDialogs.DialogActionButton; extracting a shared file would
    // need CMake/qrc registration, so the small duplication is deliberate.
    component DialogActionButton: Button {
        id: cacheActionButton
        property bool primary: false
        implicitHeight: 40
        focusPolicy: Qt.StrongFocus
        leftPadding: Theme.space3
        rightPadding: Theme.space3
        Accessible.name: text
        Keys.onReturnPressed: {
            cacheActionButton.clicked()
            event.accepted = true
        }
        Keys.onEnterPressed: {
            cacheActionButton.clicked()
            event.accepted = true
        }
        contentItem: Text {
            text: cacheActionButton.text
            font.pixelSize: Theme.fontM
            font.weight: cacheActionButton.primary ? Theme.fontWeightSemibold
                                                   : Theme.fontWeightRegular
            color: cacheActionButton.primary ? Theme.accentText : Theme.text
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
        background: Rectangle {
            radius: Theme.radiusS
            color: cacheActionButton.primary
                   ? Theme.accent
                   : (cacheActionButton.hovered || cacheActionButton.activeFocus
                      ? Theme.surface2 : Theme.surface)
            border.width: cacheActionButton.activeFocus ? 2 : 0
            border.color: cacheActionButton.primary ? Theme.accentText : Theme.accent
        }
    }

    background: Rectangle {
        color: Theme.surface
        radius: Theme.radiusL
        border.width: 1
        border.color: Theme.surface2
    }

    header: Label {
        text: cacheDialog.title
        padding: Theme.space4
        bottomPadding: 0
        elide: Label.ElideRight
        color: Theme.text
        font.pixelSize: Theme.fontL
        font.weight: Theme.fontWeightSemibold
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
            onClicked: {
                cacheDialog.close()
                Settings.clearCache()
            }
        }
        DialogActionButton {
            id: cacheCancelButton
            objectName: "cacheCancel"
            text: qsTr("Batal")
            onClicked: cacheDialog.close()
        }
    }
}
