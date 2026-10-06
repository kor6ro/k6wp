// Cache-clear confirmation dialog (Pengaturan tab). Dialog lifecycle: onOpened /
// onClosed feed the root dialogOpened / dialogClosed pair so the native mpv
// preview HWND steps aside while this modal is showing.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: cacheDialog
    property var dialogOpened: function () {}
    property var dialogClosed: function () {}

    title: qsTr("Bersihkan cache")
    modal: true
    anchors.centerIn: Overlay.overlay
    standardButtons: Dialog.Yes | Dialog.Cancel
    onAccepted: Settings.clearCache()
    onOpened: dialogOpened()
    onClosed: dialogClosed()

    contentItem: Label {
        text: qsTr("Hapus semua file sementara di:\n%1")
              .arg(Settings.cacheDir)
        wrapMode: Text.WordWrap
    }
}
