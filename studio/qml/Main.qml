// Phase 1 shell (Qt6 Widgets -> QML migration).
//
// Root is a plain Rectangle, NOT ApplicationWindow: this document is hosted by
// a QQuickWidget, which renders QQuickItems, not a top-level QQuickWindow. The
// QQuickStyle is set to "Material" in main.cpp BEFORE this file is loaded, so
// every Control below (TabBar, StackLayout, Pane, Label, ...) is the Material
// implementation.
//
// The `import QtQuick.Controls.Material` line is required even though the
// style is chosen from C++: the Material attached type (Material.theme /
// .accent / .primary / .elevation) is exported only under the
// QtQuick.Controls.Material module URI (verified in
// qml/QtQuick/Controls/Material/plugins.qmltypes:
// `exports: ["QtQuick.Controls.Material/Material 2.0", ...]`), never under
// QtQuick.Controls. QQuickStyle::setStyle only switches which control
// IMPLEMENTATION wins; it does not bring the attached property into scope.
//
// Language: Indonesian, hardcoded. The project deliberately removed its i18n
// scaffolding (MED-11), so there are no qsTr() calls here.
//
// Spacing is Material 8dp: 8 / 16 / 24.
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import K6WP

Rectangle {
    id: root
    anchors.fill: parent

    // Follow the OS light/dark setting. The app's existing branding is indigo,
    // so accent + primary are Material.Indigo and the rest is derived by the
    // style.
    Material.theme: Material.System
    Material.accent: Material.Indigo
    Material.primary: Material.Indigo

    // The Pane below paints the window surface, so the root stays clear.
    color: "transparent"

    // Duration reported by CompressBridge's long-video gate, in minutes.
    property double consentDuration: 0

    // Stable mirror of k6wp::BridgeStatusKind (studio_bridge.hpp). Kept in the
    // same order; a new C++ kind is rejected at compile time by the assert in
    // StudioBridge::ApplyStatus.
    enum StatusKind {
        Connected = 0,
        Paused = 1,
        Degraded = 2,
        NotRunning = 3,
        Disconnected = 4
    }

    // --- shared sizing tokens ---------------------------------------------
    // Every settings row is a 2-column GridLayout so labels and controls line
    // up by construction; these keep the columns identical across rows and
    // across the three tabs. Without them each row picked its own widths, which
    // is what made controls start at different x.
    readonly property int labelColWidth: 132
    readonly property int controlWidth: 320
    readonly property int narrowControlWidth: 220
    readonly property int spinWidth: 140
    readonly property int formColSpacing: 12
    readonly property int formRowSpacing: 10

    // Same four colours MainWindow::OnPollDone painted the status label with.
    function statusColor(kind) {
        switch (kind) {
        case StatusKind.Connected:
            return "green";
        case StatusKind.Paused:
        case StatusKind.Degraded:
            return "orange";
        case StatusKind.NotRunning:
            return "gray";
        default:
            return "red";
        }
    }

    // Pushes the preview hole's rectangle to the native PreviewWidget, in
    // QQuickWidget scene coordinates (the space QmlShell maps into window
    // coordinates). mapToItem(null, ...) resolves against the scene root, so
    // the hole's position inside its page is accounted for.
    //
    // A collapsed / hidden hole is pushed as a 0x0 rect, which QmlShell treats
    // as "hide the native surface" - that is what keeps the preview from
    // floating over the Kompresor / Pengaturan pages.
    function dialogOpened() {
        root.openDialogs += 1;
        Studio.syncPreviewGeometry(0, 0, 0, 0);
    }

    function dialogClosed() {
        root.openDialogs = Math.max(0, root.openDialogs - 1);
        if (root.openDialogs === 0)
            root.syncPreview();
    }

    function syncPreview() {
        if (root.openDialogs > 0)
            return;
        if (!previewHole.visible) {
            Studio.syncPreviewGeometry(0, 0, 0, 0);
            return;
        }
        const topLeft = previewHole.mapToItem(null, 0, 0);
        Studio.syncPreviewGeometry(topLeft.x, topLeft.y,
                                   previewHole.width, previewHole.height);
    }

    // The library model raises intents rather than acting on them: applying a
    // wallpaper is a StudioBridge call and re-compressing is a CompressBridge
    // call, so the wiring lives here instead of coupling the model to both.
    Connections {
        target: Library
        function onApplyRequested(dst) {
            if (!root.maybeOfferCompressFirst(dst, true))
                Studio.applyWallpaper(dst)
        }
        function onRecompressRequested(dst) {
            Compress.setSourcePath(dst)
            root.showTab(1)
        }
        // An imported file was over the threshold, so the model deliberately
        // did not reference it. With auto-compress on, just run it; otherwise
        // ask. The apply path is a separate signal and always offers.
        function onCompressFirstRequired(path) {
            root.offerPath = path
            root.offerMb = Studio.compressFirstOfferMb(path)
            root.offerApplyAfter = false
            if (Settings.autoCompressOnImport)
                root.startCompressFirst()
            else
                compressOfferDialog.open()
        }
    }

    function showTab(index) {
        tabBar.currentIndex = index
    }

    // --- Compress-first offer (restored from MainWindow::MaybeOfferCompressFirst,
    // deleted with the legacy widget tree in f03c850) ------------------------
    property string offerPath: ""
    property int offerMb: 0
    property bool offerApplyAfter: false

    // True when the offer is now showing, so the caller must skip its normal
    // action. The threshold lives in compress_first_offer.hpp.
    function maybeOfferCompressFirst(path, applyAfter) {
        if (path.length === 0)
            return false
        const mb = Studio.compressFirstOfferMb(path)
        if (mb < 0)
            return false
        offerPath = path
        offerMb = mb
        offerApplyAfter = applyAfter
        compressOfferDialog.open()
        return true
    }

    // The prompt promises 1080p, so the job is pinned to 1920x1080 instead of
    // inheriting whatever the resolution box happens to hold.
    //
    // applyAfterCompress - not Compress.autoApply - is how the apply path gets
    // its result applied. autoApply is the user's own "Langsung terapkan
    // setelah selesai" checkbox, and the offer used to write it, so accepting
    // an offer silently ticked (or unticked) a preference the user never
    // touched. This flag belongs to the offer and is one-shot.
    property bool applyAfterCompress: false
    // Modals stack (e.g. first-run wizard + compress offer): keep the native
    // preview hidden until the last one closes.
    property int openDialogs: 0

    function startCompressFirst() {
        Compress.setSourcePath(offerPath)
        Compress.setResolutionText("1920x1080")
        root.applyAfterCompress = offerApplyAfter
        showTab(1)
        Compress.start()
    }

    // Answering "No" (or Esc) means "do not compress first" - the action
    // that raised the offer still has to run, otherwise any video over the
    // threshold can never be applied: the caller already skipped its own
    // apply when maybeOfferCompressFirst returned true. The import path sets
    // offerApplyAfter = false, so declining is correctly a no-op there (that
    // file was deliberately left out of the library).
    function declineCompressFirst() {
        if (offerApplyAfter)
            Studio.applyWallpaper(offerPath)
        offerApplyAfter = false
    }

    Connections {
        target: Compress
        function onJobFinishedWithoutResult() {
            root.applyAfterCompress = false
        }
        // A QML control's own value binding is destroyed by the first user
        // edit, so re-assert every field when the backend reports a change
        // (e.g. startCompressFirst() setting the 1080p pin) instead of relying
        // on a stale one-shot binding.
        function onInputsChanged() {
            if (crfBox)
                crfBox.value = Compress.crf
            if (fpsBox)
                fpsBox.value = Compress.fps
            if (encoderCombo)
                encoderCombo.currentIndex = Math.max(0, encoderCombo.model.indexOf(Compress.encoder))
            if (advancedBox)
                advancedBox.checked = Compress.advanced
            if (resolutionField && !resolutionField.activeFocus)
                resolutionField.text = Compress.resolutionText
        }
    }

    Connections {
        target: Studio
        function onQuickSettingsChanged() {
            if (fitCombo) {
                const i = fitCombo.modes.indexOf(Studio.quickFit)
                fitCombo.currentIndex = i >= 0 ? i : 0
            }
            if (monitorCombo) {
                let m = 0
                for (let i = 0; i < Studio.monitorChoices.length; ++i) {
                    if (Studio.monitorChoices[i].id === Studio.quickMonitor) {
                        m = i
                        break
                    }
                }
                monitorCombo.currentIndex = m
            }
            if (autostartBox)
                autostartBox.checked = Studio.quickAutostart
            if (batteryBox)
                batteryBox.checked = Studio.quickBattery
        }
    }

    Pane {
        anchors.fill: parent
        padding: 0
        Material.elevation: 1

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            MenuBar {
                id: menuBar

                Menu {
                    title: qsTr("&Berkas")

                    MenuItem {
                        text: qsTr("&Impor Video...")
                        onTriggered: Library.pickAndImport()
                    }

                    MenuSeparator {}

                    MenuItem {
                        text: qsTr("&Keluar")
                        onTriggered: Qt.quit()
                    }
                }

                Menu {
                    title: qsTr("&Bantuan")

                    MenuItem {
                        text: qsTr("Check for updates")
                        enabled: !Studio.updateCheckBusy
                        onTriggered: Studio.checkForUpdatesInteractive()
                    }

                    MenuItem {
                        text: qsTr("&Tentang K6WP Studio")
                        onTriggered: aboutDialog.open()
                    }
                }
            }

            Label {
                id: updateCheckMessageLabel
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 6
                wrapMode: Text.WordWrap
                visible: Studio.updateCheckMessage.length > 0
                text: Studio.updateCheckMessage
            }

            TabBar {
                id: tabBar
                Layout.fillWidth: true

                // Re-read on entry: the Wallpaper tab's quick settings mirror
                // config.json + the autostart registry, either of which the
                // Pengaturan tab can change.
                onCurrentIndexChanged: if (currentIndex === 0) Studio.refreshQuickSettings()

                // Tab labels are the existing Studio tab names.
                TabButton {
                    text: qsTr("Wallpaper")
                }
                TabButton {
                    text: qsTr("Kompresor")
                }
                TabButton {
                    text: qsTr("Pengaturan")
                }
            }

            StackLayout {
                id: pages
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: tabBar.currentIndex

                // ---------------------------------------------------------
                // Wallpaper
                // ---------------------------------------------------------
                Item {
                    id: wallpaperPage

                    // Two columns instead of one tall stack: the old layout
                    // pushed everything full-width down the page, so a 1936px
                    // window left ~75% of the width empty while the preview sat
                    // in a 1900x320 letterbox. The left column carries the
                    // content that benefits from area (preview + library); the
                    // right rail is a fixed 340px for the things that are
                    // short text or fixed controls.
                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 8

                        // -----------------------------------------------------
                        // Left: preview + library
                        // -----------------------------------------------------
                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            spacing: 8

                            Label {
                                text: qsTr("Pratinjau:")
                            }

                            // The hole for the native mpv preview. Its rectangle
                            // is pushed to QmlShell on every change; the native
                            // PreviewWidget then paints exactly on top of it, so
                            // this Pane is what the user sees before mpv has its
                            // first frame (and if the preview is ever hidden).
                            Pane {
                                id: previewHole
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Layout.minimumHeight: 200
                                Layout.preferredHeight: 340
                                Material.elevation: 2

                                onXChanged: root.syncPreview()
                                onYChanged: root.syncPreview()
                                onWidthChanged: root.syncPreview()
                                onHeightChanged: root.syncPreview()
                                // StackLayout toggles `visible` when the tab
                                // changes, so the preview follows the page.
                                onVisibleChanged: root.syncPreview()
                                Component.onCompleted: root.syncPreview()

                                HoverHandler {
                                    id: holeHover
                                }
                                ToolTip.visible: holeHover.hovered
                                ToolTip.text: qsTr("Klik untuk jeda/jalan")

                                // Cached thumbnail, so the hole is not black
                                // while libmpv starts. Covered by the native
                                // surface once mpv renders. `visible: status ===
                                // Image.Ready` keeps a missing/blank file
                                // silent.
                                Image {
                                    id: poster
                                    anchors.fill: parent
                                    fillMode: Image.PreserveAspectFit
                                    asynchronous: true
                                    visible: status === Image.Ready
                                    source: Studio.posterPath.length > 0
                                            ? "file:///" + Studio.posterPath.replace(/\\/g, "/")
                                            : ""
                                }
                            }

                            // --- Perpustakaan (library grid) -----------------
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                Label {
                                    text: qsTr("Perpustakaan:")
                                    font.bold: true
                                }

                                TextField {
                                    id: librarySearch
                                    Layout.fillWidth: true
                                    placeholderText: qsTr("Cari video...")
                                    onTextChanged: Library.filter = text
                                }

                                Button {
                                    text: qsTr("&Impor Video")
                                    onClicked: Library.pickAndImport()
                                    ToolTip.visible: hovered
                                    ToolTip.text: qsTr("Impor video ke perpustakaan untuk mulai")
                                }
                            }

                            Label {
                                Layout.fillWidth: true
                                visible: Library.lastError.length > 0
                                wrapMode: Text.WordWrap
                                color: "red"
                                text: Library.lastError
                            }

                            // Empty state: distinct copy for "nothing imported
                            // yet" vs "no search results", which is what the
                            // old grid_widget empty label keyed off.
                            Label {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 48
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                wrapMode: Text.WordWrap
                                visible: Library.visibleCount === 0
                                opacity: 0.7
                                text: Library.isEmpty
                                      ? qsTr("Perpustakaan kosong — impor video lewat Berkas > Impor atau seret & letakkan")
                                      : qsTr("Tidak ada hasil untuk pencarian ini")
                            }

                            GridView {
                                id: libraryGrid
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Layout.minimumHeight: 140
                                Layout.preferredHeight: 220
                                clip: true
                                visible: Library.visibleCount > 0
                                model: Library

                                cellWidth: 190
                                cellHeight: 170
                                boundsBehavior: Flickable.StopAtBounds

                                delegate: Item {
                                    width: libraryGrid.cellWidth
                                    height: libraryGrid.cellHeight

                                    required property int index
                                    required property string name
                                    required property string label
                                    required property string thumbUrl
                                    required property string dst
                                    required property bool broken

                                    // thumbUrl stays empty until a thumbnail
                                    // exists, and an Image with no source never
                                    // changes status - so the onStatusChanged
                                    // trigger below can never fire on a first
                                    // run. Request the thumbnail here instead.
                                    Component.onCompleted: if (thumbUrl.length === 0)
                                        Library.ensureThumbnail(index)

                                    Pane {
                                        anchors.fill: parent
                                        anchors.margins: 4
                                        Material.elevation: 1

                                        ColumnLayout {
                                            anchors.fill: parent
                                            anchors.margins: 8
                                            spacing: 4

                                            Image {
                                                Layout.fillWidth: true
                                                Layout.fillHeight: true
                                                fillMode: Image.PreserveAspectFit
                                                asynchronous: true
                                                visible: status === Image.Ready
                                                source: thumbUrl
                                                // Ask the model for a thumbnail once
                                                // the row is realised; the model
                                                // debounces so a scroll cannot
                                                // spawn ffmpeg repeatedly.
                                                onStatusChanged: if (status !== Image.Ready)
                                                                     Library.ensureThumbnail(index)
                                            }

                                            Label {
                                                Layout.fillWidth: true
                                                maximumLineCount: 2
                                                elide: Text.ElideRight
                                                wrapMode: Text.WordWrap
                                                font.pixelSize: 11
                                                text: label
                                            }
                                        }

                                        MouseArea {
                                            anchors.fill: parent
                                            hoverEnabled: true
                                            acceptedButtons: Qt.LeftButton | Qt.RightButton
                                            onDoubleClicked: Library.applyAt(index)
                                            onClicked: function (mouse) {
                                                if (mouse.button === Qt.RightButton)
                                                    rowMenu.popup()
                                            }
                                        }

                                        Menu {
                                            id: rowMenu
                                            MenuItem {
                                                text: qsTr("Terapkan")
                                                onTriggered: Library.applyAt(index)
                                            }
                                            MenuItem {
                                                text: qsTr("Kompres-ulang")
                                                onTriggered: Library.recompressAt(index)
                                            }
                                            MenuItem {
                                                text: qsTr("Buka Lokasi")
                                                onTriggered: Library.openLocationAt(index)
                                            }
                                            MenuItem {
                                                text: qsTr("Hapus")
                                                onTriggered: removeDialog.open()
                                            }
                                        }
                                    }

                                    Dialog {
                                        id: removeDialog
                                        title: qsTr("Hapus Entri Perpustakaan")
                                        modal: true
                                        anchors.centerIn: Overlay.overlay
                                        standardButtons: Dialog.Yes | Dialog.No
                                        onAccepted: Library.removeAt(index, true)
                                        onOpened: root.dialogOpened()
                                        onClosed: root.dialogClosed()

                                        contentItem: Label {
                                            text: qsTr("Hapus entri untuk:\n%1"
                                                       + "\n\nFile dipindahkan ke Recycle Bin.")
                                                  .arg(dst)
                                            wrapMode: Text.WordWrap
                                        }
                                    }
                                }
                            }

                            // --- Daftar putar (playlist) ---------------------
                            // Small editing surface for playlist.json: the engine
                            // reads that file and rotates on its own, so Studio
                            // only adds / removes / reorders entries here.
                            Pane {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 170
                                Material.elevation: 1
                                padding: 12

                                ColumnLayout {
                                    anchors.fill: parent
                                    spacing: 6

                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: 8

                                        Label {
                                            text: qsTr("Daftar putar (%1)").arg(Playlist.count)
                                            font.bold: true
                                        }

                                        Item {
                                            Layout.fillWidth: true
                                        }

                                        Button {
                                            text: qsTr("Tambah...")
                                            onClicked: Playlist.pickAndAdd()
                                            ToolTip.visible: hovered
                                            ToolTip.text: qsTr("Tambah video ke daftar putar")
                                        }

                                        Button {
                                            text: qsTr("Kosongkan")
                                            enabled: Playlist.count > 0
                                            onClicked: Playlist.clear()
                                            ToolTip.visible: hovered
                                            ToolTip.text: qsTr("Hapus semua entri daftar putar")
                                        }
                                    }

                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: 8

                                        CheckBox {
                                            text: qsTr("Putar otomatis")
                                            checked: Playlist.enabled
                                            onToggled: Playlist.setEnabled(checked)
                                            ToolTip.visible: hovered
                                            ToolTip.text: qsTr("Ganti wallpaper secara berkala")
                                        }

                                        Label {
                                            text: qsTr("Interval:")
                                        }

                                        SpinBox {
                                            id: intervalSpin
                                            Layout.preferredWidth: root.spinWidth
                                            from: 1
                                            to: 1440
                                            editable: true
                                            value: Playlist.intervalMin
                                            onValueModified: Playlist.setIntervalMin(value)
                                            ToolTip.visible: hovered
                                            ToolTip.text: qsTr("Menit antar pergantian (1-1440)")
                                        }

                                        Label {
                                            text: qsTr(" menit")
                                        }

                                        Item {
                                            Layout.fillWidth: true
                                        }

                                        CheckBox {
                                            text: qsTr("Acak")
                                            checked: Playlist.shuffle
                                            onToggled: Playlist.setShuffle(checked)
                                            ToolTip.visible: hovered
                                            ToolTip.text: qsTr("Putar daftar putar dalam urutan acak")
                                        }
                                    }

                                    Label {
                                        Layout.fillWidth: true
                                        visible: Playlist.lastError.length > 0
                                        wrapMode: Text.WordWrap
                                        color: "red"
                                        text: Playlist.lastError
                                    }

                                    Label {
                                        Layout.fillWidth: true
                                        Layout.fillHeight: true
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                        wrapMode: Text.WordWrap
                                        visible: Playlist.count === 0
                                        opacity: 0.7
                                        text: qsTr("Belum ada video di daftar putar — tambah dari perpustakaan atau berkas")
                                    }

                                    ListView {
                                        id: playlistList
                                        Layout.fillWidth: true
                                        Layout.fillHeight: true
                                        clip: true
                                        visible: Playlist.count > 0
                                        model: Playlist.items
                                        boundsBehavior: Flickable.StopAtBounds
                                        spacing: 2

                                        delegate: RowLayout {
                                            required property var modelData
                                            required property int index

                                            width: playlistList.width
                                            spacing: 6
                                            opacity: modelData.exists === false ? 0.6 : 1.0

                                            Label {
                                                Layout.fillWidth: true
                                                elide: Text.ElideMiddle
                                                text: modelData.label
                                                color: modelData.exists === false
                                                       ? "red"
                                                       : Material.foreground
                                            }

                                            Button {
                                                text: qsTr("Naik")
                                                enabled: index > 0
                                                onClicked: Playlist.moveUp(index)
                                            }

                                            Button {
                                                text: qsTr("Turun")
                                                enabled: index < Playlist.count - 1
                                                onClicked: Playlist.moveDown(index)
                                            }

                                            Button {
                                                text: qsTr("Hapus")
                                                onClicked: Playlist.removeAt(index)
                                            }
                                        }
                                    }
                                }
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

                            Pane {
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
                                        columnSpacing: root.formColSpacing
                                        rowSpacing: 6

                                        Label {
                                            text: qsTr("Status engine:")
                                            font.bold: true
                                            Layout.preferredWidth: root.labelColWidth
                                        }

                                        Label {
                                            id: statusLabel
                                            Layout.fillWidth: true
                                            elide: Text.ElideMiddle
                                            text: Studio.engineStatusDetail
                                            color: root.statusColor(Studio.engineStatusKind)

                                            HoverHandler {
                                                id: statusHover
                                            }
                                            ToolTip.visible: statusHover.hovered
                                            ToolTip.text: Studio.engineStatusHint
                                        }

                                        Label {
                                            text: qsTr("Video saat ini:")
                                            Layout.preferredWidth: root.labelColWidth
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
                                            Layout.preferredWidth: root.labelColWidth
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
                                            Layout.preferredWidth: root.labelColWidth
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

                            // Content-width buttons in a 2-column grid. They used
                            // to fill their cell, so "Pilih Video" and "Terapkan
                            // Wallpaper" came out different widths and the row
                            // looked ragged. No fillWidth anywhere here: the grid
                            // hugs its content and sits left in the rail.
                            GridLayout {
                                columns: 2
                                columnSpacing: root.formColSpacing
                                rowSpacing: root.formRowSpacing

                                Button {
                                    text: qsTr("Pilih &Video")
                                    onClicked: Studio.pickVideo()
                                    ToolTip.visible: hovered
                                    ToolTip.text: qsTr("Pilih file untuk diterapkan tanpa impor ke perpustakaan")
                                }

                                Button {
                                    text: qsTr("&Terapkan Wallpaper")
                                    enabled: Studio.selectedVideo.length > 0 && !Studio.busy
                                    onClicked: {
                                        if (!root.maybeOfferCompressFirst(Studio.selectedVideo, true))
                                            Studio.applyWallpaper(Studio.selectedVideo)
                                    }
                                    ToolTip.visible: hovered
                                    ToolTip.text: qsTr("Terapkan video terpilih sebagai wallpaper (live-switch, tanpa restart saat engine jalan)")
                                }

                                Button {
                                    text: qsTr("Hapus")
                                    enabled: Studio.selectedVideo.length > 0
                                    onClicked: Studio.clearSelectedVideo()
                                    ToolTip.visible: hovered
                                    ToolTip.text: qsTr("Hapus dari daftar")
                                }

                                // Visible only on NotRunning, the same rule
                                // MainWindow::OnPollDone used for its start button.
                                Button {
                                    text: qsTr("Nyalakan Engine")
                                    visible: Studio.engineStatusKind === 3
                                    enabled: !Studio.busy
                                    onClicked: Studio.startEngine()
                                    ToolTip.visible: hovered
                                    ToolTip.text: qsTr("Menyalakan engine wallpaper (muncul saat engine mati)")
                                }

                                Button {
                                    text: qsTr("Hentikan Wallpaper")
                                    enabled: Studio.engineRunning && !Studio.busy
                                    onClicked: Studio.pause()
                                    ToolTip.visible: hovered
                                    ToolTip.text: qsTr("Hentikan wallpaper (IPC pause, video tetap termuat)")
                                }

                                Button {
                                    text: qsTr("Lanjutkan")
                                    enabled: Studio.engineRunning && !Studio.busy
                                    onClicked: Studio.resume()
                                    ToolTip.visible: hovered
                                    ToolTip.text: qsTr("Lanjutkan engine yang dijeda (IPC, tanpa restart)")
                                }

                                BusyIndicator {
                                    Layout.alignment: Qt.AlignVCenter
                                    running: Studio.busy
                                    visible: Studio.busy
                                }
                            }

                            // --- Pengaturan cepat -----------------------------
                            // The fit / monitor / autostart / battery-saver
                            // controls from wallpaper_tab_widget. Each writes one
                            // engine config field; the engine's config watcher
                            // applies it.
                            Pane {
                                Layout.fillWidth: true
                                Material.elevation: 1
                                padding: 12

                                ColumnLayout {
                                    anchors.fill: parent
                                    spacing: root.formRowSpacing

                                    Label {
                                        text: qsTr("Pengaturan cepat")
                                        font.bold: true
                                    }

                                    // One grid for both combos so "Isi layar:"
                                    // and "Layar:" share a label column and the
                                    // two dropdowns start (and end) together.
                                    GridLayout {
                                        Layout.fillWidth: true
                                        columns: 2
                                        columnSpacing: root.formColSpacing
                                        rowSpacing: root.formRowSpacing

                                        Label {
                                            text: qsTr("Isi layar:")
                                            Layout.preferredWidth: root.labelColWidth
                                        }

                                        ComboBox {
                                            id: fitCombo
                                            Layout.fillWidth: true
                                            Layout.maximumWidth: root.controlWidth
                                            model: [qsTr("Isi layar (potong bila perlu)"),
                                                    qsTr("Sesuaikan (seluruh video terlihat)"),
                                                    qsTr("Regang (isi penuh)"),
                                                    qsTr("Tengah (ukuran asli)")]
                                            // Order matches the fit_mode enum in
                                            // shared/config_schema.cpp.
                                            readonly property var modes: ["cover", "fit",
                                                                       "stretch", "center"]
                                            currentIndex: {
                                                const i = modes.indexOf(Studio.quickFit)
                                                return i >= 0 ? i : 0
                                            }
                                            onActivated: Studio.setQuickFit(modes[index])
                                            ToolTip.visible: hovered
                                            ToolTip.text: qsTr("Cara video mengisi layar")
                                        }

                                        Label {
                                            text: qsTr("Layar:")
                                            Layout.preferredWidth: root.labelColWidth
                                        }

                                        ComboBox {
                                            id: monitorCombo
                                            Layout.fillWidth: true
                                            Layout.maximumWidth: root.controlWidth
                                            model: Studio.monitorChoices
                                            textRole: "text"
                                            valueRole: "id"
                                            currentIndex: {
                                                for (let i = 0; i < Studio.monitorChoices.length; ++i) {
                                                    if (Studio.monitorChoices[i].id === Studio.quickMonitor)
                                                        return i
                                                }
                                                return 0
                                            }
                                            onActivated: Studio.setQuickMonitor(
                                                         Studio.monitorChoices[index].id)
                                        }
                                    }

                                    CheckBox {
                                        id: autostartBox
                                        Layout.fillWidth: true
                                        text: qsTr("Jalankan saat Windows menyala")
                                        checked: Studio.quickAutostart
                                        onToggled: Studio.setQuickAutostart(checked)
                                    }

                                    CheckBox {
                                        id: batteryBox
                                        Layout.fillWidth: true
                                        text: qsTr("Hemat baterai (wallpaper berhenti saat pakai baterai)")
                                        checked: Studio.quickBattery
                                        onToggled: Studio.setQuickBattery(checked)
                                    }
                                }
                            }

                            // --- Detail teknis (log) -------------------------
                            Pane {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Layout.minimumHeight: 100
                                Material.elevation: 1
                                padding: 8

                                ColumnLayout {
                                    anchors.fill: parent
                                    spacing: 8

                                    Label {
                                        text: qsTr("Detail teknis (log)")
                                        font.bold: true
                                    }

                                    ScrollView {
                                        Layout.fillWidth: true
                                        Layout.fillHeight: true
                                        clip: true

                                        TextArea {
                                            id: logView
                                            readOnly: true
                                            wrapMode: TextArea.NoWrap
                                            selectByMouse: true
                                            // One log view fed by both backends: the
                                            // old UI had a single MainWindow log fed
                                            // by ApplyManager, CompressService and
                                            // the IPC client, so the lines stay
                                            // interleaved here too.
                                            text: Studio.log.concat(Compress.log).join("\n")
                                            placeholderText: qsTr("Log terapkan muncul di sini...")
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // ---------------------------------------------------------
                // Kompresor
                // ---------------------------------------------------------
                Item {
                    id: compressorPage

                    // The long-video gate. AskLongVideoConsent is a QMessageBox
                    // in C++, so the decision is routed through QML instead.
                    Connections {
                        target: Compress
                        function onConsentRequired(durationMinutes) {
                            consentDuration = durationMinutes
                            consentDialog.open()
                        }
                    }

                    // "Langsung terapkan setelah selesai" plus the compress-first
                    // offer's own one-shot intent: applying is a StudioBridge
                    // call, so it is wired here rather than in C++.
                    Connections {
                        target: Compress
                        function onResultChanged() {
                            if (!Compress.hasResult)
                                return
                            // The offer's intent wins over the checkbox, and the
                            // flag is spent here. A job that never yields a
                            // result is covered by the Start button, which
                            // clears the flag so it cannot leak into an
                            // unrelated later compress.
                            const apply_it = root.applyAfterCompress || Compress.autoApply
                            root.applyAfterCompress = false
                            if (apply_it)
                                Studio.applyWallpaper(Compress.resultPath)
                        }
                    }

                    // Bounded + two columns. Previously one full-width vertical
                    // stack, so on a 1936px window "Sumber:" sat at x=10 with its
                    // Jelajahi button ~1900px away and the bottom 40% was dead
                    // space. The left column holds the paths and the compress
                    // job; the right rail the technical knobs.
                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 8

                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            spacing: 8

                            // Label | value | actions in ONE grid. As two
                            // separate RowLayouts the "Sumber:" and "Folder
                            // output:" labels had different widths and each
                            // button was flung to the far window edge, ~1900px
                            // from its label.
                            GridLayout {
                                Layout.fillWidth: true
                                columns: 3
                                columnSpacing: root.formColSpacing
                                rowSpacing: root.formRowSpacing

                                Label {
                                    text: qsTr("Sumber:")
                                    Layout.preferredWidth: root.labelColWidth
                                }

                                Label {
                                    Layout.fillWidth: true
                                    Layout.maximumWidth: 480
                                    elide: Text.ElideMiddle
                                    opacity: 0.8
                                    text: Compress.sourcePath.length > 0
                                          ? Compress.sourcePath
                                          : qsTr("(belum dipilih — klik Jelajahi)")

                                    HoverHandler {
                                        id: srcHover
                                    }
                                    ToolTip.visible: srcHover.hovered
                                    ToolTip.text: Compress.sourcePath
                                }

                                Button {
                                    text: qsTr("&Jelajahi...")
                                    onClicked: Compress.pickSource()
                                    ToolTip.visible: hovered
                                    ToolTip.text: qsTr("Pilih file video untuk dikompres")
                                }

                                Label {
                                    text: qsTr("Folder output:")
                                    Layout.preferredWidth: root.labelColWidth
                                }

                                Label {
                                    Layout.fillWidth: true
                                    Layout.maximumWidth: 480
                                    elide: Text.ElideMiddle
                                    opacity: 0.8
                                    text: Compress.outDir
                                }

                                RowLayout {
                                    spacing: root.formColSpacing

                                    Button {
                                        text: qsTr("&Ganti folder...")
                                        onClicked: Compress.pickOutDir()
                                    }

                                    Button {
                                        text: qsTr("&Buka Folder")
                                        onClicked: Compress.openOutDir()
                                        ToolTip.visible: hovered
                                        ToolTip.text: qsTr("Buka folder output di Explorer")
                                    }

                                    Button {
                                        text: qsTr("Buka &Hasil")
                                        enabled: Compress.hasResult
                                        onClicked: Compress.openResultDir()
                                        ToolTip.visible: hovered
                                        ToolTip.text: qsTr("Buka folder hasil kompres di Explorer")
                                    }
                                }
                            }

                            // --- status (CompressStatusWidget equivalent) ----
                            Pane {
                                Layout.fillWidth: true
                                Material.elevation: 1
                                padding: 12

                                ColumnLayout {
                                    anchors.fill: parent
                                    spacing: 8

                                    Label {
                                        text: qsTr("Kompres:")
                                        font.bold: true
                                    }

                                    Label {
                                        Layout.fillWidth: true
                                        wrapMode: Text.WordWrap
                                        text: Compress.statusText
                                    }

                                    ProgressBar {
                                        Layout.fillWidth: true
                                        from: 0
                                        to: 100
                                        // Indeterminate while a job is queued but
                                        // not yet reporting progress.
                                        indeterminate: Compress.running
                                                      && Compress.progress === 0
                                        value: Compress.progress
                                    }

                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: 16

                                        Label {
                                            text: Compress.etaText
                                            opacity: 0.7
                                        }

                                        Label {
                                            Layout.fillWidth: true
                                            elide: Text.ElideMiddle
                                            opacity: 0.7
                                            text: Compress.detailText
                                        }

                                        Label {
                                            text: Compress.queueText
                                            opacity: 0.7
                                        }
                                    }

                                    GridLayout {
                                        columns: 2
                                        columnSpacing: root.formColSpacing
                                        rowSpacing: root.formRowSpacing

                                        // Label flips to "Antrikan" while a job runs,
                                        // matching the Widgets queue semantics.
                                        Button {
                                            text: Compress.running || Compress.pending > 0
                                                  ? qsTr("Antrikan")
                                                  : qsTr("Kompres")
                                            enabled: Compress.sourcePath.length > 0
                                            onClicked: {
                                                // A manually started job is not the
                                                // offer's, so drop any pending
                                                // one-shot apply intent.
                                                root.applyAfterCompress = false
                                                Compress.start()
                                            }
                                            ToolTip.visible: hovered
                                            ToolTip.text: qsTr("Kompres video ke H.264 di latar belakang (UI tetap responsif)")
                                        }

                                        Button {
                                            text: qsTr("Terapkan &Hasil")
                                            enabled: Compress.hasResult
                                            onClicked: Studio.applyWallpaper(Compress.resultPath)
                                            ToolTip.visible: hovered
                                            ToolTip.text: qsTr("Terapkan hasil kompresi sebagai wallpaper")
                                        }

                                        Button {
                                            text: qsTr("&Batal")
                                            enabled: Compress.running
                                            onClicked: Compress.cancel()
                                            ToolTip.visible: hovered
                                            ToolTip.text: qsTr("Batalkan kompresi yang berjalan")
                                        }

                                        CheckBox {
                                            Layout.alignment: Qt.AlignVCenter
                                            text: qsTr("Langsung terapkan setelah selesai")
                                            checked: Compress.autoApply
                                            onToggled: Compress.setAutoApply(checked)
                                            ToolTip.visible: hovered
                                            ToolTip.text: qsTr("Otomatis terapkan hasil kompres sebagai wallpaper")
                                        }
                                    }
                                }
                            }

                            // Absorbs the column's spare height so it lands as
                            // plain background below the panel instead of being
                            // handed to every child (which spreads the rows
                            // apart) or trapped inside the panel's border.
                            Item {
                                Layout.fillHeight: true
                            }
                        }

                        // --- Mode lanjutan (teknis) -------------------------
                        Pane {
                            Layout.preferredWidth: 380
                            Layout.minimumWidth: 340
                            // Content height only. A RowLayout stretches a child
                            // to full height by default, and a stretched pane's
                            // inner ColumnLayout hands the slack to every control,
                            // spreading CRF / FPS / Resolusi / Encoder down the
                            // whole rail. fillHeight:false + AlignTop keeps the
                            // panel wrapped around its controls and lets the slack
                            // fall through as plain background.
                            Layout.fillHeight: false
                            Layout.alignment: Qt.AlignTop
                            Material.elevation: 1
                            padding: 12

                                ColumnLayout {
                                    anchors.fill: parent
                                    spacing: root.formRowSpacing

                                    CheckBox {
                                        id: advancedBox
                                        Layout.fillWidth: true
                                        text: qsTr("Mode lanjutan (teknis)")
                                        checked: Compress.advanced
                                        onToggled: Compress.setAdvanced(checked)
                                    }

                                    // One grid for all four knobs. As four
                                    // separate RowLayouts each label was only as
                                    // wide as its own text, so CRF / FPS /
                                    // Resolusi / Encoder each pushed its control
                                    // to a different x.
                                    GridLayout {
                                        Layout.fillWidth: true
                                        columns: 2
                                        columnSpacing: root.formColSpacing
                                        rowSpacing: root.formRowSpacing

                                        Label {
                                            text: qsTr("CRF:")
                                            Layout.preferredWidth: root.labelColWidth
                                        }

                                        SpinBox {
                                            id: crfBox
                                            Layout.fillWidth: true
                                            Layout.maximumWidth: root.narrowControlWidth
                                            from: 16
                                            to: 28
                                            editable: true
                                            value: Compress.crf
                                            onValueModified: Compress.setCrf(value)
                                            ToolTip.visible: hovered
                                            ToolTip.text: qsTr("Kualitas CRF 16-28 (bawaan 22)")
                                        }

                                        Label {
                                            text: qsTr("FPS:")
                                            Layout.preferredWidth: root.labelColWidth
                                        }

                                        SpinBox {
                                            id: fpsBox
                                            Layout.fillWidth: true
                                            Layout.maximumWidth: root.narrowControlWidth
                                            from: 1
                                            to: 30
                                            editable: true
                                            value: Compress.fps
                                            onValueModified: Compress.setFps(value)
                                            ToolTip.visible: hovered
                                            ToolTip.text: qsTr("Target bingkai per detik (bawaan 30, maks 30)")
                                        }

                                        Label {
                                            text: qsTr("Resolusi:")
                                            Layout.preferredWidth: root.labelColWidth
                                        }

                                        TextField {
                                            id: resolutionField
                                            Layout.fillWidth: true
                                            Layout.maximumWidth: root.narrowControlWidth
                                            placeholderText: qsTr("mis. 1920x1080 (kosong = ikut sumber)")
                                            text: Compress.resolutionText
                                            onEditingFinished: Compress.setResolutionText(text)
                                            ToolTip.visible: hovered
                                            ToolTip.text: qsTr("Resolusi output WxH")
                                        }

                                        Label {
                                            text: qsTr("Encoder:")
                                            Layout.preferredWidth: root.labelColWidth
                                        }

                                        ComboBox {
                                            id: encoderCombo
                                            Layout.fillWidth: true
                                            Layout.maximumWidth: root.narrowControlWidth
                                            model: ["auto", "nvenc", "qsv", "amf", "x264"]
                                            currentIndex: Math.max(0, model.indexOf(Compress.encoder))
                                            onActivated: Compress.setEncoder(model[index])
                                            ToolTip.visible: hovered
                                            ToolTip.text: qsTr("Encoder (bawaan auto)")
                                        }
                                    }

                                    CheckBox {
                                        Layout.fillWidth: true
                                        text: qsTr("Paksa (video >10 menit)")
                                        checked: Compress.forceLong
                                        onToggled: Compress.setForceLong(checked)
                                        ToolTip.visible: hovered
                                        ToolTip.text: qsTr("Izinkan video lebih dari 10 menit")
                                    }
                                }
                        }
                    }

                    Dialog {
                        id: consentDialog
                        title: qsTr("Video Panjang")
                        modal: true
                        anchors.centerIn: Overlay.overlay
                        standardButtons: Dialog.Yes | Dialog.No
                        onAccepted: Compress.resolveConsent(true)
                        onRejected: Compress.resolveConsent(false)

                        contentItem: Label {
                            text: qsTr("Video berdurasi %1 menit (lebih dari 10). "
                                       + "Kompres tetap?\n\nVideo panjang butuh waktu "
                                       + "lama dan memakai --force-long.")
                                  .arg(consentDuration.toFixed(1))
                            wrapMode: Text.WordWrap
                        }
                    }
                }

                // ---------------------------------------------------------
                // Pengaturan
                // ---------------------------------------------------------
                Item {
                    id: settingsPage

                    // Settings/logs are produced by three different backends, so
                    // all three are merged into the one log view.
                    property var allLog: Studio.log.concat(Compress.log, Settings.log)

                    // Two columns. The old single full-width column put "Folder
                    // output:" at the far left with its Ubah button ~1840px away,
                    // and every control row stretched across the whole window.
                    // Left carries the basic + compressor settings, right the
                    // cache / engine / update groups and the log.
                    ScrollView {
                        anchors.fill: parent
                        anchors.margins: 10
                        clip: true
                        contentWidth: availableWidth

                        RowLayout {
                            width: settingsPage.width - 20
                            spacing: 8

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: root.formRowSpacing

                                // --- Bahasa --------------------------------
                                // First, because it decides the language of
                                // everything below it. A QTranslator is only
                                // installed at startup, so a new choice takes
                                // effect on the next launch.
                                Label {
                                    text: qsTr("Bahasa")
                                    font.bold: true
                                }

                                GridLayout {
                                    Layout.fillWidth: true
                                    columns: 2
                                    columnSpacing: root.formColSpacing
                                    rowSpacing: root.formRowSpacing

                                    Label {
                                        text: qsTr("Bahasa antarmuka:")
                                        Layout.preferredWidth: root.labelColWidth
                                    }

                                    ComboBox {
                                        Layout.preferredWidth: root.narrowControlWidth
                                        model: [
                                            { text: qsTr("Indonesia"), code: "id" },
                                            { text: "English", code: "en" }
                                        ]
                                        textRole: "text"
                                        valueRole: "code"
                                        currentIndex: {
                                            for (let i = 0; i < model.length; ++i) {
                                                if (model[i].code === Settings.language)
                                                    return i
                                            }
                                            return 0
                                        }
                                        onActivated: Settings.setLanguage(model[index].code)
                                    }
                                }

                                Label {
                                    text: qsTr("Pengaturan dasar")
                                    font.bold: true
                                }

                                CheckBox {
                                    Layout.fillWidth: true
                                    text: qsTr("Jalankan saat Windows menyala")
                                    checked: Settings.startWithWindows
                                    onToggled: Settings.setStartWithWindows(checked)
                                }

                                CheckBox {
                                    Layout.fillWidth: true
                                    text: qsTr("Hemat baterai (wallpaper berhenti saat pakai baterai)")
                                    checked: Settings.batterySaver
                                    onToggled: Settings.setBatterySaver(checked)
                                }

                                // One grid for the whole basic group so
                                // "Isi layar:", "Layar:" and "Offset bingkai"
                                // share a label column and their controls all
                                // start at the same x.
                                GridLayout {
                                    Layout.fillWidth: true
                                    columns: 2
                                    columnSpacing: root.formColSpacing
                                    rowSpacing: root.formRowSpacing

                                    Label {
                                        text: qsTr("Isi layar:")
                                        Layout.preferredWidth: root.labelColWidth
                                    }

                                    ComboBox {
                                        id: sFitCombo
                                        Layout.fillWidth: true
                                        Layout.maximumWidth: root.controlWidth
                                        model: [qsTr("Isi layar (potong bila perlu)"),
                                                qsTr("Sesuaikan (seluruh video terlihat)"),
                                                qsTr("Regang (isi penuh)"),
                                                "Tengah (ukuran asli)"]
                                        // Index order must match the fit_mode enum
                                        // in shared/config_schema.cpp.
                                        readonly property var modes: ["cover", "fit",
                                                                   "stretch", "center"]
                                        currentIndex: {
                                            const i = modes.indexOf(Settings.fitMode)
                                            return i >= 0 ? i : 0
                                        }
                                        onActivated: Settings.setFitMode(modes[index])
                                    }

                                    Label {
                                        text: qsTr("Layar:")
                                        Layout.preferredWidth: root.labelColWidth
                                    }

                                    ComboBox {
                                        Layout.fillWidth: true
                                        Layout.maximumWidth: root.controlWidth
                                        model: Studio.monitorChoices
                                        textRole: "text"
                                        valueRole: "id"
                                        currentIndex: {
                                            for (let i = 0; i < Studio.monitorChoices.length; ++i) {
                                                if (Studio.monitorChoices[i].id === Settings.monitorId)
                                                    return i
                                            }
                                            return 0
                                        }
                                        onActivated: Settings.setMonitorId(
                                                         Studio.monitorChoices[index].id)
                                    }

                                    Label {
                                        text: qsTr("Offset bingkai (detik):")
                                        Layout.preferredWidth: root.labelColWidth
                                    }

                                    SpinBox {
                                        Layout.preferredWidth: root.spinWidth
                                        from: 0
                                        to: 30
                                        editable: true
                                        value: Math.round(Settings.lockscreenOffsetSec)
                                        onValueModified: Settings.setLockscreenOffsetSec(value)
                                    }
                                }

                                CheckBox {
                                    Layout.fillWidth: true
                                    text: qsTr("Samakan bingkai video ke layar kunci (gambar statis)")
                                    checked: Settings.lockscreenSync
                                    onToggled: Settings.setLockscreenSync(checked)
                                }

                                Label {
                                    text: qsTr("Kompresor (bawaan impor)")
                                    font.bold: true
                                }

                                CheckBox {
                                    Layout.fillWidth: true
                                    text: qsTr("Otomatis kompres:")
                                    checked: Settings.autoCompressOnImport
                                    onToggled: Settings.setAutoCompressOnImport(checked)
                                }

                                // Label | value | action, so "Ubah..." sits next
                                // to the path instead of at the far window edge.
                                GridLayout {
                                    Layout.fillWidth: true
                                    columns: 3
                                    columnSpacing: root.formColSpacing
                                    rowSpacing: root.formRowSpacing

                                    Label {
                                        text: qsTr("Folder output:")
                                        Layout.preferredWidth: root.labelColWidth
                                    }

                                    Label {
                                        Layout.fillWidth: true
                                        Layout.maximumWidth: 400
                                        elide: Text.ElideMiddle
                                        opacity: 0.8
                                        text: Settings.compressOutputDir
                                    }

                                    Button {
                                        text: qsTr("Ubah...")
                                        onClicked: Settings.pickCompressOutputDir()
                                    }
                                }

                                GridLayout {
                                    Layout.fillWidth: true
                                    columns: 2
                                    columnSpacing: root.formColSpacing
                                    rowSpacing: root.formRowSpacing

                                    Label {
                                        text: qsTr("CRF bawaan:")
                                        Layout.preferredWidth: root.labelColWidth
                                    }

                                    SpinBox {
                                        Layout.preferredWidth: root.spinWidth
                                        from: 16
                                        to: 28
                                        editable: true
                                        value: Settings.defaultCrf
                                        onValueModified: Settings.setDefaultCrf(value)
                                    }

                                    Label {
                                        text: qsTr("FPS bawaan:")
                                        Layout.preferredWidth: root.labelColWidth
                                    }

                                    SpinBox {
                                        Layout.preferredWidth: root.spinWidth
                                        from: 1
                                        to: 30
                                        editable: true
                                        value: Settings.defaultFps
                                        onValueModified: Settings.setDefaultFps(value)
                                    }

                                    Label {
                                        text: qsTr("Resolusi bawaan:")
                                        Layout.preferredWidth: root.labelColWidth
                                    }

                                    ComboBox {
                                        Layout.fillWidth: true
                                        Layout.maximumWidth: root.narrowControlWidth
                                        model: [qsTr("Ikuti layar"), qsTr("Ikuti sumber"), qsTr("720p"), qsTr("1080p"), qsTr("2160p")]
                                        readonly property var modes: ["match_monitor", "source",
                                                                   "720p", "1080p", "2160p"]
                                        currentIndex: {
                                            const i = modes.indexOf(Settings.defaultResolutionMode)
                                            return i >= 0 ? i : 0
                                        }
                                        onActivated: Settings.setDefaultResolutionMode(modes[index])
                                    }
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: root.formRowSpacing

                                Label {
                                    text: qsTr("Cache")
                                    font.bold: true
                                }

                                // Label | value | actions in one grid, so
                                // "Ubah..." and "Bersihkan cache..." sit next to
                                // the path rather than at the far window edge.
                                GridLayout {
                                    Layout.fillWidth: true
                                    columns: 3
                                    columnSpacing: root.formColSpacing
                                    rowSpacing: root.formRowSpacing

                                    Label {
                                        text: qsTr("Cache:")
                                        Layout.preferredWidth: root.labelColWidth
                                    }

                                    Label {
                                        Layout.fillWidth: true
                                        Layout.maximumWidth: 320
                                        elide: Text.ElideMiddle
                                        opacity: 0.8
                                        text: Settings.cacheDir
                                    }

                                    RowLayout {
                                        spacing: root.formColSpacing

                                        Button {
                                            text: qsTr("Ubah...")
                                            onClicked: Settings.pickCacheDir()
                                        }

                                        Button {
                                            text: qsTr("Bersihkan cache...")
                                            onClicked: cacheDialog.open()
                                        }
                                    }
                                }

                                Label {
                                    text: qsTr("Engine")
                                    font.bold: true
                                }

                                // One grid for the three engine knobs so their
                                // labels share a column and the controls line up.
                                GridLayout {
                                    Layout.fillWidth: true
                                    columns: 2
                                    columnSpacing: root.formColSpacing
                                    rowSpacing: root.formRowSpacing

                                    Label {
                                        text: qsTr("Mode hemat baterai:")
                                        Layout.preferredWidth: root.labelColWidth
                                    }

                                    ComboBox {
                                        Layout.fillWidth: true
                                        Layout.maximumWidth: root.controlWidth
                                        model: [qsTr("Batasi 24 fps (hemat)"), qsTr("Jeda penuh saat baterai (paling hemat)")]
                                        readonly property var modes: ["cap24", "static"]
                                        currentIndex: Math.max(0, modes.indexOf(Settings.batteryMode))
                                        onActivated: Settings.setBatteryMode(modes[index])
                                    }

                                    Label {
                                        text: qsTr("Inti CPU:")
                                        Layout.preferredWidth: root.labelColWidth
                                    }

                                    ComboBox {
                                        Layout.fillWidth: true
                                        Layout.maximumWidth: root.controlWidth
                                        model: [qsTr("Otomatis (inti efisiensi/E-core)"), qsTr("Semua inti")]
                                        readonly property var modes: ["auto", "all"]
                                        currentIndex: Math.max(0, modes.indexOf(Settings.cpuAffinity))
                                        onActivated: Settings.setCpuAffinity(modes[index])
                                    }

                                    Label {
                                        text: qsTr("GPU:")
                                        Layout.preferredWidth: root.labelColWidth
                                    }

                                    ComboBox {
                                        Layout.fillWidth: true
                                        Layout.maximumWidth: root.controlWidth
                                        model: [qsTr("Otomatis"), qsTr("Terintegrasi (hemat daya)"), qsTr("Diskrit (performa)")]
                                        readonly property var modes: ["auto", "integrated", "discrete"]
                                        currentIndex: Math.max(0, modes.indexOf(Settings.gpuAdapter))
                                        onActivated: Settings.setGpuAdapter(modes[index])
                                    }
                                }

                                Label {
                                    Layout.fillWidth: true
                                    visible: Settings.engineRestartNeeded
                                    wrapMode: Text.WordWrap
                                    opacity: 0.8
                                    text: qsTr("Perubahan Inti CPU / GPU butuh restart engine.")
                                }

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: root.formColSpacing

                                    Button {
                                        text: qsTr("&Terapkan")
                                        onClicked: Settings.apply()
                                        ToolTip.visible: hovered
                                        ToolTip.text: qsTr("Terapkan perubahan engine yang butuh restart")
                                    }

                                    Button {
                                        text: qsTr("Restart engine")
                                        onClicked: Studio.startEngine()
                                        ToolTip.visible: hovered
                                        ToolTip.text: qsTr("Mulai ulang engine supaya perubahan inti CPU / GPU langsung berlaku")
                                    }

                                    Item {
                                        Layout.fillWidth: true
                                    }
                                }

                                Label {
                                    text: qsTr("Pembaruan")
                                    font.bold: true
                                }

                                CheckBox {
                                    Layout.fillWidth: true
                                    text: qsTr("Periksa pembaruan saat Studio dimulai")
                                    checked: Settings.checkUpdates
                                    onToggled: Settings.setCheckUpdates(checked)
                                }

                                Label {
                                    Layout.fillWidth: true
                                    visible: Settings.lastError.length > 0
                                    wrapMode: Text.WordWrap
                                    color: "red"
                                    text: Settings.lastError
                                }

                                Label {
                                    text: qsTr("Detail teknis (log)")
                                    font.bold: true
                                }

                                ScrollView {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    Layout.minimumHeight: 140
                                    clip: true

                                    TextArea {
                                        readOnly: true
                                        wrapMode: TextArea.NoWrap
                                        selectByMouse: true
                                        text: settingsPage.allLog.join("\n")
                                        placeholderText: qsTr("Log terapkan muncul di sini...")
                                    }
                                }
                            }
                        }
                    }

                    Dialog {
                        id: cacheDialog
                        title: qsTr("Bersihkan cache")
                        modal: true
                        anchors.centerIn: Overlay.overlay
                        standardButtons: Dialog.Yes | Dialog.Cancel
                        onAccepted: Settings.clearCache()

                        contentItem: Label {
                            text: qsTr("Hapus semua file sementara di:\n%1")
                                  .arg(Settings.cacheDir)
                            wrapMode: Text.WordWrap
                        }
                    }
                }
            }
        }
    }

    Dialog {
        id: compressOfferDialog
        title: qsTr("Video Besar")
        modal: true
        anchors.centerIn: Overlay.overlay
        standardButtons: Dialog.Yes | Dialog.No
        onAccepted: root.startCompressFirst()
        onRejected: root.declineCompressFirst()

        // The mpv preview is a separate HWND that DWM composites above the
        // QQuickWidget, so this dialog would be hidden behind it. A 0x0 rect
        // drops the native surface; startCompressFirst / the restored preview
        // bring it back.
        onOpened: root.dialogOpened()
        onClosed: root.dialogClosed()

        contentItem: Label {
            // One translatable sentence with %1/%2 placeholders. Splitting it
            // across two qsTr() calls (as the wrap pass briefly did) would give
            // a translator two unrelated fragments instead of one sentence.
            text: qsTr("Video ini berukuran %1 MB. Kompres ke 1080p dulu agar "
                       + "hemat ~3x RAM saat dipakai sebagai wallpaper?\n\n%2")
                  .arg(root.offerMb)
                  .arg(root.offerPath)
            wrapMode: Text.WordWrap
        }
    }

    Dialog {
        id: aboutDialog
        title: qsTr("About K6WP")
        modal: true
        anchors.centerIn: parent
        standardButtons: Dialog.Close

        // The mpv preview is a separate HWND that DWM composites above the
        // QQuickWidget, so a QML dialog cannot draw over it. A 0x0 rect tells
        // QmlShell to drop the native surface; syncPreview() puts it back.
        onOpened: root.dialogOpened()
        onClosed: root.dialogClosed()

        contentItem: ColumnLayout {
            spacing: 8

            Label {
                text: "K6WP Studio " + Studio.version
                font.bold: true
            }

            Label {
                text: qsTr("License: GPL-2.0-or-later")
            }

            // Each wrapping label carries an explicit preferred width: a Label
            // with Text.WordWrap does not report a usable implicitWidth, so
            // without this the dialog collapses to nothing.
            Label {
                Layout.preferredWidth: 440
                wrapMode: Text.WordWrap
                text: qsTr("Third-party licenses: mpv (libmpv), ffmpeg, Qt, nlohmann/json — see LICENSES/ for the full texts.")
            }

            Label {
                Layout.preferredWidth: 440
                wrapMode: Text.WordWrap
                visible: Studio.updateAvailable
                text: qsTr("New version %1 is available.").arg(Studio.latestVersion)
            }

            Label {
                Layout.preferredWidth: 440
                wrapMode: Text.WordWrap
                text: qsTr("Update check contacts the releases host once per app start (never downloads/installs); disable in Settings → Pembaruan.")
            }

            RowLayout {
                spacing: 8

                Button {
                    text: qsTr("Support development")
                    onClicked: Qt.openUrlExternally(Studio.donateUrl)
                }

                Button {
                    text: qsTr("Project page")
                    onClicked: Qt.openUrlExternally(Studio.projectUrl)
                }

                Button {
                    text: qsTr("Open release page")
                    visible: Studio.updateAvailable
                    onClicked: Qt.openUrlExternally(Studio.latestPageUrl)
                }

                Item {
                    Layout.fillWidth: true
                }
            }
        }
    }

    // --- First-run wizard ---------------------------------------------------
    // QWizardPage cannot run inside a QQuickWidget, so the four wizard pages are
    // a StackLayout here. The gate is the same pure IsFirstRunCondition the
    // Widgets wizard used: no settings file, empty library, and (supplied by
    // Studio) no active video.
    Dialog {
        id: firstRunDialog
        title: qsTr("Selamat datang di K6WP")
        modal: true
        anchors.centerIn: parent
        closePolicy: Popup.NoAutoClose

        // The mpv preview HWND composites above the QQuickWidget, so it has to
        // step aside for this dialog (same as the About box).
        onOpened: root.dialogOpened()
        onClosed: root.dialogClosed()

        onAccepted: {
            // The pick was already imported by the picker (Library.
            // pickAndImport), so importing again here only re-probed the file
            // and re-raised the compress-first offer.
            Settings.setStartWithWindows(firstRunAutostart)
            Settings.setAutoCompressOnImport(firstRunAutoCompress)
            Settings.apply()
            if (firstRunFile.length > 0)
                Studio.applyWallpaper(firstRunFile)
        }

        // "Nanti saja" wrote nothing, so the wizard came back on every start
        // and Finish could then re-import the file the user had declined.
        onRejected: Library.markFirstRunHandled()

        contentItem: Item {
            implicitWidth: 520
            implicitHeight: 340

            StackLayout {
                id: firstRunPages
                anchors.fill: parent
                currentIndex: firstRunDialog.page

                // 1 - welcome
                Item {
                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 8

                        Label {
                            text: qsTr("Selamat datang di K6WP")
                            font.bold: true
                            font.pixelSize: 20
                        }

                        Label {
                            text: qsTr("Wallpaper video untuk Windows Anda.")
                            font.pixelSize: 14
                        }

                        Label {
                            Layout.preferredWidth: 480
                            Layout.fillHeight: true
                            wrapMode: Text.WordWrap
                            text: qsTr("K6WP menampilkan video sebagai wallpaper desktop Anda. Panduan singkat ini akan memilih video pertama, menyiapkan pengaturan awal, lalu menampilkan hasilnya di layar. Klik Lanjut untuk mulai — atau Nanti saja untuk melewati dan mengatur sendiri dari tab Wallpaper.")
                        }
                    }
                }

                // 2 - pick
                Item {
                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 8

                        Label {
                            text: qsTr("Pilih video pertama Anda")
                            font.bold: true
                            font.pixelSize: 20
                        }

                        Label {
                            Layout.preferredWidth: 480
                            wrapMode: Text.WordWrap
                            text: qsTr("Pilih satu video dari komputer Anda. File asli tidak dipindah atau diubah.")
                        }

                        Label {
                            Layout.preferredWidth: 480
                            wrapMode: Text.WordWrap
                            opacity: 0.8
                            text: qsTr("Format yang didukung: mp4, webm, avi, mkv, mov, wmv.")
                        }

                        RowLayout {
                            spacing: 8

                            Button {
                                text: qsTr("Pilih video...")
                                onClicked: Library.pickAndImport()
                                ToolTip.visible: hovered
                                ToolTip.text: qsTr("Pilih video pertama Anda")
                            }

                            Item {
                                Layout.fillWidth: true
                            }
                        }

                        Label {
                            Layout.preferredWidth: 480
                            elide: Text.ElideMiddle
                            text: firstRunFile.length > 0
                                  ? firstRunFile
                                  : qsTr("Belum ada video dipilih.")
                        }

                        Item {
                            Layout.fillHeight: true
                        }
                    }
                }

                // 3 - options
                Item {
                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 8

                        Label {
                            text: qsTr("Pengaturan awal")
                            font.bold: true
                            font.pixelSize: 20
                        }

                        Label {
                            Layout.preferredWidth: 480
                            wrapMode: Text.WordWrap
                            text: qsTr("Anda bisa mengubah semuanya nanti di tab Pengaturan.")
                        }

                        CheckBox {
                            id: firstRunAutostartBox
                            text: qsTr("Jalankan saat Windows menyala")
                            checked: false

                            HoverHandler {
                                id: frAutoHover
                            }
                            ToolTip.visible: frAutoHover.hovered
                            ToolTip.text: qsTr("K6WP menyala sendiri setiap masuk Windows")
                        }

                        CheckBox {
                            id: firstRunCompressBox
                            text: qsTr("Siapkan video otomatis (disarankan menyala)")
                            checked: true

                            HoverHandler {
                                id: frCompHover
                            }
                            ToolTip.visible: frCompHover.hovered
                            ToolTip.text: qsTr("Video disiapkan agar ringan dipakai sebagai wallpaper")
                        }

                        Item {
                            Layout.fillHeight: true
                        }
                    }
                }

                // 4 - tip
                Item {
                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 8

                        Label {
                            text: qsTr("Saran")
                            font.bold: true
                            font.pixelSize: 20
                        }

                        Label {
                            text: qsTr("Satu hal sebelum mulai.")
                            font.pixelSize: 14
                        }

                        Label {
                            Layout.preferredWidth: 480
                            Layout.fillHeight: true
                            wrapMode: Text.WordWrap
                            text: qsTr("Biarkan pilihan otomatis menyala. Video Anda akan disiapkan agar ringan dipakai sebagai wallpaper, lalu ditampilkan di layar. Klik Selesai untuk mulai — Anda bisa mengganti video kapan saja dari tab Wallpaper.")
                        }
                    }
                }
            }

            // Custom footer: the wizard's own Lanjut / Kembali / Selesai /
            // Nanti saja wording, driven off `page`.
            RowLayout {
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                spacing: 8

                Button {
                    text: qsTr("Nanti saja")
                    onClicked: firstRunDialog.reject()
                }

                Item {
                    Layout.fillWidth: true
                }

                Button {
                    text: qsTr("Kembali")
                    visible: firstRunDialog.page > 0
                    enabled: firstRunDialog.page > 0
                    onClicked: firstRunDialog.page -= 1
                }

                Button {
                    text: qsTr("Lanjut")
                    visible: firstRunDialog.page < 3
                    // The pick page must not advance without a video, which is
                    // the QWizardPage::isComplete gate the Widgets page used.
                    enabled: firstRunDialog.page !== 1 || firstRunFile.length > 0
                    onClicked: firstRunDialog.page += 1
                }

                Button {
                    text: qsTr("Selesai")
                    visible: firstRunDialog.page === 3
                    onClicked: firstRunDialog.accept()
                }
            }
        }

        // Picked path for the current run. The model reports the last pick even
        // when it was NOT imported (over the compress-first threshold), which
        // Library.dstAt(0) could not: that row does not exist, so an
        // over-threshold first pick left this empty and stranded the wizard on
        // page 1 with "Lanjut" permanently disabled.
        property string firstRunFile: Library.lastPickedPath
        property bool firstRunAutostart: firstRunAutostartBox.checked
        property bool firstRunAutoCompress: firstRunCompressBox.checked
        property int page: 0
    }

    Component.onCompleted: if (Library.firstRunEligible && !Studio.videoActive)
                               firstRunDialog.open()
}
