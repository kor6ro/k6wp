// Settings page (Pengaturan). Four groups per plan todo 15 / keputusan 11 /
// brief B-WIREFRAME(d): Umum / Tampilan / Hemat daya / Lanjutan.
//
// Copy is the literal brief copy deck:
//   * C-12 fit choices: "Penuh (memotong tepi)" / "Pas (seluruh video
//     terlihat)" / "Isi (mungkin melar)" - three icon options; the raw
//     Center/native value moves to Lanjutan (D2: Penuh=fill, Pas=fit,
//     Isi=stretch).
//   * C-13 performance radio + sublabel "Hemat = ringan & irit baterai";
//     the raw values stay in Lanjutan.
//   * C-17 updates: the Tentang + Pembaruan dialogs live in AppDialogs.qml;
//     this page only calls the entry points Main passes down.
//   * C-18 general wording, incl. the "(berlaku setelah restart)" language
//     note and the close-to-tray default-off toggle.
//
// One source of truth (GAP-4): the Beranda right rail (QuickSettingsPanel)
// shows only the Tampilan quick choices; every definition - including the
// raw values the performance preset rewrites - lives here. The old quick /
// basic settings duplication is gone.
//
// Consent stays in AppDialogs (todo 13): the manual compress block only
// reads Compress.* and never re-implements the C-9 consent dialog.
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
    // A few labels are longer than the shared 132px label column
    // ("Mode pengisian (mentah):", "Offset bingkai (detik):", ...); without
    // their own preferred width they overdraw into the control.
    readonly property int wideLabelWidth: 200

    // --- task 33: adaptive layout + wheel physics -------------------------
    // RECOMMENDED adaptive pattern: ONE reading column at every width. Below
    // the breakpoint it is full-bleed; at/above it the column stops growing
    // at contentMaxWidth and centres itself in the scroll pane, so a
    // fullscreen window gets balanced gutters instead of the old "empty on
    // the right" (pre-task-30 two-column) or a clipped right column (narrow).
    //
    // Why a capped+centred column rather than a two-column split at wide
    // widths: the four groups are wildly uneven - Lanjutan alone is roughly
    // as tall as the other three combined (~2000 px vs ~1000 px at the
    // default DPI) - so ANY static column assignment leaves one column
    // permanently short (dead space exactly like the reported emptiness) or
    // must re-balance on every collapse toggle, which moves groups under the
    // user's cursor mid-interaction. One capped column keeps the task-30
    // invariants intact: same reading order, same tab order, collapse
    // toggles only change the vertical flow. It is also the platform
    // precedent (Windows 11 Settings / macOS System Settings both bound the
    // settings column instead of stretching controls edge-to-edge).
    //
    // The 900 px number is measured from this page's content, not guessed:
    //   * widest fixed row is Lanjutan > "Perkecil manual" > "Folder output:":
    //     labelColWidth 132 + formColSpacing 12 + path label max 400
    //     + formColSpacing 12 + [Ganti folder... ~125 + 12 + Buka folder
    //     117] = ~810 px of preferred widths (task-24 UIA dump measures the
    //     path label at 400x19 and "Buka folder" at 117x40).
    //   * plus the 2 x 8 px page gutter = 826 and the 12 px scrollbar lane
    //     = 838 px; 900 leaves every ElideMiddle path label its full 400 px
    //     with ~60 px of slack and keeps helper text short.
    //   * below 900 the column tracks the pane, so the 804 px narrow proof
    //     stays full-width with nothing cut off.
    readonly property int contentMaxWidth: 900

    // One "clicky" wheel notch. Qt 6.6+ scrolls a notch by
    // QStyleHints::wheelScrollLines() * 24 px - 72 px on the default Windows
    // setting (Qt 6.8 qquickflickable.cpp wheelEvent, the
    // "wheelDeceleration > _q_MaximumWheelDeceleration" branch) and
    // flickDeceleration no longer applies to wheel input at all. 72 px per
    // notch reads as "heavy" on this ~3000 px page, so the WheelHandler in
    // the scroll container re-maps one notch to 120 px (~2.4 rows at the
    // 40 px target + 10 px row spacing); trackpads keep their native pixel
    // delta 1:1.
    readonly property int wheelStep: 120

    // The attached ScrollBar overlays the Flickable's right edge (Qt Quick
    // Controls does not reserve layout space for it). Reserve the same 12 px
    // hit lane VideoGrid.qml uses so a capped/centred column never slides
    // under the handle.
    readonly property int scrollBarLane: 12

    property var dialogOpened: function () {}
    property var dialogClosed: function () {}
    // Todo 15 wiring: the C-16 "Info teknis" dialog and the C-17 update flow
    // live in AppDialogs.qml. Main passes the entry points down so this page
    // never duplicates a dialog (todo 5 owns Info teknis, todo 13 the
    // consent dialogs).
    property var openInfoTeknis: function () {}
    property var checkUpdates: function () {}
    // C-17 "Tentang K6WP Studio". The B-FLOW menu removal left this dialog
    // without an opener, so Main wires appDialogs.openAbout into the Umum
    // button below. Without this the About dialog (version/license/support)
    // would be unreachable.
    property var openAbout: function () {}

    // Settings/logs are produced by three different backends, so all three
    // are merged into the one log view.
    property var allLog: Studio.log.concat(Compress.log, Settings.log)

    // Tampilan quick choice for the current config value. "cover" is the
    // schema default alias of "fill" (both scale to cover the screen), so
    // both land on "Penuh"; center / anything else means a raw value set in
    // Lanjutan and therefore no quick choice is highlighted.
    readonly property string quickFitChoice: {
        const mode = Studio.quickFit
        if (mode === "fill" || mode === "cover")
            return "fill"
        if (mode === "fit" || mode === "stretch")
            return mode
        return ""
    }

    // --- keyboard + a11y helpers (plan todo 15 MUST-DO) -------------------
    // Every toggle/radio/action is >= 40px, carries an explicit
    // Accessible.name and accepts Enter in addition to the native Space
    // activation. Space is handled by AbstractButton itself; Enter needs the
    // explicit path (repo pattern: AssignPopup.qml / ToastBar.qml).
    component SettingCheck: CheckBox {
        implicitHeight: 40
        focusPolicy: Qt.StrongFocus
        Accessible.name: text
        Keys.onReturnPressed: {
            toggle()
            event.accepted = true
        }
        Keys.onEnterPressed: {
            toggle()
            event.accepted = true
        }
    }

    component SettingRadio: RadioButton {
        implicitHeight: 40
        focusPolicy: Qt.StrongFocus
        Accessible.name: text
        Keys.onReturnPressed: {
            click()
            event.accepted = true
        }
        Keys.onEnterPressed: {
            click()
            event.accepted = true
        }
    }

    // Token-styled action button (task 31): replaces the stock Material
    // background so pressing never flashes the blue Material ripple, and
    // keyboard focus rings in ink (Theme.focusRing). A shared Theme.glyph
    // icon leads the label; the 40px target and Enter/Space contract are
    // unchanged.
    component SettingButton: Button {
        id: settingButton
        property string glyph: ""
        implicitHeight: 40
        focusPolicy: Qt.StrongFocus
        leftPadding: Theme.space2
        rightPadding: Theme.space2
        Accessible.name: text
        Keys.onReturnPressed: {
            settingButton.clicked()
            event.accepted = true
        }
        Keys.onEnterPressed: {
            settingButton.clicked()
            event.accepted = true
        }
        contentItem: RowLayout {
            spacing: Theme.space1

            Text {
                visible: settingButton.glyph.length > 0
                text: settingButton.glyph
                font.family: Theme.glyphFont
                font.pixelSize: Theme.fontM
                color: settingButton.enabled ? Theme.text : Theme.text2
                verticalAlignment: Text.AlignVCenter
                Accessible.ignored: true
            }

            Text {
                Layout.fillWidth: true
                text: settingButton.text
                font.pixelSize: Theme.fontM
                color: settingButton.enabled ? Theme.text : Theme.text2
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }
        }
        background: Rectangle {
            radius: Theme.radiusS
            color: settingButton.down ? Theme.pressedSurface
                   : (settingButton.hovered || settingButton.activeFocus
                      ? Theme.surface2 : Theme.surface)
            border.width: settingButton.activeFocus ? 2 : 0
            border.color: Theme.focusRing
            opacity: settingButton.enabled ? 1.0 : 0.6
        }
    }

    // Group chrome (brief GATE 0 #8: Windows-Settings style grouping). A
    // plain heading + hairline divider keeps the four groups readable in one
    // screenshot without inventing a card style the token table does not
    // define.
    component GroupTitle: Label {
        font.pixelSize: Theme.fontL
        font.weight: Theme.fontWeightSemibold
        color: Theme.text
    }

    component GroupDivider: Rectangle {
        Layout.fillWidth: true
        implicitHeight: 1
        color: Theme.surface2
    }

    component SubTitle: Label {
        font.weight: Theme.fontWeightSemibold
        color: Theme.text2
    }

    // Collapsible settings group (task 30). The old layout put Umum/Tampilan/
    // Hemat daya in one column and Lanjutan in a second; at a narrow window
    // the right column clipped. Every group now stacks in ONE vertical
    // column and each header collapses its body.
    //
    // Default state: EXPANDED for all four groups. That preserves the
    // pre-task behaviour (every control visible on entry, no setting hidden
    // behind a default-collapsed header) and still gives the user an explicit
    // way to shorten the long scroll at narrow widths.
    //
    // The header IS the toggle and stays reachable by keyboard/AT while the
    // body is hidden: it is a >= 40px Button with an explicit surface-fill
    // state (task 32: hover = surface2, focus/press = pressedSurface; NO
    // outline ring on this list header), Enter/Space activation (Space is
    // AbstractButton-native, Enter is explicit), a chevron indicator, and
    // Accessible.name = group title.
    // Qt 6.8's Accessible attached type has no expanded/expandable property,
    // so the collapsed/expanded state is exposed as checkable/checked plus a
    // plain-language description.
    component SettingsGroup: ColumnLayout {
        id: settingsGroup

        property string title: ""
        property bool expanded: true

        Layout.fillWidth: true
        spacing: settingsPage.formRowSpacing

        Button {
            id: groupHeader
            // Instance objectName + "Header", so every group's toggle is
            // uniquely addressable from QA/a11y tooling.
            objectName: settingsGroup.objectName + "Header"
            Layout.fillWidth: true
            implicitHeight: 40
            focusPolicy: Qt.StrongFocus
            text: settingsGroup.title
            Accessible.role: Accessible.Button
            Accessible.name: settingsGroup.title
            Accessible.checkable: true
            Accessible.checked: settingsGroup.expanded
            Accessible.description: settingsGroup.expanded
                                    ? qsTr("Bagian dibuka. Aktifkan untuk menutup.")
                                    : qsTr("Bagian ditutup. Aktifkan untuk membuka.")
            onClicked: settingsGroup.expanded = !settingsGroup.expanded
            Keys.onReturnPressed: {
                settingsGroup.expanded = !settingsGroup.expanded
                event.accepted = true
            }
            Keys.onEnterPressed: {
                settingsGroup.expanded = !settingsGroup.expanded
                event.accepted = true
            }

            contentItem: RowLayout {
                spacing: Theme.space2

                // Chevron: right when collapsed, down when expanded.
                Label {
                    Layout.alignment: Qt.AlignVCenter
                    text: settingsGroup.expanded ? Theme.glyph.chevronDown
                                                 : Theme.glyph.chevronRight
                    font.family: Theme.glyphFont
                    font.pixelSize: Theme.fontM
                    color: Theme.text2
                    // The header's own Accessible.name already carries the
                    // group title + state; the decorative pieces stay ignored.
                    Accessible.ignored: true
                }

                GroupTitle {
                    Layout.alignment: Qt.AlignVCenter
                    text: settingsGroup.title
                    Accessible.ignored: true
                }

                Item {
                    Layout.fillWidth: true
                }
            }

            background: Rectangle {
                radius: Theme.radiusS
                // Task 32: list-header states are surface fills only - hover
                // surface2, press/keyboard focus pressedSurface, no ring.
                color: groupHeader.down || groupHeader.activeFocus
                       ? Theme.pressedSurface
                       : (groupHeader.hovered ? Theme.surface2 : Theme.surface)
            }
        }

        GroupDivider {}

        ColumnLayout {
            id: groupBody
            Layout.fillWidth: true
            spacing: settingsPage.formRowSpacing
            visible: settingsGroup.expanded
        }

        // Controls written inside each SettingsGroup instance are re-parented
        // into groupBody, so they keep their ids in this file scope and stay
        // bound to the same Settings/Studio/Compress properties.
        default property alias content: groupBody.data
    }

    // A QML control's own value binding is destroyed by the first user edit,
    // so re-assert every field when the backend reports a change (same
    // contract as the old QuickSettingsPanel).
    function syncFromSettings() {
        if (languageCombo) {
            let li = 0
            for (let i = 0; i < languageCombo.model.length; ++i) {
                if (languageCombo.model[i].code === Settings.language) {
                    li = i
                    break
                }
            }
            languageCombo.currentIndex = li
        }
        if (updateCheckBox)
            updateCheckBox.checked = Settings.checkUpdates
        if (autostartBox)
            autostartBox.checked = Settings.startWithWindows
        if (trayBox)
            trayBox.checked = Settings.closeToTray
        if (batteryBox)
            batteryBox.checked = Settings.batterySaver
        if (batteryModeCombo)
            batteryModeCombo.currentIndex =
                    Math.max(0, batteryModeCombo.modes.indexOf(Settings.batteryMode))
        if (presetHematRadio) {
            presetHematRadio.checked = Settings.performancePreset === "Hemat"
            presetSeimbangRadio.checked = Settings.performancePreset === "Seimbang"
            presetMaksimalRadio.checked = Settings.performancePreset === "Maksimal"
        }
        if (rawFitCombo) {
            // "fill" (what the Tampilan quick choice writes) and "cover" are
            // the same engine behaviour; the raw combo models it as cover.
            const v = Settings.fitMode === "fill" ? "cover" : Settings.fitMode
            rawFitCombo.currentIndex = Math.max(0, rawFitCombo.modes.indexOf(v))
        }
        if (rawFpsBox)
            rawFpsBox.value = Settings.fpsCap
        if (rawCrfBox)
            rawCrfBox.value = Settings.crf
        if (affinityCombo)
            affinityCombo.currentIndex =
                    Math.max(0, affinityCombo.modes.indexOf(Settings.cpuAffinity))
        if (gpuCombo)
            gpuCombo.currentIndex =
                    Math.max(0, gpuCombo.modes.indexOf(Settings.gpuAdapter))
        if (offsetBox)
            offsetBox.value = Math.round(Settings.lockscreenOffsetSec)
        if (lockscreenSyncBox)
            lockscreenSyncBox.checked = Settings.lockscreenSync
        if (autoCompressBox)
            autoCompressBox.checked = Settings.autoCompressOnImport
        if (defaultCrfBox)
            defaultCrfBox.value = Settings.defaultCrf
        if (defaultFpsBox)
            defaultFpsBox.value = Settings.defaultFps
        if (resolutionCombo)
            resolutionCombo.currentIndex =
                    Math.max(0, resolutionCombo.modes.indexOf(Settings.defaultResolutionMode))
    }

    function syncQuickFit() {
        const sel = settingsPage.quickFitChoice
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

    function syncCompressInputs() {
        if (jobCrfBox)
            jobCrfBox.value = Compress.crf
        if (jobFpsBox)
            jobFpsBox.value = Compress.fps
        if (jobEncoderCombo)
            jobEncoderCombo.currentIndex =
                    Math.max(0, jobEncoderCombo.model.indexOf(Compress.encoder))
    }

    // Tampilan quick choices are written through Studio (config.json takes
    // effect live) AND mirrored into Settings. Both bridges keep their own
    // config.json copy, and SettingsBridge's copy is what its clean-quit
    // flush writes; without the mirror a quick change would be clobbered on
    // exit by the stale Settings copy (same rule for the monitor target).
    function setQuickFitChoice(mode) {
        Studio.setQuickFit(mode)
        Settings.setFitMode(mode)
    }

    function setQuickMonitorChoice(id) {
        Studio.setQuickMonitor(id)
        Settings.setMonitorId(id)
    }

    Connections {
        target: Settings
        function onChanged() {
            settingsPage.syncFromSettings()
        }
        // Todo 14's preset restart path: a preset that touched CPU/GPU needs
        // the wallpaper reloaded before it takes effect. The notice follows
        // glossary §5 - never the word "engine".
        function onPerformancePresetApplied(needsRestart) {
            if (!needsRestart)
                return
            performanceNotice.visible = true
            performanceNoticeTimer.restart()
            Studio.startEngine()
        }
    }

    Connections {
        target: Studio
        function onQuickSettingsChanged() {
            settingsPage.syncQuickFit()
        }
    }

    // The manual compress inputs can also move from outside this page
    // (Compress.pickSource, refreshDefaults, the 1080p pin in
    // startCompressFirst), so re-assert the editable job fields exactly like
    // the old widget root's onInputsChanged handler did - plan todo 15 moved
    // that handler here.
    Connections {
        target: Compress
        function onInputsChanged() {
            settingsPage.syncCompressInputs()
        }
    }

    // Entering the page must show the current files, not what was read at
    // startup: the Beranda rail and these raw controls write the same config,
    // so re-read the quick settings on entry.
    onVisibleChanged: {
        if (visible) {
            Studio.refreshQuickSettings()
            settingsPage.syncFromSettings()
        }
    }

    Timer {
        id: performanceNoticeTimer
        interval: 6000
        repeat: false
        onTriggered: performanceNotice.visible = false
    }

    // --- scroll container (task 33: explicit Flickable, was a ScrollView) --
    // An explicit Flickable exposes every physics knob the owner asked for:
    //
    //   * Wheel step: the WheelHandler below re-maps one notch to
    //     settingsPage.wheelStep (see the constant comment above; Qt's
    //     default is 72 px/notch on Windows). blocking:true is REQUIRED: a
    //     non-blocking handler still lets QQuickFlickable::wheelEvent run
    //     its own 72 px path for the same event (double scroll), because a
    //     WheelHandler only accepts the event point when blocking is set
    //     (Qt 6.8 qquickwheelhandler.cpp handleEventPoint).
    //   * Flick velocity/decay (drag + touch flings; wheel goes through the
    //     handler above): maximumFlickVelocity 2500 -> 4000 so a fast fling
    //     is not clamped, flickDeceleration 1500 -> 1000 so it glides longer
    //     before stopping.
    //   * pixelAligned:false removes whole-pixel rounding of contentY, so
    //     motion stays smooth on fractional-DPI displays.
    //   * ScrollBar responsiveness: always-on 12 px, token-styled exactly
    //     like the gallery scrollbar (VideoGrid.qml task 28/31), so it is
    //     always grabbable and its handle tracks the wheel immediately
    //     (contentY is the single source of truth).
    Flickable {
        id: settingsFlick
        objectName: "settingsFlick"
        anchors.fill: parent
        anchors.margins: Theme.space2
        clip: true
        contentWidth: width
        contentHeight: contentColumn.implicitHeight
        boundsBehavior: Flickable.StopAtBounds
        pixelAligned: false
        maximumFlickVelocity: 4000
        flickDeceleration: 1000

        ScrollBar.vertical: ScrollBar {
            id: settingsScrollBar
            objectName: "settingsScrollBar"
            policy: ScrollBar.AlwaysOn
            implicitWidth: 12
            background: Rectangle {
                color: Theme.surface2
                radius: Theme.radiusS
            }
            contentItem: Rectangle {
                implicitWidth: 8
                implicitHeight: 32
                radius: Theme.radiusS
                // Pressed reads as full ink, never a blue flash (task 31).
                color: settingsScrollBar.pressed ? Theme.text : Theme.text2
            }
        }

        WheelHandler {
            id: wheelScroll
            objectName: "settingsWheelScroll"
            acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
            blocking: true
            onWheel: (event) => {
                // Trackpads keep their native pixel delta; a clicky wheel
                // (no pixel delta, 120-unit angle steps) is re-mapped to the
                // 120 px notch. Both deltas share Qt's sign convention:
                // positive = wheel away from user = scroll up = contentY
                // decreases.
                const delta = event.pixelDelta.y !== 0
                            ? event.pixelDelta.y
                            : event.angleDelta.y / 120 * settingsPage.wheelStep
                if (delta === 0)
                    return
                const maxY = Math.max(0, settingsFlick.contentHeight - settingsFlick.height)
                settingsFlick.contentY = Math.max(0, Math.min(maxY, settingsFlick.contentY - delta))
            }
        }

        // ONE vertical column (task 30). The old left/right RowLayout turned
        // narrow windows into a clipped right column; the four groups now
        // stack in reading order and each one collapses, so the page scrolls
        // vertically instead of cutting controls off. Task 32 tightened the
        // page gutter to the 8 step and the group gap follows Theme.space4.
        // Task 33 makes the column adaptive: full-bleed below the 900 px
        // breakpoint (minus the 12 px scrollbar lane), capped and centred
        // above it (see contentMaxWidth above for the measurement).
        ColumnLayout {
            id: contentColumn
            width: Math.min(settingsFlick.width - settingsPage.scrollBarLane,
                            settingsPage.contentMaxWidth)
            x: Math.round((settingsFlick.width - settingsPage.scrollBarLane - width) / 2)
            spacing: Theme.space4

            // --------------------------------- UMUM -----------------------
            // C-18: language + restart note, start with Windows, close to
            // tray (default off), plus the update auto-check checkbox and
            // the C-17 "Cek pembaruan" button that replaced the old
            // English update-check menu item.
            SettingsGroup {
                id: umumGroup
                Layout.fillWidth: true
                title: qsTr("Umum")

                GridLayout {
                    Layout.fillWidth: true
                    columns: 2
                    columnSpacing: settingsPage.formColSpacing
                    rowSpacing: settingsPage.formRowSpacing

                    Label {
                        text: qsTr("Bahasa antarmuka:")
                        Layout.preferredWidth: settingsPage.labelColWidth
                    }

                    RowLayout {
                        spacing: Theme.space2

                        ComboBox {
                            id: languageCombo
                            Layout.preferredWidth: settingsPage.narrowControlWidth
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
                            Accessible.name: qsTr("Bahasa antarmuka")
                        }

                        // C-18 note: a QTranslator only loads at startup.
                        Label {
                            text: qsTr("(berlaku setelah restart)")
                            color: Theme.text2
                            font.pixelSize: Theme.fontS
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: settingsPage.formColSpacing

                    SettingCheck {
                        id: updateCheckBox
                        Layout.fillWidth: true
                        text: qsTr("Periksa pembaruan saat Studio dimulai")
                        checked: Settings.checkUpdates
                        onToggled: Settings.setCheckUpdates(checked)
                    }

                    SettingButton {
                        objectName: "checkUpdatesButton"
                        text: qsTr("Cek pembaruan")
                        glyph: Theme.glyph.refresh
                        enabled: !Studio.updateCheckBusy
                        onClicked: settingsPage.checkUpdates()
                    }
                }

                // C-17 Tentang opener. Replaces the deleted top menu's only
                // "Tentang K6WP Studio" trigger (brief B-FLOW); the dialog
                // itself stays in AppDialogs and keeps hosting [Cek
                // pembaruan] + the support links.
                RowLayout {
                    Layout.fillWidth: true
                    spacing: settingsPage.formColSpacing

                    SettingButton {
                        objectName: "aboutButton"
                        text: qsTr("Tentang K6WP Studio")
                        glyph: Theme.glyph.info
                        onClicked: settingsPage.openAbout()
                    }

                    Item {
                        Layout.fillWidth: true
                    }
                }

                SettingCheck {
                    id: autostartBox
                    Layout.fillWidth: true
                    text: qsTr("Jalankan saat Windows menyala")
                    checked: Settings.startWithWindows
                    onToggled: Settings.setStartWithWindows(checked)
                }

                SettingCheck {
                    id: trayBox
                    objectName: "closeToTrayBox"
                    Layout.fillWidth: true
                    // C-18 verbatim: "Tutup ke tray (Studio tetap jalan di
                    // latar)", default OFF.
                    text: qsTr("Tutup ke tray (Studio tetap jalan di latar)")
                    checked: Settings.closeToTray
                    // Todo 17 (B10): the C++ property + tray lifecycle have
                    // landed, so this is a normal write-through toggle.
                    onToggled: Settings.setCloseToTray(checked)
                }

            }

            // ----------------------------- TAMPILAN ------------------------
            // C-12/D2: three icon choices; Center/raw moves to Lanjutan.
            // quickFit mirrors config.json live (engine config watcher).
            SettingsGroup {
                id: tampilanGroup
                Layout.fillWidth: true
                title: qsTr("Tampilan")

                Label {
                    text: qsTr("Isi layar:")
                    color: Theme.text
                }

                ButtonGroup { id: fitChoiceGroup }

                SettingRadio {
                    id: fitPenuhRadio
                    objectName: "fitPenuh"
                    ButtonGroup.group: fitChoiceGroup
                    text: Theme.glyph.fitFill + "  " + qsTr("Penuh (memotong tepi)")
                    Accessible.name: qsTr("Penuh (memotong tepi)")
                    checked: settingsPage.quickFitChoice === "fill"
                    onToggled: {
                        if (checked)
                            settingsPage.setQuickFitChoice("fill")
                    }
                }

                SettingRadio {
                    id: fitPasRadio
                    objectName: "fitPas"
                    ButtonGroup.group: fitChoiceGroup
                    text: Theme.glyph.fitPad + "  " + qsTr("Pas (seluruh video terlihat)")
                    Accessible.name: qsTr("Pas (seluruh video terlihat)")
                    checked: settingsPage.quickFitChoice === "fit"
                    onToggled: {
                        if (checked)
                            settingsPage.setQuickFitChoice("fit")
                    }
                }

                SettingRadio {
                    id: fitIsiRadio
                    objectName: "fitIsi"
                    ButtonGroup.group: fitChoiceGroup
                    text: Theme.glyph.fitStretch + "  " + qsTr("Isi (mungkin melar)")
                    Accessible.name: qsTr("Isi (mungkin melar)")
                    checked: settingsPage.quickFitChoice === "stretch"
                    onToggled: {
                        if (checked)
                            settingsPage.setQuickFitChoice("stretch")
                    }
                }

                GridLayout {
                    Layout.fillWidth: true
                    columns: 2
                    columnSpacing: settingsPage.formColSpacing
                    rowSpacing: settingsPage.formRowSpacing

                    Label {
                        text: qsTr("Layar:")
                        Layout.preferredWidth: settingsPage.labelColWidth
                    }

                    ComboBox {
                        id: monitorCombo
                        Layout.fillWidth: true
                        Layout.maximumWidth: settingsPage.controlWidth
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
                        onActivated: settingsPage.setQuickMonitorChoice(
                                         Studio.monitorChoices[index].id)
                        Accessible.name: qsTr("Layar")
                    }
                }

            }

            // ---------------------------- HEMAT DAYA -----------------------
            // C-13: battery saver switch + mode, and the performance
            // radio (Hemat/Seimbang/Maksimal -> Settings.performancePreset,
            // todo 14). The raw values live in Lanjutan.
            SettingsGroup {
                id: hematDayaGroup
                Layout.fillWidth: true
                title: qsTr("Hemat daya")

                SettingCheck {
                    id: batteryBox
                    Layout.fillWidth: true
                    text: qsTr("Hemat baterai (wallpaper berhenti saat pakai baterai)")
                    checked: Settings.batterySaver
                    onToggled: Settings.setBatterySaver(checked)
                }

                GridLayout {
                    Layout.fillWidth: true
                    columns: 2
                    columnSpacing: settingsPage.formColSpacing
                    rowSpacing: settingsPage.formRowSpacing

                    Label {
                        text: qsTr("Mode hemat baterai:")
                        Layout.preferredWidth: settingsPage.wideLabelWidth
                    }

                    ComboBox {
                        id: batteryModeCombo
                        Layout.fillWidth: true
                        Layout.maximumWidth: settingsPage.controlWidth
                        model: [qsTr("Batasi 24 fps (hemat)"),
                                qsTr("Jeda penuh saat baterai (paling hemat)")]
                        readonly property var modes: ["cap24", "static"]
                        currentIndex: Math.max(0, modes.indexOf(Settings.batteryMode))
                        onActivated: Settings.setBatteryMode(modes[index])
                        Accessible.name: qsTr("Mode hemat baterai")
                    }
                }

                Label {
                    text: qsTr("Performa:")
                    color: Theme.text
                }

                ButtonGroup { id: presetGroup }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: settingsPage.formColSpacing

                    SettingRadio {
                        id: presetHematRadio
                        objectName: "presetHemat"
                        ButtonGroup.group: presetGroup
                        text: qsTr("Hemat")
                        checked: Settings.performancePreset === "Hemat"
                        onToggled: {
                            if (checked)
                                Settings.setPerformancePreset("Hemat")
                        }
                    }

                    SettingRadio {
                        id: presetSeimbangRadio
                        objectName: "presetSeimbang"
                        ButtonGroup.group: presetGroup
                        text: qsTr("Seimbang")
                        checked: Settings.performancePreset === "Seimbang"
                        onToggled: {
                            if (checked)
                                Settings.setPerformancePreset("Seimbang")
                        }
                    }

                    SettingRadio {
                        id: presetMaksimalRadio
                        objectName: "presetMaksimal"
                        ButtonGroup.group: presetGroup
                        text: qsTr("Maksimal")
                        checked: Settings.performancePreset === "Maksimal"
                        onToggled: {
                            if (checked)
                                Settings.setPerformancePreset("Maksimal")
                        }
                    }
                }

                // C-13 verbatim sublabel.
                Label {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    text: qsTr("Hemat = ringan & irit baterai")
                    color: Theme.text2
                    font.pixelSize: Theme.fontS
                }

                Label {
                    id: performanceNotice
                    objectName: "performanceNotice"
                    Layout.fillWidth: true
                    visible: false
                    wrapMode: Text.WordWrap
                    color: Theme.accent
                    font.pixelSize: Theme.fontS
                    text: qsTr("Menerapkan performa…")
                }
            }

            // ----------------------------- LANJUTAN ------------------------
            // Raw values, manual compress, import defaults, cache, and
            // diagnostics: the last group of the single column.
            SettingsGroup {
                id: lanjutanGroup
                Layout.fillWidth: true
                title: qsTr("Lanjutan")

                // --- raw playback values ---------------------------------------
                // C-13: "Nilai mentah tetap di Lanjutan". These are the fields
                // the performance preset rewrites (todo 14); D2 also parks the
                // raw Center/native fit here.
                SubTitle { text: qsTr("Nilai mentah") }

                GridLayout {
                    Layout.fillWidth: true
                    columns: 2
                    columnSpacing: settingsPage.formColSpacing
                    rowSpacing: settingsPage.formRowSpacing

                    Label {
                        text: qsTr("Mode pengisian (mentah):")
                        Layout.preferredWidth: settingsPage.wideLabelWidth
                    }

                    ComboBox {
                        id: rawFitCombo
                        objectName: "rawFitCombo"
                        Layout.fillWidth: true
                        Layout.maximumWidth: settingsPage.controlWidth
                        model: [qsTr("Isi layar (potong bila perlu)"),
                                qsTr("Sesuaikan (seluruh video terlihat)"),
                                qsTr("Regang (isi penuh)"),
                                qsTr("Tengah (ukuran asli)")]
                        // Index order must match the fit_mode enum in
                        // shared/config_schema.cpp; "fill" (what the Tampilan
                        // quick choice writes) and "cover" behave identically,
                        // so this raw combo normalises them onto cover.
                        readonly property var modes: ["cover", "fit",
                                                       "stretch", "center"]
                        currentIndex: {
                            const v = Settings.fitMode === "fill" ? "cover" : Settings.fitMode
                            const i = modes.indexOf(v)
                            return i >= 0 ? i : 0
                        }
                        onActivated: {
                            Settings.setFitMode(modes[index])
                            // Keep the Tampilan quick choices mirrored.
                            Studio.refreshQuickSettings()
                        }
                        Accessible.name: qsTr("Mode pengisian (mentah)")
                    }

                    Label {
                        text: qsTr("Batas FPS:")
                        Layout.preferredWidth: settingsPage.labelColWidth
                    }

                    SpinBox {
                        id: rawFpsBox
                        Layout.preferredWidth: settingsPage.spinWidth
                        from: 1
                        to: 30
                        editable: true
                        value: Settings.fpsCap
                        onValueModified: Settings.setFpsCap(value)
                        Accessible.name: qsTr("Batas FPS")
                    }

                    Label {
                        text: qsTr("CRF:")
                        Layout.preferredWidth: settingsPage.labelColWidth
                    }

                    SpinBox {
                        id: rawCrfBox
                        Layout.preferredWidth: settingsPage.spinWidth
                        from: 16
                        to: 28
                        editable: true
                        value: Settings.crf
                        onValueModified: Settings.setCrf(value)
                        Accessible.name: qsTr("CRF")
                    }

                    Label {
                        text: qsTr("Inti CPU:")
                        Layout.preferredWidth: settingsPage.labelColWidth
                    }

                    ComboBox {
                        id: affinityCombo
                        Layout.fillWidth: true
                        Layout.maximumWidth: settingsPage.controlWidth
                        model: [qsTr("Otomatis (inti efisiensi/E-core)"),
                                qsTr("Semua inti")]
                        readonly property var modes: ["auto", "all"]
                        currentIndex: Math.max(0, modes.indexOf(Settings.cpuAffinity))
                        onActivated: Settings.setCpuAffinity(modes[index])
                        Accessible.name: qsTr("Inti CPU")
                    }

                    Label {
                        text: qsTr("GPU:")
                        Layout.preferredWidth: settingsPage.labelColWidth
                    }

                    ComboBox {
                        id: gpuCombo
                        Layout.fillWidth: true
                        Layout.maximumWidth: settingsPage.controlWidth
                        model: [qsTr("Otomatis"), qsTr("Terintegrasi (hemat daya)"),
                                qsTr("Diskrit (performa)")]
                        readonly property var modes: ["auto", "integrated", "discrete"]
                        currentIndex: Math.max(0, modes.indexOf(Settings.gpuAdapter))
                        onActivated: Settings.setGpuAdapter(modes[index])
                        Accessible.name: qsTr("GPU")
                    }

                    Label {
                        text: qsTr("Offset bingkai (detik):")
                        Layout.preferredWidth: settingsPage.wideLabelWidth
                    }

                    SpinBox {
                        id: offsetBox
                        Layout.preferredWidth: settingsPage.spinWidth
                        from: 0
                        to: 30
                        editable: true
                        value: Math.round(Settings.lockscreenOffsetSec)
                        onValueModified: Settings.setLockscreenOffsetSec(value)
                        Accessible.name: qsTr("Offset bingkai (detik)")
                    }
                }

                SettingCheck {
                    id: lockscreenSyncBox
                    Layout.fillWidth: true
                    text: qsTr("Samakan bingkai video ke layar kunci (gambar statis)")
                    checked: Settings.lockscreenSync
                    onToggled: Settings.setLockscreenSync(checked)
                }

                // Raw affinity/GPU only reach the renderer on a fresh engine
                // start; the action is the glossary §5 recovery phrase (never
                // "Restart engine").
                RowLayout {
                    Layout.fillWidth: true
                    spacing: settingsPage.formColSpacing

                    Label {
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        visible: Settings.engineRestartNeeded
                        color: Theme.text2
                        font.pixelSize: Theme.fontS
                        text: qsTr("Perubahan inti CPU / GPU berlaku setelah wallpaper dimuat ulang.")
                    }

                    SettingButton {
                        objectName: "reloadWallpaperButton"
                        visible: Settings.engineRestartNeeded
                        text: qsTr("Muat ulang wallpaper")
                        glyph: Theme.glyph.refresh
                        onClicked: {
                            Settings.apply()
                            Studio.startEngine()
                        }
                    }
                }

                // --- manual compress -------------------------------------------
                // Compress.* only: the C-9 consent dialog and the C-8 offer
                // stay in AppDialogs (todo 13), and the progress surfaces in
                // ToastBar; this block is the "kompres manual" entry point.
                SubTitle { text: qsTr("Perkecil manual") }

                GridLayout {
                    Layout.fillWidth: true
                    columns: 3
                    columnSpacing: settingsPage.formColSpacing
                    rowSpacing: settingsPage.formRowSpacing

                    Label {
                        text: qsTr("Sumber:")
                        Layout.preferredWidth: settingsPage.labelColWidth
                    }

                    Label {
                        Layout.fillWidth: true
                        Layout.maximumWidth: 400
                        elide: Text.ElideMiddle
                        opacity: 0.8
                        text: Compress.sourcePath.length > 0
                              ? Compress.sourcePath
                              : qsTr("(belum dipilih — klik Jelajahi)")
                    }

                    SettingButton {
                        objectName: "manualCompressBrowse"
                        text: qsTr("Jelajahi...")
                        glyph: Theme.glyph.external
                        onClicked: Compress.pickSource()
                    }

                    Label {
                        text: qsTr("Folder output:")
                        Layout.preferredWidth: settingsPage.labelColWidth
                    }

                    Label {
                        Layout.fillWidth: true
                        Layout.maximumWidth: 400
                        elide: Text.ElideMiddle
                        opacity: 0.8
                        text: Compress.outDir
                    }

                    RowLayout {
                        spacing: settingsPage.formColSpacing

                        SettingButton {
                            text: qsTr("Ganti folder...")
                            glyph: Theme.glyph.external
                            onClicked: Compress.pickOutDir()
                        }

                        SettingButton {
                            text: qsTr("Buka folder")
                            glyph: Theme.glyph.external
                            onClicked: Compress.openOutDir()
                        }
                    }
                }

                GridLayout {
                    Layout.fillWidth: true
                    columns: 2
                    columnSpacing: settingsPage.formColSpacing
                    rowSpacing: settingsPage.formRowSpacing

                    Label {
                        text: qsTr("CRF:")
                        Layout.preferredWidth: settingsPage.labelColWidth
                    }

                    SpinBox {
                        id: jobCrfBox
                        objectName: "jobCrfBox"
                        Layout.preferredWidth: settingsPage.spinWidth
                        from: 16
                        to: 28
                        editable: true
                        value: Compress.crf
                        onValueModified: Compress.setCrf(value)
                        Accessible.name: qsTr("CRF job kompres")
                    }

                    Label {
                        text: qsTr("FPS:")
                        Layout.preferredWidth: settingsPage.labelColWidth
                    }

                    SpinBox {
                        id: jobFpsBox
                        objectName: "jobFpsBox"
                        Layout.preferredWidth: settingsPage.spinWidth
                        from: 1
                        to: 30
                        editable: true
                        value: Compress.fps
                        onValueModified: Compress.setFps(value)
                        Accessible.name: qsTr("FPS job kompres")
                    }

                    Label {
                        text: qsTr("Encoder:")
                        Layout.preferredWidth: settingsPage.labelColWidth
                    }

                    ComboBox {
                        id: jobEncoderCombo
                        objectName: "jobEncoderCombo"
                        Layout.fillWidth: true
                        Layout.maximumWidth: settingsPage.controlWidth
                        model: ["auto", "nvenc", "qsv", "amf", "x264"]
                        currentIndex: Math.max(0, model.indexOf(Compress.encoder))
                        onActivated: Compress.setEncoder(model[index])
                        Accessible.name: qsTr("Encoder")
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: settingsPage.formColSpacing

                    SettingButton {
                        objectName: "manualCompressStart"
                        text: qsTr("Perkecil")
                        glyph: Theme.glyph.shrink
                        enabled: Compress.sourcePath.length > 0 && !Compress.running
                        onClicked: Compress.start()
                    }

                    SettingButton {
                        objectName: "manualCompressCancel"
                        text: qsTr("Batal")
                        glyph: Theme.glyph.close
                        enabled: Compress.running
                        onClicked: Compress.cancel()
                    }
                }

                // --- import defaults -------------------------------------------
                // C-2 sends "Siapkan video otomatis" to Pengaturan > Lanjutan;
                // the defaults feed CompressBridge::refreshDefaults().
                SubTitle { text: qsTr("Bawaan impor") }

                SettingCheck {
                    id: autoCompressBox
                    Layout.fillWidth: true
                    text: qsTr("Siapkan video otomatis")
                    checked: Settings.autoCompressOnImport
                    onToggled: Settings.setAutoCompressOnImport(checked)
                }

                GridLayout {
                    Layout.fillWidth: true
                    columns: 3
                    columnSpacing: settingsPage.formColSpacing
                    rowSpacing: settingsPage.formRowSpacing

                    Label {
                        text: qsTr("Folder output bawaan:")
                        Layout.preferredWidth: settingsPage.wideLabelWidth
                    }

                    Label {
                        Layout.fillWidth: true
                        Layout.maximumWidth: 400
                        elide: Text.ElideMiddle
                        opacity: 0.8
                        text: Settings.compressOutputDir
                    }

                    SettingButton {
                        text: qsTr("Ubah...")
                        glyph: Theme.glyph.external
                        onClicked: Settings.pickCompressOutputDir()
                    }
                }

                GridLayout {
                    Layout.fillWidth: true
                    columns: 2
                    columnSpacing: settingsPage.formColSpacing
                    rowSpacing: settingsPage.formRowSpacing

                    Label {
                        text: qsTr("CRF bawaan:")
                        Layout.preferredWidth: settingsPage.labelColWidth
                    }

                    SpinBox {
                        id: defaultCrfBox
                        Layout.preferredWidth: settingsPage.spinWidth
                        from: 16
                        to: 28
                        editable: true
                        value: Settings.defaultCrf
                        onValueModified: Settings.setDefaultCrf(value)
                        Accessible.name: qsTr("CRF bawaan")
                    }

                    Label {
                        text: qsTr("FPS bawaan:")
                        Layout.preferredWidth: settingsPage.labelColWidth
                    }

                    SpinBox {
                        id: defaultFpsBox
                        Layout.preferredWidth: settingsPage.spinWidth
                        from: 1
                        to: 30
                        editable: true
                        value: Settings.defaultFps
                        onValueModified: Settings.setDefaultFps(value)
                        Accessible.name: qsTr("FPS bawaan")
                    }

                    Label {
                        text: qsTr("Resolusi bawaan:")
                        Layout.preferredWidth: settingsPage.labelColWidth
                    }

                    ComboBox {
                        id: resolutionCombo
                        Layout.fillWidth: true
                        Layout.maximumWidth: settingsPage.narrowControlWidth
                        model: [qsTr("Ikuti layar"), qsTr("Ikuti sumber"),
                                qsTr("720p"), qsTr("1080p"), qsTr("2160p")]
                        readonly property var modes: ["match_monitor", "source",
                                                       "720p", "1080p", "2160p"]
                        currentIndex: {
                            const i = modes.indexOf(Settings.defaultResolutionMode)
                            return i >= 0 ? i : 0
                        }
                        onActivated: Settings.setDefaultResolutionMode(modes[index])
                        Accessible.name: qsTr("Resolusi bawaan")
                    }
                }

                // --- cache ------------------------------------------------------
                SubTitle { text: qsTr("Cache") }

                GridLayout {
                    Layout.fillWidth: true
                    columns: 3
                    columnSpacing: settingsPage.formColSpacing
                    rowSpacing: settingsPage.formRowSpacing

                    Label {
                        text: qsTr("Cache:")
                        Layout.preferredWidth: settingsPage.labelColWidth
                    }

                    Label {
                        Layout.fillWidth: true
                        Layout.maximumWidth: 320
                        elide: Text.ElideMiddle
                        opacity: 0.8
                        text: Settings.cacheDir
                    }

                    RowLayout {
                        spacing: settingsPage.formColSpacing

                        SettingButton {
                            text: qsTr("Ubah...")
                            glyph: Theme.glyph.external
                            onClicked: Settings.pickCacheDir()
                        }

                        SettingButton {
                            text: qsTr("Bersihkan cache...")
                            glyph: Theme.glyph.deleteFile
                            onClicked: cacheDialog.open()
                        }
                    }
                }

                // --- diagnostics -------------------------------------------------
                SubTitle { text: qsTr("Diagnostik") }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: settingsPage.formColSpacing

                    SettingButton {
                        objectName: "infoTeknisButton"
                        text: qsTr("Info teknis")
                        glyph: Theme.glyph.info
                        onClicked: settingsPage.openInfoTeknis()
                    }

                    Item {
                        Layout.fillWidth: true
                    }
                }

                Label {
                    Layout.fillWidth: true
                    visible: Settings.lastError.length > 0
                    wrapMode: Text.WordWrap
                    color: Theme.statusError
                    text: Settings.lastError
                }

                LogPanel {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 160
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
