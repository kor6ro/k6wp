// "Pasang ke" popup (plan todo 12 / B-STATE ">1 layar = popup radio").
//
// The radio list behind AssignRow.qml: "Semua layar" (global + delete every
// per-key override, GATE 0 #3) plus one option per Studio.displays label
// ("Layar 1 (utama)", "Layar 2", ...). Choosing a radio only selects the
// install target; the C-14 confirmation for "Semua layar" with active
// overrides belongs to the install path in WallpaperPage.
//
// Overlay contract B-WIREFRAME(f): the popup opens BELOW the row, in the
// gallery area, so it can never float over the native preview surface. When
// the window is too short for that, it flips above the row and that path
// hides the preview through dialogOpened()/dialogClosed() so the popup stays
// visible (the native mpv widget paints above QML).
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Popup {
    id: assignPopup

    // Display entries offered as radio options (WallpaperPage feeds the
    // filtered Studio.displays list).
    property var displays: []
    // Currently selected target: "all" or a display key.
    property string target: "all"
    // Item the popup hangs from (the AssignRow); x/y are relative to it.
    property Item anchorItem: null
    // The page that owns dialogOpened()/dialogClosed() (var, not Item: the
    // contract is the two functions, and qmllint cannot see them on Item).
    property var pageItem: null

    // Raised when the user picks an option; the owner commits the target.
    signal targetChosen(string key)

    readonly property var entries: {
        const all = displays || []
        const out = []
        for (let i = 0; i < all.length; ++i) {
            if (String(all[i].key).length > 0)
                out.push(all[i])
        }
        return out
    }

    // Radio model: "all" first, then one row per display.
    readonly property var radioEntries: {
        const out = [{ key: "all", label: qsTr("Semua layar") }]
        const all = entries
        for (let i = 0; i < all.length; ++i)
            out.push({ key: String(all[i].key), label: String(all[i].label) })
        return out
    }

    parent: anchorItem
    x: 0
    y: anchorItem ? anchorItem.height + Theme.space2 : 0
    width: Math.max(260, contentColumn.implicitWidth + leftPadding + rightPadding)
    height: contentColumn.implicitHeight + topPadding + bottomPadding
    padding: Theme.space2

    modal: false
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    background: Rectangle {
        radius: Theme.radiusM
        color: Theme.bg
        border.width: 1
        border.color: Theme.surface2
        Material.elevation: 2
    }

    // --- positioning + the hide-preview fallback ---------------------------
    // Set while this popup is open on the flipped-above path, so closing it
    // releases exactly one dialogOpened() with one dialogClosed().
    property bool previewHideActive: false

    function positionBelow() {
        if (!anchorItem)
            return
        const below = anchorItem.height + Theme.space2
        const roomBelow = pageItem
                ? pageItem.height
                  - anchorItem.mapToItem(pageItem, 0, anchorItem.height).y
                  - below - Theme.space2
                : 10000
        if (height <= roomBelow) {
            y = below
            return
        }
        // No comfortable room under the row: flip above it. The popup then
        // sits over the preview hole, so use the hide-preview path for as
        // long as it is open.
        y = -(height + Theme.space2)
        if (!previewHideActive) {
            previewHideActive = true
            if (pageItem)
                pageItem.dialogOpened()
        }
    }

    onAboutToShow: positionBelow()
    onClosed: {
        if (previewHideActive) {
            previewHideActive = false
            if (pageItem)
                pageItem.dialogClosed()
        }
    }

    // Keyboard users land on the current choice (Tab/arrows continue there).
    onOpened: Qt.callLater(function () {
        for (let i = 0; i < radioRepeater.count; ++i) {
            const item = radioRepeater.itemAt(i)
            if (item && item.checked) {
                item.forceActiveFocus()
                return
            }
        }
        const first = radioRepeater.itemAt(0)
        if (first)
            first.forceActiveFocus()
    })

    contentItem: ColumnLayout {
        id: contentColumn
        spacing: Theme.space1

        Label {
            Layout.fillWidth: true
            text: qsTr("Pasang ke")
            font.pixelSize: Theme.fontM
            font.weight: Theme.fontWeightSemibold
            color: Theme.text
        }

        ButtonGroup {
            id: targetGroup
        }

        Repeater {
            id: radioRepeater
            model: assignPopup.radioEntries

            delegate: RadioButton {
                id: targetRadio
                objectName: "assignTargetRadio"
                required property var modelData
                ButtonGroup.group: targetGroup
                text: String(modelData.label)
                checked: String(modelData.key) === assignPopup.target
                focusPolicy: Qt.StrongFocus
                implicitHeight: 40
                Accessible.name: text
                // onToggled, not onClicked: Enter/Space activation takes the
                // same path as a mouse click. The key guard keeps the
                // binding-driven check from re-emitting.
                onToggled: {
                    if (checked && String(modelData.key) !== assignPopup.target) {
                        assignPopup.targetChosen(String(modelData.key))
                        assignPopup.close()
                    }
                }
                Keys.onReturnPressed: {
                    targetRadio.click()
                    event.accepted = true
                }
                Keys.onEnterPressed: {
                    targetRadio.click()
                    event.accepted = true
                }
            }
        }
    }
}
