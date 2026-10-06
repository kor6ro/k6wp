// "Pasang ke" row (brief B-WIREFRAME(a) + B-STATE "Pasang-ke", plan todo 12).
//
// The home view's install-target selector. Visible ONLY while more than one
// display entry is modelled: with a single screen there is nothing to choose,
// so the row is absent and the install path goes straight to the global video
// (GATE 0 #3: "Semua layar" = monitor_id -1 + delete per-key overrides).
//
// The row is the compact summary; the radio list itself lives in
// AssignPopup.qml ("Semua layar" / one option per Studio.displays label). In
// duplicate mode the engine cannot place different videos (IS-7), so the
// selector is disabled and WarningBanner carries the C-15 explanation.
//
// Overlay rule B-WIREFRAME(f): the row sits BELOW the preview hole; the popup
// it opens grows downward into the gallery area and never covers the video.
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

RowLayout {
    id: assignRow

    // Display entries to expose. WallpaperPage feeds the filtered
    // Studio.displays list; simulations can feed fixtures.
    property var displays: []
    // Selected target: "all" (global) or a display key.
    property string target: "all"
    // IS-7: duplicate mode disables the selector (per-key assignment off).
    property bool duplicateMode: false

    // Raised when the user wants to change the target; the owner opens the
    // AssignPopup anchored to this row.
    signal popupRequested

    Layout.fillWidth: true
    spacing: Theme.space2
    visible: entries.length > 1

    // Only entries with a usable key participate (BuildDisplayEntries always
    // emits one; fixtures can omit it).
    readonly property var entries: {
        const all = displays || []
        const out = []
        for (let i = 0; i < all.length; ++i) {
            if (String(all[i].key).length > 0)
                out.push(all[i])
        }
        return out
    }

    // Label of the selected target: "Semua layar" or the display's own label
    // ("Layar 1 (utama)", "Layar 2", ...) straight from the bridge model.
    function targetLabel() {
        if (target === "all")
            return qsTr("Semua layar")
        const all = entries
        for (let i = 0; i < all.length; ++i) {
            if (String(all[i].key) === target)
                return String(all[i].label)
        }
        return qsTr("Semua layar")
    }

    Label {
        text: qsTr("Pasang ke:")
        font.pixelSize: Theme.fontM
        color: Theme.text
    }

    // 40px target, visible focus ring, Enter/Space activation (a11y
    // contract). Clicking opens the radio popup.
    Button {
        id: targetButton
        objectName: "assignRowButton"
        implicitHeight: 40
        focusPolicy: Qt.StrongFocus
        enabled: !assignRow.duplicateMode
        text: assignRow.targetLabel()
        Accessible.name: qsTr("Pasang ke: %1").arg(assignRow.targetLabel())
        onClicked: assignRow.popupRequested()
        Keys.onReturnPressed: {
            targetButton.clicked()
            event.accepted = true
        }
        Keys.onEnterPressed: {
            targetButton.clicked()
            event.accepted = true
        }
        contentItem: RowLayout {
            spacing: Theme.space1

            Label {
                text: targetButton.text
                font.pixelSize: Theme.fontM
                color: targetButton.enabled ? Theme.text : Theme.text2
            }

            // Dropdown chevron (glyph, never colour alone).
            Label {
                text: "\u25BE"
                font.pixelSize: Theme.fontS
                color: Theme.text2
                Accessible.ignored: true
            }
        }
        background: Rectangle {
            radius: Theme.radiusS
            color: targetButton.hovered || targetButton.activeFocus
                   ? Theme.surface2 : Theme.surface
            border.width: targetButton.activeFocus ? 2 : 1
            border.color: targetButton.activeFocus ? Theme.accent
                                                   : Theme.surface2
            opacity: targetButton.enabled ? 1.0 : 0.6
        }
    }

    Item {
        Layout.fillWidth: true
    }
}
