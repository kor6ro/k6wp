// First-run onboarding (brief B-WIREFRAME(b) + copy deck C-2 verbatim): ONE
// screen, not the old four-page wizard. FirstRunDialog.qml (the StackLayout
// wizard) is deleted; this view replaces it and owns the first-run screen
// instead of the C-1 empty state.
//
// Gating (the old first-run pattern, located by symbol in base Main.qml:
// `Library.firstRunEligible && !Studio.videoActive` in Component.onCompleted
// plus the wizard's `firstRunFile: Library.lastPickedPath`):
//   Main.root.firstRunActive =
//       root.firstRunFlowActive
//       && ((Library.firstRunEligible && !Studio.videoActive)
//           || Library.lastPickedPath.length > 0)
// firstRunFlowActive is the DEF-1 session latch: armed once at start-up (only
// when the session begins in the first-run state), kept through the pick, and
// consumed by Selesai. The lastPickedPath term keeps this view up after the
// pick imports the video (the import flips firstRunEligible false through
// countChanged) and makes the gate re-evaluate when Selesai's
// markFirstRunHandled() clears it (lastPickedPathChanged); firstRunEligible
// itself only NOTIFYs countChanged. Without the latch, any LATER gallery
// import (which also sets lastPickedPath) re-opened this overlay.
//
// "Pilih video" runs the standard single-file picker (Studio.pickVideo, the
// same invokable StatusBar's inactive-state button uses) and then references
// the pick in the collection (Library.importPaths sets Library.lastPickedPath
// and imports into the gallery; pickAndImport semantics without a second
// dialog). "Selesai" honors both checkboxes (C-2 default ON - unchecking must
// stick), closes the durable first-run gate (Library.markFirstRunHandled
// creates studio_settings.json) and plays the pick at once, globally
// (Studio.applyWallpaper = "Semua layar" monitor_id=-1; a fresh profile has no
// per-monitor override to clear).
//
// Overlay contract (brief B-WIREFRAME(f)): this view covers the Beranda
// preview area, so it is a sibling overlay OUTSIDE PreviewHole.qml and feeds
// the root dialogOpened()/dialogClosed() pair - the native mpv surface steps
// aside while it is up, and the root's openDialogs counter stays balanced
// through announceDialog() (the initial visible=true cannot fire
// onVisibleChanged).
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

FocusScope {
    id: onboardingView

    // While this overlay is up it owns the keyboard: the scope takes focus
    // and pickButton (focus: true) is the first Tab stop. Without this, a
    // key event (Enter) can reach the page behind the scrim - the old
    // first-run dialog was a real Dialog with its own focus handling, an
    // inline overlay is not.
    focus: visible

    // Root dialog lifecycle: dialogOpened() hides the native preview surface,
    // dialogClosed() restores it when the last modal is gone.
    property var dialogOpened: function () {}
    property var dialogClosed: function () {}

    // DEF-1: consumes Main's session latch with Selesai, so a later import
    // (which also writes Library.lastPickedPath) can never re-open the flow.
    property var firstRunFinished: function () {}

    // Fires each visibility edge exactly once, including the initial
    // visible=true that never appears as a property change.
    property bool dialogAnnounced: false

    function announceDialog(opened) {
        if (opened === onboardingView.dialogAnnounced)
            return
        onboardingView.dialogAnnounced = opened
        if (opened)
            onboardingView.dialogOpened()
        else
            onboardingView.dialogClosed()
    }

    onVisibleChanged: {
        onboardingView.announceDialog(visible)
        // Main arms the gate in Component.onCompleted, so this overlay is
        // created hidden and becomes visible later: take the keyboard on
        // every show. (The created-visible path is covered by
        // Component.onCompleted below.)
        if (visible)
            Qt.callLater(onboardingView.focusPrimary)
    }
    Component.onCompleted: {
        onboardingView.announceDialog(visible)
        // The FocusScope's focus:visible + pickButton.focus:true pair alone
        // does not produce an active focus item for an overlay created
        // visible; the deferred forceActiveFocus does.
        if (visible)
            Qt.callLater(onboardingView.focusPrimary)
    }

    function focusPrimary() {
        if (pickButton)
            pickButton.forceActiveFocus()
    }

    // The focus chain follows the flow: the first Tab stop is "Pilih video",
    // and once a pick exists it moves to "Selesai" (so Enter finishes). The
    // swap is deferred - when the signal fires, finishButton.enabled is still
    // false (the binding has not re-evaluated yet) and a disabled item
    // refuses focus, which left the whole overlay without an active focus
    // item and leaked Enter to the page behind the scrim.
    Connections {
        target: Library
        function onLastPickedPathChanged() {
            Qt.callLater(onboardingView.syncFocus)
        }
    }

    function syncFocus() {
        // DEF-1: imports after Selesai still change lastPickedPath, but the
        // overlay is gone and must never take focus for a hidden flow.
        if (!onboardingView.visible)
            return
        if (Library.lastPickedPath.length > 0) {
            pickButton.focus = false
            finishButton.focus = true
            finishButton.forceActiveFocus()
        } else {
            finishButton.focus = false
            pickButton.focus = true
            pickButton.forceActiveFocus()
        }
    }

    // Enter finishes once a pick exists. This is window-scoped ON PURPOSE: the
    // native picker takes the OS focus and the QQuickWidget only regains an
    // active focus item on the next in-window interaction, so key delivery to
    // the focused button alone is not enough right after the picker closes.
    // Disabled while focus is on "Pilih video" (Enter there re-opens the
    // picker - a deliberate "change my pick" gesture) or on a checkbox
    // (Enter toggles it).
    Shortcut {
        sequence: "Return"
        enabled: onboardingView.visible
                 && Library.lastPickedPath.length > 0
                 && !pickButton.activeFocus
                 && !startWithWindowsBox.activeFocus
                 && !autoCompressBox.activeFocus
        onActivated: onboardingView.finish()
    }

    // C-2 flow, stage 1: publish the pick through the standard picker, then
    // reference it in the collection. Cancel, or re-picking the file already
    // selected, leaves everything untouched.
    function pickVideo() {
        const before = Studio.selectedVideo
        Studio.pickVideo()
        const picked = Studio.selectedVideo
        if (picked.length === 0 || picked === before)
            return
        Library.importPaths([picked])
    }

    // Basename of the pick, for the picked-file line (user data, not copy).
    function pickedFileName() {
        const p = String(Library.lastPickedPath)
        const slash = Math.max(p.lastIndexOf("/"), p.lastIndexOf("\\"))
        return slash >= 0 ? p.slice(slash + 1) : p
    }

    // C-2 flow, stage 2 ("Selesai"). The pick is captured BEFORE
    // markFirstRunHandled() - that call clears Library.lastPickedPath.
    function finish() {
        const picked = String(Library.lastPickedPath)
        if (picked.length === 0)
            return
        // "bila user matikan, hormati": the two C-2 checkboxes land exactly
        // as shown and persist through the bridge (write-through on change,
        // plus an explicit apply()).
        Settings.setStartWithWindows(startWithWindowsBox.checked)
        Settings.setAutoCompressOnImport(autoCompressBox.checked)
        Settings.apply()
        // Durable first-run marker: creates studio_settings.json, which is
        // what Library.firstRunEligible reads, and clears lastPickedPath so
        // Main.firstRunActive drops this view.
        Library.markFirstRunHandled()
        // DEF-1: consume the session latch. Later gallery imports also write
        // lastPickedPath, but firstRunActive is latched off now, so this
        // overlay can never re-open - in this session or after a restart.
        onboardingView.firstRunFinished()
        // Play immediately. The plain global apply path ("Semua layar").
        Studio.applyWallpaper(picked)
    }

    // 40px dialog-style action button: token colours, a shared Theme.glyph
    // icon, visible ink focus ring (never blue on the non-primary action),
    // Enter/Space activation, Accessible.name (repo pattern:
    // AppDialogs.DialogActionButton).
    component OnboardingButton: Button {
        id: onboardingButton
        property bool primary: false
        property string glyph: ""
        implicitHeight: 40
        // Task 33: zero vertical padding keeps the 40dp content box centred
        // (Material's verticalPadding otherwise squeezed it to 12dp).
        topPadding: 0
        bottomPadding: 0
        focusPolicy: Qt.StrongFocus
        leftPadding: Theme.space2
        rightPadding: Theme.space2
        Accessible.name: text
        Keys.onReturnPressed: {
            onboardingButton.clicked()
            event.accepted = true
        }
        Keys.onEnterPressed: {
            onboardingButton.clicked()
            event.accepted = true
        }
        contentItem: RowLayout {
            spacing: Theme.space1

            Text {
                visible: onboardingButton.glyph.length > 0
                Layout.alignment: Qt.AlignVCenter
                text: onboardingButton.glyph
                font.family: Theme.glyphFont
                font.pixelSize: Theme.fontM
                color: onboardingButton.primary ? Theme.accentText : Theme.text
                verticalAlignment: Text.AlignVCenter
                Accessible.ignored: true
            }

            Text {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
                text: onboardingButton.text
                font.pixelSize: Theme.fontM
                font.weight: onboardingButton.primary ? Theme.fontWeightSemibold
                                                      : Theme.fontWeightRegular
                color: onboardingButton.primary ? Theme.accentText : Theme.text
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }
        }
        background: Rectangle {
            radius: Theme.radiusS
            color: onboardingButton.primary
                   ? (onboardingButton.down ? Qt.darker(Theme.accent, 1.15)
                                            : Theme.accent)
                   : (onboardingButton.down ? Theme.pressedSurface
                      : (onboardingButton.hovered
                         || onboardingButton.activeFocus
                         ? Theme.surface2 : Theme.surface))
            border.width: onboardingButton.activeFocus ? 2 : 0
            border.color: onboardingButton.primary ? Theme.accentText
                                                   : Theme.focusRing
            opacity: onboardingButton.enabled ? 1.0 : 0.6
        }
    }

    // Full-window scrim: near-opaque Theme.bg so the page behind never fights
    // the onboarding copy (and the hidden native preview cannot show above it).
    Rectangle {
        anchors.fill: parent
        color: Theme.bg
        opacity: 0.97
    }

    Pane {
        id: card
        anchors.centerIn: parent
        width: Math.min(560, onboardingView.width - 2 * Theme.space4)
        Material.elevation: 2
        padding: Theme.space4

        ColumnLayout {
            anchors.fill: parent
            spacing: Theme.space3

            // Restrained illustration: the C-1 "screen + play" motif, one
            // size up. Decorative only.
            Rectangle {
                Layout.alignment: Qt.AlignHCenter
                implicitWidth: 168
                implicitHeight: 104
                radius: Theme.radiusL
                color: Theme.surface2
                Accessible.ignored: true

                Label {
                    anchors.centerIn: parent
                    text: Theme.glyph.play
                    font.family: Theme.glyphFont
                    color: Theme.text2
                    opacity: 0.7
                    font.pixelSize: 36
                    Accessible.ignored: true
                }
            }

            // C-2 verbatim headline.
            Label {
                objectName: "onboardingTitle"
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                font.pixelSize: Theme.fontXL
                font.weight: Theme.fontWeightSemibold
                color: Theme.text
                text: qsTr("Pilih video pertamamu")
            }

            // C-2 flow verbatim: [Pilih video] -> [Selesai]. The arrow is the
            // copy deck's flow marker, decorative only.
            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: Theme.space2

                OnboardingButton {
                    id: pickButton
                    objectName: "onboardingPickVideo"
                    primary: true
                    // First Tab stop of the overlay's FocusScope. The
                    // Connections above swaps this to "Selesai" once a pick
                    // lands.
                    focus: true
                    text: qsTr("Pilih video")
                    glyph: Theme.glyph.plus
                    onClicked: onboardingView.pickVideo()
                }

                Label {
                    text: Theme.glyph.arrowRight
                    font.family: Theme.glyphFont
                    color: Theme.text2
                    font.pixelSize: Theme.fontM
                    Accessible.ignored: true
                }

                OnboardingButton {
                    id: finishButton
                    objectName: "onboardingFinish"
                    // Stage 1 must have landed; the old wizard disabled its
                    // "Lanjut" on the same rule (a pick is required).
                    enabled: Library.lastPickedPath.length > 0
                    // Focus is swapped in by the Connections above (property,
                    // not just activeFocus) once a pick lands.
                    text: qsTr("Selesai")
                    glyph: Theme.glyph.check
                    onClicked: onboardingView.finish()
                }
            }

            // Picked-file feedback (basename only - user data, no new copy).
            Label {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                visible: Library.lastPickedPath.length > 0
                elide: Text.ElideMiddle
                color: Theme.text2
                font.pixelSize: Theme.fontS
                text: onboardingView.pickedFileName()
                Accessible.name: text
            }

            // C-2 preference rows, both default ON (keputusan 12). 40px
            // targets, Space toggles (AbstractButton), Enter/Return explicit
            // (repo SettingCheck pattern).
            CheckBox {
                id: startWithWindowsBox
                objectName: "onboardingStartWithWindows"
                Layout.fillWidth: true
                implicitHeight: 40
                focusPolicy: Qt.StrongFocus
                checked: true
                text: qsTr("Jalankan saat Windows menyala (bisa diubah di Pengaturan › Umum)")
                Accessible.name: text
                Keys.onReturnPressed: {
                    toggle()
                    event.accepted = true
                }
                Keys.onEnterPressed: {
                    toggle()
                    event.accepted = true
                }
            }

            CheckBox {
                id: autoCompressBox
                objectName: "onboardingAutoCompress"
                Layout.fillWidth: true
                implicitHeight: 40
                focusPolicy: Qt.StrongFocus
                checked: true
                text: qsTr("Siapkan video otomatis (bisa diubah di Pengaturan › Lanjutan)")
                Accessible.name: text
                Keys.onReturnPressed: {
                    toggle()
                    event.accepted = true
                }
                Keys.onEnterPressed: {
                    toggle()
                    event.accepted = true
                }
            }
        }
    }
}
