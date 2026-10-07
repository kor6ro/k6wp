// Koleksi: the unified collection view (brief B-WIREFRAME(c)) - the search
// row (C-4 "Cari video..."), the C-7 import button, the C-1 empty state, the
// C-4 no-results line, the Ganti otomatis panel (C-11) and the large
// thumbnail VideoGrid. It replaces the old LibraryPanel/LibraryDelegate pair;
// every string and state comes from the copy deck + the card matrix, no
// legacy jargon.
//
// The Ganti otomatis panel lives here, below the gallery's search row: it is
// part of the gallery area, never an overlay above the preview hole.
// dialogs opened by the cards feed dialogOpened/dialogClosed through the
// parent chain so the native preview steps aside.
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

ColumnLayout {
    id: collectionPage

    property var dialogOpened: function () {}
    property var dialogClosed: function () {}
    // Main.root.firstRunActive (forwarded through WallpaperPage): while the
    // first-run onboarding owns the screen, the C-1 empty state must not
    // compete with it (plan todo 16).
    property bool firstRunActive: false

    Layout.fillWidth: true
    Layout.fillHeight: true
    spacing: Theme.space2

    // --- C-15 missing-file summary (todo 12) ------------------------------
    // The Library model exposes `missing` per delegate but no collection-wide
    // counter, and this slice must not touch the C++ bridge, so a non-visual
    // Instantiator probes the model's own roles (QtObject delegates - no
    // visuals, no thumbnails). It sees the same filtered model the grid and
    // the row-addressed invokables (openLocationAt / removeAt) consume, so
    // the reported first row is directly actionable.
    property int missingCards: 0
    property int firstMissingCard: -1

    function recountMissing() {
        // Coalesce: one recount per event-loop turn covers creation, role
        // updates and removals alike.
        Qt.callLater(collectionPage.doRecountMissing)
    }

    function doRecountMissing() {
        let count = 0
        let first = -1
        for (let i = 0; i < missingProbe.count; ++i) {
            const probe = missingProbe.objectAt(i)
            if (probe && probe.missing) {
                count += 1
                if (first < 0 || probe.index < first)
                    first = probe.index
            }
        }
        collectionPage.missingCards = count
        collectionPage.firstMissingCard = first
    }

    Instantiator {
        id: missingProbe
        model: Library

        delegate: QtObject {
            required property bool missing
            required property int index
            onMissingChanged: collectionPage.recountMissing()
            Component.onCompleted: collectionPage.recountMissing()
            Component.onDestruction: collectionPage.recountMissing()
        }

        onCountChanged: collectionPage.recountMissing()
    }

    WarningBanner {
        id: missingBanner
        Layout.fillWidth: true
        kind: "missing"
        missingCount: collectionPage.missingCards
        firstMissingRow: collectionPage.firstMissingCard
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: Theme.space2

        // C-4: "Cari video..." - the one search field; it drives the model's
        // own filter, so the grid never pages through a stale copy.
        TextField {
            id: searchField
            objectName: "collectionSearch"
            Layout.fillWidth: true
            placeholderText: qsTr("Cari video\u2026")
            Accessible.name: placeholderText
            onTextChanged: Library.filter = text
        }

        // C-7 import affordance. Hidden while the C-1 empty state shows its
        // own "+ Pilih video" button - one import button per screen.
        Button {
            id: addVideoButton
            objectName: "addVideoButton"
            visible: !Library.isEmpty
            text: qsTr("+ Tambah video")
            implicitHeight: 40
            focusPolicy: Qt.StrongFocus
            Accessible.name: text
            onClicked: Library.pickAndImport()
            Keys.onReturnPressed: {
                addVideoButton.clicked()
                event.accepted = true
            }
            Keys.onEnterPressed: {
                addVideoButton.clicked()
                event.accepted = true
            }
            contentItem: Text {
                text: addVideoButton.text
                font.pixelSize: Theme.fontM
                color: Theme.text
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            background: Rectangle {
                radius: Theme.radiusS
                color: addVideoButton.hovered || addVideoButton.activeFocus
                       ? Theme.surface2 : Theme.surface
                border.width: addVideoButton.activeFocus ? 2 : 1
                border.color: addVideoButton.activeFocus
                              ? Theme.accent : Theme.surface2
            }
        }
    }

    // C-11 "Ganti otomatis": rotation source, interval, shuffle, live status.
    // Hidden with an empty collection (the C-1 empty state owns the screen).
    AutoSwitchPanel {
        id: autoSwitchPanel
        Layout.fillWidth: true
        visible: !Library.isEmpty
    }

    Label {
        Layout.fillWidth: true
        visible: Library.lastError.length > 0
        wrapMode: Text.WordWrap
        color: Theme.statusError
        text: Library.lastError
    }

    // C-4: "Tidak ada hasil untuk "{q}"" - only when a query matched nothing
    // (an empty library shows the C-1 empty state instead).
    Label {
        objectName: "noResultsLabel"
        Layout.fillWidth: true
        Layout.topMargin: Theme.space3
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
        color: Theme.text2
        font.pixelSize: Theme.fontM
        visible: !Library.isEmpty && Library.visibleCount === 0
        text: qsTr("Tidak ada hasil untuk \"%1\"").arg(Library.filter)
    }

    EmptyState {
        id: emptyState
        Layout.fillWidth: true
        Layout.fillHeight: true
        // Visibility lives in EmptyState.qml (isEmpty + !firstRunActive):
        // do NOT re-bind `visible` here or the gate would be shadowed.
        firstRunActive: collectionPage.firstRunActive
    }

    VideoGrid {
        id: videoGrid
        Layout.fillWidth: true
        Layout.fillHeight: true
        visible: !Library.isEmpty && Library.visibleCount > 0
        dialogOpened: collectionPage.dialogOpened
        dialogClosed: collectionPage.dialogClosed
    }
}
