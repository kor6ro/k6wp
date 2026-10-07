// Wallpaper page (Wallpaper tab). Contains the monitor sub-tabs strip, the
// preview caption + PreviewHole, the "Pasang ke" install-target row
// (AssignRow + AssignPopup, todo 12), the C-15 warning banners, the
// CollectionPage (Koleksi gallery with the Ganti otomatis panel) in the left
// column, and the right rail (StatusBar + ActionButtons +
// QuickSettingsPanel). Raw support details (process id, paths, log) are not
// shown here - they live in the "Info teknis" dialog (AppDialogs.qml).
//
// "Pasang ke" (todo 12 / C-14 / GATE 0 #3): the page owns the install path.
// With one screen it goes straight to the global video. With more, installs
// target the selected radio: "Semua layar" moves the global target and
// deletes every per-key override (applyToAllMonitors; the C-14 dialog is
// required only when overrides exist), a display choice assigns that key
// (assignVideoToMonitor). Duplicate mode forces the global path because the
// engine cannot place different videos there (IS-7).
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
    property var selectMonitor: function (key) {}
    property var fileNameOf: function (path) { return "" }
    property var selectedMonitorLabel: function () { return "" }
    property var selectedMonitorHasAssignment: function () { return false }
    property var maybeOfferCompressFirst: function (path, applyAfter) { return false }
    property var syncPreview: function () {}
    property int statusKindNotRunning: 3
    // BridgeStatusKind::kPaused, passed down to StatusBar for its one-button
    // Jeda/Lanjut matrix (Main owns the enum mirror).
    property int statusKindPaused: 1
    property var dialogOpened: function () {}
    property var dialogClosed: function () {}
    // Main.root.firstRunActive: true while the first-run onboarding overlay
    // owns the screen (plan todo 16). Forwarded to CollectionPage so the C-1
    // empty state stays hidden then.
    property bool firstRunActive: false

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

    // --- "Pasang ke" install target (todo 12 / C-14 / GATE 0 #3) -----------
    // Selected target: "all" (global) or a display key. Reset to "all"
    // whenever the chosen display disappears from the model.
    property string installTarget: "all"
    // Video waiting on the C-14 confirmation ("Semua layar" + overrides).
    property string pendingAllPath: ""
    // {daftar layar} for the C-14 sentence, built when the dialog opens.
    property string pendingOverrideText: ""

    // The display entries with a usable key, straight from the bridge model.
    readonly property var screenEntries: {
        const all = Studio.displays
        const out = []
        for (let i = 0; i < all.length; ++i) {
            if (String(all[i].key).length > 0)
                out.push(all[i])
        }
        return out
    }
    // One screen = no choice, install straight to the global video.
    readonly property bool multiScreen: screenEntries.length > 1
    // IS-7: in duplicate mode per-key assignment is off; installs go global.
    readonly property bool duplicateMode: Studio.duplicateModeNotice.length > 0

    function ensureInstallTarget() {
        if (installTarget === "all")
            return
        const all = screenEntries
        for (let i = 0; i < all.length; ++i) {
            if (String(all[i].key) === installTarget)
                return
        }
        installTarget = "all"
    }

    // Displays that would lose their per-screen video if "Semua layar" wins.
    function overrideEntries() {
        const all = screenEntries
        const out = []
        for (let i = 0; i < all.length; ++i) {
            if (String(all[i].assignedPath).length > 0)
                out.push(all[i])
        }
        return out
    }

    function showToast(text) {
        installToast.toastText = text
        installToast.visible = true
        installToastTimer.restart()
    }

    // THE install entry point: library cards, the row menu's "Pasang", the
    // right-rail "Terapkan Wallpaper" and the onboarding all funnel through
    // here (Main.qml routes Library.applyRequested to this function).
    function installVideo(path) {
        const p = String(path === undefined || path === null ? "" : path)
        if (p.length === 0)
            return
        if (!multiScreen || duplicateMode) {
            // One screen (or duplicate mode): the plain global path.
            applyGlobally(p)
            return
        }
        if (installTarget === "all") {
            installAllScreens(p)
            return
        }
        // A display choice: persist + push the per-key assignment. The
        // bridge repaints displaysChanged, so the row/badges follow.
        Studio.assignVideoToMonitor(installTarget, p)
    }

    // "Semua layar": confirmation is required ONLY when a per-key override
    // would be replaced (C-14 / NeedsAllScreensConfirm semantics). Without
    // overrides the one-shot runs directly and the toast confirms it.
    function installAllScreens(path) {
        const overrides = overrideEntries()
        if (overrides.length > 0) {
            pendingAllPath = path
            const labels = []
            for (let i = 0; i < overrides.length; ++i)
                labels.push(String(overrides[i].label))
            pendingOverrideText = labels.join(", ")
            confirmAllScreensDialog.open()
            return
        }
        runAllScreens(path)
    }

    // One C++ action: global target + delete every override; the video
    // itself is installed by the normal apply path afterwards.
    function runAllScreens(path) {
        Studio.applyToAllMonitors(path)
        applyGlobally(path)
        showToast(qsTr("Terpasang di semua layar"))
    }

    // The plain global install, compress-first offer included.
    function applyGlobally(path) {
        if (!maybeOfferCompressFirst(path, true))
            Studio.applyWallpaper(path)
    }

    Connections {
        target: Studio
        // A monitor left the model: never keep pointing the target at a key
        // that no longer exists.
        function onDisplaysChanged() { wallpaperPage.ensureInstallTarget() }
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
        // selected button scopes the preview caption and the
        // monitor actions; the install target itself is chosen
        // in the "Pasang ke" row below the preview (todo 12),
        // so selecting a sub-tab is scope-only and never
        // installs anything (the old arm-assign mode is gone).
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
                            // paints the selection.
                            flat: wallpaperPage.selectedMonitorKey !== String(modelData.key)
                            highlighted: wallpaperPage.selectedMonitorKey === String(modelData.key)
                            onClicked: wallpaperPage.selectMonitor(String(modelData.key))

                            // Row 42: the raw device key was a
                            // developer string in a user-facing
                            // bubble; say what the click does.
                            ToolTip.visible: hovered
                            ToolTip.text: qsTr("Pilih layar ini")

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

                                // Degraded: an assignment exists
                                // but its file is missing.
                                Label {
                                    visible: String(modelData.assignedPath).length > 0
                                             && modelData.assignedExists === false
                                    text: qsTr("file tidak ditemukan")
                                    color: "red"
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
                    // invokable.
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

                // C-15 banners (todo 12): each renders ONLY while its
                // problem is real (no problem -> no banner, B-STATE). They
                // sit above the preview so the AssignPopup, which opens
                // below the "Pasang ke" row, can never cover them.
                WarningBanner {
                    id: duplicateBanner
                    Layout.fillWidth: true
                    kind: "duplicate"
                }

                WarningBanner {
                    id: coverageBanner
                    Layout.fillWidth: true
                    kind: "coverage"
                    displays: wallpaperPage.screenEntries
                }

                PreviewHole {
                    syncPreview: wallpaperPage.syncPreview
                    onGeometryChanged: wallpaperPage.refreshPreviewHoleGeometry()
                }

                // "Pasang ke" (todo 12): the install-target row, visible ONLY
                // while Studio.displays models more than one screen. The
                // popup opens below it, into the gallery area - never over
                // the preview hole (B-WIREFRAME(f)).
                AssignRow {
                    id: assignRow
                    Layout.fillWidth: true
                    displays: wallpaperPage.screenEntries
                    target: wallpaperPage.installTarget
                    duplicateMode: wallpaperPage.duplicateMode
                    onPopupRequested: assignPopup.open()
                }

                // Koleksi: search + import + the Ganti otomatis panel + the
                // large-thumbnail gallery. Card row menus and dialogs only
                // ever open BELOW this area (the preview hole above it must
                // stay uncovered). The preferred/minimum heights are what
                // reserve the gallery (and the C-11 panel inside it) its
                // share of the column: without them a fillHeight ColumnLayout
                // has an implicit height of nearly zero and the preview takes
                // the whole column. Todo 10: the C-11 panel adds ~170px, so
                // the reservation grew from the old 320/260.
                CollectionPage {
                    id: collectionPage
                    Layout.minimumHeight: 300
                    Layout.preferredHeight: 460
                    dialogOpened: wallpaperPage.dialogOpened
                    dialogClosed: wallpaperPage.dialogClosed
                    firstRunActive: wallpaperPage.firstRunActive
                }

            }

            // -----------------------------------------------------
            // Right rail: status, actions, quick settings
            // -----------------------------------------------------
            ColumnLayout {
                Layout.preferredWidth: 340
                Layout.minimumWidth: 300
                Layout.fillHeight: true
                spacing: 8

                StatusBar {
                    statusKindPaused: wallpaperPage.statusKindPaused
                }

                ActionButtons {
                    formColSpacing: wallpaperPage.formColSpacing
                    formRowSpacing: wallpaperPage.formRowSpacing
                    statusKindNotRunning: wallpaperPage.statusKindNotRunning
                    // "Terapkan Wallpaper" now runs the same "Pasang ke"
                    // install path as the gallery cards (todo 12).
                    installVideo: wallpaperPage.installVideo
                }

                QuickSettingsPanel {
                    labelColWidth: wallpaperPage.labelColWidth
                    controlWidth: wallpaperPage.controlWidth
                    formColSpacing: wallpaperPage.formColSpacing
                    formRowSpacing: wallpaperPage.formRowSpacing
                }

                // Absorbs the rail's leftover height so the panes above stay
                // pinned to the top. A ColumnLayout without an expansive
                // child keeps its implicit height and the row centers it
                // (the removed log pane used to play this role).
                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }
            }
        }
    }

    // --- "Pasang ke" popup + C-14 confirmation + toast ---------------------
    // The popup hangs from the row (below it, gallery area). It is a Popup,
    // not a Dialog: overlay matrix B-WIREFRAME(f) forbids popups over the
    // preview hole, and it hides the preview only on the narrow-window
    // fallback path inside AssignPopup.qml.
    AssignPopup {
        id: assignPopup
        anchorItem: assignRow
        pageItem: wallpaperPage
        displays: wallpaperPage.screenEntries
        target: wallpaperPage.installTarget
        onTargetChosen: function (key) { wallpaperPage.installTarget = key }
    }

    // C-14 ("Semua layar" + active overrides). A real Dialog, so it feeds
    // dialogOpened/dialogClosed and the native preview steps aside while it
    // is up (overlay matrix: ALL dialogs hide the preview).
    Dialog {
        id: confirmAllScreensDialog
        objectName: "confirmAllScreensDialog"
        title: qsTr("Pasang ke semua layar?")
        modal: true
        anchors.centerIn: Overlay.overlay
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        onOpened: {
            wallpaperPage.dialogOpened()
            confirmAllCancel.forceActiveFocus()
        }
        onClosed: wallpaperPage.dialogClosed()

        contentItem: ColumnLayout {
            spacing: Theme.space1
            Accessible.role: Accessible.Dialog
            Accessible.name: confirmAllScreensDialog.title

            Label {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                text: qsTr("Video khusus di %1 akan ikut diganti.")
                      .arg(wallpaperPage.pendingOverrideText)
            }
        }

        footer: RowLayout {
            spacing: Theme.space2
            Item {
                Layout.fillWidth: true
            }
            Button {
                objectName: "confirmAllScreensAccept"
                implicitHeight: 40
                Accessible.name: text
                text: qsTr("Pasang ke semua")
                onClicked: {
                    confirmAllScreensDialog.close()
                    wallpaperPage.runAllScreens(wallpaperPage.pendingAllPath)
                }
            }
            Button {
                id: confirmAllCancel
                objectName: "confirmAllScreensCancel"
                implicitHeight: 40
                Accessible.name: text
                text: qsTr("Batal")
                onClicked: confirmAllScreensDialog.close()
            }
        }
    }

    // C-14 toast: the no-override "Semua layar" path confirms with this
    // sentence (never an invented string). Anchored to the page's bottom
    // edge, gallery area - never over the preview hole.
    Rectangle {
        id: installToast
        objectName: "installToast"
        property string toastText: ""
        visible: false
        z: 2
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Theme.space2
        implicitWidth: Math.min(parent.width - 2 * Theme.space2,
                                toastRow.implicitWidth + 2 * Theme.space3)
        implicitHeight: toastRow.implicitHeight + 2 * Theme.space2
        radius: Theme.radiusM
        color: Theme.statusActiveTint
        border.width: 1
        border.color: Theme.surface2

        Accessible.role: Accessible.AlertMessage
        Accessible.name: installToast.toastText

        RowLayout {
            id: toastRow
            anchors.fill: parent
            anchors.margins: Theme.space2
            spacing: Theme.space2

            Label {
                text: "\u2713"
                color: Theme.statusActive
                font.pixelSize: Theme.fontM
                font.weight: Theme.fontWeightSemibold
                Accessible.ignored: true
            }

            Label {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                color: Theme.text
                font.pixelSize: Theme.fontM
                text: installToast.toastText
            }
        }
    }

    Timer {
        id: installToastTimer
        interval: 4000
        repeat: false
        onTriggered: installToast.visible = false
    }

}


