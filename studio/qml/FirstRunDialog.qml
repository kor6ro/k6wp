// First-run wizard dialog. QWizardPage cannot run inside a QQuickWidget, so
// the four wizard pages are a StackLayout here. The gate is the same pure
// IsFirstRunCondition the Widgets wizard used: no settings file, empty
// library, and (supplied by Studio) no active video.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: firstRunDialog
    property var dialogOpened: function () {}
    property var dialogClosed: function () {}

    title: qsTr("Selamat datang di K6WP")
    modal: true
    anchors.centerIn: parent
    closePolicy: Popup.NoAutoClose

    // The mpv preview HWND composites above the QQuickWidget, so it has to
    // step aside for this dialog (same as the About box).
    onOpened: dialogOpened()
    onClosed: dialogClosed()

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
