// Status bar: the Wallpaper right-rail summary. Shows the friendly state
// (Studio.statusTitle) plus the active video's display name
// (Studio.statusVideoName) and exactly ONE action row with four compact
// controls (task 28):
//   [Jeda/Lanjut] [Pilih video] [Pasang] [Hapus]
//
// Copy deck C-3 verbatim; the words come from the todo-4 bridge surface
// (StatusTitleFor / StatusVideoNameFor) - QML never re-derives the status.
// Raw support details (process id, paths, log) live in the "Info teknis"
// dialog (AppDialogs.qml) and never render on the main screen.
//
// Task 28 merge: the old full-width status button (Jeda/Lanjut/Pilih video)
// and ActionButtons' separate grid rows (Pilih video / Pasang / Hapus)
// became this single row. The pause slot no longer doubles as "Pilih video" -
// that action has its own permanent button - so the row composition is stable
// in every state. The glossary §5 recovery action ("Aktifkan lagi", visible
// only while the engine is down) moved next to the status sentence instead of
// growing the row to five buttons, and the app-wide busy spinner is gone with
// the removed grid: every button already disables on Studio.busy.
//
// "Pasang" and "Hapus" act on the selected-path property owned by the card
// selection slice (Studio.selectedVideo; this pane keeps no copy). "Hapus"
// keeps the C-6 dual-delete semantics: a chooser menu offers exactly
// "Hapus dari koleksi" and "Hapus file ke Recycle Bin", each confirmed by its
// own C-6 dialog before Library.removeAt(row, moveToTrash) runs - the same
// model invokable the gallery cards use, delete logic untouched.
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Pane {
    id: statusBar

    // BridgeStatusKind::kPaused (studio_bridge.hpp). Passed in from Main's
    // StatusKind enum so this component never hardcodes the mirror.
    property int statusKindPaused: 1
    // BridgeStatusKind::kNotRunning: shows the "Aktifkan lagi" recovery
    // button next to the status sentence.
    property int statusKindNotRunning: 3
    // WallpaperPage.installVideo: the one install entry point (multi-screen
    // "Pasang ke" target selection + C-14 confirmation). Pasang must route
    // through it instead of calling applyWallpaper directly.
    property var installVideo: function (path) {}
    // Overlay contract B-WIREFRAME(f): the native mpv preview steps aside
    // for every dialog opened from this pane.
    property var dialogOpened: function () {}
    property var dialogClosed: function () {}

    Layout.fillWidth: true
    Material.elevation: 1
    // Task 32: tightened from 12 to the 8 step - the rail reads compact now.
    padding: Theme.space2

    // All three flags derive from the bridge surface, never recomputed:
    // engineStatusKind is the stable int contract (Main.qml enum owns the
    // mirror), engineRunning and videoActive are the bridge's own booleans.
    readonly property bool paused: Studio.engineStatusKind === statusKindPaused
    readonly property bool active: !paused && Studio.engineRunning && Studio.videoActive
    readonly property bool idle: !paused && !active

    // B-TOKEN status colours. The glyph and the text label both carry the
    // state, so colour is never the only signal.
    readonly property color statusColor: paused ? Theme.statusPaused
                                       : active ? Theme.statusActive
                                       : Theme.statusIdle
    // Shared Theme.glyph tokens (task 31): the text-presentation selector
    // inside each token keeps these as glyphs, not emoji.
    readonly property string statusGlyph: paused ? Theme.glyph.pause
                                        : active ? Theme.glyph.play
                                        : Theme.glyph.idle

    // C-3: "Wallpaper aktif • {nama}" / "Dijeda • {nama}" /
    // "Tidak aktif — pilih video untuk mulai". Composed from the bridge's
    // derived properties only.
    readonly property string statusLine: {
        const name = Studio.statusVideoName
        if (name.length > 0)
            return Studio.statusTitle + " • " + name
        if (idle)
            return Studio.statusTitle + " — " + qsTr("pilih video untuk mulai")
        return Studio.statusTitle
    }

    // Jeda/Lanjut slot of the action row. "Pilih video" is a button of its
    // own now, so idle keeps the Jeda label, disabled: there is nothing to
    // pause before a video is installed.
    readonly property string toggleLabel: paused ? qsTr("Lanjut")
                                                 : qsTr("Jeda")

    // The pause/resume action. Space activates through the Button itself;
    // Return / Enter are handled explicitly below so every style covers both.
    function activate() {
        if (paused || active)
            Studio.togglePause()
    }

    // --- Hapus target (task 28) -------------------------------------------
    // Windows paths arrive with either separator (model dst uses '\', the
    // engine config and Compress use '/'), so "same file" is normalised
    // before comparing - the same rule VideoCard.samePath uses.
    function samePath(a, b) {
        return String(a).replace(/\\/g, "/").toLowerCase()
                === String(b).replace(/\\/g, "/").toLowerCase()
    }

    // Row of the selected path inside the (filtered) Library model; -1 when
    // nothing is selected or the selection is not a collection entry. The
    // binding re-evaluates on the selection, model and filter changes.
    readonly property int selectedLibraryRow: {
        const path = String(Studio.selectedVideo)
        const count = Library.count
        const filter = Library.filter
        if (path.length === 0)
            return -1
        for (let i = 0; i < count; ++i) {
            if (samePath(Library.dstAt(i), path))
                return i
        }
        return -1
    }

    // C-6 "{nama}": the file name alone, exactly like the card dialogs.
    readonly property string selectedFileName: {
        const parts = String(Studio.selectedVideo).split(/[\\/]/)
        return parts.length > 0 ? parts[parts.length - 1] : ""
    }

    // Compact action-row / dialog-footer button: token colours, a shared
    // Theme.glyph icon, a real 40px target, a visible keyboard focus ring and
    // Enter/Space on every focus path (a11y contract). The focus ring is ink
    // (Theme.focusRing), never the blue accent; pressing steps the surface.
    component RailButton: Button {
        id: railButton
        property string glyph: ""
        implicitHeight: 40
        // Task 33: zero vertical padding so the contentItem spans the full
        // 40dp control. Material's `verticalPadding` (14dp) outranks
        // `padding` and squeezed it to a 12dp band, top-aligning the label
        // 1px below the optical centre.
        topPadding: 0
        bottomPadding: 0
        focusPolicy: Qt.StrongFocus
        leftPadding: Theme.space2
        rightPadding: Theme.space2
        Accessible.name: text
        Keys.onReturnPressed: {
            railButton.clicked()
            event.accepted = true
        }
        Keys.onEnterPressed: {
            railButton.clicked()
            event.accepted = true
        }
        contentItem: RowLayout {
            spacing: Theme.space1

            Text {
                visible: railButton.glyph.length > 0
                Layout.alignment: Qt.AlignVCenter
                text: railButton.glyph
                font.family: Theme.glyphFont
                font.pixelSize: Theme.fontM
                color: Theme.text
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                // Decorative: the label beside it is the accessible name.
                Accessible.ignored: true
            }

            Text {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
                text: railButton.text
                font.pixelSize: Theme.fontM
                color: Theme.text
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }
        }
        background: Rectangle {
            radius: Theme.radiusS
            color: railButton.down ? Theme.pressedSurface
                   : (railButton.hovered || railButton.activeFocus
                      ? Theme.surface2 : Theme.surface)
            border.width: railButton.activeFocus ? 2 : 0
            border.color: Theme.focusRing
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            // Decorative: the text label beside it is the accessible name.
            Label {
                text: statusBar.statusGlyph
                color: statusBar.statusColor
                font.pixelSize: Theme.fontL
                Accessible.ignored: true
            }

            Label {
                objectName: "statusLine"
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                maximumLineCount: 2
                elide: Text.ElideRight
                font.bold: true
                font.pixelSize: Theme.fontM
                color: statusBar.statusColor
                text: statusBar.statusLine
                Accessible.name: statusBar.statusLine
            }

            // Glossary §5 recovery action; only while the engine is down, so
            // the action row below never carries a fifth button.
            RailButton {
                id: startEngineButton
                objectName: "startEngineButton"
                visible: Studio.engineStatusKind === statusBar.statusKindNotRunning
                text: qsTr("Aktifkan lagi")
                glyph: Theme.glyph.play
                enabled: !Studio.busy
                onClicked: Studio.startEngine()
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Aktifkan lagi wallpaper yang sedang tidak berjalan")
            }
        }

        // ONE action row (task 28): the four compact controls share the row
        // width evenly, so the composition is stable in every state and no
        // label is clipped. Every target keeps its 40px height.
        RowLayout {
            id: actionRow
            objectName: "actionRow"
            Layout.fillWidth: true
            spacing: Theme.space1

            RailButton {
                id: pauseButton
                objectName: "statusActionButton"
                Layout.fillWidth: true
                text: statusBar.toggleLabel
                // The glyph follows the ACTION, not the state: Jeda shows a
                // pause mark, Lanjut a play mark. `glyph` (not `icon`): the
                // custom RailButton property; Button.icon is a FINAL
                // QQuickIcon property and silently swallowed the token.
                glyph: statusBar.paused ? Theme.glyph.play : Theme.glyph.pause
                enabled: statusBar.paused || statusBar.active
                onClicked: statusBar.activate()

                ToolTip.visible: hovered
                ToolTip.text: statusBar.paused
                              ? qsTr("Lanjutkan wallpaper yang sedang dijeda")
                              : qsTr("Jedakan wallpaper sementara")
            }

            RailButton {
                id: pickButton
                objectName: "pickVideoButton"
                Layout.fillWidth: true
                text: qsTr("Pilih video")
                glyph: Theme.glyph.plus
                enabled: !Studio.busy
                onClicked: Studio.pickVideo()
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Pilih file video untuk dipasang tanpa impor ke koleksi")
            }

            RailButton {
                id: pasangButton
                objectName: "pasangButton"
                Layout.fillWidth: true
                text: qsTr("Pasang")
                glyph: Theme.glyph.check
                // Selected card path from the selection slice (bridge
                // property); this pane keeps no selection state of its own.
                enabled: Studio.selectedVideo.length > 0 && !Studio.busy
                onClicked: statusBar.installVideo(Studio.selectedVideo)
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Pasang video terpilih sebagai wallpaper (langsung, tanpa restart)")
            }

            RailButton {
                id: hapusButton
                objectName: "hapusButton"
                Layout.fillWidth: true
                text: qsTr("Hapus")
                glyph: Theme.glyph.remove
                // Only a collection entry can be deleted (C-6 dual delete); a
                // loose pick has nothing to remove from the collection.
                enabled: statusBar.selectedLibraryRow >= 0 && !Studio.busy
                onClicked: hapusMenu.open()
            }
        }
    }

    // Row-menu item: task-27 card-menu pattern applied to the rail's Hapus
    // chooser - token surface, token ink, a shared Theme.glyph icon and a
    // neutral highlight (never the blue Material ripple).
    component HapusMenuItem: MenuItem {
        id: hapusMenuItem
        property string glyph: ""
        implicitHeight: 40
        // Task 33: 40dp rows centre their content exactly (see RailButton).
        topPadding: 0
        bottomPadding: 0
        contentItem: RowLayout {
            spacing: Theme.space2

            Text {
                visible: hapusMenuItem.glyph.length > 0
                Layout.alignment: Qt.AlignVCenter
                text: hapusMenuItem.glyph
                font.family: Theme.glyphFont
                font.pixelSize: Theme.fontM
                color: Theme.text2
                verticalAlignment: Text.AlignVCenter
                Accessible.ignored: true
            }

            Text {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
                text: hapusMenuItem.text
                font.pixelSize: Theme.fontM
                color: hapusMenuItem.enabled ? Theme.text : Theme.text2
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }
        }
        background: Rectangle {
            radius: Theme.radiusS
            color: hapusMenuItem.highlighted ? Theme.surface2 : "transparent"
        }
    }

    // C-6 chooser: the TWO delete semantics stay distinct, never merged.
    // Anchored under the Hapus button inside the right rail, so it can never
    // cover the preview hole in the left column (B-WIREFRAME(f)).
    Menu {
        id: hapusMenu
        objectName: "rowHapusMenu"
        parent: hapusButton
        y: hapusButton.height + Theme.space1
        x: hapusButton.width - width
        // Wide enough for the longer C-6 option ("Hapus file ke Recycle
        // Bin") without eliding; right-aligned under the Hapus button so it
        // stays inside the rail.
        width: 240
        padding: Theme.space1

        background: Rectangle {
            implicitWidth: 240
            implicitHeight: 2 * 40 + 2 * Theme.space1
            radius: Theme.radiusM
            color: Theme.bg
            border.width: 1
            border.color: Theme.surface2
        }

        HapusMenuItem {
            objectName: "rowHapusCollection"
            glyph: Theme.glyph.remove
            text: qsTr("Hapus dari koleksi")
            onTriggered: removeCollectionDialog.open()
        }
        HapusMenuItem {
            objectName: "rowHapusTrash"
            glyph: Theme.glyph.deleteFile
            text: qsTr("Hapus file ke Recycle Bin")
            onTriggered: removeTrashDialog.open()
        }
    }

    // C-6(a): remove the entry; the file itself stays. Copy mirrors the
    // card-level dialog; the confirm runs the same Library.removeAt(row,
    // false) contract.
    Dialog {
        id: removeCollectionDialog
        objectName: "rowRemoveCollectionDialog"
        title: qsTr("Hapus dari koleksi?")
        modal: true
        anchors.centerIn: Overlay.overlay
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        // Task 32: tighter than the Material 24 default.
        padding: Theme.space3
        onOpened: {
            statusBar.dialogOpened()
            collectionCancelButton.forceActiveFocus()
        }
        onClosed: statusBar.dialogClosed()

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
                text: statusBar.selectedFileName
            }
        }

        footer: RowLayout {
            spacing: Theme.space2
            Item {
                Layout.fillWidth: true
            }
            RailButton {
                objectName: "rowRemoveCollectionConfirm"
                text: qsTr("Hapus dari koleksi")
                glyph: Theme.glyph.remove
                onClicked: {
                    removeCollectionDialog.close()
                    Library.removeAt(statusBar.selectedLibraryRow, false)
                }
            }
            RailButton {
                id: collectionCancelButton
                objectName: "rowRemoveCollectionCancel"
                text: qsTr("Batal")
                glyph: Theme.glyph.close
                onClicked: removeCollectionDialog.close()
            }
        }
    }

    // C-6(b): move the FILE itself to the Recycle Bin.
    Dialog {
        id: removeTrashDialog
        objectName: "rowRemoveTrashDialog"
        title: qsTr("Hapus file ke Recycle Bin?")
        modal: true
        anchors.centerIn: Overlay.overlay
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        // Task 32: tighter than the Material 24 default.
        padding: Theme.space3
        onOpened: {
            statusBar.dialogOpened()
            trashCancelButton.forceActiveFocus()
        }
        onClosed: statusBar.dialogClosed()

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
                text: statusBar.selectedFileName
            }
        }

        footer: RowLayout {
            spacing: Theme.space2
            Item {
                Layout.fillWidth: true
            }
            RailButton {
                objectName: "rowRemoveTrashConfirm"
                text: qsTr("Hapus file")
                glyph: Theme.glyph.deleteFile
                onClicked: {
                    removeTrashDialog.close()
                    Library.removeAt(statusBar.selectedLibraryRow, true)
                }
            }
            RailButton {
                id: trashCancelButton
                objectName: "rowRemoveTrashCancel"
                text: qsTr("Batal")
                glyph: Theme.glyph.close
                onClicked: removeTrashDialog.close()
            }
        }
    }
}
