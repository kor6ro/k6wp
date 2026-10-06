// Quick settings panel: the Wallpaper right-rail pane with fit / monitor /
// autostart / battery-saver controls. Each writes one engine config field;
// the engine's config watcher applies it.
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Pane {
    id: quickSettingsPanel
    property int labelColWidth: 132
    property int controlWidth: 320
    property int formColSpacing: 12
    property int formRowSpacing: 10

    // A QML control's own value binding is destroyed by the first user
    // edit, so re-assert every field when the backend reports a change.
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

    Layout.fillWidth: true
    Material.elevation: 1
    padding: 12

    ColumnLayout {
        anchors.fill: parent
        spacing: formRowSpacing

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
            columnSpacing: formColSpacing
            rowSpacing: formRowSpacing

            Label {
                text: qsTr("Isi layar:")
                Layout.preferredWidth: labelColWidth
            }

            ComboBox {
                id: fitCombo
                Layout.fillWidth: true
                Layout.maximumWidth: controlWidth
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
                Layout.preferredWidth: labelColWidth
            }

            ComboBox {
                id: monitorCombo
                Layout.fillWidth: true
                Layout.maximumWidth: controlWidth
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
