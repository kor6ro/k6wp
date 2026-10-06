// Wallpaper page (Wallpaper tab). Contains the monitor sub-tabs strip, the
// preview caption + PreviewHole, the LibraryPanel and PlaylistPanel in the
// left column, and the right rail (EngineStatusPanel + ActionButtons +
// QuickSettingsPanel + LogPanel).
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Item {
    id: wallpaperPage

    // Sizing tokens from Main root.
    property int labelColWidth: 132
    property int controlWidth: 320
    property int spinWidth: 140
    property int formColSpacing: 12
    property int formRowSpacing: 10

    // State + functions from Main root.
    property string selectedMonitorKey: ""
    property string armedAssignPath: ""
    property var selectMonitor: function (key) {}
    property var armAssign: function (path) { return false }
    property var disarmAssign: function () {}
    property var fileNameOf: function (path) { return "" }
    property var selectedMonitorLabel: function () { return "" }
    property var selectedMonitorHasAssignment: function () { return false }
    property var statusColor: function (kind) { return "gray" }
    property var maybeOfferCompressFirst: function (path, applyAfter) { return false }
    property var syncPreview: function () {}
    property int statusKindNotRunning: 3
    property var dialogOpened: function () {}
    property var dialogClosed: function () {}

    // PreviewHole state exposed for Main.qml's syncPreview() function
    // (the id previewHole is scoped to this file). Updated via the
    // PreviewHole's geometryChanged signal.
    property bool previewHoleVisible: false
    property real previewHoleX: 0
    property real previewHoleY: 0
    property real previewHoleWidth: 0
    property real previewHoleHeight: 0

    function refreshPreviewHoleGeometry() {
        previewHoleVisible = previewHole.visible
        const topLeft = previewHole.mapToItem(null, 0, 0)
        previewHoleX = topLeft.x
        previewHoleY = topLeft.y
        previewHoleWidth = previewHole.width
        previewHoleHeight = previewHole.height
    }

    // Row 41: the monitor sub-tab strip sits above the two
    // columns so selecting a monitor scopes the whole view
    // (preview caption + actions), not just one column.
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        // --- row 41: monitor sub-tabs ----------------------
        // One sub-tab per entry of Studio.displays, filtered to
        // entries with a non-empty key (a zero-key entry
        // renders NO button - row 20's zero-size guard). The
        // selected button is the scope for the preview caption
        // and for the monitor actions; it is also the
        // assignment target while a video is armed (row 39).
        Pane {
            id: monitorSubTabs
            objectName: "monitorSubTabs"
            Layout.fillWidth: true
            Material.elevation: 1
            padding: 8

            ColumnLayout {
                anchors.fill: parent
                spacing: 6

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Label {
                        text: qsTr("Layar:")
                        font.bold: true
                    }

                    Repeater {
                        id: monitorTabRepeater
                        // The binding re-evaluates on every
                        // displaysChanged; entries without a
                        // key never reach the repeater, so no
                        // button is created for them.
                        model: {
                            const all = Studio.displays
                            const out = []
                            for (let i = 0; i < all.length; ++i) {
                                const entry = all[i]
                                if (String(entry.key).length > 0)
                                    out.push(entry)
                            }
                            return out
                        }

                        delegate: Button {
                            id: monitorTabButton
                            objectName: "monitorSubTab"
                            required property var modelData

                            // `text` is not painted (the custom
                            // contentItem below is), but it is
                            // what screen readers and UIA see.
                            text: String(modelData.label) + " " + String(modelData.resolutionLabel)

                            // flat/highlighted (not checkable):
                            // selectedMonitorKey is the single
                            // source of truth, so a click can
                            // never destroy the binding that
                            // paints the selection. While a
                            // video is armed every sub-tab is
                            // raised (non-flat) because each one
                            // is a valid assignment target.
                            flat: wallpaperPage.selectedMonitorKey !== String(modelData.key)
                                 && wallpaperPage.armedAssignPath.length === 0
                            highlighted: wallpaperPage.selectedMonitorKey === String(modelData.key)
                            onClicked: wallpaperPage.selectMonitor(String(modelData.key))

                            // Row 42: the raw device key was a
                            // developer string in a user-facing
                            // bubble; say what the click does,
                            // and what it will do while armed.
                            ToolTip.visible: hovered
                            ToolTip.text: wallpaperPage.armedAssignPath.length > 0
                                          ? qsTr("Klik untuk menugaskan video ke layar ini")
                                          : qsTr("Pilih layar ini")

                            contentItem: ColumnLayout {
                                spacing: 3

                                RowLayout {
                                    spacing: 6

                                    Label {
                                        Layout.fillWidth: true
                                        elide: Text.ElideRight
                                        font.bold: true
                                        text: modelData.label
                                    }

                                    // Primary mark (row 19
                                    // isPrimary), shown only on
                                    // the primary monitor. Row 42:
                                    // no explicit accent color -
                                    // accent-on-accent made the
                                    // badge nearly invisible on
                                    // the highlighted (selected)
                                    // button; it inherits the
                                    // button's foreground instead.
                                    Label {
                                        visible: modelData.isPrimary === true
                                        text: qsTr("UTAMA")
                                        font.bold: true
                                        font.pixelSize: 10
                                        opacity: 0.8
                                    }
                                }

                                Label {
                                    text: modelData.resolutionLabel
                                    opacity: 0.7
                                    font.pixelSize: 11
                                }

                                Label {
                                    visible: String(modelData.assignedPath).length > 0
                                    text: wallpaperPage.fileNameOf(modelData.assignedPath)
                                    elide: Text.ElideMiddle
                                    Layout.maximumWidth: 220
                                    opacity: 0.8
                                    font.pixelSize: 10
                                }

                                // Duplicate-mode badge: the full
                                // IS-7 notice sits once under the
                                // row (one per button would
                                // repeat the same sentence).
                                Label {
                                    visible: Studio.duplicateModeNotice.length > 0
                                    text: qsTr("mode duplikat")
                                    color: "orange"
                                    font.pixelSize: 10
                                }

                                // Degraded: an assignment exists
                                // but its file is missing.
                                Label {
                                    visible: String(modelData.assignedPath).length > 0
                                             && modelData.assignedExists === false
                                    text: qsTr("file tidak ditemukan")
                                    color: "red"
                                    font.pixelSize: 10
                                }

                                // Coverage: anything other than
                                // "covered" is a warning the
                                // engine log explains.
                                Label {
                                    visible: String(modelData.coverage) !== "covered"
                                    text: qsTr("cek engine.log")
                                    color: "orange"
                                    font.pixelSize: 10
                                }
                            }
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        visible: monitorTabRepeater.count === 0
                        opacity: 0.7
                        text: qsTr("Tidak ada layar terdeteksi")
                    }

                    // The Hapus path: drop the selected
                    // monitor's assignment through the row-19
                    // invokable (the inverse of the armed
                    // assign click).
                    Button {
                        id: clearMonitorAssignmentButton
                        objectName: "clearMonitorAssignment"
                        text: qsTr("Hapus penugasan")
                        enabled: wallpaperPage.selectedMonitorKey.length > 0
                                 && wallpaperPage.selectedMonitorHasAssignment()
                        onClicked: Studio.clearMonitorAssignment(wallpaperPage.selectedMonitorKey)
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Hapus penugasan video untuk layar terpilih")
                    }
                }

                // The full IS-7 duplicate-mode notice: the
                // state rows 19/23/24 used to surface on the
                // canvas now lives on the sub-tab row.
                Label {
                    id: duplicateModeNoticeLabel
                    objectName: "duplicateModeNotice"
                    Layout.fillWidth: true
                    visible: Studio.duplicateModeNotice.length > 0
                    wrapMode: Text.WordWrap
                    color: "orange"
                    text: Studio.duplicateModeNotice
                }
            }
        }

        // Two columns instead of one tall stack: the old layout
        // pushed everything full-width down the page, so a 1936px
        // window left ~75% of the width empty while the preview sat
        // in a 1900x320 letterbox. The left column carries the
        // content that benefits from area (preview + library); the
        // right rail is a fixed 340px for the things that are
        // short text or fixed controls.
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 8

            // -----------------------------------------------------
            // Left: preview + library
            // -----------------------------------------------------
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 8

                // Row 41: the preview hole is scoped to the
                // selected monitor; the caption names it. The
                // native mpv surface itself stays engine-wide -
                // StudioBridge exposes no per-monitor preview.
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Label {
                        text: qsTr("Pratinjau:")
                    }

                    Label {
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                        opacity: 0.7
                        font.pixelSize: 12
                        text: wallpaperPage.selectedMonitorLabel().length > 0
                              ? qsTr("cakupan: %1").arg(wallpaperPage.selectedMonitorLabel())
                              : qsTr("cakupan: semua layar")
                    }
                }

                PreviewHole {
                    syncPreview: wallpaperPage.syncPreview
                    onGeometryChanged: wallpaperPage.refreshPreviewHoleGeometry()
                }

                LibraryPanel {
                    armAssign: wallpaperPage.armAssign
                    armedAssignPath: wallpaperPage.armedAssignPath
                    dialogOpened: wallpaperPage.dialogOpened
                    dialogClosed: wallpaperPage.dialogClosed
                }

                // --- Daftar putar (playlist) ---------------------
                PlaylistPanel {
                    spinWidth: wallpaperPage.spinWidth
                    armAssign: wallpaperPage.armAssign
                    armedAssignPath: wallpaperPage.armedAssignPath
                }
            }

            // -----------------------------------------------------
            // Right rail: status, actions, quick settings, log
            // -----------------------------------------------------
            ColumnLayout {
                Layout.preferredWidth: 340
                Layout.minimumWidth: 300
                Layout.fillHeight: true
                spacing: 8

                EngineStatusPanel {
                    labelColWidth: wallpaperPage.labelColWidth
                    formColSpacing: wallpaperPage.formColSpacing
                    statusColor: wallpaperPage.statusColor
                }

                ActionButtons {
                    formColSpacing: wallpaperPage.formColSpacing
                    formRowSpacing: wallpaperPage.formRowSpacing
                    maybeOfferCompressFirst: wallpaperPage.maybeOfferCompressFirst
                    statusKindNotRunning: wallpaperPage.statusKindNotRunning
                }

                QuickSettingsPanel {
                    labelColWidth: wallpaperPage.labelColWidth
                    controlWidth: wallpaperPage.controlWidth
                    formColSpacing: wallpaperPage.formColSpacing
                    formRowSpacing: wallpaperPage.formRowSpacing
                }

                // --- Detail teknis (log) -------------------------
                Pane {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.minimumHeight: 100
                    Material.elevation: 1
                    padding: 8

                    LogPanel {
                        anchors.fill: parent
                        // One log view fed by both backends: the
                        // old UI had a single MainWindow log fed
                        // by ApplyManager, CompressService and
                        // the IPC client, so the lines stay
                        // interleaved here too.
                        text: Studio.log.concat(Compress.log).join("\n")
                    }
                }
            }
        }
    }
}
