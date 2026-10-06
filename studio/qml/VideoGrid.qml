// Koleksi gallery grid: large-thumbnail cards over the Library model with the
// live filter applied (CollectionPage's "Cari video..." field sets
// Library.filter; the view itself is a pure consumer).
//
// Empty slots: a cell without a cached thumbnail paints the delegate's own
// surface2 placeholder, and rows past the last entry stay empty - the C-1
// empty state and the C-4 no-results line belong to CollectionPage, which
// owns the copy and shows/hides this view.
//
// Card states (normal / hover / focus / Dipakai / Hilang / Belum-siap /
// Progres / Error) live in VideoCard.qml per the brief B-STATE matrix.
import QtQuick
import QtQuick.Controls

Item {
    id: videoGrid

    property var dialogOpened: function () {}
    property var dialogClosed: function () {}

    // Large thumbnails: 256px cells with the card's own 4px gutter.
    readonly property int cellWidth: 256
    readonly property int cellHeight: 244

    // One-shot startup focus: a keyboard-only user gets a visible starting
    // point on the first card (the card's own Enter/Space installs it). The
    // guard means a later model reset - typing in the search filter, a delete
    // - can never yank focus away from wherever the user is working.
    property bool initialFocusGiven: false

    function positionAtBeginning() {
        grid.positionViewAtBeginning()
    }

    function giveInitialFocus() {
        if (initialFocusGiven) {
            return
        }
        const first = grid.itemAtIndex(0)
        if (first) {
            initialFocusGiven = true
            first.forceActiveFocus()
        }
    }

    Component.onCompleted: Qt.callLater(giveInitialFocus)

    GridView {
        id: grid
        objectName: "collectionGrid"
        anchors.fill: parent
        clip: true
        model: Library
        cellWidth: videoGrid.cellWidth
        cellHeight: videoGrid.cellHeight
        boundsBehavior: Flickable.StopAtBounds

        // Delegates can be created after this component completes (and after
        // the first count change); callLater re-checks once item 0 exists.
        onCountChanged: Qt.callLater(videoGrid.giveInitialFocus)

        delegate: VideoCard {
            cellWidth: grid.cellWidth
            cellHeight: grid.cellHeight
            dialogOpened: videoGrid.dialogOpened
            dialogClosed: videoGrid.dialogClosed
        }
    }
}
