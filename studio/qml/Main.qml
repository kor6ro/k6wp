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
// Language: Indonesian is the source language; user-visible strings are wrapped
// in qsTr() and translated via studio/i18n/studio_en.ts.
//
// Spacing is Material 8dp: 8 / 16 / 24.
//
// Component split: all UI panels/dialogs live in sibling .qml files under
// studio/qml/. Shared state (sizing, StatusKind, dialog lifecycle, compress
// offer, monitor selection, armed assignment) stays on this root and is
// passed into components as properties.
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
        if (!wallpaperPage.previewHoleVisible) {
            Studio.syncPreviewGeometry(0, 0, 0, 0);
            return;
        }
        Studio.syncPreviewGeometry(wallpaperPage.previewHoleX,
                                   wallpaperPage.previewHoleY,
                                   wallpaperPage.previewHoleWidth,
                                   wallpaperPage.previewHoleHeight);
    }

    // The library model raises intents rather than acting on them: applying a
    // wallpaper is a StudioBridge call and re-compressing is a CompressBridge
    // call, so the wiring lives here instead of coupling the model to both.
    Connections {
        target: Library
        // Todo 12: the install goes through the home view's "Pasang ke" flow
        // (straight to the global video on one screen; target selection /
        // C-14 on several), never directly to applyWallpaper.
        function onApplyRequested(dst) {
            wallpaperPage.installVideo(dst)
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

    // A card's "Perkecil" action (C-5) sets Compress.setSourcePath itself,
    // then navigates here so the Kompresor tab shows the job.
    function showCompressor() {
        root.showTab(1)
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

    // Basename for the per-monitor assignment line under a sub-tab (Windows
    // or POSIX separators).
    function fileNameOf(path) {
        const p = String(path === undefined || path === null ? "" : path)
        const slash = Math.max(p.lastIndexOf("/"), p.lastIndexOf("\\"))
        return slash >= 0 ? p.slice(slash + 1) : p
    }

    // --- row 41: monitor sub-tabs (selected monitor state) -----------------
    // Exactly one selected key at a time. The primary monitor's key is the
    // default; with no primary, the first entry with a non-empty key wins
    // (deterministic Studio.displays order). Empty until the first
    // refreshDisplays() lands - the Wallpaper view is the start page, so that
    // refresh also runs from Component.onCompleted below.
    property string selectedMonitorKey: ""

    function defaultMonitorKey() {
        const all = Studio.displays
        for (let i = 0; i < all.length; ++i) {
            const entry = all[i]
            if (String(entry.key).length > 0 && entry.isPrimary === true)
                return String(entry.key)
        }
        for (let i = 0; i < all.length; ++i) {
            const entry = all[i]
            if (String(entry.key).length > 0)
                return String(entry.key)
        }
        return ""
    }

    // Keeps the selection valid across refreshes: a monitor that is still
    // modelled stays selected; a vanished one falls back to the default rule
    // instead of pointing at nothing.
    function ensureSelectedMonitor() {
        const all = Studio.displays
        for (let i = 0; i < all.length; ++i) {
            if (String(all[i].key) === root.selectedMonitorKey
                    && String(all[i].key).length > 0)
                return
        }
        root.selectedMonitorKey = root.defaultMonitorKey()
    }

    function selectedMonitorEntry() {
        const all = Studio.displays
        for (let i = 0; i < all.length; ++i) {
            if (String(all[i].key) === root.selectedMonitorKey)
                return all[i]
        }
        return null
    }

    function selectedMonitorLabel() {
        const entry = root.selectedMonitorEntry()
        return entry === null ? "" : String(entry.label)
    }

    // Enable rule for the monitor-scoped Hapus (the clear itself is the
    // row-19 clearMonitorAssignment() invokable).
    function selectedMonitorHasAssignment() {
        const entry = root.selectedMonitorEntry()
        return entry !== null && String(entry.assignedPath).length > 0
    }

    // A sub-tab click scopes the Wallpaper view to that monitor. The preview
    // hole keeps its native-surface contract, so its geometry is re-pushed
    // through the existing syncPreview() exactly like the layout-change
    // handlers do; the quick settings are re-read through the existing
    // refreshQuickSettings() pattern so the panel cannot be stale on the new
    // scope. Installing is NOT part of this gesture anymore (todo 12: the
    // "Pasang ke" row/popup owns the target), so a click only scopes.
    function selectMonitor(key) {
        const k = String(key === undefined || key === null ? "" : key)
        if (k.length === 0)
            return
        root.selectedMonitorKey = k
        root.syncPreview()
        Studio.refreshQuickSettings()
    }

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
    }

    Connections {
        target: Studio
        // Row 41: the monitor list was rebuilt (refreshDisplays) - keep the
        // sub-tab selection pointing at a monitor that still exists.
        function onDisplaysChanged() {
            root.ensureSelectedMonitor()
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

                    // C-16 entry point: the friendly status stays on the
                    // Wallpaper screen, the raw details live in this dialog.
                    MenuItem {
                        text: qsTr("&Info teknis")
                        onTriggered: appDialogs.openInfoTeknis()
                    }

                    MenuItem {
                        text: qsTr("&Tentang K6WP Studio")
                        onTriggered: aboutDialog.open()
                    }
                }
            }

            Label {
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
                onCurrentIndexChanged: {
                    if (currentIndex === 0) {
                        Studio.refreshQuickSettings()
                        // Row 41: re-read the monitor list whenever the
                        // Wallpaper view is shown (row 21's refresh-on-show
                        // pattern, now owned by this view).
                        Studio.refreshDisplays()
                        // Re-scan the library on re-entry so a video renamed
                        // or moved in Explorer flips its card to the
                        // "File tidak ketemu" state (ListItems recomputes
                        // broken on every load).
                        Library.reload()
                    }
                }

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
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: tabBar.currentIndex

                // ---------------------------------------------------------
                // Wallpaper
                // ---------------------------------------------------------
                WallpaperPage {
                    id: wallpaperPage
                    labelColWidth: root.labelColWidth
                    controlWidth: root.controlWidth
                    spinWidth: root.spinWidth
                    formColSpacing: root.formColSpacing
                    formRowSpacing: root.formRowSpacing
                    selectedMonitorKey: root.selectedMonitorKey
                    selectMonitor: root.selectMonitor
                    fileNameOf: root.fileNameOf
                    selectedMonitorLabel: root.selectedMonitorLabel
                    selectedMonitorHasAssignment: root.selectedMonitorHasAssignment
                    maybeOfferCompressFirst: root.maybeOfferCompressFirst
                    syncPreview: root.syncPreview
                    navigateToCompressor: root.showCompressor
                    statusKindNotRunning: Main.StatusKind.NotRunning
                    statusKindPaused: Main.StatusKind.Paused
                    dialogOpened: root.dialogOpened
                    dialogClosed: root.dialogClosed
                }

                // ---------------------------------------------------------
                // Kompresor
                // ---------------------------------------------------------
                CompressorPage {
                    id: compressorPage
                    labelColWidth: root.labelColWidth
                    formColSpacing: root.formColSpacing
                    formRowSpacing: root.formRowSpacing
                    applyAfterCompress: root.applyAfterCompress
                    consentDuration: root.consentDuration
                    dialogOpened: root.dialogOpened
                    dialogClosed: root.dialogClosed
                }

                // ---------------------------------------------------------
                // Pengaturan
                // ---------------------------------------------------------
                SettingsPage {
                    id: settingsPage
                    labelColWidth: root.labelColWidth
                    controlWidth: root.controlWidth
                    narrowControlWidth: root.narrowControlWidth
                    spinWidth: root.spinWidth
                    formColSpacing: root.formColSpacing
                    formRowSpacing: root.formRowSpacing
                    dialogOpened: root.dialogOpened
                    dialogClosed: root.dialogClosed
                }
            }
        }
    }

    CompressOfferDialog {
        id: compressOfferDialog
        offerPath: root.offerPath
        offerMb: root.offerMb
        startCompressFirst: root.startCompressFirst
        declineCompressFirst: root.declineCompressFirst
        dialogOpened: root.dialogOpened
        dialogClosed: root.dialogClosed
    }

    AboutDialog {
        id: aboutDialog
        dialogOpened: root.dialogOpened
        dialogClosed: root.dialogClosed
    }

    // --- First-run wizard ---------------------------------------------------
    // QWizardPage cannot run inside a QQuickWidget, so the four wizard pages are
    // a StackLayout here. The gate is the same pure IsFirstRunCondition the
    // Widgets wizard used: no settings file, empty library, and (supplied by
    // Studio) no active video.
    FirstRunDialog {
        id: firstRunDialog
        dialogOpened: root.dialogOpened
        dialogClosed: root.dialogClosed
    }

    // --- App-level dialogs ---------------------------------------------------
    // Todo 5: "Info teknis" (friendly status + raw support details + Salin
    // untuk dukungan). Hosted at the root so todos 12/13 can extend the file
    // with the compress consent / first-offer dialogs.
    AppDialogs {
        id: appDialogs
        anchors.fill: parent
        dialogOpened: root.dialogOpened
        dialogClosed: root.dialogClosed
    }

    Component.onCompleted: {
        // Row 41: the Wallpaper view is the start page, so its monitor list
        // has to be populated here as well - onCurrentIndexChanged cannot
        // fire for the initial show. refreshDisplays() emits displaysChanged,
        // which seeds selectedMonitorKey through ensureSelectedMonitor().
        Studio.refreshDisplays()
        if (Library.firstRunEligible && !Studio.videoActive)
            firstRunDialog.open()
    }
}
