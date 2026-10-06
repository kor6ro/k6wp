// Compressor page (Kompresor tab). Contains the long-video gate Connections,
// the source/output GridLayout, the compress status pane, the advanced-mode
// pane, and the consent dialog. The three sub-panels (SourcePanel /
// StatusPanel / AdvancedPanel) are inline sections of this page.
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Item {
    id: compressorPage

    property int labelColWidth: 132
    property int formColSpacing: 12
    property int formRowSpacing: 10
    property var applyAfterCompress: false
    property var consentDuration: 0
    property var dialogOpened: function () {}
    property var dialogClosed: function () {}

    // The long-video gate. AskLongVideoConsent is a QMessageBox
    // in C++, so the decision is routed through QML instead.
    Connections {
        target: Compress
        function onConsentRequired(durationMinutes) {
            compressorPage.consentDuration = durationMinutes
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
            const apply_it = compressorPage.applyAfterCompress || Compress.autoApply
            compressorPage.applyAfterCompress = false
            if (apply_it)
                Studio.applyWallpaper(Compress.resultPath)
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

    // Bounded + two columns. Previously one full-width vertical
    // stack, so on a 1936px window "Sumber:" sat at x=10 with its
    // Jelajahi button ~1900px away and the bottom 40% was dead
    // space. The left column holds the paths and the compress
    // job; the right rail the technical knobs.
    RowLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        // --- SourcePanel (inline): paths + compress job ---
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
                columnSpacing: formColSpacing
                rowSpacing: formRowSpacing

                Label {
                    text: qsTr("Sumber:")
                    Layout.preferredWidth: labelColWidth
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
                    Layout.preferredWidth: labelColWidth
                }

                Label {
                    Layout.fillWidth: true
                    Layout.maximumWidth: 480
                    elide: Text.ElideMiddle
                    opacity: 0.8
                    text: Compress.outDir
                }

                RowLayout {
                    spacing: formColSpacing

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

            // --- StatusPanel (inline): compress status ----
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
                        columnSpacing: formColSpacing
                        rowSpacing: formRowSpacing

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
                                compressorPage.applyAfterCompress = false
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

        // --- AdvancedPanel (inline): Mode lanjutan (teknis) ---
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
                spacing: formRowSpacing

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
                    columnSpacing: formColSpacing
                    rowSpacing: formRowSpacing

                    Label {
                        text: qsTr("CRF:")
                        Layout.preferredWidth: labelColWidth
                    }

                    SpinBox {
                        id: crfBox
                        Layout.fillWidth: true
                        Layout.maximumWidth: 220
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
                        Layout.preferredWidth: labelColWidth
                    }

                    SpinBox {
                        id: fpsBox
                        Layout.fillWidth: true
                        Layout.maximumWidth: 220
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
                        Layout.preferredWidth: labelColWidth
                    }

                    TextField {
                        id: resolutionField
                        Layout.fillWidth: true
                        Layout.maximumWidth: 220
                        placeholderText: qsTr("mis. 1920x1080 (kosong = ikut sumber)")
                        text: Compress.resolutionText
                        onEditingFinished: Compress.setResolutionText(text)
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Resolusi output WxH")
                    }

                    Label {
                        text: qsTr("Encoder:")
                        Layout.preferredWidth: labelColWidth
                    }

                    ComboBox {
                        id: encoderCombo
                        Layout.fillWidth: true
                        Layout.maximumWidth: 220
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

    ConsentDialog {
        id: consentDialog
        consentDuration: compressorPage.consentDuration
        dialogOpened: compressorPage.dialogOpened
        dialogClosed: compressorPage.dialogClosed
    }
}
