// Status bar: the Wallpaper right-rail summary. Shows the friendly state
// (Studio.statusTitle) plus the active video's display name
// (Studio.statusVideoName) and exactly ONE action button:
//   wallpaper active -> "Jeda"        (Studio.togglePause())
//   wallpaper paused -> "Lanjut"      (Studio.togglePause())
//   anything else    -> "Pilih video" (Studio.pickVideo())
//
// Copy deck C-3 verbatim; the words come from the todo-4 bridge surface
// (StatusTitleFor / StatusVideoNameFor) - QML never re-derives the status.
// Raw support details (process id, paths, log) live in the "Info teknis"
// dialog (AppDialogs.qml) and never render on the main screen.
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Pane {
    id: statusBar

    // BridgeStatusKind::kPaused (studio_bridge.hpp). Passed in from Main's
    // StatusKind enum so this component never hardcodes the mirror.
    property int statusKindPaused: 1

    Layout.fillWidth: true
    Material.elevation: 1
    padding: 12

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
    // Text-presentation selector (\uFE0E) keeps these as glyphs, not emoji.
    readonly property string statusGlyph: paused ? "\u23F8\uFE0E"  // pause
                                        : active ? "\u25B6\uFE0E"  // play
                                        : "\u25CB"                 // idle ring

    // C-3: "Wallpaper aktif • {nama}" / "Dijeda • {nama}" /
    // "Tidak aktif — pilih video untuk mulai". Composed from the bridge's
    // derived properties only.
    readonly property string statusLine: {
        const name = Studio.statusVideoName
        if (name.length > 0)
            return Studio.statusTitle + " • " + name
        if (idle)
            return Studio.statusTitle + " — pilih video untuk mulai"
        return Studio.statusTitle
    }

    // B-STATE matrix labels.
    readonly property string buttonLabel: paused ? qsTr("Lanjut")
                                          : active ? qsTr("Jeda")
                                          : qsTr("Pilih video")

    // The single action. Space activates through the Button itself; Return /
    // Enter are handled explicitly below so every style covers both keys.
    function activate() {
        if (paused || active)
            Studio.togglePause()
        else
            Studio.pickVideo()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            // Decorative: the text label below is the accessible name.
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
        }

        // One button, three labels (C-3 / B-STATE). 40px minimum target.
        Item {
            Layout.fillWidth: true
            Layout.minimumHeight: 40
            Layout.preferredHeight: 40

            Button {
                id: actionButton
                objectName: "statusActionButton"
                anchors.fill: parent
                text: statusBar.buttonLabel
                enabled: !Studio.busy
                focusPolicy: Qt.StrongFocus
                Accessible.name: text
                onClicked: statusBar.activate()

                Keys.onReturnPressed: {
                    statusBar.activate()
                    event.accepted = true
                }
                Keys.onEnterPressed: {
                    statusBar.activate()
                    event.accepted = true
                }

                ToolTip.visible: hovered
                ToolTip.text: statusBar.paused ? qsTr("Lanjutkan wallpaper yang sedang dijeda")
                            : statusBar.active ? qsTr("Jedakan wallpaper sementara")
                            : qsTr("Pilih file video untuk dipasang")
            }

            // Visible keyboard focus: 2px accent outline (B-TOKEN). A disabled
            // overlay, so it never steals the button's mouse events.
            Rectangle {
                anchors.fill: parent
                radius: Theme.radiusS
                color: "transparent"
                border.width: 2
                border.color: Theme.accent
                visible: actionButton.activeFocus
                enabled: false
            }
        }
    }
}
