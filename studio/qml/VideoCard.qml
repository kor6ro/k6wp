// Video card: one library entry in the Koleksi gallery.
//
// Card matrix (brief B-STATE / section 1.3):
//   Normal        - flat card, thumbnail + name
//   Hover/Fokus   - outline + [Pasang] + [....] revealed (mouse hover OR focus)
//   Dipakai       - check badge, the video the engine is currently playing
//   Hilang        - warning badge "File tidak ketemu" + Cari/Hapus dialog
//   Belum siap    - hourglass badge "Perlu disiapkan" (optimized role, todo 6)
//   Progres       - scrim + "Menyiapkan video... {p}%" + Batal (Compress)
//   Error         - warning badge "Tidak bisa dipakai" + Coba lagi
//
// Copy deck: C-4 (hover [Pasang] + [....]), C-5 (row menu), C-6 (TWO separate
// delete dialogs, never merged), C-15 actions for a missing file.
//
// Todo 10 (Pilihan sendiri mode): while Settings.playlistSource == "custom"
// the card carries a selection checkbox. Its checked state is derived from
// Playlist.items (never stored on the card) and toggling it calls
// Playlist.addPaths / Playlist.removeAt for THIS path - the bridge persists
// playlist.json, QML never writes it.
//
// Delegate scoping (todo-2 F9): the row menu and BOTH delete dialogs live in
// this file and read their row from the delegate context - no root.* lookups.
//
// Overlay rule B-WIREFRAME(f): the [....] menu anchors BELOW the card (never
// over the preview hole above the gallery); every Dialog feeds
// dialogOpened/dialogClosed so the native preview steps aside while it is up.
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Item {
    id: videoCard

    // --- delegate context: model roles (todo 6) ---------------------------
    required property int index
    required property string displayName
    required property string thumbUrl
    required property string dst
    required property bool missing
    required property bool optimized

    // --- plumbing from VideoGrid / CollectionPage -------------------------
    property int cellWidth: 250
    property int cellHeight: 238
    property var dialogOpened: function () {}
    property var dialogClosed: function () {}
    property var navigateToCompressor: function () {}

    width: cellWidth
    height: cellHeight

    // Keyboard contract: the card itself is a tab stop, Enter/Space install.
    activeFocusOnTab: true

    // Windows paths arrive with either separator (model dst uses '\', the
    // engine config and Compress use '/'), so "same file" is normalised
    // before comparing.
    function samePath(a, b) {
        return String(a).replace(/\\/g, "/").toLowerCase()
                === String(b).replace(/\\/g, "/").toLowerCase()
    }

    readonly property bool used: dst.length > 0
                                 && samePath(dst, Studio.activeVideoPath)
    readonly property bool notReady: !missing && !optimized
    // Progress/error belong to THIS card only while the bridge's source path
    // is this file; another card's job must never paint here.
    readonly property bool preparing: Compress.running
                                      && samePath(Compress.sourcePath, dst)
    readonly property bool failed: !Compress.running && !missing
                                   && Compress.lastError.length > 0
                                   && samePath(Compress.sourcePath, dst)
    // The buttons must keep the strip revealed while the pointer is ON one of
    // them: a Button's hover handler wins over cardMouse, so containsMouse
    // alone flips false the moment the cursor reaches Pasang/.... and the
    // button would vanish from under the click.
    readonly property bool hovered: cardMouse.containsMouse
                                    || pasangButton.hovered
                                    || menuButton.hovered
                                    || retryButton.hovered
    readonly property bool focusWithin: videoCard.activeFocus
                                        || pasangButton.activeFocus
                                        || menuButton.activeFocus
                                        || retryButton.activeFocus
    readonly property bool revealed: hovered || focusWithin

    // --- Pilihan sendiri selection (todo 10) ------------------------------
    // The checkbox exists only in custom-source mode; membership is
    // recomputed from the Playlist bridge so an add/remove from the row menu
    // ("Tambah ke Ganti otomatis") also flips it.
    readonly property bool selectionMode: Settings.playlistSource === "custom"
    readonly property int playlistRow: playlistRowFor(dst)

    function playlistRowFor(path) {
        const items = Playlist.items
        for (let i = 0; i < items.length; ++i) {
            if (samePath(items[i].path, path))
                return i
        }
        return -1
    }

    // Called by cardSelectCheck on user activation. The row is looked up
    // fresh (never cached): a previous toggle changed the indices.
    function togglePlaylistSelection() {
        const row = playlistRowFor(dst)
        if (row >= 0) {
            Playlist.removeAt(row)
            return
        }
        if (!missing)
            Playlist.addPaths([dst])
    }

    // EntryLabel's displayName is "name\nres - duration"; dialogs show the
    // name alone (C-6 "{nama}").
    readonly property string fileName: String(displayName).split("\n")[0]

    // Status badges carry icon + label, never colour alone (brief B-TOKEN).
    readonly property string stateLabel: missing ? qsTr("File tidak ketemu")
                                        : preparing ? qsTr("Menyiapkan")
                                        : failed ? qsTr("Tidak bisa dipakai")
                                        : used ? qsTr("Dipakai")
                                        : notReady ? qsTr("Perlu disiapkan")
                                        : ""

    Accessible.role: Accessible.ListItem
    Accessible.name: stateLabel.length > 0
                     ? displayName + " \u2014 " + stateLabel
                     : displayName

    // A broken entry cannot be installed: its action is the C-15 dialog.
    function activate() {
        if (missing) {
            missingDialog.open()
            return
        }
        Library.applyAt(index)
    }

    Keys.onPressed: function (event) {
        if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter
                || event.key === Qt.Key_Space) {
            activate()
            event.accepted = true
        }
    }

    // The cached JPEG only exists after ensureThumbnail; an Image with no
    // source never changes status, so the first request has to come from
    // here, and later failures retry through onStatusChanged.
    Component.onCompleted: if (thumbUrl.length === 0) Library.ensureThumbnail(index)

    // Inline card button: token colours, a real 40px target and a 2px focus
    // ring so keyboard focus is always visible (a11y contract).
    component CardButton: Button {
        id: cardButton
        property bool primary: false
        implicitHeight: 40
        focusPolicy: Qt.StrongFocus
        leftPadding: Theme.space2
        rightPadding: Theme.space2
        Accessible.name: text
        Keys.onReturnPressed: {
            cardButton.clicked()
            event.accepted = true
        }
        Keys.onEnterPressed: {
            cardButton.clicked()
            event.accepted = true
        }
        contentItem: Text {
            text: cardButton.text
            font.pixelSize: Theme.fontM
            font.weight: cardButton.primary ? Theme.fontWeightSemibold
                                            : Theme.fontWeightRegular
            color: cardButton.primary ? Theme.accentText : Theme.text
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
        background: Rectangle {
            radius: Theme.radiusS
            color: cardButton.primary
                   ? Theme.accent
                   : (cardButton.hovered || cardButton.activeFocus
                      ? Theme.surface2 : Theme.surface)
            border.width: cardButton.activeFocus ? 2 : 0
            border.color: cardButton.primary ? Theme.accentText : Theme.accent
        }
    }

    // Inline status badge: tinted surface + glyph + label.
    component CardBadge: Rectangle {
        id: badge
        property string glyph: ""
        property string label: ""
        property color tint: Theme.surface2
        property color ink: Theme.text
        color: badge.tint
        radius: Theme.radiusS
        implicitWidth: badgeRow.implicitWidth + 2 * Theme.space1
        implicitHeight: badgeRow.implicitHeight + Theme.space1
        RowLayout {
            id: badgeRow
            anchors.centerIn: parent
            spacing: Theme.space1
            Label {
                text: badge.glyph
                color: badge.ink
                font.pixelSize: Theme.fontS
                Accessible.ignored: true
            }
            Label {
                text: badge.label
                color: badge.ink
                font.pixelSize: Theme.fontS
                font.weight: Theme.fontWeightSemibold
            }
        }
    }

    // Inline playlist checkbox (todo 10, custom source only): a 40px target
    // with a token-styled indicator and a visible focus ring. onClicked
    // handles mouse AND Space; Enter gets its own handler (Controls never
    // maps Enter to click). togglePlaylistSelection() derives the action from
    // Playlist.items, so a binding-driven checked change can never add or
    // remove an entry by itself.
    component CardCheck: CheckBox {
        id: cardCheck
        implicitWidth: 40
        implicitHeight: 40
        focusPolicy: Qt.StrongFocus
        Accessible.name: qsTr("Ganti otomatis") + " \u2014 " + videoCard.fileName
        onClicked: videoCard.togglePlaylistSelection()
        Keys.onReturnPressed: {
            videoCard.togglePlaylistSelection()
            event.accepted = true
        }
        Keys.onEnterPressed: {
            videoCard.togglePlaylistSelection()
            event.accepted = true
        }
        indicator: Rectangle {
            anchors.centerIn: parent
            width: 24
            height: 24
            radius: Theme.radiusS
            color: cardCheck.checked
                   ? Theme.accent
                   : (cardCheck.hovered || cardCheck.activeFocus
                      ? Theme.surface2 : Theme.bg)
            border.width: cardCheck.activeFocus ? 2 : 1
            border.color: cardCheck.activeFocus || cardCheck.checked
                          ? Theme.accent : Theme.text2

            Text {
                anchors.centerIn: parent
                visible: cardCheck.checked
                text: "\u2713"
                color: Theme.accentText
                font.pixelSize: Theme.fontM
                font.weight: Theme.fontWeightSemibold
                Accessible.ignored: true
            }
        }
        contentItem: Item {}
    }

    Rectangle {
        id: frame
        anchors.fill: parent
        anchors.margins: Theme.space1
        radius: Theme.radiusM
        color: videoCard.revealed ? Theme.surface2 : Theme.surface
        // Focus ring is accent (the one allowed coloured edge, per a11y);
        // hover gets a neutral outline.
        border.width: videoCard.activeFocus ? 2 : (videoCard.hovered ? 1 : 0)
        border.color: videoCard.activeFocus ? Theme.accent : Theme.text2
    }

    // Whole-card click surface ("klik untuk memasangnya", C-1); the hover
    // buttons above it consume their own clicks.
    MouseArea {
        id: cardMouse
        anchors.fill: frame
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton
        onClicked: videoCard.activate()
    }

    ColumnLayout {
        anchors.fill: frame
        anchors.margins: Theme.space2
        spacing: Theme.space1

        // --- thumbnail (empty slot until the cached JPEG lands) -----------
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 100
            clip: true

            Rectangle {
                anchors.fill: parent
                radius: Theme.radiusS
                color: Theme.surface2
                Accessible.ignored: true
            }

            Image {
                anchors.fill: parent
                asynchronous: true
                cache: true
                fillMode: Image.PreserveAspectCrop
                visible: status === Image.Ready
                source: videoCard.thumbUrl
                onStatusChanged: if (status !== Image.Ready)
                                     Library.ensureThumbnail(videoCard.index)
            }

            ColumnLayout {
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.margins: Theme.space1
                spacing: Theme.space1

                CardBadge {
                    visible: videoCard.used && !videoCard.missing
                    glyph: "\u2713"
                    label: qsTr("Dipakai")
                    tint: Theme.statusActiveTint
                    ink: Theme.statusActive
                }

                CardBadge {
                    objectName: "missingBadge"
                    visible: videoCard.missing
                    glyph: "\u26A0\uFE0E"
                    label: qsTr("File tidak ketemu")
                    tint: Theme.statusErrorTint
                    ink: Theme.statusError
                    MouseArea {
                        anchors.fill: parent
                        onClicked: missingDialog.open()
                    }
                }

                CardBadge {
                    visible: videoCard.failed && !videoCard.preparing
                    glyph: "\u26A0\uFE0E"
                    label: qsTr("Tidak bisa dipakai")
                    tint: Theme.statusErrorTint
                    ink: Theme.statusError
                }

                CardBadge {
                    visible: videoCard.notReady && !videoCard.preparing
                             && !videoCard.failed
                    glyph: "\u23F3"
                    label: qsTr("Perlu disiapkan")
                    tint: Theme.statusPausedTint
                    ink: Theme.statusPaused
                }
            }

            // Selection checkbox: top-right corner, always visible in
            // Pilihan sendiri mode (selection is that mode's primary action,
            // so it must not hide behind hover).
            CardCheck {
                id: cardSelectCheck
                objectName: "cardSelectCheck"
                visible: videoCard.selectionMode && !videoCard.missing
                checked: videoCard.playlistRow >= 0
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.margins: Theme.space1
            }

            // "Sedang disiapkan" overlay: progress + Batal (B-STATE matrix).
            Rectangle {
                anchors.fill: parent
                radius: Theme.radiusS
                color: Theme.bg
                opacity: 0.85
                visible: videoCard.preparing
            }

            ColumnLayout {
                anchors.centerIn: parent
                visible: videoCard.preparing
                spacing: Theme.space1

                Label {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("Menyiapkan video\u2026 %1%").arg(Compress.progress)
                    font.pixelSize: Theme.fontM
                    color: Theme.text
                }

                CardButton {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("Batal")
                    onClicked: Compress.cancel()
                }
            }
        }

        // --- display name (todo-6 role, never the legacy badge label) -----
        Label {
            Layout.fillWidth: true
            maximumLineCount: 2
            elide: Text.ElideRight
            wrapMode: Text.WordWrap
            font.pixelSize: Theme.fontM
            color: Theme.text
            text: videoCard.displayName
        }

        // --- actions strip: reserved height, so reveal causes no jump -----
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            Layout.minimumHeight: 40

            RowLayout {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                spacing: Theme.space1

                CardButton {
                    id: retryButton
                    visible: videoCard.failed
                    text: qsTr("Coba lagi")
                    onClicked: {
                        Compress.setSourcePath(videoCard.dst)
                        Compress.start()
                    }
                }

                CardButton {
                    id: pasangButton
                    visible: videoCard.revealed
                    primary: true
                    text: qsTr("Pasang")
                    Accessible.name: qsTr("Pasang") + " " + videoCard.displayName
                    onClicked: videoCard.activate()
                }

                CardButton {
                    id: menuButton
                    objectName: "cardMenuButton"
                    visible: videoCard.revealed
                    implicitWidth: 40
                    text: "\u22EF"
                    Accessible.name: qsTr("Menu kartu")
                    // open(), not popup(): popup() repositions the menu at the
                    // mouse cursor / window origin, while open() honours the
                    // menu's own x/y (below the card, per B-WIREFRAME(f)).
                    onClicked: rowMenu.open()
                }
            }
        }
    }

    // --- C-5 row menu ------------------------------------------------------
    // Opens downward (below the card): the overlay matrix forbids covering
    // the preview hole above the gallery.
    Menu {
        id: rowMenu
        y: videoCard.height
        x: Math.max(0, videoCard.width - width)

        MenuItem {
            text: qsTr("Pasang")
            onTriggered: videoCard.activate()
        }
        MenuItem {
            text: qsTr("Tambah ke Ganti otomatis")
            enabled: !videoCard.missing
            onTriggered: Playlist.addPaths([videoCard.dst])
        }
        MenuItem {
            text: qsTr("Perkecil")
            enabled: !videoCard.missing
            onTriggered: {
                Compress.setSourcePath(videoCard.dst)
                videoCard.navigateToCompressor()
            }
        }
        MenuItem {
            text: qsTr("Buka lokasi")
            onTriggered: Library.openLocationAt(videoCard.index)
        }
        MenuSeparator {}
        MenuItem {
            text: qsTr("Hapus dari koleksi")
            onTriggered: removeCollectionDialog.open()
        }
        MenuItem {
            text: qsTr("Hapus file ke Recycle Bin")
            onTriggered: removeTrashDialog.open()
        }
    }

    // --- C-6(a): remove the entry; the file itself stays --------------------
    Dialog {
        id: removeCollectionDialog
        objectName: "removeCollectionDialog"
        title: qsTr("Hapus dari koleksi?")
        modal: true
        anchors.centerIn: Overlay.overlay
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        onOpened: {
            videoCard.dialogOpened()
            collectionCancelButton.forceActiveFocus()
        }
        onClosed: videoCard.dialogClosed()

        contentItem: ColumnLayout {
            spacing: Theme.space1
            Accessible.role: Accessible.Dialog
            Accessible.name: removeCollectionDialog.title

            Label {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                text: qsTr("File aslinya tetap ada.")
            }
            Label {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                font.weight: Theme.fontWeightSemibold
                text: videoCard.fileName
            }
        }

        footer: RowLayout {
            spacing: Theme.space2
            Item {
                Layout.fillWidth: true
            }
            CardButton {
                objectName: "removeCollectionConfirm"
                text: qsTr("Hapus dari koleksi")
                onClicked: {
                    removeCollectionDialog.close()
                    Library.removeAt(videoCard.index, false)
                }
            }
            CardButton {
                id: collectionCancelButton
                objectName: "removeCollectionCancel"
                text: qsTr("Batal")
                onClicked: removeCollectionDialog.close()
            }
        }
    }

    // --- C-6(b): move the FILE itself to the Recycle Bin --------------------
    Dialog {
        id: removeTrashDialog
        objectName: "removeTrashDialog"
        title: qsTr("Hapus file ke Recycle Bin?")
        modal: true
        anchors.centerIn: Overlay.overlay
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        onOpened: {
            videoCard.dialogOpened()
            trashCancelButton.forceActiveFocus()
        }
        onClosed: videoCard.dialogClosed()

        contentItem: ColumnLayout {
            spacing: Theme.space1
            Accessible.role: Accessible.Dialog
            Accessible.name: removeTrashDialog.title

            Label {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                text: qsTr("File akan hilang dari komputermu.")
            }
            Label {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                font.weight: Theme.fontWeightSemibold
                text: videoCard.fileName
            }
        }

        footer: RowLayout {
            spacing: Theme.space2
            Item {
                Layout.fillWidth: true
            }
            CardButton {
                objectName: "removeTrashConfirm"
                text: qsTr("Hapus file")
                onClicked: {
                    removeTrashDialog.close()
                    Library.removeAt(videoCard.index, true)
                }
            }
            CardButton {
                id: trashCancelButton
                objectName: "removeTrashCancel"
                text: qsTr("Batal")
                onClicked: removeTrashDialog.close()
            }
        }
    }

    // --- missing-file action dialog (C-15 actions) --------------------------
    Dialog {
        id: missingDialog
        objectName: "missingDialog"
        title: qsTr("File tidak ketemu")
        modal: true
        anchors.centerIn: Overlay.overlay
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        onOpened: videoCard.dialogOpened()
        onClosed: videoCard.dialogClosed()

        contentItem: ColumnLayout {
            spacing: Theme.space1
            Accessible.role: Accessible.Dialog
            Accessible.name: missingDialog.title

            Label {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                text: qsTr("%1 tidak ketemu (mungkin dipindah).")
                      .arg(videoCard.fileName)
            }
        }

        footer: RowLayout {
            spacing: Theme.space2
            Item {
                Layout.fillWidth: true
            }
            CardButton {
                objectName: "missingFindFile"
                text: qsTr("Cari file")
                onClicked: {
                    missingDialog.close()
                    Library.openLocationAt(videoCard.index)
                }
            }
            CardButton {
                objectName: "missingRemoveEntry"
                text: qsTr("Hapus dari koleksi")
                onClicked: {
                    missingDialog.close()
                    removeCollectionDialog.open()
                }
            }
        }
    }
}
