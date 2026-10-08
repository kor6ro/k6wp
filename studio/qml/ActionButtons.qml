// Action buttons grid: the Wallpaper right-rail GridLayout of engine actions
// (pick video, apply wallpaper, clear, start engine + busy indicator).
// Content-width buttons in a 2-column grid.
//
// Jeda / Lanjut are NOT here: the pause/resume pair merged into the single
// StatusBar button (glossary §5 "Hentikan Wallpaper + Lanjutkan -> satu
// tombol Jeda / Lanjut", plan todo 5).
//
// SUPERSEDED by task 28: the Beranda rail no longer instantiates this file -
// StatusBar.qml owns the ONE action row (Jeda/Lanjut, Pilih video, Pasang,
// Hapus) and the "Aktifkan lagi" recovery button next to the status sentence.
// It stays in the tree only because studio/CMakeLists.txt lists it in
// QML_FILES and this slice does not own CMakeLists. Do not re-instantiate:
// the controls would show up twice on Beranda.
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

GridLayout {
    id: actionButtons
    property int formColSpacing: 12
    property int formRowSpacing: 10
    property var statusKindNotRunning: 3
    // Todo 12: "Terapkan Wallpaper" installs through the home view's
    // "Pasang ke" path (target selection + C-14 on multi-screen rigs), so it
    // gets the same entry point as the gallery cards.
    property var installVideo: function (path) {}

    columns: 2
    columnSpacing: formColSpacing
    rowSpacing: formRowSpacing

    Button {
        text: qsTr("Pilih video")
        onClicked: Studio.pickVideo()
        ToolTip.visible: hovered
        // Glossary §5: "Terapkan" -> "Pasang", "perpustakaan" -> "koleksi".
        ToolTip.text: qsTr("Pilih file video untuk dipasang tanpa impor ke koleksi")
    }

    Button {
        text: qsTr("Pasang")
        enabled: Studio.selectedVideo.length > 0 && !Studio.busy
        onClicked: actionButtons.installVideo(Studio.selectedVideo)
        ToolTip.visible: hovered
        ToolTip.text: qsTr("Pasang video terpilih sebagai wallpaper (langsung, tanpa restart)")
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
        text: qsTr("Aktifkan lagi")
        visible: Studio.engineStatusKind === statusKindNotRunning
        enabled: !Studio.busy
        onClicked: Studio.startEngine()
        ToolTip.visible: hovered
        ToolTip.text: qsTr("Aktifkan lagi wallpaper yang sedang tidak berjalan")
    }

    BusyIndicator {
        Layout.alignment: Qt.AlignVCenter
        running: Studio.busy
        visible: Studio.busy
    }
}
