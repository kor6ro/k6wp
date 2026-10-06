// Ganti otomatis panel (brief B-WIREFRAME(c), copy deck C-11 verbatim).
//
// The Koleksi view's rotation editor: enable toggle, rotation source
// ("Semua koleksi" / "Pilihan sendiri"), interval (1-1440 min), shuffle and a
// live status line. It replaces the old PlaylistPanel ("Daftar putar") - the
// redesign keeps ONE list, so the panel and the card checkboxes are the only
// playlist editing surface.
//
// Data flow (GATE 0 #2/#4):
//   * Playlist.enabled / intervalMin / shuffle -> playlist.json through the
//     Playlist bridge (immediate persist); never written from QML directly.
//   * Settings.playlistSource ("all" | "custom") decides whether the order is
//     materialized from the library at all.
//   * Materialization runs ONLY while source == "all": every trigger
//     (switching to "all", a library count change, component completion)
//     restarts a 1 s QML debounce Timer and the call goes through
//     Playlist.syncOrderFromLibrary(). Switching to "custom" stops the
//     debounce immediately (no further materialization) and flashes a short
//     confirmation glyph.
//   * Studio.playlistLiveEnabled/Size/Index mirror the engine's get_state ack
//     (B4) so the status reads the engine's real rotation when it is running.
//
// >500 (C-11): a failed materialization over a >500-video collection raises
// the friendly refuse as a toast with the one resolving action, "Pilih
// sendiri", which switches the source to custom. The toast is anchored to the
// panel's bottom edge: it can never cover the preview hole above the gallery
// (B-WIREFRAME(f)).
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Pane {
    id: autoSwitchPanel

    // The panel grows with its rows; explicit so the Pane height never
    // depends on how the style derives implicit size from the contentItem.
    implicitHeight: Math.max(panelColumn.implicitHeight + 2 * padding, 40)

    // C-11 >500 refuse state: raised by a failed sync whose collection is
    // over the bridge's 500-entry cap, cleared by "Pilih sendiri" or a later
    // successful sync. The dismiss timer only ages it out like a toast.
    property bool over500: false
    // Brief "sync stopped" confirmation after switching to Pilihan sendiri.
    property bool customConfirmed: false

    readonly property bool sourceAll: Settings.playlistSource === "all"
    readonly property bool sourceCustom: Settings.playlistSource === "custom"
    // The engine is rotating right now, and the video on screen is part of
    // the playlist (index >= 0; -1 means the current video came from outside
    // it, so there is nothing of ours to report as live).
    readonly property bool live: Studio.playlistLiveEnabled
                                 && Studio.playlistLiveSize > 0
    readonly property bool liveCurrent: live && Studio.playlistLiveIndex >= 0
    // Live size wins while the engine reports it; otherwise the local order
    // count is what the user is editing.
    readonly property int liveCount: live ? Studio.playlistLiveSize
                                          : Playlist.count

    // --- materialization pipeline (source "all" only) ---------------------
    function requestSync() {
        if (!sourceAll)
            return
        syncDebounce.restart()
    }

    function runSync() {
        // The source may have flipped while the debounce was pending.
        if (!sourceAll)
            return
        const ok = Playlist.syncOrderFromLibrary()
        if (ok) {
            over500 = false
            return
        }
        // The bridge refuses >500 valid entries and leaves playlist.json
        // byte-identical; Library.count is the collection size C-11 talks
        // about (a smaller valid count syncs fine and never lands here).
        if (Library.count > 500) {
            over500 = true
            over500Dismiss.restart()
        }
    }

    // QML debounce (~1 s) - the bridge is single-writer, this only coalesces
    // the triggers.
    Timer {
        id: syncDebounce
        interval: 1000
        repeat: false
        onTriggered: autoSwitchPanel.runSync()
    }

    Timer {
        id: over500Dismiss
        interval: 10000
        repeat: false
        onTriggered: autoSwitchPanel.over500 = false
    }

    Timer {
        id: customConfirmTimer
        interval: 2500
        repeat: false
        onTriggered: autoSwitchPanel.customConfirmed = false
    }

    Connections {
        target: Library
        // An import / removal while "Semua koleksi" is active re-materializes
        // after the debounce; in custom mode requestSync() is a no-op.
        function onCountChanged() { autoSwitchPanel.requestSync() }
    }

    Component.onCompleted: autoSwitchPanel.requestSync()

    // Inline button: token colours, 40px target, Enter/Space activation and a
    // visible focus ring (a11y contract).
    component PanelButton: Button {
        id: panelButton
        implicitHeight: 40
        focusPolicy: Qt.StrongFocus
        leftPadding: Theme.space3
        rightPadding: Theme.space3
        Accessible.name: text
        Keys.onReturnPressed: {
            panelButton.clicked()
            event.accepted = true
        }
        Keys.onEnterPressed: {
            panelButton.clicked()
            event.accepted = true
        }
        contentItem: Text {
            text: panelButton.text
            font.pixelSize: Theme.fontM
            font.weight: Theme.fontWeightSemibold
            color: Theme.accentText
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
        background: Rectangle {
            radius: Theme.radiusS
            color: Theme.accent
            border.width: panelButton.activeFocus ? 2 : 0
            border.color: Theme.accentText
        }
    }

    ColumnLayout {
        id: panelColumn
        anchors.fill: parent
        spacing: Theme.space1

        // C-11 row 1: "Ganti otomatis [toggle]".
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.space2

            Label {
                text: qsTr("Ganti otomatis")
                font.pixelSize: Theme.fontM
                font.weight: Theme.fontWeightSemibold
                color: Theme.text
            }

            Switch {
                id: enabledSwitch
                objectName: "autoSwitchToggle"
                checked: Playlist.enabled
                onToggled: Playlist.setEnabled(checked)
                focusPolicy: Qt.StrongFocus
                Accessible.name: qsTr("Ganti otomatis")
                // Enter mirrors Space/mouse: click() runs the control's own
                // press/release path, which emits toggled(). The public
                // toggle() only flips checked and would leave the bridge
                // (and therefore playlist.json) behind.
                Keys.onReturnPressed: {
                    enabledSwitch.click()
                    event.accepted = true
                }
                Keys.onEnterPressed: {
                    enabledSwitch.click()
                    event.accepted = true
                }
            }

            Item {
                Layout.fillWidth: true
            }

            // C-11 row 4: "[x] Acak".
            CheckBox {
                id: shuffleCheck
                objectName: "autoSwitchShuffle"
                text: qsTr("Acak")
                checked: Playlist.shuffle
                onToggled: Playlist.setShuffle(checked)
                focusPolicy: Qt.StrongFocus
                Accessible.name: text
                Keys.onReturnPressed: {
                    shuffleCheck.click()
                    event.accepted = true
                }
                Keys.onEnterPressed: {
                    shuffleCheck.click()
                    event.accepted = true
                }
            }
        }

        // C-11 row 2: "Sumber: ( ) Semua koleksi ( ) Pilihan sendiri ({n}
        // video)". Pilihan sendiri = the playlist.json order (GATE 0 #2), so
        // {n} is the current order size.
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.space2

            Label {
                text: qsTr("Sumber:")
                font.pixelSize: Theme.fontM
                color: Theme.text
            }

            ButtonGroup {
                id: sourceGroup
            }

            RadioButton {
                id: allRadio
                objectName: "playlistSourceAll"
                text: qsTr("Semua koleksi")
                ButtonGroup.group: sourceGroup
                checked: autoSwitchPanel.sourceAll
                focusPolicy: Qt.StrongFocus
                Accessible.name: text
                // onToggled (not onClicked) so Enter/Space activation takes
                // the same path as a mouse click.
                onToggled: {
                    if (checked && Settings.playlistSource !== "all") {
                        Settings.setPlaylistSource("all")
                        autoSwitchPanel.requestSync()
                    }
                }
                // click() on an already-checked radio is a no-op (exclusive
                // group), so Enter cannot uncheck the whole group.
                Keys.onReturnPressed: {
                    allRadio.click()
                    event.accepted = true
                }
                Keys.onEnterPressed: {
                    allRadio.click()
                    event.accepted = true
                }
            }

            RadioButton {
                id: customRadio
                objectName: "playlistSourceCustom"
                text: qsTr("Pilihan sendiri (%1 video)").arg(Playlist.count)
                ButtonGroup.group: sourceGroup
                checked: autoSwitchPanel.sourceCustom
                focusPolicy: Qt.StrongFocus
                Accessible.name: text
                onToggled: {
                    if (checked && Settings.playlistSource !== "custom") {
                        Settings.setPlaylistSource("custom")
                        // STOP materialization: a pending debounce must not
                        // write the order after the switch.
                        syncDebounce.stop()
                        autoSwitchPanel.over500 = false
                        autoSwitchPanel.customConfirmed = true
                        customConfirmTimer.restart()
                    }
                }
                Keys.onReturnPressed: {
                    customRadio.click()
                    event.accepted = true
                }
                Keys.onEnterPressed: {
                    customRadio.click()
                    event.accepted = true
                }
            }

            // Brief visual confirmation of the switch (glyph, so the state
            // does not rely on colour alone).
            Label {
                visible: autoSwitchPanel.customConfirmed
                text: "\u2713"
                color: Theme.statusActive
                font.pixelSize: Theme.fontM
                font.weight: Theme.fontWeightSemibold
                Accessible.ignored: true
            }

            Item {
                Layout.fillWidth: true
            }
        }

        // C-11 row 3: "Ganti tiap: [30 menit]" (1-1440).
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.space2

            Label {
                text: qsTr("Ganti tiap:")
                font.pixelSize: Theme.fontM
                color: Theme.text
            }

            SpinBox {
                id: intervalSpin
                objectName: "autoSwitchInterval"
                from: 1
                to: 1440
                editable: true
                value: Playlist.intervalMin
                onValueModified: Playlist.setIntervalMin(value)
                focusPolicy: Qt.StrongFocus
                Accessible.name: qsTr("Ganti tiap:")
            }

            Label {
                text: qsTr("menit")
                font.pixelSize: Theme.fontM
                color: Theme.text2
            }

            Item {
                Layout.fillWidth: true
            }
        }

        // C-11 status row: "Berganti tiap {i} • {n} video • {acak/berurutan}"
        // (B4: the engine's live size/index win while it reports them).
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.space1
            visible: Playlist.enabled || autoSwitchPanel.live

            Label {
                // Glyph + colour never carry the state alone: the sentence
                // itself names the configuration.
                text: autoSwitchPanel.liveCurrent ? "\u25B6" : "\u25CB"
                color: autoSwitchPanel.liveCurrent ? Theme.statusActive
                                                   : Theme.statusIdle
                font.pixelSize: Theme.fontS
                Accessible.ignored: true
            }

            Label {
                id: statusLabel
                objectName: "autoSwitchStatus"
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                color: Theme.text2
                font.pixelSize: Theme.fontM
                text: qsTr("Berganti tiap %1 \u2022 %2 video \u2022 %3")
                      .arg(Playlist.intervalMin)
                      .arg(autoSwitchPanel.liveCount)
                      .arg(Playlist.shuffle ? qsTr("acak") : qsTr("berurutan"))
            }
        }
    }

    // C-11 >500 toast: friendly refuse + the one action that resolves it.
    // Anchored to the panel's bottom edge (gallery area, below the preview
    // hole) and raised above the panel rows.
    Rectangle {
        id: over500Toast
        objectName: "over500Toast"
        z: 1
        visible: autoSwitchPanel.over500
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Theme.space1
        implicitWidth: Math.min(parent.width - 2 * Theme.space2,
                                toastRow.implicitWidth + 2 * Theme.space3)
        implicitHeight: toastRow.implicitHeight + 2 * Theme.space2
        radius: Theme.radiusM
        color: Theme.statusPausedTint
        border.width: 1
        border.color: Theme.surface2

        Accessible.role: Accessible.AlertMessage
        Accessible.name: toastLabel.text

        RowLayout {
            id: toastRow
            anchors.fill: parent
            anchors.margins: Theme.space2
            spacing: Theme.space2

            Label {
                text: "\u26A0\uFE0E"
                color: Theme.statusPaused
                font.pixelSize: Theme.fontM
                Accessible.ignored: true
            }

            Label {
                id: toastLabel
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                color: Theme.text
                font.pixelSize: Theme.fontM
                text: qsTr("Koleksimu lebih dari 500 video. Pilih sendiri video yang ingin diganti otomatis.")
            }

            PanelButton {
                id: chooseOwnButton
                objectName: "over500ChooseOwn"
                text: qsTr("Pilih sendiri")
                onClicked: {
                    syncDebounce.stop()
                    Settings.setPlaylistSource("custom")
                    autoSwitchPanel.over500 = false
                }
            }
        }
    }
}
