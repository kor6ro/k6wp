// AppDialogs: the app-level dialogs that must not share the main screen.
//
// Hosts five modals (plan todo 5 + todo 13 + todo 15):
//   1. "Info teknis" (brief C-16) - friendly status + raw support details +
//      "Salin untuk dukungan".
//   2. Long-video consent (brief C-9, todo 13) - Compress.onConsentRequired
//      -> this dialog -> Compress.resolveConsent. Copy verbatim from C-9.
//   3. Compress-first offer (brief C-8, todo 13) - Library.onCompressFirst
//      Required / maybeOfferCompressFirst -> this dialog. Copy verbatim
//      from C-8.
//   4. "Tentang K6WP Studio" (brief C-17, todo 15) - version/license/support
//      + [Cek pembaruan]. Replaces the old standalone AboutDialog.qml.
//   5. "Versi baru tersedia ({v})" update dialog (brief C-17, todo 15) -
//      [Buka halaman unduhan] [Nanti]. Only a user-initiated check raises
//      it; the silent start-up check never pops a modal.
//
// Lifecycle contract (brief B-WIREFRAME(f)): the native mpv preview is a
// separate HWND DWM composites above the QQuickWidget, so every modal feeds
// the root dialogOpened()/dialogClosed() pair - that hides the native
// surface while the dialog is up and restores it when the last one closes.
//
// Consent pairing (Metis F2): the consentDialog is ALWAYS paired with
// resolveConsent - accept -> resolveConsent(true), reject -> resolveConsent
// (false) + the consentCancelled signal so Main can surface the C-10 cancel
// sentence in the ToastBar. Neither path may be dropped.
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Item {
    id: appDialogs

    property var dialogOpened: function () {}
    property var dialogClosed: function () {}

    // --- C-9 consent state -------------------------------------------------
    // Duration reported by CompressBridge's long-video gate, in minutes.
    property double consentDuration: 0

    // --- C-8 offer state ---------------------------------------------------
    // offerPath / offerMb mirror the values Main already holds on its root;
    // Main pushes them here right before openCompressOffer().
    property string offerPath: ""
    property int offerMb: 0
    property var startCompressFirst: function () {}
    property var declineCompressFirst: function () {}

    // Raised when the user rejects the C-9 consent. Main connects this to
    // ToastBar.showOutcome(C-10 cancel sentence) so the user sees that the
    // original file is untouched.
    signal consentCancelled()

    function openInfoTeknis() {
        infoTeknisDialog.open()
    }

    // C-9 entry point: called from Main's Compress.onConsentRequired handler.
    function openConsent(durationMinutes) {
        consentDuration = Number(durationMinutes)
        consentDialog.open()
    }

    // C-8 entry point: called from Main's onCompressFirstRequired /
    // maybeOfferCompressFirst after the root properties are set.
    function openCompressOffer(path, mb) {
        offerPath = String(path === undefined || path === null ? "" : path)
        offerMb = Number(mb)
        compressOfferDialog.open()
    }

    // C-3 composition, reused for the dialog headline and the support copy.
    function friendlyStatus() {
        const name = Studio.statusVideoName
        return name.length > 0 ? Studio.statusTitle + " • " + name
                               : Studio.statusTitle
    }

    // Everything the support copy needs, in one plain-text block.
    function supportText() {
        const log = Studio.log.length > 0 ? Studio.log.join("\n") : "(kosong)"
        const video = Studio.activeVideoPath.length > 0
                    ? Studio.activeVideoPath
                    : "(belum ada video aktif)"
        return "K6WP Studio " + Studio.version + "\n"
             + "Status: " + friendlyStatus() + "\n"
             + "pid: " + Studio.enginePid + "\n"
             + "Video aktif: " + video + "\n"
             + "Path pengaturan: " + Studio.settingsPath + "\n"
             + "Log:\n" + log
    }

    // Shared 40px dialog button component: token colours, a shared
    // Theme.glyph icon, visible focus ring (ink, never the blue accent),
    // Enter/Space, Accessible.name. Used by every dialog footer below.
    component DialogActionButton: Button {
        id: dialogActionButton
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
            dialogActionButton.clicked()
            event.accepted = true
        }
        Keys.onEnterPressed: {
            dialogActionButton.clicked()
            event.accepted = true
        }
        contentItem: RowLayout {
            spacing: Theme.space1

            Text {
                visible: dialogActionButton.glyph.length > 0
                Layout.alignment: Qt.AlignVCenter
                text: dialogActionButton.glyph
                font.family: Theme.glyphFont
                font.pixelSize: Theme.fontM
                color: dialogActionButton.primary ? Theme.accentText
                                                  : Theme.text
                verticalAlignment: Text.AlignVCenter
                Accessible.ignored: true
            }

            Text {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
                text: dialogActionButton.text
                font.pixelSize: Theme.fontM
                font.weight: dialogActionButton.primary ? Theme.fontWeightSemibold
                                                        : Theme.fontWeightRegular
                color: dialogActionButton.primary ? Theme.accentText : Theme.text
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }
        }
        background: Rectangle {
            radius: Theme.radiusS
            color: dialogActionButton.primary
                   ? (dialogActionButton.down ? Qt.darker(Theme.accent, 1.15)
                                              : Theme.accent)
                   : (dialogActionButton.down ? Theme.pressedSurface
                      : (dialogActionButton.hovered
                         || dialogActionButton.activeFocus
                         ? Theme.surface2 : Theme.surface))
            border.width: dialogActionButton.activeFocus ? 2 : 0
            border.color: dialogActionButton.primary ? Theme.accentText
                                                     : Theme.focusRing
        }
    }

    // Shared dialog chrome. The stock Material Dialog paints its surface and
    // header from Material.dialogColor, and the popup-local Material attached
    // object does not inherit the window root's Material.theme across the
    // popup boundary - so dialogs stayed light while the window was dark.
    // Every dialog below paints its own surface/header from Theme tokens and
    // pins its popup-local Material theme to Theme.dark, so dialog chrome
    // follows the app theme in both modes.
    component DialogSurface: Rectangle {
        color: Theme.surface
        radius: Theme.radiusL
        border.width: 1
        border.color: Theme.surface2
    }

    // Shared dialog title row: one Theme.glyph icon + the dialog title. The
    // glyph is decorative (the title text carries the name); padding matches
    // the old Label-only header.
    component DialogTitle: RowLayout {
        id: dialogTitle
        property string glyph: ""
        property string titleText: ""
        spacing: Theme.space2

        Label {
            Layout.leftMargin: Theme.space3
            Layout.topMargin: Theme.space3
            Layout.alignment: Qt.AlignVCenter
            text: dialogTitle.glyph
            visible: dialogTitle.glyph.length > 0
            font.family: Theme.glyphFont
            font.pixelSize: Theme.fontL
            color: Theme.text2
            Accessible.ignored: true
        }

        Label {
            Layout.leftMargin: dialogTitle.glyph.length > 0 ? 0 : Theme.space3
            Layout.topMargin: Theme.space3
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            text: dialogTitle.titleText
            elide: Label.ElideRight
            color: Theme.text
            font.pixelSize: Theme.fontL
            font.weight: Theme.fontWeightSemibold
        }
    }

    // ======================================================================
    // 1. Info teknis (brief C-16, todo 5)
    // ======================================================================
    Dialog {
        id: infoTeknisDialog
        objectName: "infoTeknisDialog"
        title: qsTr("Info teknis")
        modal: true
        anchors.centerIn: parent
        width: Math.min(640, parent.width - 48)
        // Task 32: tighter than the Material 24 default.
        padding: Theme.space3
        // Pin the popup-local Material theme to the app Theme: attached
        // Material properties do not inherit across the popup boundary.
        Material.theme: Theme.dark ? Material.Dark : Material.Light
        Material.accent: Theme.accent
        background: DialogSurface {}
        header: DialogTitle {
            glyph: Theme.glyph.info
            titleText: infoTeknisDialog.title
        }

        onOpened: {
            appDialogs.dialogOpened()
            copySupportButton.forceActiveFocus()
        }
        onClosed: appDialogs.dialogClosed()

        contentItem: ColumnLayout {
            spacing: Theme.space2
            Accessible.role: Accessible.Dialog
            Accessible.name: infoTeknisDialog.title

            // Friendly status first (C-16: "status ramah").
            Label {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                font.bold: true
                font.pixelSize: Theme.fontM
                color: Theme.text
                text: appDialogs.friendlyStatus()
            }

            // Raw details, shown only here.
            GridLayout {
                Layout.fillWidth: true
                columns: 2
                columnSpacing: 12
                rowSpacing: Theme.space1

                Label {
                    text: qsTr("pid")
                    color: Theme.text2
                }
                Label {
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                    color: Theme.text
                    text: String(Studio.enginePid)
                }

                Label {
                    text: qsTr("Video aktif")
                    color: Theme.text2
                }
                Label {
                    Layout.fillWidth: true
                    elide: Text.ElideMiddle
                    color: Theme.text
                    text: Studio.activeVideoPath.length > 0
                          ? Studio.activeVideoPath
                          : qsTr("(belum ada video aktif)")
                }

                Label {
                    text: qsTr("Path pengaturan")
                    color: Theme.text2
                }
                Label {
                    Layout.fillWidth: true
                    elide: Text.ElideMiddle
                    color: Theme.text
                    text: Studio.settingsPath
                }
            }

            Label {
                text: qsTr("Log")
                font.bold: true
                color: Theme.text
            }

            TextArea {
                id: infoTeknisLog
                objectName: "infoTeknisLog"
                Layout.fillWidth: true
                Layout.preferredHeight: 180
                readOnly: true
                selectByMouse: true
                wrapMode: TextEdit.Wrap
                font.pixelSize: Theme.fontS
                color: Theme.text
                selectionColor: Theme.accent
                selectedTextColor: Theme.accentText
                text: Studio.log.join("\n")

                background: Rectangle {
                    color: Theme.surface2
                    radius: Theme.radiusS
                    border.width: infoTeknisLog.activeFocus ? 2 : 0
                    border.color: Theme.focusRing
                }
            }

            RowLayout {
                Layout.fillWidth: true

                DialogActionButton {
                    id: copySupportButton
                    objectName: "copySupportButton"
                    text: qsTr("Salin untuk dukungan")
                    onClicked: Studio.copyToClipboard(appDialogs.supportText())
                }

                Item {
                    Layout.fillWidth: true
                }
            }
        }

        // The stock Dialog.Close standard button is replaced by a token
        // button so the close affordance follows Theme in both modes.
        footer: RowLayout {
            spacing: Theme.space2
            Item {
                Layout.fillWidth: true
            }
            DialogActionButton {
                id: infoTeknisCloseButton
                objectName: "infoTeknisClose"
                text: qsTr("Tutup")
                glyph: Theme.glyph.close
                onClicked: infoTeknisDialog.close()
            }
        }
    }

    // ======================================================================
    // 2. Long-video consent (brief C-9, todo 13) ---------------------------
    // Raised by Compress.onConsentRequired when a probed duration exceeds
    // 10 minutes and forceLong is off. Accept -> resolveConsent(true)
    // (one-shot force, job continues); reject -> resolveConsent(false)
    // (job abandoned, original stays) + consentCancelled for the C-10 toast.
    // ======================================================================
    Dialog {
        id: consentDialog
        objectName: "consentDialog"
        // C-9 title uses the glossary term "Video panjang" (keputusan 4 /
        // B-GLOSARIUM: the old "Kompres-ulang" wording becomes "Perkecil").
        // The dialog body below is C-9 verbatim.
        title: qsTr("Video panjang")
        modal: true
        anchors.centerIn: Overlay.overlay
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        // Task 32: tighter than the Material 24 default.
        padding: Theme.space3
        Material.theme: Theme.dark ? Material.Dark : Material.Light
        Material.accent: Theme.accent
        background: DialogSurface {}
        header: DialogTitle {
            glyph: Theme.glyph.hourglass
            titleText: consentDialog.title
        }

        onOpened: {
            appDialogs.dialogOpened()
            consentAcceptButton.forceActiveFocus()
        }
        onClosed: appDialogs.dialogClosed()

        contentItem: ColumnLayout {
            spacing: Theme.space2
            Accessible.role: Accessible.Dialog
            Accessible.name: consentDialog.title

            // C-9 verbatim: "Video ini panjang ({m} menit). Menyiapkannya
            // butuh waktu lama. Lanjut?" - one translatable sentence with a
            // %1 placeholder, never split across two qsTr() calls.
            Label {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                font.pixelSize: Theme.fontM
                color: Theme.text
                text: qsTr("Video ini panjang %1 menit. "
                           + "Menyiapkannya butuh waktu lama. Lanjut?")
                      .arg(appDialogs.consentDuration.toFixed(1))
            }
        }

        // C-9 buttons verbatim: [Siapkan] [Batal]. Custom footer, not
        // standardButtons, so the labels match the copy deck exactly.
        footer: RowLayout {
            spacing: Theme.space2
            Item {
                Layout.fillWidth: true
            }
            DialogActionButton {
                id: consentAcceptButton
                objectName: "consentAccept"
                primary: true
                text: qsTr("Siapkan")
                glyph: Theme.glyph.shrink
                onClicked: {
                    consentDialog.close()
                    Compress.resolveConsent(true)
                }
            }
            DialogActionButton {
                id: consentRejectButton
                objectName: "consentReject"
                text: qsTr("Batal")
                glyph: Theme.glyph.close
                onClicked: {
                    consentDialog.close()
                    Compress.resolveConsent(false)
                    // Metis F2 pairing: the reject path must also surface
                    // the C-10 cancel sentence so the user learns the
                    // original file is untouched.
                    appDialogs.consentCancelled()
                }
            }
        }
    }

    // ======================================================================
    // 3. Compress-first offer (brief C-8, todo 13) -------------------------
    // Raised when a library/apply path is over the compress threshold.
    // Accept -> startCompressFirst (Main root, 1080p pin + applyAfterCompress
    // one-shot); reject -> declineCompressFirst (the caller's skipped action
    // still runs, or a no-op on the import path). Copy verbatim from C-8.
    // ======================================================================
    Dialog {
        id: compressOfferDialog
        objectName: "compressOfferDialog"
        // C-8 title: the glossary renames the old compressor wording to
        // "Perkecil"; the offer is about preparing a big video, so the title
        // says what the dialog is for. Body + buttons below are C-8 verbatim.
        title: qsTr("Video besar")
        modal: true
        anchors.centerIn: Overlay.overlay
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        // Task 32: tighter than the Material 24 default.
        padding: Theme.space3
        Material.theme: Theme.dark ? Material.Dark : Material.Light
        Material.accent: Theme.accent
        background: DialogSurface {}
        header: DialogTitle {
            glyph: Theme.glyph.shrink
            titleText: compressOfferDialog.title
        }

        onOpened: {
            appDialogs.dialogOpened()
            offerAcceptButton.forceActiveFocus()
        }
        onClosed: appDialogs.dialogClosed()

        contentItem: ColumnLayout {
            spacing: Theme.space2
            Accessible.role: Accessible.Dialog
            Accessible.name: compressOfferDialog.title

            // C-8 verbatim: "Video ini besar ({MB} MB). Kami siapkan dulu
            // supaya ringan diputar." - one translatable sentence with %1.
            Label {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                font.pixelSize: Theme.fontM
                color: Theme.text
                text: qsTr("Video ini besar %1 MB. "
                           + "Kami siapkan dulu supaya ringan diputar.")
                      .arg(appDialogs.offerMb)
            }

            // The path, as its own line (C-6 filename pattern) - C-8's
            // sentence does not embed it, but the user needs to know WHICH
            // file the offer is about.
            Label {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                elide: Text.ElideMiddle
                font.pixelSize: Theme.fontS
                font.weight: Theme.fontWeightSemibold
                color: Theme.text2
                text: appDialogs.offerPath
            }
        }

        // C-8 buttons verbatim: [Siapkan otomatis] [Pasang saja].
        footer: RowLayout {
            spacing: Theme.space2
            Item {
                Layout.fillWidth: true
            }
            DialogActionButton {
                id: offerAcceptButton
                objectName: "offerAccept"
                primary: true
                text: qsTr("Siapkan otomatis")
                glyph: Theme.glyph.shrink
                onClicked: {
                    compressOfferDialog.close()
                    appDialogs.startCompressFirst()
                }
            }
            DialogActionButton {
                id: offerDeclineButton
                objectName: "offerDecline"
                text: qsTr("Pasang saja")
                glyph: Theme.glyph.check
                onClicked: {
                    compressOfferDialog.close()
                    appDialogs.declineCompressFirst()
                }
            }
        }
    }

    // ======================================================================
    // 4. Tentang K6WP Studio (brief C-17, todo 15) --------------------------
    // Replaces the standalone AboutDialog.qml. The mpv preview hiding pair
    // (dialogOpened/dialogClosed) is identical to the dialogs above.
    // ======================================================================
    function openAbout() {
        aboutDialog.open()
    }

    // Shared C-17 entry point: the About dialog's [Cek pembaruan] button and
    // the Umum group's button (Main passes settingsPage.checkUpdates =
    // appDialogs.checkUpdatesInteractive) both land here. pendingUpdatePrompt
    // is what makes ONLY a user-initiated check raise the update dialog -
    // the automatic start-up check must never pop a modal.
    property bool pendingUpdatePrompt: false

    function checkUpdatesInteractive() {
        pendingUpdatePrompt = true
        Studio.checkForUpdatesInteractive()
    }

    Connections {
        target: Studio
        function onUpdateChanged() {
            if (!appDialogs.pendingUpdatePrompt || Studio.updateCheckBusy)
                return
            appDialogs.pendingUpdatePrompt = false
            if (Studio.updateAvailable)
                updateDialog.open()
        }
    }

    Dialog {
        id: aboutDialog
        objectName: "aboutDialog"
        // C-17 verbatim title.
        title: qsTr("Tentang K6WP Studio")
        modal: true
        anchors.centerIn: parent
        // Task 32: tighter than the Material 24 default.
        padding: Theme.space3
        Material.theme: Theme.dark ? Material.Dark : Material.Light
        Material.accent: Theme.accent
        background: DialogSurface {}
        header: DialogTitle {
            glyph: Theme.glyph.info
            titleText: aboutDialog.title
        }

        onOpened: {
            appDialogs.dialogOpened()
            aboutCheckButton.forceActiveFocus()
        }
        onClosed: appDialogs.dialogClosed()

        contentItem: ColumnLayout {
            spacing: Theme.space2
            Accessible.role: Accessible.Dialog
            Accessible.name: aboutDialog.title

            Label {
                text: "K6WP Studio " + Studio.version
                font.bold: true
                font.pixelSize: Theme.fontM
                color: Theme.text
            }

            Label {
                text: qsTr("License: GPL-2.0-or-later")
                color: Theme.text2
            }

            Label {
                Layout.preferredWidth: 440
                wrapMode: Text.WordWrap
                color: Theme.text2
                text: qsTr("Third-party licenses: mpv (libmpv), ffmpeg, Qt, nlohmann/json — see LICENSES/ for the full texts.")
            }

            // C-17 sentence verbatim (one translatable string with %1),
            // shown only when the update state is real.
            Label {
                Layout.preferredWidth: 440
                wrapMode: Text.WordWrap
                visible: Studio.updateAvailable
                color: Theme.text
                text: qsTr("Versi baru tersedia %1.").arg(Studio.latestVersion)
            }

            // Task 30: the three C-17 actions stack vertically, full width, in
            // a narrow dialog instead of one crowded row. Same buttons, same
            // order, same wiring - only the axis changed. DialogActionButton
            // keeps its 40px hit target.
            ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.space2

                DialogActionButton {
                    id: aboutCheckButton
                    objectName: "aboutCheckUpdates"
                    Layout.fillWidth: true
                    text: qsTr("Cek pembaruan")
                    glyph: Theme.glyph.refresh
                    enabled: !Studio.updateCheckBusy
                    onClicked: appDialogs.checkUpdatesInteractive()
                }

                DialogActionButton {
                    objectName: "aboutSupport"
                    Layout.fillWidth: true
                    text: qsTr("Support development")
                    glyph: Theme.glyph.external
                    onClicked: Qt.openUrlExternally(Studio.donateUrl)
                }

                DialogActionButton {
                    objectName: "aboutProject"
                    Layout.fillWidth: true
                    text: qsTr("Project page")
                    glyph: Theme.glyph.external
                    onClicked: Qt.openUrlExternally(Studio.projectUrl)
                }
            }
        }

        // The stock Dialog.Close standard button is replaced by a token
        // button so the close affordance follows Theme in both modes.
        footer: RowLayout {
            spacing: Theme.space2
            Item {
                Layout.fillWidth: true
            }
            DialogActionButton {
                id: aboutCloseButton
                objectName: "aboutClose"
                text: qsTr("Tutup")
                glyph: Theme.glyph.close
                onClicked: aboutDialog.close()
            }
        }
    }

    // ======================================================================
    // 5. Versi baru tersedia (brief C-17, todo 15) --------------------------
    // Raised only when a user-initiated check (About or Umum > Cek pembaruan)
    // finds a newer release. Buttons verbatim: [Buka halaman unduhan] [Nanti].
    // ======================================================================
    Dialog {
        id: updateDialog
        objectName: "updateDialog"
        title: qsTr("Pembaruan")
        modal: true
        anchors.centerIn: Overlay.overlay
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        // Task 32: tighter than the Material 24 default.
        padding: Theme.space3
        Material.theme: Theme.dark ? Material.Dark : Material.Light
        Material.accent: Theme.accent
        background: DialogSurface {}
        header: DialogTitle {
            glyph: Theme.glyph.refresh
            titleText: updateDialog.title
        }

        onOpened: {
            appDialogs.dialogOpened()
            updateOpenButton.forceActiveFocus()
        }
        onClosed: appDialogs.dialogClosed()

        contentItem: ColumnLayout {
            spacing: Theme.space2
            Accessible.role: Accessible.Dialog
            Accessible.name: updateDialog.title

            // C-17 verbatim: "Versi baru tersedia ({v})." - %1 carries the
            // version, the same one-placeholder pattern C-8/C-9 use.
            Label {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                font.pixelSize: Theme.fontM
                color: Theme.text
                text: qsTr("Versi baru tersedia %1.").arg(Studio.latestVersion)
            }
        }

        footer: RowLayout {
            spacing: Theme.space2

            Item {
                Layout.fillWidth: true
            }

            DialogActionButton {
                id: updateOpenButton
                objectName: "updateOpenRelease"
                primary: true
                text: qsTr("Buka halaman unduhan")
                glyph: Theme.glyph.external
                onClicked: {
                    updateDialog.close()
                    Qt.openUrlExternally(Studio.latestPageUrl)
                }
            }

            DialogActionButton {
                id: updateLaterButton
                objectName: "updateLater"
                text: qsTr("Nanti")
                glyph: Theme.glyph.close
                onClicked: updateDialog.close()
            }
        }
    }
}
