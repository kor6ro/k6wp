// Engine status panel: the Wallpaper right-rail pane showing engine status,
// current video, pid/settings path, selected video, and last error.
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Pane {
    id: engineStatusPanel
    property int labelColWidth: 132
    property int formColSpacing: 12
    property var statusColor: function (kind) { return "gray" }

    Layout.fillWidth: true
    Material.elevation: 1
    padding: 12

    ColumnLayout {
        anchors.fill: parent
        spacing: 4

        // One 2-column grid, so "Status engine:" and
        // "Video saat ini:" share a label column and
        // both values start at the same x.
        GridLayout {
            Layout.fillWidth: true
            columns: 2
            columnSpacing: formColSpacing
            rowSpacing: 6

            Label {
                text: qsTr("Status engine:")
                font.bold: true
                Layout.preferredWidth: labelColWidth
            }

            Label {
                Layout.fillWidth: true
                elide: Text.ElideMiddle
                text: Studio.engineStatusDetail
                color: statusColor(Studio.engineStatusKind)

                HoverHandler {
                    id: statusHover
                }
                ToolTip.visible: statusHover.hovered
                ToolTip.text: Studio.engineStatusHint
            }

            Label {
                text: qsTr("Video saat ini:")
                Layout.preferredWidth: labelColWidth
            }

            Label {
                Layout.fillWidth: true
                elide: Text.ElideMiddle
                text: Studio.videoActive
                      ? Studio.activeVideoPath
                      : qsTr("(belum ada video)")

                HoverHandler {
                    id: videoHover
                }
                ToolTip.visible: videoHover.hovered
                ToolTip.text: Studio.activeVideoPath
            }

            Label {
                text: qsTr("pid")
                Layout.preferredWidth: labelColWidth
                opacity: 0.6
                font.pixelSize: 12
            }

            Label {
                Layout.fillWidth: true
                elide: Text.ElideMiddle
                opacity: 0.6
                font.pixelSize: 12
                text: Studio.enginePid + "   "
                      + Studio.settingsPath
            }

            Label {
                text: qsTr("Dipilih:")
                visible: Studio.selectedVideo.length > 0
                Layout.preferredWidth: labelColWidth
                font.pixelSize: 12
            }

            Label {
                Layout.fillWidth: true
                visible: Studio.selectedVideo.length > 0
                elide: Text.ElideMiddle
                font.pixelSize: 12
                text: Studio.selectedVideo

                HoverHandler {
                    id: selHover
                }
                ToolTip.visible: selHover.hovered
                ToolTip.text: Studio.selectedVideo
            }
        }

        // Last failed action, shown verbatim (IPC
        // errors are never swallowed).
        Label {
            Layout.fillWidth: true
            visible: Studio.lastError.length > 0
            wrapMode: Text.WordWrap
            color: "red"
            text: Studio.lastError
        }
    }
}
