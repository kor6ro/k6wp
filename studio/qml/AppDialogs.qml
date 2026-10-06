// AppDialogs: the app-level dialogs that must not share the main screen.
//
// Hosts three modals (plan todo 5 + todo 13):
//   1. "Info teknis" (brief C-16) - friendly status + raw support details +
//      "Salin untuk dukungan".
//   2. Long-video consent (brief C-9, todo 13) - Compress.onConsentRequired
//      -> this dialog -> Compress.resolveConsent. Copy verbatim from C-9.
//   3. Compress-first offer (brief C-8, todo 13) - Library.onCompressFirst
//      Required / maybeOfferCompressFirst -> this dialog. Copy verbatim
//      from C-8.
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

    // Shared 40px dialog button component: token colours, visible focus ring,
    // Enter/Space, Accessible.name. Used by the C-8/C-9 footers below.
    component DialogActionButton: Button {
        id: dialogActionButton
        property bool primary: false
        implicitHeight: 40
        focusPolicy: Qt.StrongFocus
        leftPadding: Theme.space3
        rightPadding: Theme.space3
        Accessible.name: text
        Keys.onReturnPressed: {
            dialogActionButton.clicked()
            event.accepted = true
        }
        Keys.onEnterPressed: {
            dialogActionButton.clicked()
            event.accepted = true
        }
        contentItem: Text {
            text: dialogActionButton.text
            font.pixelSize: Theme.fontM
            font.weight: dialogActionButton.primary ? Theme.fontWeightSemibold
                                                    : Theme.fontWeightRegular
            color: dialogActionButton.primary ? Theme.accentText : Theme.text
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
        background: Rectangle {
            radius: Theme.radiusS
            color: dialogActionButton.primary
                   ? Theme.accent
                   : (dialogActionButton.hovered || dialogActionButton.activeFocus
                      ? Theme.surface2 : Theme.surface)
            border.width: dialogActionButton.activeFocus ? 2 : 0
            border.color: dialogActionButton.primary ? Theme.accentText : Theme.accent
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
        standardButtons: Dialog.Close

        onOpened: {
            appDialogs.dialogOpened()
            copySupportButton.forceActiveFocus()
        }
        onClosed: appDialogs.dialogClosed()

        contentItem: ColumnLayout {
            spacing: 8
            Accessible.role: Accessible.Dialog
            Accessible.name: infoTeknisDialog.title

            // Friendly status first (C-16: "status ramah").
            Label {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                font.bold: true
                font.pixelSize: Theme.fontM
                text: appDialogs.friendlyStatus()
            }

            // Raw details, shown only here.
            GridLayout {
                Layout.fillWidth: true
                columns: 2
                columnSpacing: 12
                rowSpacing: 4

                Label { text: qsTr("pid") }
                Label {
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                    text: String(Studio.enginePid)
                }

                Label { text: qsTr("Video aktif") }
                Label {
                    Layout.fillWidth: true
                    elide: Text.ElideMiddle
                    text: Studio.activeVideoPath.length > 0
                          ? Studio.activeVideoPath
                          : qsTr("(belum ada video aktif)")
                }

                Label { text: qsTr("Path pengaturan") }
                Label {
                    Layout.fillWidth: true
                    elide: Text.ElideMiddle
                    text: Studio.settingsPath
                }
            }

            Label {
                text: qsTr("Log")
                font.bold: true
            }

            TextArea {
                objectName: "infoTeknisLog"
                Layout.fillWidth: true
                Layout.preferredHeight: 180
                readOnly: true
                selectByMouse: true
                wrapMode: TextEdit.Wrap
                font.pixelSize: Theme.fontS
                text: Studio.log.join("\n")
            }

            RowLayout {
                Layout.fillWidth: true

                Button {
                    id: copySupportButton
                    objectName: "copySupportButton"
                    text: qsTr("Salin untuk dukungan")
                    Accessible.name: text
                    onClicked: Studio.copyToClipboard(appDialogs.supportText())
                }

                Item {
                    Layout.fillWidth: true
                }
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
                onClicked: {
                    consentDialog.close()
                    Compress.resolveConsent(true)
                }
            }
            DialogActionButton {
                id: consentRejectButton
                objectName: "consentReject"
                text: qsTr("Batal")
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
                onClicked: {
                    compressOfferDialog.close()
                    appDialogs.startCompressFirst()
                }
            }
            DialogActionButton {
                id: offerDeclineButton
                objectName: "offerDecline"
                text: qsTr("Pasang saja")
                onClicked: {
                    compressOfferDialog.close()
                    appDialogs.declineCompressFirst()
                }
            }
        }
    }
}
