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
//
// Todo 13 (S6): compression runs fully in the background. The compressor tab
// is gone; navigation is a left sidebar (Beranda / Pengaturan) per brief
// B-WIREFRAME(a). Consent (C-9) and the compress-first offer (C-8) live in
// AppDialogs.qml; progress surfaces in ToastBar at the window bottom.
// applyAfterCompress, offerPath/offerMb/offerApplyAfter and the
// maybeOfferCompressFirst / startCompressFirst / declineCompressFirst trio
// stay on this root (brief Â§4.2: these paths MUST survive).
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
    // across the pages. Without them each row picked its own widths, which
    // is what made controls start at different x.
    readonly property int labelColWidth: 132
    readonly property int controlWidth: 320
    readonly property int narrowControlWidth: 220
    readonly property int spinWidth: 140
    readonly property int formColSpacing: 12
    readonly property int formRowSpacing: 10

    // --- sidebar navigation (todo 13; replaces TabBar + page-index nav) ---
    // Brief B-WIREFRAME(a): "sidebar kiri (Beranda/Koleksi/Pengaturan)".
    // Koleksi is part of the Beranda page today (CollectionPage lives inside
    // WallpaperPage), so the sidebar carries two entries. The compressor tab
    // is gone - compression is a background job surfaced by ToastBar.
    readonly property int pageBeranda: 0
    readonly property int pagePengaturan: 1
    property int currentPage: pageBeranda

    // switchPage replaces the old tab-index setter: same refresh-on-entry
    // side effects, driven by the sidebar buttons instead of a TabBar.
    function switchPage(index) {
        const i = Number(index)
        if (isNaN(i) || i === root.currentPage)
            return
        root.currentPage = i
        if (i === root.pageBeranda) {
            // Re-read on entry: the Beranda page's quick settings mirror
            // config.json + the autostart registry, either of which the
            // Pengaturan page can change. Also re-scan the library so a
            // video renamed or moved in Explorer flips its card to the
            // "File tidak ketemu" state.
            Studio.refreshQuickSettings()
            Studio.refreshDisplays()
            Library.reload()
        }
    }

    // --- first-run gate (plan todo 16; DEF-1 fix) --------------------------
    // Old pattern (base Main.qml, located by symbol): the gate on
    // `Library.firstRunEligible && !Studio.videoActive` in Component.onCompleted
    // plus the wizard's `firstRunFile: Library.lastPickedPath`. firstRunEligible
    // covers only the settings-file and library arguments (the C++ comment says
    // QML must AND it with !Studio.videoActive). The lastPickedPath term keeps
    // the onboarding up after the pick imports the video - the import flips
    // firstRunEligible false through countChanged - and re-evaluates the gate
    // when Selesai's markFirstRunHandled() clears it (lastPickedPathChanged).
    //
    // DEF-1: importPaths() writes lastPickedPath on EVERY import, so the bare
    // OR re-opened the overlay after any gallery import, long after Selesai.
    // firstRunFlowActive is a session latch, armed exactly once at start-up and
    // only when this session really begins in the first-run state; it stays
    // armed through the pick and is disarmed by Selesai (finishFirstRunFlow).
    // A later import therefore cannot re-open the overlay in this session, and
    // on restart the durable marker (studio_settings.json) makes
    // firstRunEligible false, so the latch arms false again. Single source of
    // truth: OnboardingView.visible and EmptyState.visible both read this.
    property bool firstRunFlowActive: false

    readonly property bool firstRunActive:
        root.firstRunFlowActive
        && ((Library.firstRunEligible && !Studio.videoActive)
            || Library.lastPickedPath.length > 0)

    // Selesai consumes the latch (called by OnboardingView.finish after the
    // durable marker is written). Never re-arms in the session.
    function finishFirstRunFlow() {
        root.firstRunFlowActive = false
    }

    // Pushes the preview hole's rectangle to the native PreviewWidget, in
    // QQuickWidget scene coordinates (the space QmlShell maps into window
    // coordinates). mapToItem(null, ...) resolves against the scene root, so
    // the hole's position inside its page is accounted for.
    //
    // A collapsed / hidden hole is pushed as a 0x0 rect, which QmlShell treats
    // as "hide the native surface" - that is what keeps the preview from
    // floating over the Pengaturan page.
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
        // Todo 13: re-compress runs in the background - set the source and
        // start; ToastBar carries the progress + Batal, and long videos raise
        // the C-9 consent dialog through AppDialogs.
        function onRecompressRequested(dst) {
            Compress.setSourcePath(dst)
            Compress.start()
        }
        // An imported file was over the threshold, so the model deliberately
        // did not reference it. With auto-compress on, just run it; otherwise
        // ask (C-8 offer in AppDialogs). The apply path is a separate signal
        // and always offers.
        function onCompressFirstRequired(path) {
            root.offerPath = path
            root.offerMb = Studio.compressFirstOfferMb(path)
            root.offerApplyAfter = false
            if (Settings.autoCompressOnImport)
                root.startCompressFirst()
            else
                appDialogs.openCompressOffer(path, root.offerMb)
        }
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
        appDialogs.openCompressOffer(path, mb)
        return true
    }

    // The prompt promises 1080p, so the job is pinned to 1920x1080 instead of
    // inheriting whatever the resolution box happens to hold.
    //
    // applyAfterCompress - not Compress.autoApply - is how the apply path gets
    // its result applied. autoApply is the user's own "Langsung terapkan
    // setelah selesai" preference, and the offer used to write it, so accepting
    // an offer silently ticked (or unticked) a preference the user never
    // touched. This flag belongs to the offer and is one-shot.
    //
    // Todo 13: this flag STAYS on the Main root (brief Â§4.2).
    property bool applyAfterCompress: false
    // Modals stack (e.g. first-run onboarding + compress offer): keep the
    // native preview hidden until the last one closes.
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
    // refreshDisplays() lands - the Beranda view is the start page, so that
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

    // A sub-tab click scopes the Beranda view to that monitor. The preview
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

    // Todo 13: start the compress-first job. No page navigation - the job
    // runs in the background and ToastBar carries the progress. The 1080p
    // pin matches the C-8 offer's promise.
    function startCompressFirst() {
        Compress.setSourcePath(offerPath)
        Compress.setResolutionText("1920x1080")
        root.applyAfterCompress = offerApplyAfter
        Compress.start()
    }

    // Answering "Pasang saja" (or Esc) means "do not compress first" - the
    // action that raised the offer still has to run, otherwise any video over
    // the threshold can never be applied: the caller already skipped its own
    // apply when maybeOfferCompressFirst returned true. The import path sets
    // offerApplyAfter = false, so declining is correctly a no-op there (that
    // file was deliberately left out of the library).
    function declineCompressFirst() {
        if (offerApplyAfter)
            Studio.applyWallpaper(offerPath)
        offerApplyAfter = false
    }

    // --- Compress bridge wiring (todo 13; relocated from the old compress
    // page) ---------------------------------------------------------------
    // consentRequired -> AppDialogs C-9 dialog -> resolveConsent. The pairing
    // (Metis F2) lives in AppDialogs.qml; this handler only opens it.
    //
    // resultChanged -> the one-shot applyAfterCompress flag (offer intent)
    // OR the user's autoApply preference decides whether the result is
    // installed. The flag is spent here.
    //
    // jobFinishedWithoutResult -> the flag is dropped so it cannot leak into
    // an unrelated later compress, and the C-10 outcome sentence is surfaced
    // in ToastBar (cancel vs failure, distinguished by toastBar.cancelPending).
    Connections {
        target: Compress
        function onConsentRequired(durationMinutes) {
            appDialogs.openConsent(durationMinutes)
        }
        function onResultChanged() {
            if (!Compress.hasResult)
                return
            // The offer's intent wins over the checkbox, and the flag is
            // spent here. A job that never yields a result is covered by
            // onJobFinishedWithoutResult, which clears the flag.
            const apply_it = root.applyAfterCompress || Compress.autoApply
            root.applyAfterCompress = false
            if (apply_it)
                Studio.applyWallpaper(Compress.resultPath)
        }
        function onJobFinishedWithoutResult() {
            root.applyAfterCompress = false
            // C-10 copy (brief B-COPY): cancel and failure both reassure the
            // user that the original file is untouched.
            if (toastBar.cancelPending) {
                toastBar.showOutcome(qsTr("Dibatalkan. Video aslinya tetap ada di koleksi."))
                toastBar.cancelPending = false
            } else if (Compress.lastError.length > 0) {
                toastBar.showOutcome(qsTr("Belum bisa disiapkan (%1). Kamu tetap bisa memasang video aslinya.")
                                      .arg(Compress.lastError))
            }
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

            // B-FLOW: the top menu bar (Berkas/Bantuan) is deleted; every
            // action it carried keeps a reachable opener:
            //   Impor Video -> Koleksi "+ Tambah video" / C-1 empty state
            //   Keluar      -> window close (C-19 tray "Keluar" when enabled)
            //   Info teknis -> Pengaturan > Diagnostik "Info teknis"
            //   Tentang     -> Pengaturan > Umum "Tentang K6WP Studio"
            //                  (Main passes appDialogs.openAbout down)

            Label {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 6
                wrapMode: Text.WordWrap
                visible: Studio.updateCheckMessage.length > 0
                text: Studio.updateCheckMessage
            }

            // --- sidebar + content (todo 13; replaces TabBar + StackLayout
            // currentIndex binding) ----------------------------------------
            // Sidebar left, content right (brief B-WIREFRAME(a)). The toast
            // sits below this row at the window bottom edge, never above the
            // preview hole inside the Beranda page.
            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 0

                // --- sidebar ------------------------------------------------
                // One button per page. >= 40px targets, Accessible.name,
                // highlighted = current page (no coloured accent border - the
                // highlight is a tonal wash + bold text per B-TOKEN a11y).
                // fillWidth:false + maximumWidth pin the rail so the content
                // StackLayout claims the rest of the row.
                ColumnLayout {
                    Layout.preferredWidth: 180
                    Layout.minimumWidth: 160
                    Layout.maximumWidth: 200
                    Layout.fillWidth: false
                    Layout.fillHeight: true
                    spacing: 0

                    // Beranda
                    Button {
                        id: sidebarBeranda
                        objectName: "sidebarBeranda"
                        Layout.fillWidth: true
                        Layout.margins: Theme.space2
                        implicitHeight: 48
                        focusPolicy: Qt.StrongFocus
                        flat: root.currentPage !== root.pageBeranda
                        highlighted: root.currentPage === root.pageBeranda
                        text: qsTr("Beranda")
                        Accessible.name: text
                        onClicked: root.switchPage(root.pageBeranda)

                        Keys.onReturnPressed: {
                            root.switchPage(root.pageBeranda)
                            event.accepted = true
                        }
                        Keys.onEnterPressed: {
                            root.switchPage(root.pageBeranda)
                            event.accepted = true
                        }

                        contentItem: Text {
                            text: sidebarBeranda.text
                            font.pixelSize: Theme.fontM
                            font.weight: root.currentPage === root.pageBeranda
                                         ? Theme.fontWeightSemibold
                                         : Theme.fontWeightRegular
                            color: Theme.text
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            radius: Theme.radiusS
                            color: root.currentPage === root.pageBeranda
                                   ? Theme.surface2 : "transparent"
                            border.width: sidebarBeranda.activeFocus ? 2 : 0
                            border.color: Theme.accent
                        }
                    }

                    // Pengaturan
                    Button {
                        id: sidebarPengaturan
                        objectName: "sidebarPengaturan"
                        Layout.fillWidth: true
                        Layout.margins: Theme.space2
                        implicitHeight: 48
                        focusPolicy: Qt.StrongFocus
                        flat: root.currentPage !== root.pagePengaturan
                        highlighted: root.currentPage === root.pagePengaturan
                        text: qsTr("Pengaturan")
                        Accessible.name: text
                        onClicked: root.switchPage(root.pagePengaturan)

                        Keys.onReturnPressed: {
                            root.switchPage(root.pagePengaturan)
                            event.accepted = true
                        }
                        Keys.onEnterPressed: {
                            root.switchPage(root.pagePengaturan)
                            event.accepted = true
                        }

                        contentItem: Text {
                            text: sidebarPengaturan.text
                            font.pixelSize: Theme.fontM
                            font.weight: root.currentPage === root.pagePengaturan
                                         ? Theme.fontWeightSemibold
                                         : Theme.fontWeightRegular
                            color: Theme.text
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            radius: Theme.radiusS
                            color: root.currentPage === root.pagePengaturan
                                   ? Theme.surface2 : "transparent"
                            border.width: sidebarPengaturan.activeFocus ? 2 : 0
                            border.color: Theme.accent
                        }
                    }

                    // Absorbs the rail's leftover height so the two buttons
                    // stay pinned to the top.
                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                    }
                }

                // Thin divider between sidebar and content.
                Rectangle {
                    Layout.fillHeight: true
                    Layout.preferredWidth: 1
                    color: Theme.surface2
                }

                // --- content ------------------------------------------------
                StackLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    currentIndex: root.currentPage

                    // ---------------------------------------------------------
                    // Beranda (Wallpaper page)
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
                        statusKindNotRunning: Main.StatusKind.NotRunning
                        statusKindPaused: Main.StatusKind.Paused
                        dialogOpened: root.dialogOpened
                        dialogClosed: root.dialogClosed
                        firstRunActive: root.firstRunActive
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
                        // Todo 15: the C-16 Info teknis dialog and the C-17
                        // update flow live in AppDialogs; the page gets the
                        // entry points instead of duplicating the dialogs.
                        openInfoTeknis: appDialogs.openInfoTeknis
                        checkUpdates: appDialogs.checkUpdatesInteractive
                        // C-17 Tentang: the old top menu was its only opener
                        // before B-FLOW; the Umum button now calls this.
                        openAbout: appDialogs.openAbout
                    }
                }
            }
        }
    }

    // --- ToastBar (todo 13) ----------------------------------------------
    // Window-bottom compress progress toast. Anchored to the root's bottom
    // edge, so it is never above the preview hole (B-WIREFRAME(f)). Shows
    // D3 progress text while a job runs and the C-10 outcome sentence
    // briefly after cancel / failure / consent rejection.
    ToastBar {
        id: toastBar
    }

    // --- App-level dialogs ---------------------------------------------------
    // Todo 5: "Info teknis". Todo 13: C-9 consent + C-8 compress-first offer
    // moved here from the old standalone dialog files. The offer callbacks
    // are Main-root functions (brief Â§4.2: the paths stay wired at root);
    // consentCancelled surfaces the C-10 cancel sentence in ToastBar.
    AppDialogs {
        id: appDialogs
        anchors.fill: parent
        dialogOpened: root.dialogOpened
        dialogClosed: root.dialogClosed
        startCompressFirst: root.startCompressFirst
        declineCompressFirst: root.declineCompressFirst
        onConsentCancelled: {
            toastBar.showOutcome(qsTr("Dibatalkan. Video aslinya tetap ada di koleksi."))
        }
    }

    // The standalone AboutDialog.qml was replaced by AppDialogs' C-17
    // "Tentang K6WP Studio" + "Versi baru tersedia" dialogs (plan todo 15).

    // --- First-run onboarding (plan todo 16) -------------------------------
    // ONE screen (C-2) replaces the deleted four-page wizard (FirstRunDialog.qml).
    // It is a full-window overlay OUTSIDE WallpaperPage/PreviewHole.qml; while
    // up it feeds dialogOpened()/dialogClosed() so the native preview surface
    // steps aside (overlay contract B-WIREFRAME(f)). Visibility is the single
    // firstRunActive gate above.
    OnboardingView {
        id: onboardingView
        anchors.fill: parent
        visible: root.firstRunActive
        dialogOpened: root.dialogOpened
        dialogClosed: root.dialogClosed
        // DEF-1: Selesai consumes the session latch, so no later import can
        // re-open the overlay.
        firstRunFinished: root.finishFirstRunFlow
    }

    Component.onCompleted: {
        // DEF-1: arm the one-shot first-run latch now, before the first frame
        // is drawn. A returning user (settings file present) or a mid-flow
        // restart (library non-empty) starts disarmed.
        root.firstRunFlowActive = Library.firstRunEligible && !Studio.videoActive
        // Row 41: the Beranda view is the start page, so its monitor list
        // has to be populated here as well - switchPage cannot fire for the
        // initial show. refreshDisplays() emits displaysChanged, which seeds
        // selectedMonitorKey through ensureSelectedMonitor().
        Studio.refreshDisplays()
    }

}


