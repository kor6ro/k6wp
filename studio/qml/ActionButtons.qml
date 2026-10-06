// Action buttons grid: the Wallpaper right-rail GridLayout of engine actions
// (pick video, apply wallpaper, clear, start engine + busy indicator).
// Content-width buttons in a 2-column grid.
//
// Jeda / Lanjut are NOT here: the pause/resume pair merged into the single
// StatusBar button (glossary §5 "Hentikan Wallpaper + Lanjutkan -> satu
// tombol Jeda / Lanjut", plan todo 5).
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

GridLayout {
    id: actionButtons
    property int formColSpacing: 12
    property int formRowSpacing: 10
    property var maybeOfferCompressFirst: function (path, applyAfter) { return false }
    property var statusKindNotRunning: 3

    columns: 2
    columnSpacing: formColSpacing
    rowSpacing: formRowSpacing

    Button {
        text: qsTr("Pilih &Video")
        onClicked: Studio.pickVideo()
        ToolTip.visible: hovered
        ToolTip.text: qsTr("Pilih file untuk diterapkan tanpa impor ke perpustakaan")
    }

    Button {
        text: qsTr("&Terapkan Wallpaper")
        enabled: Studio.selectedVideo.length > 0 && !Studio.busy
        onClicked: {
            if (!maybeOfferCompressFirst(Studio.selectedVideo, true))
                Studio.applyWallpaper(Studio.selectedVideo)
        }
        ToolTip.visible: hovered
        ToolTip.text: qsTr("Terapkan video terpilih sebagai wallpaper (live-switch, tanpa restart saat engine jalan)")
    }

    Button {
        text: qsTr("Hapus")
        enabled: Studio.selectedVideo.length > 0
        onClicked: Studio.clearSelectedVideo()
        ToolTip.visible: hovered
        ToolTip.text: qsTr("Hapus dari daftar")
    }

    // Visible only on NotRunning, the same rule
    // MainWindow::OnPollDone used for its start button.
    Button {
        text: qsTr("Nyalakan Engine")
        visible: Studio.engineStatusKind === statusKindNotRunning
        enabled: !Studio.busy
        onClicked: Studio.startEngine()
        ToolTip.visible: hovered
        ToolTip.text: qsTr("Menyalakan engine wallpaper (muncul saat engine mati)")
    }

    BusyIndicator {
        Layout.alignment: Qt.AlignVCenter
        running: Studio.busy
        visible: Studio.busy
    }
}
