// Video card: one library entry in the Koleksi gallery.
//
// Card matrix (brief B-STATE / section 1.3):
//   Normal        - flat card, thumbnail + name
//   Hover/Fokus   - outline + [Pasang] + [....] revealed (mouse hover OR focus)
//   Dipakai       - check badge, the video the engine is currently playing
//   Hilang        - warning badge "File tidak ketemu" + Cari/Hapus dialog
//   Belum siap    - hourglass badge "Perlu disiapkan" (optimized role, todo 6)
//   Progres       - scrim + "Menyiapkanâ€¦ {p}%" + Ã— cancel (Compress.cancel)
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
//
// Task 27 (select-only click): a single click - or Enter/Space on the focused
// card - SELECTS the card (accent outline + "Terpilih" badge, stored as
// Studio.selectedVideo, the clean selected-path surface the right-rail
// "Pasang" reads). It never applies. Applying stays explicit: hover [Pasang],
// the C-5 "Pasang" row-menu item, the right-rail action row, or the
// documented double-click power path (README: "double-click live-switch").
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

    width: cellWidth
    height: cellHeight

    // Keyboard contract (task 27): the card is a tab stop; Enter/Space SELECT
    // it. The wallpaper is only ever installed by an explicit "Pasang".
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

    // --- selected card (task 27 select-only click) -------------------------
    // Derived from the bridge, never stored on the delegate: a card is selected
    // while its dst is Studio.selectedVideo (the same picked-then-applied
    // surface the right-rail "Pasang" reads), so "Hapus" in that rail clears
    // the ring here too.
    readonly property bool selected: dst.length > 0
                                     && samePath(dst, Studio.selectedVideo)

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
    Accessible.name: {
        const parts = []
        if (videoCard.selected)
            parts.push(qsTr("Terpilih"))
        if (videoCard.stateLabel.length > 0)
            parts.push(videoCard.stateLabel)
        return parts.length > 0
                ? videoCard.displayName + " \u2014 " + parts.join(" \u2014 ")
                : videoCard.displayName
    }

    // Explicit apply path (hover [Pasang], â‹¯ "Pasang", double-click): a broken
    // entry cannot be installed, so its action is the C-15 dialog instead.
    function activate() {
        if (missing) {
            missingDialog.open()
            return
        }
        Library.applyAt(index)
    }

    // Select-only click / Enter / Space (task 27): publishes this row's dst to
    // Studio.selectedVideo via the bridge. Never applies. A missing entry keeps
    // its established C-15 dialog action (nothing installable to select).
    function select() {
        if (missing) {
            missingDialog.open()
            return
        }
        Studio.selectVideo(videoCard.dst)
    }

    Keys.onPressed: function (event) {
        if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter
                || event.key === Qt.Key_Space) {
            select()
            event.accepted = true
        }
    }

    // The cached JPEG only exists after ensureThumbnail; an Image with no
    // source never changes status, so the first request has to come from
    // here, and later failures retry through onStatusChanged.
    Component.onCompleted: if (thumbUrl.length === 0) Library.ensureThumbnail(index)

    // Inline card button: token colours, a shared Theme.glyph icon, a real
    // 40px target and a 2px focus ring so keyboard focus is always visible
    // (a11y contract). Non-primary focus is ink (Theme.focusRing), never
    // blue; pressing steps the surface.
    component CardButton: Button {
        id: cardButton
        property bool primary: false
        property string glyph: ""
        implicitHeight: 40
        // Task 33: zero vertical padding keeps the 40dp content box centred
        // (Material's verticalPadding otherwise squeezed it to 12dp and sat
        // the label 1px below the control centre).
        topPadding: 0
        bottomPadding: 0
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
        contentItem: RowLayout {
            spacing: Theme.space1

            Text {
                visible: cardButton.glyph.length > 0
                Layout.alignment: Qt.AlignVCenter
                text: cardButton.glyph
                font.family: Theme.glyphFont
                font.pixelSize: Theme.fontM
                color: cardButton.primary ? Theme.accentText : Theme.text
                verticalAlignment: Text.AlignVCenter
                Accessible.ignored: true
            }

            Text {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
                text: cardButton.text
                font.pixelSize: Theme.fontM
                font.weight: cardButton.primary ? Theme.fontWeightSemibold
                                                : Theme.fontWeightRegular
                color: cardButton.primary ? Theme.accentText : Theme.text
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }
        }
        background: Rectangle {
            radius: Theme.radiusS
            color: cardButton.primary
                   ? (cardButton.down ? Qt.darker(Theme.accent, 1.15)
                                      : Theme.accent)
                   : (cardButton.down ? Theme.pressedSurface
                      : (cardButton.hovered || cardButton.activeFocus
                         ? Theme.surface2 : Theme.surface))
            border.width: cardButton.activeFocus ? 2 : 0
            border.color: cardButton.primary ? Theme.accentText
                                             : Theme.focusRing
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
        // Task 33: symmetric 4dp vertical inset (this read `+ Theme.space1`,
        // half the horizontal inset, so the badge read squashed).
        implicitHeight: badgeRow.implicitHeight + 2 * Theme.space1
            RowLayout {
                id: badgeRow
                anchors.centerIn: parent
                spacing: Theme.space1
                Label {
                    text: badge.glyph
                    font.family: Theme.glyphFont
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
                   : (cardCheck.down ? Theme.pressedSurface
                      : (cardCheck.hovered || cardCheck.activeFocus
                         ? Theme.surface2 : Theme.bg))
            border.width: cardCheck.activeFocus || cardCheck.checked ? 2 : 1
            // Checked keeps the accent marker (selection state, not a focus
            // highlight); keyboard focus alone rings in ink.
            border.color: cardCheck.checked
                          ? Theme.accent
                          : (cardCheck.activeFocus ? Theme.focusRing
                                                   : Theme.text2)

            Text {
                anchors.centerIn: parent
                visible: cardCheck.checked
                text: Theme.glyph.check
                font.family: Theme.glyphFont
                color: Theme.accentText
                font.pixelSize: Theme.fontM
                font.weight: Theme.fontWeightSemibold
                Accessible.ignored: true
            }
        }
        contentItem: Item {}
    }

    // Row-menu item (task 27; task 31 glyphs): the default Material menu
    // painted a light surface and light item text in dark mode. Every
    // surface, ink and highlight below comes from Theme tokens, the shared
    // Theme.glyph icon leads each row, and the 40px row keeps the
    // keyboard/touch target contract.
    component CardMenuItem: MenuItem {
        id: cardMenuItem
        property string glyph: ""
        implicitHeight: 40
        // Task 33: 40dp menu rows centre exactly (see CardButton).
        topPadding: 0
        bottomPadding: 0
        contentItem: RowLayout {
            spacing: Theme.space2

            Text {
                visible: cardMenuItem.glyph.length > 0
                Layout.alignment: Qt.AlignVCenter
                text: cardMenuItem.glyph
                font.family: Theme.glyphFont
                font.pixelSize: Theme.fontM
                color: Theme.text2
                verticalAlignment: Text.AlignVCenter
                Accessible.ignored: true
            }

            Text {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
                text: cardMenuItem.text
                font.pixelSize: Theme.fontM
                color: cardMenuItem.enabled ? Theme.text : Theme.text2
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }
        }
        background: Rectangle {
            radius: Theme.radiusS
            color: cardMenuItem.highlighted ? Theme.surface2 : "transparent"
        }
    }

    Rectangle {
        id: frame
        anchors.fill: parent
        anchors.margins: Theme.space1
        radius: Theme.radiusM
        color: videoCard.revealed || videoCard.selected ? Theme.surface2
                                                        : Theme.surface
        // Focus ring is ink (Theme.focusRing); hover gets a neutral outline;
        // the selected card keeps the accent edge + "Terpilih" badge so one
        // click reads as selected even after focus moves away (task 27
        // contract: the one accent selection outline that must stay).
        border.width: videoCard.activeFocus || videoCard.selected ? 2
                                     : (videoCard.hovered ? 1 : 0)
        border.color: videoCard.selected ? Theme.accent
                     : videoCard.activeFocus ? Theme.focusRing
                     : Theme.text2
    }

    // Whole-card click surface (task 27): one click SELECTS the card; the
    // wallpaper is only ever installed by an explicit "Pasang" (hover button,
    // â‹¯ menu item, right-rail action). Double-click keeps the README's
    // "double-click live-switch" shortcut and applies deliberately.
    MouseArea {
        id: cardMouse
        anchors.fill: frame
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton
        onClicked: videoCard.select()
        onDoubleClicked: videoCard.activate()
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

                // Selection badge (task 27): accent pair, so the selected card
                // is identifiable by label + outline, never colour alone.
                CardBadge {
                    objectName: "selectedBadge"
                    visible: videoCard.selected
                    glyph: Theme.glyph.check
                    label: qsTr("Terpilih")
                    tint: Theme.accent
                    ink: Theme.accentText
                }

                CardBadge {
                    visible: videoCard.used && !videoCard.missing
                    glyph: Theme.glyph.check
                    label: qsTr("Dipakai")
                    tint: Theme.statusActiveTint
                    ink: Theme.statusActive
                }

                CardBadge {
                    objectName: "missingBadge"
                    visible: videoCard.missing
                    glyph: Theme.glyph.warning
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
                    glyph: Theme.glyph.warning
                    label: qsTr("Tidak bisa dipakai")
                    tint: Theme.statusErrorTint
                    ink: Theme.statusError
                }

                CardBadge {
                    visible: videoCard.notReady && !videoCard.preparing
                             && !videoCard.failed
                    glyph: Theme.glyph.hourglass
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

            // "Sedang disiapkan" overlay (todo 13 / B-STATE matrix): the
            // scrim carries "Menyiapkanâ€¦ N%" (D3 percent when available) and
            // a Ã— cancel button. The toast at the window bottom offers the
            // same cancel via "Batal"; both call Compress.cancel().
            Rectangle {
                id: preparingOverlay
                objectName: "preparingOverlay"
                anchors.fill: parent
                radius: Theme.radiusS
                color: Theme.bg
                opacity: 0.85
                visible: videoCard.preparing
            }

            Label {
                id: preparingLabel
                objectName: "preparingLabel"
                anchors.centerIn: parent
                visible: videoCard.preparing
                text: qsTr("Menyiapkan\u2026 %1%").arg(Compress.progress)
                font.pixelSize: Theme.fontM
                font.weight: Theme.fontWeightSemibold
                color: Theme.text
                Accessible.name: text
            }

            // Ã— cancel: 40px target, Enter/Space, Accessible.name. Top-right
            // of the thumbnail so it never collides with the status badges.
            Button {
                id: preparingCancel
                objectName: "preparingCancel"
                visible: videoCard.preparing
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.margins: Theme.space1
                implicitWidth: 40
                implicitHeight: 40
                // Task 33: explicit zero padding on the glyph-only cancel
                // (Material's verticalPadding otherwise squeezed the content
                // box to 12dp).
                topPadding: 0
                bottomPadding: 0
                leftPadding: 0
                rightPadding: 0
                focusPolicy: Qt.StrongFocus
                text: Theme.glyph.close
                Accessible.name: qsTr("Batal menyiapkan")
                onClicked: Compress.cancel()

                Keys.onReturnPressed: {
                    Compress.cancel()
                    event.accepted = true
                }
                Keys.onEnterPressed: {
                    Compress.cancel()
                    event.accepted = true
                }

                contentItem: Text {
                    text: preparingCancel.text
                    font.family: Theme.glyphFont
                    font.pixelSize: Theme.fontL
                    font.weight: Theme.fontWeightSemibold
                    color: Theme.text
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    radius: Theme.radiusS
                    color: preparingCancel.down ? Theme.pressedSurface
                           : (preparingCancel.hovered
                              || preparingCancel.activeFocus
                              ? Theme.surface2 : Theme.surface)
                    border.width: preparingCancel.activeFocus ? 2 : 1
                    border.color: preparingCancel.activeFocus
                                  ? Theme.focusRing : Theme.text2
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
                    glyph: Theme.glyph.refresh
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
                    glyph: Theme.glyph.check
                    Accessible.name: qsTr("Pasang") + " " + videoCard.displayName
                    onClicked: videoCard.activate()
                }

                CardButton {
                    id: menuButton
                    objectName: "cardMenuButton"
                    visible: videoCard.revealed
                    implicitWidth: 40
                    text: Theme.glyph.menu
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
        // Parented to the [....] button (the proven StatusBar Hapus-menu
        // pattern): a delegate-owned popup anchored to a live control opens
        // reliably, and the button is the card's bottom-right corner, so the
        // menu still opens below the card (B-WIREFRAME(f)).
        parent: menuButton
        // Below the button by default; a bottom-row card has no room below,
        // so the menu then opens above the button - still inside the
        // collection area, never over the preview hole. Decided on
        // aboutToShow: the items (and implicitHeight) exist by then.
        onAboutToShow: {
            const overlay = rowMenu.Overlay.overlay
            const sceneBottom = menuButton.mapToItem(null, 0, menuButton.height).y
            if (overlay && sceneBottom + rowMenu.implicitHeight > overlay.height)
                y = -(rowMenu.implicitHeight + Theme.space1)
            else
                y = menuButton.height + Theme.space1
        }
        x: menuButton.width - width

        background: Rectangle {
            // Explicit implicit size: a custom background with 0 implicit
            // size collapses the popup to nothing and it never shows.
            implicitWidth: 240
            implicitHeight: 260
            radius: Theme.radiusM
            color: Theme.bg
            border.width: 1
            border.color: Theme.surface2
        }

        CardMenuItem {
            glyph: Theme.glyph.check
            text: qsTr("Pasang")
            onTriggered: videoCard.activate()
        }
        CardMenuItem {
            glyph: Theme.glyph.plus
            text: qsTr("Tambah ke Ganti otomatis")
            enabled: !videoCard.missing
            onTriggered: Playlist.addPaths([videoCard.dst])
        }
        CardMenuItem {
            glyph: Theme.glyph.shrink
            text: qsTr("Perkecil")
            enabled: !videoCard.missing
            // Todo 13: compression runs fully in the background. "Perkecil"
            // sets the source and starts the job; the ToastBar at the window
            // bottom carries the progress + Batal, and long videos raise the
            // C-9 consent dialog through AppDialogs.
            onTriggered: {
                Compress.setSourcePath(videoCard.dst)
                Compress.start()
            }
        }
        CardMenuItem {
            glyph: Theme.glyph.external
            text: qsTr("Buka lokasi")
            onTriggered: Library.openLocationAt(videoCard.index)
        }
        MenuSeparator {
            contentItem: Rectangle {
                implicitHeight: 1
                color: Theme.surface2
            }
        }
        CardMenuItem {
            glyph: Theme.glyph.remove
            text: qsTr("Hapus dari koleksi")
            onTriggered: removeCollectionDialog.open()
        }
        CardMenuItem {
            glyph: Theme.glyph.deleteFile
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
        // Task 32: tighter than the Material 24 default.
        padding: Theme.space3
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
                glyph: Theme.glyph.remove
                onClicked: {
                    removeCollectionDialog.close()
                    Library.removeAt(videoCard.index, false)
                }
            }
            CardButton {
                id: collectionCancelButton
                objectName: "removeCollectionCancel"
                text: qsTr("Batal")
                glyph: Theme.glyph.close
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
        // Task 32: tighter than the Material 24 default.
        padding: Theme.space3
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
                glyph: Theme.glyph.deleteFile
                onClicked: {
                    removeTrashDialog.close()
                    Library.removeAt(videoCard.index, true)
                }
            }
            CardButton {
                id: trashCancelButton
                objectName: "removeTrashCancel"
                text: qsTr("Batal")
                glyph: Theme.glyph.close
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
        // Task 32: tighter than the Material 24 default.
        padding: Theme.space3
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
                glyph: Theme.glyph.external
                onClicked: {
                    missingDialog.close()
                    Library.openLocationAt(videoCard.index)
                }
            }
            CardButton {
                objectName: "missingRemoveEntry"
                text: qsTr("Hapus dari koleksi")
                glyph: Theme.glyph.remove
                onClicked: {
                    missingDialog.close()
                    removeCollectionDialog.open()
                }
            }
        }
    }
}

