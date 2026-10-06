// AppDialogs: the app-level dialogs that must not share the main screen.
//
// First one (plan todo 5 / brief C-16): "Info teknis" - the friendly status
// plus the raw support details (process id, paths, log) that used to render
// in the Wallpaper right rail, and a "Salin untuk dukungan" button wired to
// Studio.copyToClipboard(). Later slices extend this file with the compress
// consent / first-offer dialogs.
//
// Lifecycle contract (brief B-WIREFRAME(f)): the native mpv preview is a
// separate HWND DWM composites above the QQuickWidget, so every modal feeds
// the root dialogOpened()/dialogClosed() pair - that hides the native
// surface while the dialog is up and restores it when the last one closes.
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Item {
    id: appDialogs

    property var dialogOpened: function () {}
    property var dialogClosed: function () {}

    function openInfoTeknis() {
        infoTeknisDialog.open()
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
}
