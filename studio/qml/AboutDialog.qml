// About dialog: version, license, third-party licenses, update indicator,
// support/project/release links. Dialog lifecycle: onOpened / onClosed feed
// the root dialogOpened / dialogClosed pair so the native mpv preview HWND
// steps aside while this modal is showing.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: aboutDialog
    property var dialogOpened: function () {}
    property var dialogClosed: function () {}

    title: qsTr("About K6WP")
    modal: true
    anchors.centerIn: parent
    standardButtons: Dialog.Close

    // The mpv preview is a separate HWND that DWM composites above the
    // QQuickWidget, so a QML dialog cannot draw over it. A 0x0 rect tells
    // QmlShell to drop the native surface; syncPreview() puts it back.
    onOpened: dialogOpened()
    onClosed: dialogClosed()

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
