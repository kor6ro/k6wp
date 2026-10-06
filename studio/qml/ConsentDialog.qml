// Long-video consent dialog. AskLongVideoConsent is a QMessageBox in C++, so
// the decision is routed through QML instead. Dialog lifecycle: onOpened /
// onClosed feed the root dialogOpened / dialogClosed pair so the native mpv
// preview HWND steps aside while this modal is showing.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: consentDialog
    property double consentDuration: 0
    property var dialogOpened: function () {}
    property var dialogClosed: function () {}

    title: qsTr("Video Panjang")
    modal: true
    anchors.centerIn: Overlay.overlay
    standardButtons: Dialog.Yes | Dialog.No
    onAccepted: Compress.resolveConsent(true)
    onRejected: Compress.resolveConsent(false)
    onOpened: dialogOpened()
    onClosed: dialogClosed()

    contentItem: Label {
        text: qsTr("Video berdurasi %1 menit (lebih dari 10). "
                   + "Kompres tetap?\n\nVideo panjang butuh waktu "
                   + "lama dan memakai --force-long.")
              .arg(consentDuration.toFixed(1))
        wrapMode: Text.WordWrap
    }
}
