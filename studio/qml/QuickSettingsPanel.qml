// Tampilan quick panel: the Wallpaper right-rail pane with the fit / monitor
// choices only (brief B-WIREFRAME(a): Beranda shows the 3 icon "Isi layar"
// options; every other setting is defined once in SettingsPage.qml).
//
// Plan todo 15 (GAP-4): the old quick-settings pane duplicated the
// autostart / battery-saver / fit settings that also lived in Pengaturan.
// The duplication is gone - this pane keeps just the Tampilan choices, the
// definitions (and the raw values) live in SettingsPage.qml.
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

    // Same D2 mapping the Tampilan group uses (C-12 labels): "cover" is the
    // schema default alias of "fill", so both highlight "Penuh"; center or
    // any other raw value set in Lanjutan leaves the quick choices
    // unselected.
    readonly property string quickFitChoice: {
        const mode = Studio.quickFit
        if (mode === "fill" || mode === "cover")
            return "fill"
        if (mode === "fit" || mode === "stretch")
            return mode
        return ""
    }

    // A QML control's own value binding is destroyed by the first user edit,
    // so re-assert every field when the backend reports a change (same
    // contract the old panel used for all four controls).
    function syncQuickFit() {
        const sel = quickSettingsPanel.quickFitChoice
        if (fitPenuhRadio)
            fitPenuhRadio.checked = sel === "fill"
        if (fitPasRadio)
            fitPasRadio.checked = sel === "fit"
        if (fitIsiRadio)
            fitIsiRadio.checked = sel === "stretch"
        if (monitorCombo) {
            let mi = 0
            for (let i = 0; i < Studio.monitorChoices.length; ++i) {
                if (Studio.monitorChoices[i].id === Studio.quickMonitor) {
                    mi = i
                    break
                }
            }
            monitorCombo.currentIndex = mi
        }
    }

    Connections {
        target: Studio
        function onQuickSettingsChanged() {
            quickSettingsPanel.syncQuickFit()
        }
    }

    // Same two-bridge rule as SettingsPage: Studio writes config.json live,
    // SettingsBridge's copy is what the clean-quit flush writes - mirror the
    // choice so the exit save cannot clobber it.
    function setQuickFitChoice(mode) {
        Studio.setQuickFit(mode)
        Settings.setFitMode(mode)
    }

    function setQuickMonitorChoice(id) {
        Studio.setQuickMonitor(id)
        Settings.setMonitorId(id)
    }

    Layout.fillWidth: true
    Material.elevation: 1
    // Task 32: tightened from 12 to the 8 step (matches the StatusBar rail).
    padding: Theme.space2

    // Keyboard/a11y contract (plan todo 15): >= 40px, Accessible.name,
    // Enter on top of the native Space activation.
    component QuickFitRadio: RadioButton {
        implicitHeight: 40
        focusPolicy: Qt.StrongFocus
        Keys.onReturnPressed: {
            click()
            event.accepted = true
        }
        Keys.onEnterPressed: {
            click()
            event.accepted = true
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: formRowSpacing

        Label {
            text: qsTr("Tampilan")
            font.bold: true
        }

        Label {
            text: qsTr("Isi layar:")
            color: Theme.text
        }

        ButtonGroup { id: fitChoiceGroup }

        QuickFitRadio {
            id: fitPenuhRadio
            objectName: "quickFitPenuh"
            ButtonGroup.group: fitChoiceGroup
            Layout.fillWidth: true
            // D2/C-12 verbatim labels, with the shared Theme.glyph icon in
            // front.
            text: Theme.glyph.fitFill + "  " + qsTr("Penuh (memotong tepi)")
            Accessible.name: qsTr("Penuh (memotong tepi)")
            checked: quickSettingsPanel.quickFitChoice === "fill"
            onToggled: {
                if (checked)
                    quickSettingsPanel.setQuickFitChoice("fill")
            }
        }

        QuickFitRadio {
            id: fitPasRadio
            objectName: "quickFitPas"
            ButtonGroup.group: fitChoiceGroup
            Layout.fillWidth: true
            text: Theme.glyph.fitPad + "  " + qsTr("Pas (seluruh video terlihat)")
            Accessible.name: qsTr("Pas (seluruh video terlihat)")
            checked: quickSettingsPanel.quickFitChoice === "fit"
            onToggled: {
                if (checked)
                    quickSettingsPanel.setQuickFitChoice("fit")
            }
        }

        QuickFitRadio {
            id: fitIsiRadio
            objectName: "quickFitIsi"
            ButtonGroup.group: fitChoiceGroup
            Layout.fillWidth: true
            text: Theme.glyph.fitStretch + "  " + qsTr("Isi (mungkin melar)")
            Accessible.name: qsTr("Isi (mungkin melar)")
            checked: quickSettingsPanel.quickFitChoice === "stretch"
            onToggled: {
                if (checked)
                    quickSettingsPanel.setQuickFitChoice("stretch")
            }
        }

        Label {
            text: qsTr("Layar:")
            color: Theme.text
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
            onActivated: quickSettingsPanel.setQuickMonitorChoice(
                             Studio.monitorChoices[index].id)
            Accessible.name: qsTr("Layar")
        }
    }
}
