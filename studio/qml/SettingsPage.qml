// Settings page (Pengaturan tab). Two-column ScrollView: left carries language,
// basic settings and compressor defaults; right carries cache / engine /
// update groups and the log. The sub-panels (GeneralPanel / CacheEnginePanel)
// are inline sections of this page.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: settingsPage

    property int labelColWidth: 132
    property int controlWidth: 320
    property int narrowControlWidth: 220
    property int spinWidth: 140
    property int formColSpacing: 12
    property int formRowSpacing: 10
    property var dialogOpened: function () {}
    property var dialogClosed: function () {}

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

            // --- GeneralPanel (inline): language + basic + compressor defaults ---
            ColumnLayout {
                Layout.fillWidth: true
                spacing: formRowSpacing

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
                    columnSpacing: formColSpacing
                    rowSpacing: formRowSpacing

                    Label {
                        text: qsTr("Bahasa antarmuka:")
                        Layout.preferredWidth: labelColWidth
                    }

                    ComboBox {
                        Layout.preferredWidth: narrowControlWidth
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
                    columnSpacing: formColSpacing
                    rowSpacing: formRowSpacing

                    Label {
                        text: qsTr("Isi layar:")
                        Layout.preferredWidth: labelColWidth
                    }

                    ComboBox {
                        Layout.fillWidth: true
                        Layout.maximumWidth: controlWidth
                        model: [qsTr("Isi layar (potong bila perlu)"),
                                qsTr("Sesuaikan (seluruh video terlihat)"),
                                qsTr("Regang (isi penuh)"),
                                qsTr("Tengah (ukuran asli)")]
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
                        Layout.preferredWidth: labelColWidth
                    }

                    ComboBox {
                        Layout.fillWidth: true
                        Layout.maximumWidth: controlWidth
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
                        Layout.preferredWidth: labelColWidth
                    }

                    SpinBox {
                        Layout.preferredWidth: spinWidth
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
                    columnSpacing: formColSpacing
                    rowSpacing: formRowSpacing

                    Label {
                        text: qsTr("Folder output:")
                        Layout.preferredWidth: labelColWidth
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
                    columnSpacing: formColSpacing
                    rowSpacing: formRowSpacing

                    Label {
                        text: qsTr("CRF bawaan:")
                        Layout.preferredWidth: labelColWidth
                    }

                    SpinBox {
                        Layout.preferredWidth: spinWidth
                        from: 16
                        to: 28
                        editable: true
                        value: Settings.defaultCrf
                        onValueModified: Settings.setDefaultCrf(value)
                    }

                    Label {
                        text: qsTr("FPS bawaan:")
                        Layout.preferredWidth: labelColWidth
                    }

                    SpinBox {
                        Layout.preferredWidth: spinWidth
                        from: 1
                        to: 30
                        editable: true
                        value: Settings.defaultFps
                        onValueModified: Settings.setDefaultFps(value)
                    }

                    Label {
                        text: qsTr("Resolusi bawaan:")
                        Layout.preferredWidth: labelColWidth
                    }

                    ComboBox {
                        Layout.fillWidth: true
                        Layout.maximumWidth: narrowControlWidth
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

            // --- CacheEnginePanel (inline): cache + engine + update + log ---
            ColumnLayout {
                Layout.fillWidth: true
                spacing: formRowSpacing

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
                    columnSpacing: formColSpacing
                    rowSpacing: formRowSpacing

                    Label {
                        text: qsTr("Cache:")
                        Layout.preferredWidth: labelColWidth
                    }

                    Label {
                        Layout.fillWidth: true
                        Layout.maximumWidth: 320
                        elide: Text.ElideMiddle
                        opacity: 0.8
                        text: Settings.cacheDir
                    }

                    RowLayout {
                        spacing: formColSpacing

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
                    columnSpacing: formColSpacing
                    rowSpacing: formRowSpacing

                    Label {
                        text: qsTr("Mode hemat baterai:")
                        Layout.preferredWidth: labelColWidth
                    }

                    ComboBox {
                        Layout.fillWidth: true
                        Layout.maximumWidth: controlWidth
                        model: [qsTr("Batasi 24 fps (hemat)"), qsTr("Jeda penuh saat baterai (paling hemat)")]
                        readonly property var modes: ["cap24", "static"]
                        currentIndex: Math.max(0, modes.indexOf(Settings.batteryMode))
                        onActivated: Settings.setBatteryMode(modes[index])
                    }

                    Label {
                        text: qsTr("Inti CPU:")
                        Layout.preferredWidth: labelColWidth
                    }

                    ComboBox {
                        Layout.fillWidth: true
                        Layout.maximumWidth: controlWidth
                        model: [qsTr("Otomatis (inti efisiensi/E-core)"), qsTr("Semua inti")]
                        readonly property var modes: ["auto", "all"]
                        currentIndex: Math.max(0, modes.indexOf(Settings.cpuAffinity))
                        onActivated: Settings.setCpuAffinity(modes[index])
                    }

                    Label {
                        text: qsTr("GPU:")
                        Layout.preferredWidth: labelColWidth
                    }

                    ComboBox {
                        Layout.fillWidth: true
                        Layout.maximumWidth: controlWidth
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
                    spacing: formColSpacing

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

                LogPanel {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.minimumHeight: 140
                    text: settingsPage.allLog.join("\n")
                }
            }
        }
    }

    CacheDialog {
        id: cacheDialog
        dialogOpened: settingsPage.dialogOpened
        dialogClosed: settingsPage.dialogClosed
    }
}
