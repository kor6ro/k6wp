// Koleksi gallery grid: large-thumbnail cards over the Library model with the
// live filter applied (CollectionPage's "Cari video..." field sets
// Library.filter; the view itself is a pure consumer).
//
// Task 28 affordance: the grid sits inside a distinct Theme.border frame
// (surface fill on the page bg) and carries an ALWAYS-ON vertical ScrollBar
// with a token-styled handle, so the panel reads as one scrollable surface
// instead of a borderless grid floating on the page. The scrollbar is an
// overlay inside the frame - it never covers the preview hole above (it is
// clipped to the gallery rectangle).
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

    // Large thumbnails: 248px cells with the card's own 4px gutter (task 32
    // shaved 8px off both axes - the gallery reads denser, targets unchanged).
    readonly property int cellWidth: 248
    readonly property int cellHeight: 236

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

    // Distinct gallery frame (task 28): surface fill + Theme.border edge.
    // The 1px edge is what "borderless" was missing; clip keeps the cards
    // inside the frame while flicking.
    Rectangle {
        id: galleryFrame
        objectName: "galleryFrame"
        anchors.fill: parent
        radius: Theme.radiusM
        color: Theme.surface
        border.width: 1
        border.color: Theme.border
        clip: true

        GridView {
            id: grid
            objectName: "collectionGrid"
            anchors.fill: parent
            anchors.margins: Theme.space1
            clip: true
            model: Library
            cellWidth: videoGrid.cellWidth
            cellHeight: videoGrid.cellHeight
            boundsBehavior: Flickable.StopAtBounds

            // Always-visible vertical scrollbar (task 28): the user must see
            // that the panel scrolls BEFORE content overflows. Track and
            // handle use Theme tokens (surface2 / text2), 12px hit width.
            // ScrollBar's movable part is its contentItem (not a "handle"
            // property - that one belongs to Slider).
            ScrollBar.vertical: ScrollBar {
                id: galleryScrollBar
                objectName: "galleryScrollBar"
                policy: ScrollBar.AlwaysOn
                implicitWidth: 12
                background: Rectangle {
                    color: Theme.surface2
                    radius: Theme.radiusS
                }
                contentItem: Rectangle {
                    implicitWidth: 8
                    implicitHeight: 32
                    radius: Theme.radiusS
                    // Pressed reads as full ink, never a blue flash.
                    color: galleryScrollBar.pressed ? Theme.text
                                                     : Theme.text2
                }
            }

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
}
