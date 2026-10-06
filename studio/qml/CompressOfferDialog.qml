// Compress-first offer dialog: shown when a library/apply path is over the
// compress threshold. Dialog lifecycle: onOpened / onClosed feed the root
// dialogOpened / dialogClosed pair so the native mpv preview HWND steps aside
// while this modal is showing.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: compressOfferDialog
    property string offerPath: ""
    property int offerMb: 0
    property var startCompressFirst: function () {}
    property var declineCompressFirst: function () {}
    property var dialogOpened: function () {}
    property var dialogClosed: function () {}

    title: qsTr("Video Besar")
    modal: true
    anchors.centerIn: Overlay.overlay
    standardButtons: Dialog.Yes | Dialog.No
    onAccepted: startCompressFirst()
    onRejected: declineCompressFirst()

    // The mpv preview is a separate HWND that DWM composites above the
    // QQuickWidget, so this dialog would be hidden behind it. A 0x0 rect
    // drops the native surface; startCompressFirst / the restored preview
    // bring it back.
    onOpened: dialogOpened()
    onClosed: dialogClosed()

    contentItem: Label {
        // One translatable sentence with %1/%2 placeholders. Splitting it
        // across two qsTr() calls (as the wrap pass briefly did) would give
        // a translator two unrelated fragments instead of one sentence.
        text: qsTr("Video ini berukuran %1 MB. Kompres ke 1080p dulu agar "
                   + "hemat ~3x RAM saat dipakai sebagai wallpaper?\n\n%2")
              .arg(offerMb)
              .arg(offerPath)
        wrapMode: Text.WordWrap
    }
}
